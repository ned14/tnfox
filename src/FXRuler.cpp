/********************************************************************************
*                                                                               *
*                            R u l e r   W i d g e t                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXRuler.cpp,v 1.27 2004/02/08 17:29:07 fox Exp $                         *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXRegistry.h"
#include "FXHash.h"
#include "FXApp.h"
#include "FXDCWindow.h"
#include "FXFont.h"
#include "FXRuler.h"



/*
  Notes:
  - If showing arrows for cursor position, draw them down (right)
    when ticks are centered or bottom (right).

    Metric:

        0                             1                             2
        |                             |                             |
        |              |              |              |              |
        |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
        |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  .  .  .

    English:

        0                       1                       2
        |                       |                       |
        |           |           |           |           |           |
        |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
        |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  .  .  .

  - Subdivide repeatedly when zooming in/out:

    "pi",   1.0,   1, 2, 5, 10, 25, 50, 100, 250, 500, 1000,      1, 5, 10, 50, 100
    "in",  72.0,   1, 2, 4,  8, 16, 32,  64, 128, 256,  512,      1, 2,  4,  8,  16
    "cm", 28.35,   1, 2, 5, 10, 25, 50, 100, 250, 500, 1000,      1, 5, 10, 50, 100

  - Layout:


      docSpace                                                           docSpace
    |<-->|                                                                   |<-->|
    |    |                                                                   |    |
    +----+-------------------------------------------------------------------+----+
    |    |                                                                   |    |
    |    |                                                                   |    |
    |    |  lowerMargin                                         upperMargin  |    |
    |    |<---->|                                                     |<---->|    |
    |    |      |                                                     |      |    |
    |    +------+-----------------------------------------------------+------+    |
    |    |      |                                                     |      |    |
    |    |      |    firstPara                                        |      |    |
    |    |      |<--------->|                                         |      |    |
    |    |      |           |                                         |      |    |
    |    |      |           |                                         |      |    |
    |    |      | lowerPara |                               upperPara |      |    |
    |    |      |<---->|    |                                  |<---->|      |    |
    |    |      |      |    |                                  |      |      |    |
    |    |      +------+----+----------------------------------+------+      |    |
    |    |      |      |    |                                  |      |      |    |
    |    |      |      |    We, The People of the United States,      |      |    |
    |    |      |      in Order.................................      |      |    |
    |    |      |      .........................................      |      |    |
    |    |      |      .........................................      |      |    |
    |    |      |      .........................................      |      |    |
    |    |      |      .........................................      |      |    |
    |    |      |      |                                       |      |      |    |
    |    |      +------+---------------------------------------+------+      |    |
    |    |      |                                              |      |      |    |
    |    |      |                                              |      |      |    |
    |    +------+----------------------------------------------+------+------+    |
    |    |      |                                                     |      |    |
    |    |      |<--------------------------------------------------->|      |    |
    |    |            printable (must be smaller than docSize)               |    |
    |    |                                                                   |    |
    |    |                                                                   |    |
    |    |<----------------------------------------------------------------->|    |
    |                                docSize                                      |
    |                                                                             |
    |                                                                             |
    +-----------------------------------------------------------------------------+
    |                                                                             |
    |                                                                             |
    |<--------------------------------------------------------------------------->|
                                   contentSize


  - Values of firstPara, lowerPara, may be negative, but not less than -lowerMargin.

  - Likewise upperPara may be negative, but not less than -upperMargin.

  - Content width is docSize+2*docSpace; this may exceed viewport width, i.e.
    the width of the ruler itself (we assume the horizontal ruler extends over
    the entire usable viewport area).

  - If viewport larger that content, keep document centered in view; otherwise,
    keep document away from left edge by docSpace (and then we pop scrollbars
    into the picture).

  - Tickmarks start counting from left printable margin, i.e. common setting for
    lowerPara and upperPara would be 0, for full utilization of paper.

  - Ruler items:

      o Downpointing triangle is first line indent
      o Up pointing left triangle is left indent
      o Up pointing right triangle is right indent
      o Left, right darker area is margins

  - Metrics:

    1) Document width
    2) Document margins
    3) Paragraph start/end
    4) Paragraph firstline indent
    5) Paragraph tab positions and tab types
    6) Scroll offset

*/

