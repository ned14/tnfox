/********************************************************************************
*                                                                               *
*                          R e c t a n g l e    C l a s s                       *
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
* $Id: FXRectangle.h,v 1.12 2005/01/16 16:06:06 fox Exp $                       *
********************************************************************************/
#ifndef FXRECTANGLE_H
#define FXRECTANGLE_H


#include "FXPoint.h"

namespace FX {

/// Rectangle
class FXAPI FXRectangle {
public:
  FXshort x;
  FXshort y;
  FXshort w;
  FXshort h;
public:

  /// Constructors
  FXRectangle(){ }
  FXRectangle(FXshort xx,FXshort yy,FXshort ww,FXshort hh):x(xx),y(yy),w(ww),h(hh){ }
  FXRectangle(const FXPoint& p,const FXSize& s):x(p.x),y(p.y),w(s.w),h(s.h){ }
  FXRectangle(const FXPoint& topleft,const FXPoint& bottomright):x(topleft.x),y(topleft.y),w(bottomright.x-topleft.x+1),h(bottomright.y-topleft.y+1){ }

  /// Equality
  friend FXAPI FXbool operator==(const FXRectangle& p,const FXRectangle& q){ return p.x==q.x && p.y==q.y && p.w==q.w && p.h==q.h; }
  friend FXAPI FXbool operator!=(const FXRectangle& p,const FXRectangle& q){ return p.x!=q.x || p.y!=q.y || p.w!=q.w || p.h!=q.h; }

  /// Comparison
  friend FXAPI FXbool operator<(const FXRectangle& p,const FXRectangle& q){ return p.w*p.h<q.w*q.h; }
  friend FXAPI FXbool operator>(const FXRectangle& p,const FXRectangle& q){ return p.w*p.h>q.w*q.h; }

  /// Point in rectangle
  FXbool contains(const FXPoint& p) const { return x<=p.x && y<=p.y && p.x<x+w && p.y<y+h; }
  FXbool contains(FXshort xx,FXshort yy) const { return x<=xx && y<=yy && xx<x+w && yy<y+h; }

  /// Rectangle properly contained in rectangle
  FXbool contains(const FXRectangle& r) const { return x<=r.x && y<=r.y && r.x+r.w<=x+w && r.y+r.h<=y+h; }

  /// Rectangles overlap
  friend FXAPI FXbool overlap(const FXRectangle& a,const FXRectangle& b){ return b.x<a.x+a.w && b.y<a.y+a.h && a.x<b.x+b.w && a.y<b.y+b.h; }

  /// Return moved rectangle
  FXRectangle& move(FXshort dx,FXshort dy){ x+=dx; y+=dy; return *this; }

  /// Grow by amount
  FXRectangle& grow(FXshort margin);
  FXRectangle& grow(FXshort hormargin,FXshort vermargin);
  FXRectangle& grow(FXshort leftmargin,FXshort rightmargin,FXshort topmargin,FXshort bottommargin);

  /// Shrink by amount
  FXRectangle& shrink(FXshort margin);
  FXRectangle& shrink(FXshort hormargin,FXshort vermargin);
  FXRectangle& shrink(FXshort leftmargin,FXshort rightmargin,FXshort topmargin,FXshort bottommargin);

  /// Corners
  FXPoint tl() const { return FXPoint(x,y); }
  FXPoint tr() const { return FXPoint(x+w-1,y); }
  FXPoint bl() const { return FXPoint(x,y+h-1); }
  FXPoint br() const { return FXPoint(x+w-1,y+h-1); }

  /// Union and intersection with rectangle
  FXRectangle& operator+=(const FXRectangle &r);
  FXRectangle& operator*=(const FXRectangle &r);

  /// Union and intersection between rectangles
  friend FXAPI FXRectangle operator+(const FXRectangle& p,const FXRectangle& q);
  friend FXAPI FXRectangle operator*(const FXRectangle& p,const FXRectangle& q);

  /// Save object to a stream
  friend FXAPI FXStream& operator<<(FXStream& store,const FXRectangle& r);

  /// Load object from a stream
  friend FXAPI FXStream& operator>>(FXStream& store,FXRectangle& r);
  };

}

#endif
