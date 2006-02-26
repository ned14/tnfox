/********************************************************************************
*                                                                               *
*                                 Test Switcher                                 *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2006 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* $Id: switcher.cpp,v 1.16 2006/01/22 17:59:02 fox Exp $                        *
********************************************************************************/
#include "fx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>





/*******************************************************************************/


// Switcher Test Window
class SwitcherTest : public FXMainWindow {
  FXDECLARE(SwitcherTest)
protected:
  FXMenuBar         *menubar;
  FXMenuPane        *filemenu;
  FXHorizontalFrame *contents;
  FXVerticalFrame   *buttons;
  FXSwitcher        *switcher;
  FXList            *simplelist;
  FXFileList        *filelist;
  FXDirList         *dirlist;
  FXGIFIcon         *big_folder;
  FXGIFIcon         *mini_folder;
protected:
  SwitcherTest(){}
public:
  SwitcherTest(FXApp *a);
  virtual void create();
  virtual ~SwitcherTest();
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


// SwitcherApp implementation
FXIMPLEMENT(SwitcherTest,FXMainWindow,NULL,0)



/*******************************************************************************/


// Make some windows
SwitcherTest::SwitcherTest(FXApp *a):FXMainWindow(a,"Switcher Test",NULL,NULL,DECOR_ALL,0,0,600,400){

  // Tooltip
  new FXToolTip(getApp());

  // Make icons
  big_folder=new FXGIFIcon(getApp(),bigfolder);
  mini_folder=new FXGIFIcon(getApp(),minifolder);

  // Menubar
  menubar=new FXMenuBar(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X);

  // Separator
  new FXHorizontalSeparator(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X|SEPARATOR_GROOVE);

  // Contents
  contents=new FXHorizontalFrame(this,LAYOUT_SIDE_TOP|FRAME_NONE|LAYOUT_FILL_X|LAYOUT_FILL_Y|PACK_UNIFORM_WIDTH);

  // Buttons
  buttons=new FXVerticalFrame(contents,LAYOUT_FILL_Y|LAYOUT_LEFT|PACK_UNIFORM_WIDTH);

  // Switcher
  switcher=new FXSwitcher(contents,LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_RIGHT|FRAME_THICK|FRAME_RAISED);

  // First item in switcher is a list
  simplelist=new FXList(switcher,NULL,0,LIST_EXTENDEDSELECT);
  simplelist->appendItem("First Entry",mini_folder);
  simplelist->appendItem("Second Entry",big_folder);
  simplelist->appendItem("Third Entry",mini_folder);
  simplelist->appendItem("Fourth Entry",mini_folder);
  simplelist->appendItem("Fifth Entry",big_folder);
  simplelist->appendItem("Sixth Entry",mini_folder);

  // Second item is a file list
  filelist=new FXFileList(switcher,NULL,0,ICONLIST_EXTENDEDSELECT);

  // Third item is a directory list
  dirlist=new FXDirList(switcher,NULL,0,TREELIST_SHOWS_LINES|TREELIST_SHOWS_BOXES);

  // Add buttons
  new FXLabel(buttons,"These buttons below\nare connected to the\nFXSwitcher Control.\nSo they are checked\nautomatically depending\non the active page\nof the switcher.",NULL,LAYOUT_FILL_Y|JUSTIFY_LEFT|JUSTIFY_TOP);
  new FXButton(buttons,"&Simple List\tMake Switcher go to list",NULL,switcher,FXSwitcher::ID_OPEN_FIRST+0,FRAME_THICK|FRAME_RAISED);
  new FXButton(buttons,"&File List\tMake Switcher go to files",NULL,switcher,FXSwitcher::ID_OPEN_FIRST+1,FRAME_THICK|FRAME_RAISED);
  new FXButton(buttons,"&Tree List\tMake Switcher go to tree",NULL,switcher,FXSwitcher::ID_OPEN_FIRST+2,FRAME_THICK|FRAME_RAISED);

  // File Menu
  filemenu=new FXMenuPane(this);
  new FXMenuCommand(filemenu,"&Simple List",NULL,switcher,FXSwitcher::ID_OPEN_FIRST+0);
  new FXMenuCommand(filemenu,"&File List",NULL,switcher,FXSwitcher::ID_OPEN_FIRST+1);
  new FXMenuCommand(filemenu,"&Tree List",NULL,switcher,FXSwitcher::ID_OPEN_FIRST+2);
  new FXMenuCommand(filemenu,"Dump widgets",NULL,getApp(),FXApp::ID_DUMP);
  new FXMenuCommand(filemenu,"&Quit\tCtl-Q",NULL,getApp(),FXApp::ID_QUIT,0);
  new FXMenuTitle(menubar,"&File",NULL,filemenu);
  }


// Delete all resources
SwitcherTest::~SwitcherTest(){
  delete filemenu;
  delete big_folder;
  delete mini_folder;
  }


// Start
void SwitcherTest::create(){
  FXMainWindow::create();
  show(PLACEMENT_SCREEN);
  }


/*******************************************************************************/


// Start the whole thing
int main(int argc,char *argv[]){
  FXProcess myprocess(argc, argv);
  // Make application
  FXApp application("Switcher","FoxTest");

  // Open display
  application.init(argc,argv);

  new SwitcherTest(&application);

  // Create app
  application.create();

  // Run
  return application.run();
  }


