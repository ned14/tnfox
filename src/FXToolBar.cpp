/********************************************************************************
*                                                                               *
*                        T o o l   B a r   W i d g e t                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXToolBar.cpp,v 1.21 2004/09/17 07:46:22 fox Exp $                       *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "FXThread.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXApp.h"
#include "FXDCWindow.h"
#include "FXDrawable.h"
#include "FXWindow.h"
#include "FXFrame.h"
#include "FXComposite.h"
#include "FXPacker.h"
#include "FXToolBar.h"
#include "FXShell.h"
#include "FXTopWindow.h"
#include "FXToolBarGrip.h"


/*
  Notes:
  - Wrapping algorithm not OK yet; use D.P.
  - Divider widgets and special layout ideas:

      +---------+---------------+
      | 1 2 3 4 | 5 6 7         |
      +---------+---------------+

    after wrap:

      +-----+-----+
      | 1 2 | 5 6 |  Clue: use D.P.
      | 3 4 | 7   |
      +-----+-----+

    wrap:

      +-----------------+
      | L L L L   R R R |
      +-----------------+

    to:

      +-------------+
      | L L L   R R |
      +-------------+
      | L       R   |
      +-------------+

  - Toolbars with no wetdock should still be redockable on different sides.
  - Want to support stretchable items.
  - Want to support non-equal galley heights.

  - Look at this some more.
       1       2            3                              4
    1  w(1)  w(1)+w(2)      w(1)+w(2)+w(3)        w(1)+w(2)+w(3)+w(4)
    2  0    max(w(1),w(2)) max(w(1),w(2))+w(3)   max(w(1),w(2))+w(3)+w(4)
    3  0       0           max(w(1),w(2),w(3))   max(w(1),w(2),w(3))+w(4)
    4  0       0            0                    max(w(1),w(2),w(3),w(4))

*/

// How close to edge before considered docked
#define PROXIMITY    30
#define FUDGE        5

// Docking side
#define LAYOUT_SIDE_MASK (LAYOUT_SIDE_LEFT|LAYOUT_SIDE_RIGHT|LAYOUT_SIDE_TOP|LAYOUT_SIDE_BOTTOM)

// Horizontal placement options
#define LAYOUT_HORIZONTAL_MASK (LAYOUT_LEFT|LAYOUT_RIGHT|LAYOUT_CENTER_X|LAYOUT_FIX_X|LAYOUT_FILL_X)

// Vertical placement options
#define LAYOUT_VERTICAL_MASK   (LAYOUT_TOP|LAYOUT_BOTTOM|LAYOUT_CENTER_Y|LAYOUT_FIX_Y|LAYOUT_FILL_Y)



/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXToolBar) FXToolBarMap[]={
  FXMAPFUNC(SEL_FOCUS_PREV,0,FXToolBar::onFocusLeft),
  FXMAPFUNC(SEL_FOCUS_NEXT,0,FXToolBar::onFocusRight),
  FXMAPFUNC(SEL_UPDATE,FXToolBar::ID_UNDOCK,FXToolBar::onUpdUndock),
  FXMAPFUNC(SEL_UPDATE,FXToolBar::ID_DOCK_TOP,FXToolBar::onUpdDockTop),
  FXMAPFUNC(SEL_UPDATE,FXToolBar::ID_DOCK_BOTTOM,FXToolBar::onUpdDockBottom),
  FXMAPFUNC(SEL_UPDATE,FXToolBar::ID_DOCK_LEFT,FXToolBar::onUpdDockLeft),
  FXMAPFUNC(SEL_UPDATE,FXToolBar::ID_DOCK_RIGHT,FXToolBar::onUpdDockRight),
  FXMAPFUNC(SEL_COMMAND,FXToolBar::ID_UNDOCK,FXToolBar::onCmdUndock),
  FXMAPFUNC(SEL_COMMAND,FXToolBar::ID_DOCK_TOP,FXToolBar::onCmdDockTop),
  FXMAPFUNC(SEL_COMMAND,FXToolBar::ID_DOCK_BOTTOM,FXToolBar::onCmdDockBottom),
  FXMAPFUNC(SEL_COMMAND,FXToolBar::ID_DOCK_LEFT,FXToolBar::onCmdDockLeft),
  FXMAPFUNC(SEL_COMMAND,FXToolBar::ID_DOCK_RIGHT,FXToolBar::onCmdDockRight),
  FXMAPFUNC(SEL_BEGINDRAG,FXToolBar::ID_TOOLBARGRIP,FXToolBar::onBeginDragGrip),
  FXMAPFUNC(SEL_ENDDRAG,FXToolBar::ID_TOOLBARGRIP,FXToolBar::onEndDragGrip),
  FXMAPFUNC(SEL_DRAGGED,FXToolBar::ID_TOOLBARGRIP,FXToolBar::onDraggedGrip),
  };


