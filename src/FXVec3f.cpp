/********************************************************************************
*                                                                               *
*       S i n g l e - P r e c i s i o n   3 - E l e m e n t   V e c t o r       *
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
* $Id: FXVec3f.cpp,v 1.9 2005/01/16 16:06:07 fox Exp $                          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXObject.h"
#include "FXVec2f.h"
#include "FXVec3f.h"





/*******************************************************************************/

namespace FX {

FXVec3f::FXVec3f(FXColor color){
  x=0.003921568627f*FXREDVAL(color);
  y=0.003921568627f*FXGREENVAL(color);
  z=0.003921568627f*FXBLUEVAL(color);
  }


FXVec3f& FXVec3f::operator=(FXColor color){
  x=0.003921568627f*FXREDVAL(color);
  y=0.003921568627f*FXGREENVAL(color);
  z=0.003921568627f*FXBLUEVAL(color);
  return *this;
  }


FXVec3f::operator FXColor() const {
  return FXRGB((x*255.0f),(y*255.0f),(z*255.0f));
  }


FXVec3f normalize(const FXVec3f& a){
  register FXfloat t=len(a);
  if(t>0.0f){ return FXVec3f(a.x/t,a.y/t,a.z/t); }
  return FXVec3f(0.0f,0.0f,0.0f);
  }


// Compute normal from three points a,b,c
FXVec3f normal(const FXVec3f& a,const FXVec3f& b,const FXVec3f& c){
  return normalize((b-a)^(c-a));
  }


// Compute approximate normal from four points a,b,c,d
FXVec3f normal(const FXVec3f& a,const FXVec3f& b,const FXVec3f& c,const FXVec3f& d){
  return normalize((c-a)^(d-b));
  }


// Save vector to stream
FXStream& operator<<(FXStream& store,const FXVec3f& v){
  store << v.x << v.y << v.z;
  return store;
  }


// Load vector from stream
FXStream& operator>>(FXStream& store,FXVec3f& v){
  store >> v.x >> v.y >> v.z;
  return store;
  }

}
