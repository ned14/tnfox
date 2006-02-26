/********************************************************************************
*                                                                               *
*                           Test Icon List Widget                               *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2006 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* $Id: iconlist.cpp,v 1.23 2006/01/22 17:59:01 fox Exp $                        *
********************************************************************************/
#include "fx.h"
#include <stdio.h>
#include <stdlib.h>




/*******************************************************************************/


// Window
class IconListWindow : public FXMainWindow {
  FXDECLARE(IconListWindow)
protected:
  FXMenuBar*         menubar;
  FXMenuPane*        filemenu;
  FXMenuPane*        arrangemenu;
  FXMenuPane*        sortmenu;
  FXSplitter*        splitter;
  FXVerticalFrame*   group;
  FXVerticalFrame*   subgroup;
  FXIconList*        iconlist;
  FXTextField*       pattern;
  FXGIFIcon*         big_folder;
  FXGIFIcon*         mini_folder;
  FXDebugTarget*     dbg;
protected:
  IconListWindow(){}
public:
  long onQuit(FXObject*,FXSelector,void*);
public:
  enum{
    ID_DIRECTORY=FXMainWindow::ID_LAST,
    ID_PATTERN,
    ID_LAST
    };
public:
  IconListWindow(FXApp* a);
  virtual void create();
  virtual ~IconListWindow();
  };



/*******************************************************************************/


/* Generated by reswrap from file bigfolder.gif */
const unsigned char bigfolder[]={
  0x47,0x49,0x46,0x38,0x37,0x61,0x20,0x00,0x20,0x00,0xf2,0x00,0x00,0xb2,0xc0,0xdc,
  0x80,0x80,0x80,0xff,0xff,0xff,0xff,0xff,0x00,0xc0,0xc0,0xc0,0x00,0x00,0x00,0x80,
  0x80,0x00,0x00,0x00,0x00,0x2c,0x00,0x00,0x00,0x00,0x20,0x00,0x20,0x00,0x00,0x03,
  0x83,0x08,0xba,0xdc,0xfe,0x30,0xca,0x49,0x6b,0x0c,0x38,0x67,0x0b,0x83,0xf8,0x20,
  0x18,0x70,0x8d,0x37,0x10,0x67,0x8a,0x12,0x23,0x09,0x98,0xab,0xaa,0xb6,0x56,0x40,
  0xdc,0x78,0xae,0x6b,0x3c,0x5f,0xbc,0xa1,0xa0,0x70,0x38,0x2c,0x14,0x60,0xb2,0x98,
  0x32,0x99,0x34,0x1c,0x05,0xcb,0x28,0x53,0xea,0x44,0x4a,0xaf,0xd3,0x2a,0x74,0xca,
  0xc5,0x6a,0xbb,0xe0,0xa8,0x16,0x4b,0x66,0x7e,0xcb,0xe8,0xd3,0x38,0xcc,0x46,0x9d,
  0xdb,0xe1,0x75,0xba,0xfc,0x9e,0x77,0xe5,0x70,0xef,0x33,0x1f,0x7f,0xda,0xe9,0x7b,
  0x7f,0x77,0x7e,0x7c,0x7a,0x56,0x85,0x4d,0x84,0x82,0x54,0x81,0x88,0x62,0x47,0x06,
  0x91,0x92,0x93,0x94,0x95,0x96,0x91,0x3f,0x46,0x9a,0x9b,0x9c,0x9d,0x9e,0x9a,0x2e,
  0xa1,0xa2,0x13,0x09,0x00,0x3b
  };


/* Generated by reswrap from file minifolder.gif */
const unsigned char minifolder[]={
  0x47,0x49,0x46,0x38,0x37,0x61,0x10,0x00,0x10,0x00,0xf2,0x00,0x00,0xb2,0xc0,0xdc,
  0x80,0x80,0x80,0xc0,0xc0,0xc0,0xff,0xff,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0x00,
  0x00,0x00,0x00,0x00,0x00,0x2c,0x00,0x00,0x00,0x00,0x10,0x00,0x10,0x00,0x00,0x03,
  0x3b,0x08,0xba,0xdc,0x1b,0x10,0x3a,0x16,0xc4,0xb0,0x22,0x4c,0x50,0xaf,0xcf,0x91,
  0xc4,0x15,0x64,0x69,0x92,0x01,0x31,0x7e,0xac,0x95,0x8e,0x58,0x7b,0xbd,0x41,0x21,
  0xc7,0x74,0x11,0xef,0xb3,0x5a,0xdf,0x9e,0x1c,0x6f,0x97,0x03,0xba,0x7c,0xa1,0x64,
  0x48,0x05,0x20,0x38,0x9f,0x50,0xe8,0x66,0x4a,0x75,0x24,0x00,0x00,0x3b
  };


