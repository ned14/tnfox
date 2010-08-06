/********************************************************************************
*                                                                               *
*                          Test Header Controls                                 *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2006 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* $Id: header.cpp,v 1.27 2006/01/22 17:59:01 fox Exp $                          *
********************************************************************************/
#include "fx.h"
#include <stdio.h>
#include <stdlib.h>




/* Generated by reswrap from file minidoc.gif */
const unsigned char minidoc[]={
  0x47,0x49,0x46,0x38,0x37,0x61,0x10,0x00,0x10,0x00,0xf2,0x00,0x00,0xbf,0xbf,0xbf,
  0x80,0x80,0x80,0xff,0xff,0xff,0xc0,0xc0,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x2c,0x00,0x00,0x00,0x00,0x10,0x00,0x10,0x00,0x00,0x03,
  0x36,0x08,0x10,0xdc,0xae,0x70,0x89,0x49,0xe7,0x08,0x51,0x56,0x3a,0x04,0x86,0xc1,
  0x46,0x11,0x24,0x01,0x8a,0xd5,0x60,0x2a,0x21,0x6a,0xad,0x9a,0xab,0x9e,0xae,0x30,
  0xb3,0xb5,0x0d,0xb7,0xf2,0x9e,0xdf,0x31,0x14,0x90,0x27,0xf4,0xd5,0x86,0x83,0xa4,
  0x72,0x09,0x2c,0x39,0x9f,0xa6,0x04,0x00,0x3b
  };



/*******************************************************************************/


// Header Window
class HeaderWindow : public FXMainWindow {
  FXDECLARE(HeaderWindow)
protected:
  HeaderWindow(){}
public:
  long onQuit(FXObject*,FXSelector,void*);
  long onCmdAbout(FXObject*,FXSelector,void*);
  long onCmdHeader(FXObject*,FXSelector,void*);
  long onCmdHeaderButton(FXObject*,FXSelector,void*);
  long onCmdContinuous(FXObject*,FXSelector,void*);
public:
  enum {
    ID_ABOUT=FXMainWindow::ID_LAST,
    ID_HEADER,
    ID_TRACKING,
    ID_LAST
    };
protected:
  FXMenuBar*         menubar;
  FXMenuPane*        filemenu;
  FXMenuPane*        helpmenu;
  FXVerticalFrame*   contents;
  FXHorizontalFrame* panes;
  FXHeader*          header1;
  FXHeader*          header2;
  FXList*            list[4];
  FXGIFIcon*         doc;
  FXCheckButton*     check;
public:
  HeaderWindow(FXApp* a);
  virtual void create();
  virtual ~HeaderWindow();
  };



/*******************************************************************************/

// Map
FXDEFMAP(HeaderWindow) HeaderWindowMap[]={
  FXMAPFUNC(SEL_COMMAND,  HeaderWindow::ID_ABOUT,      HeaderWindow::onCmdAbout),
  FXMAPFUNC(SEL_CHANGED,  HeaderWindow::ID_HEADER,     HeaderWindow::onCmdHeader),
  FXMAPFUNC(SEL_COMMAND,  HeaderWindow::ID_HEADER,     HeaderWindow::onCmdHeaderButton),
  FXMAPFUNC(SEL_COMMAND,  HeaderWindow::ID_TRACKING,   HeaderWindow::onCmdContinuous),
  };


// Object implementation
FXIMPLEMENT(HeaderWindow,FXMainWindow,HeaderWindowMap,ARRAYNUMBER(HeaderWindowMap))


