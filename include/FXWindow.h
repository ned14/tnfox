/********************************************************************************
*                                                                               *
*                            W i n d o w   O b j e c t                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXWindow.h,v 1.130 2005/02/02 16:37:40 fox Exp $                         *
********************************************************************************/
#ifndef FXWINDOW_H
#define FXWINDOW_H

#include "FXString.h"
#ifndef FXDRAWABLE_H
#include "FXDrawable.h"
#endif
#include "FXProcess.h"


namespace FX {


/// Layout hints for child widgets
enum {
  LAYOUT_NORMAL      = 0,                                   /// Default layout mode
  LAYOUT_SIDE_TOP    = 0,                                   /// Pack on top side (default)
  LAYOUT_SIDE_BOTTOM = 0x00000001,                          /// Pack on bottom side
  LAYOUT_SIDE_LEFT   = 0x00000002,                          /// Pack on left side
  LAYOUT_SIDE_RIGHT  = LAYOUT_SIDE_LEFT|LAYOUT_SIDE_BOTTOM, /// Pack on right side
  LAYOUT_FILL_COLUMN = 0x00000001,                          /// Matrix column is stretchable
  LAYOUT_FILL_ROW    = 0x00000002,                          /// Matrix row is stretchable
  LAYOUT_LEFT        = 0,                                   /// Stick on left (default)
  LAYOUT_RIGHT       = 0x00000004,                          /// Stick on right
  LAYOUT_CENTER_X    = 0x00000008,                          /// Center horizontally
  LAYOUT_FIX_X       = LAYOUT_RIGHT|LAYOUT_CENTER_X,        /// X fixed
  LAYOUT_TOP         = 0,                                   /// Stick on top (default)
  LAYOUT_BOTTOM      = 0x00000010,                          /// Stick on bottom
  LAYOUT_CENTER_Y    = 0x00000020,                          /// Center vertically
  LAYOUT_FIX_Y       = LAYOUT_BOTTOM|LAYOUT_CENTER_Y,       /// Y fixed
  LAYOUT_DOCK_SAME   = 0,                                   /// Dock on same galley if it fits
  LAYOUT_DOCK_NEXT   = 0x00000040,                          /// Dock on next galley
  LAYOUT_RESERVED_1  = 0x00000080,
  LAYOUT_FIX_WIDTH   = 0x00000100,                          /// Width fixed
  LAYOUT_FIX_HEIGHT  = 0x00000200,                          /// height fixed
  LAYOUT_MIN_WIDTH   = 0,                                   /// Minimum width is the default
  LAYOUT_MIN_HEIGHT  = 0,                                   /// Minimum height is the default
  LAYOUT_FILL_X      = 0x00000400,                          /// Stretch or shrink horizontally
  LAYOUT_FILL_Y      = 0x00000800,                          /// Stretch or shrink vertically
  LAYOUT_FILL        = LAYOUT_FILL_X|LAYOUT_FILL_Y,         /// Stretch or shrink in both directions
  LAYOUT_EXPLICIT    = LAYOUT_FIX_X|LAYOUT_FIX_Y|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT   /// Explicit placement
  };


/// Frame border appearance styles (for subclasses)
enum {
  FRAME_NONE   = 0,                                     /// Default is no frame
  FRAME_SUNKEN = 0x00001000,                            /// Sunken border
  FRAME_RAISED = 0x00002000,                            /// Raised border
  FRAME_THICK  = 0x00004000,                            /// Thick border
  FRAME_GROOVE = FRAME_THICK,                           /// A groove or etched-in border
  FRAME_RIDGE  = FRAME_THICK|FRAME_RAISED|FRAME_SUNKEN, /// A ridge or embossed border
  FRAME_LINE   = FRAME_RAISED|FRAME_SUNKEN,             /// Simple line border
  FRAME_NORMAL = FRAME_SUNKEN|FRAME_THICK               /// Regular raised/thick border
  };


/// Packing style (for packers)
enum {
  PACK_NORMAL         = 0,              /// Default is each its own size
  PACK_UNIFORM_HEIGHT = 0x00008000,     /// Uniform height
  PACK_UNIFORM_WIDTH  = 0x00010000      /// Uniform width
  };


class FXIcon;
class FXBitmap;
class FXCursor;
class FXRegion;
class FXComposite;
class FXAccelTable;


/// Base class for all windows
class FXAPI FXWindow : public FXDrawable {
  FXDECLARE(FXWindow)
private:
  FXWindow     *parent;                 // Parent Window
  FXWindow     *owner;                  // Owner Window
  FXWindow     *first;                  // First Child
  FXWindow     *last;                   // Last Child
  FXWindow     *next;                   // Next Sibling
  FXWindow     *prev;                   // Previous Sibling
  FXWindow     *focus;                  // Focus Child
  FXuint        wk;                     // Window Key
protected:
  FXCursor     *defaultCursor;          // Normal Cursor
  FXCursor     *savedCursor;            // Saved Cursor
  FXCursor     *dragCursor;             // Cursor during drag
  FXAccelTable *accelTable;             // Accelerator table
  FXObject     *target;                 // Target object
  FXSelector    message;                // Message ID
  FXint         xpos;                   // Window X Position
  FXint         ypos;                   // Window Y Position
  FXColor       backColor;              // Window background color
  FXString      tag;                    // Help tag
  FXuint        flags;                  // Window state flags
  FXuint        options;                // Window options
private:
  static FXint  windowCount;            // Number of windows
public:
  static FXDragType deleteType;         // Delete request
  static FXDragType textType;           // Ascii text request
  static FXDragType colorType;          // Color
  static FXDragType urilistType;        // URI List
  static const FXDragType stringType;   // Clipboard text type (pre-registered)
  static const FXDragType imageType;    // Clipboard image type (pre-registered)

