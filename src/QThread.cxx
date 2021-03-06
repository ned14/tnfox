/********************************************************************************
*                                                                               *
*                 M u l i t h r e a d i n g   S u p p o r t                     *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2002-2010 by Niall Douglas.   All Rights Reserved.       *
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
#ifndef FXDISABLE_THREADS
KernelWaitObjectCache waitObjectCache;
#endif
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

#if defined(__APPLE__) && defined(_APPLE_C_SOURCE)
static inline void LatchWaiter(int *waiter)
{	// Mustn't overfill the pipe
	struct ::timeval delta;
	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	delta.tv_usec=0;
	delta.tv_sec=0;

	// poll() would be easier here, but it's non-standard
	FD_SET(waiter[0], &readfds);
	if(::select(waiter[0]+1,&readfds,&writefds,&exceptfds,&delta)) return;

	char c=0;
	FXERRHOS(::write(waiter[1], &c, 1));
}
static inline void ResetWaiter(int *waiter)
{	// Mustn't read if pipe is empty
	for(;;)
	{
		struct ::timeval delta;
		fd_set readfds;
		fd_set writefds;
		fd_set exceptfds;
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&exceptfds);
		delta.tv_usec=0;
		delta.tv_sec=0;

		// poll() would be easier here, but it's non-standard
		FD_SET(waiter[0], &readfds);
		if(!::select(waiter[0]+1,&readfds,&writefds,&exceptfds,&delta)) break;

		char buffer[4];
		FXERRHOS(::read(waiter[0], buffer, 4));
	}
}
#endif

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
		QMtxHold h(p);
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
		QMtxHold h(p);
		if(p->waitcnt)
		{
#ifdef USE_WINAPI
			ret=SetEvent(p->wc);
#endif
#ifdef USE_POSIX
			ret=pthread_cond_signal(&p->wc);
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
		QMtxHold h(p);
		// Let all subsequent threads through
		if(!isAutoReset)
			isSignalled=true;
		if(p->waitcnt)
		{
			int waitcnt=p->waitcnt;
			for(int n=0; n<waitcnt; n++)
			{
#ifdef USE_WINAPI
				if(!(ret=SetEvent(p->wc))) goto exit;
#endif
#ifdef USE_POSIX
				if((ret=pthread_cond_signal(&p->wc))) goto exit;
#endif
			}
		}
		else isSignalled=true; // Let next thread through
exit:
		ERRCHK;
	}
#endif
}

void QWaitCondition::reset()
{
#ifndef FXDISABLE_THREADS
	if(p)
	{
		QMtxHold h(p);
		//fxmessage("QWaitCondition::reset waitcnt=%d\n", p->waitcnt);
		assert(!isSignalled || !p->waitcnt);
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
		QMtxHold h(p);
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
inline bool QRWMutex::_lock(QMtxHold &h, bool write)
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
		QMtxHold h(p);
		if(write)
		{
#ifdef DEBUG
			FXulong myid=QThread::id();
			FXERRH(p->write.threadid==myid, "QRWMutex::unlock(true) called by thread which did not have write lock", QRWMUTEX_BADUNLOCK, FXERRH_ISDEBUG);
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
			FXERRH(p->readCnt(), "QRWMutex::unlock(false) called by thread which did not have read lock", QRWMUTEX_BADUNLOCK, FXERRH_ISDEBUG);
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
		QMtxHold h(p);
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
		QMtxHold h(p);
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
#if defined(__APPLE__) && defined(_APPLE_C_SOURCE)
	int plsCancelWaiter[2];
	bool plsCancelDisabled;
#endif
	pthread_t threadh;
#endif
	bool plsCancel;
	bool autodelete;
	bool recursiveProcessorAffinity;
	FXuval stackSize;
	QThread::ThreadScheduler threadLocation;
	FXulong id, processorAffinity;
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
		: plsCancel(false), autodelete(autodel), recursiveProcessorAffinity(false),
		stackSize(stksize), threadLocation(threadloc), id(0), processorAffinity((1<<QMutexImpl::systemProcessors)-1),
		creator(0), startedwc(0), stoppedwc(0), cleanupcalls(true), QMutex()
#ifdef USE_POSIX
		, threadh(0)
#endif
#ifdef USE_WINAPI
		, threadh(0), plsCancelDisabled(false), plsCancelWaiter(0)
#endif
	{
#if defined(__APPLE__) && defined(_APPLE_C_SOURCE)
		plsCancelWaiter[0]=plsCancelWaiter[1]=0;
		plsCancelDisabled=false;
		FXERRHOS(pipe(plsCancelWaiter));
		FXERRHOS(::fcntl(plsCancelWaiter[0], F_SETFD, ::fcntl(plsCancelWaiter[0], F_GETFD, 0)|FD_CLOEXEC));
		FXERRHOS(::fcntl(plsCancelWaiter[1], F_SETFD, ::fcntl(plsCancelWaiter[1], F_GETFD, 0)|FD_CLOEXEC));
#endif
		creator=currentThread;
		if(creator && creator->p->recursiveProcessorAffinity)
		{
			processorAffinity=creator->p->processorAffinity;
			recursiveProcessorAffinity=true;
		}
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
#if defined(__APPLE__) && defined(_APPLE_C_SOURCE)
		if(plsCancelWaiter[1])
		{
			FXERRHOS(::close(plsCancelWaiter[1]));
			plsCancelWaiter[1]=0;
		}
		if(plsCancelWaiter[0])
		{
			FXERRHOS(::close(plsCancelWaiter[0]));
			plsCancelWaiter[0]=0;
		}
#endif
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
	CreationUpcall(QThread::CreationUpcallSpec _upcallv, bool _inThread) : upcallv(std::move(_upcallv)), inThread(_inThread) { }
};
static QMutex creationupcallslock;
static QPtrList<CreationUpcall> creationupcalls(true);

class QThreadIntException : public FXException
{
public:
	QThreadIntException() : FXException(0, 0, 0, "Internal QThread cancellation exception", FXEXCEPTION_INTTHREADCANCEL, FXERRH_ISINFORMATIONAL) { }
};

static void cleanup_thread(void *t)
{
	QThread *tt=(QThread *) t;
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
#ifndef FX_DISABLEGUI
			if(FXApp::instance()) FXApp::instance()->exit(1);
#endif
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
#ifndef FX_DISABLEGUI
					if(FXApp::instance()) FXApp::instance()->exit(1);
#endif
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

/* Handy little function which sets the name of a Win32 thread in the MSVC debugger */
#define MS_VC_EXCEPTION 0x406D1388

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
   DWORD dwType; // Must be 0x1000.
   LPCSTR szName; // Pointer to name (in user addr space).
   DWORD dwThreadID; // Thread ID (-1=caller thread).
   DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

