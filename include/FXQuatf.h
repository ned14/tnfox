/********************************************************************************
*                                                                               *
*              S i n g l e - P r e c i s i o n  Q u a t e r n i o n             *
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
* $Id: FXQuatf.h,v 1.6 2004/02/27 18:30:06 fox Exp $                            *
********************************************************************************/
#ifndef FXQUATF_H
#define FXQUATF_H

#include "FXVec4f.h"

namespace FX {

class FXVec3f;

/// Single-precision quaternion
class FXAPI FXQuatf : public FXVec4f {
public:

  /// Constructors
  FXQuatf(){}

  /// Copy constructor
  FXQuatf(const FXQuatf& q):FXVec4f(q){}

  /// Construct from components
  FXQuatf(FXfloat xx,FXfloat yy,FXfloat zz,FXfloat ww):FXVec4f(xx,yy,zz,ww){}

  // Construct from array of floats
  FXQuatf(const FXfloat v[]):FXVec4f(v){}

  /// Construct from axis and angle
  FXQuatf(const FXVec3f& axis,FXfloat phi=0.0f);

  /// Construct from euler angles yaw (z), pitch (y), and roll (x)
  FXQuatf(FXfloat roll,FXfloat pitch,FXfloat yaw);

  /// Adjust quaternion length
  FXQuatf& adjust();

  /// Set quaternion from axis and angle
  void setAxisAngle(const FXVec3f& axis,FXfloat phi=0.0f);

  /// Obtain axis and angle from quaternion
  void getAxisAngle(FXVec3f& axis,FXfloat& phi) const;

  /// Set quaternion from yaw (z), pitch (y), and roll (x)
  void setRollPitchYaw(FXfloat roll,FXfloat pitch,FXfloat yaw);

  /// Obtain yaw, pitch, and roll from quaternion
  void getRollPitchYaw(FXfloat& roll,FXfloat& pitch,FXfloat& yaw) const;

  /// Exponentiate quaternion
  friend FXAPI FXQuatf exp(const FXQuatf& q);

  /// Take logarithm of quaternion
  friend FXAPI FXQuatf log(const FXQuatf& q);

  /// Invert quaternion
  friend FXAPI FXQuatf invert(const FXQuatf& q);
  
  /// Invert unit quaternion
  friend FXAPI FXQuatf unitinvert(const FXQuatf& q);

  /// Conjugate quaternion
  friend FXAPI FXQuatf conj(const FXQuatf& q);

  /// Multiply quaternions
  friend FXAPI FXQuatf operator*(const FXQuatf& p,const FXQuatf& q);

  // Rotation of a vector by a quaternion
  friend FXAPI FXVec3f operator*(const FXQuatf& quat,const FXVec3f& vec);

  /// Construct quaternion from arc a->b on unit sphere
  friend FXAPI FXQuatf arc(const FXVec3f& a,const FXVec3f& b);

  /// Spherical lerp
  friend FXAPI FXQuatf lerp(const FXQuatf& u,const FXQuatf& v,FXfloat f);
  };



}

#endif