#define ARROWBASE       9       // Size of cursor arrows (must be odd)
#define ARROWLENGTH     4       // The above divided by two
#define MARKERBASE      9       // Base of marker
#define MARKERLENGTH    4       // The above divided by two
#define EXTRASPACING    3       // Spacing below/above ticks or text
#define BETWEENSPACING  2       // Spacing between number and ticks
#define MAJORTICKSIZE   6       // Length of major ticks
#define MEDIUMTICKSIZE  4       // Length of medium ticks
#define MINORTICKSIZE   3       // Length of minor ticks
#define DOCSPACE        20      // Default space around document
#define MARGINSPACE     25      // Default margin space (0.25in x 100dpi)
//#define DOCSIZE         850     // Default document size (8.5in x 100dpi)
#define DOCSIZE         400     // Default document size (8.5in x 100dpi)
#define RULER_MASK      (RULER_NORMAL|RULER_NUMBERS)




/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXRuler) FXRulerMap[]={
  FXMAPFUNC(SEL_PAINT,0,FXRuler::onPaint),
  FXMAPFUNC(SEL_MOTION,0,FXRuler::onMotion),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS,0,FXRuler::onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE,0,FXRuler::onLeftBtnRelease),
  FXMAPFUNC(SEL_UPDATE,FXRuler::ID_QUERY_TIP,FXRuler::onQueryTip),
  FXMAPFUNC(SEL_UPDATE,FXRuler::ID_QUERY_HELP,FXRuler::onQueryHelp),
  FXMAPFUNC(SEL_COMMAND,FXRuler::ID_SETVALUE,FXRuler::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXRuler::ID_SETINTVALUE,FXRuler::onCmdSetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXRuler::ID_GETINTVALUE,FXRuler::onCmdGetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXRuler::ID_SETHELPSTRING,FXRuler::onCmdSetHelp),
  FXMAPFUNC(SEL_COMMAND,FXRuler::ID_GETHELPSTRING,FXRuler::onCmdGetHelp),
  FXMAPFUNC(SEL_COMMAND,FXRuler::ID_SETTIPSTRING,FXRuler::onCmdSetTip),
  FXMAPFUNC(SEL_COMMAND,FXRuler::ID_GETTIPSTRING,FXRuler::onCmdGetTip),
  };


// Object implementation
FXIMPLEMENT(FXRuler,FXFrame,FXRulerMap,ARRAYNUMBER(FXRulerMap))


// Deserialization
FXRuler::FXRuler(){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  font=(FXFont*)-1L;
  textColor=0;
  lowerMargin=MARGINSPACE;
  upperMargin=MARGINSPACE;
  firstPara=0;
  lowerPara=0;
  upperPara=0;
  docSpace=DOCSPACE;
  docSize=DOCSIZE;
  contentSize=DOCSPACE*DOCSPACE+DOCSIZE;
  arrowPos=DOCSPACE;
  pixelPerTick=10.0;
  majorTicks=10;
  mediumTicks=5;
  tinyTicks=1;
  shift=0;
  }


// Make a label
FXRuler::FXRuler(FXComposite* p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):
  FXFrame(p,opts,x,y,w,h,pl,pr,pt,pb){
  flags|=FLAG_ENABLED|FLAG_SHOWN;
  target=tgt;
  message=sel;
  font=getApp()->getNormalFont();
  backColor=getApp()->getBackColor();
  textColor=getApp()->getForeColor();
  lowerMargin=MARGINSPACE;
  upperMargin=MARGINSPACE;
  firstPara=20;
  lowerPara=10;
  upperPara=10;
//  firstPara=0;
//  lowerPara=0;
//  upperPara=0;
  docSpace=DOCSPACE;
  docSize=DOCSIZE;
  contentSize=DOCSPACE*DOCSPACE+DOCSIZE;
  arrowPos=DOCSPACE;
  pixelPerTick=10.0;
  majorTicks=10;
  mediumTicks=5;
  tinyTicks=1;
  shift=0;
  }


// Create window
void FXRuler::create(){
  FXFrame::create();
  font->create();
  }


// Detach window
void FXRuler::detach(){
  FXFrame::detach();
  font->detach();
  }


