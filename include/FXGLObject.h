/********************************************************************************
*                                                                               *
*                           O p e n G L   O b j e c t                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2006 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXGLObject.h,v 1.28 2006/01/22 17:58:03 fox Exp $                        *
********************************************************************************/
#ifndef FX_DISABLEGL

#ifndef FXGLOBJECT_H
#define FXGLOBJECT_H

#ifndef FXOBJECT_H
#include "FXObject.h"
#endif

namespace FX {


class FXGLViewer;
class FXGLObject;


// List of objects
typedef FXObjectListOf<FXGLObject> FXGLObjectList;

/// OpenGL color object
class FXAPI FXGLColor
{
public:
  FXfloat r,g,b,a;
public:
  // Constructors
  FXGLColor() : r(1), g(1), b(1), a(1) { }
  FXGLColor(FXfloat _r, FXfloat _g, FXfloat _b, FXfloat _a=1) : r(_r), g(_g), b(_b), a(_a) { }
  explicit FXGLColor(const FXfloat *colors) : r(colors ? colors[0] : 1), g(colors ? colors[1] : 1), b(colors ? colors[2] : 1), a(colors ? colors[3] : 1) { }
  FXGLColor(const FXColor &o) : r(FXREDVAL(o)/255.0f), g(FXGREENVAL(o)/255.0f), b(FXBLUEVAL(o)/255.0f), a(FXALPHAVAL(o)/255.0f) { }
  operator FXColor() const { return FXRGBA(((FXuchar)(r*255.0f) & 0xff), ((FXuchar)(g*255.0f) & 0xff), ((FXuchar)(b*255.0f) & 0xff), ((FXuchar)(a*255.0f) & 0xff)); }

  /// True if color is white
  bool isWhite() const { return 1==r && 1==g && 1==b; }

  /// True if color is black
  bool isBlack() const { return 0==r && 0==g && 0==b; }

  friend FXAPI FXStream &operator<<(FXStream &s, const FXGLColor &o);
  friend FXAPI FXStream &operator>>(FXStream &s, FXGLColor &o);
};


/// Basic OpenGL object
class FXAPI FXGLObject : public FXObject {
  FXDECLARE(FXGLObject)
public:
  enum {
    ID_LAST=10000       // Leaving ample room for FXGLViewer subclasses
    };
public:

  /// Constructors
  FXGLObject(){}

  /// Copy constructor
  FXGLObject(const FXGLObject& orig):FXObject(orig){}

  /// Called by the viewer to get bounds for this object
  virtual void bounds(FXRangef& box);

  /// Draw this object in a viewer
  virtual void draw(FXGLViewer* viewer);

  /// Draw this object for hit-testing purposes
  virtual void hit(FXGLViewer* viewer);

  /// Copy this object
  virtual FXGLObject* copy();

  /// Identify sub-object given path
  virtual FXGLObject* identify(FXuint* path);

  /// Return true if this object can be dragged around
  virtual FXbool canDrag() const;

  /// Return true if this object can be deleted from the scene
  virtual FXbool canDelete() const;

  /// Drag this object from one position to another
  virtual FXbool drag(FXGLViewer* viewer,FXint fx,FXint fy,FXint tx,FXint ty);

  /// Destructor
  virtual ~FXGLObject(){}
  };


// Suppress warning about FXGLGroup::list not being exported
#ifdef _MSC_VER
#if _MSC_VER >= 1200
#pragma warning( push )
#endif
#pragma warning( disable : 4251 )
#endif


/// Group object
class FXAPI FXGLGroup : public FXGLObject {
  FXDECLARE(FXGLGroup)
protected:
  FXGLObjectList list;    // List of all objects
public:

  /// Constructor
  FXGLGroup(){ }

  /// Copy constructor
  FXGLGroup(const FXGLGroup& orig):FXGLObject(orig),list(orig.list){ }

  /// Return list of childern
  FXGLObjectList& getList(){ return list; }

  /// Return bounding box
  virtual void bounds(FXRangef& box);

  /// Draw into viewer
  virtual void draw(FXGLViewer* viewer);

  /// Hit in viewer
  virtual void hit(FXGLViewer* viewer);

  /// Copy this object
  virtual FXGLObject* copy();

  /// Identify object by means of path
  virtual FXGLObject* identify(FXuint* path);

  /// Return TRUE if group can be dragged
  virtual FXbool canDrag() const;

  /// Drag group object
  virtual FXbool drag(FXGLViewer* viewer,FXint fx,FXint fy,FXint tx,FXint ty);

  /// Return number of children
  FXint no() const { return list.no(); }

  /// Child at position
  FXGLObject* child(FXint pos) const { return list[pos]; }

  /// Insert child object at given position
  void insert(FXint pos,FXGLObject* obj){ list.insert(pos,obj); }

  /// Prepend child object
  void prepend(FXGLObject* obj){ list.prepend(obj); }

  /// Append child object
  void append(FXGLObject* obj){ list.append(obj); }

  /// Replace child object
  void replace(FXint pos,FXGLObject* obj){ list.replace(pos,obj); }

  /// Remove child object
  void remove(FXGLObject* obj){ list.remove(obj); }

  /// Remove child object at given position
  void erase(FXint pos){ list.erase(pos); }

  /// Remove all children
  void clear(){ list.clear(); }

  /// Stream save and load
  virtual void save(FXStream& store) const;
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~FXGLGroup();
  };


// Restore previous warning levels
#ifdef _MSC_VER
#if _MSC_VER >= 1200
#pragma warning( pop )
#endif
#endif

}

#endif
#endif
