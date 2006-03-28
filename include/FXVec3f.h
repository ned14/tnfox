/********************************************************************************
*                                                                               *
*       S i n g l e - P r e c i s i o n   3 - E l e m e n t   V e c t o r       *
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
* $Id: FXVec3f.h,v 1.12.2.1 2006/03/21 07:08:29 fox Exp $                           *
********************************************************************************/
#ifndef FXVEC3F_H
#define FXVEC3F_H

#include "fxdefs.h"
#include "FXVec2f.h"
#include <math.h>

namespace FX {


/// Single-precision 3-element vector
class FXAPI FXVec3f {
public:
  FXfloat x;
  FXfloat y;
  FXfloat z;
public:

  /// Default constructor
  FXVec3f(){}

  /// Copy constructor
  FXVec3f(const FXVec3f& v){x=v.x;y=v.y;z=v.z;}

  /// Initialize from array of floats
  FXVec3f(const FXfloat v[]){x=v[0];y=v[1];z=v[2];}

  /// Initialize with components
  FXVec3f(FXfloat xx,FXfloat yy,FXfloat zz=1.0f){x=xx;y=yy;z=zz;}

  /// Initialize with color
  FXVec3f(FXColor color);

  /// Return a non-const reference to the ith element
  FXfloat& operator[](FXint i){return (&x)[i];}

  /// Return a const reference to the ith element
  const FXfloat& operator[](FXint i) const {return (&x)[i];}

  /// Assign color
  FXVec3f& operator=(FXColor color);

  /// Assignment
  FXVec3f& operator=(const FXVec3f& v){x=v.x;y=v.y;z=v.z;return *this;}

  /// Assignment from array of floats
  FXVec3f& operator=(const FXfloat v[]){x=v[0];y=v[1];z=v[2];return *this;}

  /// Assigning operators
  FXVec3f& operator*=(FXfloat n){x*=n;y*=n;z*=n;return *this;}
  FXVec3f& operator/=(FXfloat n){x/=n;y/=n;z/=n;return *this;}
  FXVec3f& operator+=(const FXVec3f& v){x+=v.x;y+=v.y;z+=v.z;return *this;}
  FXVec3f& operator-=(const FXVec3f& v){x-=v.x;y-=v.y;z-=v.z;return *this;}

  /// Conversions
  operator FXfloat*(){return &x;}
  operator const FXfloat*() const {return &x;}
  operator FXVec2f&(){return *reinterpret_cast<FXVec2f*>(this);}
  operator const FXVec2f&() const {return *reinterpret_cast<const FXVec2f*>(this);}

  /// Convert to color
  operator FXColor() const;

  /// Unary
  friend inline FXVec3f operator+(const FXVec3f& v);
  friend inline FXVec3f operator-(const FXVec3f& v);

  /// Adding
  friend inline FXVec3f operator+(const FXVec3f& a,const FXVec3f& b);

  /// Subtracting
  friend inline FXVec3f operator-(const FXVec3f& a,const FXVec3f& b);

  /// Scaling
  friend inline FXVec3f operator*(const FXVec3f& a,FXfloat n);
  friend inline FXVec3f operator*(FXfloat n,const FXVec3f& a);
  friend inline FXVec3f operator/(const FXVec3f& a,FXfloat n);
  friend inline FXVec3f operator/(FXfloat n,const FXVec3f& a);

  /// Dot and cross products
  friend inline FXfloat operator*(const FXVec3f& a,const FXVec3f& b);
  friend inline FXVec3f operator^(const FXVec3f& a,const FXVec3f& b);

  /// Test if zero
  friend inline int operator!(const FXVec3f& a);

  /// Equality tests
  friend inline int operator==(const FXVec3f& a,const FXVec3f& b);
  friend inline int operator!=(const FXVec3f& a,const FXVec3f& b);

  friend inline int operator==(const FXVec3f& a,FXfloat n);
  friend inline int operator!=(const FXVec3f& a,FXfloat n);

  friend inline int operator==(FXfloat n,const FXVec3f& a);
  friend inline int operator!=(FXfloat n,const FXVec3f& a);

  /// Inequality tests
  friend inline int operator<(const FXVec3f& a,const FXVec3f& b);
  friend inline int operator<=(const FXVec3f& a,const FXVec3f& b);
  friend inline int operator>(const FXVec3f& a,const FXVec3f& b);
  friend inline int operator>=(const FXVec3f& a,const FXVec3f& b);

  friend inline int operator<(const FXVec3f& a,FXfloat n);
  friend inline int operator<=(const FXVec3f& a,FXfloat n);
  friend inline int operator>(const FXVec3f& a,FXfloat n);
  friend inline int operator>=(const FXVec3f& a,FXfloat n);

  friend inline int operator<(FXfloat n,const FXVec3f& a);
  friend inline int operator<=(FXfloat n,const FXVec3f& a);
  friend inline int operator>(FXfloat n,const FXVec3f& a);
  friend inline int operator>=(FXfloat n,const FXVec3f& a);

  /// Length and square of length
  friend inline FXfloat veclen2(const FXVec3f& a);
  friend inline FXfloat veclen(const FXVec3f& a);

  /// Normalize vector
  friend FXAPI FXVec3f vecnormalize(const FXVec3f& a);

  /// Lowest or highest components
  friend inline FXVec3f veclo(const FXVec3f& a,const FXVec3f& b);
  friend inline FXVec3f vechi(const FXVec3f& a,const FXVec3f& b);

