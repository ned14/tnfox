/********************************************************************************
*                                                                               *
*              D o u b l e - P r e c i s i o n  Q u a t e r n i o n             *
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
* $Id: FXQuatd.cpp,v 1.19.2.1 2005/06/22 06:15:59 fox Exp $                         *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXObject.h"
#include "FXVec2d.h"
#include "FXVec3d.h"
#include "FXVec4d.h"
#include "FXQuatd.h"
#include "FXMat3d.h"




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


// Construct quaternion from axes
FXQuatd::FXQuatd(const FXVec3d& ex,const FXVec3d& ey,const FXVec3d& ez){
  setAxes(ex,ey,ez);
  }


// Construct quaternion from 3x3 matrix
FXQuatd::FXQuatd(const FXMat3d& mat){
  setAxes(mat[0],mat[1],mat[2]);
  }


// Set axis and angle
void FXQuatd::setAxisAngle(const FXVec3d& axis,FXdouble phi){
  register FXdouble a=0.5*phi;
  register FXdouble s=sin(a)/veclen(axis);
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
  if(n>0.0){
    axis.x=x/n;
    axis.y=y/n;
    axis.z=z/n;
    phi=2.0*acos(w);
    }
  else{
    axis.x=1.0;
    axis.y=0.0;
    axis.z=0.0;
    phi=0.0;
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
  return vec*toMatrix(quat);
  }


// Construct quaternion from arc a->b on unit sphere.
FXQuatd arc(const FXVec3d& f,const FXVec3d& t){
  register FXdouble dot,div;
  dot=f.x*t.x+f.y*t.y+f.z*t.z;
  FXASSERT(-1.0<=dot && dot<=1.0);
  div=sqrt((dot+1.0)*2.0);
  FXASSERT(0.0<div);
  return FXQuatd((f.y*t.z-f.z*t.y)/div,(f.z*t.x-f.x*t.z)/div,(f.x*t.y-f.y*t.x)/div,div*0.5);
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


// Set quaternion from axes
void FXQuatd::setAxes(const FXVec3d& ex,const FXVec3d& ey,const FXVec3d& ez){
  register FXdouble trace=ex.x+ey.y+ez.z;
  register FXdouble scale;
  if(trace>0.0){
    scale=sqrt(1.0+trace);
    w=0.5*scale;
    scale=0.5/scale;
    x=(ey.z-ez.y)*scale;
    y=(ez.x-ex.z)*scale;
    z=(ex.y-ey.x)*scale;
    }
  else if(ex.x>ey.y && ex.x>ez.z){
    scale=2.0*sqrt(1.0+ex.x-ey.y-ez.z);
    x=0.25*scale;
    y=(ex.y+ey.x)/scale;
    z=(ex.z+ez.x)/scale;
    w=(ey.z-ez.y)/scale;
    }
  else if(ey.y>ez.z){
    scale=2.0*sqrt(1.0+ey.y-ex.x-ez.z);
    y=0.25*scale;
    x=(ex.y+ey.x)/scale;
    z=(ey.z+ez.y)/scale;
    w=(ez.x-ex.z)/scale;
    }
  else{
    scale=2.0*sqrt(1.0+ez.z-ex.x-ey.y);
    z=0.25*scale;
    x=(ex.z+ez.x)/scale;
    y=(ey.z+ez.y)/scale;
    w=(ex.y-ey.x)/scale;
    }
  }


// Get quaternion axes
void FXQuatd::getAxes(FXVec3d& ex,FXVec3d& ey,FXVec3d& ez) const {
  register FXdouble tx=2.0*x;
  register FXdouble ty=2.0*y;
  register FXdouble tz=2.0*z;
  register FXdouble twx=tx*w;
  register FXdouble twy=ty*w;
  register FXdouble twz=tz*w;
  register FXdouble txx=tx*x;
  register FXdouble txy=ty*x;
  register FXdouble txz=tz*x;
  register FXdouble tyy=ty*y;
  register FXdouble tyz=tz*y;
  register FXdouble tzz=tz*z;
  ex.x=1.0-tyy-tzz;
  ex.y=txy+twz;
  ex.z=txz-twy;
  ey.x=txy-twz;
  ey.y=1.0-txx-tzz;
  ey.z=tyz+twx;
  ez.x=txz+twy;
  ez.y=tyz-twx;
  ez.z=1.0-txx-tyy;
  }


// Obtain local x axis
FXVec3d FXQuatd::getXAxis() const {
  register FXdouble ty=2.0*y;
  register FXdouble tz=2.0*z;
  return FXVec3d(1.0-ty*y-tz*z,ty*x+tz*w,tz*x-ty*w);
  }


// Obtain local y axis
FXVec3d FXQuatd::getYAxis() const {
  register FXdouble tx=2.0*x;
  register FXdouble tz=2.0*z;
  return FXVec3d(tx*y-tz*w,1.0-tx*x-tz*z,tz*y+tx*w);
  }


// Obtain local z axis
FXVec3d FXQuatd::getZAxis() const {
  register FXdouble tx=2.0*x;
  register FXdouble ty=2.0*y;
  return FXVec3d(tx*z+ty*w,ty*z-tx*w,1.0-tx*x-ty*y);
  }


// Convert quaternion to 3x3 matrix
FXMat3d toMatrix(const FXQuatd& quat){
  return FXMat3d(quat);
  }


// Convert 3x3 matrix to quaternion
FXQuatd fromMatrix(const FXMat3d& mat){
  return FXQuatd(mat[0],mat[1],mat[2]);
  }


}

