// Copyright (C) 2004 Johan Hoffman and Anders Logg.
// Licensed under the GNU GPL Version 2.

#ifndef __NEW_MATRIX_H
#define __NEW_MATRIX_H

#include <petsc/petscmat.h>
#include <dolfin/constants.h>

namespace dolfin
{
  
  /// This class represents a matrix of dimension m x n. It is a
  /// simple wrapper for a PETSc matrix (Mat). The interface is
  /// intentionally simple. For advanced usage, access the PETSc Mat
  /// pointer using the function mat() and use the standard PETSc
  /// interface.

  class NewMatrix
  {
  public:

    /// Constructor
    NewMatrix();

    /// Constructor
    NewMatrix(uint m, uint n);

    /// Destructor
    ~NewMatrix();

    /// Initialize matrix with given number of rows and columns
    void init(uint m, uint n);

    /// Return number of rows (dim = 0) or columns (dim = 1) along dimension dim
    uint size(uint dim) const;

    /// Set all entries to zero
    NewMatrix& operator= (real zero);

    /// Add block of values
    void add(real block[], uint rows[], uint m, uint cols[], uint n);

    /// Apply changes to matrix
    void apply();

    /// Return PETSc Mat pointer
    Mat mat();
    
  private:

    // PETSc Mat pointer
    Mat A;

  };

}

#endif
