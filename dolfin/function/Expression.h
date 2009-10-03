// Copyright (C) 2009 Anders Logg.
// Licensed under the GNU LGPL Version 2.1.
//
// First added:  2009-09-28
// Last changed: 2009-10-04

#ifndef __EXPRESSION_H
#define __EXPRESSION_H

#include "GenericFunction.h"

namespace dolfin
{

  class Data;

  /// This class represents a user-defined expression. Expressions can
  /// be used as coefficients in variational forms or interpolated
  /// into finite element spaces.
  ///
  /// An expression is defined by overloading the eval() method. Users
  /// may choose to overload either a simple version of eval(), in the
  /// case of expressions only depending on the coordinate x, or an
  /// optional version for functions depending on x and mesh data
  /// like cell indices or facet normals.

  class Expression : public GenericFunction
  {
  public:

    /// Constructor
    Expression();

    /// Destructor
    virtual ~Expression();

    /// Evaluate expression, must be overloaded by user (simple version)
    virtual void eval(double* values, const double* x) const;

    /// Evaluate expression, must be overloaded by user (optional version)
    virtual void eval(double* values, const Data& data) const;

    //--- Implementation of GenericFunction interface ---

    /// Restrict function to local cell (compute expansion coefficients w)
    virtual void restrict(double* w,
                          const FiniteElement& element,
                          const Cell& dolfin_cell,
                          const ufc::cell& ufc_cell,
                          int local_facet) const;

  };

}

#endif
