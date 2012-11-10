// Copyright (C) 2008-2012 Niclas Jansson, Ola Skavhaug, Anders Logg and
// Garth N. Wells
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
// Modified by Garth N. Wells, 2010
// Modified by Kent-Andre Mardal, 2011
//
// First added:  2008-12-01
// Last changed: 2012-05-18

#ifndef __MESH_PARTITIONING_H
#define __MESH_PARTITIONING_H

#include <map>
#include <utility>
#include <vector>
#include <boost/multi_array.hpp>
#include <dolfin/common/types.h>
#include <dolfin/log/log.h>
#include "LocalMeshValueCollection.h"
#include "Mesh.h"
#include "MeshDistributed.h"

namespace dolfin
{
  // Note: MeshFunction and MeshValueCollection cannot apear in the
  // implementations that appear in this file of the templated functions
  // as this leads to a circular dependency. Therefore the functions are
  // templated over these types.

  template <typename T> class MeshFunction;
  template <typename T> class MeshValueCollection;
  class LocalMeshData;

  /// This class partitions and distributes a mesh based on
  /// partitioned local mesh data. Note that the local mesh data will
  /// also be repartitioned and redistributed during the computation
  /// of the mesh partitioning.
  ///
  /// After partitioning, each process has a local mesh and set of
  /// mesh data that couples the meshes together.
  ///
  /// The following mesh data is created:
  ///
  /// 1. "global entity indices 0" (MeshFunction<uint>)
  ///
  /// This maps each local vertex to its global index.
  ///
  /// 2. "overlap" (std::map<uint, std::vector<uint> >)
  ///
  /// This maps each shared vertex to a list of the processes sharing
  /// the vertex.
  ///
  /// 3. "global entity indices %d" (MeshFunction<uint>)
  ///
  /// After partitioning, the function number_entities() may be called
  /// to create global indices for all entities of a given topological
  /// dimension. These are stored as mesh data (MeshFunction<uint>)
  /// named
  ///
  ///    "global entity indices 1"
  ///    "global entity indices 2"
  ///    etc
  ///
  /// 4. "num global entities" (std::vector<uint>)
  ///
  /// The function number_entities also records the number of global
  /// entities for the dimension of the numbered entities in the array
  /// named "num global entities". This array has size D + 1, where D
  /// is the topological dimension of the mesh. This array is
  /// initially created by the mesh and then contains only the number
  /// entities of dimension 0 (vertices) and dimension D (cells).

  class MeshPartitioning
  {
  public:

   /// Build a partitioned mesh based on local meshes
    static void build_distributed_mesh(Mesh& mesh);

    /// Build a partitioned mesh based on local mesh data
    static void build_distributed_mesh(Mesh& mesh, const LocalMeshData& data);

    template<typename T>
    static void build_distributed_value_collection(MeshValueCollection<T>& values,
               const LocalMeshValueCollection<T>& local_data, const Mesh& mesh);

    /// Create global entity indices for entities of dimension d
    static void number_entities(const Mesh& mesh, uint d);

  private:

    typedef std::vector<std::size_t> Entity;

    struct EntityData
    {
      // Constructor
      EntityData() : index(0) {}

      // Constructor
      explicit EntityData(std::size_t index) : index(index) {}

      // Constructor
      EntityData(std::size_t index, const std::vector<unsigned int>& processes)
        : index(index), processes(processes) {}

      // Entity index
      std::size_t index;

      // Processes on which entity resides
      std::vector<unsigned int> processes;
    };

    /// Create a partitioned mesh based on local mesh data
    static void partition(Mesh& mesh, const LocalMeshData& data);

    /// Create and attach distributed MeshDomains from local_data
    static void build_mesh_domains(Mesh& mesh, const LocalMeshData& local_data);

    /// Create and attach distributed MeshDomains from local_data
    /// [entry, (cell_index, local_index, value)]
    template<typename T, typename MeshValueCollection>
    static void build_mesh_value_collection(const Mesh& mesh,
      const std::vector<std::pair<std::pair<std::size_t, uint>, T> >& local_value_data,
      MeshValueCollection& mesh_values);

    // Compute and return (number of global entities, process offset)
    static std::pair<std::size_t, std::size_t>
      compute_num_global_entities(std::size_t num_local_entities,
                                  uint num_processes,
                                  uint process_number);

