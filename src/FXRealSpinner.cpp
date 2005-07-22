/********************************************************************************
*                                                                               *
*             R e a l - V a l u e d   S p i n n e r  W i d g e t                *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003,2005 by Bill Baxter.   All Rights Reserved.                *
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
* $Id: FXRealSpinner.cpp,v 1.27 2005/01/16 16:06:07 fox Exp $                   *
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
#include "FXLabel.h"
#include "FXTextField.h"
#include "FXArrowButton.h"
#include "FXRealSpinner.h"


/*
  Notes:
  - Based on Lyle's FXSpinner.
  - Changed to use arrow buttons because getting nice operation
    with the dial was impossible.
  - Wheel mouse offers incrementation/decrementation in nice
    multiples of increment, makes it very quick to get a value.
  - Needed rather complicated mechanism so as to ensure a "wheely"
    up and down gets you back to EXACTLY the same number!
*/

#define BUTTONWIDTH 14

#define REALSPINNER_MASK (REALSPIN_CYCLIC|REALSPIN_NOTEXT|REALSPIN_NOMAX|REALSPIN_NOMIN|REALSPIN_LOG)



/*******************************************************************************/

namespace FX {


//  Message map
FXDEFMAP(FXRealSpinner) FXRealSpinnerMap[]={
  FXMAPFUNC(SEL_KEYPRESS,0,FXRealSpinner::onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE,0,FXRealSpinner::onKeyRelease),
  FXMAPFUNC(SEL_FOCUS_SELF,0,FXRealSpinner::onFocusSelf),
//  FXMAPFUNC(SEL_UPDATE,FXRealSpinner::ID_ENTRY,FXRealSpinner::onUpdEntry),
  FXMAPFUNC(SEL_COMMAND,FXRealSpinner::ID_ENTRY,FXRealSpinner::onCmdEntry),
  FXMAPFUNC(SEL_CHANGED,FXRealSpinner::ID_ENTRY,FXRealSpinner::onChgEntry),
  FXMAPFUNC(SEL_MOUSEWHEEL,FXRealSpinner::ID_ENTRY,FXRealSpinner::onWheelEntry),
  FXMAPFUNC(SEL_UPDATE,FXRealSpinner::ID_INCREMENT,FXRealSpinner::onUpdIncrement),
  FXMAPFUNC(SEL_COMMAND,FXRealSpinner::ID_INCREMENT,FXRealSpinner::onCmdIncrement),
  FXMAPFUNC(SEL_UPDATE,FXRealSpinner::ID_DECREMENT,FXRealSpinner::onUpdDecrement),
  FXMAPFUNC(SEL_COMMAND,FXRealSpinner::ID_DECREMENT,FXRealSpinner::onCmdDecrement),
  FXMAPFUNC(SEL_COMMAND,FXRealSpinner::ID_SETVALUE,FXRealSpinner::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXRealSpinner::ID_SETINTVALUE,FXRealSpinner::onCmdSetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXRealSpinner::ID_GETINTVALUE,FXRealSpinner::onCmdGetIntValue),
  FXMAPFUNC(SEL_COMMAND,FXRealSpinner::ID_SETINTRANGE,FXRealSpinner::onCmdSetIntRange),
  FXMAPFUNC(SEL_COMMAND,FXRealSpinner::ID_GETINTRANGE,FXRealSpinner::onCmdGetIntRange),
  FXMAPFUNC(SEL_COMMAND,FXRealSpinner::ID_SETREALVALUE,FXRealSpinner::onCmdSetRealValue),
  FXMAPFUNC(SEL_COMMAND,FXRealSpinner::ID_GETREALVALUE,FXRealSpinner::onCmdGetRealValue),
  FXMAPFUNC(SEL_COMMAND,FXRealSpinner::ID_SETREALRANGE,FXRealSpinner::onCmdSetRealRange),
  FXMAPFUNC(SEL_COMMAND,FXRealSpinner::ID_GETREALRANGE,FXRealSpinner::onCmdGetRealRange),
  };


// Object implementation
FXIMPLEMENT(FXRealSpinner,FXPacker,FXRealSpinnerMap,ARRAYNUMBER(FXRealSpinnerMap))


// Construct spinner out of two buttons and a text field
FXRealSpinner::FXRealSpinner(){
  flags|=FLAG_ENABLED;
  textField=(FXTextField*)-1L;
  upButton=(FXArrowButton*)-1L;
  downButton=(FXArrowButton*)-1L;
  range[0]=-DBL_MAX;
  range[1]= DBL_MAX;
  base=0.0;
  incr=1.0;
  pos=0.0;
  ticks=0;
  }


// Construct spinner out of dial and a text field
FXRealSpinner::FXRealSpinner(FXComposite *p,FXint cols,FXObject *tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb):
  FXPacker(p,opts,x,y,w,h,0,0,0,0,0,0){
  flags|=FLAG_ENABLED;
  target=tgt;
  message=sel;
  textField=new FXTextField(this,cols,this,ID_ENTRY,TEXTFIELD_REAL|JUSTIFY_RIGHT,0,0,0,0,pl,pr,pt,pb);
  upButton=new FXArrowButton(this,this,FXRealSpinner::ID_INCREMENT,FRAME_RAISED|FRAME_THICK|ARROW_UP|ARROW_REPEAT, 0,0,0,0, 0,0,0,0);
  downButton=new FXArrowButton(this,this,FXRealSpinner::ID_DECREMENT,FRAME_RAISED|FRAME_THICK|ARROW_DOWN|ARROW_REPEAT, 0,0,0,0, 0,0,0,0);
  range[0]=(options&REALSPIN_NOMIN) ? -DBL_MAX : 0.0;
  range[1]=(options&REALSPIN_NOMAX) ?  DBL_MAX : 100.0;
  textField->setText("0");
  base=0.0;
  incr=1.0;
  pos=0.0;
  ticks=0;
  }


// Get default width
FXint FXRealSpinner::getDefaultWidth(){
  FXint tw=0;
  if(!(options&REALSPIN_NOTEXT)) tw=textField->getDefaultWidth();
  return tw+BUTTONWIDTH+(border<<1);
  }


// Get default height
FXint FXRealSpinner::getDefaultHeight(){
  return textField->getDefaultHeight()+(border<<1);
  }


// Enable the widget
void FXRealSpinner::enable(){
  if(!(flags&FLAG_ENABLED)){
    FXPacker::enable();
    textField->enable();
    upButton->enable();
    downButton->enable();
    }
  }


// Disable the widget
void FXRealSpinner::disable(){
  if(flags&FLAG_ENABLED){
    FXPacker::disable();
    textField->disable();
    upButton->disable();
    downButton->disable();
    }
  }


// Recompute layout
void FXRealSpinner::layout(){
  FXint buttonWidth,buttonHeight,textWidth,textHeight;

  textHeight=height-2*border;
  buttonHeight=textHeight>>1;

  // Only the dial:- place dial to take up the whole space!
  if(options&REALSPIN_NOTEXT){
    buttonWidth=width-2*border;
    upButton->position(border,border,buttonWidth,buttonHeight);
    downButton->position(border,height-buttonHeight-border,buttonWidth,buttonHeight);
    }

  // Dial plus the text; dial is default width, text stretches to fill the rest
  else{
    buttonWidth=BUTTONWIDTH;
    textWidth=width-buttonWidth-2*border;
    textField->position(border,border,textWidth,textHeight);
    upButton->position(border+textWidth,border,buttonWidth,buttonHeight);
    downButton->position(border+textWidth,height-buttonHeight-border,buttonWidth,buttonHeight);
    }
  flags&=~FLAG_DIRTY;
  }


// Respond to increment message
long FXRealSpinner::onUpdIncrement(FXObject* sender,FXSelector,void*){
  if(isEnabled() && ((options&REALSPIN_CYCLIC) || (pos<range[1])))
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
  return 1;
  }


// Respond to increment message
long FXRealSpinner::onCmdIncrement(FXObject*,FXSelector,void*){
  if(isEnabled() && isEditable()){
    increment();
    if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)&pos);
    return 1;
    }
  return 0;
  }


