// Copyright (C) 2012 Anders Logg
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
// First added:  2012-08-20
// Last changed: 2012-08-21

#include "DefaultFactory.h"
#include "KrylovMatrix.h"

using namespace dolfin;

//-----------------------------------------------------------------------------
KrylovMatrix::KrylovMatrix(uint M, uint N)
{
  DefaultFactory factory;
  _A = factory.create_krylov_matrix();
  dolfin_assert(_A);
}
//-----------------------------------------------------------------------------
void KrylovMatrix::resize(uint M, uint N)
{
  dolfin_assert(_A);
  _A->resize(M, N);
}
//-----------------------------------------------------------------------------
dolfin::uint KrylovMatrix::size(uint dim) const
{
  dolfin_assert(_A);
  return _A->size(dim);
}
//-----------------------------------------------------------------------------
std::string KrylovMatrix::str(bool verbose) const
{
  return "FIXME";
}
//-----------------------------------------------------------------------------
