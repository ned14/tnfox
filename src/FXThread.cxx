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

//#undef FX_SMPBUILD

#include <qvaluelist.h>
#include <qptrlist.h>
#include <qptrdict.h>
#include <qsortedlist.h>
#include "FXThread.h"
#include "FXErrCodes.h"
#include "FXTrans.h"

#ifdef FXDISABLE_THREADS
#undef USE_WINAPI
#undef USE_POSIX
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
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <signal.h>
#endif
#endif

#include "FXApp.h"
#include "FXProcess.h"
#include <assert.h>
#include "FXRollback.h"
#include "FXPtrHold.h"
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

#if defined(_M_IX86) || defined(__i386__) || defined(_X86_)
#define USE_X86 FX_X86PROCESSOR
#define USE_OURMUTEX
#if !defined(_MSC_VER) && !defined(__GNUC__)
#error Unknown compiler, therefore do not know how to invoke x86 assembler
#endif
#endif

#ifdef USE_WINAPI
#define DECLARERET int ret=1
#define ERRCHK FXERRHWIN(ret)
//#define   LOCK EnterCriticalSection(&p->cs)
//#define UNLOCK LeaveCriticalSection(&p->cs)
#elif defined(USE_POSIX)
#define DECLARERET int ret=0
#define ERRCHK FXERRHOS(ret)
//#define   LOCK FXERRHOS(pthread_mutex_lock(&p->m))
//#define UNLOCK FXERRHOS(pthread_mutex_unlock(&p->m))
#else
#define DECLARERET
#define ERRCHK
#define LOCK
#define UNLOCK
#endif

inline int FXAtomicInt::get() const throw()
{
	return value;
}
FXAtomicInt::operator int() const throw() { return get(); }
inline int FXAtomicInt::set(int i) throw()
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

	__asm__ __volatile__ ("xchg %2,(%1)" : "=r" (d) : "r" (&value), "0" (i));
#endif
#elif defined(USE_WINAPI)
	InterlockedExchange((PLONG) &value, i);
#else
	LOCK;
	p->value=i;
	UNLOCK;
	return v;
#endif
	return i;
}
int FXAtomicInt::operator=(int i) throw() { return set(i); }
inline int FXAtomicInt::incp() throw()
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
		"lock xadd %2,(%1)"
#else
		"xadd %2,(%1)"
#endif
		: "=a" (myret) : "r" (&value), "a" (1));
#endif
	return myret;
#elif defined(USE_WINAPI)
	return InterlockedExchangeAdd((PLONG) &value, 1);
#else
	int v;
	LOCK;
	v=value++;
	UNLOCK;
	return v;
#endif
}
int FXAtomicInt::operator++(int) throw() { return incp(); }
inline int FXAtomicInt::pinc() throw()
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
		"lock xadd %2,(%1)\n\tinc %%eax"
#else
		"xadd %2,(%1)\n\tinc %%eax"
#endif
		: "=a" (myret) : "r" (&value), "a" (1));
#endif
	return myret;
#elif defined(USE_WINAPI)
	return InterlockedIncrement((PLONG) &value);
#else
	int v;
	LOCK;
	v=++value;
	UNLOCK;
	return v;
#endif
}
int FXAtomicInt::operator++() throw() { return pinc(); }
inline int FXAtomicInt::finc() throw()
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
		"\tjl retm1a\n\tjg retp1a\n"
		"\tmov $0, %%eax\n\tjmp finisha\n"
		"retm1a:\tmov $-1, %%eax\n\tjmp finisha\n"
		"retp1a:\tmov $1, %%eax\nfinisha:\n"
		: "=a" (myret) : "r" (&value));
#endif
	return myret;
#else
	return pinc();
#endif
}
int FXAtomicInt::fastinc() throw() { return finc(); }
inline int FXAtomicInt::inc(int i) throw()
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
		"lock xadd %2,(%1)"
#else
		"xadd %2,(%1)"
#endif
		: "=a" (myret) : "r" (&value), "a" (i));
#endif
	return myret+i;
#elif defined(USE_WINAPI)
	return InterlockedExchangeAdd((PLONG) &value, i)+i;
#else
	int v;
	LOCK;
	v=value+=i;
	UNLOCK;
	return v;
#endif
}
int FXAtomicInt::operator+=(int i) throw() { return inc(i); }
inline int FXAtomicInt::decp() throw()
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
		"lock xadd %2,(%1)"
#else
		"xadd %2,(%1)"
#endif
		: "=a" (myret) : "r" (&value), "a" (-1));
#endif
	return myret;
#elif defined(USE_WINAPI)
	return InterlockedExchangeAdd((PLONG) &value, -1);
#else
	int v;
	LOCK;
	v=value--;
	UNLOCK;
	return v;
#endif
}
int FXAtomicInt::operator--(int) throw() { return decp(); }
inline int FXAtomicInt::pdec() throw()
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
		"lock xadd %2,(%1)\n\tdec %%eax"
#else
		"xadd %2,(%1)\n\tdec %%eax"
#endif
		: "=a" (myret) : "r" (&value), "a" (-1));
#endif
	return myret;
#elif defined(USE_WINAPI)
	return InterlockedDecrement((PLONG) &value);
#else
	int v;
	LOCK;
	v=--value;
	UNLOCK;
	return v;
#endif
}
int FXAtomicInt::operator--() throw() { return pdec(); }
inline int FXAtomicInt::fdec() throw()
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
		"\tjl retm1b\n\tjg retp1b\n"
		"\tmov $0, %%eax\n\tjmp finishb\n"
		"retm1b:\tmov $-1, %%eax\n\tjmp finishb\n"
		"retp1b:\tmov $1, %%eax\nfinishb:\n"
		: "=a" (myret) : "r" (&value));
