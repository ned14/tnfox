/********************************************************************************
*                                                                               *
*                 M u l i t h r e a d i n g   S u p p o r t                     *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2002-2004 by Niall Douglas.   All Rights Reserved.       *
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

#define FXBEING_INCLUDED_BY_QTHREAD 1
#include "QThread.h"
// Include the FXAtomicInt and QMutex implementations as they
// are disabled by FXBEING_INCLUDED_BY_QTHREAD
#include "int_QMutexImpl.h"
#undef FXBEING_INCLUDED_BY_QTHREAD

namespace FX { namespace QMutexImpl {

// Globals
KernelWaitObjectCache waitObjectCache;
bool yieldAfterLock=false;
FXuint systemProcessors;

} }

#include <qvaluelist.h>
#include <qptrlist.h>
#include <qptrdict.h>
#include <qsortedlist.h>
#include "QTrans.h"
#include "FXApp.h"
#include "FXProcess.h"
#include "FXPtrHold.h"
#ifdef USE_POSIX
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <errno.h>
#endif
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

#ifdef USE_WINAPI
#define DECLARERET int ret=1
#define ERRCHK FXERRHWIN(ret)
#elif defined(USE_POSIX)
#define DECLARERET int ret=0
#define ERRCHK FXERRHOS(ret)
#else
#define DECLARERET
#define ERRCHK
#endif

namespace FX {

class QWaitConditionPrivate : public QMutex
{
public:
	int waitcnt;
#ifdef USE_WINAPI
	HANDLE wc;
#endif
#ifdef USE_POSIX
	pthread_mutex_t m;
	pthread_cond_t wc;
#endif
	QWaitConditionPrivate() : waitcnt(0), QMutex() { }
};

QWaitCondition::QWaitCondition(bool autoreset, bool initialstate) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QWaitConditionPrivate);
#ifdef USE_WINAPI
	FXERRHWIN(p->wc=CreateEvent(NULL, FALSE, initialstate, NULL));
#endif
#ifdef USE_POSIX
	FXERRHOS(pthread_mutex_init(&p->m, NULL));
	FXERRHOS(pthread_cond_init(&p->wc, NULL));
#endif
	isAutoReset=autoreset;
	isSignalled=initialstate;
	unconstr.dismiss();
}

QWaitCondition::~QWaitCondition()
{ FXEXCEPTIONDESTRUCT1 {
	if(p)
	{
#ifdef USE_WINAPI
		FXERRHWIN(CloseHandle(p->wc));
		p->wc=0;
#endif
#ifdef USE_POSIX
		FXERRHOS(pthread_cond_destroy(&p->wc));
		FXERRHOS(pthread_mutex_destroy(&p->m));
#endif
		FXDELETE(p);
	}
} FXEXCEPTIONDESTRUCT2; }

bool QWaitCondition::wait(FXuint time)
{
#ifndef FXDISABLE_THREADS
	if(p)
	{
		DECLARERET;
		FXMtxHold h(p);
		if(isSignalled)
		{
			if(isAutoReset) isSignalled=false;
			return true;
		}
		else p->waitcnt++;
#ifdef DEBUG
		if(QThread::current()->inCleanup())
			fxwarning("WARNING: Calling QWaitCondition::wait() from a cleanup handler is non-portable\n");
#endif
#ifdef USE_WINAPI
		{
			HANDLE waitlist[2];
			waitlist[0]=p->wc;
			waitlist[1]=QThread::int_cancelWaiterHandle();
			ret=ResetEvent(p->wc);
			h.unlock();
			if(!ret) goto exit;
			ret=WaitForMultipleObjects(2, waitlist, FALSE, (FXINFINITE!=time) ? time : INFINITE);
		}
		if(ret==WAIT_OBJECT_0+1) QThread::current()->checkForTerminate();
exit:
		h.relock();
		p->waitcnt--;
		if(ret!=WAIT_TIMEOUT && ret!=WAIT_OBJECT_0 && ret!=WAIT_OBJECT_0+1)
		{
			FXERRHWIN(0);
		}
		if(isAutoReset)
			isSignalled=false;
		return (WAIT_OBJECT_0==ret);
#endif
#ifdef USE_POSIX
		pthread_mutex_lock(&p->m);	// Stop a wake between lock below and wait
		h.unlock();
		if(FXINFINITE!=time)
		{
			struct timespec ts;
			struct timeval tv;
			FXProcess::getTimeOfDay(&tv);
			ts.tv_sec=tv.tv_sec+(time/1000);
			ts.tv_nsec=(tv.tv_usec*1000)+(time % 1000)*1000000;
			ret=pthread_cond_timedwait(&p->wc, &p->m, &ts);
		}
		else ret=pthread_cond_wait(&p->wc, &p->m);
		pthread_mutex_unlock(&p->m);
		h.relock();
		p->waitcnt--;
		if(ret && ETIMEDOUT!=ret)
		{
			FXERRHOS(ret);
		}
		if(isAutoReset)
			isSignalled=false;
		return (ETIMEDOUT!=ret);
#endif
	}
#endif
	return true;
}

void QWaitCondition::wakeOne()
{
#ifndef FXDISABLE_THREADS
	if(p)
	{
		DECLARERET;
		FXMtxHold h(p);
		if(p->waitcnt)
		{
#ifdef USE_WINAPI
			ret=SetEvent(p->wc);
#endif
#ifdef USE_POSIX
			pthread_mutex_lock(&p->m);
			ret=pthread_cond_signal(&p->wc);
			pthread_mutex_unlock(&p->m);
#endif
		}
		ERRCHK;
	}
#endif
}

void QWaitCondition::wakeAll()
{
#ifndef FXDISABLE_THREADS
	if(p)
	{
		DECLARERET;
		FXMtxHold h(p);
		if(p->waitcnt)
		{
#ifdef USE_WINAPI
			int waitcnt=p->waitcnt;
			for(int n=0; n<waitcnt; n++)
			{
				if(!(ret=SetEvent(p->wc))) goto exit;
			}
#endif
#ifdef USE_POSIX
			pthread_mutex_lock(&p->m);
			ret=pthread_cond_broadcast(&p->wc);
			pthread_mutex_unlock(&p->m);
			if(ret) goto exit;
#endif
		}
exit:
		if(!p->waitcnt || !isAutoReset)
			isSignalled=true;	// Let next thread through
		ERRCHK;
	}
#endif
}

void QWaitCondition::reset()
{
#ifndef FXDISABLE_THREADS
	if(p)
	{
		FXMtxHold h(p);
		isSignalled=false;
	}
#endif
}


/**************************************************************************************************************/
class QRWMutexPrivate : public QMutex
{
#ifndef FXDISABLE_THREADS
public:
	struct ReadInfo
	{
		volatile int count;
	} read;
	QWaitCondition readcntZeroed;
	struct PreWriteInfo
	{
		volatile int count, rws;
	} prewrite;
	QWaitCondition prewritecntZeroed;
	struct WriteInfo
	{
		FXulong threadid;
		volatile int count;
		bool readLockLost;
	} write;
	QWaitCondition writecntZeroed;
	QThreadLocalStorageBase myreadcnt;
	QRWMutexPrivate() : readcntZeroed(false, false), prewritecntZeroed(false, false), writecntZeroed(false, false), QMutex() { }
	FXuint readCnt() { return (FXuint)(FXuval) myreadcnt.getPtr(); }
	void setReadCnt(FXuint v) { myreadcnt.setPtr((void *) v); }
	void incReadCnt() { setReadCnt(readCnt()+1); }
	void decReadCnt() { setReadCnt(readCnt()-1); }
#endif
};

