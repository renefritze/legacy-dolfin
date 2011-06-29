// Copyright (C) 2010 Marie E. Rognes
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
// Modified by Anders Logg, 2010-2011.
//
// First added:  2010-08-19
// Last changed: 2011-03-31

#ifndef __ADAPTIVE_NONLINEAR_VARIATIONAL_SOLVER_H
#define __ADAPTIVE_NONLINEAR_VARIATIONAL_SOLVER_H

#include <boost/shared_ptr.hpp>
#include <dolfin/fem/BoundaryCondition.h>

#include "GenericAdaptiveVariationalSolver.h"

namespace dolfin
{
  // Forward declarations
  class Function;
  class NonlinearVariationalProblem;
  class GoalFunctional;
  class Mesh;

  /// A class for goal-oriented adaptive solution of nonlinear
  /// variational problems.
  ///
  /// For a nonlinear variational problem of the form: find u in V
  /// satisfying
  ///
  ///     F(u; v) = 0 for all v in :math:`\hat V`
  ///
  /// and a corresponding conforming discrete problem: find u_h in V_h
  /// satisfying (at least approximately)
  ///
  ///     F(u_h; v) = 0 for all v in :math:`\hat V_h`
  ///
  /// and a given goal functional M and tolerance tol, the aim is to
  /// find a V_H and a u_H in V_H satisfying the discrete problem such
  /// that
  ///
  ///     \|M(u) - M(u_H)\| < tol
  ///
  /// This strategy is based on dual-weighted residual error
  /// estimators designed and automatically generated for the primal
  /// problem and subsequent h-adaptivity.

  class AdaptiveNonlinearVariationalSolver
    : public GenericAdaptiveVariationalSolver
  {
  public:

    /// Create AdaptiveNonlinearVariationalSolver
    ///
    /// *Arguments*
    ///
    ///     problem (_NonlinearVariationalProblem_)
    ///         The primal problem
    ///
    AdaptiveNonlinearVariationalSolver(NonlinearVariationalProblem& problem);

    /// Destructor
    ~AdaptiveNonlinearVariationalSolver() {/* Do nothing */};

    /// Solve problem such that the error measured in the goal
    /// functional 'M' is less than the given tolerance using the
    /// GoalFunctional's ErrorControl object.
    ///
    /// *Arguments*
    ///
    ///     tol  (double)
    ///         The error tolerance
    ///
    ///     goal  (_GoalFunctional_)
    ///         The goal functional
    ///
    virtual void solve(const double tol, GoalFunctional& M);

    /// Solve the primal problem.
    ///
    /// *Returns*
    ///
    ///     boost::shared_ptr<const _Function_>
    ///         The solution to the primal problem
    ///
    virtual boost::shared_ptr<const Function> solve_primal();

    /// Extract the boundary conditions for the primal problem.
    ///
    /// *Returns*
    ///
    ///     std::vector<boost::shared_ptr<const _BoundaryCondition_> >
    ///         The primal boundary conditions
    ///
    virtual std::vector<boost::shared_ptr<const BoundaryCondition> >
      extract_bcs() const;

    /// Evaluate the goal functional.
    ///
    /// *Arguments*
    ///
    ///    M (_Form_)
    ///        The functional to be evaluated
    ///
    ///    u (_Function_)
    ///        The function at which to evaluate the functional
    ///
    /// *Returns*
    ///
    ///     double
    ///         The value of M evaluated at u
    ///
    virtual double evaluate_goal(Form& M,
                                 boost::shared_ptr<const Function> u) const;

    /// Adapt the problem to other mesh.
    ///
    /// *Arguments*
    ///
    ///    mesh (_Mesh_)
    ///        The other mesh
    ///
    virtual void adapt_problem(boost::shared_ptr<const Mesh> mesh);

  private:

    // The problem
    boost::shared_ptr<NonlinearVariationalProblem> problem;

  };

}

#endif
