// Copyright (C) 2012 Chris Richardson
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
// 
// First Added: 2012-12-19
// Last Changed: 2013-01-10

#include <vector>
#include <map>
#include <boost/unordered_map.hpp>
#include <boost/multi_array.hpp>

#include <dolfin/common/types.h>
#include <dolfin/common/MPI.h>
#include <dolfin/common/Timer.h>
#include <dolfin/mesh/Mesh.h>
#include <dolfin/mesh/Cell.h>
#include <dolfin/mesh/Edge.h>
#include <dolfin/mesh/Vertex.h>

#include <dolfin/refinement/ParallelRefinement.h>

#include "ParallelRefinement2D.h"

using namespace dolfin;

bool ParallelRefinement2D::length_compare(std::pair<double, std::size_t> a, std::pair<double, std::size_t> b)
{
  return (a.first > b.first);
}

//-----------------------------------------------------------------------------
// Work out which edge will be the reference edge for each cell

void ParallelRefinement2D::generate_reference_edges(const Mesh& mesh, std::vector<std::size_t>& ref_edge)
{
  uint D = mesh.topology().dim();
  
  ref_edge.resize(mesh.size(D));
  
  for(CellIterator cell(mesh); !cell.end(); ++cell)
  {
    std::size_t cell_index = cell->index();
    
    std::vector<std::pair<double,std::size_t> > lengths;
    EdgeIterator celledge(*cell);
    for(std::size_t i = 0; i < 3; i++)
    {
      lengths.push_back(std::make_pair(celledge[i].length(), i));
    }
      
    std::sort(lengths.begin(), lengths.end(), length_compare);
    
    // for now - just pick longest edge - this is not the Carstensen algorithm, which tries
    // to pair edges off. Because that is more difficult in parallel, it is not implemented yet.
    const std::size_t edge_index = lengths[0].second;
    ref_edge[cell_index] = edge_index;
  }
}

//-----------------------------------------------------------------------------
void ParallelRefinement2D::refine(Mesh& new_mesh, const Mesh& mesh)
{
  if(MPI::num_processes()==1)
  {
    dolfin_error("ParallelRefinement2D.cpp",
                 "refine mesh",
                 "Only works in parallel");
  }

  const uint tdim = mesh.topology().dim();

  if(tdim != 2)
  {
    dolfin_error("ParallelRefinement2D.cpp",
                 "refine mesh",
                 "Only works in 2D");
  }

  // Ensure connectivity is there
  mesh.init(tdim - 1, tdim);

  // Create a class to hold most of the refinement information
  ParallelRefinement p(mesh);
  
  // Mark all edges, and create new vertices
  EdgeFunction<bool> markedEdges(mesh, true);
  p.create_new_vertices(markedEdges);
  std::map<std::size_t, std::size_t>& edge_to_new_vertex = p.edge_to_new_vertex();
  
  // Generate new topology

  for(CellIterator cell(mesh); !cell.end(); ++cell)
  {
    EdgeIterator e(*cell);
    VertexIterator v(*cell);

    const std::size_t v0 = v[0].global_index();
    const std::size_t v1 = v[1].global_index();
    const std::size_t v2 = v[2].global_index();
    const std::size_t e0 = edge_to_new_vertex[e[0].index()];
    const std::size_t e1 = edge_to_new_vertex[e[1].index()];
    const std::size_t e2 = edge_to_new_vertex[e[2].index()];

    p.new_cell(v0, e2, e1);
    p.new_cell(e2, v1, e0);
    p.new_cell(e1, e0, v2);
    p.new_cell(e0, e1, e2);
  }

  p.partition(new_mesh);

}

