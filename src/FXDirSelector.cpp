/********************************************************************************
*                                                                               *
*              D i r e c t o r y   S e l e c t i o n   W i d g e t              *
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
* $Id: FXDirSelector.cpp,v 1.28 2004/02/08 17:29:06 fox Exp $                   *
********************************************************************************/
#ifndef BUILDING_TCOMMON
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxkeys.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXFile.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXAccelTable.h"
#include "FXHash.h"
#include "FXApp.h"
#include "FXFont.h"
#include "FXGIFIcon.h"
#include "FXFrame.h"
#include "FXLabel.h"
#include "FXButton.h"
#include "FXMenuButton.h"
#include "FXComposite.h"
#include "FXPacker.h"
#include "FXShell.h"
#include "FXPopup.h"
#include "FXTopWindow.h"
#include "FXDialogBox.h"
#include "FXScrollBar.h"
#include "FXTextField.h"
#include "FXScrollArea.h"
#include "FXTreeList.h"
#include "FXTreeListBox.h"
#include "FXVerticalFrame.h"
#include "FXHorizontalFrame.h"
#include "FXDirList.h"
#include "FXList.h"
#include "FXListBox.h"
#include "FXDirSelector.h"


/*
  Notes:
  - Need a button to quickly hop to home directory.
  - Need a button to hop to current working directory.
  - Keep list of recently visited places.
  - Need button to hide/show hidden directories.
  - Need option to show files and directories, instead of only
    directories.
*/



/*******************************************************************************/

namespace FX {

// Map
FXDEFMAP(FXDirSelector) FXDirSelectorMap[]={
  FXMAPFUNC(SEL_COMMAND,FXDirSelector::ID_DIRNAME,FXDirSelector::onCmdName),
  FXMAPFUNC(SEL_OPENED,FXDirSelector::ID_DIRLIST,FXDirSelector::onCmdOpened),
  FXMAPFUNC(SEL_COMMAND,FXDirSelector::ID_HOME,FXDirSelector::onCmdHome),
  FXMAPFUNC(SEL_COMMAND,FXDirSelector::ID_WORK,FXDirSelector::onCmdWork),
  FXMAPFUNC(SEL_COMMAND,FXDirSelector::ID_DIRECTORY_UP,FXDirSelector::onCmdDirectoryUp),
  };


// Implementation
FXIMPLEMENT(FXDirSelector,FXPacker,FXDirSelectorMap,ARRAYNUMBER(FXDirSelectorMap))


// Make directory selector widget
FXDirSelector::FXDirSelector(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):
  FXPacker(p,opts,x,y,w,h){
  FXString currentdirectory=FXFile::getCurrentDirectory();
  FXAccelTable *table=getShell()->getAccelTable();
  target=tgt;
  message=sel;
  FXHorizontalFrame *buttons=new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH);
  accept=new FXButton(buttons,"&Accept",NULL,NULL,0,LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0,20,20);
  cancel=new FXButton(buttons,"&Cancel",NULL,NULL,0,LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0,20,20);
  new FXLabel(this,"&Directory name:",NULL,JUSTIFY_LEFT|LAYOUT_SIDE_TOP|LAYOUT_FILL_X);
  dirname=new FXTextField(this,25,this,ID_DIRNAME,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|FRAME_SUNKEN|FRAME_THICK);
  FXHorizontalFrame *frame=new FXHorizontalFrame(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_SUNKEN|FRAME_THICK,0,0,0,0,0,0,0,0);
  dirbox=new FXDirList(frame,this,ID_DIRLIST,LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_TOP|TREELIST_SHOWS_LINES|TREELIST_SHOWS_BOXES|TREELIST_BROWSESELECT);
  if(table){
    table->addAccel(MKUINT(KEY_BackSpace,0),this,FXSEL(SEL_COMMAND,ID_DIRECTORY_UP));
    table->addAccel(MKUINT(KEY_h,CONTROLMASK),this,FXSEL(SEL_COMMAND,ID_HOME));
    table->addAccel(MKUINT(KEY_w,CONTROLMASK),this,FXSEL(SEL_COMMAND,ID_WORK));
    }
  dirbox->setDirectory(currentdirectory);
  dirname->setText(currentdirectory);
  dirbox->setFocus();
  }


// Set directory
void FXDirSelector::setDirectory(const FXString& path){
  dirname->setText(path);
  dirbox->setDirectory(path);
  }


// Return directory
FXString FXDirSelector::getDirectory() const {
  return dirname->getText();
  }


// Change Directory List style
void FXDirSelector::setDirBoxStyle(FXuint style){
  dirbox->setListStyle(style);
  }


// Return Directory List style
FXuint FXDirSelector::getDirBoxStyle() const {
  return dirbox->getListStyle();
  }


// Typed in new directory name, open path in the tree
long FXDirSelector::onCmdName(FXObject*,FXSelector,void*){
  dirbox->setDirectory(dirname->getText());
  return 1;
  }


// Opened an item, making it the current one
long FXDirSelector::onCmdOpened(FXObject*,FXSelector,void* ptr){
  const FXTreeItem* item=(const FXTreeItem*)ptr;
  dirname->setText(dirbox->getItemPathname(item));
  return 1;
  }


// Back to home directory
long FXDirSelector::onCmdHome(FXObject*,FXSelector,void*){
  setDirectory(FXFile::getHomeDirectory());
  return 1;
  }


// Back to current working directory
long FXDirSelector::onCmdWork(FXObject*,FXSelector,void*){
  setDirectory(FXFile::getCurrentDirectory());
  return 1;
  }


// User clicked up directory button
long FXDirSelector::onCmdDirectoryUp(FXObject*,FXSelector,void*){
  setDirectory(FXFile::upLevel(getDirectory()));
  return 1;
  }


// Save data
void FXDirSelector::save(FXStream& store) const {
  FXPacker::save(store);
  store << dirbox;
  store << dirname;
  store << accept;
  store << cancel;
  }


// Load data
void FXDirSelector::load(FXStream& store){
  FXPacker::load(store);
  store >> dirbox;
  store >> dirname;
  store >> accept;
  store >> cancel;
  }


// Clean up
FXDirSelector::~FXDirSelector(){
  dirbox=(FXDirList*)-1L;
  dirname=(FXTextField*)-1L;
  accept=(FXButton*)-1L;
  cancel=(FXButton*)-1L;
  }

}

#endif
