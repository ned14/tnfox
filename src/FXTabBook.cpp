/********************************************************************************
*                                                                               *
*                         T a b   B o o k   W i d g e t                         *
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
* $Id: FXTabBook.cpp,v 1.18 2005/01/16 16:06:07 fox Exp $                       *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxkeys.h"
#include "FXHash.h"
#include "QThread.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXApp.h"
#include "FXDCWindow.h"
#include "FXFont.h"
#include "FXIcon.h"
#include "FXTabBook.h"


/*
  Notes:
  - Should focus go to tab items?
  - Should callbacks come from tab items?
  - Should redesign this stuff a little.
  - Tab items should observe various border styles.
  - TAB/TABTAB should go into content, arrow keys navigate between tabs.
  - FXTabBook: pane's hints make no sense to observe
  - We hide the panes in FXTabBook.  This way, we don't have to change
    the position of each pane when the FXTabBook itself changes.
    Only the active pane needs to be resized, leading to much faster
    layouts.
  - Fix setCurrent() to be like FXSwitcher.
*/


#define TABBOOK_MASK       (TABBOOK_SIDEWAYS|TABBOOK_BOTTOMTABS)



/*******************************************************************************/

namespace FX {

FXDEFMAP(FXTabBook) FXTabBookMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXTabBook::onPaint),
  FXMAPFUNC(SEL_FOCUS_NEXT,0,FXTabBook::onFocusNext),
  FXMAPFUNC(SEL_FOCUS_PREV,0,FXTabBook::onFocusPrev),
  FXMAPFUNC(SEL_FOCUS_UP,0,FXTabBook::onFocusUp),
  FXMAPFUNC(SEL_FOCUS_DOWN,0,FXTabBook::onFocusDown),
  FXMAPFUNC(SEL_FOCUS_LEFT,0,FXTabBook::onFocusLeft),
  FXMAPFUNC(SEL_FOCUS_RIGHT,0,FXTabBook::onFocusRight),
  FXMAPFUNC(SEL_COMMAND,FXTabBar::ID_OPEN_ITEM,FXTabBook::onCmdOpenItem),
  };


// Object implementation
FXIMPLEMENT(FXTabBook,FXTabBar,FXTabBookMap,ARRAYNUMBER(FXTabBookMap))


// Make a tab book
FXTabBook::FXTabBook(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):
  FXTabBar(p,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb){
  }


// Get width
FXint FXTabBook::getDefaultWidth(){
  register FXint w,wtabs,wmaxtab,wpnls,t,ntabs;
  register FXWindow *tab,*pane;
  register FXuint hints;

  // Left or right tabs
  if(options&TABBOOK_SIDEWAYS){
    wtabs=wpnls=0;
    for(tab=getFirst(); tab && tab->getNext(); tab=tab->getNext()->getNext()){
      pane=tab->getNext();
      if(tab->shown()){
        hints=tab->getLayoutHints();
        if(hints&LAYOUT_FIX_WIDTH) t=tab->getWidth()-2; else t=tab->getDefaultWidth()-2;
        if(t>wtabs) wtabs=t;
        t=pane->getDefaultWidth();
        if(t>wpnls) wpnls=t;
        }
      }
    w=wtabs+wpnls;
    }

  // Top or bottom tabs
  else{
    wtabs=wpnls=wmaxtab=ntabs=0;
    for(tab=getFirst(); tab && tab->getNext(); tab=tab->getNext()->getNext()){
      pane=tab->getNext();
      if(tab->shown()){
        hints=tab->getLayoutHints();
        if(hints&LAYOUT_FIX_WIDTH) t=tab->getWidth(); else t=tab->getDefaultWidth();
        if(t>wmaxtab) wmaxtab=t;
        wtabs+=t;
        t=pane->getDefaultWidth();
        if(t>wpnls) wpnls=t;
        ntabs++;
        }
      }
    if(options&PACK_UNIFORM_WIDTH) wtabs=ntabs*wmaxtab;
    wtabs+=5;
    w=FXMAX(wtabs,wpnls);
    }
  return w+padleft+padright+(border<<1);
  }


// Get height
FXint FXTabBook::getDefaultHeight(){
  register FXint h,htabs,hmaxtab,hpnls,t,ntabs;
  register FXWindow *tab,*pane;
  register FXuint hints;

  // Left or right tabs
  if(options&TABBOOK_SIDEWAYS){
    htabs=hpnls=hmaxtab=ntabs=0;
    for(tab=getFirst(); tab && tab->getNext(); tab=tab->getNext()->getNext()){
      pane=tab->getNext();
      if(tab->shown()){
        hints=tab->getLayoutHints();
        if(hints&LAYOUT_FIX_HEIGHT) t=tab->getHeight(); else t=tab->getDefaultHeight();
        if(t>hmaxtab) hmaxtab=t;
        htabs+=t;
        t=pane->getDefaultHeight();
        if(t>hpnls) hpnls=t;
        ntabs++;
        }
      }
    if(options&PACK_UNIFORM_HEIGHT) htabs=ntabs*hmaxtab;
    htabs+=5;
    h=FXMAX(htabs,hpnls);
    }

  // Top or bottom tabs
  else{
    htabs=hpnls=0;
    for(tab=getFirst(); tab && tab->getNext(); tab=tab->getNext()->getNext()){
      pane=tab->getNext();
      if(tab->shown()){
        hints=tab->getLayoutHints();
        if(hints&LAYOUT_FIX_HEIGHT) t=tab->getHeight()-2; else t=tab->getDefaultHeight()-2;
        if(t>htabs) htabs=t;
        t=pane->getDefaultHeight();
        if(t>hpnls) hpnls=t;
        }
      }
    h=htabs+hpnls;
    }
  return h+padtop+padbottom+(border<<1);
  }


