/********************************************************************************
*                                                                               *
*                     A p p l i c a t i o n   O b j e c t                       *
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
* $Id: FXApp.h,v 1.194 2005/01/31 01:22:58 fox Exp $                            *
********************************************************************************/
#ifndef FXAPP_H
#define FXAPP_H

#ifndef FXOBJECT_H
#include "FXObject.h"
#endif
#include "FXRectangle.h"
#include "FXRegistry.h"
#include "FXHash.h"

namespace FX {


// Forward declarations
class FXWindow;
class FXIcon;
class FXBitmap;
class FXCursor;
class FXRootWindow;
class FXMainWindow;
class FXPopup;
class FXFont;
class FXDC;
class FXDCWindow;
class FXVisual;
class FXGLVisual;
class FXGLContext;
class FXId;


// Opaque FOX objects
struct FXTimer;
struct FXChore;
struct FXSignal;
struct FXRepaint;
struct FXInput;
struct FXAsyncMsg;
struct FXInvocation;



/// File input modes for addInput
enum FXInputMode {
  INPUT_NONE   = 0,                 /// Inactive
  INPUT_READ   = 1,                 /// Read input fd
  INPUT_WRITE  = 2,                 /// Write input fd
  INPUT_EXCEPT = 4                  /// Except input fd
  };


/// All ways of being modal
enum FXModality {
  MODAL_FOR_NONE,                 /// Non modal event loop (dispatch normally)
  MODAL_FOR_WINDOW,               /// Modal dialog (beep if outside of modal dialog)
  MODAL_FOR_POPUP                 /// Modal for popup (always dispatch to popup)
  };


/// Default cursors provided by the application
enum FXDefaultCursor {
  DEF_ARROW_CURSOR,                     /// Arrow cursor
  DEF_RARROW_CURSOR,                    /// Reverse arrow cursor
  DEF_TEXT_CURSOR,                      /// Text cursor
  DEF_HSPLIT_CURSOR,                    /// Horizontal split cursor
  DEF_VSPLIT_CURSOR,                    /// Vertical split cursor
  DEF_XSPLIT_CURSOR,                    /// Cross split cursor
  DEF_SWATCH_CURSOR,                    /// Color swatch drag cursor
  DEF_MOVE_CURSOR,                      /// Move cursor
  DEF_DRAGH_CURSOR,                     /// Resize horizontal edge
  DEF_DRAGV_CURSOR,                     /// Resize vertical edge
  DEF_DRAGTL_CURSOR,                    /// Resize upper-leftcorner
  DEF_DRAGBR_CURSOR=DEF_DRAGTL_CURSOR,  /// Resize bottom-right corner
  DEF_DRAGTR_CURSOR,                    /// Resize upper-right corner
  DEF_DRAGBL_CURSOR=DEF_DRAGTR_CURSOR,  /// Resize bottom-left corner
  DEF_DNDSTOP_CURSOR,                   /// Drag and drop stop
  DEF_DNDCOPY_CURSOR,                   /// Drag and drop copy
  DEF_DNDMOVE_CURSOR,                   /// Drag and drop move
  DEF_DNDLINK_CURSOR,                   /// Drag and drop link
  DEF_CROSSHAIR_CURSOR,                 /// Cross hair cursor
  DEF_CORNERNE_CURSOR,                  /// North-east cursor
  DEF_CORNERNW_CURSOR,                  /// North-west cursor
  DEF_CORNERSE_CURSOR,                  /// South-east cursor
  DEF_CORNERSW_CURSOR,                  /// South-west cursor
  DEF_HELP_CURSOR,                      /// Help arrow cursor
  DEF_HAND_CURSOR,                      /// Hand cursor
  DEF_ROTATE_CURSOR                     /// Rotate cursor
  };



/// FOX Event
struct FXAPI FXEvent {
  FXuint      type;           /// Event type
  FXuint      time;           /// Time of last event
  FXint       win_x;          /// Window-relative x-coord
  FXint       win_y;          /// Window-relative y-coord
  FXint       root_x;         /// Root x-coord
  FXint       root_y;         /// Root y-coord
  FXint       state;          /// Keyboard/Modifier state
  FXint       code;           /// Button, Keysym, or mode; DDE Source
  FXString    text;           /// Text of keyboard event
  FXint       last_x;         /// Window-relative x-coord of previous mouse location
  FXint       last_y;         /// Window-relative y-coord of previous mouse location
  FXint       click_x;        /// Window-relative x-coord of mouse press
  FXint       click_y;        /// Window-relative y-coord of mouse press
  FXint       rootclick_x;    /// Root-relative x-coord of mouse press
  FXint       rootclick_y;    /// Root-relative y-coord of mouse press
  FXuint      click_time;     /// Time of mouse button press
  FXuint      click_button;   /// Mouse button pressed
  FXint       click_count;    /// Click-count
  FXbool      moved;          /// Moved cursor since press
  FXRectangle rect;           /// Rectangle
  FXbool      synthetic;      /// True if synthetic expose event
  FXDragType  target;         /// Target drag type being requested
  };


/// Placeholder class for a lockable object
class FXAPI FXLockable {
public:
  virtual ~FXLockable() { }
  virtual void lock() { }               /// Locks the object for exclusive access
  virtual void unlock() { }             /// Unlocks the object
  };


/// FOX event dispatch loop
class FXAPI FXEventLoop : public FXLockable, public FXObject {
  FXDECLARE(FXEventLoop)

protected:
  friend class FXApp;
  friend class FXPopup;
  friend class FXWindow;
  template<typename type> friend class FXEventLoop_Static;

