/********************************************************************************
*                                                                               *
*                         I n p u t   D i a l o g   B o x                       *
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
* $Id: FXInputDialog.cpp,v 1.22 2004/02/08 17:29:06 fox Exp $                   *
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
#include "FXAccelTable.h"
#include "FXHash.h"
#include "FXApp.h"
#include "FXGIFIcon.h"
#include "FXSeparator.h"
#include "FXLabel.h"
#include "FXButton.h"
#include "FXTextField.h"
#include "FXPacker.h"
#include "FXHorizontalFrame.h"
#include "FXVerticalFrame.h"
#include "FXInputDialog.h"

/*
  Notes:
  - This is a useful little class which we should probably have created
    sooner; I just didn't think of it until now.
*/

// Padding for buttons
#define HORZ_PAD 20
#define VERT_PAD 2



/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXInputDialog) FXInputDialogMap[]={
  FXMAPFUNC(SEL_COMMAND,FXDialogBox::ID_ACCEPT,FXInputDialog::onCmdAccept),
  };


// Object implementation
FXIMPLEMENT(FXInputDialog,FXDialogBox,FXInputDialogMap,ARRAYNUMBER(FXInputDialogMap))




// Create message box
FXInputDialog::FXInputDialog(FXWindow* owner,const FXString& caption,const FXString& label,FXIcon* ic,FXuint opts,FXint x,FXint y,FXint w,FXint h):
  FXDialogBox(owner,caption,opts|DECOR_TITLE|DECOR_BORDER,x,y,w,h,10,10,10,10, 10,10){
  FXuint textopts=TEXTFIELD_ENTER_ONLY|FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_X;
  FXHorizontalFrame* buttons=new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0,0,0,0,0);
  new FXButton(buttons,"&OK",NULL,this,ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y|LAYOUT_RIGHT,0,0,0,0,HORZ_PAD,HORZ_PAD,VERT_PAD,VERT_PAD);
  new FXButton(buttons,"&Cancel",NULL,this,ID_CANCEL,BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK|LAYOUT_CENTER_Y|LAYOUT_RIGHT,0,0,0,0,HORZ_PAD,HORZ_PAD,VERT_PAD,VERT_PAD);
  new FXHorizontalSeparator(this,SEPARATOR_GROOVE|LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X);
  FXHorizontalFrame* toppart=new FXHorizontalFrame(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0, 0,0,0,0, 10,10);
  new FXLabel(toppart,NULL,ic,ICON_BEFORE_TEXT|JUSTIFY_CENTER_X|JUSTIFY_CENTER_Y|LAYOUT_FILL_Y|LAYOUT_FILL_X);
  FXVerticalFrame* entry=new FXVerticalFrame(toppart,LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0, 0,0,0,0);
  new FXLabel(entry,label,NULL,JUSTIFY_LEFT|ICON_BEFORE_TEXT|LAYOUT_TOP|LAYOUT_LEFT|LAYOUT_FILL_X);
  if(options&INPUTDIALOG_PASSWORD) textopts|=TEXTFIELD_PASSWD;
  if(options&INPUTDIALOG_INTEGER) textopts|=TEXTFIELD_INTEGER|JUSTIFY_RIGHT;
  if(options&INPUTDIALOG_REAL) textopts|=TEXTFIELD_REAL|JUSTIFY_RIGHT;
  input=new FXTextField(entry,20,this,ID_ACCEPT,textopts,0,0,0,0, 8,8,4,4);
  limlo=1.0;
  limhi=0.0;
  }


// Get input string
FXString FXInputDialog::getText() const {
  return input->getText();
  }


// Set input string
void FXInputDialog::setText(const FXString& text){
  input->setText(text);
  }


// Change number of visible columns of text
void FXInputDialog::setNumColumns(FXint num){
  input->setNumColumns(num);
  }


// Return number of visible columns of text
FXint FXInputDialog::getNumColumns() const {
  return input->getNumColumns();
  }


// We have to do this so as to force the initial text to be seleced
FXuint FXInputDialog::execute(FXuint placement){
  create();
  input->setFocus();
  input->selectAll();
  show(placement);
  return getApp()->runModalFor(this);
  }


