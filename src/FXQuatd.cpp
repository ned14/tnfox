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
* $Id: FXQuatd.cpp,v 1.7 2004/02/27 18:30:06 fox Exp $                          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXStream.h"
#include "FXObject.h"
#include "FXVec2d.h"
#include "FXVec3d.h"
#include "FXVec4d.h"
#include "FXQuatd.h"




/*******************************************************************************/

namespace FX {

// Construct from angle and axis
FXQuatd::FXQuatd(const FXVec3d& axis,FXdouble phi){
  setAxisAngle(axis,phi);
  }


// Construct from roll, pitch, yaw
FXQuatd::FXQuatd(FXdouble roll,FXdouble pitch,FXdouble yaw){
  setRollPitchYaw(roll,pitch,yaw);
  }


// Set axis and angle
void FXQuatd::setAxisAngle(const FXVec3d& axis,FXdouble phi){
  register FXdouble a=0.5*phi;
  register FXdouble s=sin(a)/len(axis);
  x=axis.x*s;
  y=axis.y*s;
  z=axis.z*s;
  w=cos(a);
  }


// Obtain axis and angle
// Remeber that: q = sin(A/2)*(x*i+y*j+z*k)+cos(A/2)
// for unit quaternion |q| == 1
void FXQuatd::getAxisAngle(FXVec3d& axis,FXdouble& phi) const {
  register FXdouble n=sqrt(x*x+y*y+z*z);
  if(n>0.0f){
    axis.x=x/n;
    axis.y=y/n;
    axis.z=z/n;
    phi=2.0f*acos(w);
    }
  else{
    axis.x=1.0f;
    axis.y=0.0f;
    axis.z=0.0f;
    phi=0.0f;
    }
  }


// Set quaternion from roll (x), pitch(y) yaw (z)
void FXQuatd::setRollPitchYaw(FXdouble roll,FXdouble pitch,FXdouble yaw){
  register FXdouble sr,cr,sp,cp,sy,cy;
  register FXdouble rr=0.5*roll;
  register FXdouble pp=0.5*pitch;
  register FXdouble yy=0.5*yaw;
  sr=sin(rr); cr=cos(rr);
  sp=sin(pp); cp=cos(pp);
  sy=sin(yy); cy=cos(yy);
  x=sr*cp*cy-cr*sp*sy;
  y=cr*sp*cy+sr*cp*sy;
  z=cr*cp*sy-sr*sp*cy;
  w=cr*cp*cy+sr*sp*sy;
  }


// Obtain yaw, pitch, and roll
// Math is from "3D Game Engine Design" by David Eberly pp 19-20.
// However, instead of testing asin(Sy) against -PI/2 and PI/2, I
// test Sy against -1 and 1; this is numerically more stable, as
// asin doesn't like arguments outside [-1,1].
void FXQuatd::getRollPitchYaw(FXdouble& roll,FXdouble& pitch,FXdouble& yaw) const {
  register FXdouble s=2.0*(w*y-x*z);
  if(s<1.0){
    if(-1.0<s){
      roll=atan2(2.0*(y*z+w*x),1.0-2.0*(x*x+y*y));
      pitch=asin(s);
      yaw=atan2(2.0*(x*y+w*z),1.0-2.0*(y*y+z*z));
      }
    else{
      roll=-atan2(2.0*(x*y-w*z),1.0-2.0*(x*x+z*z));
      pitch=-1.57079632679489661923;
      yaw=0.0;
      }
    }
  else{
    roll=atan2(2.0*(x*y-w*z),1.0-2.0*(x*x+z*z));
    pitch=1.57079632679489661923;
    yaw=0.0;
    }
  }


FXQuatd& FXQuatd::adjust(){
  register FXdouble t=x*x+y*y+z*z+w*w;
  register FXdouble f;
  if(t>0.0){
    f=1.0/sqrt(t);
    x*=f;
    y*=f;
    z*=f;
    w*=f;
    }
  return *this;
  }


// Exponentiate unit quaternion
// Given q = theta*(x*i+y*j+z*k), where length of (x,y,z) is 1,
// then exp(q) = sin(theta)*(x*i+y*j+z*k)+cos(theta).
FXQuatd exp(const FXQuatd& q){
  register FXdouble theta=sqrt(q.x*q.x+q.y*q.y+q.z*q.z);
  register FXdouble scale;
  FXQuatd result(q.x,q.y,q.z,cos(theta));
  if(theta>0.000001){
    scale=sin(theta)/theta;
    result.x*=scale;
    result.y*=scale;
    result.z*=scale;
    }
  return result;
  }


// Take logarithm of unit quaternion
// Given q = sin(theta)*(x*i+y*j+z*k)+cos(theta), length of (x,y,z) is 1,
// then log(q) = theta*(x*i+y*j+z*k).
FXQuatd log(const FXQuatd& q){
  register FXdouble scale=sqrt(q.x*q.x+q.y*q.y+q.z*q.z);
  register FXdouble theta=atan2(scale,q.w);
  FXQuatd result(q.x,q.y,q.z,0.0);
  if(scale>0.0){
    scale=theta/scale;
    result.x*=scale;
    result.y*=scale;
    result.z*=scale;
    }
  return result;
  }


// Invert quaternion
FXQuatd invert(const FXQuatd& q){
  register FXdouble n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
  return FXQuatd(-q.x/n,-q.y/n,-q.z/n,q.w/n);
  }


// Invert unit quaternion
FXQuatd unitinvert(const FXQuatd& q){
  return FXQuatd(-q.x,-q.y,-q.z,q.w);
  }


// Conjugate quaternion
FXQuatd conj(const FXQuatd& q){
  return FXQuatd(-q.x,-q.y,-q.z,q.w);
  }


// Multiply quaternions
FXQuatd operator*(const FXQuatd& p,const FXQuatd& q){
  return FXQuatd(p.w*q.x+p.x*q.w+p.y*q.z-p.z*q.y,
                 p.w*q.y+p.y*q.w+p.z*q.x-p.x*q.z,
                 p.w*q.z+p.z*q.w+p.x*q.y-p.y*q.x,
                 p.w*q.w-p.x*q.x-p.y*q.y-p.z*q.z);
  }


// Rotation of a vector by a quaternion; this is defined as q.v.q*
// where q* is the conjugate of q.
FXVec3d operator*(const FXQuatd& quat,const FXVec3d& vec){
  register FXdouble tx=2.0*quat.x;
  register FXdouble ty=2.0*quat.y;
  register FXdouble tz=2.0*quat.z;
  register FXdouble twx=tx*quat.w;
  register FXdouble twy=ty*quat.w;
  register FXdouble twz=tz*quat.w;
  register FXdouble txx=tx*quat.x;
  register FXdouble txy=ty*quat.x;
  register FXdouble txz=tz*quat.x;
  register FXdouble tyy=ty*quat.y;
  register FXdouble tyz=tz*quat.y;
  register FXdouble tzz=tz*quat.z;

  return FXVec3d(vec.x*(1.0-tyy-tzz)+vec.y*(txy-twz)+vec.z*(txz-twy),
                 vec.x*(txy+twz)+vec.y*(1.0-txx-tzz)+vec.z*(tyz-twx),
                 vec.x*(txz-twy)+vec.y*(tyz+twx)+vec.z*(1.0-txx-tyy));
  }


// Construct quaternion from arc a->b on unit sphere
FXQuatd arc(const FXVec3d& f,const FXVec3d& t){
  return FXQuatd(f.y*t.z-f.z*t.y, f.z*t.x-f.x*t.z, f.x*t.y-f.y*t.x, f.x*t.x+f.y*t.y+f.z*t.z);
  }


// Spherical lerp
FXQuatd lerp(const FXQuatd& u,const FXQuatd& v,FXdouble f){
  register FXdouble alpha,beta,theta,sin_t,cos_t;
  register FXint flip=0;
  cos_t = u.x*v.x+u.y*v.y+u.z*v.z+u.w*v.w;
  if(cos_t<0.0){ cos_t = -cos_t; flip=1; }
  if((1.0-cos_t)<0.000001){
    beta = 1.0-f;
    alpha = f;
    }
  else{
    theta = acos(cos_t);
    sin_t = sin(theta);
    beta = sin(theta-f*theta)/sin_t;
    alpha = sin(f*theta)/sin_t;
    }
  if(flip) alpha = -alpha;
  return FXQuatd(beta*u.x+alpha*v.x, beta*u.y+alpha*v.y, beta*u.z+alpha*v.z, beta*u.w+alpha*v.w);
  }

}