static void SetThreadName(DWORD dwThreadID, const char* threadName)
{
   THREADNAME_INFO info;
   info.dwType = 0x1000;
   info.szName = threadName;
   info.dwThreadID = dwThreadID;
   info.dwFlags = 0;

   __try
   {
      RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
   }
}
#endif

#ifdef USE_POSIX
#ifdef HAVE_NPTL
}
extern "C" {
// The default NPTL pthread_cleanup_push() macro uses C++ exception handling to cancel
// threads, thus allowing stack unwinding and destruction of stacked C++ objects
// UNFORTUNATELY this prevents threads being cancelled when they have gone through
// C library code (eg; OpenSSL) or throw() marked functions. We therefore hack
// the problem away by direct use of the non-EH pthread_cleanup_push() implementation
#undef pthread_cleanup_push
#undef pthread_cleanup_pop


// The following comes from glibc's pthread.h
// The first is the GCC-optimised implementation, the second is the default
#if 0
/* Function called to call the cleanup handler.  As an extern inline
   function the compiler is free to decide inlining the change when
   needed or fall back on the copy which must exist somewhere
   else.  */
extern __inline void
__pthread_cleanup_routine (struct __pthread_cleanup_frame *__frame)
{
  if (__frame->__do_it)
    __frame->__cancel_routine (__frame->__cancel_arg);
}

/* Install a cleanup handler: ROUTINE will be called with arguments ARG
   when the thread is canceled or calls pthread_exit.  ROUTINE will also
   be called with arguments ARG when the matching pthread_cleanup_pop
   is executed with non-zero EXECUTE argument.

   pthread_cleanup_push and pthread_cleanup_pop are macros and must always
   be used in matching pairs at the same nesting level of braces.  */
#  define pthread_cleanup_push(routine, arg) \
  do {									      \
    struct __pthread_cleanup_frame __clframe				      \
      __attribute__ ((__cleanup__ (__pthread_cleanup_routine)))		      \
      = { (routine), (arg),	 	      \
	  1 };

/* Remove a cleanup handler installed by the matching pthread_cleanup_push.
   If EXECUTE is non-zero, the handler function is called. */
#  define pthread_cleanup_pop(execute) \
    __clframe.__do_it = (execute);					      \
  } while (0)

#else
/* Function called to call the cleanup handler.  As an extern inline
   function the compiler is free to decide inlining the change when
   needed or fall back on the copy which must exist somewhere
   else.  */
extern __inline void
__pthread_cleanup_routine (struct __pthread_cleanup_frame *__frame)
{
  if (__frame->__do_it)
    __frame->__cancel_routine (__frame->__cancel_arg);
}

/* Install a cleanup handler: ROUTINE will be called with arguments ARG
   when the thread is canceled or calls pthread_exit.  ROUTINE will also
   be called with arguments ARG when the matching pthread_cleanup_pop
   is executed with non-zero EXECUTE argument.

   pthread_cleanup_push and pthread_cleanup_pop are macros and must always
   be used in matching pairs at the same nesting level of braces.  */
# define pthread_cleanup_push(routine, arg) \
  do {									      \
    __pthread_unwind_buf_t __cancel_buf;				      \
    void (*__cancel_routine) (void *) = (routine);			      \
    void *__cancel_arg = (arg);						      \
    int not_first_call = __sigsetjmp ((struct __jmp_buf_tag *)		      \
				      __cancel_buf.__cancel_jmp_buf, 0);      \
    if (__builtin_expect (not_first_call, 0))				      \
      {									      \
	__cancel_routine (__cancel_arg);				      \
	__pthread_unwind_next (&__cancel_buf);				      \
	/* NOTREACHED */						      \
      }									      \
									      \
    __pthread_register_cancel (&__cancel_buf);				      \
    do {
extern void __pthread_register_cancel (__pthread_unwind_buf_t *__buf)
     __cleanup_fct_attribute;

/* Remove a cleanup handler installed by the matching pthread_cleanup_push.
   If EXECUTE is non-zero, the handler function is called. */
# define pthread_cleanup_pop(execute) \
    } while (0);							      \
    __pthread_unregister_cancel (&__cancel_buf);			      \
    if (execute)							      \
      __cancel_routine (__cancel_arg);					      \
  } while (0)
extern void __pthread_unregister_cancel (__pthread_unwind_buf_t *__buf)
  __cleanup_fct_attribute;

/* Internal interface to initiate cleanup.  */
extern void __pthread_unwind_next (__pthread_unwind_buf_t *__buf)
     __cleanup_fct_attribute __attribute__ ((__noreturn__))
# ifndef SHARED
     __attribute__ ((__weak__))
# endif
     ;
#endif
}
namespace FX {
#endif
#endif

void QThreadPrivate::run(QThread *t)
{
#ifdef USE_POSIX
	FXERRHOS(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL));
	FXERRHOS(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL));
