/********************************************************************************
*                                                                               *
*       D o u b l e - P r e c i s i o n   3 - E l e m e n t   V e c t o r       *
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
* $Id: FXVec3d.h,v 1.8.2.1 2006/03/21 07:08:29 fox Exp $                            *
********************************************************************************/
#ifndef FXVEC3D_H
#define FXVEC3D_H

#include "fxdefs.h"
#include "FXVec2d.h"
#include <math.h>

namespace FX {


/// Double-precision 3-element vector
class FXAPI FXVec3d {
public:
  FXdouble x;
  FXdouble y;
  FXdouble z;
public:

  /// Default constructor
  FXVec3d(){}

  /// Copy constructor
  FXVec3d(const FXVec3d& v){x=v.x;y=v.y;z=v.z;}

  /// Initialize from array of doubles
  FXVec3d(const FXdouble v[]){x=v[0];y=v[1];z=v[2];}

  /// Initialize with components
  FXVec3d(FXdouble xx,FXdouble yy,FXdouble zz=1.0){x=xx;y=yy;z=zz;}

  /// Initialize with color
  FXVec3d(FXColor color);

  /// Return a non-const reference to the ith element
  FXdouble& operator[](FXint i){return (&x)[i];}

  /// Return a const reference to the ith element
  const FXdouble& operator[](FXint i) const {return (&x)[i];}

  /// Assign color
  FXVec3d& operator=(FXColor color);

  /// Assignment
  FXVec3d& operator=(const FXVec3d& v){x=v.x;y=v.y;z=v.z;return *this;}

  /// Assignment from array of doubles
  FXVec3d& operator=(const FXdouble v[]){x=v[0];y=v[1];z=v[2];return *this;}

  /// Assigning operators
  FXVec3d& operator*=(FXdouble n){x*=n;y*=n;z*=n;return *this;}
  FXVec3d& operator/=(FXdouble n){x/=n;y/=n;z/=n;return *this;}
  FXVec3d& operator+=(const FXVec3d& v){x+=v.x;y+=v.y;z+=v.z;return *this;}
  FXVec3d& operator-=(const FXVec3d& v){x-=v.x;y-=v.y;z-=v.z;return *this;}

  /// Conversions
  operator FXdouble*(){return &x;}
  operator const FXdouble*() const {return &x;}
  operator FXVec2d&(){return *reinterpret_cast<FXVec2d*>(this);}
  operator const FXVec2d&() const {return *reinterpret_cast<const FXVec2d*>(this);}

  /// Convert to color
  operator FXColor() const;

  /// Unary
  friend FXAPI FXVec3d operator+(const FXVec3d& v);
  friend FXAPI FXVec3d operator-(const FXVec3d& v);

  /// Adding
  friend FXAPI FXVec3d operator+(const FXVec3d& a,const FXVec3d& b);

  /// Subtracting
  friend FXAPI FXVec3d operator-(const FXVec3d& a,const FXVec3d& b);

  /// Scaling
  friend FXAPI FXVec3d operator*(const FXVec3d& a,FXdouble n);
  friend FXAPI FXVec3d operator*(FXdouble n,const FXVec3d& a);
  friend FXAPI FXVec3d operator/(const FXVec3d& a,FXdouble n);
  friend FXAPI FXVec3d operator/(FXdouble n,const FXVec3d& a);

  /// Dot product
  friend FXAPI FXdouble operator*(const FXVec3d& a,const FXVec3d& b);

  /// Cross product
  friend FXAPI FXVec3d operator^(const FXVec3d& a,const FXVec3d& b);

  /// Test if zero
  friend FXAPI int operator!(const FXVec3d& a);

  /// Equality tests
  friend FXAPI int operator==(const FXVec3d& a,const FXVec3d& b);
  friend FXAPI int operator!=(const FXVec3d& a,const FXVec3d& b);

  friend FXAPI int operator==(const FXVec3d& a,FXdouble n);
  friend FXAPI int operator!=(const FXVec3d& a,FXdouble n);

  friend FXAPI int operator==(FXdouble n,const FXVec3d& a);
  friend FXAPI int operator!=(FXdouble n,const FXVec3d& a);

  /// Inequality tests
  friend FXAPI int operator<(const FXVec3d& a,const FXVec3d& b);
  friend FXAPI int operator<=(const FXVec3d& a,const FXVec3d& b);
  friend FXAPI int operator>(const FXVec3d& a,const FXVec3d& b);
  friend FXAPI int operator>=(const FXVec3d& a,const FXVec3d& b);

  friend FXAPI int operator<(const FXVec3d& a,FXdouble n);
  friend FXAPI int operator<=(const FXVec3d& a,FXdouble n);
  friend FXAPI int operator>(const FXVec3d& a,FXdouble n);
  friend FXAPI int operator>=(const FXVec3d& a,FXdouble n);

  friend FXAPI int operator<(FXdouble n,const FXVec3d& a);
  friend FXAPI int operator<=(FXdouble n,const FXVec3d& a);
  friend FXAPI int operator>(FXdouble n,const FXVec3d& a);
  friend FXAPI int operator>=(FXdouble n,const FXVec3d& a);

  /// Length and square of length
  friend FXAPI FXdouble veclen2(const FXVec3d& a);
  friend FXAPI FXdouble veclen(const FXVec3d& a);

  /// Normalize vector
  friend FXAPI FXVec3d vecnormalize(const FXVec3d& a);

