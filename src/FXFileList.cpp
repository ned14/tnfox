/********************************************************************************
*                                                                               *
*                        F i l e    L i s t   O b j e c t                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXFileList.cpp,v 1.156 2004/09/17 07:46:21 fox Exp $                     *
********************************************************************************/
#ifndef BUILDING_TCOMMON

#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxkeys.h"
#include "FXHash.h"
#include "FXThread.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXFile.h"
#include "FXURL.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXApp.h"
#include "FXFont.h"
#include "FXIcon.h"
#include "FXGIFIcon.h"
#include "FXScrollBar.h"
#include "FXFileDict.h"
#include "FXHeader.h"
#include "FXIconList.h"
#include "FXFileList.h"
#ifdef WIN32
#include <shellapi.h>
#endif
#include "icons.h"


/*
  Notes:
  - Share icons with other widgets; upgrade icons to some nicer ones.
  - Should some of these icons move to FXFileDict?
  - Clipboard of filenames.
  - Clipboard, DND, etc. support.
  - When being dragged over, if hovering over a directory item for some
    time we need to open it.
  - We should generate SEL_INSERTED, SEL_DELETED, SEL_REPLACED, SEL_CHANGED
    messages as the FXFileList updates itself from the file system.
*/


#define REFRESHINTERVAL     1000        // Interval between refreshes
#define REFRESHFREQUENCY    30          // File systems not supporting mod-time, refresh every nth time

#define HASH1(x,n) (((unsigned int)(x)*13)%(n))           // Probe Position [0..n-1]
#define HASH2(x,n) (1|(((unsigned int)(x)*17)%((n)-1)))   // Probe Distance [1..n-2]

#ifndef TIMEFORMAT
#define TIMEFORMAT "%m/%d/%Y %H:%M:%S"
#endif




/*******************************************************************************/

namespace FX {


// Object implementation
FXIMPLEMENT(FXFileItem,FXIconItem,NULL,0)


// Map
FXDEFMAP(FXFileList) FXFileListMap[]={
  FXMAPFUNC(SEL_DRAGGED,0,FXFileList::onDragged),
  FXMAPFUNC(SEL_TIMEOUT,FXFileList::ID_REFRESHTIMER,FXFileList::onRefreshTimer),
  FXMAPFUNC(SEL_TIMEOUT,FXFileList::ID_OPENTIMER,FXFileList::onOpenTimer),
  FXMAPFUNC(SEL_DND_ENTER,0,FXFileList::onDNDEnter),
  FXMAPFUNC(SEL_DND_LEAVE,0,FXFileList::onDNDLeave),
  FXMAPFUNC(SEL_DND_DROP,0,FXFileList::onDNDDrop),
  FXMAPFUNC(SEL_DND_MOTION,0,FXFileList::onDNDMotion),
  FXMAPFUNC(SEL_DND_REQUEST,0,FXFileList::onDNDRequest),
  FXMAPFUNC(SEL_BEGINDRAG,0,FXFileList::onBeginDrag),
  FXMAPFUNC(SEL_ENDDRAG,0,FXFileList::onEndDrag),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_DIRECTORY_UP,FXFileList::onUpdDirectoryUp),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SORT_BY_NAME,FXFileList::onUpdSortByName),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SORT_BY_TYPE,FXFileList::onUpdSortByType),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SORT_BY_SIZE,FXFileList::onUpdSortBySize),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SORT_BY_TIME,FXFileList::onUpdSortByTime),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SORT_BY_USER,FXFileList::onUpdSortByUser),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SORT_BY_GROUP,FXFileList::onUpdSortByGroup),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SORT_REVERSE,FXFileList::onUpdSortReverse),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SORT_CASE,FXFileList::onUpdSortCase),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SET_PATTERN,FXFileList::onUpdSetPattern),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SET_DIRECTORY,FXFileList::onUpdSetDirectory),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_SHOW_HIDDEN,FXFileList::onUpdShowHidden),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_HIDE_HIDDEN,FXFileList::onUpdHideHidden),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_TOGGLE_HIDDEN,FXFileList::onUpdToggleHidden),
  FXMAPFUNC(SEL_UPDATE,FXFileList::ID_HEADER_CHANGE,FXFileList::onUpdHeader),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_DIRECTORY_UP,FXFileList::onCmdDirectoryUp),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SORT_BY_NAME,FXFileList::onCmdSortByName),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SORT_BY_TYPE,FXFileList::onCmdSortByType),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SORT_BY_SIZE,FXFileList::onCmdSortBySize),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SORT_BY_TIME,FXFileList::onCmdSortByTime),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SORT_BY_USER,FXFileList::onCmdSortByUser),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SORT_BY_GROUP,FXFileList::onCmdSortByGroup),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SORT_REVERSE,FXFileList::onCmdSortReverse),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SORT_CASE,FXFileList::onCmdSortCase),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SET_PATTERN,FXFileList::onCmdSetPattern),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SET_DIRECTORY,FXFileList::onCmdSetDirectory),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETVALUE,FXFileList::onCmdSetValue),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_SETSTRINGVALUE,FXFileList::onCmdSetStringValue),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_GETSTRINGVALUE,FXFileList::onCmdGetStringValue),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_SHOW_HIDDEN,FXFileList::onCmdShowHidden),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_HIDE_HIDDEN,FXFileList::onCmdHideHidden),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_TOGGLE_HIDDEN,FXFileList::onCmdToggleHidden),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_HEADER_CHANGE,FXFileList::onCmdHeader),
  FXMAPFUNC(SEL_COMMAND,FXFileList::ID_REFRESH,FXFileList::onCmdRefresh),
  };


// Object implementation
FXIMPLEMENT(FXFileList,FXIconList,FXFileListMap,ARRAYNUMBER(FXFileListMap))


// For serialization
FXFileList::FXFileList(){
  flags|=FLAG_ENABLED|FLAG_DROPTARGET;
  sortfunc=ascendingCase;
  associations=NULL;
  list=NULL;
  timestamp=0;
  associations=NULL;
  dropaction=DRAG_MOVE;
  counter=0;
  };


// File List
FXFileList::FXFileList(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):
  FXIconList(p,tgt,sel,opts,x,y,w,h),directory(PATHSEPSTRING),orgdirectory(PATHSEPSTRING),pattern("*"){
  flags|=FLAG_ENABLED|FLAG_DROPTARGET;
  associations=NULL;
  appendHeader("Name",NULL,200);
  appendHeader("Type",NULL,100);
  appendHeader("Size",NULL,60);
  appendHeader("Modified Date",NULL,150);
  appendHeader("User",NULL,50);
  appendHeader("Group",NULL,50);
  appendHeader("Attributes",NULL,100);
#ifndef WIN32
  appendHeader("Link",NULL,200);
#endif
  big_folder=new FXGIFIcon(getApp(),bigfolder);
  mini_folder=new FXGIFIcon(getApp(),minifolder);
  big_doc=new FXGIFIcon(getApp(),bigdoc);
  mini_doc=new FXGIFIcon(getApp(),minidoc);
  big_app=new FXGIFIcon(getApp(),bigapp);
  mini_app=new FXGIFIcon(getApp(),miniapp);
#ifdef WIN32
  matchmode=FILEMATCH_FILE_NAME|FILEMATCH_NOESCAPE|FILEMATCH_CASEFOLD;
  sortfunc=ascendingCase;
#else
  matchmode=FILEMATCH_FILE_NAME|FILEMATCH_NOESCAPE;
  sortfunc=ascending;
#endif
  if(!(options&FILELIST_NO_OWN_ASSOC)) associations=new FXFileDict(getApp());
  list=NULL;
  dropaction=DRAG_MOVE;
  counter=0;
  timestamp=0;
  }