// Object implementation
FXIMPLEMENT(FXToolBar,FXPacker,FXToolBarMap,ARRAYNUMBER(FXToolBarMap))


// Deserialization
FXToolBar::FXToolBar(){
  drydock=NULL;
  wetdock=NULL;
  outline.x=0;
  outline.y=0;
  outline.w=0;
  outline.h=0;
  dockafter=NULL;
  dockside=0;
  docking=FALSE;
  }


// Make a dockable and, possibly, floatable toolbar
FXToolBar::FXToolBar(FXComposite* p,FXComposite* q,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):
  FXPacker(p,opts,x,y,w,h,pl,pr,pt,pb,hs,vs){
  drydock=p;
  wetdock=q;
  outline.x=0;
  outline.y=0;
  outline.w=0;
  outline.h=0;
  dockafter=NULL;
  dockside=0;
  docking=FALSE;
  }


// Make a non-floatable toolbar
FXToolBar::FXToolBar(FXComposite* p,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):
  FXPacker(p,opts,x,y,w,h,pl,pr,pt,pb,hs,vs){
  drydock=NULL;
  wetdock=NULL;
  outline.x=0;
  outline.y=0;
  outline.w=0;
  outline.h=0;
  dockafter=NULL;
  dockside=0;
  docking=FALSE;
  }


// Compute minimum width based on child layout hints
FXint FXToolBar::getDefaultWidth(){
  register FXint w,wcum,wmax,mw=0,n;
  register FXWindow* child;
  register FXuint hints;
  wcum=wmax=n=0;
  if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(child->isMemberOf(FXMETACLASS(FXToolBarGrip))) w=child->getDefaultWidth();
      else if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
      else if(options&PACK_UNIFORM_WIDTH) w=mw;
      else w=child->getDefaultWidth();
      if(wmax<w) wmax=w;
      wcum+=w;
      n++;
      }
    }
  if(!(options&LAYOUT_SIDE_LEFT)){      // Horizontal
    if(n>1) wcum+=(n-1)*hspacing;
    wmax=wcum;
    }
  return padleft+padright+wmax+(border<<1);
  }


// Compute minimum height based on child layout hints
FXint FXToolBar::getDefaultHeight(){
  register FXint h,hcum,hmax,mh=0,n;
  register FXWindow* child;
  register FXuint hints;
  hcum=hmax=n=0;
  if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(child->isMemberOf(FXMETACLASS(FXToolBarGrip))) h=child->getDefaultHeight();
      else if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
      else if(options&PACK_UNIFORM_HEIGHT) h=mh;
      else h=child->getDefaultHeight();
      if(hmax<h) hmax=h;
      hcum+=h;
      n++;
      }
    }
  if(options&LAYOUT_SIDE_LEFT){         // Vertical
    if(n>1) hcum+=(n-1)*vspacing;
    hmax=hcum;
    }
  return padtop+padbottom+hmax+(border<<1);
  }