// Get default width
FXint FXRuler::getDefaultWidth(){
  FXint tw,th,s=0,w=0;
  if(options&RULER_VERTICAL){           // Vertical
    if(options&RULER_NUMBERS){
      tw=font->getTextWidth("0",1);     // Ruler should be same width regardless of orientation
      th=font->getFontHeight();
      w=FXMAX(tw,th);
      }
    if(options&(RULER_TICKS_LEFT|RULER_TICKS_RIGHT)){
      if(w) s=BETWEENSPACING;
      if(!(options&RULER_TICKS_LEFT)) w=MAJORTICKSIZE+s+w;              // Ticks on right
      else if(!(options&RULER_TICKS_RIGHT)) w=MAJORTICKSIZE+s+w;        // Ticks on left
      else w=FXMAX(MAJORTICKSIZE,w);                                    // Ticks centered
      }
    w+=EXTRASPACING+EXTRASPACING+4;
    }
  return w+padleft+padright+(border<<1);
  }


// Get default height
FXint FXRuler::getDefaultHeight(){
  FXint tw,th,s=0,h=0;
  if(!(options&RULER_VERTICAL)){        // Horizontal
    if(options&RULER_NUMBERS){
      tw=font->getTextWidth("0",1);     // Ruler should be same width regardless of orientation
      th=font->getFontHeight();
      h=FXMAX(tw,th);
      }
    if(options&(RULER_TICKS_TOP|RULER_TICKS_BOTTOM)){
      if(h) s=BETWEENSPACING;
      if(!(options&RULER_TICKS_TOP)) h=MAJORTICKSIZE+s+h;               // Ticks on bottom
      else if(!(options&RULER_TICKS_BOTTOM)) h=MAJORTICKSIZE+s+h;       // Ticks on top
      else h=FXMAX(MAJORTICKSIZE,h);                                    // Ticks centered
      }
    h+=EXTRASPACING+EXTRASPACING+4;
    }
  return h+padtop+padbottom+(border<<1);
  }


// Draw left arrow, with point at x,y
void FXRuler::drawLeftArrow(FXDCWindow& dc,FXint x,FXint y){
  FXPoint points[3];
  points[0].x=x+ARROWLENGTH;
  points[0].y=y-ARROWLENGTH;
  points[1].x=x+ARROWLENGTH;
  points[1].y=y+ARROWLENGTH;
  points[2].x=x;
  points[2].y=y;
  dc.fillPolygon(points,3);
  }


// Draw right arrow, with point at x,y
void FXRuler::drawRightArrow(FXDCWindow& dc,FXint x,FXint y){
  FXPoint points[3];
  points[0].x=x-ARROWLENGTH+1;
  points[0].y=y-ARROWLENGTH;
  points[1].x=x-ARROWLENGTH+1;
  points[1].y=y+ARROWLENGTH;
  points[2].x=x+1;
  points[2].y=y;
  dc.fillPolygon(points,3);
  }


// Draw up arrow, with point at x,y
void FXRuler::drawUpArrow(FXDCWindow& dc,FXint x,FXint y){
  FXPoint points[3];
  points[0].x=x;
  points[0].y=y-1;
  points[1].x=x-ARROWLENGTH;
  points[1].y=y+ARROWLENGTH;
  points[2].x=x+ARROWLENGTH;
  points[2].y=y+ARROWLENGTH;
  dc.fillPolygon(points,3);
  }


// Draw down arrow, with point at x,y
void FXRuler::drawDownArrow(FXDCWindow& dc,FXint x,FXint y){
  FXPoint points[3];
  points[0].x=x-ARROWLENGTH+1;
  points[0].y=y-ARROWLENGTH+1;
  points[1].x=x+ARROWLENGTH;
  points[1].y=y-ARROWLENGTH+1;
  points[2].x=x;
  points[2].y=y+1;
  dc.fillPolygon(points,3);
  }


