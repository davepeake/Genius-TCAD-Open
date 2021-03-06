// $Id: type_vector.h,v 1.5 2008/06/12 14:07:42 gdiso Exp $

// The libMesh Finite Element Library.
// Copyright (C) 2002-2007  Benjamin S. Kirk, John W. Peterson

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA



#ifndef __type_vector_h__
#define __type_vector_h__

// C++ includes
#include <cmath>

/* in the math.h, # define M_PI           3.14159265358979323846 */
#ifndef M_PI
 #define M_PI           3.14159265358979323846
#endif

// Local includes
#include "genius_common.h"
#include "compare_types.h"



// Forward declaration for friend class
template <typename T>
class TypeTensor;


/**
 * This class defines a vector in \p DIM dimensional space of type T.
 * T may either be Real or Complex.  The default constructor for
 * this class is protected, suggesting that you should not instantiate
 * one of these directly.  Instead use one of the derived types: \p Point
 * for a real-valued point in DIM-space, or \p SpaceVector for a real
 * or complex-valued vector in DIM-space.
 *
 * \author Benjamin S. Kirk, 2003.
 */

template <typename T>
class TypeVector
{
  friend class TypeTensor<T>;

protected:

  /**
   * Constructor.  By default sets all entries to 0.  Gives the vector 0 in
   * \p DIM dimensions.
   */
  TypeVector  (const T x=0.,
               const T y=0.,
               const T z=0.);

  /**
   * Constructor. set TypeVector by an array
   */
  TypeVector  (const T*);

public:

  /**
   * Copy-constructor.
   */
  TypeVector (const TypeVector<T>& p);

  /**
   * Destructor.
   */
  virtual ~TypeVector ();

  /**
   * Assign to a vector without creating a temporary.
   */
  void assign (const TypeVector<T> &);

  /**
   * @return the \f$ i^{th} \f$ element of the vector.
   */
  T coord(const unsigned int i) const;

  /**
   * @return the reference to \f$ i^{th} \f$ element of the vector.
   */
  const T & operator [] (const unsigned int i) const;

  /**
   * @return a writeable reference to the \f$ i^{th} \f$ element of the vector.
   */
  T & operator [] (const unsigned int i);


  /**
   * @return the reference to \f$ i^{th} \f$ element of the vector.
   */
  const T & operator () (const unsigned int i) const;

  /**
   * @return a writeable reference to the \f$ i^{th} \f$ element of the vector.
   */
  T & operator () (const unsigned int i);

  /**
   * Add two vectors.
   */
  TypeVector<T> operator + (const TypeVector<T> &) const;

  /**
   * Add to this vectors.
   */
  const TypeVector<T> & operator += (const TypeVector<T> &);

  /**
   * Add to this vector without creating a temporary.
   */
  void add (const TypeVector<T> &);

  /**
   * Add a scaled value to this vector without
   * creating a temporary.
   */
  template <typename T2>
  void add_scaled (const TypeVector<T2> &, const T);

  /**
   * Subtract two vectors.
   */
  TypeVector<T> operator - (const TypeVector<T> &) const;

  /**
   * Subtract from this vector.
   */
  const TypeVector<T> & operator -= (const TypeVector<T> &);

  /**
   * Subtract from this vector without creating a temporary.
   */
  void subtract (const TypeVector<T> &);

  /**
   * Subtract a scaled value from this vector without
   * creating a temporary.
   */
  template <typename T2>
  void subtract_scaled (const TypeVector<T2> &, const T);

  /**
   * Return the opposite of a vector
   */
  TypeVector<T> operator - () const;

  /**
   * Multiply a vector by a number, i.e. scale.
   */
  template <typename Scalar>
  typename boostcopy::enable_if_c<
  ScalarTraits<Scalar>::value,
  TypeVector<T> >::type
  operator * (const Scalar) const;

  /**
   * Multiply this vector by a number, i.e. scale.
   */
  template <typename Scalar>
  const TypeVector<T> & operator *= (const Scalar);

  /**
   * Divide a vector by a number, i.e. scale.
   */
  TypeVector<T> operator / (const T) const;

  /**
   * Divide this vector by a number, i.e. scale.
   */
  const TypeVector<T> & operator /= (const T);

