// Copyright (C) 2012 Johannes Ring
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
// Modified by Joachim B Haga 2012
// Modified by Benjamin Kehlet 2012-2013
//
// First added:  2012-05-10
// Last changed: 2013-04-05

#include <vector>
#include <cmath>

#include <dolfin/common/constants.h>
#include <dolfin/math/basic.h>
#include <dolfin/mesh/Mesh.h>
#include <dolfin/mesh/MeshEditor.h>
#include <dolfin/mesh/MeshFunction.h>
#include <dolfin/mesh/MeshDomains.h>
#include <dolfin/mesh/MeshValueCollection.h>

#include "CSGCGALMeshGenerator2D.h"
#include "CSGGeometry.h"
#include "CSGOperators.h"
#include "CSGPrimitives2D.h"

#ifdef HAS_CGAL
#include "CGALMeshBuilder.h"

#include <CGAL/basic.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

#include <CGAL/Gmpq.h>
#include <CGAL/Lazy_exact_nt.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Bounded_kernel.h>
#include <CGAL/Nef_polyhedron_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>
#include <CGAL/Delaunay_mesher_2.h>
#include <CGAL/Delaunay_mesh_face_base_2.h>
#include <CGAL/Delaunay_mesh_size_criteria_2.h>

#include <CGAL/Min_circle_2.h>
#include <CGAL/Min_circle_2_traits_2.h>

typedef CGAL::Exact_predicates_exact_constructions_kernel Exact_Kernel;
typedef CGAL::Exact_predicates_inexact_constructions_kernel Inexact_Kernel;

typedef CGAL::Lazy_exact_nt<CGAL::Gmpq> FT;
typedef CGAL::Simple_cartesian<FT> EKernel;
typedef CGAL::Bounded_kernel<EKernel> Extended_kernel;
typedef CGAL::Nef_polyhedron_2<Extended_kernel> Nef_polyhedron_2;
typedef Nef_polyhedron_2::Point Nef_point_2;

typedef Nef_polyhedron_2::Explorer Explorer;
typedef Explorer::Face_const_iterator Face_const_iterator;
typedef Explorer::Hole_const_iterator Hole_const_iterator;
typedef Explorer::Vertex_const_iterator Vertex_const_iterator;
typedef Explorer::Halfedge_around_face_const_circulator Halfedge_around_face_const_circulator;
typedef Explorer::Vertex_const_handle Vertex_const_handle;
typedef Explorer::Halfedge_const_handle Halfedge_const_handle;

typedef CGAL::Triangulation_vertex_base_2<Inexact_Kernel>  Vertex_base;
typedef CGAL::Constrained_triangulation_face_base_2<Inexact_Kernel> Face_base;

// Min enclosing circle typedefs
typedef CGAL::Min_circle_2_traits_2<Extended_kernel>  Min_Circle_Traits;
typedef CGAL::Min_circle_2<Min_Circle_Traits>      Min_circle;
typedef CGAL::Circle_2<Extended_kernel> CGAL_Circle;


template <class Gt,
          class Fb >
class Enriched_face_base_2 : public Fb {
public:
  typedef Gt Geom_traits;
  typedef typename Fb::Vertex_handle Vertex_handle;
  typedef typename Fb::Face_handle Face_handle;

  template < typename TDS2 >
  struct Rebind_TDS {
    typedef typename Fb::template Rebind_TDS<TDS2>::Other Fb2;
    typedef Enriched_face_base_2<Gt,Fb2> Other;
  };

protected:
  int status;
  bool in_domain;

public:
  Enriched_face_base_2(): Fb(), status(-1) {}

  Enriched_face_base_2(Vertex_handle v0,
                       Vertex_handle v1,
                       Vertex_handle v2)
    : Fb(v0,v1,v2), status(-1), in_domain(true) {}

