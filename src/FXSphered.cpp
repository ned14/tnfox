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
* $Id: FXSphered.cpp,v 1.4 2004/02/24 23:16:10 fox Exp $                        *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXStream.h"
#include "FXVec2d.h"
#include "FXVec3d.h"
#include "FXVec4d.h"
#include "FXSphered.h"
#include "FXRanged.h"

/*
  Notes:
  - This is new, untested code; caveat emptor!
*/


using namespace FX;

/**************************  S p h e r e   C l a s s   *************************/

namespace FX {


inline FXdouble sqr(FXdouble x){ return x*x; }


// Initialize from bounding box
FXSphered::FXSphered(const FXRanged& bounds){
  center=bounds.center();
  radius=bounds.diameter()*0.5;
  }



// Test if sphere contains point x,y,z
FXbool FXSphered::contains(FXdouble x,FXdouble y,FXdouble z) const {
  register FXdouble dx=center.x-x;
  register FXdouble dy=center.y-y;
  register FXdouble dz=center.z-z;
  return dx*dx+dx*dy+dz*dz<radius*radius;
  }


// Test if sphere contains point p
FXbool FXSphered::contains(const FXVec3d& p) const {
  return contains(p.x,p.y,p.z);
  }


// Test if sphere contains another box
FXbool FXSphered::contains(const FXRanged& box) const {
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
FXbool FXSphered::contains(const FXSphered& sphere) const {
  if(radius>=sphere.radius){
    register FXdouble dx=center.x-sphere.center.x;
    register FXdouble dy=center.y-sphere.center.y;
    register FXdouble dz=center.z-sphere.center.z;
    return sqrt(dx*dx+dx*dy+dz*dz)<radius-sphere.radius;
    }
  return FALSE;
  }


// Include point
FXSphered& FXSphered::include(FXdouble x,FXdouble y,FXdouble z){
  register FXdouble dx=center.x-x;
  register FXdouble dy=center.y-y;
  register FXdouble dz=center.z-z;
  register FXdouble dd=sqrt(dx*dx+dx*dy+dz*dz);
  if(dd>radius) radius=dd;
  return *this;
  }


// Include point
FXSphered& FXSphered::include(const FXVec3d& p){
  return include(p.x,p.y,p.z);
  }


// Include given range into this one
FXSphered& FXSphered::include(const FXRanged& box){
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
FXSphered& FXSphered::include(const FXSphered& sphere){
  register FXdouble dx=sphere.center.x-center.x;
  register FXdouble dy=sphere.center.y-center.y;
  register FXdouble dz=sphere.center.z-center.z;
  register FXdouble dist=sqrtf(dx*dx+dy*dy+dz*dz);
  register FXdouble new_radius,delta;

  // Rule out easy case
  if(radius<dist+sphere.radius){

    // New sphere contains this one
    if(sphere.radius>dist+radius){
      center=sphere.center;
      radius=sphere.radius;
      }

    // Same centers, take max radius
    else if(dist<=0.0){
      if(radius<sphere.radius) radius=sphere.radius;
      }

    // Update this sphere if sphere extends outside this radius
    else{
      new_radius=0.5*(radius+dist+sphere.radius);
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
FXint FXSphered::intersect(const FXVec4d& plane) const {
  register FXdouble rr=radius*sqrt(plane.x*plane.x+plane.y*plane.y+plane.z*plane.z);

  // Lower point on positive side of plane
  if(plane.x*center.x+plane.y*center.y+plane.z*center.z+plane.w>=rr) return 1;

  // Upper point on negative side of plane
  if(plane.x*center.x+plane.y*center.y+plane.z*center.z+plane.w<=rr) return -1;

  // Overlap
  return 0;
  }


// Intersect sphere with ray u-v
FXbool FXSphered::intersect(const FXVec3d& u,const FXVec3d& v) const {
  if(radius>0.0){
    FXdouble rr=radius*radius;
    FXVec3d uc=center-u;        // Vector from u to center
    FXdouble dd=len2(uc);
    if(dd>rr){                  // Ray start point outside sphere
      FXVec3d uv=v-u;           // Vector from u to v
      FXdouble hh=uc*uv;        // If hh<0, uv points away from center
      if(0.0<=hh){              // Not away from sphere
        FXdouble kk=len2(uv);
        FXdouble disc=hh*hh-kk*(dd-rr); // FIXME this needs to be checked again!
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
FXbool overlap(const FXRanged& a,const FXSphered& b){
  register FXdouble dd=0.0;

  if(b.center.x<a.lower.x)
    dd+=sqr(b.center.x-a.lower.x);
  else if(b.center.x>a.upper.x)
    dd+=sqr(b.center.x-a.upper.x);

  if(b.center.y<a.lower.y)
    dd+=sqr(b.center.y-a.lower.y);
  else if(b.center.y>a.upper.y)
    dd+=sqr(b.center.y-a.upper.y);

  if(b.center.z<a.lower.z)
    dd+=sqr(b.center.z-a.lower.z);
  else if(b.center.z>a.upper.z)
    dd+=sqr(b.center.z-a.upper.z);

  return dd<=b.radius*b.radius;
  }


// Test if sphere overlaps with box
FXbool overlap(const FXSphered& a,const FXRanged& b){
  return overlap(b,a);
  }


// Test if spheres overlap
FXbool overlap(const FXSphered& a,const FXSphered& b){
  register FXdouble dx=a.center.x-b.center.x;
  register FXdouble dy=a.center.y-b.center.y;
  register FXdouble dz=a.center.z-b.center.z;
  return sqrt(dx*dx+dy*dy+dz*dz)<(a.radius+b.radius);
  }


// Saving
FXStream& operator<<(FXStream& store,const FXSphered& sphere){
  store << sphere.center << sphere.radius;
  return store;
  }


// Loading
FXStream& operator>>(FXStream& store,FXSphered& sphere){
  store >> sphere.center >> sphere.radius;
  return store;
  }

}
