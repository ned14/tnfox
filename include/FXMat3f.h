/********************************************************************************
*                                                                               *
*            S i n g l e - P r e c i s i o n   3 x 3   M a t r i x              *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXMat3f.h,v 1.5 2004/02/08 17:17:33 fox Exp $                            *
********************************************************************************/
#ifndef FXMAT3F_H
#define FXMAT3F_H

#include "FXVec2f.h"
#include "FXVec3f.h"

namespace FX {


/// Single-precision 3x3 matrix
class FXAPI FXMat3f {
protected:
  FXVec3f m[3];
public:
  /// Constructors
  FXMat3f(){}
  FXMat3f(FXfloat w);
  FXMat3f(FXfloat a00,FXfloat a01,FXfloat a02,
          FXfloat a10,FXfloat a11,FXfloat a12,
          FXfloat a20,FXfloat a21,FXfloat a22);
  FXMat3f(const FXVec3f& a,const FXVec3f& b,const FXVec3f& c);
  FXMat3f(const FXMat3f& other);

  /// Assignment operators
  FXMat3f& operator=(const FXMat3f& other);
  FXMat3f& operator=(FXfloat w);
  FXMat3f& operator+=(const FXMat3f& w);
  FXMat3f& operator-=(const FXMat3f& w);
  FXMat3f& operator*=(FXfloat w);
  FXMat3f& operator*=(const FXMat3f& w);
  FXMat3f& operator/=(FXfloat w);

  /// Indexing
  FXVec3f& operator[](FXint i){return m[i];}
  const FXVec3f& operator[](FXint i) const {return m[i];}

  /// Conversion
  operator FXfloat*(){return m[0];}
  operator const FXfloat*() const {return m[0];}

  /// Other operators
  friend FXAPI FXMat3f operator+(const FXMat3f& a,const FXMat3f& b);
  friend FXAPI FXMat3f operator-(const FXMat3f& a,const FXMat3f& b);
  friend FXAPI FXMat3f operator-(const FXMat3f& a);
  friend FXAPI FXMat3f operator*(const FXMat3f& a,const FXMat3f& b);
  friend FXAPI FXMat3f operator*(FXfloat x,const FXMat3f& a);
  friend FXAPI FXMat3f operator*(const FXMat3f& a,FXfloat x);
  friend FXAPI FXMat3f operator/(const FXMat3f& a,FXfloat x);
  friend FXAPI FXMat3f operator/(FXfloat x,const FXMat3f& a);

  /// Multiply matrix and vector
  friend FXAPI FXVec3f operator*(const FXVec3f& v,const FXMat3f& m);
  friend FXAPI FXVec3f operator*(const FXMat3f& a,const FXVec3f& v);

  /// Mutiply matrix and vector, for non-projective matrix
  friend FXAPI FXVec2f operator*(const FXVec2f& v,const FXMat3f& m);
  friend FXAPI FXVec2f operator*(const FXMat3f& a,const FXVec2f& v);

  /// Set identity matrix
  FXMat3f& eye();

  /// Multiply by rotation of phi
  FXMat3f& rot(FXfloat c,FXfloat s);
  FXMat3f& rot(FXfloat phi);

  /// Multiply by translation
  FXMat3f& trans(FXfloat tx,FXfloat ty);

  /// Multiply by scaling
  FXMat3f& scale(FXfloat sx,FXfloat sy);
  FXMat3f& scale(FXfloat s);

  /// Determinant
  friend FXAPI FXfloat det(const FXMat3f& m);

  /// Transpose
  friend FXAPI FXMat3f transpose(const FXMat3f& m);

  /// Invert
  friend FXAPI FXMat3f invert(const FXMat3f& m);

  /// Save to a stream
  friend FXAPI FXStream& operator<<(FXStream& store,const FXMat3f& m);

  /// Load from a stream
  friend FXAPI FXStream& operator>>(FXStream& store,FXMat3f& m);
  };

}

#endif
