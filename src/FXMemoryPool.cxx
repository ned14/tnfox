/********************************************************************************
*                                                                               *
*                              Custom Memory Pool                               *
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

//#define DEBUG
//#define FXDISABLE_GLOBAL_MARKER 0
//#define FXDISABLE_SEPARATE_POOLS 1
//#define FXENABLE_DEBUG_PRINTING



/* TODO: Remove header detection in favour of improved nedpgetvalue()
*/

#ifdef USE_POSIX
// Force a "real" copy of global operators new & delete to satisfy GNU
// linker that we've really redefined them. It would seem that when they're
// inline the linker outputs them as weak which is no good for other modules
#define inline
#endif
#include "fxdefs.h"
#ifdef USE_POSIX
#undef inline
#endif

#include "FXProcess.h"
#include "QThread.h"
#include "FXRollback.h"
#include "FXPtrHold.h"
#include <stdlib.h>
#include <assert.h>
#include <qptrdict.h>
#include <qmemarray.h>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif

//#pragma optimize("gaw", off)
//#pragma inline_depth(0)

/* Define to force exclusive use of the system allocator by the global allocator
replacements (directly using FXMemoryPool still uses the special heap). If you
interchange direct use of FXMemoryPool with parameterised use of the global
allocator functions, you will get a segfault. Note that this comes with a twelve
byte book-keeping overhead on every allocation and is intended only for debugging
purposes (especially memory leak checking) */
#ifndef FXDISABLE_SEPARATE_POOLS
// Define to force use of system allocator
 #ifdef DEBUG
  #define FXDISABLE_SEPARATE_POOLS 1
 #else
  #define FXDISABLE_SEPARATE_POOLS 0
 #endif
#endif

/* Customise global allocator replacements according to system.
On all systems except Linux and Mac OS X, replace system allocator with our own
superior implementation (which is the same as Linux's) */
#if !defined(__linux__) && !defined(__APPLE__)
#define MYMALLOC(size, alignment) (alignment ? nedalloc::nedmemalign(alignment, size) : nedalloc::nedmalloc(size))
 #define MYREALLOC(ptr, size)      nedalloc::nedrealloc(ptr, size)
 #define MYFREE(ptr, alignment)    nedalloc::nedfree(ptr)
 #ifndef _MSC_VER
  // Python bindings need this on POSIX
  #define FXDISABLE_GLOBAL_MARKER 0
 #endif
#else
 #ifdef _MSC_VER
  #define MYMALLOC(size, alignment) (alignment ? ::_aligned_malloc(size, alignment) : ::malloc(size))
  #define MYREALLOC(ptr, size)      ::realloc(ptr, size)
  #define MYFREE(ptr, alignment)    (alignment ? ::_aligned_free(ptr) : ::free(ptr))
 #else
  #define MYMALLOC(size, alignment) (alignment ? ::memalign(alignment, size) : ::malloc(size))
  #define MYREALLOC(ptr, size)      ::realloc(ptr, size)
  #define MYFREE(ptr, alignment)    ::free(ptr)
 #endif
#endif

/* Defining this saves the four or eight byte overhead used to mark a block
as having been allocated by us on every allocation in the global heap. While
this saves memory, it does mean that the allocator replacements can no longer
detect when they are freeing memory allocated by the system allocator and thus
invoke that instead.

By default, this is enabled on both Win32 and POSIX but for different reasons:-
on Win32, the global allocator is totally replaced and this works fine due to
Win32's much better DLL encapsulation. On POSIX, the global allocator is not
replaced and thus the many mismatched allocator function calls don't matter.

Note that allocations from a custom pool \b always contain the overhead (this
is how free() knows when not to pass through to ::free()) */
#ifndef FXDISABLE_GLOBAL_MARKER
 #define FXDISABLE_GLOBAL_MARKER 1
#endif

/* Defining this enables the failonfree() function whereby freeing a specified
block causes an assertion failure. Only really has a point in debug builds */
#ifdef DEBUG
 #define FXFAILONFREESLOTS 32
#else
 #define FXFAILONFREESLOTS 0
#endif