  /// Returns LAYOUT_RIGHT or LAYOUT_LEFT suitable for the current user's handedness
  static inline FXuint userHandednessLayout()
  {
	  return FXProcess::userHandedness()==FXProcess::RIGHT_HANDED ?
		LAYOUT_RIGHT : LAYOUT_LEFT;
  }
  /// Returns the default padding quantity
  static inline FXuint defaultPadding() { return FXProcess::screenScale(2); }
  /// Returns the default child spacing quantity
  static inline FXuint defaultSpacing() { return FXProcess::screenScale(4); }
  /// Scales a layout value by a factor
  static inline FXuint scaleLayoutValue(FXuint value) { return FXProcess::screenScale(value); }

protected:
  FXWindow();
  FXWindow(FXApp* a,FXVisual *vis);
  FXWindow(FXApp* a,FXWindow* own,FXuint opts,FXint x,FXint y,FXint w,FXint h);
  static FXWindow* findDefault(FXWindow* window);
  static FXWindow* findInitial(FXWindow* window);
  virtual FXbool doesOverrideRedirect() const;
protected:
#ifdef WIN32
  virtual FXID GetDC() const;
  virtual int ReleaseDC(FXID) const;
  virtual const char* GetClass() const;
#else
  void addColormapWindows();
  void remColormapWindows();
#endif
private:
  FXWindow(const FXWindow&);
  FXWindow& operator=(const FXWindow&);
protected:

