/********************************************************************************
*                                                                               *
*                  FXAtomicInt and FXMutex implementations                      *
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

#ifndef FXBEING_INCLUDED_BY_FXTHREAD
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

namespace FX {

#if defined(_M_IX86) || defined(__i386__) || defined(_X86_) || defined(__x86_64__)
#define USE_X86 FX_X86PROCESSOR
#define USE_OURMUTEX
#if !defined(_MSC_VER) && !defined(__GNUC__)
#error Unknown compiler, therefore do not know how to invoke x86 assembler
#endif
#endif

#ifndef USE_OURMUTEX
#error Unsupported architecture, please add atomic int support to FXThread.cxx
#endif


FXMUTEX_INLINEI int FXAtomicInt::get() const throw()
{
	return value;
}
FXMUTEX_INLINEP FXAtomicInt::operator int() const throw() { return get(); }
FXMUTEX_INLINEI int FXAtomicInt::set(int i) throw()
{	// value=i; is write-buffered out and we need it immediate
#ifdef USE_X86
#ifdef _MSC_VER
	volatile int *val=&value;
	_asm
	{
		mov ecx, [val]
		mov edx, [i]
		xchg edx, [ecx]
	}
#endif
#ifdef __GNUC__
	int d;

	__asm__ __volatile__ ("xchgl %2,(%1)" : "=r" (d) : "r" (&value), "0" (i));
#endif
#elif defined(USE_WINAPI)
	InterlockedExchange((PLONG) &value, i);
#endif
	return i;
}
FXMUTEX_INLINEP int FXAtomicInt::operator=(int i) throw() { return set(i); }
FXMUTEX_INLINEI int FXAtomicInt::incp() throw()
{
#ifdef USE_X86
	int myret;
#ifdef _MSC_VER
	volatile int *val=&value;
	_asm
	{
		mov ecx, [val]
		mov eax, 1
#ifdef FX_SMPBUILD
		lock xadd [ecx], eax
#else
		xadd [ecx], eax
#endif
		mov [myret], eax
	}
#endif
#ifdef __GNUC__
	__asm__ __volatile__ (
#ifdef FX_SMPBUILD
		"lock xaddl %2,(%1)"
#else
		"xaddl %2,(%1)"
#endif
		: "=a" (myret) : "r" (&value), "a" (1));
#endif
	return myret;
#elif defined(USE_WINAPI)
	return InterlockedExchangeAdd((PLONG) &value, 1);
#endif
}
FXMUTEX_INLINEP int FXAtomicInt::operator++(int) throw() { return incp(); }
FXMUTEX_INLINEI int FXAtomicInt::pinc() throw()
{
#ifdef USE_X86
	int myret;
#ifdef _MSC_VER
	volatile int *val=&value;
	_asm
	{
		mov ecx, [val]
		mov eax, 1
#ifdef FX_SMPBUILD
		lock xadd [ecx], eax
#else
		xadd [ecx], eax
#endif
		inc eax
		mov [myret], eax
	}
#endif
#ifdef __GNUC__
	__asm__ __volatile__ (
#ifdef FX_SMPBUILD
		"lock xaddl %2,(%1)\n\tinc %%eax"
#else
		"xaddl %2,(%1)\n\tinc %%eax"
#endif
		: "=a" (myret) : "r" (&value), "a" (1));
#endif
	return myret;
#elif defined(USE_WINAPI)
	return InterlockedIncrement((PLONG) &value);
#endif
}
FXMUTEX_INLINEP int FXAtomicInt::operator++() throw() { return pinc(); }
FXMUTEX_INLINEI int FXAtomicInt::finc() throw()
{	// Returns -1, 0, +1 on value AFTER inc
#if defined(USE_X86)
	int myret;
#ifdef _MSC_VER
	volatile int *val=&value;
	_asm
	{
		mov ecx, [val]
#ifdef FX_SMPBUILD
		lock inc dword ptr [ecx]
#else
		inc dword ptr [ecx]
#endif
		jl retm1
		jg retp1
		mov [myret], 0
		jmp finish
retm1:	mov [myret], -1
		jmp finish
retp1:	mov [myret], 1
finish:
	}
#endif
#ifdef __GNUC__
	__asm__ __volatile__ (
#ifdef FX_SMPBUILD
		"lock incl (%1)\n"
#else
		"incl (%1)\n"
#endif
		"\tjl 1f\n\tjg 2f\n"
		"\tmov $0, %%eax\n\tjmp 3f\n"
		"1:\tmov $-1, %%eax\n\tjmp 3f\n"
		"2:\tmov $1, %%eax\n"
		"3:\n"
		: "=a" (myret) : "r" (&value));
#endif
	return myret;
#else
	return pinc();
#endif
}
FXMUTEX_INLINEP int FXAtomicInt::fastinc() throw() { return finc(); }
FXMUTEX_INLINEI int FXAtomicInt::inc(int i) throw()
{
#ifdef USE_X86
	int myret;
#ifdef _MSC_VER
	volatile int *val=&value;
	_asm
	{
		mov ecx, [val]
		mov eax, [i]
#ifdef FX_SMPBUILD
		lock xadd [ecx], eax
#else
		xadd [ecx], eax
#endif
		mov [myret], eax
	}
#endif
#ifdef __GNUC__
	__asm__ __volatile__ (
#ifdef FX_SMPBUILD
		"lock xaddl %2,(%1)"
#else
		"xaddl %2,(%1)"
#endif
		: "=a" (myret) : "r" (&value), "a" (i));
#endif
	return myret+i;
#elif defined(USE_WINAPI)
	return InterlockedExchangeAdd((PLONG) &value, i)+i;
#endif
}
FXMUTEX_INLINEP int FXAtomicInt::operator+=(int i) throw() { return inc(i); }
FXMUTEX_INLINEI int FXAtomicInt::decp() throw()
{
#ifdef USE_X86
	int myret;
#ifdef _MSC_VER
	volatile int *val=&value;
	_asm
	{
		mov ecx, [val]
		mov eax, 0xffffffff
#ifdef FX_SMPBUILD
		lock xadd [ecx], eax
#else
		xadd [ecx], eax
#endif
		mov [myret], eax
	}
#endif
#ifdef __GNUC__
	__asm__ __volatile__ (
#ifdef FX_SMPBUILD
		"lock xaddl %2,(%1)"
#else
		"xaddl %2,(%1)"
#endif
		: "=a" (myret) : "r" (&value), "a" (-1));
#endif
	return myret;
#elif defined(USE_WINAPI)
	return InterlockedExchangeAdd((PLONG) &value, -1);
#endif
}
FXMUTEX_INLINEP int FXAtomicInt::operator--(int) throw() { return decp(); }
FXMUTEX_INLINEI int FXAtomicInt::pdec() throw()
{
#ifdef USE_X86
	int myret;
#ifdef _MSC_VER
	volatile int *val=&value;
	_asm
	{
		mov ecx, [val]
		mov eax, -1
#ifdef FX_SMPBUILD
		lock xadd [ecx], eax
#else
		xadd [ecx], eax
#endif
		dec eax
		mov [myret], eax
	}
#endif
#ifdef __GNUC__
	__asm__ __volatile__ (
#ifdef FX_SMPBUILD
		"lock xaddl %2,(%1)\n\tdec %%eax"
#else
		"xaddl %2,(%1)\n\tdec %%eax"
#endif
		: "=a" (myret) : "r" (&value), "a" (-1));
#endif
	return myret;
#elif defined(USE_WINAPI)
	return InterlockedDecrement((PLONG) &value);
#endif
}
FXMUTEX_INLINEP int FXAtomicInt::operator--() throw() { return pdec(); }
FXMUTEX_INLINEI int FXAtomicInt::fdec() throw()
{	// Returns -1, 0, +1 on value AFTER inc
#if defined(USE_X86)
	int myret;
#ifdef _MSC_VER
	volatile int *val=&value;
	_asm
	{
		mov ecx, [val]
#ifdef FX_SMPBUILD
		lock dec dword ptr [ecx]
#else
		dec dword ptr [ecx]
#endif
		jl retm1
		jg retp1
		mov [myret], 0
		jmp finish
retm1:	mov [myret], -1
		jmp finish
retp1:	mov [myret], 1
finish:
	}
#endif
#ifdef __GNUC__
	__asm__ __volatile__ (
#ifdef FX_SMPBUILD
		"lock decl (%1)\n"
#else
		"decl (%1)\n"
#endif
		"\tjl 1f\n\tjg 2f\n"
		"\tmov $0, %%eax\n\tjmp 3f\n"
		"1:\tmov $-1, %%eax\n\tjmp 3f\n"
		"2:\tmov $1, %%eax\n"
		"3:\n"
		: "=a" (myret) : "r" (&value));
#endif
	return myret;
#else
	return pdec();
#endif
}
FXMUTEX_INLINEP int FXAtomicInt::fastdec() throw() { return fdec(); }
FXMUTEX_INLINEI int FXAtomicInt::dec(int i) throw()
{
#ifdef USE_X86
	int myret;
	i=-i;
#ifdef _MSC_VER
	volatile int *val=&value;
	_asm
	{
		mov ecx, [val]
		mov eax, [i]
#ifdef FX_SMPBUILD
		lock xadd [ecx], eax
#else
		xadd [ecx], eax
#endif
		mov [myret], eax
	}
#endif
#ifdef __GNUC__
	__asm__ __volatile__ (
#ifdef FX_SMPBUILD
		"lock xaddl %2,(%1)"
#else
		"xaddl %2,(%1)"
#endif
		: "=a" (myret) : "r" (&value), "a" (i));
#endif
	return myret+i;
#elif defined(USE_WINAPI)
	return InterlockedExchangeAdd((PLONG) &value, -i)-i;
#endif
}
FXMUTEX_INLINEP int FXAtomicInt::operator-=(int i) throw() { return dec(i); }
FXMUTEX_INLINEI int FXAtomicInt::swapI(int i) throw()
{
#ifdef USE_X86
	int myret;
#ifdef _MSC_VER
	volatile int *val=&value;
	_asm
	{
		mov ecx, [val]
		mov eax, [i]
		xchg eax, [ecx]
		mov [myret], eax
	}
#endif
#ifdef __GNUC__
	__asm__ __volatile__ ("xchgl %2,(%1)" : "=r" (myret) : "r" (&value), "0" (i));
#endif
	return myret;
#elif defined(USE_WINAPI)
	return InterlockedExchange((PLONG) &value, i);
#endif
}
FXMUTEX_INLINEP int FXAtomicInt::swap(int i) throw() { return swapI(i); }
FXMUTEX_INLINEI int FXAtomicInt::cmpXI(int compare, int newval) throw()
{
#ifdef USE_X86
	int myret;
#ifdef _MSC_VER
	volatile int *val=&value;
	_asm
	{
		mov ecx, [val]
		mov edx, [newval]
		mov eax, [compare]
#ifdef FX_SMPBUILD
		lock cmpxchg [ecx], edx
#else
		cmpxchg [ecx], edx
#endif
		mov [myret], eax
	}
#endif
#ifdef __GNUC__
	__asm__ __volatile__ (
#ifdef FX_SMPBUILD
		"pause\n\tlock cmpxchgl %2,(%1)"
#else
		"pause\n\tcmpxchgl %2,(%1)"
#endif
		: "=a" (myret) : "r" (&value), "r" (newval), "a" (compare));
#endif
	return myret;
#elif defined(USE_WINAPI)
	return InterlockedExchange((PLONG) &value, i);
#endif
}
FXMUTEX_INLINEP int FXAtomicInt::cmpX(int compare, int newval) throw() { return cmpXI(compare, newval); }
#if 0
/* Optimised spin just for FXMutex. This implementation avoids
costly xchg instructions which are very expensive on the x86 memory
bus as they effectively hang multiprocessing */
FXMUTEX_INLINEI int FXAtomicInt::spinI(int count) throw()
{
	int myret;
#ifdef USE_X86
#ifdef _MSC_VER
	volatile int *val=&value;
	_asm
	{
		xor eax, eax
		mov ecx, [val]
		mov edx, count
loop1:	xchg eax, [ecx]
		cmp eax, 1
		je exitloop
loop2:	dec edx
		je exitloop
		pause					// Hint to newer processors that this is a spin lock,
		cmp [ecx], 1			// nop otherwise
		jne loop2
		jmp loop1
exitloop:
		mov [myret], eax
	}
#endif
#ifdef __GNUC__
#error todo
	__asm__ __volatile__ (
#ifdef FX_SMPBUILD
		"pause\n\tlock cmpxchgl %2,(%1)"
#else
		"pause\n\tcmpxchgl %2,(%1)"
#endif
		: "=a" (myret) : "r" (&value), "r" (newval), "a" (compare));
#endif
	return myret;
#elif defined(USE_WINAPI)
	for(int n=0; n<count && (myret=InterlockedExchange((PLONG) &value, 1)); n++)
	return myret;
#endif
}
#endif

