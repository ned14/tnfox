/********************************************************************************
*                                                                               *
*                   Redefines the global memory operators                       *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2004 by Niall Douglas.   All Rights Reserved.            *
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

#ifndef FXMEMORYOPS_H
#define FXMEMORYOPS_H

//#define FXDISABLE_GLOBALALLOCATORREPLACEMENTS


#include "FXMemoryPool.h"
#include <string.h>

/*! \file fxmemoryops.h
\brief Redefines the global memory operators to use a third party
allocation system
*/

#ifdef _MSC_VER
// This is a totally braindead warning :(
#pragma warning(disable: 4290) // C++ exception specification ignored except it's not nothrow
#endif

/*! \defgroup fxmemoryops Custom memory allocation infrastructure

Tn for security reasons requires a substantially more enhanced dynamic memory
allocation system - in particular, threads performing operations by remote instruction
must ensure that resource depletion attacks are prevented. This is done by running
a separate memory pool per remote client which can either be chosen explicitly or
via a TLS variable.

Concurrent with this is the need to replace the system allocator on Win32 with TnFOX's
one which typically yields a 6x speed increase for multithreaded C++ programs. A
unified system is required for all this as there is only one global new and delete
operator. <b>It is rather important that you read and understand the following
discussion</b>.

Due to how C++ (and C) works, there is one global \c new and \c delete operator which supposedly
can be wholly replaced by any program according to the standard. This isn't actually
true - on Win32, you can only replace per binary (DLL or EXE) and on GNU/ELF/POSIX your operator
replacements get happily overrided by any shared objects loaded after your binary.
It gets worse - when working with containers or any code which speculatively allocates
memory (eg; buffering), a container can quickly come to consist of blocks allocated
from a multitude of memory pools all at once leaving us with no choice other than to
permit any thread with any memory pool currently set to be able to free any other
allocation, no matter where it was allocated. Furthermore, we must also tolerate
other parts of the C library or STL allocating with the default allocator and freeing
using ours (GCC is particularly bad for this when optimisation is turned on).

It was for all these reasons that getting the custom memory allocation infrastructure working
took a week or so of testing. I tried a number of schemes and eventually came down
with one which is reasonably fast but does cost an extra four bytes per allocation
within a non-global memory pool. You the user must also observe some caveats:
<ol>
 <li>Do not combine TnFOX with any code which also replaces the global \c new and
\c delete operators where those operators do not use directly \c ::malloc() and \c ::free().
Both MSVC and GCC are fine here.
 <li>Do not mix direct use of a FX::FXMemoryPool via its methods with passing it as
a parameter to any of the global allocation functions. If you do you will get a segfault.
 <li>By using TnFOX, you imply you will not define your own global \c new and \c delete
operators. If you try, you will get a compile error as an inlined version is defined
anywhere a TnFOX header file is included.
</ol>
That inlined version just mentioned is done to ensure all TnFOX code is permanently
bound to the TnFOX allocator. It cannot be overriden subsequently. It furthermore doesn't
bother calling a new handler as set by \c std::set_new_handler().

There are some defines which affect how the custom memory allocation system works:
\li \c \b FXDISABLE_SEPARATE_POOLS when defined to 1 forces exclusive use of the
system allocator by the global allocator replacements (directly using FXMemoryPool
still uses the special heap). Note that this comes with a twelve byte book-keeping
overhead (24 bytes on 64 bit architectures) on every allocation and is intended only
for debugging purposes (especially memory leak checking). If undefined, is 1 in debug
mode and 0 in release mode.
\li \c \b FXDISABLE_GLOBAL_MARKER when defined to 1 does not place a marker in each
non-pool specific allocation (thus saving four bytes or eight bytes on 64 bit
architectures). While this saves memory, it does mean that the allocator replacements
can no longer detect when they are freeing memory allocated by the system allocator
and thus invoke that instead - therefore the onus is on you to ensure that if you
receive a block allocated from say within another library, it is up to you to call
\c ::free() on it rather than \c free(). If undefined, is 1 on both Win32 and POSIX.
*/

namespace FX
{	// These being the "greedy" templated versions which prevent ambiguity with the
	// STDC export "C" versions. Also more convenient to use from C++.
	/*! \ingroup fxmemoryops
	Allocates memory */
	template<typename T> inline FXMALLOCATTR T *malloc(size_t size, FXMemoryPool *heap=0) throw();
	template<typename T> inline T *malloc(size_t size, FXMemoryPool *heap) throw() { return (T *) FX::malloc(size, heap); }
	/*! \ingroup fxmemoryops
	Allocates memory */
	template<typename T> inline FXMALLOCATTR T *calloc(size_t no, size_t size, FXMemoryPool *heap=0) throw();
	template<typename T> inline T *calloc(size_t no, size_t size, FXMemoryPool *heap) throw() { return (T *) FX::calloc(no, size, heap); }
	/*! \ingroup fxmemoryops
	Resizes memory */
	template<typename T> inline FXMALLOCATTR T *realloc(T *p, size_t size, FXMemoryPool *heap=0) throw();
	template<typename T> inline T *realloc(T *p, size_t size, FXMemoryPool *heap) throw() { return (T *) FX::realloc((void *) p, size, heap); }
	/*! \ingroup fxmemoryops
	Frees memory */
	template<typename T> inline void free(T *p, FXMemoryPool *heap=0) throw() { FX::free((void *) p, heap); }