// Return width for given height
FXint FXToolBar::getWidthForHeight(FXint givenheight){
  FXint wtot,wmax,hcum,w,h,space,ngalleys,mw=0,mh=0;
  FXWindow* child;
  FXuint hints;
  wtot=wmax=hcum=ngalleys=0;
  space=givenheight-padtop-padbottom-(border<<1);
  if(space<1) space=1;
  if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();
  if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(child->isMemberOf(FXMETACLASS(FXToolBarGrip))) w=child->getDefaultWidth();
      else if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
      else if(options&PACK_UNIFORM_WIDTH) w=mw;
      else w=child->getDefaultWidth();
      if(child->isMemberOf(FXMETACLASS(FXToolBarGrip))) h=child->getDefaultHeight();
      else if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
      else if(options&PACK_UNIFORM_HEIGHT) h=mh;
      else h=child->getDefaultHeight();
      if(hcum+h>space) hcum=0;
      if(hcum==0) ngalleys++;
      hcum+=h+vspacing;
      if(wmax<w) wmax=w;
      }
    }
  wtot=wmax*ngalleys;
  return padleft+padright+wtot+(border<<1);
  }


// Return height for given width
FXint FXToolBar::getHeightForWidth(FXint givenwidth){
  FXint htot,hmax,wcum,w,h,space,ngalleys,mw=0,mh=0;
  FXWindow* child;
  FXuint hints;
  htot=hmax=wcum=ngalleys=0;
  space=givenwidth-padleft-padright-(border<<1);
  if(space<1) space=1;
  if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();
  if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(child->isMemberOf(FXMETACLASS(FXToolBarGrip))) w=child->getDefaultWidth();
      else if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
      else if(options&PACK_UNIFORM_WIDTH) w=mw;
      else w=child->getDefaultWidth();
      if(child->isMemberOf(FXMETACLASS(FXToolBarGrip))) h=child->getDefaultHeight();
      else if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
      else if(options&PACK_UNIFORM_HEIGHT) h=mh;
      else h=child->getDefaultHeight();
      if(wcum+w>space) wcum=0;
      if(wcum==0) ngalleys++;
      wcum+=w+hspacing;
      if(hmax<h) hmax=h;
      }
    }
  htot=hmax*ngalleys;
  return padtop+padbottom+htot+(border<<1);
  }


// Recalculate layout
void FXToolBar::layout(){
  FXint galleyleft,galleyright,galleytop,galleybottom,galleywidth,galleyheight;
  FXint tleft,tright,ttop,bleft,bbottom;
  FXint ltop,lbottom,lleft,rtop,rright;
  FXWindow *child;
  FXint x,y,w,h,mw=0,mh=0;
  FXuint hints;

  // Get maximum child size
  if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();
  if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();

  // Vertical toolbar
  if(options&LAYOUT_SIDE_LEFT){
    galleywidth=0;
    for(child=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        if(child->isMemberOf(FXMETACLASS(FXToolBarGrip))) w=child->getDefaultWidth();
        else if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=mw;
        else w=child->getDefaultWidth();
        if(galleywidth<w) galleywidth=w;
        }
      }
    galleyleft=border+padleft;
    galleyright=width-border-padright;
    galleytop=border+padtop;
    galleybottom=height-border-padbottom;
    tleft=galleyleft;
    tright=galleyleft+galleywidth;
    ttop=galleytop;
    bleft=galleyright-galleywidth;
    bbottom=galleybottom;
    for(child=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        if(child->isMemberOf(FXMETACLASS(FXToolBarGrip))){
          w=galleywidth;
          h=child->getDefaultHeight();
          }
        else{
          if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
          else if(options&PACK_UNIFORM_WIDTH) w=mw;
          else w=child->getDefaultWidth();
          if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
          else if(options&PACK_UNIFORM_HEIGHT) h=mh;
          else h=child->getDefaultHeight();
          }
        if(hints&LAYOUT_BOTTOM){
          if(bbottom-h<galleytop && bbottom!=galleybottom){
            bleft-=galleywidth;
            bbottom=galleybottom;
            }
          y=bbottom-h;
          bbottom-=(h+vspacing);
          x=bleft+(galleywidth-w)/2;
          }
        else{
          if(ttop+h>galleybottom && ttop!=galleytop){
            tleft=tright;
            tright+=galleywidth;
            ttop=galleytop;
            }
          y=ttop;
          ttop+=(h+vspacing);
          x=tleft+(galleywidth-w)/2;
          }
        child->position(x,y,w,h);
        }
      }
    }

  // Horizontal toolbar
  else{
    galleyheight=0;
    for(child=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        if(child->isMemberOf(FXMETACLASS(FXToolBarGrip))) h=child->getDefaultHeight();
        else if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=mh;
        else h=child->getDefaultHeight();
        if(galleyheight<h) galleyheight=h;
        }
      }
    galleyleft=border+padleft;
    galleyright=width-border-padright;
    galleytop=border+padtop;
    galleybottom=height-border-padbottom;
    ltop=galleytop;
    lbottom=galleytop+galleyheight;
    lleft=galleyleft;
    rtop=galleybottom-galleyheight;
    rright=galleyright;
    for(child=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        if(child->isMemberOf(FXMETACLASS(FXToolBarGrip))){
          w=child->getDefaultWidth();
          h=galleyheight;
          }
        else{
          if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
          else if(options&PACK_UNIFORM_WIDTH) w=mw;
          else w=child->getDefaultWidth();
          if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
          else if(options&PACK_UNIFORM_HEIGHT) h=mh;
          else h=child->getDefaultHeight();
          }
        if(hints&LAYOUT_RIGHT){
          if(rright-w<galleyleft && rright!=galleyright){
            rtop-=galleyheight;
            rright=galleyright;
            }
          x=rright-w;
          rright-=(w+hspacing);
          y=rtop+(galleyheight-h)/2;
          }
        else{
          if(lleft+w>galleyright && lleft!=galleyleft){
            ltop=lbottom;
            lbottom+=galleyheight;
            lleft=galleyleft;
            }
          x=lleft;
          lleft+=(w+hspacing);
          y=ltop+(galleyheight-h)/2;
          }
        child->position(x,y,w,h);
        }
      }
    }
  flags&=~FLAG_DIRTY;
  }