#endif
	return myret;
#else
	return pdec();
#endif
}
int FXAtomicInt::fastdec() throw() { return fdec(); }
inline int FXAtomicInt::dec(int i) throw()
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
		"lock xadd %2,(%1)"
#else
		"xadd %2,(%1)"
#endif
		: "=a" (myret) : "r" (&value), "a" (i));
#endif
	return myret+i;
#elif defined(USE_WINAPI)
	return InterlockedExchangeAdd((PLONG) &value, -i)-i;
#else
	int v;
	LOCK;
	v=value-=i;
	UNLOCK;
	return v;
#endif
}
int FXAtomicInt::operator-=(int i) throw() { return dec(i); }
inline int FXAtomicInt::swapI(int i) throw()
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
	__asm__ __volatile__ ("xchg %2,(%1)" : "=r" (myret) : "r" (&value), "0" (i));
#endif
	return myret;
#elif defined(USE_WINAPI)
	return InterlockedExchange((PLONG) &value, i);
#else
	int v;
	LOCK;
	v=value;
	p->value=i;
	UNLOCK;
	return v;
#endif
}
int FXAtomicInt::swap(int i) throw() { return swapI(i); }
inline int FXAtomicInt::cmpXI(int compare, int newval) throw()
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
		"pause\n\tlock cmpxchg %2,(%1)"
#else
		"pause\n\tcmpxchg %2,(%1)"
#endif
		: "=a" (myret) : "r" (&value), "r" (newval), "a" (compare));
#endif
	return myret;
#elif defined(USE_WINAPI)
	return InterlockedExchange((PLONG) &value, i);
#else
	int v;
	LOCK;
	v=value;
	p->value=i;
	UNLOCK;
	return v;
#endif
}
int FXAtomicInt::cmpX(int compare, int newval) throw() { return cmpXI(compare, newval); }
#if 0
/* Optimised spin just for FXMutex. This implementation avoids
costly xchg instructions which are very expensive on the x86 memory
bus as they effectively hang multiprocessing */
inline int FXAtomicInt::spinI(int count) throw()
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
		"pause\n\tlock cmpxchg %2,(%1)"
#else
		"pause\n\tcmpxchg %2,(%1)"
#endif
		: "=a" (myret) : "r" (&value), "r" (newval), "a" (compare));
#endif
	return myret;
#elif defined(USE_WINAPI)
	for(int n=0; n<count && (myret=InterlockedExchange((PLONG) &value, 1)); n++)
	return myret;
#else
	for(int n=0; n<count; n++)
	{
		LOCK;
		myret=value;
		value=1;
		UNLOCK;
		if(!myret) return myret;
	}
	return myret;
#endif
}
#endif

/**************************************************************************************************************/

void FXShrdMemMutex::lock()
{
	FXuint start=(FXINFINITE==timeout) ? 0 : FXProcess::getMsCount();
	while(lockvar.swapI(1) && (FXINFINITE==timeout || FXProcess::getMsCount()-start<timeout))
#ifndef FX_SMPBUILD
		FXThread::yield()
#endif
		;
}

bool FXShrdMemMutex::tryLock()
{
	return !lockvar.swapI(1);
}

/**************************************************************************************************************/
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
static class FXDLLLOCAL KernelWaitObjectCache
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
	inline void lock() throw() { lockvar.lock(); }
	inline void unlock() throw() { lockvar.unlock(); }
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
} waitObjectCache;

struct FXDLLLOCAL FXMutexPrivate
{
#ifdef USE_OURMUTEX
	FXAtomicInt lockCount, wakeSema;
	FXuint threadId, recurseCount, spinCount;
#ifdef USE_WINAPI
	KernelWaitObjectCache::Entry *wc;
#endif
#ifdef USE_POSIX
	KernelWaitObjectCache::Entry *sema;
#endif
#elif defined(USE_POSIX)
	KernelWaitObjectCache::Entry *m;
#endif
};
static bool yieldAfterLock=false;
static FXuint systemProcessors;

FXMutex::FXMutex(FXuint spinc) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new FXMutexPrivate);
#ifdef USE_OURMUTEX
	p->lockCount.set(-1);
	p->wakeSema.set(0);
	p->threadId=p->recurseCount=0;
	if(!systemProcessors)
		systemProcessors=FXProcess::noOfProcessors();
	p->spinCount=spinc; //(systemProcessors>1) ? spinc : 0;
#ifdef USE_WINAPI
	if(!(p->wc=waitObjectCache.fetch()))
	{
		FXERRHM(p->wc=new KernelWaitObjectCache::Entry);
		FXRBOp unwc=FXRBNew(p->wc);
		FXERRHWIN(p->wc->wo=CreateEvent(NULL, FALSE, FALSE, NULL));
		unwc.dismiss();
	}
#endif
#ifdef USE_POSIX
#ifdef MUTEX_USESEMA
	if(!(p->sema=waitObjectCache.fetch()))
	{
		FXERRHM(p->sema=new KernelWaitObjectCache::Entry);
		FXRBOp unsema=FXRBNew(p->sema);
		FXERRHOS(sem_init(&p->sema->wo, 0, 0));
		unsema.dismiss();
	}
#else
	if(!(p->sema=waitObjectCache.fetch()))
	{
		FXERRHM(p->sema=new KernelWaitObjectCache::Entry);
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

FXMutex::~FXMutex()
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
			waitObjectCache.addFreed(_p->wc);
			_p->wc=0;
		}