  FXApp           *app;                 // Back link to application object
  FXHash           hash;                // Window handle hash table
  FXWindow        *focusWindow;         // Window which has the focus
  FXWindow        *cursorWindow;        // Window under the cursor
  FXWindow        *mouseGrabWindow;     // Window which grabbed the mouse
  FXWindow        *keyboardGrabWindow;  // Window which grabbed the keyboard
  FXWindow        *keyWindow;           // Window in which keyboard key was pressed
  FXWindow        *selectionWindow;     // Selection window
  FXWindow        *clipboardWindow;     // Clipboard window
  FXWindow        *dropWindow;          // Drop target window
  FXWindow        *dragWindow;          // Drag source window
  FXWindow        *refresher;           // GUI refresher pointer
  FXWindow        *refresherstop;       // GUI refresher end pointer
  FXPopup         *popupWindow;         // Current popup window
  FXRootWindow    *root;                // Root window
  FXTimer         *timers;              // List of timers, sorted by time
  FXChore         *chores;              // List of chores
  FXRepaint       *repaints;            // Unhandled repaint rectangles
  FXTimer         *timerrecs;           // List of recycled timer records
  FXChore         *chorerecs;           // List of recycled chore records
  FXRepaint       *repaintrecs;         // List of recycled repaint records
  FXInvocation    *invocation;          // Modal loop invocation
  FXSignal        *signals;             // Array of signal records
  FXint            nsignals;            // Number of signals
  FXEvent          event;               // Event
  FXInput         *inputs;              // Input file descriptors being watched
  FXint            ninputs;             // Number of inputs
  FXint            maxinput;            // Maximum input number
  FXAsyncMsg      *asyncmsgs;           // Asynchronously posted messages
  FXAsyncMsg      *asyncmsgsrecs;       // Recycled asynchronously posted messages
  FXbool          &initialized;         // FXApp has been initialized
  void           *&display;             // Display we're talking to


  // Platform dependent private stuff
#ifndef WIN32
  void           *&xim;                 // Input method
  void           *&xic;                 // Input method context
  void            *r_fds;               // Set of file descriptors for read
  void            *w_fds;               // Set of file descriptors for write
  void            *e_fds;               // Set of file descriptors for exceptions
  int              latch[2];            // Used to latch event loop

#else
  void           **handles;             // Waitable object handles
  void            *lastmovehwnd;
  long            lastmovelParam;
  void            *oldhwnd;
  void            *latch;               // Used to latch event loop
  FXuint          repostimer;           // Used to ensure timers & chores work
  FXuint          msgloopcount;         // How many times the message loop has run modally
  FXuint          tickmsgloopcount;     // Msg loop count at last tick

#endif

  // Internal helper functions
  FXEventLoop(const FXEventLoop&);
  FXEventLoop &operator=(const FXEventLoop&);
  void setup();
  static void signalhandler(int sig);
  static void immediatesignalhandler(int sig);
  void leaveWindow(FXWindow *window,FXWindow *ancestor);
  void enterWindow(FXWindow *window,FXWindow *ancestor);
  void selectionSetData(const FXWindow* window,FXDragType type,FXuchar* data,FXuint size);
  void selectionGetData(const FXWindow* window,FXDragType type,FXuchar*& data,FXuint& size);
  void selectionGetTypes(const FXWindow* window,FXDragType*& types,FXuint& numtypes);
  void clipboardSetData(const FXWindow* window,FXDragType type,FXuchar* data,FXuint size);
  void clipboardGetData(const FXWindow* window,FXDragType type,FXuchar*& data,FXuint& size);
  void clipboardGetTypes(const FXWindow* window,FXDragType*& types,FXuint& numtypes);
  void dragdropSetData(const FXWindow* window,FXDragType type,FXuchar* data,FXuint size);
  void dragdropGetData(const FXWindow* window,FXDragType type,FXuchar*& data,FXuint& size);
  void dragdropGetTypes(const FXWindow* window,FXDragType*& types,FXuint& numtypes);
#ifndef WIN32
  void addRepaint(FXID win,FXint x,FXint y,FXint w,FXint h,FXbool synth=0);
  void removeRepaints(FXID win,FXint x,FXint y,FXint w,FXint h);
  void scrollRepaints(FXID win,FXint dx,FXint dy);
#else
  static long CALLBACK wndproc(FXID hwnd,unsigned int iMsg,unsigned int wParam,long lParam);
protected:
  long dispatchEvent(FXID hwnd,unsigned int iMsg,unsigned int wParam,long lParam);
#endif
private:
  FXbool doIdleProcessing();
  void doTimers();
  void doAsyncMsgs();

protected:

  /// Constructs an event loop.
  FXEventLoop(FXApp* app=NULL);

  /// Return TRUE when new raw event is available
  FXbool getNextEvent(FXRawEvent& ev,FXbool blocking=TRUE);

  /// Dispatch raw event
  virtual FXbool dispatchEvent(FXRawEvent& ev);

  /// Latches the loop (stops any wait and restarts it)
  virtual void latchLoop();

  /// Resets the loop latch
  virtual void resetLoopLatch();

  /// Implementation of fetching an event
  virtual FXbool getNextEventI(FXRawEvent& ev,FXbool blocking);

  /// Implementation of peeking for an event
  virtual FXbool peekEventI();