// Disable decrement if at low end already
long FXRealSpinner::onUpdDecrement(FXObject* sender,FXSelector,void*){
  if(isEnabled() && ((options&REALSPIN_CYCLIC) || (range[0]<pos)))
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),NULL);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),NULL);
  return 1;
  }


// Respond to decrement message
long FXRealSpinner::onCmdDecrement(FXObject*,FXSelector,void*){
  if(isEnabled() && isEditable()){
    decrement();
    if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)&pos);
    return 1;
    }
  return 0;
  }


// Rolling mouse wheel in text field behaves as if inside dial
long FXRealSpinner::onWheelEntry(FXObject*,FXSelector,void* ptr){
  if(isEnabled() && isEditable()){
    if(((FXEvent*)ptr)->code>0)
      increment();
    else
      decrement();
    if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)&pos);
    return 1;
    }
  return 0;
  }


// Text field changed
long FXRealSpinner::onChgEntry(FXObject*,FXSelector,void*){
  register FXdouble value=FXDoubleVal(textField->getText());
  if(value<range[0]) value=range[0];
  if(value>range[1]) value=range[1];
  if(value!=pos){
    pos=base=value;
    ticks=0;
    if(target) target->tryHandle(this,FXSEL(SEL_CHANGED,message),(void*)&pos);
    }
  return 1;
  }


