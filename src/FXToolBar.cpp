/********************************************************************************
*                                                                               *
*                        D o c k B a r   W i d g e t                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXToolBar.cpp,v 1.34 2005/02/01 07:02:49 fox Exp $                       *
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
#include "FXGIFIcon.h"
#include "FXDCWindow.h"
#include "FXDrawable.h"
#include "FXWindow.h"
#include "FXFrame.h"
#include "FXComposite.h"
#include "FXPacker.h"
#include "FXToolBar.h"
#include "FXPopup.h"
#include "FXMenuPane.h"
#include "FXMenuCaption.h"
#include "FXMenuCommand.h"
#include "FXMenuCascade.h"
#include "FXMenuSeparator.h"
#include "FXMenuRadio.h"
#include "FXMenuCheck.h"
#include "FXShell.h"
#include "FXSeparator.h"
#include "FXTopWindow.h"
#include "FXToolBarDock.h"
#include "FXToolBarGrip.h"
#include "FXToolBarShell.h"
#include "icons.h"


/*
  Notes:
  - May want to add support for centered layout mode.
*/



// Docking side
#define LAYOUT_SIDE_MASK (LAYOUT_SIDE_LEFT|LAYOUT_SIDE_RIGHT|LAYOUT_SIDE_TOP|LAYOUT_SIDE_BOTTOM)

using namespace FX;

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
  FXMAPFUNC(SEL_TIMEOUT,FXToolBar::ID_DOCK_TIMER,FXToolBar::onDockTimer),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,FXToolBar::ID_TOOLBARGRIP,FXToolBar::onPopupMenu),
  };


// Object implementation
FXIMPLEMENT(FXToolBar,FXPacker,FXToolBarMap,ARRAYNUMBER(FXToolBarMap))


// Deserialization
FXToolBar::FXToolBar():drydock(NULL),wetdock(NULL){
  flags|=FLAG_ENABLED;
  }


// Make a dockable and, possibly, floatable toolbar
FXToolBar::FXToolBar(FXComposite* p,FXComposite* q,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):
  FXPacker(p,opts,x,y,w,h,pl,pr,pt,pb,hs,vs),drydock(p),wetdock(q){
  flags|=FLAG_ENABLED;
  }


// Make a non-floatable toolbar
FXToolBar::FXToolBar(FXComposite* p,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):
  FXPacker(p,opts,x,y,w,h,pl,pr,pt,pb,hs,vs),drydock(NULL),wetdock(NULL){
  flags|=FLAG_ENABLED;
  }


// Compute minimum width based on child layout hints
FXint FXToolBar::getDefaultWidth(){
  register FXint total=0,mw=0,w;
  register FXWindow* child;
  register FXuint hints;
  if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(dynamic_cast<FXSeparator*>(child) || dynamic_cast<FXToolBarGrip*>(child)) w=child->getDefaultWidth();
      else if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
      else if(options&PACK_UNIFORM_WIDTH) w=mw;
      else w=child->getDefaultWidth();
      if(!(options&LAYOUT_SIDE_LEFT)){  // Horizontal
        if(total) total+=hspacing;
        total+=w;
        }
      else{                             // Vertical
        if(total<w) total=w;
        }
      }
    }
  return padleft+padright+total+(border<<1);
  }


// Compute minimum height based on child layout hints
FXint FXToolBar::getDefaultHeight(){
  register FXint total=0,mh=0,h;
  register FXWindow* child;
  register FXuint hints;
  if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(dynamic_cast<FXSeparator*>(child) || dynamic_cast<FXToolBarGrip*>(child)) h=child->getDefaultHeight();
      else if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
      else if(options&PACK_UNIFORM_HEIGHT) h=mh;
      else h=child->getDefaultHeight();
      if(options&LAYOUT_SIDE_LEFT){     // Vertical
        if(total) total+=vspacing;
        total+=h;
        }
      else{                             // Horizontal
        if(total<h) total=h;
        }
      }
    }
  return padtop+padbottom+total+(border<<1);
  }


