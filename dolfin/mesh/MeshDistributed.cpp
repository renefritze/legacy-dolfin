// Copyright (C) 2011 Garth N. Wells
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
// Modified by Anders Logg 2011
//
// First added:  2011-09-17
// Last changed: 2011-11-14

#include "dolfin/common/MPI.h"
#include "dolfin/common/Timer.h"
#include "dolfin/log/log.h"
#include "dolfin/mesh/Facet.h"
#include "dolfin/mesh/Mesh.h"
#include "dolfin/mesh/MeshEntityIterator.h"
#include "dolfin/mesh/Vertex.h"
#include "MeshFunction.h"

#include "MeshDistributed.h"

using namespace dolfin;

//-----------------------------------------------------------------------------
void MeshDistributed::number_entities(const Mesh& _mesh, std::size_t d)
{
  Timer timer("PARALLEL x: Number mesh entities");

  // Return if global entity indices have already been calculated
  if (_mesh.topology().have_global_indices(d))
    return;

  Mesh& mesh = const_cast<Mesh&>(_mesh);

  // Check that we're not re-numbering vertices
  if (d == 0)
  {
    dolfin_error("MeshPartitioning.cpp",
                 "number mesh entities",
                 "Gloval vertex indices exist at input. Cannot be renumbered");
  }

  // Check that we're not re-numbering cells
  if (d == mesh.topology().dim())
  {
    dolfin_error("MeshPartitioning.cpp",
                 "number mesh entities",
                 "Global cells indices exist at input. Cannot be renumbered");
  }

  // Get number of processes and process number
  const std::size_t num_processes = MPI::num_processes();
  const std::size_t process_number = MPI::process_number();

  // Initialize entities of dimension d
  mesh.init(d);

  // Compute ownership of entities ([entity vertices], data):
  //  [0]: owned and shared (will be numbered by this process, and number
  //       communicated to other processes)
  //  [1]: not owned but shared (will be numbered by another process, and number
  //       communicated to this processes)
  boost::array<std::map<Entity, EntityData>, 2> entity_ownership;
  std::vector<std::size_t> owned_entities;
  compute_entity_ownership(mesh, d, owned_entities, entity_ownership);
  const std::map<Entity, EntityData>& owned_shared_entities    = entity_ownership[0];
  const std::map<Entity, EntityData>& unowned_shared_entities  = entity_ownership[1];

  // Number of entities 'owned' by this process
  const std::size_t num_local_entities = owned_entities.size()
                                       + owned_shared_entities.size();

  // Compute global number of entities and local process offset
  const std::pair<std::size_t, std::size_t> num_global_entities
      = compute_num_global_entities(num_local_entities, num_processes,
                                    process_number);

  // Extract offset
  std::size_t offset = num_global_entities.second;

  // Prepare list of entity numbers. Check later that nothing is
  // equal to std::numeric_limits<std::size_t>::max()
  std::vector<std::size_t> global_entity_indices(mesh.size(d),
                                 std::numeric_limits<std::size_t>::max());

  std::map<Entity, EntityData>::const_iterator it;

  // Number exlusively owned entities
  for (std::size_t i = 0; i < owned_entities.size(); ++i)
    global_entity_indices[owned_entities[i]] = offset++;

  // Number shared entities that this process is responsible for numbering
  std::map<Entity, EntityData>::const_iterator it1;
  for (it1 = owned_shared_entities.begin(); it1 != owned_shared_entities.end(); ++it1)
    global_entity_indices[it1->second.local_index] = offset++;

  // Communicate indices for shared entities (owned by this process)
  // and get indices for shared but not owned entities
  std::vector<std::size_t> send_values;
  std::vector<std::size_t> destinations;
  for (it1 = owned_shared_entities.begin(); it1 != owned_shared_entities.end(); ++it1)
  {
    // Get entity index
    const std::size_t local_entity_index = it1->second.local_index;
    const std::size_t global_entity_index = global_entity_indices[local_entity_index];
    dolfin_assert(global_entity_index != std::numeric_limits<std::size_t>::max());

    // Get entity vertices (global vertex indices)
    const Entity& entity = it1->first;

    // Get entity processes (processes sharing the entity)
    const std::vector<std::size_t>& entity_processes = it1->second.processes;

    // Prepare data for sending
    for (std::size_t j = 0; j < entity_processes.size(); ++j)
    {
      // Store interleaved: entity index, number of vertices, global
      // vertex indices
      send_values.push_back(global_entity_index);
      send_values.push_back(entity.size());
      send_values.insert(send_values.end(), entity.begin(), entity.end());
      destinations.insert(destinations.end(), entity.size() + 2, entity_processes[j]);
    }
  }

  // Send data
  std::vector<std::size_t> received_values;
  std::vector<std::size_t> sources;
  MPI::distribute(send_values, destinations, received_values, sources);

  // Fill in global entity indices received from lower ranked processes
  for (std::size_t i = 0; i < received_values.size();)
  {
    const std::size_t p = sources[i];
    const std::size_t global_index = received_values[i++];
    const std::size_t entity_size = received_values[i++];
    Entity entity;
    for (std::size_t j = 0; j < entity_size; ++j)
      entity.push_back(received_values[i++]);

    // Access unowned entity data
    std::map<Entity, EntityData>::const_iterator recv_entity;
    recv_entity = unowned_shared_entities.find(entity);

    // Sanity check, should not receive an entity we don't need
    if (recv_entity == unowned_shared_entities.end())
    {
      std::stringstream msg;
      msg << "Process " << MPI::process_number() << " received illegal entity given by ";
      msg << " with global index " << global_index;
      msg << " from process " << p;
      dolfin_error("MeshPartitioning.cpp",
                   "number mesh entities",
                   msg.str());
    }

    const std::size_t local_entity_index = recv_entity->second.local_index;
    dolfin_assert(global_entity_indices[local_entity_index] == std::numeric_limits<std::size_t>::max());
    global_entity_indices[local_entity_index] = global_index;
  }

  // Set mesh topology and store number of global entities
  mesh.topology().init_global(d, num_global_entities.first);
  mesh.topology().init_global_indices(d, global_entity_indices.size());
  for (std::size_t i = 0; i < global_entity_indices.size(); ++i)
  {
    if (global_entity_indices[i] == std::numeric_limits<std::size_t>::max())
      log(WARNING, "Missing global number for local entity (%d, %d).", d, i);

    dolfin_assert(global_entity_indices[i] >= 0);
    mesh.topology().set_global_index(d, i, global_entity_indices[i]);
  }

  // Get shared entities map
  std::map<std::size_t, std::set<std::size_t> >&
    shared_entities = mesh.topology().shared_entities(d);
  shared_entities.clear();

  // Build shared_entities (global index, [sharing processes])
  std::map<Entity, EntityData>::const_iterator e;
  for (e = owned_shared_entities.begin(); e != owned_shared_entities.end(); ++e)
  {
    const EntityData& ed = e->second;
    shared_entities[ed.local_index] = std::set<std::size_t>(ed.processes.begin(),
                                                            ed.processes.end());
  }
  for (e = unowned_shared_entities.begin(); e != unowned_shared_entities.end(); ++e)
  {
    const EntityData& ed = e->second;
    shared_entities[ed.local_index] = std::set<std::size_t>(ed.processes.begin(),
                                                            ed.processes.end());
  }
}
//-----------------------------------------------------------------------------
std::map<std::size_t, std::set<std::pair<std::size_t, std::size_t> > >
MeshDistributed::locate_off_process_entities(const std::vector<std::size_t>& entity_indices,
                                             std::size_t dim, const Mesh& mesh)
{
  if (dim == 0)
    warning("MeshDistributed::host_processes has not been tested for vertices.");

  // Mesh topology dim
  const std::size_t D = mesh.topology().dim();

  // Check that entity is a vertex or a cell
  if (dim != 0 && dim != D)
  {
    dolfin_error("MeshDistributed.cpp",
                 "compute off-process indices",
                 "This version of MeshDistributed::host_processes is only for vertices or cells");
  }

  // Check that global numbers have been computed.
  if (!mesh.topology().have_global_indices(dim))
  {
    dolfin_error("MeshDistributed.cpp",
                 "compute off-process indices",
                 "Global mesh entity numbers have not been computed");
  }

  // Check that global numbers have been computed.
  if (!mesh.topology().have_global_indices(D))
  {
    dolfin_error("MeshDistributed.cpp",
                 "compute off-process indices",
                 "Global mesh entity numbers have not been computed");
  }

  // Get global cell entity indices on this process
  const std::vector<std::size_t> global_entity_indices
      = mesh.topology().global_indices(dim);

  dolfin_assert(global_entity_indices.size() == mesh.num_cells());

  // Prepare map to hold process numbers
  std::map<std::size_t, std::set<std::pair<std::size_t, std::size_t> > > processes;

  // FIXME: work on optimising below code

  // List of indices to send
  std::vector<std::size_t> my_entities;

  // Remove local cells from my_entities to reduce communication
  if (dim == D)
  {
    // In order to fill vector my_entities...
    // build and populate a local set for non-local cells
    std::set<std::size_t> set_of_my_entities(entity_indices.begin(), entity_indices.end());

    // FIXME: This can be made more efficient by exploiting fact that
    //        set is sorted
    // Remove local cells from set_of_my_entities to reduce communication
    for (std::size_t j = 0; j < global_entity_indices.size(); ++j)
      set_of_my_entities.erase(global_entity_indices[j]);

    // Copy entries from set_of_my_entities to my_entities
    my_entities = std::vector<std::size_t>(set_of_my_entities.begin(), set_of_my_entities.end());
  }
  else
    my_entities = entity_indices;

  // FIXME: handle case when my_entities.empty()
  //dolfin_assert(!my_entities.empty());

  // Prepare data structures for send/receive
  const std::size_t num_proc = MPI::num_processes();
  const std::size_t proc_num = MPI::process_number();
  const std::size_t max_recv = MPI::max(my_entities.size());
  std::vector<std::size_t> off_process_entities(max_recv);

  // Send and receive data
  for (std::size_t k = 1; k < MPI::num_processes(); ++k)
  {
    const std::size_t src  = (proc_num - k + num_proc) % num_proc;
    const std::size_t dest = (proc_num + k) % num_proc;

    MPI::send_recv(my_entities, dest, off_process_entities, src);

    const std::size_t recv_entity_count = off_process_entities.size();

    // Check if this process owns received entities, and if so
    // store local index
    std::vector<std::size_t> my_hosted_entities;
    {
      // Build a temporary map hosting global_entity_indices
      std::map<std::size_t, std::size_t> map_of_global_entity_indices;
      for (std::size_t j = 0; j < global_entity_indices.size(); j++)
        map_of_global_entity_indices[global_entity_indices[j]] = j;

      for (std::size_t j = 0; j < recv_entity_count; j++)
      {
        // Check if this process hosts 'received_entity'
        const std::size_t received_entity = off_process_entities[j];
        std::map<std::size_t, std::size_t>::const_iterator it;
        it = map_of_global_entity_indices.find(received_entity);
        if (it != map_of_global_entity_indices.end())
        {
          const std::size_t local_index = it->second;
          my_hosted_entities.push_back(received_entity);
          my_hosted_entities.push_back(local_index);
        }
      }
    }

    // Send/receive hosted cells
    const std::size_t max_recv_host_proc = MPI::max(my_hosted_entities.size());
    std::vector<std::size_t> host_processes(max_recv_host_proc);
    MPI::send_recv(my_hosted_entities, src, host_processes, dest);

    const std::size_t recv_hostproc_count = host_processes.size();
    for (std::size_t j = 0; j < recv_hostproc_count; j += 2)
    {
      const std::size_t global_index = host_processes[j];
      const std::size_t local_index  = host_processes[j + 1];
      processes[global_index].insert(std::make_pair(dest, local_index));
    }

    // FIXME: Do later for efficiency
    // Remove entries from entities (from my_entities) that cannot
    // reside on more processes (i.e., cells)
  }

  // Sanity check
  const std::set<std::size_t> test_set(my_entities.begin(), my_entities.end());
  const std::size_t number_expected = test_set.size();
  if (number_expected != processes.size())
  {
    dolfin_error("MeshDistributed.cpp",
                 "compute off-process indices",
                 "Sanity check failed");
  }

  return processes;
}
//-----------------------------------------------------------------------------
boost::unordered_map<std::size_t, std::vector<std::pair<std::size_t, std::size_t> > >
  MeshDistributed::compute_shared_entities(const Mesh& mesh, std::size_t d)
{
  // Number entities (globally)
  number_entities(mesh, d);

  // Initialize entities of dimension d
  mesh.init(d);

  // Get shared entities to processes map
  const std::map<std::size_t, std::set<std::size_t> >&
    shared_entities = mesh.topology().shared_entities(d);

  // Get local-to-global indices map
  const std::vector<std::size_t>& global_indices_map = mesh.topology().global_indices(d);

  // Global-to-local map for each process
  boost::unordered_map<std::size_t, boost::unordered_map<std::size_t, std::size_t> > global_to_local;

  // Pack global indices for sending to sharing processes
  boost::unordered_map<std::size_t, std::vector<std::size_t> > send_indices;
  boost::unordered_map<std::size_t, std::vector<std::size_t> > local_sent_indices;
  std::map<std::size_t, std::set<std::size_t> >::const_iterator shared_entity;
  for (shared_entity = shared_entities.begin(); shared_entity != shared_entities.end(); ++shared_entity)
  {
    // Local index
    const std::size_t local_index = shared_entity->first;

    // Global index
    dolfin_assert(local_index < global_indices_map.size());
    std::size_t global_index = global_indices_map[local_index];

    // Destinarion process
    const std::set<std::size_t>& sharing_processes = shared_entity->second;

    // Pack data for sending and build global-to-local map
    std::set<std::size_t>::const_iterator dest;
    for (dest = sharing_processes.begin(); dest != sharing_processes.end(); ++dest)
    {
      send_indices[*dest].push_back(global_index);
      local_sent_indices[*dest].push_back(local_index);
      global_to_local[*dest].insert(std::make_pair(global_index, local_index));
    }
  }

  // Communicators
  MPICommunicator mpi_comm;
  boost::mpi::communicator comm(*mpi_comm, boost::mpi::comm_attach);

  // Send/receive global indices
  std::vector<boost::mpi::request> reqs;
  boost::unordered_map<std::size_t, std::vector<std::size_t> > recv_entities;
  boost::unordered_map<std::size_t, std::vector<std::size_t> >::const_iterator global_indices;
  for (global_indices = send_indices.begin(); global_indices != send_indices.end(); ++global_indices)
  {
    const std::size_t destination = global_indices->first;
    reqs.push_back(comm.isend(destination, MPI::process_number(), global_indices->second));
    reqs.push_back(comm.irecv(destination, destination, recv_entities[destination]));
  }
  boost::mpi::wait_all(reqs.begin(), reqs.end());

  // Clear send data
  send_indices.clear();

  // Determine local entities indices for received global entity indices
  boost::unordered_map<std::size_t, std::vector<std::size_t> >::const_iterator received_global_indices;
  for (received_global_indices = recv_entities.begin();
      received_global_indices != recv_entities.end(); ++received_global_indices)
  {
    // Get process number of neighbour
    const std::size_t sending_proc = received_global_indices->first;

    // Get global-to-local map for neighbour process
    boost::unordered_map<std::size_t, boost::unordered_map<std::size_t, std::size_t> >::const_iterator
      it = global_to_local.find(sending_proc);
    dolfin_assert(it != global_to_local.end());
    const boost::unordered_map<std::size_t, std::size_t>& neighbour_global_to_local = it->second;

    // Build vector of local indices
    const std::vector<std::size_t>& global_indices = received_global_indices->second;
    for (std::size_t i = 0; i < global_indices.size(); ++i)
    {
      // Global index
      const std::size_t global_index = global_indices[i];

      // Find local index corresponding to global index
      boost::unordered_map<std::size_t, std::size_t>::const_iterator it = neighbour_global_to_local.find(global_index);

      dolfin_assert(it != neighbour_global_to_local.end());
      const std::size_t my_local_index = it->second;
      send_indices[sending_proc].push_back(my_local_index);
    }
  }

  // Clear receive data for re-use
  recv_entities.clear();

  // Send back/receive local indices
  std::vector<boost::mpi::request> reqs1;
  boost::unordered_map<std::size_t, std::vector<std::size_t> >::const_iterator local_indices;
  for (local_indices = send_indices.begin(); local_indices != send_indices.end(); ++local_indices)
  {
    const std::size_t destination = local_indices->first;
    reqs1.push_back(comm.isend(destination, MPI::process_number(), local_indices->second));
    reqs1.push_back(comm.irecv(destination, destination, recv_entities[destination]));
  }
  boost::mpi::wait_all(reqs1.begin(), reqs1.end());


  // Build map
  boost::unordered_map<std::size_t, std::vector<std::pair<std::size_t, std::size_t> > > shared_local_indices_map;

  // Loop over data received from each process
  boost::unordered_map<std::size_t, std::vector<std::size_t> >::const_iterator received_local_indices;
  for (received_local_indices = recv_entities.begin();
          received_local_indices != recv_entities.end(); ++received_local_indices)
  {
    // Process that shares entities
    const std::size_t proc = received_local_indices->first;

    // Local indices on sharing process
    const std::vector<std::size_t>& neighbour_local_indices = received_local_indices->second;

    // Local indices on this process
    dolfin_assert(local_sent_indices.find(proc) != local_sent_indices.end());
    const std::vector<std::size_t>& my_local_indices = local_sent_indices.find(proc)->second;

    // Check that sizes match
    dolfin_assert(my_local_indices.size() == neighbour_local_indices.size());

    for (std::size_t i = 0; i < neighbour_local_indices.size(); ++i)
      shared_local_indices_map[my_local_indices[i]].push_back(std::make_pair(proc, neighbour_local_indices[i]));
  }

  return shared_local_indices_map;
}
//-----------------------------------------------------------------------------
void MeshDistributed::compute_entity_ownership(const Mesh& mesh, std::size_t d,
      std::vector<std::size_t>& owned_entities,
      boost::array<std::map<Entity, EntityData>, 2>& shared_entities)
{
  // Initialize entities of dimension d
  mesh.init(d);

  // Build entity global vertex list -to- local entity index map
  std::map<std::vector<std::size_t>, std::size_t> entities;
  for (MeshEntityIterator e(mesh, d); !e.end(); ++e)
  {
    std::vector<std::size_t> entity;
    for (VertexIterator vertex(*e); !vertex.end(); ++vertex)
      entity.push_back(vertex->global_index());
    std::sort(entity.begin(), entity.end());
    entities[entity] = e->index();
  }

  // Get shared vertices (local index, [sharing processes])
  const std::map<std::size_t, std::set<std::size_t> >& shared_vertices_local
                            = mesh.topology().shared_entities(0);

  // Build local-to-global indices map for shared vertices
  const std::vector<std::size_t>& global_indices_map = mesh.topology().global_indices(0);
  std::map<std::size_t, std::set<std::size_t> > shared_vertices;
  std::map<std::size_t, std::set<std::size_t> >::const_iterator v;
  for (v = shared_vertices_local.begin(); v != shared_vertices_local.end(); ++v)
  {
    dolfin_assert(v->first < global_indices_map.size());
    shared_vertices.insert(std::make_pair(global_indices_map[v->first], v->second));
  }

  // Entity ownership list ([entity vertices], data):
  //  [0]: owned and shared (will be numbered by this process, and number
  //       communicated to other processes)
  //  [1]: not owned but shared (will be numbered by another process, and number
  //       communicated to this processes)

  // Compute preliminary ownership lists (shared_entities)
  compute_preliminary_entity_ownership(shared_vertices, entities,
                                       owned_entities, shared_entities);

  // Qualify boundary entities. We need to find out if the ignored
  // (shared with lower ranked process) entities are entities of a
  // lower ranked process.  If not, this process becomes the lower
  // ranked process for the entity in question, and is therefore
  // responsible for communicating values to the higher ranked
  // processes (if any).
  compute_final_entity_ownership(owned_entities, shared_entities);
}
//-----------------------------------------------------------------------------
void MeshDistributed::compute_preliminary_entity_ownership(
  const std::map<std::size_t, std::set<std::size_t> >& shared_vertices,
  const std::map<Entity, std::size_t>& entities,
  std::vector<std::size_t>& owned_entities,
  boost::array<std::map<Entity, EntityData>, 2>& shared_entities)
{
  // Entities
  std::map<Entity, EntityData>& owned_shared_entities = shared_entities[0];
  std::map<Entity, EntityData>& unowned_shared_entities = shared_entities[1];

  // Clear maps
  owned_entities.clear();
  owned_shared_entities.clear();
  unowned_shared_entities.clear();

  // Get my process number
  const std::size_t process_number = MPI::process_number();

  // Iterate over all local entities
  std::map<std::vector<std::size_t>, std::size_t>::const_iterator it;
  for (it = entities.begin(); it != entities.end(); ++it)
  {
    const Entity& entity = it->first;
    const std::size_t local_entity_index = it->second;

    // Compute which processes entity is shared with
    std::vector<std::size_t> entity_processes;
    if (is_shared(entity, shared_vertices))
    {
      // Processes sharing first vertex of entity
      std::vector<std::size_t> intersection(shared_vertices.find(entity[0])->second.begin(),
                                            shared_vertices.find(entity[0])->second.end());
      std::vector<std::size_t>::iterator intersection_end = intersection.end();

      // Loop over entity vertices
      for (std::size_t i = 1; i < entity.size(); ++i)
      {
        // Global vertex index
        const std::size_t v = entity[i];

        // Sharing processes
        const std::set<std::size_t>& shared_vertices_v
          = shared_vertices.find(v)->second;

        intersection_end
          = std::set_intersection(intersection.begin(), intersection_end,
                                  shared_vertices_v.begin(), shared_vertices_v.end(),
                                  intersection.begin());
      }
      entity_processes = std::vector<std::size_t>(intersection.begin(), intersection_end);
    }

    // Check if entity is ignored (shared with lower ranked process)
    bool ignore = false;
    for (std::size_t i = 0; i < entity_processes.size(); ++i)
    {
      if (entity_processes[i] < process_number)
      {
        ignore = true;
        break;
      }
    }

    // Check cases
    if (entity_processes.empty())
      owned_entities.push_back(local_entity_index);
    else if (ignore)
    {
      unowned_shared_entities[entity] = EntityData(local_entity_index,
                                                   entity_processes);
    }
    else
    {
      owned_shared_entities[entity] = EntityData(local_entity_index,
                                                 entity_processes);
    }
  }
}
//-----------------------------------------------------------------------------
void MeshDistributed::compute_final_entity_ownership(std::vector<std::size_t>& owned_entities,
  boost::array<std::map<Entity, EntityData>, 2>& shared_entities)
{
  // Entities ([entity vertices], index) to be numbered
  std::map<Entity, EntityData>& owned_shared_entities = shared_entities[0];
  std::map<Entity, EntityData>& unowned_shared_entities = shared_entities[1];

  // Get MPI process number
  const std::size_t process_number = MPI::process_number();

  // Convenience iterator
  std::map<Entity, EntityData>::const_iterator it;

  // Communicate common entities, starting with the entities we think
  // are shared but not owned
  std::vector<std::size_t> send_common_entity_values;
  std::vector<std::size_t> destinations_common_entity;
  for (it = unowned_shared_entities.begin(); it != unowned_shared_entities.end(); ++it)
  {
    // Get entity vertices (global vertex indices)
    const Entity& entity = it->first;

    // Get entity processes (processes that might share the entity)
    const std::vector<std::size_t>& entity_processes = it->second.processes;

    // Prepare data for sending
    for (std::size_t j = 0; j < entity_processes.size(); ++j)
    {
      const std::size_t p = entity_processes[j];
      send_common_entity_values.push_back(entity.size());
      send_common_entity_values.insert(send_common_entity_values.end(), entity.begin(), entity.end());
      destinations_common_entity.insert(destinations_common_entity.end(), entity.size() + 1, p);
    }
  }

  // Communicate common entities, add the entities we think are owned
  // and shared
  for (it = owned_shared_entities.begin(); it != owned_shared_entities.end(); ++it)
  {
    // Get entity vertices (global vertex indices)
    const Entity& entity = it->first;

    // Get entity processes (processes that might share the entity)
    const std::vector<std::size_t>& entity_processes = it->second.processes;

    // Prepare data for sending
    for (std::size_t j = 0; j < entity_processes.size(); ++j)
    {
      const std::size_t p = entity_processes[j];
      dolfin_assert(process_number < p);
      send_common_entity_values.push_back(entity.size());
      send_common_entity_values.insert(send_common_entity_values.end(), entity.begin(), entity.end());
      destinations_common_entity.insert(destinations_common_entity.end(), entity.size() + 1, p);
    }
  }

  // Communicate common entities
  std::vector<std::size_t> received_common_entity_values;
  std::vector<std::size_t> sources_common_entity;
  MPI::distribute(send_common_entity_values, destinations_common_entity,
                  received_common_entity_values, sources_common_entity);

  // Check if entities received are really entities
  std::vector<std::size_t> send_is_entity_values;
  std::vector<std::size_t> destinations_is_entity;
  for (std::size_t i = 0; i < received_common_entity_values.size();)
  {
    // Get entity
    const std::size_t p =  sources_common_entity[i];
    const std::size_t entity_size = received_common_entity_values[i++];
    Entity entity;
    for (std::size_t j = 0; j < entity_size; ++j)
      entity.push_back(received_common_entity_values[i++]);

    // Check if it is an entity (in which case it will be in owned or
    // unowned entities)
    bool is_entity = false;
    if (unowned_shared_entities.find(entity) != unowned_shared_entities.end()
          || owned_shared_entities.find(entity) != owned_shared_entities.end())
    {
      is_entity = true;
    }

    // Add information about entity (whether it's actually an entity) to send
    // to other processes
    send_is_entity_values.push_back(entity_size);
    destinations_is_entity.push_back(p);
    for (std::size_t j = 0; j < entity_size; ++j)
    {
      send_is_entity_values.push_back(entity[j]);
      destinations_is_entity.push_back(p);
    }
    send_is_entity_values.push_back(is_entity);
    destinations_is_entity.push_back(p);
  }

  // Send data back (list of requested entities that are really entities)
  std::vector<std::size_t> received_is_entity_values;
  std::vector<std::size_t> sources_is_entity;
  MPI::distribute(send_is_entity_values, destinations_is_entity,
                  received_is_entity_values, sources_is_entity);

  // Create map from entities to processes where it is an entity
  std::map<Entity, std::vector<std::size_t> > entity_processes;
  for (std::size_t i = 0; i < received_is_entity_values.size();)
  {
    const std::size_t p = sources_is_entity[i];
    const std::size_t entity_size = received_is_entity_values[i++];
    Entity entity;
    for (std::size_t j = 0; j < entity_size; ++j)
      entity.push_back(received_is_entity_values[i++]);
    const std::size_t is_entity = received_is_entity_values[i++];
    if (is_entity == 1)
    {
      // Add entity since it is actually an entity for process p
      entity_processes[entity].push_back(p);
    }
  }

  // Fix the list of entities we do not own (numbered by lower ranked
  // process)
  std::vector<std::vector<std::size_t> > unignore_entities;
  std::map<Entity, EntityData>::iterator entity;
  for (entity = unowned_shared_entities.begin(); entity != unowned_shared_entities.end(); ++entity)
  {
    const Entity& entity_vertices = entity->first;
    EntityData& entity_data = entity->second;
    const std::size_t local_entity_index = entity_data.local_index;
    if (entity_processes.find(entity_vertices) != entity_processes.end())
    {
      const std::vector<std::size_t>& common_processes = entity_processes[entity_vertices];
      dolfin_assert(!common_processes.empty());
      const std::size_t min_proc = *(std::min_element(common_processes.begin(), common_processes.end()));

      if (process_number < min_proc)
      {
        // Move from unowned to owned
        owned_shared_entities[entity_vertices] = EntityData(local_entity_index,
                                                            common_processes);

        // Add entity to list of entities that should be removed from
        // the unowned entity list.
        unignore_entities.push_back(entity_vertices);
      }
      else
        entity_data.processes = common_processes;
    }
    else
    {
      // Move from unowned to owned exclusively
      owned_entities.push_back(local_entity_index);

      // Add entity to list of entities that should be removed from the
      // shared but not owned entity list
      unignore_entities.push_back(entity_vertices);
    }
  }

  // Remove unowned shared entities that should not be shared
  for (std::size_t i = 0; i < unignore_entities.size(); ++i)
    unowned_shared_entities.erase(unignore_entities[i]);

  // Fix the list of entities we share
  std::vector<std::vector<std::size_t> > unshare_entities;
  for (std::map<Entity, EntityData>::iterator it = owned_shared_entities.begin();
         it != owned_shared_entities.end(); ++it)
  {
    const Entity& entity = it->first;

    const std::size_t local_entity_index = it->second.local_index;
    if (entity_processes.find(entity) == entity_processes.end())
    {
      // Move from shared to owned elusively
      owned_entities.push_back(local_entity_index);
      unshare_entities.push_back(entity);
    }
    else
    {
      // Update processor list of shared entities
      it->second.processes = entity_processes[entity];
    }
  }

  // Remove shared entities that should not be shared
  for (std::size_t i = 0; i < unshare_entities.size(); ++i)
    owned_shared_entities.erase(unshare_entities[i]);
}
//-----------------------------------------------------------------------------
bool MeshDistributed::is_shared(const Entity& entity,
         const std::map<std::size_t, std::set<std::size_t> >& shared_vertices)
{
  // Iterate over entity vertices
  Entity::const_iterator e;
  for (e = entity.begin(); e != entity.end(); ++e)
  {
    // Return false if an entity vertex is not in the list (map) of
    // shared entities
    if (shared_vertices.find(*e) == shared_vertices.end())
      return false;
  }
  return true;
}
//-----------------------------------------------------------------------------
std::pair<std::size_t, std::size_t>
  MeshDistributed::compute_num_global_entities(std::size_t num_local_entities,
                                                std::size_t num_processes,
                                                std::size_t process_number)
{
  // Communicate number of local entities
  std::vector<std::size_t> num_entities_to_number;
  MPI::all_gather(num_local_entities, num_entities_to_number);

  // Compute offset
  const std::size_t offset = std::accumulate(num_entities_to_number.begin(),
                           num_entities_to_number.begin() + process_number, 0);

  // Compute number of global entities
  const std::size_t num_global = std::accumulate(num_entities_to_number.begin(),
                                                 num_entities_to_number.end(), 0);

  return std::make_pair(num_global, offset);
}
//-----------------------------------------------------------------------------
void MeshDistributed::init_facet_cell_connections(Mesh& mesh)
{
  // Topological dimension
  const std::size_t D = mesh.topology().dim();

  // Initialize entities of dimension d
  mesh.init(D - 1);

  // Build entity(vertex list)-to-global-vertex-index map
  std::map<std::vector<std::size_t>, std::size_t> entities;
  for (MeshEntityIterator e(mesh, D - 1); !e.end(); ++e)
  {
    std::vector<std::size_t> entity;
    for (VertexIterator vertex(*e); !vertex.end(); ++vertex)
      entity.push_back(vertex->global_index());
    std::sort(entity.begin(), entity.end());
    entities[entity] = e->index();
  }

  // Compute ownership of entities ([entity vertices], data):
  //  [0]: owned and shared (will be numbered by this process, and number
  //       communicated to other processes)
  //  [1]: not owned but shared (will be numbered by another process, and number
  //       communicated to this processes)
  std::vector<std::size_t> owned_entities;
  boost::array<std::map<Entity, EntityData>, 2> entity_ownership;
  compute_entity_ownership(mesh, D - 1, owned_entities, entity_ownership);

  const std::map<Entity, EntityData>& owned_shared_entities   = entity_ownership[0];
  const std::map<Entity, EntityData>& unowned_shared_entities = entity_ownership[1];

  // Create vector to hold number of cells connected to each facet. Assume
  // facet is internal, then modify for external facets.
  std::vector<std::size_t> num_global_neighbors(mesh.num_facets(), 2);

  // Add facets that are locally connected to one cell only
  for (FacetIterator facet(mesh); !facet.end(); ++facet)
  {
    if (facet->num_entities(D) == 1)
      num_global_neighbors[facet->index()] = 1;
  }

  // Handle facets on internal partition boundaries
  std::map<Entity, EntityData>::const_iterator it;

  for (it = owned_shared_entities.begin(); it != owned_shared_entities.end(); ++it)
    num_global_neighbors[entities.find(it->first)->second] = 2;

  for (it = unowned_shared_entities.begin(); it != unowned_shared_entities.end(); ++it)
    num_global_neighbors[entities.find(it->first)->second] = 2;

  mesh.topology()(D - 1, mesh.topology().dim()).set_global_size(num_global_neighbors);
}
//-----------------------------------------------------------------------------