#endif
#ifdef USE_POSIX
		if(_p->sema)
		{
			waitObjectCache.addFreed(_p->sema);
			_p->sema=0;
		}
#endif
#elif defined(USE_POSIX)
		if(_p->m)
		{
			waitObjectCache.addFreed(_p->m);
			p->m=0;
		}
#endif
		FXDELETE(_p);
	}
} FXEXCEPTIONDESTRUCT2; }

bool FXMutex::isLocked() const
{
	return p->lockCount>=0;
}

FXuint FXMutex::spinCount() const
{
	return p->spinCount;
}

void FXMutex::setSpinCount(FXuint c)
{
	p->spinCount=c;
}

inline void FXMutex::int_lock()
{
	assert(this);
	assert(p);
	if(!p) return;
#ifdef USE_OURMUTEX
	FXuint myid=FXThread::id();
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
					if(1==systemProcessors)
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
	if(yieldAfterLock) FXThread::yield();
}
void FXMutex::lock() { int_lock(); }

void FXMutex::int_unlock()
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
		FXuint myid=FXThread::id();	// For debug knowledge only
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
void FXMutex::unlock() { int_unlock(); }

bool FXMutex::tryLock()
{
#ifdef USE_OURMUTEX
	FXuint myid=FXThread::id();
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

bool FXMutex::setMutexDebugYield(bool v)
{
	bool old=yieldAfterLock;
	yieldAfterLock=v;
	return old;
}

/**************************************************************************************************************/
class FXWaitConditionPrivate : public FXMutex
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
	FXWaitConditionPrivate() : waitcnt(0), FXMutex() { }
};

FXWaitCondition::FXWaitCondition(bool autoreset, bool initialstate) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new FXWaitConditionPrivate);
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

FXWaitCondition::~FXWaitCondition()
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

bool FXWaitCondition::wait(FXuint time)
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
		if(FXThread::current()->inCleanup())
			fxwarning("WARNING: Calling FXWaitCondition::wait() from a cleanup handler is non-portable\n");
#endif
#ifdef USE_WINAPI
		{
			HANDLE waitlist[2];
			waitlist[0]=p->wc;
			waitlist[1]=FXThread::int_cancelWaiterHandle();
			ret=ResetEvent(p->wc);
			h.unlock();
			if(!ret) goto exit;
			ret=WaitForMultipleObjects(2, waitlist, FALSE, (FXINFINITE!=time) ? time : INFINITE);
		}
		if(ret==WAIT_OBJECT_0+1) FXThread::current()->checkForTerminate();
exit:
		h.relock();
		if(ret!=WAIT_TIMEOUT && ret!=WAIT_OBJECT_0 && ret!=WAIT_OBJECT_0+1)
		{
			p->waitcnt--;
			FXERRHWIN(0);
		}
		if(ret==WAIT_TIMEOUT)
			p->waitcnt--;
		else if(isAutoReset)
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
		if(ret && ETIMEDOUT!=ret)
		{
			p->waitcnt--;
			FXERRHOS(ret);
		}
		if(ret==ETIMEDOUT)
			p->waitcnt--;
		else if(isAutoReset)
			isSignalled=false;
		return (ETIMEDOUT!=ret);
#endif
	}
#endif
	return true;
}

void FXWaitCondition::wakeOne()
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

void FXWaitCondition::wakeAll()
{
#ifndef FXDISABLE_THREADS
	if(p)
	{
		DECLARERET;
		FXMtxHold h(p);
		if(p->waitcnt)
		{
#ifdef USE_WINAPI
			for(int n=0; n<p->waitcnt; n++)
			{
				if(!(ret=SetEvent(p->wc))) goto exit;
			}
#endif
#ifdef USE_POSIX
			pthread_mutex_lock(&p->m);
			ret=pthread_cond_broadcast(&p->wc);
			pthread_mutex_unlock(&p->m);
			if(ret) goto exit;
			p->waitcnt=0;
#endif
		}
exit:
		if(!p->waitcnt || !isAutoReset)
			isSignalled=true;	// Let next thread through
		ERRCHK;
	}
#endif
}

void FXWaitCondition::reset()
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
class FXRWMutexPrivate : public FXMutex
{
#ifndef FXDISABLE_THREADS
public:
	struct ReadInfo
	{
		volatile int count;
	} read;
	FXWaitCondition readcntZeroed;
	struct PreWriteInfo
	{
		volatile int count, rws;
	} prewrite;
	FXWaitCondition prewritecntZeroed;
	struct WriteInfo
	{
		FXuint threadid;
		volatile int count;
		bool readLockLost;
	} write;
	FXWaitCondition writecntZeroed;
	FXThreadLocalStorageBase myreadcnt;
	FXRWMutexPrivate() : readcntZeroed(false, false), prewritecntZeroed(false, false), writecntZeroed(false, false), FXMutex() { }
	FXuint readCnt() { return (FXuint) myreadcnt.getPtr(); }
	void setReadCnt(FXuint v) { myreadcnt.setPtr((void *) v); }
	void incReadCnt() { setReadCnt(readCnt()+1); }
	void decReadCnt() { setReadCnt(readCnt()-1); }
#endif
};

FXRWMutex::FXRWMutex() : p(0)
{
	FXERRHM(p=new FXRWMutexPrivate);
#ifndef FXDISABLE_THREADS
	p->read.count=0;
	p->prewrite.count=0;
	p->prewrite.rws=0;

	p->write.threadid=0;
	p->write.count=0;
	p->write.readLockLost=false;
#endif
}