// Recalculate layout
void FXTabBook::layout(){
  register int i,x,y,w,h,px,py,pw,ph,wmaxtab,hmaxtab,newcurrent;
  register FXWindow *raisepane=NULL;
  register FXWindow *raisetab=NULL;
  register FXWindow *pane,*tab;
  register FXuint hints;

  newcurrent=-1;

  // Measure tabs again
  wmaxtab=hmaxtab=0;
  for(tab=getFirst(),i=0; tab && tab->getNext(); tab=tab->getNext()->getNext(),i++){
    pane=tab->getNext();
    if(tab->shown()){
      hints=tab->getLayoutHints();
      if(hints&LAYOUT_FIX_WIDTH) w=tab->getWidth(); else w=tab->getDefaultWidth();
      if(hints&LAYOUT_FIX_HEIGHT) h=tab->getHeight(); else h=tab->getDefaultHeight();
      if(w>wmaxtab) wmaxtab=w;
      if(h>hmaxtab) hmaxtab=h;
      if(newcurrent<0 || i<=current) newcurrent=i;
      }
    }

  // This will change only if current now invisible
  current=newcurrent;

  // Left or right tabs
  if(options&TABBOOK_SIDEWAYS){

    // Place panel
    px=(options&TABBOOK_BOTTOMTABS) ? border+padleft : border+padleft+wmaxtab-2;
    py=border+padtop;
    pw=width-padleft-padright-(border<<1)-wmaxtab+2;
    ph=height-padtop-padbottom-(border<<1);

    // Place all of the children
    for(tab=getFirst(),y=py,i=0; tab && tab->getNext(); tab=tab->getNext()->getNext(),i++){
      pane=tab->getNext();
      if(tab->shown()){
        pane->position(px,py,pw,ph);
        hints=tab->getLayoutHints();
        if(hints&LAYOUT_FIX_WIDTH) w=tab->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=wmaxtab;
        else w=tab->getDefaultWidth();
        if(hints&LAYOUT_FIX_HEIGHT) h=tab->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=hmaxtab;
        else h=tab->getDefaultHeight();
        if(current==i){
          if(options&TABBOOK_BOTTOMTABS)
            tab->position(px+pw-2,y,w,h);
          else
            tab->position(px-w+2,y,w,h);
          pane->show();
          raisetab=tab;
          raisepane=pane;
          y+=h-3;
          }
        else{
          if(options&TABBOOK_BOTTOMTABS)
            tab->position(px+pw-4,y+2,w,h);
          else
            tab->position(px-w+4,y+2,w,h);
          tab->update(0,0,wmaxtab,h);
          pane->hide();
          y+=h;
          }
        }
      else{
        pane->hide();
        }
      }

    // Hide spurious last tab
    if(tab) tab->resize(0,0);
    }

  // Top or bottom tabs
  else{

    // Place panel
    px=border+padleft;
    py=(options&TABBOOK_BOTTOMTABS) ? border+padtop : border+padtop+hmaxtab-2;
    pw=width-padleft-padright-(border<<1);
    ph=height-padtop-padbottom-(border<<1)-hmaxtab+2;

    // Place all of the children
    for(tab=getFirst(),x=px,i=0; tab && tab->getNext(); tab=tab->getNext()->getNext(),i++){
      pane=tab->getNext();
      if(tab->shown()){
        pane->position(px,py,pw,ph);
        hints=tab->getLayoutHints();
        if(hints&LAYOUT_FIX_WIDTH) w=tab->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=wmaxtab;
        else w=tab->getDefaultWidth();
        if(hints&LAYOUT_FIX_HEIGHT) h=tab->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=hmaxtab;
        else h=tab->getDefaultHeight();
        if(current==i){
          if(options&TABBOOK_BOTTOMTABS)
            tab->position(x,py+ph-2,w,h);
          else
            tab->position(x,py-h+2,w,h);
          pane->show();
          raisepane=pane;
          raisetab=tab;
          x+=w-3;
          }
        else{
          if(options&TABBOOK_BOTTOMTABS)
            tab->position(x+2,py+ph-4,w,h);
          else
            tab->position(x+2,py-h+4,w,h);
          pane->hide();
          x+=w;
          }
        }
      else{
        pane->hide();
        }
      }

    // Hide spurious last tab
    if(tab) tab->resize(0,0);
    }

  // Raise tab over panel and panel over all other tabs
  if(raisepane) raisepane->raise();
  if(raisetab) raisetab->raise();

  flags&=~FLAG_DIRTY;
  }


