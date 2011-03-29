// Copyright (C) 2009 Anders Logg.
// Licensed under the GNU LGPL Version 2.1.
//
// Modified by Marie E. Rognes (meg@simula.no) 2010.
// Modified by Garth N. Wells 2010.
//
// First added:  2009-12-08
// Last changed: 2010-12-28

#ifndef __EXTRAPOLATION_H
#define __EXTRAPOLATION_H

#include <map>
#include <set>
#include <vector>

#include <dolfin/common/types.h>

namespace arma
{
  template <class T> class Mat;
  template <class T> class Col;
}

namespace ufc
{
  class cell;
}

namespace dolfin
{

  class Cell;
  class DirichletBC;
  class Function;
  class FunctionSpace;
  class GenericFunctionSpace;

  /// This class implements an algorithm for extrapolating a function
  /// on a given function space from an approximation of that function
  /// on a possibly lower-order function space.
  ///
  /// This can be used to obtain a higher-order approximation of a
  /// computed dual solution, which is necessary when the computed
  /// dual approximation is in the test space of the primal problem,
  /// thereby being orthogonal to the residual.
  ///
  /// It is assumed that the extrapolation is computed on the same
  /// mesh as the original function.

  class Extrapolation
  {
  public:

    /// Compute extrapolation w from v
    static void extrapolate(Function& w, const Function& v);

  private:

    // Build data structures for unique dofs on patch of given cell
    static void build_unique_dofs(std::set<uint>& unique_dofs,
                                  std::map<uint, std::map<uint, uint> >& cell2dof2row,
                                  const Cell& cell0,
                                  const ufc::cell& c0,
                                  const GenericFunctionSpace& V);

    // Compute unique dofs in given cell
    static std::map<uint, uint> compute_unique_dofs(const Cell& cell, const ufc::cell& c,
                                                    const GenericFunctionSpace& V,
                                                    uint& row, std::set<uint>& unique_dofs);

    // Compute coefficients on given cell
    static void compute_coefficients(std::vector<std::vector<double> >& coefficients,
                                     const Function&v, const GenericFunctionSpace& V,
                                     const GenericFunctionSpace& W, const Cell& cell0,
                                     const ufc::cell& c0,
                                     const std::vector<uint>& dofs,
                                     uint& offset);

    // Add equations for current cell
    static void add_cell_equations(arma::Mat<double>& A,
                                   arma::Col<double>& b,
                                   const Cell& cell0,
                                   const Cell& cell1,
                                   const ufc::cell& c0,
                                   const ufc::cell& c1,
                                   const GenericFunctionSpace& V,
                                   const GenericFunctionSpace& W,
                                   const Function& v,
                                   std::map<uint, uint>& dof2row);

    // Average coefficients
    static void average_coefficients(Function& w,
                                     std::vector<std::vector<double> >& coefficients);

  };

}

#endif