/*
// Return width for given height
FXint FXToolBar::getWidthForHeight(FXint givenheight){
  register FXint wtot,wmax,hcum,w,h,space,ngalleys,mw=0,mh=0;
  register FXWindow* child;
  register FXuint hints;
  wtot=wmax=hcum=ngalleys=0;
  space=givenheight-padtop-padbottom-(border<<1);
  if(space<1) space=1;
  if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();
  if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(dynamic_cast<FXToolBarGrip*>(child)) w=child->getDefaultWidth();
      else if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
      else if(options&PACK_UNIFORM_WIDTH) w=mw;
      else w=child->getDefaultWidth();
      if(dynamic_cast<FXToolBarGrip*>(child)) h=child->getDefaultHeight();
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
  register FXint htot,hmax,wcum,w,h,space,ngalleys,mw=0,mh=0;
  register FXWindow* child;
  register FXuint hints;
  htot=hmax=wcum=ngalleys=0;
  space=givenwidth-padleft-padright-(border<<1);
  if(space<1) space=1;
  if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();
  if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();
  for(child=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      if(dynamic_cast<FXToolBarGrip*>(child)) w=child->getDefaultWidth();
      else if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
      else if(options&PACK_UNIFORM_WIDTH) w=mw;
      else w=child->getDefaultWidth();
      if(dynamic_cast<FXToolBarGrip*>(child)) h=child->getDefaultHeight();
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

*/


// Recalculate layout
void FXToolBar::layout(){
  register FXint left,right,top,bottom,remain,expand,mw=0,mh=0,x,y,w,h,e,t;
  register FXWindow *child;
  register FXuint hints;

  // Placement rectangle; right/bottom non-inclusive
  left=border+padleft;
  right=width-border-padright;
  top=border+padtop;
  bottom=height-border-padbottom;

  // Get maximum child size
  if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();
  if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();

  // Vertical toolbar
  if(options&LAYOUT_SIDE_LEFT){

    // Find stretch
    for(child=getFirst(),remain=bottom-top,expand=0; child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        if(dynamic_cast<FXSeparator*>(child) || dynamic_cast<FXToolBarGrip*>(child)) h=child->getDefaultHeight();
        else if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=mh;
        else h=child->getDefaultHeight();
	if(hints&LAYOUT_FILL_Y)
	  expand+=h;
	else
	  remain-=h;
	remain-=vspacing;
        }
      }

    // Adjust
    remain+=vspacing;

    // Placement
    for(child=getFirst(),e=0; child; child=child->getNext()){
      if(child->shown()){

        hints=child->getLayoutHints();

        // Determine child width
        if(dynamic_cast<FXSeparator*>(child) || dynamic_cast<FXToolBarGrip*>(child)) w=right-left;
        else if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=mw;
        else if(hints&LAYOUT_FILL_X) w=right-left;
        else w=child->getDefaultWidth();

        // Determine child x-position
        if(hints&LAYOUT_CENTER_X) x=left+(right-left-w)/2;
        else if(hints&LAYOUT_RIGHT) x=right-w;
        else x=left;

        // Determine child height
        if(dynamic_cast<FXToolBarGrip*>(child)) h=child->getDefaultHeight();
        else if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=mh;
        else h=child->getDefaultHeight();

        // Account for fill or center
	if(hints&LAYOUT_FILL_Y){
          t=h*remain;
          e+=t%expand;
          h=t/expand+e/expand;
          e%=expand;
          }

        // Determine child x-position
        if(hints&LAYOUT_BOTTOM){
          y=bottom-h;
          bottom-=h+vspacing;
          }
        else{
          y=top;
          top+=h+vspacing;
          }

        // Place it
        child->position(x,y,w,h);
        }
      }
    }

  // Horizontal toolbar
  else{

    // Find stretch
    for(child=getFirst(),remain=right-left,expand=0; child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        if(dynamic_cast<FXSeparator*>(child) || dynamic_cast<FXToolBarGrip*>(child)) w=child->getDefaultWidth();
        else if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=mw;
        else w=child->getDefaultWidth();
	if(hints&LAYOUT_FILL_X)
	  expand+=w;
	else
	  remain-=w;
	remain-=hspacing;
        }
      }

    // Adjust
    remain+=hspacing;

    // Placement
    for(child=getFirst(),e=0; child; child=child->getNext()){
      if(child->shown()){

        hints=child->getLayoutHints();

        // Determine child height
        if(dynamic_cast<FXSeparator*>(child) || dynamic_cast<FXToolBarGrip*>(child)) h=bottom-top;
        else if(hints&LAYOUT_FIX_HEIGHT) h=child->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=mh;
        else if(hints&LAYOUT_FILL_Y) h=bottom-top;
        else h=child->getDefaultHeight();

        // Determine child y-position
        if(hints&LAYOUT_CENTER_Y) y=top+(bottom-top-h)/2;
        else if(hints&LAYOUT_BOTTOM) y=bottom-h;
        else y=top;

        // Determine child width
        if(dynamic_cast<FXToolBarGrip*>(child)) w=child->getDefaultWidth();
        else if(hints&LAYOUT_FIX_WIDTH) w=child->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=mw;
        else w=child->getDefaultWidth();

        // Account for fill or center
	if(hints&LAYOUT_FILL_X){
          t=w*remain;
          e+=t%expand;
          w=t/expand+e/expand;
          e%=expand;
          }

        // Determine child x-position
        if(hints&LAYOUT_RIGHT){
          x=right-w;
          right-=w+hspacing;
          }
        else{
          x=left;
          left+=w+hspacing;
          }

        // Place it
        child->position(x,y,w,h);
        }
      }
    }
  flags&=~FLAG_DIRTY;
  }


