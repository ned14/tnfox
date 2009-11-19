/********************************************************************************
*                                                                               *
*                              Custom Memory Pool                               *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003-2009 by Niall Douglas.   All Rights Reserved.       *
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
//#define FXENABLE_DEBUG_PRINTING



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
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
extern "C" int PatchInNedmallocDLL(void);
#endif

//#pragma optimize("gaw", off)
//#pragma inline_depth(0)

/* Defining this enables the failonfree() function whereby freeing a specified
block causes an assertion failure. Only really has a point in debug builds */
#ifdef DEBUG
 #define FXFAILONFREESLOTS 32
#else
 #define FXFAILONFREESLOTS 0
#endif

#define NO_NED_NAMESPACE
#include "nedmalloc/nedmalloc.h"
#undef malloc
#undef calloc
#undef free
#undef realloc

/* Set uthash to use nedmalloc */
#ifdef __GNUC__
#define typeof(x) __typeof__(x)
#endif
#include "nedmalloc/uthash/src/uthash.h"
#undef uthash_malloc
#undef uthash_free
#define uthash_malloc(sz) nedpmalloc(0, sz)
#define uthash_free(ptr) nedpfree(0, ptr)
#define HASH_FIND_PTR(head,findptr,out)                                         \
    HASH_FIND(hh,head,findptr,sizeof(void *),out)
#define HASH_ADD_PTR(head,ptrfield,add)                                         \
    HASH_ADD(hh,head,ptrfield,sizeof(void *),add)

