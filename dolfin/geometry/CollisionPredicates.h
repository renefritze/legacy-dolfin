// Copyright (C) 2014-2016 Anders Logg, August Johansson and Benjamin Kehlet
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
// First added:  2014-02-03
// Last changed: 2017-09-22

#ifndef __COLLISION_PREDICATES_H
#define __COLLISION_PREDICATES_H

#include "CGALExactArithmetic.h"

namespace dolfin
{

  // Forward declarations
  class Point;
  class MeshEntity;

  /// This class implements algorithms for detecting pairwise
  /// collisions between mesh entities of varying dimensions.

  class CollisionPredicates
  {
  public:

    //--- High-level collision detection predicates ---

    /// Check whether entity collides with point.
    ///
    /// *Arguments*
    ///     entity (_MeshEntity_)
    ///         The entity.
    ///     point (_Point_)
    ///         The point.
    ///
    /// *Returns*
    ///     bool
    ///         True iff entity collides with cell.
    static bool collides(const MeshEntity& entity,
                         const Point& point);

    /// Check whether two entities collide.
    ///
    /// *Arguments*
    ///     entity_0 (_MeshEntity_)
    ///         The first entity.
    ///     entity_1 (_MeshEntity_)
    ///         The second entity.
    ///
    /// *Returns*
    ///     bool
    ///         True iff entity collides with cell.
    static bool collides(const MeshEntity& entity_0,
                         const MeshEntity& entity_1);

    //--- Low-level collision detection predicates ---

    /// Check whether segment p0-p1 collides with point
    static bool collides_segment_point(const Point& p0,
                                       const Point& p1,
                                       const Point& point,
                                       std::size_t gdim);

    /// Check whether segment p0-p1 collides with point (1D version)
    static bool collides_segment_point_1d(double p0,
                                          double p1,
                                          double point)
    {
      // FIXME: Skip CGAL for now
      return _collides_segment_point_1d(p0, p1, point);
    }

    /// Check whether segment p0-p1 collides with point (2D version)
    static bool collides_segment_point_2d(const Point& p0,
                                          const Point& p1,
                                          const Point& point)
    {
      return CHECK_CGAL(_collides_segment_point_2d(p0, p1, point),
                        cgal_collides_segment_point_2d(p0, p1, point));
    }

    /// Check whether segment p0-p1 collides with point (3D version)
    static bool collides_segment_point_3d(const Point& p0,
                                          const Point& p1,
                                          const Point& point)
    {
      return CHECK_CGAL(_collides_segment_point_3d(p0, p1, point),
                        cgal_collides_segment_point_3d(p0, p1, point));
    }

    /// Check whether segment p0-p1 collides with segment q0-q1
    static bool collides_segment_segment(const Point& p0,
                                         const Point& p1,
                                         const Point& q0,
                                         const Point& q1,
                                         std::size_t gdim);

    /// Check whether segment p0-p1 collides with segment q0-q1 (1D version)
    static bool collides_segment_segment_1d(double p0,
                                            double p1,
                                            double q0,
                                            double q1)
    {
      return _collides_segment_segment_1d(p0, p1, q0, q1);
    }

    /// Check whether segment p0-p1 collides with segment q0-q1 (2D version)
    static bool collides_segment_segment_2d(const Point& p0,
                                            const Point& p1,
                                            const Point& q0,
                                            const Point& q1)
    {
      return CHECK_CGAL(_collides_segment_segment_2d(p0, p1, q0, q1),
                        cgal_collides_segment_segment_2d(p0, p1, q0, q1));
    }

    /// Check whether segment p0-p1 collides with segment q0-q1 (3D version)
    static bool collides_segment_segment_3d(const Point& p0,
                                            const Point& p1,
                                            const Point& q0,
                                            const Point& q1)
    {
      return CHECK_CGAL(_collides_segment_segment_3d(p0, p1, q0, q1),
                        cgal_collides_segment_segment_3d(p0, p1, q0, q1));
    }

    /// Check whether segment p collides with segment q0-q1 in its
    /// interior (2D version)
    static bool collides_interior_point_segment_2d(const Point& q0,
                                                   const Point& q1,
                                                   const Point& p)
    {
      return CHECK_CGAL(_collides_interior_point_segment_2d(q0, q1, p),
                        cgal_collides_segment_point_2d(q0, q1, p, true));
    }

    /// Check whether segment p collides with segment q0-q1 in its
    /// interior (3D version)
    static bool collides_interior_point_segment_3d(const Point& q0,
                                                   const Point& q1,
                                                   const Point& p)
    {
      return CHECK_CGAL(_collides_interior_point_segment_3d(q0, q1, p),
                        cgal_collides_segment_point_3d(q0, q1, p, true));
    }