/**************************************************************************************************************/

FXMUTEX_INLINEP void FXShrdMemMutex::lock()
{
	FXuint start=(FXINFINITE==timeout) ? 0 : FXProcess::getMsCount();
	while(lockvar.swapI(1) && (FXINFINITE==timeout || FXProcess::getMsCount()-start<timeout))
#ifndef FX_SMPBUILD
		FXThread::yield()
#endif
		;
}

FXMUTEX_INLINEP bool FXShrdMemMutex::tryLock()
{
	return !lockvar.swapI(1);
}

/**************************************************************************************************************/

namespace FXMutexImpl {

/* On POSIX, a more efficient mutex is one which avoids calling the kernel.
If MUTEX_USESEMA is defined, it uses a POSIX semaphore like an OS/2 event to
signal waiting threads when the holder is done - unfortunately, only sem_post()
is legal from a cleanup/signal handler, not sem_wait(). Linux is happy with
this, FreeBSD is not and must use a real POSIX mutex - nevertheless, we still
save on recursion overheads and get spin counts.
*/
#ifndef __FreeBSD__
#define MUTEX_USESEMA
#endif

/* 23rd June 2004 ned: Testing shows that a lot of the cost of using FXMutex is
creating and deleting the kernel wait object. Therefore cache instances here.
*/
class FXDLLLOCAL KernelWaitObjectCache
{
	FXShrdMemMutex lockvar;
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
	FXMUTEX_INLINEI void lock() throw() { lockvar.lock(); }
	FXMUTEX_INLINEI void unlock() throw() { lockvar.unlock(); }
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
extern FXMUTEX_GLOBALS_FXAPI KernelWaitObjectCache waitObjectCache;
extern FXMUTEX_GLOBALS_FXAPI bool yieldAfterLock;
extern FXMUTEX_GLOBALS_FXAPI FXuint systemProcessors;

} // namespace FXMutexImpl

struct FXDLLLOCAL FXMutexPrivate
{
#ifdef USE_OURMUTEX
	FXAtomicInt lockCount, wakeSema;
	FXulong threadId;
	FXuint recurseCount, spinCount;
#ifdef USE_WINAPI
	FXMutexImpl::KernelWaitObjectCache::Entry *wc;
#endif
#ifdef USE_POSIX
	FXMutexImpl::KernelWaitObjectCache::Entry *sema;
#endif
#elif defined(USE_POSIX)
	FXMutexImpl::KernelWaitObjectCache::Entry *m;
#endif
};

FXMUTEX_INLINEP FXMutex::FXMutex(FXuint spinc) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new FXMutexPrivate);
#ifdef USE_OURMUTEX
	p->lockCount.set(-1);
	p->wakeSema.set(0);
	p->threadId=p->recurseCount=0;
	if(!FXMutexImpl::systemProcessors)
		FXMutexImpl::systemProcessors=FXProcess::noOfProcessors();
	p->spinCount=spinc; //(systemProcessors>1) ? spinc : 0;
