/********************************************************************************
*                                                                               *
*            S i n g l e - P r e c i s i o n   4 x 4   M a t r i x              *
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
* $Id: FXMat4f.cpp,v 1.10 2004/10/25 04:20:26 fox Exp $                          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXObject.h"
#include "FXVec2f.h"
#include "FXVec3f.h"
#include "FXVec4f.h"
#include "FXQuatf.h"
#include "FXMat3f.h"
#include "FXMat4f.h"


/*
  Notes:
  - Transformations pre-multiply.
  - Goal is same effect as OpenGL.
*/


#define DET2(a00,a01, \
             a10,a11) ((a00)*(a11)-(a10)*(a01))

#define DET3(a00,a01,a02, \
             a10,a11,a12, \
             a20,a21,a22) ((a00)*DET2(a11,a12,a21,a22) - \
                           (a10)*DET2(a01,a02,a21,a22) + \
                           (a20)*DET2(a01,a02,a11,a12))

#define DET4(a00,a01,a02,a03, \
             a10,a11,a12,a13, \
             a20,a21,a22,a23, \
             a30,a31,a32,a33) ((a00)*DET3(a11,a12,a13,a21,a22,a23,a31,a32,a33) - \
                               (a10)*DET3(a01,a02,a03,a21,a22,a23,a31,a32,a33) + \
                               (a20)*DET3(a01,a02,a03,a11,a12,a13,a31,a32,a33) - \
                               (a30)*DET3(a01,a02,a03,a11,a12,a13,a21,a22,a23))



/*******************************************************************************/

