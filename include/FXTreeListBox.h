/********************************************************************************
*                                                                               *
*                      T r e e   L i s t   B o x   W i d g e t                  *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXTreeListBox.h,v 1.27 2004/02/08 17:17:34 fox Exp $                     *
********************************************************************************/
#ifndef FXTREELISTBOX_H
#define FXTREELISTBOX_H

#ifndef FXPACKER_H
#include "FXPacker.h"
#endif
#include "FXTreeList.h"
#include "FXFrame.h"

namespace FX {


/// Tree List Box styles
enum {
  TREELISTBOX_NORMAL         = 0          /// Normal style
  };


class FXButton;
class FXMenuButton;
class FXTreeList;
class FXPopup;
class FXTreeItem;


/// Tree List Box
class FXAPI FXTreeListBox : public FXPacker {
  FXDECLARE(FXTreeListBox)
protected:
  FXButton      *field;
  FXMenuButton  *button;
  FXTreeList    *tree;
  FXPopup       *pane;
protected:
  FXTreeListBox(){}
private:
  FXTreeListBox(const FXTreeListBox&);
  FXTreeListBox& operator=(const FXTreeListBox&);
public:
  long onFocusUp(FXObject*,FXSelector,void*);
  long onFocusDown(FXObject*,FXSelector,void*);
  long onFocusSelf(FXObject*,FXSelector,void*);
  long onChanged(FXObject*,FXSelector,void*);
  long onCommand(FXObject*,FXSelector,void*);
  long onFieldButton(FXObject*,FXSelector,void*);
  long onTreeChanged(FXObject*,FXSelector,void*);
  long onTreeClicked(FXObject*,FXSelector,void*);
  long onUpdFmTree(FXObject*,FXSelector,void*);
public:
  enum{
    ID_TREE=FXPacker::ID_LAST,
    ID_FIELD,
    ID_LAST
    };
public:

  /// Construct tree list box
  FXTreeListBox(FXComposite *p,FXObject* tgt=NULL,FXSelector sel=0,FXuint opts=FRAME_SUNKEN|FRAME_THICK|TREELISTBOX_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);

  /// Create server-side resources
  virtual void create();

  /// Detach server-side resources
  virtual void detach();

  /// Destroy server-side resources
  virtual void destroy();

  /// Perform layout
  virtual void layout();

  /// Enable widget
  virtual void enable();

  /// Disable widget
  virtual void disable();

  /// Return default with
  virtual FXint getDefaultWidth();

  /// Return default height
  virtual FXint getDefaultHeight();

  /// Return number of items
  FXint getNumItems() const;

  /// Return number of visible items
  FXint getNumVisible() const;

  /// Set number of visible items to determine default height
  void setNumVisible(FXint nvis);

  /// Return first top-level item
  FXTreeItem* getFirstItem() const;

  /// Return last top-level item
  FXTreeItem* getLastItem() const;

  /// Add item as first child of parent p
  FXTreeItem* addItemFirst(FXTreeItem* p,FXTreeItem* item);

  /// Add item as last child after parent p
  FXTreeItem* addItemLast(FXTreeItem* p,FXTreeItem* item);

  /// Add item after other item
  FXTreeItem* addItemAfter(FXTreeItem* other,FXTreeItem* item);

  /// Add item before other item
  FXTreeItem* addItemBefore(FXTreeItem* other,FXTreeItem* item);

  /// Add item as first child of parent p
  FXTreeItem* addItemFirst(FXTreeItem* p,const FXString& text,FXIcon* oi=NULL,FXIcon* ci=NULL,void* ptr=NULL);

  /// Add item as last child of parent p
  FXTreeItem* addItemLast(FXTreeItem* p,const FXString& text,FXIcon* oi=NULL,FXIcon* ci=NULL,void* ptr=NULL);

  /// Add item after other item
  FXTreeItem* addItemAfter(FXTreeItem* other,const FXString& text,FXIcon* oi=NULL,FXIcon* ci=NULL,void* ptr=NULL);

  /// Add item before other item
  FXTreeItem* addItemBefore(FXTreeItem* other,const FXString& text,FXIcon* oi=NULL,FXIcon* ci=NULL,void* ptr=NULL);

  /// Remove item
  void removeItem(FXTreeItem* item);

  /// Remove all items in range [fm...to]
  void removeItems(FXTreeItem* fm,FXTreeItem* to);

  /// Remove all items from list
  void clearItems();

  /**
  * Search items for item by name, starting from start item; the
  * flags argument controls the search direction, and case sensitivity.
  */
  FXTreeItem* findItem(const FXString& text,FXTreeItem* start=NULL,FXuint flags=SEARCH_FORWARD|SEARCH_WRAP) const;

  /// Return TRUE if item is the current item
  FXbool isItemCurrent(const FXTreeItem* item) const;

  /// Return TRUE if item is leaf-item, i.e. has no children
  FXbool isItemLeaf(const FXTreeItem* item) const;

  /// Sort the toplevel items with the sort function
  void sortRootItems();

  /// Sort all items recursively
  void sortItems();

  /// Sort child items of item
  void sortChildItems(FXTreeItem* item);

  /// Change current item
  void setCurrentItem(FXTreeItem* item,FXbool notify=FALSE);

  /// Return current item
  FXTreeItem* getCurrentItem() const;

  /// Change item label
  void setItemText(FXTreeItem* item,const FXString& text);

  /// Return item label
  FXString getItemText(const FXTreeItem* item) const;

  /// Change item's open icon
  void setItemOpenIcon(FXTreeItem* item,FXIcon* icon);

  /// Return item's open icon
  FXIcon* getItemOpenIcon(const FXTreeItem* item) const;

  /// Change item's closed icon
  void setItemClosedIcon(FXTreeItem* item,FXIcon* icon);

  /// Return item's closed icon
  FXIcon* getItemClosedIcon(const FXTreeItem* item) const;

  /// Change item's user data
  void setItemData(FXTreeItem* item,void* ptr) const;

  /// Return item's user data
  void* getItemData(const FXTreeItem* item) const;

  /// Return item sort function
  FXTreeListSortFunc getSortFunc() const;

  /// Change item sort function
  void setSortFunc(FXTreeListSortFunc func);

  /// Is the pane shown
  FXbool isPaneShown() const;

  /// Change font
  void setFont(FXFont* fnt);

  /// Return font
  FXFont* getFont() const;

  /// Return list style
  FXuint getListStyle() const;

  /// Change list style
  void setListStyle(FXuint style);

  /// Change help text
  void setHelpText(const FXString& txt);

  /// Return help text
  FXString getHelpText() const;

  /// Change tip text
  void setTipText(const FXString& txt);

  /// Return tip text
  FXString getTipText() const;

  /// Save object to a stream
  virtual void save(FXStream& store) const;

  /// Load object from a stream
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~FXTreeListBox();
  };

}

#endif

