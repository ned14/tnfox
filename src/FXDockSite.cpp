/********************************************************************************
*                                                                               *
*                         D o c k   S i t e   W i d g e t                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004 by Jeroen van der Zijp.   All Rights Reserved.             *
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
* $Id: FXDockSite.cpp,v 1.11 2004/11/18 13:46:16 fox Exp $                      *
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
#include "FXDockSite.h"


/*
  Notes:
*/


using namespace FX;

/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXDockSite) FXDockSiteMap[]={
  FXMAPFUNC(SEL_FOCUS_PREV,0,FXDockSite::onFocusLeft),
  FXMAPFUNC(SEL_FOCUS_NEXT,0,FXDockSite::onFocusRight),
  };


// Object implementation
FXIMPLEMENT(FXDockSite,FXPacker,FXDockSiteMap,ARRAYNUMBER(FXDockSiteMap))


// Make a dock site
FXDockSite::FXDockSite(FXComposite* p,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):
  FXPacker(p,opts,x,y,w,h,pl,pr,pt,pb,hs,vs){
  }


// Compute minimum width based on child layout hints
FXint FXDockSite::getDefaultWidth(){
  register FXint w,tot,ww,mw;
  register FXWindow *child,*cc;
  register FXuint hints;
  tot=mw=0;
  if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();
  for(child=getFirst(); child; child=cc){
    ww=0;
    cc=child;
    do{
      if(cc->shown()){
        hints=cc->getLayoutHints();
        if(hints&LAYOUT_FIX_WIDTH) w=cc->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=mw;
        else w=cc->getDefaultWidth();
        if(ww) ww+=hspacing;
        ww+=w;
        }
      cc=cc->getNext();
      }
    while(cc && (cc->getLayoutHints()&LAYOUT_SIDE_LEFT));
    if(ww>tot) tot=ww;
    }
  return padleft+padright+tot+(border<<1);
  }


// Compute minimum height based on child layout hints
FXint FXDockSite::getDefaultHeight(){
  register FXint tot,h,hh,mh;
  register FXWindow *child,*cc;
  register FXuint hints;
  tot=mh=0;
  if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();
  for(child=getFirst(); child; child=cc){
    hh=0;
    cc=child;
    do{
      if(cc->shown()){
        hints=cc->getLayoutHints();
        if(hints&LAYOUT_FIX_HEIGHT) h=cc->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=mh;
        else h=cc->getDefaultHeight();
        if(h>hh) hh=h;
        }
      cc=cc->getNext();
      }
    while(cc && (cc->getLayoutHints()&LAYOUT_SIDE_LEFT));
    if(tot) tot+=vspacing;
    tot+=hh;
    }
  return padtop+padbottom+tot+(border<<1);
  }


