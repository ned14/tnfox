/********************************************************************************
*                                                                               *
*                  FXAtomicInt and QMutex implementations                      *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2002-2005 by Niall Douglas.   All Rights Reserved.       *
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

#ifndef FXBEING_INCLUDED_BY_QTHREAD
#error This file is not supposed to be included by public code
#endif

//#undef FX_SMPBUILD

#include "FXErrCodes.h"
#include "FXRollback.h"

#ifdef FXDISABLE_THREADS
 #undef USE_WINAPI
 #undef USE_OURMUTEX
 #undef USE_X86
#else
// Decide which thread API to use
 #ifndef USE_POSIX
  #define USE_WINAPI
  #define USE_OURMUTEX
  #include "WindowsGubbins.h"
  #include <intrin.h>
 #endif
 #ifdef USE_POSIX
  // POSIX threads for the Unices
  #define __CLEANUP_C			// for pthreads_win32
  //#define USE_OURTHREADID	// Define when pthread_self() is not unique across processes
  #include <semaphore.h>
  #include <pthread.h>
 #endif
#endif

#include <assert.h>

// On Windows, always use our mutex implementation
#ifdef USE_WINAPI
 #define USE_OURMUTEX
 #ifdef _MSC_VER
  // Use MSVC7.1 Intrinsics
  #pragma intrinsic (_InterlockedCompareExchange)
  #pragma intrinsic (_InterlockedExchange)
  #pragma intrinsic (_InterlockedExchangeAdd)
  #pragma intrinsic (_InterlockedIncrement)
  #pragma intrinsic (_InterlockedDecrement)
 #endif
#else
 // On GCC, always use our mutex implementation
 #if defined(__GNUC__)
  #define USE_OURMUTEX
  #if (defined(__i386__) || defined(__x86_64__))
   #define USE_X86 FX_X86PROCESSOR					// On x86 or x64, use inline assembler
  #else
   #include <bits/atomicity.h>
  #endif
 #endif
#endif

#ifndef USE_OURMUTEX
 #error Unsupported compiler, please add atomic int support to QThread.cxx
#endif

namespace FX {

QMUTEX_INLINEI int FXAtomicInt::get() const throw()
{
	return value;
}
QMUTEX_INLINEP FXAtomicInt::operator int() const throw() { return get(); }
QMUTEX_INLINEI int FXAtomicInt::set(int i) throw()
{	// value=i; is write-buffered out and we need it immediate
#ifdef __GNUC__
	__sync_lock_test_and_set(&value, i);
#elif defined(USE_WINAPI)
	_InterlockedExchange((PLONG) &value, i);
#endif
	return i;
}
QMUTEX_INLINEP int FXAtomicInt::operator=(int i) throw() { return set(i); }
QMUTEX_INLINEI int FXAtomicInt::incp() throw()
{
#ifdef __GNUC__
	return __sync_fetch_and_add(&value, 1);
#elif defined(USE_WINAPI)
	return _InterlockedExchangeAdd((PLONG) &value, 1);
#endif
}
QMUTEX_INLINEP int FXAtomicInt::operator++(int) throw() { return incp(); }
QMUTEX_INLINEI int FXAtomicInt::pinc() throw()
{
#ifdef __GNUC__
	return __sync_add_and_fetch(&value, 1);
#elif defined(USE_WINAPI)
	return _InterlockedIncrement((PLONG) &value);
#endif
}
QMUTEX_INLINEP int FXAtomicInt::operator++() throw() { return pinc(); }
QMUTEX_INLINEI int FXAtomicInt::finc() throw()
{	// Returns -1, 0, +1 on value AFTER inc
	return pinc();
}
QMUTEX_INLINEP int FXAtomicInt::fastinc() throw() { return finc(); }
QMUTEX_INLINEI int FXAtomicInt::inc(int i) throw()
{
#ifdef __GNUC__
	return __sync_add_and_fetch(&value, i)+i;
#elif defined(USE_WINAPI)
	return _InterlockedExchangeAdd((PLONG) &value, i)+i;
#endif
}
QMUTEX_INLINEP int FXAtomicInt::operator+=(int i) throw() { return inc(i); }
QMUTEX_INLINEI int FXAtomicInt::decp() throw()
{
#ifdef __GNUC__
	return __sync_fetch_and_sub(&value, 1);
#elif defined(USE_WINAPI)
	return _InterlockedExchangeAdd((PLONG) &value, -1);
#endif
}
QMUTEX_INLINEP int FXAtomicInt::operator--(int) throw() { return decp(); }
QMUTEX_INLINEI int FXAtomicInt::pdec() throw()
{
#ifdef __GNUC__
	return __sync_sub_and_fetch(&value, 1);
#elif defined(USE_WINAPI)
	return _InterlockedDecrement((PLONG) &value);
#endif
}
QMUTEX_INLINEP int FXAtomicInt::operator--() throw() { return pdec(); }
QMUTEX_INLINEI int FXAtomicInt::fdec() throw()
{	// Returns -1, 0, +1 on value AFTER inc
	return pdec();
}
QMUTEX_INLINEP int FXAtomicInt::fastdec() throw() { return fdec(); }
QMUTEX_INLINEI int FXAtomicInt::dec(int i) throw()
{
#ifdef __GNUC__
	return __sync_fetch_and_sub(&value, i)-i;
#elif defined(USE_WINAPI)
	return _InterlockedExchangeAdd((PLONG) &value, -i)-i;
#endif
}
QMUTEX_INLINEP int FXAtomicInt::operator-=(int i) throw() { return dec(i); }
QMUTEX_INLINEI int FXAtomicInt::swapI(int i) throw()
{
#ifdef __GNUC__
	return __sync_lock_test_and_set(&value, i);
#elif defined(USE_WINAPI)
	return _InterlockedExchange((PLONG) &value, i);
#endif
}
QMUTEX_INLINEP int FXAtomicInt::swap(int i) throw() { return swapI(i); }
QMUTEX_INLINEI int FXAtomicInt::cmpXI(int compare, int newval) throw()
{
#ifdef __GNUC__
	return __sync_val_compare_and_swap(&value, compare, newval);
#elif defined(USE_WINAPI)
	return _InterlockedCompareExchange((PLONG) &value, newval, compare);
#endif
}
QMUTEX_INLINEP int FXAtomicInt::cmpX(int compare, int newval) throw() { return cmpXI(compare, newval); }

/**************************************************************************************************************/