// Text field command
long FXRealSpinner::onCmdEntry(FXObject*,FXSelector,void*){
  textField->setText(FXStringVal(pos));       // Put back adjusted value
  if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)&pos);
  return 1;
  }


// Keyboard press
long FXRealSpinner::onKeyPress(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(isEnabled()){
    if(target && target->tryHandle(this,FXSEL(SEL_KEYPRESS,message),ptr)) return 1;
    switch(event->code){
      case KEY_Up:
      case KEY_KP_Up:
        if(isEditable()){
          increment();
          if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)&pos);
          }
        else{
          getApp()->beep();
          }
        return 1;
      case KEY_Down:
      case KEY_KP_Down:
        if(isEditable()){
          decrement();
          if(target) target->tryHandle(this,FXSEL(SEL_COMMAND,message),(void*)&pos);
          }
        else{
          getApp()->beep();
          }
        return 1;
      default:
        return textField->handle(sender,sel,ptr);
      }
    }
  return 0;
  }


// Keyboard release
long FXRealSpinner::onKeyRelease(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  if(isEnabled()){
    if(target && target->tryHandle(this,FXSEL(SEL_KEYRELEASE,message),ptr)) return 1;
    switch(event->code){
      case KEY_Up:
      case KEY_KP_Up:
      case KEY_Down:
      case KEY_KP_Down:
        return 1;
      default:
        return textField->handle(sender,sel,ptr);
      }
    }
  return 0;
  }


// Force focus on the text field
long FXRealSpinner::onFocusSelf(FXObject* sender,FXSelector,void* ptr){
  return textField->tryHandle(sender,FXSEL(SEL_FOCUS_SELF,0),ptr);
  }


// Update value from a message
long FXRealSpinner::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setValue((FXdouble)(FXival)ptr);
  return 1;
  }


// Update value from a message
long FXRealSpinner::onCmdSetIntValue(FXObject*,FXSelector,void* ptr){
  setValue((FXdouble) *((FXint*)ptr));
  return 1;
  }


// Obtain value from spinner
long FXRealSpinner::onCmdGetIntValue(FXObject*,FXSelector,void* ptr){
  *((FXint*)ptr)=(FXint)getValue();
  return 1;
  }


// Update range from a message
long FXRealSpinner::onCmdSetIntRange(FXObject*,FXSelector,void* ptr){
  setRange((FXdouble)((FXint*)ptr)[0],(FXdouble)((FXint*)ptr)[1]);
  return 1;
  }


// Get range with a message
long FXRealSpinner::onCmdGetIntRange(FXObject*,FXSelector,void* ptr){
  ((FXint*)ptr)[0]=(FXint)range[0];
  ((FXint*)ptr)[1]=(FXint)range[1];
  return 1;
  }


