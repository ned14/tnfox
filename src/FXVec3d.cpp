/********************************************************************************
*                                                                               *
*       D o u b l e - P r e c i s i o n   3 - E l e m e n t   V e c t o r       *
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
* $Id: FXVec3d.cpp,v 1.7 2005/01/16 16:06:07 fox Exp $                          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXObject.h"
#include "FXVec2d.h"
#include "FXVec3d.h"




/*******************************************************************************/

namespace FX {

FXVec3d::FXVec3d(FXColor color){
  x=0.003921568627*FXREDVAL(color);
  y=0.003921568627*FXGREENVAL(color);
  z=0.003921568627*FXBLUEVAL(color);
  }


FXVec3d& FXVec3d::operator=(FXColor color){
  x=0.003921568627*FXREDVAL(color);
  y=0.003921568627*FXGREENVAL(color);
  z=0.003921568627*FXBLUEVAL(color);
  return *this;
  }


FXVec3d::operator FXColor() const {
  return FXRGB((FXuchar)(x*255.0),(FXuchar)(y*255.0),(FXuchar)(z*255.0));
  }


FXVec3d vecnormalize(const FXVec3d& a){
  register FXdouble t=veclen(a);
  if(t>0.0){ return FXVec3d(a.x/t,a.y/t,a.z/t); }
  return FXVec3d(0.0,0.0,0.0);
  }


// Compute normal from three points a,b,c
FXVec3d vecnormal(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c){
  return vecnormalize((b-a)^(c-a));
  }


// Compute approximate normal from four points a,b,c,d
FXVec3d vecnormal(const FXVec3d& a,const FXVec3d& b,const FXVec3d& c,const FXVec3d& d){
  return vecnormalize((c-a)^(d-b));
  }


// Save vector to stream
FXStream& operator<<(FXStream& store,const FXVec3d& v){
  store << v.x << v.y << v.z;
  return store;
  }


// Load vector from stream
FXStream& operator>>(FXStream& store,FXVec3d& v){
  store >> v.x >> v.y >> v.z;
  return store;
  }

}
