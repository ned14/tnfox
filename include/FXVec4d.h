/********************************************************************************
*                                                                               *
*       D o u b l e - P r e c i s i o n   4 - E l e m e n t   V e c t o r       *
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
* $Id: FXVec4d.h,v 1.12 2005/01/16 16:06:06 fox Exp $                           *
********************************************************************************/
#ifndef FXVEC4D_H
#define FXVEC4D_H

#include "FXVec3d.h"
#include <math.h>

namespace FX {


/// Double-precision 4-element vector
class FXAPI FXVec4d {
public:
  FXdouble x;
  FXdouble y;
  FXdouble z;
  FXdouble w;
public:

  /// Default constructor
  FXVec4d(){}

  /// Copy constructor
  FXVec4d(const FXVec4d& v){x=v.x;y=v.y;z=v.z;w=v.w;}

  /// Construct with 3-vector and optional scalar
  FXVec4d(const FXVec3d& v,FXdouble ww=1.0){x=v.x;y=v.y;z=v.z;w=ww;}

  /// Initialize from array of doubles
  FXVec4d(const FXdouble v[]){x=v[0];y=v[1];z=v[2];w=v[3];}

  /// Initialize with components
  FXVec4d(FXdouble xx,FXdouble yy,FXdouble zz,FXdouble ww=1.0){x=xx;y=yy;z=zz;w=ww;}

  /// Initialize with color
  FXVec4d(FXColor color);

  /// Return a non-const reference to the ith element
  FXdouble& operator[](FXint i){return (&x)[i];}

  /// Return a const reference to the ith element
  const FXdouble& operator[](FXint i) const {return (&x)[i];}

  /// Assign color
  FXVec4d& operator=(FXColor color);

  /// Assignment
  FXVec4d& operator=(const FXVec3d& v){x=v.x;y=v.y;z=v.z;w=1.0;return *this;}
  FXVec4d& operator=(const FXVec4d& v){x=v.x;y=v.y;z=v.z;w=v.w;return *this;}

  /// Assignment from array of doubles
  FXVec4d& operator=(const FXdouble v[]){x=v[0];y=v[1];z=v[2];w=v[3];return *this;}

  /// Assigning operators
  FXVec4d& operator*=(FXdouble n){x*=n;y*=n;z*=n;w*=n;return *this;}
  FXVec4d& operator/=(FXdouble n){x/=n;y/=n;z/=n;w/=n;return *this;}
  FXVec4d& operator+=(const FXVec4d& v){x+=v.x;y+=v.y;z+=v.z;w+=v.w;return *this;}
  FXVec4d& operator-=(const FXVec4d& v){x-=v.x;y-=v.y;z-=v.z;w-=v.w;return *this;}

  /// Conversion
  operator FXdouble*(){return &x;}
  operator const FXdouble*() const {return &x;}
  operator FXVec3d&(){return *reinterpret_cast<FXVec3d*>(this);}
  operator const FXVec3d&() const {return *reinterpret_cast<const FXVec3d*>(this);}

  /// Convert to color
  operator FXColor() const;

  /// Unary
  friend FXAPI FXVec4d operator+(const FXVec4d& v){return v;}
  friend FXAPI FXVec4d operator-(const FXVec4d& v){return FXVec4d(-v.x,-v.y,-v.z,-v.w);}

  /// Adding
  friend FXAPI FXVec4d operator+(const FXVec4d& a,const FXVec4d& b){return FXVec4d(a.x+b.x,a.y+b.y,a.z+b.z,a.w+b.w);}

  /// Subtracting
  friend FXAPI FXVec4d operator-(const FXVec4d& a,const FXVec4d& b){return FXVec4d(a.x-b.x,a.y-b.y,a.z-b.z,a.w-b.w);}

  /// Scaling
  friend FXAPI FXVec4d operator*(const FXVec4d& a,FXdouble n){return FXVec4d(a.x*n,a.y*n,a.z*n,a.w*n);}
  friend FXAPI FXVec4d operator*(FXdouble n,const FXVec4d& a){return FXVec4d(n*a.x,n*a.y,n*a.z,n*a.w);}
  friend FXAPI FXVec4d operator/(const FXVec4d& a,FXdouble n){return FXVec4d(a.x/n,a.y/n,a.z/n,a.w/n);}
  friend FXAPI FXVec4d operator/(FXdouble n,const FXVec4d& a){return FXVec4d(n/a.x,n/a.y,n/a.z,n/a.w);}

  /// Dot product
  friend FXAPI FXdouble operator*(const FXVec4d& a,const FXVec4d& b){return a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;}

  /// Test if zero
  friend FXAPI int operator!(const FXVec4d& a){return a.x==0.0 && a.y==0.0 && a.z==0.0 && a.w==0.0;}

