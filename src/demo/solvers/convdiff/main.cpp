// Copyright (C) 2002 Johan Hoffman and Anders Logg.
// Licensed under the GNU GPL Version 2.
//
// A simple test program for convection-diffusion, solving
//
//     du/dt + b.grad u - div a grad u = f
//
// around a hot dolphin in 2D, with convection given by
//
//     b(x,y,t) = (-10,0).

#include <dolfin.h>

using namespace dolfin;

// Source term
real f(real x, real y, real z, real t)
{
  return 0.0;
}

// Diffusivity
real a(real x, real y, real z, real t)
{
  return 0.1;
}

// Convection
real b(real x, real y, real z, real t, int i)
{
  if ( i == 0 )
    return 1.0;

  return 0.0;
}

// Boundary conditions
void mybc(BoundaryCondition& bc)
{
  // u = 0 on the inflow boundary
  if ( bc.coord().x == 1.0 )
    bc.set(BoundaryCondition::DIRICHLET, 0.0);

  // u = 1 on the dolphin
  if ( bc.node() < 77 )
    bc.set(BoundaryCondition::DIRICHLET, 1.0);
}

int main(int argc, char **argv)
{
  Grid grid("dolfin.xml.gz");
  Problem convdiff("convection-diffusion", grid);

  convdiff.set("source", f);
  convdiff.set("diffusivity", a);
  convdiff.set("convection", b);
  convdiff.set("boundary condition", mybc);
  convdiff.set("final time", 0.5);
  convdiff.set("time step", 0.1);

  convdiff.solve();
  
  return 0;
}