  // Window state flags
  enum {
    FLAG_SHOWN        = 0x00000001,     // Is shown
    FLAG_ENABLED      = 0x00000002,     // Able to receive input
    FLAG_UPDATE       = 0x00000004,     // Is subject to GUI update
    FLAG_DROPTARGET   = 0x00000008,     // Drop target
    FLAG_FOCUSED      = 0x00000010,     // Has focus
    FLAG_DIRTY        = 0x00000020,     // Needs layout
    FLAG_RECALC       = 0x00000040,     // Needs recalculation
    FLAG_TIP          = 0x00000080,     // Show tip
    FLAG_HELP         = 0x00000100,     // Show help
    FLAG_DEFAULT      = 0x00000200,     // Default widget
    FLAG_INITIAL      = 0x00000400,     // Initial widget
    FLAG_SHELL        = 0x00000800,     // Shell window
    FLAG_ACTIVE       = 0x00001000,     // Window is active
    FLAG_PRESSED      = 0x00002000,     // Button has been pressed
    FLAG_KEY          = 0x00004000,     // Keyboard key pressed
    FLAG_CARET        = 0x00008000,     // Caret is on
    FLAG_CHANGED      = 0x00010000,     // Window data changed
    FLAG_LASSO        = 0x00020000,     // Lasso mode
    FLAG_TRYDRAG      = 0x00040000,     // Tentative drag mode
    FLAG_DODRAG       = 0x00080000,     // Doing drag mode
    FLAG_SCROLLINSIDE = 0x00100000,     // Scroll only when inside
    FLAG_SCROLLING    = 0x00200000,     // Right mouse scrolling
    FLAG_OWNED        = 0x00400000,     // Window handle owned by widget
    FLAG_POPUP        = 0x00800000
    };

public:

  // Message handlers
  long onPaint(FXObject*,FXSelector,void*);
  long onMap(FXObject*,FXSelector,void*);
  long onUnmap(FXObject*,FXSelector,void*);
  long onConfigure(FXObject*,FXSelector,void*);
  long onUpdate(FXObject*,FXSelector,void*);
  long onMotion(FXObject*,FXSelector,void*);
  long onMouseWheel(FXObject*,FXSelector,void*);
  long onEnter(FXObject*,FXSelector,void*);
  long onLeave(FXObject*,FXSelector,void*);
  long onLeftBtnPress(FXObject*,FXSelector,void*);
  long onLeftBtnRelease(FXObject*,FXSelector,void*);
  long onMiddleBtnPress(FXObject*,FXSelector,void*);
  long onMiddleBtnRelease(FXObject*,FXSelector,void*);
  long onRightBtnPress(FXObject*,FXSelector,void*);
  long onRightBtnRelease(FXObject*,FXSelector,void*);
  long onBeginDrag(FXObject*,FXSelector,void*);
  long onEndDrag(FXObject*,FXSelector,void*);
  long onDragged(FXObject*,FXSelector,void*);
  long onKeyPress(FXObject*,FXSelector,void*);
  long onKeyRelease(FXObject*,FXSelector,void*);
  long onUngrabbed(FXObject*,FXSelector,void*);
  long onDestroy(FXObject*,FXSelector,void*);
  long onFocusSelf(FXObject*,FXSelector,void*);
  long onFocusIn(FXObject*,FXSelector,void*);
  long onFocusOut(FXObject*,FXSelector,void*);
  long onSelectionLost(FXObject*,FXSelector,void*);
  long onSelectionGained(FXObject*,FXSelector,void*);
  long onSelectionRequest(FXObject*,FXSelector,void*);
  long onClipboardLost(FXObject*,FXSelector,void*);
  long onClipboardGained(FXObject*,FXSelector,void*);
  long onClipboardRequest(FXObject*,FXSelector,void*);
  long onDNDEnter(FXObject*,FXSelector,void*);
  long onDNDLeave(FXObject*,FXSelector,void*);
  long onDNDMotion(FXObject*,FXSelector,void*);
  long onDNDDrop(FXObject*,FXSelector,void*);
  long onDNDRequest(FXObject*,FXSelector,void*);
  long onQueryHelp(FXObject*,FXSelector,void*);
  long onQueryTip(FXObject*,FXSelector,void*);
  long onCmdShow(FXObject*,FXSelector,void*);
  long onCmdHide(FXObject*,FXSelector,void*);
  long onUpdToggleShown(FXObject*,FXSelector,void*);
  long onCmdToggleShown(FXObject*,FXSelector,void*);
  long onCmdRaise(FXObject*,FXSelector,void*);
  long onCmdLower(FXObject*,FXSelector,void*);
  long onCmdEnable(FXObject*,FXSelector,void*);
  long onCmdDisable(FXObject*,FXSelector,void*);
  long onUpdToggleEnabled(FXObject*,FXSelector,void*);
  long onCmdToggleEnabled(FXObject*,FXSelector,void*);
  long onCmdUpdate(FXObject*,FXSelector,void*);
  long onUpdYes(FXObject*,FXSelector,void*);
  long onCmdDelete(FXObject*,FXSelector,void*);

public:

