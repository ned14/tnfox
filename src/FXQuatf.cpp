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
* $Id: FXQuatf.cpp,v 1.11 2004/02/27 23:30:45 fox Exp $                         *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXStream.h"
#include "FXObject.h"
#include "FXVec2f.h"
#include "FXVec3f.h"
#include "FXVec4f.h"
#include "FXQuatf.h"




/*******************************************************************************/

namespace FX {

// Construct from angle and axis
FXQuatf::FXQuatf(const FXVec3f& axis,FXfloat phi){
  setAxisAngle(axis,phi);
  }


// Construct from roll, pitch, yaw
FXQuatf::FXQuatf(FXfloat roll,FXfloat pitch,FXfloat yaw){
  setRollPitchYaw(roll,pitch,yaw);
  }


// Set axis and angle
void FXQuatf::setAxisAngle(const FXVec3f& axis,FXfloat phi){
  register FXfloat a=0.5f*phi;
  register FXfloat s=sinf(a)/len(axis);
  x=axis.x*s;
  y=axis.y*s;
  z=axis.z*s;
  w=cosf(a);
  }


// Obtain axis and angle
// Remeber that: q = sin(A/2)*(x*i+y*j+z*k)+cos(A/2)
// for unit quaternion |q| == 1
void FXQuatf::getAxisAngle(FXVec3f& axis,FXfloat& phi) const {
  register FXfloat n=sqrtf(x*x+y*y+z*z);
  if(n>0.0f){
    axis.x=x/n;
    axis.y=y/n;
    axis.z=z/n;
    phi=2.0f*acosf(w);
    }
  else{
    axis.x=1.0f;
    axis.y=0.0f;
    axis.z=0.0f;
    phi=0.0f;
    }
  }


// Set quaternion from roll (x), pitch(y) yaw (z)
void FXQuatf::setRollPitchYaw(FXfloat roll,FXfloat pitch,FXfloat yaw){
  register FXfloat sr,cr,sp,cp,sy,cy;
  register FXfloat rr=0.5f*roll;
  register FXfloat pp=0.5f*pitch;
  register FXfloat yy=0.5f*yaw;
  sr=sinf(rr); cr=cosf(rr);
  sp=sinf(pp); cp=cosf(pp);
  sy=sinf(yy); cy=cosf(yy);
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
void FXQuatf::getRollPitchYaw(FXfloat& roll,FXfloat& pitch,FXfloat& yaw) const {
  register FXfloat s=2.0f*(w*y-x*z);
  if(s<1.0f){
    if(-1.0f<s){
      roll=atan2f(2.0f*(y*z+w*x),1.0f-2.0f*(x*x+y*y));
      pitch=asinf(s);
      yaw=atan2f(2.0f*(x*y+w*z),1.0f-2.0f*(y*y+z*z));
      }
    else{
      roll=-atan2f(2.0f*(x*y-w*z),1.0f-2.0f*(x*x+z*z));
      pitch=-1.57079632679489661923f;
      yaw=0.0f;
      }
    }
  else{
    roll=atan2f(2.0f*(x*y-w*z),1.0f-2.0f*(x*x+z*z));
    pitch=1.57079632679489661923f;
    yaw=0.0f;
    }
  }


FXQuatf& FXQuatf::adjust(){
  register FXfloat t=x*x+y*y+z*z+w*w;
  register FXfloat f;
  if(t>0.0f){
    f=1.0f/sqrtf(t);
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
FXQuatf exp(const FXQuatf& q){
  register FXfloat theta=sqrtf(q.x*q.x+q.y*q.y+q.z*q.z);
  register FXfloat scale;
  FXQuatf result(q.x,q.y,q.z,cosf(theta));
  if(theta>0.000001f){
    scale=sinf(theta)/theta;
    result.x*=scale;
    result.y*=scale;
    result.z*=scale;
    }
  return result;
  }


// Take logarithm of unit quaternion
// Given q = sin(theta)*(x*i+y*j+z*k)+cos(theta), length of (x,y,z) is 1,
// then log(q) = theta*(x*i+y*j+z*k).
FXQuatf log(const FXQuatf& q){
  register FXfloat scale=sqrtf(q.x*q.x+q.y*q.y+q.z*q.z);
  register FXfloat theta=atan2f(scale,q.w);
  FXQuatf result(q.x,q.y,q.z,0.0f);
  if(scale>0.0f){
    scale=theta/scale;
    result.x*=scale;
    result.y*=scale;
    result.z*=scale;
    }
  return result;
  }


// Invert quaternion
FXQuatf invert(const FXQuatf& q){
  register FXfloat n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
  return FXQuatf(-q.x/n,-q.y/n,-q.z/n,q.w/n);
  }


// Invert unit quaternion
FXQuatf unitinvert(const FXQuatf& q){
  return FXQuatf(-q.x,-q.y,-q.z,q.w);
  }


// Conjugate quaternion
FXQuatf conj(const FXQuatf& q){
  return FXQuatf(-q.x,-q.y,-q.z,q.w);
  }


// Multiply quaternions
FXQuatf operator*(const FXQuatf& p,const FXQuatf& q){
  return FXQuatf(p.w*q.x+p.x*q.w+p.y*q.z-p.z*q.y,
                 p.w*q.y+p.y*q.w+p.z*q.x-p.x*q.z,
                 p.w*q.z+p.z*q.w+p.x*q.y-p.y*q.x,
                 p.w*q.w-p.x*q.x-p.y*q.y-p.z*q.z);
  }



// Rotation of a vector by a quaternion; this is defined as q.v.q*
// where q* is the conjugate of q.
FXVec3f operator*(const FXQuatf& quat,const FXVec3f& vec){
  register FXfloat tx=2.0f*quat.x;
  register FXfloat ty=2.0f*quat.y;
  register FXfloat tz=2.0f*quat.z;
  register FXfloat twx=tx*quat.w;
  register FXfloat twy=ty*quat.w;
  register FXfloat twz=tz*quat.w;
  register FXfloat txx=tx*quat.x;
  register FXfloat txy=ty*quat.x;
  register FXfloat txz=tz*quat.x;
  register FXfloat tyy=ty*quat.y;
  register FXfloat tyz=tz*quat.y;
  register FXfloat tzz=tz*quat.z;

  return FXVec3f(vec.x*(1.0f-tyy-tzz)+vec.y*(txy-twz)+vec.z*(txz-twy),
                 vec.x*(txy+twz)+vec.y*(1.0f-txx-tzz)+vec.z*(tyz-twx),
                 vec.x*(txz-twy)+vec.y*(tyz+twx)+vec.z*(1.0f-txx-tyy));
  }


// Construct quaternion from arc a->b on unit sphere
FXQuatf arc(const FXVec3f& f,const FXVec3f& t){
  return FXQuatf(f.y*t.z-f.z*t.y, f.z*t.x-f.x*t.z, f.x*t.y-f.y*t.x, f.x*t.x+f.y*t.y+f.z*t.z);
  }


// Spherical lerp
FXQuatf lerp(const FXQuatf& u,const FXQuatf& v,FXfloat f){
  register FXfloat alpha,beta,theta,sin_t,cos_t;
  register FXint flip=0;
  cos_t = u.x*v.x+u.y*v.y+u.z*v.z+u.w*v.w;
  if(cos_t<0.0f){ cos_t = -cos_t; flip=1; }
  if((1.0f-cos_t)<0.000001f){
    beta = 1.0f-f;
    alpha = f;
    }
  else{
    theta = acosf(cos_t);
    sin_t = sinf(theta);
    beta = sinf(theta-f*theta)/sin_t;
    alpha = sinf(f*theta)/sin_t;
    }
  if(flip) alpha = -alpha;
  return FXQuatf(beta*u.x+alpha*v.x, beta*u.y+alpha*v.y, beta*u.z+alpha*v.z, beta*u.w+alpha*v.w);
  }

}

