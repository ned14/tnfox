/********************************************************************************
*                                                                               *
*            D o u b l e - P r e c i s i o n   3 x 3   M a t r i x              *
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
* $Id: FXMat3d.cpp,v 1.5 2004/02/08 17:29:06 fox Exp $                          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXStream.h"
#include "FXObject.h"
#include "FXVec2d.h"
#include "FXVec3d.h"
#include "FXMat3d.h"


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




/*******************************************************************************/

namespace FX {

// Build matrix from constant
FXMat3d::FXMat3d(FXdouble w){
  m[0][0]=w; m[0][1]=w; m[0][2]=w;
  m[1][0]=w; m[1][1]=w; m[1][2]=w;
  m[2][0]=w; m[2][1]=w; m[2][2]=w;
  }


// Build matrix from scalars
FXMat3d::FXMat3d(FXdouble a00,FXdouble a01,FXdouble a02,
                 FXdouble a10,FXdouble a11,FXdouble a12,
                 FXdouble a20,FXdouble a21,FXdouble a22){
  m[0][0]=a00; m[0][1]=a01; m[0][2]=a02;
  m[1][0]=a10; m[1][1]=a11; m[1][2]=a12;
  m[2][0]=a20; m[2][1]=a21; m[2][2]=a22;
  }


// Build matrix from three vectors
FXMat3d::FXMat3d(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c){
  m[0][0]=a[0]; m[0][1]=a[1]; m[0][2]=a[2];
  m[1][0]=b[0]; m[1][1]=b[1]; m[1][2]=b[2];
  m[2][0]=c[0]; m[2][1]=c[1]; m[2][2]=c[2];
  }


// Copy constructor
FXMat3d::FXMat3d(const FXMat3d& other){
  m[0]=other[0];
  m[1]=other[1];
  m[2]=other[2];
  }


// Assignment operator
FXMat3d& FXMat3d::operator=(const FXMat3d& other){
  m[0]=other[0];
  m[1]=other[1];
  m[2]=other[2];
  return *this;
  }


// Set matrix to constant
FXMat3d& FXMat3d::operator=(FXdouble w){
  m[0][0]=w; m[0][1]=w; m[0][2]=w;
  m[1][0]=w; m[1][1]=w; m[1][2]=w;
  m[2][0]=w; m[2][1]=w; m[2][2]=w;
  return *this;
  }


// Add matrices
FXMat3d& FXMat3d::operator+=(const FXMat3d& w){
  m[0][0]+=w[0][0]; m[0][1]+=w[0][1]; m[0][2]+=w[0][2];
  m[1][0]+=w[1][0]; m[1][1]+=w[1][1]; m[1][2]+=w[1][2];
  m[2][0]+=w[2][0]; m[2][1]+=w[2][1]; m[2][2]+=w[2][2];
  return *this;
  }


// Substract matrices
FXMat3d& FXMat3d::operator-=(const FXMat3d& w){
  m[0][0]-=w[0][0]; m[0][1]-=w[0][1]; m[0][2]-=w[0][2];
  m[1][0]-=w[1][0]; m[1][1]-=w[1][1]; m[1][2]-=w[1][2];
  m[2][0]-=w[2][0]; m[2][1]-=w[2][1]; m[2][2]-=w[2][2];
  return *this;
  }


// Multiply matrix by scalar
FXMat3d& FXMat3d::operator*=(FXdouble w){
  m[0][0]*=w; m[0][1]*=w; m[0][2]*=w;
  m[1][0]*=w; m[1][1]*=w; m[1][2]*=w;
  m[2][0]*=w; m[2][1]*=w; m[2][2]*=w;
  return *this;
  }


// Multiply matrix by matrix
FXMat3d& FXMat3d::operator*=(const FXMat3d& w){
  register FXdouble x,y,z;
  x=m[0][0]; y=m[0][1]; z=m[0][2];
  m[0][0]=x*w[0][0]+y*w[1][0]+z*w[2][0];
  m[0][1]=x*w[0][1]+y*w[1][1]+z*w[2][1];
  m[0][2]=x*w[0][2]+y*w[1][2]+z*w[2][2];
  x=m[1][0]; y=m[1][1]; z=m[1][2];
  m[1][0]=x*w[0][0]+y*w[1][0]+z*w[2][0];
  m[1][1]=x*w[0][1]+y*w[1][1]+z*w[2][1];
  m[1][2]=x*w[0][2]+y*w[1][2]+z*w[2][2];
  x=m[2][0]; y=m[2][1]; z=m[2][2];
  m[2][0]=x*w[0][0]+y*w[1][0]+z*w[2][0];
  m[2][1]=x*w[0][1]+y*w[1][1]+z*w[2][1];
  m[2][2]=x*w[0][2]+y*w[1][2]+z*w[2][2];
  return *this;
  }


// Divide matric by scalar
FXMat3d& FXMat3d::operator/=(FXdouble w){
  m[0][0]/=w; m[0][1]/=w; m[0][2]/=w;
  m[1][0]/=w; m[1][1]/=w; m[1][2]/=w;
  m[2][0]/=w; m[2][1]/=w; m[2][2]/=w;
  return *this;
  }


// Add matrices
FXMat3d operator+(const FXMat3d& a,const FXMat3d& b){
  return FXMat3d(a[0][0]+b[0][0],a[0][1]+b[0][1],a[0][2]+b[0][2],
                 a[1][0]+b[1][0],a[1][1]+b[1][1],a[1][2]+b[1][2],
                 a[2][0]+b[2][0],a[2][1]+b[2][1],a[2][2]+b[2][2]);
  }


// Substract matrices
FXMat3d operator-(const FXMat3d& a,const FXMat3d& b){
  return FXMat3d(a[0][0]-b[0][0],a[0][1]-b[0][1],a[0][2]-b[0][2],
                 a[1][0]-b[1][0],a[1][1]-b[1][1],a[1][2]-b[1][2],
                 a[2][0]-b[2][0],a[2][1]-b[2][1],a[2][2]-b[2][2]);
  }


// Negate matrix
FXMat3d operator-(const FXMat3d& a){
  return FXMat3d(-a[0][0],-a[0][1],-a[0][2],
                 -a[1][0],-a[1][1],-a[1][2],
                 -a[2][0],-a[2][1],-a[2][2]);
  }



// Composite matrices
FXMat3d operator*(const FXMat3d& a,const FXMat3d& b){
  register FXdouble x,y,z;
  FXMat3d r;
  x=a[0][0]; y=a[0][1]; z=a[0][2];
  r[0][0]=x*b[0][0]+y*b[1][0]+z*b[2][0];
  r[0][1]=x*b[0][1]+y*b[1][1]+z*b[2][1];
  r[0][2]=x*b[0][2]+y*b[1][2]+z*b[2][2];
  x=a[1][0]; y=a[1][1]; z=a[1][2];
  r[1][0]=x*b[0][0]+y*b[1][0]+z*b[2][0];
  r[1][1]=x*b[0][1]+y*b[1][1]+z*b[2][1];
  r[1][2]=x*b[0][2]+y*b[1][2]+z*b[2][2];
  x=a[2][0]; y=a[2][1]; z=a[2][2];
  r[2][0]=x*b[0][0]+y*b[1][0]+z*b[2][0];
  r[2][1]=x*b[0][1]+y*b[1][1]+z*b[2][1];
  r[2][2]=x*b[0][2]+y*b[1][2]+z*b[2][2];
  return r;
  }


// Multiply scalar by matrix
FXMat3d operator*(FXdouble x,const FXMat3d& m){
  return FXMat3d(x*m[0][0],x*m[0][1],x*m[0][2],
                 x*m[1][0],x*m[1][1],x*m[1][2],
                 x*m[2][0],x*m[2][1],x*m[2][2]);
  }


// Multiply matrix by scalar
FXMat3d operator*(const FXMat3d& m,FXdouble x){
  return FXMat3d(m[0][0]*x,m[0][1]*x,m[0][2]*x,
                 m[1][0]*x,m[1][1]*x,m[1][2]*x,
                 m[2][0]*x,m[2][1]*x,m[2][2]*x);
  }


// Divide scalar by matrix
FXMat3d operator/(FXdouble x,const FXMat3d& m){
  return FXMat3d(x/m[0][0],x/m[0][1],x/m[0][2],
                 x/m[1][0],x/m[1][1],x/m[1][2],
                 x/m[2][0],x/m[2][1],x/m[2][2]);
  }


// Divide matrix by scalar
FXMat3d operator/(const FXMat3d& m,FXdouble x){
  return FXMat3d(m[0][0]/x,m[0][1]/x,m[0][2]/x,
                 m[1][0]/x,m[1][1]/x,m[1][2]/x,
                 m[2][0]/x,m[2][1]/x,m[2][2]/x);
  }


// Vector times matrix
FXVec3d operator*(const FXVec3d& v,const FXMat3d& m){
  register FXdouble x=v[0],y=v[1],z=v[2];
  return FXVec3d(x*m[0][0]+y*m[1][0]+z*m[2][0],
                 x*m[0][1]+y*m[1][1]+z*m[2][1],
                 x*m[0][2]+y*m[1][2]+z*m[2][2]);
  }


// Matrix times vector
FXVec3d operator*(const FXMat3d& m,const FXVec3d& v){
  register FXdouble x=v.x,y=v.y,z=v.z;
  return FXVec3d(x*m[0][0]+y*m[0][1]+z*m[0][2],
                 x*m[1][0]+y*m[1][1]+z*m[1][2],
                 x*m[2][0]+y*m[2][1]+z*m[2][2]);
  }


// Vector times matrix
FXVec2d operator*(const FXVec2d& v,const FXMat3d& m){
  register FXdouble x=v.x,y=v.y;
  FXASSERT(m[0][2]==0.0 && m[1][2]==0.0 && m[2][2]==1.0);
  return FXVec2d(x*m[0][0]+y*m[1][0]+m[2][0],
                 x*m[0][1]+y*m[1][1]+m[2][1]);
  }


// Matrix times vector
FXVec2d operator*(const FXMat3d& m,const FXVec2d& v){
  register FXdouble x=v.x,y=v.y;
  FXASSERT(m[0][2]==0.0 && m[1][2]==0.0 && m[2][2]==1.0);
  return FXVec2d(x*m[0][0]+y*m[0][1]+m[0][2],
                 x*m[1][0]+y*m[1][1]+m[1][2]);
  }


// Make unit matrix
FXMat3d& FXMat3d::eye(){
  m[0][0]=1.0; m[0][1]=0.0; m[0][2]=0.0;
  m[1][0]=0.0; m[1][1]=1.0; m[1][2]=0.0;
  m[2][0]=0.0; m[2][1]=0.0; m[2][2]=1.0;
  return *this;
  }


// Rotate by cosine, sine
FXMat3d& FXMat3d::rot(FXdouble c,FXdouble s){
  register FXdouble u,v;
  FXASSERT(-1.00001<c && c<1.00001 && -1.00001<s && s<1.00001);
  u=m[0][0]; v=m[1][0]; m[0][0]=c*u+s*v; m[1][0]=c*v-s*u;
  u=m[0][1]; v=m[1][1]; m[0][1]=c*u+s*v; m[1][1]=c*v-s*u;
  u=m[0][2]; v=m[1][2]; m[0][2]=c*u+s*v; m[1][2]=c*v-s*u;
  return *this;
  }


// Rotate by angle
FXMat3d& FXMat3d::rot(FXdouble phi){
  return rot(cos(phi),sin(phi));
  }


// Translate
FXMat3d& FXMat3d::trans(FXdouble tx,FXdouble ty){
  m[2][0]=m[2][0]+tx*m[0][0]+ty*m[1][0];
  m[2][1]=m[2][1]+tx*m[0][1]+ty*m[1][1];
  m[2][2]=m[2][2]+tx*m[0][2]+ty*m[1][2];
  return *this;
  }


// Scale unqual
FXMat3d& FXMat3d::scale(FXdouble sx,FXdouble sy){
  m[0][0]*=sx; m[0][1]*=sx; m[0][2]*=sx;
  m[1][0]*=sy; m[1][1]*=sy; m[1][2]*=sy;
  return *this;
  }


// Scale uniform
FXMat3d& FXMat3d::scale(FXdouble s){
  return scale(s,s);
  }


// Calculate determinant
FXdouble det(const FXMat3d& a){
  return DET3(a[0][0],a[0][1],a[0][2],
              a[1][0],a[1][1],a[1][2],
              a[2][0],a[2][1],a[2][2]);
  }


// Transpose matrix
FXMat3d transpose(const FXMat3d& a){
  return FXMat3d(a[0][0],a[1][0],a[2][0],
                 a[0][1],a[1][1],a[2][1],
                 a[0][2],a[1][2],a[2][2]);
  }


// Invert matrix
FXMat3d invert(const FXMat3d& s){
  register FXdouble det;
  FXMat3d m;
  m[0][0]=s[1][1]*s[2][2]-s[1][2]*s[2][1];
  m[0][1]=s[0][2]*s[2][1]-s[0][1]*s[2][2];
  m[0][2]=s[0][1]*s[1][2]-s[0][2]*s[1][1];

  m[1][0]=s[1][2]*s[2][0]-s[1][0]*s[2][2];
  m[1][1]=s[0][0]*s[2][2]-s[0][2]*s[2][0];
  m[1][2]=s[0][2]*s[1][0]-s[0][0]*s[1][2];

  m[2][0]=s[1][0]*s[2][1]-s[1][1]*s[2][0];
  m[2][1]=s[0][1]*s[2][0]-s[0][0]*s[2][1];
  m[2][2]=s[0][0]*s[1][1]-s[0][1]*s[1][0];

  det=s[0][0]*m[0][0]+s[0][1]*m[1][0]+s[0][2]*m[2][0];
  //if(det==0.0) throw FXException("FXMat3d is singular.");
  FXASSERT(det!=0.0);

  m[0][0]/=det;
  m[0][1]/=det;
  m[0][2]/=det;
  m[1][0]/=det;
  m[1][1]/=det;
  m[1][2]/=det;
  m[2][0]/=det;
  m[2][1]/=det;
  m[2][2]/=det;
  return m;
  }


// Save to archive
FXStream& operator<<(FXStream& store,const FXMat3d& m){
  store << m[0] << m[1] << m[2];
  return store;
  }


// Load from archive
FXStream& operator>>(FXStream& store,FXMat3d& m){
  store >> m[0] >> m[1] >> m[2];
  return store;
  }

}
