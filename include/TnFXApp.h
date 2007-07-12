/********************************************************************************
*                                                                               *
*                        Main TnFOX application object                          *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003-2006 by Niall Douglas.   All Rights Reserved.       *
*       NOTE THAT I DO NOT PERMIT ANY OF MY CODE TO BE PROMOTED TO THE GPL      *
*********************************************************************************
* This code is free software; you can redistribute it and/or modify it under    *
* the terms of the GNU Library General Public License v2.1 as published by the  *
* Free Software Foundation EXCEPT that clause 3 does not apply ie; you may not  *
* "upgrade" this code to the GPL without my prior written permission.           *
* Please consult the file "License_Addendum2.txt" accompanying this file.       *
*                                                                               *
* This code is distributed in the hope that it will be useful,                  *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                          *
*********************************************************************************
* $Id:                                                                          *
********************************************************************************/

#if !defined(TNFXAPP_H) && !defined(FX_DISABLEGUI)
#define TNFXAPP_H

#include "FXApp.h"
#include "QThread.h"

namespace FX {

/*! \file TnFXApp.h
\brief Defines the main TnFOX application object
*/

class TnFXAppEventLoop;


/*! \class TnFXApp
\brief The main TnFOX application object

In your TnFOX applications you should create a TnFXApp instead of a FX::FXApp
to enable the per-thread event loops enhancement TnFOX provides. This
permits you to run multiple widget trees across multiple threads concurrently.

In such an environment, appropriate use of FX::FXEventLoop_Static and
FX::FXLockHold of the application object is important. The latter only really
arises where you use the FX::FXRegistry as returned by reg() as everything
else in FXApp can be read and set in a threadsafe fashion (they are pointers
only and furthermore rarely change after initialisation).

As the event loop management methods in FXApp now automatically vector through
the return from getEventLoop(), you can literally copy & paste your code into
its new thread and everything works just fine. create(), destroy() and detach()
do the needful depending on what thread calls them. exit() shuts down all event
loops and causes the primary loop to exit. It should be a transparent
move.

Furthermore as event loops are created on demand automatically, you can
litter opens of FX::FXMessageBox or FX::FXExceptionDialog around the place and
they shall run in the thread which creates them.

<h3>Usage:</h3>
For portable behaviour, you must change your \c main() function slightly.
The form goes from:
\code
int main(int argc, char *argv[])
{
   FXProcess myprocess(argc, argv);
   FXApp myapp("MyApp");
   myapp.init(argc, argv);
   FXMainWindow *mainwnd=new FXMainWindow(&myapp, "My Main Window");
   <widgets inside mainwnd>
   ...
   myapp.create()
   return myapp.run();
}
\endcode
to:
\code
class MyPrimaryLoop : public TnFXAppEventLoop
{
   FXMainWindow *mainwnd;
   <widgets inside mainwnd>
public:
   MyPrimaryLoop(TnFXApp *app) : TnFXAppEventLoop(app) { }
   FXint execute(FXApp *app)
   {
      mainwnd=new FXMainWindow(app, "My Main Window");
      <widgets inside mainwnd>
      ...
      mainwnd->show();
      app->create();
      return app->run();
   }
   // May want to override cleanup() here
};

int main(int argc, char *argv[])
{
   FXProcess myprocess(argc, argv);
   TnFXApp myapp("MyApp");
   myapp.init(argc, argv);
   MyPrimaryLoop primaryLoop(&myapp);
   return myapp.run(primaryLoop);
}
\endcode
The reason for this is that on X11 per-thread event queues must be emulated
and to prevent all threads deadlocking when the primary thread is busy, you
must execute the application's core code separately.

\warning Neither TnFXApp nor any event loops it returns are threadsafe apart
from the postAsyncMessage() function. Do NOT use an event loop not belonging
to your thread and if you use TnFXApp::reg(), lock TnFXApp first.

<h3>Signals:</h3>
Signals are a real hash with POSIX threads. Each thread maintains a signal
mask which determines which signals it can received but signal handlers
are on a per-thread basis. Thus the FOX signal intercept can be called in
any thread but it should post an asynchronous message invoking the handler
in the correct event loop.

<h3>Implementation:</h3>
You should probably know something about the implementation so you can
correctly diagnose and avoid any issues which may arise. On Windows the
implementation is extremely simple as Windows already provides per-thread
event queues whereby messages are posted to the thread which created the
window. This makes implementation on that platform trivial.

On X11 it gets more complicated as there's one event queue only. Therefore
the primary thread must fetch X11 events, determine which event loop they
belong to and post them to that loop for handling. Obviously to prevent
deadlock the primary dispatch thread must always be available. Also
because of the event loops in each thread being totally different
implementations, internally TnFXApp subclasses FX::FXEventLoop twice -
one for the the main X11 dispatch loop and the other for all other threads.
You can always retrieve the event loop which quits the application using
FXEventLoop::primaryLoop().

Watches on file descriptors are not performed the same way
on Windows and X11. On Windows, the event loop local list is monitored
as it monitors the thread message queue so all is well. On X11 it's much
more complex as POSIX won't let you wait on a wait condition and kernel
handles simultaneously, so the primary loop coalesces all the file
descriptors and posts asynchronous messages when they signal.
\note File descriptor watching on POSIX is not implemented currently. It
wouldn't be hard to add, but I personally have no use for it and I view
it as a pure legacy feature.
*/
struct TnFXAppPrivate;
class FXAPI TnFXApp : public QMutex, public FXApp
{
	FXDECLARE(TnFXApp)
	TnFXAppPrivate *p;
	TnFXApp(const TnFXApp &);
	TnFXApp &operator=(const TnFXApp &);
public:
	//! Constructs the application object
	TnFXApp(const FXString &name="Application", const FXString &vendor="FoxDefault");
	~TnFXApp();
	virtual FXEventLoop* getEventLoop() const;
public:
	long onCmdQuit(FXObject*,FXSelector,void*);
public:
	static TnFXApp *instance() { return (TnFXApp *) FXApp::instance(); }
	virtual void create();
	virtual void destroy();
	virtual void detach();
	virtual void init(int& argc,char** argv,bool connect=TRUE);
	/*! Threadsafe way to immediately end the application. Closes down all
	event loops and returns back into main() returning \em code */
	virtual void exit(FXint code=0);
	virtual void lock();
	virtual void unlock();