QRWMutex::QRWMutex() : p(0)
{
	FXERRHM(p=new QRWMutexPrivate);
#ifndef FXDISABLE_THREADS
	p->read.count=0;
	p->prewrite.count=0;
	p->prewrite.rws=0;

	p->write.threadid=0;
	p->write.count=0;
	p->write.readLockLost=false;
#endif
}

QRWMutex::~QRWMutex()
{ FXEXCEPTIONDESTRUCT1 {
	if(p)
	{
		lock(true);
		FXMtxHold h(p);
		QRWMutexPrivate *_p=p;
		p=0;
		h.unlock();
		FXDELETE(_p);
	}
} FXEXCEPTIONDESTRUCT2; }

FXuint QRWMutex::spinCount() const
{
	if(p)
		return p->spinCount();
	else
		return 0;
}

void QRWMutex::setSpinCount(FXuint c)
{
	if(p) p->setSpinCount(c);
}

/* ned 5th Nov 2002: Third attempt at this algorithm ... :)

Problem with old algorithm was it was starving writers when lots
of stuff was reading and that was causing poor performance. This new algorithm uses
thread local storage to replace the old array so I've also removed the limit on max
threads.
*/
inline bool QRWMutex::_lock(FXMtxHold &h, bool write)
{
	bool lockLost=false;
#ifndef FXDISABLE_THREADS
	FXulong myid=QThread::id();
	if(write)
	{
		if(p->write.threadid!=myid)
		{	// Wait for other writers
			bool otherRWlock=false;		// Becomes true if other threads already with a read lock are waiting for a write
			FXuint thisthreadreadcnt=0;
			p->prewrite.count++;
			do
			{
				while(p->write.count)
				{
					h.unlock();
					QThread::current()->disableTermination();
					p->writecntZeroed.wait();
					QThread::current()->enableTermination();
					h.relock();
				}
				// Wait for readers to exit, except those read locks I hold
				thisthreadreadcnt=p->readCnt();
				p->read.count-=thisthreadreadcnt;
				p->prewrite.rws+=thisthreadreadcnt;
				while(p->read.count)
				{
					h.unlock();
					QThread::current()->disableTermination();
					p->readcntZeroed.wait();
					QThread::current()->enableTermination();
					h.relock();
				}
				p->read.count+=thisthreadreadcnt;
				p->prewrite.rws-=thisthreadreadcnt;
				otherRWlock=(p->prewrite.rws)!=0;
			} while(p->write.count);
			// Ok, now it's mine
			assert(!p->write.threadid);
			p->write.threadid=myid;
			lockedstate=ReadWrite;
			if(thisthreadreadcnt)
			{	// If I'm asking for write lock already holding read lock, see what the other fellah did
				lockLost=p->write.readLockLost;
				p->write.readLockLost=false;
			}
			if(otherRWlock) p->write.readLockLost=true; // Tell next nested write requester that I have altered the data
		}
		p->write.count++;
	}
	else
	{	// Wait for all writers and anything else waiting to write (give writers preference)
		// unless I already have a read lock or a write lock
		if(!p->readCnt() && p->write.threadid!=myid)
		{

			while(p->prewrite.count)
			{
				h.unlock();
				QThread::current()->disableTermination();
				p->prewritecntZeroed.wait();
				QThread::current()->enableTermination();
				h.relock();
			}
			lockedstate=ReadOnly;
		}
		p->read.count++;
		p->incReadCnt();
	}
#endif
	return lockLost;
}

void QRWMutex::unlock(bool write)
{
#ifndef FXDISABLE_THREADS
	if(p)
	{
		FXMtxHold h(p);
		if(write)
		{
#ifdef DEBUG
			FXulong myid=QThread::id();
			FXERRH(p->write.threadid==myid, "QRWMutex::unlock(true) called by thread which did not have write lock", FXRWMUTEX_BADUNLOCK, FXERRH_ISDEBUG);
#endif
			// Release writers before readers (makes no difference on POSIX though)
			if(!--p->write.count)
			{
				p->writecntZeroed.wakeAll();
				lockedstate=(p->readCnt()) ? ReadOnly : Unlocked;
				p->write.threadid=0;
			}
			if(p->prewrite.count)
			{
				if(!--p->prewrite.count)
				{
					p->prewritecntZeroed.wakeAll();
				}
			}
		}
		else
		{
#ifdef DEBUG
			FXERRH(p->readCnt(), "QRWMutex::unlock(false) called by thread which did not have read lock", FXRWMUTEX_BADUNLOCK, FXERRH_ISDEBUG);
#endif
			p->decReadCnt();
			if(!--p->read.count)
			{
				p->readcntZeroed.wakeAll();
				lockedstate=(p->write.count) ? ReadWrite : Unlocked;
			}
		}
	}
#endif
}

bool QRWMutex::lock(bool write)
{
#ifndef FXDISABLE_THREADS
	if(p)
	{
		FXMtxHold h(p);
		return _lock(h, write);
	}
#endif
	return false;
}

bool QRWMutex::trylock(bool write)
{
#ifdef FXDISABLE_THREADS
	return true;
#else
	bool ret=false;
	if(p)
	{
		FXMtxHold h(p);
		if(!p->write.count)
		{
			_lock(h, write);
			ret=true;
		}
	}
	return ret;
#endif
}

/**************************************************************************************************************/
class QThreadLocalStorageBasePrivate
{
public:
#ifdef FXDISABLE_THREADS
	void *data;
#endif
#ifdef USE_WINAPI
	DWORD key;
#endif
#ifdef USE_POSIX
	pthread_key_t tls;
#endif
};

QThreadLocalStorageBase::QThreadLocalStorageBase(void *initval) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QThreadLocalStorageBasePrivate);
#ifdef FXDISABLE_THREADS
	p->data=initval;
#endif
#ifdef USE_WINAPI
	p->key=TlsAlloc();
	FXERRH(p->key!=(DWORD) -1, QTrans::tr("QThreadLocalStorage", "Exceeded Windows TLS hard key number limit"), QTHREADLOCALSTORAGE_NOMORETLS, FXERRH_ISDEBUG);
	FXERRHWIN(TlsSetValue(p->key, initval));
#endif
#ifdef USE_POSIX
	FXERRHOS(pthread_key_create(&p->tls, NULL));
	FXERRHOS(pthread_setspecific(p->tls, initval));
#endif
	unconstr.dismiss();
}


