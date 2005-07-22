/********************************************************************************
*                                                                               *
*                       F o u r - W a y   S p l i t t e r                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FX4Splitter.cpp,v 1.43 2005/01/16 16:06:06 fox Exp $                     *
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
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXApp.h"
#include "FXDCWindow.h"
#include "FX4Splitter.h"

/*
  Notes:
  - 4Splitter always splits into four partitions.
  - 4Splitter determines pane sizes by split fraction, i.e. if 4Splitter
    resizes, each sub pane gets proportionally resized also.
  - Should we send SEL_CHANGED and SEL_COMMAND also when splitter arrangement
    was changed programmatically?
  - If we're just re-sizing a split, do we need to incur a GUI-Update?
*/


// Splitter styles
#define FOURSPLITTER_MASK     FOURSPLITTER_TRACKING

// Fudge
#define FUDGE    10

// Modes
#define NOWHERE      0
#define ONVERTICAL   1
#define ONHORIZONTAL 2
#define ONCENTER     (ONVERTICAL|ONHORIZONTAL)





/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FX4Splitter) FX4SplitterMap[]={
  FXMAPFUNC(SEL_MOTION,0,FX4Splitter::onMotion),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FX4Splitter::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FX4Splitter::onLeftBtnRelease),
  FXMAPFUNC(SEL_FOCUS_UP,0,FX4Splitter::onFocusUp),
  FXMAPFUNC(SEL_FOCUS_DOWN,0,FX4Splitter::onFocusDown),
  FXMAPFUNC(SEL_FOCUS_LEFT,0,FX4Splitter::onFocusLeft),
  FXMAPFUNC(SEL_FOCUS_RIGHT,0,FX4Splitter::onFocusRight),
  FXMAPFUNCS(SEL_UPDATE,FX4Splitter::ID_EXPAND_ALL,FX4Splitter::ID_EXPAND_BOTTOMRIGHT,FX4Splitter::onUpdExpand),
  FXMAPFUNCS(SEL_COMMAND,FX4Splitter::ID_EXPAND_ALL,FX4Splitter::ID_EXPAND_BOTTOMRIGHT,FX4Splitter::onCmdExpand),
  };


// Object implementation
FXIMPLEMENT(FX4Splitter,FXComposite,FX4SplitterMap,ARRAYNUMBER(FX4SplitterMap))


// Make a splitter
FX4Splitter::FX4Splitter(){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  splitx=0;
  splity=0;
  expanded=-1;
  barsize=4;
  fhor=5000;
  fver=5000;
  offx=0;
  offy=0;
  mode=NOWHERE;
  }


// Make a splitter; it has no interior padding, and no borders
FX4Splitter::FX4Splitter(FXComposite* p,FXuint opts,FXint x,FXint y,FXint w,FXint h):
  FXComposite(p,opts,x,y,w,h){
  defaultCursor=getApp()->getDefaultCursor(DEF_ARROW_CURSOR);
  dragCursor=defaultCursor;
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  splitx=0;
  splity=0;
  expanded=-1;
  barsize=4;
  fhor=5000;
  fver=5000;
  offx=0;
  offy=0;
  mode=NOWHERE;
  }


// Make a splitter; it has no interior padding, and no borders
FX4Splitter::FX4Splitter(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):
  FXComposite(p,opts,x,y,w,h){
  defaultCursor=getApp()->getDefaultCursor(DEF_ARROW_CURSOR);
  dragCursor=defaultCursor;
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  target=tgt;
  message=sel;
  splitx=0;
  splity=0;
  expanded=-1;
  barsize=4;
  fhor=5000;
  fver=5000;
  offx=0;
  offy=0;
  mode=NOWHERE;
  }


// Get top left child
FXWindow *FX4Splitter::getTopLeft() const {
  return getFirst();
  }


// Get top right child
FXWindow *FX4Splitter::getTopRight() const {
  if(!getFirst()) return NULL;
  return getFirst()->getNext();
  }


// Get bottom left child
FXWindow *FX4Splitter::getBottomLeft() const {
  if(!getFirst() || !getFirst()->getNext()) return NULL;
  return getFirst()->getNext()->getNext();
  }


