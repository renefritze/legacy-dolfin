// Copyright (C) 2012 Chris N Richardson
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
// Modified by Garth N. Wells, 2012
//
// First added:  2012-06-01
// Last changed: 2012-11-16

#ifdef HAS_HDF5

#include <cstdio>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/assign.hpp>
#include <boost/multi_array.hpp>
#include <boost/bind.hpp>

#include <dolfin/common/types.h>
#include <dolfin/common/constants.h>
#include <dolfin/common/MPI.h>
#include <dolfin/common/NoDeleter.h>
#include <dolfin/common/Timer.h>
#include <dolfin/function/Function.h>
#include <dolfin/la/GenericVector.h>
#include <dolfin/log/log.h>
#include <dolfin/mesh/Cell.h>
#include <dolfin/mesh/LocalMeshData.h>
#include <dolfin/mesh/Mesh.h>
#include <dolfin/mesh/MeshPartitioning.h>
#include <dolfin/mesh/MeshEntityIterator.h>
#include <dolfin/mesh/MeshFunction.h>
#include <dolfin/mesh/Vertex.h>

#include "HDF5File.h"
#include "HDF5Interface.h"

using namespace dolfin;

//-----------------------------------------------------------------------------
HDF5File::HDF5File(const std::string filename, const bool use_mpiio)
  : GenericFile(filename, "H5"), hdf5_file_open(false), hdf5_file_id(0),
    mpi_io(MPI::num_processes() > 1 && use_mpiio ? true : false)
{
  // HDF5 chunking
  parameters.add("chunking", false);
  // Optional duplicate vertex suppression for H5 Mesh output
  parameters.add("remove_duplicates", true);
}
//-----------------------------------------------------------------------------
HDF5File::~HDF5File()
{
  // Close HDF5 file
  if (hdf5_file_open)
  {
    herr_t status = H5Fclose(hdf5_file_id);
    dolfin_assert(status != HDF5_FAIL);
  }
}
//-----------------------------------------------------------------------------
void HDF5File::flush()
{
  dolfin_assert(hdf5_file_open);
  HDF5Interface::flush_file(hdf5_file_id);
}
//-----------------------------------------------------------------------------
void HDF5File::operator<< (const GenericVector& x)
{
  dolfin_assert(x.size() > 0);

  // Open file on first write and add Vector group (overwrite any existing file)
  if (counter == 0)
  {
    // Open file
    dolfin_assert(!hdf5_file_open);
    hdf5_file_id = HDF5Interface::open_file(filename, true, mpi_io);
    hdf5_file_open = true;

    // Add group
    HDF5Interface::add_group(hdf5_file_id, "/Vector");
  }
  dolfin_assert(HDF5Interface::has_group(hdf5_file_id, "/Vector"));

  // Get all local data
  std::vector<double> local_data;
  x.get_local(local_data);

  // Form HDF5 dataset tag
  const std::string dataset_name
      = "/Vector/" + boost::lexical_cast<std::string>(counter);

  // Write data to file
  std::pair<std::size_t, std::size_t> local_range = x.local_range();
  const bool chunking = parameters["chunking"];
  const std::vector<std::size_t> global_size(1, x.size());
  HDF5Interface::write_dataset(hdf5_file_id, dataset_name, local_data,
                               local_range, global_size, mpi_io, chunking);

  // Add partitioning attribute to dataset
  std::vector<std::size_t> partitions;
  MPI::gather(local_range.first, partitions);
  MPI::broadcast(partitions);

  HDF5Interface::add_attribute(hdf5_file_id, dataset_name, "partition",
                               partitions);

  // Increment counter
  counter++;
}
//-----------------------------------------------------------------------------
void HDF5File::operator>> (GenericVector& x)
{
  // Open file
  if (!hdf5_file_open)
  {
    dolfin_assert(!hdf5_file_open);
    hdf5_file_id = HDF5Interface::open_file(filename, false, mpi_io);
    hdf5_file_open = true;
  }

  // Check that 'Vector' group exists
  dolfin_assert(HDF5Interface::has_group(hdf5_file_id, "/Vector") == 1);

  // Get list all datasets in group
  const std::vector<std::string> datasets
      = HDF5Interface::dataset_list(hdf5_file_id, "/Vector");

  // Make sure there is only one dataset
  dolfin_assert(datasets.size() == 1);

  // Read data set
  read("/Vector/" + datasets[0], x);
}
//-----------------------------------------------------------------------------
void HDF5File::read(const std::string dataset_name, GenericVector& x,
                    const bool use_partition_from_file)
{
  // Open HDF5 file
  if (!hdf5_file_open)
  {
    // Open file
    dolfin_assert(!hdf5_file_open);
    hdf5_file_id = HDF5Interface::open_file(filename, false, mpi_io);
    hdf5_file_open = true;
  }
  dolfin_assert(HDF5Interface::has_group(hdf5_file_id, dataset_name));

  // Get dataset rank
  const uint rank = HDF5Interface::dataset_rank(hdf5_file_id, dataset_name);
  dolfin_assert(rank == 1);

  // Get global dataset size
  const std::vector<std::size_t> data_size
      = HDF5Interface::get_dataset_size(hdf5_file_id, dataset_name);

  // Check that rank is 1
  dolfin_assert(data_size.size() == 1);

  // Check input vector, and re-size if not already sized
  if (x.size() == 0)
  {
    // Resize vector
    if (use_partition_from_file)
    {
      // Get partition from file
      std::vector<std::size_t> partitions;
      HDF5Interface::get_attribute(hdf5_file_id, dataset_name, "partition", partitions);

      // Check that number of MPI processes matches partitioning
      if(MPI::num_processes() != partitions.size())
      {
        dolfin_error("HDF5File.cpp",
                     "read vector from file",
                     "Different number of processes used when writing. Cannot restore partitioning");
      }

      // Add global size at end of partition vectors
      partitions.push_back(data_size[0]);

      // Initialise vector
      const uint process_num = MPI::process_number();
      const std::pair<std::size_t, std::size_t>
          local_range(partitions[process_num], partitions[process_num + 1]);
      x.resize(local_range);
    }
    else
      x.resize(data_size[0]);
  }
  else if (x.size() != data_size[0])
  {
    dolfin_error("HDF5File.cpp",
                 "read vector from file",
                 "Size mis-match between vector in file and input vector");
  }

  // Get local range
  const std::pair<std::size_t, std::size_t> local_range = x.local_range();

  // Read data from file
  std::vector<double> data;
  HDF5Interface::read_dataset(hdf5_file_id, dataset_name, local_range, data);

  // Set data
  x.set_local(data);
}
//-----------------------------------------------------------------------------
std::string HDF5File::search_list(const std::vector<std::string>& list,
                                  const std::string& search_term)
{
  std::vector<std::string>::const_iterator it;
  for (it = list.begin(); it != list.end(); ++it)
  {
    if (it->find(search_term) != std::string::npos)
      return *it;
  }
  return std::string("");
}
//-----------------------------------------------------------------------------
void HDF5File::operator>> (Mesh& input_mesh)
{

  // Open file if not already open
  if (!hdf5_file_open)
  {
    dolfin_assert(!hdf5_file_open);
    hdf5_file_id = HDF5Interface::open_file(filename, false, mpi_io);
    hdf5_file_open = true;
  }

  // Check that 'Mesh' group exists
  if(!HDF5Interface::has_group(hdf5_file_id, "/Mesh"))
  {
    dolfin_error("HDF5File.cpp",
                 "open HDF5 /Mesh group",
                 "HDF5 file does not contain a suitable Mesh");
  }
  
  // Get list of all datasets in the /Mesh group
  std::vector<std::string> _dataset_list
      = HDF5Interface::dataset_list(hdf5_file_id, "/Mesh");
  
  // Look for one dataset group, which usually will be "/Mesh/0"

  if(_dataset_list.size() == 0)
  {
    dolfin_error("HDF5File.cpp",
                 "find Mesh",
                 "Empty /Mesh group");
  }
  
  if(_dataset_list.size() != 1)
  {
    warning("Multiple Mesh datasets found. Using first dataset.");
  }

  read_mesh(input_mesh, "/Mesh/" + _dataset_list[0]);
}
//-----------------------------------------------------------------------------
void HDF5File::read_mesh(Mesh& input_mesh,const std::string name)
{
  warning("HDF5 Mesh input is still experimental");
  warning("HDF5 Mesh input will always repartition the mesh");

  // Open file if not already open
  if (!hdf5_file_open)
  {
    dolfin_assert(!hdf5_file_open);
    hdf5_file_id = HDF5Interface::open_file(filename, false, mpi_io);
    hdf5_file_open = true;
  } 
  
  std::vector<std::string> _dataset_list = 
    HDF5Interface::dataset_list(hdf5_file_id, name);
      
  std::string topology_name = search_list(_dataset_list,"topology");
  if (topology_name.size() == 0)
  {
    dolfin_error("HDF5File.cpp", 
                 "read topology dataset",
                 "Dataset not found");
  }
  topology_name = name + "/" + topology_name;

  // Look for global_index dataset
  std::string global_index_name = search_list(_dataset_list,"global_index");
  if (global_index_name.size() == 0)
  {
    dolfin_error("HDF5File.cpp",
                 "read global index dataset",
                 "Dataset not found");
  }
  global_index_name = name + "/" + global_index_name;

  // Look for Coordinates dataset
  std::string coordinates_name=search_list(_dataset_list,"coordinates");
  if(coordinates_name.size()==0)
  {
    dolfin_error("HDF5File.cpp",
                 "read coordinates dataset",
                 "Dataset not found");
  }
  coordinates_name = name + "/" + coordinates_name;

  read_mesh_repartition(input_mesh, coordinates_name, global_index_name,
                                   topology_name);
}
//-----------------------------------------------------------------------------
void HDF5File::read_mesh_repartition(Mesh& input_mesh,
                                     const std::string coordinates_name,
                                     const std::string global_index_name,
                                     const std::string topology_name)
{
  // FIXME:
  // This function is experimental, and not checked or optimised

  warning("HDF5 Mesh read is still experimental");
  warning("HDF5 Mesh read will repartition this mesh");

  // Structure to store local mesh
  LocalMeshData mesh_data;
  mesh_data.clear();

  // --- Topology ---
  // Discover size of topology dataset
  std::vector<std::size_t> topology_dim
      = HDF5Interface::get_dataset_size(hdf5_file_id, topology_name);

  // Get total number of cells, as number of rows in topology dataset
  const std::size_t num_global_cells = topology_dim[0];
  mesh_data.num_global_cells = num_global_cells;

  // Set vertices-per-cell from number of columns
  const uint num_vertices_per_cell = topology_dim[1];
  mesh_data.num_vertices_per_cell = num_vertices_per_cell;
  mesh_data.tdim = topology_dim[1] - 1;

  // Divide up cells ~equally between processes
  const std::pair<std::size_t,std::size_t> cell_range = MPI::local_range(num_global_cells);
  const std::size_t num_local_cells = cell_range.second - cell_range.first;

  // Read a block of cells
  std::vector<std::size_t> topology_data;
  topology_data.reserve(num_local_cells*num_vertices_per_cell);
  mesh_data.cell_vertices.resize(boost::extents[num_local_cells][num_vertices_per_cell]);
  HDF5Interface::read_dataset(hdf5_file_id, topology_name, cell_range, topology_data);

  mesh_data.global_cell_indices.reserve(num_local_cells);

  for(std::size_t i = 0; i < num_local_cells; i++)
    mesh_data.global_cell_indices.push_back(cell_range.first + i);

  // Copy to boost::multi_array
  std::copy(topology_data.begin(), topology_data.end(), 
            mesh_data.cell_vertices.data());

  // --- Coordinates ---
  // Get dimensions of coordinate dataset
  std::vector<std::size_t> coords_dim
    = HDF5Interface::get_dataset_size(hdf5_file_id, coordinates_name);
  mesh_data.num_global_vertices = coords_dim[0];
  const uint vertex_dim = coords_dim[1];
  mesh_data.gdim = vertex_dim;

  // Divide range into equal blocks for each process
  const std::pair<std::size_t, std::size_t> vertex_range = MPI::local_range(coords_dim[0]);
  const std::size_t num_local_vertices = vertex_range.second - vertex_range.first;

  // Read vertex data to temporary vector

  std::vector<double> tmp_vertex_data;
  tmp_vertex_data.reserve(num_local_vertices*vertex_dim);
  HDF5Interface::read_dataset(hdf5_file_id, coordinates_name, vertex_range,
                              tmp_vertex_data);
  
  // Copy to vector<vector>
  // FIXME: improve
  std::vector<std::vector<double> > vertex_coordinates;
  vertex_coordinates.reserve(num_local_vertices);
  for(std::vector<double>::iterator v = tmp_vertex_data.begin();
      v != tmp_vertex_data.end(); v += vertex_dim)
  {
    vertex_coordinates.push_back(std::vector<double>(v, v + vertex_dim));
  }

  // Fill vertex indices with values - 
  mesh_data.vertex_indices.resize(num_local_vertices);

  HDF5Interface::read_dataset(hdf5_file_id, global_index_name, vertex_range,
                              mesh_data.vertex_indices);

  // MeshPartitioning::build_distributed_mesh() does not 
  // use the vertex indices values, so need to sort
  // vertices into global order before calling it.
  
  redistribute_by_global_index(mesh_data.vertex_indices, vertex_coordinates, mesh_data.vertex_coordinates);

  // redistribute_by_global_index() has eliminated duplicates, 
  // so need to resize global total
  mesh_data.num_global_vertices = MPI::sum(mesh_data.vertex_coordinates.size());
  
  // FIXME: Should put global index back here - not used at present
  //  for(std::size_t i = 0; i < mesh_data.vertex_coordinates.size(); ++i)
  //    mesh_data.vertex_indices[i] = vertex_range.first + i;

  // Build distributed mesh
  MeshPartitioning::build_distributed_mesh(input_mesh, mesh_data);
}
//-----------------------------------------------------------------------------
void HDF5File::operator<< (const Mesh& mesh)
{
  const std::string name = "/Mesh/" + boost::lexical_cast<std::string>(counter);
  write_mesh_global_index(mesh, mesh.topology().dim(), name);
  counter++;
}
//-----------------------------------------------------------------------------
void HDF5File::write_mesh(const Mesh& mesh, const std::string name)
{
  write_mesh_global_index(mesh, mesh.topology().dim(), name);
}
//-----------------------------------------------------------------------------
void HDF5File::write_mesh_global_index(const Mesh& mesh, uint cell_dim, const std::string name)
{

  warning("Writing mesh with global index - not suitable for visualisation");
  
  // Clear file when writing to file for the first time
  if(!hdf5_file_open)
  {
    hdf5_file_id = HDF5Interface::open_file(filename, false, mpi_io);
    hdf5_file_open = true;
  }

  // Create Mesh group in HDF5 file
  if (!HDF5Interface::has_group(hdf5_file_id, "/Mesh"))
    HDF5Interface::add_group(hdf5_file_id, "/Mesh");

  //const CellType::Type _cell_type = mesh.type().cell_type();
  CellType::Type _cell_type = mesh.type().cell_type();
  if (cell_dim == mesh.topology().dim())
    _cell_type = mesh.type().cell_type();
  else if (cell_dim == mesh.topology().dim() - 1)
    _cell_type = mesh.type().facet_type();
  else
  {
    dolfin_error("HDF5File.cpp",
                 "write mesh to file",
                 "Only Mesh for Mesh facets can be written to file");
  }

  const std::string cell_type = CellType::type2string(_cell_type);

  // ------ Output vertex coordinates and global index

  // Vertex numbers, ranges and offsets

  // Write vertex data to HDF5 file
  const std::string coord_dataset = name + "/coordinates";
  const std::string index_dataset = name + "/global_index";
  const std::vector<std::size_t>& global_indices = mesh.topology().global_indices(0);

  // Optionally reduce file size by deleting duplicates in vertex list.
  if(parameters["remove_duplicates"])
  {
    const uint gdim = mesh.geometry().dim();
    // Copy coordinates and indices and remove off-process values
    std::vector<double> vertex_coords(mesh.coordinates()); 
    remove_duplicate_values(mesh, vertex_coords, gdim);

    std::vector<std::size_t> vertex_indices(global_indices);
    remove_duplicate_values(mesh, vertex_indices, 1);

    // Write coordinates contiguously from each process
    std::vector<std::size_t> global_size(2);
    global_size[0] = MPI::sum(vertex_indices.size()); // reduced
    global_size[1] = gdim;
    write_data(coord_dataset, vertex_coords, global_size);
    global_size.resize(1); //remove second dimension
    write_data(index_dataset, vertex_indices, global_size);
  }
  else //just output in blocks - faster and less memory intensive
  {
    const uint gdim = mesh.geometry().dim();
    const std::vector<double>& vertex_coords = mesh.coordinates();

    // Write coordinates contiguously from each process
    std::vector<std::size_t> global_size(2);
    global_size[0] = MPI::sum(mesh.num_vertices());
    global_size[1] = gdim;
    write_data(coord_dataset, vertex_coords, global_size);
    global_size.resize(1); //remove second dimension
    write_data(index_dataset, global_indices, global_size);
  }

  // ------ Topology

  // Get/build topology data
  std::vector<std::size_t> topological_data;
  if (cell_dim == mesh.topology().dim())
  {
    topological_data = mesh.cells();
    std::transform(topological_data.begin(), topological_data.end(),
                   topological_data.begin(),
    boost::bind<const std::size_t &>(&std::vector<std::size_t>::at,
                                     &global_indices, _1));
  }
  else
  {
    //    topological_data.reserve(mesh.num_cells()*(cell_dim - 1));
    topological_data.reserve(mesh.num_entities(cell_dim)*(cell_dim + 1));
    for (MeshEntityIterator c(mesh, cell_dim); !c.end(); ++c)
      for (VertexIterator v(*c); !v.end(); ++v)
        topological_data.push_back(v->global_index());
  }

  // Write topology data
  const std::string topology_dataset = name + "/topology";
  std::vector<std::size_t> global_size(2);
  global_size[0] = MPI::sum(topological_data.size()/(cell_dim + 1));
  global_size[1] = cell_dim + 1;
  write_data(topology_dataset, topological_data, global_size);
  
  HDF5Interface::add_attribute(hdf5_file_id, topology_dataset, "celltype",
                               cell_type);

}
//-----------------------------------------------------------------------------
void HDF5File::write_visualisation_mesh(const Mesh& mesh, const std::string name)
{
  write_visualisation_mesh(mesh, mesh.topology().dim(), name);
}
//-----------------------------------------------------------------------------
void HDF5File::write_visualisation_mesh(const Mesh& mesh, const uint cell_dim,
                          const std::string name)
{
  // Clear file when writing to file for the first time
  if (!hdf5_file_open)
  {
    hdf5_file_id = HDF5Interface::open_file(filename, false, mpi_io);
    hdf5_file_open = true;
  }

  // Create VisualisationMesh group in HDF5 file
  if (!HDF5Interface::has_group(hdf5_file_id, "/VisualisationMesh"))
    HDF5Interface::add_group(hdf5_file_id, "/VisualisationMesh");

  CellType::Type _cell_type = mesh.type().cell_type();
  if (cell_dim == mesh.topology().dim())
    _cell_type = mesh.type().cell_type();
  else if (cell_dim == mesh.topology().dim() - 1)
    _cell_type = mesh.type().facet_type();
  else
  {
    dolfin_error("HDF5File.cpp",
                 "write mesh to file",
                 "Only Mesh for Mesh facets can be written to file");
  }

  // Cell type string
  const std::string cell_type = CellType::type2string(_cell_type);

  // Vertex numbers, ranges and offsets
  const std::size_t num_local_vertices = mesh.num_vertices();
  const std::size_t vertex_offset = MPI::global_offset(num_local_vertices, true);

  // Write vertex data to HDF5 file
  const std::string coord_dataset = name + "/coordinates";
  {
    const uint gdim = mesh.geometry().dim();
    const std::vector<double>& vertex_coords = mesh.coordinates();
    
    // Write coordinates contiguously from each process
    std::vector<std::size_t> global_size(2);
    global_size[0] = MPI::sum(num_local_vertices);
    global_size[1] = gdim;
    write_data(coord_dataset, vertex_coords, global_size);
  }

  // Write connectivity to HDF5 file (using local indices + offset)
  {
    // Get/build topology data
    std::vector<std::size_t> topological_data;
    if (cell_dim == mesh.topology().dim())
    {
      topological_data = mesh.cells();
      std::transform(topological_data.begin(), topological_data.end(),
                     topological_data.begin(),
                     std::bind2nd(std::plus<std::size_t>(), vertex_offset));
    }
    else
    {
    //    topological_data.reserve(mesh.num_cells()*(cell_dim - 1));
      topological_data.reserve(mesh.num_entities(cell_dim)*(cell_dim + 1));
      for (MeshEntityIterator c(mesh, cell_dim); !c.end(); ++c)
        for (VertexIterator v(*c); !v.end(); ++v)
         topological_data.push_back(v->index() + vertex_offset);
    }

    // Write topology data
    const std::string topology_dataset = name + "/topology";
    std::vector<std::size_t> global_size(2);
    global_size[0] = MPI::sum(topological_data.size()/(cell_dim + 1));
    global_size[1] = cell_dim + 1;
    write_data(topology_dataset, topological_data, global_size);

    HDF5Interface::add_attribute(hdf5_file_id, topology_dataset, "celltype",
                                 cell_type);
  }

  counter++;

}
//-----------------------------------------------------------------------------
bool HDF5File::has_dataset(const std::string dataset_name) const
{
  dolfin_assert(hdf5_file_open);
  return HDF5Interface::has_dataset(hdf5_file_id, dataset_name);
}
//-----------------------------------------------------------------------------
void HDF5File::open_hdf5_file(bool truncate)
{
  dolfin_assert(!hdf5_file_open);
  hdf5_file_id = HDF5Interface::open_file(filename, truncate, mpi_io);
  hdf5_file_open = true;
}
//-----------------------------------------------------------------------------

