/********************************************************************************
*                                                                               *
*                            T a b l e   W i d g e t                            *
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
* $Id: FXTable.h,v 1.130 2004/03/29 17:39:40 fox Exp $                          *
********************************************************************************/
#ifndef FXTABLE_H
#define FXTABLE_H

#ifndef FXSCROLLAREA_H
#include "FXScrollArea.h"
#endif
#include "FXDC.h"

namespace FX {


//////////////////////////////  UNDER DEVELOPMENT  //////////////////////////////

class FXIcon;
class FXFont;
class FXTable;
class FXHeader;
class FXButton;


/// Default cell margin
enum { DEFAULT_MARGIN = 2 };



/// Table options
enum {
  TABLE_COL_SIZABLE     = 0x00100000,   /// Columns are resizable
  TABLE_ROW_SIZABLE     = 0x00200000,   /// Rows are resizable
  TABLE_HEADERS_SIZABLE = 0x00400000,   /// Headers are sizable
  TABLE_NO_COLSELECT    = 0x00900000,   /// Disallow column selections
  TABLE_NO_ROWSELECT    = 0x01000000    /// Disallow row selections
  };


/// Position in table
struct FXTablePos {
  FXint  row;
  FXint  col;
  };


/// Range of table cells
struct FXTableRange {
  FXTablePos fm;
  FXTablePos to;
  };


/// Item in table
class FXAPI FXTableItem : public FXObject {
  FXDECLARE(FXTableItem)
  friend class FXTable;
protected:
  FXString    label;
  FXIcon     *icon;
  void       *data;
  FXuint      state;
protected:
  FXTableItem():icon(NULL),data(NULL),state(0){}
  FXint textWidth(const FXTable* table) const;
  FXint textHeight(const FXTable* table) const;
  virtual void draw(const FXTable* table,FXDC& dc,FXint x,FXint y,FXint w,FXint h) const;
  virtual void drawBorders(const FXTable* table,FXDC& dc,FXint x,FXint y,FXint w,FXint h) const;
  virtual void drawContent(const FXTable* table,FXDC& dc,FXint x,FXint y,FXint w,FXint h) const;
  virtual void drawPattern(const FXTable* table,FXDC& dc,FXint x,FXint y,FXint w,FXint h) const;
  virtual void drawBackground(const FXTable* table,FXDC& dc,FXint x,FXint y,FXint w,FXint h) const;
protected:
  enum{
    SELECTED   = 0x00000001,
    FOCUS      = 0x00000002,
    DISABLED   = 0x00000004,
    DRAGGABLE  = 0x00000008,
    RESERVED1  = 0x00000010,
    RESERVED2  = 0x00000020,
    ICONOWNED  = 0x00000040
    };
public:
  enum{
    RIGHT      = 0x00002000,      /// Align on right
    LEFT       = 0x00004000,      /// Align on left
    CENTER_X   = 0,               /// Aling centered horizontally (default)
    TOP        = 0x00008000,      /// Align on top
    BOTTOM     = 0x00010000,      /// Align on bottom
    CENTER_Y   = 0,               /// Aling centered vertically (default)
    BEFORE     = 0x00020000,      /// Icon before the text
    AFTER      = 0x00040000,      /// Icon after the text
    ABOVE      = 0x00080000,      /// Icon above the text
    BELOW      = 0x00100000,      /// Icon below the text
    LBORDER    = 0x00200000,      /// Draw left border
    RBORDER    = 0x00400000,      /// Draw right border
    TBORDER    = 0x00800000,      /// Draw top border
    BBORDER    = 0x01000000       /// Draw bottom border
    };
public:
  FXTableItem(const FXString& text,FXIcon* ic=NULL,void* ptr=NULL):label(text),icon(ic),data(ptr),state(FXTableItem::RIGHT){}
  virtual void setText(const FXString& txt){ label=txt; }
  const FXString& getText() const { return label; }
  virtual void setIcon(FXIcon* icn){ icon=icn; }
  FXIcon* getIcon() const { return icon; }
  void setData(void* ptr){ data=ptr; }
  void* getData() const { return data; }
  virtual void setFocus(FXbool focus);
  FXbool hasFocus() const { return (state&FOCUS)!=0; }
  virtual void setSelected(FXbool selected);
  FXbool isSelected() const { return (state&SELECTED)!=0; }
  virtual void setEnabled(FXbool enabled);
  FXbool isEnabled() const { return (state&DISABLED)==0; }
  virtual void setDraggable(FXbool draggable);
  FXbool isDraggable() const { return (state&DRAGGABLE)!=0; }
  void setJustify(FXuint justify);
  FXuint getJustify() const { return state&(RIGHT|LEFT|TOP|BOTTOM); }
  void setIconPosition(FXuint mode);
  FXuint getIconPosition() const { return state&(BEFORE|AFTER|ABOVE|BELOW); }
  void setBorders(FXuint borders);
  FXuint getBorders() const { return state&(LBORDER|RBORDER|TBORDER|BBORDER); }
  void setStipple(FXStipplePattern pattern);
  FXStipplePattern getStipple() const;
  virtual void setIconOwned(FXuint owned=ICONOWNED);
  FXuint isIconOwned() const { return (state&ICONOWNED); }
  virtual FXint getWidth(const FXTable* table) const;
  virtual FXint getHeight(const FXTable* table) const;
  virtual void create();
  virtual void detach();
  virtual void destroy();
  virtual void save(FXStream& store) const;
  virtual void load(FXStream& store);
  virtual ~FXTableItem();
  };



/// Table Widget
class FXAPI FXTable : public FXScrollArea {
  FXDECLARE(FXTable)
protected:
  FXHeader     *colHeader;              // Column header
  FXHeader     *rowHeader;              // Row header
  FXButton     *cornerButton;           // Corner button
  FXTableItem **cells;                  // Cells
  FXFont       *font;                   // Font
  FXint         nrows;                  // Number of rows
  FXint         ncols;                  // Number of columns
  FXint         visiblerows;            // Visible rows
  FXint         visiblecols;            // Visible columns
  FXint         margintop;              // Margin top
  FXint         marginbottom;           // Margin bottom
  FXint         marginleft;             // Margin left
  FXint         marginright;            // Margin right
  FXColor       textColor;              // Normal text color
  FXColor       baseColor;              // Base color
  FXColor       hiliteColor;            // Highlight color
  FXColor       shadowColor;            // Shadow color
  FXColor       borderColor;            // Border color
  FXColor       selbackColor;           // Select background color
  FXColor       seltextColor;           // Select text color
  FXColor       gridColor;              // Grid line color
  FXColor       stippleColor;           // Stipple color
  FXColor       cellBorderColor;        // Cell border color
  FXint         cellBorderWidth;        // Cell border width
  FXColor       cellBackColor[2][2];    // Row/Column even/odd background color
  FXint         defColWidth;            // Default column width [if uniform columns]
  FXint         defRowHeight;           // Default row height [if uniform rows]
  FXTablePos    current;                // Current position
  FXTablePos    anchor;                 // Anchor position
  FXTableRange  selection;              // Range of selected cells
  FXchar       *clipbuffer;             // Clipped text
  FXint         cliplength;             // Length of clipped text
  FXbool        hgrid;                  // Horizontal grid lines shown
  FXbool        vgrid;                  // Vertical grid lines shown
  FXuchar       mode;                   // Mode widget is in
  FXint         grabx;                  // Grab point x
  FXint         graby;                  // Grab point y
  FXint         rowcol;                 // Row or column being resized
  FXString      help;
public:
  static FXDragType csvType;
  static const FXchar *csvTypeName;
protected:
  FXTable();
  FXint startRow(FXint row,FXint col) const;
  FXint startCol(FXint row,FXint col) const;
  FXint endRow(FXint row,FXint col) const;
  FXint endCol(FXint row,FXint col) const;
  void spanningRange(FXint& sr,FXint& er,FXint& sc,FXint& ec,FXint anchrow,FXint anchcol,FXint currow,FXint curcol);
  virtual void moveContents(FXint x,FXint y);
  virtual void drawCell(FXDC& dc,FXint sr,FXint er,FXint sc,FXint ec);
  virtual void drawRange(FXDC& dc,FXint rlo,FXint rhi,FXint clo,FXint chi);
  virtual void drawHGrid(FXDC& dc,FXint rlo,FXint rhi,FXint clo,FXint chi);
  virtual void drawVGrid(FXDC& dc,FXint rlo,FXint rhi,FXint clo,FXint chi);
  virtual void drawContents(FXDC& dc,FXint x,FXint y,FXint w,FXint h);
  virtual FXTableItem* createItem(const FXString& text,FXIcon* icon,void* ptr);
  void countText(FXint& nr,FXint& nc,const FXchar* text,FXint size,FXchar cs='\t',FXchar rs='\n') const;
protected:
  enum {
    MOUSE_NONE,
    MOUSE_SCROLL,
    MOUSE_DRAG,
    MOUSE_SELECT,
    MOUSE_COL_SIZE,
    MOUSE_ROW_SIZE
    };
private:
  FXTable(const FXTable&);
  FXTable& operator=(const FXTable&);
public:
  long onPaint(FXObject*,FXSelector,void*);
  long onFocusIn(FXObject*,FXSelector,void*);
  long onFocusOut(FXObject*,FXSelector,void*);
  long onMotion(FXObject*,FXSelector,void*);
  long onKeyPress(FXObject*,FXSelector,void*);
  long onKeyRelease(FXObject*,FXSelector,void*);
  long onLeftBtnPress(FXObject*,FXSelector,void*);
  long onLeftBtnRelease(FXObject*,FXSelector,void*);
  long onRightBtnPress(FXObject*,FXSelector,void*);
  long onRightBtnRelease(FXObject*,FXSelector,void*);
  long onUngrabbed(FXObject*,FXSelector,void*);
  long onSelectionLost(FXObject*,FXSelector,void*);
  long onSelectionGained(FXObject*,FXSelector,void*);
  long onSelectionRequest(FXObject*,FXSelector,void* ptr);
  long onClipboardLost(FXObject*,FXSelector,void*);
  long onClipboardGained(FXObject*,FXSelector,void*);
  long onClipboardRequest(FXObject*,FXSelector,void*);
  long onAutoScroll(FXObject*,FXSelector,void*);
  long onCommand(FXObject*,FXSelector,void*);
  long onClicked(FXObject*,FXSelector,void*);
  long onDoubleClicked(FXObject*,FXSelector,void*);
  long onTripleClicked(FXObject*,FXSelector,void*);

