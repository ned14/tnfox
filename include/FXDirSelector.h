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
* $Id: FXDirSelector.h,v 1.16 2004/02/08 17:17:33 fox Exp $                     *
********************************************************************************/
#ifndef BUILDING_TCOMMON

#ifndef FXDIRSELECTOR_H
#define FXDIRSELECTOR_H

#ifndef FXPACKER_H
#include "FXPacker.h"
#endif

namespace FX {


class FXDirList;
class FXTextField;
class FXButton;


/// Directory selection widget
class FXAPI FXDirSelector : public FXPacker {
  FXDECLARE(FXDirSelector)
protected:
  FXDirList     *dirbox;          // Directory list widget
  FXTextField   *dirname;         // Directory name entry field
  FXButton      *accept;          // Accept button
  FXButton      *cancel;          // Cancel button
protected:
  FXDirSelector(){}
private:
  FXDirSelector(const FXDirSelector&);
  FXDirSelector &operator=(const FXDirSelector&);
public:
  long onCmdName(FXObject*,FXSelector,void*);
  long onCmdOpened(FXObject*,FXSelector,void*);
  long onCmdHome(FXObject*,FXSelector,void*);
  long onCmdWork(FXObject*,FXSelector,void*);
  long onCmdDirectoryUp(FXObject*,FXSelector,void*);
public:
  enum {
    ID_DIRNAME=FXPacker::ID_LAST,
    ID_DIRLIST,
    ID_HOME,
    ID_WORK,
    ID_DIRECTORY_UP,
    ID_LAST
    };
public:

  /// Constructor
  FXDirSelector(FXComposite *p,FXObject* tgt=NULL,FXSelector sel=0,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0);

  /// Return a pointer to the "Accept" button
  FXButton *acceptButton() const { return accept; }

  /// Return a pointer to the "Cancel" button
  FXButton *cancelButton() const { return cancel; }

  /// Change directory
  void setDirectory(const FXString& path);

  /// Return directory
  FXString getDirectory() const;

  /// Change Directory List style
  void setDirBoxStyle(FXuint style);

  /// Return Directory List style
  FXuint getDirBoxStyle() const;

  /// Save to stream
  virtual void save(FXStream& store) const;

  /// Load from stream
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~FXDirSelector();
  };

}

#endif
#endif