#define ABORT_ON_ASSERT_FAILURE 0
#ifdef NDEBUG
// Some speed increase options
//#define INSECURE 1
#endif
#include "nedmalloc/nedmalloc.c"
#undef malloc
#undef free
#undef realloc


namespace FX {

static struct nedmalloc_init_t
{
	nedmalloc_init_t()
	{
#ifndef FXDISABLE_NO_SEPARATE_POOL_WARNING
#if defined(_MSC_VER) && (FXDISABLE_GLOBALALLOCATORREPLACEMENTS || FXDISABLE_SEPARATE_POOLS)
		fxmessage("WARNING: Using Win32 memory allocator, performance will be degraded!\n");
#endif
#endif
#ifdef WIN32
		ULONG data=2;
		if(!HeapSetInformation(GetProcessHeap(), HeapCompatibilityInformation, &data, sizeof(data)))
		{
			fxmessage("WARNING: Failed to set process heap to thread caching variant (probably you are in a debugger)!\n");
		}
#endif
	}
} nedmalloc_init;

struct FXMemoryPoolPrivate;
static struct MemPoolsList
{
	volatile bool enabled;
	QMutex lock;
	QPtrDict<FXMemoryPoolPrivate> pools;
	QThreadLocalStorage<FXMemoryPoolPrivate> current;
	MemPoolsList() : enabled(true), pools(1) { }
	~MemPoolsList() { enabled=false; }
} mempools;

struct FXDLLLOCAL FXMemoryPoolPrivate
{
	FXMemoryPool *parent;		// =0 when orphaned
	nedalloc::nedpool *heap;
	FXuval maxsize;
	bool deleted, lazydeleted;
	const char *identifier;
	QThread *owner;
	FXulong threadId;
	Generic::BoundFunctorV *cleanupcall;
	FXAtomicInt allocated;
	FXMemoryPoolPrivate(FXMemoryPool *_parent, FXuval _maxsize, const char *_identifier, QThread *_owner, bool _lazydeleted)
		: parent(_parent), heap(0), maxsize(_maxsize), deleted(false), lazydeleted(_lazydeleted), identifier(_identifier), owner(_owner), threadId(_owner ? _owner->myId() : 0), cleanupcall(0)
	{	// NOTE TO SELF: Must be safe to be called during static init/deinit!!!
		if(mempools.enabled)
		{
			QMtxHold h(mempools.lock);
			mempools.pools.insert(this, this);
			QDICTDYNRESIZE(mempools.pools);
		}
	}
	~FXMemoryPoolPrivate()
	{	// NOTE TO SELF: Must be safe to be called during static init/deinit!!!
		if(mempools.enabled && mempools.current==this) mempools.current=0;
		if(cleanupcall)
		{
			owner->removeCleanupCall(cleanupcall);
			cleanupcall=0;
		}
		if(heap)
		{
			nedalloc::neddestroypool(heap);
			heap=0;
		}
		if(mempools.enabled)
		{
			QMtxHold h(mempools.lock);
			mempools.pools.remove(this);
			QDICTDYNRESIZE(mempools.pools);
		}
	}
	FXuval size() const throw()
	{
		return allocated;
	}
	void *malloc(FXuval size, FXuint alignment=0) throw()
	{
		if((FXuval)-1!=maxsize)
		{
			if(allocated+size>maxsize) return 0;
		}
		void *ret=alignment ? nedalloc::nedpmemalign(heap, alignment, size) : nedalloc::nedpmalloc(heap, size);
		if(ret && (FXuval)-1!=maxsize) allocated+=(int) nedalloc::nedblksize(ret);
		return ret;
	}
	void *calloc(FXuint no, FXuval esize, FXuint alignment=0) throw()
	{
		FXuval size=esize*no;
		if((FXuval)-1!=maxsize)
		{
			if(allocated+size>maxsize) return 0;
		}
		void *ret=alignment ? nedalloc::nedpmemalign(heap, alignment, size) : nedalloc::nedpmalloc(heap, size);
		memset(ret, 0, size);
		if(ret && (FXuval)-1!=maxsize) allocated+=(int) nedalloc::nedblksize(ret);
		return ret;
	}
	void free(void *blk, FXuint alignment) throw()
	{
		if((FXuval)-1!=maxsize)
			allocated-=(int) nedalloc::nedblksize(blk);
		nedalloc::nedpfree(heap, blk);
	}
	void *realloc(void *blk, FXuval size) throw()
	{
		if(!blk) return malloc(size);
		FXuval oldsize=nedalloc::nedblksize(blk);
		if((FXuval)-1!=maxsize)
		{
			if(allocated+(size-oldsize)>maxsize) return 0;
		}
		void *ret=nedalloc::nedprealloc(heap, blk, size);
		if(ret && (FXuval)-1!=maxsize) allocated+=(int)(nedalloc::nedblksize(ret)-oldsize);
		return ret;
	}
	static FXMemoryPoolPrivate *poolFromBlk(void *blk) throw()
	{
		nedalloc::nedpool *pool=0;
		FXMemoryPoolPrivate *p=(FXMemoryPoolPrivate *) nedgetvalue(&pool, blk);
		if(p) return p;
		if(pool) return 0;					// system pool
		return (FXMemoryPoolPrivate *)-1;	// unknown
	}
};

static void DisableThreadCache()
{
	if(mempools.enabled)
	{
		FXMemoryPoolPrivate *pool;
		QMtxHold h(mempools.lock);
		for(QPtrDictIterator<FXMemoryPoolPrivate> it(mempools.pools); (pool=it.current()); ++it)
		{
			nedalloc::neddisablethreadcache(pool->heap);
		}
	}
	nedalloc::neddisablethreadcache(0 /*system*/);
}
struct RegisterThreadCacheCleanup
{
	RegisterThreadCacheCleanup()
	{	// Register thread deallocation call per thread
		QThread::addCreationUpcall(QThread::CreationUpcallSpec(RegisterSystemCleanupCall));
	}
	static void RegisterSystemCleanupCall(QThread *t)
	{
		t->addCleanupCall(Generic::BindFuncN(DisableThreadCache), true);
	}
};
static FXProcess_StaticInit<RegisterThreadCacheCleanup> registerthreadcachecleaup("ThreadCacheCleanup");
static void callfree(FXMemoryPoolPrivate *p)
{
	p->cleanupcall=0;
	FXDELETE(p->parent);
}
FXMemoryPool::FXMemoryPool(FXuval maxsize, const char *identifier, QThread *owner, bool lazydeleted) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new FXMemoryPoolPrivate(this, maxsize, identifier, owner, lazydeleted));
	FXERRHM(p->heap=nedalloc::nedcreatepool(0, 0));
	unconstr.dismiss();
	nedalloc::nedpsetvalue(p->heap, (void *) p);
	if(owner)
	{
		if(mempools.enabled && QThread::current()==owner)
			mempools.current=p;
		p->cleanupcall=owner->addCleanupCall(Generic::BindFuncN(&callfree, p));
	}
}
FXMemoryPool::~FXMemoryPool()
{ FXEXCEPTIONDESTRUCT1 {
	if(mempools.enabled && mempools.current==p) mempools.current=0;
	if(p->lazydeleted && p->size())
	{
		p->parent=0;
		p->deleted=true;
		p=0;
	}
	else
	{
		FXDELETE(p);
	}
} FXEXCEPTIONDESTRUCT2; }
FXuval FXMemoryPool::size() const throw()
{
	return p->size();
}
FXuval FXMemoryPool::maxsize() const throw()
{
	return p->maxsize;
}
void *FXMemoryPool::malloc(FXuval size, FXuint alignment) throw()
{
	return p->malloc(size, alignment);
}
void *FXMemoryPool::calloc(FXuint no, FXuval esize, FXuint alignment) throw()
{
	return p->calloc(no, esize, alignment);
}
void FXMemoryPool::free(void *blk, FXuint alignment) throw()
{
	return p->free(blk, alignment);
}
void *FXMemoryPool::realloc(void *blk, FXuval size) throw()
{
	return p->realloc(blk, size);
}
FXMemoryPool *FXMemoryPool::poolFromBlk(void *blk) throw()
{
	FXMemoryPoolPrivate *p=FXMemoryPoolPrivate::poolFromBlk(blk);
	if(!p) return 0;
	if((FXMemoryPoolPrivate *)-1!=p) return p->parent;
	return (FXMemoryPool *)-1;
}



