/********************************************************************************
*                                                                               *
*       D o u b l e - P r e c i s i o n   4 - E l e m e n t   V e c t o r       *
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
* $Id: FXVec4d.cpp,v 1.3 2004/02/08 17:29:07 fox Exp $                          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXStream.h"
#include "FXObject.h"
#include "FXVec2d.h"
#include "FXVec3d.h"
#include "FXVec4d.h"



/*******************************************************************************/

namespace FX {

FXVec4d::FXVec4d(FXColor color){
  x=0.003921568627*FXREDVAL(color);
  y=0.003921568627*FXGREENVAL(color);
  z=0.003921568627*FXBLUEVAL(color);
  w=0.003921568627*FXALPHAVAL(color);
  }


FXVec4d& FXVec4d::operator=(FXColor color){
  x=0.003921568627*FXREDVAL(color);
  y=0.003921568627*FXGREENVAL(color);
  z=0.003921568627*FXBLUEVAL(color);
  w=0.003921568627*FXALPHAVAL(color);
  return *this;
  }


FXVec4d::operator FXColor() const {
  return FXRGBA((x*255.0),(y*255.0),(z*255.0),(w*255.0));
  }


FXVec4d normalize(const FXVec4d& a){
  register FXdouble t=len(a);
  if(t>0.0){ return FXVec4d(a.x/t,a.y/t,a.z/t,a.w/t); }
  return FXVec4d(0.0,0.0,0.0,0.0);
  }


FXStream& operator<<(FXStream& store,const FXVec4d& v){
  store << v.x << v.y << v.z << v.w;
  return store;
  }

FXStream& operator>>(FXStream& store,FXVec4d& v){
  store >> v.x >> v.y >> v.z >> v.w;
  return store;
  }

}
