/********************************************************************************
*                                                                               *
*           S i n g l e - P r e c i s i o n    S p h e r e    C l a s s         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXSpheref.h,v 1.10.2.1 2006/03/21 07:08:29 fox Exp $                          *
********************************************************************************/
#ifndef FXSPHEREF_H
#define FXSPHEREF_H

#include "FXVec3f.h"

namespace FX {

class FXRangef;
class FXVec4f;


/// Spherical bounds
class FXAPI FXSpheref {
public:
  FXVec3f center;
  FXfloat radius;
public:

  /// Default constructor
  FXSpheref(){}

  /// Copy constructor
  FXSpheref(const FXSpheref& sphere):center(sphere.center),radius(sphere.radius){}

  /// Initialize from center and radius
  FXSpheref(const FXVec3f& cen,FXfloat rad=0.0f):center(cen),radius(rad){}

  /// Initialize from center and radius
  FXSpheref(FXfloat x,FXfloat y,FXfloat z,FXfloat rad=0.0f):center(x,y,z),radius(rad){}

  /// Initialize sphere to fully contain the given bounding box
  FXSpheref(const FXRangef& bounds);

  /// Assignment
  FXSpheref& operator=(const FXSpheref& sphere){ center=sphere.center; radius=sphere.radius; return *this; }

  /// Diameter of sphere
  FXfloat diameter() const { return radius*2.0f; }

  /// Test if empty
  FXbool empty() const { return radius<0.0f; }

  /// Test if sphere contains point x,y,z
  FXbool contains(FXfloat x,FXfloat y,FXfloat z) const;

  /// Test if sphere contains point p
  FXbool contains(const FXVec3f& p) const;

  /// Test if sphere properly contains another box
  FXbool contains(const FXRangef& box) const;

  /// Test if sphere properly contains another sphere
  FXbool contains(const FXSpheref& sphere) const;

  /// Include point
  FXSpheref& include(FXfloat x,FXfloat y,FXfloat z);

  /// Include point
  FXSpheref& include(const FXVec3f& p);

  /// Include given range into this one
  FXSpheref& include(const FXRangef& box);

  /// Include given sphere into this one
  FXSpheref& include(const FXSpheref& sphere);

  /// Intersect sphere with normalized plane ax+by+cz+w; returns -1,0,+1
  FXint intersect(const FXVec4f& plane) const;

  /// Intersect sphere with ray u-v
  FXbool intersect(const FXVec3f& u,const FXVec3f& v) const;

  /// Test if box overlaps with sphere
  friend FXAPI FXbool overlap(const FXRangef& a,const FXSpheref& b);

  /// Test if sphere overlaps with box
  friend FXAPI FXbool overlap(const FXSpheref& a,const FXRangef& b);

  /// Test if spheres overlap
  friend FXAPI FXbool overlap(const FXSpheref& a,const FXSpheref& b);

  /// Save object to a stream
  friend FXAPI FXStream& operator<<(FXStream& store,const FXSpheref& sphere);

  /// Load object from a stream
  friend FXAPI FXStream& operator>>(FXStream& store,FXSpheref& sphere);
  };


extern FXAPI FXbool overlap(const FXRangef& a,const FXSpheref& b);
extern FXAPI FXbool overlap(const FXSpheref& a,const FXRangef& b);
extern FXAPI FXbool overlap(const FXSpheref& a,const FXSpheref& b);

extern FXAPI FXStream& operator<<(FXStream& store,const FXSpheref& sphere);
extern FXAPI FXStream& operator>>(FXStream& store,FXSpheref& sphere);

}

#endif
