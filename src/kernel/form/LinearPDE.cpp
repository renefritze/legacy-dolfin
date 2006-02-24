// Copyright (C) 2004-2006 Anders Logg.
// Licensed under the GNU GPL Version 2.
//
// Modified by Garth N. Wells, 2006
//
// First added:  2004
// Last changed: 2006-02-22

#include <dolfin/dolfin_log.h>
#include <dolfin/Matrix.h>
#include <dolfin/Vector.h>
#include <dolfin/FEM.h>
#include <dolfin/GMRES.h>
#include <dolfin/LU.h>
#include <dolfin/BilinearForm.h>
#include <dolfin/LinearForm.h>
#include <dolfin/Mesh.h>
#include <dolfin/BoundaryCondition.h>
#include <dolfin/Function.h>
#include <dolfin/LinearPDE.h>
#include <dolfin/Parametrized.h>

using namespace dolfin;

//-----------------------------------------------------------------------------
LinearPDE::LinearPDE(BilinearForm& a, LinearForm& L, Mesh& mesh)
  : GenericPDE(), _a(&a), _Lf(&L), _mesh(&mesh), _bc(0)
{
  // Do nothing
}
//-----------------------------------------------------------------------------
LinearPDE::LinearPDE(BilinearForm& a, LinearForm& L, Mesh& mesh, 
  BoundaryCondition& bc) : GenericPDE(), _a(&a), _Lf(&L), _mesh(&mesh), _bc(&bc)
{
  // Do nothing
}
//-----------------------------------------------------------------------------
LinearPDE::~LinearPDE()
{
  // Do nothing
}
//-----------------------------------------------------------------------------
dolfin::uint LinearPDE::solve(Function& u)
{
  dolfin_assert(_a);
  dolfin_assert(_Lf);
  dolfin_assert(_mesh);

  // Write a message
  dolfin_info("Solving static linear PDE.");

  // Make sure u is a discrete function associated with the trial space
  u.init(*_mesh, _a->trial());
  Vector& x = u.vector();

  // Assemble linear system
  Matrix A;
  Vector b;
  if ( _bc )
    FEM::assemble(*_a, *_Lf, A, b, *_mesh, *_bc);
  else
    FEM::assemble(*_a, *_Lf, A, b, *_mesh);
  
  // Solve the linear system
  const std::string solver_type = get("solver");
  if ( solver_type == "direct" )
  {
    LU solver;
    solver.solve(A, x, b);
  }
  else if ( solver_type == "iterative" || solver_type == "default" )
  {
    GMRES solver;
    solver.solve(A, x, b);
  }
  else
    dolfin_error1("Unknown solver type \"%s\".", solver_type.c_str());

  return 0;
}
//-----------------------------------------------------------------------------
dolfin::uint LinearPDE::elementdim()
{
  dolfin_assert(_a);
  return _a->trial().elementdim();
}
//-----------------------------------------------------------------------------
BilinearForm& LinearPDE::a()
{
  dolfin_assert(_a);
  return *_a;
}
//-----------------------------------------------------------------------------
LinearForm& LinearPDE::L()
{
  dolfin_assert(_Lf);
  return *_Lf;
}
//-----------------------------------------------------------------------------
Mesh& LinearPDE::mesh()
{
  dolfin_assert(_mesh);
  return *_mesh;
}
//-----------------------------------------------------------------------------
BoundaryCondition& LinearPDE::bc()
{
  dolfin_assert(_bc);
  return *_bc;
}
//-----------------------------------------------------------------------------