  /**
   * Multiply 2 vectors together, i.e. dot-product.
   * The vectors may be of different types.
   */
  template <typename T2>
  T operator * (const TypeVector<T2> &) const;

  /**
   * Multiply 2 vectors together, i.e. dot-product.
   * The vectors may be of different types.
   */
  template <typename T2>
  T  dot (const TypeVector<T2> &) const;

  /**
   * Cross 2 vectors together, i.e. cross-product.
   */
  TypeVector<T> cross(const TypeVector<T> &) const;


  /**
   * angle between the vectors
   * @return angle of two vectors in range (0, PI)
   * the input order of 2 vectors is not important
   */
  T angle(const TypeVector<T> & ) const;

  /**
   * @return cos(angle) of two vectors
   */
  T cos_angle(const TypeVector<T> & ) const;

  /**
   * Think of a vector as a \p dim dimensional vector.  This
   * will return a unit vector aligned in that direction.
   * when zero is true, return zero vector when my size is zero
   */
  TypeVector<T> unit(bool zero=false) const;


  /**
   * convert itself to unit vector
   */
  TypeVector<T> & to_unit();


  /**
   * Returns the magnitude of the vector, i.e. the square-root of the
   * sum of the elements squared.
   */
  T size() const;

  /**
   * Returns the magnitude of the vector squared, i.e. the square-root
   * of the sum of the elements squared.
   */
  T size_sq() const;

  /**
   * Zero the vector in any dimension.
   */
  void zero();

  /**
   * assign vector v with min value this vector own. useful for find the bound box
   */
  void assign_min_to(TypeVector<T> &v) const;

  /**
   * assign vector v with max value this vector own. useful for find the bound box
   */
  void assign_max_to(TypeVector<T> &v) const;

  /**
   * @returns \p true iff two vectors occupy approximately the same
   * physical location in space, to within a relative tolerance of \p tol.
   */
  bool relative_fuzzy_equals(const TypeVector<T>& rhs, T tol = TOLERANCE) const;

  /**
   * @returns \p true iff two vectors occupy approximately the same
   * physical location in space, to within an absolute tolerance of \p tol.
   */
  bool absolute_fuzzy_equals(const TypeVector<T>& rhs, T tol = TOLERANCE) const;

  /**
   * @returns \p true iff two vectors occupy approximately the same
   * physical location in space, to within an absolute tolerance of \p TOLERANCE.
   */
  bool operator == (const TypeVector<T>& rhs) const;

  /**
   * @returns \p true iff two vectors do not occupy approximately the same
   * physical location in space.
   */
  bool operator != (const TypeVector<T>& rhs) const;

  /**
   * @returns \p true if this vector is "less"
   * than another.  Useful for sorting.
   * Also used for choosing some arbitrary basis function
   * orientations
   */
  bool operator < (const TypeVector<T>& rhs) const;

  /**
   * @returns \p true if this vector is "greater"
   * than another.  Useful for sorting.
   * Also used for choosing some arbitrary basis function
   * orientations
   */
  bool operator > (const TypeVector<T>& rhs) const;

  /**
   * Formatted print to \p std::cout.
   */
  void print(std::ostream& os) const;

  /**
   * Formatted print as above but allows you to do
   * Point p(1,2,3);
   * std::cout << p << std::endl;
   */
  friend std::ostream& operator << (std::ostream& os, const TypeVector<T>& t)
  {
    t.print(os);
    return os;
  }

  /**
   * Unformatted print to the stream \p out.  Simply prints the elements
   * of the vector separated by spaces.  Optionally prints a newline,
   * which it does by default.
   */
  void write_unformatted (std::ostream &out, const bool newline = true) const;

protected:

  /**
   * The coordinates of the \p TypeVector
   */
  T _coords[DIM];
};



//------------------------------------------------------
// Inline functions
template <typename T>
inline
TypeVector<T>::TypeVector (const T x,
                           const T y,
                           const T z)
{
  _coords[0] = x;

  if (DIM > 1)
  {
    _coords[1] = y;

    if (DIM == 3)
      _coords[2] = z;
  }
}


