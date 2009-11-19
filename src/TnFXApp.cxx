/********************************************************************************
*                                                                               *
*                        Main TnFOX application object                          *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003 by Niall Douglas.   All Rights Reserved.            *
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

#include "xincs.h"
#include "TnFXApp.h"
#include "FXRollback.h"
#include "FXExceptionDialog.h"
#include <qptrlist.h>
#include <qvaluelist.h>
#include <qptrdict.h>

#include "FXMemDbg.h"
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

#ifdef USE_POSIX
#define MANUAL_DISPATCH
#endif

namespace FX {

// Callback Record from FXApp
struct FXCBSpec {
  FXObject      *target;            // Receiver object
  FXSelector     message;           // Message sent to receiver
  };

// Timer record from FXApp
struct FXTimer {
  FXTimer       *next;              // Next timeout in list
  FXObject      *target;            // Receiver object
  void          *data;              // User data
  FXSelector     message;           // Message sent to receiver
  FXlong         due;               // When timer is due (ns)
  };

// Input record from FXApp
struct FXInput {
  FXCBSpec       read;              // Callback spec for read
  FXCBSpec       write;             // Callback spec for write
  FXCBSpec       excpt;             // Callback spec for except
  };


// Holds the event loop for the calling thread
static QThreadLocalStorage<FXEventLoop> myeventloop;
// Deletes the same as a thread cleanup
static void deleteEventLoop(FXEventLoop *el)
{
	delete el;
}

// Base event loop
class FXDLLLOCAL EventLoopBase : public QMutex, public FXEventLoop
{
	FXDECLARE(EventLoopBase)
	QThread *creator;
protected:
	virtual void* makeStaticPtr(void* val) { return new QThreadLocalStorageBase(val); }
	virtual void deleteStaticPtr(void* ref) { delete (QThreadLocalStorageBase *) ref; }
	virtual void* getStaticPtr(const void* ref) const
	{
		QThreadLocalStorageBase *tls=(QThreadLocalStorageBase *) ref;
		return tls->getPtr();
	}
	virtual void setStaticPtr(void* ref,void* val)
	{
		QThreadLocalStorageBase *tls=(QThreadLocalStorageBase *) ref;
		tls->setPtr(val);
	}
	virtual void latchLoop()
	{	// Stop cancellation
		QThread_DTHold dth;
		FXEventLoop::latchLoop();
	}
	virtual void resetLoopLatch()
	{	// Stop cancellation
		QThread_DTHold dth;
		FXEventLoop::resetLoopLatch();
	}
public:
	EventLoopBase(TnFXApp *app=NULL) : FXEventLoop(app), creator(QThread::current()) { }
	void setup() { FXEventLoop::setup(); }
	virtual void lock() { QMutex::lock(); }
	virtual void unlock() { QMutex::unlock(); }
	virtual FXulong getThreadId() const { return creator->myId(); }
};
FXIMPLEMENT(EventLoopBase,FXEventLoop,NULL,0)

// Client event loop
class FXDLLLOCAL EventLoopC : public EventLoopBase
{
	friend class EventLoopP;
	FXDECLARE(EventLoopC)
protected:
	QWaitCondition newevent;
	QValueList<FXRawEvent> events;
protected:
#ifndef WIN32
	virtual void latchLoop() { newevent.wakeAll(); }
	virtual void resetLoopLatch() { /* Do nothing */ }
#endif
	virtual bool getNextEventI(FXRawEvent& ev,bool blocking);
	virtual bool peekEventI();
    virtual bool dispatchEvent(FXRawEvent& ev);
public:
	EventLoopC(TnFXApp *app=NULL);
	~EventLoopC();
};

// Primary event loop
class FXDLLLOCAL EventLoopP : public EventLoopBase
{
	friend class TnFXApp;
	FXDECLARE(EventLoopP)
	QPtrList<EventLoopC> eventLoops;
	QWaitCondition noEventLoops;
protected:
	virtual bool getNextEventI(FXRawEvent& ev,bool blocking);
    virtual bool dispatchEvent(FXRawEvent& ev);
public:
	EventLoopP(TnFXApp *app=NULL);
	void clientNew(EventLoopC *loop);
	void clientDie(EventLoopC *loop);
};

//*************************************************************************