FXRWMutex::~FXRWMutex()
{ FXEXCEPTIONDESTRUCT1 {
	if(p)
	{
		lock(true);
		FXMtxHold h(p);
		FXRWMutexPrivate *_p=p;
		p=0;
		h.unlock();
		FXDELETE(_p);
	}
} FXEXCEPTIONDESTRUCT2; }

FXuint FXRWMutex::spinCount() const
{
	if(p)
		return p->spinCount();
	else
		return 0;
}

void FXRWMutex::setSpinCount(FXuint c)
{
	if(p) p->setSpinCount(c);
}

/* ned 5th Nov 2002: Third attempt at this algorithm ... :)

Problem with old algorithm was it was starving writers when lots
of stuff was reading and that was causing poor performance. This new algorithm uses
thread local storage to replace the old array so I've also removed the limit on max
threads.
*/
inline bool FXRWMutex::_lock(FXMtxHold &h, bool write)
{
	bool lockLost=false;
#ifndef FXDISABLE_THREADS
	FXuint myid=FXThread::id();
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
					FXThread::current()->disableTermination();
					p->writecntZeroed.wait();
					FXThread::current()->enableTermination();
					h.relock();
				}
				// Wait for readers to exit, except those read locks I hold
				thisthreadreadcnt=p->readCnt();
				p->read.count-=thisthreadreadcnt;
				p->prewrite.rws+=thisthreadreadcnt;
				while(p->read.count)
				{
					h.unlock();
					FXThread::current()->disableTermination();
					p->readcntZeroed.wait();
					FXThread::current()->enableTermination();
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
				FXThread::current()->disableTermination();
				p->prewritecntZeroed.wait();
				FXThread::current()->enableTermination();
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

void FXRWMutex::unlock(bool write)
{
#ifndef FXDISABLE_THREADS
	if(p)
	{
		FXMtxHold h(p);
		if(write)
		{
#ifdef DEBUG
			FXuint myid=FXThread::id();
			FXERRH(p->write.threadid==myid, "FXRWMutex::unlock(true) called by thread which did not have write lock", FXRWMUTEX_BADUNLOCK, FXERRH_ISDEBUG);
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
			FXERRH(p->readCnt(), "FXRWMutex::unlock(false) called by thread which did not have read lock", FXRWMUTEX_BADUNLOCK, FXERRH_ISDEBUG);
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

bool FXRWMutex::lock(bool write)
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

bool FXRWMutex::trylock(bool write)
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
class FXThreadLocalStorageBasePrivate
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

FXThreadLocalStorageBase::FXThreadLocalStorageBase(void *initval) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new FXThreadLocalStorageBasePrivate);
#ifdef FXDISABLE_THREADS
	p->data=initval;
#endif
#ifdef USE_WINAPI
	p->key=TlsAlloc();
	FXERRH(p->key!=(DWORD) -1, FXTrans::tr("FXThreadLocalStorage", "Exceeded Windows TLS hard key number limit"), FXTHREADLOCALSTORAGE_NOMORETLS, FXERRH_ISDEBUG);
	FXERRHWIN(TlsSetValue(p->key, initval));
#endif
#ifdef USE_POSIX
	FXERRHOS(pthread_key_create(&p->tls, NULL));
	FXERRHOS(pthread_setspecific(p->tls, initval));
#endif
	unconstr.dismiss();
}


FXThreadLocalStorageBase::~FXThreadLocalStorageBase() FXERRH_NODESTRUCTORMOD
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

void FXThreadLocalStorageBase::setPtr(void *val)
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

void *FXThreadLocalStorageBase::getPtr() const
{
	if(p)
#ifdef FXDISABLE_THREADS
		return p->data;
#endif
#ifdef USE_WINAPI
		return TlsGetValue(p->key);
#endif
#ifdef USE_POSIX
		return pthread_getspecific(p->tls);
#endif
	else
		return 0;
}

/**************************************************************************************************************/
class FXThreadPrivate : public FXMutex
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
	FXThread::ThreadScheduler threadLocation;
	FXuint id;
	FXThread *creator;
	FXWaitCondition *startedwc, *stoppedwc;
	struct CleanupCall
	{
		Generic::BoundFunctorV *code;
		bool inThread;
		CleanupCall(Generic::BoundFunctorV *_code, bool _inThread) : code(_code), inThread(_inThread) { }
		~CleanupCall() { FXDELETE(code); }
	};
	QPtrList<CleanupCall> cleanupcalls;
	static FXThreadLocalStorage<FXThread> currentThread;
	static FXAtomicInt idlistlock;
	static QValueList<FXushort> idlist;
	static void setResultAddr(FXThread *t, void **res) { t->p->result=res; }
	static void run(FXThread *t);
	static void cleanup(FXThread *t);
	static void forceCleanup(FXThread *t);
	FXThreadPrivate(bool autodel, FXuval stksize, FXThread::ThreadScheduler threadloc)
		: autodelete(autodel), plsCancel(false), stackSize(stksize), threadLocation(threadloc), startedwc(0), stoppedwc(0), cleanupcalls(true), FXMutex()
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
	~FXThreadPrivate()
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
FXThreadLocalStorage<FXThread> FXThreadPrivate::currentThread;
#ifdef USE_OURTHREADID
FXAtomicInt FXThreadPrivate::idlistlock;
QValueList<FXushort> FXThreadPrivate::idlist;
#endif
static FXThread *primaryThread;
// An internal dummy FXThread used to make primary thread code work correctly
class FXPrimaryThread : public FXThread
{
	static void zeroCThread()
	{
		FXThreadPrivate::currentThread=0;
	}
public:
	FXPrimaryThread() : FXThread("Primary thread")
	{
		FX::primaryThread=this;
#ifdef USE_WINAPI
		p->id=(FXuint) GetCurrentThreadId();
#endif
#ifdef USE_POSIX
#ifdef USE_OURTHREADID
		p->id=(FXProcess::id()<<16)+0xffff;	// Highest id possible for POSIX
#else
		p->id=(FXuint) pthread_self();
#endif
#endif
		p->currentThread=this;
		isRunning=true;
	}
	~FXPrimaryThread()
	{
		FXThreadPrivate::forceCleanup(this);
		// Need to delay setting currentThread to zero to after cleanup calls
		FXThread::addCleanupCall(Generic::BindFuncN(zeroCThread));
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
	FXThread::CreationUpcallSpec upcallv;
	bool inThread;
	CreationUpcall(FXThread::CreationUpcallSpec &_upcallv, bool _inThread) : upcallv(_upcallv), inThread(_inThread) { }
};
static FXMutex creationupcallslock;
static QPtrList<CreationUpcall> creationupcalls(true);

class FXThreadIntException : public FXException
{
public:
	FXThreadIntException() : FXException(0, 0, "Internal FXThread cancellation exception", FXEXCEPTION_INTTHREADCANCEL, FXERRH_ISINFORMATIONAL) { }
};

static void cleanup_thread(void *t)
{
	FXThread *tt=(FXThread *) t;
#ifdef DEBUG
	fxmessage("Thread %d (%s) cleanup\n", FXThread::id(), tt->name());
#endif
	FXThreadPrivate::cleanup(tt);
}
static void *start_thread(void *t)
{
	void *result=0;		// This is stored into by code further down the call stack
	FXThread *tt=(FXThread *) t;
	if(!tt->isValid())
	{
		fxwarning("Thread appears to have been destructed before it could start!\n");
		return (void *)-1;
	}
	FXThreadPrivate::setResultAddr(tt, &result);
	try // Must use normal try here, otherwise the nested exception framework leaks memory
	{
		FXThreadPrivate::run(tt);
		return result;
	}
	catch(FXThreadIntException &)
	{
		cleanup_thread(t);
		return result;
	}
	catch(FXException &e)
	{
#ifdef DEBUG
		fxmessage("Exception occurred in thread %d (%s): %s\n", FXThread::id(), tt->name(), e.report().text());
#endif
		if(e.flags() & FXERRH_ISFATAL)
		{
			FXThreadPrivate::forceCleanup(tt);
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
				fxmessage("Exception occurred in cleanup of thread %d (%s) after other exception\n", FXThread::id(), tt->name());
#endif
				if(e.flags() & FXERRH_ISFATAL)
				{
					FXThreadPrivate::forceCleanup(tt);
					if(FXApp::instance()) FXApp::instance()->exit(1);
					FXProcess::exit(1);
				}
				//QThread::postEvent(TProcess::getProcess(), new TNonGUIThreadExceptionEvent(e));
			}
		}
		FXThreadPrivate::forceCleanup(tt);
		return result;
	}
}
#ifdef USE_WINAPI
static DWORD WINAPI start_threadwin(void *p)
{
	return (DWORD) start_thread(p);
}
#endif

void FXThreadPrivate::run(FXThread *t)
{
#ifdef USE_POSIX
	FXERRHOS(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL)); 
	FXERRHOS(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL));
#endif
	// Set currentThread to point to t so we will know who we are in the future
	currentThread=t;
	{
#ifdef USE_WINAPI
		t->p->id=(FXuint) GetCurrentThreadId();
#endif
#ifdef USE_POSIX
#ifdef USE_OURTHREADID
		static FXushort idt=0;
		// Can't use a FXMutex as idlistlock as FXMutex depends on FXThread::id()
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
		t->p->id=(FXuint) pthread_self();
#endif
#endif
	}
	FXMtxHold h(t->p);
#ifdef DEBUG
	fxmessage("Thread %d (%s) started\n", FXThread::id(), t->name());
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

void FXThreadPrivate::cleanup(FXThread *t)
{
	bool doAutoDelete=false;
	FXWaitCondition *stoppedwc=0;
	{
		FXThread_DTHold dth;
		FXMtxHold h(t->p);
		FXERRH(t->p, "Possibly a 'delete this' was called during thread cleanup?", FXTHREAD_DELETETHIS, FXERRH_ISFATAL);
		FXERRH(!t->isInCleanup, "Exception occured during thread cleanup", FXTHREAD_CLEANUPEXCEPTION, FXERRH_ISFATAL);
		t->isInCleanup=true;
		h.unlock();
		*t->p->result=t->cleanup();
		h.relock();
		forceCleanup(t);
#ifdef DEBUG
		fxmessage("Thread %d (%s) cleanup exits with code %d\n", FXThread::id(), t->name(), (int) *t->p->result);
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

void FXThreadPrivate::forceCleanup(FXThread *t)
{	// Note: called by primary thread destructor too
	t->isRunning=false;
	t->isFinished=true;
	FXThreadPrivate::CleanupCall *cc;
	for(QPtrListIterator<FXThreadPrivate::CleanupCall> it(t->p->cleanupcalls); (cc=it.current());)
	{
		if(cc->inThread)
		{
			(*cc->code)();
			QPtrListIterator<FXThreadPrivate::CleanupCall> it2(it);
			++it;
			t->p->cleanupcalls.removeByIter(it2);
		}
		else ++it;
	}
}

FXThread::FXThread(const char *name, bool autodelete, FXuval stackSize, FXThread::ThreadScheduler threadloc) : magic(*((FXuint *) "THRD")), p(0), myname(name)
{
	FXERRHM(p=new FXThreadPrivate(autodelete, stackSize, threadloc));
	isRunning=isFinished=isInCleanup=false;
	termdisablecnt=0;
#ifdef FXDISABLE_THREADS
	FXERRG("This build of FOX has threads disabled", FXTHREAD_NOTHREADS, FXERRH_ISDEBUG);
#endif
}

FXThread::~FXThread()
{ FXEXCEPTIONDESTRUCT1 {
	if(isFinished || !isRunning)
	{
		FXMtxHold h(p);
		FXThreadPrivate::CleanupCall *cc;
		for(QPtrListIterator<FXThreadPrivate::CleanupCall> it(p->cleanupcalls); (cc=it.current()); ++it)
		{
			h.unlock();
			(*cc->code)();
			h.relock();
		}
		h.unlock();
		FXDELETE(p);
	}
	else FXERRG(FXTrans::tr("FXThread", "You cannot destruct a running thread"), FXTHREAD_STILLRUNNING, FXERRH_ISDEBUG);
	magic=0;
} FXEXCEPTIONDESTRUCT2; }

FXuval FXThread::stackSize() const
{
	FXMtxHold h(p);
	return p->stackSize;
}

void FXThread::setStackSize(FXuval newsize)
{
	FXMtxHold h(p);
	p->stackSize=newsize;
}

FXThread::ThreadScheduler FXThread::threadLocation() const
{
	FXMtxHold h(p);
	return p->threadLocation;
}

void FXThread::setThreadLocation(FXThread::ThreadScheduler threadloc)
{
	FXMtxHold h(p);
	p->threadLocation=threadloc;
}

bool FXThread::wait(FXuint time)
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
		FXWaitCondition *wc;
		if(!p->stoppedwc) FXERRHM(p->stoppedwc=new FXWaitCondition);
		wc=p->stoppedwc;
		h.unlock();
		if(!wc->wait(time)) return false;
	}
#endif
	return true;
}

void FXThread::start(bool waitTillStarted)
{
	FXMtxHold h(p);
	if(waitTillStarted && !p->startedwc)
	{
		FXERRHM(p->startedwc=new FXWaitCondition(false, false));
	}
#ifdef USE_WINAPI
	{
		DWORD threadId;
		p->plsCancel=false;
		FXERRHWIN(ResetEvent(p->plsCancelWaiter));
#ifdef FX_SMPBUILD
		FXERRHWIN(NULL!=(p->threadh=CreateThread(NULL, p->stackSize, start_threadwin, (void *) this, 0, &threadId)));
#else
		// Keep on processor 0 if not SMP build
		FXERRHWIN(NULL!=(p->threadh=CreateThread(NULL, p->stackSize, start_threadwin, (void *) this, CREATE_SUSPENDED, &threadId)));
		FXERRHWIN(SetThreadAffinityMask(p->threadh, 1));
		FXERRHWIN(ResumeThread(p->threadh));
#endif
	}
#endif
#ifdef USE_POSIX
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
		FXERRH(p->startedwc->wait(10000), FXTrans::tr("FXThread", "Failed to start thread"), FXTHREAD_STARTFAILED, 0);
		FXDELETE(p->startedwc);
	}
}

