/********************************************************************************
*                                                                               *
*                         D o c k   S i t e   W i d g e t                       *
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
* $Id: FXToolBarDock.cpp,v 1.12 2005/02/01 07:02:49 fox Exp $                   *
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
#include "FXApp.h"
#include "FXPacker.h"
#include "FXToolBar.h"
#include "FXToolBarDock.h"


/*
  Notes:
  - This layout manager probably has the most complicated algorithm of all
    the layout managers in FOX.
  - Use "Box-Car" algorithm when sliding horizontally (vertically).
  - Vertical arrangement is very tricky; we don't insert in between
    galleys when dragging since its not so easy to use; but this remains
    a possibility for future expansion.
  - We can STILL do wrapping of individual toolbars inside the toolbar dock;
    normally we compute the width the standard way, but if this does not
    fit the available space and its the first widget on the galley, we
    can call getHeightForWidth() and thereby wrap the item in on the
    galley.  Thus we have both wrapping of the toobars as well as
    wrapping inside the toolbar.
*/

// How close to edge before considered docked
#define PROXIMITY    10
#define TOLERANCE    30

using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXToolBarDock) FXToolBarDockMap[]={
  FXMAPFUNC(SEL_FOCUS_PREV,0,FXToolBarDock::onFocusLeft),
  FXMAPFUNC(SEL_FOCUS_NEXT,0,FXToolBarDock::onFocusRight),
  };


// Object implementation
FXIMPLEMENT(FXToolBarDock,FXPacker,FXToolBarDockMap,ARRAYNUMBER(FXToolBarDockMap))


// Make a dock site
FXToolBarDock::FXToolBarDock(FXComposite* p,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):
  FXPacker(p,opts,x,y,w,h,pl,pr,pt,pb,hs,vs){
  }


// Return width for given height (vertical orientation)
FXint FXToolBarDock::getWidthForHeight(FXint givenheight){
  register FXint space,total,galh,galw,any,w,h;
  register FXWindow *child,*begin;
  register FXuint hints;
  total=galh=galw=any=0;
  space=givenheight-padtop-padbottom-(border<<1);
  for(child=begin=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      w=(hints&LAYOUT_FIX_WIDTH)?child->getWidth():child->getDefaultWidth();
      h=(hints&LAYOUT_FIX_HEIGHT)?child->getHeight():child->getDefaultHeight();
      if(any && ((galh+h>space) || (hints&LAYOUT_DOCK_NEXT))){
        total+=galw+hspacing;
        begin=child;
        galw=w;
        galh=h+vspacing;
        }
      else{
        galh+=h+vspacing;
        if(w>galw) galw=w;
        }
      any=1;
      }
    }
  total+=galw;
  return padleft+padright+total+(border<<1);
  }


// Return height for given width (horizontal orientation)
FXint FXToolBarDock::getHeightForWidth(FXint givenwidth){
  register FXint space,total,galh,galw,any,w,h;
  register FXWindow *child,*begin;
  register FXuint hints;
  total=galh=galw=any=0;
  space=givenwidth-padleft-padright-(border<<1);
  for(child=begin=getFirst(); child; child=child->getNext()){
    if(child->shown()){
      hints=child->getLayoutHints();
      w=(hints&LAYOUT_FIX_WIDTH)?child->getWidth():child->getDefaultWidth();
      h=(hints&LAYOUT_FIX_HEIGHT)?child->getHeight():child->getDefaultHeight();
      if(any && ((galw+w>space) || (hints&LAYOUT_DOCK_NEXT))){
        total+=galh+vspacing;
        begin=child;
        galw=w+hspacing;
        galh=h;
        }
      else{
        galw+=w+hspacing;
        if(h>galh) galh=h;
        }
      any=1;
      }
    }
  total+=galh;
  return padtop+padbottom+total+(border<<1);
  }


