/********************************************************************************
*                                                                               *
*                        Assembler optimised operations                         *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2005 by Niall Douglas.   All Rights Reserved.            *
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

#ifndef FXASSEMBLEROPS_H
#define FXASSEMBLEROPS_H

/*! \file fxassemblerops.h
\brief Defines useful inline functions for assembler-optimised common operations
*/

/*! \defgroup fxassemblerops Useful inline functions for assembler-optimised common operations

There are little operations which one needs from time to time which can be
relatively expensive or impossible to do in C. For these situations, TnFOX provides
a series of small inline functions capable of providing them written in assembler
on x86 (i686 or later only) and x64 but with fallback generic implementations in C.

Using these functions can \i seriously improve the speed of your code.
*/

#if defined(_MSC_VER) && ((defined(_M_IX86) && _M_IX86>=600) || defined(_M_AMD64))
// Get the intrinsic definitions
#include "xmmintrin.h"
#include <stdlib.h>		// For byteswap
//        unsigned short __cdecl _byteswap_ushort(unsigned short);
//        unsigned long  __cdecl _byteswap_ulong (unsigned long);
//        unsigned __int64 __cdecl _byteswap_uint64(unsigned __int64);
#pragma intrinsic(_byteswap_ushort)
#pragma intrinsic(_byteswap_ulong)
#pragma intrinsic(_byteswap_uint64)