// Primary event loop
FXIMPLEMENT(EventLoopP,EventLoopBase,NULL,0)

EventLoopP::EventLoopP(TnFXApp *app) : EventLoopBase(app)
{
}

bool EventLoopP::getNextEventI(FXRawEvent& ev,bool blocking)
{
	FXEXCEPTION_FOXCALLING1 {
#ifndef MANUAL_DISPATCH
		return FXEventLoop::getNextEventI(ev, blocking);
#else
		// TODO: File descriptor import
		if(!FXEventLoop::getNextEventI(ev, blocking)) return false;
		//fxmessage("Primary received msg %d, repaints=%p, refresher=%p\n", ev.xany.type, repaints, refresher);
		return true;
#endif
	} FXEXCEPTION_FOXCALLING2;
	return false;
}

bool EventLoopP::dispatchEvent(FXRawEvent& ev)
{
	FXEXCEPTION_FOXCALLING1 {
#ifndef MANUAL_DISPATCH
		return FXEventLoop::dispatchEvent(ev);
#else
		QMtxHold h(this);
		EventLoopC *el;
		//fxmessage("Primary dispatching msg %d, repaints=%p, refresher=%p\n", ev.xany.type, repaints, refresher);
		for(QPtrListIterator<EventLoopC> it(eventLoops); (el=it.current()); ++it)
		{
			h.unlock();
			QMtxHold h2(el);
			if(el->hash.find((void *) ev.xany.window))
			{
				//fxmessage("Event belongs to loop 0x%p: %s\n", el, fxdump32((FXuint*)&ev, sizeof(FXRawEvent)/4).text());
				el->events.push_back(ev);
				el->newevent.wakeAll();
				return true;
			}
			h2.unlock();
			h.relock();
		}
		// Does it belong to me?
		if(hash.find((void *) ev.xany.window))
		{
			h.unlock();
#ifdef DEBUG
			fxwarning("WARNING: It's dangerous to run windows in the main thread\n");
#endif
			return FXEventLoop::dispatchEvent(ev);
		}
#ifdef DEBUG
		if(KeymapNotify!=ev.xany.type)
			fxmessage("WARNING: Failed to find event loop for msg %d to window %d, repaints=%p, refresher=%p\n", ev.xany.type, (int) ev.xany.window, repaints, refresher);
#endif
		return true;
#endif
	} FXEXCEPTION_FOXCALLING2;
	return FALSE;
}

void EventLoopP::clientNew(EventLoopC *loop)
{
	FXEXCEPTION_FOXCALLING1 {
#ifdef DEBUG
		fxmessage("Received notification of creation of loop 0x%p\n", loop);
#endif
		QMtxHold h(this);
		eventLoops.append(loop);
	} FXEXCEPTION_FOXCALLING2;
}
void EventLoopP::clientDie(EventLoopC *loop)
{
	FXEXCEPTION_FOXCALLING1 {
		QMtxHold h(this);
		eventLoops.removeRef(loop);
#ifdef DEBUG
		fxmessage("Received notification of death of loop 0x%p, %d to go\n", loop, eventLoops.count());
#endif
		if(eventLoops.isEmpty())
		{
			noEventLoops.wakeAll();
			stop(0);		// And exit
			latchLoop();
		}
	} FXEXCEPTION_FOXCALLING2;
}

//*************************************************************************

// Client event loop
FXIMPLEMENT(EventLoopC,EventLoopBase,NULL,0)

EventLoopC::EventLoopC(TnFXApp *app) : EventLoopBase(app)
{	// Don't need this latch so delete
#ifndef WIN32
	if(latch[1]) { close(latch[1]); latch[1]=0; }
	if(latch[0]) { close(latch[0]); latch[0]=0; }
#endif
	static_cast<EventLoopP *>(FXApp::getPrimaryEventLoop())->clientNew(this);
}
EventLoopC::~EventLoopC()
{ FXEXCEPTIONDESTRUCT1 {
	static_cast<EventLoopP *>(FXApp::getPrimaryEventLoop())->clientDie(this);
} FXEXCEPTIONDESTRUCT2; }