  // Visual characteristics
  long onCmdHorzGrid(FXObject*,FXSelector,void*);
  long onUpdHorzGrid(FXObject*,FXSelector,void*);
  long onCmdVertGrid(FXObject*,FXSelector,void*);
  long onUpdVertGrid(FXObject*,FXSelector,void*);

  // Row/Column manipulations
  long onCmdDeleteColumn(FXObject*,FXSelector,void*);
  long onUpdDeleteColumn(FXObject*,FXSelector,void*);
  long onCmdDeleteRow(FXObject*,FXSelector,void*);
  long onUpdDeleteRow(FXObject*,FXSelector,void*);
  long onCmdInsertColumn(FXObject*,FXSelector,void*);
  long onCmdInsertRow(FXObject*,FXSelector,void*);

  // Movement
  long onCmdMoveRight(FXObject*,FXSelector,void*);
  long onCmdMoveLeft(FXObject*,FXSelector,void*);
  long onCmdMoveUp(FXObject*,FXSelector,void*);
  long onCmdMoveDown(FXObject*,FXSelector,void*);
  long onCmdMoveHome(FXObject*,FXSelector,void*);
  long onCmdMoveEnd(FXObject*,FXSelector,void*);
  long onCmdMoveTop(FXObject*,FXSelector,void*);
  long onCmdMoveBottom(FXObject*,FXSelector,void*);
  long onCmdMovePageDown(FXObject*,FXSelector,void*);
  long onCmdMovePageUp(FXObject*,FXSelector,void*);