  // Message ID's common to most Windows
  enum {
    ID_NONE,
    ID_HIDE,            // ID_HIDE+FALSE
    ID_SHOW,            // ID_HIDE+TRUE
    ID_TOGGLESHOWN,
    ID_LOWER,
    ID_RAISE,
    ID_DELETE,
    ID_DISABLE,         // ID_DISABLE+FALSE
    ID_ENABLE,          // ID_DISABLE+TRUE
    ID_TOGGLEENABLED,
    ID_UNCHECK,         // ID_UNCHECK+FALSE
    ID_CHECK,           // ID_UNCHECK+TRUE
    ID_UNKNOWN,         // ID_UNCHECK+MAYBE
    ID_UPDATE,
    ID_AUTOSCROLL,
    ID_TIPTIMER,
    ID_HSCROLLED,
    ID_VSCROLLED,
    ID_SETVALUE,
    ID_SETINTVALUE,
    ID_SETREALVALUE,
    ID_SETSTRINGVALUE,
    ID_SETICONVALUE,
    ID_SETINTRANGE,
    ID_SETREALRANGE,
    ID_GETINTVALUE,
    ID_GETREALVALUE,
    ID_GETSTRINGVALUE,
    ID_GETICONVALUE,
    ID_GETINTRANGE,
    ID_GETREALRANGE,
    ID_SETHELPSTRING,
    ID_GETHELPSTRING,
    ID_SETTIPSTRING,
    ID_GETTIPSTRING,
    ID_QUERY_MENU,
    ID_HOTKEY,
    ID_ACCEL,
    ID_UNPOST,
    ID_POST,
    ID_MDI_TILEHORIZONTAL,
    ID_MDI_TILEVERTICAL,
    ID_MDI_CASCADE,
    ID_MDI_MAXIMIZE,
    ID_MDI_MINIMIZE,
    ID_MDI_RESTORE,
    ID_MDI_CLOSE,
    ID_MDI_WINDOW,
    ID_MDI_MENUWINDOW,
    ID_MDI_MENUMINIMIZE,
    ID_MDI_MENURESTORE,
    ID_MDI_MENUCLOSE,
    ID_MDI_NEXT,
    ID_MDI_PREV,
    ID_LAST
    };

public:

  // Common DND type names
  static const FXchar *deleteTypeName;
  static const FXchar *textTypeName;
  static const FXchar *colorTypeName;
  static const FXchar *urilistTypeName;

public:

  /// Constructor
  FXWindow(FXComposite* p,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0);

  /// Return a pointer to the parent window
  FXWindow* getParent() const { return parent; }

  /// Return a pointer to the owner window
  FXWindow* getOwner() const { return owner; }

  /// Return a pointer to the shell window
  FXWindow* getShell() const;

  /// Return a pointer to the root window
  FXWindow* getRoot() const;

  /// Return a pointer to the next (sibling) window, if any
  FXWindow* getNext() const { return next; }

  /// Return a pointer to the previous (sibling) window , if any
  FXWindow* getPrev() const { return prev; }

  /// Return a pointer to this window's first child window , if any
  FXWindow* getFirst() const { return first; }

  /// Return a pointer to this window's last child window, if any
  FXWindow* getLast() const { return last; }

  /// Return a pointer to the currently focused child window
  FXWindow* getFocus() const { return focus; }

  /// Change window key
  void setKey(FXuint k){ wk=k; }

  /// Return window key
  FXuint getKey() const { return wk; }

  /// Set the message target object for this window
  void setTarget(FXObject *t){ target=t; }

