#include <cmath>
#include <dolfin/Point.h>

//-----------------------------------------------------------------------------
Point::Point()
{
  x = 0.0;
  y = 0.0;
  z = 0.0;
}
//-----------------------------------------------------------------------------
Point::Point(real x)
{
  this->x = x;
  y = 0.0;
  z = 0.0;
}
//-----------------------------------------------------------------------------
Point::Point(real x, real y)
{
  this->x = x;
  this->y = y;
  z = 0.0;
}
//-----------------------------------------------------------------------------
Point::Point(real x, real y, real z)
{
  this->x = x;
  this->y = y;
  this->z = z;
}
//-----------------------------------------------------------------------------
real Point::dist(Point p)
{
  real dx = x - p.x;
  real dy = y - p.y;
  real dz = z - p.z;

  return sqrt( dx*dx + dy*dy + dz*dz );
}
//-----------------------------------------------------------------------------
