/********************************************************************************
*                                                                               *
*       D o u b l e - P r e c i s i o n   2 - E l e m e n t   V e c t o r       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1994,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXVec2d.h,v 1.5 2004/06/09 06:05:23 fox Exp $                            *
********************************************************************************/
#ifndef FXVEC2D_H
#define FXVEC2D_H

#include "fxdefs.h"
#include <math.h>

namespace FX {


/// Double-precision 2-element vector
class FXAPI FXVec2d {
public:
  FXdouble x;
  FXdouble y;
public:

  /// Default constructor
  FXVec2d(){}

  /// Copy constructor
  FXVec2d(const FXVec2d& v){x=v.x;y=v.y;}

  // Initialize from array of floats
  FXVec2d(const FXdouble v[]){x=v[0];y=v[1];}

  /// Initialize with components
  FXVec2d(FXdouble xx,FXdouble yy){x=xx;y=yy;}

  /// Return a non-const reference to the ith element
  FXdouble& operator[](FXint i){return (&x)[i];}

  /// Return a const reference to the ith element
  const FXdouble& operator[](FXint i) const {return (&x)[i];}

  /// Assignment
  FXVec2d& operator=(const FXVec2d& v){x=v.x;y=v.y;return *this;}

  /// Assignment from array of floats
  FXVec2d& operator=(const FXdouble v[]){x=v[0];y=v[1];return *this;}

  /// Assigning operators
  FXVec2d& operator*=(FXdouble n){x*=n;y*=n;return *this;}
  FXVec2d& operator/=(FXdouble n){x/=n;y/=n;return *this;}
  FXVec2d& operator+=(const FXVec2d& v){x+=v.x;y+=v.y;return *this;}
  FXVec2d& operator-=(const FXVec2d& v){x-=v.x;y-=v.y;return *this;}

  /// Conversions
  operator FXdouble*(){return &x;}
  operator const FXdouble*() const {return &x;}

  /// Unary
  friend FXAPI FXVec2d operator+(const FXVec2d& v){return v;}
  friend FXAPI FXVec2d operator-(const FXVec2d& v){return FXVec2d(-v.x,-v.y);}

  /// Adding
  friend FXAPI FXVec2d operator+(const FXVec2d& a,const FXVec2d& b){return FXVec2d(a.x+b.x,a.y+b.y);}

  /// Subtracting
  friend FXAPI FXVec2d operator-(const FXVec2d& a,const FXVec2d& b){return FXVec2d(a.x-b.x,a.y-b.y);}

  /// Scaling
  friend FXAPI FXVec2d operator*(const FXVec2d& a,FXdouble n){return FXVec2d(a.x*n,a.y*n);}
  friend FXAPI FXVec2d operator*(FXdouble n,const FXVec2d& a){return FXVec2d(n*a.x,n*a.y);}
  friend FXAPI FXVec2d operator/(const FXVec2d& a,FXdouble n){return FXVec2d(a.x/n,a.y/n);}
  friend FXAPI FXVec2d operator/(FXdouble n,const FXVec2d& a){return FXVec2d(n/a.x,n/a.y);}

  /// Dot product
  friend FXAPI FXdouble operator*(const FXVec2d& a,const FXVec2d& b){return a.x*b.x+a.y*b.y;}

  /// Test if zero
  friend FXAPI int operator!(const FXVec2d& a){return a.x==0.0 && a.y==0.0;}

  /// Equality tests
  friend FXAPI int operator==(const FXVec2d& a,const FXVec2d& b){return a.x==b.x && a.y==b.y;}
  friend FXAPI int operator!=(const FXVec2d& a,const FXVec2d& b){return a.x!=b.x || a.y!=b.y;}

  friend FXAPI int operator==(const FXVec2d& a,FXdouble n){return a.x==n && a.y==n;}
  friend FXAPI int operator!=(const FXVec2d& a,FXdouble n){return a.x!=n || a.y!=n;}

  friend FXAPI int operator==(FXdouble n,const FXVec2d& a){return n==a.x && n==a.y;}
  friend FXAPI int operator!=(FXdouble n,const FXVec2d& a){return n!=a.x || n!=a.y;}

  /// Inequality tests
  friend FXAPI int operator<(const FXVec2d& a,const FXVec2d& b){return a.x<b.x && a.y<b.y;}
  friend FXAPI int operator<=(const FXVec2d& a,const FXVec2d& b){return a.x<=b.x && a.y<=b.y;}
  friend FXAPI int operator>(const FXVec2d& a,const FXVec2d& b){return a.x>b.x && a.y>b.y;}
  friend FXAPI int operator>=(const FXVec2d& a,const FXVec2d& b){return a.x>=b.x && a.y>=b.y;}

  friend FXAPI int operator<(const FXVec2d& a,FXdouble n){return a.x<n && a.y<n;}
  friend FXAPI int operator<=(const FXVec2d& a,FXdouble n){return a.x<=n && a.y<=n;}
  friend FXAPI int operator>(const FXVec2d& a,FXdouble n){return a.x>n && a.y>n;}
  friend FXAPI int operator>=(const FXVec2d& a,FXdouble n){return a.x>=n && a.y>=n;}

  friend FXAPI int operator<(FXdouble n,const FXVec2d& a){return n<a.x && n<a.y;}
  friend FXAPI int operator<=(FXdouble n,const FXVec2d& a){return n<=a.x && n<=a.y;}
  friend FXAPI int operator>(FXdouble n,const FXVec2d& a){return n>a.x && n>a.y;}
  friend FXAPI int operator>=(FXdouble n,const FXVec2d& a){return n>=a.x && n>=a.y;}

  /// Length and square of length
  friend FXAPI FXdouble len2(const FXVec2d& a){ return a.x*a.x+a.y*a.y; }
  friend FXAPI FXdouble len(const FXVec2d& a){ return sqrt(len2(a)); }

  /// Normalize vector
  friend FXAPI FXVec2d normalize(const FXVec2d& a);

  /// Lowest or highest components
  friend FXAPI FXVec2d lo(const FXVec2d& a,const FXVec2d& b){return FXVec2d(FXMIN(a.x,b.x),FXMIN(a.y,b.y));}
  friend FXAPI FXVec2d hi(const FXVec2d& a,const FXVec2d& b){return FXVec2d(FXMAX(a.x,b.x),FXMAX(a.y,b.y));}

  /// Save vector to a stream
  friend FXAPI FXStream& operator<<(FXStream& store,const FXVec2d& v);

  /// Load vector from a stream
  friend FXAPI FXStream& operator>>(FXStream& store,FXVec2d& v);
  };

}

#endif
