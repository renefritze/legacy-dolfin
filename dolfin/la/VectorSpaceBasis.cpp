// Copyright (C) 2013 Patrick E. Farrell
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
// First added:  2013-05-29
// Last changed: 2013-05-29

#include "VectorSpaceBasis.h"
#include <dolfin/common/constants.h>

using namespace dolfin;

//-----------------------------------------------------------------------------
VectorSpaceBasis::VectorSpaceBasis(std::vector<boost::shared_ptr<const GenericVector*> > basis, const bool check):
  _basis(basis)
{
  if (check)
  {
    if (!check_orthonormality())
    {
    dolfin_error("VectorSpaceBasis.cpp",
                 "verify orthonormality",
                 "Input vector space basis is not orthonormal");
    }
  }
}
//-----------------------------------------------------------------------------
bool VectorSpaceBasis::check_orthonormality() const
{
  for (std::size_t i = 0; i < _basis.size(); i++)
  {
    const GenericVector* vec_i = *_basis[i];
    for (std::size_t j = i; j < _basis.size(); j++)
    {
      double delta_ij = (i == j) ? 1.0 : 0.0;
      const GenericVector* vec_j = *_basis[j];
      double dot_ij = vec_i->inner(*vec_j);
      if (abs(delta_ij - dot_ij) > DOLFIN_EPS) return false;
    }
  }

  return true;
}
//-----------------------------------------------------------------------------
void VectorSpaceBasis::orthogonalize(GenericVector& x)
{
  for (std::size_t i = 0; i < _basis.size(); i++)
  {
    const GenericVector* vec_i = *_basis[i];
    double dot = vec_i->inner(x);
    x.axpy(-dot, *vec_i);
  }
}
//-----------------------------------------------------------------------------
const std::size_t VectorSpaceBasis::size() const
{
  return _basis.size();
}
//-----------------------------------------------------------------------------
const GenericVector* VectorSpaceBasis::operator[] (int i) const
{
  const GenericVector* vec = *_basis[i];
  return vec;
}
//-----------------------------------------------------------------------------