  /// Compute normal from three points a,b,c
  friend FXAPI FXVec3f vecnormal(const FXVec3f& a,const FXVec3f& b,const FXVec3f& c);

  /// Compute approximate normal from four points a,b,c,d
  friend FXAPI FXVec3f vecnormal(const FXVec3f& a,const FXVec3f& b,const FXVec3f& c,const FXVec3f& d);

  /// Save vector to a stream
  friend FXAPI FXStream& operator<<(FXStream& store,const FXVec3f& v);

  /// Load vector from a stream
  friend FXAPI FXStream& operator>>(FXStream& store,FXVec3f& v);
  };


inline FXVec3f operator+(const FXVec3f& v){return v;}
inline FXVec3f operator-(const FXVec3f& v){return FXVec3f(-v.x,-v.y,-v.z);}

inline FXVec3f operator+(const FXVec3f& a,const FXVec3f& b){return FXVec3f(a.x+b.x,a.y+b.y,a.z+b.z);}
inline FXVec3f operator-(const FXVec3f& a,const FXVec3f& b){return FXVec3f(a.x-b.x,a.y-b.y,a.z-b.z);}

inline FXVec3f operator*(const FXVec3f& a,FXfloat n){return FXVec3f(a.x*n,a.y*n,a.z*n);}
inline FXVec3f operator*(FXfloat n,const FXVec3f& a){return FXVec3f(n*a.x,n*a.y,n*a.z);}
inline FXVec3f operator/(const FXVec3f& a,FXfloat n){return FXVec3f(a.x/n,a.y/n,a.z/n);}
inline FXVec3f operator/(FXfloat n,const FXVec3f& a){return FXVec3f(n/a.x,n/a.y,n/a.z);}

inline FXfloat operator*(const FXVec3f& a,const FXVec3f& b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline FXVec3f operator^(const FXVec3f& a,const FXVec3f& b){return FXVec3f(a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x);}

inline int operator!(const FXVec3f& a){return a.x==0.0f && a.y==0.0f && a.z==0.0f;}

inline int operator==(const FXVec3f& a,const FXVec3f& b){return a.x==b.x && a.y==b.y && a.z==b.z;}
inline int operator!=(const FXVec3f& a,const FXVec3f& b){return a.x!=b.x || a.y!=b.y || a.z!=b.z;}

inline int operator==(const FXVec3f& a,FXfloat n){return a.x==n && a.y==n && a.z==n;}
inline int operator!=(const FXVec3f& a,FXfloat n){return a.x!=n || a.y!=n || a.z!=n;}

inline int operator==(FXfloat n,const FXVec3f& a){return n==a.x && n==a.y && n==a.z;}
inline int operator!=(FXfloat n,const FXVec3f& a){return n!=a.x || n!=a.y || n!=a.z;}

inline int operator<(const FXVec3f& a,const FXVec3f& b){return a.x<b.x && a.y<b.y && a.z<b.z;}
inline int operator<=(const FXVec3f& a,const FXVec3f& b){return a.x<=b.x && a.y<=b.y && a.z<=b.z;}
inline int operator>(const FXVec3f& a,const FXVec3f& b){return a.x>b.x && a.y>b.y && a.z>b.z;}
inline int operator>=(const FXVec3f& a,const FXVec3f& b){return a.x>=b.x && a.y>=b.y && a.z>=b.z;}

inline int operator<(const FXVec3f& a,FXfloat n){return a.x<n && a.y<n && a.z<n;}
inline int operator<=(const FXVec3f& a,FXfloat n){return a.x<=n && a.y<=n && a.z<=n;}
inline int operator>(const FXVec3f& a,FXfloat n){return a.x>n && a.y>n && a.z>n;}
inline int operator>=(const FXVec3f& a,FXfloat n){return a.x>=n && a.y>=n && a.z>=n;}

inline int operator<(FXfloat n,const FXVec3f& a){return n<a.x && n<a.y && n<a.z;}
inline int operator<=(FXfloat n,const FXVec3f& a){return n<=a.x && n<=a.y && n<=a.z;}
inline int operator>(FXfloat n,const FXVec3f& a){return n>a.x && n>a.y && n>a.z;}
inline int operator>=(FXfloat n,const FXVec3f& a){return n>=a.x && n>=a.y && n>=a.z;}

inline FXfloat veclen2(const FXVec3f& a){ return a.x*a.x+a.y*a.y+a.z*a.z; }
inline FXfloat veclen(const FXVec3f& a){ return sqrtf(veclen2(a)); }

extern FXAPI FXVec3f vecnormalize(const FXVec3f& a);

/// Lowest or highest components
inline FXVec3f veclo(const FXVec3f& a,const FXVec3f& b){return FXVec3f(FXMIN(a.x,b.x),FXMIN(a.y,b.y),FXMIN(a.z,b.z));}
inline FXVec3f vechi(const FXVec3f& a,const FXVec3f& b){return FXVec3f(FXMAX(a.x,b.x),FXMAX(a.y,b.y),FXMAX(a.z,b.z));}

extern FXAPI FXVec3f vecnormal(const FXVec3f& a,const FXVec3f& b,const FXVec3f& c);
extern FXAPI FXVec3f vecnormal(const FXVec3f& a,const FXVec3f& b,const FXVec3f& c,const FXVec3f& d);

extern FXAPI FXStream& operator<<(FXStream& store,const FXVec3f& v);
extern FXAPI FXStream& operator>>(FXStream& store,FXVec3f& v);

}

#endif