  // Mark and extend
  long onCmdMark(FXObject*,FXSelector,void*);
  long onCmdExtend(FXObject*,FXSelector,void*);

  // Changing Selection
  long onCmdSelectCell(FXObject*,FXSelector,void*);
  long onCmdSelectRow(FXObject*,FXSelector,void*);
  long onCmdSelectColumn(FXObject*,FXSelector,void*);
  long onCmdSelectRowIndex(FXObject*,FXSelector,void*);
  long onCmdSelectColumnIndex(FXObject*,FXSelector,void*);
  long onCmdSelectAll(FXObject*,FXSelector,void*);
  long onCmdDeselectAll(FXObject*,FXSelector,void*);

  // Manipulation Selection
  long onCmdCutSel(FXObject*,FXSelector,void*);
  long onCmdCopySel(FXObject*,FXSelector,void*);
  long onCmdDeleteSel(FXObject*,FXSelector,void*);
  long onCmdPasteSel(FXObject*,FXSelector,void*);
  long onUpdHaveSelection(FXObject*,FXSelector,void*);
public:

  enum {
    ID_HORZ_GRID=FXScrollArea::ID_LAST,
    ID_VERT_GRID,
    ID_DELETE_COLUMN,
    ID_DELETE_ROW,
    ID_INSERT_COLUMN,
    ID_INSERT_ROW,
    ID_SELECT_COLUMN_INDEX,
    ID_SELECT_ROW_INDEX,
    ID_SELECT_COLUMN,
    ID_SELECT_ROW,
    ID_SELECT_CELL,
    ID_SELECT_ALL,
    ID_DESELECT_ALL,
    ID_MOVE_LEFT,
    ID_MOVE_RIGHT,
    ID_MOVE_UP,
    ID_MOVE_DOWN,
    ID_MOVE_HOME,
    ID_MOVE_END,
    ID_MOVE_TOP,
    ID_MOVE_BOTTOM,
    ID_MOVE_PAGEDOWN,
    ID_MOVE_PAGEUP,
    ID_MARK,
    ID_EXTEND,
    ID_CUT_SEL,
    ID_COPY_SEL,
    ID_PASTE_SEL,
    ID_DELETE_SEL,
    ID_LAST
    };

public:

