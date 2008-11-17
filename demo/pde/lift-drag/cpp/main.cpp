// Copyright (C) 2007-2008 Anders Logg.
// Licensed under the GNU LGPL Version 2.1.
//
// First added:  2007-05-14
// Last changed: 2008-11-17
//
// This demo demonstrates how to compute functionals (or forms
// in general) over subsets of the mesh. The two functionals
// lift and drag are computed for the pressure field around
// a dolphin. Here, we use the pressure field obtained from
// solving the Stokes equations (see demo program in the
// sub directory src/demo/pde/stokes/taylor-hood).
//
// The calculation only includes the pressure contribution (not shear
// forces).

#include <dolfin.h>
#include "Lift.h"
#include "Drag.h"

using namespace dolfin;

// Define sub domain for the dolphin
class Fish : public SubDomain
{
  bool inside(const double* x, bool on_boundary) const
  {
    return (x[0] > DOLFIN_EPS && x[0] < (1.0 - DOLFIN_EPS) && 
            x[1] > DOLFIN_EPS && x[1] < (1.0 - DOLFIN_EPS) &&
            on_boundary);
  }
};  

int main()
{
  // Read velocity field from file
  Function p("../pressure.xml.gz");

  // Functionals for lift and drag
  FacetNormal n;
  LiftFunctional L(p, n);
  DragFunctional D(p, n);

  // Assemble functionals over sub domain
  Fish fish;
  double lift = assemble(L, fish);
  double drag = assemble(D, fish);

  message("Lift: %f", lift);
  message("Drag: %f", drag);
}
