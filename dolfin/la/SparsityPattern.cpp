// Copyright (C) 2007-2009 Garth N. Wells.
// Licensed under the GNU LGPL Version 2.1.
//
// Modified by Magnus Vikstrom, 2008.
// Modified by Anders Logg, 2008-2009.
// Modified by Ola Skavhaug, 2009.
//
// First added:  2007-03-13
// Last changed: 2010-04-21

#include <algorithm>
#include <dolfin/log/dolfin_log.h>
#include <dolfin/main/MPI.h>
#include "SparsityPattern.h"

using namespace dolfin;

//-----------------------------------------------------------------------------
SparsityPattern::SparsityPattern() : row_range_min(0), row_range_max(0),
                                     col_range_min(0), col_range_max(0)
{
  // Do nothing
}
//-----------------------------------------------------------------------------
void SparsityPattern::init(uint rank, const uint* dims)
{
  // Only rank 1 and 2 sparsity patterns are supported
  assert(rank < 3);

  // Store dimensions
  shape.resize(rank);
  for (uint i = 0; i < rank; ++i)
    shape[i] = dims[i];

  // Clear sparsity pattern
  diagonal.clear();
  off_diagonal.clear();
  non_local.clear();

  // Check rank, ignore if not a matrix
  if (shape.size() != 2)
    return;

  // Get local range
  const std::pair<uint, uint> _row_range = local_range(0);
  const std::pair<uint, uint> _col_range = local_range(1);
  row_range_min = _row_range.first;
  row_range_max = _row_range.second;
  col_range_min = _col_range.first;
  col_range_max = _col_range.second;

  // Resize diagonal block
  assert(row_range_max > row_range_min);
  diagonal.resize(row_range_max - row_range_min);

  // Resize off-diagonal block (only needed when local range != global range)
  if (row_range_min != 0 || row_range_max != shape[0])
    off_diagonal.resize(row_range_max - row_range_min);
}
//-----------------------------------------------------------------------------
void SparsityPattern::insert(const uint* num_rows, const uint * const * rows)
{
  // Check rank, ignore if not a matrix
  if (shape.size() != 2)
    return;

  // Get local rows and columsn to insert
  const uint  m = num_rows[0];
  const uint  n = num_rows[1];
  const uint* map_i = rows[0];
  const uint* map_j = rows[1];

  // Check local range
  if (row_range_min == 0 && row_range_max == shape[0])
  {
    // Sequential mode, do simple insertion
    for (uint i = 0; i < m; ++i)
    {
      const uint I = map_i[i];
      for (uint j = 0; j < n; ++j)
        diagonal[I].insert(map_j[j]);
    }
  }
  else
  {
    // Parallel mode, use either diagonal, off_diagonal or non_local
    for (uint i = 0; i < m; ++i)
    {
      uint I = map_i[i];
      if (row_range_min <= I && I < row_range_max)
      {
        // Subtract offset
        I -= row_range_min;

        // Store local entry in diagonal or off-diagonal block
        for (uint j = 0; j < n; ++j)
        {
          const uint J = map_j[j];
          if (col_range_min <= J && J < col_range_max)
          {
            assert(I < diagonal.size());
            diagonal[I].insert(J);
          }
          else
          {
            assert(I < off_diagonal.size());
            off_diagonal[I].insert(J);
          }
        }
      }
      else
      {
        // Store non-local entry (communicated later during apply())
        for (uint j = 0; j < n; ++j)
        {
          const uint J = map_j[j];
          non_local.push_back(I);
          non_local.push_back(J);
        }
      }
    }
  }
}
//-----------------------------------------------------------------------------
dolfin::uint SparsityPattern::rank() const
{
  return shape.size();
}
//-----------------------------------------------------------------------------
dolfin::uint SparsityPattern::size(uint i) const
{
  assert(i < shape.size());
  return shape[i];
}
//-----------------------------------------------------------------------------
std::pair<dolfin::uint, dolfin::uint> SparsityPattern::local_range(uint dim) const
{
  assert(dim < 2);
  return MPI::local_range(size(dim));
}
//-----------------------------------------------------------------------------
dolfin::uint SparsityPattern::num_nonzeros() const
{
  uint nz = 0;
  typedef std::vector<set_type>::const_iterator row_it;
  for (row_it row = diagonal.begin(); row != diagonal.end(); ++row)
    nz += row->size();
  return nz;
}
//-----------------------------------------------------------------------------
void SparsityPattern::num_nonzeros_diagonal(uint* num_nonzeros) const
{
  // Check rank
  if (shape.size() != 2)
    error("Non-zero entries per row can be computed for matrices only.");

  // Compute number of nonzeros per row
  typedef std::vector<set_type>::const_iterator row_it;
  for (row_it row = diagonal.begin(); row != diagonal.end(); ++row)
    num_nonzeros[row - diagonal.begin()] = row->size();
}
//-----------------------------------------------------------------------------
void SparsityPattern::num_nonzeros_off_diagonal(uint* num_nonzeros) const
{
  // Check rank
  if (shape.size() != 2)
    error("Non-zero entries per row can be computed for matrices only.");

  // Compute number of nonzeros per row
  typedef std::vector<set_type>::const_iterator row_it;
  for (row_it row = off_diagonal.begin(); row != off_diagonal.end(); ++row)
    num_nonzeros[row - off_diagonal.begin()] = row->size();
}
//-----------------------------------------------------------------------------
void SparsityPattern::apply()
{
  // Check rank, ignore if not a matrix
  if (shape.size() != 2)
    return;

  // Print some useful information
  if (get_log_level() <= DBG)
    info_statistics();

  // Communicate non-local blocks if any
  if (row_range_min != 0 || row_range_max != shape[0])
  {
    // Figure out correct process for each non-local entry
    assert(non_local.size() % 2 == 0);
    std::vector<uint> partition(non_local.size());
    for (uint i = 0; i < non_local.size(); i+= 2)
    {
      // Get row for non-local entry
      const uint I = non_local[i];

      // Figure out which process owns the row
      const uint p = MPI::index_owner(I, shape[0]);
      assert(p < MPI::num_processes());
      assert(p != MPI::process_number());
      partition[i] = p;
      partition[i + 1] = p;
    }

    // Communicate non-local entries
    MPI::distribute(non_local, partition);

    // Insert non-local entries received from other processes
    assert(non_local.size() % 2 == 0);
    for (uint i = 0; i < non_local.size(); i+= 2)
    {
      // Get row and column
      uint I = non_local[i];
      const uint J = non_local[i + 1];

      // Sanity check
      if (I < row_range_min || I >= row_range_max)
      {
        error("Received illegal sparsity pattern entry for row %d, not in range [%d, %d].",
              I, row_range_min, row_range_max);
      }

      // Subtract offset
      I -= row_range_min;

      // Insert in diagonal or off-diagonal block
      if (col_range_min <= J && J < col_range_max)
      {
        assert(I < diagonal.size());
        diagonal[I].insert(J);
      }
      else
      {
        assert(I < off_diagonal.size());
        off_diagonal[I].insert(J);
      }
    }

    // Clear non-local entries
    non_local.clear();
  }
}
//-----------------------------------------------------------------------------
std::string SparsityPattern::str() const
{
  // Check rank
  if (shape.size() != 2)
    error("Sparsity pattern can only be displayed for matrices.");

  // Print each row
  std::stringstream s;
  typedef set_type::const_iterator entry_it;
  for (uint i = 0; i < diagonal.size(); i++)
  {
    s << "Row " << i << ":";
    for (entry_it entry = diagonal[i].begin(); entry != diagonal[i].end(); ++entry)
      s << " " << *entry;
    s << std::endl;
  }

  return s.str();
}
//-----------------------------------------------------------------------------
std::vector<std::vector<dolfin::uint> > SparsityPattern::diagonal_pattern(Type type) const
{
  std::vector<std::vector<uint> > v(diagonal.size());
  for (uint i = 0; i < diagonal.size(); ++i)
    v[i] = std::vector<uint>(diagonal[i].begin(), diagonal[i].end());

  if (type == sorted)
  {
    for (uint i = 0; i < v.size(); ++i)
      std::sort(v[i].begin(), v[i].end());
  }
  return v;
}
//-----------------------------------------------------------------------------
std::vector<std::vector<dolfin::uint> > SparsityPattern::off_diagonal_pattern(Type type) const
{
  std::vector<std::vector<uint> > v(diagonal.size());
  for (uint i = 0; i < off_diagonal.size(); ++i)
    v[i] = std::vector<uint>(off_diagonal[i].begin(), off_diagonal[i].end());

  if (type == sorted)
  {
    for (uint i = 0; i < v.size(); ++i)
      std::sort(v[i].begin(), v[i].end());
  }
  return v;
}
//-----------------------------------------------------------------------------
void SparsityPattern::info_statistics() const
{
  // Count nonzeros in diagonal block
  uint num_nonzeros_diagonal = 0;
  for (uint i = 0; i < diagonal.size(); ++i)
    num_nonzeros_diagonal += diagonal[i].size();

  // Count nonzeros in off-diagonal block
  uint num_nonzeros_off_diagonal = 0;
  for (uint i = 0; i < off_diagonal.size(); ++i)
    num_nonzeros_off_diagonal += off_diagonal[i].size();

  // Count nonzeros in non-local block
  const uint num_nonzeros_non_local = non_local.size() / 2;

  // Count total number of nonzeros
  const uint num_nonzeros_total
    = num_nonzeros_diagonal + num_nonzeros_off_diagonal + num_nonzeros_non_local;

  // Return number of entries
  cout << "Matrix of size " << shape[0] << " x " << shape[1] << " has "
       << num_nonzeros_total << " nonzero entries." << endl;
  if (num_nonzeros_total != num_nonzeros_diagonal)
  {
    cout << "Diagonal: " << num_nonzeros_diagonal << " ("
         << (100.0 * static_cast<double>(num_nonzeros_diagonal) / static_cast<double>(num_nonzeros_total))
         << "\%), ";
    cout << "off-diagonal: " << num_nonzeros_off_diagonal << " ("
         << (100.0 * static_cast<double>(num_nonzeros_off_diagonal) / static_cast<double>(num_nonzeros_total))
         << "\%), ";
    cout << "non-local: " << num_nonzeros_non_local << " ("
         << (100.0 * static_cast<double>(num_nonzeros_non_local) / static_cast<double>(num_nonzeros_total))
         << "\%)";
    cout << endl;
  }
}
//-----------------------------------------------------------------------------