	/*! Runs the application, returning when all event loops have terminated
	or exit() was called */
	FXint run(TnFXAppEventLoop &primaryLoop);
};


/*! \class TnFXAppEventLoop
\brief The base class for an event loop

Convenience base class for an event loop. You don't need to use this
except for the primary loop.

\sa FX::TnFXApp
*/
struct TnFXAppEventLoopPrivate;
class FXAPI TnFXAppEventLoop : public QMutex, public FXObject, public QThread
{
	FXDECLARE_ABSTRACT(TnFXAppEventLoop)
	TnFXAppEventLoopPrivate *p;
	TnFXAppEventLoop(const TnFXAppEventLoop &);
	TnFXAppEventLoop &operator=(const TnFXAppEventLoop &);
public:
	enum
	{
		ID_LAST=0
	};
public:
	//! Constructs an event loop
	TnFXAppEventLoop(const char *threadname="Event Loop thread", TnFXApp *app=TnFXApp::instance());
	~TnFXAppEventLoop();
	//! Returns the application object associated with this loop
	TnFXApp *getApp() const;
	//! Returns the event loop associated with this loop (unavailable until after thread is started)
	FXEventLoop *getEventLoop() const;
protected:
	//! Defined by you to execute the main loop of the thread
	virtual FXint execute(FXApp *app)=0;
	virtual void run();
	virtual void *cleanup();
	//! Returns and lets you alter the return code returned by execute()
	void *&executeRetCode();
};

} // namespace

#endif