// Draw left marker
void FXRuler::drawLeftMarker(FXDCWindow& dc,FXint x,FXint y){
  FXPoint points[6];
  points[0].x=x;
  points[0].y=y;
  points[1].x=x+MARKERLENGTH;
  points[1].y=y-MARKERLENGTH;
  points[2].x=x+MARKERLENGTH+MARKERLENGTH-1;
  points[2].y=y-MARKERLENGTH;
  points[3].x=x+MARKERLENGTH+MARKERLENGTH-1;
  points[3].y=y+MARKERLENGTH;
  points[4].x=x+MARKERLENGTH;
  points[4].y=y+MARKERLENGTH;
  points[5].x=x;
  points[5].y=y;
  dc.setForeground(baseColor);
  dc.fillPolygon(points,5);
  dc.setForeground(textColor);
  dc.drawLines(points,6);
  points[0].x=x+1;
  points[0].y=y;
  points[1].x=x+MARKERLENGTH;
  points[1].y=y+MARKERLENGTH-1;
  points[2].x=x+MARKERLENGTH+MARKERLENGTH-2;
  points[2].y=y+MARKERLENGTH-1;
  points[3].x=x+MARKERLENGTH+MARKERLENGTH-2;
  points[3].y=y-MARKERLENGTH+1;
  dc.setForeground(shadowColor);
  dc.drawLines(points,4);
  points[0].x=x+1;
  points[0].y=y;
  points[1].x=x+MARKERLENGTH;
  points[1].y=y-MARKERLENGTH+1;
  points[2].x=x+MARKERLENGTH+MARKERLENGTH-2;
  points[2].y=y-MARKERLENGTH+1;
  dc.setForeground(hiliteColor);
  dc.drawLines(points,3);
  }


// Draw right marker
void FXRuler::drawRightMarker(FXDCWindow& dc,FXint x,FXint y){
  FXPoint points[6];
  points[0].x=x;
  points[0].y=y;
  points[1].x=x-MARKERLENGTH;
  points[1].y=y-MARKERLENGTH;
  points[2].x=x-MARKERLENGTH-MARKERLENGTH+1;
  points[2].y=y-MARKERLENGTH;
  points[3].x=x-MARKERLENGTH-MARKERLENGTH+1;
  points[3].y=y+MARKERLENGTH;
  points[4].x=x-MARKERLENGTH;
  points[4].y=y+MARKERLENGTH;
  points[5].x=x;
  points[5].y=y;
  dc.setForeground(baseColor);
  dc.fillPolygon(points,5);
  dc.setForeground(textColor);
  dc.drawLines(points,6);
  points[0].x=x-1;
  points[0].y=y;
  points[1].x=x-MARKERLENGTH;
  points[1].y=y+MARKERLENGTH-1;
  points[2].x=x-MARKERLENGTH-MARKERLENGTH+3;
  points[2].y=y+MARKERLENGTH-1;
  dc.setForeground(shadowColor);
  dc.drawLines(points,3);
  points[0].x=x-1;
  points[0].y=y;
  points[1].x=x-MARKERLENGTH;
  points[1].y=y-MARKERLENGTH+1;
  points[2].x=x-MARKERLENGTH-MARKERLENGTH+2;
  points[2].y=y-MARKERLENGTH+1;
  points[3].x=x-MARKERLENGTH-MARKERLENGTH+2;
  points[3].y=y+MARKERLENGTH-1;
  dc.setForeground(hiliteColor);
  dc.drawLines(points,4);
  }


// Draw up marker
void FXRuler::drawUpMarker(FXDCWindow& dc,FXint x,FXint y){
  FXPoint points[6];
  points[0].x=x;
  points[0].y=y;
  points[1].x=x-MARKERLENGTH;
  points[1].y=y+MARKERLENGTH;
  points[2].x=x-MARKERLENGTH;
  points[2].y=y+MARKERLENGTH+MARKERLENGTH-1;
  points[3].x=x+MARKERLENGTH;
  points[3].y=y+MARKERLENGTH+MARKERLENGTH-1;
  points[4].x=x+MARKERLENGTH;
  points[4].y=y+MARKERLENGTH;
  points[5].x=x;
  points[5].y=y;
  dc.setForeground(baseColor);
  dc.fillPolygon(points,5);
  dc.setForeground(textColor);
  dc.drawLines(points,6);
  points[0].x=x;
  points[0].y=y+1;
  points[1].x=x+MARKERLENGTH-1;
  points[1].y=y+MARKERLENGTH;
  points[2].x=x+MARKERLENGTH-1;
  points[2].y=y+MARKERLENGTH+MARKERLENGTH-2;
  points[3].x=x-MARKERLENGTH+1;
  points[3].y=y+MARKERLENGTH+MARKERLENGTH-2;
  dc.setForeground(shadowColor);
  dc.drawLines(points,4);
  points[0].x=x;
  points[0].y=y+1;
  points[1].x=x-MARKERLENGTH+1;
  points[1].y=y+MARKERLENGTH;
  points[2].x=x-MARKERLENGTH+1;
  points[2].y=y+MARKERLENGTH+MARKERLENGTH-3;
  dc.setForeground(hiliteColor);
  dc.drawLines(points,3);
  }


