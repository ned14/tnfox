/********************************************************************************
*                                                                               *
*                   T o o l B a r    D o c k    W i d g e t                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXToolBarDock.h,v 1.6 2005/01/31 00:29:17 fox Exp $                      *
********************************************************************************/
#ifndef FXTOOLBARDOCK_H
#define FXTOOLBARDOCK_H

#ifndef FXPACKER_H
#include "FXPacker.h"
#endif

namespace FX {


class FXToolBar;

/**
* The toolbar dock widget is a widget where toolbars can be docked.
* Toolbar dock widgets are typically embedded inside the main window, placed
* against those sides where docking of toolbars is to be allowed.
* Toolbars placed inside a toolbar dock are laid out in horizontal or vertical bands
* called galleys.  A toolbar with the LAYOUT_DOCK_SAME hint is preferentially placed
* on the same galley as its previous sibling.  A toolbar with the LAYOUT_DOCK_NEXT is
* always placed on the next galley.
* Each galley will have at least one toolbar shown in it.  Several toolbars
* may be placed side-by-side inside one galley, unless there is insufficient
* room.  If there is insufficient room to place another toolbar, that toolbar
* will be moved to the next galley, even though its LAYOUT_DOCK_NEXT option
* is not set.  This implies that when the main window is resized and more room
* becomes available, it will jump back to its preferred galley.
* Within a galley, toolbars will be placed from left to right, at the given
* x and y coordinates, with the constraints that the toolbars will stay within
* the galley, and do not overlap each other.  It is possible to use LAYOUT_FILL_X
* and/or LAYOUT_FILL_Y to stretch a toolbar to the available space on its galley.
* The galleys are oriented horizontally if the dock site is placed inside
* a top level window using LAYOUT_SIDE_TOP or LAYOUT_SIDE_BOTTOM, and
* vertically oriented if placed with LAYOUT_SIDE_LEFT or LAYOUT_SIDE_RIGHT.
*/
class FXAPI FXToolBarDock : public FXPacker {
  FXDECLARE(FXToolBarDock)
protected:
  FXToolBarDock(){}
private:
  FXToolBarDock(const FXToolBarDock&);
  FXToolBarDock &operator=(const FXToolBarDock&);
protected:
  void moveVerBar(FXWindow* bar,FXWindow *begin,FXWindow* end,FXint bx,FXint by);
  void moveHorBar(FXWindow* bar,FXWindow *begin,FXWindow* end,FXint bx,FXint by);
  FXint galleyWidth(FXWindow *begin,FXWindow*& end,FXint space,FXint& require,FXint& expand) const;
  FXint galleyHeight(FXWindow *begin,FXWindow*& end,FXint space,FXint& require,FXint& expand) const;
public:

  /**
  * Construct a toolbar dock layout manager.  Passing LAYOUT_SIDE_TOP or LAYOUT_SIDE_BOTTOM
  * causes the toolbar dock to be oriented horizontally.  Passing LAYOUT_SIDE_LEFT or
  * LAYOUT_SIDE_RIGHT causes it to be oriented vertically.
  */
  FXToolBarDock(FXComposite *p,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=0,FXint pr=0,FXint pt=0,FXint pb=0,FXint hs=0,FXint vs=0);

  /// Perform layout
  virtual void layout();

  /**
  * Return width for given height.  This should be
  * used for vertically oriented toolbar docks only.
  */
  virtual FXint getWidthForHeight(FXint h);

  /**
  * Return height for given width.  This should be
  * used for horizontally oriented toolbar docks only.
  */
  virtual FXint getHeightForWidth(FXint w);

  /**
  * Return default width.  This is the width the toolbar
  * dock would have if no toolbars need to be moved to other
  * galleys than they would be logically placed.
  */
  virtual FXint getDefaultWidth();

  /**
  * Return default height.  This is the height the toolbar
  * dock would have if no toolbars need to be moved to other
  * galleys than they would be logically placed.
  */
  virtual FXint getDefaultHeight();

  /**
  * Move tool bar, changing its options to suite the new position.
  * Used by the toolbar dragging to rearrange the toolbars inside the
  * toolbar dock.  This function returns FALSE if the given position
  * is too far outside the toolbar dock to consider docking it.
  */
  virtual FXbool moveToolBar(FXToolBar* bar,FXint barx,FXint bary);

  /**
  * Attempt to dock.  Inspect shape and location of bar in relation
  * to this toolbar dock site and return TRUE if a dock is indicated.
  * Note: to prevent oscillation, the logic of this routine should
  * closely match that of moveToolBar().
  */
  virtual FXbool dockToolBar(FXToolBar* bar,FXint barx,FXint bary);
  };

}

#endif