#endif
	// Before doing anything else, set my name, my affinity and ensure I'm only on allowed processors
	{
#ifdef USE_WINAPI
		SetThreadName(-1, t->name());
		DWORD_PTR _mask=(DWORD_PTR) t->p->processorAffinity;
		FXERRHWIN(SetThreadAffinityMask(GetCurrentThread(), _mask));
		t->p->id=QThread::id();
#endif
#ifdef USE_POSIX
#ifdef HAVE_NPTL
		cpu_set_t _mask;
		CPU_ZERO(&_mask);
		for(int n=0; n<64; n++)
			if(t->p->processorAffinity & (1<<n)) CPU_SET(n, &_mask);
		FXERRHOS(pthread_setaffinity_np(pthread_self(), sizeof(_mask), &_mask));
#endif
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
		// Only allow signals we like
		//sigset_t sigmask;
		//FXERRHOS(sigfillset(&sigmask));
		//FXERRHOS(sigdelset(&sigmask, SIGPIPE));
		//FXERRHOS(pthread_sigmask(SIG_SETMASK, &sigmask, NULL));
#endif
		QThread::yield();
	}
	FXASSERT(t->isValid());
	QMtxHold h(t->p);
#ifdef DEBUG
	fxmessage("Thread %u (%s) started with affinity=0x%x\n", (FXuint) QThread::id(), t->name(), (FXuint) t->p->processorAffinity);
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
		QMtxHold h(creationupcallslock);
		CreationUpcall *cu;
		for(QPtrListIterator<CreationUpcall> it(creationupcalls); (cu=it.current()); ++it)
		{
			h.unlock();
			if(cu->inThread) cu->upcallv(t);
			h.relock();
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
	if(t->isInCleanup)
	{	// You have to be careful here - some broken pthreads implementations will recall cleanup()
		// as soon as you reenable termination below
		fxwarning("WARNING: QThread: cleanup() handler for thread %u reentered!\n", (FXuint) QThread::id());
		return;
	}
	{
		QThread_DTHold dth;
#ifdef DEBUG
		fxmessage("Thread %u (%s) cleanup\n", (FXuint) QThread::id(), t->name());
#endif
		QMtxHold h(t->p);
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
		// Leave it linger for threads to wake up before deletion
		QThread::msleep(1000);
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
		QMtxHold h(p);
		QThreadPrivate::CleanupCall *cc;
		for(QPtrListIterator<QThreadPrivate::CleanupCall> it(p->cleanupcalls); (cc=it.current()); ++it)
		{
			if(!cc->inThread)
			{
				h.unlock();
				(*cc->code)();
				h.relock();
			}
		}
		h.unlock();
		FXDELETE(p);
	}
	else FXERRG(QTrans::tr("QThread", "You cannot destruct a running thread"), QTHREAD_STILLRUNNING, FXERRH_ISDEBUG);
	magic=0;
} FXEXCEPTIONDESTRUCT2; }

