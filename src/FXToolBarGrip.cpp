/********************************************************************************
*                                                                               *
*                       T o o l   B a r   G r i p   W i d g e t                 *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXToolBarGrip.cpp,v 1.18 2005/01/29 05:02:02 fox Exp $                   *
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
#include "FXDCWindow.h"
#include "FXToolBar.h"
#include "FXToolBarGrip.h"


/*
  Notes:
  - On MS-Windows, this works just fine, without fanfare.
  - On X11, some quaintness in the X-Server causes havoc if reparent() 
    is called on a window while it was grabbed.
  - Not to be deterred, we implement here the following workaround:
  
    1) We create a temporary dummy window, just 1x1 pixels in size, in the
       upper left corner of the screen [yes, that was not a dead pixel 
       after all!].
       
    2) We add this to the hash table, so now events from the "true" window 
       as well those from the dummy window are dispatched to the toolbar grip.
       
    3) We temporarily replace the xid of the true window with the dummy one,
       then invoke grab() to grab the mouse, then restore the original xid.
       
    4) Now you can wave your mouse around and dock or undock toolbars.
    
    5) When we're done, we replace the xid again with the dummy window, 
       call ungrab(), then restore the original xid again.
       
    6) Then, delete the dummy window.
    
  - The only downside of this method is that the win_x and win_y member
    data in FXEvent is unreliable; fortunately, the standard toolbar docking
    algorithms do not use these members. 
  - Of course, we'd rather not have to do all this; so don't hesitate
    to inform us if you have a better way!
       
*/


// Size
#define GRIP_SINGLE  3          // Single grip for arrangable toolbars
#define GRIP_DOUBLE  7          // Double grip for dockable toolbars



/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXToolBarGrip) FXToolBarGripMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXToolBarGrip::onPaint),
  FXMAPFUNC(SEL_ENTER,0,FXToolBarGrip::onEnter),
  FXMAPFUNC(SEL_LEAVE,0,FXToolBarGrip::onLeave),
  FXMAPFUNC(SEL_MOTION,0,FXToolBarGrip::onMotion),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXToolBarGrip::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXToolBarGrip::onLeftBtnRelease),
  FXMAPFUNC(SEL_QUERY_TIP,0,FXToolBarGrip::onQueryTip),
  FXMAPFUNC(SEL_COMMAND,FXToolBarGrip::ID_SETTIPSTRING,FXToolBarGrip::onCmdSetTip),
  FXMAPFUNC(SEL_COMMAND,FXToolBarGrip::ID_GETTIPSTRING,FXToolBarGrip::onCmdGetTip),
  };


// Object implementation
FXIMPLEMENT(FXToolBarGrip,FXWindow,FXToolBarGripMap,ARRAYNUMBER(FXToolBarGripMap))


// Deserialization
FXToolBarGrip::FXToolBarGrip(){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  activeColor=0;
  hiliteColor=0;
  shadowColor=0;
  xxx=0;
  }


// Construct and init
FXToolBarGrip::FXToolBarGrip(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):
  FXWindow(p,opts,x,y,w,h){
  flags|=FLAG_SHOWN|FLAG_ENABLED;
  dragCursor=getApp()->getDefaultCursor(DEF_MOVE_CURSOR);
  target=tgt;
  message=sel;
  backColor=getApp()->getBaseColor();
  activeColor=FXRGB(0,0,255);
  hiliteColor=getApp()->getHiliteColor();
  shadowColor=getApp()->getShadowColor();
  xxx=0;
  }


// Get default width
FXint FXToolBarGrip::getDefaultWidth(){
  return (options&TOOLBARGRIP_DOUBLE)?GRIP_DOUBLE:GRIP_SINGLE;
  }


// Get default height
FXint FXToolBarGrip::getDefaultHeight(){
  return (options&TOOLBARGRIP_DOUBLE)?GRIP_DOUBLE:GRIP_SINGLE;
  }


// Change toolbar orientation
void FXToolBarGrip::setDoubleBar(FXbool dbl){
  FXuint opts=dbl?(options|TOOLBARGRIP_DOUBLE):(options&~TOOLBARGRIP_DOUBLE);
  if(opts!=options){
    options=opts;
    recalc();
    }
  }


