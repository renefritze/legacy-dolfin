// Copyright (C) 2006 Garth N. Wells
// Licensed under the GNU GPL Version 2.
//
// First added:  2006-04-24
// Last changed: 

#ifndef __GENERIC_MATRIX_H
#define __GENERIC_MATRIX_H

#include <dolfin/constants.h>
#include <dolfin/Vector.h>

namespace dolfin
{
  /// This template provdies a uniform interface to both dense and sparse 
  /// matrices. It provides member functions that are required by functions 
  /// that operate with both dense and sparse matrices. 

  // FIXME: For testing
  class GenericMatrixNoTemplate
  {
  public:
 
    /// Constructor
    GenericMatrixNoTemplate(){}

    /// Destructor
    virtual ~GenericMatrixNoTemplate(){}

    /// Initialize M x N matrix
    virtual void init(uint M, uint N) = 0;

    /// Initialize M x N matrix with given maximum number of nonzeros in each row
    virtual void init(uint M, uint N, uint nzmax) = 0;
    
    /// Return number of rows (dim = 0) or columns (dim = 1) along dimension dim
    virtual uint size(uint dim)  const = 0;

    /// Set all entries to zero (want to replace this with GenericMatrix::clear())
    virtual const GenericMatrixNoTemplate& operator= (real zero) = 0;

    /// Set all entries to zero (meaning is different for sparse matrices)
    // FIXME: should be pure virtual but not implemented for Matrix
    virtual void clear() {}

    /// Return maximum number of nonzero entries
    virtual uint nzmax() const = 0;

    /// Add block of values
    virtual void add(const real block[], const int rows[], int m, const int cols[], int n) = 0;

    /// Apply changes to matrix (only needed for sparse matrices)
    virtual void apply() = 0;

    /// Set given rows to identity matrix
    virtual void ident(const int rows[], int m) = 0;
    
  };

  template < class T >
  class GenericMatrix 
  {
  public:
 
    /// Constructor
    GenericMatrix(){}

    /// Destructor
    ~GenericMatrix(){}

    /// Return the "leaf" object
    T& matrix() 
      { return static_cast<T&>(*this); }

    /// Return the "leaf" object (constant version)
    const T& matrix() const
      { return static_cast<const T&>(*this); }

    /// Initialize M x N matrix
    void init(uint M, uint N)
      { matrix().init(M, N); }

    /// Initialize M x N matrix with given maximum number of nonzeros in each row
    void init(uint M, uint N, uint nzmax)
      { matrix().init(M, N, nzmax); }

    /// Return number of rows (dim = 0) or columns (dim = 1) along dimension dim
    uint size(uint dim)  const
      { return matrix().size(dim); }

    /// Set all entries to zero (want to replace this with GenericMatrix::clear())
    GenericMatrix& operator= (real zero)
      { return matrix() = zero; }

    /// Set all entries to zero (meaning is different for sparse matrices)
    void clear()
      { matrix().clear(); }

    /// Return maximum number of nonzero entries
    uint nzmax() const
      { return matrix().nzmax(); }

    /// Add block of values
    void add(const real block[], const int rows[], int m, const int cols[], int n)
      { matrix().add(block, rows, m, cols, n); }

    /// Apply changes to matrix (only needed for sparse matrices)
    void apply()
      { matrix().apply(); }

    /// Set given rows to identity matrix
    void ident(const int rows[], int m)
      { matrix().ident(rows, m); }

  private:

    
  };  
}
#endif
