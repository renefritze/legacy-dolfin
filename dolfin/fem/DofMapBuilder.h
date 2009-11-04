// Copyright (C) 2008 Anders Logg and Ola Skavhaug.
// Licensed under the GNU LGPL Version 2.1.
//
// Modified by Niclas Jansson 2009.
//
// First added:  2008-08-12
// Last changed: 2009-11-04

#ifndef __DOF_MAP_BUILDER_H
#define __DOF_MAP_BUILDER_H

namespace dolfin
{

  class DofMap;
  class UFC;
  class Mesh;

  /// Documentation of class

  class DofMapBuilder
  {
  public:

    /// Build dof map
    static void parallel_build(DofMap& dofmap, const Mesh& mesh);

  };

}

#endif