bool FXThread::isValid() const throw()
{
	return *((FXuint *) "THRD")==magic;
}

bool FXThread::setAutoDelete(bool doso) throw()
{
	FXMtxHold h(p);
	bool ret=p->autodelete;
	p->autodelete=doso;
	return ret;
}

void FXThread::requestTermination()
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

FXuint FXThread::id() throw()
{
#ifdef USE_WINAPI
	return (FXuint) GetCurrentThreadId();
#endif
#ifdef USE_POSIX
#ifdef USE_OURTHREADID
	FXThread *me=FXThread::current();
	if(me && me->p)
		return me->p->id;
	else
	{	// TODO: Breakpoint this every so often and make sure it's not getting used too much
		return 0;
	}
#else
	return (FXuint) pthread_self();
#endif
#endif
	return 0;
}

FXuint FXThread::myId() const
{
	return p->id;
}

FXThread *FXThread::current()
{
	FXThread *c=FXThreadPrivate::currentThread;
#ifdef DEBUG
	//assert(c);
#endif
	return c;
}

FXThread *FXThread::primaryThread() throw()
{
	return FX::primaryThread;
}

FXThread *FXThread::creator() const
{
	return p->creator;
}

signed char FXThread::priority() const
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

void FXThread::setPriority(signed char pri)
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

