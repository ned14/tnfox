/********************************************************************************
*                                                                               *
*              D o u b l e - P r e c i s i o n  Q u a t e r n i o n             *
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
* $Id: FXQuatd.h,v 1.3 2004/02/27 18:30:06 fox Exp $                            *
********************************************************************************/
#ifndef FXQUATD_H
#define FXQUATD_H

#include "FXVec4d.h"

namespace FX {

class FXVec3d;

/// double-precision quaternion
class FXAPI FXQuatd : public FXVec4d {
public:

  /// Constructors
  FXQuatd(){}

  /// Copy constructor
  FXQuatd(const FXQuatd& q):FXVec4d(q){}

  /// Construct from components
  FXQuatd(FXdouble xx,FXdouble yy,FXdouble zz,FXdouble ww):FXVec4d(xx,yy,zz,ww){}

  // Construct from array of floats
  FXQuatd(const FXdouble v[]):FXVec4d(v){}

  /// Construct from axis and angle
  FXQuatd(const FXVec3d& axis,FXdouble phi=0.0);

  /// Construct from euler angles yaw (z), pitch (y), and roll (x)
  FXQuatd(FXdouble roll,FXdouble pitch,FXdouble yaw);

  /// Adjust quaternion length
  FXQuatd& adjust();

  /// Set quaternion from axis and angle
  void setAxisAngle(const FXVec3d& axis,FXdouble phi=0.0);

  /// Obtain axis and angle from quaternion
  void getAxisAngle(FXVec3d& axis,FXdouble& phi) const;

  /// Set quaternion from yaw (z), pitch (y), and roll (x)
  void setRollPitchYaw(FXdouble roll,FXdouble pitch,FXdouble yaw);

  /// Obtain yaw, pitch, and roll from quaternion
  void getRollPitchYaw(FXdouble& roll,FXdouble& pitch,FXdouble& yaw) const;

  /// Exponentiate quaternion
  friend FXAPI FXQuatd exp(const FXQuatd& q);

  /// Take logarithm of quaternion
  friend FXAPI FXQuatd log(const FXQuatd& q);

  /// Invert quaternion
  friend FXAPI FXQuatd invert(const FXQuatd& q);
  
  /// Invert unit quaternion
  friend FXAPI FXQuatd unitinvert(const FXQuatd& q);

  /// Conjugate quaternion
  friend FXAPI FXQuatd conj(const FXQuatd& q);

  /// Multiply quaternions
  friend FXAPI FXQuatd operator*(const FXQuatd& p,const FXQuatd& q);

  // Rotation of a vector by a quaternion
  friend FXAPI FXVec3d operator*(const FXQuatd& quat,const FXVec3d& vec);

  /// Construct quaternion from arc a->b on unit sphere
  friend FXAPI FXQuatd arc(const FXVec3d& a,const FXVec3d& b);

  /// Spherical lerp
  friend FXAPI FXQuatd lerp(const FXQuatd& u,const FXQuatd& v,FXdouble f);
  };



}

#endif
