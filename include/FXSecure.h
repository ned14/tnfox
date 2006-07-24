/********************************************************************************
*                                                                               *
*                                  Security Tools                               *
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

#ifndef FXSECURE_H
#define FXSECURE_H

#include "FXStream.h"
#include "FXString.h"
#include <string.h>
#include <stdlib.h>
#include <new>

namespace FX {

/*! \file FXSecure.h
\brief Defines things used in providing secure data transport
*/
namespace Secure { struct heap { }; }
}

#if defined(_MSC_VER) && _MSC_VER<=1310
#pragma warning(push)
#pragma warning(disable: 4290)
#endif
/*! \ingroup security
<tt>operator new</tt> allocating in the secure heap */
FXAPI void *operator new(size_t size, const FX::Secure::heap &) throw(std::bad_alloc);
/*! \ingroup security
<tt>operator new[]</tt> allocating in the secure heap */
FXAPI void *operator new[](size_t size, const FX::Secure::heap &) throw(std::bad_alloc);
/*! \ingroup security
<tt>operator delete</tt> freeing in the secure heap */
FXAPI void operator delete(void *p, const FX::Secure::heap &) throw();
/*! \ingroup security
<tt>operator delete[]</tt> freeing in the secure heap */
FXAPI void operator delete[](void *p, const FX::Secure::heap &) throw();
#if defined(_MSC_VER) && _MSC_VER<=1310
#pragma warning(pop)
#endif

namespace FX {
/*! \namespace Secure
\ingroup security
\brief Defines a secure namespace
*/
namespace Secure
{
	extern FXAPI void *mallocI(size_t size) throw();
	extern FXAPI void *callocI(size_t no, size_t size) throw();
	extern FXAPI void free(void *p) throw();
	/*! \ingroup security
	Allocates memory in the secure heap */
	template<typename T> T *malloc(size_t size) throw() { return (T *) mallocI(size); }
	/*! \ingroup security
	Allocates memory in the secure heap */
	template<typename T> T *calloc(size_t no, size_t size) throw() { return (T *) callocI(no, size); }
	/*! \ingroup security
	Frees memory previously allocated in the secure heap */
	template<typename T> void free(T *p) throw() { free((void *) p); }

	/*! \struct TigerHashValue
	\ingroup security
	\brief Represents a 192 bit Tiger hash value

	\sa FX::Secure::TigerHash
	*/
	struct TigerHashValue
	{
		//! The data held by the value
		union Data_t	// 192 bits
		{
			FXuchar bytes[24];		//!< The value as bytes
			FXulong longlong[3];	//!< The value as 64 bit words
		} data;
		~TigerHashValue()
		{
			::memset(data.bytes, 0, sizeof(data));
		}
		//! Returns the hash as a hexadecimal string
		FXString asString() const
		{
#if FOX_BIGENDIAN==1
			FXString ret("0x%1%2%3");
#else
			FXString ret("0x%3%2%1");
#endif
			return ret.arg(data.longlong[0]).arg(data.longlong[1]).arg(data.longlong[2]);
		}
		bool operator==(const TigerHashValue &o) const throw() { return data.longlong[0]==o.data.longlong[0]
															&& data.longlong[1]==o.data.longlong[1]
															&& data.longlong[2]==o.data.longlong[2]; }
		bool operator!=(const TigerHashValue &o) const throw() { return !(*this==o); }
		friend FXStream &operator<<(FXStream &s, const TigerHashValue &v) { s.writeRawBytes(v.data.bytes, 24); return s; }
		friend FXStream &operator>>(FXStream &s, TigerHashValue &v) { s.readRawBytes(v.data.bytes, 24); return s; }
	};

