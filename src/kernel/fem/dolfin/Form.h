// Copyright (C) 2007 Anders Logg.
// Licensed under the GNU LGPL Version 2.1.
//
// First added:  2007-04-02
// Last changed: 2007-04-05

#ifndef __FORM_H
#define __FORM_H

#include <ufc.h>

#include <dolfin/Array.h>
#include <dolfin/DofMapSet.h>
#include <dolfin/Function.h>
#include <dolfin/MeshFunction.h>

namespace dolfin
{

  /// Base class for UFC code generated by FFC for DOLFIN with option -l

  class Form
  {
  public:

    /// Constructor
    Form() : dof_map_set(0), local_dof_map_set(false) {}

    /// Destructor
    virtual ~Form();

    /// Return UFC form
    virtual const ufc::form& form() const = 0;

    /// Return array of coefficients
    virtual const Array<Function*>& coefficients() const = 0;

    /// Create degree of freedom maps 
    void updateDofMaps(Mesh& mesh);

    /// Create degree of freedom maps 
    void updateDofMaps(Mesh& mesh, MeshFunction<uint>& partitions);

    /// Set degree of freedom maps
    void setDofMaps(DofMapSet& dof_map_set);

    /// Return DofMapSet
    DofMapSet& dofMaps() const;

  private:

    // Degree of freedom maps
    DofMapSet* dof_map_set;

    // Local degree of freedom maps (locally owned)
    DofMapSet* local_dof_map_set;

  };

}

#endif
