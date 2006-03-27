/********************************************************************************
*                                                                               *
*       S i n g l e - P r e c i s i o n   4 - E l e m e n t   V e c t o r       *
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
* $Id: FXVec4f.h,v 1.17.2.1 2006/03/21 07:08:29 fox Exp $                           *
********************************************************************************/
#ifndef FXVEC4F_H
#define FXVEC4F_H

#include "FXVec3f.h"
#include <math.h>

namespace FX {


/// Single-precision 4-element vector
class FXAPI FXVec4f {
public:
  FXfloat x;
  FXfloat y;
  FXfloat z;
  FXfloat w;
public:

  /// Default constructor
  FXVec4f(){}

  /// Copy constructor
  FXVec4f(const FXVec4f& v){x=v.x;y=v.y;z=v.z;w=v.w;}

  /// Construct with 3-vector and optional scalar
  FXVec4f(const FXVec3f& v,FXfloat ww=1.0f){x=v.x;y=v.y;z=v.z;w=ww;}

  /// Construct from array of floats
  FXVec4f(const FXfloat v[]){x=v[0];y=v[1];z=v[2];w=v[3];}

  /// Construct from components
  FXVec4f(FXfloat xx,FXfloat yy,FXfloat zz,FXfloat ww=1.0f){x=xx;y=yy;z=zz;w=ww;}

  /// Construct from color
  FXVec4f(FXColor color);

  /// Return a non-const reference to the ith element
  FXfloat& operator[](FXint i){return (&x)[i];}

  /// Return a const reference to the ith element
  const FXfloat& operator[](FXint i) const {return (&x)[i];}

  /// Assign color
  FXVec4f& operator=(FXColor color);

  /// Assignment
  FXVec4f& operator=(const FXVec3f& v){x=v.x;y=v.y;z=v.z;w=1.0f;return *this;}
  FXVec4f& operator=(const FXVec4f& v){x=v.x;y=v.y;z=v.z;w=v.w;return *this;}

  /// Assignment from array of floats
  FXVec4f& operator=(const FXfloat v[]){x=v[0];y=v[1];z=v[2];w=v[3];return *this;}

  /// Assigning operators
  FXVec4f& operator*=(FXfloat n){x*=n;y*=n;z*=n;w*=n;return *this;}
  FXVec4f& operator/=(FXfloat n){x/=n;y/=n;z/=n;w/=n;return *this;}
  FXVec4f& operator+=(const FXVec4f& v){x+=v.x;y+=v.y;z+=v.z;w+=v.w;return *this;}
  FXVec4f& operator-=(const FXVec4f& v){x-=v.x;y-=v.y;z-=v.z;w-=v.w;return *this;}

  /// Conversion
  operator FXfloat*(){return &x;}
  operator const FXfloat*() const {return &x;}
  operator FXVec3f&(){return *reinterpret_cast<FXVec3f*>(this);}
  operator const FXVec3f&() const {return *reinterpret_cast<const FXVec3f*>(this);}

  /// Convert to color
  operator FXColor() const;

  /// Unary
  friend inline FXVec4f operator+(const FXVec4f& v);
  friend inline FXVec4f operator-(const FXVec4f& v);

  /// Adding
  friend inline FXVec4f operator+(const FXVec4f& a,const FXVec4f& b);

  /// Subtracting
  friend inline FXVec4f operator-(const FXVec4f& a,const FXVec4f& b);

  /// Scaling
  friend inline FXVec4f operator*(const FXVec4f& a,FXfloat n);
  friend inline FXVec4f operator*(FXfloat n,const FXVec4f& a);
  friend inline FXVec4f operator/(const FXVec4f& a,FXfloat n);
  friend inline FXVec4f operator/(FXfloat n,const FXVec4f& a);

  /// Dot product
  friend inline FXfloat operator*(const FXVec4f& a,const FXVec4f& b);

  /// Test if zero
  friend inline int operator!(const FXVec4f& a);

  /// Equality tests
  friend inline int operator==(const FXVec4f& a,const FXVec4f& b);
  friend inline int operator!=(const FXVec4f& a,const FXVec4f& b);