	// This one caught me out for a while
	/*! \ingroup fxmemoryops
	Duplicates a string */
	inline FXMALLOCATTR char *strdup(const char *str) throw();
	inline char *strdup(const char *str) throw()
	{
		int len=strlen(str);
		void *ret=FX::malloc(len+1);
		if(!ret) return NULL;
		memcpy(ret, str, len+1);
		return (char *) ret;
	}
}

#ifndef FXDISABLE_GLOBALALLOCATORREPLACEMENTS
// Okay back in the global namespace. Define replacement new/delete but weakly
// (side effect of inline) so the linker can elide all duplicates and leave one
// in each binary. Ensure it's public visibility on ELF.
/*! \ingroup fxmemoryops
Global operator new replacement */
inline FXDLLPUBLIC FXMALLOCATTR void *operator new(size_t size) throw(std::bad_alloc);
inline FXDLLPUBLIC void *operator new(size_t size) throw(std::bad_alloc)
{
	void *ret;
	if(!(ret=FX::malloc(size))) throw std::bad_alloc();
	return ret;
}
/*! \ingroup fxmemoryops
Global operator new replacement */
inline FXDLLPUBLIC FXMALLOCATTR void *operator new[](size_t size) throw(std::bad_alloc);
inline FXDLLPUBLIC void *operator new[](size_t size) throw(std::bad_alloc)
{
	void *ret;
	if(!(ret=FX::malloc(size))) throw std::bad_alloc();
	return ret;
}
/*! \ingroup fxmemoryops
Global operator delete replacement */
inline FXDLLPUBLIC void operator delete(void *p) throw()
{
	if(p) FX::free(p);
}
/*! \ingroup fxmemoryops
Global operator delete replacement */
inline FXDLLPUBLIC void operator delete[](void *p) throw()
{
	if(p) FX::free(p);
}
#if defined(DEBUG) && defined(_MSC_VER)		// The MSVC CRT debug allocators
inline FXDLLPUBLIC FXMALLOCATTR void *operator new(size_t size, int blockuse, const char *file, int lineno);
inline FXDLLPUBLIC void *operator new(size_t size, int blockuse, const char *file, int lineno)
{
	void *ret;
	if(!(ret=FX::_malloc_dbg(size, blockuse, file, lineno))) throw std::bad_alloc();
	return ret;
}
inline FXDLLPUBLIC FXMALLOCATTR void *operator new[](size_t size, int blockuse, const char *file, int lineno);
inline FXDLLPUBLIC void *operator new[](size_t size, int blockuse, const char *file, int lineno)
{
	void *ret;
	if(!(ret=FX::_malloc_dbg(size, blockuse, file, lineno))) throw std::bad_alloc();
	return ret;
}
#endif
#endif

/*! \ingroup fxmemoryops
operator new with a specific pool */
inline FXDLLPUBLIC FXMALLOCATTR void *operator new(size_t size, FX::FXMemoryPool *heap) throw(std::bad_alloc);
inline FXDLLPUBLIC void *operator new(size_t size, FX::FXMemoryPool *heap) throw(std::bad_alloc)
{
	void *ret;
	if(!(ret=FX::malloc(size, heap))) throw std::bad_alloc();
	return ret;
}
/*! \ingroup fxmemoryops
operator new with a specific pool */
inline FXDLLPUBLIC FXMALLOCATTR void *operator new[](size_t size, FX::FXMemoryPool *heap) throw(std::bad_alloc);
inline FXDLLPUBLIC void *operator new[](size_t size, FX::FXMemoryPool *heap) throw(std::bad_alloc)
{
	void *ret;
	if(!(ret=FX::malloc(size, heap))) throw std::bad_alloc();
	return ret;
}
/*! \ingroup fxmemoryops
operator delete with a specific pool */
inline FXDLLPUBLIC void operator delete(void *p, FX::FXMemoryPool *heap) throw()
{
	if(p) FX::free(p, heap);
}
/*! \ingroup fxmemoryops
operator delete with a specific pool */
inline FXDLLPUBLIC void operator delete[](void *p, FX::FXMemoryPool *heap) throw()
{
	if(p) FX::free(p, heap);
}

#endif