// Update value from a message
long FXRealSpinner::onCmdSetRealValue(FXObject*,FXSelector,void* ptr){
  setValue(*((FXdouble*)ptr));
  return 1;
  }


// Obtain value from spinner
long FXRealSpinner::onCmdGetRealValue(FXObject*,FXSelector,void* ptr){
  *((FXdouble*)ptr)=getValue();
  return 1;
  }


// Update range from a message
long FXRealSpinner::onCmdSetRealRange(FXObject*,FXSelector,void* ptr){
  setRange(((FXdouble*)ptr)[0],((FXdouble*)ptr)[1]);
  return 1;
  }


// Get range with a message
long FXRealSpinner::onCmdGetRealRange(FXObject*,FXSelector,void* ptr){
  ((FXdouble*)ptr)[0]=range[0];
  ((FXdouble*)ptr)[1]=range[1];
  return 1;
  }


// Increment spinner
void FXRealSpinner::increment(){
  if(range[0]<range[1]){
    if(options&REALSPIN_CYCLIC){
      FXdouble value=++ticks*incr+base;
      pos=value-floor((value-range[0])/(range[1]-range[0]))*(range[1]-range[0]);
      }
    else if(options&REALSPIN_LOG){
      pos=base*pow(10.0,++ticks*incr);
      if(pos>range[1]){pos=base=range[1];ticks=0;}
      }
    else{
      pos=++ticks*incr+base;
      if(pos>range[1]){pos=base=range[1];ticks=0;}
      }
    textField->setText(FXStringVal(pos));
    }
  }


// Decrement spinner
void FXRealSpinner::decrement(){
  if(range[0]<range[1]){
    if(options&REALSPIN_CYCLIC){
      FXdouble value=--ticks*incr+base;
      pos=value-floor((value-range[0])/(range[1]-range[0]))*(range[1]-range[0]);
      }
    else if(options&REALSPIN_LOG){
      pos=base*pow(10.0,--ticks*incr);
      if(pos<range[0]){pos=base=range[0];ticks=0;}
      }
    else{
      pos=--ticks*incr+base;
      if(pos<range[0]){pos=base=range[0];ticks=0;}
      }
    textField->setText(FXStringVal(pos));
    }
  }


// True if spinner is cyclic
FXbool FXRealSpinner::isCyclic() const {
  return (options&REALSPIN_CYCLIC)!=0;
  }


// Set spinner cyclic mode
void FXRealSpinner::setCyclic(FXbool cyclic){
  if(cyclic) options|=REALSPIN_CYCLIC; else options&=~REALSPIN_CYCLIC;
  }


// Set spinner range; this also revalidates the position,
void FXRealSpinner::setRange(FXdouble lo,FXdouble hi){
  if(lo>hi){ fxerror("%s::setRange: trying to set negative range.\n",getClassName()); }
  if(range[0]!=lo || range[1]!=hi){
    range[0]=lo;
    range[1]=hi;
    setValue(pos);
    }
  }


// Set new value
void FXRealSpinner::setValue(FXdouble value){
  if(value<range[0]) value=range[0];
  if(value>range[1]) value=range[1];
  if(pos!=value){
    textField->setText(FXStringVal(value));
    pos=base=value; ticks=0;
    }
  }


// Change value increment
void FXRealSpinner::setIncrement(FXdouble inc){
  if(inc<=0.0){ fxerror("%s::setIncrement: negative or zero increment specified.\n",getClassName()); }
  incr=inc; base=pos; ticks=0;
  }


// True if text supposed to be visible
FXbool FXRealSpinner::isTextVisible() const {
  return textField->shown();
  }


// Change text visibility
void FXRealSpinner::setTextVisible(FXbool shown){
  FXuint opts=shown?(options&~REALSPIN_NOTEXT):(options|REALSPIN_NOTEXT);
  if(options!=opts){
    options=opts;
    recalc();
    }
  }


// Set the font used in the text field|
void FXRealSpinner::setFont(FXFont *fnt) {
  textField->setFont(fnt);
  }


