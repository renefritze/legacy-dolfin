// Copyright (C) 2005-2011 Anders Logg
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
// Modified by Garth N. Wells 2009-2010
// Modified by Niclas Jansson 2009
// Modified by Fredrik Valdmanis 2011
//
// First added:  2005
// Last changed: 2011-11-11

#ifdef HAS_PETSC

#include <dolfin/common/Timer.h>

#include <boost/assign/list_of.hpp>
#include <dolfin/common/constants.h>
#include <dolfin/common/NoDeleter.h>
#include <dolfin/log/dolfin_log.h>
#include <dolfin/common/MPI.h>
#include <dolfin/parameter/GlobalParameters.h>
#include "LUSolver.h"
#include "PETScMatrix.h"
#include "PETScVector.h"
#include "PETScLUSolver.h"

using namespace dolfin;

// Utility function
namespace dolfin
{
  class PETScKSPDeleter
  {
  public:
    void operator() (KSP* ksp)
    {
      if (ksp)
      {
        #if PETSC_VERSION_MAJOR == 3 && PETSC_VERSION_MINOR <= 1
        KSPDestroy(*ksp);
        #else
        KSPDestroy(ksp);
        #endif
      }
      delete ksp;
    }
  };
}

// Compatibility with petsc 3.2
#if PETSC_VERSION_MAJOR == 3 && PETSC_VERSION_MINOR > 1
#define MAT_SOLVER_UMFPACK      MATSOLVERUMFPACK
#define MAT_SOLVER_MUMPS        MATSOLVERMUMPS
#define MAT_SOLVER_PASTIX       MATSOLVERPASTIX
#define MAT_SOLVER_PETSC        MATSOLVERPETSC
#define MAT_SOLVER_SPOOLES      MATSOLVERSPOOLES
#define MAT_SOLVER_SUPERLU_DIST MATSOLVERSUPERLU_DIST
#define MAT_SOLVER_SUPERLU      MATSOLVERSUPERLU
#endif

// List of available LU solvers
const std::map<std::string, const MatSolverPackage> PETScLUSolver::_methods
  = boost::assign::map_list_of("default", "")
                              #if PETSC_HAVE_UMFPACK
                              ("umfpack",      MAT_SOLVER_UMFPACK)
                              #endif
                              #if PETSC_HAVE_MUMPS
                              ("mumps",        MAT_SOLVER_MUMPS)
                              #endif
                              #if PETSC_HAVE_PASTIX
                              ("pastix",       MAT_SOLVER_PASTIX)
                              #endif
                              #if PETSC_HAVE_SPOOLES
                              ("spooles",      MAT_SOLVER_SPOOLES)
                              #endif
                              #if PETSC_HAVE_SUPERLU
                              ("superlu",      MAT_SOLVER_SUPERLU)
                              #endif
                              #if PETSC_HAVE_SUPERLU_DIST
                              ("superlu_dist", MAT_SOLVER_SUPERLU_DIST)
                              #endif
                              ("petsc",        MAT_SOLVER_PETSC);
//-----------------------------------------------------------------------------
const std::vector<std::pair<std::string, std::string> > PETScLUSolver::_methods_descr
  = boost::assign::pair_list_of("default", "default LU solver")
                               #if PETSC_HAVE_UMFPACK
                               ("umfpack", "UMFPACK (Unsymmetric MultiFrontal sparse LU factorization)")
                               #endif
                               #if PETSC_HAVE_MUMPS
                               ("mumps", "MUMPS (MUltifrontal Massively Parallel Sparse direct Solver)")
                               #endif
                               #if PETSC_HAVE_PASTIX
                               ("pastix", "PaStiX (Parallel Sparse matriX package)")
                               #endif
                               #if PETSC_HAVE_SPOOLES
                               ("spooles", "SPOOLES (SParse Object Oriented Linear Equations Solver)")
                               #endif
                               #if PETSC_HAVE_SUPERLU
                               ("superlu", "SuperLU")
                               #endif
                               #if PETSC_HAVE_SUPERLU_DIST
                               ("superlu_dist", "Parallel SuperLU")
                               #endif
                               ("petsc", "PETSc builtin LU solver");