  /**
  * Construct a new table.
  * The table is initially empty, and reports a default size based on
  * the scroll areas's scrollbar placement policy.
  */
  FXTable(FXComposite *p,FXObject* tgt=NULL,FXSelector sel=0,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_MARGIN,FXint pr=DEFAULT_MARGIN,FXint pt=DEFAULT_MARGIN,FXint pb=DEFAULT_MARGIN);

  /// Return default width
  virtual FXint getDefaultWidth();

  /// Return default height
  virtual FXint getDefaultHeight();

  /// Computes content width
  virtual FXint getContentWidth();

  /// Computes content height
  virtual FXint getContentHeight();

  /// Create the server-side resources
  virtual void create();

  /// Detach the server-side resources
  virtual void detach();

  /// Perform layout
  virtual void layout();

  /// Mark this window's layout as dirty
  virtual void recalc();

  /// Table widget can receive focus
  virtual FXbool canFocus() const;

  /// Move the focus to this window
  virtual void setFocus();

  /// Remove the focus from this window
  virtual void killFocus();

  /// Return column header control
  FXHeader* getColumnHeader() const { return colHeader; }

  /// Return row header control
  FXHeader* getRowHeader() const { return rowHeader; }

  /// Change visible rows/columns
  void setVisibleRows(FXint nvrows);
  FXint getVisibleRows() const { return visiblerows; }
  void setVisibleColumns(FXint nvcols);
  FXint getVisibleColumns() const { return visiblecols; }