QThreadLocalStorageBase::~QThreadLocalStorageBase() FXERRH_NODESTRUCTORMOD
{
	if(p)
	{
#ifdef USE_WINAPI
		FXERRHWIN(TlsFree(p->key));
#endif
#ifdef USE_POSIX
		FXERRHOS(pthread_key_delete(p->tls));
#endif
		FXDELETE(p);
	}
}

void QThreadLocalStorageBase::setPtr(void *val)
{
	if(p)
	{
#ifdef FXDISABLE_THREADS
		p->data=val;
#endif
#ifdef USE_WINAPI
		FXERRHWIN(TlsSetValue(p->key, val));
#endif
#ifdef USE_POSIX
		FXERRHOS(pthread_setspecific(p->tls, val));
#endif
	}
}

void *QThreadLocalStorageBase::getPtr() const
{
#ifdef FXDISABLE_THREADS
	if(p)
		return p->data;
#endif
#ifdef USE_WINAPI
	if(p)
		return TlsGetValue(p->key);
#endif
#ifdef USE_POSIX
	if(p)
		return pthread_getspecific(p->tls);
#endif
	return 0;
}

/**************************************************************************************************************/
class QThreadPrivate : public QMutex
{
public:
	void **result;
#ifdef USE_WINAPI
	HANDLE threadh;
	HANDLE plsCancelWaiter;
	bool plsCancelDisabled;
#endif
#ifdef USE_POSIX
	pthread_t threadh;
#endif
	bool plsCancel;
	bool autodelete;
	FXuval stackSize;
	QThread::ThreadScheduler threadLocation;
	FXulong id;
	QThread *creator;
	QWaitCondition *startedwc, *stoppedwc;
	struct CleanupCall
	{
		Generic::BoundFunctorV *code;
		bool inThread;
		CleanupCall(Generic::BoundFunctorV *_code, bool _inThread) : code(_code), inThread(_inThread) { }
		~CleanupCall() { FXDELETE(code); }
	};
	QPtrList<CleanupCall> cleanupcalls;
	static QThreadLocalStorage<QThread> currentThread;
	static FXAtomicInt idlistlock;
	static QValueList<FXushort> idlist;
	static void setResultAddr(QThread *t, void **res) { t->p->result=res; }
	static void run(QThread *t);
	static void cleanup(QThread *t);
	static void forceCleanup(QThread *t);
	QThreadPrivate(bool autodel, FXuval stksize, QThread::ThreadScheduler threadloc)
		: autodelete(autodel), plsCancel(false), stackSize(stksize), threadLocation(threadloc), startedwc(0), stoppedwc(0), cleanupcalls(true), QMutex()
#ifdef USE_POSIX
		, threadh(0)
#endif
#ifdef USE_WINAPI
		, threadh(0), plsCancelDisabled(false), plsCancelWaiter(0)
#endif
	{
		creator=currentThread;
#ifdef USE_WINAPI
		FXERRHWIN(NULL!=(plsCancelWaiter=CreateEvent(NULL, FALSE, FALSE, NULL)));
#endif
	}
	~QThreadPrivate()
	{
#ifdef USE_WINAPI
		if(threadh)
		{
			FXERRHWIN(CloseHandle(threadh));
			threadh=0;
		}
		if(plsCancelWaiter)
		{
			FXERRHWIN(CloseHandle(plsCancelWaiter));
			plsCancelWaiter=0;
		}
#endif
#ifdef USE_POSIX
		if(threadh)
		{	// Frees resources
			void *result;
			FXERRHOS(pthread_join(threadh, &result));
			threadh=0;
		}
#endif
		FXDELETE(stoppedwc);
	}
};
// Used by methods to know what thread they're in at any given time
QThreadLocalStorage<QThread> QThreadPrivate::currentThread;
#ifdef USE_OURTHREADID
FXAtomicInt QThreadPrivate::idlistlock;
QValueList<FXushort> QThreadPrivate::idlist;
#endif
static QThread *primaryThread;
// An internal dummy QThread used to make primary thread code work correctly
class FXPrimaryThread : public QThread
{
	static void zeroCThread()
	{
		QThreadPrivate::currentThread=0;
	}
public:
	FXPrimaryThread() : QThread("Primary thread")
	{
		FX::primaryThread=this;
#ifdef USE_WINAPI
		p->id=QThread::id();
#endif
#ifdef USE_POSIX
#ifdef USE_OURTHREADID
		p->id=(FXProcess::id()<<16)+0xffff;	// Highest id possible for POSIX
#else
		p->id=QThread::id();
#endif
#endif
		p->currentThread=this;
		isRunning=true;
	}
	~FXPrimaryThread()
	{
		QThreadPrivate::forceCleanup(this);
		// Need to delay setting currentThread to zero to after cleanup calls
		QThread::addCleanupCall(Generic::BindFuncN(zeroCThread));
	}
	void run()
	{
		FXERRG("Should never be called", FXPRIMARYTHREAD_RESERVED, FXERRH_ISFATAL);
	}
	void *cleanup() { return 0; }
};
static FXProcess_StaticInit<FXPrimaryThread> primarythreadinit("FXPrimaryThread");
struct CreationUpcall
{
	QThread::CreationUpcallSpec upcallv;
	bool inThread;
	CreationUpcall(QThread::CreationUpcallSpec &_upcallv, bool _inThread) : upcallv(_upcallv), inThread(_inThread) { }
};
static QMutex creationupcallslock;
static QPtrList<CreationUpcall> creationupcalls(true);

class QThreadIntException : public FXException
{
public:
	QThreadIntException() : FXException(0, 0, "Internal QThread cancellation exception", FXEXCEPTION_INTTHREADCANCEL, FXERRH_ISINFORMATIONAL) { }
};

static void cleanup_thread(void *t)
{
	QThread *tt=(QThread *) t;
#ifdef DEBUG
	fxmessage("Thread %u (%s) cleanup\n", (FXuint) QThread::id(), tt->name());
#endif
	QThreadPrivate::cleanup(tt);
}
static void *start_thread(void *t)
{
	void *result=0;		// This is stored into by code further down the call stack
	QThread *tt=(QThread *) t;
	if(!tt->isValid())
	{
		fxwarning("Thread appears to have been destructed before it could start!\n");
		return (void *)-1;
	}
	QThreadPrivate::setResultAddr(tt, &result);
	// Set currentThread to point to t so we will know who we are in the future
	// Must be done for nested exception handling framework to function
	QThreadPrivate::currentThread=tt;
	FXERRH_TRY
	{
		QThreadPrivate::run(tt);
		return result;
	}
	FXERRH_CATCH(QThreadIntException &)
	{
		cleanup_thread(t);
		return result;
	}
	FXERRH_CATCH(FXException &e)
	{
#ifdef DEBUG
		fxmessage("Exception occurred in thread %u (%s): %s\n", (FXuint) QThread::id(), tt->name(), e.report().text());
#endif
		if(e.flags() & FXERRH_ISFATAL)
		{
			QThreadPrivate::forceCleanup(tt);
			if(FXApp::instance()) FXApp::instance()->exit(1);
			FXProcess::exit(1);
		}
		//QThread::postEvent(TProcess::getProcess(), new TNonGUIThreadExceptionEvent(e));
		if(!tt->inCleanup())
		{
			try
			{
				cleanup_thread(t);
				return result;
			}
			catch(FXException &e)
			{
#ifdef DEBUG
				fxmessage("Exception occurred in cleanup of thread %u (%s) after other exception\n", (FXuint) QThread::id(), tt->name());
#endif
				if(e.flags() & FXERRH_ISFATAL)
				{
					QThreadPrivate::forceCleanup(tt);
					if(FXApp::instance()) FXApp::instance()->exit(1);
					FXProcess::exit(1);
				}
				//QThread::postEvent(TProcess::getProcess(), new TNonGUIThreadExceptionEvent(e));
			}
		}
		QThreadPrivate::forceCleanup(tt);
		return result;
	}
	FXERRH_ENDTRY
}
#ifdef USE_WINAPI
static DWORD WINAPI start_threadwin(void *p)
{
	return (DWORD) start_thread(p);
}
#endif

