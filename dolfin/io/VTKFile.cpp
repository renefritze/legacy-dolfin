// Copyright (C) 2005-2009 Garth N. Wells.
// Licensed under the GNU LGPL Version 2.1.
//
// Modified by Anders Logg 2005-2006.
// Modified by Kristian Oelgaard 2006.
// Modified by Martin Alnes 2008.
// Modified by Niclas Jansson 2009.
//
// First added:  2005-07-05
// Last changed: 2009-07-05

#include <cmath>
#include <sstream>
#include <fstream>

#include <dolfin/mesh/Mesh.h>
#include <dolfin/mesh/MeshFunction.h>
#include <dolfin/mesh/Vertex.h>
#include <dolfin/mesh/Cell.h>
#include <dolfin/fem/FiniteElement.h>
#include <dolfin/fem/DofMap.h>
#include <dolfin/function/Function.h>
#include <dolfin/function/FunctionSpace.h>
#include <dolfin/la/Vector.h>
#include "VTKFile.h"


using namespace dolfin;

//----------------------------------------------------------------------------
VTKFile::VTKFile(const std::string filename) : GenericFile(filename)
{
  type = "VTK";
}
//----------------------------------------------------------------------------
VTKFile::~VTKFile()
{
  // Do nothing
}
//----------------------------------------------------------------------------
void VTKFile::operator<<(const Mesh& mesh)
{ 
  // Get vtu file name and intialise out files
  std::string vtu_filename = init(mesh);

  // Write mesh to vtu file
  mesh_write(mesh, vtu_filename);

  // Finalise and write pvd files
  finalize(vtu_filename);
 
  info(1, "Saved mesh %s (%s) to file %s in VTK format.",
          mesh.name().c_str(), mesh.label().c_str(), filename.c_str());
}
//----------------------------------------------------------------------------
void VTKFile::operator<<(const MeshFunction<int>& meshfunction)
{
  mesh_function_write(meshfunction);
}
//----------------------------------------------------------------------------
void VTKFile::operator<<(const MeshFunction<unsigned int>& meshfunction)
{
  mesh_function_write(meshfunction);
}
//----------------------------------------------------------------------------
void VTKFile::operator<<(const MeshFunction<double>& meshfunction)
{
  mesh_function_write(meshfunction);
}
//----------------------------------------------------------------------------
void VTKFile::operator<<(const Function& u)
{
  const Mesh& mesh = u.function_space().mesh();

  // Get vtu file name and intialise
  std::string vtu_filename = init(mesh);

  // Write Mesh
  mesh_write(mesh, vtu_filename);

  // Write results
  results_write(u, vtu_filename);

  // Finalise and write pvd files
  finalize(vtu_filename);

  info(1, "Saved function %s (%s) to file %s in VTK format.",
          u.name().c_str(), u.label().c_str(), filename.c_str());
}
//----------------------------------------------------------------------------
std::string VTKFile::init(const Mesh& mesh) const
{
  // Get vtu file name and clear file
  std::string vtu_filename = vtu_name(MPI::process_number(), MPI::num_processes(), counter, ".vtu");
  clear_file(vtu_filename);

  // Write headers
  vtk_header_open(mesh.num_vertices(), mesh.num_cells(), vtu_filename);
  
  return vtu_filename;
}
//----------------------------------------------------------------------------
void VTKFile::finalize(std::string vtu_filename)
{
  // Close headers
  vtk_header_close(vtu_filename);

  // Parallel-specfic files
  if (MPI::num_processes() > 1)
  { 
    if (MPI::process_number() == 0)
    {
      // Get pvtu file name and clear file
      std::string pvtu_filename = vtu_name(0, 0, counter, ".pvtu");
      clear_file(pvtu_filename);
      
      // Write pvtu file
      pvtu_file_write(pvtu_filename, vtu_filename);
      
      // Write pvd file (parallel)
      pvd_file_write(counter, pvtu_filename);  
    }
  }
  else
  {
    // Write pvd file (serial)
    pvd_file_write(counter, vtu_filename);  
  }

  // Increase the number of times we have saved the object
  counter++;
}
//----------------------------------------------------------------------------
void VTKFile::mesh_write(const Mesh& mesh, std::string vtu_filename) const
{
  // Open file
  FILE* fp = fopen(vtu_filename.c_str(), "a");
  if (!fp)
    error("Unable to open file %s", filename.c_str());

  // Write vertex positions
  fprintf(fp, "<Points>  \n");
  fprintf(fp, "<DataArray  type=\"Float64\"  NumberOfComponents=\"3\"  format=\"ascii\">  \n");
  for (VertexIterator v(mesh); !v.end(); ++v)
  {
    Point p = v->point();
    fprintf(fp," %f %f %f \n", p.x(), p.y(), p.z());
  }
  fprintf(fp, "</DataArray>  \n");
  fprintf(fp, "</Points>  \n");

  // Write cell connectivity
  fprintf(fp, "<Cells>  \n");
  fprintf(fp, "<DataArray  type=\"Int32\"  Name=\"connectivity\"  format=\"ascii\">  \n");
  for (CellIterator c(mesh); !c.end(); ++c)
  {
    for (VertexIterator v(*c); !v.end(); ++v)
      fprintf(fp," %8u ",v->index());
    fprintf(fp," \n");
  }
  fprintf(fp, "</DataArray> \n");

  // Write offset into connectivity array for the end of each cell
  fprintf(fp, "<DataArray  type=\"Int32\"  Name=\"offsets\"  format=\"ascii\">  \n");
  for (uint offsets = 1; offsets <= mesh.num_cells(); offsets++)
  {
    if (mesh.type().cell_type() == CellType::tetrahedron )
      fprintf(fp, " %8u \n",  offsets*4);
    if (mesh.type().cell_type() == CellType::triangle )
      fprintf(fp, " %8u \n", offsets*3);
    if (mesh.type().cell_type() == CellType::interval )
      fprintf(fp, " %8u \n",  offsets*2);
  }
  fprintf(fp, "</DataArray> \n");

  //Write cell type
  fprintf(fp, "<DataArray  type=\"UInt8\"  Name=\"types\"  format=\"ascii\">  \n");
  for (uint types = 1; types <= mesh.num_cells(); types++)
  {
    if (mesh.type().cell_type() == CellType::tetrahedron )
      fprintf(fp, " 10 \n");
    if (mesh.type().cell_type() == CellType::triangle )
      fprintf(fp, " 5 \n");
    if (mesh.type().cell_type() == CellType::interval )
      fprintf(fp, " 3 \n");
  }
  fprintf(fp, "</DataArray> \n");
  fprintf(fp, "</Cells> \n");

  // Close file
  fclose(fp);
}
//----------------------------------------------------------------------------
void VTKFile::results_write(const Function& u, std::string vtu_filename) const
{
  // Type of data (point or cell). Point by default.
  std::string data_type = "point";

  // For brevity
  const FunctionSpace& V = u.function_space();
  const Mesh& mesh(V.mesh());
  const FiniteElement& element(V.element());
  const DofMap& dofmap(V.dofmap());

  // Get rank of Function
  const uint rank = element.value_rank();
  if(rank > 2)
    error("Only scalar, vector and tensor functions can be saved in VTK format.");

  // Get number of components
  uint dim = 1;
  for (uint i = 0; i < rank; i++)
    dim *= element.value_dimension(i);

  // Test for cell-based element type
  uint cell_based_dim = 1;
  for (uint i = 0; i < rank; i++)
    cell_based_dim *= mesh.topology().dim();
  if (dofmap.max_local_dimension() == cell_based_dim)
    data_type = "cell";

  // Open file
  std::ofstream fp(vtu_filename.c_str(), std::ios_base::app);

  // Write function data at mesh cells
  if (data_type == "cell")
  {
    // Allocate memory for function values at cell centres
    const uint size = mesh.num_cells()*dim;
    double* values = new double[size];

    // Get function values on cells
    u.vector().get(values);

    // Write headers
    if (rank == 0)
    {
      fp << "<CellData  Scalars=\"U\"> " << std::endl;
      fp << "<DataArray  type=\"Float64\"  Name=\"U\"  format=\"ascii\"> " << std::endl;
    }
    else if (rank == 1)
    {
      if(!(dim == 2 || dim == 3))
        error("don't know what to do with vector function with dim other than 2 or 3.");
      fp << "<CellData  Vectors=\"U\"> " << std::endl;
      fp << "<DataArray  type=\"Float64\"  Name=\"U\"  NumberOfComponents=\"3\" format=\"ascii\"> " << std::endl;
    }
    else if (rank == 2)
    {
      if(!(dim == 4 || dim == 9))
        error("Don't know what to do with tensor function with dim other than 4 or 9.");
      fp << "<CellData  Tensors=\"U\"> " << std::endl;
      fp << "<DataArray  type=\"Float64\"  Name=\"U\"  NumberOfComponents=\"9\" format=\"ascii\">     " << std::endl;
    }

    std::ostringstream ss;
    ss << std::scientific;
    for (CellIterator cell(mesh); !cell.end(); ++cell)
    {
      ss.str("");

      if (rank == 1 && dim == 2)
      {
        // Append 0.0 to 2D vectors to make them 3D
        for(uint i = 0; i < dim; i++)
          ss << " " << values[cell->index() + i*mesh.num_cells()];
        ss << " " << 0.0;
      }
      else if (rank == 2 && dim == 4)
      {
        // Pad with 0.0 to 2D tensors to make them 3D
        for(uint i = 0; i < 2; i++)
        {
          ss << " " << values[cell->index() + (2*i+0)*mesh.num_cells()];
          ss << " " << values[cell->index() + (2*i+1)*mesh.num_cells()];
          ss << " " << 0.0;
        }
        ss << " " << 0.0;
        ss << " " << 0.0;
        ss << " " << 0.0;
      }
      else
      {
        // Write all components
        for (uint i = 0; i < dim; i++)
          ss << " " << values[cell->index() + i*mesh.num_cells()];
      }
      ss << std::endl;

      fp << ss.str();
    }
    fp << "</DataArray> " << std::endl;
    fp << "</CellData> " << std::endl;

    delete [] values;
  }
  else if (data_type == "point")
  {
    // Allocate memory for function values at vertices
    uint size = mesh.num_vertices()*dim;
    double* values = new double[size];

    // Get function values at vertices
    u.interpolate_vertex_values(values);

    if (rank == 0)
    {
      fp << "<PointData  Scalars=\"U\"> " << std::endl;
      fp << "<DataArray  type=\"Float64\"  Name=\"U\"  format=\"ascii\"> " << std::endl;
    }
    else if (rank == 1)
    {
      fp << "<PointData  Vectors=\"U\"> " << std::endl;
      fp << "<DataArray  type=\"Float64\"  Name=\"U\"  NumberOfComponents=\"3\" format=\"ascii\">  " << std::endl;
    }
    else if (rank == 2)
    {
      fp << "<PointData  Tensors=\"U\"> " << std::endl;
      fp << "<DataArray  type=\"Float64\"  Name=\"U\"  NumberOfComponents=\"9\" format=\"ascii\">  " << std::endl;
    }

    std::ostringstream ss;
    ss << std::scientific;
    for (VertexIterator vertex(mesh); !vertex.end(); ++vertex)
    {
      ss.str("");

      if(rank == 1 && dim == 2)
      {
        // Append 0.0 to 2D vectors to make them 3D
        for(uint i = 0; i < dim; i++)
          ss << " " << values[vertex->index() + i*mesh.num_vertices()];
        ss << " " << 0.0;
      }
      else if (rank == 2 && dim == 4)
      {
        // Pad with 0.0 to 2D tensors to make them 3D
        for(uint i = 0; i < 2; i++)
        {
          ss << " " << values[vertex->index() + (2*i+0)*mesh.num_vertices()];
          ss << " " << values[vertex->index() + (2*i+1)*mesh.num_vertices()];
          ss << " " << 0.0;
        }
        ss << " " << 0.0;
        ss << " " << 0.0;
        ss << " " << 0.0;
      }
      else
      {
        // Write all components
        for(uint i = 0; i < dim; i++)
          ss << " " << values[vertex->index() + i*mesh.num_vertices()];
      }
      ss << std::endl;

      fp << ss.str();
    }
    fp << "</DataArray> " << std::endl;
    fp << "</PointData> " << std::endl;

    delete [] values;
  }
  else
    error("Unknown VTK data type.");
}
//----------------------------------------------------------------------------
void VTKFile::pvd_file_write(uint num, std::string _filename)
{
  std::fstream pvd_file;

  if( num == 0)
  {
    // Open pvd file
    pvd_file.open(filename.c_str(), std::ios::out|std::ios::trunc);

    // Write header
    pvd_file << "<?xml version=\"1.0\"?> " << std::endl;
    pvd_file << "<VTKFile type=\"Collection\" version=\"0.1\" > " << std::endl;
    pvd_file << "<Collection> " << std::endl;
  }
  else
  {
    // Open pvd file
    pvd_file.open(filename.c_str(),  std::ios::out|std::ios::in);
    pvd_file.seekp(mark);
  }

  // Remove directory path from name for pvd file
  std::string fname = strip_path(_filename);

  // Data file name
  pvd_file << "<DataSet timestep=\"" << num << "\" part=\"0\"" << " file=\"" <<  fname <<  "\"/>" << std::endl;
  mark = pvd_file.tellp();

  // Close headers
  pvd_file << "</Collection> " << std::endl;
  pvd_file << "</VTKFile> " << std::endl;

  // Close file
  pvd_file.close();
}
//----------------------------------------------------------------------------
void VTKFile::pvtu_file_write(std::string pvtu_filename, std::string vtu_filename) const
{
  // Open pvtu file
  std::fstream pvtu_file;
  pvtu_file.open(pvtu_filename.c_str(), std::ios::out|std::ios::trunc);

  // Write header
  pvtu_file << "<?xml version=\"1.0\"?> " << std::endl;
  pvtu_file << "<VTKFile type=\"PUnstructuredGrid\" version=\"0.1\">" << std::endl;
  pvtu_file << "<PUnstructuredGrid GhostLevel=\"0\">" << std::endl;

  pvtu_file << "<PCellData>" << std::endl;
  pvtu_file << "<PDataArray  type=\"Int32\"  Name=\"connectivity\"  format=\"ascii\"/>" << std::endl;
  pvtu_file << "<PDataArray  type=\"Int32\"  Name=\"offsets\"  format=\"ascii\"/>" << std::endl;
  pvtu_file << "<PDataArray  type=\"UInt8\"  Name=\"types\"  format=\"ascii\"/>"  << std::endl;
  pvtu_file << "</PCellData>" << std::endl;

  pvtu_file << "<PPoints>" <<std::endl;
  pvtu_file << "<PDataArray  type=\"Float64\"  NumberOfComponents=\"3\"  format=\"ascii\"/>" << std::endl;
  pvtu_file << "</PPoints>" << std::endl;

  for(uint i=0; i< MPI::num_processes(); i++)
  {
    std::string tmp_string = strip_path(vtu_name(i, MPI::num_processes(), counter, ".vtu"));
    pvtu_file << "<Piece Source=\"" << tmp_string << "\"/>" << std::endl;
  }

  pvtu_file << "</PUnstructuredGrid>" << std::endl;
  pvtu_file << "</VTKFile>" << std::endl;
  pvtu_file.close();
}
//----------------------------------------------------------------------------
void VTKFile::vtk_header_open(uint num_vertices, uint num_cells, 
                              std::string vtu_filename) const
{
  // Open file
  FILE *fp = fopen(vtu_filename.c_str(), "a");
  if (!fp)
    error("Unable to open file %s", filename.c_str());

  // Write headers
  fprintf(fp, "<VTKFile type=\"UnstructuredGrid\"  version=\"0.1\"   >\n");
  fprintf(fp, "<UnstructuredGrid>  \n");
  fprintf(fp, "<Piece  NumberOfPoints=\" %8u\"  NumberOfCells=\" %8u\">  \n",
          num_vertices, num_cells);

  // Close file
  fclose(fp);
}
//----------------------------------------------------------------------------
void VTKFile::vtk_header_close(std::string vtu_filename) const
{
  // Open file
  FILE *fp = fopen(vtu_filename.c_str(), "a");
  if (!fp)
    error("Unable to open file %s", filename.c_str());

  // Close headers
  fprintf(fp, "</Piece> \n </UnstructuredGrid> \n </VTKFile>");

  // Close file
  fclose(fp);
}
//----------------------------------------------------------------------------
std::string VTKFile::vtu_name(const int process, const int num_processes,
                              const int counter, std::string ext) const
{
  std::string filestart, extension;
  std::ostringstream fileid, newfilename;

  fileid.fill('0');
  fileid.width(6);

  filestart.assign(filename, 0, filename.find("."));
  extension.assign(filename, filename.find("."), filename.size());

  fileid << counter;

  // Add process number to .vtu file name
  std::string proc = "";
  if (num_processes > 1)
  {
    std::ostringstream _p;
    _p << "_p" << process << "_";
    proc = _p.str();
  }
  newfilename << filestart << proc << fileid.str() << ext;

  return newfilename.str();
}
//----------------------------------------------------------------------------
template<class T>
void VTKFile::mesh_function_write(T& meshfunction)
{
  const Mesh& mesh = meshfunction.mesh();
  if( meshfunction.dim() != mesh.topology().dim() )
    error("VTK output of mesh functions is implemented for cell-based functions only.");

  // Update vtu file name and clear file
  std::string vtu_filename = init(mesh);

  // Write mesh
  mesh_write(mesh, vtu_filename);

  // Open file
  std::ofstream fp(vtu_filename.c_str(), std::ios_base::app);

  fp << "<CellData  Scalars=\"U\">" << std::endl;
  fp << "<DataArray  type=\"Float64\"  Name=\"U\"  format=\"ascii\">" << std::endl;
  for (CellIterator cell(mesh); !cell.end(); ++cell)
    fp << meshfunction.get( cell->index() )  << std::endl;
  fp << "</DataArray>" << std::endl;
  fp << "</CellData>" << std::endl;

  // Close file
  fp.close();

  // Write pvd files
  finalize(vtu_filename);

  cout << "saved mesh function " << counter << " times." << endl;

  cout << "Saved mesh function " << mesh.name() << " (" << mesh.label()
       << ") to file " << filename << " in VTK format." << endl;
}
//----------------------------------------------------------------------------
void VTKFile::clear_file(std::string file) const
{
  FILE* fp = fopen(file.c_str(), "w");
  if (!fp)
    error("Unable to open file %s", file.c_str());
  fclose(fp);
}
//----------------------------------------------------------------------------
std::string VTKFile::strip_path(std::string file) const
{
  std::string fname;
  fname.assign(file, filename.find_last_of("/") + 1, file.size());
  return fname;
}
//----------------------------------------------------------------------------