  /// Show or hide horizontal grid
  void showHorzGrid(FXbool on=TRUE);

  /// Is horizontal grid shown
  FXbool isHorzGridShown() const { return hgrid; }

  /// Show or hide vertical grid
  void showVertGrid(FXbool on=TRUE);

  /// Is vertical grid shown
  FXbool isVertGridShown() const { return vgrid; }

  /// Get number of rows
  FXint getNumRows() const { return nrows; }

  /// Get number of columns
  FXint getNumColumns() const { return ncols; }

  /// Change top cell margin
  void setMarginTop(FXint pt);

  /// Return top cell margin
  FXint getMarginTop() const { return margintop; }

  /// Change bottom cell margin
  void setMarginBottom(FXint pb);

  /// Return bottom cell margin
  FXint getMarginBottom() const { return marginbottom; }

  /// Change left cell margin
  void setMarginLeft(FXint pl);

  /// Return left cell margin
  FXint getMarginLeft() const { return marginleft; }

  /// Change right cell margin
  void setMarginRight(FXint pr);

  /// Return right cell margin
  FXint getMarginRight() const { return marginright; }

  /**
  * Determine row containing y.
  * Returns -1 if y above first row, and nrows if y below last row;
  * otherwise, returns row in table containing y.
  */
  FXint rowAtY(FXint y) const;

  /**
  * Determine column containing x.
  * Returns -1 if x left of first column, and ncols if x right of last column;
  * otherwise, returns columns in table containing x.
  */
  FXint colAtX(FXint x) const;

  /// Return the item at the given index
  FXTableItem *getItem(FXint row,FXint col) const;

  /// Replace the item with a [possibly subclassed] item
  void setItem(FXint row,FXint col,FXTableItem* item,FXbool notify=FALSE);

  /// Set the table size to nr rows and nc columns; all existing items will be removed
  virtual void setTableSize(FXint nr,FXint nc,FXbool notify=FALSE);

  /// Insert new row
  virtual void insertRows(FXint row,FXint nr=1,FXbool notify=FALSE);

  /// Insert new column
  virtual void insertColumns(FXint col,FXint nc=1,FXbool notify=FALSE);

  /// Remove rows of cells
  virtual void removeRows(FXint row,FXint nr=1,FXbool notify=FALSE);

  /// Remove column of cells
  virtual void removeColumns(FXint col,FXint nc=1,FXbool notify=FALSE);

  /// Clear single cell
  virtual void removeItem(FXint row,FXint col,FXbool notify=FALSE);

  /// Clear all cells in the given range
  virtual void removeRange(FXint startrow,FXint endrow,FXint startcol,FXint endcol,FXbool notify=FALSE);

  /// Remove all items from table
  virtual void clearItems(FXbool notify=FALSE);

  /// Scroll to make cell at r,c fully visible
  void makePositionVisible(FXint r,FXint c);

  // Return TRUE if item partially visible
  FXbool isItemVisible(FXint r,FXint c) const;

  /**
  * Change column header height mode to fixed or variable.
  * In variable height mode, the column header will size to
  * fit the contents in it.  In fixed mode, the size is
  * explicitly set using setColumnHeaderHeight().
  */
  void setColumnHeaderMode(FXuint hint=LAYOUT_FIX_HEIGHT);

  /// Return column header height mode
  FXuint getColumnHeaderMode() const;

  /**
  * Change row header width mode to fixed or variable.
  * In variable width mode, the row header will size to
  * fit the contents in it.  In fixed mode, the size is
  * explicitly set using setRowHeaderWidth().
  */
  void setRowHeaderMode(FXuint hint=LAYOUT_FIX_WIDTH);