// Change toolbar orientation
void FXToolBar::setDockingSide(FXuint side){
  if((options&LAYOUT_SIDE_MASK)!=side){

    // New orientation is vertical
    if(side&LAYOUT_SIDE_LEFT){
      if(!(options&LAYOUT_SIDE_LEFT)){    // Was horizontal
        if((options&LAYOUT_RIGHT) && (options&LAYOUT_CENTER_X)) side|=LAYOUT_FIX_Y;
        else if(options&LAYOUT_RIGHT) side|=LAYOUT_BOTTOM;
        else if(options&LAYOUT_CENTER_X) side|=LAYOUT_CENTER_Y;
        if(options&LAYOUT_FILL_X) side|=LAYOUT_FILL_Y;
        }
      else{                               // Was vertical already
        side|=(options&(LAYOUT_BOTTOM|LAYOUT_CENTER_Y|LAYOUT_FILL_Y));
        }
      }

    // New orientation is horizontal
    else{
      if(options&LAYOUT_SIDE_LEFT){       // Was vertical
        if((options&LAYOUT_BOTTOM) && (options&LAYOUT_CENTER_Y)) side|=LAYOUT_FIX_X;
        else if(options&LAYOUT_BOTTOM) side|=LAYOUT_RIGHT;
        else if(options&LAYOUT_CENTER_Y) side|=LAYOUT_CENTER_X;
        if(options&LAYOUT_FILL_Y) side|=LAYOUT_FILL_X;
        }
      else{
        side|=(options&(LAYOUT_RIGHT|LAYOUT_CENTER_X|LAYOUT_FILL_X));
        }
      }

    // Simply preserve these options
    side|=(options&(LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT));

    // Update the layout
    setLayoutHints(side);
    }
  }


// Get toolbar orientation
FXuint FXToolBar::getDockingSide() const {
  return (options&LAYOUT_SIDE_MASK);
  }


// Return true if toolbar is docked
FXbool FXToolBar::isDocked() const {
  return (getParent()!=(FXWindow*)wetdock);
  }