template <typename T>
inline
TypeVector<T>::TypeVector (const T* v)
{
  _coords[0] = v[0];

  if (DIM > 1)
  {
    _coords[1] = v[1];

    if (DIM == 3)
      _coords[2] = v[2];
  }
}


template <typename T>
inline
TypeVector<T>::TypeVector (const TypeVector<T> &p)
{
  // copy the nodes from vector p to me
  for (unsigned int i=0; i<DIM; i++)
    _coords[i] = p._coords[i];
}



template <typename T>
inline
TypeVector<T>::~TypeVector ()
{}



template <typename T>
inline
void TypeVector<T>::assign (const TypeVector<T> &p)
{
  for (unsigned int i=0; i<DIM; i++)
    _coords[i] = p._coords[i];
}


template <typename T>
inline
T TypeVector<T>::coord(const unsigned int i) const
{
  assert (i<3);
  return _coords[i];
}


template <typename T>
inline
const T & TypeVector<T>::operator [] (const unsigned int i) const
{
  assert (i<3);
  return _coords[i];
}



template <typename T>
inline
T & TypeVector<T>::operator [] (const unsigned int i)
{
#if DIM < 3

  if (i >= DIM)
  {
    //       std::cerr << "ERROR:  You are assigning to a vector component" << std::endl
    //      << "that is out of range for the compiled DIM!"      << std::endl
    //      << " DIM=" << DIM << " , i=" << i
    //      << std::endl;
    genius_error();
  }

#endif

  assert (i<DIM);

  return _coords[i];
}


template <typename T>
inline
const T & TypeVector<T>::operator () (const unsigned int i) const
{
  assert (i<3);
  return _coords[i];
}



template <typename T>
inline
T & TypeVector<T>::operator () (const unsigned int i)
{
#if DIM < 3

  if (i >= DIM)
  {
    //       std::cerr << "ERROR:  You are assigning to a vector component" << std::endl
    //      << "that is out of range for the compiled DIM!"      << std::endl
    //      << " DIM=" << DIM << " , i=" << i
    //      << std::endl;
    genius_error();
  }

#endif

  assert (i<DIM);

  return _coords[i];
}



template <typename T>
inline
TypeVector<T> TypeVector<T>::operator + (const TypeVector<T> &p) const
{

#if DIM == 1
  return TypeVector(_coords[0] + p._coords[0]);
#endif

#if DIM == 2
  return TypeVector(_coords[0] + p._coords[0],
                    _coords[1] + p._coords[1]);
#endif

#if DIM == 3
  return TypeVector(_coords[0] + p._coords[0],
                    _coords[1] + p._coords[1],
                    _coords[2] + p._coords[2]);
#endif

}



template <typename T>
inline
const TypeVector<T> & TypeVector<T>::operator += (const TypeVector<T> &p)
{
  this->add (p);

  return *this;
}



template <typename T>
inline
void TypeVector<T>::add (const TypeVector<T> &p)
{
#if DIM == 1
  _coords[0] += p._coords[0];
#endif

#if DIM == 2
  _coords[0] += p._coords[0];
  _coords[1] += p._coords[1];
#endif

#if DIM == 3
  _coords[0] += p._coords[0];
  _coords[1] += p._coords[1];
  _coords[2] += p._coords[2];
#endif

}



template <typename T>
template <typename T2>
inline
void TypeVector<T>::add_scaled (const TypeVector<T2> &p, const T factor)
{
#if DIM == 1
  _coords[0] += factor*p(0);
#endif

#if DIM == 2
  _coords[0] += factor*p(0);
  _coords[1] += factor*p(1);
#endif

#if DIM == 3
  _coords[0] += factor*p(0);
  _coords[1] += factor*p(1);
  _coords[2] += factor*p(2);
#endif

}



template <typename T>
inline
TypeVector<T> TypeVector<T>::operator - (const TypeVector<T> &p) const
{

#if DIM == 1
  return TypeVector(_coords[0] - p._coords[0]);
#endif

#if DIM == 2
  return TypeVector(_coords[0] - p._coords[0],
                    _coords[1] - p._coords[1]);
#endif

#if DIM == 3
  return TypeVector(_coords[0] - p._coords[0],
                    _coords[1] - p._coords[1],
                    _coords[2] - p._coords[2]);
#endif

}