// Get bottom right child
FXWindow *FX4Splitter::getBottomRight() const {
  if(!getFirst() || !getFirst()->getNext() || !getFirst()->getNext()->getNext()) return NULL;
  return getFirst()->getNext()->getNext()->getNext();
  }


// Get default width
FXint FX4Splitter::getDefaultWidth(){
  register FXint tlw,blw,trw,brw,bs;
  register FXWindow *tl,*tr,*bl,*br;
  tlw=blw=trw=brw=bs=0;
  tl=getTopLeft();
  tr=getTopRight();
  bl=getBottomLeft();
  br=getBottomRight();
  if(tl) tlw=tl->getDefaultWidth();
  if(bl) blw=bl->getDefaultWidth();
  if(tr) trw=tr->getDefaultWidth();
  if(br) brw=br->getDefaultWidth();
  if((tl && tr) || (bl && br)) bs=barsize;
  return bs+FXMAX(tlw,blw)+FXMAX(trw,brw);
  }


// Get default height
FXint FX4Splitter::getDefaultHeight(){
  register FXint tlh,blh,trh,brh,bs;
  register FXWindow *tl,*tr,*bl,*br;
  tlh=blh=trh=brh=bs=0;
  tl=getTopLeft();
  tr=getTopRight();
  bl=getBottomLeft();
  br=getBottomRight();
  if(tl) tlh=tl->getDefaultHeight();
  if(bl) blh=bl->getDefaultHeight();
  if(tr) trh=tr->getDefaultHeight();
  if(br) brh=br->getDefaultHeight();
  if((tl && bl) || (tr && br)) bs=barsize;
  return bs+FXMAX(tlh,trh)+FXMAX(blh,brh);
  }


// Recompute layout
void FX4Splitter::layout(){
  register FXint totw,toth,bottomh,rightw;
  FXWindow *win[4];
  FXASSERT(expanded<4);
  win[0]=getTopLeft();
  win[1]=getTopRight();
  win[2]=getBottomLeft();
  win[3]=getBottomRight();
  if(expanded<0){
    totw=width-barsize;
    toth=height-barsize;
    FXASSERT(0<=fhor && fhor<=10000);
    FXASSERT(0<=fver && fver<=10000);
    splitx=(fhor*totw)/10000;
    splity=(fver*toth)/10000;
    rightw=totw-splitx;
    bottomh=toth-splity;
    if(win[0]){ win[0]->position(0,0,splitx,splity); win[0]->show(); }
    if(win[1]){ win[1]->position(splitx+barsize,0,rightw,splity); win[1]->show(); }
    if(win[2]){ win[2]->position(0,splity+barsize,splitx,bottomh); win[2]->show(); }
    if(win[3]){ win[3]->position(splitx+barsize,splity+barsize,rightw,bottomh); win[3]->show(); }
    }
  else{
    if(win[0] && expanded!=0) win[0]->hide();
    if(win[1] && expanded!=1) win[1]->hide();
    if(win[2] && expanded!=2) win[2]->hide();
    if(win[3] && expanded!=3) win[3]->hide();
    if(win[expanded]){ win[expanded]->position(0,0,width,height); win[expanded]->show(); }
    }
  flags&=~FLAG_DIRTY;
  }


// Determine split mode
FXuchar FX4Splitter::getMode(FXint x,FXint y){
  register FXuchar mm=ONCENTER;
  if(x<splitx-FUDGE) mm&=~ONVERTICAL;
  if(y<splity-FUDGE) mm&=~ONHORIZONTAL;
  if(x>=splitx+barsize+FUDGE) mm&=~ONVERTICAL;
  if(y>=splity+barsize+FUDGE) mm&=~ONHORIZONTAL;
  return mm;
  }


// Move the split intelligently
void FX4Splitter::moveSplit(FXint x,FXint y){
  if(x<0) x=0;
  if(y<0) y=0;
  if(x>width-barsize) x=width-barsize;
  if(y>height-barsize) y=height-barsize;
  splitx=x;
  splity=y;
  }