  /// Equality tests
  friend FXAPI int operator==(const FXVec4d& a,const FXVec4d& b){return a.x==b.x && a.y==b.y && a.z==b.z && a.w==b.w;}
  friend FXAPI int operator!=(const FXVec4d& a,const FXVec4d& b){return a.x!=b.x || a.y!=b.y || a.z!=b.z || a.w!=b.w;}

  friend FXAPI int operator==(const FXVec4d& a,FXdouble n){return a.x==n && a.y==n && a.z==n && a.w==n;}
  friend FXAPI int operator!=(const FXVec4d& a,FXdouble n){return a.x!=n || a.y!=n || a.z!=n || a.w!=n;}

  friend FXAPI int operator==(FXdouble n,const FXVec4d& a){return n==a.x && n==a.y && n==a.z && n==a.w;}
  friend FXAPI int operator!=(FXdouble n,const FXVec4d& a){return n!=a.x || n!=a.y || n!=a.z || n!=a.w;}

  /// Inequality tests
  friend FXAPI int operator<(const FXVec4d& a,const FXVec4d& b){return a.x<b.x && a.y<b.y && a.z<b.z && a.w<b.w;}
  friend FXAPI int operator<=(const FXVec4d& a,const FXVec4d& b){return a.x<=b.x && a.y<=b.y && a.z<=b.z && a.w<=b.w;}
  friend FXAPI int operator>(const FXVec4d& a,const FXVec4d& b){return a.x>b.x && a.y>b.y && a.z>b.z && a.w>b.w;}
  friend FXAPI int operator>=(const FXVec4d& a,const FXVec4d& b){return a.x>=b.x && a.y>=b.y && a.z>=b.z && a.w>=b.w;}

  friend FXAPI int operator<(const FXVec4d& a,FXdouble n){return a.x<n && a.y<n && a.z<n && a.w<n;}
  friend FXAPI int operator<=(const FXVec4d& a,FXdouble n){return a.x<=n && a.y<=n && a.z<=n && a.w<=n;}
  friend FXAPI int operator>(const FXVec4d& a,FXdouble n){return a.x>n && a.y>n && a.z>n && a.w>n;}
  friend FXAPI int operator>=(const FXVec4d& a,FXdouble n){return a.x>=n && a.y>=n && a.z>=n && a.w>=n;}

  friend FXAPI int operator<(FXdouble n,const FXVec4d& a){return n<a.x && n<a.y && n<a.z && n<a.w;}
  friend FXAPI int operator<=(FXdouble n,const FXVec4d& a){return n<=a.x && n<=a.y && n<=a.z && n<=a.w;}
  friend FXAPI int operator>(FXdouble n,const FXVec4d& a){return n>a.x && n>a.y && n>a.z && n>a.w;}
  friend FXAPI int operator>=(FXdouble n,const FXVec4d& a){return n>=a.x && n>=a.y && n>=a.z && n>=a.w;}

  /// Length and square of length
  friend FXAPI FXdouble len2(const FXVec4d& a){ return a.x*a.x+a.y*a.y+a.z*a.z+a.w*a.w; }
  friend FXAPI FXdouble len(const FXVec4d& a){ return sqrt(len2(a)); }

  /// Normalize vector
  friend FXAPI FXVec4d normalize(const FXVec4d& a);

  /// Lowest or highest components
  friend FXAPI FXVec4d lo(const FXVec4d& a,const FXVec4d& b){return FXVec4d(FXMIN(a.x,b.x),FXMIN(a.y,b.y),FXMIN(a.z,b.z),FXMIN(a.w,b.w));}
  friend FXAPI FXVec4d hi(const FXVec4d& a,const FXVec4d& b){return FXVec4d(FXMAX(a.x,b.x),FXMAX(a.y,b.y),FXMAX(a.z,b.z),FXMAX(a.w,b.w));}

  /// Compute normalized plane equation ax+by+cz+d=0
  friend FXAPI FXVec4d plane(const FXVec4d& vec);
  friend FXAPI FXVec4d plane(const FXVec3d& vec,FXdouble dist);
  friend FXAPI FXVec4d plane(const FXVec3d& vec,const FXVec3d& p);
  friend FXAPI FXVec4d plane(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c);

  /// Signed distance normalized plane and point
  friend FXAPI FXdouble distance(const FXVec4d& plane,const FXVec3d& p);

  /// Return true if edge a-b crosses plane
  friend FXAPI FXbool crosses(const FXVec4d& plane,const FXVec3d& a,const FXVec3d& b);

  /// Save to a stream
  friend FXAPI FXStream& operator<<(FXStream& store,const FXVec4d& v);

  /// Load from a stream
  friend FXAPI FXStream& operator>>(FXStream& store,FXVec4d& v);
  };

}

#endif
