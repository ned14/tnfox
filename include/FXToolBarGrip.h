/********************************************************************************
*                                                                               *
*                     T o o l   B a r   G r i p   W i d g e t                   *
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
* $Id: FXToolBarGrip.h,v 1.8 2004/10/19 00:48:59 fox Exp $                      *
********************************************************************************/
#ifndef FXTOOLBARGRIP_H
#define FXTOOLBARGRIP_H

#ifndef FXWINDOW_H
#include "FXWindow.h"
#endif

namespace FX {


/// Tool Bar Grip styles
enum {
  TOOLBARGRIP_SINGLE     = 0,             /// Single bar mode for movable toolbars
  TOOLBARGRIP_DOUBLE     = 0x00008000,    /// Double bar mode for dockable toolbars
  TOOLBARGRIP_SEPARATOR  = 0x00010000     /// Separator mode
  };

class FXToolBar;


/**
* A toolbar grip is used to move its parent [an FXToolBar].
* The grip draws either a single or double bar; it is customary
* to use the single bar grip for toolbar-rearrangements only,
* and use the double-bar when the toolbar needs to be floated
* or docked.
* The toolbar grip is automatically oriented properly by the
* the toolbar widget.
*/
class FXAPI FXToolBarGrip : public FXWindow {
  FXDECLARE(FXToolBarGrip)
protected:
  FXColor  activeColor; // Color when active
  FXColor  hiliteColor; // Highlight color
  FXColor  shadowColor; // Shadow color
  FXString tip;         // Tooltip
protected:
  FXToolBarGrip();
private:
  FXToolBarGrip(const FXToolBarGrip&);
  FXToolBarGrip& operator=(const FXToolBarGrip&);
public:
  long onPaint(FXObject*,FXSelector,void*);
  long onLeftBtnPress(FXObject*,FXSelector,void*);
  long onLeftBtnRelease(FXObject*,FXSelector,void*);
  long onMotion(FXObject*,FXSelector,void*);
  long onEnter(FXObject*,FXSelector,void*);
  long onLeave(FXObject*,FXSelector,void*);
  long onCmdSetTip(FXObject*,FXSelector,void*);
  long onCmdGetTip(FXObject*,FXSelector,void*);
  long onQueryTip(FXObject*,FXSelector,void*);
public:

  /// Construct toolbar grip
  FXToolBarGrip(FXToolBar* p,FXObject* tgt=NULL,FXSelector sel=0,FXuint opts=TOOLBARGRIP_SINGLE,FXint x=0,FXint y=0,FXint w=0,FXint h=0);

  /// Return default width
  virtual FXint getDefaultWidth();

  /// Return default height
  virtual FXint getDefaultHeight();

  /// Change toolbar grip to double
  void setDoubleBar(FXbool dbl=TRUE);

  /// Return TRUE if toolbar grip is displayed as a double bar
  FXbool isDoubleBar() const;

  /// Change highlight color
  void setHiliteColor(FXColor clr);

  /// Get highlight color
  FXColor getHiliteColor() const { return hiliteColor; }

  /// Change shadow color
  void setShadowColor(FXColor clr);

  /// Get shadow color
  FXColor getShadowColor() const { return shadowColor; }

  /// Set the active color
  void setActiveColor(FXColor clr);

  /// Get the active color
  FXColor getActiveColor() const { return activeColor; }

  /// Set the tool tip message for the toolbar grip
  void setTipText(const FXString& text){ tip=text; }

  /// Get the tool tip message for the toolbar grip
  FXString getTipText() const { return tip; }

  /// Save to stream
  virtual void save(FXStream& store) const;

  /// Load from stream
  virtual void load(FXStream& store);
  };

}

#endif