// Adjust layout
void FX4Splitter::adjustLayout(){
  FXWindow *win;
  FXint bottomh,rightw;
  fhor=(width>barsize) ? (10000*splitx+(width-barsize-1))/(width-barsize) : 0;
  fver=(height>barsize) ? (10000*splity+(height-barsize-1))/(height-barsize) : 0;
  rightw=width-barsize-splitx;
  bottomh=height-barsize-splity;
  if((win=getTopLeft())!=NULL){
    win->position(0,0,splitx,splity);
    }
  if((win=getTopRight())!=NULL){
    win->position(splitx+barsize,0,rightw,splity);
    }
  if((win=getBottomLeft())!=NULL){
    win->position(0,splity+barsize,splitx,bottomh);
    }
  if((win=getBottomRight())!=NULL){
    win->position(splitx+barsize,splity+barsize,rightw,bottomh);
    }
  }


// Button being pressed
long FX4Splitter::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  FXEvent* ev=(FXEvent*)ptr;
  if(isEnabled()){
    grab();
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;
    mode=getMode(ev->win_x,ev->win_y);
    if(mode){
      offx=ev->win_x-splitx;
      offy=ev->win_y-splity;
      if(!(options&FOURSPLITTER_TRACKING)){
        drawSplit(splitx,splity);
        }
      flags&=~FLAG_UPDATE;
      flags|=FLAG_PRESSED;
      }
    return 1;
    }
  return 0;
  }


// Button being released
long FX4Splitter::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  FXuint flgs=flags;
  if(isEnabled()){
    ungrab();
    flags|=FLAG_UPDATE;
    flags&=~FLAG_CHANGED;
    flags&=~FLAG_PRESSED;
    mode=NOWHERE;
    if(target && target->tryHandle(this,FXSEL(SEL_LEFTBUTTONRELEASE,message),ptr)) return 1;
    if(flgs&FLAG_PRESSED){
      if(!(options&FOURSPLITTER_TRACKING)){
        drawSplit(splitx,splity);
        adjustLayout();
        if(flgs&FLAG_CHANGED){
          if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),NULL);
          }
        }
      if(flgs&FLAG_CHANGED){
        if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),NULL);
        }
      }
    return 1;
    }
  return 0;
  }


// Button being released
long FX4Splitter::onMotion(FXObject*,FXSelector,void* ptr){
  FXEvent* ev=(FXEvent*)ptr;
  FXuchar ff;

  // Moving split
  if(flags&FLAG_PRESSED){
    FXint oldsplitx=splitx;
    FXint oldsplity=splity;
    if(mode==ONCENTER){
      moveSplit(ev->win_x-offx,ev->win_y-offy);
      }
    else if(mode==ONVERTICAL){
      moveSplit(ev->win_x-offx,splity);
      }
    else if(mode==ONHORIZONTAL){
      moveSplit(splitx,ev->win_y-offy);
      }
    if((oldsplitx!=splitx) || (oldsplity!=splity)){
      if(!(options&FOURSPLITTER_TRACKING)){
        drawSplit(oldsplitx,oldsplity);
        drawSplit(splitx,splity);
        }
      else{
        adjustLayout();
        if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),NULL);
        }
      flags|=FLAG_CHANGED;
      }
    return 1;
    }

  // Change cursor based on position
  ff=getMode(ev->win_x,ev->win_y);
  if(ff==ONCENTER){
    setDefaultCursor(getApp()->getDefaultCursor(DEF_XSPLIT_CURSOR));
    setDragCursor(getApp()->getDefaultCursor(DEF_XSPLIT_CURSOR));
    }
  else if(ff==ONVERTICAL){
    setDefaultCursor(getApp()->getDefaultCursor(DEF_HSPLIT_CURSOR));
    setDragCursor(getApp()->getDefaultCursor(DEF_HSPLIT_CURSOR));
    }
  else if(ff==ONHORIZONTAL){
    setDefaultCursor(getApp()->getDefaultCursor(DEF_VSPLIT_CURSOR));
    setDragCursor(getApp()->getDefaultCursor(DEF_VSPLIT_CURSOR));
    }
  else{
    setDefaultCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
    setDragCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
    }
  return 0;
  }


// Focus moved up
long FX4Splitter::onFocusUp(FXObject*,FXSelector,void* ptr){
  FXWindow *child=NULL;
  if(getFocus()){
    if(getFocus()==getBottomLeft()) child=getTopLeft();
    else if(getFocus()==getBottomRight()) child=getTopRight();
    }
  else{
    child=getLast();
    }
  if(child){
    if(child->shown()){
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      if(child->handle(this,FXSEL(SEL_FOCUS_UP,0),ptr)) return 1;
      }
    }
  return 0;
  }