    // Build entity ownership
    static void compute_entity_ownership(
          const std::map<Entity, std::size_t>& entities,
          const std::map<std::size_t, std::set<uint> >& shared_vertices,
          std::map<Entity, EntityData>& owned_exclusive_entities,
          std::map<Entity, EntityData>& owned_shared_entities,
          std::map<Entity, EntityData>& unowned_shared_entities);

    // Build preliminary 'guess' of shared enties
    static void compute_preliminary_entity_ownership(
          const std::map<Entity, std::size_t>& entities,
          const std::map<std::size_t, std::set<uint> >& shared_vertices,
          std::map<Entity, EntityData>& owned_exclusive_entities,
          std::map<Entity, EntityData>& owned_shared_entities,
          std::map<Entity, EntityData>& unowned_shared_entities);

    // Communicate with other processes to finalise entity ownership
    static void compute_final_entity_ownership(
          std::map<Entity, EntityData>& owned_exclusively_entities,
          std::map<Entity, EntityData>& owned_shared_entities,
          std::map<Entity, EntityData>& unowned_shared_entities);

    // Distribute cells
    static void distribute_cells(std::vector<std::size_t>& global_cell_indices,
                                 boost::multi_array<std::size_t, 2>& cell_vertices,
                                 const LocalMeshData& data,
                                 const std::vector<uint>& cell_partition);

    // Distribute vertices
    static void distribute_vertices(std::vector<std::size_t>& vertex_indices,
                  boost::multi_array<double, 2>& vertex_coordinates,
                  std::map<std::size_t, std::size_t>& glob2loc,
                  const boost::multi_array<std::size_t, 2>& cell_vertices,
                  const LocalMeshData& data);

    // Build mesh
    static void build_mesh(Mesh& mesh,
                   const std::vector<std::size_t>& global_cell_indices,
                   const boost::multi_array<std::size_t, 2>& cell_vertices,
                   const std::vector<std::size_t>& vertex_indices,
                   const boost::multi_array<double, 2>& vertex_coordinates,
                   const std::map<std::size_t, std::size_t>& glob2loc,
                   uint tdim, uint gdim, std::size_t num_global_cells,
                   std::size_t num_global_vertices);

    // Check if all entity vertices are in overlap
    static bool in_overlap(const std::vector<std::size_t>& entity_vertices,
               const std::map<std::size_t, std::set<unsigned int> >& overlap);

    /// Compute number of cells connected to each facet (globally). Facets
    /// on internal boundaries will be connected to two cells (with the
    /// cells residing on neighboring processes)
    static std::vector<unsigned int> num_connected_cells(const Mesh& mesh,
               const std::map<Entity, std::size_t>& entities,
               const std::map<Entity, EntityData>& owned_shared_entities,
               const std::map<Entity, EntityData>& unowned_shared_entities);
  };