// Make some windows
HeaderWindow::HeaderWindow(FXApp* a):FXMainWindow(a,"Header Control Test",NULL,NULL,DECOR_ALL,0,0,800,600){

  // Make menu bar
  menubar=new FXMenuBar(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X);

  new FXStatusBar(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X);

  filemenu=new FXMenuPane(this);
  new FXMenuCommand(filemenu,"&Quit\tCtl-Q\tQuit the application",NULL,getApp(),FXApp::ID_QUIT);
  new FXMenuTitle(menubar,"&File",NULL,filemenu);
  helpmenu=new FXMenuPane(this);
  new FXMenuCommand(helpmenu,"&About Header...",NULL,this,ID_ABOUT,0);
  new FXMenuTitle(menubar,"&Help",NULL,helpmenu,LAYOUT_RIGHT);

  // Make Main Window contents
  contents=new FXVerticalFrame(this,FRAME_SUNKEN|FRAME_THICK|LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 0,0,0,0, 0,0);

  // Make header control
  header1=new FXHeader(contents,this,ID_HEADER,HEADER_BUTTON|HEADER_RESIZE|FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_X);

  // Document icon
  doc=new FXGIFIcon(getApp(),minidoc);

  header1->appendItem("Name",doc,150);
  header1->appendItem("Type",NULL,120);
  header1->appendItem("Layout Option",doc,230);
  header1->appendItem("Attributes",NULL,80);

  // Below header
  panes=new FXHorizontalFrame(contents,FRAME_SUNKEN|FRAME_THICK|LAYOUT_SIDE_TOP|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 0,0,0,0, 0,0);

  // Make 4 lists
  list[0]=new FXList(panes,NULL,0,LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH|LIST_BROWSESELECT,0,0,150,0);
  list[1]=new FXList(panes,NULL,0,LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH|LIST_SINGLESELECT,0,0,120,0);
  list[2]=new FXList(panes,NULL,0,LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH|LIST_MULTIPLESELECT,0,0,230,0);
  list[3]=new FXList(panes,NULL,0,LAYOUT_FILL_Y|LAYOUT_FIX_WIDTH|LIST_EXTENDEDSELECT,0,0,80,0);
  list[0]->setBackColor(FXRGB(255,240,240));
  list[1]->setBackColor(FXRGB(240,255,240));
  list[2]->setBackColor(FXRGB(240,240,255));
  list[3]->setBackColor(FXRGB(255,255,240));

  // Add some contents
  list[0]->appendItem("Jeroen van der Zijp");
  list[0]->appendItem("Lyle Johnson");
  list[0]->appendItem("Freddy Golos");
  list[0]->appendItem("Charles Warren");
  list[0]->appendItem("Jonathan Bush");
  list[0]->appendItem("Guoqing Tian");

  list[1]->appendItem("Incorrigible Hacker");
  list[1]->appendItem("Windows Hacker");
  list[1]->appendItem("Russian Hacker");
  list[1]->appendItem("Shutter Widget");
  list[1]->appendItem("Progress Bar");
  list[1]->appendItem("Dial Widget");

  list[2]->appendItem("LAYOUT_FILL_X|LAYOUT_FILL_Y");
  list[2]->appendItem("LAYOUT_FILL_Y");
  list[2]->appendItem("LAYOUT_NORMAL");
  list[2]->appendItem("LAYOUT_NORMAL");
  list[2]->appendItem("LAYOUT_NORMAL");
  list[2]->appendItem("LAYOUT_NORMAL");

  list[3]->appendItem("A");
  list[3]->appendItem("B");
  list[3]->appendItem("C");
  list[3]->appendItem("D");
  list[3]->appendItem("E");
  list[3]->appendItem("F");

  header2=new FXHeader(panes,NULL,0,HEADER_VERTICAL|HEADER_BUTTON|HEADER_RESIZE|FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_Y);
  header2->appendItem("Example",NULL,30);
  header2->appendItem("Of",NULL,30);
  header2->appendItem("Vertical",NULL,30);
  header2->appendItem("Header",NULL,30);

  // Group box with some controls
  FXGroupBox *groupie=new FXGroupBox(panes,"Controls",GROUPBOX_TITLE_CENTER|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  check=new FXCheckButton(groupie,"Continuous Tracking\tContinuous\tTrack Header continuously",this,ID_TRACKING,ICON_BEFORE_TEXT|LAYOUT_SIDE_TOP);

  // Whip out a tooltip control, jeez, that's hard
  new FXToolTip(getApp());
  }


HeaderWindow::~HeaderWindow(){
  delete filemenu;
  delete helpmenu;
  delete doc;
  }


// About
long HeaderWindow::onCmdAbout(FXObject*,FXSelector,void*){
  FXMessageBox::information(this,MBOX_OK,"About Header",
    "An example of how to work with the header control\n\n\nAnd some attributes of the developers!"
    );
  return 1;
  }


// Changed the header control
long HeaderWindow::onCmdHeader(FXObject*,FXSelector,void* ptr){
  FXint which=(FXint)(long)ptr;
  FXASSERT(0<=which && which<4);
  FXTRACE((1,"Width of item %d = %d\n",which,header1->getItemSize(which)));
  list[which]->setWidth(header1->getItemSize(which));
  return 1;
  }


// Clicked a header button:- we highlight all in the list
long HeaderWindow::onCmdHeaderButton(FXObject*,FXSelector,void* ptr){
  FXint which=(FXint)(long)ptr;
  FXint i;
  FXASSERT(0<=which && which<4);
  for(i=0; i<list[which]->getNumItems(); i++){
    list[which]->selectItem(i);
    }
  return 1;
  }


// Tracking continuously
long HeaderWindow::onCmdContinuous(FXObject*,FXSelector,void*){
  header1->setHeaderStyle(HEADER_TRACKING^header1->getHeaderStyle());
  header2->setHeaderStyle(HEADER_TRACKING^header2->getHeaderStyle());
  return 1;
  }


// Start
void HeaderWindow::create(){
  FXMainWindow::create();
  show(PLACEMENT_SCREEN);
  }


/*******************************************************************************/


// Start the whole thing
int main(int argc,char *argv[]){
  FXProcess myprocess(argc, argv);

  // Make application
  FXApp application("Header","FoxTest");

  // Initialize application and open display;
  // FOX will parse some parameters, and leave the remaining
  // ones for the main application.
  application.init(argc,argv);

  // Make window
  new HeaderWindow(&application);

  // Create all the windows.
  application.create();

  // Run the application
  return application.run();
  }