void *FXMemoryPool::glmalloc(FXuval size, FXuint alignment) throw()
{
	void *ret=alignment ? nedalloc::nedmemalign(alignment, size) : nedalloc::nedmalloc(size);
	return ret;
}
void *FXMemoryPool::glcalloc(FXuval no, FXuval size, FXuint alignment) throw()
{
	return alignment ? memset(nedalloc::nedmemalign(alignment, size), 0, size) : nedalloc::nedcalloc(no, size);
}
void FXMemoryPool::glfree(void *blk, FXuint alignment) throw()
{
	nedalloc::nedfree(blk);
}
void *FXMemoryPool::glrealloc(void *blk, FXuval size) throw()
{
	return nedalloc::nedrealloc(blk, size);
}
bool FXMemoryPool::gltrim(FXuval left) throw()
{
	return nedalloc::nedmalloc_trim(left)!=0;
}
FXMemoryPool::Statistics FXMemoryPool::glstats() throw()
{	// Workaround GCC's refusal to find struct mallinfo
	using namespace nedalloc;
	struct mallinfo mi;
	mi=nedmallinfo();
	return Statistics(mi.arena, mi.ordblks, mi.smblks, mi.hblks, mi.hblkhd,
		mi.usmblks, mi.fsmblks, mi.uordblks, mi.fordblks, mi.keepcost);
}
FXString FXMemoryPool::glstatsAsString()
{
	Statistics s=glstats();
	return FXString().format("Heap is %lu bytes big with %lu used (%lu free), "
					"%lu in core, %lu in %lu mmapped regions. There are "
					"%lu bytes used by %lu fast reuse blocks and %lu "
					"could be returned to system",
		s.maxAlloc, s.totalAlloc, s.totalFree,
		s.arena, s.mmapBytes, s.mmapRegions,
		s.totalFast, s.fastChunks, s.keepCost);
}


