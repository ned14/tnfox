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
* $Id: FXVec2f.h,v 1.6 2005/01/20 07:14:03 fox Exp $                            *
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
  friend FXAPI FXVec2f operator+(const FXVec2f& v){return v;}
  friend FXAPI FXVec2f operator-(const FXVec2f& v){return FXVec2f(-v.x,-v.y);}

  /// Adding
  friend FXAPI FXVec2f operator+(const FXVec2f& a,const FXVec2f& b){return FXVec2f(a.x+b.x,a.y+b.y);}

  /// Subtracting
  friend FXAPI FXVec2f operator-(const FXVec2f& a,const FXVec2f& b){return FXVec2f(a.x-b.x,a.y-b.y);}

  /// Scaling
  friend FXAPI FXVec2f operator*(const FXVec2f& a,FXfloat n){return FXVec2f(a.x*n,a.y*n);}
  friend FXAPI FXVec2f operator*(FXfloat n,const FXVec2f& a){return FXVec2f(n*a.x,n*a.y);}
  friend FXAPI FXVec2f operator/(const FXVec2f& a,FXfloat n){return FXVec2f(a.x/n,a.y/n);}
  friend FXAPI FXVec2f operator/(FXfloat n,const FXVec2f& a){return FXVec2f(n/a.x,n/a.y);}

  /// Dot product
  friend FXAPI FXfloat operator*(const FXVec2f& a,const FXVec2f& b){return a.x*b.x+a.y*b.y;}

  /// Test if zero
  friend FXAPI int operator!(const FXVec2f& a){return a.x==0.0f && a.y==0.0f;}

  /// Equality tests
  friend FXAPI int operator==(const FXVec2f& a,const FXVec2f& b){return a.x==b.x && a.y==b.y;}
  friend FXAPI int operator!=(const FXVec2f& a,const FXVec2f& b){return a.x!=b.x || a.y!=b.y;}

  friend FXAPI int operator==(const FXVec2f& a,FXfloat n){return a.x==n && a.y==n;}
  friend FXAPI int operator!=(const FXVec2f& a,FXfloat n){return a.x!=n || a.y!=n;}

  friend FXAPI int operator==(FXfloat n,const FXVec2f& a){return n==a.x && n==a.y;}
  friend FXAPI int operator!=(FXfloat n,const FXVec2f& a){return n!=a.x || n!=a.y;}

  /// Inequality tests
  friend FXAPI int operator<(const FXVec2f& a,const FXVec2f& b){return a.x<b.x && a.y<b.y;}
  friend FXAPI int operator<=(const FXVec2f& a,const FXVec2f& b){return a.x<=b.x && a.y<=b.y;}
  friend FXAPI int operator>(const FXVec2f& a,const FXVec2f& b){return a.x>b.x && a.y>b.y;}
  friend FXAPI int operator>=(const FXVec2f& a,const FXVec2f& b){return a.x>=b.x && a.y>=b.y;}

  friend FXAPI int operator<(const FXVec2f& a,FXfloat n){return a.x<n && a.y<n;}
  friend FXAPI int operator<=(const FXVec2f& a,FXfloat n){return a.x<=n && a.y<=n;}
  friend FXAPI int operator>(const FXVec2f& a,FXfloat n){return a.x>n && a.y>n;}
  friend FXAPI int operator>=(const FXVec2f& a,FXfloat n){return a.x>=n && a.y>=n;}

  friend FXAPI int operator<(FXfloat n,const FXVec2f& a){return n<a.x && n<a.y;}
  friend FXAPI int operator<=(FXfloat n,const FXVec2f& a){return n<=a.x && n<=a.y;}
  friend FXAPI int operator>(FXfloat n,const FXVec2f& a){return n>a.x && n>a.y;}
  friend FXAPI int operator>=(FXfloat n,const FXVec2f& a){return n>=a.x && n>=a.y;}

  /// Length and square of length
  friend FXAPI FXfloat len2(const FXVec2f& a){ return a.x*a.x+a.y*a.y; }
  friend FXAPI FXfloat len(const FXVec2f& a){ return sqrtf(len2(a)); }

  /// Normalize vector
  friend FXAPI FXVec2f normalize(const FXVec2f& a);

  /// Lowest or highest components
  friend FXAPI FXVec2f lo(const FXVec2f& a,const FXVec2f& b){return FXVec2f(FXMIN(a.x,b.x),FXMIN(a.y,b.y));}
  friend FXAPI FXVec2f hi(const FXVec2f& a,const FXVec2f& b){return FXVec2f(FXMAX(a.x,b.x),FXMAX(a.y,b.y));}

  /// Save vector to a stream
  friend FXAPI FXStream& operator<<(FXStream& store,const FXVec2f& v);

  /// Load vector from a stream
  friend FXAPI FXStream& operator>>(FXStream& store,FXVec2f& v);
  };

}

#endif
