// Copyright (C) 2008 Ola Skavhaug
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
// First added:  2008-11-28
// Last changed: 2011-05-30
//
// Modified by Anders Logg, 2008.
// Modified by Kent-Andre Mardal, 2011.

#include <boost/assign/list_of.hpp>
#include <boost/scoped_ptr.hpp>

#include <dolfin/common/constants.h>
#include <dolfin/common/MPI.h>
#include <dolfin/log/log.h>
#include <dolfin/mesh/CellType.h>
#include "SaxHandler.h"
#include "XMLLocalMeshDataDistributed.h"

using namespace dolfin;

//-----------------------------------------------------------------------------
XMLLocalMeshDataDistributed::XMLLocalMeshDataDistributed(LocalMeshData& mesh_data,
  const std::string filename) : state(OUTSIDE), mesh_data(mesh_data), filename(filename)
{
  // Do nothing
}
//-----------------------------------------------------------------------------
void XMLLocalMeshDataDistributed::read()
{
  // Create handler
  xmlSAXHandler sax_handler;

  // Call back functions
  sax_handler.startDocument = XMLLocalMeshDataDistributed::sax_start_document;
  sax_handler.endDocument   = XMLLocalMeshDataDistributed::sax_end_document;

  sax_handler.startElement  = XMLLocalMeshDataDistributed::sax_start_element;
  sax_handler.endElement    = XMLLocalMeshDataDistributed::sax_end_element;

  // Parse
  xmlSAXUserParseFile(&sax_handler, (void *) this, filename.c_str());

  /*
  saxHandler.initialized = XML_SAX2_MAGIC;
  */
}
//-----------------------------------------------------------------------------
void XMLLocalMeshDataDistributed::start_element(const xmlChar* name, const xmlChar** attrs)
{
  switch (state)
  {
  case OUTSIDE:
    if (xmlStrcasecmp(name, (xmlChar* ) "mesh") == 0)
    {
      read_mesh(name, attrs);
      state = INSIDE_MESH;
    }
    break;

  case INSIDE_MESH:
    if (xmlStrcasecmp(name, (xmlChar* ) "vertices") == 0)
    {
      read_vertices(name, attrs);
      state = INSIDE_VERTICES;
    }
    else if (xmlStrcasecmp(name, (xmlChar* ) "cells") == 0)
    {
      read_cells(name, attrs);
      state = INSIDE_CELLS;
    }
    else if (xmlStrcasecmp(name, (xmlChar* ) "data") == 0)
    {
      //read_mesh_data(name, attrs);
      state = INSIDE_DATA;
    }
    break;

  case INSIDE_VERTICES:
    if (xmlStrcasecmp(name, (xmlChar* ) "vertex") == 0)
    {
      read_vertex(name, attrs);
    }
    break;

  case INSIDE_CELLS:
    if (xmlStrcasecmp(name, (xmlChar* ) "interval") == 0)
    {
      //read_interval(name, attrs);
    }
    else if (xmlStrcasecmp(name, (xmlChar* ) "triangle") == 0)
    {
      read_triangle(name, attrs);
    }
    else if (xmlStrcasecmp(name, (xmlChar* ) "tetrahedron") == 0)
    {
      //read_tetrahedron(name, attrs);
    }
    break;

  case INSIDE_DATA:
    if (xmlStrcasecmp(name, (xmlChar* ) "meshfunction") == 0)
    {
      //read_mesh_function(name, attrs);
      state = INSIDE_MESH_FUNCTION;
    }
    else if (xmlStrcasecmp(name, (xmlChar* ) "array") == 0)
    {
      //read_array(name, attrs);
      state = INSIDE_ARRAY;
    }
    else if (xmlStrcasecmp(name, (xmlChar* ) "data_entry") == 0)
    {
      //read_data_entry(name, attrs);
      state = INSIDE_DATA_ENTRY;
    }
    break;

  case INSIDE_DATA_ENTRY:
    if (xmlStrcasecmp(name, (xmlChar* ) "array") == 0)
    {
      //read_array(name, attrs);
      state = INSIDE_ARRAY;
    }
    break;
  case DONE:
    error("Inconsistent state in XML reader: %d.", state);

  default:
    error("Inconsistent state in XML reader: %d.", state);
  }
}
//-----------------------------------------------------------------------------
void XMLLocalMeshDataDistributed::end_element(const xmlChar *name)
{
  //std::cout << "!!!Yes 2 " << state << std::endl;
  switch (state)
  {
  case INSIDE_MESH:
    if (xmlStrcasecmp(name, (xmlChar* ) "mesh") == 0)
    {
      state = DONE;
      //release();
    }
    break;

  case INSIDE_VERTICES:
    if (xmlStrcasecmp(name, (xmlChar* ) "vertices") == 0)
    {
      state = INSIDE_MESH;
    }
    break;

  case INSIDE_CELLS:
    if (xmlStrcasecmp(name, (xmlChar* ) "cells") == 0)
    {
      state = INSIDE_MESH;
    }
    break;

  case INSIDE_DATA:
    if (xmlStrcasecmp(name, (xmlChar* ) "data") == 0)
    {
      state = INSIDE_MESH;
    }
    break;

  case INSIDE_MESH_FUNCTION:
    if (xmlStrcasecmp(name, (xmlChar* ) "meshfunction") == 0)
    {
      state = INSIDE_DATA;
    }
    break;

  case INSIDE_DATA_ENTRY:
    if (xmlStrcasecmp(name, (xmlChar* ) "data_entry") == 0)
    {
      state = INSIDE_DATA;
    }

  case INSIDE_ARRAY:
    if (xmlStrcasecmp(name, (xmlChar* ) "array") == 0)
    {
      state = INSIDE_DATA_ENTRY;
    }

    if (xmlStrcasecmp(name, (xmlChar* ) "data_entry") == 0)
    {
      state = INSIDE_DATA;
    }
    break;

  default:
    {
     //warning("Closing XML tag '%s', but state is %d.", name, state);
    }
  }
}
//-----------------------------------------------------------------------------
void XMLLocalMeshDataDistributed::sax_start_document(void *ctx)
{
  // Do nothing
}
//-----------------------------------------------------------------------------
void XMLLocalMeshDataDistributed::sax_end_document(void *ctx)
{
  // Do nothing
}
//-----------------------------------------------------------------------------
void XMLLocalMeshDataDistributed::sax_start_element(void *ctx,
                                                    const xmlChar* name,
                                                    const xmlChar** attrs)
{
  ((XMLLocalMeshDataDistributed*) ctx)->start_element(name, attrs);
}
//-----------------------------------------------------------------------------
void XMLLocalMeshDataDistributed::sax_end_element(void *ctx, const xmlChar *name)
{
  ((XMLLocalMeshDataDistributed*) ctx)->end_element(name);
}
//-----------------------------------------------------------------------------
void XMLLocalMeshDataDistributed::sax_warning(void *ctx, const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  char buffer[DOLFIN_LINELENGTH];
  vsnprintf(buffer, DOLFIN_LINELENGTH, msg, args);
  warning("Incomplete XML data: " + std::string(buffer));
  va_end(args);
}
//-----------------------------------------------------------------------------
void XMLLocalMeshDataDistributed::sax_error(void *ctx, const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  char buffer[DOLFIN_LINELENGTH];
  vsnprintf(buffer, DOLFIN_LINELENGTH, msg, args);
  error("Illegal XML data: " + std::string(buffer));
  va_end(args);
}
//-----------------------------------------------------------------------------
void XMLLocalMeshDataDistributed::sax_fatal_error(void *ctx, const char *msg, ...)
{
  va_list args;
  va_start(args, msg);
  char buffer[DOLFIN_LINELENGTH];
  vsnprintf(buffer, DOLFIN_LINELENGTH, msg, args);
  error("Illegal XML data: " + std::string(buffer));
  va_end(args);
}
//-----------------------------------------------------------------------------
void XMLLocalMeshDataDistributed::read_mesh(const xmlChar* name, const xmlChar** attrs)
{
  // Clear all data
  mesh_data.clear();

  // Parse values
  std::string type = SaxHandler::parse_string(name, attrs, "celltype");
  gdim = SaxHandler::parse_uint(name, attrs, "dim");

  // Create cell type to get topological dimension
  boost::scoped_ptr<CellType> cell_type(CellType::create(type));
  tdim = cell_type->dim();

  // Get number of entities for topological dimension 0
  mesh_data.tdim = tdim;
  mesh_data.gdim = gdim;
}
//-----------------------------------------------------------------------------
void XMLLocalMeshDataDistributed::read_vertices(const xmlChar* name, const xmlChar** attrs)
{
  // Parse the number of global vertices
  const uint num_global_vertices = SaxHandler::parse_uint(name, attrs, "size");
  mesh_data.num_global_vertices = num_global_vertices;

  // Compute vertex range
  vertex_range = MPI::local_range(num_global_vertices);

  // Reserve space for local-to-global vertex map and vertex coordinates
  mesh_data.vertex_indices.reserve(num_local_vertices());
  mesh_data.vertex_coordinates.reserve(num_local_vertices());
}
//-----------------------------------------------------------------------------
void XMLLocalMeshDataDistributed::read_vertex(const xmlChar* name, const xmlChar** attrs)
{
  // Read vertex index
  const uint v = SaxHandler::parse_uint(name, attrs, "index");

  // Skip vertices not in range for this process
  if (v < vertex_range.first || v >= vertex_range.second)
    return;

  // Parse vertex coordinates
  switch (gdim)
  {
  case 1:
    {
      const std::vector<double> coordinate
          = boost::assign::list_of(SaxHandler::parse_float(name, attrs, "x"));
      mesh_data.vertex_coordinates.push_back(coordinate);
    }
  break;
  case 2:
    {
      const std::vector<double> coordinate = boost::assign::list_of(SaxHandler::parse_float(name, attrs, "x"))
                                                                   (SaxHandler::parse_float(name, attrs, "y"));
      mesh_data.vertex_coordinates.push_back(coordinate);
    }
    break;
  case 3:
    {
      const std::vector<double> coordinate = boost::assign::list_of(SaxHandler::parse_float(name, attrs, "x"))
                                                                   (SaxHandler::parse_float(name, attrs, "y"))
                                                                   (SaxHandler::parse_float(name, attrs, "z"));
      mesh_data.vertex_coordinates.push_back(coordinate);
    }
    break;
  default:
    error("Geometric dimension of mesh must be 1, 2 or 3.");
  }

  // Store global vertex numbering
  mesh_data.vertex_indices.push_back(v);
}
//-----------------------------------------------------------------------------
void XMLLocalMeshDataDistributed::read_cells(const xmlChar* name, const xmlChar** attrs)
{
  // Parse the number of global cells
  const uint num_global_cells = SaxHandler::parse_uint(name, attrs, "size");
  mesh_data.num_global_cells = num_global_cells;

  // Compute cell range
  cell_range = MPI::local_range(num_global_cells);

  // Reserve space for cells
  mesh_data.cell_vertices.reserve(num_local_cells());

  // Reserve space for global cell indices
  mesh_data.global_cell_indices.reserve(num_local_cells());
}
//-----------------------------------------------------------------------------
void XMLLocalMeshDataDistributed::read_interval(const xmlChar *name, const xmlChar **attrs)
{
  // Check dimension
  if (tdim != 1)
    error("Mesh entity (interval) does not match dimension of mesh (%d).", tdim);

  // Read cell index
  const uint c = SaxHandler::parse_uint(name, attrs, "index");

  // Skip cells not in range for this process
  if (c < cell_range.first || c >= cell_range.second)
    return;

  // Parse values
  std::vector<uint> cell(2);
  cell[0] = SaxHandler::parse_uint(name, attrs, "v0");
  cell[1] = SaxHandler::parse_uint(name, attrs, "v1");

  // Add cell
  mesh_data.cell_vertices.push_back(cell);

  // Add global cell index
  mesh_data.global_cell_indices.push_back(c);

  // Vertices per cell
  mesh_data.num_vertices_per_cell = 2;
}
//-----------------------------------------------------------------------------
void XMLLocalMeshDataDistributed::read_triangle(const xmlChar *name, const xmlChar **attrs)
{
  // Check dimension
  if (tdim != 2)
    error("Mesh entity (interval) does not match dimension of mesh (%d).", tdim);

  // Read cell index
  const uint c = SaxHandler::parse_uint(name, attrs, "index");

  // Skip cells not in range for this process
  if (c < cell_range.first || c >= cell_range.second)
    return;

  // Parse values
  std::vector<uint> cell(3);
  cell[0] = SaxHandler::parse_uint(name, attrs, "v0");
  cell[1] = SaxHandler::parse_uint(name, attrs, "v1");
  cell[2] = SaxHandler::parse_uint(name, attrs, "v2");

  // Add cell
  mesh_data.cell_vertices.push_back(cell);

  // Add global cell index
  mesh_data.global_cell_indices.push_back(c);

  // Vertices per cell
  mesh_data.num_vertices_per_cell = 3;
}
//-----------------------------------------------------------------------------
void XMLLocalMeshDataDistributed::read_tetrahedron(const xmlChar *name, const xmlChar **attrs)
{
  // Check dimension
  if (tdim != 3)
    error("Mesh entity (interval) does not match dimension of mesh (%d).", tdim);

  // Read cell index
  const uint c = SaxHandler::parse_uint(name, attrs, "index");

  // Skip cells not in range for this process
  if (c < cell_range.first || c >= cell_range.second)
    return;

  // Parse values
  std::vector<uint> cell(4);
  cell[0] = SaxHandler::parse_uint(name, attrs, "v0");
  cell[1] = SaxHandler::parse_uint(name, attrs, "v1");
  cell[2] = SaxHandler::parse_uint(name, attrs, "v2");
  cell[3] = SaxHandler::parse_uint(name, attrs, "v3");

  // Add cell
  mesh_data.cell_vertices.push_back(cell);

  // Add global cell index
  mesh_data.global_cell_indices.push_back(c);

  // Vertices per cell
  mesh_data.num_vertices_per_cell = 4;
}
//-----------------------------------------------------------------------------
dolfin::uint XMLLocalMeshDataDistributed::num_local_vertices() const
{
  return vertex_range.second - vertex_range.first;
}
//-----------------------------------------------------------------------------
dolfin::uint XMLLocalMeshDataDistributed::num_local_cells() const
{
  return cell_range.second - cell_range.first;
}
//-----------------------------------------------------------------------------
