/********************************************************************************
*                                                                               *
*                         D o c k   S i t e   W i d g e t                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004 by Jeroen van der Zijp.   All Rights Reserved.             *
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
* $Id: FXDockSite.h,v 1.3 2004/09/09 03:49:45 fox Exp $                         *
********************************************************************************/
#ifndef FXDOCKSITE_H
#define FXDOCKSITE_H

#ifndef FXPACKER_H
#include "FXPacker.h"
#endif

namespace FX {


//////////////////////////////  UNDER DEVELOPMENT  //////////////////////////////


/**
* Dock site widget is a widget where toolbars and so on can be docked.
* The dock site widgets are typically embedded in a main window, where
* docking is to be allowed.
*/
class FXAPI FXDockSite : public FXPacker {
  FXDECLARE(FXDockSite)
protected:
  FXDockSite(){}
private:
  FXDockSite(const FXDockSite&);
  FXDockSite &operator=(const FXDockSite&);
public:

  /// Construct a dock site layout manager
  FXDockSite(FXComposite *p,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_SPACING,FXint pr=DEFAULT_SPACING,FXint pt=DEFAULT_SPACING,FXint pb=DEFAULT_SPACING,FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING);

  /// Perform layout
  virtual void layout();

  /// Return default width
  virtual FXint getDefaultWidth();

  /// Return default height
  virtual FXint getDefaultHeight();
  };

}

#endif
