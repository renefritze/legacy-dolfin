// Copyright (C) 2002 Johan Hoffman and Anders Logg.
// Licensed under the GNU GPL Version 2.

#include <dolfin.h>

using namespace dolfin;

class Single : public ODE
{
public:
  
  Single() : ODE(1)
  {
    T = 30.0;
  }
  
  real u0(unsigned int i)
  {
    return 0.0;
  }

  real f(const Vector& u, real t, unsigned int i)
  {
    return cos(t);
  }

};

class Harmonic : public ODE
{
public:
  
  Harmonic() : ODE(2)
  {
    // Final time
    T = 30.0;

    // Compute sparsity
    sparse();
  }

  real u0(unsigned int i)
  {
    if ( i == 0 )
      return 0.0;

    return 1.0;
  }

  real f(const Vector& u, real t, unsigned int i)
  {
    if ( i == 0 )
      return u(1);

    return -u(0);
  }
  
};

class SpringSystem : public ODE
{
public:
  
  SpringSystem(unsigned int N) : ODE(2*N)
  {
    // Final time
    T = 5.0;

    // Compute sparsity
    sparse();
  }

  real u0(unsigned int i)
  {
    return 1.0;
  }

  real f(const Vector& u, real t, unsigned int i)
  {
    if ( i < N / 2 )
      return u(i+N/2);
    
    real k = (real) (i+1);
    return -k*u(i-N/2);
  }

};

int main()
{
  dolfin_set("output", "plain text");
  dolfin_set("debug time steps", 1);
  dolfin_set("tolerance", 0.01);
  dolfin_set("initial time step", 0.1);
  //dolfin_set("maximum time step", 1.0);
  //dolfin_set("fixed time step", true);
  dolfin_set("partitioning threshold", 1.0);
  dolfin_set("interval threshold", 0.9);
  dolfin_set("number of samples", 100);
  dolfin_set("element cache size", 32);
  dolfin_set("maximum iterations", 100);

  Single single;
  single.solve();

  //Harmonic harmonic;
  //harmonic.solve();
  
  //SpringSystem springSystem(10);
  //springSystem.solve();

  return 0;
}