// Return true if toolbar is docked
FXbool FXToolBar::isDocked() const {
  return (getParent()!=(FXWindow*)wetdock);
  }


// Set parent when docked, if it was docked it will remain docked
void FXToolBar::setDryDock(FXComposite* dry){
  if(dry && dry->id() && getParent()==(FXWindow*)drydock){
    reparent(dry,NULL);
    }
  drydock=dry;
  }


// Set parent when floating
void FXToolBar::setWetDock(FXComposite* wet){
  if(wet && wet->id() && getParent()==(FXWindow*)wetdock){
    reparent(wet,NULL);
    }
  wetdock=wet;
  }


// Dock the bar before other window
void FXToolBar::dock(FXToolBarDock* docksite,FXWindow* before){
  if(docksite && getParent()!=docksite){
    setDockingSide(docksite->getLayoutHints());
    setDryDock(docksite);
    reparent(docksite,before);
    wetdock->hide();
    }
  }


// Dock the bar near position in dock site
void FXToolBar::dock(FXToolBarDock* docksite,FXint localx,FXint localy){
  if(docksite && getParent()!=docksite){
    setDockingSide(docksite->getLayoutHints());
    setDryDock(docksite);
    reparent(docksite,NULL);
    docksite->moveToolBar(this,localx,localy);
    wetdock->hide();
    }
  }


// Undock the bar
void FXToolBar::undock(FXint rootx,FXint rooty){
  if(wetdock && isDocked()){
    reparent(wetdock);
    setLayoutHints(getLayoutHints()&~LAYOUT_DOCK_NEXT);
    wetdock->position(rootx,rooty,wetdock->getDefaultWidth(),wetdock->getDefaultHeight());
    wetdock->show();
    }
  }


// Search siblings of drydock for first dock opportunity
FXToolBarDock* FXToolBar::findDockAtSide(FXuint side){
  register FXToolBarDock* docksite;
  register FXWindow *child;
  if(drydock){
    child=drydock->getParent()->getFirst();
    while(child){
      docksite=dynamic_cast<FXToolBarDock*>(child);
      if(docksite && docksite->shown() && side==(docksite->getLayoutHints()&LAYOUT_SIDE_MASK)) return docksite;
      child=child->getNext();
      }
    }
  return NULL;
  }