  /// Return row header width mode
  FXuint getRowHeaderMode() const;

  /// Change column header height
  void setColumnHeaderHeight(FXint h);

  /// Return column header height
  FXint getColumnHeaderHeight() const;

  /// Change row header width
  void setRowHeaderWidth(FXint w);

  /// Return row header width
  FXint getRowHeaderWidth() const;

  /// Change column width
  virtual void setColumnWidth(FXint col,FXint cwidth);
  FXint getColumnWidth(FXint col) const;

  /// Change row height
  virtual void setRowHeight(FXint row,FXint rheight);
  FXint getRowHeight(FXint row) const;

  /// Change X coordinate of column c
  virtual void setColumnX(FXint col,FXint x);
  FXint getColumnX(FXint col) const;

  /// Change Y coordinate of row r
  virtual void setRowY(FXint row,FXint y);
  FXint getRowY(FXint row) const;

  /// Change default column width
  void setDefColumnWidth(FXint cwidth);
  FXint getDefColumnWidth() const { return defColWidth; }

  /// Change default row height
  void setDefRowHeight(FXint rheight);
  FXint getDefRowHeight() const { return defRowHeight; }

  /// Return minimum row height
  FXint getMinRowHeight(FXint r) const;

  /// Return minimum column width
  FXint getMinColumnWidth(FXint c) const;

  /// Change column header
  void setColumnText(FXint index,const FXString& text);

  /// Return text of column header at index
  FXString getColumnText(FXint index) const;

  /// Change row header
  void setRowText(FXint index,const FXString& text);

  /// Return text of row header at index
  FXString getRowText(FXint index) const;

  /// Modify cell text
  void setItemText(FXint r,FXint c,const FXString& text);
  FXString getItemText(FXint r,FXint c) const;

  /// Modify cell icon
  void setItemIcon(FXint r,FXint c,FXIcon* icon);
  FXIcon* getItemIcon(FXint r,FXint c) const;

  /// Modify cell user-data
  void setItemData(FXint r,FXint c,void* ptr);
  void* getItemData(FXint r,FXint c) const;

  /// Extract cells from given range as text.
  void extractText(FXchar*& text,FXint& size,FXint startrow,FXint endrow,FXint startcol,FXint endcol,FXchar cs='\t',FXchar rs='\n') const;

  /// Overlay text over given cell range
  void overlayText(FXint startrow,FXint endrow,FXint startcol,FXint endcol,const FXchar* text,FXint size,FXchar cs='\t',FXchar rs='\n');

  /// Return TRUE if its a spanning cell
  FXbool isItemSpanning(FXint r,FXint c) const;

  /// Repaint cells between grid lines sr,er and grid lines sc,ec
  void updateRange(FXint sr,FXint er,FXint sc,FXint ec) const;

  /// Repaint cell at r,c
  void updateItem(FXint r,FXint c) const;

  /// Enable item
  FXbool enableItem(FXint r,FXint c);

  /// Disable item
  FXbool disableItem(FXint r,FXint c);

  // Is item enabled
  FXbool isItemEnabled(FXint r,FXint c) const;

  /// Change item justification
  void setItemJustify(FXint r,FXint c,FXuint justify);

  /// Return item justification
  FXuint getItemJustify(FXint r,FXint c) const;

  /// Change relative position of icon and text of item
  void setItemIconPosition(FXint r,FXint c,FXuint mode);

  /// Return relative icon and text position
  FXuint getItemIconPosition(FXint r,FXint c) const;

  /// Change item border style
  void setItemBorders(FXint r,FXint c,FXuint borders);

  /// Return item border style
  FXuint getItemBorders(FXint r,FXint c) const;

  /// Change item background stipple style
  void setItemStipple(FXint r,FXint c,FXStipplePattern pat);

  /// return item background stipple style
  FXStipplePattern getItemStipple(FXint r,FXint c) const;

