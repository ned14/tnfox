/********************************************************************************
*                                                                               *
*                D i r e c t o r y   S e l e c t i o n   D i a l o g            *
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
* $Id: FXDirDialog.h,v 1.13 2004/02/08 17:17:33 fox Exp $                       *
********************************************************************************/
#ifndef BUILDING_TCOMMON

#ifndef FXDIRDIALOG_H
#define FXDIRDIALOG_H

#ifndef FXDIALOGBOX_H
#include "FXDialogBox.h"
#endif

namespace FX {


class FXDirSelector;


/// Directory selection dialog
class FXAPI FXDirDialog : public FXDialogBox {
  FXDECLARE(FXDirDialog)
protected:
  FXDirSelector *dirbox;          // Directory selection widget
protected:
  FXDirDialog(){}
private:
  FXDirDialog(const FXDirDialog&);
  FXDirDialog &operator=(const FXDirDialog&);
public:

  /// Construct Directory Dialog Box
  FXDirDialog(FXWindow* owner,const FXString& name,FXuint opts=0,FXint x=0,FXint y=0,FXint w=400,FXint h=300);

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
  virtual ~FXDirDialog();
  };

}

#endif
#endif