  /// Get the message target object for this window, if any
  FXObject* getTarget() const { return target; }

  /// Set the message identifier for this window
  void setSelector(FXSelector sel){ message=sel; }

  /// Get the message identifier for this window
  FXSelector getSelector() const { return message; }

  /// Get this window's x-coordinate, in the parent's coordinate system
  FXint getX() const { return xpos; }

  /// Get this window's y-coordinate, in the parent's coordinate system
  FXint getY() const { return ypos; }

  /// Return the default width of this window
  virtual FXint getDefaultWidth();

  /// Return the default height of this window
  virtual FXint getDefaultHeight();

  /// Return width for given height
  virtual FXint getWidthForHeight(FXint givenheight);

  /// Return height for given width
  virtual FXint getHeightForWidth(FXint givenwidth);

  /// Set this window's x-coordinate, in the parent's coordinate system
  void setX(FXint x);

  /// Set this window's y-coordinate, in the parent's coordinate system
  void setY(FXint y);

  /// Set the window width
  void setWidth(FXint w);

  /// Set the window height
  void setHeight(FXint h);

  /// Set layout hints for this window
  void setLayoutHints(FXuint lout);

  /// Get layout hints for this window
  FXuint getLayoutHints() const;

  /// Return a pointer to the accelerator table
  FXAccelTable* getAccelTable() const { return accelTable; }

  /// Set the accelerator table
  void setAccelTable(FXAccelTable* acceltable){ accelTable=acceltable; }

  /// Add a hot key
  void addHotKey(FXHotKey code);

  /// Remove a hot key
  void remHotKey(FXHotKey code);

  /// Change help tag for this widget
  void setHelpTag(const FXString&  text){ tag=text; }

  /// Get the help tag for this widget
  const FXString& getHelpTag() const { return tag; }

  /// Return true if window is a shell window
  FXbool isShell() const;

  /// Return true if window is a popup window
  FXbool isPopup() const;

  /// Return true if specified window is owned by this window
  FXbool isOwnerOf(const FXWindow* window) const;

  /// Return true if specified window is ancestor of this window
  FXbool isChildOf(const FXWindow* window) const;

  /// Return true if this window contains child in its subtree
  FXbool containsChild(const FXWindow* child) const;

  /// Return the child window at specified coordinates
  FXWindow* getChildAt(FXint x,FXint y) const;

  /// Return the number of child windows for this window
  FXint numChildren() const;

  /**
  * Return the index (starting from zero) of the specified child window,
  * or -1 if the window is not a child or NULL
  */
  FXint indexOfChild(const FXWindow *window) const;

  /**
  * Return the child window at specified index,
  * or NULL if the index is negative or out of range
  */
  FXWindow* childAtIndex(FXint index) const;

  /// Return the common ancestor of window a and window b
  static FXWindow* commonAncestor(FXWindow* a,FXWindow* b);

  /// Return TRUE if sibling a <= sibling b in list
  static FXbool before(const FXWindow *a,const FXWindow* b);

  /// Return TRUE if sibling a >= sibling b in list
  static FXbool after(const FXWindow *a,const FXWindow* b);

  /// Get number of existing windows
  static FXint getWindowCount(){ return windowCount; }

  /// Set the default cursor for this window
  void setDefaultCursor(FXCursor* cur);

  /// Return the default cursor for this window
  FXCursor* getDefaultCursor() const { return defaultCursor; }

  /// Set the saved cursor for this window
  void setSavedCursor(FXCursor* cur) { savedCursor=cur; }

  /// Return the saved cursor for this window
  FXCursor* getSavedCursor() const { return savedCursor; }

  /// Set the drag cursor for this window
  void setDragCursor(FXCursor* cur);

  /// Return the drag cursor for this window
  FXCursor* getDragCursor() const { return dragCursor; }

  /// Return the cursor position and mouse button-state
  FXint getCursorPosition(FXint& x,FXint& y,FXuint& buttons) const;