  /// Allocates a per-thread static instance
  virtual void* makeStaticPtr(void* val);

  /// Deallocates a per-thread static instance
  virtual void deleteStaticPtr(void* ref);

  /// Retrieves the value stored in the per-thread static instance
  virtual void* getStaticPtr(const void* ref) const;

  /// Sets the value stored in the per-thread static instance
  virtual void setStaticPtr(void* ref,void* val);

public:
  enum {
    ID_ASYNCRECALC=1,
    ID_LAST
    };
public:
  /// Get application
  FXApp* getApp() const { return app; }

  /// Get root Window
  FXRootWindow* getRootWindow() const { return root; }

  /// Set root Window
  void setRootWindow(FXRootWindow* rt);

  /// Get the window under the cursor, if any
  FXWindow *getCursorWindow() const { return cursorWindow; }

  /// Get the window which has the focus, if any
  FXWindow *getFocusWindow() const { return focusWindow; }

  /// Get current popup window, if any
  FXPopup* getPopupWindow() const { return popupWindow; }

  /// Find window from id
  FXWindow* findWindowWithId(FXID xid) const;

  /// Find window from root x,y, starting from given window
  FXWindow* findWindowAt(FXint rx,FXint ry,FXID window=0) const;

  /**
  * Add timeout message to be sent to target object in ms milliseconds;
  * the timer fires only once after the interval expires.  The void* ptr
  * is user data which will be passed into the void* ptr of the message
  * handler.  If a timer with the same target and message already exists,
  * it will be rescheduled.
  */
  void addTimeout(FXObject* tgt,FXSelector sel,FXuint ms=1000,void* ptr=NULL);

  /**
  * Remove timeout identified by tgt and sel.
  */
  void removeTimeout(FXObject* tgt,FXSelector sel);

  /**
  * Return TRUE if given timeout has been set
  */
  FXbool hasTimeout(FXObject *tgt,FXSelector sel) const;

  /**
  * Return, in ms, the time remaining until the given timer fires.
  * If the timer is past due, 0 is returned.  If there is no such
  * timer, infinity (UINT_MAX) is returned.
  */
  FXuint remainingTimeout(FXObject *tgt,FXSelector sel);

  /**
  * Process any timouts due at this time.
  */
  void handleTimeouts();

  /**
  * Add a idle processing message to be sent to target object when
  * the system becomes idle, i.e. there are no events to be processed.
  * The void* ptr is user data which will be passed into the void* ptr
  * of the message handler. If a chore with the same target and message
  * already exists, it will be rescheduled.
  */
  void addChore(FXObject* tgt,FXSelector sel,void *ptr=NULL);

  /**
  * Remove idle processing message identified by tgt and sel.
  */
  void removeChore(FXObject* tgt,FXSelector sel);

  /**
  * Return TRUE if given chore has been set
  */
  FXbool hasChore(FXObject *tgt,FXSelector sel) const;

  /**
  * Add signal processing message to be sent to target object when
  * the signal sig is raised; flags are to be set as per POSIX definitions.
  * When immediate is TRUE, the message will be sent to the target right away;
  * this should be used with extreme care as the application is interrupted
  * at an unknown point in its execution.
  */
  void addSignal(FXint sig,FXObject* tgt,FXSelector sel,FXbool immediate=FALSE,FXuint flags=0);

  /// Remove signal message for signal sig
  void removeSignal(FXint sig);

  /**
  * Add a file descriptor fd to be watched for activity as determined
  * by mode, where mode is a bitwise OR (INPUT_READ, INPUT_WRITE, INPUT_EXCEPT).
  * A message of type SEL_IO_READ, SEL_IO_WRITE, or SEL_IO_EXCEPT will be sent
  * to the target when the specified activity is detected on the file descriptor.
  */
  FXbool addInput(FXInputHandle fd,FXuint mode,FXObject *tgt,FXSelector sel);

  /**
  * Remove input message and target object for the specified file descriptor
  * and mode, which is a bitwise OR of (INPUT_READ, INPUT_WRITE, INPUT_EXCEPT).
  */
  FXbool removeInput(FXInputHandle fd,FXuint mode);

  /// Return key state
  FXbool getKeyState(FXuint keysym) const;

  /// Peek to determine if there's an event
  FXbool peekEvent();

  /// Perform one event dispatch; return true if event was dispatched
  FXbool runOneEvent(FXbool blocking=TRUE);

  /**
  * Run the main application event loop until stop() is called,
  * and return the exit code passed as argument to stop().
  */
  FXint run();

  /**
  * Run an event loop till some flag becomes non-zero, and
  * then return.
  */
  FXint runUntil(FXuint& condition);

  /**
  * Run event loop while events are available, non-modally.
  * Return when no more events, timers, or chores are outstanding.
  */
  FXint runWhileEvents();

  /**
  * Run event loop while there are events are available in the queue.
  * Returns 1 when all events in the queue have been handled, and 0 when
  * the event loop was terminated due to stop() or stopModal().
  * Except for the modal window and its children, user input to all windows
  * is blocked; if the modal window is NULL, all user input is blocked.
  */
  FXint runModalWhileEvents(FXWindow* window=NULL);

  /**
  * Run modal event loop, blocking keyboard and mouse events to all windows
  * until stopModal is called.
  */
  FXint runModal();

  /**
  * Run a modal event loop for the given window, until stop() or stopModal() is
  * called. Except for the modal window and its children, user input to all
  * windows is blocked; if the modal window is NULL all user input is blocked.
  */
  FXint runModalFor(FXWindow* window);