// Return the font used in the text field
FXFont *FXRealSpinner::getFont() const {
  return textField->getFont();
  }


// Set help text
void FXRealSpinner::setHelpText(const FXString&  text){
  textField->setHelpText(text);
  upButton->setHelpText(text);
  downButton->setHelpText(text);
  }


// Get help text
const FXString& FXRealSpinner::getHelpText() const {
  return textField->getHelpText();
  }


// Set tip text
void FXRealSpinner::setTipText(const FXString&  text){
  textField->setTipText(text);
  upButton->setTipText(text);
  downButton->setTipText(text);
  }



// Get tip text
const FXString& FXRealSpinner::getTipText() const {
  return textField->getTipText();
  }


// Change spinner style
void FXRealSpinner::setSpinnerStyle(FXuint style){
  FXuint opts=(options&~REALSPINNER_MASK) | (style&REALSPINNER_MASK);
  if(options!=opts){
    if(opts&REALSPIN_NOMIN) range[0]=-DBL_MAX;
    if(opts&REALSPIN_NOMAX) range[1]=DBL_MAX;
    options=opts;
    recalc();
    }
  }


// Get spinner style
FXuint FXRealSpinner::getSpinnerStyle() const {
  return (options&REALSPINNER_MASK);
  }


// Allow editing of the text field
void FXRealSpinner::setEditable(FXbool edit){
  textField->setEditable(edit);
  }


// Return TRUE if text field is editable
FXbool FXRealSpinner::isEditable() const {
  return textField->isEditable();
  }

// Change color of the up arrow
void FXRealSpinner::setUpArrowColor(FXColor clr){
  upButton->setArrowColor(clr);
  }

// Return color of the up arrow
FXColor FXRealSpinner::getUpArrowColor() const {
  return upButton->getArrowColor();
  }

// Change color of the down arrow
void FXRealSpinner::setDownArrowColor(FXColor clr){
  downButton->setArrowColor(clr);
  }

// Return color of the the down arrow
FXColor FXRealSpinner::getDownArrowColor() const {
  return downButton->getArrowColor();
  }

// Change text color
void FXRealSpinner::setTextColor(FXColor clr){
  textField->setTextColor(clr);
  }

// Return text color
FXColor FXRealSpinner::getTextColor() const {
  return textField->getTextColor();
  }

// Change selected background color
void FXRealSpinner::setSelBackColor(FXColor clr){
  textField->setSelBackColor(clr);
  }

// Return selected background color
FXColor FXRealSpinner::getSelBackColor() const {
  return textField->getSelBackColor();
  }

// Change selected text color
void FXRealSpinner::setSelTextColor(FXColor clr){
  textField->setSelTextColor(clr);
  }

// Return selected text color
FXColor FXRealSpinner::getSelTextColor() const {
  return textField->getSelTextColor();
  }

// Changes the cursor color
void FXRealSpinner::setCursorColor(FXColor clr){
  textField->setCursorColor(clr);
  }

// Return the cursor color
FXColor FXRealSpinner::getCursorColor() const {
  return textField->getCursorColor();
  }


// Change number of columns
void FXRealSpinner::setNumColumns(FXint ncols){
  textField->setNumColumns(ncols);
  }


// Return number of columns
FXint FXRealSpinner::getNumColumns() const {
  return textField->getNumColumns();
  }


// Save object to stream
void FXRealSpinner::save(FXStream& store) const {
  FXPacker::save(store);
  store << textField;
  store << upButton;
  store << downButton;
  store << range[0] << range[1];
  store << incr;
  store << pos;
  }


// Load object from stream
void FXRealSpinner::load(FXStream& store){
  FXPacker::load(store);
  store >> textField;
  store >> upButton;
  store >> downButton;
  store >> range[0] >> range[1];
  store >> incr;
  store >> pos;
  }


// Destruct spinner:- trash it!
FXRealSpinner::~FXRealSpinner(){
  textField=(FXTextField*)-1L;
  upButton=(FXArrowButton*)-1L;
  downButton=(FXArrowButton*)-1L;
  }

}
