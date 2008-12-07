"This demo illustrate basic plotting."

__author__ = "Anders Logg (logg@simula.no)"
__date__ = "2007-05-29 -- 2008-12-07"
__copyright__ = "Copyright (C) 2007-2008 Anders Logg"
__license__  = "GNU LGPL Version 2.1"

from dolfin import *
from math import sqrt

import sys

# Read and plot mesh from file
mesh = Mesh("dolfin-2.xml.gz")
mesh.order()

try:
    demos = [int(sys.argv[-1])]
except:
    demos = [0, 1, 2]

# Have some fun with the mesh
if 0 in demos:

    R = 0.15
    H = 0.025
    X = 0.3
    Y = 0.4
    dX = H
    dY = 1.5*H
    coordinates = mesh.coordinates()
    original = coordinates.copy()

    for i in xrange(100):

        if X < H or X > 1.0 - H:
            dX = -dX
        if Y < H or Y > 1.0 - H:
            dY = -dY
        X += dX
        Y += dY

        for j in xrange(mesh.numVertices()):
            x, y = coordinates[j]
            r = sqrt((x - X)**2 + (y - Y)**2)
            if r < R:
                coordinates[j] = [X + (r/R)**2*(x - X), Y + (r/R)**2*(y - Y)]

        plot(mesh)

        for j in xrange(mesh.numVertices()):
            coordinates[j] = original[j]

# Define a vector function
class VectorFunction(Function):
    def __init__(self, mesh):
        self.t = 0.0
        Function.__init__(self, mesh)

    def eval(self, values, x):
        dx = x[0] - self.t
        dy = x[1] - self.t
        values[0] = -dy*exp(-10.0*(dx*dx + dy*dy))
        values[1] = dx*exp(-10.0*(dx*dx + dy*dy))

    def rank(self):
        return 1

    def dim(self, i):
        return 2

if 1 in demos:
    # Plot scalar function
    V = FunctionSpace(mesh, "Lagrange", 1)
    f = Function(V, cppexpr="t * 100 * exp(-10.0 * (pow(x[0] - t, 2) + pow(x[1] - t, 2)))")
    f.t = 0.0
    for i in range(100):
        f.t += 0.01
        plot(f, rescale=True, title="Scalar function")

if 2 in demos:
    # Plot vector function
    mesh = UnitSquare(16, 16)
    V = VectorFunctionSpace(mesh, "Lagrange", 1)
    f = Function(V, cppexpr=("-(x[1] - t)*exp(-10.0*(pow(x[0] - t, 2) + pow(x[1] - t, 2)))",
                             " (x[0] - t)*exp(-10.0*(pow(x[0] - t, 2) + pow(x[1] - t, 2)))"))
    for i in range(200):
        f.t += 0.005
        plot(f, rescale=True, title="Vector function")