// Draw down marker
void FXRuler::drawDownMarker(FXDCWindow& dc,FXint x,FXint y){
  FXPoint points[6];
  points[0].x=x;
  points[0].y=y;
  points[1].x=x-MARKERLENGTH;
  points[1].y=y-MARKERLENGTH;
  points[2].x=x-MARKERLENGTH;
  points[2].y=y-MARKERLENGTH-MARKERLENGTH+1;
  points[3].x=x+MARKERLENGTH;
  points[3].y=y-MARKERLENGTH-MARKERLENGTH+1;
  points[4].x=x+MARKERLENGTH;
  points[4].y=y-MARKERLENGTH;
  points[5].x=x;
  points[5].y=y;
  dc.setForeground(baseColor);
  dc.fillPolygon(points,5);
  dc.setForeground(textColor);
  dc.drawLines(points,6);
  points[0].x=x;
  points[0].y=y-1;
  points[1].x=x+MARKERLENGTH-1;
  points[1].y=y-MARKERLENGTH;
  points[2].x=x+MARKERLENGTH-1;
  points[2].y=y-MARKERLENGTH-MARKERLENGTH+3;
  dc.setForeground(shadowColor);
  dc.drawLines(points,3);
  points[0].x=x;
  points[0].y=y-1;
  points[1].x=x-MARKERLENGTH+1;
  points[1].y=y-MARKERLENGTH;
  points[2].x=x-MARKERLENGTH+1;
  points[2].y=y-MARKERLENGTH-MARKERLENGTH+2;
  points[3].x=x+MARKERLENGTH-1;
  points[3].y=y-MARKERLENGTH-MARKERLENGTH+2;
  dc.setForeground(hiliteColor);
  dc.drawLines(points,4);
  }