FXuval QThread::stackSize() const
{
	QMtxHold h(p);
	return p->stackSize;
}

void QThread::setStackSize(FXuval newsize)
{
	QMtxHold h(p);
	p->stackSize=newsize;
}

QThread::ThreadScheduler QThread::threadLocation() const
{
	QMtxHold h(p);
	return p->threadLocation;
}

void QThread::setThreadLocation(QThread::ThreadScheduler threadloc)
{
	QMtxHold h(p);
	p->threadLocation=threadloc;
}

bool QThread::wait(FXuint time)
{
	QMtxHold h(p);
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
		if(!isFinished)
		{
			fxwarning("WARNING: Thread appears to have been terminated unnaturally\n");
			if(p->isLocked())
				fxerror("Thread terminated during system code - your pthreads implementation is broken!\n");
		}
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
	QMtxHold h(p);
#ifndef FX_SMPBUILD
	if(QMutexImpl::systemProcessors!=1)
	{	// Keep on processor 0 if not SMP build
		fxmessage("WARNING: Running non-SMP build executable on SMP system, forcing thread to only use processor zero!\n");
		p->processorAffinity=1;
	}
#endif
	p->plsCancel=false;
	if(waitTillStarted && !p->startedwc)
	{
		FXERRHM(p->startedwc=new QWaitCondition(false, false));
	}
#ifdef USE_WINAPI
	{
		DWORD threadId;
		FXERRHWIN(ResetEvent(p->plsCancelWaiter));
		FXERRHWIN(NULL!=(p->threadh=CreateThread(NULL, p->stackSize, start_threadwin, (void *) this, 0, &threadId)));
	}
#endif
#ifdef USE_POSIX
#if defined(__APPLE__) && defined(_APPLE_C_SOURCE)
	ResetWaiter(p->plsCancelWaiter);
#endif
	pthread_attr_t attr;
	FXERRHOS(pthread_attr_init(&attr));
	int scope=0;
	if(InProcess==p->threadLocation)
		scope=PTHREAD_SCOPE_PROCESS;
	else if(InKernel==p->threadLocation)
		scope=PTHREAD_SCOPE_SYSTEM;
	else
#if defined(__FreeBSD__)
		/* On KSE threads, process scheduled threads can still run across multiple
		CPU's though with scope process, a number of very useful optimisations
		can come into play. Therefore it's the default here */
		scope=PTHREAD_SCOPE_PROCESS;
#else
		scope=PTHREAD_SCOPE_SYSTEM;
#endif
#if defined(__FreeBSD__)
#warning Mixing KSE process & system scope threads still not stable in FreeBSD 6.0 so disabling. This may change in the future!
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
		QMtxHold h(creationupcallslock);
		CreationUpcall *cu;
		for(QPtrListIterator<CreationUpcall> it(creationupcalls); (cu=it.current()); ++it)
		{
			h.unlock();
			if(!cu->inThread) cu->upcallv(this);
			h.relock();
		}
	}
	h.unlock();
	if(waitTillStarted)
	{
		FXERRH(p->startedwc->wait(10000), QTrans::tr("QThread", "Failed to start thread"), QTHREAD_STARTFAILED, 0);
		FXDELETE(p->startedwc);
	}
}

bool QThread::isBeingCancelled() const throw()
{
	return p->plsCancel;
}

