/********************************************************************************
*                                                                               *
*                       H a s h   T a b l e   C l a s s                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXHash.h,v 1.9 2005/01/16 16:06:06 fox Exp $                             *
********************************************************************************/
#ifndef FXHASH_H
#define FXHASH_H

#include "fxdefs.h"

namespace FX {


/**
* A hash table for associating pointers to pointers.
*/
class FXAPI FXHash {
private:
  struct FXEntry {
    void* key;
    void* val;
    };
private:
  FXEntry *table;       // Hash table
  FXuint   used;        // Number of used entries
  FXuint   free;        // Number of free entries
  FXuint   max;         // Maximum entry index
private:
  void resize(FXuint m);
private:
  FXHash(const FXHash&);
  FXHash &operator=(const FXHash&);
public:

  /// Construct empty hash table
  FXHash();

  /// Return number of items in table
  FXuint no() const { return used; }

  /// Insert key into the table
  void* insert(void* key,void* val);

  /// Replace key in table
  void* replace(void* key,void* val);

  /// Remove key from the table
  void* remove(void* key);

  /// Return value of key
  void* find(void* key) const;

  /// Clear hash table
  void clear();

  /// Destructor
  virtual ~FXHash();
  };


}

#endif
