/********************************************************************************
*                                                                               *
*                       C o m b o   B o x   W i d g e t                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXComboBox.h,v 1.33 2004/02/08 17:17:33 fox Exp $                        *
********************************************************************************/
#ifndef FXCOMBOBOX_H
#define FXCOMBOBOX_H

#ifndef FXPACKER_H
#include "FXPacker.h"
#endif
#include "FXFrame.h"
class FXListSortFunc;

namespace FX {


// ComboBox styles
enum {
  COMBOBOX_NO_REPLACE     = 0,                  // Leave the list the same
  COMBOBOX_REPLACE        = 0x00020000,         // Replace current item with typed text
  COMBOBOX_INSERT_BEFORE  = 0x00040000,         // Typed text inserted before current
  COMBOBOX_INSERT_AFTER   = 0x00060000,         // Typed text inserted after current
  COMBOBOX_INSERT_FIRST   = 0x00080000,         // Typed text inserted at begin of list
  COMBOBOX_INSERT_LAST    = 0x00090000,         // Typed text inserted at end of list
  COMBOBOX_STATIC         = 0x00100000,         // Unchangable text box
  COMBOBOX_NORMAL         = 0                   // Can type text but list is not changed
  };


class FXTextField;
class FXMenuButton;
class FXList;
class FXPopup;
class FXFont;

/// Combobox
class FXAPI FXComboBox : public FXPacker {
  FXDECLARE(FXComboBox)
protected:
  FXTextField   *field;
  FXMenuButton  *button;
  FXList        *list;
  FXPopup       *pane;
protected:
  FXComboBox(){}
private:
  FXComboBox(const FXComboBox&);
  FXComboBox &operator=(const FXComboBox&);
public:
  long onFocusUp(FXObject*,FXSelector,void*);
  long onFocusDown(FXObject*,FXSelector,void*);
  long onFocusSelf(FXObject*,FXSelector,void*);
  long onTextButton(FXObject*,FXSelector,void*);
  long onTextChanged(FXObject*,FXSelector,void*);
  long onTextCommand(FXObject*,FXSelector,void*);
  long onListClicked(FXObject*,FXSelector,void*);
  long onFwdToText(FXObject*,FXSelector,void*);
  long onUpdFmText(FXObject*,FXSelector,void*);
public:
  enum {
    ID_LIST=FXPacker::ID_LAST,
    ID_TEXT,
    ID_LAST
    };
public:

  /// Constructor
  FXComboBox(FXComposite *p,FXint cols,FXObject* tgt=NULL,FXSelector sel=0,FXuint opts=COMBOBOX_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);

  /// Create server-side resources
  virtual void create();

  /// Detach server-side resources
  virtual void detach();

  /// Destroy server-side resources
  virtual void destroy();

  /// Enable combo box
  virtual void enable();

  /// Disable combo box
  virtual void disable();

  /// Return default width
  virtual FXint getDefaultWidth();

  /// Return default height
  virtual FXint getDefaultHeight();

  /// Perform layout
  virtual void layout();

  /// Return true if combobox is editable
  FXbool isEditable() const;

  /// Set editable state
  void setEditable(FXbool edit=TRUE);

  /// Set the text
  void setText(const FXString& text);

  /// Get the text
  FXString getText() const;

  /// Set the number of columns
  void setNumColumns(FXint cols);

  /// Get the number of columns
  FXint getNumColumns() const;

  /// Return the number of items in the list
  FXint getNumItems() const;

  /// Return the number of visible items
  FXint getNumVisible() const;

  /// Set the number of visible items
  void setNumVisible(FXint nvis);

  /// Return true if current item
  FXbool isItemCurrent(FXint index) const;

  /// Set the current item (index is zero-based)
  void setCurrentItem(FXint indexz);

  /// Get the current item's index
  FXint getCurrentItem() const;

  /// Return the item at the given index
  FXString getItem(FXint index) const;

  /// Replace the item at index
  FXint setItem(FXint index,const FXString& text,void* ptr=NULL);

  /// Insert a new item at index
  FXint insertItem(FXint index,const FXString& text,void* ptr=NULL);

  /// Append an item to the list
  FXint appendItem(const FXString& text,void* ptr=NULL);

  /// Prepend an item to the list
  FXint prependItem(const FXString& text,void* ptr=NULL);

  /// Move item from oldindex to newindex
  FXint moveItem(FXint newindex,FXint oldindex);

  /// Remove this item from the list
  void removeItem(FXint index);

  /// Remove all items from the list
  void clearItems();

  /**
  * Search items for item by name, starting from start item; the
  * flags argument controls the search direction, and case sensitivity.
  */
  FXint findItem(const FXString& text,FXint start=-1,FXuint flags=SEARCH_FORWARD|SEARCH_WRAP) const;

  /// Set text for specified item
  void setItemText(FXint index,const FXString& text);

  /// Get text for specified item
  FXString getItemText(FXint index) const;

  /// Set data pointer for specified item
  void setItemData(FXint index,void* ptr) const;

  /// Get data pointer for specified item
  void* getItemData(FXint index) const;

  /// Is the pane shown
  FXbool isPaneShown() const;

  /// Sort items using current sort function
  void sortItems();

  /// Set text font
  void setFont(FXFont* fnt);

  /// Get text font
  FXFont* getFont() const;

  /// Set the combobox style.
  void setComboStyle(FXuint mode);

  /// Get the combobox style.
  FXuint getComboStyle() const;

  /// Set window background color
  virtual void setBackColor(FXColor clr);

  /// Get background color
  FXColor getBackColor() const;

  /// Change text color
  void setTextColor(FXColor clr);

  /// Return text color
  FXColor getTextColor() const;

  /// Change selected background color
  void setSelBackColor(FXColor clr);

  /// Return selected background color
  FXColor getSelBackColor() const;

  /// Change selected text color
  void setSelTextColor(FXColor clr);

  /// Return selected text color
  FXColor getSelTextColor() const;

  /// Return sort function
  FXListSortFunc getSortFunc() const;

  /// Change sort function
  void setSortFunc(FXListSortFunc func);

  /// Set the combobox help text
  void setHelpText(const FXString& txt);

  /// Get the combobox help text
  FXString getHelpText() const;

  /// Set the tool tip message for this combobox
  void setTipText(const FXString& txt);

  /// Get the tool tip message for this combobox
  FXString getTipText() const;

  /// Save combobox to a stream
  virtual void save(FXStream& store) const;

  /// Load combobox from a stream
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~FXComboBox();
  };

}

#endif