    /// Check whether triangle p0-p1-p2 collides with point
    static bool collides_triangle_point(const Point& p0,
                                        const Point& p1,
                                        const Point& p2,
                                        const Point& point,
                                        std::size_t gdim);

    /// Check whether triangle p0-p1-p2 collides with point (2D version)
    static bool collides_triangle_point_2d(const Point& p0,
                                           const Point& p1,
                                           const Point& p2,
                                           const Point& point)
    {
      return CHECK_CGAL(_collides_triangle_point_2d(p0, p1, p2, point),
                        cgal_collides_triangle_point_2d(p0, p1, p2, point));
    }

    /// Check whether triangle p0-p1-p2 collides with point (3D version)
    static bool collides_triangle_point_3d(const Point& p0,
                                           const Point& p1,
                                           const Point& p2,
                                           const Point& point)
    {
      return CHECK_CGAL(_collides_triangle_point_3d(p0, p1, p2, point),
                        cgal_collides_triangle_point_3d(p0, p1, p2, point));
    }

    /// Check whether triangle p0-p1-p2 collides with segment q0-q1
    static bool collides_triangle_segment(const Point& p0,
                                          const Point& p1,
                                          const Point& p2,
                                          const Point& q0,
                                          const Point& q1,
                                          std::size_t gdim);

    /// Check whether triangle p0-p1-p2 collides with segment q0-q1 (2D version)
    static bool collides_triangle_segment_2d(const Point& p0,
                                             const Point& p1,
                                             const Point& p2,
                                             const Point& q0,
                                             const Point& q1)
    {
      return CHECK_CGAL(_collides_triangle_segment_2d(p0, p1, p2, q0, q1),
                        cgal_collides_triangle_segment_2d(p0, p1, p2, q0, q1));
    }

    /// Check whether triangle p0-p1-p2 collides with segment q0-q1 (3D version)
    static bool collides_triangle_segment_3d(const Point& p0,
                                             const Point& p1,
                                             const Point& p2,
                                             const Point& q0,
                                             const Point& q1)
    {
      return CHECK_CGAL(_collides_triangle_segment_3d(p0, p1, p2, q0, q1),
                        cgal_collides_triangle_segment_3d(p0, p1, p2, q0, q1));
    }

    /// Check whether triangle p0-p1-p2 collides with triangle q0-q1-q2
    static bool collides_triangle_triangle(const Point& p0,
                                           const Point& p1,
                                           const Point& p2,
                                           const Point& q0,
                                           const Point& q1,
                                           const Point& q2,
                                           std::size_t gdim);

    /// Check whether triangle p0-p1-p2 collides with triangle q0-q1-q2 (2D version)
    static bool collides_triangle_triangle_2d(const Point& p0,
                                              const Point& p1,
                                              const Point& p2,
                                              const Point& q0,
                                              const Point& q1,
                                              const Point& q2)
    {
      return CHECK_CGAL(_collides_triangle_triangle_2d(p0, p1, p2, q0, q1, q2),
                        cgal_collides_triangle_triangle_2d(p0, p1, p2, q0, q1, q2));
    }

    /// Check whether triangle p0-p1-p2 collides with triangle q0-q1-q2 (3D version)
    static bool collides_triangle_triangle_3d(const Point& p0,
                                              const Point& p1,
                                              const Point& p2,
                                              const Point& q0,
                                              const Point& q1,
                                              const Point& q2)
    {
      return CHECK_CGAL(_collides_triangle_triangle_3d(p0, p1, p2, q0, q1, q2),
                        cgal_collides_triangle_triangle_3d(p0, p1, p2, q0, q1, q2));
    }


    /// Check whether tetrahedron p0-p1-p2-p3 collides with point
    static bool collides_tetrahedron_point_3d(const Point& p0,
                                              const Point& p1,
                                              const Point& p2,
                                              const Point& p3,
                                              const Point& point)
    {
      return CHECK_CGAL(_collides_tetrahedron_point_3d(p0, p1, p2, p3, point),
                        cgal_collides_tetrahedron_point_3d(p0, p1, p2, p3, point));
    }

    /// Check whether tetrahedron point is in the interior of p0-p1-p2-p3
    /// ie.  collides but is not on the boundary
    static bool collides_interior_tetrahedron_point_3d(const Point& p0,
						       const Point& p1,
						       const Point& p2,
						       const Point& p3,
						       const Point& point)
    {
      return CHECK_CGAL(_collides_interior_tetrahedron_point_3d(p0, p1, p2, p3, point),
                        cgal_collides_interior_tetrahedron_point_3d(p0, p1, p2, p3, point));
    }


