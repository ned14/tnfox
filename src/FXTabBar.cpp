/********************************************************************************
*                                                                               *
*                               T a b   O b j e c t                             *
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
* $Id: FXTabBar.cpp,v 1.15 2004/02/08 17:29:07 fox Exp $                        *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxkeys.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXHash.h"
#include "FXApp.h"
#include "FXDCWindow.h"
#include "FXFont.h"
#include "FXIcon.h"
#include "FXTabBar.h"


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
    Only the active pane needs to be moved.
  - Fix setCurrent() to be like FXSwitcher.
*/


#define TAB_ORIENT_MASK    (TAB_TOP|TAB_LEFT|TAB_RIGHT|TAB_BOTTOM)
#define TABBOOK_MASK       (TABBOOK_SIDEWAYS|TABBOOK_BOTTOMTABS)



/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXTabBar) FXTabBarMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXTabBar::onPaint),
  FXMAPFUNC(SEL_FOCUS_NEXT,0,FXTabBar::onFocusNext),
  FXMAPFUNC(SEL_FOCUS_PREV,0,FXTabBar::onFocusPrev),
  FXMAPFUNC(SEL_FOCUS_UP,0,FXTabBar::onFocusUp),
  FXMAPFUNC(SEL_FOCUS_DOWN,0,FXTabBar::onFocusDown),
  FXMAPFUNC(SEL_FOCUS_LEFT,0,FXTabBar::onFocusLeft),
  FXMAPFUNC(SEL_FOCUS_RIGHT,0,FXTabBar::onFocusRight),
  FXMAPFUNC(SEL_COMMAND,FXTabBar::ID_OPEN_ITEM,FXTabBar::onCmdOpenItem),
  FXMAPFUNC(SEL_COMMAND,FXTabBar::ID_SETVALUE,FXTabBar::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXTabBar::ID_SETINTVALUE,FXTabBar::onCmdSetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXTabBar::ID_GETINTVALUE,FXTabBar::onCmdGetIntValue),
  FXMAPFUNCS(SEL_UPDATE,FXTabBar::ID_OPEN_FIRST,FXTabBar::ID_OPEN_LAST,FXTabBar::onUpdOpen),
  FXMAPFUNCS(SEL_COMMAND,FXTabBar::ID_OPEN_FIRST,FXTabBar::ID_OPEN_LAST,FXTabBar::onCmdOpen),
  };


// Object implementation
FXIMPLEMENT(FXTabBar,FXPacker,FXTabBarMap,ARRAYNUMBER(FXTabBarMap))


// Make a tab bar
FXTabBar::FXTabBar(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):
  FXPacker(p,opts,x,y,w,h,pl,pr,pt,pb,0,0){
  flags|=FLAG_ENABLED;
  target=tgt;
  message=sel;
  current=0;
  }


// Get width
FXint FXTabBar::getDefaultWidth(){
  register FXint w,wtabs,wmaxtab,t,ntabs;
  register FXuint hints;
  register FXWindow *child;
  if(options&TABBOOK_SIDEWAYS){
    wtabs=0;
    for(child=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        if(hints&LAYOUT_FIX_WIDTH) t=child->getWidth(); else t=child->getDefaultWidth();
        if(t>wtabs) wtabs=t;
        }
      }
    w=wtabs;
    }
  else{
    wtabs=wmaxtab=ntabs=0;
    for(child=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        if(hints&LAYOUT_FIX_WIDTH) t=child->getWidth(); else t=child->getDefaultWidth();
        if(t>wmaxtab) wmaxtab=t;
        wtabs+=t;
        ntabs++;
        }
      }
    if(options&PACK_UNIFORM_WIDTH) wtabs=ntabs*wmaxtab;
    w=wtabs+5;
    }
  return w+padleft+padright+(border<<1);
  }


