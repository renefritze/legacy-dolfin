// Copyright (C) 2005 Anders Logg.
// Licensed under the GNU GPL Version 2.
//
// First added:  2005
// Last changed: 2006-03-13

#ifndef __LU_H
#define __LU_H

#include <petscksp.h>
#include <petscmat.h>

#include <dolfin/LinearSolver.h>
#include <dolfin/Parametrized.h>

namespace dolfin
{
  /// This class implements the direct solution (LU factorization) for
  /// linear systems of the form Ax = b. It is a wrapper for the LU
  /// solver of PETSc.
  
  class LU : public LinearSolver, public Parametrized
  {
  public:
    
    /// Constructor
    LU();

    /// Destructor
    ~LU();

    /// Solve linear system Ax = b
    uint solve(const Matrix& A, Vector& x, const Vector& b);

    /// Solve linear system Ax = b
    uint solve(const VirtualMatrix& A, Vector& x, const Vector& b);

    /// Display LU solver data
    void disp() const;

  private:
    
    // Create dense copy of virtual matrix
    real copyToDense(const VirtualMatrix& A);

    KSP ksp;

    Mat B;
    int* idxm;
    int* idxn;

    Vector e;
    Vector y;

  };

}

#endif
