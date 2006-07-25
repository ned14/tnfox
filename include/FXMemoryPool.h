/********************************************************************************
*                                                                               *
*                              Custom Memory Pool                               *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003-2005 by Niall Douglas.   All Rights Reserved.       *
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

#if !defined(FXDEFS_H) && !defined(DOXYGEN_SHOULD_SKIP_THIS)
#include "fxdefs.h"
#else

#ifndef FXMEMORYPOOL_H
#define FXMEMORYPOOL_H

// included by fxdefs.h/fxmemoryops.h
#include <new>

namespace FX {

/*! \file FXMemoryPool.h
\brief Defines classes used to implement custom memory pools
*/
template<class type> class QMemArray;
class FXMemoryPool;
class QThread;

/*! \ingroup fxmemoryops
Allocates memory */
extern FXAPI FXMALLOCATTR void *malloc(size_t size, FXMemoryPool *heap=0) throw();
/*! \ingroup fxmemoryops
Allocates memory */
extern FXAPI FXMALLOCATTR void *calloc(size_t no, size_t size, FXMemoryPool *heap=0) throw();
/*! \ingroup fxmemoryops
Resizes memory */
extern FXAPI FXMALLOCATTR void *realloc(void *p, size_t size, FXMemoryPool *heap=0) throw();
/*! \ingroup fxmemoryops
Frees memory */
extern FXAPI void free(void *p, FXMemoryPool *heap=0) throw();
#if defined(DEBUG) && defined(_MSC_VER)
extern FXAPI FXMALLOCATTR void *_malloc_dbg(size_t size, int blockuse, const char *file, int lineno) throw();
#endif
/*! \ingroup fxmemoryops
Causes an assertion failure when the specified memory block is freed */
extern FXAPI void failonfree(void *p, FXMemoryPool *heap=0) throw();
/*! \ingroup fxmemoryops
Removes a previous FX::failonfree() */
extern FXAPI void unfailonfree(void *p, FXMemoryPool *heap=0) throw();