  friend inline int operator==(const FXVec4f& a,FXfloat n);
  friend inline int operator!=(const FXVec4f& a,FXfloat n);

  friend inline int operator==(FXfloat n,const FXVec4f& a);
  friend inline int operator!=(FXfloat n,const FXVec4f& a);

  /// Inequality tests
  friend inline int operator<(const FXVec4f& a,const FXVec4f& b);
  friend inline int operator<=(const FXVec4f& a,const FXVec4f& b);
  friend inline int operator>(const FXVec4f& a,const FXVec4f& b);
  friend inline int operator>=(const FXVec4f& a,const FXVec4f& b);

  friend inline int operator<(const FXVec4f& a,FXfloat n);
  friend inline int operator<=(const FXVec4f& a,FXfloat n);
  friend inline int operator>(const FXVec4f& a,FXfloat n);
  friend inline int operator>=(const FXVec4f& a,FXfloat n);

  friend inline int operator<(FXfloat n,const FXVec4f& a);
  friend inline int operator<=(FXfloat n,const FXVec4f& a);
  friend inline int operator>(FXfloat n,const FXVec4f& a);
  friend inline int operator>=(FXfloat n,const FXVec4f& a);

  /// Length and square of length
  friend inline FXfloat veclen2(const FXVec4f& a);
  friend inline FXfloat veclen(const FXVec4f& a);

  /// Normalize vector
  friend FXAPI FXVec4f vecnormalize(const FXVec4f& a);

  /// Lowest or highest components
  friend inline FXVec4f veclo(const FXVec4f& a,const FXVec4f& b);
  friend inline FXVec4f vechi(const FXVec4f& a,const FXVec4f& b);

  /// Compute normalized plane equation ax+by+cz+d=0
  friend FXAPI FXVec4f vecplane(const FXVec4f& vec);
  friend FXAPI FXVec4f vecplane(const FXVec3f& vec,FXfloat dist);
  friend FXAPI FXVec4f vecplane(const FXVec3f& vec,const FXVec3f& p);
  friend FXAPI FXVec4f vecplane(const FXVec3f& a,const FXVec3f& b,const FXVec3f& c);

  /// Signed distance normalized plane and point
  friend FXAPI FXfloat vecdistance(const FXVec4f& plane,const FXVec3f& p);

  /// Return true if edge a-b crosses plane
  friend FXAPI FXbool veccrosses(const FXVec4f& plane,const FXVec3f& a,const FXVec3f& b);

  /// Save to a stream
  friend FXAPI FXStream& operator<<(FXStream& store,const FXVec4f& v);