  /// Change current item
  virtual void setCurrentItem(FXint r,FXint c,FXbool notify=FALSE);

  /// Get row number of current item
  FXint getCurrentRow() const { return current.row; }

  /// Get column number of current item
  FXint getCurrentColumn() const { return current.col; }

  // Is item current
  FXbool isItemCurrent(FXint r,FXint c) const;

  /// Change anchor item
  void setAnchorItem(FXint r,FXint c);

  /// Get row number of anchor item
  FXint getAnchorRow() const { return anchor.row; }

  /// Get column number of anchor item
  FXint getAnchorColumn() const { return anchor.col; }

  /// Get selection start row; returns -1 if no selection
  FXint getSelStartRow() const { return selection.fm.row; }

  /// Get selection start column; returns -1 if no selection
  FXint getSelStartColumn() const { return selection.fm.col; }

  /// Get selection end row; returns -1 if no selection
  FXint getSelEndRow() const { return selection.to.row; }

  /// Get selection end column; returns -1 if no selection
  FXint getSelEndColumn() const { return selection.to.col; }

  /// Is cell selected
  FXbool isItemSelected(FXint r,FXint c) const;

  /// Is row of cells selected
  FXbool isRowSelected(FXint r) const;

  /// Is column selected
  FXbool isColumnSelected(FXint c) const;

  /// Is anything selected
  FXbool isAnythingSelected() const;

  /// Select a row
  virtual FXbool selectRow(FXint row,FXbool notify=FALSE);

  /// Select a column
  virtual FXbool selectColumn(FXint col,FXbool notify=FALSE);

  /// Select range
  virtual FXbool selectRange(FXint startrow,FXint endrow,FXint startcol,FXint endcol,FXbool notify=FALSE);

  /// Extend selection
  virtual FXbool extendSelection(FXint r,FXint c,FXbool notify=FALSE);

  /// Kill selection
  virtual FXbool killSelection(FXbool notify=FALSE);

  /// Change font
  void setFont(FXFont* fnt);

  /// Return current font
  FXFont* getFont() const { return font; }

  /// Obtain colors of various parts
  FXColor getTextColor() const { return textColor; }
  FXColor getBaseColor() const { return baseColor; }
  FXColor getHiliteColor() const { return hiliteColor; }
  FXColor getShadowColor() const { return shadowColor; }
  FXColor getBorderColor() const { return borderColor; }
  FXColor getSelBackColor() const { return selbackColor; }
  FXColor getSelTextColor() const { return seltextColor; }
  FXColor getGridColor() const { return gridColor; }
  FXColor getStippleColor() const { return stippleColor; }
  FXColor getCellBorderColor() const { return cellBorderColor; }

  /// Change colors of various parts
  void setTextColor(FXColor clr);
  void setBaseColor(FXColor clr);
  void setHiliteColor(FXColor clr);
  void setShadowColor(FXColor clr);
  void setBorderColor(FXColor clr);
  void setSelBackColor(FXColor clr);
  void setSelTextColor(FXColor clr);
  void setGridColor(FXColor clr);
  void setStippleColor(FXColor clr);
  void setCellBorderColor(FXColor clr);

  /// Change cell background color for even/odd rows/columns
  void setCellColor(FXint r,FXint c,FXColor clr);

  /// Obtain cell background color for even/odd rows/columns
  FXColor getCellColor(FXint r,FXint c) const;

  /// Change cell border width
  void setCellBorderWidth(FXint borderwidth);

  /// Return cell border width
  FXint getCellBorderWidth() const { return cellBorderWidth; }

  /// Change table style
  void setTableStyle(FXuint style);

  /// Return table style
  FXuint getTableStyle() const;

  /// Change help text
  void setHelpText(const FXString& text){ help=text; }
  FXString getHelpText() const { return help; }

  /// Serialize
  virtual void save(FXStream& store) const;
  virtual void load(FXStream& store);

  virtual ~FXTable();
  };

}

#endif