bool EventLoopC::getNextEventI(FXRawEvent& ev,bool blocking)
{
	FXEXCEPTION_FOXCALLING1 {
#ifndef MANUAL_DISPATCH
		// Very easy on Win32 which implements per-thread event queues for us
		return FXEventLoop::getNextEventI(ev, blocking);
#else
		QMtxHold h(this);
		if(events.isEmpty())
		{
			FXuint wait=FXINFINITE;
			if(!blocking) return false;
			newevent.reset();
			h.unlock();
			if(getApp()->isInitialized())
			{	// Need to do this manually as it's async from XNextEvent()
				int eventstogo=XEventsQueued((Display*)display,QueuedAfterFlush);
#ifdef DEBUG
				//fxmessage("Thread %luu flushing X11 queue containing %d events\n", QThread::id(), eventstogo);
#endif
				XFlush((Display*) display);
			}
			if(timers)
			{
				FXint togo=(FXint)((timers->due-FXApp::time())/1000000);
				if(togo<=0) return false;
				wait=togo;
			}
			newevent.wait(wait);
			h.relock();
			if(events.isEmpty()) return false;
		}
		ev=events.front();
		//fxmessage("Returning event %s\n", fxdump32((FXuint*)&ev, sizeof(FXRawEvent)/4).text());
		events.pop_front();
		//fxmessage("Client received msg %d, repaints=%p, refresher=%p\n", ev.xany.type, repaints, refresher);
		return true;
#endif
	} FXEXCEPTION_FOXCALLING2;
	return false;
}

bool EventLoopC::peekEventI()
{
	FXEXCEPTION_FOXCALLING1 {
#ifndef MANUAL_DISPATCH
		return FXEventLoop::peekEventI();
#else
		QMtxHold h(this);
		return !events.isEmpty();
#endif
	} FXEXCEPTION_FOXCALLING2;
	return false;
}

bool EventLoopC::dispatchEvent(FXRawEvent& ev)
{
	//fxmessage("Client dispatching msg %d, repaints=%p, refresher=%p\n", ev.xany.type, repaints, refresher);
#if defined(DEBUG) && defined(MANUAL_DISPATCH)
	bool ret;
	FXulong start=FXProcess::getNsCount();
	ret=FXEventLoop::dispatchEvent(ev);
	FXulong end=FXProcess::getNsCount();
	//fxmessage("EventLoopC: Dispatching event %d took %f secs\n", ev.xany.type, (end-start)/1000000000.0);
	return ret;
#else
	return FXEventLoop::dispatchEvent(ev);
#endif
}

//*************************************************************************

struct FXDLLLOCAL TnFXAppPrivate
{
	EventLoopP *primaryLoop;
	bool inExit;
	TnFXAppPrivate() : primaryLoop(0), inExit(false) { }
};

FXDEFMAP(TnFXApp) TnFXAppMap[]={
  FXMAPFUNC(SEL_TIMEOUT,TnFXApp::ID_QUIT,TnFXApp::onCmdQuit),
  FXMAPFUNC(SEL_SIGNAL, TnFXApp::ID_QUIT,TnFXApp::onCmdQuit),
  FXMAPFUNC(SEL_CHORE,  TnFXApp::ID_QUIT,TnFXApp::onCmdQuit),
  FXMAPFUNC(SEL_COMMAND,TnFXApp::ID_QUIT,TnFXApp::onCmdQuit),
};

FXIMPLEMENT(TnFXApp,FXApp,TnFXAppMap,ARRAYNUMBER(TnFXAppMap))

TnFXApp::TnFXApp(const FXString &name, const FXString &vendor) : FXApp(name, vendor), p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new TnFXAppPrivate);
	FXDELETE(eventLoop);
	myeventloop=p->primaryLoop=new EventLoopP(this);
	p->primaryLoop->setup();
	unconstr.dismiss();
}
TnFXApp::~TnFXApp()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p->primaryLoop);
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

FXEventLoop *TnFXApp::getEventLoop() const
{
	FXEventLoop *ml=0;
	FXEXCEPTION_FOXCALLING1 {
		if(eventLoop) return eventLoop; // During init
		ml=myeventloop;
		if(!ml)
		{
			EventLoopC *el;
			FXERRHM(myeventloop=ml=el=new EventLoopC(const_cast<TnFXApp *>(this)));
			QThread::current()->addCleanupCall(Generic::BindFuncN(deleteEventLoop, el), true);
			el->setup();
#ifdef DEBUG
			fxmessage("Created event loop 0x%p in thread %u\n", el, (FXuint) QThread::id());
#endif
		}
	} FXEXCEPTION_FOXCALLING2;
	return ml;
}