template <typename T>
inline
const TypeVector<T> & TypeVector<T>::operator -= (const TypeVector<T> &p)
{
  this->subtract (p);

  return *this;
}



template <typename T>
inline
void TypeVector<T>::subtract (const TypeVector<T>& p)
{
  for (unsigned int i=0; i<DIM; i++)
    _coords[i] -= p._coords[i];
}



template <typename T>
template <typename T2>
inline
void TypeVector<T>::subtract_scaled (const TypeVector<T2> &p, const T factor)
{
  for (unsigned int i=0; i<DIM; i++)
    _coords[i] -= factor*p(i);
}



template <typename T>
inline
TypeVector<T> TypeVector<T>::operator - () const
{

#if DIM == 1
  return TypeVector(-_coords[0]);
#endif

#if DIM == 2
  return TypeVector(-_coords[0],
                    -_coords[1]);
#endif

#if DIM == 3
  return TypeVector(-_coords[0],
                    -_coords[1],
                    -_coords[2]);
#endif

}



template <typename T>
template <typename Scalar>
inline
typename boostcopy::enable_if_c<
ScalarTraits<Scalar>::value,
TypeVector<T> >::type
TypeVector<T>::operator * (const Scalar factor) const
{

#if DIM == 1
  return TypeVector(_coords[0]*factor);
#endif

#if DIM == 2
  return TypeVector(_coords[0]*factor,
                    _coords[1]*factor);
#endif

#if DIM == 3
  return TypeVector(_coords[0]*factor,
                    _coords[1]*factor,
                    _coords[2]*factor);
#endif
}



template <typename T, typename Scalar>
inline
typename boostcopy::enable_if_c<
ScalarTraits<Scalar>::value,
TypeVector<T> >::type
operator * (const Scalar factor,
            const TypeVector<T> &v)
{
  return v * factor;
}



template <typename T>
template <typename Scalar>
inline
const TypeVector<T> & TypeVector<T>::operator *= (const Scalar factor)
{
#if DIM == 1
  _coords[0] *= factor;
#endif

#if DIM == 2
  _coords[0] *= factor;
  _coords[1] *= factor;
#endif

#if DIM == 3
  _coords[0] *= factor;
  _coords[1] *= factor;
  _coords[2] *= factor;
#endif

  return *this;
}




template <typename T>
inline
TypeVector<T> TypeVector<T>::operator / (const T factor) const
{
  assert (factor != static_cast<T>(0.));

#if DIM == 1
  return TypeVector(_coords[0]/factor);
#endif

#if DIM == 2
  return TypeVector(_coords[0]/factor,
                    _coords[1]/factor);
#endif

#if DIM == 3
  return TypeVector(_coords[0]/factor,
                    _coords[1]/factor,
                    _coords[2]/factor);
#endif

}




template <typename T>
inline
const TypeVector<T> & TypeVector<T>::operator /= (const T factor)
{
  assert (factor != static_cast<T>(0.));

  for (unsigned int i=0; i<DIM; i++)
    _coords[i] /= factor;

  return *this;
}




template <typename T>
template <typename T2>
inline
T TypeVector<T>::operator * (const TypeVector<T2> &p) const
{
#if DIM == 1
  return _coords[0]*p._coords[0];
#endif

#if DIM == 2
  return (_coords[0]*p._coords[0] +
          _coords[1]*p._coords[1]);
#endif

#if DIM == 3
  return (_coords[0]*p(0) +
          _coords[1]*p(1) +
          _coords[2]*p(2));
#endif
}


template <typename T>
template <typename T2>
inline
T TypeVector<T>::dot (const TypeVector<T2> &p) const
{
#if DIM == 1
  return _coords[0]*p._coords[0];
#endif

#if DIM == 2
  return (_coords[0]*p._coords[0] +
          _coords[1]*p._coords[1]);
#endif

#if DIM == 3
  return (_coords[0]*p(0) +
          _coords[1]*p(1) +
          _coords[2]*p(2));
#endif
}