void QThreadPrivate::run(QThread *t)
{
#ifdef USE_POSIX
	FXERRHOS(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL)); 
	FXERRHOS(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL));
#endif
	{
#ifdef USE_WINAPI
		t->p->id=QThread::id();
#endif
#ifdef USE_POSIX
#ifdef USE_OURTHREADID
		static FXushort idt=0;
		// Can't use a QMutex as idlistlock as QMutex depends on QThread::id()
		while(idlistlock.cmpX(0,1));	// Lock list
		while(-1!=idlist.findIndex(idt))
		{
			if(0xfffe==++idt) idt=0;
		}
		idlist.append(idt);
		idlistlock=0;					// Free list
		// May need someday to take a CRC of the pid as it don't fit into 16 bit
		t->p->id=idt | (FXProcess::id()<<16);
#else
		t->p->id=QThread::id();
#endif
#endif
	}
	FXMtxHold h(t->p);
#ifdef DEBUG
	fxmessage("Thread %u (%s) started\n", (FXuint) QThread::id(), t->name());
#endif
#ifdef USE_POSIX
#ifdef _MSC_VER
	// Get around the over optimisation bug in MSVC
#pragma inline_depth(0)
#endif
	pthread_cleanup_push(cleanup_thread, (void *) t);
	if(t->isInCleanup)
	{
		// Weird bug in pthreads-win32: it seems to rerun this block sometimes after
		// cleanup
		return;
	}
#endif
	t->isRunning=true;
	t->isFinished=false;
	{	// Do creation upcalls
		FXMtxHold h(creationupcallslock);
		CreationUpcall *cu;
		for(QPtrListIterator<CreationUpcall> it(creationupcalls); (cu=it.current()); ++it)
		{
			if(cu->inThread) cu->upcallv(t);
		}
	}
	if(t->p->startedwc) t->p->startedwc->wakeAll();
	h.unlock();
	t->run();
#ifdef USE_WINAPI
	cleanup_thread(t);
#endif
#ifdef USE_POSIX
	pthread_cleanup_pop(1);
#ifdef _MSC_VER
#pragma inline_depth()
#endif
#endif
	// t and t->p now considered dead
}

void QThreadPrivate::cleanup(QThread *t)
{
	bool doAutoDelete=false;
	QWaitCondition *stoppedwc=0;
	{
		QThread_DTHold dth;
		FXMtxHold h(t->p);
		FXERRH(t->p, "Possibly a 'delete this' was called during thread cleanup?", QTHREAD_DELETETHIS, FXERRH_ISFATAL);
		FXERRH(!t->isInCleanup, "Exception occured during thread cleanup", QTHREAD_CLEANUPEXCEPTION, FXERRH_ISFATAL);
		t->isInCleanup=true;
		h.unlock();
		*t->p->result=t->cleanup();
		h.relock();
		forceCleanup(t);
#ifdef DEBUG
		fxmessage("Thread %u (%s) cleanup exits with code %d\n", (FXuint) QThread::id(), t->name(), (int)(FXuval) *t->p->result);
#endif
		{
#ifdef USE_OURTHREADID
			FXushort idt=(FXushort)(t->p->id & 0xffff);
			while(idlistlock.cmpX(0,1));	// Lock list
			assert(-1!=idlist.findIndex(idt));
			idlist.remove(idt);
			idlistlock=0;			// Free list
#endif
			t->p->id=0;
		}
		doAutoDelete=t->p->autodelete;
		stoppedwc=t->p->stoppedwc; t->p->stoppedwc=0;
	}
	//assert(!(t->p->isLocked() && t->isValid()));
	// t now considered dead
	currentThread=0;
	if(doAutoDelete)
		t->selfDestruct();
    if(stoppedwc)
	{
		stoppedwc->wakeAll();
		FXDELETE(stoppedwc);
	}
}

void QThreadPrivate::forceCleanup(QThread *t)
{	// Note: called by primary thread destructor too
	t->isRunning=false;
	t->isFinished=true;
	QThreadPrivate::CleanupCall *cc;
	for(QPtrListIterator<QThreadPrivate::CleanupCall> it(t->p->cleanupcalls); (cc=it.current());)
	{
		if(cc->inThread)
		{
			(*cc->code)();
			QPtrListIterator<QThreadPrivate::CleanupCall> it2(it);
			++it;
			t->p->cleanupcalls.removeByIter(it2);
		}
		else ++it;
	}
}

QThread::QThread(const char *name, bool autodelete, FXuval stackSize, QThread::ThreadScheduler threadloc) : magic(*((FXuint *) "THRD")), p(0), myname(name)
{
	FXERRHM(p=new QThreadPrivate(autodelete, stackSize, threadloc));
	isRunning=isFinished=isInCleanup=false;
	termdisablecnt=0;
#ifdef FXDISABLE_THREADS
	FXERRG("This build of FOX has threads disabled", QTHREAD_NOTHREADS, FXERRH_ISDEBUG);
#endif
}

QThread::~QThread()
{ FXEXCEPTIONDESTRUCT1 {
	if(isFinished || !isRunning)
	{
		FXMtxHold h(p);
		QThreadPrivate::CleanupCall *cc;
		for(QPtrListIterator<QThreadPrivate::CleanupCall> it(p->cleanupcalls); (cc=it.current()); ++it)
		{
			h.unlock();
			(*cc->code)();
			h.relock();
		}
		h.unlock();
		FXDELETE(p);
	}
	else FXERRG(QTrans::tr("QThread", "You cannot destruct a running thread"), QTHREAD_STILLRUNNING, FXERRH_ISDEBUG);
	magic=0;
} FXEXCEPTIONDESTRUCT2; }

FXuval QThread::stackSize() const
{
	FXMtxHold h(p);
	return p->stackSize;
}

void QThread::setStackSize(FXuval newsize)
{
	FXMtxHold h(p);
	p->stackSize=newsize;
}

QThread::ThreadScheduler QThread::threadLocation() const
{
	FXMtxHold h(p);
	return p->threadLocation;
}

void QThread::setThreadLocation(QThread::ThreadScheduler threadloc)
{
	FXMtxHold h(p);
	p->threadLocation=threadloc;
}

