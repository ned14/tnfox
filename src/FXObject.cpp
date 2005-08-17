/********************************************************************************
*                                                                               *
*                         T o p l e v el   O b j e c t                          *
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
* $Id: FXObject.cpp,v 1.40 2005/02/06 17:20:00 fox Exp $                        *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXObject.h"
#include "FXException.h"
#include "FXString.h"
#include "QTrans.h"


/*
  Notes:

  - We need a table of all metaclasses, as we should be able to create any type
    of object during deserialization.
*/


#define EMPTYSLOT  ((FXMetaClass*)-1L)


namespace FX {


/*************************  FXMetaClass Implementation  ************************/

// Hash table of metaclasses
const FXMetaClass** FXMetaClass::metaClassTable=NULL;
FXuint              FXMetaClass::nmetaClassTable=0;
FXuint              FXMetaClass::nmetaClasses=0;


// Hash function for string
static inline FXuint hashstring(const FXchar* str){
  register const FXuchar *s=(const FXuchar*)str;
  register FXuint h=0;
  register FXuint c;
  while((c=*s++)!='\0'){
    h = ((h << 5) + h) ^ c;
    }
  return h;
  }


// Constructor adds metaclass to the table
FXMetaClass::FXMetaClass(const FXchar* name,FXObject *(fac)(),const FXMetaClass* base,const void* ass,FXuint nass,FXuint assz):
  className(name),manufacture(fac),baseClass(base),assoc(ass),nassocs(nass),assocsz(assz){
  register FXuint p,x,m;

  // Adding one
  ++nmetaClasses;

  // Table is almost full?
  if(nmetaClassTable < (nmetaClasses<<1)){
    resize(nmetaClassTable?nmetaClassTable<<1:1);
    }

  // Should always be maintained
  FXASSERT(nmetaClassTable>=nmetaClasses);

  // Find hash slot
  p=hashstring(className);
  x=(p<<1)|1;
  m=nmetaClassTable-1;
  while(1){
    p=(p+x)&m;
    if(metaClassTable[p]==0) break;
    }

  // Place in table
  metaClassTable[p]=this;
  }


// Find the FXMetaClass belonging to class name
const FXMetaClass* FXMetaClass::getMetaClassFromName(const FXchar* name){
  if(nmetaClassTable){
    register FXuint p,x,m;
    p=hashstring(name);
    x=(p<<1)|1;
    m=nmetaClassTable-1;
    while(1){
      p=(p+x)&m;
      if(metaClassTable[p]==0) break;
      if(metaClassTable[p]!=EMPTYSLOT && strcmp(metaClassTable[p]->className,name)==0){
        return metaClassTable[p];
        }
      }
    }
  return NULL;
  }


// Test if subclass
FXbool FXMetaClass::isSubClassOf(const FXMetaClass* metaclass) const {
  register const FXMetaClass* cls;
  for(cls=this; cls; cls=cls->baseClass){
    if(cls==metaclass) return TRUE;
    }
  return FALSE;
  }


// Create an object instance
FXObject* FXMetaClass::makeInstance() const {
  return (*manufacture)();
  }


// Find function
const void* FXMetaClass::search(FXSelector key) const {
  register const FXObject::FXMapEntry* lst=(const FXObject::FXMapEntry*)assoc;
  register FXuint n=nassocs;
  while(n--){
    if(lst->keylo<=key && key<=lst->keyhi) return lst;
    lst=(const FXObject::FXMapEntry*) (((const FXchar*)lst)+assocsz);
    }
  return NULL;
  }


// Destructor removes metaclass from the table
FXMetaClass::~FXMetaClass(){
  register FXuint p,x,m;

  // Find hash slot
  p=hashstring(className);
  x=(p<<1)|1;
  m=nmetaClassTable-1;
  while(1){
    p=(p+x)&m;
    if(metaClassTable[p]==this) break;
    }

  // Remove from table
  metaClassTable[p]=EMPTYSLOT;

  // Table is empty?
  --nmetaClasses;

  // Table is almost empty?
  if(nmetaClassTable >= (nmetaClasses<<1)){
    resize(nmetaClassTable>>1);
    }

  // Should always be maintained
  FXASSERT(nmetaClassTable>=nmetaClasses);
  }


// Resize global hash table
void FXMetaClass::resize(FXuint n){
  const FXMetaClass **newtable,*ptr;
  register FXuint p,x,i,m;
  FXCALLOC(&newtable,FXMetaClass*,n);
  for(i=0; i<nmetaClassTable; i++){
    ptr=metaClassTable[i];
    if(ptr && ptr!=EMPTYSLOT){
      p=hashstring(ptr->className);
      x=(p<<1)|1;
      m=n-1;
      while(1){
        p=(p+x)&m;
        if(newtable[p]==NULL) break;
        }
      newtable[p]=ptr;
      }
    }
  FXFREE(&metaClassTable);
  metaClassTable=newtable;
  nmetaClassTable=n;
  }


/***************************  FXObject Implementation  *************************/

// Have to do this one `by hand' as it has no base class
const FXMetaClass FXObject::metaClass("FXObject",FXObject::manufacture,NULL,NULL,0,0);


// Build an object
FXObject* FXObject::manufacture(){return new FXObject;}


// Get class name of object
const FXchar* FXObject::getClassName() const { return getMetaClass()->getClassName(); }


// Check if object belongs to a class
FXbool FXObject::isMemberOf(const FXMetaClass* metaclass) const {
  return getMetaClass()->isSubClassOf(metaclass);
  }


// Try handle message safely; we catch only resource exceptions, things like 
// running out of memory, window handles, system resources; others are ignored.
long FXObject::tryHandle(FXObject* sender,FXSelector sel,void* ptr){
  try { return handle(sender,sel,ptr); } catch(const FXResourceException&) { return 0; }
  }


// Save to stream
void FXObject::save(FXStream&) const { }


// Load from stream
void FXObject::load(FXStream&){ }


// Unhandled function
long FXObject::onDefault(FXObject*,FXSelector,void*){ return 0; }


// Handle message
long FXObject::handle(FXObject* sender,FXSelector sel,void* ptr){
  return onDefault(sender,sel,ptr);
  }

QTransString FXObject::tr(const char *text, const char *hint)
{
	return QTrans::tr(getMetaClass()->getClassName(), text, hint);
}


// This really messes the object up; note that it is intentional,
// as further references to a destructed object should not happen.
FXObject::~FXObject(){*((void**)this)=(void*)-1L;}

}