// Handle repaint
long FXRuler::onPaint(FXObject*,FXSelector,void* ptr){
  FXEvent *ev=(FXEvent*)ptr;
  FXDCWindow dc(this,ev);
  FXint docx,docy,docw,doch,boxx,boxy,boxw,boxh,doclo,dochi,p;
  FXdouble lower,upper,inc,tick;

  // Background
  dc.setForeground(baseColor);
  dc.fillRectangle(ev->rect.x,ev->rect.y,ev->rect.w,ev->rect.h);

  boxx=padleft+border;
  boxy=padtop+border;
  boxw=width-padleft-padright-(border<<1);
  boxh=height-padtop-padbottom-(border<<1);

  // Vertically oriented ruler
  if(options&RULER_VERTICAL){

    docx=padleft+border;
    docy=shift+docSpace;
    docw=width-padleft-padright-border-border;
    doch=docSpace+docSize+docSpace;

    boxx=docx;
    boxy=docy+lowerMargin;
    boxw=docw;
    boxh=doch-upperMargin-lowerMargin;

    drawGrooveRectangle(dc,docx,docy,docw,doch+1);      // One pixel extra!

    dc.setForeground(backColor);
    dc.fillRectangle(boxx+2,boxy+2,boxw-4,boxh-1);

    dc.setForeground(shadowColor);
    dc.fillRectangle(boxx,boxy,boxw-2,1);
    dc.fillRectangle(boxx,boxy+boxh-1,boxw-2,1);

    dc.setForeground(borderColor);
    dc.fillRectangle(boxx+1,boxy+1,boxw-3,1);
    dc.fillRectangle(boxx+1,boxy+1,1,boxh-2);

    dc.setForeground(baseColor);
    dc.fillRectangle(boxx+2,boxy+boxh-2,boxw-4,1);

    // Draw optional arrow to signify cursor location
    if(options&RULER_ARROW){
      dc.setForeground(textColor);
      if(options&RULER_TICKS_RIGHT)                     // Ticks on right or center
        drawRightArrow(dc,boxx+boxw-3,arrowPos);
      else if(options&RULER_TICKS_LEFT)                 // Ticks on left
        drawLeftArrow(dc,boxx+2,arrowPos);
      }

    // Draw optional markers for paragraph margins
    if(options&RULER_MARKERS){
      dc.setForeground(textColor);
      drawLeftMarker(dc,boxx+boxw-MARKERLENGTH-MARKERLENGTH+1,boxy+lowerPara);
      drawLeftMarker(dc,boxx+boxw-MARKERLENGTH-MARKERLENGTH+1,boxy+boxh-upperPara-1);
      }
    }

  // Horizontally oriented ruler
  else{

    docx=shift+docSpace;
    docy=border+padtop;
    docw=docSpace+docSize+docSpace;
    doch=height-padtop-padbottom-border-border;

    boxx=docx+lowerMargin;
    boxy=docy;
    boxw=docw-upperMargin-lowerMargin;
    boxh=doch;

    doclo=-lowerMargin;
    dochi=docw-lowerMargin;

    drawGrooveRectangle(dc,docx,docy,docw+1,doch);      // One pixel extra!

    dc.setForeground(backColor);
    dc.fillRectangle(boxx+2,boxy+2,boxw-1,boxh-4);

    dc.setForeground(shadowColor);
    dc.fillRectangle(boxx,boxy,1,boxh-2);
    dc.fillRectangle(boxx+boxw-1,boxy+1,1,boxh-2);

    dc.setForeground(borderColor);
    dc.fillRectangle(boxx+1,boxy+1,boxw-2,1);
    dc.fillRectangle(boxx+1,boxy+1,1,boxh-2);

    dc.setForeground(baseColor);
    dc.fillRectangle(boxx+2,boxy+boxh-2,boxw-3,1);

    // Draw ticks
    if(options&(RULER_TICKS_TOP|RULER_TICKS_BOTTOM)){
      inc=tinyTicks*pixelPerTick;
      lower=doclo;
      upper=dochi;
      dc.setForeground(borderColor);
      for(tick=lower; tick<upper; tick+=inc){
        p=boxx+(FXint)(tick+0.5);
        if((options&RULER_TICKS_TOP)&&(options&RULER_TICKS_BOTTOM)){
          dc.fillRectangle(p,boxy+(boxh>>1)-(MINORTICKSIZE>>1),1,MINORTICKSIZE);
          }
        else if(options&RULER_TICKS_TOP){
          dc.fillRectangle(p,boxy+2,1,MINORTICKSIZE);
          }
        else{
          dc.fillRectangle(p,boxy+boxh-2-MINORTICKSIZE,1,MINORTICKSIZE);
          }
        }
      }

    // Draw optional arrow to signify cursor location
    if(options&RULER_ARROW){
      dc.setForeground(textColor);
      if(options&RULER_TICKS_BOTTOM)                    // Ticks on bottom or center
        drawDownArrow(dc,arrowPos,boxy+boxh-3);
      else if(options&RULER_TICKS_TOP)                  // Ticks on top
        drawUpArrow(dc,arrowPos,boxy+2);
      }

    // Draw optional markers for paragraph margins
    if(options&RULER_MARKERS){
      drawDownMarker(dc,boxx+firstPara,boxy+MARKERLENGTH+MARKERLENGTH-2);
      drawUpMarker(dc,boxx+lowerPara,boxy+boxh-MARKERLENGTH-MARKERLENGTH+1);
      drawUpMarker(dc,boxx+boxw-upperPara-1,boxy+boxh-MARKERLENGTH-MARKERLENGTH+1);
      }
    }

  // Frame it
  drawFrame(dc,0,0,width,height);
  return 1;
  }