QMUTEX_INLINEP void QShrdMemMutex::lock()
{
	FXuint start=!timeout ? 0 : FXProcess::getMsCount();
	while(lockvar.swapI(1) && (!timeout || FXProcess::getMsCount()-start<timeout-1))
#ifndef FX_SMPBUILD
		QThread::yield()
#endif
		;
}

QMUTEX_INLINEP bool QShrdMemMutex::tryLock() throw()
{
	return !lockvar.swapI(1);
}

/**************************************************************************************************************/

#ifndef FXDISABLE_THREADS
namespace QMutexImpl {

/* On POSIX, a more efficient mutex is one which avoids calling the kernel.
If MUTEX_USESEMA is defined, it uses a POSIX semaphore like an OS/2 event to
signal waiting threads when the holder is done - unfortunately, only sem_post()
is legal from a cleanup/signal handler, not sem_wait(). Linux is happy with
this, FreeBSD is not and must use a real POSIX mutex - nevertheless, we still
save on recursion overheads and get spin counts.
*/
#if !defined(__FreeBSD__) && !defined(__APPLE__)
#define MUTEX_USESEMA
#endif

/* 23rd June 2004 ned: Testing shows that a lot of the cost of using QMutex is
creating and deleting the kernel wait object. Therefore cache instances here.
*/
class FXDLLLOCAL KernelWaitObjectCache
{
	QShrdMemMutex lockvar;
public:
#ifdef USE_WINAPI
	typedef HANDLE WaitObjectType;
#endif
#ifdef USE_POSIX
#ifdef MUTEX_USESEMA
	typedef sem_t WaitObjectType;
#else
	typedef pthread_mutex_t WaitObjectType;
#endif
#endif
	struct Entry
	{
		WaitObjectType wo;
		Entry *next;
	};
private:
	Entry *entries;
	QMUTEX_INLINEI void lock() throw() { lockvar.lock(); }
	QMUTEX_INLINEI void unlock() throw() { lockvar.unlock(); }
public:
	bool dead;
	KernelWaitObjectCache() : lockvar(FXINFINITE) { }
	~KernelWaitObjectCache()
	{
		lock();
		FXRBOp undolock=FXRBObj(*this, &KernelWaitObjectCache::unlock);
		while(entries)
		{
#ifdef USE_WINAPI
			FXERRHWIN(CloseHandle(entries->wo));
#endif
#ifdef USE_POSIX
#ifdef MUTEX_USESEMA
			FXERRHOS(sem_destroy(&entries->wo));
#else
			FXERRHOS(pthread_mutex_destroy(&entries->wo));
#endif
#endif
			Entry *e=entries;
			entries=e->next;
			FXDELETE(e);
		}
		dead=true;
	}
	Entry *fetch() throw()
	{
		if(!entries) return 0;
		lock();
		if(!entries)
		{
			unlock();
			return 0;
		}
		Entry *ret=entries;
		entries=entries->next;
		unlock();
		return ret;
	}
	void addFreed(Entry *e) throw()
	{
		if(dead)
			delete e;
		else
		{
			lock();
			e->next=entries;
			entries=e;
			unlock();
		}
	}
};
extern QMUTEX_GLOBALS_FXAPI KernelWaitObjectCache waitObjectCache;
extern QMUTEX_GLOBALS_FXAPI bool yieldAfterLock;
extern QMUTEX_GLOBALS_FXAPI FXuint systemProcessors;

} // namespace QMutexImpl
#endif

