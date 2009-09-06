// Copyright (C) 2009 Johan Hake and Anders Logg.
// Licensed under the GNU LGPL Version 2.1.
//
// First added:  2009-09-05
// Last changed: 2009-09-06
//
// This demo demonstrates the DOLFIN parameter system.
//
// Try running this demo with
//
// ./demo --bar 1 --solver_parameters.max_iterations 1000 --petsc.info

#include <dolfin.h>

using namespace dolfin;

int main(int argc, char* argv[])
{
  //--- Demo of global DOLFIN parameters ---

  // Set some global DOLFIN parameters
  parameters("linear_algebra_backend") = "uBLAS";
  parameters("floating_point_precision") = 32;

  // Print global DOLFIN parameters
  info(parameters, true);
  cout << endl;

  // Save parameters to file
  File file("parameters.xml");
  file << parameters;

  // Read parameters from file
  Parameters parameters_copy;
  file >> parameters_copy;
  info(parameters_copy, true);
  cout << endl;

  //--- Demo of nested parameter sets ---

  // Create an application parameter set
  Parameters application_parameters("application_parameters");

  // Create application parameters
  application_parameters.add("foo", 1.0);
  application_parameters.add("bar", 100);
  application_parameters.add("pc", "amg");

  // Create a solver parameter set
  Parameters solver_parameters("solver_parameters");

  // Create solver parameters
  solver_parameters.add("max_iterations", 100);
  solver_parameters.add("tolerance", 1e-16);
  solver_parameters.add("relative_tolerance", 1e-16, 1e-16, 1.0);

  // Set range for parameter
  solver_parameters("max_iterations").set_range(0, 1000);

  // Set some parameter values
  solver_parameters("max_iterations") = 500;
  solver_parameters("relative_tolerance") = 0.1;

  // Set solver parameters as nested parameters of application parameters
  application_parameters.add(solver_parameters);

  // Parse command-line options
  application_parameters.parse(argc, argv);

  // Access parameter values
  double foo = application_parameters("foo");
  int bar = application_parameters("bar");
  double tol = application_parameters["solver_parameters"]("tolerance");

  // Print parameter values
  cout << "foo = " << foo << endl;
  cout << "bar = " << bar << endl;
  cout << "tol = " << tol << endl;
  cout << endl;

  // Print application parameters
  info(application_parameters, true);
  cout << endl;

  //--- Demo of Krylov solver parameters ---

  // Set a parameter for the Krylov solver
  KrylovSolver solver;
  solver.parameters("relative_tolerance") = 1e-20;

  // Print Krylov solver parameters
  info(solver.parameters, true);
  cout << endl;

  //--- Demo of updating a parameter set ---

  // Create a subset of the application parameter set
  Parameters parameter_subset("parameter_subset");
  parameter_subset.add("foo", 3.0);
  Parameters nested_subset("solver_parameters");
  nested_subset.add("max_iterations", 850);
  parameter_subset.add(nested_subset);

  // Update application parameters
  application_parameters.update(parameter_subset);
  info(application_parameters, true);

  return 0;
}