  /**
  * Run modal while window is shown, or until stop() or stopModal() is called.
  * Except for the modal window and its children, user input to all windows
  * is blocked; if the modal window is NULL all user input is blocked.
  */
  FXint runModalWhileShown(FXWindow* window);

  /**
  * Run popup menu while shown, until stop() or stopModal() is called.
  * Also returns when entering previous cascading popup menu.
  */
  FXint runPopup(FXWindow* window);

  /// True if the window is modal
  FXbool isModal(FXWindow* window) const;

  /// Return window of current modal loop
  FXWindow* getModalWindow() const;

  /// Return mode of current modal loop
  FXModality getModality() const;

  /**
  * Terminate the outermost event loop, and all inner modal loops;
  * All more deeper nested event loops will be terminated with code equal
  * to 0, while the outermost event loop will return code equal to value.
  */
  void stop(FXint value=0);

  /**
  * Break out of the matching modal loop, returning code equal to value.
  * All deeper nested event loops are terminated with code equal to 0.
  */
  void stopModal(FXWindow* window,FXint value=0);

  /**
  * Break out of the innermost modal loop, returning code equal to value.
  */
  void stopModal(FXint value=0);

  /// Force GUI refresh
  void forceRefresh();

  /// Schedule a refresh
  void refresh();

  /// Flush pending repaints
  void flush(FXbool sync=FALSE);

  /**
  * Paint all windows marked for repainting.
  * On return all the applications windows have been painted.
  */
  void repaint();

  /// Return drag window if a drag operation is in progress
  FXWindow* getDragWindow() const { return dragWindow; }

  virtual ~FXEventLoop();

public:
  long onCmdAsyncRecalc(FXObject*,FXSelector,void*);

  /// Posts a message asynchronously
  void postAsyncMessage(FXObject* tgt,FXuint message,void *ptr=NULL,FXObject* src=NULL);

  /// Posts a message asynchronously
  static void postAsyncMessage(FXId* tgt,FXuint message,void *ptr=NULL,FXObject* src=NULL);

  /// Returns the id of the thread this event loop runs in (only valid in subclasses)
  virtual FXuint getThreadId() const { return 0; }

  /*! Requests that a recalc() be issued for the sender before the next event fetch
  \warning You should avoid this call except where absolutely necessary */
  static void requestAsyncRecalc(FXWindow *what);

};

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

/// Application Object
class FXAPI FXApp : public FXLockable, public FXObject {
  FXDECLARE(FXApp)

  // We've got many friends
  friend class FXEventLoop;
  friend class FXId;
  friend class FXBitmap;
  friend class FXImage;
  friend class FXIcon;
  friend class FXCursor;
  friend class FXDrawable;
  friend class FXWindow;
  friend class FXShell;
  friend class FXRootWindow;
  friend class FXTopWindow;
  friend class FXMainWindow;
  friend class FXPopup;
  friend class FXFont;
  friend class FXVisual;
  friend class FXGLVisual;
  friend class FXGLContext;
  friend class FXDC;
  friend class FXDCWindow;
  friend class FXToolBarGrip;
  friend class FXEmbedderWindow;
protected:
  mutable FXEventLoop *eventLoop;       // Event loop

private:

  // Platform independent private data
  void            *display;             // Display we're talking to
  const FXchar    *dpy;                 // Initial display guess
  FXRegistry       registry;            // Application setting registry
  FXVisual        *monoVisual;          // Monochrome visual
  FXVisual        *defaultVisual;       // Default [color] visual
  FXFont          *normalFont;          // Normal font
  FXFont          *stockFont;           // Stock font
  FXuchar         *ddeData;             // DDE array
  FXuint           ddeSize;             // DDE array size
  FXuint           maxcolors;           // Maximum number of colors to allocate
  FXuint           typingSpeed;         // Typing speed
  FXuint           clickSpeed;          // Double click speed
  FXuint           scrollSpeed;         // Scroll speed
  FXuint           scrollDelay;         // Scroll delay
  FXuint           blinkSpeed;          // Cursor blink speed
  FXuint           animSpeed;           // Animation speed
  FXuint           menuPause;           // Menu popup delay
  FXuint           tooltipPause;        // Tooltip popup delay
  FXuint           tooltipTime;         // Tooltip display time
  FXint            dragDelta;           // Minimum distance considered a move
  FXint            wheelLines;          // Scroll by this many lines
  FXColor          borderColor;         // Border color
  FXColor          baseColor;           // Background color of GUI controls
  FXColor          hiliteColor;         // Highlight color of GUI controls
  FXColor          shadowColor;         // Shadow color of GUI controls
  FXColor          backColor;           // Background color
  FXColor          foreColor;           // Foreground color
  FXColor          selforeColor;        // Select foreground color
  FXColor          selbackColor;        // Select background color
  FXColor          tipforeColor;        // Tooltip foreground color
  FXColor          tipbackColor;        // Tooltip background color
  FXColor          selMenuTextColor;    // Select foreground color in menus
  FXColor          selMenuBackColor;    // Select background color in menus
  FXCursor        *waitCursor;          // Current wait cursor
  FXuint           waitCount;           // Number of times wait cursor was called
  FXCursor        *cursor[DEF_ROTATE_CURSOR+1];
  FXint            appArgc;             // Argument count
  const FXchar   **appArgv;             // Argument vector
  FXbool           initialized;         // Has been initialized

private:
  static FXApp    *app;                 // Application pointer