bool QThread::isValid() const throw()
{
	return *((FXuint *) "THRD")==magic;
}

bool QThread::setAutoDelete(bool doso) throw()
{
	QMtxHold h(p);
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
		QMtxHold h(p);
		if(!p->plsCancelDisabled) FXERRHWIN(SetEvent(p->plsCancelWaiter));
#endif
#ifdef USE_POSIX
#if defined(__APPLE__) && defined(_APPLE_C_SOURCE)
		if(!p->plsCancelDisabled) LatchWaiter(p->plsCancelWaiter);
#endif
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
  #if defined(__linux__) || defined(__APPLE__)
	// On Linux and MacOS X pthread_t is already an uint, so recast
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
	assert(0xd09ba115==pt->magic);
	return 1000000+pt->uniqueid;
  #else
   #error Unknown POSIX architecture, do not know how to convert pthread_self()
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

FXulong QThread::processorAffinity() const
{
	if(!p) return (FXulong)-1;
#ifdef USE_WINAPI
	return p->processorAffinity;
#endif
#ifdef USE_POSIX
#ifdef HAVE_NPTL
	if(!p->threadh) return p->processorAffinity;
	cpu_set_t mask;
	FXERRHOS(pthread_getaffinity_np(p->threadh, sizeof(mask), &mask));
	FXulong processorAffinity=0;
	for(int n=0; n<64; n++)
		if(CPU_ISSET(n, &mask)) processorAffinity|=1<<n;
	return p->processorAffinity=processorAffinity;
#endif
#ifdef __FreeBSD__
	// Not supported as yet
#endif
#endif
	return p->processorAffinity;
}

void QThread::setProcessorAffinity(FXulong mask, bool recursive)
{
	if(p)
	{
		mask&=(1<<QMutexImpl::systemProcessors)-1;
		p->processorAffinity=mask;
		p->recursiveProcessorAffinity=recursive;
		if(p->threadh)
		{
#ifdef USE_WINAPI
			DWORD_PTR _mask=(DWORD_PTR) mask;
			FXERRHWIN(SetThreadAffinityMask(p->threadh, _mask));
#endif
#ifdef USE_POSIX
#ifdef HAVE_NPTL
			cpu_set_t _mask;
			CPU_ZERO(&_mask);
			for(int n=0; n<64; n++)
				if(mask & (1<<n)) CPU_SET(n, &_mask);
			FXERRHOS(pthread_setaffinity_np(p->threadh, sizeof(_mask), &_mask));
#endif
#ifdef __FreeBSD__
			// Not supported as yet
#endif
#endif
		}
	}
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
#ifdef __FreeBSD__
	// sched_yield() appears to be a noop on FreeBSD
	::usleep(0);
#else
	FXERRHOS(sched_yield());
#endif
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
			QMtxHold h(p);
			p->plsCancelDisabled=true;
			FXERRHWIN(ResetEvent(p->plsCancelWaiter));
#endif
#ifdef USE_POSIX
#if defined(__APPLE__) && defined(_APPLE_C_SOURCE)
			p->plsCancelDisabled=true;
			ResetWaiter(p->plsCancelWaiter);
#endif
			FXERRHOS(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL));
#endif
		}
	}
}

bool QThread::checkForTerminate()
{
#ifdef USE_WINAPI
	// QMtxHold h(p); I think we can get away with not having this?
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
			QMtxHold h(p);
			p->plsCancelDisabled=false;
			if(p->plsCancel) FXERRHWIN(SetEvent(p->plsCancelWaiter));
#endif
#ifdef USE_POSIX
			FXERRHOS(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL));
#if defined(__APPLE__) && defined(_APPLE_C_SOURCE)
			p->plsCancelDisabled=false;
			if(p->plsCancel) LatchWaiter(p->plsCancelWaiter);
#endif
#endif
		}
		if(termdisablecnt<0) termdisablecnt=0;
	}
}


void *QThread::int_cancelWaiterHandle()
{
#ifdef USE_WINAPI
	return (void *) QThread::current()->p->plsCancelWaiter;
#elif defined(__APPLE__) && defined(_APPLE_C_SOURCE)
	return (void *)(FXuval) QThread::current()->p->plsCancelWaiter[0];
#else
	return 0;
#endif
}

void QThread::addCreationUpcall(QThread::CreationUpcallSpec upcallv, bool inThread)
{
	QMtxHold h(creationupcallslock);
	CreationUpcall *cu;
	FXERRHM(cu=new CreationUpcall(std::move(upcallv), inThread));
	FXRBOp unnew=FXRBNew(cu);
	creationupcalls.append(cu);
	unnew.dismiss();
}