// Close dialog with an accept, after an validation of the number
long FXInputDialog::onCmdAccept(FXObject* sender,FXSelector sel,void* ptr){
  if(options&INPUTDIALOG_INTEGER){
    FXint iresult;
    if((sscanf(input->getText().text(),"%d",&iresult)!=1) || (limlo<=limhi && (iresult<limlo || limhi<iresult))){
      input->setFocus();
      input->selectAll();
      getApp()->beep();
      return 1;
      }
    }
  else if(options&INPUTDIALOG_REAL){
    FXdouble dresult;
    if((sscanf(input->getText().text(),"%lf",&dresult)!=1) || (limlo<=limhi && (dresult<limlo || limhi<dresult))){
      input->setFocus();
      input->selectAll();
      getApp()->beep();
      return 1;
      }
    }
  FXDialogBox::onCmdAccept(sender,sel,ptr);
  return 1;
  }


/*******************************************************************************/


// // Cool looking icon
// static const unsigned char askIcon[]={
//   0x47,0x49,0x46,0x38,0x37,0x61,0x20,0x00,0x20,0x00,0xf1,0x00,0x00,0xb2,0xc0,0xdc,
//   0x00,0x00,0x00,0xff,0xff,0x00,0x00,0x00,0x00,0x2c,0x00,0x00,0x00,0x00,0x20,0x00,
//   0x20,0x00,0x00,0x02,0x70,0x84,0x8f,0x16,0xcb,0x9d,0x0f,0x55,0x10,0xb4,0xda,0x15,
//   0xe3,0xaa,0x6d,0xdf,0x90,0x21,0x13,0x85,0x25,0x1e,0x17,0x02,0xa3,0x00,0x66,0x2b,
//   0x9b,0xae,0xad,0x66,0xc1,0x34,0x99,0xaa,0x97,0x8b,0xc6,0xfb,0x6d,0x0b,0xbd,0x66,
//   0x8f,0x61,0xa9,0x58,0x23,0x9a,0x6a,0x24,0xa5,0xae,0xc7,0x63,0x2a,0x5f,0xc1,0xe8,
//   0x4f,0x91,0xcc,0x49,0xa0,0x4f,0xae,0x96,0x21,0xca,0x6a,0x85,0xe2,0x31,0x10,0x67,
//   0x3e,0x57,0xd3,0xcb,0x2b,0x3b,0xec,0x7e,0x1f,0xa8,0x4e,0x39,0x5d,0x0e,0x91,0xe1,
//   0x35,0xc7,0xbd,0xff,0x0f,0x08,0x06,0x38,0xe7,0xb5,0x77,0x17,0x58,0xf6,0xa7,0x37,
//   0x28,0x51,0x07,0x51,0x00,0x00,0x3b
//   };


// Obtain a string
FXbool FXInputDialog::getString(FXString& result,FXWindow* owner,const FXString& caption,const FXString& label,FXIcon* icon){
  FXInputDialog inputdialog(owner,caption,label,icon,INPUTDIALOG_STRING,0,0,0,0);
  inputdialog.setText(result);
  if(inputdialog.execute()){
    result=inputdialog.getText();
    return TRUE;
    }
  return FALSE;
  }


// Obtain an integer
FXbool FXInputDialog::getInteger(FXint& result,FXWindow* owner,const FXString& caption,const FXString& label,FXIcon* icon,FXint lo,FXint hi){
  FXInputDialog inputdialog(owner,caption,label,icon,INPUTDIALOG_INTEGER,0,0,0,0);
  inputdialog.setLimits(lo,hi);
  inputdialog.setText(FXStringVal(FXCLAMP(lo,result,hi)));
  if(inputdialog.execute()){
    result=FXIntVal(inputdialog.getText());
    return TRUE;
    }
  return FALSE;
  }


// Obtain a real
FXbool FXInputDialog::getReal(FXdouble& result,FXWindow* owner,const FXString& caption,const FXString& label,FXIcon* icon,FXdouble lo,FXdouble hi){
  FXInputDialog inputdialog(owner,caption,label,icon,INPUTDIALOG_REAL,0,0,0,0);
  inputdialog.setLimits(lo,hi);
  inputdialog.setText(FXStringVal(FXCLAMP(lo,result,hi),10));
  if(inputdialog.execute()){
    result=FXDoubleVal(inputdialog.getText());
    return TRUE;
    }
  return FALSE;
  }

}
