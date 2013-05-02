// Copyright (C) 2013 Anders Logg
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
// First added:  2013-04-09
// Last changed: 2013-05-02

#ifndef __BOUNDING_BOX_TREE_3D_H
#define __BOUNDING_BOX_TREE_3D_H

// FIXME: Testing
#include <iostream>



#include <vector>
#include <dolfin/common/constants.h>
#include <dolfin/log/log.h>
#include "GenericBoundingBoxTree.h"

namespace dolfin
{

  // Forward declarations
  class Mesh;
  class Point;

  /// This class implements a (distributed) axis aligned bounding box
  /// tree (AABB tree). Bounding box trees can be created from meshes
  /// and [other data structures, to be filled in].

  class BoundingBoxTree3D : public GenericBoundingBoxTree
  {
  public:

    /// Create empty bounding box tree
    BoundingBoxTree3D();

    /// Destructor
    ~BoundingBoxTree3D();

  protected:

    // Build bounding box tree (recursive, 3d)
    unsigned int build(std::vector<double>& leaf_bboxes,
                       const std::vector<unsigned int>::iterator& begin,
                       const std::vector<unsigned int>::iterator& end,
                       unsigned int gdim);

    // Check whether point is in bounding box
    bool point_in_bbox(const double* x, unsigned int node) const
    {
      const double* _x = bbox_coordinates.data() + 6*node;
      return (_x[0] - DOLFIN_EPS < x[0] && x[0] < _x[3] + DOLFIN_EPS &&
              _x[1] - DOLFIN_EPS < x[1] && x[1] < _x[4] + DOLFIN_EPS &&
              _x[2] - DOLFIN_EPS < x[2] && x[2] < _x[5] + DOLFIN_EPS);
    }

    void sort_bboxes(unsigned short int axis,
                     const std::vector<double>& leaf_bboxes,
                     const std::vector<unsigned int>::iterator& begin,
                     const std::vector<unsigned int>::iterator& middle,
                     const std::vector<unsigned int>::iterator& end);

  private:

    // Compute bounding box of bounding boxes (3d)
    void
    compute_bbox_of_bboxes(double* bbox,
                           unsigned short int& axis,
                           const std::vector<double>& leaf_bboxes,
                           const std::vector<unsigned int>::iterator& begin,
                           const std::vector<unsigned int>::iterator& end);

  };

}

#endif