void FXThread::sleep(FXuint t)
{
#ifdef USE_WINAPI
	msleep(t*1000);
#endif
#ifdef USE_POSIX
	::sleep(t);
#endif
}

void FXThread::msleep(FXuint t)
{
#ifdef USE_WINAPI
	if(WAIT_OBJECT_0==WaitForSingleObject((HANDLE) FXThread::int_cancelWaiterHandle(), t))
	{
		FXThread::current()->checkForTerminate();
	}
#endif
#ifdef USE_POSIX
	::usleep(t*1000);
#endif
}

void FXThread::yield()
{
#ifdef USE_WINAPI
	Sleep(0);
#endif
#ifdef USE_POSIX
	FXERRHOS(sched_yield());
#endif
}

void FXThread::exit(void *r)
{
#ifdef USE_WINAPI
	ExitThread((DWORD) r);
#endif
#ifdef USE_POSIX
	pthread_exit(r);
#endif
}

void *FXThread::result() const throw()
{
	if(isValid() && p)
		return *p->result;
	else
		return 0;
}

void FXThread::disableTermination()
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

bool FXThread::checkForTerminate()
{
#ifdef USE_WINAPI
	FXMtxHold h(p);
	if(!p->plsCancelDisabled && p->plsCancel)
	{
		h.unlock();
		void **resultaddr=p->result;
		cleanup_thread((void *) this);
		ExitThread((DWORD) *resultaddr);
		// This alternative mechanism is much nicer but unfortunately unavailable to POSIX :(
		//FXERRH_THROW(FXThreadIntException());
	}
#endif
#ifdef USE_POSIX
	pthread_testcancel();
#endif
	return p->plsCancel;
}