  Enriched_face_base_2(Vertex_handle v0,
                       Vertex_handle v1,
                       Vertex_handle v2,
                       Face_handle n0,
                       Face_handle n1,
                       Face_handle n2)
    : Fb(v0,v1,v2,n0,n1,n2), status(-1), in_domain(true) {}

  inline
  bool is_in_domain() const
  //{ return (status%2 == 1); }
  { return in_domain; }

  inline
  void set_in_domain(const bool b)
  //{ status = (b ? 1 : 0); }
  { in_domain = b; }

  inline
  void set_counter(int i)
  { status = i; }

  inline
  int counter() const
  { return status; }

  inline
  int& counter()
  { return status; }
};

typedef CGAL::Triangulation_vertex_base_2<Inexact_Kernel> Vb;
typedef CGAL::Triangulation_vertex_base_with_info_2<std::size_t, Inexact_Kernel, Vb> Vbb;
typedef Enriched_face_base_2<Inexact_Kernel, Face_base> Fb;
typedef CGAL::Triangulation_data_structure_2<Vbb, Fb> TDS;
typedef CGAL::Exact_predicates_tag Itag;
typedef CGAL::Constrained_Delaunay_triangulation_2<Inexact_Kernel, TDS, Itag> CDT;
typedef CGAL::Delaunay_mesh_size_criteria_2<CDT> Mesh_criteria_2;
typedef CGAL::Delaunay_mesher_2<CDT, Mesh_criteria_2> CGAL_Mesher_2;

typedef CDT::Vertex_handle Vertex_handle;
typedef CDT::Face_handle Face_handle;
typedef CDT::All_faces_iterator All_faces_iterator;

typedef CGAL::Polygon_2<Inexact_Kernel> Polygon_2;
typedef Inexact_Kernel::Point_2 Point_2;

using namespace dolfin;

