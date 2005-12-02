// Copyright (C) 2002-2005 Johan Hoffman and Anders Logg.
// Licensed under the GNU GPL Version 2.
//
// First added:  2002
// Last changed: 2005-12-01

#include <stdio.h>
#include <math.h>
#include <strings.h>

#include <map>

#include <dolfin/dolfin_log.h>
#include <dolfin/utils.h>
#include <dolfin/constants.h>
#include <dolfin/Triangle.h>
#include <dolfin/Tetrahedron.h>
#include <dolfin/VertexIterator.h>
#include <dolfin/CellIterator.h>
#include <dolfin/File.h>
#include <dolfin/Boundary.h>
#include <dolfin/MeshHierarchy.h>
#include <dolfin/MeshInit.h>
#include <dolfin/MeshRefinement.h>
#include <dolfin/Mesh.h>

using namespace dolfin;

//-----------------------------------------------------------------------------
Mesh::Mesh()
{
  md = new MeshData(*this);
  bd = new BoundaryData(*this);
  _parent = 0;

  rename("mesh", "no description");
  clear();
}
//-----------------------------------------------------------------------------
Mesh::Mesh(const char* filename)
{
  md = new MeshData(*this);
  bd = new BoundaryData(*this);
  _parent = 0;

  rename("mesh", "no description");
  clear();

  // Read mesh from file
  File file(filename);
  file >> *this;
}
//-----------------------------------------------------------------------------
Mesh::Mesh(const Mesh& mesh)
{
  md = new MeshData(*this);
  bd = new BoundaryData(*this);
  _parent = 0;

  rename("mesh", "no description");
  clear();

  // Specify vertices
  for (VertexIterator n(mesh); !n.end(); ++n)
    createVertex(n->coord());
  
  // Specify cells
  for (CellIterator c(mesh); !c.end(); ++c)
    switch (c->type()) {
    case Cell::triangle:
      createCell(c->vertex(0), c->vertex(1), c->vertex(2));
      break;
    case Cell::tetrahedron:
      createCell(c->vertex(0), c->vertex(1), c->vertex(2), c->vertex(3));
      break;
    default:
      dolfin_error("Unknown cell type.");
    }

  // Compute connectivity
  init();
}
//-----------------------------------------------------------------------------
Mesh::~Mesh()
{
  clear();

  if ( md )
    delete md;
  md = 0;

  if ( bd )
    delete bd;
  bd = 0;
}
//-----------------------------------------------------------------------------
void Mesh::clear()
{
  md->clear();
  bd->clear();

  _type = triangles;
  _child = 0;

  // Assume that we need to delete the parent which is created by refine().
  if ( _parent )
    delete _parent;
  _parent = 0;
}
//-----------------------------------------------------------------------------
int Mesh::noVertices() const
{
  return md->noVertices();
}
//-----------------------------------------------------------------------------
int Mesh::noCells() const
{
  return md->noCells();
}
//-----------------------------------------------------------------------------
int Mesh::noEdges() const
{
  return md->noEdges();
}
//-----------------------------------------------------------------------------
int Mesh::noFaces() const
{
  return md->noFaces();
}
//-----------------------------------------------------------------------------
Mesh::Type Mesh::type() const
{
  return _type;
}
//-----------------------------------------------------------------------------
Vertex& Mesh::vertex(uint id)
{
  return md->vertex(id);
}
//-----------------------------------------------------------------------------
Cell& Mesh::cell(uint id)
{
  return md->cell(id);
}
//-----------------------------------------------------------------------------
Edge& Mesh::edge(uint id)
{
  return md->edge(id);
}
//-----------------------------------------------------------------------------
Face& Mesh::face(uint id)
{
  return md->face(id);
}
//-----------------------------------------------------------------------------
Boundary Mesh::boundary()
{
  Boundary boundary(*this);
  return boundary;
}
//-----------------------------------------------------------------------------
void Mesh::refine()
{
  // Check that this is the finest mesh
  if ( _child )
    dolfin_error("Only the finest mesh in a mesh hierarchy can be refined.");

  // Create mesh hierarchy
  dolfin_log(false);
  MeshHierarchy meshes(*this);
  dolfin_log(true);

  // Refine mesh hierarchy
  MeshRefinement::refine(meshes);

  // Swap data structures with the new finest mesh. This is necessary since
  // refine() should replace the current mesh with the finest mesh. At the
  // same time, we store the data structures of the current mesh in the
  // newly created finest mesh, which becomes the next finest mesh:
  //
  // Before refinement:  g0 <-> g1 <-> g2 <-> ... <-> *this(md)
  // After refinement:   g0 <-> g1 <-> g2 <-> ... <-> *this(md) <-> new(nmd)
  // After swap:         g0 <-> g1 <-> g2 <-> ... <-> new(md)   <-> *this(nmd)

  // Get pointer to new mesh
  Mesh* new_mesh = &(meshes.fine());

  // Swap data only if a new mesh was created
  if ( new_mesh != this ) {

    // Swap data
    swap(*new_mesh);
    
    // Set parent and child
    if ( new_mesh->_parent )
      new_mesh->_parent->_child = new_mesh;
    this->_parent = new_mesh;
    new_mesh->_child = this;
    
  }    
}
//-----------------------------------------------------------------------------
void Mesh::refineUniformly()
{
  // Mark all cells for refinement
  for (CellIterator c(*this); !c.end(); ++c)
    c->mark();

  // Refine
  refine();
}
//-----------------------------------------------------------------------------
void Mesh::refineUniformly(int i)
{
  // Refine uniformly i times. 
  for (int j=0; j < i; j++)
    refineUniformly();
}
//-----------------------------------------------------------------------------
Mesh& Mesh::parent()
{
  if ( _parent )
    return *_parent;

  dolfin_warning("Mesh has no parent.");
  return *this;
}
//-----------------------------------------------------------------------------
Mesh& Mesh::child()
{
  if ( _child )
    return *_child;

  dolfin_warning("Mesh has no child.");
  return *this;
}
//-----------------------------------------------------------------------------
bool Mesh::operator==(const Mesh& mesh) const
{
  return this == &mesh;
}
//-----------------------------------------------------------------------------
bool Mesh::operator!=(const Mesh& mesh) const
{
  return this != &mesh;
}
//-----------------------------------------------------------------------------
void Mesh::disp() const
{
  cout << "Mesh data:" << endl;
  cout << "----------" << endl << endl;

  cout << "  " << "Number of vertices: " << noVertices() << endl;
  cout << "  " << "Number of edges: " << noEdges() << endl;
  if ( type() == tetrahedra )
    cout << "  " << "Number of faces: " << noFaces() << endl;
  cout << "  " << "Number of cells: " << noCells() << endl;

  cout << endl;
  for (VertexIterator n(this); !n.end(); ++n)
    cout << "  " << *n << endl;

  cout << endl;
  for (EdgeIterator e(this); !e.end(); ++e)
    cout << "  " << *e << endl;

  if ( type() == tetrahedra )
  {
    cout << endl;
    for (FaceIterator f(this); !f.end(); ++f)
      cout << "  " << *f << endl;
  }

  cout << endl;  
  for (CellIterator c(this); !c.end(); ++c)
    cout << "  " << *c << endl;
  
  cout << endl;
}
//-----------------------------------------------------------------------------
Mesh& Mesh::createChild()
{
  // Make sure that we have not already created a child
  dolfin_assert(!_child);
  
  // Create the new mesh
  Mesh* new_mesh = new Mesh();
  
  // Set child and parent info
  _child = new_mesh;
  new_mesh->_parent = this;

  // Return the new mesh
  return *new_mesh;
}
//-----------------------------------------------------------------------------
Vertex& Mesh::createVertex(Point p)
{
  return md->createVertex(p);
}
//-----------------------------------------------------------------------------
Vertex& Mesh::createVertex(real x, real y, real z)
{
  return md->createVertex(x, y, z);
}
//-----------------------------------------------------------------------------
Cell& Mesh::createCell(int n0, int n1, int n2)
{
  // Warning: mesh type will be type of last added cell
  _type = triangles;
  
  return md->createCell(n0, n1, n2);
}
//-----------------------------------------------------------------------------
Cell& Mesh::createCell(int n0, int n1, int n2, int n3)
{
  // Warning: mesh type will be type of last added cell
  _type = tetrahedra;
  
  return md->createCell(n0, n1, n2, n3);
}
//-----------------------------------------------------------------------------
Cell& Mesh::createCell(Vertex& n0, Vertex& n1, Vertex& n2)
{
  // Warning: mesh type will be type of last added cell
  _type = triangles;
  
  return md->createCell(n0, n1, n2);
}
//-----------------------------------------------------------------------------
Cell& Mesh::createCell(Vertex& n0, Vertex& n1, Vertex& n2, Vertex& n3)
{
  // Warning: mesh type will be type of last added cell
  _type = tetrahedra;
  
  return md->createCell(n0, n1, n2, n3);
}
//-----------------------------------------------------------------------------
Edge& Mesh::createEdge(int n0, int n1)
{
  return md->createEdge(n0, n1);
}
//-----------------------------------------------------------------------------
Edge& Mesh::createEdge(Vertex& n0, Vertex& n1)
{
  return md->createEdge(n0, n1);
}
//-----------------------------------------------------------------------------
Face& Mesh::createFace(int e0, int e1, int e2)
{
  return md->createFace(e0, e1, e2);
}
//-----------------------------------------------------------------------------
Face& Mesh::createFace(Edge& e0, Edge& e1, Edge& e2)
{
  return md->createFace(e0, e1, e2);
}
//-----------------------------------------------------------------------------
void Mesh::remove(Vertex& vertex)
{
  md->remove(vertex);
}
//-----------------------------------------------------------------------------
void Mesh::remove(Cell& cell)
{
  md->remove(cell);
}
//-----------------------------------------------------------------------------
void Mesh::remove(Edge& edge)
{
  md->remove(edge);
}
//-----------------------------------------------------------------------------
void Mesh::remove(Face& face)
{
  md->remove(face);
}
//-----------------------------------------------------------------------------
void Mesh::init()
{
  MeshInit::init(*this);
}
//-----------------------------------------------------------------------------
void Mesh::merge(Mesh& mesh2)
{
  std::map<int, int> vertexmap;

  for (VertexIterator n(&mesh2); !n.end(); ++n)
  {
    Vertex& vertex = *n;
    int nid = vertex.id();

    Vertex& newvertex = createVertex(vertex.coord());
    int newnid = newvertex.id();

    vertexmap[nid] = newnid;
  }

  for (CellIterator c(mesh2); !c.end(); ++c)
  {
    Cell& cell = *c;

    if(cell.type() == Cell::tetrahedron)
    {
      int nid0 = vertexmap[cell.vertex(0).id()];
      int nid1 = vertexmap[cell.vertex(1).id()];
      int nid2 = vertexmap[cell.vertex(2).id()];
      int nid3 = vertexmap[cell.vertex(3).id()];

      createCell(nid0, nid1, nid2, nid3);
    }
    else
    {
      int nid0 = vertexmap[cell.vertexID(0)];
      int nid1 = vertexmap[cell.vertexID(1)];
      int nid2 = vertexmap[cell.vertexID(2)];

      createCell(nid0, nid1, nid2);
    }
  }

  init();
}
//-----------------------------------------------------------------------------
void Mesh::swap(Mesh& mesh)
{
  // Swap data
  MeshData*           tmp_md     = this->md;
  BoundaryData*       tmp_bd     = this->bd;
  Mesh*               tmp_parent = this->_parent;
  Mesh*               tmp_child  = this->_child;
  Type                tmp_type   = this->_type;

  this->md      = mesh.md;
  this->bd      = mesh.bd;
  this->_parent = mesh._parent;
  this->_child  = mesh._child;
  this->_type   = mesh._type;

  mesh.md      = tmp_md;
  mesh.bd      = tmp_bd;
  mesh._parent = tmp_parent;
  mesh._child  = tmp_child;
  mesh._type   = tmp_type;

  // Change mesh reference in all data structures
  mesh.md->setMesh(mesh);
  mesh.bd->setMesh(mesh);

  this->md->setMesh(*this);
  this->bd->setMesh(*this);
}
//-----------------------------------------------------------------------------
// Additional operators
//-----------------------------------------------------------------------------
dolfin::LogStream& dolfin::operator<< (LogStream& stream, const Mesh& mesh)
{
  stream << "[ Mesh with " << mesh.noVertices() << " vertices, "
	 << mesh.noCells() << " cells ";

  switch ( mesh.type() ) {
  case Mesh::triangles:
    stream << "(triangles)";
    break;
  case Mesh::tetrahedra:
    stream << "(tetrahedra)";
    break;
  default:
    stream << "(unknown type)";
  }

  stream << ", and " << mesh.noEdges() << " edges ]";

  return stream;
}
//-----------------------------------------------------------------------------
