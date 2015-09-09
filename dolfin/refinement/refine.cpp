// Copyright (C) 2010 Garth N. Wells
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
// Modified by Anders Logg, 2010-2011.
//
// First added:  2010-02-10
// Last changed: 2013-01-13

#include <dolfin/parameter/GlobalParameters.h>
#include <dolfin/mesh/Mesh.h>
#include <dolfin/mesh/MeshHierarchy.h>
#include <dolfin/mesh/MeshFunction.h>
#include "BisectionRefinement1D.h"
#include "PlazaRefinementND.h"
#include "RegularCutRefinement.h"
#include "refine.h"

using namespace dolfin;

//-----------------------------------------------------------------------------
dolfin::Mesh dolfin::refine(const Mesh& mesh, bool redistribute)
{
  Mesh refined_mesh;
  refine(refined_mesh, mesh, redistribute);
  return refined_mesh;
}
//-----------------------------------------------------------------------------
std::shared_ptr<const MeshHierarchy> dolfin::refine(
                    const MeshHierarchy& hierarchy,
                    const MeshFunction<bool>& markers)
{
  return hierarchy.refine(markers);
}
//-----------------------------------------------------------------------------
void dolfin::refine(Mesh& refined_mesh, const Mesh& mesh, bool redistribute)
{
  // Topological dimension
  const std::size_t D = mesh.topology().dim();

  const std::string refinement_algorithm = parameters["refinement_algorithm"];
  bool parent_facets = (refinement_algorithm == "plaza_with_parent_facets");

  // Dispatch to appropriate refinement function
  if (D == 1)
    BisectionRefinement1D::refine(refined_mesh, mesh, redistribute);
  else if(D == 2 or D == 3)
    PlazaRefinementND::refine(refined_mesh, mesh, redistribute, parent_facets);
  else
  {
    dolfin_error("refine.cpp",
                 "refine mesh",
                 "Cannot refine mesh of topological dimension %d", D);
  }

  // Report the number of refined cells
  const std::size_t n0 = mesh.size_global(D);
  const std::size_t n1 = refined_mesh.size_global(D);
  info("Number of cells increased from %d to %d (%.1f%% increase).",
       n0, n1, 100.0 * (static_cast<double>(n1) / static_cast<double>(n0) - 1.0));

}
//-----------------------------------------------------------------------------
dolfin::Mesh dolfin::refine(const Mesh& mesh,
                            const MeshFunction<bool>& cell_markers,
                            bool redistribute)
{
  Mesh refined_mesh;
  refine(refined_mesh, mesh, cell_markers, redistribute);
  return refined_mesh;
}
//-----------------------------------------------------------------------------
void dolfin::refine(Mesh& refined_mesh, const Mesh& mesh,
                    const MeshFunction<bool>& cell_markers, bool redistribute)
{
  // Topological dimension
  const std::size_t D = mesh.topology().dim();

  const std::string refinement_algorithm = parameters["refinement_algorithm"];
  bool parent_facets = (refinement_algorithm == "plaza_with_parent_facets");



  // Dispatch to appropriate refinement function
  if (D == 1)
    BisectionRefinement1D::refine(refined_mesh, mesh, cell_markers, redistribute);
  else if (refinement_algorithm == "regular_cut")
    RegularCutRefinement::refine(refined_mesh, mesh, cell_markers);
  else if (D == 2 or D == 3)
    PlazaRefinementND::refine(refined_mesh, mesh, cell_markers,
                              redistribute, parent_facets);
  else
  {
    dolfin_error("refine.cpp",
                 "refine mesh",
                 "Cannot refine mesh of topological dimension %d", D);
  }

  // Report the number of refined cells
  const std::size_t n0 = mesh.size_global(D);
  const std::size_t n1 = refined_mesh.size_global(D);
  info("Number of cells increased from %d to %d (%.1f%% increase).",
       n0, n1, 100.0 * (static_cast<double>(n1) / static_cast<double>(n0) - 1.0));

}
//-----------------------------------------------------------------------------