struct FXDLLLOCAL QMutexPrivate
{
#ifndef FXDISABLE_THREADS
#ifdef USE_OURMUTEX
	FXAtomicInt lockCount, wakeSema;
	FXulong threadId;
	FXuint recurseCount, spinCount;
#ifdef USE_WINAPI
	QMutexImpl::KernelWaitObjectCache::Entry *wc;
#endif
#ifdef USE_POSIX
	QMutexImpl::KernelWaitObjectCache::Entry *sema;
#endif
#elif defined(USE_POSIX)
	QMutexImpl::KernelWaitObjectCache::Entry *m;
#endif
#endif
};

QMUTEX_INLINEP QMutex::QMutex(FXuint spinc) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QMutexPrivate);
#ifndef FXDISABLE_THREADS
#ifdef USE_OURMUTEX
	p->lockCount.set(-1);
	p->wakeSema.set(0);
	p->threadId=p->recurseCount=0;
	if(!QMutexImpl::systemProcessors)
		QMutexImpl::systemProcessors=FXProcess::noOfProcessors();
	p->spinCount=spinc; //(systemProcessors>1) ? spinc : 0;
#ifdef USE_WINAPI
	if(!(p->wc=QMutexImpl::waitObjectCache.fetch()))
	{
		FXERRHM(p->wc=new QMutexImpl::KernelWaitObjectCache::Entry);
		FXRBOp unwc=FXRBNew(p->wc);
		FXERRHWIN(p->wc->wo=CreateEvent(NULL, FALSE, FALSE, NULL));
		unwc.dismiss();
	}
#endif
#ifdef USE_POSIX
#ifdef MUTEX_USESEMA
	if(!(p->sema=QMutexImpl::waitObjectCache.fetch()))
	{
		FXERRHM(p->sema=new QMutexImpl::KernelWaitObjectCache::Entry);
		FXRBOp unsema=FXRBNew(p->sema);
		FXERRHOS(sem_init(&p->sema->wo, 0, 0));
		unsema.dismiss();
	}
#else
	if(!(p->sema=QMutexImpl::waitObjectCache.fetch()))
	{
		FXERRHM(p->sema=new QMutexImpl::KernelWaitObjectCache::Entry);
		FXRBOp unsema=FXRBNew(p->sema);
		FXERRHOS(pthread_mutex_init(&p->sema->wo, NULL));
		unsema.dismiss();
	}
