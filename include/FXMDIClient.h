/********************************************************************************
*                                                                               *
*         M u l t i p l e   D o c u m e n t   C l i e n t   W i n d o w         *
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
* $Id: FXMDIClient.h,v 1.29 2004/02/08 17:17:33 fox Exp $                       *
********************************************************************************/
#ifndef FXMDICLIENT_H
#define FXMDICLIENT_H

#ifndef FXCOMPOSITE_H
#include "FXComposite.h"
#endif

namespace FX {


class FXMDIChild;


/**
* The MDI client window manages a number of MDI child windows in
* a multiple-document interface (MDI) application.
* MDI child windows usually receive messages from the GUI through
* delegation via the MDI client, i.e. the MDI client window is set as
* the target for most GUI commands; the MDI client filters out a few messages
* and forwards all other messages to the active MDI child.
* MDI client can arrange the MDI child windows in various ways:-
* it may maximize one of the MDI child windows, arrange them side-by-side,
* cascade them, or iconify them.
* MDI child windows are notified about changes in the active MDI child
* window by the MDI client.
*/
class FXAPI FXMDIClient : public FXComposite {
  FXDECLARE(FXMDIClient)
  friend class FXMDIChild;
protected:
  FXMDIChild *active;             // Active child
  FXint       cascadex;           // Cascade offset X
  FXint       cascadey;           // Cascade offset Y
protected:
  FXMDIClient();
private:
  FXMDIClient(const FXMDIClient&);
  FXMDIClient &operator=(const FXMDIClient&);
public:
  long onCmdActivateNext(FXObject*,FXSelector,void*);
  long onCmdActivatePrev(FXObject*,FXSelector,void*);
  long onCmdTileHorizontal(FXObject*,FXSelector,void*);
  long onCmdTileVertical(FXObject*,FXSelector,void*);
  long onCmdCascade(FXObject*,FXSelector,void*);
  long onUpdActivateNext(FXObject*,FXSelector,void*);
  long onUpdActivatePrev(FXObject*,FXSelector,void*);
  long onUpdTileVertical(FXObject*,FXSelector,void*);
  long onUpdTileHorizontal(FXObject*,FXSelector,void*);
  long onUpdCascade(FXObject*,FXSelector,void*);
  long onUpdClose(FXObject*,FXSelector,void*);
  long onUpdMenuClose(FXObject*,FXSelector,void*);
  long onUpdRestore(FXObject*,FXSelector,void*);
  long onUpdMenuRestore(FXObject*,FXSelector,void*);
  long onUpdMinimize(FXObject*,FXSelector,void*);
  long onUpdMenuMinimize(FXObject*,FXSelector,void*);
  long onUpdMaximize(FXObject*,FXSelector,void*);
  long onUpdMenuWindow(FXObject*,FXSelector,void*);
  long onCmdWindowSelect(FXObject*,FXSelector,void*);
  long onUpdWindowSelect(FXObject*,FXSelector,void*);
  long onCmdOthersWindows(FXObject*,FXSelector,void*);
  long onUpdOthersWindows(FXObject*,FXSelector,void*);
  long onUpdAnyWindows(FXObject*,FXSelector,void*);
  virtual long onDefault(FXObject*,FXSelector,void*);
public:
  enum {
    ID_MDI_ANY=65400,
    ID_MDI_1,           // Select MDI child 1
    ID_MDI_2,
    ID_MDI_3,
    ID_MDI_4,
    ID_MDI_5,
    ID_MDI_6,
    ID_MDI_7,
    ID_MDI_8,
    ID_MDI_9,
    ID_MDI_10,
    ID_MDI_OVER_1,      // Sensitize MDI menu when 1 or more children
    ID_MDI_OVER_2,
    ID_MDI_OVER_3,
    ID_MDI_OVER_4,
    ID_MDI_OVER_5,
    ID_MDI_OVER_6,
    ID_MDI_OVER_7,
    ID_MDI_OVER_8,
    ID_MDI_OVER_9,
    ID_MDI_OVER_10,
    ID_LAST
    };
public:

  /// Construct MDI Client window
  FXMDIClient(FXComposite* p,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0);

  /// Return default width
  virtual FXint getDefaultWidth();

  /// Return default height
  virtual FXint getDefaultHeight();

  /// Perform layout
  virtual void layout();

  /**
  * Pass message to all MDI windows, stopping when one of
  * the MDI windows fails to handle the message.
  */
  long forallWindows(FXObject* sender,FXSelector sel,void* ptr);

  /**
  * Pass message once to all MDI windows with the same document,
  * stopping when one of the MDI windows fails to handle the message.
  */
  long forallDocuments(FXObject* sender,FXSelector sel,void* ptr);

  /**
  * Pass message to all MDI Child windows whose target is document,
  * stopping when one of the MDI windows fails to handle the message.
  */
  long forallDocWindows(FXObject* document,FXObject* sender,FXSelector sel,void* ptr);

  /// Set active MDI Child
  virtual FXbool setActiveChild(FXMDIChild* child=NULL,FXbool notify=TRUE);

  /// Get current active child; may be NULL!
  FXMDIChild* getActiveChild() const { return active; }

  // Cascade windows
  virtual void cascade(FXbool notify=FALSE);

  // Layout horizontally
  virtual void horizontal(FXbool notify=FALSE);

  // Layout vertically
  virtual void vertical(FXbool notify=FALSE);

  /// Change cascade offset X
  void setCascadeX(FXint off){ cascadex=off; }

  /// Change cascade offset Y
  void setCascadeY(FXint off){ cascadey=off; }

  /// Get cascade offset X
  FXint getCascadeX() const { return cascadex; }

  /// Get cascade offset Y
  FXint getCascadeY() const { return cascadey; }

  /// Save object to a stream
  virtual void save(FXStream& store) const;

  /// Load object from a stream
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~FXMDIClient();
  };

}

#endif