FXMemoryPool *FXMemoryPool::current()
{
	return mempools.enabled ? mempools.current->parent : 0;
}
void FXMemoryPool::setCurrent(FXMemoryPool *heap)
{
	if(mempools.enabled) mempools.current=heap->p;
}

QMemArray<FXMemoryPool::MemoryPoolInfo> FXMemoryPool::statistics()
{
	if(mempools.enabled)
	{
		QMtxHold h(mempools.lock);
		FXuint len=mempools.pools.count();
		QMemArray<MemoryPoolInfo> ret(len+1);
		FXuint n=0;
#ifndef FXDISABLE_SEPARATE_POOLS
		// Add global heap
		FXMemoryPool::Statistics stats=FXMemoryPool::glstats();
		ret[n].deleted=false;
		ret[n].owner=0;
		ret[n].identifier="Global heap";
		ret[n].maximum=(FXuval) -1;
		ret[n].allocated=(FXuval) stats.totalAlloc;
		n++;
#else
		ret.resize(len);
#endif
		FXMemoryPoolPrivate *mp;
		for(QPtrDictIterator<FXMemoryPoolPrivate> it(mempools.pools); (mp=it.current()); ++it, ++n)
		{
			ret[n].deleted=mp->deleted;
			ret[n].owner=mp->owner;
			ret[n].identifier=mp->identifier;
			ret[n].maximum=mp->maxsize;
#ifndef FXDISABLE_SEPARATE_POOLS
			ret[n].allocated=mp->size();
#else
			ret[n].allocated=mp->size()+(FXuval) mp->allocated;
#endif
		}
		return ret;
	}
	return QMemArray<FXMemoryPool::MemoryPoolInfo>();
}




// **** The memory allocator redirectors ****
#if FXFAILONFREESLOTS>0
static struct FailOnFreeEntry
{
	void *blk;
	FXMemoryPool *heap;
	FailOnFreeEntry() : blk(0), heap(0) { }
} failOnFrees[FXFAILONFREESLOTS];
static QMutex failOnFreesLock;
#endif

