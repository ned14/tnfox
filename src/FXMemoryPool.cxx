/********************************************************************************
*                                                                               *
*                              Custom Memory Pool                               *
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

//#define DEBUG
//#define FXDISABLE_GLOBAL_MARKER 0
//#define FXDISABLE_SEPARATE_POOLS 1

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
#include "FXThread.h"
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
On all systems except Linux, replace system allocator with our own
superior implementation (which is the same as Linux's) */
#ifndef __linux__
 #define MYMALLOC(size)       ptmalloc2::dlmalloc(size)
 #define MYREALLOC(ptr, size) ptmalloc2::dlrealloc(ptr, size)
 #define MYFREE(ptr)          ptmalloc2::dlfree(ptr)
 #ifndef _MSC_VER
  // Python bindings need this on POSIX
  #define FXDISABLE_GLOBAL_MARKER 0
 #endif
#else
 #define MYMALLOC(size)       ::malloc(size)
 #define MYREALLOC(ptr, size) ::realloc(ptr, size)
 #define MYFREE(ptr)          ::free(ptr)
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

namespace FX {

#define USE_DL_PREFIX				// Prevent symbol collision
#ifndef WIN32
#define USE_PTHREADS
#undef _LIBC						// Prevent glibc's ptmalloc2 customisations
#endif
#define PTMALLOC_IN_CPPNAMESPACE	// Prevent symbol collision with glibc's ptmalloc2
#define MORECORE_IS_MMAP			// Always use mmap to allocate (high memory downwards)
#ifdef DEBUG
#define MALLOC_DEBUG 1
#endif
//#define TRACE
namespace ptmalloc2
{
#include "ptmalloc2/win32.c"
#include "ptmalloc2/malloc.c"
}
}
#undef malloc
#undef free
#undef realloc


