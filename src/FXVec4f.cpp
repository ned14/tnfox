/********************************************************************************
*                                                                               *
*       S i n g l e - P r e c i s i o n   4 - E l e m e n t   V e c t o r       *
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
* $Id: FXVec4f.cpp,v 1.10 2005/01/16 16:06:07 fox Exp $                          *
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



/*******************************************************************************/

namespace FX {

FXVec4f::FXVec4f(FXColor color){
  x=0.003921568627f*FXREDVAL(color);
  y=0.003921568627f*FXGREENVAL(color);
  z=0.003921568627f*FXBLUEVAL(color);
  w=0.003921568627f*FXALPHAVAL(color);
  }


FXVec4f& FXVec4f::operator=(FXColor color){
  x=0.003921568627f*FXREDVAL(color);
  y=0.003921568627f*FXGREENVAL(color);
  z=0.003921568627f*FXBLUEVAL(color);
  w=0.003921568627f*FXALPHAVAL(color);
  return *this;
  }


FXVec4f::operator FXColor() const {
  return FXRGBA((FXuchar)(x*255.0f),(FXuchar)(y*255.0f),(FXuchar)(z*255.0f),(FXuchar)(w*255.0f));
  }


// Normalize vector
FXVec4f vecnormalize(const FXVec4f& a){
  register FXfloat t=veclen(a);
  if(t>0.0f){ return FXVec4f(a.x/t,a.y/t,a.z/t,a.w/t); }
  return FXVec4f(0.0f,0.0f,0.0f,0.0f);
  }


// Compute plane equation from 3 points a,b,c
FXVec4f vecplane(const FXVec3f& a,const FXVec3f& b,const FXVec3f& c){
  FXVec3f nm(vecnormal(a,b,c));
  return FXVec4f(nm,-(nm.x*a.x+nm.y*a.y+nm.z*a.z));
  }


// Compute plane equation from vector and distance
FXVec4f vecplane(const FXVec3f& vec,FXfloat dist){
  FXVec3f nm(vecnormalize(vec));
  return FXVec4f(nm,-dist);
  }


// Compute plane equation from vector and point on plane
FXVec4f vecplane(const FXVec3f& vec,const FXVec3f& p){
  FXVec3f nm(vecnormalize(vec));
  return FXVec4f(nm,-(nm.x*p.x+nm.y*p.y+nm.z*p.z));
  }


// Compute plane equation from 4 vector
FXVec4f vecplane(const FXVec4f& vec){
  register FXfloat t=sqrtf(vec.x*vec.x+vec.y*vec.y+vec.z*vec.z);
  return FXVec4f(vec.x/t,vec.y/t,vec.z/t,vec.w/t);
  }


// Signed distance normalized plane and point
FXfloat vecdistance(const FXVec4f& plane,const FXVec3f& p){
  return plane.x*p.x+plane.y*p.y+plane.z*p.z+plane.z;
  }


// Return true if edge a-b crosses plane
FXbool veccrosses(const FXVec4f& plane,const FXVec3f& a,const FXVec3f& b){
  return (vecdistance(plane,a)>=0.0f) ^ (vecdistance(plane,b)>=0.0f);
  }


// Save vector to stream
FXStream& operator<<(FXStream& store,const FXVec4f& v){
  store << v.x << v.y << v.z << v.w;
  return store;
  }


// Load vector from stream
FXStream& operator>>(FXStream& store,FXVec4f& v){
  store >> v.x >> v.y >> v.z >> v.w;
  return store;
  }

}