// Get height
FXint FXTabBar::getDefaultHeight(){
  register FXint h,htabs,hmaxtab,t,ntabs;
  register FXuint hints;
  register FXWindow *child;
  if(options&TABBOOK_SIDEWAYS){
    htabs=hmaxtab=ntabs=0;
    for(child=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        if(hints&LAYOUT_FIX_HEIGHT) t=child->getHeight(); else t=child->getDefaultHeight();
        if(t>hmaxtab) hmaxtab=t;
        htabs+=t;
        ntabs++;
        }
      }
    if(options&PACK_UNIFORM_HEIGHT) htabs=ntabs*hmaxtab;
    h=htabs+5;
    }
  else{
    htabs=0;
    for(child=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        if(hints&LAYOUT_FIX_HEIGHT) t=child->getHeight(); else t=child->getDefaultHeight();
        if(t>htabs) htabs=t;
        }
      }
    h=htabs;
    }
  return h+padtop+padbottom+(border<<1);
  }


// Recalculate layout
void FXTabBar::layout(){
  register int i,x,y,w,h,wmaxtab,hmaxtab,newcurrent;
  register FXWindow *raisetab=NULL;
  register FXWindow *tab;
  register FXuint hints;

  newcurrent=-1;

  // Measure tabs again
  wmaxtab=hmaxtab=0;
  for(tab=getFirst(),i=0; tab; tab=tab->getNext(),i++){
    if(tab->shown()){
      hints=tab->getLayoutHints();
      if(hints&LAYOUT_FIX_WIDTH) w=tab->getWidth(); else w=tab->getDefaultWidth();
      if(hints&LAYOUT_FIX_HEIGHT) h=tab->getHeight(); else h=tab->getDefaultHeight();
      if(w>wmaxtab) wmaxtab=w;
      if(h>hmaxtab) hmaxtab=h;
      if(newcurrent<0 || i<=current) newcurrent=i;
      }
    }

  // Changes current only if old current no longer visible
  current=newcurrent;

  // Tabs on left or right
  if(options&TABBOOK_SIDEWAYS){

    // Placements for tab items and tab panels
    y=border+padtop;
    if(options&TABBOOK_BOTTOMTABS){         // Right tabs
      x=width-padright-border-wmaxtab;
      }
    else{
      x=border+padleft;
      }

    // Place all of the children
    for(i=0,tab=getFirst(); tab; tab=tab->getNext(),i++){
      if(tab->shown()){
        hints=tab->getLayoutHints();
        if(hints&LAYOUT_FIX_HEIGHT) h=tab->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=hmaxtab;
        else h=tab->getDefaultHeight();
        if(current==i){
          if(options&TABBOOK_BOTTOMTABS)      // Right tabs
            tab->position(x-2,y,wmaxtab+2,h+3);
          else
            tab->position(x,y,wmaxtab+2,h+3);
          tab->update(0,0,wmaxtab+2,h+3);
          raisetab=tab;
          }
        else{
          if(options&TABBOOK_BOTTOMTABS)      // Right tabs
            tab->position(x-2,y+2,wmaxtab,h);
          else
            tab->position(x+2,y+2,wmaxtab,h);
          tab->update(0,0,wmaxtab,h);
          }
        y+=h;
        }
      }
    }

  // Tabs on top or bottom
  else{

    // Placements for tab items and tab panels
    x=border+padleft;
    if(options&TABBOOK_BOTTOMTABS){         // Bottom tabs
      y=height-padbottom-border-hmaxtab;
      }
    else{
      y=border+padtop;
      }

    // Place all of the children
    for(i=0,tab=getFirst(); tab; tab=tab->getNext(),i++){
      if(tab->shown()){
        hints=tab->getLayoutHints();
        if(hints&LAYOUT_FIX_WIDTH) w=tab->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=wmaxtab;
        else w=tab->getDefaultWidth();
        if(current==i){
          if(options&TABBOOK_BOTTOMTABS)      // Bottom tabs
            tab->position(x,y-2,w+3,hmaxtab+2);
          else
            tab->position(x,y,w+3,hmaxtab+2);
          tab->update(0,0,w+3,hmaxtab+2);
          raisetab=tab;
          }
        else{
          if(options&TABBOOK_BOTTOMTABS)      // Bottom tabs
            tab->position(x+2,y-2,w,hmaxtab);
          else
            tab->position(x+2,y+2,w,hmaxtab);
          tab->update(0,0,w,hmaxtab);
          }
        x+=w;
        }
      }
    }
  if(raisetab) raisetab->raise();
  flags&=~FLAG_DIRTY;
  }