void *malloc(size_t size, FXMemoryPool *heap, FXuint alignment) throw()
{
	void *ret, *trueret;
	FXMemoryPoolPrivate *mp=!heap ? (!mempools.enabled ? (FXMemoryPoolPrivate *) 0 : (FXMemoryPoolPrivate *) mempools.current) : heap->p;
#ifdef FXENABLE_DEBUG_PRINTING
	fxmessage("FX::malloc(%u, %p, %u)", size, mp, alignment);
#endif
	if(!size) size=1;	// BSD allocator doesn't like zero allocations
#if !FXDISABLE_SEPARATE_POOLS
	if(mp)
	{
		size+=alignment+sizeof(FXuval);
		if(!(trueret=ret=mp->malloc(size))) return 0;
		FXuval *_ret=(FXuval *) ret;
		ret=FXOFFSETPTR(ret, sizeof(FXuval));
		if(alignment) ret=(void *)(((FXuval) ret+alignment-1)&~(alignment-1));
		for(; _ret<ret; _ret++) *_ret=*(FXuval *) "FXMPFXMP";
	}
	else
	{
#if !FXDISABLE_GLOBAL_MARKER
		size+=alignment+sizeof(FXuval);
		if(!(trueret=ret=MYMALLOC(size, 0))) return 0;
		FXuval *_ret=(FXuval *) ret;
		ret=FXOFFSETPTR(ret, sizeof(FXuval));
		if(alignment) ret=(void *)(((FXuval) ret+alignment-1)&~(alignment-1));
		for(; _ret<ret; _ret++) *_ret=*(FXuval *) "FYMPFYMP";
#else
		if(!(trueret=ret=MYMALLOC(size, alignment))) return 0;
#endif
	}
#else //!FXDISABLE_SEPARATE_POOLS
	size+=alignment+sizeof(FXuval)*3;
	if(mp && mp->allocated+size>mp->maxsize) return 0;
	if(!(trueret=ret=::malloc(size))) return 0;
	FXuval *_ret=(FXuval *) ret;
	ret=FXOFFSETPTR(ret, 3*sizeof(FXuval));
	if(alignment) ret=(void *)(((FXuval) ret+alignment-1)&~(alignment-1));
	for(; _ret<(FXuval *)ret-2; _ret++) *_ret=*(FXuval *) "FXMPFXMP";
	_ret[0]=(FXuval) mp;
	_ret[1]=(FXuval) size;
	if(mp) mp->allocated+=(int) size;
#endif
#ifdef FXENABLE_DEBUG_PRINTING
	fxmessage("=%p (%p)\n", ret, trueret);
#endif
	return ret;
}
void *calloc(size_t no, size_t _size, FXMemoryPool *heap, FXuint alignment) throw()
{
	FXuval size=no*_size;
	void *ret, *trueret;
	FXMemoryPoolPrivate *mp=!heap ? (!mempools.enabled ? (FXMemoryPoolPrivate *) 0 : (FXMemoryPoolPrivate *) mempools.current) : heap->p;
#ifdef FXENABLE_DEBUG_PRINTING
	fxmessage("FX::calloc(%u, %p, %u)", size, mp, alignment);
#endif
	if(!size) size=1;	// BSD allocator doesn't like zero allocations
#if !FXDISABLE_SEPARATE_POOLS
	if(mp)
	{
		size+=alignment+sizeof(FXuval);
		if(!(trueret=ret=mp->malloc(size))) return 0;
		memset(ret, 0, size);
		FXuval *_ret=(FXuval *) ret;
		ret=FXOFFSETPTR(ret, sizeof(FXuval));
		if(alignment) ret=(void *)(((FXuval) ret+alignment-1)&~(alignment-1));
		for(; _ret<ret; _ret++) *_ret=*(FXuval *) "FXMPFXMP";
	}
	else
	{
#if !FXDISABLE_GLOBAL_MARKER
		size+=alignment+sizeof(FXuval);
		if(!(trueret=ret=MYMALLOC(size, 0))) return 0;
		memset(ret, 0, size);
		FXuval *_ret=(FXuval *) ret;
		ret=FXOFFSETPTR(ret, sizeof(FXuval));
		if(alignment) ret=(void *)(((FXuval) ret+alignment-1)&~(alignment-1));
		for(; _ret<ret; _ret++) *_ret=*(FXuval *) "FYMPFYMP";
#else
		if(!(trueret=ret=MYMALLOC(size, alignment))) return 0;
		memset(ret, 0, size);
#endif
	}
#else
	size+=alignment+sizeof(FXuval)*3;
	if(mp && mp->allocated+size>mp->maxsize) return 0;
	trueret=ret=::malloc(size);
	memset(ret, 0, size);
	FXuval *_ret=(FXuval *) ret;
	ret=FXOFFSETPTR(ret, 3*sizeof(FXuval));
	if(alignment) ret=(void *)(((FXuval) ret+alignment-1)&~(alignment-1));
	for(; _ret<(FXuval *)ret-2; _ret++) *_ret=*(FXuval *) "FXMPFXMP";
	_ret[0]=(FXuval) mp;
	_ret[1]=(FXuval) size;
	if(mp) mp->allocated+=(int) size;
#endif
#ifdef FXENABLE_DEBUG_PRINTING
	fxmessage("=%p (%p)\n", ret, trueret);
#endif
	return ret;
}
void *realloc(void *p, size_t size, FXMemoryPool *heap) throw()
{
	if(!p) return malloc(size, heap);
	void *ret=0, *trueret;
	FXMemoryPoolPrivate *realmp=0, *mp=!heap ? (!mempools.enabled ? (FXMemoryPoolPrivate *) 0 : (FXMemoryPoolPrivate *) mempools.current) : heap->p;
	FXuval *_p=(FXuval *) p;
#ifdef FXENABLE_DEBUG_PRINTING
	fxmessage("FX::realloc(%p, %u, %p)", p, size, mp);
#endif
	if(!size) size=1;	// BSD allocator doesn't like zero allocations
#if !FXDISABLE_SEPARATE_POOLS
	if(_p[-1]==*(FXuval *) "FXMPFXMP")
	{
		for(_p-=1;*_p==*(FXuval *) "FXMPFXMP"; *_p--=*(FXuval *) "RLOCRLOC"); _p++;
		realmp=FXMemoryPoolPrivate::poolFromBlk(_p);
		assert((FXMemoryPoolPrivate *)-1!=realmp);
	}
	if(realmp!=mp)
	{	// Reparent
		if(!(trueret=ret=malloc(size, heap))) return 0;
		memcpy(ret, p, size);
		free(p);
		return ret;
	}
	p=(void *) _p;
	if(realmp)
	{	// i.e. this is the path when the realloc is within a specified pool (mp==realmp)
		size+=sizeof(FXuval);
		if(!(trueret=ret=realmp->realloc(p, size))) return 0;
		*(FXuval *)ret=*(FXuval *) "FXMPFXMP";
		ret=FXOFFSETPTR(ret, sizeof(FXuval));
	}
	else
	{	// this is the path when the realloc is within the system pool
#if !FXDISABLE_GLOBAL_MARKER
		size+=sizeof(FXuval);
		if(_p[-1]!=*(FXuval *) "FYMPFYMP")
		{
#ifdef DEBUG
			fxmessage("*** FX::realloc(%p) of block not allocated by FX::malloc()\n", p);
#endif
			if(!(trueret=ret=MYMALLOC(size, 0))) return 0;
			memcpy(ret, p, size);
			::free(p);
		}
		else
#endif
		{
#if !FXDISABLE_GLOBAL_MARKER
			for(_p-=1;*_p==*(FXuval *) "FYMPFYMP"; *_p--=*(FXuval *) "RLOCRLOC"); _p++;
#endif
			if(!(trueret=ret=MYREALLOC(_p, size))) return 0;
		}
#if !FXDISABLE_GLOBAL_MARKER
		ret=FXOFFSETPTR(ret, sizeof(FXuval));
#endif
	}
#else
	_p-=3;
	if(_p[0]!=*(FXuval *) "FXMPFXMP")
	{	// Transfer
		trueret=ret=malloc(size, heap);
		memcpy(ret, p, size);
		::free(p);
		goto end;
	}
	size+=3*sizeof(FXuval);
	realmp=(FXMemoryPoolPrivate *) _p[1];
	FXuval oldsize=_p[2];
	if(mp && mp->allocated-(mp==realmp ? oldsize : 0)+size>mp->maxsize) goto end;
	for(; *_p==*(FXuval *) "FXMPFXMP"; *_p--=*(FXuval *) "RLOCRLOC");
	p=(void *)(++_p);
	if(!(trueret=ret=::realloc(p, size)))
		goto end;
	FXuval *_ret=(FXuval *) ret;
	ret=FXOFFSETPTR(ret, 3*sizeof(FXuval));
	_ret[0]=*(FXuval *) "FXMPFXMP";
	_ret[1]=(FXuval) mp;
	_ret[2]=(FXuval) size;
	if(realmp) realmp->allocated-=(int) oldsize;
	if(mp) mp->allocated+=(int) size;
#endif
end:
#ifdef FXENABLE_DEBUG_PRINTING
	fxmessage("=%p (%p)\n", ret, trueret);
#endif
	return ret;
}
void free(void *p, FXMemoryPool *heap, FXuint alignment) throw()
{
	if(!p) return;
	FXMemoryPoolPrivate *realmp=0;
	FXuval *_p=(FXuval *) p;
#ifdef FXENABLE_DEBUG_PRINTING
	fxmessage("FX::free(%p, %p, %u)=", p, heap, alignment);
#endif
#if FXFAILONFREESLOTS>0
	for(int n=0; n<FXFAILONFREESLOTS; n++)
	{
		assert(!failOnFrees[n].blk || p!=failOnFrees[n].blk);
	}
#endif
#if !FXDISABLE_SEPARATE_POOLS
	if(_p[-1]==*(FXuval *) "FXMPFXMP")
	{
		for(_p-=1;*_p==*(FXuval *) "FXMPFXMP"; *_p--=*(FXuval *) "FREEFREE"); _p++;
		realmp=FXMemoryPoolPrivate::poolFromBlk(_p);
		assert((FXMemoryPoolPrivate *)-1!=realmp);
	}
	p=(void *) _p;
	if(realmp)
	{
#ifdef FXENABLE_DEBUG_PRINTING
		fxmessage("=%p\n", p);
#endif
		realmp->free(p, 0);
	}
	else
	{
#if !FXDISABLE_GLOBAL_MARKER
		if(_p[-1]!=*(FXuval *) "FYMPFYMP")
		{
#ifdef DEBUG
			fxmessage("*** FX::free(%p) of block not allocated by FX::malloc()\n", p);
#endif
#ifdef FXENABLE_DEBUG_PRINTING
			fxmessage("=%p\n", p);
#endif
			::free(p);
		}
		else
#endif
		{
#if !FXDISABLE_GLOBAL_MARKER
			for(_p-=1;*_p==*(FXuval *) "FYMPFYMP"; *_p--=*(FXuval *) "FREEFREE"); _p++;
			alignment=0;
#endif
#ifdef FXENABLE_DEBUG_PRINTING
			fxmessage("=%p\n", _p);
#endif
			MYFREE(_p, alignment);
		}
	}
	if(realmp && realmp->deleted && realmp->size()==0)
	{
#ifdef DEBUG
		fxmessage("Deleting FXMemoryPoolPrivate %p (%p) as it is empty and pending deletion (%u remaining)\n", realmp, realmp->heap, mempools.pools.count());
#endif
		FXDELETE(realmp);
	}
#else
	_p-=3;
	if(_p[0]!=*(FXuval *) "FXMPFXMP")
	{
#ifdef DEBUG
		fxmessage("*** FX::free(%p) of block not allocated by FX::malloc()\n", p);
#endif
#ifdef FXENABLE_DEBUG_PRINTING
		fxmessage("=%p\n", p);
#endif
		::free(p);
		return;
	}
	realmp=(FXMemoryPoolPrivate *) _p[1];
	if(realmp) realmp->allocated-=(int) _p[2];
	for(; *_p==*(FXuval *) "FXMPFXMP"; *_p--=*(FXuval *) "FREEFREE");
	p=(void *)(++_p);
#ifdef FXENABLE_DEBUG_PRINTING
	fxmessage("=%p\n", p);
#endif
	::free(p);
	if(realmp && realmp->deleted && realmp->allocated==0)
	{
#ifdef DEBUG
		fxmessage("Deleting FXMemoryPoolPrivate %p as it is empty and pending deletion (%u remaining)\n", realmp, mempools.pools.count());
#endif
		FXDELETE(realmp);
	}
#endif
}