namespace FX {

static struct nedmalloc_init_t
{
	nedmalloc_init_t()
	{
#ifdef WIN32
		PatchInNedmallocDLL();

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
	nedpool *heap;
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
			neddestroypool(heap);
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
		void *ret=alignment ? nedpmemalign(heap, alignment, size) : nedpmalloc(heap, size);
		if(ret && (FXuval)-1!=maxsize) allocated+=(int) nedblksize(0, ret);
		return ret;
	}
	void *calloc(FXuint no, FXuval esize, FXuint alignment=0) throw()
	{
		FXuval size=esize*no;
		if((FXuval)-1!=maxsize)
		{
			if(allocated+size>maxsize) return 0;
		}
		void *ret=alignment ? nedpmemalign(heap, alignment, size) : nedpmalloc(heap, size);
		memset(ret, 0, size);
		if(ret && (FXuval)-1!=maxsize) allocated+=(int) nedblksize(0, ret);
		return ret;
	}
	void free(void *blk, FXuint alignment) throw()
	{
		if((FXuval)-1!=maxsize)
			allocated-=(int) nedblksize(0, blk);
		nedpfree(heap, blk);
	}
	void *realloc(void *blk, FXuval size) throw()
	{
		if(!blk) return malloc(size);
		FXuval oldsize=nedblksize(0, blk);
		if((FXuval)-1!=maxsize)
		{
			if(allocated+(size-oldsize)>maxsize) return 0;
		}
		void *ret=nedprealloc(heap, blk, size);
		if(ret && (FXuval)-1!=maxsize) allocated+=(int)(nedblksize(0, ret)-oldsize);
		return ret;
	}
	static FXMemoryPoolPrivate *poolFromBlk(void *blk) throw()
	{
		nedpool *pool=0;
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
			neddisablethreadcache(pool->heap);
		}
	}
	neddisablethreadcache(0 /*system*/);
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
	FXERRHM(p->heap=nedcreatepool(0, 0));
	unconstr.dismiss();
	nedpsetvalue(p->heap, (void *) p);
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
	void *ret=alignment ? nedmemalign(alignment, size) : nedmalloc(size);
	return ret;
}
void *FXMemoryPool::glcalloc(FXuval no, FXuval size, FXuint alignment) throw()
{
	return alignment ? memset(nedmemalign(alignment, size), 0, size) : nedcalloc(no, size);
}
void FXMemoryPool::glfree(void *blk, FXuint alignment) throw()
{
	nedfree(blk);
}
void *FXMemoryPool::glrealloc(void *blk, FXuval size) throw()
{
	return nedrealloc(blk, size);
}
bool FXMemoryPool::gltrim(FXuval left) throw()
{
	return nedmalloc_trim(left)!=0;
}
FXMemoryPool::Statistics FXMemoryPool::glstats() throw()
{
	struct nedmallinfo mi;
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
		// Add global heap
		FXMemoryPool::Statistics stats=FXMemoryPool::glstats();
		ret[n].deleted=false;
		ret[n].owner=0;
		ret[n].identifier="Global heap";
		ret[n].maximum=(FXuval) -1;
		ret[n].allocated=(FXuval) stats.totalAlloc;
		n++;
		FXMemoryPoolPrivate *mp;
		for(QPtrDictIterator<FXMemoryPoolPrivate> it(mempools.pools); (mp=it.current()); ++it, ++n)
		{
			ret[n].deleted=mp->deleted;
			ret[n].owner=mp->owner;
			ret[n].identifier=mp->identifier;
			ret[n].maximum=mp->maxsize;
			ret[n].allocated=mp->size();
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
#ifdef DEBUG
struct AllocatedBlock
{
	enum AllocatedBlockType { FREE_TYPE=0, MALLOC_TYPE=1, CALLOC_TYPE, REALLOC_TYPE } type;
	void *ptr;
	FXMemoryPoolPrivate *mp;
	const char *file, *function;
	int lineno;
	size_t amount;
	UT_hash_handle hh;
};
static AllocatedBlock *allocatedBlocks, *freeBlocks;
static QShrdMemMutex allocatedBlocksLock(FXINFINITE);
static void DebugExpanded(UT_hash_table *tbl) throw()
{
	fxmessage("FXMemoryPool: Expanded AllocatedBlocks to %d buckets\n", tbl->num_buckets);
	if(tbl->num_buckets>=16*1024*1024)
	{	// Something's wrong
		int a=1;
	}
}
#undef uthash_expand_fyi
#define uthash_expand_fyi(tbl) DebugExpanded(tbl)
#undef uthash_noexpand_fyi
#define uthash_noexpand_fyi(tbl) fxmessage("FXMemoryPool: WARNING: AllocatedBlocks bucket expansion inhibited\n")
static void AddAllocatedBlock(AllocatedBlock::AllocatedBlockType type, void *ret, FXMemoryPoolPrivate *mp, const char *file, const char *function, int lineno, size_t size) throw()
{
	QMtxHold h(allocatedBlocksLock);
	AllocatedBlock *ab=0;
	if(freeBlocks)
	{
		ab=freeBlocks;
		freeBlocks=(AllocatedBlock *) freeBlocks->hh.next;
	}
	else
	{
		if(!(ab=(AllocatedBlock *) uthash_malloc(sizeof(AllocatedBlock)))) return;
	}
	ab->type=type;
	ab->ptr=ret;
	ab->mp=mp;
	ab->file=file;
	ab->function=function;
	ab->lineno=lineno;
	ab->amount=size;
	HASH_ADD_PTR(allocatedBlocks, ptr, ab);
}
static void FreeAllocatedBlock(void *ptr) throw()
{
	QMtxHold h(allocatedBlocksLock);
	AllocatedBlock *ab=0;
	HASH_FIND_PTR(allocatedBlocks, &ptr, ab);
	if(ab)
	{
		HASH_DEL(allocatedBlocks, ab);
		ab->type=AllocatedBlock::FREE_TYPE;
		ab->ptr=0;
		ab->mp=0;
		ab->file=0;
		ab->function=0;
		ab->lineno=0;
		ab->amount=0;
		ab->hh.next=freeBlocks;
		freeBlocks=ab;
	}
}

void *malloc(size_t size, FXMemoryPool *heap, FXuint alignment) throw() { return malloc_dbg(0, 0, 0, size, heap, alignment); }
void *malloc_dbg(const char *file, const char *function, int lineno, size_t size, FXMemoryPool *heap, FXuint alignment) throw()
#else
void *malloc(size_t size, FXMemoryPool *heap, FXuint alignment) throw()
#endif
{
	void *ret, *trueret;
	FXMemoryPoolPrivate *mp=!heap ? (!mempools.enabled ? (FXMemoryPoolPrivate *) 0 : (FXMemoryPoolPrivate *) mempools.current) : heap->p;
#ifdef FXENABLE_DEBUG_PRINTING
	fxmessage("FX::malloc(%u, %p, %u)", size, mp, alignment);
#endif
	if(!size) size=1;	// BSD allocator doesn't like zero allocations
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
		if(!(trueret=ret=alignment ? nedmemalign(size, alignment) : nedmalloc(size))) return 0;
	}
#ifdef FXENABLE_DEBUG_PRINTING
	fxmessage("=%p (%p)\n", ret, trueret);
#endif
#ifdef DEBUG
	AddAllocatedBlock(AllocatedBlock::MALLOC_TYPE, ret, mp, file, function, lineno, size);
#endif
	return ret;
}
#ifdef DEBUG
void *calloc(size_t no, size_t size, FXMemoryPool *heap, FXuint alignment) throw() { return calloc_dbg(0, 0, 0, no, size, heap, alignment); }
void *calloc_dbg(const char *file, const char *function, int lineno, size_t no, size_t _size, FXMemoryPool *heap, FXuint alignment) throw()
#else
void *calloc(size_t no, size_t _size, FXMemoryPool *heap, FXuint alignment) throw()
#endif
{
	FXuval size=no*_size;
	void *ret, *trueret;
	FXMemoryPoolPrivate *mp=!heap ? (!mempools.enabled ? (FXMemoryPoolPrivate *) 0 : (FXMemoryPoolPrivate *) mempools.current) : heap->p;
#ifdef FXENABLE_DEBUG_PRINTING
	fxmessage("FX::calloc(%u, %p, %u)", size, mp, alignment);
#endif
	if(!size) size=1;	// BSD allocator doesn't like zero allocations
	if(mp)
	{
		size+=alignment+sizeof(FXuval);
		if(!(trueret=ret=mp->calloc(1, size))) return 0;
		FXuval *_ret=(FXuval *) ret;
		ret=FXOFFSETPTR(ret, sizeof(FXuval));
		if(alignment) ret=(void *)(((FXuval) ret+alignment-1)&~(alignment-1));
		for(; _ret<ret; _ret++) *_ret=*(FXuval *) "FXMPFXMP";
	}
	else
	{
		if(!(trueret=ret=alignment ? nedmemalign(size, alignment) : nedcalloc(1, size))) return 0;
		if(alignment) memset(ret, 0, size);
	}
#ifdef FXENABLE_DEBUG_PRINTING
	fxmessage("=%p (%p)\n", ret, trueret);
#endif
#ifdef DEBUG
	AddAllocatedBlock(AllocatedBlock::CALLOC_TYPE, ret, mp, file, function, lineno, size);
#endif
	return ret;
}
#ifdef DEBUG
void *realloc(void *p, size_t size, FXMemoryPool *heap) throw() { return realloc_dbg(0, 0, 0, p, size, heap); }
void *realloc_dbg(const char *file, const char *function, int lineno, void *p, size_t size, FXMemoryPool *heap) throw()
#else
void *realloc(void *p, size_t size, FXMemoryPool *heap) throw()
#endif
{
	if(!p) return malloc(size, heap);
	void *ret=0, *trueret;
	FXMemoryPoolPrivate *realmp=0, *mp=!heap ? (!mempools.enabled ? (FXMemoryPoolPrivate *) 0 : (FXMemoryPoolPrivate *) mempools.current) : heap->p;
	FXuval *_p=(FXuval *) p;
#ifdef FXENABLE_DEBUG_PRINTING
	fxmessage("FX::realloc(%p, %u, %p)", p, size, mp);
#endif
#ifdef DEBUG
	FreeAllocatedBlock(p);
#endif
	if(!size) size=1;	// BSD allocator doesn't like zero allocations
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
		if(!(trueret=ret=nedrealloc(_p, size))) return 0;
	}
#ifdef FXENABLE_DEBUG_PRINTING
	fxmessage("=%p (%p)\n", ret, trueret);
#endif
#ifdef DEBUG
	AddAllocatedBlock(AllocatedBlock::REALLOC_TYPE, ret, mp, file, function, lineno, size);
#endif
	return ret;
}
void free(void *p, FXMemoryPool *heap) throw()
{
	if(!p) return;
	FXMemoryPoolPrivate *realmp=0;
	FXuval *_p=(FXuval *) p;
#ifdef FXENABLE_DEBUG_PRINTING
	fxmessage("FX::free(%p, %p)=", p, heap);
#endif
#if FXFAILONFREESLOTS>0
	for(int n=0; n<FXFAILONFREESLOTS; n++)
	{
		assert(!failOnFrees[n].blk || p!=failOnFrees[n].blk);
	}
#endif
#ifdef DEBUG
	FreeAllocatedBlock(p);
#endif
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
#ifdef FXENABLE_DEBUG_PRINTING
		fxmessage("=%p\n", _p);
#endif
		nedfree(_p);
	}
	if(realmp && realmp->deleted && realmp->size()==0)
	{
#ifdef DEBUG
		fxmessage("Deleting FXMemoryPoolPrivate %p (%p) as it is empty and pending deletion (%u remaining)\n", realmp, realmp->heap, mempools.pools.count());
#endif
		FXDELETE(realmp);
	}
}