  // Platform dependent private stuff
#ifndef WIN32
private:
  FXID             wmDeleteWindow;      // Catch delete window
  FXID             wmQuitApp;           // Catch quit application
  FXID             wmProtocols;         // Window manager protocols
  FXID             wmMotifHints;        // Motif hints
  FXID             wmTakeFocus;         // Focus explicitly set by app
  FXID             wmState;             // Window state
  FXID             wmNetSupported;      // Extended Window Manager states list
  FXID             wmNetState;          // Extended Window Manager window state
  FXID             wmNetHMaximized;     // Extended Window Manager horizontally maximized
  FXID             wmNetVMaximized;     // Extended Window Manager vertically maximized
  FXID             embedAtom;           // XEMBED support
  FXID             embedInfoAtom;       // XEMBED info support
  FXID             timestampAtom;       // Server time
  FXID             ddeTargets;          // DDE targets atom
  FXID             ddeAtom;             // DDE exchange atom
  FXID             ddeDelete;           // DDE delete target atom
  FXID             ddeIncr;             // DDE incremental data exchange atom
  FXDragType      *ddeTypeList;         // DDE drop type list
  FXuint           ddeNumTypes;         // DDE number of drop types
  FXDragAction     ddeAction;           // DDE action
  FXDragAction     ansAction;           // Reply action
  FXID             xcbSelection;        // Clipboard selection
  FXDragType      *xcbTypeList;         // Clipboard type list
  FXuint           xcbNumTypes;         // Clipboard number of types on list
  FXDragType      *xselTypeList;        // Selection type list
  FXuint           xselNumTypes;        // Selection number of types on list
  FXDragType      *xdndTypeList;        // XDND type list
  FXuint           xdndNumTypes;        // XDND number of types
  FXID             xdndProxy;           // XDND proxy atom
  FXID             xdndAware;           // XDND awareness atom
  FXID             xdndEnter;           // XDND enter window message
  FXID             xdndLeave;           // XDND leave window message
  FXID             xdndPosition;        // XDND position update message
  FXID             xdndStatus;          // XDND status feedback message
  FXID             xdndDrop;            // XDND drop message
  FXID             xdndFinished;        // XDND finished message
  FXID             xdndSelection;       // XDND selection atom
  FXID             xdndActionMove;      // XDND Move action
  FXID             xdndActionCopy;      // XDND Copy action
  FXID             xdndActionLink;      // XDND Link action
  FXID             xdndActionPrivate;   // XDND Private action
  FXID             xdndTypes;           // XDND types list atom
  FXID             xdndSource;          // XDND drag source window
  FXID             xdndTarget;          // XDND drop target window
  FXID             xdndProxyTarget;     // XDND window to set messages to
  FXbool           xdndStatusPending;   // XDND waiting for status feedback
  FXbool           xdndStatusReceived;  // XDND received at least one status
  FXbool           xdndWantUpdates;     // XDND target wants new positions while in rect
  FXRectangle      xdndRect;            // XDND rectangle bounding target
  FXint            xrreventbase;        // XRR event base
  FXID             stipples[23];        // Standard stipple patterns
  void            *xim;                 // Input method
  void            *xic;                 // Input method context
  FXbool           shmi;                // Use XSHM Image possible
  FXbool           shmp;                // Use XSHM Pixmap possible
  FXbool           synchronize;         // Synchronized

#else

  FXushort         ddeTargets;          // DDE targets atom
  FXushort         ddeAtom;             // DDE Exchange Atom
  FXDragType       ddeDelete;           // DDE Delete Target Atom
  FXDragType      *ddeTypeList;         // DDE drop type list
  FXuint           ddeNumTypes;         // DDE number of drop types
  FXDragAction     ddeAction;           // DDE action
  FXDragAction     ansAction;           // Reply action
  FXDragType      *xselTypeList;        // Selection type list
  FXuint           xselNumTypes;        // Selection number of types on list
  void*            xdndTypes;           // Handle to file mapping object for types list
  FXushort         xdndAware;           // XDND awareness atom
  FXID             xdndSource;          // XDND drag source window
  FXID             xdndTarget;          // XDND drop target window
  FXbool           xdndStatusPending;   // XDND waiting for status feedback
  FXbool           xdndFinishPending;   // XDND waiting for drop-confirmation
  FXbool           xdndStatusReceived;  // XDND received at least one status
  FXRectangle      xdndRect;            // XDND rectangle bounding target
  FXID             stipples[17];        // Standard stipple bitmaps

#endif

private:

  // Internal helper functions
  FXApp(const FXApp&);
  FXApp &operator=(const FXApp&);
#ifndef WIN32
#else
protected:
  virtual long dispatchEvent(FXID hwnd,unsigned int iMsg,unsigned int wParam,long lParam);
#endif

protected:

  /// Return TRUE when new raw event is available
  virtual FXbool getNextEvent(FXRawEvent& ev,FXbool blocking=TRUE);

  /// Dispatch raw event
  virtual FXbool dispatchEvent(FXRawEvent& ev);

public:
  long onCmdQuit(FXObject*,FXSelector,void*);
  long onCmdDump(FXObject*,FXSelector,void*);
  long onCmdHover(FXObject*,FXSelector,void*);

public:

  /// Messages applications understand
  enum {
    ID_QUIT=1,    /// Terminate the application normally
    ID_DUMP,      /// Dump the current widget tree
    ID_HOVER,
    ID_LAST
    };

public:

  /// Information
  static const FXuchar *copyright;     /// Copyright notice of library

public:

  /**
  * Construct application object; the name and vendor strings are used
  * as keys into the registry database for this application's settings.
  * Only one single application object can be constructed.
  */
  FXApp(const FXString& name="Application",const FXString& vendor="FoxDefault");

  /// Get application name
  const FXString& getAppName() const { return registry.getAppKey(); }

  /// Get vendor name
  const FXString& getVendorName() const { return registry.getVendorKey(); }

  /// Connection to display; this is called by init()
  FXbool openDisplay(const FXchar* dpyname=NULL);

  /// Close connection to the display
  FXbool closeDisplay();

  /// Return pointer
  void* getDisplay() const { return display; }

  /// Is application initialized
  FXbool isInitialized() const { return initialized; }

  /// Get argument count
  FXint getArgc() const { return appArgc; }

  /// Get argument vector
  const FXchar **getArgv() const { return appArgv; }

  /// Get event loop for callee
  virtual FXEventLoop* getEventLoop() const;

  /// Get primary event loop of the process
  static FXEventLoop* getPrimaryEventLoop();

  /// Get default visual
  FXVisual* getDefaultVisual() const { return defaultVisual; }

  /// Change default visual
  void setDefaultVisual(FXVisual* vis);

  /// Get monochrome visual
  FXVisual* getMonoVisual() const { return monoVisual; }

  /// Get root Window
  FXRootWindow* getRootWindow() const { return getEventLoop()->getRootWindow(); }

  /// Set root Window
  void setRootWindow(FXRootWindow* rt) { return getEventLoop()->setRootWindow(rt); }

  /// Get the window under the cursor, if any
  FXWindow *getCursorWindow() const { return getEventLoop()->getCursorWindow(); }

  /// Get the window which has the focus, if any
  FXWindow *getFocusWindow() const { return getEventLoop()->getFocusWindow(); }

  /// Get current popup window, if any
  FXPopup* getPopupWindow() const { return getEventLoop()->getPopupWindow(); }

  /// Find window from id
  FXWindow* findWindowWithId(FXID xid) const { return getEventLoop()->findWindowWithId(xid); }

  /// Find window from root x,y, starting from given window
  FXWindow* findWindowAt(FXint rx,FXint ry,FXID window=0) const { return getEventLoop()->findWindowAt(rx,ry,window); }

  /// Create application's windows
  virtual void create();

  /// Destroy application's windows
  virtual void destroy();

  /// Detach application's windows
  virtual void detach();

  /**
  * Add timeout message to be sent to target object in ms milliseconds;
  * the timer fires only once after the interval expires.  The void* ptr
  * is user data which will be passed into the void* ptr of the message
  * handler.  If a timer with the same target and message already exists,
  * it will be rescheduled.
  */
  void addTimeout(FXObject* tgt,FXSelector sel,FXuint ms=1000,void* ptr=NULL) { return getEventLoop()->addTimeout(tgt,sel,ms,ptr); }

  /**
  * Remove timeout identified by tgt and sel.
  */
  void removeTimeout(FXObject* tgt,FXSelector sel) { return getEventLoop()->removeTimeout(tgt,sel); }

  /**
  * Return TRUE if given timeout has been set
  */
  FXbool hasTimeout(FXObject *tgt,FXSelector sel) const { return getEventLoop()->hasTimeout(tgt,sel); }

  /**
  * Return, in ms, the time remaining until the given timer fires.
  * If the timer is past due, 0 is returned.  If there is no such
  * timer, infinity (UINT_MAX) is returned.
  */
  FXuint remainingTimeout(FXObject *tgt,FXSelector sel) const { return getEventLoop()->remainingTimeout(tgt,sel); }

  /**
  * Process any timeouts due at this time.
  */
  void handleTimeouts() { return getEventLoop()->handleTimeouts(); }

  /**
  * Add a idle processing message to be sent to target object when
  * the system becomes idle, i.e. there are no events to be processed.
  * The void* ptr is user data which will be passed into the void* ptr
  * of the message handler. If a chore with the same target and message
  * already exists, it will be rescheduled.
  */
  void addChore(FXObject* tgt,FXSelector sel,void *ptr=NULL) { return getEventLoop()->addChore(tgt,sel,ptr); }

  /**
  * Remove idle processing message identified by tgt and sel; returns NULL.
  */
  void removeChore(FXObject* tgt,FXSelector sel) { return getEventLoop()->removeChore(tgt,sel); }

  /**
  * Return TRUE if given chore has been set
  */
  FXbool hasChore(FXObject *tgt,FXSelector sel) const { return getEventLoop()->hasChore(tgt,sel); }

  /**
  * Add signal processing message to be sent to target object when
  * the signal sig is raised; flags are to be set as per POSIX definitions.
  * When immediate is TRUE, the message will be sent to the target right away;
  * this should be used with extreme care as the application is interrupted
  * at an unknown point in its execution.
  */
  void addSignal(FXint sig,FXObject* tgt,FXSelector sel,FXbool immediate=FALSE,FXuint flags=0) { getEventLoop()->addSignal(sig,tgt,sel,immediate,flags); }

  /// Remove signal message for signal sig
  void removeSignal(FXint sig) { getEventLoop()->removeSignal(sig); }