bool QThread::wait(FXuint time)
{
	FXMtxHold h(p);
	if (isFinished || !isRunning) return true;
#ifdef USE_WINAPI
	h.unlock();
	DWORD ret=WaitForSingleObject(p->threadh, (FXINFINITE==time) ? INFINITE : time);
	if(WAIT_TIMEOUT==ret) return false;
	if(WAIT_OBJECT_0!=ret) FXERRHWIN(ret);
	if(!isFinished) fxwarning("WARNING: Thread appears to have been terminated unnaturally\n");
	isFinished=true; isRunning=false; isInCleanup=false;	// In case thread had died
#endif
#ifdef USE_POSIX
	if(FXINFINITE==time)
	{
		void *result;
		h.unlock();
		FXERRHOS(pthread_join(p->threadh, &result));
		if(p) p->threadh=0;
		if(!isFinished) fxwarning("WARNING: Thread appears to have been terminated unnaturally\n");
		isFinished=true; isRunning=false; isInCleanup=false;	// In case thread had died
	}
	else
	{
		QWaitCondition *wc;
		if(!p->stoppedwc) FXERRHM(p->stoppedwc=new QWaitCondition);
		wc=p->stoppedwc;
		h.unlock();
		if(!wc->wait(time)) return false;
	}
#endif
	return true;
}

void QThread::start(bool waitTillStarted)
{
	static FXuint noOfProcessors=FXProcess::noOfProcessors();
	FXMtxHold h(p);
	if(waitTillStarted && !p->startedwc)
	{
		FXERRHM(p->startedwc=new QWaitCondition(false, false));
	}
#ifdef USE_WINAPI
	{
		DWORD threadId;
		p->plsCancel=false;
		FXERRHWIN(ResetEvent(p->plsCancelWaiter));
#ifndef FX_SMPBUILD
		if(noOfProcessors!=1)
		{	// Keep on processor 0 if not SMP build
			fxmessage("WARNING: Running non-SMP build executable on SMP system, forcing thread to only use processor zero!\n");
			FXERRHWIN(NULL!=(p->threadh=CreateThread(NULL, p->stackSize, start_threadwin, (void *) this, CREATE_SUSPENDED, &threadId)));
			FXERRHWIN(SetThreadAffinityMask(p->threadh, 1));
			FXERRHWIN(ResumeThread(p->threadh));
		}
		else
#endif
		{
			FXERRHWIN(NULL!=(p->threadh=CreateThread(NULL, p->stackSize, start_threadwin, (void *) this, 0, &threadId)));
		}
	}
#endif
#ifdef USE_POSIX
#ifndef FX_SMPBUILD
	if(noOfProcessors!=1)
	{
		fxerror("FATAL ERROR: You cannot run a non-SMP build executable on a SMP system!\n");
	}
#endif
	pthread_attr_t attr;
	FXERRHOS(pthread_attr_init(&attr));
	int scope=0;
	if(InProcess==p->threadLocation)
		scope=PTHREAD_SCOPE_PROCESS;
	else if(InKernel==p->threadLocation)
		scope=PTHREAD_SCOPE_SYSTEM;
	else
#ifdef __FreeBSD__
		/* On KSE threads, process scheduled threads can still run across multiple
		CPU's though with scope process, a number of very useful optimisations
		can come into play. Therefore it's the default here */
		scope=PTHREAD_SCOPE_PROCESS;
#else
		scope=PTHREAD_SCOPE_SYSTEM;
#endif
	FXERRHOS(pthread_attr_setscope(&attr, scope));
	if(p->stackSize) FXERRHOS(pthread_attr_setstacksize(&attr, p->stackSize));
	if(p->threadh)
	{
		void *result;
		FXERRHOS(pthread_join(p->threadh, &result));
		p->threadh=0;
	}
	FXERRHOS(pthread_create(&p->threadh, &attr, start_thread, (void *) this));
	FXERRHOS(pthread_attr_destroy(&attr));
#endif
	{	// Do creation upcalls
		FXMtxHold h(creationupcallslock);
		CreationUpcall *cu;
		for(QPtrListIterator<CreationUpcall> it(creationupcalls); (cu=it.current()); ++it)
		{
			if(!cu->inThread) cu->upcallv(this);
		}
	}
	h.unlock();
	if(waitTillStarted)
	{
		FXERRH(p->startedwc->wait(10000), QTrans::tr("QThread", "Failed to start thread"), QTHREAD_STARTFAILED, 0);
		FXDELETE(p->startedwc);
	}
}

bool QThread::isValid() const throw()
{
	return *((FXuint *) "THRD")==magic;
}

bool QThread::setAutoDelete(bool doso) throw()
{
	FXMtxHold h(p);
	bool ret=p->autodelete;
	p->autodelete=doso;
	return ret;
}

void QThread::requestTermination()
{
	if(isRunning)
	{
		p->plsCancel=true;
#ifdef USE_WINAPI
		FXMtxHold h(p);
		if(!p->plsCancelDisabled) FXERRHWIN(SetEvent(p->plsCancelWaiter)); 
#endif
#ifdef USE_POSIX
		FXERRHOS(pthread_cancel(p->threadh));
#endif
	}
}

FXulong QThread::id() throw()
{
#ifdef USE_WINAPI
	return (FXulong) GetCurrentThreadId();
#endif
#ifdef USE_POSIX
 #ifdef USE_OURTHREADID
	QThread *me=QThread::current();
	if(me && me->p)
		return me->p->id;
	else
	{	// TODO: Breakpoint this every so often and make sure it's not getting used too much
		return 0;
	}
 #else
  #if defined(__linux__)
	// On Linux pthread_t is already an uint, so recast
	return (FXulong) pthread_self();
  #elif defined(__FreeBSD__)
	/* On FreeBSD pthread_t is a pointer, so dereference. Unfortunately right now
	we must define the kernel structure as it's opaque */
	struct FreeBSD_pthread
	{
		void *tcb;		// struct tcb *tcb;
		FXuint magic;
		char *name;
		FXulong uniqueid;
		// don't care about rest
	};
	FreeBSD_pthread *pt=(FreeBSD_pthread *) pthread_self();
	FXulong ret=getpid();
	ret=ret<<32;
	ret|=(FXuint) pt->uniqueid;
	return ret;
  #else
   #error Unknown POSIX architecture, don't know how to convert pthread_self()
  #endif
 #endif
#endif
	return 0;
}

FXulong QThread::myId() const
{
	return p->id;
}

QThread *QThread::current()
{
	QThread *c=QThreadPrivate::currentThread;
#ifdef DEBUG
	//assert(c);
#endif
	return c;
}

QThread *QThread::primaryThread() throw()
{
	return FX::primaryThread;
}

QThread *QThread::creator() const
{
	return p->creator;
}