//-----------------------------------------------------------------------------
CSGCGALMeshGenerator2D::CSGCGALMeshGenerator2D(const CSGGeometry& geometry)
: geometry(geometry)
{
  parameters = default_parameters();
  //subdomains.push_back(reference_to_no_delete_pointer(geometry));
}
//-----------------------------------------------------------------------------
CSGCGALMeshGenerator2D::~CSGCGALMeshGenerator2D() {}
//-----------------------------------------------------------------------------
Nef_polyhedron_2 make_circle(const Circle* c)
{
  std::vector<Nef_point_2> pts;

  for (std::size_t i = 0; i < c->fragments(); i++)
  {
    const double phi = (2*DOLFIN_PI*i) / c->fragments();
    const double x = c->center().x() + c->radius()*cos(phi);
    const double y = c->center().y() + c->radius()*sin(phi);
    pts.push_back(Nef_point_2(x, y));
  }

  return Nef_polyhedron_2(pts.begin(), pts.end(), Nef_polyhedron_2::INCLUDED);
}
//-----------------------------------------------------------------------------
Nef_polyhedron_2 make_ellipse(const Ellipse* e)
{
  std::vector<Nef_point_2> pts;

  for (std::size_t i = 0; i < e->fragments(); i++)
  {
    const double phi = (2*DOLFIN_PI*i) / e->fragments();
    const double x = e->center().x() + e->a()*cos(phi);
    const double y = e->center().y() + e->b()*sin(phi);
    pts.push_back(Nef_point_2(x, y));
  }

  return Nef_polyhedron_2(pts.begin(), pts.end(), Nef_polyhedron_2::INCLUDED);
}
//-----------------------------------------------------------------------------
Nef_polyhedron_2 make_rectangle(const Rectangle* r)
{
  const double x0 = std::min(r->first_corner().x(), r->first_corner().y());
  const double y0 = std::max(r->first_corner().x(), r->first_corner().y());

  const double x1 = std::min(r->second_corner().x(), r->second_corner().y());
  const double y1 = std::max(r->second_corner().x(), r->second_corner().y());

  std::vector<Nef_point_2> pts;
  pts.push_back(Nef_point_2(x0, x1));
  pts.push_back(Nef_point_2(y0, x1));
  pts.push_back(Nef_point_2(y0, y1));
  pts.push_back(Nef_point_2(x0, y1));

  return Nef_polyhedron_2(pts.begin(), pts.end(), Nef_polyhedron_2::INCLUDED);
}
//-----------------------------------------------------------------------------
Nef_polyhedron_2 make_polygon(const Polygon* p)
{
  std::vector<Nef_point_2> pts;
  std::vector<Point>::const_iterator v;
  for (v = p->vertices().begin(); v != p->vertices().end(); ++v)
    pts.push_back(Nef_point_2(v->x(), v->y()));

  return Nef_polyhedron_2(pts.begin(), pts.end(), Nef_polyhedron_2::INCLUDED);
}
//-----------------------------------------------------------------------------
static Nef_polyhedron_2 convertSubTree(const CSGGeometry *geometry)
{
  switch (geometry->getType()) {
  case CSGGeometry::Union:
  {
    const CSGUnion* u = dynamic_cast<const CSGUnion*>(geometry);
    dolfin_assert(u);
    return convertSubTree(u->_g0.get()) + convertSubTree(u->_g1.get());
    break;
  }
  case CSGGeometry::Intersection:
  {
    const CSGIntersection* u = dynamic_cast<const CSGIntersection*>(geometry);
    dolfin_assert(u);
    return convertSubTree(u->_g0.get()) * convertSubTree(u->_g1.get());
    break;
  }
  case CSGGeometry::Difference:
  {
    const CSGDifference* u = dynamic_cast<const CSGDifference*>(geometry);
    dolfin_assert(u);
    return convertSubTree(u->_g0.get()) - convertSubTree(u->_g1.get());
    break;
  }
  case CSGGeometry::Circle:
  {
    const Circle* c = dynamic_cast<const Circle*>(geometry);
    dolfin_assert(c);
    return make_circle(c);
    break;
  }
  case CSGGeometry::Ellipse:
  {
    const Ellipse* c = dynamic_cast<const Ellipse*>(geometry);
    dolfin_assert(c);
    return make_ellipse(c);
    break;
  }
  case CSGGeometry::Rectangle:
  {
    const Rectangle* r = dynamic_cast<const Rectangle*>(geometry);
    dolfin_assert(r);
    return make_rectangle(r);
    break;
  }
  case CSGGeometry::Polygon:
  {
    const Polygon* p = dynamic_cast<const Polygon*>(geometry);
    dolfin_assert(p);
    return make_polygon(p);
    break;
  }
  default:
    dolfin_error("CSGCGALMeshGenerator2D.cpp",
                 "converting geometry to cgal polyhedron",
                 "Unhandled primitive type");
  }
  return Nef_polyhedron_2();
}
//-----------------------------------------------------------------------------
void explore_subdomain(CDT &ct,
                        CDT::Face_handle start, 
                        std::list<CDT::Face_handle>& other_domains)
{
  std::list<Face_handle> queue;
  queue.push_back(start);

  while (!queue.empty())
  {
    CDT::Face_handle face = queue.front();
    queue.pop_front();

    for(int i = 0; i < 3; i++) 
    {
      Face_handle n = face->neighbor(i);
      if (ct.is_infinite(n))
        continue;

      const CDT::Edge e(face,i);

      if (n->counter() == -1)
      {
        if (ct.is_constrained(e))
        {
          // Reached a border
          other_domains.push_back(n);
        } else
        {
          // Set neighbor interface to the same and push to queue
          n->set_counter(face->counter());
          n->set_in_domain(face->is_in_domain());
          queue.push_back(n);
        }
      }
    }
  }
}
//-----------------------------------------------------------------------------
void explore_subdomains(CDT& cdt, 
                        const Nef_polyhedron_2& total_domain,
                        const std::vector<std::pair<std::size_t, Nef_polyhedron_2> > &subdomain_geometries)
{
  // Set counter to -1 for all faces
  for (CDT::Finite_faces_iterator it = cdt.finite_faces_begin(); it != cdt.finite_faces_end(); ++it)
  {
    it->set_counter(-1);
    it->set_in_domain(false);
  }

  std::list<CDT::Face_handle> subdomains;
  const CDT::Face_handle f = cdt.finite_faces_begin();
  subdomains.push_back(f);

  while (!subdomains.empty())
  {
    const CDT::Face_handle f = subdomains.front();
    subdomains.pop_front();

    // Construct a nef_polyhedron consisting of a single point
    // TODO: Is there a better way of querying if points are inside
    // nef polyhedrons?
    Point_2 p0 = f->vertex(0)->point();
    Point_2 p1 = f->vertex(1)->point();
    Point_2 p2 = f->vertex(2)->point();
    Nef_point_2 p( (p0[0] + p1[0] + p2[0]) / 3.0,
                   (p0[1] + p1[1] + p2[1]) / 3.0 );

    Nef_polyhedron_2 p_polyhedron(&p, (&p)+1);

    if (f->counter() < 0)
    {
      // Set default marker (0)
      f->set_counter(0);

      // Check if the face is in the total domain
      f->set_in_domain( !(p_polyhedron*total_domain).is_empty());

      for (int i = subdomain_geometries.size(); i > 0; --i)
      {
        if (!(subdomain_geometries[i-1].second*p_polyhedron).is_empty())
        {
          f->set_counter(subdomain_geometries[i-1].first);
          break;
        } 
      }
      
      explore_subdomain(cdt, f, subdomains);
    }
  }
}
//-----------------------------------------------------------------------------
void add_subdomain(CDT& cdt, const Nef_polyhedron_2& cgal_geometry)
{
  // Explore the Nef polyhedron and insert constraints in the triangulation
  Explorer explorer = cgal_geometry.explorer();
  for (Face_const_iterator fit = explorer.faces_begin() ; fit != explorer.faces_end(); fit++)
  {
    // Skip face if it is not part of polygon
    if (!explorer.mark(fit))
    {
      continue;
    }

    Halfedge_around_face_const_circulator hafc = explorer.face_cycle(fit), done(hafc);
    do
    {
      Point_2 pa(CGAL::to_double(hafc->vertex()->point().x()),
                 CGAL::to_double(hafc->vertex()->point().y()));

      Vertex_handle va = cdt.insert(pa);
          
      Point_2 pb(CGAL::to_double(hafc->next()->vertex()->point().x()),
                 CGAL::to_double(hafc->next()->vertex()->point().y()));
      Vertex_handle vb = cdt.insert(pb);

      cdt.insert_constraint(va, vb);
      hafc++;
    } while (hafc != done);
    
    Hole_const_iterator hit = explorer.holes_begin(fit);
    for (; hit != explorer.holes_end(fit); hit++)
    {
      Halfedge_around_face_const_circulator hafc1(hit), done1(hit);
      do
      {
        Point_2 pa(CGAL::to_double(hafc1->vertex()->point().x()),
                   CGAL::to_double(hafc1->vertex()->point().y()));
        Vertex_handle va = cdt.insert(pa);
        
        Point_2 pb(CGAL::to_double(hafc1->next()->vertex()->point().x()),
                   CGAL::to_double(hafc1->next()->vertex()->point().y()));
        Vertex_handle vb = cdt.insert(pb);
        cdt.insert_constraint(va, vb);
        hafc1++;
      } while (hafc1 != done1);
    }
  }
}
//-----------------------------------------------------------------------------
double shortest_constrained_edge(const CDT &cdt)
{
  double min_length = 1000.0;
  for (CDT::Finite_edges_iterator it = cdt.finite_edges_begin();
       it != cdt.finite_edges_end();
       it++)
  {
    if (!cdt.is_constrained(*it))
    {
      // An edge is an std::pair<Face_handle, int>
      // see CGAL/Triangulation_data_structure_2.h
      CDT::Face_handle f = it->first;
      const int i = it->second;

      CDT::Point p1 = f->vertex(i)->point();
      CDT::Point p2 = f->vertex( (i+1)%3 )->point();

      min_length = std::min(CGAL::to_double((p1-p2).squared_length()), min_length);
    }
  }

  return min_length;
}
//-----------------------------------------------------------------------------
void CSGCGALMeshGenerator2D::generate(Mesh& mesh)
{
  // Create empty CGAL triangulation
  CDT cdt;

  Nef_polyhedron_2 total_domain = convertSubTree(&geometry);
  Nef_polyhedron_2 overlaying;

  add_subdomain(cdt, total_domain);

  std::vector<std::pair<std::size_t, Nef_polyhedron_2> > subdomain_geometries(geometry.subdomains.size());

  // Assuming that the last in the vector contains the entire domain
  // Insert this first and mark the holes

  // Add the subdomains to the CDT

  std::list<std::pair<std::size_t, boost::shared_ptr<const CSGGeometry> > >::const_reverse_iterator it;
  std::size_t i = geometry.subdomains.size()-1;
  for (it = geometry.subdomains.rbegin(); 
       it != geometry.subdomains.rend(); it++)
  {
    const std::size_t current_index = it->first;
    boost::shared_ptr<const CSGGeometry> current_subdomain = it->second;
    subdomain_geometries[i] = std::make_pair(current_index, convertSubTree(current_subdomain.get())*total_domain);
    const Nef_polyhedron_2& cgal_geometry = subdomain_geometries[i].second;

    add_subdomain(cdt, cgal_geometry-overlaying);

    overlaying += cgal_geometry;

    i--;
  }

  cout << "Shortest edge: " << shortest_constrained_edge(cdt) << endl;

  explore_subdomains(cdt, total_domain, subdomain_geometries);

  // Create mesher
  CGAL_Mesher_2 mesher(cdt);

  // Add seeds for all faces in the domain
  std::list<Point_2> list_of_seeds;
  for(CDT::Finite_faces_iterator fit = cdt.finite_faces_begin();
      fit != cdt.finite_faces_end(); ++fit)
  {
    if (fit->is_in_domain())
    {
      // Calculate center of triangle and add to list of seeds
      Point_2 p0 = fit->vertex(0)->point();
      Point_2 p1 = fit->vertex(1)->point();
      Point_2 p2 = fit->vertex(2)->point();
      const double x = (p0[0] + p1[0] + p2[0]) / 3;
      const double y = (p0[1] + p1[1] + p2[1]) / 3;

      list_of_seeds.push_back(Point_2(x, y));
    }
  }

  mesher.set_seeds(list_of_seeds.begin(), list_of_seeds.end(), true);

  // Set shape and size criteria
  const int mesh_resolution = parameters["mesh_resolution"];
  if (mesh_resolution > 0)
  {
    // Set the cell size criteria according to the mesh_resolution parameter
    std::vector<Nef_point_2> points;
    for (CDT::Point_iterator it = cdt.points_begin();
         it != cdt.points_end();
         it++)
      points.push_back(Nef_point_2(it->x(), it->y()));

    Min_circle min_circle (points.begin(),
                           points.end(),
                           true); //randomize point order

    const double cell_size = 2.0*sqrt(CGAL::to_double(min_circle.circle().squared_radius()))/mesh_resolution;


    Mesh_criteria_2 criteria(parameters["triangle_shape_bound"],
                             cell_size);
    mesher.set_criteria(criteria);
  } 
  else
  {
    // Set shape and size criteria
    Mesh_criteria_2 criteria(parameters["triangle_shape_bound"],
                             parameters["cell_size"]);
    mesher.set_criteria(criteria);
  }

  // Refine CGAL mesh/triangulation
  mesher.refine_mesh();

  // Make sure triangulation is valid
  dolfin_assert(cdt.is_valid());

  // Mark the subdomains
  explore_subdomains(cdt, total_domain, subdomain_geometries);

  // Clear mesh
  mesh.clear();

  // Get various dimensions
  const std::size_t gdim = cdt.finite_vertices_begin()->point().dimension();
  const std::size_t tdim = cdt.dimension();
  const std::size_t num_vertices = cdt.number_of_vertices();

  // Count valid cells
  std::size_t num_cells = 0;
  CDT::Finite_faces_iterator cgal_cell;
  for (cgal_cell = cdt.finite_faces_begin(); 
       cgal_cell != cdt.finite_faces_end(); ++cgal_cell)
  {
    // Add cell if it is in the domain
    if (cgal_cell->is_in_domain())
    {
      num_cells++;
    }
  }

  // Create a MeshEditor and open
  dolfin::MeshEditor mesh_editor;
  mesh_editor.open(mesh, tdim, gdim);
  mesh_editor.init_vertices(num_vertices);
  mesh_editor.init_cells(num_cells);

  // Add vertices to mesh
  std::size_t vertex_index = 0;
  CDT::Finite_vertices_iterator cgal_vertex;
  for (cgal_vertex = cdt.finite_vertices_begin();
       cgal_vertex != cdt.finite_vertices_end(); ++cgal_vertex)
  {
    // Get vertex coordinates and add vertex to the mesh
    Point p;
    p[0] = cgal_vertex->point()[0];
    p[1] = cgal_vertex->point()[1];
    if (gdim == 3)
      p[2] = cgal_vertex->point()[2];

    // Add mesh vertex
    mesh_editor.add_vertex(vertex_index, p);

    // Attach index to vertex and increment
    cgal_vertex->info() = vertex_index++;
  }
  dolfin_assert(vertex_index == num_vertices);

  // Add cells to mesh
  std::size_t cell_index = 0;
  for (cgal_cell = cdt.finite_faces_begin(); cgal_cell != cdt.finite_faces_end(); ++cgal_cell)
  {
    // Add cell if it is in the domain
    if (cgal_cell->is_in_domain())
    {
      mesh_editor.add_cell(cell_index++,
                           cgal_cell->vertex(0)->info(),
                           cgal_cell->vertex(1)->info(),
                           cgal_cell->vertex(2)->info());
    }
  }
  dolfin_assert(cell_index == num_cells);

  // Close mesh editor
  mesh_editor.close();

  MeshFunction<std::size_t> mf(mesh, 2);

  cell_index=0;
  for (cgal_cell = cdt.finite_faces_begin(); 
       cgal_cell != cdt.finite_faces_end(); 
       ++cgal_cell)
  {
    // Add cell if it is in the domain
    if (cgal_cell->is_in_domain())
    {
      mf[cell_index] = cgal_cell->counter();
      cell_index++;
    }
  }

  // Assign the markers to the mesh's domain
  boost::shared_ptr<MeshValueCollection<std::size_t> >m = mesh.domains().markers(2);
  *m = mf;
  


  // Build DOLFIN mesh from CGAL triangulation
  // FIXME: Why does this not work correctly?
  //CGALMeshBuilder::build(mesh, cdt);
}

#else
namespace dolfin
{
  CSGCGALMeshGenerator2D::CSGCGALMeshGenerator2D(const CSGGeometry& geometry)
  {
    dolfin_error("CSGCGALMeshGenerator2D.cpp",
                 "Create mesh generator",
                 "Dolfin must be compiled with CGAL to use this feature.");
  }
  //-----------------------------------------------------------------------------
  CSGCGALMeshGenerator2D::~CSGCGALMeshGenerator2D()
  {
    // Do nothing
  }
  //-----------------------------------------------------------------------------
  void CSGCGALMeshGenerator2D::generate(Mesh& mesh)
  {
    // Do nothing
  }
}

#endif
//-----------------------------------------------------------------------------