// Compute minimum width based on child layout hints
FXint FXToolBarDock::getDefaultWidth(){
  register FXint total=0,galw=0,any=0,w;
  register FXWindow *child,*begin;
  register FXuint hints;

  // Vertically oriented
  if(options&LAYOUT_SIDE_LEFT){
    for(child=begin=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        w=(hints&LAYOUT_FIX_WIDTH)?child->getWidth():child->getDefaultWidth();
        if(any && (hints&LAYOUT_DOCK_NEXT)){
          total+=galw+hspacing;
          begin=child;
          galw=w;
          }
        else{
          if(w>galw) galw=w;
          }
        any=1;
        }
      }
    total+=galw;
    }

  // Horizontally oriented
  else{
    for(child=begin=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        w=(hints&LAYOUT_FIX_WIDTH)?child->getWidth():child->getDefaultWidth();
        if(any && (hints&LAYOUT_DOCK_NEXT)){
          if(galw>total) total=galw;
          begin=child;
          galw=w;
          }
        else{
          if(galw) galw+=hspacing;
          galw+=w;
          }
        any=1;
        }
      }
    if(galw>total) total=galw;
    }
  return padleft+padright+total+(border<<1);
  }


// Compute minimum height based on child layout hints
FXint FXToolBarDock::getDefaultHeight(){
  register FXint total=0,galh=0,any=0,h;
  register FXWindow *child,*begin;
  register FXuint hints;

  // Vertically oriented
  if(options&LAYOUT_SIDE_LEFT){
    for(child=begin=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        h=(hints&LAYOUT_FIX_HEIGHT)?child->getHeight():child->getDefaultHeight();
        if(any && (hints&LAYOUT_DOCK_NEXT)){
          if(galh>total) total=galh;
          begin=child;
          galh=h;
          }
        else{
          if(galh) galh+=vspacing;
          galh+=h;
          }
        any=1;
        }
      }
    if(galh>total) total=galh;
    }

  // Horizontally oriented
  else{
    for(child=begin=getFirst(); child; child=child->getNext()){
      if(child->shown()){
        hints=child->getLayoutHints();
        h=(hints&LAYOUT_FIX_HEIGHT)?child->getHeight():child->getDefaultHeight();
        if(any && (hints&LAYOUT_DOCK_NEXT)){
          total+=galh+vspacing;
          begin=child;
          galh=h;
          }
        else{
          if(h>galh) galh=h;
          }
        any=1;
        }
      }
    total+=galh;
    }
  return padtop+padbottom+total+(border<<1);
  }


// Determine vertical galley size
FXint FXToolBarDock::galleyWidth(FXWindow *begin,FXWindow*& end,FXint space,FXint& require,FXint& expand) const {
  register FXint galley,any,w,h;
  register FXWindow *child;
  register FXuint hints;
  require=expand=galley=any=0;
  for(child=end=begin; child; end=child,child=child->getNext()){
    if(child->shown()){

      // Get size
      hints=child->getLayoutHints();
      w=(hints&LAYOUT_FIX_WIDTH)?child->getWidth():child->getDefaultWidth();
      h=(hints&LAYOUT_FIX_HEIGHT)?child->getHeight():child->getDefaultHeight();

      // Break for new galley?
      if(any && ((require+h>space) || (hints&LAYOUT_DOCK_NEXT))) break;

      // Expanding widgets
      if(hints&LAYOUT_FILL_Y) expand+=h;

      // Figure galley size
      require+=h+vspacing;
      if(w>galley) galley=w;
      any=1;
      }
    }
  require-=vspacing;
  return galley;
  }


// Determine horizontal galley size
FXint FXToolBarDock::galleyHeight(FXWindow *begin,FXWindow*& end,FXint space,FXint& require,FXint& expand) const {
  register FXint galley,any,w,h;
  register FXWindow *child;
  register FXuint hints;
  require=expand=galley=any=0;
  for(child=end=begin; child; end=child,child=child->getNext()){
    if(child->shown()){

      // Get size
      hints=child->getLayoutHints();
      w=(hints&LAYOUT_FIX_WIDTH)?child->getWidth():child->getDefaultWidth();
      h=(hints&LAYOUT_FIX_HEIGHT)?child->getHeight():child->getDefaultHeight();

      // Break for new galley?
      if(any && ((require+w>space) || (hints&LAYOUT_DOCK_NEXT))) break;

      // Expanding widgets
      if(hints&LAYOUT_FILL_X) expand+=w;

      // Figure galley size
      require+=w+hspacing;
      if(h>galley) galley=h;
      any=1;
      }
    }
  require-=hspacing;
  return galley;
  }