/*! \class FXMemoryPool
\ingroup fxmemoryops
\brief A threadsafe custom memory pool

One of the things I've always wanted in my programming library but haven't had
since my RISC-OS days is the ability to create arbitrary heaps (called memory
pools nowadays as some idiot called a variant of an ordered tree programming pattern
a "heap"). RISC-OS made custom heaps very easy as you just called <tt>SWI
OS_HeapCreate</tt> and even today it has the unique property of letting you
create one wherever you like. Sadly we couldn't offer this here.

But why would you want to have lots of custom heaps around the place? Because
put simply, it can increase performance \b and reduce memory usage, never mind
allowing some isolation of memory corruption and making it easy to prevent
memory overuse attacks by malicious parties. Despite the many advantages of
using custom heaps, they are very rarely used despite C++ offering per-class
allocators. This is a shame, but understandable given you must do slightly more
work to make full use of them.

Given there's a lack of documentation on why they're useful, I'll outline it
here. Performance increases can be gained especially when allocating lots of
small simple objects - you can skip the destruction completely for the entire
set by allocating from a custom heap, then simply deleting the heap rather
than destroying each individually. Memory usage can be reduced because heaps
fragment - the C++ freestore is particularly notorious for this and any
algorithm with good performance tends to waste lots of memory in unallocated
portions. Thus if you can reduce the fragmentation, you can substantially
reduce the working set of your application. Finally, memory corruption
can be isolated if you can contain some piece of code to work from a custom
heap - if it dies horribly, you can be pretty safe with some kinds of code
to delete the heap and be sure the corruption won't spread - like cutting
out a cancer. Don't over-rely on this however.

As of v0.80 of TnFOX, FXMemoryPool has been substantially augmented from the
merging of TMemPool into it from Tn. Furthermore there is now process-wide
replacement of all dynamic memory allocation to use this library instead.
The default STDC \c malloc, \c calloc, \c realloc and \c free have been left
available as sometimes a third party library will return a pointer to you
which you must later free and using the TnFOX replacements will cause a segfault.
There is now a facility for a "current pool" set per thread which causes all
global memory allocations to occur from that pool. See FX::FXMemPoolHold.

This particular heap implementation is not my own - previous to v0.86 it used
the famous ptmalloc2 which is the standard allocator in GNU glibc (and thus
is the standard allocator for Linux). As of v0.86 onwards it uses nedmalloc,
an even better memory allocator once again. nedmalloc has a similar design to
ptmalloc2 except that it adds a threadcache for improved scalability so the
following resources remain useful:

ptmalloc2 was shown to have the optimum speed versus wastefulness of all
thread-optimised allocators according to Sun (<a
href="http://developers.sun.com/solaris/articles/multiproc/multiproc.html">
http://developers.sun.com/solaris/articles/multiproc/multiproc.html</a>). You
can read more about how ptmalloc2's algorithm works at <a
href="http://gee.cs.oswego.edu/dl/html/malloc.html">
http://gee.cs.oswego.edu/dl/html/malloc.html</a>. Note that nedmalloc is
compiled in debug mode when TnFOX is compiled as such - thus you may get
assertion errors if you corrupt the nedmalloc based heap.

If you'd like to know more in general about heap algorithms, implementations and
performance, please see <a href="ftp://ftp.cs.utexas.edu/pub/garbage/allocsrv.ps">
ftp://ftp.cs.utexas.edu/pub/garbage/allocsrv.ps</a>

\note In debug modes, by default the system allocator is used exclusively with
extensive additional sanity checks. Please consult \link fxmemoryops
the custom memory infrastructure
\endlink
page for more information.

<h3>Usage:</h3>
You are guaranteed that irrespective of which memory pool you allocate from,
you can free it with any other memory pool in effect. This is a requirement for any
serious usage. When
you create a FXMemoryPool, you can supply which thread you wish to associate it with - when
that thread exits, the memory pool will automatically be deleted. Deletion of a memory
pool can take one of two forms - either immediate, or marked for deletion when the
last block allocated within it is freed.

FXMemoryPool keeps some statistics of how many memory pools are in use and how much
data has been allocated in each. This can be monitored during testing to ensure that
you haven't forgotten to use a memory pool when you should.

TnFOX defines implementations of the ANSI C dynamic memory functions within the FX
namespace but it also defines templated versions for easier type casting plus all
versions take an optional parameter allowing you to manually specify which heap to use.
The global new and delete operator overrides similarly have such an extra (optional)
parameter.
*/
struct FXMemoryPoolPrivate;
class FXAPI FXMemoryPool
{
	friend FXAPI void *malloc(size_t size, FXMemoryPool *heap) throw();
	friend FXAPI void *calloc(size_t no, size_t _size, FXMemoryPool *heap) throw();
	friend FXAPI void *realloc(void *p, size_t size, FXMemoryPool *heap) throw();
	friend FXAPI void free(void *p, FXMemoryPool *heap) throw();
	FXMemoryPoolPrivate *p;
	FXMemoryPool(const FXMemoryPool &);
	FXMemoryPool &operator=(const FXMemoryPool &);
public:
	//! Defines the size of block virtual address space is allocated in
	static const FXuval minHeapVirtualSpace=1024*1024;
	//! A structure containing statistics about the pool
	struct Statistics
	{
		FXuval arena;		//!< Current total non-mmapped bytes allocated from system
		FXuval freeChunks;	//!< Number of free chunks
		FXuval fastChunks;	//!< Number of chunks available for fast reuse
		FXuval mmapRegions;	//!< Number of memory mapped regions used
		FXuval mmapBytes;	//!< Total bytes held in memory mapped regions
		FXuval maxAlloc;	//!< Maximum allocated space
		FXuval totalFast;	//!< Total bytes available for fast reuse
		FXuval totalAlloc;	//!< Total bytes allocated (normal or mmapped)
		FXuval totalFree;	//!< Total bytes not allocated
		FXuval keepCost;	//!< Max bytes which could be returned to system ideally
		Statistics(FXuval a, FXuval b, FXuval c, FXuval d, FXuval e, FXuval f,
			FXuval g, FXuval h, FXuval i, FXuval j) : arena(a), freeChunks(b), fastChunks(c),
			mmapRegions(d), mmapBytes(e), maxAlloc(f), totalFast(g), totalAlloc(h),
			totalFree(i), keepCost(j) { }
	};
	/*! Allocates a pool which can grow to size \em maximum. Virtual address space
	is allocated in blocks of FX::FXMemoryPool::minHeapVirtualSpace so you might as
	well allocate one of those by default. You can also set an identifier (for debugging)
	and associate the pool with a thread whereby when that thread exits, the pool will
	be deleted. Setting \em lazydeleted means the pool doesn't really die after destruction
	until the last block is freed from it. */
	FXMemoryPool(FXuval maximum=(FXuval)-1, const char *identifier=0, QThread *owner=0, bool lazydeleted=false);
	~FXMemoryPool();
	//! Returns the size of the pool
	FXuval size() const throw();
	//! Returns the maximum size of the pool
	FXuval maxsize() const throw();
	//! Allocates a block, returning zero if unable
	FXMALLOCATTR void *malloc(FXuval size) throw();
	//! Allocates a zero initialised block, returning zero if unable
	FXMALLOCATTR void *calloc(FXuint no, FXuval size) throw();
	//! Frees a block
	void free(void *blk) throw();
	/*! Extends a block, returning zero if unable. Note that like the
	ANSI \c realloc() you must still free \em blk on failure.
	*/
	FXMALLOCATTR void *realloc(void *blk, FXuval size) throw();
	//! Returns the memory pool associated with a memory chunk (=0 if from system pool, =-1 if not from any pool)
	static FXMemoryPool *poolFromBlk(void *blk) throw();
public:
	//! Allocates a block from the global heap, returning zero if unable
	static FXMALLOCATTR void *glmalloc(FXuval size) throw();
	/*! Allocates zero-filled \em no of blocks of size \em size from the
	global heap, returning zero if unable */
	static FXMALLOCATTR void *glcalloc(FXuval no, FXuval size) throw();
	//! Frees a block from the global heap
	static void glfree(void *blk) throw();
	/*! Extends a block in the global heap, returning zero if unable.
	Note that like the ANSI \c realloc() you must still free \em blk on failure.
	*/
	static FXMALLOCATTR void *glrealloc(void *blk, FXuval size) throw();
	//! Trims the heap down to the minimum possible, releasing memory back to the system
	static bool gltrim(FXuval left) throw();
	//! Returns a set of usage statistics about the heap
	static Statistics glstats() throw();
	//! Returns a set of usage statistics about the heap as a string
	static FXString glstatsAsString();
public:
	//! Retrieves the memory pool in use by the current thread (=0 for system pool)
	static FXMemoryPool *current();
	//! Sets the memory pool to be used by the current thread (=0 for system pool)
	static void setCurrent(FXMemoryPool *heap);