#ifdef USE_WINAPI
	if(!(p->wc=FXMutexImpl::waitObjectCache.fetch()))
	{
		FXERRHM(p->wc=new FXMutexImpl::KernelWaitObjectCache::Entry);
		FXRBOp unwc=FXRBNew(p->wc);
		FXERRHWIN(p->wc->wo=CreateEvent(NULL, FALSE, FALSE, NULL));
		unwc.dismiss();
	}
#endif
#ifdef USE_POSIX
#ifdef MUTEX_USESEMA
	if(!(p->sema=FXMutexImpl::waitObjectCache.fetch()))
	{
		FXERRHM(p->sema=new FXMutexImpl::KernelWaitObjectCache::Entry);
		FXRBOp unsema=FXRBNew(p->sema);
		FXERRHOS(sem_init(&p->sema->wo, 0, 0));
		unsema.dismiss();
	}
#else
	if(!(p->sema=FXMutexImpl::waitObjectCache.fetch()))
	{
		FXERRHM(p->sema=new FXMutexImpl::KernelWaitObjectCache::Entry);
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
	unconstr.dismiss();
}

FXMUTEX_INLINEP FXMutex::~FXMutex()
{ FXEXCEPTIONDESTRUCT1 {
	if(p)
	{	// Force exception if something else uses us after now
		FXMutexPrivate *_p=p;
		p=0;
#ifdef USE_OURMUTEX
#ifdef USE_WINAPI
		if(_p->wc && _p->wc->wo)
		{
			FXERRHWIN(ResetEvent(_p->wc->wo));
			FXMutexImpl::waitObjectCache.addFreed(_p->wc);
			_p->wc=0;
		}
#endif
#ifdef USE_POSIX
		if(_p->sema)
		{
			FXMutexImpl::waitObjectCache.addFreed(_p->sema);
			_p->sema=0;
		}
#endif
#elif defined(USE_POSIX)
		if(_p->m)
		{
			FXMutexImpl::waitObjectCache.addFreed(_p->m);
			p->m=0;
		}
#endif
		FXDELETE(_p);
	}
} FXEXCEPTIONDESTRUCT2; }