#endif
#endif
#elif defined(USE_POSIX)
	pthread_mutexattr_t mattr;
	FXERRHOS(pthread_mutexattr_init(&mattr));
	FXERRHOS(pthread_mutexattr_setkind_np(&mattr, PTHREAD_MUTEX_RECURSIVE_NP));
	if(!(p->m=waitObjectCache.fetch()))
	{
		FXERRHM(p->m=new KernelWaitObjectCache::Entry);
		FXRBOp unm=FXRBNew(p->m);
		FXERRHOS(pthread_mutex_init(&p->m->wo, &mattr));
		unm.dismiss();
	}
	FXERRHOS(pthread_mutexattr_destroy(&mattr));
#endif
#endif
	unconstr.dismiss();
}

QMUTEX_INLINEP QMutex::~QMutex()
{ FXEXCEPTIONDESTRUCT1 {
	if(p)
	{	// Force exception if something else uses us after now
		QMutexPrivate *_p=p;
		p=0;
#ifndef FXDISABLE_THREADS
#ifdef USE_OURMUTEX
#ifdef USE_WINAPI
		if(_p->wc && _p->wc->wo)
		{
			FXERRHWIN(ResetEvent(_p->wc->wo));
			QMutexImpl::waitObjectCache.addFreed(_p->wc);
			_p->wc=0;
		}
#endif
#ifdef USE_POSIX
		if(_p->sema)
		{
			QMutexImpl::waitObjectCache.addFreed(_p->sema);
			_p->sema=0;
		}
#endif
#elif defined(USE_POSIX)
		if(_p->m)
		{
			QMutexImpl::waitObjectCache.addFreed(_p->m);
			p->m=0;
		}
#endif
#endif
		FXDELETE(_p);
	}
} FXEXCEPTIONDESTRUCT2; }

QMUTEX_INLINEP bool QMutex::isLocked() const
{
#ifndef FXDISABLE_THREADS
	return p->lockCount>=0;
#else
	return false;
#endif
}

QMUTEX_INLINEP FXuint QMutex::spinCount() const
{
#ifndef FXDISABLE_THREADS
#ifdef USE_OURMUTEX
	return p->spinCount;
#else
	return 0;
#endif
#else
	return 0;
#endif
}

QMUTEX_INLINEP void QMutex::setSpinCount(FXuint c)
{
#ifndef FXDISABLE_THREADS
#ifdef USE_OURMUTEX
	p->spinCount=c;
#endif
#endif
}

