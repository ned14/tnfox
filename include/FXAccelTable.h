/********************************************************************************
*                                                                               *
*                   A c c e l e r a t o r   T a b l e   C l a s s               *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXAccelTable.h,v 1.21 2005/01/16 16:06:06 fox Exp $                      *
********************************************************************************/
#ifndef FXACCELTABLE_H
#define FXACCELTABLE_H

#ifndef FXOBJECT_H
#include "FXObject.h"
#endif

namespace FX {


/**
* The accelerator table sends a message to a specific
* target object when the indicated key and modifier combination
* is pressed.
*/
class FXAPI FXAccelTable : public FXObject {
  FXDECLARE(FXAccelTable)
protected:
  struct FXAccelKey {
    FXObject    *target;    // Target object of message
    FXSelector   messagedn; // Message being sent
    FXSelector   messageup; // Message being sent
    FXHotKey     code;      // Keysym and modifier mask to match
    };
private:
  FXAccelKey *key;          // Accelerator table
  FXuint      max;          // Largest table index
  FXuint      num;          // Number of entries
private:
  void resize(FXuint m);
private:
  FXAccelTable(const FXAccelTable&);
  FXAccelTable &operator=(const FXAccelTable&);
public:
  long onKeyPress(FXObject*,FXSelector,void*);
  long onKeyRelease(FXObject*,FXSelector,void*);
public:

  /// Construct empty accelerator table
  FXAccelTable();

  /// Add an accelerator into the table
  void addAccel(FXHotKey hotkey,FXObject* target=NULL,FXSelector seldn=0,FXSelector selup=0);

  /// Remove an accelerator from the table
  void removeAccel(FXHotKey hotkey);

  /// Return true if accelerator specified
  FXbool hasAccel(FXHotKey hotkey) const;

  /// Return target object of the given accelerator
  FXObject* targetOfAccel(FXHotKey hotkey) const;

  /// Save table to a stream
  virtual void save(FXStream& store) const;

  /// Load table from a stream
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~FXAccelTable();
  };


}

#endif
