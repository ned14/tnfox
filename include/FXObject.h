/********************************************************************************
*                                                                               *
*                         T o p l e v el   O b j e c t                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXObject.h,v 1.25 2004/02/08 17:17:34 fox Exp $                          *
********************************************************************************/
#ifndef FXOBJECT_H
#define FXOBJECT_H

#include "FXStream.h"
#include "FXTrans.h"

namespace FX {

/// Minimum and maximum message id
enum {
  MINKEY = 0,
  MAXKEY = 65535
  };


/// Minimum and maximum message type
enum {
  MINTYPE = 0,
  MAXTYPE = 65535
  };


// Association key
typedef FXuint FXSelector;


// Forward
class FXObject;


/// Describes a FOX object
class FXAPI FXMetaClass {
private:
  const FXchar              *className;
  FXObject*                (*manufacture)();
  const FXMetaClass         *baseClass;
  const void                *assoc;
  FXuint                     nassocs;
  FXuint                     assocsz;
  FXuint                     namelen;
private:
  static const FXMetaClass **metaClassTable;
  static FXuint              nmetaClassTable;
  static FXuint              nmetaClasses;
private:
  static void resize(FXuint n);
public:
  FXMetaClass(const FXchar* name,FXObject *(fac)(),const FXMetaClass* base,const void* ass,FXuint nass,FXuint assz,FXuint len);

  /// Check if metaclass is subclass of some other metaclass
  FXbool isSubClassOf(const FXMetaClass* metaclass) const;

  /// Make instance of some object
  FXObject* makeInstance() const;

  /// Ask class name
  const FXchar* getClassName() const { return className; }

  /// Obtain class name length
  FXuint getClassNameLength() const { return namelen; }

  /// Ask base class
  const FXMetaClass* getBaseClass() const { return baseClass; }

  /// Find metaclass object
  static const FXMetaClass* getMetaClassFromName(const FXchar* name);

  /// Search message map
  const void* search(FXSelector key) const;

 ~FXMetaClass();
  };


/// Macro to set up class declaration
#define FXDECLARE(classname) \
  public: \
   struct FXMapEntry { FX::FXSelector keylo; FX::FXSelector keyhi; long (classname::* func)(FX::FXObject*,FX::FXSelector,void*); }; \
   static const FX::FXMetaClass metaClass; \
   static FX::FXObject* manufacture(); \
   virtual long handle(FX::FXObject* sender,FX::FXSelector sel,void* ptr); \
   virtual const FX::FXMetaClass* getMetaClass() const { return &metaClass; } \
   friend FX::FXStream& operator<<(FX::FXStream& store,const classname* obj){return store.saveObject((FX::FXObjectPtr)(obj));} \
   friend FX::FXStream& operator>>(FX::FXStream& store,classname*& obj){return store.loadObject((FX::FXObjectPtr&)(obj));} \
  private:


/// Macro to set up class implementation
#define FXIMPLEMENT(classname,baseclassname,mapping,nmappings) \
  FX::FXObject* classname::manufacture(){return new classname;} \
  const FX::FXMetaClass classname::metaClass(#classname,classname::manufacture,&baseclassname::metaClass,mapping,nmappings,sizeof(classname::FXMapEntry),sizeof(#classname)); \
  long classname::handle(FX::FXObject* sender,FX::FXSelector sel,void* ptr){ \
    const FXMapEntry* me=(const FXMapEntry*)metaClass.search(sel); \
    return me ? (this->* me->func)(sender,sel,ptr) : baseclassname::handle(sender,sel,ptr); \
    }


/// Macro to set up abstract class declaration
#define FXDECLARE_ABSTRACT(classname) \
  public: \
   struct FXMapEntry { FX::FXSelector keylo; FX::FXSelector keyhi; long (classname::* func)(FX::FXObject*,FX::FXSelector,void*); }; \
   static const FX::FXMetaClass metaClass; \
   virtual long handle(FX::FXObject* sender,FX::FXSelector sel,void* ptr); \
   virtual const FX::FXMetaClass* getMetaClass() const { return &metaClass; } \
   friend FX::FXStream& operator<<(FX::FXStream& store,const classname* obj){return store.saveObject((FX::FXObjectPtr)(obj));} \
   friend FX::FXStream& operator>>(FX::FXStream& store,classname*& obj){return store.loadObject((FX::FXObjectPtr&)(obj));} \
  private:


/// Macro to set up abstract class implementation
#define FXIMPLEMENT_ABSTRACT(classname,baseclassname,mapping,nmappings) \
  const FX::FXMetaClass classname::metaClass(#classname,NULL,&baseclassname::metaClass,mapping,nmappings,sizeof(classname::FXMapEntry),sizeof(#classname)); \
  long classname::handle(FX::FXObject* sender,FX::FXSelector sel,void* ptr){ \
    const FXMapEntry* me=(const FXMapEntry*)metaClass.search(sel); \
    return me ? (this->* me->func)(sender,sel,ptr) : baseclassname::handle(sender,sel,ptr); \
    }


/// MetaClass of a class
#define FXMETACLASS(classname) (&classname::metaClass)


/// Set up map type
#define FXDEFMAP(classname) static const classname::FXMapEntry

/// Define range of function types
#define FXMAPTYPES(typelo,typehi,func) {MKUINT(MINKEY,typelo),MKUINT(MAXKEY,typehi),&func}

/// Define range of function types
#define FXMAPTYPE(type,func) {MKUINT(MINKEY,type),MKUINT(MAXKEY,type),&func}

/// Define range of functions
#define FXMAPFUNCS(type,keylo,keyhi,func) {MKUINT(keylo,type),MKUINT(keyhi,type),&func}

/// Define one function
#define FXMAPFUNC(type,key,func) {MKUINT(key,type),MKUINT(key,type),&func}


/// Base of all FOX object
class FXAPI FXObject {
  FXDECLARE(FXObject)
public:

  /// Called for unhandled messages
  virtual long onDefault(FXObject*,FXSelector,void*);

public:

  /// Get class name of some object
  const FXchar* getClassName() const;

  /// Check if object is member of metaclass
  FXbool isMemberOf(const FXMetaClass* metaclass) const;

  /// Save object to stream
  virtual void save(FXStream& store) const;

  /// Load object from stream
  virtual void load(FXStream& store);

  /// Returns a human translated version of the text to the locale language
  /// (warning: uses a virtual method call, so can't use before construction)
  FXTransString tr(const char *text, const char *hint=0);

  /// Virtual destructor
  virtual ~FXObject();
  };

}

#endif