// Set current subwindow
void FXTabBar::setCurrent(FXint panel,FXbool notify){
  if(0<=panel && panel!=current){
    current=panel;
    if(notify && target){ target->handle(this,FXSEL(SEL_COMMAND,message),(void*)(FXival)current); }
    recalc();
    }
  }


// Handle repaint
long FXTabBar::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  FXDCWindow dc(this,ev);
  dc.setForeground(backColor);
  dc.fillRectangle(ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h);
  drawFrame(dc,0,0,width,height);
  return 1;
  }


// Focus moved to next visible tab
long FXTabBar::onFocusNext(FXObject*,FXSelector,void* ptr){
  FXWindow *child=getFocus();
  if(child) child=child->getNext(); else child=getFirst();
  while(child && !child->shown()) child=child->getNext();
  if(child){
    setCurrent(indexOfChild(child),TRUE);
    child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
    return 1;
    }
  return 0;
  }


// Focus moved to previous visible tab
long FXTabBar::onFocusPrev(FXObject*,FXSelector,void* ptr){
  FXWindow *child=getFocus();
  if(child) child=child->getPrev(); else child=getLast();
  while(child && !child->shown()) child=child->getPrev();
  if(child){
    setCurrent(indexOfChild(child),TRUE);
    child->handle(this,FXSEL(SEL_FOCUS_SELF,0),ptr);
    return 1;
    }
  return 0;
  }


// Focus moved up
long FXTabBar::onFocusUp(FXObject*,FXSelector,void* ptr){
  if(options&TABBOOK_SIDEWAYS){
    return handle(this,FXSEL(SEL_FOCUS_PREV,0),ptr);
    }
  return 0;
  }


// Focus moved down
long FXTabBar::onFocusDown(FXObject*,FXSelector,void* ptr){
  if(options&TABBOOK_SIDEWAYS){
    return handle(this,FXSEL(SEL_FOCUS_NEXT,0),ptr);
    }
  return 0;
  }


// Focus moved left
long FXTabBar::onFocusLeft(FXObject*,FXSelector,void* ptr){
  if(!(options&TABBOOK_SIDEWAYS)){
    return handle(this,FXSEL(SEL_FOCUS_PREV,0),ptr);
    }
  return 0;
  }


// Focus moved right
long FXTabBar::onFocusRight(FXObject*,FXSelector,void* ptr){
  if(!(options&TABBOOK_SIDEWAYS)){
    return handle(this,FXSEL(SEL_FOCUS_NEXT,0),ptr);
    }
  return 0;
  }


// Update value from a message
long FXTabBar::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setCurrent((FXint)(FXival)ptr);
  return 1;
  }


// Update value from a message
long FXTabBar::onCmdSetIntValue(FXObject*,FXSelector,void* ptr){
  setCurrent(*((FXint*)ptr));
  return 1;
  }


// Obtain value from text field
long FXTabBar::onCmdGetIntValue(FXObject*,FXSelector,void* ptr){
  *((FXint*)ptr)=getCurrent();
  return 1;
  }


// Open item
long FXTabBar::onCmdOpen(FXObject*,FXSelector sel,void*){
  setCurrent(FXSELID(sel)-ID_OPEN_FIRST,TRUE);
  return 1;
  }


// Update the nth button
long FXTabBar::onUpdOpen(FXObject* sender,FXSelector sel,void*){
  sender->handle(this,((FXSELID(sel)-ID_OPEN_FIRST)==current)?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


// The sender of the message is the item to open up
long FXTabBar::onCmdOpenItem(FXObject* sender,FXSelector,void*){
  setCurrent(indexOfChild((FXWindow*)sender),TRUE);
  return 1;
  }


// Get tab style
FXuint FXTabBar::getTabStyle() const {
  return (options&TABBOOK_MASK);
  }


// Set tab style
void FXTabBar::setTabStyle(FXuint style){
  FXuint opts=(options&~TABBOOK_MASK) | (style&TABBOOK_MASK);
  if(options!=opts){
    options=opts;
    recalc();
    update();
    }
  }


// Save object to stream
void FXTabBar::save(FXStream& store) const {
  FXPacker::save(store);
  store << current;
  }


// Load object from stream
void FXTabBar::load(FXStream& store){
  FXPacker::load(store);
  store >> current;
  }

}
