// Copyright (C) 2004-2005 Johan Jansson and Anders Logg.
// Licensed under the GNU GPL Version 2.
//
// First added:  2004
// Last changed: 2005-12-19

#include <algorithm>
#include <cmath>

#include <dolfin/dolfin_log.h>
#include <dolfin/dolfin_math.h>
#include <dolfin/ParameterSystem.h>
#include <dolfin/MultiAdaptivity.h>
#include <dolfin/Partition.h>

using namespace dolfin;

//-----------------------------------------------------------------------------
Partition::Partition(uint N) : indices(N)
{
  // Get parameter for threshold
  threshold = get("partitioning threshold");

  // Reset all indices
  for (uint i = 0; i < N; i++)
    indices[i] = i;
}
//-----------------------------------------------------------------------------
Partition::~Partition()
{
  // Do nothing
}
//-----------------------------------------------------------------------------
dolfin::uint Partition::size() const
{
  return indices.size();
}
//-----------------------------------------------------------------------------
dolfin::uint Partition::index(uint i) const
{
  dolfin_assert(i < indices.size());  
  return indices[i];
}
//-----------------------------------------------------------------------------
real Partition::update(uint offset, uint& end, MultiAdaptivity& adaptivity,
		       real K)
{
  // Compute time steps for partition. We partition the components into two
  // groups, one group with k < Kpivot and one with k >= Kpivot.

  // Compute time step for partitioning
  real Kpivot = threshold * maximum(offset, adaptivity);

  // Comparison operator
  Less less(Kpivot, adaptivity);

  // Partition using std::partition
  Array<uint>::iterator start = indices.begin();
  std::advance(start, offset);
  Array<uint>::iterator middle = std::partition(start, indices.end(), less);
  
  // Compute pivot index
  end = std::distance(indices.begin(), middle);

  // Modify time step to the smallest k such that k >= Kpivot.
  Kpivot = minimum(offset, end, adaptivity);

  // Modify time step so K is a multiple of Kpivot
  //Kpivot = K / ceil(K / Kpivot);

  /*
  for (uint i = offset; i < end ; i++)
  {
    uint index = indices[i];
    cout << "i = " << index << ": " << adaptivity.timestep(index)
	 << " --> " << Kpivot << endl;
  }
  */

  return Kpivot;
}
//-----------------------------------------------------------------------------
void Partition::debug(uint offset, uint end) const
{
  cout << "Partition:";

  for (uint i = 0; i < indices.size(); i++)
  {
    if ( i == offset || i == end )
      cout << " |";

    cout << " " << indices[i];
  }

  cout << endl;
}
//-----------------------------------------------------------------------------
real Partition::maximum(uint offset, MultiAdaptivity& adaptivity) const
{
  real K = 0.0;

  for (uint i = offset; i < indices.size(); i++)
    K = std::max(adaptivity.timestep(indices[i]), K);

  return K;
}
//-----------------------------------------------------------------------------
real Partition::minimum(uint offset, uint end,
			   MultiAdaptivity& adaptivity) const
{
  real k = adaptivity.timestep(indices[offset]);

  for (uint i = offset + 1; i < end; i++)
    k = std::min(adaptivity.timestep(indices[i]), k);

  return k;
}
//-----------------------------------------------------------------------------
Partition::Less::Less(real& K, MultiAdaptivity& adaptivity)
  : K(K), adaptivity(adaptivity)
{
  // Do nothing
}
//-----------------------------------------------------------------------------
bool Partition::Less::operator()(uint index) const
{
  return adaptivity.timestep(index) >= K;
}
//-----------------------------------------------------------------------------
