#!/usr/bin/env python
#
# Read doxygen xml files to find all members of the dolfin
# name space and generate API doc files per subdirectory of
# dolfin
#
# Written by Tormod Landet, 2017
#
from __future__ import print_function
import sys, os
import parse_doxygen


DOXYGEN_XML_DIR = 'doxygen/xml'
API_GEN_DIR = 'generated_rst_files'
SWIG_DIR = '../dolfin/swig/'
SWIG_FILE = 'docstrings.i'
MOCK_PY = 'mock_cpp_modules.py'


def get_subdir(hpp_file_name):
    """
    Return "subdir" for a path name like
        /path/to/dolfin/subdir/a_header.h
    """
    path_components = hpp_file_name.split(os.sep)
    path_components_rev = path_components[::-1]
    idx = path_components_rev.index('dolfin')
    subdir = path_components_rev[idx - 1]
    return subdir


def get_short_path(hpp_file_name):
    """
    Return "dolfin/subdir/a_header.h" for a path name like
        /path/to/dolfin/subdir/a_header.h
    """
    path_components = hpp_file_name.split(os.sep)
    path_components_rev = path_components[::-1]
    idx = path_components_rev.index('dolfin')
    short_path = path_components_rev[:idx + 1]
    return os.sep.join(short_path[::-1])


def write_rst(subdir, subdir_members, api_gen_dir):
    """
    Write files for Sphinx C++ API documentation
    """
    rst_name = os.path.join(api_gen_dir, 'api_gen_%s.rst' % subdir)
    print('Generating', rst_name)
    
    # Make output directory
    if not os.path.isdir(api_gen_dir):
        os.mkdir(api_gen_dir)

    prev_short_name = ''
    with open(rst_name, 'wt') as rst:
        rst.write('.. automatically generated by generate_api_rst.py and parse_doxygen.py\n')
        #rst.write('dolfin/%s\n%s' % (subdir, '=' * 80))
        #rst.write('\nDocumentation for C++ code found in dolfin/%s/*.h\n\n' % subdir)
        rst.write('\n.. contents::\n\n\n')
        
        kinds = [('typedef', 'Type definitions', 'doxygentypedef'),
                 ('enum', 'Enumerations', 'doxygenenum'),
                 ('function', 'Functions', 'doxygenfunction'),
                 ('struct', 'Structures', 'doxygenstruct'),
                 ('variable', 'Variables', 'doxygenvariable'),
                 ('class', 'Classes', 'doxygenclass')]
        
        for kind, kind_name, directive in kinds:
            if kind in subdir_members:
                # Write header H2
                rst.write('%s\n%s\n\n' % (kind_name, '-'*70))

                for name, member in sorted(subdir_members[kind].items()):
                    short_name = member.short_name
                    fn = get_short_path(member.hpp_file_name)
                    
                    # Write header H3
                    if short_name != prev_short_name:
                        rst.write('%s\n%s\n\n' % (short_name, '~'*60))
                    prev_short_name = short_name

                    # Info about filename
                    rst.write('C++ documentation for ``%s`` from ``%s``:\n\n' % (short_name, fn))

                    # Write documentation for this item
                    rst.write(member.to_rst())
                    rst.write('\n\n')


def write_swig(subdir, subdir_members, swig_dir, swig_file_name, swig_header=''):
    """
    Write files for SWIG so that we get docstrings in Python
    """
    swig_subdir = os.path.join(swig_dir, subdir)
    if not os.path.isdir(swig_subdir):
        os.mkdir(swig_subdir)
    
    swig_iface_name = os.path.join(swig_subdir, swig_file_name)
    print('Generating', swig_iface_name)
    
    with open(swig_iface_name, 'wt') as out:
        out.write(swig_header)
        out.write('// SWIG docstrings generated by doxygen and generate_api_rst.py / parse_doxygen.py\n\n')
        
        for kind in subdir_members:
            for name, member in sorted(subdir_members[kind].items()):
                out.write(member.to_swig())
                out.write('\n')