#if defined(DEBUG) && defined(_MSC_VER)
void *_malloc_dbg(size_t size, int blockuse, const char *file, int lineno) throw()
{
	void *ret, *trueret;
	FXMemoryPoolPrivate *mp=!mempools.enabled ? (FXMemoryPoolPrivate *) 0 : (FXMemoryPoolPrivate *) mempools.current;
#ifdef FXENABLE_DEBUG_PRINTING
	fxmessage("FX::malloc_dbg(%u, %p, %s, %d)", size, mp, file, lineno);
#endif
#if !FXDISABLE_SEPARATE_POOLS
	if(mp)
	{
		size+=sizeof(FXuval);
		if(!(trueret=ret=mp->malloc(size))) return 0;
		FXuval *_ret=(FXuval *) ret;
		ret=FXOFFSETPTR(ret, sizeof(FXuval));
		_ret[0]=*(FXuval *) "FXMPFXMP";
	}
	else
	{
#if !FXDISABLE_GLOBAL_MARKER
		size+=sizeof(FXuval);
#endif
		if(!(trueret=ret=MYMALLOC(size, 0))) return 0;
#if !FXDISABLE_GLOBAL_MARKER
		FXuval *_ret=(FXuval *) ret;
		ret=FXOFFSETPTR(ret, sizeof(FXuval));
		_ret[0]=*(FXuval *) "FYMPFYMP";
#endif
	}
#else
	size+=sizeof(FXuval)*3;
	if(mp && mp->allocated+size>mp->maxsize) return 0;
	if(!(trueret=ret=::_malloc_dbg(size, blockuse, file, lineno))) return 0;
	FXuval *_ret=(FXuval *) ret;
	ret=FXOFFSETPTR(ret, 3*sizeof(FXuval));
	_ret[0]=*(FXuval *) "FXMPFXMP";
	_ret[1]=(FXuval) mp;
	_ret[2]=(FXuval) size;
	if(mp) mp->allocated+=(int) size;
#endif
#ifdef FXENABLE_DEBUG_PRINTING
	fxmessage("=%p (%p)\n", ret, trueret);
#endif
	return ret;
}
#endif

