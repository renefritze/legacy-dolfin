// Copyright (C) 2011 Fredrik Valdmanis 
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
// First added:  2011-09-13
// Last changed:  2011-09-29

#ifdef HAS_PETSC_CUSP

#ifndef __PETSC_CUSP_FACTORY_H
#define __PETSC_CUSP_FACTORY_H

// TODO: FREDRIK: Should the solver includes be changed?
#include "PETScKrylovSolver.h"
#include "PETScLUSolver.h"
#include "PETScMatrix.h"
#include "PETScVector.h"
#include "SparsityPattern.h"
#include "LinearAlgebraFactory.h"

namespace dolfin
{

  class PETScCuspFactory : public LinearAlgebraFactory
  {
  public:

    /// Destructor
    virtual ~PETScCuspFactory() {}

    /// Create empty matrix
    PETScMatrix* create_matrix() const;

    /// Create empty vector (global)
    PETScVector* create_vector() const;

    /// Create empty vector (local)
    PETScVector* create_local_vector() const;

    /// Create empty sparsity pattern
    SparsityPattern* create_pattern() const;

    /// Create LU solver
    PETScLUSolver* create_lu_solver(std::string method) const;

    /// Create Krylov solver
    PETScKrylovSolver* create_krylov_solver(std::string method,
                                            std::string preconditioner) const;

    /// Return a list of available LU solver methods
    std::vector<std::pair<std::string, std::string> >
    lu_solver_methods() const;

    /// Return a list of available Krylov solver methods
    std::vector<std::pair<std::string, std::string> >
    krylov_solver_methods() const;

    /// Return a list of available preconditioners
    std::vector<std::pair<std::string, std::string> >
    krylov_solver_preconditioners() const;

    /// Return singleton instance
    static PETScCuspFactory& instance()
    { return factory; }

  private:

    /// Private constructor
    PETScCuspFactory() {}
    static PETScCuspFactory factory;

  };

}

#endif

#endif
