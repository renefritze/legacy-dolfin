// Copyright (C) 2003-2008 Anders Logg.
// Licensed under the GNU LGPL Version 2.1.
//
// Modified by Benjamin Kehlet 2009
//
// First added:  2005-05-02
// Last changed: 2009-03-23

#include <dolfin/common/constants.h>
#include <dolfin/log/dolfin_log.h>
#include <dolfin/math/dolfin_math.h>
#include <dolfin/math/Lagrange.h>
#include <dolfin/quadrature/LobattoQuadrature.h>
#include <dolfin/la/uBLASVector.h>
#include <dolfin/la/uBLASDenseMatrix.h>
#include <dolfin/ode/ODE.h>
#include <dolfin/ode/SORSolver.h>
#include "cGqMethod.h"

using namespace dolfin;

//-----------------------------------------------------------------------------
cGqMethod::cGqMethod(uint q) : Method(q, q + 1, q)
{
  info("Initializing continuous Galerkin method cG(%d).", q);

  init();

  _type = Method::cG;

  p = 2*q;
}
//-----------------------------------------------------------------------------
real cGqMethod::ueval(real x0, real values[], real tau) const
{
  real sum = x0 * trial->eval(0, tau);
  for (uint i = 0; i < nn; i++)
    sum += values[i] * trial->eval(i + 1, tau);

  return sum;
}
//-----------------------------------------------------------------------------
real cGqMethod::residual(real x0, real values[], real f, real k) const
{
  real sum = x0 * derivatives[0];
  for (uint i = 0; i < nn; i++)
    sum += values[i] * derivatives[i + 1];

  return sum / k - f;
}
//-----------------------------------------------------------------------------
real cGqMethod::timestep(real r, real tol, real k0, real kmax) const
{
  // FIXME: Missing stability factor and interpolation constant

  if ( real_abs(r) < real_epsilon() )
    return kmax;

  const real qq = static_cast<real>(q);
  return real_pow(tol * real_pow(k0, q) / real_abs(r), 0.5 / qq);
}
//-----------------------------------------------------------------------------
real cGqMethod::error(real k, real r) const
{
  // FIXME: Missing interpolation constant
  return real_pow(k, static_cast<real>(q)) * real_abs(r);
}
//-----------------------------------------------------------------------------
void cGqMethod::get_nodal_values(const real& u0, const real* x, real* nodal_values) const
{
  nodal_values[0] = u0;

  for (uint i = 0; i < nn; i++)
    nodal_values[i+1] = x[i];
}
//-----------------------------------------------------------------------------
void cGqMethod::disp() const
{
  info("Data for the cG(%d) method", q);
  info("=========================");
  info("");

  info("Lobatto quadrature points and weights on [0,1]:");
  info("");
  info(" i   points                   weights");
  info("----------------------------------------------------");

  for (unsigned int i = 0; i < nq; i++)
    info("%2d   %.15e   %.15e", i, to_double(qpoints[i]), to_double(qweights[i]));
  info("");

  for (uint i = 0; i < nn; i++)
  {
    info("");
    info("cG(%d) weights for degree of freedom %d:", q, i);
    info("");
    info(" i   weights");
    info("---------------------------");
    for (unsigned int j = 0; j < nq; j++)
      info("%2d   %.15e", j, to_double(nweights[i][j]));
  }
  info("");

  info("cG(%d) weights in matrix format:", q);
  if ( q < 10 )
    info("-------------------------------");
  else
    info("--------------------------------");
  for (unsigned int i = 0; i < nn; i++)
  {
    for (uint j = 0; j < nq; j++)
      cout << nweights[i][j] << " ";
    cout << endl;
  }
}
//-----------------------------------------------------------------------------
void cGqMethod::compute_quadrature()
{
  // Use Lobatto quadrature
  LobattoQuadrature quadrature(nq);

  // Get quadrature points and rescale from [-1,1] to [0,1]
  for (uint i = 0; i < nq; i++)
    qpoints[i] = (quadrature.point(i) + 1.0) / 2.0;

  // Get nodal points and rescale from [-1,1] to [0,1]
  for (uint i = 0; i < nn; i++)
    npoints[i] = (quadrature.point(i + 1) + 1.0) / 2.0;

  // Get quadrature weights and rescale from [-1,1] to [0,1]
  for (uint i = 0; i < nq; i++)
    qweights[i] = 0.5 * quadrature.weight(i);
}
//-----------------------------------------------------------------------------
void cGqMethod::compute_basis()
{
  assert(!trial);
  assert(!test);

  // Compute Lagrange basis for trial space
  trial = new Lagrange(q);
  for (uint i = 0; i < nq; i++)
    trial->set(i, qpoints[i]);

  // Compute Lagrange basis for test space using the Lobatto points for q - 1
  test = new Lagrange(q - 1);
  if ( q > 1 )
  {
    LobattoQuadrature lobatto(nq - 1);
    for (uint i = 0; i < (nq - 1); i++)
      test->set(i, (lobatto.point(i) + 1.0) / 2.0);
  }
  else
    test->set(0, 1.0);
}
//-----------------------------------------------------------------------------
void cGqMethod::compute_weights()
{
  uBLASDenseMatrix A(q, q);
  ublas_dense_matrix& _A = A.mat();

  real A_real[q*q];
  //real_zero(q*q, A_real);

  real trial_ddx[nn * nq];
  real test_eval[nn * nq];

  for (uint a = 0; a < nn; ++a) {
    for (uint b = 0; b < nq; ++b) {
      trial_ddx[a + nn*b] = trial->ddx(a+1, qpoints[b]);
      test_eval[a + nn*b] = test->eval(a, qpoints[b]);
    }
  }

  // Compute matrix coefficients
  for (uint i = 0; i < nn; i++)
  {
    for (uint j = 0; j < nn; j++)
    {
      // Use Lobatto quadrature which is exact for the order we need, 2q-1
      real integral = 0.0;
      for (uint k = 0; k < nq; k++)
      {
        //real x = qpoints[k];
        //integral += qweights[k] * trial->ddx(j + 1, x) * test->eval(i, x);
	integral += qweights[k] * trial_ddx[j + nn*k] * test_eval[i + nn*k];
      }

      A_real[i + nn*j] = integral;
      _A(i, j) = to_double(integral);

    }
  }

  //A.disp();

  uBLASVector b(q);
  ublas_vector& _b = b.vec();
  real b_real[q];

  // Compute nodal weights for each degree of freedom (loop over points)
  for (uint i = 0; i < nq; i++)
  {
    // Get nodal point
    //real x = qpoints[i];
    
    // Evaluate test functions at current nodal point
    for (uint j = 0; j < nn; j++)
    {
      b_real[j] = test_eval[i*nn + j];
      _b[j] = to_double(b_real[j]);
    }

    //b.disp();

#ifndef HAS_GMP

    uBLASVector w(q);
    ublas_vector& _w = w.vec();

    // Solve for the weight functions at the nodal point
    A.solve(w, b);

    // Save weights including quadrature
    for (uint j = 0; j < nn; j++)
      nweights[j][i] = qweights[i] * _w[j];

#else

    real w_real[q];
    
    uBLASDenseMatrix A_inv(A);
    A_inv.invert();
    
    // Solve system using the double precision invert as preconditioner
    SORSolver::SOR_precond(q, A_real, w_real, b_real, A_inv, real_epsilon());

    for (uint j = 0; j < nn; ++j)
      nweights[j][i] = qweights[i] * w_real[j];
#endif

    //disp();
  }
}
//-----------------------------------------------------------------------------