  /**
  * Add a file descriptor fd to be watched for activity as determined
  * by mode, where mode is a bitwise OR (INPUT_READ, INPUT_WRITE, INPUT_EXCEPT).
  * A message of type SEL_IO_READ, SEL_IO_WRITE, or SEL_IO_EXCEPT will be sent
  * to the target when the specified activity is detected on the file descriptor.
  */
  FXbool addInput(FXInputHandle fd,FXuint mode,FXObject *tgt,FXSelector sel) { return getEventLoop()->addInput(fd,mode,tgt,sel); }

  /**
  * Remove input message and target object for the specified file descriptor
  * and mode, which is a bitwise OR of (INPUT_READ, INPUT_WRITE, INPUT_EXCEPT).
  */
  FXbool removeInput(FXInputHandle fd,FXuint mode) { return getEventLoop()->removeInput(fd,mode); }

  /// Return key state
  FXbool getKeyState(FXuint keysym) const;

  /// Peek to determine if there's an event
  FXbool peekEvent() { return getEventLoop()->peekEvent(); }

  /// Perform one event dispatch; return true if event was dispatched
  FXbool runOneEvent(FXbool blocking=TRUE) { return getEventLoop()->runOneEvent(blocking); }

  /**
  * Run the main application event loop until stop() is called,
  * and return the exit code passed as argument to stop().
  */
  FXint run() { return getEventLoop()->run(); }

  /**
  * Run an event loop till some flag becomes non-zero, and
  * then return.
  */
  FXint runUntil(FXuint& condition) { return getEventLoop()->runUntil(condition); }

  /**
  * Run event loop while events are available, non-modally.
  * Return when no more events, timers, or chores are outstanding.
  */
  FXint runWhileEvents() { return getEventLoop()->runWhileEvents(); }

  /**
  * Run event loop while there are events are available in the queue.
  * Returns 1 when all events in the queue have been handled, and 0 when
  * the event loop was terminated due to stop() or stopModal().
  * Except for the modal window and its children, user input to all windows
  * is blocked; if the modal window is NULL, all user input is blocked.
  */
  FXint runModalWhileEvents(FXWindow* window=NULL) { return getEventLoop()->runModalWhileEvents(window); }

  /**
  * Run modal event loop, blocking keyboard and mouse events to all windows
  * until stopModal is called.
  */
  FXint runModal() { return getEventLoop()->runModal(); }

  /**
  * Run a modal event loop for the given window, until stop() or stopModal() is
  * called. Except for the modal window and its children, user input to all
  * windows is blocked; if the modal window is NULL all user input is blocked.
  */
  FXint runModalFor(FXWindow* window) { return getEventLoop()->runModalFor(window); }

  /**
  * Run modal while window is shown, or until stop() or stopModal() is called.
  * Except for the modal window and its children, user input to all windows
  * is blocked; if the modal window is NULL all user input is blocked.
  */
  FXint runModalWhileShown(FXWindow* window) { return getEventLoop()->runModalWhileShown(window); }

  /**
  * Run popup menu while shown, until stop() or stopModal() is called.
  * Also returns when entering previous cascading popup menu.
  */
  FXint runPopup(FXWindow* window) { return getEventLoop()->runPopup(window); }

  /// True if the window is modal
  FXbool isModal(FXWindow* window) const { return getEventLoop()->isModal(window); }

  /// Return window of current modal loop
  FXWindow* getModalWindow() const { return getEventLoop()->getModalWindow(); }

  /// Return mode of current modal loop
  FXModality getModality() const { return getEventLoop()->getModality(); }

  /**
  * Terminate the outermost event loop, and all inner modal loops;
  * All more deeper nested event loops will be terminated with code equal
  * to 0, while the outermost event loop will return code equal to value.
  */
  void stop(FXint value=0) { getEventLoop()->stop(value); }

  /**
  * Break out of the matching modal loop, returning code equal to value.
  * All deeper nested event loops are terminated with code equal to 0.
  */
  void stopModal(FXWindow* window,FXint value=0) { getEventLoop()->stopModal(window, value); }

  /**
  * Break out of the innermost modal loop, returning code equal to value.
  */
  void stopModal(FXint value=0) { getEventLoop()->stopModal(value); }

  /// Force GUI refresh
  void forceRefresh() { getEventLoop()->forceRefresh(); }

  /// Schedule a refresh
  void refresh() { getEventLoop()->refresh(); }

  /// Flush pending repaints
  void flush(FXbool sync=FALSE) { getEventLoop()->flush(sync); }

  /**
  * Paint all windows marked for repainting.
  * On return all the applications windows have been painted.
  */
  void repaint() { getEventLoop()->repaint(); }

  /**
  * Initialize application.
  * Parses and removes common command line arguments, reads the registry.
  * Finally, if connect is TRUE, it opens the display.
  */
  virtual void init(int& argc,char** argv,FXbool connect=TRUE);

  /**
  * Exit application.
  * Closes the display and writes the registry.
  */
  virtual void exit(FXint code=0);

  /**
  * Return a reference to the registry.  The registry keeps
  * settings and configuration information for an application,
  * which are automatically loaded when the application starts
  * up, and saved when the application terminates.
  */
  FXRegistry& reg(){ return registry; }

  /// Register new DND type
  FXDragType registerDragType(const FXString& name) const;

  /// Get drag type name
  FXString getDragTypeName(FXDragType type) const;

  /// Return drag window if a drag operation is in progress
  FXWindow* getDragWindow() const { return getEventLoop()->getDragWindow(); }