// Set parent when docked, if it was docked it will remain docked
void FXToolBar::setDryDock(FXComposite* dry){
  if(dry && dry->id() && getParent()==(FXWindow*)drydock){
    reparent(dry);
    FXWindow* child=dry->getFirst();
    FXWindow* after=NULL;
    while(child){
      FXuint hints=child->getLayoutHints();
      if((hints&LAYOUT_FILL_X) && (hints&LAYOUT_FILL_Y)) break;
      after=child;
      child=child->getNext();
      }
    linkAfter(after);
    }
  drydock=dry;
  }


// Set parent when floating
void FXToolBar::setWetDock(FXComposite* wet){
  if(wet && wet->id() && getParent()==(FXWindow*)wetdock){
    reparent(wet);
    }
  wetdock=wet;
  }


// Dock the bar
void FXToolBar::dock(FXuint side,FXWindow* after){
  setDockingSide(side);
  if(drydock && !isDocked()){
    reparent(drydock);
    wetdock->hide();
    }
  if(after==(FXWindow*)-1L){
    after=NULL;
    FXWindow* child=getParent()->getFirst();
    while(child){
      FXuint hints=child->getLayoutHints();
      if((hints&LAYOUT_FILL_X) && (hints&LAYOUT_FILL_Y)) break;
      after=child;
      child=child->getNext();
      }
    }
  linkAfter(after);
  }


// Undock the bar
void FXToolBar::undock(){
  if(wetdock && isDocked()){
    FXint rootx,rooty;
    translateCoordinatesTo(rootx,rooty,getRoot(),8,8);
    reparent(wetdock);
    wetdock->position(rootx,rooty,wetdock->getDefaultWidth(),wetdock->getDefaultHeight());
    wetdock->show();
    }
  }


// Undock
long FXToolBar::onCmdUndock(FXObject*,FXSelector,void*){
  undock();
  return 1;
  }