  /// Lowest or highest components
  friend FXAPI FXVec3d veclo(const FXVec3d& a,const FXVec3d& b);
  friend FXAPI FXVec3d vechi(const FXVec3d& a,const FXVec3d& b);

  /// Compute normal from three points a,b,c
  friend FXAPI FXVec3d vecnormal(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c);

  /// Compute approximate normal from four points a,b,c,d
  friend FXAPI FXVec3d vecnormal(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c,const FXVec3d& d);

  /// Save vector to a stream
  friend FXAPI FXStream& operator<<(FXStream& store,const FXVec3d& v);

  /// Load vector from a stream
  friend FXAPI FXStream& operator>>(FXStream& store,FXVec3d& v);
  };


inline FXVec3d operator+(const FXVec3d& v){return v;}
inline FXVec3d operator-(const FXVec3d& v){return FXVec3d(-v.x,-v.y,-v.z);}

inline FXVec3d operator+(const FXVec3d& a,const FXVec3d& b){return FXVec3d(a.x+b.x,a.y+b.y,a.z+b.z);}
inline FXVec3d operator-(const FXVec3d& a,const FXVec3d& b){return FXVec3d(a.x-b.x,a.y-b.y,a.z-b.z);}

inline FXVec3d operator*(const FXVec3d& a,FXdouble n){return FXVec3d(a.x*n,a.y*n,a.z*n);}
inline FXVec3d operator*(FXdouble n,const FXVec3d& a){return FXVec3d(n*a.x,n*a.y,n*a.z);}
inline FXVec3d operator/(const FXVec3d& a,FXdouble n){return FXVec3d(a.x/n,a.y/n,a.z/n);}
inline FXVec3d operator/(FXdouble n,const FXVec3d& a){return FXVec3d(n/a.x,n/a.y,n/a.z);}

inline FXdouble operator*(const FXVec3d& a,const FXVec3d& b){return a.x*b.x+a.y*b.y+a.z*b.z;}

inline FXVec3d operator^(const FXVec3d& a,const FXVec3d& b){return FXVec3d(a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x);}

inline int operator!(const FXVec3d& a){return a.x==0.0 && a.y==0.0 && a.z==0.0;}

inline int operator==(const FXVec3d& a,const FXVec3d& b){return a.x==b.x && a.y==b.y && a.z==b.z;}
inline int operator!=(const FXVec3d& a,const FXVec3d& b){return a.x!=b.x || a.y!=b.y || a.z!=b.z;}

inline int operator==(const FXVec3d& a,FXdouble n){return a.x==n && a.y==n && a.z==n;}
inline int operator!=(const FXVec3d& a,FXdouble n){return a.x!=n || a.y!=n || a.z!=n;}

inline int operator==(FXdouble n,const FXVec3d& a){return n==a.x && n==a.y && n==a.z;}
inline int operator!=(FXdouble n,const FXVec3d& a){return n!=a.x || n!=a.y || n!=a.z;}

inline int operator<(const FXVec3d& a,const FXVec3d& b){return a.x<b.x && a.y<b.y && a.z<b.z;}
inline int operator<=(const FXVec3d& a,const FXVec3d& b){return a.x<=b.x && a.y<=b.y && a.z<=b.z;}
inline int operator>(const FXVec3d& a,const FXVec3d& b){return a.x>b.x && a.y>b.y && a.z>b.z;}
inline int operator>=(const FXVec3d& a,const FXVec3d& b){return a.x>=b.x && a.y>=b.y && a.z>=b.z;}

inline int operator<(const FXVec3d& a,FXdouble n){return a.x<n && a.y<n && a.z<n;}
inline int operator<=(const FXVec3d& a,FXdouble n){return a.x<=n && a.y<=n && a.z<=n;}
inline int operator>(const FXVec3d& a,FXdouble n){return a.x>n && a.y>n && a.z>n;}
inline int operator>=(const FXVec3d& a,FXdouble n){return a.x>=n && a.y>=n && a.z>=n;}

inline int operator<(FXdouble n,const FXVec3d& a){return n<a.x && n<a.y && n<a.z;}
inline int operator<=(FXdouble n,const FXVec3d& a){return n<=a.x && n<=a.y && n<=a.z;}
inline int operator>(FXdouble n,const FXVec3d& a){return n>a.x && n>a.y && n>a.z;}
inline int operator>=(FXdouble n,const FXVec3d& a){return n>=a.x && n>=a.y && n>=a.z;}

inline FXdouble veclen2(const FXVec3d& a){ return a.x*a.x+a.y*a.y+a.z*a.z; }
inline FXdouble veclen(const FXVec3d& a){ return sqrt(veclen2(a)); }

extern FXAPI FXVec3d vecnormalize(const FXVec3d& a);

inline FXVec3d veclo(const FXVec3d& a,const FXVec3d& b){return FXVec3d(FXMIN(a.x,b.x),FXMIN(a.y,b.y),FXMIN(a.z,b.z));}
inline FXVec3d vechi(const FXVec3d& a,const FXVec3d& b){return FXVec3d(FXMAX(a.x,b.x),FXMAX(a.y,b.y),FXMAX(a.z,b.z));}

extern FXAPI FXVec3d vecnormal(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c);
extern FXAPI FXVec3d vecnormal(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c,const FXVec3d& d);

extern FXAPI FXStream& operator<<(FXStream& store,const FXVec3d& v);
extern FXAPI FXStream& operator>>(FXStream& store,FXVec3d& v);

}

#endif