  /// Beep
  void beep();

  /// Error beep
  void errorBeep();

  /// Return application instance
  static inline FXApp* instance(){ return app; }

  /// Change default font
  void setNormalFont(FXFont* font);

  /// Return default font
  FXFont* getNormalFont() const { return normalFont; }

  /// Begin of wait-cursor block; wait-cursor blocks may be nested.
  void beginWaitCursor();

  /// End of wait-cursor block
  void endWaitCursor();

  /// Change to a new wait cursor
  void setWaitCursor(FXCursor *cur);

  /// Return current wait cursor
  FXCursor* getWaitCursor() const { return waitCursor; }

  /// Obtain a default cursor
  FXCursor* getDefaultCursor(FXDefaultCursor which) const { return cursor[which]; }

  /// Change default cursor
  void setDefaultCursor(FXDefaultCursor which,FXCursor* cur);

  /// Obtain application-wide settings
  FXuint getTypingSpeed() const { return typingSpeed; }
  FXuint getClickSpeed() const { return clickSpeed; }
  FXuint getScrollSpeed() const { return scrollSpeed; }
  FXuint getScrollDelay() const { return scrollDelay; }
  FXuint getBlinkSpeed() const { return blinkSpeed; }
  FXuint getAnimSpeed() const { return animSpeed; }
  FXuint getMenuPause() const { return menuPause; }
  FXuint getTooltipPause() const { return tooltipPause; }
  FXuint getTooltipTime() const { return tooltipTime; }
  FXint getDragDelta() const { return dragDelta; }
  FXint getWheelLines() const { return wheelLines; }

  /// Change application-wide settings
  void setTypingSpeed(FXuint speed);
  void setClickSpeed(FXuint speed);
  void setScrollSpeed(FXuint speed);
  void setScrollDelay(FXuint delay);
  void setBlinkSpeed(FXuint speed);
  void setAnimSpeed(FXuint speed);
  void setMenuPause(FXuint pause);
  void setTooltipPause(FXuint pause);
  void setTooltipTime(FXuint time);
  void setDragDelta(FXint delta);
  void setWheelLines(FXint lines);

  /// Obtain default colors
  FXColor getBorderColor() const { return borderColor; }
  FXColor getBaseColor() const { return baseColor; }
  FXColor getHiliteColor() const { return hiliteColor; }
  FXColor getShadowColor() const { return shadowColor; }
  FXColor getBackColor() const { return backColor; }
  FXColor getForeColor() const { return foreColor; }
  FXColor getSelforeColor() const { return selforeColor; }
  FXColor getSelbackColor() const { return selbackColor; }
  FXColor getTipforeColor() const { return tipforeColor; }
  FXColor getTipbackColor() const { return tipbackColor; }
  FXColor getSelMenuTextColor() const { return selMenuTextColor; }
  FXColor getSelMenuBackColor() const { return selMenuBackColor; }

  /// Change default colors
  void setBorderColor(FXColor color);
  void setBaseColor(FXColor color);
  void setHiliteColor(FXColor color);
  void setShadowColor(FXColor color);
  void setBackColor(FXColor color);
  void setForeColor(FXColor color);
  void setSelforeColor(FXColor color);
  void setSelbackColor(FXColor color);
  void setTipforeColor(FXColor color);
  void setTipbackColor(FXColor color);
  void setSelMenuTextColor(FXColor color);
  void setSelMenuBackColor(FXColor color);

  /// Save
  virtual void save(FXStream& store) const;

  /// Load
  virtual void load(FXStream& store);

  /// Dump widget information
  void dumpWidgets() const;

  /// Destroy the application and all reachable resources
  virtual ~FXApp();
  };

#ifdef _MSC_VER
#pragma warning(pop)
#endif

/// Exclusive access lock holder for any FXLockable subclass
class FXAPI FXLockHold
{
  FXbool locked;
  FXLockable *item;
public:
  /// Opens and holds the lock
  FXLockHold(FXLockable *obj) : locked(FALSE), item(obj) { relock(); }

  /// Opens and holds the lock
  FXLockHold(FXLockable &obj) : locked(FALSE), item(&obj) { relock(); }

  /// Releases the lock
  ~FXLockHold() { unlock(); }

  /// Relocks the item if not already locked
  void relock(){
    if(!locked && item){
        item->lock();
        locked=TRUE;
      }
    }

  // Unlocks the item if locked
  void unlock(){
    if(locked && item){
      item->unlock();
      locked=FALSE;
      }
    }

  };

/// Creates a per-thread static instance
template<typename type> class FXEventLoop_Static
{
  void *ref;
public:
  FXEventLoop_Static(type *v=NULL) : ref(FXApp::instance()->getEventLoop()->makeStaticPtr(static_cast<void *>(v))) { }
  ~FXEventLoop_Static() { FXApp::instance()->getEventLoop()->deleteStaticPtr(ref); }
  /// Sets the per-thread static data
  type *operator=(type *v){
    FXApp::instance()->getEventLoop()->setStaticPtr(ref, static_cast<void *>(v));
    return v;
    }
  /// Returns the per-thread static data
  operator type *() const {
    return static_cast<type *>(FXApp::instance()->getEventLoop()->getStaticPtr(ref));
    }
  /// Accesses the per-thread static data
  type *operator->() const {
    return static_cast<type *>(FXApp::instance()->getEventLoop()->getStaticPtr(ref));
    }
};

}

#endif
