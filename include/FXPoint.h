/********************************************************************************
*                                                                               *
*                             P o i n t    C l a s s                            *
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
* $Id: FXPoint.h,v 1.8 2005/01/16 16:06:06 fox Exp $                            *
********************************************************************************/
#ifndef FXPOINT_H
#define FXPOINT_H

#include "FXSize.h"

namespace FX {

/// Point
class FXAPI FXPoint {
public:
  FXshort x;
  FXshort y;
public:

  /// Constructors
  FXPoint(){ }
  FXPoint(const FXSize& s):x(s.w),y(s.h){ }
  FXPoint(const FXPoint& p):x(p.x),y(p.y){ }
  FXPoint(FXshort xx,FXshort yy):x(xx),y(yy){ }

  /// Equality
  friend FXAPI FXbool operator==(const FXPoint& p,const FXPoint& q){ return p.x==q.x && p.y==q.y; }
  friend FXAPI FXbool operator!=(const FXPoint& p,const FXPoint& q){ return p.x!=q.x || p.y!=q.y; }

  /// Assignment
  FXPoint& operator=(const FXPoint& p){ x=p.x; y=p.y; return *this; }
  FXPoint& operator=(const FXSize& s){ x=s.w; y=s.h; return *this; }

  /// Assignment operators
  FXPoint& operator+=(const FXPoint& p){ x+=p.x; y+=p.y; return *this; }
  FXPoint& operator+=(const FXSize& s){ x+=s.w; y+=s.h; return *this; }
  FXPoint& operator-=(const FXPoint& p){ x-=p.x; y-=p.y; return *this; }
  FXPoint& operator-=(const FXSize& s){ x-=s.w; y-=s.h; return *this; }
  FXPoint& operator*=(FXshort c){ x*=c; y*=c; return *this; }
  FXPoint& operator/=(FXshort c){ x/=c; y/=c; return *this; }

  /// Negation
  FXPoint operator-(){ return FXPoint(-x,-y); }

  /// Other operators
  friend FXAPI FXPoint operator+(const FXPoint& p,const FXPoint& q){ return FXPoint(p.x+q.x,p.y+q.y); }
  friend FXAPI FXPoint operator+(const FXPoint& p,const FXSize& s){ return FXPoint(p.x+s.w,p.y+s.h); }
  friend FXAPI FXPoint operator+(const FXSize& s,const FXPoint& p){ return FXPoint(s.w+p.x,s.h+p.y); }

  friend FXAPI FXPoint operator-(const FXPoint& p,const FXPoint& q){ return FXPoint(p.x-q.x,p.y-q.y); }
  friend FXAPI FXPoint operator-(const FXPoint& p,const FXSize& s){ return FXPoint(p.x-s.w,p.y-s.h); }
  friend FXAPI FXPoint operator-(const FXSize& s,const FXPoint& p){ return FXPoint(s.w-p.x,s.h-p.y); }

  friend FXAPI FXPoint operator*(const FXPoint& p,FXshort c){ return FXPoint(p.x*c,p.y*c); }
  friend FXAPI FXPoint operator*(FXshort c,const FXPoint& p){ return FXPoint(c*p.x,c*p.y); }
  friend FXAPI FXPoint operator/(const FXPoint& p,FXshort c){ return FXPoint(p.x/c,p.y/c); }
  friend FXAPI FXPoint operator/(FXshort c,const FXPoint& p){ return FXPoint(c/p.x,c/p.y); }

  /// Save object to a stream
  friend FXAPI FXStream& operator<<(FXStream& store,const FXPoint& p);

  /// Load object from a stream
  friend FXAPI FXStream& operator>>(FXStream& store,FXPoint& p);
  };

}

#endif

