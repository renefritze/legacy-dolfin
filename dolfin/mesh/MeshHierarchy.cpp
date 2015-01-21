// Copyright (C) 2015 Chris Richardson
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

#include "MeshHierarchy.h"
#include <dolfin/mesh/Mesh.h>
#include <dolfin/mesh/MeshFunction.h>
#include <dolfin/refinement/refine.h>

using namespace dolfin;

//-----------------------------------------------------------------------------
// void MeshHierarchy::refine(MeshHierarchy& refined_mesh_hierarchy,
//                            const MeshFunction<bool>& markers) const
// {
//   std::shared_ptr<Mesh> refined_mesh(new Mesh);

//   // Make sure markers are on correct mesh, i.e. finest of hierarchy
//   dolfin_assert(markers.mesh()->id() == _meshes.back()->id());
//   dolfin::refine(*refined_mesh, *_meshes.back(), markers);

//   refined_mesh_hierarchy._meshes = _meshes;
//   refined_mesh_hierarchy._meshes.push_back(refined_mesh);

//   refined_mesh_hierarchy._parent = std::make_shared<const MeshHierarchy>(*this);
// }
//-----------------------------------------------------------------------------
std::shared_ptr<const MeshHierarchy> MeshHierarchy::refine(
                           const MeshFunction<bool>& markers) const
{
  std::shared_ptr<Mesh> refined_mesh(new Mesh);
  std::shared_ptr<MeshHierarchy> refined_hierarchy(new MeshHierarchy);

  // Make sure markers are on correct mesh, i.e. finest of hierarchy
  dolfin_assert(markers.mesh()->id() == _meshes.back()->id());
  dolfin::refine(*refined_mesh, *_meshes.back(), markers);

  refined_hierarchy->_meshes = _meshes;
  refined_hierarchy->_meshes.push_back(refined_mesh);

  refined_hierarchy->_parent = std::make_shared<const MeshHierarchy>(*this);

  return refined_hierarchy;
}
//-----------------------------------------------------------------------------
