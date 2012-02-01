// Auto generated SWIG file for Python interface of DOLFIN
//
// Copyright (C) 2012 Kristian B. Oelgaard
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
// First added:  2012-01-18
// Last changed: 2012-01-18

// Autogenerated docstrings file, extracted from the DOLFIN source C++ files.

// Documentation extracted from: (module=math, header=basic.h)
%feature("docstring")  dolfin::ipow "
Return a to the power n
";

%feature("docstring")  dolfin::rand "
Return a random number, uniformly distributed between [0.0, 1.0)
";

%feature("docstring")  dolfin::seed "
Seed random number generator
";

%feature("docstring")  dolfin::near "
Check whether x is close to x0 (to within DOLFIN_EPS)
";

%feature("docstring")  dolfin::between "
Check whether x is between x0 and x1 (inclusive, to within DOLFIN_EPS)
";

// Documentation extracted from: (module=math, header=Lagrange.h)
%feature("docstring")  dolfin::Lagrange "
Lagrange polynomial (basis) with given degree q determined by n = q + 1 nodal points.

Example: q = 1 (n = 2)

  Lagrange p(1);
  p.set(0, 0.0);
  p.set(1, 1.0);

It is the callers reponsibility that the points are distinct.

This creates a Lagrange polynomial (actually two Lagrange polynomials):

  p(0,x) = 1 - x   (one at x = 0, zero at x = 1)
  p(1,x) = x       (zero at x = 0, one at x = 1)

";

%feature("docstring")  dolfin::Lagrange::Lagrange "
**Overloaded versions**

* Lagrange\ (q)

  Constructor

* Lagrange\ (p)

  Copy constructor
";

%feature("docstring")  dolfin::Lagrange::set "
Specify point
";

%feature("docstring")  dolfin::Lagrange::size "
Return number of points
";

%feature("docstring")  dolfin::Lagrange::degree "
Return degree
";

%feature("docstring")  dolfin::Lagrange::point "
Return point
";

%feature("docstring")  dolfin::Lagrange::operator "
Return value of polynomial i at given point x
";

%feature("docstring")  dolfin::Lagrange::eval "
Return value of polynomial i at given point x
";

%feature("docstring")  dolfin::Lagrange::ddx "
Return derivate of polynomial i at given point x
";

%feature("docstring")  dolfin::Lagrange::dqdx "
Return derivative q (a constant) of polynomial
";

%feature("docstring")  dolfin::Lagrange::str "
Return informal string representation (pretty-print)
";

// Documentation extracted from: (module=math, header=Legendre.h)
%feature("docstring")  dolfin::Legendre "
Interface for computing Legendre polynomials via Boost.
";

%feature("docstring")  dolfin::Legendre::eval "
Evaluate polynomial of order n at point x
";

%feature("docstring")  dolfin::Legendre::ddx "
Evaluate first derivative of polynomial of order n at point x
";

%feature("docstring")  dolfin::Legendre::d2dx "
Evaluate second derivative of polynomial of order n at point x
";