// Recalculate layout
void FXToolBarDock::layout(){
  FXint expand,remain,require,left,right,top,bottom,galx,galy,galw,galh,e,t,x,y,w,h;
  FXWindow *begin,*end,*child;
  FXuint hints;

  // Vertically oriented
  if(options&LAYOUT_SIDE_LEFT){

    // Galley height
    left=border+padleft;
    right=width-padright-border;

    // Loop over galleys
    for(begin=getFirst(); begin; begin=end->getNext()){

      // Space available
      top=border+padtop;
      bottom=height-padbottom-border;

      // Galley width
      galw=galleyWidth(begin,end,bottom-top,require,expand);

      // Remaining space
      remain=bottom-top-require;
      if(expand) require=bottom-top;

      // Start next galley
      galx=left;
      left+=galw+hspacing;

      // Placement of widgets on galley
      for(child=begin,e=0; child; child=child->getNext()){
        if(child->shown()){

          // Get size
          hints=child->getLayoutHints();
          w=(hints&LAYOUT_FIX_WIDTH)?child->getWidth():child->getDefaultWidth();
          h=(hints&LAYOUT_FIX_HEIGHT)?child->getHeight():child->getDefaultHeight();

          // X-filled
          if(hints&LAYOUT_FILL_X) w=galw;

          // Y-filled
          if(hints&LAYOUT_FILL_Y){
            t=h*remain;
            e+=t%expand;
            h+=t/expand+e/expand;
            e%=expand;
            }

          require-=h;

          // Determine child x-position
          x=child->getX();
          if(x<galx) x=galx;
          if(x+w>galx+galw) x=galx+galw-w;

          // Determine child y-position
          y=child->getY();
          if(y+h>bottom-require) y=bottom-require-h;
          if(y<top) y=top;
          top=y+h+vspacing;

          require-=vspacing;

          // Placement on this galley
          child->position(x,y,w,h);
          }
        if(child==end) break;
        }
      }
    }

  // Horizontally oriented
  else{

    // Galley height
    top=border+padtop;
    bottom=height-padbottom-border;

    // Loop over galleys
    for(begin=getFirst(); begin; begin=end->getNext()){

      // Space available
      left=border+padleft;
      right=width-padright-border;

      // Galley height
      galh=galleyHeight(begin,end,right-left,require,expand);

      // Remaining space
      remain=right-left-require;
      if(expand) require=right-left;

      // Start next galley
      galy=top;
      top+=galh+vspacing;

      // Placement of widgets on galley
      for(child=begin,e=0; child; child=child->getNext()){
        if(child->shown()){

          // Get size
          hints=child->getLayoutHints();
          w=(hints&LAYOUT_FIX_WIDTH)?child->getWidth():child->getDefaultWidth();
          h=(hints&LAYOUT_FIX_HEIGHT)?child->getHeight():child->getDefaultHeight();

          // Y-filled
          if(hints&LAYOUT_FILL_Y) h=galh;

          // X-filled
          if(hints&LAYOUT_FILL_X){
            t=w*remain;
            e+=t%expand;
            w+=t/expand+e/expand;
            e%=expand;
            }

          require-=w;

          // Determine child y-position
          y=child->getY();
          if(y<galy) y=galy;
          if(y+h>galy+galh) y=galy+galh-h;

          // Determine child x-position
          x=child->getX();
          if(x+w>right-require) x=right-require-w;
          if(x<left) x=left;
          left=x+w+hspacing;

          require-=hspacing;

          // Placement on this galley
          child->position(x,y,w,h);
          }
        if(child==end) break;
        }
      }
    }
  flags&=~FLAG_DIRTY;
  }