  //---------------------------------------------------------------------------
  template<typename T>
  void MeshPartitioning::build_distributed_value_collection(MeshValueCollection<T>& values,
             const LocalMeshValueCollection<T>& local_data, const Mesh& mesh)
  {
    // Extract data
    const std::vector<std::pair<std::pair<std::size_t, uint>, T> >& local_values
      = local_data.values();

    // Build MeshValueCollection from local data
    build_mesh_value_collection(mesh, local_values, values);
  }
  //---------------------------------------------------------------------------
  template<typename T, typename MeshValueCollection>
  void MeshPartitioning::build_mesh_value_collection(const Mesh& mesh,
    const std::vector<std::pair<std::pair<std::size_t, uint>, T> >& local_value_data,
    MeshValueCollection& mesh_values)
  {
    // Get topological dimensions
    const uint D = mesh.topology().dim();
    const uint dim = mesh_values.dim();

    // Clear MeshValueCollection values
    mesh_values.values().clear();

    // Initialise global entity numbering
    MeshPartitioning::number_entities(mesh, dim);
    MeshPartitioning::number_entities(mesh, D);

    if (dim == 0)
    {
      // MeshPartitioning::build_mesh_value_collection needs updating
      // for vertices
      dolfin_not_implemented();
    }

    // Get mesh value collection used for marking
    MeshValueCollection& markers = mesh_values;

    // Get local mesh data for domains
    const std::vector< std::pair<std::pair<std::size_t, uint>, T> >&
      ldata = local_value_data;

    // Get local local-to-global map
    if (!mesh.topology().have_global_indices(D))
    {
      dolfin_error("MeshPartitioning.h",
                   "build mesh value collection",
                   "Do not have have_global_entity_indices");
    }

    // Get global indices on local process
    const std::vector<std::size_t> global_entity_indices
      = mesh.topology().global_indices(D);

    // Add local (to this process) data to domain marker
    std::vector<std::size_t>::iterator it;
    std::vector<std::size_t> off_process_global_cell_entities;

    // Build and populate a local map for global_entity_indices
    std::map<std::size_t, std::size_t> map_of_global_entity_indices;
    for (std::size_t i = 0; i < global_entity_indices.size(); i++)
      map_of_global_entity_indices[global_entity_indices[i]] = i;

    for (std::size_t i = 0; i < ldata.size(); ++i)
    {
      const std::size_t global_cell_index = ldata[i].first.first;
      std::map<std::size_t, std::size_t>::const_iterator it
        = map_of_global_entity_indices.find(global_cell_index);
      if (it != map_of_global_entity_indices.end())
      {
        const std::size_t local_cell_index = it->second;
        const std::size_t entity_local_index = ldata[i].first.second;
        const T value = ldata[i].second;
        markers.set_value(local_cell_index, entity_local_index, value);
      }
      else
        off_process_global_cell_entities.push_back(global_cell_index);
    }

    // Get destinations and local cell index at destination for off-process cells
    const std::map<std::size_t, std::set<std::pair<unsigned int, std::size_t> > >
      entity_hosts = MeshDistributed::off_process_indices(off_process_global_cell_entities, D, mesh);

    // Pack data to send to appropriate process
    std::vector<std::size_t> send_data0;
    std::vector<T> send_data1;
    std::vector<uint> destinations0;
    std::vector<uint> destinations1;
    std::map<std::size_t, std::set<std::pair<unsigned int, std::size_t> > >::const_iterator entity_host;

    {
      // Build a convenience map in order to speedup the loop over local data
      std::map<std::size_t, std::set<std::size_t> > map_of_ldata;
      for (std::size_t i = 0; i < ldata.size(); ++i)
        map_of_ldata[ldata[i].first.first].insert(i);

      for (entity_host = entity_hosts.begin(); entity_host != entity_hosts.end(); ++entity_host)
      {
        const std::size_t host_global_cell_index = entity_host->first;
        const std::set<std::pair<unsigned int, std::size_t> >& processes_data = entity_host->second;

        // Loop over local data
        std::map<std::size_t, std::set<std::size_t> >::const_iterator ldata_it
          = map_of_ldata.find(host_global_cell_index);
        if (ldata_it != map_of_ldata.end())
        {
          for (std::set<std::size_t>::const_iterator it = ldata_it->second.begin(); it != ldata_it->second.end(); it++)
          {
            const std::size_t local_entity_index = ldata[*it].first.second;
            const T domain_value = ldata[*it].second;

            std::set<std::pair<unsigned int, std::size_t> >::const_iterator process_data;
            for (process_data = processes_data.begin(); process_data != processes_data.end(); ++process_data)
            {
              const std::size_t proc = process_data->first;
              const std::size_t local_cell_entity = process_data->second;

              send_data0.push_back(local_cell_entity);
              send_data0.push_back(local_entity_index);
              destinations0.insert(destinations0.end(), 2, proc);

              send_data1.push_back(domain_value);
              destinations1.push_back(proc);
            }
          }
        }
      }
    }

    // Send/receive data
    std::vector<std::size_t> received_data0;
    std::vector<T> received_data1;
    MPI::distribute(send_data0, destinations0, received_data0);
    MPI::distribute(send_data1, destinations1, received_data1);
    dolfin_assert(2*received_data1.size() == received_data0.size());

    // Add received data to mesh domain
    for (std::size_t i = 0; i < received_data1.size(); ++i)
    {
      const std::size_t local_cell_entity = received_data0[2*i];
      const std::size_t local_entity_index = received_data0[2*i + 1];
      const T value = received_data1[i];
      dolfin_assert(local_cell_entity < mesh.num_cells());
      markers.set_value(local_cell_entity, local_entity_index, value);
    }
  }
  //---------------------------------------------------------------------------

}

#endif