// The sender of the message is the item to open up
long FXTabBook::onCmdOpenItem(FXObject* sender,FXSelector,void*){
  setCurrent(indexOfChild((FXWindow*)sender)/2,TRUE);
  return 1;
  }


// Handle repaint
long FXTabBook::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  FXDCWindow dc(this,ev);
  dc.setForeground(backColor);
  dc.fillRectangle(ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h);
  drawFrame(dc,0,0,width,height);
  return 1;
  }


// Focus moved to next tab
long FXTabBook::onFocusNext(FXObject*,FXSelector,void* ptr){
  FXWindow *child=getFocus();
  FXint which;
  if(child){
    child=child->getNext();
    if(!child) return 0;
    which=indexOfChild(child);
    if(which&1){
      child=child->getNext();
      which++;
      }
    }
  else{
    child=getFirst();
    which=0;
    }
  while(child && child->getNext() && !(child->shown() && child->isEnabled())){
    child=child->getNext()->getNext();
    which+=2;
    }
  if(child){
    setCurrent(which>>1,TRUE);
    child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
    return 1;
    }
  return 0;
  }


// Focus moved to previous
long FXTabBook::onFocusPrev(FXObject*,FXSelector,void* ptr){
  FXWindow *child=getFocus();
  FXint which;
  if(child){
    child=child->getPrev();
    if(!child) return 0;
    which=indexOfChild(child);
    }
  else{
    child=getLast();
    if(!child) return 0;
    which=indexOfChild(child);
    }
  if(which&1){
    child=child->getPrev();
    }
  while(child && child->getPrev() && !(child->shown() && child->isEnabled())){
    child=child->getPrev()->getPrev();
    which-=2;
    }
  if(child){
    setCurrent(which>>1,TRUE);
    child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
    return 1;
    }
  return 0;
  }


// Focus moved up
long FXTabBook::onFocusUp(FXObject*,FXSelector,void* ptr){
  if(options&TABBOOK_SIDEWAYS){
    return handle(this,FXSEL(SEL_FOCUS_PREV,0),ptr);
    }
  if(getFocus()){
    FXWindow *child=NULL;
    if(indexOfChild(getFocus())&1){     // We're on a panel
      if(!(options&TABBOOK_BOTTOMTABS)) child=getFocus()->getPrev();
      }
    else{                               // We're on a tab
      if(options&TABBOOK_BOTTOMTABS) child=getFocus()->getNext();
      }
    if(child){
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      if(child->handle(this,FXSEL(SEL_FOCUS_UP,0),ptr)) return 1;
      }
    }
  return 0;
  }


// Focus moved down
long FXTabBook::onFocusDown(FXObject*,FXSelector,void* ptr){
  if(options&TABBOOK_SIDEWAYS){
    return handle(this,FXSEL(SEL_FOCUS_NEXT,0),ptr);
    }
  if(getFocus()){
    FXWindow *child=NULL;
    if(indexOfChild(getFocus())&1){     // We're on a panel
      if(options&TABBOOK_BOTTOMTABS) child=getFocus()->getPrev();
      }
    else{                               // We're on a tab
      if(!(options&TABBOOK_BOTTOMTABS)) child=getFocus()->getNext();
      }
    if(child){
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      if(child->handle(this,FXSEL(SEL_FOCUS_DOWN,0),ptr)) return 1;
      }
    }
  return 0;
  }


// Focus moved left
long FXTabBook::onFocusLeft(FXObject*,FXSelector,void* ptr){
  if(!(options&TABBOOK_SIDEWAYS)){
    return handle(this,FXSEL(SEL_FOCUS_PREV,0),ptr);
    }
  if(getFocus()){
    FXWindow *child=NULL;
    if(indexOfChild(getFocus())&1){     // We're on a panel
      if(!(options&TABBOOK_BOTTOMTABS)) child=getFocus()->getPrev();
      }
    else{                               // We're on a tab
      if(options&TABBOOK_BOTTOMTABS) child=getFocus()->getNext();
      }
    if(child){
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      if(child->handle(this,FXSEL(SEL_FOCUS_LEFT,0),ptr)) return 1;
      }
    }
  return 0;
  }


// Focus moved right
long FXTabBook::onFocusRight(FXObject*,FXSelector,void* ptr){
  if(!(options&TABBOOK_SIDEWAYS)){
    return handle(this,FXSEL(SEL_FOCUS_NEXT,0),ptr);
    }
  if(getFocus()){
    FXWindow *child=NULL;
    if(indexOfChild(getFocus())&1){     // We're on a panel
      if(options&TABBOOK_BOTTOMTABS) child=getFocus()->getPrev();
      }
    else{                               // We're on a tab
      if(!(options&TABBOOK_BOTTOMTABS)) child=getFocus()->getNext();
      }
    if(child){
      if(child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr)) return 1;
      if(child->handle(this,FXSEL(SEL_FOCUS_RIGHT,0),ptr)) return 1;
      }
    }
  return 0;
  }

}
