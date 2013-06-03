// Copyright (C) 2012 Chris N. Richardson
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
// First added:  2012-05-22
// Last changed: 2013-05-14

#ifndef __DOLFIN_HDF5FILE_H
#define __DOLFIN_HDF5FILE_H

#ifdef HAS_HDF5

#include <string>
#include <utility>
#include <vector>

#include "dolfin/common/MPI.h"
#include "dolfin/common/Variable.h"
#include "HDF5Interface.h"

namespace dolfin
{

  class Function;
  class GenericVector;
  class LocalMeshData;
  class Mesh;
  template<typename T> class MeshFunction;
  template<typename T> class MeshValueCollection;

  class HDF5File : public Variable
  {
  public:

    /// Constructor. file_mode should "a" (append), "w" (write) ot "r"
    /// (read).
    HDF5File(const std::string filename, const std::string file_mode,
             bool use_mpiio = true);

    /// Destructor
    ~HDF5File();

    /// Write Vector to file in a format suitable for re-reading
    void write(const GenericVector& x, const std::string name);

    /// Read vector from file
    void read(GenericVector& x, const std::string dataset_name,
              const bool use_partition_from_file = true);


    /// Write Mesh to file in a format suitable for re-reading
    void write(const Mesh& mesh, const std::string name);

    /// Write Mesh of given cell dimension to file in a format
    /// suitable for re-reading
    void write(const Mesh& mesh, const std::size_t cell_dim,
               const std::string name);

    /// Read Mesh from file
    void read(Mesh& mesh, const std::string name);

    /// Write MeshFunction to file in a format suitable for re-reading
    void write(const MeshFunction<std::size_t>& meshfunction,
               const std::string name);

    /// Write MeshFunction to file in a format suitable for re-reading
    void write(const MeshFunction<int>& meshfunction, const std::string name);

    /// Write MeshFunction to file in a format suitable for re-reading
    void write(const MeshFunction<double>& meshfunction,
               const std::string name);

    /// Write MeshFunction to file in a format suitable for re-reading
    void write(const MeshFunction<bool>& meshfunction, const std::string name);

    /// Read MeshFunction from file
    void read(MeshFunction<std::size_t>& meshfunction, const std::string name);

    /// Read MeshFunction from file
    void read(MeshFunction<int>& meshfunction, const std::string name);

    /// Read MeshFunction from file
    void read(MeshFunction<double>& meshfunction, const std::string name);

    /// Read MeshFunction from file
    void read(MeshFunction<bool>& meshfunction, const std::string name);


    /// Write MeshValueCollection to file
    void write(const MeshValueCollection<std::size_t>& mesh_values,
               const std::string name);

    /// Write MeshValueCollection to file
    void write(const MeshValueCollection<double>& mesh_values,
               const std::string name);

    /// Write MeshValueCollection to file
    void write(const MeshValueCollection<bool>& mesh_values,
               const std::string name);

    /// Read MeshValueCollection from file
    void read(MeshValueCollection<std::size_t>& mesh_values,
              const std::string name);

    /// Read MeshValueCollection from file
    void read(MeshValueCollection<double>& mesh_values,
              const std::string name);

    /// Read MeshValueCollection from file
    void read(MeshValueCollection<bool>& mesh_values,
              const std::string name);


    /// Check if dataset exists in HDF5 file
    bool has_dataset(const std::string dataset_name) const;

    /// Flush buffered I/O to disk
    void flush();

  private:

    // Friend
    friend class XDMFFile;
    friend class TimeSeriesHDF5;

    // Read a mesh and repartition (if running in parallel)
    void read_mesh_repartition(Mesh &input_mesh,
                               const std::string coordinates_name,
                               const std::string topology_name);

    // Write a MeshFunction to file
    template <typename T>
    void write_mesh_function(const MeshFunction<T>& meshfunction,
                             const std::string name);

    // Read a MeshFunction from file
    template <typename T>
    void read_mesh_function(MeshFunction<T>& meshfunction,
                            const std::string name);

    // Write a MeshValueCollection to file
    template <typename T>
    void write_mesh_value_collection(const MeshValueCollection<T>& mesh_values,
                                     const std::string name);

    // Read a MeshValueCollection from file
    template <typename T>
    void read_mesh_value_collection(MeshValueCollection<T>& mesh_values,
                                    const std::string name);

    // Write contiguous data to HDF5 data set. Data is flattened into
    // a 1D array, e.g. [x0, y0, z0, x1, y1, z1] for a vector in 3D
    template <typename T>
    void write_data(const std::string dataset_name,
                    const std::vector<T>& data,
                    const std::vector<std::size_t> global_size);

    // Reorder values into global order (used by XDMFFile when saving
    // vertex data)
    void reorder_values_by_global_indices(const Mesh& mesh,
                                 std::vector<double>& data,
                                 std::vector<std::size_t>& global_size) const;

    // HDF5 file descriptor/handle
    bool hdf5_file_open;
    hid_t hdf5_file_id;

    // Parallel mode
    const bool mpi_io;
  };

  //---------------------------------------------------------------------------
  template <typename T>
  void HDF5File::write_data(const std::string dataset_name,
                            const std::vector<T>& data,
                            const std::vector<std::size_t> global_size)
  {
    dolfin_assert(hdf5_file_open);

    dolfin_assert(global_size.size() > 0);

    // Get number of 'items'
    std::size_t num_local_items = 1;
    for (std::size_t i = 1; i < global_size.size(); ++i)
      num_local_items *= global_size[i];
    num_local_items = data.size()/num_local_items;

    // Compute offset
    const std::size_t offset = MPI::global_offset(num_local_items, true);
    std::pair<std::size_t, std::size_t> range(offset,
                                              offset + num_local_items);

    const bool chunking = parameters["chunking"];
    // Write data to HDF5 file
    HDF5Interface::write_dataset(hdf5_file_id, dataset_name, data,
                                 range, global_size, mpi_io, chunking);
  }
  //---------------------------------------------------------------------------

}

#endif
#endif
