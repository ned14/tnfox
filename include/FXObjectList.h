/********************************************************************************
*                                                                               *
*                            O b j e c t   L i s t                              *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXObjectList.h,v 1.27.2.1 2005/02/11 01:02:47 fox Exp $                      *
********************************************************************************/
#ifndef FXOBJECTLIST_H
#define FXOBJECTLIST_H

#ifndef FXOBJECT_H
#include "FXObject.h"
#endif

namespace FX {

/// List of pointers to objects
class FXAPI FXObjectList {
protected:
  FXObject **data;
public:

  /// Default constructor
  FXObjectList();

  /// Copy constructor
  FXObjectList(const FXObjectList& orig);

  /// Construct and init with single object
  FXObjectList(FXObject* object);

  /// Construct and init with list of objects
  FXObjectList(FXObject** objects,FXint n);

  /// Assignment operator
  FXObjectList& operator=(const FXObjectList& orig);

  /// Return number of objects
  FXint no() const { return *((FXint*)(data-1)); }

  /// Set number of objects
  void no(FXint num);

  /// Indexing operator
  FXObject*& operator[](FXint i){ return data[i]; }
  FXObject* const& operator[](FXint i) const { return data[i]; }

  /// Access to list
  FXObject*& list(FXint i){ return data[i]; }
  FXObject* const& list(FXint i) const { return data[i]; }

  /// Access to content array
  FXObject** list() const { return data; }

  /// Assign object p to list
  FXObjectList& assign(FXObject* object);

  /// Assign n objects to list
  FXObjectList& assign(FXObject** objects,FXint n);

  /// Assign objects to list
  FXObjectList& assign(FXObjectList& objects);

  /// Insert object at certain position
  FXObjectList& insert(FXint pos,FXObject* object);

  /// Insert n objects at specified position
  FXObjectList& insert(FXint pos,FXObject** objects,FXint n);

  /// Insert objects at specified position
  FXObjectList& insert(FXint pos,FXObjectList& objects);

  /// Prepend object
  FXObjectList& prepend(FXObject* object);

  /// Prepend n objects
  FXObjectList& prepend(FXObject** objects,FXint n);

  /// Prepend objects
  FXObjectList& prepend(FXObjectList& objects);

  /// Append object
  FXObjectList& append(FXObject* object);

  /// Append n objects
  FXObjectList& append(FXObject** objects,FXint n);

  /// Append objects
  FXObjectList& append(FXObjectList& objects);

  /// Replace object at position by given object
  FXObjectList& replace(FXint pos,FXObject* object);

  /// Replaces the m objects at pos with n objects
  FXObjectList& replace(FXint pos,FXint m,FXObject** objects,FXint n);

  /// Replace the m objects at pos with objects
  FXObjectList& replace(FXint pos,FXint m,FXObjectList& objects);

  /// Remove object at pos
  FXObjectList& remove(FXint pos,FXint n=1);

  /// Remove object
  FXObjectList& remove(const FXObject* object);

  /// Find object in list, searching forward; return position or -1
  FXint find(const FXObject *object,FXint pos=0) const;

  /// Find object in list, searching backward; return position or -1
  FXint rfind(const FXObject *object,FXint pos=2147483647) const;

  /// Remove all objects
  FXObjectList& clear();

  /// Save to a stream
  void save(FXStream& store) const;

  /// Load from a stream
  void load(FXStream& store);

  /// Destructor
  virtual ~FXObjectList();
  };


/// Specialize list to pointers to TYPE
template<class TYPE>
class FXAPI FXObjectListOf : public FXObjectList {
public:
  FXObjectListOf(){}

  /// Indexing operator
  TYPE*& operator[](FXint i){ return (TYPE*&)data[i]; }
  TYPE *const& operator[](FXint i) const { return (TYPE*const&)data[i]; }

  /// Access to list
  TYPE*& list(FXint i){ return (TYPE*&)data[i]; }
  TYPE *const& list(FXint i) const { return (TYPE*const&)data[i]; }

  /// Access to content array
  TYPE** list() const { return (TYPE**)data; }
  };

}

#endif