FXMUTEX_INLINEP bool FXMutex::isLocked() const
{
	return p->lockCount>=0;
}

FXMUTEX_INLINEP FXuint FXMutex::spinCount() const
{
	return p->spinCount;
}

FXMUTEX_INLINEP void FXMutex::setSpinCount(FXuint c)
{
	p->spinCount=c;
}

FXMUTEX_INLINEI void FXMutex::int_lock()
{
	assert(this);
	assert(p);
	if(!p) return;
#ifdef USE_OURMUTEX
	FXulong myid=FXThread::id();
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
					if(1==FXMutexImpl::systemProcessors)
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
					FXThread *c=FXThread::current();
					if(c) c->disableTermination();
					sem_wait(&p->sema->wo);
					if(c) c->enableTermination();
#endif
				}
			}
#if defined(USE_POSIX) && !defined(MUTEX_USESEMA)
			pthread_mutex_lock(&p->sema->wo);
#endif
			//fxmessage(FXString("%1 %6 lock lc=%2, rc=%3, kc=%4, ti=%5\n").arg(FXThread::id(),0, 16).arg(p->lockCount).arg(p->recurseCount).arg(p->kernelCount).arg(p->threadId, 0, 16).arg((FXuint)this,0,16).text());
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
	if(FXMutexImpl::yieldAfterLock) FXThread::yield();
}
FXMUTEX_INLINEP void FXMutex::lock() { int_lock(); }