    /// Check whether tetrahedron p0-p1-p2-p3 collides with segment q0-q1
    static bool collides_tetrahedron_segment_3d(const Point& p0,
                                                const Point& p1,
                                                const Point& p2,
                                                const Point& p3,
                                                const Point& q0,
                                                const Point& q1)
    {
      return CHECK_CGAL(_collides_tetrahedron_segment_3d(p0, p1, p2, p3, q0, q1),
                        cgal_collides_tetrahedron_segment_3d(p0, p1, p2, p3, q0, q1));
    }

    /// Check whether tetrahedron p0-p1-p2-p3 collides with triangle q0-q1-q2
    static bool collides_tetrahedron_triangle_3d(const Point& p0,
                                                 const Point& p1,
                                                 const Point& p2,
                                                 const Point& p3,
                                                 const Point& q0,
                                                 const Point& q1,
                                                 const Point& q2)
    {
      return CHECK_CGAL(_collides_tetrahedron_triangle_3d(p0, p1, p2, p3, q0, q1, q2),
                        cgal_collides_tetrahedron_triangle_3d(p0, p1, p2, p3, q0, q1, q2));
    }

    /// Check whether tetrahedron p0-p1-p2-p3 collides with tetrahedron q0-q1-q2
    static bool collides_tetrahedron_tetrahedron_3d(const Point& p0,
                                                    const Point& p1,
                                                    const Point& p2,
                                                    const Point& p3,
                                                    const Point& q0,
                                                    const Point& q1,
                                                    const Point& q2,
                const Point& q3)
    {
      return CHECK_CGAL(_collides_tetrahedron_tetrahedron_3d(p0, p1, p2, p3, q0, q1, q2, q3),
                        cgal_collides_tetrahedron_tetrahedron_3d(p0, p1, p2, p3, q0, q1, q2, q3));
    }

  private:

    // Implementation of collision detection predicates

    static bool _collides_segment_point_1d(double p0,
                                           double p1,
                                           double point);

    static bool _collides_segment_point_2d(const Point& p0,
                                           const Point& p1,
                                           const Point& point);

    static bool _collides_segment_point_3d(const Point& p0,
                                           const Point& p1,
                                           const Point& point);

    static bool _collides_segment_segment_1d(double p0,
                                             double p1,
                                             double q0,
                                             double q1);

    static bool _collides_segment_segment_2d(const Point& p0,
                                             const Point& p1,
                                             const Point& q0,
                                             const Point& q1);

    static bool _collides_segment_segment_3d(const Point& p0,
                                             const Point& p1,
                                             const Point& q0,
                                             const Point& q1);

    static bool _collides_interior_point_segment_2d(const Point& q0,
                                                    const Point& q1,
                                                    const Point& p);

    static bool _collides_interior_point_segment_3d(const Point& q0,
                                                    const Point& q1,
                                                    const Point& p);

    static bool _collides_triangle_point_2d(const Point& p0,
                                            const Point& p1,
                                            const Point& p2,
                                            const Point& point);

    static bool _collides_triangle_point_3d(const Point& p0,
                                            const Point& p1,
                                            const Point& p2,
                                            const Point& point);

    static bool _collides_triangle_segment_2d(const Point& p0,
                                              const Point& p1,
                                              const Point& p2,
                                              const Point& q0,
                                              const Point& q1);

    static bool _collides_triangle_segment_3d(const Point& p0,
                                              const Point& p1,
                                              const Point& p2,
                                              const Point& q0,
                                              const Point& q1);

    static bool _collides_triangle_triangle_2d(const Point& p0,
                                               const Point& p1,
                                               const Point& p2,
                                               const Point& q0,
                                               const Point& q1,
                                               const Point& q2);

    static bool _collides_triangle_triangle_3d(const Point& p0,
                                               const Point& p1,
                                               const Point& p2,
                                               const Point& q0,
                                               const Point& q1,
                                               const Point& q2);

    static bool _collides_tetrahedron_point_3d(const Point& p0,
                                               const Point& p1,
                                               const Point& p2,
                                               const Point& p3,
                                               const Point& point);

    static bool _collides_interior_tetrahedron_point_3d(const Point& p0,
							const Point& p1,
							const Point& p2,
							const Point& p3,
							const Point& point);

    static bool _collides_tetrahedron_segment_3d(const Point& p0,
                                                 const Point& p1,
                                                 const Point& p2,
                                                 const Point& p3,
                                                 const Point& q0,
                                                 const Point& q1);

    static bool _collides_tetrahedron_triangle_3d(const Point& p0,
                                                  const Point& p1,
                                                  const Point& p2,
                                                  const Point& p3,
                                                  const Point& q0,
                                                  const Point& q1,
                                                  const Point& q2);

    static bool _collides_tetrahedron_tetrahedron_3d(const Point& p0,
                                                     const Point& p1,
                                                     const Point& p2,
                                                     const Point& p3,
                                                     const Point& q0,
                                                     const Point& q1,
                                                     const Point& q2,
                                                     const Point& q3);
  };

}

#endif