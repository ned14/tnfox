/********************************************************************************
*                                                                               *
*           S i n g l e - P r e c i s i o n    S p h e r e    C l a s s         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004 by Jeroen van der Zijp.   All Rights Reserved.             *
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
* $Id: FXSpheref.cpp,v 1.6 2004/02/24 23:16:10 fox Exp $                        *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXStream.h"
#include "FXVec2f.h"
#include "FXVec3f.h"
#include "FXVec4f.h"
#include "FXSpheref.h"
#include "FXRangef.h"

/*
  Notes:
  - This is new, untested code; caveat emptor!
*/


using namespace FX;

/**************************  S p h e r e   C l a s s   *************************/

namespace FX {


inline FXfloat sqrf(FXfloat x){ return x*x; }


// Initialize from bounding box
FXSpheref::FXSpheref(const FXRangef& bounds){
  center=bounds.center();
  radius=bounds.diameter()*0.5f;
  }



// Test if sphere contains point x,y,z
FXbool FXSpheref::contains(FXfloat x,FXfloat y,FXfloat z) const {
  register FXfloat dx=center.x-x;
  register FXfloat dy=center.y-y;
  register FXfloat dz=center.z-z;
  return dx*dx+dx*dy+dz*dz<radius*radius;
  }


// Test if sphere contains point p
FXbool FXSpheref::contains(const FXVec3f& p) const {
  return contains(p.x,p.y,p.z);
  }


// Test if sphere contains another box
FXbool FXSpheref::contains(const FXRangef& box) const {
  return contains(box.corner(0)) &&
         contains(box.corner(1)) &&
         contains(box.corner(2)) &&
         contains(box.corner(3)) &&
         contains(box.corner(4)) &&
         contains(box.corner(5)) &&
         contains(box.corner(6)) &&
         contains(box.corner(7));
  }


// Test if sphere properly contains another sphere
FXbool FXSpheref::contains(const FXSpheref& sphere) const {
  if(radius>=sphere.radius){
    register FXfloat dx=center.x-sphere.center.x;
    register FXfloat dy=center.y-sphere.center.y;
    register FXfloat dz=center.z-sphere.center.z;
    return sqrtf(dx*dx+dx*dy+dz*dz)<radius-sphere.radius;
    }
  return FALSE;
  }


// Include point
FXSpheref& FXSpheref::include(FXfloat x,FXfloat y,FXfloat z){
  register FXfloat dx=center.x-x;
  register FXfloat dy=center.y-y;
  register FXfloat dz=center.z-z;
  register FXfloat dd=sqrtf(dx*dx+dx*dy+dz*dz);
  if(dd>radius) radius=dd;
  return *this;
  }


// Include point
FXSpheref& FXSpheref::include(const FXVec3f& p){
  return include(p.x,p.y,p.z);
  }


// Include given range into this one
FXSpheref& FXSpheref::include(const FXRangef& box){
  include(box.corner(0));
  include(box.corner(1)); // FIXME there's got to be a better trick!
  include(box.corner(2));
  include(box.corner(3));
  include(box.corner(4));
  include(box.corner(5));
  include(box.corner(6));
  include(box.corner(7));
  return *this;
  }


// Include given sphere into this one
FXSpheref& FXSpheref::include(const FXSpheref& sphere){
  register FXfloat dx=sphere.center.x-center.x;
  register FXfloat dy=sphere.center.y-center.y;
  register FXfloat dz=sphere.center.z-center.z;
  register FXfloat dist=sqrtf(dx*dx+dy*dy+dz*dz);
  register FXfloat new_radius,delta;

  // Rule out easy case
  if(radius<dist+sphere.radius){

    // New sphere contains this one
    if(sphere.radius>dist+radius){
      center=sphere.center;
      radius=sphere.radius;
      }

    // Same centers, take max radius
    else if(dist<=0.0f){
      if(radius<sphere.radius) radius=sphere.radius;
      }

    // Update this sphere if sphere extends outside this radius
    else{
      new_radius=0.5f*(radius+dist+sphere.radius);
      delta=(new_radius-radius);
      center.x+=delta*dx/dist;
      center.y+=delta*dy/dist;
      center.z+=delta*dz/dist;
      radius=new_radius;
      }
    }
  return *this;
  }



// Intersect sphere with plane ax+by+cz+w; returns -1,0,+1
FXint FXSpheref::intersect(const FXVec4f& plane) const {
  register FXfloat rr=radius*sqrtf(plane.x*plane.x+plane.y*plane.y+plane.z*plane.z);

  // Lower point on positive side of plane
  if(plane.x*center.x+plane.y*center.y+plane.z*center.z+plane.w>=rr) return 1;

  // Upper point on negative side of plane
  if(plane.x*center.x+plane.y*center.y+plane.z*center.z+plane.w<=rr) return -1;

  // Overlap
  return 0;
  }


// Intersect sphere with ray u-v
FXbool FXSpheref::intersect(const FXVec3f& u,const FXVec3f& v) const {
  if(radius>0.0f){
    FXfloat rr=radius*radius;
    FXVec3f uc=center-u;        // Vector from u to center
    FXfloat dd=len2(uc);
    if(dd>rr){                  // Ray start point outside sphere
      FXVec3f uv=v-u;           // Vector from u to v
      FXfloat hh=uc*uv;         // If hh<0, uv points away from center
      if(0.0f<=hh){             // Not away from sphere
        FXfloat kk=len2(uv);
        FXfloat disc=hh*hh-kk*(dd-rr);  // FIXME this needs to be checked again!
        if(disc<=0.0) return FALSE;
        return TRUE;
        }
      return FALSE;
      }
    return TRUE;
    }
  return FALSE;
  }


// Test if box overlaps with sphere; algorithm due to Arvo (GEMS I)
FXbool overlap(const FXRangef& a,const FXSpheref& b){
  register FXfloat dd=0.0f;

  if(b.center.x<a.lower.x)
    dd+=sqrf(b.center.x-a.lower.x);
  else if(b.center.x>a.upper.x)
    dd+=sqrf(b.center.x-a.upper.x);

  if(b.center.y<a.lower.y)
    dd+=sqrf(b.center.y-a.lower.y);
  else if(b.center.y>a.upper.y)
    dd+=sqrf(b.center.y-a.upper.y);

  if(b.center.z<a.lower.z)
    dd+=sqrf(b.center.z-a.lower.z);
  else if(b.center.z>a.upper.z)
    dd+=sqrf(b.center.z-a.upper.z);

  return dd<=b.radius*b.radius;
  }


// Test if sphere overlaps with box
FXbool overlap(const FXSpheref& a,const FXRangef& b){
  return overlap(b,a);
  }


// Test if spheres overlap
FXbool overlap(const FXSpheref& a,const FXSpheref& b){
  register FXfloat dx=a.center.x-b.center.x;
  register FXfloat dy=a.center.y-b.center.y;
  register FXfloat dz=a.center.z-b.center.z;
  return sqrtf(dx*dx+dy*dy+dz*dz)<(a.radius+b.radius);
  }


// Saving
FXStream& operator<<(FXStream& store,const FXSpheref& sphere){
  store << sphere.center << sphere.radius;
  return store;
  }


// Loading
FXStream& operator>>(FXStream& store,FXSpheref& sphere){
  store >> sphere.center >> sphere.radius;
  return store;
  }

}
