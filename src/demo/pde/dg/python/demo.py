# This demo program solves Poisson's equation
#
#     - div grad u(x, y) = f(x, y)
#
# on the unit square with source f given by
#
#     f(x, y) = 500*exp(-((x-0.5)^2 + (y-0.5)^2)/0.02)
#
# and boundary conditions given by
#
#     u(x, y)     = 0
#     du/dn(x, y) = 0
#
# using a discontinuous Galerkin formulation (interior penalty method).

__author__ = "Kristian B. Oelgaard (k.b.oelgaard@tudelft.nl)"
__date__ = "2007-10-02 -- 2007-10-02"
__copyright__ = "Copyright (C) 2007 Kristian B. Oelgaard"
__license__  = "GNU LGPL Version 2.1"

from dolfin import *

# Create mesh and finite element
mesh = UnitSquare(64, 64)
element = FiniteElement("Discontinuous Lagrange", "triangle", 1)

# Source term
class Source(Function):
    def __init__(self, element, mesh):
        Function.__init__(self, element, mesh)
    def eval(self, values, x):
        dx = x[0] - 0.5
        dy = x[1] - 0.5
        values[0] = 500.0*exp(-(dx*dx + dy*dy)/0.02)

# Define variational problem
# Test and trial functions
v = TestFunction(element)
u = TrialFunction(element)
f = Source(element, mesh)

# Normal component, mesh size and right-hand side
n = FacetNormal("triangle", mesh)
h = AvgMeshSize("triangle", mesh)

# Parameters
alpha = 4.0
gamma = 8.0

# Bilinear form
a = dot(grad(v), grad(u))*dx \
   - dot(avg(grad(v)), jump(u, n))*dS \
   - dot(jump(v, n), avg(grad(u)))*dS \
   + alpha/h('+')*dot(jump(v, n), jump(u, n))*dS \
   - dot(grad(v), mult(u, n))*ds \
   - dot(mult(v, n), grad(u))*ds \
   + gamma/h*v*u*ds

# Linear form
L = v*f*dx

# Solve PDE and plot solution
pde = LinearPDE(a, L, mesh)
u = pde.solve()
plot(u)

# Save solution to file
file = File("poisson.pvd")
file << u
