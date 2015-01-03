// Copyright (C) 2015 Chris Richardson
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

#ifdef HAS_TRILINOS

#include <dolfin/common/MPI.h>
#include <dolfin/common/NoDeleter.h>
#include <dolfin/common/Timer.h>
#include "GenericMatrix.h"
#include "GenericVector.h"
#include "KrylovSolver.h"
#include "TpetraMatrix.h"
#include "TpetraVector.h"
#include "BelosKrylovSolver.h"

using namespace dolfin;

//-----------------------------------------------------------------------------
BelosKrylovSolver::BelosKrylovSolver(std::string method,
                                     std::string preconditioner)
  : _solver(NULL)
{
  // Check that the requested method is known
  // if (_methods.count(method) == 0)
  // {
  //   dolfin_error("BelosKrylovSolver.cpp",
  //                "create Belos Krylov solver",
  //                "Unknown Krylov method \"%s\"", method.c_str());
  // }

  // Set parameter values
  parameters = default_parameters();

  init(method);
}
//-----------------------------------------------------------------------------
BelosKrylovSolver::~BelosKrylovSolver()
{
}
//-----------------------------------------------------------------------------
void
BelosKrylovSolver::set_operator(std::shared_ptr<const GenericLinearOperator> A)
{
  set_operators(A, A);
}
//-----------------------------------------------------------------------------
void BelosKrylovSolver::set_operator(std::shared_ptr<const TpetraMatrix> A)
{
  set_operators(A, A);
}
//-----------------------------------------------------------------------------
void BelosKrylovSolver::set_operators(
  std::shared_ptr<const GenericLinearOperator> A,
  std::shared_ptr<const GenericLinearOperator> P)
{
  set_operators(as_type<const TpetraMatrix>(A),
                as_type<const TpetraMatrix>(P));
}
//-----------------------------------------------------------------------------
void
BelosKrylovSolver::set_operators(std::shared_ptr<const TpetraMatrix> A,
                                 std::shared_ptr<const TpetraMatrix> P)
{
  _matA = A;
  _matP = P;
  dolfin_assert(_matA);
  dolfin_assert(_matP);
  //  dolfin_assert(_solver);
}
//-----------------------------------------------------------------------------
const TpetraMatrix& BelosKrylovSolver::get_operator() const
{
  if (!_matA)
  {
    dolfin_error("BelosKrylovSolver.cpp",
                 "access operator for Belos Krylov solver",
                 "Operator has not been set");
  }
  return *_matA;
}
//-----------------------------------------------------------------------------
std::size_t BelosKrylovSolver::solve(GenericVector& x, const GenericVector& b)
{
  return solve(as_type<TpetraVector>(x), as_type<const TpetraVector>(b));
}
//-----------------------------------------------------------------------------
std::size_t BelosKrylovSolver::solve(const GenericLinearOperator& A,
                                     GenericVector& x,
                                     const GenericVector& b)
{
  return solve(as_type<const TpetraMatrix>(A),
               as_type<TpetraVector>(x),
               as_type<const TpetraVector>(b));
}
//-----------------------------------------------------------------------------
std::size_t BelosKrylovSolver::solve(TpetraVector& x, const TpetraVector& b)
{
  Timer timer("Belos Krylov solver");

  dolfin_assert(_matA);

  // Check dimensions
  const std::size_t M = _matA->size(0);
  const std::size_t N = _matA->size(1);

  if (_matA->size(0) != b.size())
  {
    dolfin_error("BelosKrylovSolver.cpp",
                 "unable to solve linear system with Belos Krylov solver",
                 "Non-matching dimensions for linear system (matrix has %ld rows and right-hand side vector has %ld rows)",
                 _matA->size(0), b.size());
  }

  // Write a message
  const bool report = parameters["report"];
  const int mpi_rank = MPI::rank(_matA->mpi_comm());

  if (report && mpi_rank == 0)
  {
    info("Solving linear system of size %ld x %ld (Belos Krylov solver).",
         M, N);
  }

  // Reinitialize solution vector if necessary
  if (x.empty())
  {
    _matA->init_vector(x, 1);
    x.zero();
  }

  // Set any Belos-specific options
  //   set_options();

  // Solve linear system
  if (mpi_rank == 0)
  {
    log(PROGRESS, "Belos Krylov solver starting to solve %i x %i system.",
        _matA->size(0), _matA->size(1));
  }


  Belos::ReturnType result =_solver->solve();
  if (result == Belos::Converged)
    std::cout << "Converged OK\n";

  const std::size_t num_iterations = _solver->getNumIters();

  return num_iterations;
}
//-----------------------------------------------------------------------------
std::size_t BelosKrylovSolver::solve(const TpetraMatrix& A,
                                     TpetraVector& x,
                                     const TpetraVector& b)
{
  // Set operator
  std::shared_ptr<const TpetraMatrix> Atmp(&A, NoDeleter());
  set_operator(Atmp);

  // Call solve
  return solve(x, b);
}
//-----------------------------------------------------------------------------
std::string BelosKrylovSolver::str(bool verbose) const
{
  std::stringstream s;

  s << "<BelosKrylovSolver>";

  return s.str();
}
//-----------------------------------------------------------------------------
void BelosKrylovSolver::init(const std::string& method)
{

  typedef Belos::LinearProblem<scalar_type, mv_type, op_type> problem_type;

  Teuchos::RCP<Teuchos::ParameterList> solverParams = Teuchos::parameterList();
  solverParams->set ("Num Blocks", 40);
  solverParams->set ("Maximum Iterations", 400);
  solverParams->set ("Convergence Tolerance", 1.0e-8);

  Belos::SolverFactory<scalar_type, mv_type, op_type> factory;
  _solver = factory.create("GMRES", solverParams);

  Teuchos::RCP<problem_type> problem
    = Teuchos::rcp(new problem_type);

  // problem->setProblem();

  _solver->setProblem(problem);
}
//-----------------------------------------------------------------------------
void BelosKrylovSolver::check_dimensions(const TpetraMatrix& A,
                                         const GenericVector& x,
                                         const GenericVector& b) const
{
  // Check dimensions of A
  if (A.size(0) == 0 || A.size(1) == 0)
  {
    dolfin_error("BelosKrylovSolver.cpp",
                 "unable to solve linear system with Belos Krylov solver",
                 "Matrix does not have a nonzero number of rows and columns");
  }

  // Check dimensions of A vs b
  if (A.size(0) != b.size())
  {
    dolfin_error("BelosKrylovSolver.cpp",
                 "unable to solve linear system with Belos Krylov solver",
                 "Non-matching dimensions for linear system (matrix has %ld rows and right-hand side vector has %ld rows)",
                 A.size(0), b.size());
  }

  // Check dimensions of A vs x
  if (!x.empty() && x.size() != A.size(1))
  {
    dolfin_error("BelosKrylovSolver.cpp",
                 "unable to solve linear system with Belos Krylov solver",
                 "Non-matching dimensions for linear system (matrix has %ld columns and solution vector has %ld rows)",
                 A.size(1), x.size());
  }

}
//-----------------------------------------------------------------------------

#endif