// Recalculate layout
void FXDockSite::layout(){
  register FXint left,right,top,bottom,galx,galy,galw,galh,mw,mh,x,y,w,h;
  register FXWindow *beg,*end,*c;
  register FXuint hints;

  // Get maximum child size
  mw=mh=0;
  if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();
  if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();

  // Galley size
  top=border+padtop;
  bottom=height-padbottom-border;

  // Loop over galleys
  for(beg=getFirst(); beg; beg=end){

    // Figure number of items on galley and resulting galley height
    left=border+padleft;
    right=width-padright-border;
    galh=0;
    c=beg;
    do{
      if(c->shown()){
      
        // Hints
        hints=c->getLayoutHints();

        // Get height
        if(hints&LAYOUT_FIX_HEIGHT) h=c->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=mh;
        else h=c->getDefaultHeight();

        // Get width
        if(hints&LAYOUT_FIX_WIDTH) w=c->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=mw;
        else if(hints&LAYOUT_FILL_X) w=right-left;
        else w=c->getDefaultWidth();

        // Determine child x-position
        if((hints&LAYOUT_RIGHT)&&(hints&LAYOUT_CENTER_X)){
          x=c->getX();
          if(x+w>right) x=right-w;    
          if(x<left) x=left;
          left=x+w+hspacing;
          }
        else if(hints&LAYOUT_CENTER_X){
          x=left+(right-left-w)/2;
          }
        else if(hints&LAYOUT_RIGHT){
          x=right-w;
          right-=w+hspacing;
          }
        else{
          x=left;
          left+=w+hspacing;
          }

        // No room
        if(c!=beg && left>=right) break;
        
        // Update galley height
        if(h>galh) galh=h;
        }
      c=c->getNext();
      }
    while(c);
    
    // Start of next galley
    end=c;

    // Build from the bottom up
    hints=getLayoutHints();
    if(hints&LAYOUT_SIDE_BOTTOM){
      galy=bottom-galh;
      bottom-=galh+vspacing;
      }
      
    // Build from the top down
    else{
      galy=top;
      top+=galh+vspacing;
      }

    // Fix vertical placement on galley
    left=border+padleft;
    right=width-padright-border;
    c=beg;
    do{
      if(c->shown()){
      
        // Hints
        hints=c->getLayoutHints();

        // Get height
        if(hints&LAYOUT_FIX_HEIGHT) h=c->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=mh;
        else if(hints&LAYOUT_FILL_Y) h=galh;
        else h=c->getDefaultHeight();

        // Get width
        if(hints&LAYOUT_FIX_WIDTH) w=c->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=mw;
        else if(hints&LAYOUT_FILL_X) w=right-left;
        else w=c->getDefaultWidth();

        // Determine child y-position
        if((hints&LAYOUT_BOTTOM)&&(hints&LAYOUT_CENTER_Y)){
          y=c->getY();
          if(y<galy) y=galy;
          if(y+h>galy) y=galy+galh-h;
          }
        else if(hints&LAYOUT_CENTER_Y){
          y=galy+(galh-h)/2;
          }
        else if(hints&LAYOUT_BOTTOM){
          y=galy+galh-h;
          }
        else{ 
          y=galy;
          }

        // Determine child x-position
        if((hints&LAYOUT_RIGHT)&&(hints&LAYOUT_CENTER_X)){
          x=c->getX();
          if(x+w>right) x=right-w;    
          if(x<left) x=left;
          left=x+w+hspacing;
          }
        else if(hints&LAYOUT_CENTER_X){
          x=left+(right-left-w)/2;
          }
        else if(hints&LAYOUT_RIGHT){
          x=right-w;
          right-=w+hspacing;
          }
        else{
          x=left;
          left+=w+hspacing;
          }

        // Placement on this galley
        c->position(x,y,w,h);
        }
      c=c->getNext();
      }
    while(c!=end);
    }
  flags&=~FLAG_DIRTY;
  }


/*
// Recalculate layout
void FXDockSite::layout(){
  register FXint left,right,top,bottom,galx,galy,galw,galh,mw,mh,x,y,w,h;
  register FXWindow *child,*cc;
  register FXuint hints;

  // Get maximum child size
  mw=mh=0;
  if(options&PACK_UNIFORM_WIDTH) mw=maxChildWidth();
  if(options&PACK_UNIFORM_HEIGHT) mh=maxChildHeight();

  top=border+padtop;
  bottom=height-padbottom-border;

  // Find number of paddable children and total width
  for(child=getFirst(); child; child=cc){

    // Figure galley height
    cc=child;
    galh=0;
    do{
      if(cc->shown()){
        hints=cc->getLayoutHints();
        if(hints&LAYOUT_FIX_HEIGHT) h=cc->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=mh;
        else h=cc->getDefaultHeight();
        if(h>galh) galh=h;
        }
      cc=cc->getNext();
      }
    while(cc && (cc->getLayoutHints()&LAYOUT_SIDE_LEFT));

    // Figure galley y location
    hints=child->getLayoutHints();
    if(hints&LAYOUT_SIDE_BOTTOM){
      galy=bottom-galh;
      bottom-=galh+vspacing;
      }
    else{
      galy=top;
      top+=galh+vspacing;
      }

    // Place widgets in galley
    cc=child;
    left=border+padleft;
    right=width-padright-border;
    do{
      if(cc->shown()){
        hints=cc->getLayoutHints();
        y=galy;

        if(hints&LAYOUT_FIX_WIDTH) w=cc->getWidth();
        else if(options&PACK_UNIFORM_WIDTH) w=mw;
        else w=cc->getDefaultWidth();

        if(hints&LAYOUT_FIX_HEIGHT) h=cc->getHeight();
        else if(options&PACK_UNIFORM_HEIGHT) h=mh;
        else h=cc->getDefaultHeight();

/// FIXME
// if not stretchable
//   then should be after left child or at desired position
//   but move back from desired position if x+w exceeds right edge
// if not starting new galley and not enough space
//   then shrink to fit
// if not enough space or a new line starts
//   then place on new line
        x=cc->getX();
        if(!((hints&LAYOUT_RIGHT)&&(hints&LAYOUT_CENTER_X))){
          if(hints&LAYOUT_RIGHT){
            x=right-w;
            right-=w+hspacing;
            }
          else{
            x=left;
            left+=w+hspacing;
            }
          }
        cc->position(x,y,w,h);
        }
      cc=cc->getNext();
      }
    while(cc && (cc->getLayoutHints()&LAYOUT_SIDE_LEFT));
    }
  flags&=~FLAG_DIRTY;
  }

*/

}