  /// Warp the cursor to the new position
  FXint setCursorPosition(FXint x,FXint y);

  /// Return true if this window is able to receive mouse and keyboard events
  FXbool isEnabled() const;

  /// Return true if the window is active
  FXbool isActive() const;

  /// Return true if this window is a control capable of receiving the focus
  virtual FXbool canFocus() const;

  /// Return true if this window has the focus
  FXbool hasFocus() const;
  
  /// Return true if this window is in focus chain
  FXbool inFocusChain() const;

  /// Move the focus to this window
  virtual void setFocus();

  /// Remove the focus from this window
  virtual void killFocus();

  /// Notification that focus moved to new child
  virtual void changeFocus(FXWindow *child);

  /**
  * This changes the default window which responds to the Return
  * key in a dialog. If enable is TRUE, this window becomes the default
  * window; when enable is FALSE, this window will be no longer the
  * default window.  Finally, when enable is MAYBE, the default window
  * will revert to the initial default window.
  */
  virtual void setDefault(FXbool enable=TRUE);

  /// Return true if this is the default window
  FXbool isDefault() const;

  /// Make this window the initial default window
  void setInitial(FXbool enable=TRUE);

  /// Return true if this is the initial default window
  FXbool isInitial() const;

  /// Enable the window to receive mouse and keyboard events
  virtual void enable();

  /// Disable the window from receiving mouse and keyboard events
  virtual void disable();

  /// Create all of the server-side resources for this window
  virtual void create();

  /// Attach foreign window handle to this window
  virtual void attach(FXID w);

  /// Detach the server-side resources for this window
  virtual void detach();

  /// Destroy the server-side resources for this window
  virtual void destroy();

  /// Set window shape by means of region
  virtual void setShape(const FXRegion& region);

  /// Set window shape by means of bitmap
  virtual void setShape(FXBitmap* bitmap);

  /// Set window shape by means of icon
  virtual void setShape(FXIcon* icon);

  /// Clear window shape
  virtual void clearShape();

  /// Raise this window to the top of the stacking order
  virtual void raise();

  /// Lower this window to the bottom of the stacking order
  virtual void lower();

  /// Move this window to the specified position in the parent's coordinates
  virtual void move(FXint x,FXint y);

  /// Resize this window to the specified width and height
  virtual void resize(FXint w,FXint h);

  /// Move and resize this window in the parent's coordinates
  virtual void position(FXint x,FXint y,FXint w,FXint h);

  /// Mark this window's layout as dirty for later layout
  virtual void recalc();

  /// Perform layout immediately
  virtual void layout();

  /// Force a GUI update of this window and its children
  void forceRefresh();

  /// Reparent this window under new father before other
  virtual void reparent(FXWindow* father,FXWindow *other=NULL);

  /// Scroll rectangle x,y,w,h by a shift of dx,dy
  void scroll(FXint x,FXint y,FXint w,FXint h,FXint dx,FXint dy) const;

  /// Mark the specified rectangle to be repainted later
  void update(FXint x,FXint y,FXint w,FXint h) const;

  /// Mark the entire window to be repainted later
  void update() const;

  /// If marked but not yet painted, paint the given rectangle now
  void repaint(FXint x,FXint y,FXint w,FXint h) const;

  /// If marked but not yet painted, paint the window now
  void repaint() const;

  /**
  * Grab the mouse to this window; future mouse events will be
  * reported to this window even while the cursor goes outside of this window
  */
  void grab();

  /// Release the mouse grab
  void ungrab();

  /// Return true if the window has been grabbed
  FXbool grabbed() const;

  /// Grab keyboard device
  void grabKeyboard();

  /// Ungrab keyboard device
  void ungrabKeyboard();

  /// Return true if active grab is in effect
  FXbool grabbedKeyboard() const;

  /// Show this window
  virtual void show();

  /// Hide this window
  virtual void hide();

  /// Return true if the window is shown
  FXbool shown() const;

  /// Return true if the window is composite
  virtual FXbool isComposite() const;

