/********************************************************************************
*                                                                               *
*            D o u b l e - P r e c i s i o n   3 x 3   M a t r i x              *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXMat3d.h,v 1.5 2005/01/16 16:06:06 fox Exp $                            *
********************************************************************************/
#ifndef FXMAT3D_H
#define FXMAT3D_H

#include "FXVec2d.h"
#include "FXVec3d.h"

namespace FX {


class FXQuatd;


/// Double-precision 3x3 matrix
class FXAPI FXMat3d {
protected:
  FXVec3d m[3];
public:

  /// Default constructor
  FXMat3d(){}

  /// Copy constructor
  FXMat3d(const FXMat3d& other);

  /// Construct from scalar number
  FXMat3d(FXdouble w);

  /// Construct from components
  FXMat3d(FXdouble a00,FXdouble a01,FXdouble a02,
          FXdouble a10,FXdouble a11,FXdouble a12,
          FXdouble a20,FXdouble a21,FXdouble a22);

  /// Construct matrix from three vectors
  FXMat3d(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c);

  /// Construct rotation matrix from quaternion
  FXMat3d(const FXQuatd& quat);

  /// Assignment operators
  FXMat3d& operator=(const FXMat3d& other);
  FXMat3d& operator=(FXdouble w);
  FXMat3d& operator+=(const FXMat3d& w);
  FXMat3d& operator-=(const FXMat3d& w);
  FXMat3d& operator*=(FXdouble w);
  FXMat3d& operator*=(const FXMat3d& w);
  FXMat3d& operator/=(FXdouble w);

  /// Indexing
  FXVec3d& operator[](FXint i){return m[i];}
  const FXVec3d& operator[](FXint i) const {return m[i];}

  /// Conversion
  operator FXdouble*(){return m[0];}
  operator const FXdouble*() const {return m[0];}

  /// Other operators
  friend FXAPI FXMat3d operator+(const FXMat3d& a,const FXMat3d& b);
  friend FXAPI FXMat3d operator-(const FXMat3d& a,const FXMat3d& b);
  friend FXAPI FXMat3d operator-(const FXMat3d& a);
  friend FXAPI FXMat3d operator*(const FXMat3d& a,const FXMat3d& b);
  friend FXAPI FXMat3d operator*(FXdouble x,const FXMat3d& a);
  friend FXAPI FXMat3d operator*(const FXMat3d& a,FXdouble x);
  friend FXAPI FXMat3d operator/(const FXMat3d& a,FXdouble x);
  friend FXAPI FXMat3d operator/(FXdouble x,const FXMat3d& a);

  /// Multiply matrix and vector
  friend FXAPI FXVec3d operator*(const FXVec3d& v,const FXMat3d& m);
  friend FXAPI FXVec3d operator*(const FXMat3d& a,const FXVec3d& v);

  /// Mutiply matrix and vector, for non-projective matrix
  friend FXAPI FXVec2d operator*(const FXVec2d& v,const FXMat3d& m);
  friend FXAPI FXVec2d operator*(const FXMat3d& a,const FXVec2d& v);

  /// Set identity matrix
  FXMat3d& eye();

  /// Multiply by rotation of phi
  FXMat3d& rot(FXdouble c,FXdouble s);
  FXMat3d& rot(FXdouble phi);

  /// Multiply by translation
  FXMat3d& trans(FXdouble tx,FXdouble ty);

  /// Multiply by scaling
  FXMat3d& scale(FXdouble sx,FXdouble sy);
  FXMat3d& scale(FXdouble s);

  /// Determinant
  friend FXAPI FXdouble det(const FXMat3d& m);

  /// Transpose
  friend FXAPI FXMat3d transpose(const FXMat3d& m);

  /// Invert
  friend FXAPI FXMat3d invert(const FXMat3d& m);

  /// Save to a stream
  friend FXAPI FXStream& operator<<(FXStream& store,const FXMat3d& m);

  /// Load from a stream
  friend FXAPI FXStream& operator>>(FXStream& store,FXMat3d& m);
  };

}

#endif
