/********************************************************************************
*                                                                               *
*                   A c c e l e r a t o r   T a b l e   C l a s s               *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXAccelTable.cpp,v 1.35 2005/01/16 16:06:06 fox Exp $                    *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "QThread.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXObject.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXApp.h"
#include "FXAccelTable.h"


/*
  Notes:
  - Mostly complete.
*/

#define EMPTYSLOT       0xfffffffe   // Previously used, now empty slot
#define UNUSEDSLOT      0xffffffff   // Unsused slot marker




/*******************************************************************************/

namespace FX {


// Map
FXDEFMAP(FXAccelTable) FXAccelTableMap[]={
  FXMAPFUNC(SEL_KEYPRESS,0,FXAccelTable::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,FXAccelTable::onKeyRelease),
  };


// Object implementation
FXIMPLEMENT(FXAccelTable,FXObject,FXAccelTableMap,ARRAYNUMBER(FXAccelTableMap))


// Make empty accelerator table
FXAccelTable::FXAccelTable(){
  FXTRACE((100,"%p->FXAccelTable::FXAccelTable\n",this));
  FXMALLOC(&key,FXAccelKey,1);
  key[0].code=UNUSEDSLOT;
  key[0].target=NULL;
  key[0].messagedn=0;
  key[0].messageup=0;
  max=0;
  num=0;
  }


// Resize hash table, and rehash old stuff into it
void FXAccelTable::resize(FXuint m){
  register FXuint p,i,c;
  FXAccelKey *newkey;
  FXMALLOC(&newkey,FXAccelKey,m+1);
  for(i=0; i<=m; i++){
    newkey[i].code=UNUSEDSLOT;
    newkey[i].target=NULL;
    newkey[i].messagedn=0;
    newkey[i].messageup=0;
    }
  for(i=0; i<=max; i++){
    if((c=key[i].code)>=EMPTYSLOT) continue;
    p=(c*13)&m;
    while(newkey[p].code!=UNUSEDSLOT) p=(p+1)&m;
    newkey[p]=key[i];
    }
  FXFREE(&key);
  key=newkey;
  max=m;
  }


// Add (or replace) accelerator
void FXAccelTable::addAccel(FXHotKey hotkey,FXObject* target,FXSelector seldn,FXSelector selup){
  if(hotkey){
    FXTRACE((150,"%p->FXAccelTable::addAccel: code=%04x state=%04x\n",this,(FXushort)hotkey,(FXushort)(hotkey>>16)));
    register FXuint p=(hotkey*13)&max;
    register FXuint c;
    FXASSERT(hotkey!=UNUSEDSLOT);
    FXASSERT(hotkey!=EMPTYSLOT);
    while((c=key[p].code)!=UNUSEDSLOT){ // Check if in table already
      if(c==hotkey) goto x;
      p=(p+1)&max;
      }
    ++num;
    if(max<(num<<1)) resize((max<<1)+1);
    FXASSERT(num<=max);
    p=(hotkey*13)&max;                  // Locate first unused or empty slot
    while((c=key[p].code)<EMPTYSLOT){
      p=(p+1)&max;
      }
x:  key[p].code=hotkey;                 // Add or replace accelerator info
    key[p].target=target;
    key[p].messagedn=seldn;
    key[p].messageup=selup;
    }
  }


// Remove accelerator.
// When removed, the slot may still be in a chain of probe
// positions, unless it is demonstrably the last item in a chain.
void FXAccelTable::removeAccel(FXHotKey hotkey){
  if(hotkey){
    FXTRACE((150,"%p->FXAccelTable::removeAccel: code=%04x state=%04x\n",this,(FXushort)hotkey,(FXushort)(hotkey>>16)));
    register FXuint p=(hotkey*13)&max;
    register FXuint c;
    FXASSERT(hotkey!=UNUSEDSLOT);
    FXASSERT(hotkey!=EMPTYSLOT);
    while((c=key[p].code)!=hotkey){
      if(c==UNUSEDSLOT) return;
      p=(p+1)&max;
      }
    if(key[(p+1)&max].code==UNUSEDSLOT){// Last in chain
      key[p].code=UNUSEDSLOT;
      }
    else{                               // Middle of chain
      key[p].code=EMPTYSLOT;
      }
    key[p].target=NULL;
    key[p].messagedn=0;
    key[p].messageup=0;
    if(max>=(num<<2)) resize(max>>1);
    --num;
    FXASSERT(num<=max);
    }
  }


// See if accelerator exists
FXbool FXAccelTable::hasAccel(FXHotKey hotkey) const {
  if(hotkey){
    register FXuint p=(hotkey*13)&max;
    register FXuint c;
    FXASSERT(hotkey!=UNUSEDSLOT);
    FXASSERT(hotkey!=EMPTYSLOT);
    while((c=key[p].code)!=hotkey){
      if(c==UNUSEDSLOT) return FALSE;
      p=(p+1)&max;
      }
    return TRUE;
    }
  return FALSE;
  }


// Return target object of the given accelerator
FXObject* FXAccelTable::targetOfAccel(FXHotKey hotkey) const {
  if(hotkey){
    register FXuint p=(hotkey*13)&max;
    register FXuint c;
    FXASSERT(hotkey!=UNUSEDSLOT);
    FXASSERT(hotkey!=EMPTYSLOT);
    while((c=key[p].code)!=hotkey){
      if(c==UNUSEDSLOT) return NULL;
      p=(p+1)&max;
      }
    return key[p].target;
    }
  return NULL;
  }


// Keyboard press; forward to accelerator target
long FXAccelTable::onKeyPress(FXObject* sender,FXSelector,void* ptr){
  FXTRACE((200,"%p->FXAccelTable::onKeyPress keysym=0x%04x state=%04x\n",this,((FXEvent*)ptr)->code,((FXEvent*)ptr)->state));
  register FXEvent* event=(FXEvent*)ptr;
  register FXuint code=MKUINT(event->code,event->state&(SHIFTMASK|CONTROLMASK|ALTMASK|METAMASK));
  register FXuint p=(code*13)&max;
  register FXuint c;
  FXASSERT(code!=UNUSEDSLOT);
  FXASSERT(code!=EMPTYSLOT);
  while((c=key[p].code)!=code){
    if(c==UNUSEDSLOT) return 0;
    p=(p+1)&max;
    }
  if(key[p].target && key[p].messagedn){
    key[p].target->tryHandle(sender,key[p].messagedn,ptr);
    }
  return 1;
  }


// Keyboard release; forward to accelerator target
long FXAccelTable::onKeyRelease(FXObject* sender,FXSelector,void* ptr){
  FXTRACE((200,"%p->FXAccelTable::onKeyRelease keysym=0x%04x state=%04x\n",this,((FXEvent*)ptr)->code,((FXEvent*)ptr)->state));
  register FXEvent* event=(FXEvent*)ptr;
  register FXuint code=MKUINT(event->code,event->state&(SHIFTMASK|CONTROLMASK|ALTMASK|METAMASK));
  register FXuint p=(code*13)&max;
  register FXuint c;
  FXASSERT(code!=UNUSEDSLOT);
  FXASSERT(code!=EMPTYSLOT);
  while((c=key[p].code)!=code){
    if(c==UNUSEDSLOT) return 0;
    p=(p+1)&max;
    }
  if(key[p].target && key[p].messageup){
    key[p].target->tryHandle(sender,key[p].messageup,ptr);
    }
  return 1;
  }


// Save data
void FXAccelTable::save(FXStream& store) const {
  register FXuint i;
  FXObject::save(store);
  store << max;
  store << num;
  for(i=0; i<=max; i++){
    store << key[i].target;
    store << key[i].messagedn;
    store << key[i].messageup;
    store << key[i].code;
    }
  }


// Load data
void FXAccelTable::load(FXStream& store){
  register FXuint i;
  FXObject::load(store);
  store >> max;
  store >> num;
  FXRESIZE(&key,FXAccelKey,max+1);
  for(i=0; i<=max; i++){
    store >> key[i].target;
    store >> key[i].messagedn;
    store >> key[i].messageup;
    store >> key[i].code;
    }
  }


// Destroy table
FXAccelTable::~FXAccelTable(){
  FXTRACE((100,"%p->FXAccelTable::~FXAccelTable\n",this));
  FXFREE(&key);
  key=(FXAccelKey*)-1L;
  }

}