  /// Return true if the window is under the cursor
  FXbool underCursor() const;

  /// Return true if this window owns the primary selection
  FXbool hasSelection() const;

  /// Try to acquire the primary selection, given a list of drag types
  FXbool acquireSelection(const FXDragType *types,FXuint numtypes);

  /// Release the primary selection
  FXbool releaseSelection();

  /// Return true if this window owns the clipboard
  FXbool hasClipboard() const;

  /// Try to acquire the clipboard, given a list of drag types
  FXbool acquireClipboard(const FXDragType *types,FXuint numtypes);

  /// Release the clipboard
  FXbool releaseClipboard();

  /// Enable this window to receive drops
  void dropEnable();

  /// Disable this window from receiving drops
  void dropDisable();

  /// Return true if this window is able to receive drops
  FXbool isDropEnabled() const;

  /// Return true if a drag operaion has been initiated from this window
  FXbool isDragging() const;

  /// Initiate a drag operation with a list of previously registered drag types
  FXbool beginDrag(const FXDragType *types,FXuint numtypes);

  /**
  * When dragging, inform the drop-target of the new position and
  * the drag action
  */
  FXbool handleDrag(FXint x,FXint y,FXDragAction action=DRAG_COPY);

  /**
  * Terminate the drag operation with or without actually dropping the data
  * Returns the action performed by the target
  */
  FXDragAction endDrag(FXbool drop=TRUE);

  /// Return true if this window is the target of a drop
  FXbool isDropTarget() const;

  /**
  * When being dragged over, indicate that no further SEL_DND_MOTION messages
  * are required while the cursor is inside the given rectangle
  */
  void setDragRectangle(FXint x,FXint y,FXint w,FXint h,FXbool wantupdates=TRUE) const;

  /**
  * When being dragged over, indicate we want to receive SEL_DND_MOTION messages
  * every time the cursor moves
  */
  void clearDragRectangle() const;

  /// When being dragged over, indicate acceptance or rejection of the dragged data
  void acceptDrop(FXDragAction action=DRAG_ACCEPT) const;

  /// The target accepted our drop
  FXDragAction didAccept() const;

  /// When being dragged over, inquire the drag types which are being offered
  FXbool inquireDNDTypes(FXDNDOrigin origin,FXDragType*& types,FXuint& numtypes) const;

  /// When being dragged over, return true if we are offered the given drag type
  FXbool offeredDNDType(FXDNDOrigin origin,FXDragType type) const;

  /// When being dragged over, return the drag action
  FXDragAction inquireDNDAction() const;

  /**
  * Set DND data; the array must be allocated with FXMALLOC and ownership is
  * transferred to the system
  */
  FXbool setDNDData(FXDNDOrigin origin,FXDragType type,FXuchar* data,FXuint size) const;

  /**
  * Get DND data; the caller becomes the owner of the array and must free it
  * with FXFREE
  */
  FXbool getDNDData(FXDNDOrigin origin,FXDragType type,FXuchar*& data,FXuint& size) const;

  /// Return true if window logically contains the given point
  virtual FXbool contains(FXint parentx,FXint parenty) const;

  /// Translate coordinates from fromwindow's coordinate space to this window's coordinate space
  void translateCoordinatesFrom(FXint& tox,FXint& toy,const FXWindow* fromwindow,FXint fromx,FXint fromy) const;

  /// Translate coordinates from this window's coordinate space to towindow's coordinate space
  void translateCoordinatesTo(FXint& tox,FXint& toy,const FXWindow* towindow,FXint fromx,FXint fromy) const;

  /// Set window background color
  virtual void setBackColor(FXColor clr);

  /// Get background color
  FXColor getBackColor() const { return backColor; }

  virtual FXbool doesSaveUnder() const;

  /// Save window to stream
  virtual void save(FXStream& store) const;

  /// Restore window from stream
  virtual void load(FXStream& store);

  /// Destroy window
  virtual ~FXWindow();
  };

}

#endif
