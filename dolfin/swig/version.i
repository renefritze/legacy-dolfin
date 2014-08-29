// Copyright (C) 2006-2009 Johan Hake
//
// This file is part of DOLFIN.
//
// DOLFIN is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DOLFIN is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DOLFIN. If not, see <http://www.gnu.org/licenses/>.
//
// Modified by Garth N. Wells, 2013
//
// First added:  2006-02-06
// Last changed: 2013-11-01

//-----------------------------------------------------------------------------
// Include code to generate a __swigversion__ attributes, from defines during
// compile time, to the cpp module
//-----------------------------------------------------------------------------
%inline %{
unsigned int dolfin_swigversion() { return  SWIGVERSION; }
unsigned int dolfin_pythonversion() { return  PY_VERSION_HEX; }
%}

%pythoncode %{
tmp = hex(dolfin_swigversion())
int_hex = lambda x : int(x, base=16)
__swigversion__ = "%d.%d.%d"%(tuple(map(int_hex, [tmp[2], tmp[4], tmp[6]])))
tmp = hex(dolfin_pythonversion())
__pythonversion__ = "%d.%d.%d"%(tuple(map(int_hex, [tmp[2], tmp[4], tmp[6]])))
del tmp, int_hex, dolfin_pythonversion, dolfin_swigversion
%}
