/********************************************************************************
*                                                                               *
*                   C a n v a s   W i n d o w   O b j e c t                     *
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
* $Id: FXCanvas.cpp,v 1.26 2004/02/08 17:29:06 fox Exp $                        *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXHash.h"
#include "FXApp.h"
#include "FXCanvas.h"




/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXCanvas) FXCanvasMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXCanvas::onPaint),
  FXMAPFUNC(SEL_MOTION,0,FXCanvas::onMotion),
  FXMAPFUNC(SEL_KEYPRESS,0,FXCanvas::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,FXCanvas::onKeyRelease),
  };


// Object implementation
FXIMPLEMENT(FXCanvas,FXWindow,FXCanvasMap,ARRAYNUMBER(FXCanvasMap))


// For serialization
FXCanvas::FXCanvas(){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  }


// Make a canvas
FXCanvas::FXCanvas(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):
  FXWindow(p,opts,x,y,w,h){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  backColor=getApp()->getBackColor();
  target=tgt;
  message=sel;
  }



// It can be focused on
FXbool FXCanvas::canFocus() const { return TRUE; }


// Canvas is an object drawn by another
long FXCanvas::onPaint(FXObject*,FXSelector,void* ptr){
  return target && target->handle(this,FXSEL(SEL_PAINT,message),ptr);
  }


// Mouse moved
long FXCanvas::onMotion(FXObject*,FXSelector,void* ptr){
  return isEnabled() && target && target->handle(this,FXSEL(SEL_MOTION,message),ptr);
  }


// Handle keyboard press/release
long FXCanvas::onKeyPress(FXObject*,FXSelector,void* ptr){
  flags&=~FLAG_TIP;
  return isEnabled() && target && target->handle(this,FXSEL(SEL_KEYPRESS,message),ptr);
  }


long FXCanvas::onKeyRelease(FXObject*,FXSelector,void* ptr){
  return isEnabled() && target && target->handle(this,FXSEL(SEL_KEYRELEASE,message),ptr);
  }

}