bool QThread::removeCreationUpcall(QThread::CreationUpcallSpec upcallv)
{
	QMtxHold h(creationupcallslock);
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
	QMtxHold h(p);
	FXERRHM(PtrPtr(handler));
	FXPtrHold<QThreadPrivate::CleanupCall> cc=new QThreadPrivate::CleanupCall(PtrPtr(handler), inThread);
	p->cleanupcalls.append(cc);
	cc=0;
	return PtrRelease(handler);
}

bool QThread::removeCleanupCall(Generic::BoundFunctorV *handler)
{
	QMtxHold h(p);
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
#ifdef __GNUC__
#warning QThreadPool is under construction!
#endif
#ifdef _MSC_VER
#pragma message(__FILE__ ": WARNING: QThreadPool is under construction!")
#endif
struct FXDLLLOCAL QThreadPoolPrivate : public QMutex
{
	QThreadPool *parent;
	FXuint total, maximum;
	FXAtomicInt free;
	bool dynamic;
	struct CodeItem
	{
		FXAutoPtr<Generic::BoundFunctorV> code;
		QThreadPool::DispatchUpcallSpec upcallv;
		CodeItem() { }
		CodeItem(FXAutoPtr<Generic::BoundFunctorV> _code, QThreadPool::DispatchUpcallSpec *_upcallv) : code(_code)
		{
			if(_upcallv) upcallv=*_upcallv;
		}
#ifndef HAVE_CPP0XRVALUEREFS
#ifdef HAVE_CONSTTEMPORARIES
		CodeItem(const CodeItem &other)
		{
			CodeItem &o=const_cast<CodeItem &>(other);
#else
		CodeItem(CodeItem &o)
		{
#endif
			code=o.code;
			upcallv=o.upcallv;
		}
#else
private:
		CodeItem(const CodeItem &);		// disable copy constructor
public:
		CodeItem(CodeItem &&o) : code(std::move(o.code)), upcallv(std::move(o.upcallv))
		{
		}
#endif
	private:
		CodeItem &operator=(const CodeItem &);
	public:
		bool operator<(const CodeItem &o) const { return PtrPtr(code)<PtrPtr(o.code); }
		bool operator==(const CodeItem &o) const { return PtrPtr(code)==PtrPtr(o.code); }
	};
	struct Thread : public QMutex, public QThread
	{
		QThreadPoolPrivate *parent;
		volatile bool free;
		QWaitCondition wc;
		FXAutoPtr<CodeItem> codeitem;
		Thread(QThreadPoolPrivate *_parent)
			: parent(_parent), free(true), wc(true), QThread("Pool thread", true) { }
		~Thread() { parent=0; assert(!codeitem || !codeitem->code); }
		void run();
		void *cleanup() { return 0; }
		void selfDestruct()
		{
			{
				QMtxHold h(parent);
				parent->threads.takeRef(this);
			}
			delete this;
		}
	};
	QPtrList<Thread> threads;
	QPtrList<CodeItem> timed, waiting;
	QPtrDict<QWaitCondition> waitingwcs;
	QPtrDict<FXuint> timedtimes;

	QThreadPoolPrivate(QThreadPool *_parent, bool _dynamic) : parent(_parent), total(0), maximum(0), free(0), dynamic(_dynamic), threads(true), waitingwcs(7, true), QMutex() { }
	~QThreadPoolPrivate()
	{
		QMtxHold h(this);
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
			QMtxHold h(parent);
			for(;;)
			{
				bool goFree=true;
				assert(parent->isLocked());			// Parent threadpool is currently locked
				if(!parent->waiting.isEmpty())
				{
					codeitem=parent->waiting.getFirst();
					parent->waiting.takeFirst();
					assert(codeitem && codeitem->code);
					lock();		// I am now busy
					goFree=false;
				}
				h.unlock();	// Release parent threadpool
				if(goFree)
				{
					free=true;	// Mark myself as free
					assert(!codeitem);
					// parent->free is atomic so don't need lock
					if(++parent->free>(int) parent->total)
					{
						free=false;
						--parent->free;
						return;	// Exit thread
					}
					wc.wait();	// Wait for new job
					free=false;
					lock();		// I am now busy
				}

				FXRBOp unlockme=FXRBObj(*this, &QThreadPoolPrivate::Thread::unlock);
				QThread_DTHold dth(this);
				assert(codeitem && codeitem->code);
				//fxmessage("Thread pool calling %p\n", code);
				Generic::BoundFunctorV *_code=PtrPtr(codeitem->code);
				assert(dynamic_cast<void *>(_code));
				if(!codeitem->upcallv || codeitem->upcallv(parent->parent, (QThreadPool::handle) codeitem->code, QThreadPool::PreDispatch))
				{
					(*_code)();
					if(codeitem->upcallv) codeitem->upcallv(parent->parent, (QThreadPool::handle) codeitem->code, QThreadPool::PostDispatch);
				}
				// Reset everything
				codeitem->code=0;
				codeitem->upcallv=std::move(QThreadPool::DispatchUpcallSpec((QThreadPool::DispatchUpcallSpec::void_ *) 0));
				codeitem=0;
				h.relock();				// Relock parent threadpool
				unlockme.dismiss();
				unlock();				// I am no longer busy
				{
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
		QThreadPoolPrivate::CodeItem codeitem;
		Entry(FXuint _when, QThreadPool *creator, QThreadPoolPrivate::CodeItem _codeitem) : when(_when), which(creator), codeitem(std::move(_codeitem)) { }
		~Entry()
		{
			if(codeitem.code)
			{
				fxmessage("WARNING: QThreadPool %p timed job %p firing in %d ms was not cancelled!\n", which, PtrPtr(codeitem.code), when-FXProcess::getMsCount());
				codeitem.code=0;
			}
		}
		bool operator<(const Entry &o) const { return o.when-when<0x80000000; }
		bool operator==(const Entry &o) const { return when==o.when && which==o.which && codeitem.code==o.codeitem.code; }
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
			QMtxHold h(mastertimekeeperlock);
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
						entryh->which->dispatch(entryh->codeitem.code, 0, &entryh->codeitem.upcallv);
						assert(!entryh->codeitem.code);
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
		QMtxHold h(mastertimekeeperlock);
		Entry *entry;
		//assert(entries.isEmpty());	// Otherwise it's probably a memory leak
		while((entry=entries.getFirst()))
			entries.removeFirst();
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
	FXERRHM(p=new QThreadPoolPrivate(this, dynamic));
	setTotal(total);
	unconstr.dismiss();
}

QThreadPool::~QThreadPool()
{ FXEXCEPTIONDESTRUCT1 {
	if(mastertimekeeper)
	{
		QMtxHold h(mastertimekeeperlock);
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
	QMtxHold h(p);
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

QThreadPool::handle QThreadPool::dispatch(FXAutoPtr<Generic::BoundFunctorV> code, FXuint delay, DispatchUpcallSpec *upcallv)
{
	Generic::BoundFunctorV *_code=0;
	//fxmessage("Thread pool dispatch %p in %d ms\n", PtrPtr(code), delay);
	assert(code);
	if(delay)
	{
		QMtxHold h(mastertimekeeperlock);
		if(!mastertimekeeper)
		{
			FXERRHM(mastertimekeeper=new QThreadPoolTimeKeeper);
			mastertimekeeper->start();
			// Slightly nasty this :)
			primaryThread->addCleanupCall(Generic::BindFuncN(DestroyThreadPoolTimeKeeper));
		}
		QThreadPoolTimeKeeper::Entry *entry;
		_code=PtrPtr(code);
		FXERRHM(entry=new QThreadPoolTimeKeeper::Entry(FXProcess::getMsCount()+delay, this, QThreadPoolPrivate::CodeItem(code, upcallv)));
		FXRBOp unnew=FXRBNew(entry);
		mastertimekeeper->entries.insert(entry);
		unnew.dismiss();
		h.unlock();
		mastertimekeeper->wc.wakeAll();
	}
	else
	{
		FXAutoPtr<QThreadPoolPrivate::CodeItem> ci=new QThreadPoolPrivate::CodeItem(code, upcallv);
		QMtxHold h(p);
		if(p->free)
		{
			QThreadPoolPrivate::Thread *t;
			for(QPtrListIterator<QThreadPoolPrivate::Thread> it(p->threads); (t=it.current()); ++it)
			{
				if(t->free && !t->codeitem)
				{
					--p->free;
					t->codeitem=ci;
					_code=PtrPtr(t->codeitem->code);
					t->wc.wakeAll();
					//fxmessage("waking %p\n", t);
					break;
				}
			}
			assert(t);
		}
		if(!_code)
		{
			_code=PtrPtr(ci->code);
			p->waiting.append(PtrPtr(ci));
			PtrRelease(ci);
			//fxmessage("appending\n");
			if(p->dynamic && p->total<p->maximum)
			{
				startThreads(p->total+1);
			}
		}
	}
	return (handle) _code;
}

QThreadPool::CancelledState QThreadPool::cancel(QThreadPool::handle _code, bool wait)
{
	Generic::BoundFunctorV *code=(Generic::BoundFunctorV *) _code;
	if(!code) return NotFound;
	QMtxHold h(p);
	//fxmessage("Thread pool cancel %p\n", code);
	QThreadPoolPrivate::CodeItem *ci;
	for(QPtrListIterator<QThreadPoolPrivate::CodeItem> it=p->waiting; (ci=it.current()); ++it)
	{
		if(PtrPtr(ci->code)==code)
		{
			p->waiting.removeByIter(it);
			break;
		}
	}
	if(!ci)
	{
		h.unlock();
		QMtxHold h2(mastertimekeeperlock);
		h.relock();
		for(QPtrListIterator<QThreadPoolPrivate::CodeItem> it=p->waiting; (ci=it.current()); ++it)
		{
			if(PtrPtr(ci->code)==code)
			{
				p->waiting.removeByIter(it);
				break;
			}
		}
		if(!ci)
		{
			QThreadPoolTimeKeeper::Entry *entry;
			if(mastertimekeeper)
			{
				for(QSortedListIterator<QThreadPoolTimeKeeper::Entry> it(mastertimekeeper->entries); (entry=it.current()); ++it)
				{
					if(this==entry->which && PtrPtr(entry->codeitem.code)==code)
					{
						//fxmessage("Thread pool cancel %p found\n", code);
						entry->codeitem.code=0;
						mastertimekeeper->entries.removeRef(entry);
						return Cancelled;
					}
				}
			}
			h2.unlock();	// Unlock time keeper
			{	// Ok, is it currently being executed? If so, wait till it's done
				QThreadPoolPrivate::Thread *t;
				for(QPtrListIterator<QThreadPoolPrivate::Thread> it(p->threads); (t=it.current()); ++it)
				{
					if(t->codeitem && PtrPtr(t->codeitem->code)==code)
					{	// Wait for it to complete
						if(QThread::id()==t->myId())
						{
							FXERRH(!wait, "You cannot cancel a thread pool dispatch from within that dispatch with wait as a deadlock would occur!", 0, FXERRH_ISDEBUG);
						}
						//fxmessage("Thread pool cancel %p waiting for completion\n", code);
						h.unlock();
						if(wait) { QMtxHold h3(t); }
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
	QMtxHold h(mastertimekeeperlock);
	if(!mastertimekeeper) return false;
	QThreadPoolTimeKeeper::Entry *entry;
	QSortedListIterator<QThreadPoolTimeKeeper::Entry> it=mastertimekeeper->entries;
	for(; (entry=it.current()); ++it)
	{
#if defined(__GNUC__) && __GNUC__==3 && __GNUC_MINOR__<=2
		// Assuming GCC v3.4 will fix this
		Generic::BoundFunctorV *codeentry=PtrRef(entry->codeitem.code);
		if(codeentry==code) break;
#else
		if(entry->codeitem.code==code) break;
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
	QMtxHold h(p);
	QThreadPoolPrivate::CodeItem *ci;
	// Search the waiting list
	for(QPtrListIterator<QThreadPoolPrivate::CodeItem> it=p->waiting; (ci=it.current()); ++it)
	{
		if(PtrPtr(ci->code)==code)
			break;
	}
	if(!ci)
	{	// Search the running jobs
		QThreadPoolPrivate::Thread *t;
		for(QPtrListIterator<QThreadPoolPrivate::Thread> it(p->threads); (t=it.current()); ++it)
		{
			if(t->codeitem && PtrPtr(t->codeitem->code)==code) break;
		}
		if(!t)
		{	// Search the timed jobs
			if(mastertimekeeper)
			{
				QThreadPoolTimeKeeper::Entry *e;
				for(QSortedListIterator<QThreadPoolTimeKeeper::Entry> it(mastertimekeeper->entries); (e=it.current()); ++it)
				{
#if defined(__GNUC__) && __GNUC__==3 && __GNUC_MINOR__<=2
					// Assuming GCC v3.4 will fix this
					Generic::BoundFunctorV *codeentry=PtrRef(e->codeitem.code);
					if(codeentry==code) break;
#else
					if(e->codeitem.code==code) break;
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