void FXThread::enableTermination()
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


void *FXThread::int_cancelWaiterHandle()
{
#ifdef USE_WINAPI
	return (void *) FXThread::current()->p->plsCancelWaiter;
#else
	return 0;
#endif
}

void FXThread::addCreationUpcall(FXThread::CreationUpcallSpec upcallv, bool inThread)
{
	FXMtxHold h(creationupcallslock);
	CreationUpcall *cu;
	FXERRHM(cu=new CreationUpcall(upcallv, inThread));
	FXRBOp unnew=FXRBNew(cu);
	creationupcalls.append(cu);
	unnew.dismiss();
}

bool FXThread::removeCreationUpcall(FXThread::CreationUpcallSpec upcallv)
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

Generic::BoundFunctorV *FXThread::addCleanupCall(FXAutoPtr<Generic::BoundFunctorV> handler, bool inThread)
{
	assert(this);	// Static init check
	FXMtxHold h(p);
	FXERRHM(PtrPtr(handler));
	FXPtrHold<FXThreadPrivate::CleanupCall> cc=new FXThreadPrivate::CleanupCall(PtrPtr(handler), inThread);
	p->cleanupcalls.append(cc);
	cc=0;
	return PtrRelease(handler);
}

bool FXThread::removeCleanupCall(Generic::BoundFunctorV *handler)
{
	FXMtxHold h(p);
	FXThreadPrivate::CleanupCall *cc;
	for(QPtrListIterator<FXThreadPrivate::CleanupCall> it(p->cleanupcalls); (cc=it.current()); ++it)
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

void *FXThread::int_disableSignals()
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
}
void FXThread::int_enableSignals(void *oldmask)
{
#ifdef USE_POSIX
	sigset_t *old=(sigset_t *) oldmask;
	FXRBOp unold=FXRBNew(old);
	FXERRHOS(pthread_sigmask(SIG_SETMASK, old, NULL));
#endif
}

/**************************************************************************************************************/
struct FXDLLLOCAL FXThreadPoolPrivate : public FXMutex
{
	FXuint total, maximum;
	FXAtomicInt free;
	bool dynamic;
	struct Thread : public FXMutex, public FXThread
	{
		FXThreadPoolPrivate *parent;
		volatile bool free;
		FXWaitCondition wc;
		Generic::BoundFunctorV *volatile code;
		Thread(FXThreadPoolPrivate *_parent)
			: parent(_parent), free(true), wc(true), code(0), FXThread("Pool thread", true) { }
		~Thread() { parent=0; code=0; }
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
	QPtrDict<FXWaitCondition> waitingwcs;
	QPtrDict<FXuint> timedtimes;

	FXThreadPoolPrivate(bool _dynamic) : total(0), maximum(0), free(0), dynamic(_dynamic), threads(true), timed(true), waiting(true), waitingwcs(7, true), FXMutex() { }
	~FXThreadPoolPrivate()
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

void FXThreadPoolPrivate::Thread::run()
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
					if(++parent->free>(int) parent->total)
					{
						FXMtxHold h(parent);
						if(parent->free>(int) parent->total) 
						{
							--parent->free;
							return;	// Exit thread
						}
					}
					free=true;
					wc.wait();
					lock();
					free=false;
				}

				FXRBOp unlockme=FXRBObj(*this, &FXThreadPoolPrivate::Thread::unlock);
				FXThread_DTHold dth(this);
				assert(code);
				//fxmessage("Thread pool calling %p\n", code);
				Generic::BoundFunctorV *_code=code;
				(*_code)();
				FXDELETE(code);
				unlockme.dismiss();
				unlock();
				//if(!parent->waitingwcs.isEmpty())
				{
					FXMtxHold h(parent);
					FXWaitCondition *codewc=parent->waitingwcs.find(_code);
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

static FXMutex mastertimekeeperlock;
class FXThreadPoolTimeKeeper : public FXThread
{
public:
	struct Entry
	{
		FXuint when;
		FXThreadPool *which;
		FXAutoPtr<Generic::BoundFunctorV> code;
		Entry(FXuint _when, FXThreadPool *creator, FXAutoPtr<Generic::BoundFunctorV> _code) : when(_when), which(creator), code(_code) { }
		bool operator<(const Entry &o) const { return o.when-when<0x80000000; }
		bool operator==(const Entry &o) const { return when==o.when && which==o.which && code==o.code; }
	};
	FXWaitCondition wc;
	QSortedList<Entry> entries;
	FXThreadPoolTimeKeeper() : wc(true), entries(true), FXThread("Thread pool time keeper") { }
	~FXThreadPoolTimeKeeper()
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
					if((now-entry->when)<0x80000000)
					{
						FXPtrHold<Entry> entryh(entry);
						entries.takeFirst();
						entryh->which->dispatch(entryh->code);
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
		while((entry=entries.getFirst()))
		{
			PtrRelease(entry->code);
			entries.removeFirst();
		}
		return 0;
	}
};
static FXPtrHold<FXThreadPoolTimeKeeper> mastertimekeeper;
static void DestroyThreadPoolTimeKeeper()
{
	delete static_cast<FXThreadPoolTimeKeeper *>(mastertimekeeper);
	mastertimekeeper=0;
}

FXThreadPool::FXThreadPool(FXuint total, bool dynamic) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new FXThreadPoolPrivate(dynamic));
	setTotal(total);
	unconstr.dismiss();
}