// Search siblings of drydock for dock opportunity near given coordinates
FXToolBarDock* FXToolBar::findDockNear(FXint rootx,FXint rooty){
  register FXToolBarDock *docksite;
  register FXWindow *child;
  FXint localx,localy;
  if(drydock){

/*
    // Translate without pain; this can be put pack if we make sure
    // we always have correct position data in FXTopWindow after a maximize
    // or minimize operation.
    for(child=drydock->getParent(),localx=rootx,localy=rooty; child!=getRoot(); child=child->getParent()){
      localx-=child->getX();
      localy-=child->getY();
      }
*/
    drydock->getParent()->translateCoordinatesFrom(localx,localy,getRoot(),rootx,rooty);

    // Localize dock site
    child=drydock->getParent()->getFirst();
    while(child){
      docksite=dynamic_cast<FXToolBarDock*>(child);
      if(docksite && docksite->shown() && docksite->dockToolBar(this,localx,localy)) return docksite;
      child=child->getNext();
      }
    }
  return NULL;
  }


// Undock
long FXToolBar::onCmdUndock(FXObject*,FXSelector,void*){
  FXint rootx,rooty;
  translateCoordinatesTo(rootx,rooty,getRoot(),8,8);
  undock(rootx,rooty);
  return 1;
  }


