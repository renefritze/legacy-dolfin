// Copyright (C) 2007 Kristian B. Oelgaard.
// Licensed under the GNU LGPL Version 2.1.
//
// First added:  2007-11-23
// Last changed: 2007-11-23
//
// This demo program solves Poisson's equation
//
//     - div grad u(x) = f(x)
//
// on the unit square with source f given by
//
//     f(x) = 9.0*DOLFIN_PI * DOLFIN_PI * sin(3.0*DOLFIN_PI*x[0]);
//
// and boundary conditions given by
//
//     u(x)     = 0  for x = 0 and x = 1

#include <dolfin.h>
#include "Poisson.h"
  
using namespace dolfin;

// Boundary condition
class DirichletBoundary : public SubDomain
{
  bool inside(const real* x, bool on_boundary) const
  {
      return on_boundary;
  }
};

// Source term
class Source : public Function
{
public:
    
  Source(Mesh& mesh) : Function(mesh) {}

  real eval(const real* x) const
  {
      return 9.0*DOLFIN_PI * DOLFIN_PI * sin(3.0*DOLFIN_PI*x[0]);
  }
};

int main()
{
  // Create mesh
  UnitInterval mesh(15);

  // Set up BCs
  Function zero(mesh, 0.0);
  DirichletBoundary boundary;
  DirichletBC bc(zero, mesh, boundary);

  // Create source
  Source f(mesh);

  // Define PDE
  PoissonBilinearForm a;
  PoissonLinearForm L(f);
  LinearPDE pde(a, L, mesh, bc);

  // Solve PDE
  Function u;
  pde.solve(u);

  // Plot solution doesn't work yet
//  plot(u);


  // Save solution to file
  File file_u("poisson.pvd");
  file_u << u;

  return 0;
}