	/*! \class Randomness
	\ingroup security
	\brief A source of true entropy

	For cryptographic work, a source of good randomness is essential. On modern Unices,
	\c /dev/urandom does the job but on Windows there is no such facility. This class
	fixes this problem portably.

	On Linux or BSD/MacOS X, it just uses \c /dev/urandom. On Windows NT, it creates
	randomness from the following sources:
	\li The disc i/o delta NT performance counter + salt
	\li The network i/o delta NT performance counter
	\li Mouse movements

	This should provide adequate randomness on both server and home machines, but
	especially on dual-purpose machines. Obviously if an attacker could access these
	values you would have a problem, but the same goes for \c /dev/urandom. If the
	attacker has an intercept directly on the network connection of the secure
	machine, they could probably guess the network i/o counter & machine uptime -
	however in today's modern computer installations, disc i/o is relatively
	unconnected with server load as various individual processes and the swap
	file interact with the system. To add a further system-dependent randomness,
	the salt is the average disc queue length (how much latency between asking
	for a read or write and the disc actually doing it), average time per transfer
	plus the percentage
	of time the system is currently spending doing disc activity. Since these
	figures depend greatly on the specification of your hard drive model and
	its connection to your motherboard's bus (never mind driver design), this
	should provide adequate saltiness.
	
	This data is all written into a 8192+8 bit ring buffer by a background thread
	started with the process. Only changed bits are stored as so to encourage
	entropy density and where no activity took place, nothing is stored. The +8
	in case you were wondering is because data reads are usually a power of two
	(eg; 256, 512, 1024 bit) and the extra byte causes a byte stipple per
	revolution through the ring buffer.

	Lastly, because it takes some time to read 8192+8 bits of this kind of
	randomness especially on a machine doing no i/o at all, the ring buffer
	is actually placed in shared memory so that
	a new process can immediately get to work. If random data is requested
	just after startup and it must first be read, reads will block until
	the requested quantity of random data is available.
	*/
	class FXAPI Randomness
	{
		Randomness();
		Randomness(const Randomness &);
		Randomness &operator=(const Randomness &);
	public:
		/*! Reads up to 1024 bytes (8192 bits) of randomness. Blocks until
		sufficient random data is available if necessary
		*/
		static FXuval readBlock(FXuchar *buffer, FXuval length);
		//! Returns how much randomness is already available
		static FXuval size();
	};
	/*! \class PRandomness
	\ingroup security
	\brief A cryptographically secure source of pseudo entropy

	The great trouble with a source of true entropy like FX::Secure::Randomness
	is that there is very little new random information accumulated by a computer
	with time (unless additional hardware is fitted). Therefore it makes sense to
	seed a cryptographically secure random number generator with true randomness
	and use the generator to generate lots of nearly true randomness. This is
	similar to normal pseudo random number generators such as \c fxrandom(), but
	the output is far more random plus the generated sequence is far longer.
	\note This module requires support for the OpenSSL library to be compiled in
	*/
	class FXAPI PRandomness
	{	// Defined in QSSLDevice
		PRandomness();
		PRandomness(const PRandomness &);
		PRandomness &operator=(const PRandomness &);
	public:
		/*! Reads any quantity of nearly random data. Reasonably fast, but
		you wouldn't want it in a repetitive loop.
		*/
		static FXuval readBlock(FXuchar *buffer, FXuval length);
		//! Freshens the generator with the specified number of bytes
		static void freshen(FXuval amount);
		//! Returns how much randomness is already available. Always (FXuval)-1
		static FXuval size() { return (FXuval)-1; }
	};

	/*! \class TigerHash
	\ingroup security
	\brief An implementation of the Tiger fast hashing algorithm

	This is an implementation of the Tiger fast hashing algorithm by Ross Anderson
	and Eli Biham. This hash algorithm outputs a 192 bit hash (as a FX::Secure::TigerHashValue)
	and is suitable for use in cryptography. At the time of writing (August 2003) no
	known faults exist within it, though because it is relatively new (1996) this may change.

	The advantages of this algorithm over others such as SHA-1 though are substantial.
	It works in 64 bit quantities which means all current PC memory architectures
	work to their best and with the introduction of 64 bit processors shortly, this is
	set to improve still further. Tiger is also \b fast, from the same to three times as
	fast as SHA-1. It is also much more non-linear than more conventional hash algorithms so
	that each bit of input has a much quicker propagation effect on the hash bits.
	However it also uses a brand-new method of generation and thus potential
	weaknesses are less well known. See http://www.cs.technion.ac.il/~biham/ and
	http://www.cl.cam.ac.uk/users/rja14/ for more information.

	Users should note that this implementation is biased in favour of little endian
	architectures and is only as good as the compiler can make it (though significant
	use is made of code inlining and custom versions for 32 & 64 bit architectures).
	*/
	class FXAPI TigerHash
	{
		FXuint mypasses;
	public:
		//! Constructs an instance
		TigerHash(FXuint passes=3) : mypasses(passes) { }
		//! Returns the number of passes used
		FXuint passes() const throw() { return mypasses; }
		//! Sets the number of passes to be used (default is 3, use higher values for more security)
		void setPasses(FXuint passes) throw() { mypasses=passes; }
		//! Calculates the tiger hash value of the specified block of data
		TigerHashValue calc(FXuchar *buffer, FXuval length) const throw();
	};

} // namespace
} // namespace
#endif