// Focus moved down
long FX4Splitter::onFocusDown(FXObject*,FXSelector,void* ptr){
  FXWindow *child=NULL;
  if(getFocus()){
    if(getFocus()==getTopLeft()) child=getBottomLeft();
    else if(getFocus()==getTopRight()) child=getBottomRight();
    }
  else{
    child=getFirst();
    }
  if(child){
    if(child->shown()){
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      if(child->handle(this,FXSEL(SEL_FOCUS_DOWN,0),ptr)) return 1;
      }
    }
  return 0;
  }


// Focus moved to left
long FX4Splitter::onFocusLeft(FXObject*,FXSelector,void* ptr){
  FXWindow *child=NULL;
  if(getFocus()){
    if(getFocus()==getTopRight()) child=getTopLeft();
    else if(getFocus()==getBottomRight()) child=getBottomLeft();
    }
  else{
    child=getLast();
    }
  if(child){
    if(child->shown()){
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      if(child->handle(this,FXSEL(SEL_FOCUS_LEFT,0),ptr)) return 1;
      }
    }
  return 0;
  }


// Focus moved to right
long FX4Splitter::onFocusRight(FXObject*,FXSelector,void* ptr){
  FXWindow *child=NULL;
  if(getFocus()){
    if(getFocus()==getTopLeft()) child=getTopRight();
    else if(getFocus()==getBottomLeft()) child=getBottomRight();
    }
  else{
    child=getFirst();
    }
  if(child){
    if(child->shown()){
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      if(child->handle(this,FXSEL(SEL_FOCUS_RIGHT,0),ptr)) return 1;
      }
    }
  return 0;
  }


// Show the pane(s)
long FX4Splitter::onCmdExpand(FXObject*,FXSelector sel,void*){
  FXint ex=FXSELID(sel)-ID_EXPAND_ALL-1;
  setExpanded(ex);
  return 1;
  }


// Update show pane
long FX4Splitter::onUpdExpand(FXObject* sender,FXSelector sel,void*){
  register FXint ex=FXSELID(sel)-ID_EXPAND_ALL-1;
  sender->handle(this,(expanded==ex)?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


// Draw the horizontal split
void FX4Splitter::drawSplit(FXint x,FXint y){
  FXDCWindow dc(this);
  dc.clipChildren(FALSE);
  dc.setFunction(BLT_NOT_DST);
  if(mode&ONVERTICAL){
    dc.fillRectangle(x,0,barsize,height);
    }
  if(mode&ONHORIZONTAL){
    dc.fillRectangle(0,y,width,barsize);
    }
  }


// Change horizontal split [fraction*10000]
void FX4Splitter::setHSplit(FXint s){
  if(s<0) s=0;
  if(s>10000) s=10000;
  if(s!=fhor){
    fhor=s;
    recalc();
    }
  }


// Change vertical split [fraction*10000]
void FX4Splitter::setVSplit(FXint s){
  if(s<0) s=0;
  if(s>10000) s=10000;
  if(s!=fver){
    fver=s;
    recalc();
    }
  }


// Save object to stream
void FX4Splitter::save(FXStream& store) const {
  FXComposite::save(store);
  store << expanded;
  store << barsize;
  store << fhor;
  store << fver;
  }



// Load object from stream
void FX4Splitter::load(FXStream& store){
  FXComposite::load(store);
  store >> expanded;
  store >> barsize;
  store >> fhor;
  store >> fver;
  }


// Return splitter style
FXuint FX4Splitter::getSplitterStyle() const {
  return (options&FOURSPLITTER_MASK);
  }


// Change mode
void FX4Splitter::setSplitterStyle(FXuint style){
  options=(options&~FOURSPLITTER_MASK) | (style&FOURSPLITTER_MASK);
  }


// Expand one or all of the four panes
void FX4Splitter::setExpanded(FXint ex){
  if(ex>=4){ fxerror("%s::setExpanded: index out of range\n",getClassName()); }
  if(expanded!=ex){
    expanded=ex;
    recalc();
    }
  }


// Change bar size
void FX4Splitter::setBarSize(FXint bs){
  if(bs!=barsize){
    barsize=bs;
    recalc();
    }
  }

}
