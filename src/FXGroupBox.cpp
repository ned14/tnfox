/********************************************************************************
*                                                                               *
*                G r o u p  B o x   W i n d o w   O b j e c t                   *
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
* $Id: FXGroupBox.cpp,v 1.29 2004/02/08 17:29:06 fox Exp $                      *
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
#include "FXDC.h"
#include "FXDCWindow.h"
#include "FXFont.h"
#include "FXLabel.h"
#include "FXGroupBox.h"


/*
  Notes:

  - Radio behaviour of groupbox has been dropped; groupbox is now
    purely a decorative layout manager.
*/


#define FRAME_MASK           (FRAME_SUNKEN|FRAME_RAISED|FRAME_THICK)
#define GROUPBOX_TITLE_MASK  (GROUPBOX_TITLE_LEFT|GROUPBOX_TITLE_CENTER|GROUPBOX_TITLE_RIGHT)



/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXGroupBox) FXGroupBoxMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXGroupBox::onPaint),
  };


// Object implementation
FXIMPLEMENT(FXGroupBox,FXPacker,FXGroupBoxMap,ARRAYNUMBER(FXGroupBoxMap))


// Deserialization
FXGroupBox::FXGroupBox(){
  flags|=FLAG_ENABLED;
  font=(FXFont*)-1L;
  textColor=0;
  }


// Make a groupbox
FXGroupBox::FXGroupBox(FXComposite* p,const FXString& text,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs):
  FXPacker(p,opts,x,y,w,h,pl,pr,pt,pb,hs,vs),label(text){
  flags|=FLAG_ENABLED;
  font=getApp()->getNormalFont();
  textColor=getApp()->getForeColor();
  }


// Create window
void FXGroupBox::create(){
  FXPacker::create();
  font->create();
  }


// Detach window
void FXGroupBox::detach(){
  FXPacker::detach();
  font->detach();
  }


// Enable the window
void FXGroupBox::enable(){
  if(!(flags&FLAG_ENABLED)){
    FXPacker::enable();
    update();
    }
  }


// Disable the window
void FXGroupBox::disable(){
  if(flags&FLAG_ENABLED){
    FXPacker::disable();
    update();
    }
  }


// Change the font
void FXGroupBox::setFont(FXFont* fnt){
  if(!fnt){ fxerror("%s::setFont: NULL font specified.\n",getClassName()); }
  if(font!=fnt){
    font=fnt;
    recalc();
    update();
    }
  }


// Get default width
FXint FXGroupBox::getDefaultWidth(){
  FXint cw=FXPacker::getDefaultWidth();
  if(!label.empty()){
    FXint tw=font->getTextWidth(label.text(),label.length())+16;
    return FXMAX(cw,tw);
    }
  return cw;
  }


// Get default height
FXint FXGroupBox::getDefaultHeight(){
  FXint ch=FXPacker::getDefaultHeight();
  if(!label.empty()){
    return ch+font->getFontHeight()+4;
    }
  return ch;
  }


// Recompute layout
void FXGroupBox::layout(){
  FXint tmp=padtop;
  if(!label.empty()){
    padtop=padtop+font->getFontHeight()+4-border;
    }
  FXPacker::layout();
  padtop=tmp;
  flags&=~FLAG_DIRTY;
  }


// Handle repaint
long FXGroupBox::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  FXDCWindow dc(this,ev);
  FXint tw,yy,xx,hh;

  tw=0;
  xx=0;
  yy=0;
  hh=height;

  // Paint background
  dc.setForeground(backColor);
  dc.fillRectangle(ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h);

  // Draw label if there is one
  if(!label.empty()){
    tw=font->getTextWidth(label.text(),label.length());
    yy=2+font->getFontAscent()/2;
    hh=height-yy;
    }

  // We should really just draw what's exposed!
  switch(options&FRAME_MASK) {
    case FRAME_LINE: drawBorderRectangle(dc,0,yy,width,hh); break;
    case FRAME_SUNKEN: drawSunkenRectangle(dc,0,yy,width,hh); break;
    case FRAME_RAISED: drawRaisedRectangle(dc,0,yy,width,hh); break;
    case FRAME_GROOVE: drawGrooveRectangle(dc,0,yy,width,hh); break;
    case FRAME_RIDGE: drawRidgeRectangle(dc,0,yy,width,hh); break;
    case FRAME_SUNKEN|FRAME_THICK: drawDoubleSunkenRectangle(dc,0,yy,width,hh); break;
    case FRAME_RAISED|FRAME_THICK: drawDoubleRaisedRectangle(dc,0,yy,width,hh); break;
    }

  // Draw label
  if(!label.empty()){
    if(options&GROUPBOX_TITLE_RIGHT) xx=width-tw-16;
    else if(options&GROUPBOX_TITLE_CENTER) xx=(width-tw)/2-4;
    else xx=8;
    dc.setForeground(backColor);
    dc.setFont(font);
    dc.fillRectangle(xx,yy,tw+8,2);
    if(isEnabled()){
      dc.setForeground(textColor);
      dc.drawText(xx+4,2+font->getFontAscent(),label.text(),label.length());
      }
    else{
      dc.setForeground(hiliteColor);
      dc.drawText(xx+5,3+font->getFontAscent(),label.text(),label.length());
      dc.setForeground(shadowColor);
      dc.drawText(xx+4,2+font->getFontAscent(),label.text(),label.length());
      }
    }
  return 1;
  }


// Get group box style
FXuint FXGroupBox::getGroupBoxStyle() const {
  return (options&GROUPBOX_TITLE_MASK);
  }


// Set group box style
void FXGroupBox::setGroupBoxStyle(FXuint style){
  FXuint opts=(options&~GROUPBOX_TITLE_MASK) | (style&GROUPBOX_TITLE_MASK);
  if(options!=opts){
    options=opts;
    recalc();
    update();
    }
  }


// Change text
void FXGroupBox::setText(const FXString& text){
  if(label!=text){
    label=text;
    recalc();
    update();
    }
  }


// Set text color
void FXGroupBox::setTextColor(FXColor clr){
  if(textColor!=clr){
    textColor=clr;
    update();
    }
  }


// Save object to stream
void FXGroupBox::save(FXStream& store) const {
  FXPacker::save(store);
  store << label;
  store << font;
  store << textColor;
  }


// Load object from stream
void FXGroupBox::load(FXStream& store){
  FXPacker::load(store);
  store >> label;
  store >> font;
  store >> textColor;
  }

}