long TnFXApp::onCmdQuit(FXObject*,FXSelector,void*)
{	// Exit loop
#ifdef DEBUG
	fxmessage("Loop 0x%p in thread %u wants to quit\n", getEventLoop(), (FXuint) QThread::id());
#endif
	stop(0); // Stop the calling event loop
	return 1;
}

void TnFXApp::create()
{
	QMtxHold h(this);
	FXApp::create();
}

void TnFXApp::destroy()
{
	QMtxHold h(this);
	FXApp::destroy();
}

void TnFXApp::detach()
{
	QMtxHold h(this);
	FXApp::detach();
}

void TnFXApp::init(int &argc, char **argv, bool connect)
{
	QMtxHold h(this);
	FXApp::init(argc, argv, connect);
}

void TnFXApp::exit(FXint code)
{
	if(getEventLoop()!=p->primaryLoop)
	{	// Ask primary loop to quit
		if(!p->inExit) p->primaryLoop->postAsyncMessage(this, FXSEL(SEL_COMMAND, ID_QUIT));
		stop(code);
		return;
	}
	if(!p->inExit)
	{
		p->inExit=true;
		QMtxHold h(p->primaryLoop);
		EventLoopC *el;
		for(QPtrListIterator<EventLoopC> it(p->primaryLoop->eventLoops); (el=it.current()); ++it)
		{
			h.unlock();
			el->postAsyncMessage(this, FXSEL(SEL_COMMAND, ID_QUIT));
			h.relock();
		}
		h.unlock();
		if(!p->primaryLoop->eventLoops.isEmpty())
			p->primaryLoop->noEventLoops.wait();
		// All event loops have now exited
		{
			QMtxHold h(this);
			FXApp::exit(code);
			p->inExit=false;
		}
	}
}

void TnFXApp::lock()
{ FXEXCEPTION_FOXCALLING1 {
	QMutex::lock();
} FXEXCEPTION_FOXCALLING2; }
void TnFXApp::unlock()
{ FXEXCEPTION_FOXCALLING1 {
	QMutex::unlock();
} FXEXCEPTION_FOXCALLING2; }

FXint TnFXApp::run(TnFXAppEventLoop &pl)
{
	pl.start(true);
	return p->primaryLoop->run();
}

//************************************************************************

struct FXDLLLOCAL TnFXAppEventLoopPrivate
{
	TnFXApp *app;
	FXEventLoop *eventLoop;
	void *retcode;
	TnFXAppEventLoopPrivate(TnFXApp *_app) : app(_app), eventLoop(0), retcode(0) { }
	void resetEventLoop()
	{
		eventLoop=0;
	}
};
FXIMPLEMENT_ABSTRACT(TnFXAppEventLoop,FXObject,NULL,0)

TnFXAppEventLoop::TnFXAppEventLoop(const char *threadname, TnFXApp *app) : QThread(threadname), p(0)
{
	FXERRHM(p=new TnFXAppEventLoopPrivate(app));
}
TnFXAppEventLoop::~TnFXAppEventLoop()
{ FXEXCEPTIONDESTRUCT1 {
	if(running() && !inCleanup())
	{
		p->eventLoop->postAsyncMessage(getApp(), TnFXApp::ID_QUIT);
		wait();
	}
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }
TnFXApp *TnFXAppEventLoop::getApp() const
{
	return p->app;
}
FXEventLoop *TnFXAppEventLoop::getEventLoop() const
{
	return p->eventLoop;
}

void TnFXAppEventLoop::run()
{	// We must assume that if they're using this class at all it'll cause
	// an event loop to be created at some point
	addCleanupCall(Generic::BindObjN(*p, &TnFXAppEventLoopPrivate::resetEventLoop), true);
	p->eventLoop=p->app->getEventLoop();
	p->retcode=(void *) execute(p->app);
}
void *TnFXAppEventLoop::cleanup()
{
	return p->retcode;
}

void *&TnFXAppEventLoop::executeRetCode()
{
	return p->retcode;
}

} // namespace