QMUTEX_INLINEI void QMutex::int_lock()
{
	assert(this);
	assert(p);
	if(!p) return;
#ifndef FXDISABLE_THREADS
#ifdef USE_OURMUTEX
	FXulong myid=QThread::id();
	if(!p->lockCount.finc())
	{	// Nothing owns me
		assert(p->threadId==0);
		assert(p->recurseCount==0);
		p->threadId=myid;
		p->recurseCount=1;
	}
	else
	{
		if(p->threadId==myid)
		{	// Recurse
			p->recurseCount++;
		}
		else
		{	// Spin & Wait
#if 0
			// In theory this implementation is meant to be faster, but it wasn't on my
			// dual Athlon :(
			int gotit;
			while(!(gotit=p->wakeSema.swapI(0)))
			{
				gotit=p->wakeSema.spinI(p->spinCount*3);
#else
			int gotit;
			while(!(gotit=p->wakeSema.swapI(0)))
			{
				for(FXuint n=0; n<p->spinCount; n++)
				{
					if(1==QMutexImpl::systemProcessors)
					{	// Always give up remaining time slice on uniprocessor machines
#ifdef USE_WINAPI
						Sleep(0);
#endif
#ifdef USE_POSIX
						sched_yield();
#endif
					}
					if((gotit=p->wakeSema.swapI(0)))
					{
						break;
					}
				}
#endif
				if(gotit)
					break;
				else
				{	// Ok, then wait on kernel
#ifdef USE_WINAPI
					WaitForSingleObject(p->wc->wo, INFINITE);
#endif
#if defined(USE_POSIX) && defined(MUTEX_USESEMA)
					QThread *c=QThread::current();
					if(c) c->disableTermination();
					sem_wait(&p->sema->wo);
					if(c) c->enableTermination();
#endif
				}
			}
#if defined(USE_POSIX) && !defined(MUTEX_USESEMA)
			pthread_mutex_lock(&p->sema->wo);
#endif
			//fxmessage(FXString("%1 %6 lock lc=%2, rc=%3, kc=%4, ti=%5\n").arg(QThread::id(),0, 16).arg(p->lockCount).arg(p->recurseCount).arg(p->kernelCount).arg(p->threadId, 0, 16).arg((FXuint)this,0,16).text());
			{	// Nothing owns me
				assert(p->threadId==0);
				p->threadId=myid;
				assert(p->recurseCount==0);
				p->recurseCount=1;
			}
		}
	}
#elif defined(USE_POSIX)
	FXERRHOS(pthread_mutex_lock(&p->m->wo));
#endif
	if(QMutexImpl::yieldAfterLock) QThread::yield();
#endif
}
QMUTEX_INLINEP void QMutex::lock() { int_lock(); }

QMUTEX_INLINEI void QMutex::int_unlock()
{
	assert(this);
	assert(p);
	if(!p) return;
#ifndef FXDISABLE_THREADS
#ifdef USE_OURMUTEX
	if(--p->recurseCount>0)
	{
		assert(p->recurseCount<0x80000000);		// Someone unlocked when not already locked
		p->lockCount.fdec();
	}
	else
	{
#ifdef DEBUG
		FXulong myid=QThread::id();	// For debug knowledge only
		if(myid && p->threadId)
			FXERRH(QThread::id()==p->threadId, "QMutex::unlock() performed by thread which did not own mutex", QMUTEX_BADUNLOCK, FXERRH_ISDEBUG);
#endif
		p->threadId=0;
		//fxmessage(FXString("%1 %6 unlock lc=%2, rc=%3, kc=%4, ti=%5\n").arg(QThread::id(),0, 16).arg(p->lockCount).arg(p->recurseCount).arg(p->kernelCount).arg(p->threadId, 0, 16).arg((FXuint)this,0,16).text());
		if(p->lockCount.fdec()>=0)
		{	// Others waiting
			p->wakeSema.set(1);	// Wake either a spinner or sleeper
#ifdef USE_WINAPI
			SetEvent(p->wc->wo);// Wake one sleeper
#endif
#if defined(USE_POSIX) && defined(MUTEX_USESEMA)
			sem_post(&p->sema->wo);
#endif
		}
#if defined(USE_POSIX) && !defined(MUTEX_USESEMA)
		pthread_mutex_unlock(&p->sema->wo);
#endif
	}
#elif defined(USE_POSIX)
	FXERRHOS(pthread_mutex_unlock(&p->m->wo));
#endif
#endif
}
QMUTEX_INLINEP void QMutex::unlock() { int_unlock(); }

QMUTEX_INLINEP bool QMutex::tryLock()
{
#ifndef FXDISABLE_THREADS
#ifdef USE_OURMUTEX
	FXulong myid=QThread::id();
	if(!p->lockCount.finc())
	{	// Nothing owns me
		assert(p->threadId==0);
		p->threadId=myid;
		p->recurseCount=1;
		return true;
	}
	else
	{	// Restore
		if(p->threadId==myid)
		{	// Recurse
			p->recurseCount++;
			return true;
		}
		// Restore
		p->lockCount.fdec();
		return false;
	}
#elif defined(USE_POSIX)

	if(0==pthread_mutex_trylock(&p->m->wo))
		return true;
	else
		return false;
#endif
#endif
}

QMUTEX_INLINEP bool QMutex::setMutexDebugYield(bool v)
{
#ifndef FXDISABLE_THREADS
	bool old=QMutexImpl::yieldAfterLock;
	QMutexImpl::yieldAfterLock=v;
	return old;
#endif
}

}
