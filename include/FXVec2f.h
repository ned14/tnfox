/********************************************************************************
*                                                                               *
*       S i n g l e - P r e c i s i o n   2 - E l e m e n t   V e c t o r       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1994,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or                 *
* modify it under the terms of the GNU Lesser General Public                    *
* License as published by the Free Software Foundation; either                  *
* version 2.1 of the License, or (at your option) any later version.            *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU             *
* Lesser General Public License for more details.                               *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public              *
* License along with this library; if not, write to the Free Software           *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.    *
*********************************************************************************
* $Id: FXVec2f.h,v 1.6.2.1 2006/03/21 07:08:29 fox Exp $                            *
********************************************************************************/
#ifndef FXVEC2F_H
#define FXVEC2F_H

#include "fxdefs.h"
#include <math.h>

namespace FX {


/// Single-precision 2-element vector
class FXAPI FXVec2f {
public:
  FXfloat x;
  FXfloat y;
public:

  /// Default constructor
  FXVec2f(){}

  /// Copy constructor
  FXVec2f(const FXVec2f& v){x=v.x;y=v.y;}

  /// Initialize from array of floats
  FXVec2f(const FXfloat v[]){x=v[0];y=v[1];}

  /// Initialize with components
  FXVec2f(FXfloat xx,FXfloat yy){x=xx;y=yy;}

  /// Return a non-const reference to the ith element
  FXfloat& operator[](FXint i){return (&x)[i];}

  /// Return a const reference to the ith element
  const FXfloat& operator[](FXint i) const {return (&x)[i];}

  /// Assignment
  FXVec2f& operator=(const FXVec2f& v){x=v.x;y=v.y;return *this;}

  /// Assignment from array of floats
  FXVec2f& operator=(const FXfloat v[]){x=v[0];y=v[1];return *this;}

  /// Assigning operators
  FXVec2f& operator*=(FXfloat n){x*=n;y*=n;return *this;}
  FXVec2f& operator/=(FXfloat n){x/=n;y/=n;return *this;}
  FXVec2f& operator+=(const FXVec2f& v){x+=v.x;y+=v.y;return *this;}
  FXVec2f& operator-=(const FXVec2f& v){x-=v.x;y-=v.y;return *this;}

  /// Conversions
  operator FXfloat*(){return &x;}
  operator const FXfloat*() const {return &x;}

  /// Unary
  friend FXAPI FXVec2f operator+(const FXVec2f& v);
  friend FXAPI FXVec2f operator-(const FXVec2f& v);

  /// Adding
  friend FXAPI FXVec2f operator+(const FXVec2f& a,const FXVec2f& b);

  /// Subtracting
  friend FXAPI FXVec2f operator-(const FXVec2f& a,const FXVec2f& b);

  /// Scaling
  friend FXAPI FXVec2f operator*(const FXVec2f& a,FXfloat n);
  friend FXAPI FXVec2f operator*(FXfloat n,const FXVec2f& a);
  friend FXAPI FXVec2f operator/(const FXVec2f& a,FXfloat n);
  friend FXAPI FXVec2f operator/(FXfloat n,const FXVec2f& a);

  /// Dot product
  friend FXAPI FXfloat operator*(const FXVec2f& a,const FXVec2f& b);

  /// Test if zero
  friend FXAPI int operator!(const FXVec2f& a);

  /// Equality tests
  friend FXAPI int operator==(const FXVec2f& a,const FXVec2f& b);
  friend FXAPI int operator!=(const FXVec2f& a,const FXVec2f& b);

  friend FXAPI int operator==(const FXVec2f& a,FXfloat n);
  friend FXAPI int operator!=(const FXVec2f& a,FXfloat n);

  friend FXAPI int operator==(FXfloat n,const FXVec2f& a);
  friend FXAPI int operator!=(FXfloat n,const FXVec2f& a);

  /// Inequality tests
  friend FXAPI int operator<(const FXVec2f& a,const FXVec2f& b);
  friend FXAPI int operator<=(const FXVec2f& a,const FXVec2f& b);
  friend FXAPI int operator>(const FXVec2f& a,const FXVec2f& b);
  friend FXAPI int operator>=(const FXVec2f& a,const FXVec2f& b);

  friend FXAPI int operator<(const FXVec2f& a,FXfloat n);
  friend FXAPI int operator<=(const FXVec2f& a,FXfloat n);
  friend FXAPI int operator>(const FXVec2f& a,FXfloat n);
  friend FXAPI int operator>=(const FXVec2f& a,FXfloat n);

  friend FXAPI int operator<(FXfloat n,const FXVec2f& a);
  friend FXAPI int operator<=(FXfloat n,const FXVec2f& a);
  friend FXAPI int operator>(FXfloat n,const FXVec2f& a);
  friend FXAPI int operator>=(FXfloat n,const FXVec2f& a);

