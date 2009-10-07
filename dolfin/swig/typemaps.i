/* -*- C -*- */
// Copyright (C) 2007-2009 Anders logg
// Licensed under the GNU LGPL Version 2.1.
//
// Modified by Ola Skavhaug, 2007-2009.
// Modified by Garth N. Wells, 2007.
// Modified by Johan Hake, 2008-2009.
//
// First added:  2006-04-16
// Last changed: 2009-10-07

//=============================================================================
// General typemaps for PyDOLFIN
//=============================================================================

//-----------------------------------------------------------------------------
// A home brewed type check for checking integers 
// Needed due to problems with PyInt_Check from python 2.6 and NumPy
//-----------------------------------------------------------------------------
%{
SWIGINTERNINLINE bool PyInteger_Check(PyObject* in)
{
  return  PyInt_Check(in) || (PyArray_CheckScalar(in) && 
			      PyArray_IsScalar(in,Integer));
}
%}

//-----------------------------------------------------------------------------
// Apply the builtin out-typemap for int to dolfin::uint
//-----------------------------------------------------------------------------
%typemap(out) dolfin::uint = int;

//-----------------------------------------------------------------------------
// Typemaps for dolfin::real
// We do not pass any high precision values here. This might change in future
// FIXME: We need to find out what to do with Parameters of real. Now they are 
// treated the same as double, and need to have a different typecheck value than
// DOUBLE 90!= 95. However this will render real parameters unusefull if we do
// not pick something else thatn PyFloat_Check in the typecheck.
//-----------------------------------------------------------------------------
%typecheck(95) dolfin::real
{
  // When implementing high precision type, check for that here.
  $1 = PyFloat_Check($input) ? 1 : 0;
}

%typemap(in) dolfin::real
{
  $1 = dolfin::to_real(PyFloat_AsDouble($input));
}

%typemap(out) dolfin::real
{
  $result = SWIG_From_double(dolfin::to_double($1));
}

//-----------------------------------------------------------------------------
// Typemaps for dolfin::uint and int
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// The typecheck (dolfin::uint)
//-----------------------------------------------------------------------------
%typecheck(SWIG_TYPECHECK_INTEGER) dolfin::uint
{
  $1 = PyInteger_Check($input) ? 1 : 0;
}

//-----------------------------------------------------------------------------
// The typemap (dolfin::uint)
//-----------------------------------------------------------------------------
%typemap(in) dolfin::uint
{
  if (PyInteger_Check($input))
  {
    long tmp = static_cast<long>(PyInt_AsLong($input));
    if (tmp>=0)
      $1 = static_cast<dolfin::uint>(tmp);
    else
      SWIG_exception(SWIG_TypeError, "expected positive 'int' for argument $argnum");
  }
  else
    SWIG_exception(SWIG_TypeError, "expected positive 'int' for argument $argnum");
}

//-----------------------------------------------------------------------------
// The typecheck (int)
//-----------------------------------------------------------------------------
%typecheck(SWIG_TYPECHECK_INTEGER) int
{
    $1 =  PyInteger_Check($input) ? 1 : 0;
}

//-----------------------------------------------------------------------------
// The typemap (int)
//-----------------------------------------------------------------------------
%typemap(in) int
{
  if (PyInteger_Check($input))
  {
    long tmp = static_cast<long>(PyInt_AsLong($input));
    $1 = static_cast<int>(tmp);
  }
  else
    SWIG_exception(SWIG_TypeError, "expected 'int' for argument $argnum");
}

//-----------------------------------------------------------------------------
// Out typemap for std::pair<TYPE,TYPE>
//-----------------------------------------------------------------------------
%typemap(out) std::pair<dolfin::uint,dolfin::uint>
{
  $result = Py_BuildValue("ii",$1.first,$1.second);
}
%typemap(out) std::pair<dolfin::uint,bool>
{
  $result = Py_BuildValue("ib",$1.first,$1.second);
}