// Check if undocked
long FXToolBar::onUpdUndock(FXObject* sender,FXSelector,void*){
  sender->handle(this,(wetdock && wetdock!=getParent())?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
  return 1;
  }


// Redock on top
long FXToolBar::onCmdDockTop(FXObject*,FXSelector,void*){
  dock(findDockAtSide(LAYOUT_SIDE_TOP),NULL);
  return 1;
  }


// Check if docked at top
long FXToolBar::onUpdDockTop(FXObject* sender,FXSelector,void*){
  FXToolBarDock* docksite=findDockAtSide(LAYOUT_SIDE_TOP);
  sender->handle(this,(docksite && docksite!=getParent())?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
  return 1;
  }


// Redock on bottom
long FXToolBar::onCmdDockBottom(FXObject*,FXSelector,void*){
  dock(findDockAtSide(LAYOUT_SIDE_BOTTOM),NULL);
  return 1;
  }


// Check if docked at bottom
long FXToolBar::onUpdDockBottom(FXObject* sender,FXSelector,void*){
  FXToolBarDock* docksite=findDockAtSide(LAYOUT_SIDE_BOTTOM);
  sender->handle(this,(docksite && docksite!=getParent())?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
  return 1;
  }


// Redock on left
long FXToolBar::onCmdDockLeft(FXObject*,FXSelector,void*){
  dock(findDockAtSide(LAYOUT_SIDE_LEFT),NULL);
  return 1;
  }


// Check if docked at left
long FXToolBar::onUpdDockLeft(FXObject* sender,FXSelector,void*){
  FXToolBarDock* docksite=findDockAtSide(LAYOUT_SIDE_LEFT);
  sender->handle(this,(docksite && docksite!=getParent())?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
  return 1;
  }


// Redock on right
long FXToolBar::onCmdDockRight(FXObject*,FXSelector,void*){
  dock(findDockAtSide(LAYOUT_SIDE_RIGHT),NULL);
  return 1;
  }


// Check if docked at right
long FXToolBar::onUpdDockRight(FXObject* sender,FXSelector,void*){
  FXToolBarDock* docksite=findDockAtSide(LAYOUT_SIDE_RIGHT);
  sender->handle(this,(docksite && docksite!=getParent())?FXSEL(SEL_COMMAND,ID_ENABLE):FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
  return 1;
  }


// Right clicked on bar
long FXToolBar::onPopupMenu(FXObject*,FXSelector,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  if(event->moved) return 1;
  FXMenuPane dockmenu(this);
  FXGIFIcon docktopicon(getApp(),docktop,FXRGB(255,255,255),IMAGE_ALPHACOLOR);
  FXGIFIcon dockbottomicon(getApp(),dockbottom,FXRGB(255,255,255),IMAGE_ALPHACOLOR);
  FXGIFIcon docklefticon(getApp(),dockleft,FXRGB(255,255,255),IMAGE_ALPHACOLOR);
  FXGIFIcon dockrighticon(getApp(),dockright,FXRGB(255,255,255),IMAGE_ALPHACOLOR);
  FXGIFIcon dockfreeicon(getApp(),dockfree,FXRGB(255,255,255),IMAGE_ALPHACOLOR);
  new FXMenuCaption(&dockmenu,"Docking");
  new FXMenuSeparator(&dockmenu);
  new FXMenuCommand(&dockmenu,"Top",&docktopicon,this,ID_DOCK_TOP);
  new FXMenuCommand(&dockmenu,"Bottom",&dockbottomicon,this,ID_DOCK_BOTTOM);
  new FXMenuCommand(&dockmenu,"Left",&docklefticon,this,ID_DOCK_LEFT);
  new FXMenuCommand(&dockmenu,"Right",&dockrighticon,this,ID_DOCK_RIGHT);
  new FXMenuCommand(&dockmenu,"Float",&dockfreeicon,this,ID_UNDOCK);
  dockmenu.create();
  dockmenu.popup(NULL,event->root_x,event->root_y);
  // FIXME funny problem: menu doesn't update until move despite call to refresh here
  getApp()->refresh();
  getApp()->runModalWhileShown(&dockmenu);
  return 1;
  }


// Tool bar grip drag started
long FXToolBar::onBeginDragGrip(FXObject* sender,FXSelector,void* ptr){
  FXWindow *grip=static_cast<FXWindow*>(sender);
  FXEvent* event=static_cast<FXEvent*>(ptr);
  if(dynamic_cast<FXToolBarShell*>(getParent()) || dynamic_cast<FXToolBarDock*>(getParent())){
    gripx=event->click_x+grip->getX();
    gripy=event->click_y+grip->getY();
    raise();
    return 1;
    }
  return 0;
  }


// Tool bar grip drag ended
long FXToolBar::onEndDragGrip(FXObject*,FXSelector,void* ptr){
  FXToolBarShell *toolbarshell=dynamic_cast<FXToolBarShell*>(getParent());
  FXEvent* event=static_cast<FXEvent*>(ptr);
  FXToolBarDock *toolbardock;
  FXint rootx,rooty;

  // Root position
  rootx=event->root_x-gripx;
  rooty=event->root_y-gripy;

  // Stop dock timer
  getApp()->removeTimeout(this,ID_DOCK_TIMER);

  // We are floating
  if(toolbarshell){
    toolbardock=findDockNear(rootx,rooty);
    if(toolbardock) dock(toolbardock);
    }
  return 1;
  }


// Hovered near dock site:- dock it!
long FXToolBar::onDockTimer(FXObject*,FXSelector,void* ptr){
  FXToolBarDock *toolbardock=static_cast<FXToolBarDock*>(ptr);
  FXint localx,localy;
  translateCoordinatesTo(localx,localy,toolbardock,0,0);
  dock(toolbardock,localx,localy);
  return 1;
  }


// Tool bar grip dragged
long FXToolBar::onDraggedGrip(FXObject*,FXSelector,void* ptr){
  FXToolBarShell *toolbarshell=dynamic_cast<FXToolBarShell*>(getParent());
  FXToolBarDock *toolbardock=dynamic_cast<FXToolBarDock*>(getParent());
  FXEvent* event=static_cast<FXEvent*>(ptr);
  FXint rootx,rooty,dockx,docky;

  // Root position
  rootx=event->root_x-gripx;
  rooty=event->root_y-gripy;

  // Stop dock timer
  getApp()->removeTimeout(this,ID_DOCK_TIMER);

  // We are docked
  if(toolbardock){

    // Get mouse position relative to dock site
    toolbardock->translateCoordinatesFrom(dockx,docky,getRoot(),rootx,rooty);

    // Move in dock site, undock if we've pulled it out of the dock
    if(!toolbardock->moveToolBar(this,dockx,docky)){
      undock(rootx,rooty);
      }
    }

  // We are floating
  else if(toolbarshell){

    // We're near a dock, if we hover around we'll dock there
    toolbardock=findDockNear(rootx,rooty);
    if(toolbardock) getApp()->addTimeout(this,ID_DOCK_TIMER,200,toolbardock);

    // Move around freely
    wetdock->move(rootx,rooty);
    }

  return 1;
  }


// Change toolbar orientation
void FXToolBar::setDockingSide(FXuint side){
  side&=LAYOUT_SIDE_MASK;
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
  getApp()->removeTimeout(this,ID_DOCK_TIMER);
  drydock=(FXComposite*)-1L;
  wetdock=(FXComposite*)-1L;
  }

}