// Return TRUE if toolbar grip is displayed as a double bar
FXbool FXToolBarGrip::isDoubleBar() const {
  return (options&TOOLBARGRIP_DOUBLE)!=0;
  }


// Handle repaint
long FXToolBarGrip::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  FXDCWindow dc(this,ev);
  dc.setForeground(backColor);
  dc.fillRectangle(ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h);
  if(width>height){
    if(options&TOOLBARGRIP_DOUBLE){     // =
      dc.setForeground(hiliteColor);
      dc.fillRectangle(0,0,1,2);
      dc.fillRectangle(0,4,1,2);
      dc.fillRectangle(0,0,width-1,1);
      dc.fillRectangle(0,4,width-1,1);
      dc.setForeground(shadowColor);
      dc.fillRectangle(width-1,0,1,3);
      dc.fillRectangle(width-1,4,1,3);
      dc.fillRectangle(0,2,width-1,1);
      dc.fillRectangle(0,6,width-1,1);
      if(flags&(FLAG_ACTIVE|FLAG_TRYDRAG|FLAG_DODRAG)){
        dc.setForeground(activeColor);
        dc.fillRectangle(1,1,width-2,1);
        dc.fillRectangle(1,5,width-2,1);
        }
      }
    else{                               // -
      dc.setForeground(hiliteColor);
      dc.fillRectangle(0,0,1,2);
      dc.fillRectangle(0,0,width-1,1);
      dc.setForeground(shadowColor);
      dc.fillRectangle(width-1,0,1,3);
      dc.fillRectangle(0,2,width-1,1);
      if(flags&(FLAG_ACTIVE|FLAG_TRYDRAG|FLAG_DODRAG)){
        dc.setForeground(activeColor);
        dc.fillRectangle(1,1,width-2,1);
        }
      }
    }
  else{
    if(options&TOOLBARGRIP_DOUBLE){     // ||
      dc.setForeground(hiliteColor);
      dc.fillRectangle(0,0,2,1);
      dc.fillRectangle(4,0,2,1);
      dc.fillRectangle(0,0,1,height-1);
      dc.fillRectangle(4,0,1,height-1);
      dc.setForeground(shadowColor);
      dc.fillRectangle(0,height-1,3,1);
      dc.fillRectangle(4,height-1,3,1);
      dc.fillRectangle(2,0,1,height-1);
      dc.fillRectangle(6,0,1,height-1);
      if(flags&(FLAG_ACTIVE|FLAG_TRYDRAG|FLAG_DODRAG)){
        dc.setForeground(activeColor);
        dc.fillRectangle(1,1,1,height-2);
        dc.fillRectangle(5,1,1,height-2);
        }
      }
    else{                               // |
      dc.setForeground(hiliteColor);
      dc.fillRectangle(0,0,2,1);
      dc.fillRectangle(0,0,1,height-1);
      dc.setForeground(shadowColor);
      dc.fillRectangle(0,height-1,3,1);
      dc.fillRectangle(2,0,1,height-1);
      if(flags&(FLAG_ACTIVE|FLAG_TRYDRAG|FLAG_DODRAG)){
        dc.setForeground(activeColor);
        dc.fillRectangle(1,1,1,height-2);
        }
      }
    }
  return 1;
  }


// Entered button
long FXToolBarGrip::onEnter(FXObject* sender,FXSelector sel,void* ptr){
  FXWindow::onEnter(sender,sel,ptr);
  if(isEnabled()){ flags|=FLAG_ACTIVE; update(); }
  return 1;
  }


// Leave button
long FXToolBarGrip::onLeave(FXObject* sender,FXSelector sel,void* ptr){
  FXWindow::onLeave(sender,sel,ptr);
  if(isEnabled()){ flags&=~FLAG_ACTIVE; update(); }
  return 1;
  }


// Moved
long FXToolBarGrip::onMotion(FXObject*,FXSelector,void* ptr){
  if(flags&FLAG_DODRAG){
    handle(this,FXSEL(SEL_DRAGGED,0),ptr);
    return 1;
    }
  if((flags&FLAG_TRYDRAG) && ((FXEvent*)ptr)->moved){
    if(handle(this,FXSEL(SEL_BEGINDRAG,0),ptr)) flags|=FLAG_DODRAG;
    flags&=~FLAG_TRYDRAG;
    return 1;
    }
  return 0;
  }