  /// Length and square of length
  friend FXAPI FXfloat veclen2(const FXVec2f& a);
  friend FXAPI FXfloat veclen(const FXVec2f& a);

  /// Normalize vector
  friend FXAPI FXVec2f vecnormalize(const FXVec2f& a);

  /// Lowest or highest components
  friend FXAPI FXVec2f veclo(const FXVec2f& a,const FXVec2f& b);
  friend FXAPI FXVec2f vechi(const FXVec2f& a,const FXVec2f& b);

  /// Save vector to a stream
  friend FXAPI FXStream& operator<<(FXStream& store,const FXVec2f& v);

  /// Load vector from a stream
  friend FXAPI FXStream& operator>>(FXStream& store,FXVec2f& v);
  };


inline FXVec2f operator+(const FXVec2f& v){return v;}
inline FXVec2f operator-(const FXVec2f& v){return FXVec2f(-v.x,-v.y);}

inline FXVec2f operator+(const FXVec2f& a,const FXVec2f& b){return FXVec2f(a.x+b.x,a.y+b.y);}
inline FXVec2f operator-(const FXVec2f& a,const FXVec2f& b){return FXVec2f(a.x-b.x,a.y-b.y);}

inline FXVec2f operator*(const FXVec2f& a,FXfloat n){return FXVec2f(a.x*n,a.y*n);}
inline FXVec2f operator*(FXfloat n,const FXVec2f& a){return FXVec2f(n*a.x,n*a.y);}
inline FXVec2f operator/(const FXVec2f& a,FXfloat n){return FXVec2f(a.x/n,a.y/n);}
inline FXVec2f operator/(FXfloat n,const FXVec2f& a){return FXVec2f(n/a.x,n/a.y);}

inline FXfloat operator*(const FXVec2f& a,const FXVec2f& b){return a.x*b.x+a.y*b.y;}

inline int operator!(const FXVec2f& a){return a.x==0.0f && a.y==0.0f;}

inline int operator==(const FXVec2f& a,const FXVec2f& b){return a.x==b.x && a.y==b.y;}
inline int operator!=(const FXVec2f& a,const FXVec2f& b){return a.x!=b.x || a.y!=b.y;}

inline int operator==(const FXVec2f& a,FXfloat n){return a.x==n && a.y==n;}
inline int operator!=(const FXVec2f& a,FXfloat n){return a.x!=n || a.y!=n;}

inline int operator==(FXfloat n,const FXVec2f& a){return n==a.x && n==a.y;}
inline int operator!=(FXfloat n,const FXVec2f& a){return n!=a.x || n!=a.y;}

inline int operator<(const FXVec2f& a,const FXVec2f& b){return a.x<b.x && a.y<b.y;}
inline int operator<=(const FXVec2f& a,const FXVec2f& b){return a.x<=b.x && a.y<=b.y;}
inline int operator>(const FXVec2f& a,const FXVec2f& b){return a.x>b.x && a.y>b.y;}
inline int operator>=(const FXVec2f& a,const FXVec2f& b){return a.x>=b.x && a.y>=b.y;}

inline int operator<(const FXVec2f& a,FXfloat n){return a.x<n && a.y<n;}
inline int operator<=(const FXVec2f& a,FXfloat n){return a.x<=n && a.y<=n;}
inline int operator>(const FXVec2f& a,FXfloat n){return a.x>n && a.y>n;}
inline int operator>=(const FXVec2f& a,FXfloat n){return a.x>=n && a.y>=n;}

inline int operator<(FXfloat n,const FXVec2f& a){return n<a.x && n<a.y;}
inline int operator<=(FXfloat n,const FXVec2f& a){return n<=a.x && n<=a.y;}
inline int operator>(FXfloat n,const FXVec2f& a){return n>a.x && n>a.y;}
inline int operator>=(FXfloat n,const FXVec2f& a){return n>=a.x && n>=a.y;}

inline FXfloat veclen2(const FXVec2f& a){ return a.x*a.x+a.y*a.y; }
inline FXfloat veclen(const FXVec2f& a){ return sqrtf(veclen2(a)); }

extern FXAPI FXVec2f vecnormalize(const FXVec2f& a);

inline FXVec2f veclo(const FXVec2f& a,const FXVec2f& b){return FXVec2f(FXMIN(a.x,b.x),FXMIN(a.y,b.y));}
inline FXVec2f vechi(const FXVec2f& a,const FXVec2f& b){return FXVec2f(FXMAX(a.x,b.x),FXMAX(a.y,b.y));}

extern FXAPI FXStream& operator<<(FXStream& store,const FXVec2f& v);
extern FXAPI FXStream& operator>>(FXStream& store,FXVec2f& v);

}

#endif