signed char QThread::priority() const
{
#ifdef USE_WINAPI
	if(p && p->threadh)
	{
		LONG pri=(LONG) GetThreadPriority(p->threadh);
		return (signed char)(((255*(pri+THREAD_BASE_PRIORITY_MIN))/(THREAD_BASE_PRIORITY_MAX-THREAD_BASE_PRIORITY_MIN))-127);
	}
#endif
#ifdef USE_POSIX
	if(p && p->threadh)
	{
		int policy, min, max;
		sched_param s={0};
		FXERRHOS(pthread_getschedparam(p->threadh, &policy, &s));
		min=sched_get_priority_min(policy);
		max=sched_get_priority_max(policy);
		return (signed char)(((255*(s.sched_priority+min))/(max-min))-127);
	}
#endif
	return 0;
}

void QThread::setPriority(signed char pri)
{
#ifdef USE_WINAPI
	if(p && p->threadh)
	{
		DWORD _pri=(((pri+127)*(THREAD_BASE_PRIORITY_MAX-THREAD_BASE_PRIORITY_MIN))/255)-THREAD_BASE_PRIORITY_MIN;
		FXERRHWIN(SetThreadPriority(p->threadh, _pri));
	}
#endif
#ifdef USE_POSIX
	if(p && p->threadh)
	{
		int policy, min, max;
		sched_param s={0};
		FXERRHOS(pthread_getschedparam(p->threadh, &policy, &s));
		min=sched_get_priority_min(policy);
		max=sched_get_priority_max(policy);
		s.sched_priority=(((pri+127)*(max-min))/255)-min;
		FXERRHOS(pthread_setschedparam(p->threadh, SCHED_OTHER, &s));
	}
#endif
}

void QThread::sleep(FXuint t)
{
#ifdef USE_WINAPI
	msleep(t*1000);
#endif
#ifdef USE_POSIX
	::sleep(t);
#endif
}

void QThread::msleep(FXuint t)
{
#ifdef USE_WINAPI
	if(WAIT_OBJECT_0==WaitForSingleObject((HANDLE) QThread::int_cancelWaiterHandle(), t))
	{
		QThread::current()->checkForTerminate();
	}
#endif
#ifdef USE_POSIX
	::usleep(t*1000);
#endif
}

void QThread::yield()
{
#ifdef USE_WINAPI
	Sleep(0);
#endif
#ifdef USE_POSIX
	FXERRHOS(sched_yield());
#endif
}

void QThread::exit(void *r)
{
#ifdef USE_WINAPI
	ExitThread((DWORD) r);
#endif
#ifdef USE_POSIX
	pthread_exit(r);
#endif
}

void *QThread::result() const throw()
{
	if(isValid() && p)
		return *p->result;
	else
		return 0;
}

void QThread::disableTermination()
{
	if(this)
	{
		assert(isValid());
		assert(p);
		if(!termdisablecnt++)
		{
#ifdef USE_WINAPI
			FXMtxHold h(p);
			p->plsCancelDisabled=true;
			FXERRHWIN(ResetEvent(p->plsCancelWaiter)); 
#endif
#ifdef USE_POSIX
			FXERRHOS(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL));
#endif
		}
	}
}

bool QThread::checkForTerminate()
{
#ifdef USE_WINAPI
	// FXMtxHold h(p); I think we can get away with not having this?
	if(!p->plsCancelDisabled && p->plsCancel)
	{
		//h.unlock();
		void **resultaddr=p->result;
		cleanup_thread((void *) this);
		ExitThread((DWORD) *resultaddr);
		// This alternative mechanism is much nicer but unfortunately unavailable to POSIX :(
		//FXERRH_THROW(QThreadIntException());
	}
#endif
#ifdef USE_POSIX
	pthread_testcancel();
#endif
	return p->plsCancel;
}

void QThread::enableTermination()
{
	if(this)
	{
		assert(isValid());
		assert(p);
		if(!--termdisablecnt)
		{
#ifdef USE_WINAPI
			FXMtxHold h(p);
			p->plsCancelDisabled=false;
			if(p->plsCancel) FXERRHWIN(SetEvent(p->plsCancelWaiter)); 
#endif
#ifdef USE_POSIX
			FXERRHOS(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL));
#endif
		}
		if(termdisablecnt<0) termdisablecnt=0;
	}
}


void *QThread::int_cancelWaiterHandle()
{
#ifdef USE_WINAPI
	return (void *) QThread::current()->p->plsCancelWaiter;
#else
	return 0;
#endif
}

void QThread::addCreationUpcall(QThread::CreationUpcallSpec upcallv, bool inThread)
{
	FXMtxHold h(creationupcallslock);
	CreationUpcall *cu;
	FXERRHM(cu=new CreationUpcall(upcallv, inThread));
	FXRBOp unnew=FXRBNew(cu);
	creationupcalls.append(cu);
	unnew.dismiss();
}

bool QThread::removeCreationUpcall(QThread::CreationUpcallSpec upcallv)
{
	FXMtxHold h(creationupcallslock);
	CreationUpcall *cu;
	for(QPtrListIterator<CreationUpcall> it(creationupcalls); (cu=it.current()); ++it)
	{
		if(cu->upcallv==upcallv)
		{
			creationupcalls.removeByIter(it);
			return true;
		}
	}
	return false;
}

Generic::BoundFunctorV *QThread::addCleanupCall(FXAutoPtr<Generic::BoundFunctorV> handler, bool inThread)
{
	assert(this);	// Static init check
	FXMtxHold h(p);
	FXERRHM(PtrPtr(handler));
	FXPtrHold<QThreadPrivate::CleanupCall> cc=new QThreadPrivate::CleanupCall(PtrPtr(handler), inThread);
	p->cleanupcalls.append(cc);
	cc=0;
	return PtrRelease(handler);
}

bool QThread::removeCleanupCall(Generic::BoundFunctorV *handler)
{
	FXMtxHold h(p);
	QThreadPrivate::CleanupCall *cc;
	for(QPtrListIterator<QThreadPrivate::CleanupCall> it(p->cleanupcalls); (cc=it.current()); ++it)
	{
		if(cc->code==handler)
		{
			cc->code=0;
			p->cleanupcalls.removeByIter(it);
			return true;
		}
	}
	return false;
}

void *QThread::int_disableSignals()
{
#ifdef USE_WINAPI
	return 0;
#endif
#ifdef USE_POSIX
	sigset_t *old, new_;
	FXERRHM(old=new sigset_t);
	FXRBOp unold=FXRBNew(old);
	FXERRHOS(sigfillset(&new_));
	FXERRHOS(pthread_sigmask(SIG_SETMASK, &new_, old));
	unold.dismiss();
	return (void *) old;
#endif
	return 0;
}
void QThread::int_enableSignals(void *oldmask)
{
#ifdef USE_POSIX
	sigset_t *old=(sigset_t *) oldmask;
	FXRBOp unold=FXRBNew(old);
	FXERRHOS(pthread_sigmask(SIG_SETMASK, old, NULL));
#endif
}