template <typename T>
inline TypeVector<T> TypeVector<T>::cross(const TypeVector<T>& p) const
{
  assert (DIM == 3);

  // |     i          j          k    |
  // |(*this)(0) (*this)(1) (*this)(2)|
  // |   p(0)       p(1)       p(2)   |

  return TypeVector<T>(  _coords[1]*p._coords[2] - _coords[2]*p._coords[1],
                         -_coords[0]*p._coords[2] + _coords[2]*p._coords[0],
                         _coords[0]*p._coords[1] - _coords[1]*p._coords[0]);
}


//angle between the vectors
template <typename T>
inline T TypeVector<T>::angle( const TypeVector<T> & p ) const
{
  T len1 = this->size( );
  T len2 = p.size( );
  T denom = len1 * len2;

  T cosa = (*this)*p / denom;
  if ( cosa > 1.0 ) cosa = 1.0;
  if ( cosa < -1.0 ) cosa = -1.0;

  T alph = acos( cosa );
  return alph;
}


//cos angle between the vectors
template <typename T>
inline T TypeVector<T>::cos_angle( const TypeVector<T> & p ) const
{
  T len1 = this->size( );
  T len2 = p.size( );
  T denom = len1 * len2;

  T cosa = (*this)*p / denom;
  return cosa;
}


template <typename T>
inline
T TypeVector<T>::size() const
{
  return sqrt(this->size_sq());
}



template <typename T>
inline
void TypeVector<T>::zero()
{
  for (unsigned int i=0; i<DIM; i++)
    _coords[i] = 0.;
}



template <typename T>
inline
void TypeVector<T>::assign_min_to(TypeVector<T> &v) const
{
  for (unsigned int i=0; i<DIM; i++)
  {
    if( _coords[i] < v._coords[i]  )  v._coords[i] = _coords[i];
  }
}


template <typename T>
inline
void TypeVector<T>::assign_max_to(TypeVector<T> &v) const
{
  for (unsigned int i=0; i<DIM; i++)
  {
    if( _coords[i] > v._coords[i]  )  v._coords[i] = _coords[i];
  }
}


template <typename T>
inline
T TypeVector<T>::size_sq () const
{
  return (*this)*(*this);
}





template <typename T>
inline
bool TypeVector<T>::absolute_fuzzy_equals(const TypeVector<T>& rhs, T tol) const
{
#if DIM == 1
  return (fabs(_coords[0] - rhs._coords[0])
          <= tol);
#endif

#if DIM == 2
  return (fabs(_coords[0] - rhs._coords[0]) +
          fabs(_coords[1] - rhs._coords[1])
          <= tol);
#endif

#if DIM == 3
  return (fabs(_coords[0] - rhs._coords[0]) +
          fabs(_coords[1] - rhs._coords[1]) +
          fabs(_coords[2] - rhs._coords[2])
          <= tol);
#endif
}



template <typename T>
inline
bool TypeVector<T>::relative_fuzzy_equals(const TypeVector<T>& rhs, T tol) const
{
#if DIM == 1
  return this->absolute_fuzzy_equals(rhs, tol *
                                     (fabs(_coords[0]) + fabs(rhs._coords[0])));
#endif

#if DIM == 2
  return this->absolute_fuzzy_equals(rhs, tol *
                                     (fabs(_coords[0]) + fabs(rhs._coords[0]) +
                                      fabs(_coords[1]) + fabs(rhs._coords[1])));
#endif

#if DIM == 3
  return this->absolute_fuzzy_equals(rhs, tol *
                                     (fabs(_coords[0]) + fabs(rhs._coords[0]) +
                                      fabs(_coords[1]) + fabs(rhs._coords[1]) +
                                      fabs(_coords[2]) + fabs(rhs._coords[2])));
#endif
}



template <typename T>
inline
bool TypeVector<T>::operator == (const TypeVector<T>& rhs) const
{
#if DIM == 1
  return (_coords[0] == rhs._coords[0]);
#endif

#if DIM == 2
  return (_coords[0] == rhs._coords[0] &&
          _coords[1] == rhs._coords[1]);
#endif

#if DIM == 3
  return (_coords[0] == rhs._coords[0] &&
          _coords[1] == rhs._coords[1] &&
          _coords[2] == rhs._coords[2]);
#endif
}



template <>
inline
bool TypeVector<Real>::operator != (const TypeVector<Real>& rhs) const
{
  return (!(*this == rhs));
}







#endif // #define __type_vector_h__