def write_mock_modules(namespace_members, mock_py_module):
    """
    Write a mock module so that we can create documentation for 
    dolfin on ReadTheDocs where we cannot compile so that the
    dolfin.cpp.* module are not available. We fake those, but
    include the correct docstrings
    """
    print('Generating', mock_py_module)
    
    mydir = os.path.dirname(os.path.abspath(__file__))
    swig_module_dir = os.path.join(mydir, '..', 'dolfin', 'swig', 'modules')
    swig_module_dir = os.path.abspath(swig_module_dir)

    if not os.path.isdir(swig_module_dir):
        print('SWIG module directory is not present,', swig_module_dir)
        print('No mock Python code will be generated')
        return
    
    with open(mock_py_module, 'wt') as out:
        out.write('#!/usr/bin/env python\n')
        out.write('#\n')
        out.write('# This file is AUTO GENERATED!\n')
        out.write('# This file is fake, full of mock stubs\n')
        out.write('# This file is made by generate_api_rst.py\n')
        out.write('#\n\n')
        out.write('from __future__ import print_function\n')
        out.write('from types import ModuleType\n')
        out.write('import sys\n')
        out.write('\n\nWARNING = "This is a mock object!"\n')
        
        # Loop over SWIG modules and generate mock Python modules
        for module_name in os.listdir(swig_module_dir):
            module_i = os.path.join(swig_module_dir, module_name, 'module.i')
            if not os.path.isfile(module_i):
                continue
            
            # Find out which headers are included in this SWIG module
            included_headers = set()
            for line in open(module_i):
                if line.startswith('#include'):
                    header = line[8:].strip()[1:-1]
                    included_headers.add(header)
                elif line.startswith('%import'):
                    header = line.split(')')[1].strip()[1:-1]
                    included_headers.add(header)
            
            module_py_name = '_' + module_name
            full_module_py_name = 'dolfin.cpp.' + module_py_name
            out.write('\n\n' + '#'*80 + '\n')
            out.write('%s = ModuleType("%s")\n' % (module_py_name, full_module_py_name))
            out.write('sys.modules["%s"] = %s\n' % (full_module_py_name, module_py_name))
            out.write('\n')
            print('    Generating module', full_module_py_name)
            
            for member in namespace_members:
                # Check if this member is included in the given SWIG module
                hpp_file_name = get_short_path(member.hpp_file_name)
                if hpp_file_name not in included_headers:
                    continue
                
                out.write(member.to_mock(modulename=module_py_name))
                out.write('\n\n')


def parse_doxygen_xml_and_generate_rst_and_swig(xml_dir, api_gen_dir, swig_dir, swig_file_name,
                                                swig_header='', mock_py_module='', allow_empty_xml=False):
    # Read doxygen XML files and split namespace members into
    # groups based on subdir and kind (class, function, enum etc)
    create_subdir_groups_if_missing = False
    if os.path.isdir(xml_dir):
        namespaces = parse_doxygen.read_doxygen_xml_files(xml_dir, ['dolfin'])
    elif allow_empty_xml:
        namespaces = {'dolfin': parse_doxygen.Namespace('dolfin')} 
        create_subdir_groups_if_missing = True 
    else:
        raise OSError('Missing doxygen XML directory %r' % xml_dir)
    
    # Group all documented members into subdir groups (io, la, mesh, fem etc)
    sorted_members = list(namespaces['dolfin'].members.values())
    sorted_members.sort(key=lambda m: m.name)
    all_members = {}
    for member in sorted_members:
        subdir = get_subdir(member.hpp_file_name)
        sd = all_members.setdefault(subdir, {})
        kd = sd.setdefault(member.kind, {})
        kd[member.name] = member
    
    if create_subdir_groups_if_missing:
        # Create empty docstrings.i files in all relevvant subdirs of
        # dolfin/swig to enable builds without doxygen being present
        mydir =  os.path.dirname(os.path.abspath(__file__))
        dolfin_dir = os.path.abspath(os.path.join(mydir, '..', 'dolfin'))
        for subdir in os.listdir(dolfin_dir):
            path = os.path.join(dolfin_dir, subdir)
            if not os.path.isdir(path):
                continue
            all_members.setdefault(subdir, {})
    
    # Generate Sphinx RST files and SWIG interface files
    for subdir, subdir_members in sorted(all_members.items()):
        if subdir:
            if api_gen_dir:
                write_rst(subdir, subdir_members, api_gen_dir)
            if swig_dir:
                write_swig(subdir, subdir_members, swig_dir, swig_file_name, swig_header)
    
    # Generate a mock Python module
    if mock_py_module:
        write_mock_modules(sorted_members, mock_py_module)


if __name__ == '__main__':
    swig_dir = SWIG_DIR
    allow_empty_xml = False
    
    if '--no-swig' in sys.argv:
        swig_dir = None
    if '--allow-empty-xml' in sys.argv:
        allow_empty_xml = True
    
    parse_doxygen_xml_and_generate_rst_and_swig(DOXYGEN_XML_DIR, API_GEN_DIR, swig_dir,
                                                SWIG_FILE, '', MOCK_PY, allow_empty_xml)