// Pressed LEFT button
long FXRuler::onLeftBtnPress(FXObject*,FXSelector,void* ptr){
  flags&=~FLAG_TIP;
  if(isEnabled()){
    grab();
    if(target && target->handle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;
    flags&=~FLAG_UPDATE;
    return 1;
    }
  return 0;
  }


// Released Left button
long FXRuler::onLeftBtnRelease(FXObject*,FXSelector,void* ptr){
  if(isEnabled()){
    ungrab();
    flags|=FLAG_UPDATE;
    if(target && target->handle(this,FXSEL(SEL_LEFTBUTTONPRESS,message),ptr)) return 1;
    return 1;
    }
  return 0;
  }


// Moving
long FXRuler::onMotion(FXObject*,FXSelector,void*){
  if(isEnabled()){
/*
    if(options&RULER_HORIZONTAL){
      setDefaultCursor(getApp()->getDefaultCursor(DEF_DRAGH_CURSOR));
      }
    else{
      setDefaultCursor(getApp()->getDefaultCursor(DEF_DRAGV_CURSOR));
      }
    setDefaultCursor(getApp()->getDefaultCursor(DEF_ARROW_CURSOR));
*/
    return 1;
    }
  return 0;
  }


// Change arrow location
void FXRuler::setValue(FXint val){
  FXint doct,docb,docr,docl;
  if(options&RULER_VERTICAL){
    doct=shift+docSpace;
    docb=doct+docSpace+docSize+docSpace;
    if(val<doct) val=doct;
    if(val>docb) val=docb;
    if(arrowPos!=val){
      if(options&RULER_ARROW){
        update(padleft+border,arrowPos-ARROWLENGTH,width-padleft-padright-(border<<1),ARROWBASE);
        update(padleft+border,val-ARROWLENGTH,width-padleft-padright-(border<<1),ARROWBASE);
        }
      arrowPos=val;
      }
    }
  else{
    docl=shift+docSpace;
    docr=docl+docSpace+docSize+docSpace;
    if(val<docl) val=docl;
    if(val>docr) val=docr;
    if(arrowPos!=val){
      if(options&RULER_ARROW){
        update(arrowPos-ARROWLENGTH,padtop+border,ARROWBASE,height-padtop-padbottom-(border<<1));
        update(val-ARROWLENGTH,padtop+border,ARROWBASE,height-padtop-padbottom-(border<<1));
        }
      arrowPos=val;
      }
    }
  }


// Update value from a message
long FXRuler::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setValue((FXint)(FXival)ptr);
  return 1;
  }


// Update value from a message
long FXRuler::onCmdSetIntValue(FXObject*,FXSelector,void* ptr){
  setValue(*((FXint*)ptr));
  return 1;
  }


// Obtain value from text field
long FXRuler::onCmdGetIntValue(FXObject*,FXSelector,void* ptr){
  *((FXint*)ptr)=getValue();
  return 1;
  }


// Set help using a message
long FXRuler::onCmdSetHelp(FXObject*,FXSelector,void* ptr){
  setHelpText(*((FXString*)ptr));
  return 1;
  }


// Get help using a message
long FXRuler::onCmdGetHelp(FXObject*,FXSelector,void* ptr){
  *((FXString*)ptr)=getHelpText();
  return 1;
  }


// Set tip using a message
long FXRuler::onCmdSetTip(FXObject*,FXSelector,void* ptr){
  setTipText(*((FXString*)ptr));
  return 1;
  }


// Get tip using a message
long FXRuler::onCmdGetTip(FXObject*,FXSelector,void* ptr){
  *((FXString*)ptr)=getTipText();
  return 1;
  }


// We were asked about status text
long FXRuler::onQueryHelp(FXObject* sender,FXSelector,void*){
  if(!help.empty() && (flags&FLAG_HELP)){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&help);
    return 1;
    }
  return 0;
  }


// We were asked about tip text
long FXRuler::onQueryTip(FXObject* sender,FXSelector,void*){
  if(!tip.empty() && (flags&FLAG_TIP)){
    sender->handle(this,FXSEL(SEL_COMMAND,ID_SETSTRINGVALUE),(void*)&tip);
    return 1;
    }
  return 0;
  }


// Change the font
void FXRuler::setFont(FXFont *fnt){
  if(!fnt){ fxerror("%s::setFont: NULL font specified.\n",getClassName()); }
  if(font!=fnt){
    font=fnt;
    recalc();
    update();
    }
  }


// Set text color
void FXRuler::setTextColor(FXColor clr){
  if(clr!=textColor){
    textColor=clr;
    update();
    }
  }


// Set ruler style
void FXRuler::setRulerStyle(FXuint style){
  FXuint opts=(options&~RULER_MASK) | (style&RULER_MASK);
  if(options!=opts){
    options=opts;
    recalc();
    update();
    }
  }


// Get ruler style
FXuint FXRuler::getRulerStyle() const {
  return (options&RULER_MASK);
  }


// Change help text
void FXRuler::setHelpText(const FXString& text){
  help=text;
  }


// Change tip text
void FXRuler::setTipText(const FXString& text){
  tip=text;
  }


// Save object to stream
void FXRuler::save(FXStream& store) const {
  FXFrame::save(store);
  store << font;
  store << textColor;
  store << tip;
  store << help;
  }


// Load object from stream
void FXRuler::load(FXStream& store){
  FXFrame::load(store);
  store >> font;
  store >> textColor;
  store >> tip;
  store >> help;
  }


// Destroy label
FXRuler::~FXRuler(){
  font=(FXFont*)-1L;
  }

}