/**************************************************************************************************************/
struct FXDLLLOCAL QThreadPoolPrivate : public QMutex
{
	FXuint total, maximum;
	FXAtomicInt free;
	bool dynamic;
	struct Thread : public QMutex, public QThread
	{
		QThreadPoolPrivate *parent;
		volatile bool free;
		QWaitCondition wc;
		Generic::BoundFunctorV *volatile code;
		Thread(QThreadPoolPrivate *_parent)
			: parent(_parent), free(true), wc(true), code(0), QThread("Pool thread", true) { }
		~Thread() { parent=0; assert(!code); code=0; }
		void run();
		void *cleanup() { return 0; }
		void selfDestruct()
		{
			{
				FXMtxHold h(parent);
				parent->threads.takeRef(this);
			}
			delete this;
		}
	};
	QPtrList<Thread> threads;
	QPtrList<Generic::BoundFunctorV> timed, waiting;
	QPtrDict<QWaitCondition> waitingwcs;
	QPtrDict<FXuint> timedtimes;

	QThreadPoolPrivate(bool _dynamic) : total(0), maximum(0), free(0), dynamic(_dynamic), threads(true), timed(true), waiting(true), waitingwcs(7, true), QMutex() { }
	~QThreadPoolPrivate()
	{
		FXMtxHold h(this);
		Thread *t;
		for(QPtrListIterator<Thread> it(threads); (t=it.current()); ++it)
			t->requestTermination();
		h.unlock();
		// Not especially pleasant this, but it's also not awful
		for(bool notdone=true;notdone;)
		{
			h.relock();
			notdone=!threads.isEmpty();
			h.unlock();
		}
		assert(threads.count()==0);
	}
};

void QThreadPoolPrivate::Thread::run()
{
	for(;;)
	{
		FXERRH_TRY
		{
			for(;;)
			{
				bool goFree=true;
				if(!parent->waiting.isEmpty())
				{
					FXMtxHold h(parent);
					if(!parent->waiting.isEmpty())
					{
						code=parent->waiting.getFirst();
						parent->waiting.takeFirst();
						assert(code);
						lock();		// I am now busy
						goFree=false;
					}
				}
				if(goFree)
				{
					free=true;
					assert(!code);
					if(++parent->free>(int) parent->total)
					{
						FXMtxHold h(parent);
						if(parent->free>(int) parent->total) 
						{
							free=false;
							--parent->free;
							return;	// Exit thread
						}
					}
					wc.wait();
					free=false;
					lock();
				}

				FXRBOp unlockme=FXRBObj(*this, &QThreadPoolPrivate::Thread::unlock);
				QThread_DTHold dth(this);
				assert(code);
				//fxmessage("Thread pool calling %p\n", code);
				Generic::BoundFunctorV *_code=code;
				assert(dynamic_cast<void *>(_code));
				(*_code)();
				FXDELETE(code);
				unlockme.dismiss();
				unlock();
				//if(!parent->waitingwcs.isEmpty())
				{
					FXMtxHold h(parent);
					QWaitCondition *codewc=parent->waitingwcs.find(_code);
					if(codewc)
					{
						//fxmessage("Waking %p\n", _code);
						codewc->wakeAll();
					}
				}
			}
		}
		FXERRH_CATCH(FXException &e)
		{
			fxwarning("Exception thrown during thread pool dispatch: %s\n", e.report().text());
		}
		FXERRH_ENDTRY
	}
}

static QMutex mastertimekeeperlock;
class QThreadPoolTimeKeeper : public QThread
{
public:
	struct Entry
	{
		FXuint when;
		QThreadPool *which;
		FXAutoPtr<Generic::BoundFunctorV> code;
		Entry(FXuint _when, QThreadPool *creator, FXAutoPtr<Generic::BoundFunctorV> _code) : when(_when), which(creator), code(_code) { }
		~Entry()
		{
			assert(!code);
		}
		bool operator<(const Entry &o) const { return o.when-when<0x80000000; }
		bool operator==(const Entry &o) const { return when==o.when && which==o.which && code==o.code; }
	};
	QWaitCondition wc;
	QSortedList<Entry> entries;
	QThreadPoolTimeKeeper() : wc(true), entries(true), QThread("Thread pool time keeper") { }
	~QThreadPoolTimeKeeper()
	{
		requestTermination();
		wait();
	}
	void run()
	{
		FXuint untilNext=FXINFINITE;
		for(;;)
		{
			bool callFunct=!wc.wait(untilNext);
			FXMtxHold h(mastertimekeeperlock);
			do
			{
				Entry *entry=entries.isEmpty() ? 0 : entries.getFirst();
				if(entry)
				{
					FXuint now=FXProcess::getMsCount();
					FXuint diff=now-entry->when;
#ifdef DEBUG
					//if(diff>=0x80000000)
					//	fxmessage("NOTE: threadpool system returned %d ms early, rescheduling\n", -((FXint)diff));
					//if(diff<0x80000000)
					//	fxmessage("NOTE: threadpool dispatch %d ms late\n", diff);
#endif
					if(diff<0x80000000)
					{
						FXPtrHold<Entry> entryh(entry);
						entries.takeFirst();
						entryh->which->dispatch(entryh->code);
						assert(!entryh->code);
					}
					if(!entries.isEmpty())
					{
						entry=entries.getFirst();
						FXint _untilNext=entry->when-now;
						//fxmessage("%d ms until next\n", _untilNext);
						if(_untilNext<0) _untilNext=0;
						untilNext=(FXuint) _untilNext;
					} else untilNext=FXINFINITE;
				}
				else untilNext=FXINFINITE;
			} while(!untilNext);
		}
	}
	void *cleanup()
	{
		FXMtxHold h(mastertimekeeperlock);
		Entry *entry;
		assert(entries.isEmpty());	// Otherwise it's probably a memory leak
		while((entry=entries.getFirst()))
		{
			PtrRelease(entry->code);
			entries.removeFirst();
		}
		return 0;
	}
};
static FXPtrHold<QThreadPoolTimeKeeper> mastertimekeeper;
static void DestroyThreadPoolTimeKeeper()
{
	delete static_cast<QThreadPoolTimeKeeper *>(mastertimekeeper);
	mastertimekeeper=0;
}

QThreadPool::QThreadPool(FXuint total, bool dynamic) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QThreadPoolPrivate(dynamic));
	setTotal(total);
	unconstr.dismiss();
}

