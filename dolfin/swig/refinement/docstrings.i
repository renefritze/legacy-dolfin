// Auto generated SWIG file for Python interface of DOLFIN
//
// Copyright (C) 2012 Kristian B. Oelgaard
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
// First added:  2012-01-18
// Last changed: 2012-03-09

// Autogenerated docstrings file, extracted from the DOLFIN source C++ files.

// Documentation extracted from: (module=refinement, header=refine.h)
%feature("docstring")  dolfin::refine "
**Overloaded versions**

* refine\ (mesh)

  Create uniformly refined mesh
  
  *Arguments*
      mesh (:py:class:`Mesh`)
          The mesh to refine.
  
  *Returns*
      :py:class:`Mesh`
          The refined mesh.
  
  *Example*
      .. note::
      
          No example code available for this function.

* refine\ (refined_mesh, mesh)

  Create uniformly refined mesh
  
  *Arguments*
      refined_mesh (:py:class:`Mesh`)
          The mesh that will be the refined mesh.
      mesh (:py:class:`Mesh`)
          The original mesh.

* refine\ (mesh, cell_markers)

  Create locally refined mesh
  
  *Arguments*
      mesh (:py:class:`Mesh`)
          The mesh to refine.
      cell_markers (:py:class:`MeshFunction`)
          A mesh function over booleans specifying which cells
          that should be refined (and which should not).
  
  *Returns*
      :py:class:`Mesh`
          The locally refined mesh.
  
  *Example*
      .. note::
      
          No example code available for this function.

* refine\ (refined_mesh, mesh, cell_markers)

  Create locally refined mesh
  
  *Arguments*
      refined_mesh (:py:class:`Mesh`)
          The mesh that will be the refined mesh.
      mesh (:py:class:`Mesh`)
          The original mesh.
      cell_markers (:py:class:`MeshFunction`)
          A mesh function over booleans specifying which cells
          that should be refined (and which should not).
";