long FXToolBar::onUpdUndock(FXObject* sender,FXSelector,void*){
  sender->handle(this,isDocked()?FXSEL(SEL_COMMAND,ID_UNCHECK):FXSEL(SEL_COMMAND,ID_CHECK),NULL);
  sender->handle(this,wetdock?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
  return 1;
  }


// Redock on top
long FXToolBar::onCmdDockTop(FXObject*,FXSelector,void*){
  dock(LAYOUT_SIDE_TOP,(FXWindow*)-1L);
  return 1;
  }

long FXToolBar::onUpdDockTop(FXObject* sender,FXSelector,void*){
  if(isDocked() && (options&LAYOUT_SIDE_MASK)==LAYOUT_SIDE_TOP)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


// Redock on bottom
long FXToolBar::onCmdDockBottom(FXObject*,FXSelector,void*){
  dock(LAYOUT_SIDE_BOTTOM,(FXWindow*)-1L);
  return 1;
  }

long FXToolBar::onUpdDockBottom(FXObject* sender,FXSelector,void*){
  if(isDocked() && (options&LAYOUT_SIDE_MASK)==LAYOUT_SIDE_BOTTOM)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


// Redock on left
long FXToolBar::onCmdDockLeft(FXObject*,FXSelector,void*){
  dock(LAYOUT_SIDE_LEFT,(FXWindow*)-1L);
  return 1;
  }

long FXToolBar::onUpdDockLeft(FXObject* sender,FXSelector,void*){
  if(isDocked() && (options&LAYOUT_SIDE_MASK)==LAYOUT_SIDE_LEFT)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


// Redock on right
long FXToolBar::onCmdDockRight(FXObject*,FXSelector,void*){
  dock(LAYOUT_SIDE_RIGHT,(FXWindow*)-1L);
  return 1;
  }

long FXToolBar::onUpdDockRight(FXObject* sender,FXSelector,void*){
  if(isDocked() && (options&LAYOUT_SIDE_MASK)==LAYOUT_SIDE_RIGHT)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


// FXToolBar grip drag started
long FXToolBar::onBeginDragGrip(FXObject*,FXSelector,void*){
  FXDCWindow dc(getRoot());
  FXint x,y;
  translateCoordinatesTo(x,y,getRoot(),0,0);
  outline.x=x;
  outline.y=y;
  outline.w=width;
  outline.h=height;
  dockafter=getPrev();
  dockside=(options&LAYOUT_SIDE_MASK);
  docking=isDocked();
  dc.clipChildren(FALSE);
  dc.setFunction(BLT_SRC_XOR_DST);
  dc.setForeground(FXRGB(255,255,255));
  dc.setLineWidth(3);
  dc.drawRectangles(&outline,1);
  getApp()->flush();
  return 1;
  }


// FXToolBar grip drag ended
long FXToolBar::onEndDragGrip(FXObject* sender,FXSelector,void* ptr){
  FXToolBarGrip *grip=(FXToolBarGrip*)sender;
  FXDCWindow dc(getRoot());
  FXint x,y;
  dc.clipChildren(FALSE);
  dc.setFunction(BLT_SRC_XOR_DST);
  dc.setForeground(FXRGB(255,255,255));
  dc.setLineWidth(3);
  dc.drawRectangles(&outline,1);
  getApp()->flush();
  if(docking){
    dock(dockside,dockafter);
    }
  else{
    undock();
    x=((FXEvent*)ptr)->root_x-((FXEvent*)ptr)->click_x-grip->getX();
    y=((FXEvent*)ptr)->root_y-((FXEvent*)ptr)->click_y-grip->getY();
    wetdock->move(x,y);
    }
  return 1;
  }


// FXToolBar grip dragged
long FXToolBar::onDraggedGrip(FXObject* sender,FXSelector,void* ptr){
  FXToolBarGrip *grip=(FXToolBarGrip*)sender;
  FXint left,right,top,bottom,x,y,twx,twy;
  FXWindow *child,*after,*harbor;
  FXRectangle newoutline;
  FXuint hints,opts;

  // Current grip location
  x=((FXEvent*)ptr)->root_x-((FXEvent*)ptr)->click_x-grip->getX();
  y=((FXEvent*)ptr)->root_y-((FXEvent*)ptr)->click_y-grip->getY();

  // Move the highlight
  newoutline.x=x;
  newoutline.y=y;
  newoutline.w=width;
  newoutline.h=height;

  // Initialize
  if(drydock && wetdock){     // We can float if not close enough to docking spot
    //newoutline.w=outline.w;
    //newoutline.h=outline.h;
    harbor=drydock;
    dockafter=NULL;
    docking=FALSE;
    }
  else{                       // If too far from docking spot, snap back to original location
    //newoutline.w=width;
    //newoutline.h=height;
    harbor=getParent();
    dockside=(options&LAYOUT_SIDE_MASK);
    dockafter=getPrev();
    docking=TRUE;
    }

  // Drydock location in root coordinates
  harbor->translateCoordinatesTo(twx,twy,getRoot(),0,0);

  // Inner bounds
  left = twx; right  = twx + harbor->getWidth();
  top  = twy; bottom = twy + harbor->getHeight();

  // Find place where to dock
  after=NULL;
  child=harbor->getFirst();
  while(left<right && top<bottom){

    // Determine docking side
    if(top<=y && y<bottom){
      if(FXABS(x-left)<PROXIMITY){
        opts=options;
        options=(options&~LAYOUT_SIDE_MASK)|LAYOUT_SIDE_LEFT;
        if(options&LAYOUT_FIX_HEIGHT) newoutline.h=height;
        else if(options&(LAYOUT_FILL_Y|LAYOUT_FILL_X)){ newoutline.h=bottom-top; newoutline.y=top; }
        else newoutline.h=getDefaultHeight();
        if(options&LAYOUT_FIX_WIDTH) newoutline.w=width;
        else newoutline.w=getWidthForHeight(newoutline.h);
        options=opts;
        newoutline.x=left;
        dockside=LAYOUT_SIDE_LEFT;
        dockafter=after;
        docking=TRUE;
        }
      if(FXABS(x-right)<PROXIMITY){
        opts=options;
        options=(options&~LAYOUT_SIDE_MASK)|LAYOUT_SIDE_RIGHT;
        if(options&LAYOUT_FIX_HEIGHT) newoutline.h=height;
        else if(options&(LAYOUT_FILL_Y|LAYOUT_FILL_X)){ newoutline.h=bottom-top; newoutline.y=top; }
        else newoutline.h=getDefaultHeight();
        if(options&LAYOUT_FIX_WIDTH) newoutline.w=width;
        else newoutline.w=getWidthForHeight(newoutline.h);
        options=opts;
        newoutline.x=right-newoutline.w;
        dockside=LAYOUT_SIDE_RIGHT;
        dockafter=after;
        docking=TRUE;
        }
      }
    if(left<=x && x<right){
      if(FXABS(y-top)<PROXIMITY){
        opts=options;
        options=(options&~LAYOUT_SIDE_MASK)|LAYOUT_SIDE_TOP;
        if(options&LAYOUT_FIX_WIDTH) newoutline.w=width;
        else if(options&(LAYOUT_FILL_X|LAYOUT_FILL_Y)){ newoutline.w=right-left; newoutline.x=left; }
        else newoutline.w=getDefaultWidth();
        if(options&LAYOUT_FIX_HEIGHT) newoutline.h=height;
        else newoutline.h=getHeightForWidth(newoutline.w);
        options=opts;
        newoutline.y=top;
        dockside=LAYOUT_SIDE_TOP;
        dockafter=after;
        docking=TRUE;
        }
      if(FXABS(y-bottom)<PROXIMITY){
        opts=options;
        options=(options&~LAYOUT_SIDE_MASK)|LAYOUT_SIDE_BOTTOM;
        if(options&LAYOUT_FIX_WIDTH) newoutline.w=width;
        else if(options&(LAYOUT_FILL_X|LAYOUT_FILL_Y)){ newoutline.w=right-left; newoutline.x=left; }
        else newoutline.w=getDefaultWidth();
        if(options&LAYOUT_FIX_HEIGHT) newoutline.h=height;
        else newoutline.h=getHeightForWidth(newoutline.w);
        options=opts;
        newoutline.y=bottom-newoutline.h;
        dockside=LAYOUT_SIDE_BOTTOM;
        dockafter=after;
        docking=TRUE;
        }
      }

    // Done
    if(!child) break;

    // Get child hints
    hints=child->getLayoutHints();

    // Some final fully stretched child also marks the end
    if((hints&LAYOUT_FILL_X) && (hints&LAYOUT_FILL_Y)) break;

    // Advance inward
    if(child!=this){
      if(child->shown()){

        // Vertical
        if(hints&LAYOUT_SIDE_LEFT){
          if(!((hints&LAYOUT_RIGHT)&&(hints&LAYOUT_CENTER_X))){
            if(hints&LAYOUT_SIDE_BOTTOM){
              right=twx+child->getX();
              }
            else{
              left=twx+child->getX()+child->getWidth();
              }
            }
          }

        // Horizontal
        else{
          if(!((hints&LAYOUT_BOTTOM)&&(hints&LAYOUT_CENTER_Y))){
            if(hints&LAYOUT_SIDE_BOTTOM){
              bottom=twy+child->getY();
              }
            else{
              top=twy+child->getY()+child->getHeight();
              }
            }
          }
        }
      }
    after=child;

    // Next one
    child=child->getNext();
    }

  // Did rectangle move?
  if(newoutline.x!=outline.x || newoutline.y!=outline.y || newoutline.w!=outline.w || newoutline.h!=outline.h){
    FXDCWindow dc(getRoot());
    dc.clipChildren(FALSE);
    dc.setFunction(BLT_SRC_XOR_DST);
    dc.setForeground(FXRGB(255,255,255));
    dc.setLineWidth(3);
    dc.drawRectangles(&outline,1);
    outline=newoutline;
    dc.drawRectangles(&outline,1);
    getApp()->flush();
    }
  return 1;
  }


// Save data
void FXToolBar::save(FXStream& store) const {
  FXPacker::save(store);
  store << drydock;
  store << wetdock;
  }


// Load data
void FXToolBar::load(FXStream& store){
  FXPacker::load(store);
  store >> drydock;
  store >> wetdock;
  }


// Destroy
FXToolBar::~FXToolBar(){
  drydock=(FXComposite*)-1L;
  wetdock=(FXComposite*)-1L;
  dockafter=(FXWindow*)-1L;
  }

}