//-----------------------------------------------------------------------------
std::vector<std::pair<std::string, std::string> >
PETScLUSolver::methods()
{
  return PETScLUSolver::_methods_descr;
}
//-----------------------------------------------------------------------------
Parameters PETScLUSolver::default_parameters()
{
  Parameters p(LUSolver::default_parameters());
  p.rename("petsc_lu_solver");

  // Number of threads per process for multi-threaded solvers
  p.add<uint>("num_threads");

  return p;
}
//-----------------------------------------------------------------------------
PETScLUSolver::PETScLUSolver(std::string method)
{
  // Set parameter values
  parameters = default_parameters();

  // Initialize PETSc LU solver
  init_solver(method);
}
//-----------------------------------------------------------------------------
PETScLUSolver::PETScLUSolver(boost::shared_ptr<const PETScMatrix> A,
                             std::string method) : A(A)
{
  // Check dimensions
  if (A->size(0) != A->size(1))
  {
    dolfin_error("PETScLUSolver.cpp",
                 "create PETSc LU solver",
                 "Cannot LU factorize non-square PETSc matrix");
  }

  // Set parameter values
  parameters = default_parameters();

  // Initialize PETSc LU solver
  init_solver(method);
}
//-----------------------------------------------------------------------------
PETScLUSolver::~PETScLUSolver()
{
  // Do nothing
}
//-----------------------------------------------------------------------------
void PETScLUSolver::set_operator(const boost::shared_ptr<const GenericLinearOperator> A)
{
  this->A = as_type<const PETScMatrix>(require_matrix(A));
  dolfin_assert(this->A);
}
//-----------------------------------------------------------------------------
void PETScLUSolver::set_operator(const boost::shared_ptr<const PETScMatrix> A)
{
  this->A = A;
  dolfin_assert(this->A);
}
//-----------------------------------------------------------------------------
const GenericLinearOperator& PETScLUSolver::get_operator() const
{
  if (!A)
  {
    dolfin_error("PETScLUSolver.cpp",
                 "access operator of PETSc LU solver",
                 "Operator has not been set");
  }
  return *A;
}
//-----------------------------------------------------------------------------
dolfin::uint PETScLUSolver::solve(GenericVector& x, const GenericVector& b)
{
  dolfin_assert(_ksp);
  dolfin_assert(A);

  // Downcast matrix and vectors
  const PETScVector& _b = as_type<const PETScVector>(b);
  PETScVector& _x = as_type<PETScVector>(x);

  // Check dimensions
  if (A->size(0) != b.size())
  {
    dolfin_error("PETScLUSolver.cpp",
                 "solve linear system using PETSc LU solver",
                 "Cannot factorize non-square PETSc matrix");
  }

  // Initialize solution vector if required (make compatible with A in parallel)
  if (A->size(1) != x.size())
    A->resize(x, 1);

  // Set PETSc operators (depends on factorization re-use options);
  set_petsc_operators();

  // Write a pre-solve message
  pre_report(*A);

  // FIXME: Check for solver type
  // Set number of threads if using PaStiX
  if (parameters["num_threads"].is_set())
  {
    // Use number of threads specified for LU solver
    // FIXME: This option is not used by PETSc 3.2
    PetscOptionsSetValue("-mat_pastix_threadnbr", parameters["num_threads"].value_str().c_str());
  }
  else
  {
    // Use global number of threads
    PetscOptionsSetValue("-mat_pastix_threadnbr", dolfin::parameters["num_threads"].value_str().c_str());
  }
  //PetscOptionsSetValue("-mat_pastix_verbose", "2");

  // Solve linear system
  KSPSolve(*_ksp, *_b.vec(), *_x.vec());

  return 1;
}
//-----------------------------------------------------------------------------
dolfin::uint PETScLUSolver::solve(const GenericLinearOperator& A,
                                  GenericVector& x,
                                  const GenericVector& b)
{
  return solve(as_type<const PETScMatrix>(require_matrix(A)),
               as_type<PETScVector>(x),
               as_type<const PETScVector>(b));
}
//-----------------------------------------------------------------------------
dolfin::uint PETScLUSolver::solve(const PETScMatrix& A, PETScVector& x,
                                  const PETScVector& b)
{
  boost::shared_ptr<const PETScMatrix> _A(&A, NoDeleter());
  set_operator(_A);
  return solve(x, b);
}
//-----------------------------------------------------------------------------
std::string PETScLUSolver::str(bool verbose) const
{
  std::stringstream s;

  if (verbose)
  {
    warning("Verbose output for PETScLUSolver not implemented, calling PETSc KSPView directly.");
    KSPView(*_ksp, PETSC_VIEWER_STDOUT_WORLD);
  }
  else
    s << "<PETScLUSolver>";

  return s.str();
}
//-----------------------------------------------------------------------------
boost::shared_ptr<KSP> PETScLUSolver::ksp() const
{
  return _ksp;
}
//-----------------------------------------------------------------------------
const MatSolverPackage PETScLUSolver::select_solver(std::string& method) const
{
  // Check package string
  if (_methods.count(method) == 0)
  {
    dolfin_error("PETScLUSolver.cpp",
                 "solve linear system using PETSc LU solver",
                 "Unknown LU method \"%s\"", method.c_str());
  }

  // Choose appropriate 'default' solver
  if (method == "default")
  {
    if (MPI::num_processes() == 1)
    {
      #if PETSC_HAVE_UMFPACK
      method = "umfpack";
      #elif PETSC_HAVE_MUMPS
      method = "mumps";
      #elif PETSC_HAVE_PASTIX
      method = "pastix";
      #elif PETSC_HAVE_SUPERLU
      method = "superlu";
      #elif PETSC_HAVE_SPOOLES
      method = "spooles";
      #else
      method = "petsc";
      warning("Using PETSc native LU solver. Consider configuring PETSc with an efficient LU solver (e.g. UMFPACK, MUMPS).");
      #endif
    }
    else
    {
      #if PETSC_HAVE_MUMPS
      method = "mumps";
      #elif PETSC_HAVE_PASTIX
      method = "pastix";
      #elif PETSC_HAVE_SPOOLES
      method = "spooles";
      #elif PETSC_HAVE_SUPERLU_DIST
      method = "superlu_dist";
      #else
      dolfin_error("PETScLUSolver.cpp",
                   "solve linear system using PETSc LU solver",
                   "No suitable solver for parallel LU found. Consider configuring PETSc with MUMPS or SPOOLES");
      #endif
    }
  }

  return _methods.find(method)->second;
}
//-----------------------------------------------------------------------------
void PETScLUSolver::init_solver(std::string& method)
{
  // Select solver
  const MatSolverPackage solver_package = select_solver(method);

  // Destroy old solver environment if necessary
  if (_ksp)
  {
    if (!_ksp.unique())
    {
      dolfin_error("PETScLUSolver.cpp",
                   "initialize PETSc LU solver",
                   "More than one object points to the underlying PETSc object");
    }
  }
  _ksp.reset(new KSP, PETScKSPDeleter());

  // Create solver
  if (MPI::num_processes() > 1)
    KSPCreate(PETSC_COMM_WORLD, _ksp.get());
  else
    KSPCreate(PETSC_COMM_SELF, _ksp.get());

  // Make solver preconditioner only
  KSPSetType(*_ksp, KSPPREONLY);

  // Set preconditioner to LU factorization
  PC pc;
  KSPGetPC(*_ksp, &pc);
  PCSetType(pc, PCLU);

  // Set solver package
  PCFactorSetMatSolverPackage(pc, solver_package);

  // Allow matrices with zero diagonals to be solved
  #if PETSC_VERSION_MAJOR == 3 && PETSC_VERSION_MINOR >= 1
  PCFactorSetShiftType(pc, MAT_SHIFT_NONZERO);
  PCFactorSetShiftAmount(pc, PETSC_DECIDE);
  #else
  PCFactorSetShiftNonzero(pc, PETSC_DECIDE);
  #endif
}
//-----------------------------------------------------------------------------
void PETScLUSolver::set_petsc_operators()
{
  dolfin_assert(A->mat());

  // Get some parameters
  const bool reuse_fact   = parameters["reuse_factorization"];
  const bool same_pattern = parameters["same_nonzero_pattern"];

  // Set operators with appropriate preconditioner option
  if (reuse_fact)
    KSPSetOperators(*_ksp, *A->mat(), *A->mat(), SAME_PRECONDITIONER);
  else if (same_pattern)
    KSPSetOperators(*_ksp, *A->mat(), *A->mat(), SAME_NONZERO_PATTERN);
  else
    KSPSetOperators(*_ksp, *A->mat(), *A->mat(), DIFFERENT_NONZERO_PATTERN);
}
//-----------------------------------------------------------------------------
void PETScLUSolver::pre_report(const PETScMatrix& A) const
{
  const MatSolverPackage solver_type;
  PC pc;
  KSPGetPC(*_ksp, &pc);
  PCFactorGetMatSolverPackage(pc, &solver_type);

  // Get parameter
  const bool report = parameters["report"];

  if (report && dolfin::MPI::process_number() == 0)
  {
    log(PROGRESS, "Solving linear system of size %d x %d (PETSc LU solver, %s).",
        A.size(0), A.size(1), solver_type);
  }
}
//-----------------------------------------------------------------------------
#endif
