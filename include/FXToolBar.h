/********************************************************************************
*                                                                               *
*                        T o o l B a r   W i d g e t                            *
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
* $Id: FXToolBar.h,v 1.6 2004/04/05 14:49:33 fox Exp $                          *
********************************************************************************/
#ifndef FXTOOLBAR_H
#define FXTOOLBAR_H

#ifndef FXPACKER_H
#include "FXPacker.h"
#endif
#include "FXRectangle.h"

namespace FX {


/**
* ToolBar control.
*/
class FXAPI FXToolBar : public FXPacker {
  FXDECLARE(FXToolBar)
protected:
  FXComposite   *drydock;     // Parent when docked
  FXComposite   *wetdock;     // Parent when floating
  FXRectangle    outline;     // Outline shown while dragging
  FXWindow      *dockafter;   // Dock after this window
  FXuint         dockside;    // Dock on this side
  FXbool         docking;     // Dock it
protected:
  FXToolBar();
private:
  FXToolBar(const FXToolBar&);
  FXToolBar &operator=(const FXToolBar&);
public:
  long onCmdUndock(FXObject*,FXSelector,void*);
  long onUpdUndock(FXObject*,FXSelector,void*);
  long onCmdDockTop(FXObject*,FXSelector,void*);
  long onUpdDockTop(FXObject*,FXSelector,void*);
  long onCmdDockBottom(FXObject*,FXSelector,void*);
  long onUpdDockBottom(FXObject*,FXSelector,void*);
  long onCmdDockLeft(FXObject*,FXSelector,void*);
  long onUpdDockLeft(FXObject*,FXSelector,void*);
  long onCmdDockRight(FXObject*,FXSelector,void*);
  long onUpdDockRight(FXObject*,FXSelector,void*);
  long onBeginDragGrip(FXObject*,FXSelector,void*);
  long onEndDragGrip(FXObject*,FXSelector,void*);
  long onDraggedGrip(FXObject*,FXSelector,void*);
public:
  enum {
    ID_UNDOCK=FXPacker::ID_LAST,  /// Undock the toolbar
    ID_DOCK_TOP,                  /// Dock on the top
    ID_DOCK_BOTTOM,               /// Dock on the bottom
    ID_DOCK_LEFT,                 /// Dock on the left
    ID_DOCK_RIGHT,                /// Dock on the right
    ID_TOOLBARGRIP,               /// Notifications from toolbar grip
    ID_LAST
    };
public:

  /**
  * Construct a floatable toolbar
  * Normally, the toolbar is docked under window p.
  * When floated, the toolbar can be docked under window q, which is
  * typically an FXToolBarShell window.
  */
  FXToolBar(FXComposite* p,FXComposite* q,FXuint opts=LAYOUT_TOP|LAYOUT_LEFT|LAYOUT_FILL_X,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=3,FXint pr=3,FXint pt=2,FXint pb=2,FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING);

  /**
  * Construct a non-floatable toolbar.
  * The toolbar can not be undocked.
  */
  FXToolBar(FXComposite* p,FXuint opts=LAYOUT_TOP|LAYOUT_LEFT|LAYOUT_FILL_X,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=2,FXint pr=3,FXint pt=3,FXint pb=2,FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING);

  /**
  * Set parent when docked.
  * If it was docked, reparent under the new docking window.
  */
  void setDryDock(FXComposite* dry);

  /**
  * Set parent when floating.
  * If it was undocked, then reparent under the new floating window.
  */
  void setWetDock(FXComposite* wet);

  /// Return parent when docked
  FXComposite* getDryDock() const { return drydock; }

  /// Return parent when floating
  FXComposite* getWetDock() const { return wetdock; }

  /// Return true if toolbar is docked
  FXbool isDocked() const;

  /**
  * Dock the bar against the given side, after some other widget.
  * However, if after is -1, it will be docked as the innermost bar just before
  * the work-area, while if after is 0, if will be docked as the outermost bar.
  */
  virtual void dock(FXuint side=LAYOUT_SIDE_TOP,FXWindow* after=(FXWindow*)-1L);

  /**
  * Undock or float the bar.
  * The initial position of the wet dock is a few pixels
  * below and to the right of the original docked position.
  */
  virtual void undock();

  /// Return default width
  virtual FXint getDefaultWidth();

  /// Return default height
  virtual FXint getDefaultHeight();

  /// Perform layout
  virtual void layout();

  /// Return width for given height
  virtual FXint getWidthForHeight(FXint h);

  /// Return height for given width
  virtual FXint getHeightForWidth(FXint w);

  /// Set docking side
  void setDockingSide(FXuint side=LAYOUT_SIDE_TOP);

  /// Return docking side
  FXuint getDockingSide() const;

  /// Save toolbar to a stream
  virtual void save(FXStream& store) const;

  /// Load toolbar from a stream
  virtual void load(FXStream& store);

  /// Destroy
  virtual ~FXToolBar();
  };

}

#endif

