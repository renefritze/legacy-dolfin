// Copyright (C) 2003 Johan Hoffman and Anders Logg.
// Licensed under the GNU GPL Version 2.

#include <dolfin/Element.h>
#include <dolfin/ODEFunction.h>

using namespace dolfin;

//-----------------------------------------------------------------------------
ODEFunction::ODEFunction(unsigned int N) : _elmdata(N)
{
  // Do nothing
}
//-----------------------------------------------------------------------------
ODEFunction::~ODEFunction()
{
  // Do nothing
}
//-----------------------------------------------------------------------------
real ODEFunction::operator() (unsigned int i, real t)
{
  // Get element
  Element* element = _elmdata.element(i,t);

  // Check if we got the element
  if ( !element )
    dolfin_error("Requested value not available.");

  // Evaluate element at given time
  return element->value(t);
}
//-----------------------------------------------------------------------------
ElementData& ODEFunction::elmdata()
{
  return _elmdata;
}
//-----------------------------------------------------------------------------