namespace FX {

static struct ptmalloc2_init_t
{
	ptmalloc2_init_t()
	{	// This must be done in a single thread
		ptmalloc2::ptmalloc_init();
#ifndef FXDISABLE_NO_SEPARATE_POOL_WARNING
#if defined(_MSC_VER) && (FXDISABLE_GLOBALALLOCATORREPLACEMENTS || FXDISABLE_SEPARATE_POOLS)
		fxmessage("WARNING: Using Win32 memory allocator, performance will be degraded!\n");
#endif
#endif
	}
} ptmalloc2_init;

static void delHeap(ptmalloc2::mstate h)
{
	void *heap=(void *) (((FXuval) h) & ~0xff);
	FXERRH(!ptmalloc2::delete_heap(heap), "Unknown error deleting ptmalloc2 heap", 0, FXERRH_ISDEBUG);
}
struct FXMemoryPoolPrivate;
static struct MemPoolsList
{
	volatile bool enabled;
	FXMutex lock;
	QPtrDict<FXMemoryPoolPrivate> pools;
	FXThreadLocalStorage<FXMemoryPoolPrivate> current;
	MemPoolsList() : enabled(true), pools(1) { }
	~MemPoolsList() { enabled=false; }
} mempools;

struct FXDLLLOCAL FXMemoryPoolPrivate
{
	FXMemoryPool *parent;		// =0 when orphaned
	ptmalloc2::mstate heap;
	FXuval maxsize;
	bool deleted, lazydeleted;
	const char *identifier;
	FXThread *owner;
	FXulong threadId;
	Generic::BoundFunctorV *cleanupcall;
#ifdef FXDISABLE_SEPARATE_POOLS
	FXAtomicInt allocated;
#endif
	FXMemoryPoolPrivate(FXMemoryPool *_parent, FXuval _maxsize, const char *_identifier, FXThread *_owner, bool _lazydeleted)
		: parent(_parent), heap(0), maxsize(_maxsize), deleted(false), lazydeleted(_lazydeleted), identifier(_identifier), owner(_owner), threadId(_owner ? _owner->myId() : 0), cleanupcall(0)
	{	// NOTE TO SELF: Must be safe to be called during static init/deinit!!!
		if(mempools.enabled)
		{
			FXMtxHold h(mempools.lock);
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
			delHeap(heap);
			heap=0;
		}
		if(mempools.enabled)
		{
			FXMtxHold h(mempools.lock);
			mempools.pools.remove(this);
			QDICTDYNRESIZE(mempools.pools);
		}
	}
	FXuval size() const throw()
	{
		return heap->system_mem;
	}
	void *malloc(FXuval size) throw()
	{
		if((FXuval)-1!=maxsize && heap->system_mem+size>maxsize) return 0;
		void *ret;
		ptmalloc2::mutex_lock(&heap->mutex);
		ret=ptmalloc2::_int_malloc(heap, size);
		ptmalloc2::mutex_unlock(&heap->mutex);
		return ret;
	}
	void *calloc(FXuint no, FXuval esize) throw()
	{
		FXuval size=esize*no;
		if((FXuval)-1!=maxsize && heap->system_mem+size>maxsize) return 0;
		ptmalloc2::mutex_lock(&heap->mutex);
		void *ret=ptmalloc2::_int_malloc(heap, size);
		ptmalloc2::mutex_unlock(&heap->mutex);
		if(!ret) return 0;
		memset(ret, 0, size);
		return ret;
	}
	void free(void *blk) throw()
	{
		using namespace ptmalloc2;
		mchunkptr cp=mem2chunk(blk);
		if(chunk_is_mmapped(cp))                       /* release mmapped memory. */
		{
			munmap_chunk(cp);
			return;
		}
		assert(inuse(cp));
		mstate ar_ptr=arena_for_chunk(cp);
		assert(ar_ptr==heap);
		ptmalloc2::mutex_lock(&ar_ptr->mutex);
		_int_free(ar_ptr, blk);
		ptmalloc2::mutex_unlock(&ar_ptr->mutex);
	}
	void *realloc(void *blk, FXuval size) throw()
	{
		using namespace ptmalloc2;
		if(!blk) return malloc(size);
		mchunkptr oldp=mem2chunk(blk);
		void *newmem;
		FXuval oldsize=chunksize(oldp);
		if((FXuval)-1!=maxsize && heap->system_mem-oldsize+size>maxsize) return 0;
		if(chunk_is_mmapped(oldp))
		{	// Don't have mremap on Win32 (could be implemented easily enough though)
			if(!(newmem=malloc(size))) return 0;
			memcpy(newmem, blk, FXMIN(oldsize, size));
			munmap_chunk(oldp);
			return newmem;
		}
		mstate ar_ptr=arena_for_chunk(oldp);
		assert(ar_ptr==heap);
		void *ret;
		ptmalloc2::mutex_lock(&ar_ptr->mutex);
		ret=_int_realloc(ar_ptr, blk, size);
		ptmalloc2::mutex_unlock(&ar_ptr->mutex);
		return ret;
	}
	static FXMemoryPoolPrivate *poolFromBlk(void *blk) throw()
	{
		using namespace ptmalloc2;
		mchunkptr cp=mem2chunk(blk);
		if(!chunk_non_main_arena(cp)) return 0;
		mstate ar_ptr=arena_for_chunk(cp);
		return static_cast<FXMemoryPoolPrivate *>(ar_ptr->data);
	}
};

static void callfree(FXMemoryPoolPrivate *p)
{
	p->cleanupcall=0;
	FXDELETE(p->parent);
}
FXMemoryPool::FXMemoryPool(FXuval maxsize, const char *identifier, FXThread *owner, bool lazydeleted) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new FXMemoryPoolPrivate(this, maxsize, identifier, owner, lazydeleted));
	FXERRHM(p->heap=ptmalloc2::_int_new_arena(0));
	unconstr.dismiss();
	p->heap->data=p;
	if(owner)
	{
		if(mempools.enabled && FXThread::current()==owner)
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
void *FXMemoryPool::malloc(FXuval size) throw()
{
	return p->malloc(size);
}
void *FXMemoryPool::calloc(FXuint no, FXuval esize) throw()
{
	return p->calloc(no, esize);
}
void FXMemoryPool::free(void *blk) throw()
{
	return p->free(blk);
}
void *FXMemoryPool::realloc(void *blk, FXuval size) throw()
{
	return p->realloc(blk, size);
}
FXMemoryPool *FXMemoryPool::poolFromBlk(void *blk) throw()
{
	return FXMemoryPoolPrivate::poolFromBlk(blk)->parent;
}



void *FXMemoryPool::glmalloc(FXuval size) throw()
{
	void *ret=ptmalloc2::dlmalloc(size);
	return ret;
}
void *FXMemoryPool::glcalloc(FXuval no, FXuval size) throw()
{
	return ptmalloc2::dlcalloc(no, size);
}
void FXMemoryPool::glfree(void *blk) throw()
{
	ptmalloc2::dlfree(blk);
}
void *FXMemoryPool::glrealloc(void *blk, FXuval size) throw()
{
	return ptmalloc2::dlrealloc(blk, size);
}
bool FXMemoryPool::gltrim(FXuval left) throw()
{
	return ptmalloc2::dlmalloc_trim(left)!=0;
}
FXMemoryPool::Statistics FXMemoryPool::glstats() throw()
{	// Workaround GCC's refusal to find struct mallinfo
	using namespace ptmalloc2;
	struct mallinfo mi;
	mi=dlmallinfo();
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
		FXMtxHold h(mempools.lock);
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
static FXMutex failOnFreesLock;
#endif

void *malloc(size_t size, FXMemoryPool *heap) throw()
{
	void *ret;
	FXMemoryPoolPrivate *mp=!heap ? (!mempools.enabled ? (FXMemoryPoolPrivate *) 0 : (FXMemoryPoolPrivate *) mempools.current) : heap->p;
	//fxmessage("FX::malloc(%u, %p)", size, mp);
	if(!size) size=1;	// BSD allocator doesn't like zero allocations
#if !FXDISABLE_SEPARATE_POOLS
	if(mp)
	{
		size+=sizeof(FXuval);
		if(!(ret=mp->malloc(size))) return 0;
		FXuval *_ret=(FXuval *) ret;
		ret=FXOFFSETPTR(ret, sizeof(FXuval));
		_ret[0]=*(FXuval *) "FXMPFXMP";
	}
	else
	{
#if !FXDISABLE_GLOBAL_MARKER
		size+=sizeof(FXuval);
#endif
		if(!(ret=MYMALLOC(size))) return 0;
#if !FXDISABLE_GLOBAL_MARKER
		FXuval *_ret=(FXuval *) ret;
		ret=FXOFFSETPTR(ret, sizeof(FXuval));
		_ret[0]=*(FXuval *) "FYMPFYMP";
#endif
	}
#else
	size+=sizeof(FXuval)*3;
	if(mp && mp->allocated+size>mp->maxsize) return 0;
	if(!(ret=::malloc(size))) return 0;
	FXuval *_ret=(FXuval *) ret;
	ret=FXOFFSETPTR(ret, 3*sizeof(FXuval));
	_ret[0]=*(FXuval *) "FXMPFXMP";
	_ret[1]=(FXuval) mp;
	_ret[2]=(FXuval) size;
	if(mp) mp->allocated+=size;
#endif
	//fxmessage("=%p\n", ret);
	return ret;
}
void *calloc(size_t no, size_t _size, FXMemoryPool *heap) throw()
{
	FXuval size=no*_size;
	void *ret;
	FXMemoryPoolPrivate *mp=!heap ? (!mempools.enabled ? (FXMemoryPoolPrivate *) 0 : (FXMemoryPoolPrivate *) mempools.current) : heap->p;
	//fxmessage("FX::calloc(%u, %p)", size, mp);
	if(!size) size=1;	// BSD allocator doesn't like zero allocations
#if !FXDISABLE_SEPARATE_POOLS
	if(mp)
	{
		size+=sizeof(FXuval);
		if(!(ret=mp->malloc(size))) return 0;
		memset(ret, 0, size);
		FXuval *_ret=(FXuval *) ret;
		ret=FXOFFSETPTR(ret, sizeof(FXuval));
		_ret[0]=*(FXuval *) "FXMPFXMP";
	}
	else
	{
#if !FXDISABLE_GLOBAL_MARKER
		size+=sizeof(FXuval);
#endif
		if(!(ret=MYMALLOC(size))) return 0;
		memset(ret, 0, size);
#if !FXDISABLE_GLOBAL_MARKER
		FXuval *_ret=(FXuval *) ret;
		ret=FXOFFSETPTR(ret, sizeof(FXuval));
		_ret[0]=*(FXuval *) "FYMPFYMP";
#endif
	}
#else
	size+=sizeof(FXuval)*3;
	if(mp && mp->allocated+size>mp->maxsize) return 0;
	ret=::malloc(size);
	memset(ret, 0, size);
	FXuval *_ret=(FXuval *) ret;
	ret=FXOFFSETPTR(ret, 3*sizeof(FXuval));
	_ret[0]=*(FXuval *) "FXMPFXMP";
	_ret[1]=(FXuval) mp;
	_ret[2]=(FXuval) size;
	if(mp) mp->allocated+=size;
#endif
	//fxmessage("=%p\n", ret);
	return ret;
}
void *realloc(void *p, size_t size, FXMemoryPool *heap) throw()
{
	if(!p) return malloc(size, heap);
	void *ret;
	FXMemoryPoolPrivate *realmp=0, *mp=!heap ? (!mempools.enabled ? (FXMemoryPoolPrivate *) 0 : (FXMemoryPoolPrivate *) mempools.current) : heap->p;
	FXuval *_p=(FXuval *) p;
	//fxmessage("FX::realloc(%p, %u, %p)", p, size, mp);
	if(!size) size=1;	// BSD allocator doesn't like zero allocations
#if !FXDISABLE_SEPARATE_POOLS
	if(_p[-1]==*(FXuval *) "FXMPFXMP")
	{
		_p-=1;
		using namespace ptmalloc2;
		mchunkptr cp=mem2chunk(_p);
		if(!chunk_is_mmapped(cp))
		{
			mstate ar_ptr=arena_for_chunk(cp);
			realmp=(FXMemoryPoolPrivate *) ar_ptr->data;
		}
	}
	if(realmp!=mp)
	{	// Reparent
		if(!(ret=malloc(size, heap))) return 0;
		memcpy(ret, p, size);
		free(p);
		return ret;
	}
	p=(void *) _p;
	if(realmp)
	{
		size+=sizeof(FXuval);
		if(!(ret=realmp->realloc(p, size))) return 0;
		ret=FXOFFSETPTR(ret, sizeof(FXuval));
	}
	else
	{
#if !FXDISABLE_GLOBAL_MARKER
		size+=sizeof(FXuval);
		if(_p[-1]!=*(FXuval *) "FYMPFYMP")
		{
#ifdef DEBUG
			fxmessage("*** FX::realloc(%p) of block not allocated by FX::malloc()\n", p);
#endif
			if(!(ret=MYMALLOC(size))) return 0;
			memcpy(ret, p, size);
			::free(p);
		}
		else
#endif
		{
#if !FXDISABLE_GLOBAL_MARKER
			_p-=1;
#endif
			if(!(ret=MYREALLOC(_p, size))) return 0;
		}
#if !FXDISABLE_GLOBAL_MARKER
		ret=FXOFFSETPTR(ret, sizeof(FXuval));
#endif
	}
#else
	_p-=3;
	if(_p[0]!=*(FXuval *) "FXMPFXMP")
	{	// Transfer
		ret=malloc(size, heap);
		memcpy(ret, p, size);
		::free(p);
		return ret;
	}
	else p=(void *) _p;
	size+=3*sizeof(FXuval);
	realmp=(FXMemoryPoolPrivate *) _p[1];
	FXuval oldsize=_p[2];
	if(mp && mp->allocated-(mp==realmp ? oldsize : 0)+size>mp->maxsize) return 0;
	if(!(ret=::realloc(p, size)))
		return 0;
	FXuval *_ret=(FXuval *) ret;
	ret=FXOFFSETPTR(ret, 3*sizeof(FXuval));
	//_ret[0]=*(FXuval *) "FXMPFXMP";
	_ret[1]=(FXuval) mp;
	_ret[2]=(FXuval) size;
	if(realmp) realmp->allocated-=oldsize;
	if(mp) mp->allocated+=size;
#endif
	//fxmessage("=%p\n", ret);
	return ret;
}
void free(void *p, FXMemoryPool *) throw()
{
	if(!p) return;
	FXMemoryPoolPrivate *realmp=0;
	FXuval *_p=(FXuval *) p;
	//fxmessage("FX::free(%p)\n", p);
#if FXFAILONFREESLOTS>0
	for(int n=0; n<FXFAILONFREESLOTS; n++)
	{
		assert(!failOnFrees[n].blk || p!=failOnFrees[n].blk);
	}
#endif
#if !FXDISABLE_SEPARATE_POOLS
	if(_p[-1]==*(FXuval *) "FXMPFXMP")
	{
		_p-=1;
		using namespace ptmalloc2;
		mchunkptr cp=mem2chunk(_p);
		if(!chunk_is_mmapped(cp))
		{
			mstate ar_ptr=arena_for_chunk(cp);
			realmp=(FXMemoryPoolPrivate *) ar_ptr->data;
		}
	}
	p=(void *) _p;
	if(realmp)
		realmp->free(p);
	else
	{
#if !FXDISABLE_GLOBAL_MARKER
		if(_p[-1]!=*(FXuval *) "FYMPFYMP")
		{
#ifdef DEBUG
			fxmessage("*** FX::free(%p) of block not allocated by FX::malloc()\n", p);
#endif
			::free(p);
		}
		else
#endif
		{
#if !FXDISABLE_GLOBAL_MARKER
			_p-=1;
#endif
			MYFREE(_p);
		}
	}
	if(realmp && realmp->deleted && realmp->size()==0)
	{
#ifdef DEBUG
		fxmessage("Deleting FXMemoryPoolPrivate %p (%p) as it is empty and pending deletion (%u remaining)\n", realmp, realmp->heap, mempools.count());
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
		::free(p);
		return;
	}
	else p=(void *) _p;
	realmp=(FXMemoryPoolPrivate *) _p[1];
	if(realmp) realmp->allocated-=_p[2];
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
	void *ret;
	FXMemoryPoolPrivate *mp=!mempools.enabled ? (FXMemoryPoolPrivate *) 0 : (FXMemoryPoolPrivate *) mempools.current;
#if !FXDISABLE_SEPARATE_POOLS
	if(mp)
	{
		size+=sizeof(FXuval);
		if(!(ret=mp->malloc(size))) return 0;
		FXuval *_ret=(FXuval *) ret;
		ret=FXOFFSETPTR(ret, sizeof(FXuval));
		_ret[0]=*(FXuval *) "FXMPFXMP";
	}
	else
	{
#if !FXDISABLE_GLOBAL_MARKER
		size+=sizeof(FXuval);
#endif
		if(!(ret=MYMALLOC(size))) return 0;
#if !FXDISABLE_GLOBAL_MARKER
		FXuval *_ret=(FXuval *) ret;
		ret=FXOFFSETPTR(ret, sizeof(FXuval));
		_ret[0]=*(FXuval *) "FXMPFXMP";
#endif
	}
#else
	size+=sizeof(FXuval)*3;
	if(mp && mp->allocated+size>mp->maxsize) return 0;
	ret=::_malloc_dbg(size, blockuse, file, lineno);
	FXuval *_ret=(FXuval *) ret;
	ret=FXOFFSETPTR(ret, 3*sizeof(FXuval));
	_ret[0]=*(FXuval *) "FXMPFXMP";
	_ret[1]=(FXuval) mp;
	_ret[2]=(FXuval) size;
	if(mp) mp->allocated+=size;
#endif
	return ret;
}
#endif

void failonfree(void *p, FXMemoryPool *heap) throw()
{
#if FXFAILONFREESLOTS>0
	FXMtxHold h(failOnFreesLock);
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
	FXMtxHold h(failOnFreesLock);
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