namespace FX {

// Build matrix from constant
FXMat4f::FXMat4f(FXfloat w){
  m[0][0]=w; m[0][1]=w; m[0][2]=w; m[0][3]=w;
  m[1][0]=w; m[1][1]=w; m[1][2]=w; m[1][3]=w;
  m[2][0]=w; m[2][1]=w; m[2][2]=w; m[2][3]=w;
  m[3][0]=w; m[3][1]=w; m[3][2]=w; m[3][3]=w;
  }


// Build matrix from scalars
FXMat4f::FXMat4f(FXfloat a00,FXfloat a01,FXfloat a02,FXfloat a03,
                 FXfloat a10,FXfloat a11,FXfloat a12,FXfloat a13,
                 FXfloat a20,FXfloat a21,FXfloat a22,FXfloat a23,
                 FXfloat a30,FXfloat a31,FXfloat a32,FXfloat a33){
  m[0][0]=a00; m[0][1]=a01; m[0][2]=a02; m[0][3]=a03;
  m[1][0]=a10; m[1][1]=a11; m[1][2]=a12; m[1][3]=a13;
  m[2][0]=a20; m[2][1]=a21; m[2][2]=a22; m[2][3]=a23;
  m[3][0]=a30; m[3][1]=a31; m[3][2]=a32; m[3][3]=a33;
  }


// Build matrix from four vectors
FXMat4f::FXMat4f(const FXVec4f& a,const FXVec4f& b,const FXVec4f& c,const FXVec4f& d){
  m[0][0]=a[0]; m[0][1]=a[1]; m[0][2]=a[2]; m[0][3]=a[3];
  m[1][0]=b[0]; m[1][1]=b[1]; m[1][2]=b[2]; m[1][3]=b[3];
  m[2][0]=c[0]; m[2][1]=c[1]; m[2][2]=c[2]; m[2][3]=c[3];
  m[3][0]=d[0]; m[3][1]=d[1]; m[3][2]=d[2]; m[3][3]=d[3];
  }


// Copy constructor
FXMat4f::FXMat4f(const FXMat4f& other){
  m[0]=other[0];
  m[1]=other[1];
  m[2]=other[2];
  m[3]=other[3];
  }


// Assignment operator
FXMat4f& FXMat4f::operator=(const FXMat4f& other){
  m[0]=other[0];
  m[1]=other[1];
  m[2]=other[2];
  m[3]=other[3];
  return *this;
  }


// Set matrix to constant
FXMat4f& FXMat4f::operator=(FXfloat w){
  m[0][0]=w; m[0][1]=w; m[0][2]=w; m[0][3]=w;
  m[1][0]=w; m[1][1]=w; m[1][2]=w; m[1][3]=w;
  m[2][0]=w; m[2][1]=w; m[2][2]=w; m[2][3]=w;
  m[3][0]=w; m[3][1]=w; m[3][2]=w; m[3][3]=w;
  return *this;
  }


// Add matrices
FXMat4f& FXMat4f::operator+=(const FXMat4f& w){
  m[0][0]+=w[0][0]; m[0][1]+=w[0][1]; m[0][2]+=w[0][2]; m[0][3]+=w[0][3];
  m[1][0]+=w[1][0]; m[1][1]+=w[1][1]; m[1][2]+=w[1][2]; m[1][3]+=w[1][3];
  m[2][0]+=w[2][0]; m[2][1]+=w[2][1]; m[2][2]+=w[2][2]; m[2][3]+=w[2][3];
  m[3][0]+=w[3][0]; m[3][1]+=w[3][1]; m[3][2]+=w[3][2]; m[3][3]+=w[3][3];
  return *this;
  }


// Substract matrices
FXMat4f& FXMat4f::operator-=(const FXMat4f& w){
  m[0][0]-=w[0][0]; m[0][1]-=w[0][1]; m[0][2]-=w[0][2]; m[0][3]-=w[0][3];
  m[1][0]-=w[1][0]; m[1][1]-=w[1][1]; m[1][2]-=w[1][2]; m[1][3]-=w[1][3];
  m[2][0]-=w[2][0]; m[2][1]-=w[2][1]; m[2][2]-=w[2][2]; m[2][3]-=w[2][3];
  m[3][0]-=w[3][0]; m[3][1]-=w[3][1]; m[3][2]-=w[3][2]; m[3][3]-=w[3][3];
  return *this;
  }


// Multiply matrix by scalar
FXMat4f& FXMat4f::operator*=(FXfloat w){
  m[0][0]*=w; m[0][1]*=w; m[0][2]*=w; m[0][3]*=w;
  m[1][0]*=w; m[1][1]*=w; m[1][2]*=w; m[2][3]*=w;
  m[2][0]*=w; m[2][1]*=w; m[2][2]*=w; m[3][3]*=w;
  m[3][0]*=w; m[3][1]*=w; m[3][2]*=w; m[3][3]*=w;
  return *this;
  }


// Multiply matrix by matrix
FXMat4f& FXMat4f::operator*=(const FXMat4f& w){
  register FXfloat x,y,z,h;
  x=m[0][0]; y=m[0][1]; z=m[0][2]; h=m[0][3];
  m[0][0]=x*w[0][0]+y*w[1][0]+z*w[2][0]+h*w[3][0];
  m[0][1]=x*w[0][1]+y*w[1][1]+z*w[2][1]+h*w[3][1];
  m[0][2]=x*w[0][2]+y*w[1][2]+z*w[2][2]+h*w[3][2];
  m[0][3]=x*w[0][3]+y*w[1][3]+z*w[2][3]+h*w[3][3];
  x=m[1][0]; y=m[1][1]; z=m[1][2]; h=m[1][3];
  m[1][0]=x*w[0][0]+y*w[1][0]+z*w[2][0]+h*w[3][0];
  m[1][1]=x*w[0][1]+y*w[1][1]+z*w[2][1]+h*w[3][1];
  m[1][2]=x*w[0][2]+y*w[1][2]+z*w[2][2]+h*w[3][2];
  m[1][3]=x*w[0][3]+y*w[1][3]+z*w[2][3]+h*w[3][3];
  x=m[2][0]; y=m[2][1]; z=m[2][2]; h=m[2][3];
  m[2][0]=x*w[0][0]+y*w[1][0]+z*w[2][0]+h*w[3][0];
  m[2][1]=x*w[0][1]+y*w[1][1]+z*w[2][1]+h*w[3][1];
  m[2][2]=x*w[0][2]+y*w[1][2]+z*w[2][2]+h*w[3][2];
  m[2][3]=x*w[0][3]+y*w[1][3]+z*w[2][3]+h*w[3][3];
  x=m[3][0]; y=m[3][1]; z=m[3][2]; h=m[3][3];
  m[3][0]=x*w[0][0]+y*w[1][0]+z*w[2][0]+h*w[3][0];
  m[3][1]=x*w[0][1]+y*w[1][1]+z*w[2][1]+h*w[3][1];
  m[3][2]=x*w[0][2]+y*w[1][2]+z*w[2][2]+h*w[3][2];
  m[3][3]=x*w[0][3]+y*w[1][3]+z*w[2][3]+h*w[3][3];
  return *this;
  }


// Divide matric by scalar
FXMat4f& FXMat4f::operator/=(FXfloat w){
  m[0][0]/=w; m[0][1]/=w; m[0][2]/=w; m[0][3]/=w;
  m[1][0]/=w; m[1][1]/=w; m[1][2]/=w; m[1][3]/=w;
  m[2][0]/=w; m[2][1]/=w; m[2][2]/=w; m[2][3]/=w;
  m[3][0]/=w; m[3][1]/=w; m[3][2]/=w; m[3][3]/=w;
  return *this;
  }


// Add matrices
FXMat4f operator+(const FXMat4f& a,const FXMat4f& b){
  return FXMat4f(a[0][0]+b[0][0],a[0][1]+b[0][1],a[0][2]+b[0][2],a[0][3]+b[0][3],
                 a[1][0]+b[1][0],a[1][1]+b[1][1],a[1][2]+b[1][2],a[1][3]+b[1][3],
                 a[2][0]+b[2][0],a[2][1]+b[2][1],a[2][2]+b[2][2],a[2][3]+b[2][3],
                 a[3][0]+b[3][0],a[3][1]+b[3][1],a[3][2]+b[3][2],a[3][3]+b[3][3]);
  }


// Substract matrices
FXMat4f operator-(const FXMat4f& a,const FXMat4f& b){
  return FXMat4f(a.m[0][0]-b.m[0][0],a.m[0][1]-b.m[0][1],a.m[0][2]-b.m[0][2],a.m[0][3]-b.m[0][3],
                 a.m[1][0]-b.m[1][0],a.m[1][1]-b.m[1][1],a.m[1][2]-b.m[1][2],a.m[1][3]-b.m[1][3],
                 a.m[2][0]-b.m[2][0],a.m[2][1]-b.m[2][1],a.m[2][2]-b.m[2][2],a.m[2][3]-b.m[2][3],
                 a.m[3][0]-b.m[3][0],a.m[3][1]-b.m[3][1],a.m[3][2]-b.m[3][2],a.m[3][3]-b.m[3][3]);
  }


// Negate matrix
FXMat4f operator-(const FXMat4f& a){
  return FXMat4f(-a[0][0],-a[0][1],-a[0][2],-a[0][3],
                 -a[1][0],-a[1][1],-a[1][2],-a[1][3],
                 -a[2][0],-a[2][1],-a[2][2],-a[2][3],
                 -a[3][0],-a[3][1],-a[3][2],-a[3][3]);
  }



// Composite matrices
FXMat4f operator*(const FXMat4f& a,const FXMat4f& b){
  FXMat4f r;
  register FXfloat x,y,z,h;
  x=a[0][0]; y=a[0][1]; z=a[0][2]; h=a[0][3];
  r[0][0]=x*b[0][0]+y*b[1][0]+z*b[2][0]+h*b[3][0];
  r[0][1]=x*b[0][1]+y*b[1][1]+z*b[2][1]+h*b[3][1];
  r[0][2]=x*b[0][2]+y*b[1][2]+z*b[2][2]+h*b[3][2];
  r[0][3]=x*b[0][3]+y*b[1][3]+z*b[2][3]+h*b[3][3];
  x=a[1][0]; y=a[1][1]; z=a[1][2]; h=a[1][3];
  r[1][0]=x*b[0][0]+y*b[1][0]+z*b[2][0]+h*b[3][0];
  r[1][1]=x*b[0][1]+y*b[1][1]+z*b[2][1]+h*b[3][1];
  r[1][2]=x*b[0][2]+y*b[1][2]+z*b[2][2]+h*b[3][2];
  r[1][3]=x*b[0][3]+y*b[1][3]+z*b[2][3]+h*b[3][3];
  x=a[2][0]; y=a[2][1]; z=a[2][2]; h=a[2][3];
  r[2][0]=x*b[0][0]+y*b[1][0]+z*b[2][0]+h*b[3][0];
  r[2][1]=x*b[0][1]+y*b[1][1]+z*b[2][1]+h*b[3][1];
  r[2][2]=x*b[0][2]+y*b[1][2]+z*b[2][2]+h*b[3][2];
  r[2][3]=x*b[0][3]+y*b[1][3]+z*b[2][3]+h*b[3][3];
  x=a[3][0]; y=a[3][1]; z=a[3][2]; h=a[3][3];
  r[3][0]=x*b[0][0]+y*b[1][0]+z*b[2][0]+h*b[3][0];
  r[3][1]=x*b[0][1]+y*b[1][1]+z*b[2][1]+h*b[3][1];
  r[3][2]=x*b[0][2]+y*b[1][2]+z*b[2][2]+h*b[3][2];
  r[3][3]=x*b[0][3]+y*b[1][3]+z*b[2][3]+h*b[3][3];
  return r;
  }


// Multiply scalar by matrix
FXMat4f operator*(FXfloat x,const FXMat4f& a){
  return FXMat4f(x*a[0][0],x*a[0][1],x*a[0][2],a[0][3],
                 x*a[1][0],x*a[1][1],x*a[1][2],a[1][3],
                 x*a[2][0],x*a[2][1],x*a[2][2],a[2][3],
                 x*a[3][0],x*a[3][1],x*a[3][2],a[3][3]);
  }


// Multiply matrix by scalar
FXMat4f operator*(const FXMat4f& a,FXfloat x){
  return FXMat4f(a[0][0]*x,a[0][1]*x,a[0][2]*x,a[0][3],
                 a[1][0]*x,a[1][1]*x,a[1][2]*x,a[1][3],
                 a[2][0]*x,a[2][1]*x,a[2][2]*x,a[2][3],
                 a[3][0]*x,a[3][1]*x,a[3][2]*x,a[3][3]);
  }


// Divide scalar by matrix
FXMat4f operator/(FXfloat x,const FXMat4f& a){
  return FXMat4f(x/a[0][0],x/a[0][1],x/a[0][2],a[0][3],
                 x/a[1][0],x/a[1][1],x/a[1][2],a[1][3],
                 x/a[2][0],x/a[2][1],x/a[2][2],a[2][3],
                 x/a[3][0],x/a[3][1],x/a[3][2],a[3][3]);
  }


// Divide matrix by scalar
FXMat4f operator/(const FXMat4f& a,FXfloat x){
  return FXMat4f(a[0][0]/x,a[0][1]/x,a[0][2]/x,a[0][3],
                 a[1][0]/x,a[1][1]/x,a[1][2]/x,a[1][3],
                 a[2][0]/x,a[2][1]/x,a[2][2]/x,a[2][3],
                 a[3][0]/x,a[3][1]/x,a[3][2]/x,a[3][3]);
  }


// Vector times matrix
FXVec4f operator*(const FXVec4f& v,const FXMat4f& m){
  register FXfloat x=v.x,y=v.y,z=v.z,w=v.w;
  return FXVec4f(x*m[0][0]+y*m[1][0]+z*m[2][0]+w*m[3][0],
                 x*m[0][1]+y*m[1][1]+z*m[2][1]+w*m[3][1],
                 x*m[0][2]+y*m[1][2]+z*m[2][2]+w*m[3][2],
                 x*m[0][3]+y*m[1][3]+z*m[2][3]+w*m[3][3]);
  }


// Matrix times vector
FXVec4f operator*(const FXMat4f& m,const FXVec4f& v){
  register FXfloat x=v.x,y=v.y,z=v.z,w=v.w;
  return FXVec4f(x*m[0][0]+y*m[0][1]+z*m[0][2]+w*m[0][3],
                 x*m[1][0]+y*m[1][1]+z*m[1][2]+w*m[1][3],
                 x*m[2][0]+y*m[2][1]+z*m[2][2]+w*m[2][3],
                 x*m[3][0]+y*m[3][1]+z*m[3][2]+w*m[3][3]);
  }


// Vector times matrix
FXVec3f operator*(const FXVec3f& v,const FXMat4f& m){
  register FXfloat x=v.x,y=v.y,z=v.z;
  FXASSERT(m[0][3]==0.0f && m[1][3]==0.0f && m[2][3]==0.0f && m[3][3]==1.0f);
  return FXVec3f(x*m[0][0]+y*m[1][0]+z*m[2][0]+m[3][0],
                 x*m[0][1]+y*m[1][1]+z*m[2][1]+m[3][1],
                 x*m[0][2]+y*m[1][2]+z*m[2][2]+m[3][2]);
  }


// Matrix times vector
FXVec3f operator*(const FXMat4f& m,const FXVec3f& v){
  register FXfloat x=v.x,y=v.y,z=v.z;
  FXASSERT(m[0][3]==0.0f && m[1][3]==0.0f && m[2][3]==0.0f && m[3][3]==1.0f);
  return FXVec3f(x*m[0][0]+y*m[0][1]+z*m[0][2]+m[0][3],
                 x*m[1][0]+y*m[1][1]+z*m[1][2]+m[1][3],
                 x*m[2][0]+y*m[2][1]+z*m[2][2]+m[2][3]);
  }


// Make unit matrix
FXMat4f& FXMat4f::eye(){
  m[0][0]=1.0f; m[0][1]=0.0f; m[0][2]=0.0f; m[0][3]=0.0f;
  m[1][0]=0.0f; m[1][1]=1.0f; m[1][2]=0.0f; m[1][3]=0.0f;
  m[2][0]=0.0f; m[2][1]=0.0f; m[2][2]=1.0f; m[2][3]=0.0f;
  m[3][0]=0.0f; m[3][1]=0.0f; m[3][2]=0.0f; m[3][3]=1.0f;
  return *this;
  }


// Orthographic projection
FXMat4f& FXMat4f::ortho(FXfloat left,FXfloat right,FXfloat bottom,FXfloat top,FXfloat hither,FXfloat yon){
  register FXfloat x,y,z,tx,ty,tz,rl,tb,yh,r0,r1,r2,r3;
  rl=right-left;
  tb=top-bottom;
  yh=yon-hither;
  FXASSERT(rl && tb && yh);         // Throw exception in future
  x= 2.0f/rl;
  y= 2.0f/tb;
  z=-2.0f/yh;
  tx=-(right+left)/rl;
  ty=-(top+bottom)/tb;
  tz=-(yon+hither)/yh;
  r0=m[0][0];
  r1=m[1][0];
  r2=m[2][0];
  r3=m[3][0];
  m[0][0]=x*r0;
  m[1][0]=y*r1;
  m[2][0]=z*r2;
  m[3][0]=tx*r0+ty*r1+tz*r2+r3;
  r0=m[0][1];
  r1=m[1][1];
  r2=m[2][1];
  r3=m[3][1];
  m[0][1]=x*r0;
  m[1][1]=y*r1;
  m[2][1]=z*r2;
  m[3][1]=tx*r0+ty*r1+tz*r2+r3;
  r0=m[0][2];
  r1=m[1][2];
  r2=m[2][2];
  r3=m[3][2];
  m[0][2]=x*r0;
  m[1][2]=y*r1;
  m[2][2]=z*r2;
  m[3][2]=tx*r0+ty*r1+tz*r2+r3;
  r0=m[0][3];
  r1=m[1][3];
  r2=m[2][3];
  r3=m[3][3];
  m[0][3]=x*r0;
  m[1][3]=y*r1;
  m[2][3]=z*r2;
  m[3][3]=tx*r0+ty*r1+tz*r2+r3;
  return *this;
  }


// Perspective projection
FXMat4f& FXMat4f::frustum(FXfloat left,FXfloat right,FXfloat bottom,FXfloat top,FXfloat hither,FXfloat yon){
  register FXfloat x,y,a,b,c,d,rl,tb,yh,r0,r1,r2,r3;
  FXASSERT(0.0f<hither && hither<yon);  // Throw exception in future
  rl=right-left;
  tb=top-bottom;
  yh=yon-hither;
  FXASSERT(rl && tb);                   // Throw exception in future
  x= 2.0f*hither/rl;
  y= 2.0f*hither/tb;
  a= (right+left)/rl;
  b= (top+bottom)/tb;
  c=-(yon+hither)/yh;
  d=-(2.0f*yon*hither)/yh;
  r0=m[0][0];
  r1=m[1][0];
  r2=m[2][0];
  r3=m[3][0];
  m[0][0]=x*r0;
  m[1][0]=y*r1;
  m[2][0]=a*r0+b*r1+c*r2-r3;
  m[3][0]=d*r2;
  r0=m[0][1];
  r1=m[1][1];
  r2=m[2][1];
  r3=m[3][1];
  m[0][1]=x*r0;
  m[1][1]=y*r1;
  m[2][1]=a*r0+b*r1+c*r2-r3;
  m[3][1]=d*r2;
  r0=m[0][2];
  r1=m[1][2];
  r2=m[2][2];
  r3=m[3][2];
  m[0][2]=x*r0;
  m[1][2]=y*r1;
  m[2][2]=a*r0+b*r1+c*r2-r3;
  m[3][2]=d*r2;
  r0=m[0][3];
  r1=m[1][3];
  r2=m[2][3];
  r3=m[3][3];
  m[0][3]=x*r0;
  m[1][3]=y*r1;
  m[2][3]=a*r0+b*r1+c*r2-r3;
  m[3][3]=d*r2;
  return *this;
  }


// Make left hand matrix
FXMat4f& FXMat4f::left(){
  m[2][0]= -m[2][0];
  m[2][1]= -m[2][1];
  m[2][2]= -m[2][2];
  m[2][3]= -m[2][3];
  return *this;
  }


// Rotate using quaternion
FXMat4f& FXMat4f::rot(const FXQuatf& q){
  register FXfloat x,y,z;

  // Get rotation matrix
  FXMat3f r=toMatrix(q);

  // Pre-multiply
  x=m[0][0]; y=m[1][0]; z=m[2][0];
  m[0][0]=x*r[0][0]+y*r[0][1]+z*r[0][2];
  m[1][0]=x*r[1][0]+y*r[1][1]+z*r[1][2];
  m[2][0]=x*r[2][0]+y*r[2][1]+z*r[2][2];
  x=m[0][1]; y=m[1][1]; z=m[2][1];
  m[0][1]=x*r[0][0]+y*r[0][1]+z*r[0][2];
  m[1][1]=x*r[1][0]+y*r[1][1]+z*r[1][2];
  m[2][1]=x*r[2][0]+y*r[2][1]+z*r[2][2];
  x=m[0][2]; y=m[1][2]; z=m[2][2];
  m[0][2]=x*r[0][0]+y*r[0][1]+z*r[0][2];
  m[1][2]=x*r[1][0]+y*r[1][1]+z*r[1][2];
  m[2][2]=x*r[2][0]+y*r[2][1]+z*r[2][2];
  x=m[0][3]; y=m[1][3]; z=m[2][3];
  m[0][3]=x*r[0][0]+y*r[0][1]+z*r[0][2];
  m[1][3]=x*r[1][0]+y*r[1][1]+z*r[1][2];
  m[2][3]=x*r[2][0]+y*r[2][1]+z*r[2][2];
  return *this;
  }


// Rotate by angle (cos,sin) about arbitrary vector
FXMat4f& FXMat4f::rot(const FXVec3f& v,FXfloat c,FXfloat s){
  register FXfloat xx,yy,zz,xy,yz,zx,xs,ys,zs,t;
  register FXfloat r00,r01,r02,r10,r11,r12,r20,r21,r22;
  register FXfloat x=v.x;
  register FXfloat y=v.y;
  register FXfloat z=v.z;
  register FXfloat mag=x*x+y*y+z*z;
  FXASSERT(-1.00001f<c && c<1.00001f && -1.00001f<s && s<1.00001f);
  if(mag<=1.0E-30f) return *this;         // Rotation about 0-length axis
  mag=sqrtf(mag);
  x/=mag;
  y/=mag;
  z/=mag;
  xx=x*x;
  yy=y*y;
  zz=z*z;
  xy=x*y;
  yz=y*z;
  zx=z*x;
  xs=x*s;
  ys=y*s;
  zs=z*s;
  t=1.0f-c;
  r00=t*xx+c;  r10=t*xy-zs; r20=t*zx+ys;
  r01=t*xy+zs; r11=t*yy+c;  r21=t*yz-xs;
  r02=t*zx-ys; r12=t*yz+xs; r22=t*zz+c;
  x=m[0][0];
  y=m[1][0];
  z=m[2][0];
  m[0][0]=x*r00+y*r01+z*r02;
  m[1][0]=x*r10+y*r11+z*r12;
  m[2][0]=x*r20+y*r21+z*r22;
  x=m[0][1];
  y=m[1][1];
  z=m[2][1];
  m[0][1]=x*r00+y*r01+z*r02;
  m[1][1]=x*r10+y*r11+z*r12;
  m[2][1]=x*r20+y*r21+z*r22;
  x=m[0][2];
  y=m[1][2];
  z=m[2][2];
  m[0][2]=x*r00+y*r01+z*r02;
  m[1][2]=x*r10+y*r11+z*r12;
  m[2][2]=x*r20+y*r21+z*r22;
  x=m[0][3];
  y=m[1][3];
  z=m[2][3];
  m[0][3]=x*r00+y*r01+z*r02;
  m[1][3]=x*r10+y*r11+z*r12;
  m[2][3]=x*r20+y*r21+z*r22;
  return *this;
  }


// Rotate by angle (in radians) about arbitrary vector
FXMat4f& FXMat4f::rot(const FXVec3f& v,FXfloat phi){
  return rot(v,cosf(phi),sinf(phi));
  }


// Rotate about x-axis
FXMat4f& FXMat4f::xrot(FXfloat c,FXfloat s){
  register FXfloat u,v;
  FXASSERT(-1.00001f<c && c<1.00001f && -1.00001f<s && s<1.00001f);
  u=m[1][0]; v=m[2][0]; m[1][0]=c*u+s*v; m[2][0]=c*v-s*u;
  u=m[1][1]; v=m[2][1]; m[1][1]=c*u+s*v; m[2][1]=c*v-s*u;
  u=m[1][2]; v=m[2][2]; m[1][2]=c*u+s*v; m[2][2]=c*v-s*u;
  u=m[1][3]; v=m[2][3]; m[1][3]=c*u+s*v; m[2][3]=c*v-s*u;
  return *this;
  }


// Rotate by angle about x-axis
FXMat4f& FXMat4f::xrot(FXfloat phi){
  return xrot(cosf(phi),sinf(phi));
  }


// Rotate about y-axis
FXMat4f& FXMat4f::yrot(FXfloat c,FXfloat s){
  register FXfloat u,v;
  FXASSERT(-1.00001f<c && c<1.00001f && -1.00001f<s && s<1.00001f);
  u=m[0][0]; v=m[2][0]; m[0][0]=c*u-s*v; m[2][0]=c*v+s*u;
  u=m[0][1]; v=m[2][1]; m[0][1]=c*u-s*v; m[2][1]=c*v+s*u;
  u=m[0][2]; v=m[2][2]; m[0][2]=c*u-s*v; m[2][2]=c*v+s*u;
  u=m[0][3]; v=m[2][3]; m[0][3]=c*u-s*v; m[2][3]=c*v+s*u;
  return *this;
  }


// Rotate by angle about y-axis
FXMat4f& FXMat4f::yrot(FXfloat phi){
  return yrot(cosf(phi),sinf(phi));
  }


// Rotate about z-axis
FXMat4f& FXMat4f::zrot(FXfloat c,FXfloat s){
  register FXfloat u,v;
  FXASSERT(-1.00001f<c && c<1.00001f && -1.00001f<s && s<1.00001f);
  u=m[0][0]; v=m[1][0]; m[0][0]=c*u+s*v; m[1][0]=c*v-s*u;
  u=m[0][1]; v=m[1][1]; m[0][1]=c*u+s*v; m[1][1]=c*v-s*u;
  u=m[0][2]; v=m[1][2]; m[0][2]=c*u+s*v; m[1][2]=c*v-s*u;
  u=m[0][3]; v=m[1][3]; m[0][3]=c*u+s*v; m[1][3]=c*v-s*u;
  return *this;
  }


// Rotate by angle about z-axis
FXMat4f& FXMat4f::zrot(FXfloat phi){
  return zrot(cosf(phi),sinf(phi));
  }


// Translate
FXMat4f& FXMat4f::trans(FXfloat tx,FXfloat ty,FXfloat tz){
  m[3][0]=m[3][0]+tx*m[0][0]+ty*m[1][0]+tz*m[2][0];
  m[3][1]=m[3][1]+tx*m[0][1]+ty*m[1][1]+tz*m[2][1];
  m[3][2]=m[3][2]+tx*m[0][2]+ty*m[1][2]+tz*m[2][2];
  m[3][3]=m[3][3]+tx*m[0][3]+ty*m[1][3]+tz*m[2][3];
  return *this;
  }


// Translate over vector
FXMat4f& FXMat4f::trans(const FXVec3f& v){
  return trans(v[0],v[1],v[2]);
  }


// Scale unqual
FXMat4f& FXMat4f::scale(FXfloat sx,FXfloat sy,FXfloat sz){
  m[0][0]*=sx; m[0][1]*=sx; m[0][2]*=sx; m[0][3]*=sx;
  m[1][0]*=sy; m[1][1]*=sy; m[1][2]*=sy; m[1][3]*=sy;
  m[2][0]*=sz; m[2][1]*=sz; m[2][2]*=sz; m[2][3]*=sz;
  return *this;
  }


// Scale uniform
FXMat4f& FXMat4f::scale(FXfloat s){
  return scale(s,s,s);
  }


// Scale matrix
FXMat4f& FXMat4f::scale(const FXVec3f& v){
  return scale(v[0],v[1],v[2]);
  }


// Calculate determinant
FXfloat det(const FXMat4f& a){
  return DET4(a[0][0],a[0][1],a[0][2],a[0][3],
              a[1][0],a[1][1],a[1][2],a[1][3],
              a[2][0],a[2][1],a[2][2],a[2][3],
              a[3][0],a[3][1],a[3][2],a[3][3]);
  }


// Transpose matrix
FXMat4f transpose(const FXMat4f& a){
  return FXMat4f(a[0][0],a[1][0],a[2][0],a[3][0],
                 a[0][1],a[1][1],a[2][1],a[3][1],
                 a[0][2],a[1][2],a[2][2],a[3][2],
                 a[0][3],a[1][3],a[2][3],a[3][3]);
  }


// Invert matrix
FXMat4f invert(const FXMat4f& s){
  FXMat4f m(1.0f,0.0f,0.0f,0.0f,0.0f,1.0f,0.0f,0.0f,0.0f,0.0f,1.0f,0.0f,0.0f,0.0f,0.0f,1.0f);
  FXMat4f x(s);
  register FXfloat pvv,t;
  register int i,j,pvi;
  for(i=0; i<4; i++){
    pvv=x[i][i];
    pvi=i;
    for(j=i+1; j<4; j++){   // Find pivot (largest in column i)
      if(fabsf(x[j][i])>fabsf(pvv)){
        pvi=j;
        pvv=x[j][i];
        }
      }
    FXASSERT(pvv != 0.0f);  // Should not be singular
    if(pvi!=i){             // Swap rows i and pvi
      FXSWAP(m[i][0],m[pvi][0],t); FXSWAP(m[i][1],m[pvi][1],t); FXSWAP(m[i][2],m[pvi][2],t); FXSWAP(m[i][3],m[pvi][3],t);
      FXSWAP(x[i][0],x[pvi][0],t); FXSWAP(x[i][1],x[pvi][1],t); FXSWAP(x[i][2],x[pvi][2],t); FXSWAP(x[i][3],x[pvi][3],t);
      }
    x[i][0]/=pvv; x[i][1]/=pvv; x[i][2]/=pvv; x[i][3]/=pvv;
    m[i][0]/=pvv; m[i][1]/=pvv; m[i][2]/=pvv; m[i][3]/=pvv;
    for(j=0; j<4; j++){     // Eliminate column i
      if(j!=i){
        t=x[j][i];
        x[j][0]-=x[i][0]*t; x[j][1]-=x[i][1]*t; x[j][2]-=x[i][2]*t; x[j][3]-=x[i][3]*t;
        m[j][0]-=m[i][0]*t; m[j][1]-=m[i][1]*t; m[j][2]-=m[i][2]*t; m[j][3]-=m[i][3]*t;
        }
      }
    }
  return m;
  }


// Look at
FXMat4f& FXMat4f::look(const FXVec3f& eye,const FXVec3f& cntr,const FXVec3f& vup){
  register FXfloat x0,x1,x2,tx,ty,tz;
  FXVec3f rz,rx,ry;
  rz=normalize(eye-cntr);
  rx=normalize(vup^rz);
  ry=normalize(rz^rx);
  tx= -eye[0]*rx[0]-eye[1]*rx[1]-eye[2]*rx[2];
  ty= -eye[0]*ry[0]-eye[1]*ry[1]-eye[2]*ry[2];
  tz= -eye[0]*rz[0]-eye[1]*rz[1]-eye[2]*rz[2];
  x0=m[0][0]; x1=m[0][1]; x2=m[0][2];
  m[0][0]=rx[0]*x0+rx[1]*x1+rx[2]*x2+tx*m[0][3];
  m[0][1]=ry[0]*x0+ry[1]*x1+ry[2]*x2+ty*m[0][3];
  m[0][2]=rz[0]*x0+rz[1]*x1+rz[2]*x2+tz*m[0][3];
  x0=m[1][0]; x1=m[1][1]; x2=m[1][2];
  m[1][0]=rx[0]*x0+rx[1]*x1+rx[2]*x2+tx*m[1][3];
  m[1][1]=ry[0]*x0+ry[1]*x1+ry[2]*x2+ty*m[1][3];
  m[1][2]=rz[0]*x0+rz[1]*x1+rz[2]*x2+tz*m[1][3];
  x0=m[2][0]; x1=m[2][1]; x2=m[2][2];
  m[2][0]=rx[0]*x0+rx[1]*x1+rx[2]*x2+tx*m[2][3];
  m[2][1]=ry[0]*x0+ry[1]*x1+ry[2]*x2+ty*m[2][3];
  m[2][2]=rz[0]*x0+rz[1]*x1+rz[2]*x2+tz*m[2][3];
  x0=m[3][0]; x1=m[3][1]; x2=m[3][2];
  m[3][0]=rx[0]*x0+rx[1]*x1+rx[2]*x2+tx*m[3][3];
  m[3][1]=ry[0]*x0+ry[1]*x1+ry[2]*x2+ty*m[3][3];
  m[3][2]=rz[0]*x0+rz[1]*x1+rz[2]*x2+tz*m[3][3];
  return *this;
  }


// Save to archive
FXStream& operator<<(FXStream& store,const FXMat4f& m){
  store << m[0] << m[1] << m[2] << m[3];
  return store;
  }


// Load from archive
FXStream& operator>>(FXStream& store,FXMat4f& m){
  store >> m[0] >> m[1] >> m[2] >> m[3];
  return store;
  }

}