#if defined(DEBUG)
/* As of TnFOX v0.89 we no longer use the MSVC debug allocator. Instead we track our
own allocations and frees */
static int AllocatedBlockCompare(const AllocatedBlock *a, const AllocatedBlock *b) throw()
{
	FXival diff=(FXival)a->ptr-(FXival)b->ptr;
	if(diff<0) return -1;
	else if(diff>0) return 1;
	else return 0;
}
bool printLeakedBlocks() throw()
{
	QMtxHold h(allocatedBlocksLock);
	AllocatedBlock *ab=0;
	while((ab=freeBlocks))
	{
		freeBlocks=(AllocatedBlock *) freeBlocks->hh.next;
		uthash_free(ab);
	}
	if(allocatedBlocks)
	{
		FXuval blocks=0, amount=0;
		HASH_SORT(allocatedBlocks, AllocatedBlockCompare);
		for(ab=allocatedBlocks; ab; ab=(AllocatedBlock *) ab->hh.next)
		{
			static const char *blocktypes[4]={ "free()", "malloc()", "calloc()", "realloc()" };
			if(ab->file)
				fxmessage("*** LEAKED %s block %p length %u in pool %p allocated by %s:%s:%d\n", blocktypes[ab->type], ab->ptr, ab->amount, ab->mp, ab->file, ab->function, ab->lineno);
			else
				fxmessage("*** LEAKED %s block %p length %u in pool %p allocated by UNKNOWN\n", blocktypes[ab->type], ab->ptr, ab->amount, ab->mp);
			blocks++;
			amount+=ab->amount;
		}
		fxmessage("***\n***TOTAL LEAKAGE: %u blocks representing %u bytes\n", blocks, amount);
		return true;
	}
	return false;
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
