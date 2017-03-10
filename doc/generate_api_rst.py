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
SWIG_FILE = 'docstrings.i'


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



def write_rst(subdir, subdir_members):
    rst_name = os.path.join(API_GEN_DIR, 'api_gen_%s.rst' % subdir)
    print('Generating', rst_name)

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

# Read doxygen XML files and split namespace members into
# groups based on subdir and kind (class, function, enum etc)
namespaces = parse_doxygen.read_doxygen_xml_files(DOXYGEN_XML_DIR, ['dolfin'])
sorted_members = list(namespaces['dolfin'].members.values())
sorted_members.sort(key=lambda m: m.name)
all_members = {}
for member in sorted_members:
    subdir = get_subdir(member.hpp_file_name)
    sd = all_members.setdefault(subdir, {})
    kd = sd.setdefault(member.kind, {})
    kd[member.name] = member

# Make output directory
if not os.path.isdir(API_GEN_DIR):
    os.mkdir(API_GEN_DIR)

# Generate rst files
for subdir, subdir_members in sorted(all_members.items()):
    if subdir:
        write_rst(subdir, subdir_members)


# Make SWIG interface file
with open(SWIG_FILE, 'wt') as out:
    out.write('// SWIG docstrings generated by doxygen and generate_api_rst.py / parse_doxygen.py\n\n')
    for member in sorted_members:
        out.write(member.to_swig())
        out.write('\n')