// Move bar vertically
void FXToolBarDock::moveVerBar(FXWindow* bar,FXWindow *begin,FXWindow* end,FXint bx,FXint by){
  FXint minpos,maxpos,pos;
  FXWindow *child,*other;

  // Pushing up
  if(by<bar->getY()){

    // Figure minimum position
    for(child=begin,minpos=border+padtop; child; child=child->getNext()){
      if(child->shown()){ minpos+=child->getHeight()+vspacing; }
      if(child==bar) break;
      }

    // Move bars in box-car fashion
    for(child=bar,pos=by+bar->getHeight()+vspacing,other=NULL; child; child=child->getPrev()){
      if(child->shown()){
        minpos=minpos-child->getHeight()-vspacing;
        pos=pos-child->getHeight()-vspacing;
        if(child->getY()<=pos) break;
        if(by<child->getY()) other=child;
        child->move((child==bar)?bx:child->getX(),FXMAX(pos,minpos));
        }
      if(child==begin) break;
      }

    // Hop bar over other if top of bar is above of top of other
    if(other && other!=bar){

      // Hopping over first on galley:- transfer flag over to new first
      if((other==begin) && (other->getLayoutHints()&LAYOUT_DOCK_NEXT)){
        other->setLayoutHints(other->getLayoutHints()&~LAYOUT_DOCK_NEXT);
        bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
        }

      // And rearrange order of children
      bar->move(bar->getX(),other->getY());
      other->move(other->getX(),bar->getY()+bar->getHeight()+vspacing);
      bar->reparent(this,other);
      }
    }

  // Pushing down
  else if(by>bar->getY()){

    // Figure maximum position
    for(child=end,maxpos=height-padbottom-border; child; child=child->getPrev()){
      if(child->shown()){ maxpos-=child->getHeight()+vspacing; }
      if(child==bar) break;
      }

    // Move bars in box-car fashion
    for(child=bar,pos=by,other=NULL; child; child=child->getNext()){
      if(child->shown()){
        if(pos<=child->getY()) break;
        if(by+bar->getHeight()>child->getY()+child->getHeight()) other=child;
        child->move((child==bar)?bx:child->getX(),FXMIN(pos,maxpos));
        maxpos=maxpos+child->getHeight()+vspacing;
        pos=pos+child->getHeight()+vspacing;
        }
      if(child==end) break;
      }

    // Hop bar over other if bottom of bar is below of bottom of other
    if(other && other!=bar){

      // First on galley hopped over to the right:- transfer flag to new first
      if((bar==begin) && (bar->getLayoutHints()&LAYOUT_DOCK_NEXT)){
        bar->setLayoutHints(bar->getLayoutHints()&~LAYOUT_DOCK_NEXT);
        other->setLayoutHints(other->getLayoutHints()|LAYOUT_DOCK_NEXT);
        }

      // And rearrange order of children
      bar->move(bar->getX(),other->getY()+other->getHeight()-bar->getHeight());
      other->move(other->getX(),bar->getY()-other->getHeight()-vspacing);
      bar->reparent(this,other->getNext());
      }
    }

  // Move horizontally
  else{
    bar->move(bx,bar->getY());
    }
  }


// Move bar horizontally
void FXToolBarDock::moveHorBar(FXWindow* bar,FXWindow *begin,FXWindow* end,FXint bx,FXint by){
  FXint minpos,maxpos,pos;
  FXWindow *child,*other;

  // Pushing left
  if(bx<bar->getX()){

    // Figure minimum position
    for(child=begin,minpos=border+padleft; child; child=child->getNext()){
      if(child->shown()){ minpos+=child->getWidth()+hspacing; }
      if(child==bar) break;
      }

    // Move bars in box-car fashion
    for(child=bar,pos=bx+bar->getWidth()+hspacing,other=NULL; child; child=child->getPrev()){
      if(child->shown()){
        minpos=minpos-child->getWidth()-hspacing;
        pos=pos-child->getWidth()-hspacing;
        if(child->getX()<=pos) break;
        if(bx<child->getX()) other=child;
        child->move(FXMAX(pos,minpos),(child==bar)?by:child->getY());
        }
      if(child==begin) break;
      }

    // Hop bar over other if left of bar is leftward of left of other
    if(other && other!=bar){

      // Hopping over first on galley:- transfer flag over to new first
      if((other==begin) && (other->getLayoutHints()&LAYOUT_DOCK_NEXT)){
        other->setLayoutHints(other->getLayoutHints()&~LAYOUT_DOCK_NEXT);
        bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
        }

      // And rearrange order of children
      bar->move(other->getX(),bar->getY());
      other->move(bar->getX()+bar->getWidth()+hspacing,other->getY());
      bar->reparent(this,other);
      }
    }

  // Pushing right
  else if(bx>bar->getX()){

    // Figure maximum position
    for(child=end,maxpos=width-padright-border; child; child=child->getPrev()){
      if(child->shown()){ maxpos-=child->getWidth()+hspacing; }
      if(child==bar) break;
      }

    // Move bars in box-car fashion
    for(child=bar,pos=bx,other=NULL; child; child=child->getNext()){
      if(child->shown()){
        if(pos<=child->getX()) break;
        if(bx+bar->getWidth()>child->getX()+child->getWidth()) other=child;
        child->move(FXMIN(pos,maxpos),(child==bar)?by:child->getY());
        maxpos=maxpos+child->getWidth()+hspacing;
        pos=pos+child->getWidth()+hspacing;
        }
      if(child==end) break;
      }

    // Hop bar over other if right of bar is rightward of right of other
    if(other && other!=bar){

      // First on galley hopped over to the right:- transfer flag to new first
      if((bar==begin) && (bar->getLayoutHints()&LAYOUT_DOCK_NEXT)){
        bar->setLayoutHints(bar->getLayoutHints()&~LAYOUT_DOCK_NEXT);
        other->setLayoutHints(other->getLayoutHints()|LAYOUT_DOCK_NEXT);
        }

      // And rearrange order of children
      bar->move(other->getX()+other->getWidth()-bar->getWidth(),bar->getY());
      other->move(bar->getX()-other->getWidth()-hspacing,other->getY());
      bar->reparent(this,other->getNext());
      }
    }

  // Move vertically
  else{
    bar->move(bar->getX(),by);
    }
  }