QThreadPool::~QThreadPool()
{ FXEXCEPTIONDESTRUCT1 {
	if(mastertimekeeper)
	{
		FXMtxHold h(mastertimekeeperlock);
		QThreadPoolTimeKeeper::Entry *entry;
		for(QSortedListIterator<QThreadPoolTimeKeeper::Entry> it(mastertimekeeper->entries); (entry=it.current());)
		{
			if(this==entry->which)
			{
				++it;
				mastertimekeeper->entries.removeRef(entry);
			}
			else ++it;
		}
	}
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

FXuint QThreadPool::total() const throw()
{
	return p->total;
}

FXuint QThreadPool::maximum() const throw()
{
	return p->maximum;
}

FXuint QThreadPool::free() const throw()
{
	return p->free;
}

void QThreadPool::startThreads(FXuint newno)
{
	QThreadPoolPrivate::Thread *t;
	if(newno>p->total)
	{
		for(FXuint n=p->total; n<newno; n++)
		{
			FXERRHM(t=new QThreadPoolPrivate::Thread(p));
			FXRBOp unnew=FXRBNew(t);
			p->threads.append(t);
			unnew.dismiss();
		}
	}
	p->total=newno;
	for(QPtrListIterator<QThreadPoolPrivate::Thread> it(p->threads); (t=it.current()); ++it)
	{
		if(!t->running()) t->start();
	}
}

void QThreadPool::setTotal(FXuint newno)
{
	FXMtxHold h(p);
	p->maximum=newno;
	if(!p->dynamic) startThreads(newno);
}

bool QThreadPool::dynamic() const throw()
{
	return p->dynamic;
}

void QThreadPool::setDynamic(bool v)
{
	p->dynamic=v;
}

QThreadPool::handle QThreadPool::dispatch(FXAutoPtr<Generic::BoundFunctorV> code, FXuint delay)
{
	Generic::BoundFunctorV *_code=0;
	//fxmessage("Thread pool dispatch %p in %d ms\n", PtrPtr(code), delay);
	assert(code);
	if(delay)
	{
		FXMtxHold h(mastertimekeeperlock);
		if(!mastertimekeeper)
		{
			FXERRHM(mastertimekeeper=new QThreadPoolTimeKeeper);
			mastertimekeeper->start();
			// Slightly nasty this :)
			primaryThread->addCleanupCall(Generic::BindFuncN(DestroyThreadPoolTimeKeeper));
		}
		QThreadPoolTimeKeeper::Entry *entry;
		_code=PtrPtr(code);
		FXERRHM(entry=new QThreadPoolTimeKeeper::Entry(FXProcess::getMsCount()+delay, this, code));
		FXRBOp unnew=FXRBNew(entry);
		mastertimekeeper->entries.insert(entry);
		unnew.dismiss();
		h.unlock();
		mastertimekeeper->wc.wakeAll();
	}
	else
	{
		FXMtxHold h(p);
		if(p->free)
		{
			QThreadPoolPrivate::Thread *t;
			for(QPtrListIterator<QThreadPoolPrivate::Thread> it(p->threads); (t=it.current()); ++it)
			{
				if(t->free && !t->code)
				{
					--p->free;
					t->code=_code=PtrRelease(code);
					t->wc.wakeAll();
					//fxmessage("waking %p\n", t);
					break;
				}
			}
			assert(t);
		}
		else
		{
			p->waiting.append((_code=PtrRelease(code)));
			//fxmessage("appending\n");
			if(p->dynamic && p->total<p->maximum)
			{
				startThreads(p->total+1);
			}
		}
	}
	assert(!code);
	return (handle) _code;
}

QThreadPool::CancelledState QThreadPool::cancel(QThreadPool::handle _code, bool wait)
{
	Generic::BoundFunctorV *code=(Generic::BoundFunctorV *) _code;
	if(!code) return NotFound;
	FXMtxHold h(p);
	//fxmessage("Thread pool cancel %p\n", code);
	if(!p->waiting.takeRef(code))
	{
		h.unlock();
		FXMtxHold h2(mastertimekeeperlock);
		h.relock();
		if(!p->waiting.takeRef(code))
		{
			QThreadPoolTimeKeeper::Entry *entry;
			for(QSortedListIterator<QThreadPoolTimeKeeper::Entry> it(mastertimekeeper->entries); (entry=it.current()); ++it)
			{
				if(this==entry->which && PtrPtr(entry->code)==code)
				{
					PtrReset(entry->code, 0);		// deletes functor
					mastertimekeeper->entries.removeRef(entry);
					//fxmessage("Thread pool cancel %p found\n", code);
					return Cancelled;
				}
			}
			h2.unlock();	// Unlock time keeper
			{	// Ok, is it currently being executed? If so, wait till it's done
				QThreadPoolPrivate::Thread *t;
				for(QPtrListIterator<QThreadPoolPrivate::Thread> it(p->threads); (t=it.current()); ++it)
				{
					if(t->code==code)
					{	// Wait for it to complete
						if(QThread::id()==t->myId())
						{
							FXERRH(!wait, "You cannot cancel a thread pool dispatch from within that dispatch with wait as a deadlock would occur!", 0, FXERRH_ISDEBUG);
						}
						//fxmessage("Thread pool cancel %p waiting for completion\n", code);
						h.unlock();
						if(wait) { FXMtxHold h3(t); }
						// Should be deleted if we waited
						return WasRunning;
					}
				}
			}
			//fxmessage("Thread pool cancel %p not found!\n", code);
			return NotFound;
		}
	}
	return Cancelled;
}

bool QThreadPool::reset(QThreadPool::handle _code, FXuint delay)
{
	Generic::BoundFunctorV *code=(Generic::BoundFunctorV *) _code;
	FXMtxHold h(mastertimekeeperlock);
	if(!mastertimekeeper) return false;
	QThreadPoolTimeKeeper::Entry *entry;
	QSortedListIterator<QThreadPoolTimeKeeper::Entry> it=mastertimekeeper->entries;
	for(; (entry=it.current()); ++it)
	{
#if defined(__GNUC__) && __GNUC__==3 && __GNUC_MINOR__<=2
		// Assuming GCC v3.4 will fix this
		Generic::BoundFunctorV *codeentry=PtrRef(entry->code);
		if(codeentry==code) break;
#else
		if(entry->code==code) break;
#endif
	}
	if(!entry) return false;
	mastertimekeeper->entries.takeByIter(it);
	entry->when=FXProcess::getMsCount()+delay;
	mastertimekeeper->entries.insert(entry);
	mastertimekeeper->wc.wakeOne();
	return true;
}

bool QThreadPool::wait(QThreadPool::handle _code, FXuint period)
{
	Generic::BoundFunctorV *code=(Generic::BoundFunctorV *) _code;
	FXMtxHold h(p);
	if(-1==p->waiting.findRef(code))
	{
		QThreadPoolPrivate::Thread *t;
		for(QPtrListIterator<QThreadPoolPrivate::Thread> it(p->threads); (t=it.current()); ++it)
		{
			if(t->code==code) break;
		}
		if(!t)
		{
			if(mastertimekeeper)
			{
				QThreadPoolTimeKeeper::Entry *e;
				for(QSortedListIterator<QThreadPoolTimeKeeper::Entry> it(mastertimekeeper->entries); (e=it.current()); ++it)
				{
#if defined(__GNUC__) && __GNUC__==3 && __GNUC_MINOR__<=2
					// Assuming GCC v3.4 will fix this
					Generic::BoundFunctorV *codeentry=PtrRef(e->code);
					if(codeentry==code) break;
#else
					if(e->code==code) break;
#endif
				}
				if(!e) return true;
			}
			else return true;
		}
	}
	QWaitCondition *wc=p->waitingwcs.find(code);
	if(!wc)
	{
		FXERRHM(wc=new QWaitCondition);
		FXRBOp unnew=FXRBNew(wc);
		p->waitingwcs.insert(code, wc);
		unnew.dismiss();
	}
	//fxmessage("Waiting %p\n", code);
	h.unlock();
	if(!wc->wait(period)) return false;
	h.relock();
	p->waitingwcs.remove(code);
	return true;
}

} // namespace

