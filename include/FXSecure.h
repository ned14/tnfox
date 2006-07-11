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
	/*! \class FRandomness
	\ingroup security
	\brief A fast quality source of pseudo entropy

	Unlike FX::Secure::PRandomness, this class provides a much faster but
	not cryptographically secure pseudo random generator. In this it is much
	like FX::fxrandom(), except that instead of a 2^32 period this one has a
	mathematically proven 2^19937 period and is considered a much higher quality
	generator. Also, unlike FX::Secure::PRandomness, this class doesn't require
	the OpenSSL library to be compiled in.

	More specifically, this class is a 64 bit implementation of the Mersenne Twister.
	You can find out more about this algorithm at http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html.

	\code
   A C-program for MT19937-64 (2004/9/29 version).
   Coded by Takuji Nishimura and Makoto Matsumoto.

   This is a 64-bit version of Mersenne Twister pseudorandom number
   generator.

   Before using, initialize the state by using init_genrand64(seed)  
   or init_by_array64(init_key, key_length).

   Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   References:
   T. Nishimura, ``Tables of 64-bit Mersenne Twisters''
     ACM Transactions on Modeling and 
     Computer Simulation 10. (2000) 348--357.
   M. Matsumoto and T. Nishimura,
     ``Mersenne Twister: a 623-dimensionally equidistributed
       uniform pseudorandom number generator''
     ACM Transactions on Modeling and 
     Computer Simulation 8. (Jan. 1998) 3--30.

   Any feedback is very welcome.
   http://www.math.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove spaces)
	\endcode
	*/
	class FRandomness
	{
		static const FXuint NN=312;
		static const FXuint MM=156;
		static const FXulong MATRIX_A=0xB5026F5AA96619E9ULL;
		static const FXulong UM=0xFFFFFFFF80000000ULL;	/* Most significant 33 bits */
		static const FXulong LM=0x7FFFFFFFULL;			/* Least significant 31 bits */

		FXulong mt[NN];									/* The array for the state vector */
		int mti;
	public:
		//! Constructs, using seed \em seed
		FRandomness(FXulong seed) throw() : mti(NN+1)
		{
			mt[0] = seed;
			for (mti=1; mti<NN; mti++) 
				mt[mti] =  (6364136223846793005ULL * (mt[mti-1] ^ (mt[mti-1] >> 62)) + mti);
		}

        //! Generates a random number on [0, 2^64-1]-interval
		FXulong int64() throw()
		{
			int i;
			FXulong x;
			static FXulong mag01[2]={0ULL, MATRIX_A};

			if (mti >= NN) { /* generate NN words at one time */

				for (i=0;i<NN-MM;i++) {
					x = (mt[i]&UM)|(mt[i+1]&LM);
					mt[i] = mt[i+MM] ^ (x>>1) ^ mag01[(int)(x&1ULL)];
				}
				for (;i<NN-1;i++) {
					x = (mt[i]&UM)|(mt[i+1]&LM);
					mt[i] = mt[i+(MM-NN)] ^ (x>>1) ^ mag01[(int)(x&1ULL)];
				}
				x = (mt[NN-1]&UM)|(mt[0]&LM);
				mt[NN-1] = mt[MM-1] ^ (x>>1) ^ mag01[(int)(x&1ULL)];

				mti = 0;
			}
		  
			x = mt[mti++];

			x ^= (x >> 29) & 0x5555555555555555ULL;
			x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
			x ^= (x << 37) & 0xFFF7EEE000000000ULL;
			x ^= (x >> 43);

			return x;
		}

		//! generates a random number on [0,1]-real-interval by division of 2^53-1
		double real1(void) throw()
		{
			return (int64() >> 11) * (1.0/9007199254740991.0);
		}

		//! generates a random number on [0,1)-real-interval by division of 2^53
		double real2(void) throw()
		{
			return (int64() >> 11) * (1.0/9007199254740992.0);
		}

		//! generates a random number on (0,1)-real-interval by division of 2^52
		double real3(void) throw()
		{
			return ((int64() >> 12) + 0.5) * (1.0/4503599627370496.0);
		}
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