	//! Data reflecting a memory pool
	struct MemoryPoolInfo
	{
		bool deleted;			//!< Pool is pending deletion
		QThread *owner;		//!< Owning thread (=0 for process-wide)
		const char *identifier;	//!< Pool identifier
		FXuval maximum;			//!< Maximum size of this pool in bytes
		FXuval allocated;		//!< Allocated bytes right now
	};
	//! Returns a list of memory pools existing
	static QMemArray<MemoryPoolInfo> statistics();
};

/*! \class FXMemPoolHold
\brief Changes the memory pool in use by the current thread for the duration of its existance

\sa FX::FXMemoryPool
*/
class FXMemPoolHold
{
	FXMemoryPool *oldheap;
public:
	FXMemPoolHold(FXMemoryPool *newheap) : oldheap(FXMemoryPool::current()) { FXMemoryPool::setCurrent(newheap); }
	~FXMemPoolHold() { FXMemoryPool::setCurrent(oldheap); }
};

} // namespace

/*! \ingroup fxmemoryops
Allocates memory */
extern "C" FXAPI FXMALLOCATTR void *tnfxmalloc(size_t size);
/*! \ingroup fxmemoryops
Allocates memory */
extern "C" FXAPI FXMALLOCATTR void *tnfxcalloc(size_t no, size_t size);
/*! \ingroup fxmemoryops
Resizes memory */
extern "C" FXAPI FXMALLOCATTR void *tnfxrealloc(void *p, size_t size);
/*! \ingroup fxmemoryops
Frees memory */
extern "C" FXAPI void tnfxfree(void *p);

#endif
#endif