FXMUTEX_INLINEI void FXMutex::int_unlock()
{
	assert(this);
	assert(p);
	if(!p) return;
#ifdef USE_OURMUTEX
	if(--p->recurseCount>0)
	{
		assert(p->recurseCount<0x80000000);		// Someone unlocked when not already locked
		p->lockCount.fdec();
	}
	else
	{
#ifdef DEBUG
		FXulong myid=FXThread::id();	// For debug knowledge only
		if(myid && p->threadId)
			FXERRH(FXThread::id()==p->threadId, "FXMutex::unlock() performed by thread which did not own mutex", FXMUTEX_BADUNLOCK, FXERRH_ISDEBUG);
#endif
		p->threadId=0;
		//fxmessage(FXString("%1 %6 unlock lc=%2, rc=%3, kc=%4, ti=%5\n").arg(FXThread::id(),0, 16).arg(p->lockCount).arg(p->recurseCount).arg(p->kernelCount).arg(p->threadId, 0, 16).arg((FXuint)this,0,16).text());
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
}
FXMUTEX_INLINEP void FXMutex::unlock() { int_unlock(); }

FXMUTEX_INLINEP bool FXMutex::tryLock()
{
#ifdef USE_OURMUTEX
	FXulong myid=FXThread::id();
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
}

FXMUTEX_INLINEP bool FXMutex::setMutexDebugYield(bool v)
{
	bool old=FXMutexImpl::yieldAfterLock;
	FXMutexImpl::yieldAfterLock=v;
	return old;
}

}