FXThreadPool::~FXThreadPool()
{ FXEXCEPTIONDESTRUCT1 {
	if(mastertimekeeper)
	{
		FXMtxHold h(mastertimekeeperlock);
		FXThreadPoolTimeKeeper::Entry *entry;
		for(QSortedListIterator<FXThreadPoolTimeKeeper::Entry> it(mastertimekeeper->entries); (entry=it.current());)
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

FXuint FXThreadPool::total() const throw()
{
	return p->total;
}

FXuint FXThreadPool::maximum() const throw()
{
	return p->maximum;
}

FXuint FXThreadPool::free() const throw()
{
	return p->free;
}

void FXThreadPool::startThreads(FXuint newno)
{
	FXThreadPoolPrivate::Thread *t;
	if(newno>p->total)
	{
		for(FXuint n=p->total; n<newno; n++)
		{
			FXERRHM(t=new FXThreadPoolPrivate::Thread(p));
			FXRBOp unnew=FXRBNew(t);
			p->threads.append(t);
			unnew.dismiss();
		}
	}
	p->total=newno;
	for(QPtrListIterator<FXThreadPoolPrivate::Thread> it(p->threads); (t=it.current()); ++it)
	{
		if(!t->running()) t->start();
	}
}

void FXThreadPool::setTotal(FXuint newno)
{
	FXMtxHold h(p);
	p->maximum=newno;
	if(!p->dynamic) startThreads(newno);
}

bool FXThreadPool::dynamic() const throw()
{
	return p->dynamic;
}

void FXThreadPool::setDynamic(bool v)
{
	p->dynamic=v;
}

Generic::BoundFunctorV *FXThreadPool::dispatch(FXAutoPtr<Generic::BoundFunctorV> code, FXuint delay)
{
	Generic::BoundFunctorV *_code=0;
	//fxmessage("Thread pool dispatch %p in %d ms\n", PtrPtr(code), delay);
	assert(code);
	if(delay)
	{
		FXMtxHold h(mastertimekeeperlock);
		if(!mastertimekeeper)
		{
			FXERRHM(mastertimekeeper=new FXThreadPoolTimeKeeper);
			mastertimekeeper->start();
			// Slightly nasty this :)
			primaryThread->addCleanupCall(Generic::BindFuncN(DestroyThreadPoolTimeKeeper));
		}
		FXThreadPoolTimeKeeper::Entry *entry;
		_code=PtrPtr(code);
		FXERRHM(entry=new FXThreadPoolTimeKeeper::Entry(FXProcess::getMsCount()+delay, this, code));
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
			FXThreadPoolPrivate::Thread *t;
			for(QPtrListIterator<FXThreadPoolPrivate::Thread> it(p->threads); (t=it.current()); ++it)
			{
				if(t->free && !t->code)
				{
					t->code=_code=PtrRelease(code);
					t->wc.wakeAll();
					--p->free;
					break;
				}
			}
		}
		else
		{
			p->waiting.append((_code=PtrRelease(code)));
			if(p->dynamic && p->total<p->maximum)
			{
				startThreads(p->total+1);
			}
		}
	}
	return _code;
}

FXThreadPool::CancelledState FXThreadPool::cancel(Generic::BoundFunctorV *code, bool wait)
{
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
			FXThreadPoolTimeKeeper::Entry *entry;
			for(QSortedListIterator<FXThreadPoolTimeKeeper::Entry> it(mastertimekeeper->entries); (entry=it.current()); ++it)
			{
				if(this==entry->which && PtrPtr(entry->code)==code)
				{
					PtrRelease(entry->code);
					mastertimekeeper->entries.removeRef(entry);
					//fxmessage("Thread pool cancel %p found\n", code);
					return Cancelled;
				}
			}
			h2.unlock();	// Unlock time keeper
			{	// Ok, is it currently being executed? If so, wait till it's done
				FXThreadPoolPrivate::Thread *t;
				for(QPtrListIterator<FXThreadPoolPrivate::Thread> it(p->threads); (t=it.current()); ++it)
				{
					if(t->code==code)
					{	// Wait for it to complete
						//fxmessage("Thread pool cancel %p waiting for completion\n", code);
						h.unlock();
						if(wait) { FXMtxHold h3(t); }
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

bool FXThreadPool::reset(Generic::BoundFunctorV *code, FXuint delay)
{
	FXMtxHold h(mastertimekeeperlock);
	if(!mastertimekeeper) return false;
	FXThreadPoolTimeKeeper::Entry *entry;
	QSortedListIterator<FXThreadPoolTimeKeeper::Entry> it=mastertimekeeper->entries;
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

bool FXThreadPool::wait(Generic::BoundFunctorV *code, FXuint period)
{
	FXMtxHold h(p);
	if(-1==p->waiting.findRef(code))
	{
		FXThreadPoolPrivate::Thread *t;
		for(QPtrListIterator<FXThreadPoolPrivate::Thread> it(p->threads); (t=it.current()); ++it)
		{
			if(t->code==code) break;
		}
		if(!t)
		{
			if(mastertimekeeper)
			{
				FXThreadPoolTimeKeeper::Entry *e;
				for(QSortedListIterator<FXThreadPoolTimeKeeper::Entry> it(mastertimekeeper->entries); (e=it.current()); ++it)
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
	FXWaitCondition *wc=p->waitingwcs.find(code);
	if(!wc)
	{
		FXERRHM(wc=new FXWaitCondition);
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