template <typename T>
void HDF5File::redistribute_by_global_index(const std::vector<std::size_t>& global_index,
                                            const std::vector<T>& local_vector,
                                            std::vector<T>& global_vector)
{
  dolfin_assert(local_vector.size() == global_index.size());

  Timer t("HDF5: Redistribute");

  // Get number of processes
  const uint num_processes = MPI::num_processes();

  // Calculate size of overall global vector by finding max index value
  // anywhere
  const uint global_vector_size
    = MPI::max(*std::max_element(global_index.begin(), global_index.end())) + 1;

  // Divide up the global vector into local chunks and distribute the
  // partitioning information
  std::pair<uint, uint> range = MPI::local_range(global_vector_size);
  std::vector<uint> partitions;
  MPI::gather(range.first, partitions);
  MPI::broadcast(partitions);
  partitions.push_back(global_vector_size); // add end of last partition

  // Go through each remote process number, finding local values with
  // a global index in the remote partition range, and add to a list.
  std::vector<std::vector<std::pair<uint,T> > > values_to_send(num_processes);
  std::vector<uint> destinations;
  destinations.reserve(num_processes);

  // Set up destination vector for communication with remote processes
  for(uint process_j = 0; process_j < num_processes ; ++process_j)
    destinations.push_back(process_j);

  // Go through local vector and append value to the appropriate list
  // to send to correct process
  for(uint i = 0; i < local_vector.size() ; ++i)
  {
    const uint global_i = global_index[i];

    // Identify process which needs this value, by searching through
    // partitioning
    const uint process_i
       = (uint)(std::upper_bound(partitions.begin(), partitions.end(), global_i) - partitions.begin()) - 1;

    if(global_i >= partitions[process_i] && global_i < partitions[process_i + 1])
    {
      // Send the global index along with the value
      values_to_send[process_i].push_back(make_pair(global_i,local_vector[i]));
    }
    else
    {
      dolfin_error("HDF5File.cpp",
                   "work out which process to send data to",
                   "This should not happen");
    }
  }

  // Redistribute the values to the appropriate process
  std::vector<std::vector<std::pair<uint,T> > > received_values;
  MPI::distribute(values_to_send, destinations, received_values);

  // When receiving, just go through all received values
  // and place them in global_vector, which is the local
  // partition of the global vector.
  global_vector.resize(range.second - range.first);
  for(uint i = 0; i < received_values.size(); ++i)
  {
    const std::vector<std::pair<uint, T> >& received_global_data = received_values[i];
    for(uint j = 0; j < received_global_data.size(); ++j)
    {
      const uint global_i = received_global_data[j].first;
      if(global_i >= range.first && global_i < range.second)
        global_vector[global_i - range.first] = received_global_data[j].second;
      else
      {
        dolfin_error("HDF5File.cpp",
                     "unpack values in vector redistribution",
                     "This should not happen");
      }
    }
  }
}
//-----------------------------------------------------------------------------
void HDF5File::remove_duplicate_vertices(const Mesh &mesh,
                                         std::vector<double>& vertex_data,
                                         std::vector<std::size_t>& topological_data)
{
  Timer t("remove duplicate vertices");

  const uint num_processes = MPI::num_processes();
  const uint process_number = MPI::process_number();
  const std::size_t num_local_vertices = mesh.num_vertices();

  const std::map<std::size_t, std::set<uint> >& shared_vertices
    = mesh.topology().shared_entities(0);

  // Create global => local map for shared vertices only
  std::map<std::size_t, std::size_t> local;
  for (VertexIterator v(mesh); !v.end(); ++v)
  {
    std::size_t global_index = v->global_index();
    if(shared_vertices.count(global_index) != 0)
      local[global_index] = v->index();
  }

  // New local indexing vector "remap" after removing duplicates
  // Initialise to '1' and mark removed vertices with '0'
  std::vector<std::size_t> remap(num_local_vertices, 1);

  // Structures for MPI::distribute
  // list all processes, though some may get nothing from here
  std::vector<uint> destinations;
  destinations.reserve(num_processes);
  for(uint j = 0; j < num_processes; j++)
    destinations.push_back(j);
  std::vector<std::vector<std::pair<uint, std::size_t> > > values_to_send(num_processes);

  // Go through shared vertices looking for vertices which are
  // on a lower numbered process. Mark these as being off-process.
  // Meanwhile, push locally owned shared vertices to values_to_send to
  // remote processes.

  std::size_t count = num_local_vertices;
  for(std::map<std::size_t, std::set<uint> >::const_iterator
      shared_v_it = shared_vertices.begin(); shared_v_it != shared_vertices.end();
      shared_v_it++)
  {
    const std::size_t global_index = shared_v_it->first;
    const std::size_t local_index = local[global_index];
    const std::set<uint>& procs = shared_v_it->second;
    // Determine whether this vertex is also on a lower numbered process
    // FIXME: may change with concept of vertex ownership
    if(*(procs.begin()) < process_number)
    {
      // mark for excision on this process
      remap[local_index] = 0;
      count--;
    }
    else // locally owned.
    {
      // send std::pair(global, local) indices to each sharing process
      const std::pair<std::size_t, std::size_t> global_local(global_index, local_index);
      for(std::set<uint>::iterator proc = procs.begin();
          proc != procs.end(); ++proc)
      {
        values_to_send[*proc].push_back(global_local);
      }
    }
  }

  // make vertex data
  const uint gdim = mesh.geometry().dim();
  vertex_data.clear();
  vertex_data.reserve(gdim*num_local_vertices);

  for (VertexIterator v(mesh); !v.end(); ++v)
  {
    if(remap[v->index()] != 0)
    {
      for (uint i = 0; i < gdim; ++i)
        vertex_data.push_back(v->x(i));
    }
  }
  //  std::cout << "total vertices = " << MPI::sum(vertex_data.size())/gdim << std::endl;

  // Remap local indices to account for missing vertices
  // Also add offset
  const std::size_t vertex_offset = MPI::global_offset(count, true);
  std::size_t new_index = vertex_offset - 1;
  for(std::size_t i = 0; i < num_local_vertices; i++)
  {
    new_index += remap[i]; // add either 1 or 0
    remap[i] = new_index;
  }

  // Second value of pairs contains local index. Now revise
  // to contain the new local index + vertex_offset
  for(std::vector<std::vector<std::pair<uint, std::size_t> > >::iterator
        p = values_to_send.begin(); p != values_to_send.end(); ++p)
  {
    for(std::vector<std::pair<uint, std::size_t> >::iterator lmap = p->begin();
          lmap != p->end(); ++lmap)
    {
      lmap->second = remap[lmap->second];
    }
  }

  // Redistribute the values to the appropriate process
  std::vector<std::vector<std::pair<uint, std::size_t> > > received_values;
  MPI::distribute(values_to_send, destinations, received_values);

  // flatten and insert received global remappings into remap
  std::vector<std::vector<std::pair<uint, std::size_t> > >::iterator p;
  for(p = received_values.begin(); p != received_values.end(); ++p)
  {
    std::vector<std::pair<uint, std::size_t> >::const_iterator lmap;
    for(lmap = p->begin(); lmap != p->end(); ++lmap)
      remap[local[lmap->first]] = lmap->second;
  }
  // remap should now contain the appropriate mapping
  // which can be used to reindex the topology

  const uint cell_dim = mesh.topology().dim(); // FIXME: facet mesh
  const std::size_t num_local_cells = mesh.num_cells();
  topological_data.clear();
  topological_data.reserve(num_local_cells*(cell_dim - 1));

  for (MeshEntityIterator c(mesh, cell_dim); !c.end(); ++c)
    for (VertexIterator v(*c); !v.end(); ++v)
      topological_data.push_back(remap[v->index()]);
}
//-----------------------------------------------------------------------------
 



#endif
