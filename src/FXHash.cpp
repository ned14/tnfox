/********************************************************************************
*                                                                               *
*                       H a s h   T a b l e   C l a s s                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXHash.cpp,v 1.13 2004/02/08 17:29:06 fox Exp $                          *
********************************************************************************/
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"


/*
  Notes:
  - The members used and free keep track of the number of slots
    in the table which are used and which are free.
  - When an item is inserted, used is incremented if the item isn't in the table
    yet, and free is decremented if a free slot is used; if an empty slot is
    used, free stays the same.  If the table exceeds the load factor, its
    size is doubled.
  - When an item is removed, used is decremented but free stays the same
    because the slot remains marked as empty instead of free; when the
    number of used items drops below some minimum, the table's size is
    halved.
  - If the table is resized, the empty slots all become free slots since
    the empty holes are not compied into the table.  All used items will
    be rehashed into the new table.
*/

#define HASH1(x,m) (((FXuint)((FXuval)(key)^(((FXuval)(key))>>13)))&(m))
#define HASH2(x,m) (((FXuint)((FXuval)(key)^(((FXuval)(key))>>17)|1))&(m))





/*******************************************************************************/

namespace FX {

// Make empty table
FXHash::FXHash():used(0),free(2),max(1){
  FXCALLOC(&table,FXEntry,2);
  }


// Resize hash table, and rehash old stuff into it
void FXHash::resize(FXuint m){
  register void *key,*val;
  register FXuint p,x,i;
  FXEntry *newtable;
  FXCALLOC(&newtable,FXEntry,m+1);
  for(i=0; i<=max; i++){
    key=table[i].key;
    val=table[i].val;
    if(key==NULL || key==(void*)-1L) continue;
    p=HASH1(key,m);
    x=HASH2(key,m);
    while(newtable[p].key) p=(p+x)&m;
    newtable[p].key=key;
    newtable[p].val=val;
    }
  FXFREE(&table);
  table=newtable;
  free=m+1-used;
  max=m;
  }


// Insert or replace association into the table
void* FXHash::insert(void* key,void* val){
  register FXuint p,pp,x,xx;
  if(key){
    if((free<<1)<=max) resize((max<<1)|1);
    p=pp=HASH1(key,max);
    x=xx=HASH2(key,max);
    while(table[p].key){
      if(table[p].key==key) goto y;             // Replace existing
      p=(p+x)&max;
      }
    p=pp;
    x=xx;
    while(table[p].key){
      if(table[p].key==(void*)-1L) goto x;      // Put it in empty slot
      p=(p+x)&max;
      }
    free--;
x:  used++;
y:  table[p].key=key;
    table[p].val=val;
    return val;
    }
  return NULL;
  }


// Remove association from the table
void* FXHash::remove(void* key){
  register FXuint p,x;
  register void* val;
  if(key){
    p=HASH1(key,max);
    x=HASH2(key,max);
    while(table[p].key!=key){
      if(table[p].key==NULL) goto x;
      p=(p+x)&max;
      }
    val=table[p].val;
    table[p].key=(void*)-1L;                    // Empty but not free
    table[p].val=NULL;
    used--;
    if((used<<2)<=max) resize(max>>1);
    return val;
    }
x:return NULL;
  }


// Return true if association in table
void* FXHash::find(void* key) const {
  register FXuint p,x;
  if(key){
    p=HASH1(key,max);
    x=HASH2(key,max);
    while(table[p].key!=key){
      if(table[p].key==NULL) goto x;
      p=(p+x)&max;
      }
    return table[p].val;
    }
x:return NULL;
  }


// Destroy table
FXHash::~FXHash(){
  FXFREE(&table);
  table=(FXEntry*)-1L;
  }

}