//-----------------------------------------------------------------------------
void ParallelRefinement2D::refine(Mesh& new_mesh, const Mesh& mesh, 
                                  const MeshFunction<bool>& refinement_marker)
{
  const uint tdim = mesh.topology().dim();

  bool diag=false;   // Enable output for diagnostics
  
  if(MPI::num_processes()==1)
  {
    dolfin_error("ParallelRefinement2D.cpp",
                 "refine mesh",
                 "Only works in parallel");
  }

  if(tdim != 2)
  {
    dolfin_error("ParallelRefinement2D.cpp",
                 "refine mesh",
                 "Only works in 2D");
  }

  // Ensure connectivity is there
  mesh.init(tdim - 1, tdim);

  // Create a class to hold most of the refinement information
  ParallelRefinement p(mesh);

  // Vector over all cells - the reference edge is the cell's edge (0, 1 or 2) 
  // which always must split, if any edge splits in the cell
  std::vector<std::size_t> ref_edge;
  generate_reference_edges(mesh, ref_edge);
   
  if(diag)
  {
    EdgeFunction<bool> ref_edge_fn(mesh,false);
    CellFunction<std::size_t> ref_edge_fn2(mesh);
    for(CellIterator cell(mesh); !cell.end(); ++cell)
    {
      EdgeIterator e(*cell);
      ref_edge_fn[ e[ref_edge[cell->index()]] ] = true;
      ref_edge_fn2[*cell] = ref_edge[cell->index()];
    }
    
    File refEdgeFile("ref_edge.xdmf");
    refEdgeFile << ref_edge_fn;
    
    File refEdgeFile2("ref_edge2.xdmf");
    refEdgeFile2 << ref_edge_fn2;
  }
  
  // Set marked edges from marked cells
  EdgeFunction<bool> markedEdges(mesh,false);
  
  // Mark all edges of marked cells
  for(CellIterator cell(mesh); !cell.end(); ++cell)
  {
    if(refinement_marker[*cell])
      for(EdgeIterator edge(*cell); !edge.end(); ++edge)
      {
        //        p.mark_edge(edge->index());
        markedEdges[*edge] = true;
      }
  }
  
  // Mark reference edges of cells with any marked edge
  // and repeat until no more marking takes place

  uint update_count = 1;
  while(update_count != 0)
  {
    update_count = 0;
    
    // Transmit values between processes - could be streamlined
    p.update_logical_edgefunction(markedEdges);
    
    for(CellIterator cell(mesh); !cell.end(); ++cell)
    {
      bool marked = false;
      // Check if any edge of this cell is marked
      for(EdgeIterator edge(*cell); !edge.end(); ++edge)
      {
        if(markedEdges[*edge])
          marked = true;
      }

      EdgeIterator edge(*cell);
      std::size_t ref_edge_index = edge[ref_edge[cell->index()]].index();

      if(marked && markedEdges[ref_edge_index] == false)
      {
        update_count = 1;
        markedEdges[ref_edge_index] = true;
      }
    }

    //    std::cout << MPI::process_number() << ":" << update_count << std::endl;
    update_count = MPI::sum(update_count);
  
  }
  
  if(diag)
  {
      // Diagnostic output
    File markedEdgeFile("marked_edges.xdmf");
    markedEdgeFile << markedEdges;
  }

  // Generate new vertices from marked edges, and assign global vertex index map.

  p.create_new_vertices(markedEdges);
  std::map<std::size_t, std::size_t>& edge_to_new_vertex = p.edge_to_new_vertex();

  // Stage 4 - do refinement 
  // FIXME - keep reference edges somehow?...

  for(CellIterator cell(mesh); !cell.end(); ++cell)
  {
    
    std::size_t rgb_count = 0;
    for(EdgeIterator edge(*cell); !edge.end(); ++edge)
    {
      if(markedEdges[*edge])
        rgb_count++;
    }

    EdgeIterator e(*cell);
    VertexIterator v(*cell);

    const std::size_t ref = ref_edge[cell->index()];
    const std::size_t i0 = ref;
    const std::size_t i1 = (ref + 1)%3;
    const std::size_t i2 = (ref + 2)%3;
    const std::size_t v0 = v[i0].global_index();
    const std::size_t v1 = v[i1].global_index();
    const std::size_t v2 = v[i2].global_index();
    const std::size_t e0 = edge_to_new_vertex[e[i0].index()];
    const std::size_t e1 = edge_to_new_vertex[e[i1].index()];
    const std::size_t e2 = edge_to_new_vertex[e[i2].index()];

    if(rgb_count == 0) //straight copy of cell (1->1)
    {
      p.new_cell(v0, v1, v2);
    }
    else if(rgb_count == 1) // "green" refinement (1->2)
    {
      // Always splitting the reference edge...
      p.new_cell(e0, v0, v1);
      p.new_cell(e0, v2, v0);
    }
    else if(rgb_count == 2) // "blue" refinement (1->3) left or right
    {
      if(markedEdges[e[i2]])
      {
        p.new_cell(e2, v1, e0);
        p.new_cell(e2, e0, v0);
        p.new_cell(e0, v2, v0);
      }
      else if(markedEdges[e[i1]])
      {
        p.new_cell(e0, v0, v1);
        p.new_cell(e1, e0, v2);
        p.new_cell(e1, v0, e0);
      }

    }
    else if(rgb_count == 3) // "red" refinement - all split (1->4) cells
    {
      p.new_cell(v0, e2, e1);
      p.new_cell(e2, v1, e0);
      p.new_cell(e1, e0, v2);
      p.new_cell(e0, e1, e2);
    }
    
  }

  // Call partitioning from within ParallelRefinement class
  p.partition(new_mesh);

  if(diag)
  {
    const std::size_t process_number = MPI::process_number();
    CellFunction<std::size_t> partitioning1(mesh, process_number);
    CellFunction<std::size_t> partitioning2(new_mesh, process_number);
    
    File meshFile1("old_mesh.xdmf");
    meshFile1 << partitioning1;  
    
    File meshFile2("new_mesh.xdmf");
    meshFile2 << partitioning2;  
  }
  

}