// Starts the timer
void FXFileList::create(){
  if(!id()) getApp()->addTimeout(this,ID_REFRESHTIMER,REFRESHINTERVAL);
  FXIconList::create();
  if(!deleteType){deleteType=getApp()->registerDragType(deleteTypeName);}
  if(!urilistType){urilistType=getApp()->registerDragType(urilistTypeName);}
  big_folder->create();
  mini_folder->create();
  big_doc->create();
  mini_doc->create();
  big_app->create();
  mini_app->create();
  scan(FALSE);
  }


// Detach disconnects the icons
void FXFileList::detach(){
  if(id()) getApp()->removeTimeout(this,ID_REFRESHTIMER);
  if(id()) getApp()->removeTimeout(this,ID_OPENTIMER);
  FXIconList::detach();
  big_folder->detach();
  mini_folder->detach();
  big_doc->detach();
  mini_doc->detach();
  big_app->detach();
  mini_app->detach();
  deleteType=0;
  urilistType=0;
  }


// Destroy zaps the icons
void FXFileList::destroy(){
  if(id()) getApp()->removeTimeout(this,ID_REFRESHTIMER);
  if(id()) getApp()->removeTimeout(this,ID_OPENTIMER);
  FXIconList::destroy();
  big_folder->destroy();
  mini_folder->destroy();
  big_doc->destroy();
  mini_doc->destroy();
  big_app->destroy();
  mini_app->destroy();
  }



/*******************************************************************************/

// Open up folder when howvering long over a folder
long FXFileList::onOpenTimer(FXObject*,FXSelector,void*){
  FXint xx,yy,index; FXuint buttons;
  getCursorPosition(xx,yy,buttons);
  index=getItemAt(xx,yy);
  if(0<=index && isItemDirectory(index)){
    dropdirectory=getItemPathname(index);
    setDirectory(dropdirectory);
    getApp()->addTimeout(this,ID_OPENTIMER,700);
    }
  return 1;
  }


// Handle drag-and-drop enter
long FXFileList::onDNDEnter(FXObject* sender,FXSelector sel,void* ptr){
  FXIconList::onDNDEnter(sender,sel,ptr);

  // Keep original directory
  orgdirectory=getDirectory();

  return 1;
  }


// Handle drag-and-drop leave
long FXFileList::onDNDLeave(FXObject* sender,FXSelector sel,void* ptr){
  FXIconList::onDNDLeave(sender,sel,ptr);

  // Cancel open up timer
  getApp()->removeTimeout(this,ID_OPENTIMER);

  // Stop scrolling
  stopAutoScroll();

  // Restore original directory
  setDirectory(orgdirectory);
  return 1;
  }


// Handle drag-and-drop motion
long FXFileList::onDNDMotion(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  FXint index=-1;

  // Cancel open up timer
  getApp()->removeTimeout(this,ID_OPENTIMER);

  // Start autoscrolling
  if(startAutoScroll(event,FALSE)) return 1;

  // Give base class a shot
  if(FXIconList::onDNDMotion(sender,sel,ptr)) return 1;

  // Dropping list of filenames
  if(offeredDNDType(FROM_DRAGNDROP,urilistType)){

    // Drop in the background
    dropdirectory=getDirectory();

    // What is being done (move,copy,link)
    dropaction=inquireDNDAction();

    // We will open up a folder if we can hover over it for a while
    index=getItemAt(event->win_x,event->win_y);
    if(0<=index && isItemDirectory(index)){

      // Set open up timer
      getApp()->addTimeout(this,ID_OPENTIMER,700);

      // Directory where to drop, or directory to open up
      dropdirectory=getItemPathname(index);
      }

    // See if dropdirectory is writable
    if(FXFile::isWritable(dropdirectory)){
      FXTRACE((100,"accepting drop on %s\n",dropdirectory.text()));
      acceptDrop(DRAG_ACCEPT);
      }
    return 1;
    }
  return 0;
  }


// Handle drag-and-drop drop
long FXFileList::onDNDDrop(FXObject* sender,FXSelector sel,void* ptr){
  FXuchar *data; FXuint len;

  // Cancel open up timer
  getApp()->removeTimeout(this,ID_OPENTIMER);

  // Stop scrolling
  stopAutoScroll();

  // Restore original directory
  setDirectory(orgdirectory);

  // Perhaps target wants to deal with it
  if(FXIconList::onDNDDrop(sender,sel,ptr)) return 1;

  // Get uri-list of files being dropped
  if(getDNDData(FROM_DRAGNDROP,urilistType,data,len)){
    FXRESIZE(&data,FXuchar,len+1); data[len]='\0';
    FXchar *p,*q;
    p=q=(FXchar*)data;
    while(*p){
      while(*q && *q!='\r') q++;
      FXString url(p,q-p);
      FXString filesrc(FXURL::fileFromURL(url));
      FXString filedst(dropdirectory+PATHSEPSTRING+FXFile::name(filesrc));

      // Move, Copy, or Link as appropriate
      if(dropaction==DRAG_MOVE){
        FXTRACE((100,"Moving file: %s to %s\n",filesrc.text(),filedst.text()));
        if(!FXFile::move(filesrc,filedst)) getApp()->beep();
        }
      else if(dropaction==DRAG_COPY){
        FXTRACE((100,"Copying file: %s to %s\n",filesrc.text(),filedst.text()));
        if(!FXFile::copy(filesrc,filedst)) getApp()->beep();
        }
      else if(dropaction==DRAG_LINK){
        FXTRACE((100,"Linking file: %s to %s\n",filesrc.text(),filedst.text()));
        if(!FXFile::symlink(filesrc,filedst)) getApp()->beep();
        }
      if(*q=='\r') q+=2;
      p=q;
      }

    FXFREE(&data);
    return 1;
    }

  return 0;
  }


// Somebody wants our dragged data
long FXFileList::onDNDRequest(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent *event=(FXEvent*)ptr;
  FXuchar *data; FXuint len;

  // Perhaps the target wants to supply its own data
  if(FXIconList::onDNDRequest(sender,sel,ptr)) return 1;

  // Return list of filenames as a uri-list
  if(event->target==urilistType){
    if(!dragfiles.empty()){
      len=dragfiles.length();
      FXMEMDUP(&data,dragfiles.text(),FXuchar,len);
      setDNDData(FROM_DRAGNDROP,event->target,data,len);
      }
    return 1;
    }

  // Delete selected files
  if(event->target==deleteType){
    FXTRACE((100,"Delete files not yet implemented\n"));
    return 1;
    }

  return 0;
  }



