// Copyright (C) 2007-2008 Anders Logg.
// Licensed under the GNU LGPL Version 2.1.
//
// Modified by Garth N. Wells, 2008.
//
// First added:  2007-04-02
// Last changed: 2008-10-23

#ifndef __FORM_H
#define __FORM_H

#include <vector>
#include <tr1/memory>
#include <dolfin/common/NoDeleter.h>

// Forward declaration
namespace ufc 
{
  class form; 
}

namespace dolfin
{

  // Forward declarations
  class FunctionSpace;
  class Function;

  /// Base class for UFC code generated by FFC for DOLFIN with option -l

  class Form
  {
  public:

    /// Constructor
    Form();

    /// Destructor
    virtual ~Form();

    /// Return rank of form (bilinear form = 2, linear form = 1, etc)
    uint rank() const
      { dolfin_assert(_ufc_form); return _ufc_form->rank(); }

    /// Return function space for given argument
    const FunctionSpace& function_space(uint i) const;

    /// Return function spaces
    const std::vector<FunctionSpace*> function_spaces() const;

    /// Return function for given coefficient
    const Function& coefficient(uint i) const;

    /// Return coefficient functions
    const std::vector<Function*> coefficients() const;

    /// Return UFC form
    const ufc::form& ufc_form() const;

  protected:

    // Function spaces (one for each argument)
    std::vector<std::tr1::shared_ptr<FunctionSpace> > _function_spaces;

    // Coefficients
    std::vector<std::tr1::shared_ptr<Function> > _coefficients;

    // Check that function spaces match the form
    void check() const;

    // The UFC form
    ufc::form* _ufc_form;

  };

}

#endif
