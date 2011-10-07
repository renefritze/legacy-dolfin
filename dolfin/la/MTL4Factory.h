// Copyright (C) 2008 Dag Lindbo
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
// Modified by Anders Logg, 2008.
//
// First added:  2008-07-06
// Last changed: 2008-08-11

#ifdef HAS_MTL4

#ifndef __MTL4_FACTORY_H
#define __MTL4_FACTORY_H

#include <string>
#include <boost/assign/list_of.hpp>

#include "ITLKrylovSolver.h"
#include "MTL4Matrix.h"
#include "MTL4Vector.h"
#include "GenericSparsityPattern.h"
#include "UmfpackLUSolver.h"
#include "LinearAlgebraFactory.h"

namespace dolfin
{

  class MTL4Factory : public LinearAlgebraFactory
  {
  public:

    /// Destructor
    virtual ~MTL4Factory() {}

    /// Create empty matrix
    MTL4Matrix* create_matrix() const
    { return new MTL4Matrix(); }

    /// Create empty vector (global)
    MTL4Vector* create_vector() const
    { return new MTL4Vector(); }

    /// Create empty vector (local)
    MTL4Vector* create_local_vector() const
    { return new MTL4Vector(); }

    /// Dummy sparsity pattern
    GenericSparsityPattern* create_pattern() const
    { return 0; }

    /// Create LU solver
    UmfpackLUSolver* create_lu_solver(std::string method) const
    { return new UmfpackLUSolver(); }

    /// Create Krylov solver
    ITLKrylovSolver* create_krylov_solver(std::string method,
                                          std::string preconditioner) const
    { return new ITLKrylovSolver(method, preconditioner); }

    /// List available LU methods
    std::vector<std::pair<std::string, std::string> > list_lu_methods() const
    {
      return boost::assign::pair_list_of
        ("umfpack", "UMFPACK (Unsymmetric MultiFrontal sparse LU factorization)");
    }

    /// List available Krylov methods
    std::vector<std::pair<std::string, std::string> > list_krylov_methods() const
    { return ITLKrylovSolver::list_methods(); }

    /// List available preconditioners
    std::vector<std::pair<std::string, std::string> > list_preconditioners() const
    { return ITLKrylovSolver::list_preconditioners(); }

    // Return singleton instance
    static MTL4Factory& instance()
    { return factory; }

  private:

    // Private constructor
    MTL4Factory() {}

    // Singleton instance
    static MTL4Factory factory;

  };
}

#endif

#endif
