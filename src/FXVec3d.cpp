/********************************************************************************
*                                                                               *
*       D o u b l e - P r e c i s i o n   3 - E l e m e n t   V e c t o r       *
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
* $Id: FXVec3d.cpp,v 1.3 2004/02/08 17:29:07 fox Exp $                          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
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
  return FXRGB((x*255.0),(y*255.0),(z*255.0));
  }


FXVec3d normalize(const FXVec3d& a){
  register FXdouble t=len(a);
  if(t>0.0){ return FXVec3d(a.x/t,a.y/t,a.z/t); }
  return FXVec3d(0.0,0.0,0.0);
  }


FXStream& operator<<(FXStream& store,const FXVec3d& v){
  store << v.x << v.y << v.z;
  return store;
  }


FXStream& operator>>(FXStream& store,FXVec3d& v){
  store >> v.x >> v.y >> v.z;
  return store;
  }

}