// Move dock bar, changing its options to suit position
FXbool FXToolBarDock::moveToolBar(FXToolBar* bar,FXint barx,FXint bary){
  FXint left,right,top,bottom,galx,galy,galw,galh,dockx,docky,barw,barh,expand,require,cx,cy,w,h;
  FXWindow *begin,*end,*cur,*curend,*nxt,*nxtend,*prv,*prvend;

  // We insist this bar hangs under this dock site
  if(bar && bar->getParent()==this){

    // Interior
    top=border+padtop;
    bottom=height-padbottom-border;
    left=border+padleft;
    right=width-padright-border;

    dockx=barx;
    docky=bary;
    barw=bar->getWidth();
    barh=bar->getHeight();

    // Vertically oriented
    if(options&LAYOUT_SIDE_LEFT){

      cx=barx+barw/2;

      // Determine galley sizes
      for(begin=getFirst(),cur=prv=nxt=NULL; begin; begin=end->getNext()){
        w=galleyWidth(begin,end,bottom-top,require,expand);
        if(!after(end,bar)){ if(left<=cx && cx<left+w){ prv=begin; prvend=end; } }
        else if(!after(bar,begin)){ if(left<=cx && cx<left+w){ nxt=begin; nxtend=end; }  }
        else{ cur=begin; curend=end; galx=left; galw=w; }
        left+=w+hspacing;
        }

      // Same bar, move vertically
      if(dockx<galx) dockx=galx;
      if(dockx+barw>galx+galw) dockx=galx+galw-barw;

      // Move bar vertically; this may change the galley start and end!
      moveVerBar(bar,cur,curend,dockx,docky);

      // Moving bar right
      if(cx>=galx+galw){
        if(nxt){                                  // Hang at end of next galley
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          nxt->setLayoutHints(nxt->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()&~LAYOUT_DOCK_NEXT);
          bar->reparent(this,nxtend->getNext());
          return TRUE;
          }
        else{                                     // Hang below last
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          else cur->setLayoutHints(cur->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->reparent(this,NULL);
          return barx<=width+PROXIMITY;
          }
        }

      // Moving bar left
      else if(cx<galx){
        if(prv){                                  // Hang at end of previous galley
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          prv->setLayoutHints(prv->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()&~LAYOUT_DOCK_NEXT);
          bar->reparent(this,prvend->getNext());
          return TRUE;
          }
        else{                                     // Hand above first
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          else cur->setLayoutHints(cur->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->reparent(this,getFirst());
          return barx+barw+PROXIMITY>=0;
          }
        }
      }

    // Horizontally oriented
    else{

      cy=bary+barh/2;

      // Determine galley sizes
      for(begin=getFirst(),cur=prv=nxt=NULL; begin; begin=end->getNext()){
        h=galleyHeight(begin,end,right-left,require,expand);
        if(!after(end,bar)){ if(top<=cy && cy<top+h){ prv=begin; prvend=end; } }
        else if(!after(bar,begin)){ if(top<=cy && cy<top+h){ nxt=begin; nxtend=end; }  }
        else{ cur=begin; curend=end; galy=top; galh=h; }
        top+=h+vspacing;
        }

      // Same bar, move horizontally
      if(docky<galy) docky=galy;
      if(docky+barh>galy+galh) docky=galy+galh-barh;

      // Move bar horizontally; this may change the galley start and end!
      moveHorBar(bar,cur,curend,dockx,docky);

      // Moving bar down
      if(cy>=galy+galh){
        if(nxt){                                  // Hang at end of next galley
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          nxt->setLayoutHints(nxt->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()&~LAYOUT_DOCK_NEXT);
          bar->reparent(this,nxtend->getNext());
          return TRUE;
          }
        else{                                     // Hang below last
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          else cur->setLayoutHints(cur->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->reparent(this,NULL);
          return bary<=height+PROXIMITY;
          }
        }

      // Moving bar up
      else if(cy<galy){
        if(prv){                                  // Hang at end of previous galley
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          prv->setLayoutHints(prv->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()&~LAYOUT_DOCK_NEXT);
          bar->reparent(this,prvend->getNext());
          return TRUE;
          }
        else{                                     // Hand above first
          if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
          else cur->setLayoutHints(cur->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
          bar->reparent(this,getFirst());
          return bary+barh+PROXIMITY>=0;
          }
        }
        
      }
    }
  return TRUE;
  }

/*
      // Pull left
      if(barx<-TOLERANCE){
        if(cur==bar && bar!=curend) cur->getNext()->setLayoutHints(cur->getNext()->getLayoutHints()|LAYOUT_DOCK_NEXT);
        else cur->setLayoutHints(cur->getLayoutHints()|LAYOUT_DOCK_NEXT);
        bar->setLayoutHints(bar->getLayoutHints()|LAYOUT_DOCK_NEXT);
        bar->reparent(this,NULL);
        return FALSE;
        }


      // Bar or dock "sticks out" too much 
      if(barw>width){
        if((barx>TOLERANCE) || (width>barx+barw+TOLERANCE)) return FALSE;
        }
      else{
        if((-TOLERANCE>barx) || (barx+barw>width+TOLERANCE)) return FALSE;
        }
*/

// Attempt docking of bar inside this dock
FXbool FXToolBarDock::dockToolBar(FXToolBar* bar,FXint barx,FXint bary){

  // Vertically oriented dock
  if(getLayoutHints()&LAYOUT_SIDE_LEFT){

    // Right or left edge of bar inside dock
    if((xpos-PROXIMITY<=barx && barx<xpos+width+PROXIMITY) || (xpos-PROXIMITY<=barx+bar->getWidth() && barx+bar->getWidth()<=xpos+width+PROXIMITY)){

      // Test if either bar or dock "sticks out" too much to dock
      if(bar->getHeight()>height){
        if(bary-TOLERANCE<=ypos && ypos+height<=bary+bar->getHeight()+TOLERANCE) return TRUE;
        }
      else{
        if(ypos-TOLERANCE<=bary && bary+bar->getHeight()<=ypos+height+TOLERANCE) return TRUE;
        }
      }
    }

  // Horizontally oriented dock
  else{

    // Upper or lower edge of bar inside dock
    if((ypos-PROXIMITY<=bary && bary<=ypos+height+PROXIMITY) || (ypos-PROXIMITY<=bary+bar->getHeight() && bary+bar->getHeight()<=ypos+height+PROXIMITY)){

      // Test if either bar or dock "sticks out" too much to dock
      if(bar->getWidth()>width){
        if(barx-TOLERANCE<=xpos && xpos+width<=barx+bar->getWidth()+TOLERANCE) return TRUE;
        }
      else{
        if(xpos-TOLERANCE<=barx && barx+bar->getWidth()<=xpos+width+TOLERANCE) return TRUE;
        }
      }
    }

  return FALSE;
  }



}