  /// Load from a stream
  friend FXAPI FXStream& operator>>(FXStream& store,FXVec4f& v);
  };


inline FXVec4f operator+(const FXVec4f& v){return v;}
inline FXVec4f operator-(const FXVec4f& v){return FXVec4f(-v.x,-v.y,-v.z,-v.w);}

inline FXVec4f operator+(const FXVec4f& a,const FXVec4f& b){return FXVec4f(a.x+b.x,a.y+b.y,a.z+b.z,a.w+b.w);}
inline FXVec4f operator-(const FXVec4f& a,const FXVec4f& b){return FXVec4f(a.x-b.x,a.y-b.y,a.z-b.z,a.w-b.w);}

inline FXVec4f operator*(const FXVec4f& a,FXfloat n){return FXVec4f(a.x*n,a.y*n,a.z*n,a.w*n);}
inline FXVec4f operator*(FXfloat n,const FXVec4f& a){return FXVec4f(n*a.x,n*a.y,n*a.z,n*a.w);}
inline FXVec4f operator/(const FXVec4f& a,FXfloat n){return FXVec4f(a.x/n,a.y/n,a.z/n,a.w/n);}
inline FXVec4f operator/(FXfloat n,const FXVec4f& a){return FXVec4f(n/a.x,n/a.y,n/a.z,n/a.w);}

inline FXfloat operator*(const FXVec4f& a,const FXVec4f& b){return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;}

inline int operator!(const FXVec4f& a){return a.x==0.0f && a.y==0.0f && a.z==0.0f && a.w==0.0f;}

inline int operator==(const FXVec4f& a,const FXVec4f& b){return a.x==b.x && a.y==b.y && a.z==b.z && a.w==b.w;}
inline int operator!=(const FXVec4f& a,const FXVec4f& b){return a.x!=b.x || a.y!=b.y || a.z!=b.z || a.w!=b.w;}

inline int operator==(const FXVec4f& a,FXfloat n){return a.x==n && a.y==n && a.z==n && a.w==n;}
inline int operator!=(const FXVec4f& a,FXfloat n){return a.x!=n || a.y!=n || a.z!=n || a.w!=n;}

inline int operator==(FXfloat n,const FXVec4f& a){return n==a.x && n==a.y && n==a.z && n==a.w;}
inline int operator!=(FXfloat n,const FXVec4f& a){return n!=a.x || n!=a.y || n!=a.z || n!=a.w;}

inline int operator<(const FXVec4f& a,const FXVec4f& b){return a.x<b.x && a.y<b.y && a.z<b.z && a.w<b.w;}
inline int operator<=(const FXVec4f& a,const FXVec4f& b){return a.x<=b.x && a.y<=b.y && a.z<=b.z && a.w<=b.w;}
inline int operator>(const FXVec4f& a,const FXVec4f& b){return a.x>b.x && a.y>b.y && a.z>b.z && a.w>b.w;}
inline int operator>=(const FXVec4f& a,const FXVec4f& b){return a.x>=b.x && a.y>=b.y && a.z>=b.z && a.w>=b.w;}

inline int operator<(const FXVec4f& a,FXfloat n){return a.x<n && a.y<n && a.z<n && a.w<n;}
inline int operator<=(const FXVec4f& a,FXfloat n){return a.x<=n && a.y<=n && a.z<=n && a.w<=n;}
inline int operator>(const FXVec4f& a,FXfloat n){return a.x>n && a.y>n && a.z>n && a.w>n;}
inline int operator>=(const FXVec4f& a,FXfloat n){return a.x>=n && a.y>=n && a.z>=n && a.w>=n;}

inline int operator<(FXfloat n,const FXVec4f& a){return n<a.x && n<a.y && n<a.z && n<a.w;}
inline int operator<=(FXfloat n,const FXVec4f& a){return n<=a.x && n<=a.y && n<=a.z && n<=a.w;}
inline int operator>(FXfloat n,const FXVec4f& a){return n>a.x && n>a.y && n>a.z && n>a.w;}
inline int operator>=(FXfloat n,const FXVec4f& a){return n>=a.x && n>=a.y && n>=a.z && n>=a.w;}

inline FXfloat veclen2(const FXVec4f& a){ return a.x*a.x+a.y*a.y+a.z*a.z+a.w*a.w; }
inline FXfloat veclen(const FXVec4f& a){ return sqrtf(len2(a)); }

extern FXAPI FXVec4f vecnormalize(const FXVec4f& a);

inline FXVec4f veclo(const FXVec4f& a,const FXVec4f& b){return FXVec4f(FXMIN(a.x,b.x),FXMIN(a.y,b.y),FXMIN(a.z,b.z),FXMIN(a.w,b.w));}
inline FXVec4f vechi(const FXVec4f& a,const FXVec4f& b){return FXVec4f(FXMAX(a.x,b.x),FXMAX(a.y,b.y),FXMAX(a.z,b.z),FXMAX(a.w,b.w));}

extern FXAPI FXVec4f vecplane(const FXVec4f& vec);
extern FXAPI FXVec4f vecplane(const FXVec3f& vec,FXfloat dist);
extern FXAPI FXVec4f vecplane(const FXVec3f& vec,const FXVec3f& p);
extern FXAPI FXVec4f vecplane(const FXVec3f& a,const FXVec3f& b,const FXVec3f& c);

extern FXAPI FXfloat vecdistance(const FXVec4f& plane,const FXVec3f& p);

extern FXAPI FXbool veccrosses(const FXVec4f& plane,const FXVec3f& a,const FXVec3f& b);

extern FXAPI FXStream& operator<<(FXStream& store,const FXVec4f& v);
extern FXAPI FXStream& operator>>(FXStream& store,FXVec4f& v);

}

#endif