// Pressed LEFT button
long FXToolBarGrip::onLeftBtnPress(FXObject*,FXSelector,void*){
  if(isEnabled()){
    flags=(flags&~(FLAG_UPDATE|FLAG_DODRAG))|FLAG_TRYDRAG;
#ifndef WIN32
    Display *display=(Display*)getApp()->getDisplay();
    const unsigned long mask=CWBackPixmap|CWWinGravity|CWBitGravity|CWBorderPixel|CWOverrideRedirect|CWSaveUnder|CWEventMask|CWDontPropagate|CWColormap|CWCursor;
    XSetWindowAttributes wattr;
    FXID tempxid=xid;
    wattr.background_pixmap=None;
    wattr.background_pixel=0;
    wattr.border_pixmap=None;
    wattr.border_pixel=0;
    wattr.bit_gravity=ForgetGravity;
    wattr.win_gravity=NorthWestGravity;
    wattr.backing_store=NotUseful;
    wattr.backing_planes=0;
    wattr.backing_pixel=0;
    wattr.save_under=FALSE;
    wattr.event_mask=ButtonPressMask|ButtonReleaseMask|PointerMotionMask|KeyPressMask|KeyReleaseMask | FocusChangeMask|StructureNotifyMask | StructureNotifyMask|ExposureMask|PropertyChangeMask|EnterWindowMask|LeaveWindowMask;
    wattr.do_not_propagate_mask=KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|ButtonMotionMask;
    wattr.override_redirect=TRUE;
    wattr.colormap=DefaultColormap(display,DefaultScreen(display));
    wattr.cursor=None;
    xxx=XCreateWindow(display,RootWindow(display,DefaultScreen(display)),0,0,1,1,0,DefaultDepth(display,DefaultScreen(display)),InputOutput,DefaultVisual(display,DefaultScreen(display)),mask,&wattr);
    getApp()->hash.insert((void*)xxx,this);
    XMapWindow(display,xxx);
    xid=xxx;
    grab();
    xid=tempxid; 
#else    
    grab();
#endif
    update();
    }
  return 1;
  }


// Released LEFT button
long FXToolBarGrip::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    if(flags&FLAG_DODRAG){handle(this,FXSEL(SEL_ENDDRAG,0),ptr);}
    flags=(flags&~(FLAG_TRYDRAG|FLAG_DODRAG))|FLAG_UPDATE;
#ifndef WIN32
    Display *display=(Display*)getApp()->getDisplay();
    FXID tempxid=xid;
    xid=xxx;
    ungrab();
    xid=tempxid;    
    getApp()->hash.remove((void*)xxx);
    XDestroyWindow(display,xxx);
    xxx=0;
#else    
    ungrab();
#endif
    update();
    }
  return 1;
  }


// We were asked about tip text
long FXToolBarGrip::onQueryTip(FXObject* sender,FXSelector sel,void* ptr){
  if(FXWindow::onQueryTip(sender,sel,ptr)) return 1;
  if((flags&FLAG_TIP) && !tip.empty()){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&tip);
    return 1;
    }
  return 0;
  }


// Set tip using a message
long FXToolBarGrip::onCmdSetTip(FXObject*,FXSelector,void* ptr){
  setTipText(*((FXString*)ptr));
  return 1;
  }


// Get tip using a message
long FXToolBarGrip::onCmdGetTip(FXObject*,FXSelector,void* ptr){
  *((FXString*)ptr)=getTipText();
  return 1;
  }


// Set highlight color
void FXToolBarGrip::setHiliteColor(FXColor clr){
  if(clr!=hiliteColor){
    hiliteColor=clr;
    update();
    }
  }


// Set shadow color
void FXToolBarGrip::setShadowColor(FXColor clr){
  if(clr!=shadowColor){
    shadowColor=clr;
    update();
    }
  }


// Set active color
void FXToolBarGrip::setActiveColor(FXColor clr){
  if(clr!=activeColor){
    activeColor=clr;
    update();
    }
  }


// Save data
void FXToolBarGrip::save(FXStream& store) const {
  FXWindow::save(store);
  store << activeColor;
  store << hiliteColor;
  store << shadowColor;
  }


// Load data
void FXToolBarGrip::load(FXStream& store){
  FXWindow::load(store);
  store >> activeColor;
  store >> hiliteColor;
  store >> shadowColor;
  }

}