// Start a drag operation
long FXFileList::onBeginDrag(FXObject* sender,FXSelector sel,void* ptr){
  register FXint i;
  if(FXIconList::onBeginDrag(sender,sel,ptr)) return 1;
  if(beginDrag(&urilistType,1)){
    dragfiles=FXString::null;
    for(i=0; i<getNumItems(); i++){
      if(isItemSelected(i)){
        if(!dragfiles.empty()) dragfiles+="\r\n";
        dragfiles+=FXURL::fileToURL(getItemPathname(i));
        FXTRACE((100,"url=%s\n",FXURL::fileToURL(getItemPathname(i)).text()));
        }
      }
    return 1;
    }
  return 0;
  }


// End drag operation
long FXFileList::onEndDrag(FXObject* sender,FXSelector sel,void* ptr){
  if(FXIconList::onEndDrag(sender,sel,ptr)) return 1;
  endDrag((didAccept()!=DRAG_REJECT));
  setDragCursor(getDefaultCursor());
  dragfiles=FXString::null;
  return 1;
  }


// Dragged stuff around
long FXFileList::onDragged(FXObject* sender,FXSelector sel,void* ptr){
  FXEvent* event=(FXEvent*)ptr;
  FXDragAction action;
  if(FXIconList::onDragged(sender,sel,ptr)) return 1;
  action=DRAG_MOVE;
  if(event->state&CONTROLMASK) action=DRAG_COPY;
  if(event->state&SHIFTMASK) action=DRAG_MOVE;
  if(event->state&ALTMASK) action=DRAG_LINK;
  handleDrag(event->root_x,event->root_y,action);
  if(didAccept()!=DRAG_REJECT){
    if(action==DRAG_MOVE)
      setDragCursor(getApp()->getDefaultCursor(DEF_DNDMOVE_CURSOR));
    else if(action==DRAG_LINK)
      setDragCursor(getApp()->getDefaultCursor(DEF_DNDLINK_CURSOR));
    else
      setDragCursor(getApp()->getDefaultCursor(DEF_DNDCOPY_CURSOR));
    }
  else{
    setDragCursor(getApp()->getDefaultCursor(DEF_DNDSTOP_CURSOR));
    }
  return 1;
  }


/*******************************************************************************/


// Set value from a message
long FXFileList::onCmdSetValue(FXObject*,FXSelector,void* ptr){
  setCurrentFile((const FXchar*)ptr);
  return 1;
  }


// Set current directory from dir part of filename
long FXFileList::onCmdSetStringValue(FXObject*,FXSelector,void* ptr){
  setCurrentFile(*((FXString*)ptr));
  return 1;
  }


// Get current file name (NULL if no current file)
long FXFileList::onCmdGetStringValue(FXObject*,FXSelector,void* ptr){
  *((FXString*)ptr)=getCurrentFile();
  return 1;
  }


// Toggle hidden files display
long FXFileList::onCmdToggleHidden(FXObject*,FXSelector,void*){
  showHiddenFiles(!showHiddenFiles());
  return 1;
  }