namespace FX {
/* One has a choice of increments: 32 for P6, 64 for Athlon and 128 for P4, so we choose 64 */
inline void fxprefetchmemT(const void *ptr) throw()
{
	_mm_prefetch((const char *) ptr, _MM_HINT_T2);
}
inline void fxprefetchmemNT(const void *ptr) throw()
{
	_mm_prefetch((const char *) ptr, _MM_HINT_NTA);
}
inline FXuint fxbitscan(FXuint x) throw()
{
	FXuint m;
#if defined(BitScanForward)
	DWORD _m;
	BitScanForward(&_m, x);
	m=(unsigned int) _m;
#elif defined(_M_IX86)
	__asm
	{
		bsf eax, [x]
		mov [m], eax
	}
#endif
	return m;
}
inline FXuint fxbitscanrev(FXuint x) throw()
{
	FXuint m;
#if defined(BitScanReverse)
	DWORD _m;
	BitScanReverse(&_m, x);
	m=(unsigned int) _m;
#elif defined(_M_IX86)
	__asm
	{
		bsr eax, [x]
		mov [m], eax
	}
#endif
	return m;
}
inline void fxendianswap2(void *_p)
{	// Can't improve on this
	FXuchar *p=(FXuchar *) _p, t;
	t=p[0]; p[0]=p[1]; p[1]=t;
}
inline void fxendianswap4(void *_p)
{	// Even with misalignment penalties, this is faster
	FXuint *p=(FXuint *) _p;
	*p=_byteswap_ulong(*p);			// Invokes bswap x86 instruction
}
inline void fxendianswap8(void *_p)
{	// Even with misalignment penalties, this is definitely faster
	FXulong *p=(FXulong *) _p;
	*p=_byteswap_uint64(*p);		// Invokes bswap x86 instruction
}
#elif defined(__GNUC__) && (defined(__i686__) || defined(__x86_64__))
namespace FX {

inline void fxprefetchmemT(const void *ptr) throw()
{
	__builtin_prefetch(ptr, 0, 3);
}
inline void fxprefetchmemNT(const void *ptr) throw()
{
	__builtin_prefetch(ptr, 0, 0);
}
inline FXuint fxbitscan(FXuint x) throw()
{
	FXuint m;
	__asm__("bsfl %1,%0\n\t"
			: "=r" (m) 
			: "g"  (x));
	return m;
}
inline FXuint fxbitscanrev(FXuint x) throw()
{
	FXuint m;
	__asm__("bsrl %1,%0\n\t"
			: "=r" (m) 
			: "g"  (x));
	return m;
}
inline void fxendianswap2(void *_p)
{	// Can't improve on this
	FXuchar *p=(FXuchar *) _p, t;
	t=p[0]; p[0]=p[1]; p[1]=t;
}
inline void fxendianswap4(void *_p)
{	// Even with misalignment penalties, this is faster
	__asm__("mov %0, %%edx\n\t"
			"bswap %%edx\n\t"
			"mov %%edx, %0\n\t"
			: "=m" (*(FXuint *)_p));
/*	__asm
	{
		mov edx, [_p]
		mov eax, [edx]
		bswap eax
		mov [edx], eax
	}*/
}
inline void fxendianswap8(void *_p)
{	// Even with misalignment penalties, this is definitely faster
#if defined(__x86_64__)
	__asm__("mov %0, %%rdx\n\t"
			"bswap %%rdx\n\t"
			"mov %%rdx, %0\n\t"
			: "=m" (*(FXulong *)_p));
#else
	__asm__("mov %0, %%edx\n\t"
			"mov 4%0, %%ecx\n\t"
			"bswap %%edx\n\t"
			"bswap %%ecx\n\t"
			"mov %%edx, 4%0\n\t"
			"mov %%ecx, %0\n\t"
			: "=m" (*(FXulong *)_p));
#endif
/*	__asm
	{
		mov edx, [_p]
		mov eax, [edx]
		mov ecx, [edx+4]
		bswap eax
		bswap ecx
		mov [edx+4], eax
		mov [edx], ecx
	}*/
}
#else
namespace FX {

/*! \ingroup fxassemblerops
Pretches a cache line into the processor cache temporally (ie; it will
be used multiple times) */
inline void fxprefetchmemT(const void *ptr) throw()
{
}
/*! \ingroup fxassemblerops
Pretches a cache line into the processor cache non-temporally (ie; it
will be used only once) */
inline void fxprefetchmemNT(const void *ptr) throw()
{
}
/*! \ingroup fxassemblerops
Forward scans an unsigned integer, returning the index of the first
set bit. Compiles into 21 x86 cycles with no branching, though on
x86 and x64 it directly uses the bsl instruction */
inline FXuint fxbitscan(FXuint x) throw()
{
   x = ~x & (x - 1);
   x = x - ((x >> 1) & 0x55555555);
   x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
   x = (x + (x >> 4)) & 0x0F0F0F0F;
   x = x + (x << 8);
   x = x + (x << 16);
   return x >> 24;
}
/*! \ingroup fxassemblerops
Backward scans an unsigned integer, returning the index of the
first set bit. Compiles into roughly 24 x86 cycles with no
branching, though on x86 and x64 it directly uses the bsr instruction.
You should note that this implementation uses illegal C++ which may
fail with aggressive enough optimisation - in this situation, enable
the alternative 36 x86 cycle implementation in the source code.
*/
inline FXuint fxbitscanrev(FXuint x) throw()
{
#if 1
	union {
		unsigned asInt[2];
		double asDouble;
	};
	int n;

	asDouble = (double)x + 0.5;
	n = 1054 - (asInt[!FOX_BIGENDIAN] >> 20);
	return n;
#else
	x = x | (x >> 1);
	x = x | (x >> 2);
	x = x | (x >> 4);
	x = x | (x >> 8);
	x = x | (x >>16);
	x = ~x;
	x = x - ((x >> 1) & 0x55555555);
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	x = (x + (x >> 4)) & 0x0F0F0F0F;
	x = x + (x << 8);
	x = x + (x << 16);
	return x >> 24;
#endif
}

/*! \ingroup fxassemblerops
Endian swaps the two bytes pointed to by \em p
*/
inline void fxendianswap2(void *_p)
{
	FXuchar *p=(FXuchar *) _p, t;
	t=p[0]; p[0]=p[1]; p[1]=t;
}

/*! \ingroup fxassemblerops
Endian swaps the four bytes pointed to by \em p
*/
inline void fxendianswap4(void *_p)
{
	FXuchar *p=(FXuchar *) _p, t;
	t=p[0]; p[0]=p[3]; p[3]=t;
	t=p[1]; p[1]=p[2]; p[2]=t;
}

/*! \ingroup fxassemblerops
Endian swaps the eight bytes pointed to by \em p
*/
inline void fxendianswap8(void *_p)
{
	FXuchar *p=(FXuchar *) _p, t;
	t=p[0]; p[0]=p[7]; p[7]=t;
	t=p[1]; p[1]=p[6]; p[6]=t;
	t=p[2]; p[2]=p[5]; p[5]=t;
	t=p[3]; p[3]=p[4]; p[4]=t;
}
#endif

} // namespace

#endif