void failonfree(void *p, FXMemoryPool *heap) throw()
{
#if FXFAILONFREESLOTS>0
	QMtxHold h(failOnFreesLock);
	for(int n=0; n<FXFAILONFREESLOTS; n++)
	{
		if(!failOnFrees[n].blk)
		{
			failOnFrees[n].blk=p;
			failOnFrees[n].heap=heap;
			break;
		}
	}
#endif
}
void unfailonfree(void *p, FXMemoryPool *heap) throw()
{
#if FXFAILONFREESLOTS>0
	QMtxHold h(failOnFreesLock);
	for(int n=0; n<FXFAILONFREESLOTS; n++)
	{
		if(failOnFrees[n].blk==p)
		{
			failOnFrees[n].blk=0;
			failOnFrees[n].heap=0;
			break;
		}
	}
#endif
}

} // namespace

/* Declare some C API functions reflecting our allocator so C code
bound into the library can find them */
extern "C" void *tnfxmalloc(size_t size) { return FX::malloc(size); }
/*! \ingroup fxmemoryops
Allocates memory */
extern "C" void *tnfxcalloc(size_t no, size_t size) { return FX::calloc(no, size); }
/*! \ingroup fxmemoryops
Resizes memory */
extern "C" void *tnfxrealloc(void *p, size_t size) { return FX::realloc(p, size); }
/*! \ingroup fxmemoryops
Frees memory */
extern "C" void tnfxfree(void *p) { return FX::free(p); }