// Update toggle hidden files widget
long FXFileList::onUpdToggleHidden(FXObject* sender,FXSelector,void*){
  sender->handle(this,showHiddenFiles()?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


// Show hidden files
long FXFileList::onCmdShowHidden(FXObject*,FXSelector,void*){
  showHiddenFiles(TRUE);
  return 1;
  }


// Update show hidden files widget
long FXFileList::onUpdShowHidden(FXObject* sender,FXSelector,void*){
  sender->handle(this,showHiddenFiles()?FXSEL(SEL_COMMAND,ID_CHECK):FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


// Hide hidden files
long FXFileList::onCmdHideHidden(FXObject*,FXSelector,void*){
  showHiddenFiles(FALSE);
  return 1;
  }


// Update hide hidden files widget
long FXFileList::onUpdHideHidden(FXObject* sender,FXSelector,void*){
  sender->handle(this,showHiddenFiles()?FXSEL(SEL_COMMAND,ID_UNCHECK):FXSEL(SEL_COMMAND,ID_CHECK),NULL);
  return 1;
  }


// Move up one level
long FXFileList::onCmdDirectoryUp(FXObject*,FXSelector,void*){
  setDirectory(FXFile::upLevel(directory));
  return 1;
  }


// Determine if we can still go up more
long FXFileList::onUpdDirectoryUp(FXObject* sender,FXSelector,void* ptr){
  sender->handle(this,FXFile::isTopDirectory(directory)?FXSEL(SEL_COMMAND,ID_DISABLE):FXSEL(SEL_COMMAND,ID_ENABLE),ptr);
  return 1;
  }


// Change pattern
long FXFileList::onCmdSetPattern(FXObject*,FXSelector,void* ptr){
  setPattern((const char*)ptr);
  return 1;
  }


// Update pattern
long FXFileList::onUpdSetPattern(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SETVALUE),(void*)pattern.text());
  return 1;
  }


// Change directory
long FXFileList::onCmdSetDirectory(FXObject*,FXSelector,void* ptr){
  setDirectory((const char*)ptr);
  return 1;
  }


// Update directory
long FXFileList::onUpdSetDirectory(FXObject* sender,FXSelector,void*){
  sender->handle(this,FXSEL(SEL_COMMAND,FXWindow::ID_SETVALUE),(void*)directory.text());
  return 1;
  }


// Sort by name
long FXFileList::onCmdSortByName(FXObject*,FXSelector,void*){
  if(sortfunc==ascending) sortfunc=descending;
  else if(sortfunc==ascendingCase) sortfunc=descendingCase;
  else if(sortfunc==descending) sortfunc=ascending;
  else sortfunc=ascendingCase;
  scan(TRUE);
  return 1;
  }


// Update sender
long FXFileList::onUpdSortByName(FXObject* sender,FXSelector,void*){
  sender->handle(this,(sortfunc==ascending || sortfunc==descending || sortfunc==ascendingCase || sortfunc==descendingCase) ? FXSEL(SEL_COMMAND,ID_CHECK) : FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


// Sort by type
long FXFileList::onCmdSortByType(FXObject*,FXSelector,void*){
  sortfunc=(sortfunc==ascendingType) ? descendingType : ascendingType;
  scan(TRUE);
  return 1;
  }


// Update sender
long FXFileList::onUpdSortByType(FXObject* sender,FXSelector,void*){
  sender->handle(this,(sortfunc==ascendingType || sortfunc==descendingType) ? FXSEL(SEL_COMMAND,ID_CHECK) : FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


// Sort by size
long FXFileList::onCmdSortBySize(FXObject*,FXSelector,void*){
  sortfunc=(sortfunc==ascendingSize) ? descendingSize : ascendingSize;
  scan(TRUE);
  return 1;
  }


// Update sender
long FXFileList::onUpdSortBySize(FXObject* sender,FXSelector,void*){
  sender->handle(this,(sortfunc==ascendingSize || sortfunc==descendingSize) ? FXSEL(SEL_COMMAND,ID_CHECK) : FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


// Sort by time
long FXFileList::onCmdSortByTime(FXObject*,FXSelector,void*){
  sortfunc=(sortfunc==ascendingTime) ? descendingTime : ascendingTime;
  scan(TRUE);
  return 1;
  }


// Update sender
long FXFileList::onUpdSortByTime(FXObject* sender,FXSelector,void*){
  sender->handle(this,(sortfunc==ascendingTime || sortfunc==descendingTime) ? FXSEL(SEL_COMMAND,ID_CHECK) : FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


// Sort by user
long FXFileList::onCmdSortByUser(FXObject*,FXSelector,void*){
  sortfunc=(sortfunc==ascendingUser) ? descendingUser : ascendingUser;
  scan(TRUE);
  return 1;
  }


// Update sender
long FXFileList::onUpdSortByUser(FXObject* sender,FXSelector,void*){
  sender->handle(this,(sortfunc==ascendingUser || sortfunc==descendingUser) ? FXSEL(SEL_COMMAND,ID_CHECK) : FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


// Sort by group
long FXFileList::onCmdSortByGroup(FXObject*,FXSelector,void*){
  sortfunc=(sortfunc==ascendingGroup) ? descendingGroup : ascendingGroup;
  scan(TRUE);
  return 1;
  }


// Update sender
long FXFileList::onUpdSortByGroup(FXObject* sender,FXSelector,void*){
  sender->handle(this,(sortfunc==ascendingGroup || sortfunc==descendingGroup) ? FXSEL(SEL_COMMAND,ID_CHECK) : FXSEL(SEL_COMMAND,ID_UNCHECK),NULL);
  return 1;
  }


// Reverse sort order
long FXFileList::onCmdSortReverse(FXObject*,FXSelector,void*){
  if(sortfunc==ascending) sortfunc=descending;
  else if(sortfunc==descending) sortfunc=ascending;
  else if(sortfunc==ascendingCase) sortfunc=descendingCase;
  else if(sortfunc==descendingCase) sortfunc=ascendingCase;
  else if(sortfunc==ascendingType) sortfunc=descendingType;
  else if(sortfunc==descendingType) sortfunc=ascendingType;
  else if(sortfunc==ascendingSize) sortfunc=descendingSize;
  else if(sortfunc==descendingSize) sortfunc=ascendingSize;
  else if(sortfunc==ascendingTime) sortfunc=descendingTime;
  else if(sortfunc==descendingTime) sortfunc=ascendingTime;
  else if(sortfunc==ascendingUser) sortfunc=descendingUser;
  else if(sortfunc==descendingUser) sortfunc=ascendingUser;
  else if(sortfunc==ascendingGroup) sortfunc=descendingGroup;
  else if(sortfunc==descendingGroup) sortfunc=ascendingGroup;
  scan(TRUE);
  return 1;
  }


// Update sender
long FXFileList::onUpdSortReverse(FXObject* sender,FXSelector,void*){
  FXSelector selector=FXSEL(SEL_COMMAND,ID_UNCHECK);
  if(sortfunc==descending) selector=FXSEL(SEL_COMMAND,ID_CHECK);
  else if(sortfunc==descendingCase) selector=FXSEL(SEL_COMMAND,ID_CHECK);
  else if(sortfunc==descendingType) selector=FXSEL(SEL_COMMAND,ID_CHECK);
  else if(sortfunc==descendingSize) selector=FXSEL(SEL_COMMAND,ID_CHECK);
  else if(sortfunc==descendingTime) selector=FXSEL(SEL_COMMAND,ID_CHECK);
  else if(sortfunc==descendingUser) selector=FXSEL(SEL_COMMAND,ID_CHECK);
  else if(sortfunc==descendingGroup) selector=FXSEL(SEL_COMMAND,ID_CHECK);
  sender->handle(this,selector,NULL);
  return 1;
  }


// Toggle case sensitivity
long FXFileList::onCmdSortCase(FXObject*,FXSelector,void*){
  if(sortfunc==ascending) sortfunc=ascendingCase;
  else if(sortfunc==ascendingCase) sortfunc=ascending;
  else if(sortfunc==descending) sortfunc=descendingCase;
  else if(sortfunc==descendingCase) sortfunc=descending;
  scan(TRUE);
  return 1;
  }


// Check if case sensitive
long FXFileList::onUpdSortCase(FXObject* sender,FXSelector,void* ptr){
  sender->handle(this,(sortfunc==ascendingCase || sortfunc==descendingCase) ? FXSEL(SEL_COMMAND,ID_CHECK) : FXSEL(SEL_COMMAND,ID_UNCHECK),ptr);
  return 1;
  }


// Clicked header button
long FXFileList::onCmdHeader(FXObject*,FXSelector,void* ptr){
  if(((FXuint)(FXuval)ptr)<6) handle(this,FXSEL(SEL_COMMAND,(ID_SORT_BY_NAME+(FXuint)(FXuval)ptr)),NULL);
  return 1;
  }


// Clicked header button
long FXFileList::onUpdHeader(FXObject*,FXSelector,void*){
  header->setArrowDir(0,(sortfunc==ascending || sortfunc==ascendingCase)  ? FALSE : (sortfunc==descending || sortfunc==descendingCase) ? TRUE : MAYBE);   // Name
  header->setArrowDir(1,(sortfunc==ascendingType)  ? FALSE : (sortfunc==descendingType) ? TRUE : MAYBE);   // Type
  header->setArrowDir(2,(sortfunc==ascendingSize)  ? FALSE : (sortfunc==descendingSize) ? TRUE : MAYBE);   // Size
  header->setArrowDir(3,(sortfunc==ascendingTime)  ? FALSE : (sortfunc==descendingTime) ? TRUE : MAYBE);   // Date
  header->setArrowDir(4,(sortfunc==ascendingUser)  ? FALSE : (sortfunc==descendingUser) ? TRUE : MAYBE);   // User
  header->setArrowDir(5,(sortfunc==ascendingGroup) ? FALSE : (sortfunc==descendingGroup)? TRUE : MAYBE);   // Group
  return 1;
  }


// Compare file names
FXint FXFileList::ascending(const FXIconItem* pa,const FXIconItem* pb){
  register const FXFileItem *a=(FXFileItem*)pa;
  register const FXFileItem *b=(FXFileItem*)pb;
  register FXint diff=(FXint)b->isDirectory() - (FXint)a->isDirectory();
  if(diff) return diff;
  register const unsigned char *p=(const unsigned char*)a->label.text();
  register const unsigned char *q=(const unsigned char*)b->label.text();
  while(1){
    if(*p > *q) return 1;
    if(*p < *q) return -1;
    if(*p<='\t') break;
    p++;
    q++;
    }
  return 0;
  }


// Compare file names, case insensitive
FXint FXFileList::ascendingCase(const FXIconItem* pa,const FXIconItem* pb){
  register const FXFileItem *a=(FXFileItem*)pa;
  register const FXFileItem *b=(FXFileItem*)pb;
  register FXint diff=(FXint)b->isDirectory() - (FXint)a->isDirectory();
  if(diff) return diff;
  register const unsigned char *p=(const unsigned char*)a->label.text();
  register const unsigned char *q=(const unsigned char*)b->label.text();
  while(1){
    if(tolower(*p) > tolower(*q)) return 1;
    if(tolower(*p) < tolower(*q)) return -1;
    if(*p<='\t') break;
    p++;
    q++;
    }
  return 0;
  }


// Compare file types
FXint FXFileList::ascendingType(const FXIconItem* pa,const FXIconItem* pb){
  register const FXFileItem *a=(FXFileItem*)pa;
  register const FXFileItem *b=(FXFileItem*)pb;
  register FXint diff=(FXint)b->isDirectory() - (FXint)a->isDirectory();
  if(diff) return diff;
  register const unsigned char *p=(const unsigned char*)strchr(a->label.text(),'\t')+1;
  register const unsigned char *q=(const unsigned char*)strchr(b->label.text(),'\t')+1;
  while(1){
    if(*p > *q) return 1;
    if(*p < *q) return -1;
    if(*p<='\t') break;
    p++;
    q++;
    }
  return FXIconList::ascendingCase(pa,pb);
  }


// Compare file size
FXint FXFileList::ascendingSize(const FXIconItem* pa,const FXIconItem* pb){
  register const FXFileItem *a=(FXFileItem*)pa;
  register const FXFileItem *b=(FXFileItem*)pb;
  register FXint diff=(FXint)b->isDirectory() - (FXint)a->isDirectory();
  if(diff) return diff;
  register long l=(long)a->size - (long)b->size;
  if(l) return l;
  return FXIconList::ascendingCase(pa,pb);
  }


// Compare file time
FXint FXFileList::ascendingTime(const FXIconItem* pa,const FXIconItem* pb){
  register const FXFileItem *a=(FXFileItem*)pa;
  register const FXFileItem *b=(FXFileItem*)pb;
  register FXint diff=(FXint)b->isDirectory() - (FXint)a->isDirectory();
  if(diff) return diff;
  register long l=(long)a->date - (long)b->date;
  if(l) return l;
  return FXIconList::ascendingCase(pa, pb);
  }


// Compare file user
FXint FXFileList::ascendingUser(const FXIconItem* pa,const FXIconItem* pb){
  register const FXFileItem *a=(FXFileItem*)pa;
  register const FXFileItem *b=(FXFileItem*)pb;
  register const unsigned char *p,*q;
  register int i;
  FXint diff=(FXint)b->isDirectory() - (FXint)a->isDirectory();
  if(diff) return diff;
  for(p=(const unsigned char*)a->label.text(),i=4; *p && i; i-=(*p++=='\t'));
  for(q=(const unsigned char*)b->label.text(),i=4; *q && i; i-=(*q++=='\t'));
  while(1){
    if(*p > *q) return 1;
    if(*p < *q) return -1;
    if(*p<='\t') break;
    p++;
    q++;
    }
  return FXIconList::ascendingCase(pa,pb);
  }


// Compare file group
FXint FXFileList::ascendingGroup(const FXIconItem* pa,const FXIconItem* pb){
  register const FXFileItem *a=(FXFileItem*)pa;
  register const FXFileItem *b=(FXFileItem*)pb;
  register const unsigned char *p,*q;
  register int i;
  register FXint diff=(FXint)b->isDirectory() - (FXint)a->isDirectory();
  if(diff) return diff;
  for(p=(const unsigned char*)a->label.text(),i=5; *p && i; i-=(*p++=='\t'));
  for(q=(const unsigned char*)b->label.text(),i=5; *q && i; i-=(*q++=='\t'));
  while(1){
    if(*p > *q) return 1;
    if(*p < *q) return -1;
    if(*p<='\t') break;
    p++;
    q++;
    }
  return FXIconList::ascendingCase(pa,pb);
  }


// Reversed compare file name, case insensitive
FXint FXFileList::descendingCase(const FXIconItem* pa,const FXIconItem* pb){
  return -FXFileList::ascendingCase(pa,pb);
  }


// Reversed compare file name
FXint FXFileList::descending(const FXIconItem* pa,const FXIconItem* pb){
  return -FXFileList::ascending(pa,pb);
  }


// Reversed compare file type
FXint FXFileList::descendingType(const FXIconItem* pa,const FXIconItem* pb){
  return -FXFileList::ascendingType(pa,pb);
  }


// Reversed compare file size
FXint FXFileList::descendingSize(const FXIconItem* pa,const FXIconItem* pb){
  return -FXFileList::ascendingSize(pa,pb);
  }


// Reversed compare file time
FXint FXFileList::descendingTime(const FXIconItem* pa,const FXIconItem* pb){
  return -FXFileList::ascendingTime(pa,pb);
  }


// Reversed compare file user
FXint FXFileList::descendingUser(const FXIconItem* pa,const FXIconItem* pb){
  return -FXFileList::ascendingUser(pa,pb);
  }


// Reversed compare file group
FXint FXFileList::descendingGroup(const FXIconItem* pa,const FXIconItem* pb){
  return -FXFileList::ascendingGroup(pa,pb);
  }


//HANDLE FindFirstChangeNotification(
//  LPCTSTR lpPathName,    // directory name
//  BOOL bWatchSubtree,    // monitoring option
//  DWORD dwNotifyFilter   // filter conditions
//);
//
//The HANDLE can be passed to FXApp::addInput(), and you'll be notified
//for all events as specified in dwNotifyFilter.



// Refresh; don't update if user is interacting with the list
long FXFileList::onRefreshTimer(FXObject*,FXSelector,void*){
  if(flags&FLAG_UPDATE){
    scan(FALSE);
    counter=(counter+1)%REFRESHFREQUENCY;
    }
  getApp()->addTimeout(this,ID_REFRESHTIMER,REFRESHINTERVAL);
  return 0;
  }


// Force an immediate update of the list
long FXFileList::onCmdRefresh(FXObject*,FXSelector,void*){
  scan(TRUE);
  return 1;
  }


// Scan items to see if listing is necessary
void FXFileList::scan(FXbool force){
  struct stat info;

  // Stat the current directory
  if(FXFile::info(directory,info)){

    // New date of directory
    FXTime newdate=(FXTime)FXMAX(info.st_mtime,info.st_ctime);

    // Forced, date was changed, or failed to get proper date and counter expired
    if(force || (timestamp!=newdate) || (counter==0)){

      // And do the refresh
      listItems();
      sortItems();

      // Remember when we did this
      timestamp=newdate;
      }
    }

  // Move to higher directory
  else{
    setDirectory(FXFile::upLevel(directory));
    }
  }


// Set current filename
void FXFileList::setCurrentFile(const FXString& pathname){
  if(!pathname.empty()){
    FXTRACE((100,"%s::setCurrentFile(%s)\n",getClassName(),pathname.text()));
    setDirectory(FXFile::directory(pathname));
    setCurrentItem(findItem(FXFile::name(pathname)));
    setAnchorItem(getCurrentItem());
    if(0<=getCurrentItem()) selectItem(getCurrentItem());
    }
  }


// Get pathname to current file, if any
FXString FXFileList::getCurrentFile() const {
  if(current<0) return FXString::null;
  return getItemPathname(current);
  }


// Set directory being displayed
void FXFileList::setDirectory(const FXString& pathname){
  if(!pathname.empty()){
    FXTRACE((100,"%s::setDirectory(%s)\n",getClassName(),pathname.text()));
    FXString path=FXFile::absolute(directory,pathname);
    while(!FXFile::isTopDirectory(path) && !(FXFile::isShare(path) || FXFile::isDirectory(path))){
      path=FXFile::upLevel(path);
      }
    FXTRACE((1,"setDirectory path=%s\n",path.text()));
    if(directory!=path){
      directory=path;
      clearItems();
      list=NULL;
      scan(TRUE);
      }
    }
  }


// Set the pattern to filter
void FXFileList::setPattern(const FXString& ptrn){
  if(ptrn.empty()) return;
  if(pattern!=ptrn){
    pattern=ptrn;
    scan(TRUE);
    }
  }


// Change file match mode
void FXFileList::setMatchMode(FXuint mode){
  if(matchmode!=mode){
    matchmode=mode;
    scan(TRUE);
    }
  }


// Return TRUE if showing hidden files
FXbool FXFileList::showHiddenFiles() const {
  return (options&FILELIST_SHOWHIDDEN)!=0;
  }


// Change show hidden files mode
void FXFileList::showHiddenFiles(FXbool shown){
  FXuint opts=shown ? (options|FILELIST_SHOWHIDDEN) : (options&~FILELIST_SHOWHIDDEN);
  if(opts!=options){
    options=opts;
    scan(TRUE);
    }
  }


// Return TRUE if showing directories only
FXbool FXFileList::showOnlyDirectories() const {
  return (options&FILELIST_SHOWDIRS)!=0;
  }


// Change show directories only mode
void FXFileList::showOnlyDirectories(FXbool shown){
  FXuint opts=shown ? (options|FILELIST_SHOWDIRS) : (options&~FILELIST_SHOWDIRS);
  if(opts!=options){
    options=opts;
    scan(TRUE);
    }
  }


// Return TRUE if showing files only
FXbool FXFileList::showOnlyFiles() const {
  return (options&FILELIST_SHOWFILES)!=0;
  }


// Show files only
void FXFileList::showOnlyFiles(FXbool shown){
  FXuint opts=shown ? (options|FILELIST_SHOWFILES) : (options&~FILELIST_SHOWFILES);
  if(opts!=options){
    options=opts;
    scan(TRUE);
    }
  }


// Compare till '\t' or '\0'
static FXbool fileequal(const FXString& a,const FXString& b){
  register const FXuchar *p1=(const FXuchar *)a.text();
  register const FXuchar *p2=(const FXuchar *)b.text();
  register FXint c1,c2;
  do{
    c1=*p1++;
    c2=*p2++;
    }
  while(c1!='\0' && c1!='\t' && c1==c2);
  return (c1=='\0' || c1=='\t') && (c2=='\0' || c2=='\t');
  }


// Create custom item
FXIconItem *FXFileList::createItem(const FXString& text,FXIcon *big,FXIcon* mini,void* ptr){
  return new FXFileItem(text,big,mini,ptr);
  }


/********************************************************************************
*                                    X-Windows                                  *
********************************************************************************/

#ifndef WIN32


// List directory
void FXFileList::listItems(){
  FXFileItem     *oldlist=list; // Old insert-order list
  FXFileItem     *newlist=NULL; // New insert-order list
  FXFileItem    **po=&oldlist;  // Head of old list
  FXFileItem    **pn=&newlist;  // Head of new list
  FXFileItem     *curitem=NULL;
  FXFileItem     *item,*link,**pp;
  FXFileAssoc    *fileassoc;
  FXIcon         *mini;
  FXIcon         *big;
  FXString        pathname;
  FXString        extension;
  FXString        name;
  FXString        grpid;
  FXString        usrid;
  FXString        atts;
  FXString        mod;
  FXString        linkname;
  FXint           islink;
  time_t          filetime;
  struct stat     info;
  struct dirent  *dp;
  DIR            *dirp;

  // Remember current item
  if(0<=current){ curitem=(FXFileItem*)items[current]; }

  // Start inserting
  nitems=0;

  // Get directory stream pointer
  dirp=opendir(directory.text());
  if(dirp){

    // Loop over directory entries
#ifdef FOX_THREAD_SAFE
    struct fxdirent dirresult;
    while(!readdir_r(dirp,&dirresult,&dp) && dp){
#else
    while((dp=readdir(dirp))!=NULL){
#endif
      name=dp->d_name;

      // Hidden file (.xxx) or directory (. or .yyy) normally not shown,
      // but directory .. is always shown so we can navigate up or down
      if(name[0]=='.' && (name[1]==0 || (!(name[1]=='.' && name[2]==0) && !(options&FILELIST_SHOWHIDDEN)))) continue;

      // Build full pathname
      pathname=directory;
      if(!ISPATHSEP(pathname[pathname.length()-1])) pathname+=PATHSEPSTRING;
      pathname+=name;

      // Get file/link info
      if(!FXFile::linkinfo(pathname,info)) continue;

      // If its a link, get the info on file itself
      islink=S_ISLNK(info.st_mode);
      if(islink && !FXFile::info(pathname,info)) continue;

      // If it is a file and we want only directories or doesn't match, skip it
      if(!S_ISDIR(info.st_mode) && ((options&FILELIST_SHOWDIRS) || !FXFile::match(pattern,name,matchmode))) continue;

      // If it is a directory and we want only files, skip it
      if(S_ISDIR(info.st_mode) && (options&FILELIST_SHOWFILES)) continue;

      // Mod time
      filetime=info.st_mtime;

      // Find it, and take it out from the old list if found
      for(pp=po; (item=*pp)!=NULL; pp=&item->link){
        if(fileequal(item->label,name)){
          *pp=item->link;
          item->link=NULL;
          po=pp;
          goto fnd;
          }
        }

      // Make new item if we have to
      item=(FXFileItem*)createItem(NULL,NULL,NULL,NULL);

      // Add to insert-order list
fnd:  *pn=item;
      pn=&item->link;

      // Append
      FXRESIZE(&items,FXIconItem*,nitems+1);
      if(item==curitem) current=nitems;
      items[nitems]=item;
      nitems++;

      // Obtain user name
      usrid=FXFile::owner(info.st_uid);

      // Obtain group name
      grpid=FXFile::group(info.st_gid);

      // Permissions
      atts=FXFile::permissions(info.st_mode);

      // Mod time
      mod=FXFile::time(filetime);

      // Link
      if(islink) linkname=FXFile::symlink(pathname); else linkname=FXString::null;

      // Item flags
      if(info.st_mode&(S_IXUSR|S_IXGRP|S_IXOTH)){item->state|=FXFileItem::EXECUTABLE;}else{item->state&=~FXFileItem::EXECUTABLE;}
      if(S_ISDIR(info.st_mode)){item->state|=FXFileItem::FOLDER;item->state&=~FXFileItem::EXECUTABLE;}else{item->state&=~FXFileItem::FOLDER;}
      if(S_ISCHR(info.st_mode)){item->state|=FXFileItem::CHARDEV;item->state&=~FXFileItem::EXECUTABLE;}else{item->state&=~FXFileItem::CHARDEV;}
      if(S_ISBLK(info.st_mode)){item->state|=FXFileItem::BLOCKDEV;item->state&=~FXFileItem::EXECUTABLE;}else{item->state&=~FXFileItem::BLOCKDEV;}
      if(S_ISFIFO(info.st_mode)){item->state|=FXFileItem::FIFO;item->state&=~FXFileItem::EXECUTABLE;}else{item->state&=~FXFileItem::FIFO;}
      if(S_ISSOCK(info.st_mode)){item->state|=FXFileItem::SOCK;item->state&=~FXFileItem::EXECUTABLE;}else{item->state&=~FXFileItem::SOCK;}
      if(islink){item->state|=FXFileItem::SYMLINK;}else{item->state&=~FXFileItem::SYMLINK;}

      // We can drag items
      item->state|=FXFileItem::DRAGGABLE;

      // Assume no associations
      fileassoc=NULL;

      // Determine icons and type
      if(item->state&FXFileItem::FOLDER){
        big=big_folder;
        mini=mini_folder;
        extension="File Folder";
        if(associations) fileassoc=associations->findDirBinding(pathname.text());
        }
      else if(item->state&FXFileItem::EXECUTABLE){
        big=big_app;
        mini=mini_app;
        extension="Application";
        if(associations) fileassoc=associations->findExecBinding(pathname.text());
        }
      else{
        big=big_doc;
        mini=mini_doc;
        extension=FXFile::extension(pathname).upper();
        if(!extension.empty()) extension+=" File";
        if(associations) fileassoc=associations->findFileBinding(pathname.text());
        }

      // If association is found, use it
      if(fileassoc){
        extension=fileassoc->extension;
        if(fileassoc->bigicon) big=fileassoc->bigicon;
        if(fileassoc->miniicon) mini=fileassoc->miniicon;
        // FIXME use open icons also, useful when dragging over directory
        }

      // Update item information
      item->label.format("%s\t%s\t%lu\t%s\t%s\t%s\t%s\t%s",name.text(),extension.text(),(unsigned long)info.st_size,mod.text(),usrid.text(),grpid.text(),atts.text(),linkname.text());
      item->bigIcon=big;
      item->miniIcon=mini;
      item->size=(unsigned long)info.st_size;
      item->assoc=fileassoc;
      item->date=filetime;

      // Create item
      if(id()) item->create();
      }
    closedir(dirp);
    }

  // Wipe items remaining in list:- they have disappeared!!
  for(item=oldlist; item; item=link){
    link=item->link;
    delete item;
    }

  // Validate
  if(current>=nitems) current=-1;
  if(anchor>=nitems) anchor=-1;
  if(extent>=nitems) extent=-1;

  // Remember new list
  list=newlist;

  // Gotta recalc size of content
  recalc();
  }

/********************************************************************************
*                                   MS-Windows                                  *
********************************************************************************/

#else


// Convert FILETIME (# 100ns since 01/01/1601) to time_t (# s since 01/01/1970)
static time_t fxfiletime(const FILETIME& ft){
  FXlong ll=(((FXlong)ft.dwHighDateTime)<<32) | (FXlong)ft.dwLowDateTime;
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__SC__)
  ll=ll-116444736000000000LL;
#else
  ll=ll-116444736000000000L;
#endif
  ll=ll/10000000;
  return (time_t)ll;
  }


// List directory
void FXFileList::listItems(){
  FXFileItem     *oldlist=list; // Old insert-order list
  FXFileItem     *newlist=NULL; // New insert-order list
  FXFileItem    **po=&oldlist;  // Head of old list
  FXFileItem    **pn=&newlist;  // Head of new list
  FXFileItem     *curitem=NULL;
  FXFileItem     *item,*link,**pp;
  FXFileAssoc    *fileassoc;
  FXIcon         *mini;
  FXIcon         *big;
  FXString        pathname;
  FXString        extension;
  FXString        name;
  FXString        grpid;
  FXString        usrid;
  FXString        atts;
  FXString        mod;
  time_t          filetime;
  WIN32_FIND_DATA ffData;
  SHFILEINFO      sfi;
  HANDLE          hFindFile;

  // Remember current item
  if(0<=current){ curitem=(FXFileItem*)items[current]; }

  // Start inserting
  nitems=0;

  // Set path to stat with
  pathname=directory;
  if(!ISPATHSEP(pathname[pathname.length()-1])) pathname+=PATHSEPSTRING;
  pathname+="*";

  // Get file find handle and first file's info
  hFindFile=FindFirstFile(pathname.text(),&ffData);
  if(hFindFile!=INVALID_HANDLE_VALUE){

    // Loop over directory entries
    do{
      name=ffData.cFileName;

      // A dot special file?
      if(name[0]=='.' && name[1]==0) continue;

      // Hidden file or directory normally not shown
      if((ffData.dwFileAttributes&FILE_ATTRIBUTE_HIDDEN) && !(options&FILELIST_SHOWHIDDEN)) continue;

      // If it is a file and we want only directories or doesn't match, skip it
      if(!(ffData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && ((options&FILELIST_SHOWDIRS) || !FXFile::match(pattern,name,matchmode))) continue;

      // If it is a directory and we want only files, skip it
      if((ffData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && (options&FILELIST_SHOWFILES)) continue;

      // Build full pathname
      pathname=directory;
      if(!ISPATHSEP(pathname[pathname.length()-1])) pathname+=PATHSEPSTRING;
      pathname+=name;

      // Convert it
      filetime=fxfiletime(ffData.ftLastWriteTime);

      // Find it, and take it out from the old list if found
      for(pp=po; (item=*pp)!=NULL; pp=&item->link){
        if(fileequal(item->label,name)){
          *pp=item->link;
          item->link=NULL;
          po=pp;
          goto fnd;
          }
        }

      // Make new item if we have to
      item=(FXFileItem*)createItem(NULL,NULL,NULL,NULL);

      // Add to insert-order list
fnd:  *pn=item;
      pn=&item->link;

      // Append
      FXRESIZE(&items,FXIconItem*,nitems+1);
      if(item==curitem) current=nitems;
      items[nitems]=item;
      nitems++;

      // Obtain user name (no Win95 equivalent?)
      usrid=FXFile::owner(0);

      // Obtain group name (no Win95 equivalent?)
      grpid=FXFile::group(0);

      // Permissions
      atts=FXFile::permissions(0666);

      // Mod time
      mod=FXFile::time(filetime);

      // Is it a directory?
      if(ffData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){
        atts[0]='d';
        }

      // Check if file is read-only
      if(ffData.dwFileAttributes&FILE_ATTRIBUTE_READONLY){
        atts[2]='-'; atts[5]='-'; atts[8]='-';
        }

      // Is it an executable file?
      if(SHGetFileInfo(pathname.text(),0,&sfi,sizeof(SHFILEINFO),SHGFI_EXETYPE)==0){
        atts[3]='-'; atts[6]='-'; atts[9]='-';
        item->state&=~FXFileItem::EXECUTABLE;
        }
      else{
        item->state|=FXFileItem::EXECUTABLE;
        }

      // Flags
      if(ffData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) item->state|=FXFileItem::FOLDER; else item->state&=~FXFileItem::FOLDER;

      // We can drag items
      item->state|=FXFileItem::DRAGGABLE;

      // Assume no associations
      fileassoc=NULL;

      // Determine icons and type
      if(item->state&FXFileItem::FOLDER){
        big=big_folder;
        mini=mini_folder;
        extension="File Folder";
        if(associations) fileassoc=associations->findDirBinding(pathname.text());
        }
      else if(item->state&FXFileItem::EXECUTABLE){
        big=big_app;
        mini=mini_app;
        extension="Application";
        if(associations) fileassoc=associations->findExecBinding(pathname.text());
        }
      else{
        big=big_doc;
        mini=mini_doc;
        extension=FXFile::extension(pathname).upper();
        if(!extension.empty()) extension+=" File";
        if(associations) fileassoc=associations->findFileBinding(pathname.text());
        }

      // If association is found, use it
      if(fileassoc){
        extension=fileassoc->extension;
        if(fileassoc->bigicon) big=fileassoc->bigicon;
        if(fileassoc->miniicon) mini=fileassoc->miniicon;
        // FIXME use open icons also, useful when dragging over directory
        }

      // Update item information
      item->label.format("%s\t%s\t%u\t%s\t%s\t%s\t%s",name.text(),extension.text(),(unsigned)ffData.nFileSizeLow,mod.text(),usrid.text(),grpid.text(),atts.text());
      item->bigIcon=big;
      item->miniIcon=mini;
      item->size=(unsigned long)ffData.nFileSizeLow;
      item->assoc=fileassoc;
      item->date=filetime;

      // Create item
      if(id()) item->create();
      }
    while(FindNextFile(hFindFile,&ffData));
    FindClose(hFindFile);
    }

  // Wipe items remaining in list:- they have disappeared!!
  for(item=oldlist; item; item=link){
    link=item->link;
    delete item;
    }

  // Validate
  if(current>=nitems) current=-1;
  if(anchor>=nitems) anchor=-1;
  if(extent>=nitems) extent=-1;

  // Remember new list
  list=newlist;

  // Gotta recalc size of content
  recalc();
  }


#endif


/*******************************************************************************/


// Is directory
FXbool FXFileList::isItemDirectory(FXint index) const {
  if(index<0 || nitems<=index){ fxerror("%s::isItemDirectory: index out of range.\n",getClassName()); }
  return ((FXFileItem*)items[index])->isDirectory();
  }


// Is share
FXbool FXFileList::isItemShare(FXint index) const {
  if(index<0 || nitems<=index){ fxerror("%s::isItemShare: index out of range.\n",getClassName()); }
  return ((FXFileItem*)items[index])->isShare();
  }


// Is file
FXbool FXFileList::isItemFile(FXint index) const {
  if(index<0 || nitems<=index){ fxerror("%s::isItemFile: index out of range.\n",getClassName()); }
  return ((FXFileItem*)items[index])->isFile();
  }


// Is executable
FXbool FXFileList::isItemExecutable(FXint index) const {
  if(index<0 || nitems<=index){ fxerror("%s::isItemExecutable: index out of range.\n",getClassName()); }
  return ((FXFileItem*)items[index])->isExecutable();
  }


// Get file name from item
FXString FXFileList::getItemFilename(FXint index) const {
  if(index<0 || nitems<=index){ fxerror("%s::getItemFilename: index out of range.\n",getClassName()); }
  return items[index]->label.section('\t',0);
  }


// Get full pathname to item
FXString FXFileList::getItemPathname(FXint index) const {
  if(index<0 || nitems<=index){ fxerror("%s::getItemPathname: index out of range.\n",getClassName()); }
  return FXFile::absolute(directory,items[index]->label.section('\t',0));
  }


// Get associations (if any) from the file
FXFileAssoc* FXFileList::getItemAssoc(FXint index) const {
  if(index<0 || nitems<=index){ fxerror("%s::getItemAssoc: index out of range.\n",getClassName()); }
  return ((FXFileItem*)items[index])->getAssoc();
  }


// Change associations table; force a rescan so as to
// update the bindings in each item to the new associations
void FXFileList::setAssociations(FXFileDict* assocs){
  if(associations!=assocs){
    associations=assocs;
    scan(TRUE);
    }
  }


// Save data
void FXFileList::save(FXStream& store) const {
  FXIconList::save(store);
  store << directory;
  store << associations;
  store << pattern;
  store << matchmode;
  store << big_folder;
  store << mini_folder;
  store << big_doc;
  store << mini_doc;
  store << big_app;
  store << mini_app;
  }


// Load data
void FXFileList::load(FXStream& store){
  FXIconList::load(store);
  store >> directory;
  store >> associations;
  store >> pattern;
  store >> matchmode;
  store >> big_folder;
  store >> mini_folder;
  store >> big_doc;
  store >> mini_doc;
  store >> big_app;
  store >> mini_app;
  }


// Cleanup
FXFileList::~FXFileList(){
  getApp()->removeTimeout(this,ID_REFRESHTIMER);
  getApp()->removeTimeout(this,ID_OPENTIMER);
  if(!(options&FILELIST_NO_OWN_ASSOC)) delete associations;
  delete big_folder;
  delete mini_folder;
  delete big_doc;
  delete mini_doc;
  delete big_app;
  delete mini_app;
  associations=(FXFileDict*)-1L;
  list=(FXFileItem*)-1L;
  big_folder=(FXIcon*)-1L;
  mini_folder=(FXIcon*)-1L;
  big_doc=(FXIcon*)-1L;
  mini_doc=(FXIcon*)-1L;
  big_app=(FXIcon*)-1L;
  mini_app=(FXIcon*)-1L;
  }

}

#endif