// Object implementation
FXIMPLEMENT(IconListWindow,FXMainWindow,NULL,0)


// Make some windows
IconListWindow::IconListWindow(FXApp* a):FXMainWindow(a,"Icon List Test",NULL,NULL,DECOR_ALL,0,0,800,600){
  int i;

  // Menu bar
  menubar=new FXMenuBar(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X);

  // File menu
  filemenu=new FXMenuPane(this);
  new FXMenuCommand(filemenu,"&Quit\tCtl-Q",NULL,getApp(),FXApp::ID_QUIT);
  new FXMenuTitle(menubar,"&File",NULL,filemenu);

  // Status bar
  FXStatusBar *status=new FXStatusBar(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|STATUSBAR_WITH_DRAGCORNER);

  // Main window interior
  group=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0);

  // Files
  new FXLabel(group,"Icon List WIdget",NULL,LAYOUT_TOP|LAYOUT_FILL_X|FRAME_SUNKEN);
  subgroup=new FXVerticalFrame(group,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0);

  dbg=new FXDebugTarget();

  // Icon list on the right
  iconlist=new FXIconList(subgroup,dbg,1,LAYOUT_FILL_X|LAYOUT_FILL_Y|ICONLIST_DETAILED|ICONLIST_EXTENDEDSELECT);
  iconlist->appendHeader("Name",NULL,200);
  iconlist->appendHeader("Type",NULL,100);
  iconlist->appendHeader("Size",NULL,60);
  iconlist->appendHeader("Modified Date",NULL,150);
  iconlist->appendHeader("User",NULL,50);
  iconlist->appendHeader("Group",NULL,50);

  big_folder=new FXGIFIcon(getApp(),bigfolder);
  mini_folder=new FXGIFIcon(getApp(),minifolder);

  iconlist->appendItem("Really BIG and wide item to test\tDocument\t10000\tJune 13, 1999\tUser\tSoftware",big_folder,mini_folder);
  for(i=1; i<400; i++){
    iconlist->appendItem("Filename_"+FXStringVal(i)+"\tDocument\t10000\tJune 13, 1999\tUser\tSoftware",big_folder,mini_folder);
    }
  iconlist->setCurrentItem(iconlist->getNumItems()-1);

  // Arrange menu
  arrangemenu=new FXMenuPane(this);
  new FXMenuRadio(arrangemenu,"&Details",iconlist,FXIconList::ID_SHOW_DETAILS);
  new FXMenuRadio(arrangemenu,"&Small Icons",iconlist,FXIconList::ID_SHOW_MINI_ICONS);
  new FXMenuRadio(arrangemenu,"&Big Icons",iconlist,FXIconList::ID_SHOW_BIG_ICONS);
  new FXMenuSeparator(arrangemenu);
  new FXMenuRadio(arrangemenu,"&Rows",iconlist,FXIconList::ID_ARRANGE_BY_ROWS);
  new FXMenuRadio(arrangemenu,"&Columns",iconlist,FXIconList::ID_ARRANGE_BY_COLUMNS);
  new FXMenuTitle(menubar,"&Arrange",NULL,arrangemenu);

  // Sort menu
  sortmenu=new FXMenuPane(this);
  new FXMenuCommand(sortmenu,"&Name",NULL,NULL,0);
  new FXMenuCommand(sortmenu,"&Type",NULL,NULL,0);
  new FXMenuCommand(sortmenu,"&Size",NULL,NULL,0);
  new FXMenuCommand(sortmenu,"T&ime",NULL,NULL,0);
  new FXMenuCommand(sortmenu,"&User",NULL,NULL,0);
  new FXMenuCommand(sortmenu,"&Group",NULL,NULL,0);
  new FXMenuCheck(sortmenu,"&Reverse",NULL,0);
  new FXMenuCommand(sortmenu,"Hide status",NULL,status,FXWindow::ID_HIDE);
  new FXMenuCommand(sortmenu,"Show status",NULL,status,FXWindow::ID_SHOW);
  new FXMenuTitle(menubar,"&Sort",NULL,sortmenu);

  new FXToolTip(getApp());
  }


IconListWindow::~IconListWindow(){
  delete filemenu;
  delete arrangemenu;
  delete sortmenu;
  delete big_folder;
  delete mini_folder;
  delete dbg;
  }

// Make application
void IconListWindow::create(){
  FXMainWindow::create();
  show(PLACEMENT_SCREEN);
  }



/*******************************************************************************/


// Start the whole thing
int main(int argc,char *argv[]){
  FXProcess myprocess(argc, argv);
  FXApp application("IconList","FoxTest");
  application.init(argc,argv);
  new IconListWindow(&application);
  application.create();
  return application.run();
  }


