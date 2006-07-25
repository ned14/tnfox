/********************************************************************************
*                                                                               *
*                                 Tools for maths                               *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2006 by Niall Douglas.   All Rights Reserved.            *
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

#ifndef FXMATHS_H
#define FXMATHS_H

#include "FXProcess.h"
#include "QThread.h"
#include "FXGenericTools.h"
#include "qmemarray.h"
#include <math.h>

namespace FX {

/*! \file FXMaths.h
\brief Defines a number of tools useful for maths
*/

namespace Maths {
	/*! \namespace Maths
	\brief Defines a set of tools for maths

	In the FX::Maths namespace are a number of assorted tools for performing maths
	operations. Each is optimised for as much inline generation as possible via
	metaprogramming, as well as making use of any SSE optimisations on your processor.

	Typically, your compiler will use x87 floating point operations. When SSE1
	is available, it will use SSE1 ops for float operations and when SSE2 is available,
	it will use SSE2 for both float and double operations. You will typically see a
	25% increase in performance with SSE2 over x87.

	Much like FX::Generic::BiggestValue and FX::Generic::SmallestValue, we have here
	FX::Maths::InfinityValue and FX::Maths::NaNValue. This solves a major problem of
	specifying these values as constants in C++ as both have very useful properties
	when working in floating point numbers - for example, if you have a floating point
	number which is invalid, you should set it to NaN so any operations with it
	will propagate the NaN, thereby catching the misuse.
	*/


	namespace Impl { namespace
	{
		template<typename type, bool minus> struct CalcInfinity;
		template<bool minus> struct CalcInfinity<float, minus>
		{
			union
			{
				FXuint integer;
				float floatingpoint;
			};
			CalcInfinity()
			{	// Directly poke in our value
				integer=minus ? 0xff800000 : 0x7f800000;
			}
			operator const float &()
			{
				return floatingpoint;
			}
		};
		template<bool minus> struct CalcInfinity<double, minus>
		{
			union
			{
				FXulong integer;
				double floatingpoint;
			};
			CalcInfinity()
			{	// Directly poke in our value
				integer=minus ? 0xfff0000000000000 : 0x7ff0000000000000;
			}
			operator const double &()
			{
				return floatingpoint;
			}
		};
		template<typename type, bool minus> struct InfinityValue
		{
			static CalcInfinity<type, minus> value;
		};
		CalcInfinity< float, false> InfinityValue< float, false>::value;
		CalcInfinity< float,  true> InfinityValue< float,  true>::value;
		CalcInfinity<double, false> InfinityValue<double, false>::value;
		CalcInfinity<double,  true> InfinityValue<double,  true>::value;

		template<typename type> struct CalcNaN;
		template<> struct CalcNaN<float>
		{
			union
			{
				FXuint integer;
				float floatingpoint;
			};
			CalcNaN()
			{	// Directly poke in our value
				integer=0x7fffffff;
			}
			operator const float &()
			{
				return floatingpoint;
			}
		};
		template<> struct CalcNaN<double>
		{
			union
			{
				FXulong integer;
				double floatingpoint;
			};
			CalcNaN()
			{	// Directly poke in our value
				integer=0x7fffffffffffffff;
			}
			operator const double &()
			{
				return floatingpoint;
			}
		};
		template<typename type> struct NaNValue
		{
			static CalcNaN<type> value;
		};
		CalcNaN< float> NaNValue< float>::value;
		CalcNaN<double> NaNValue<double>::value;
	} }
	/*! \struct InfinityValue
	\brief Returns -inf or +inf floating point values
	*/
	template<typename type, bool minus=false> struct InfinityValue : public Impl::InfinityValue<type, minus> { };
	/*! \struct NaNValue
	\brief Returns NaN floating point value
	*/
	template<typename type> struct NaNValue : public Impl::NaNValue<type> { };
	//! Returns true if the floating point value is a NaN
	template<typename type> inline bool isNaN(type val) throw();
	template<> inline bool isNaN<float>(float val) throw() { return *((FXuint *) &val)==*((FXuint *) &NaNValue<float>::value); }
	template<> inline bool isNaN<double>(double val) throw() { return *((FXulong *) &val)==*((FXulong *) &NaNValue<double>::value); }

	/*! \class Array
	\brief An N dimensional array
	*/
	template<typename type, unsigned int A> class Array
	{
	protected:
		type data[A];
	public:
		Array() { }
		explicit Array(const type *d) { for(FXuint n=0; n<A; n++) data[n]=d ? d[n] : 0; }
		type &operator[](unsigned int i) throw() { assert(i<A); return data[i]; }
		const type &operator[](unsigned int i) const throw() { assert(i<A); return data[i]; }
	};
	template<typename type, unsigned int A, unsigned int B> class Array2
	{
	protected:
		Array<type, A> data[B];
	public:
		explicit Array2(const type **d=0) throw() { for(FXuint b=0; b<B; b++) for(FXuint a=0; a<A; a++) data[b][a]=d ? d[b][a] : 0; }
		Array<type, A> &operator[](unsigned int i) throw() { assert(i<B); return data[i]; }
		const Array<type, A> &operator[](unsigned int i) const throw() { assert(i<B); return data[i]; }
	};

	/*! \class FRandomness
	\brief A fast quality source of pseudo entropy

	Unlike FX::Secure::PRandomness, this class provides a much faster but
	not cryptographically secure pseudo random generator. In this it is much
	like FX::fxrandom(), except that instead of a 2^32 period this one has a
	mathematically proven 2^19937 period and is considered a much higher quality
	generator. Also, unlike FX::Secure::PRandomness, this class doesn't require
	the OpenSSL library to be compiled in.

	Most use will by via FX::Maths::SysRandSrc.

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
		double real1() throw()
		{
			return (int64() >> 11) * (1.0/9007199254740991.0);
		}

		//! generates a random number on [0,1)-real-interval by division of 2^53
		double real2() throw()
		{
			return (int64() >> 11) * (1.0/9007199254740992.0);
		}

		//! generates a random number on (0,1)-real-interval by division of 2^52
		double real3() throw()
		{
			return ((int64() >> 12) + 0.5) * (1.0/4503599627370496.0);
		}
	};

	/*! \class SysRandomness
	\brief Threadsafe system source of randomness

	FX::Maths::FRandomness is not threadsafe, but this source is. You can
	access it via \c FX::Maths::SysRandSrc.
	*/
	class SysRandomness : protected QMutex, protected FRandomness
	{
	public:
		SysRandomness() : FRandomness(FXProcess::getNsCount()) { }
		FXulong int64()
		{
			QMtxHold h(this);
			return FRandomness::int64();
		}
		double real1()
		{
			QMtxHold h(this);
			return FRandomness::real1();
		}
		double real2()
		{
			QMtxHold h(this);
			return FRandomness::real2();
		}
		double real3()
		{
			QMtxHold h(this);
			return FRandomness::real3();
		}
	};
	extern FXAPI SysRandomness SysRandSrc;

	//! Optimised normal distribution PRNG. Returns stddevs from mean.
	inline double normalrand(FRandomness &src, double stddevs) throw()
	{
		double x, y, r2;
		do
		{
			x=-1+2*src.real3();
			y=-1+2*src.real3();
			r2=x*x+y*y;
		}
		while(r2>1.0 || r2==0);
		return stddevs*y*sqrt(-2.0*log(r2)/r2);
	}
	inline double normalrand(SysRandomness &src, double stddevs) throw()
	{
		double x, y, r2;
		do
		{
			x=-1+2*src.real3();
			y=-1+2*src.real3();
			r2=x*x+y*y;
		}
		while(r2>1.0 || r2==0);
		return stddevs*y*sqrt(-2.0*log(r2)/r2);
	}

	//! Transform a value according to the normal distribution. Returns stddevs from mean.
	template<typename type> inline type normaldist(type x, type stddevs) throw()
	{
		type u=x/fabs(stddevs);
		return (1/(sqrt(2*(type) PI)*fabs(stddevs)))*exp(-u*u/2);
	}

	//! Computes the mean, max, min and mode of an array
	template<typename type> inline type mean(const type *FXRESTRICT array, FXuval len, FXuint stride=1, type *FXRESTRICT min=0, type *FXRESTRICT max=0, type *FXRESTRICT mode=0) throw()
	{
		type m=0;
		if(min) *min=Generic::BiggestValue<type>::value;
		if(max) *max=Generic::BiggestValue<type, true>::value;
		if(mode) *mode=0;
		for(FXuval n=0; n<len; n+=stride)
		{
			m+=array[n];
			if(min && array[n]<*min) *min=array[n];
			if(max && array[n]>*max) *max=array[n];
			if(mode && (len/2==n || (len+1)/2==n)) *mode=*mode ? (*mode+array[n])/2 : array[n];
		}
		return m/len;
	}
	template<typename type> inline type mean(const QMemArray<type> &array, FXuint stride=1, type *FXRESTRICT min=0, type *FXRESTRICT max=0, type *FXRESTRICT mode=0) throw()
	{
		return mean(array.data(), array.count(), stride, max, min, mode);
	}

	//! Computes the variance of an array
	template<typename type> inline type variance(const type *FXRESTRICT array, FXuval len, FXuint stride=1, const type *FXRESTRICT _mean=0) throw()
	{
		type v=0, m=_mean ? *_mean : mean(array, len, stride);
		for(FXuval n=0; n<len; n+=stride)
		{
			const type d=array[n]-m;
			v+=d*d;
		}
		return v/((len/stride)-1);
	}
	template<typename type> inline type variance(const QMemArray<type> &array, FXuint stride=1, const type *FXRESTRICT _mean=0) throw()
	{
		return variance(array.data(), array.count(), stride, _mean);
	}

	//! Computes the standard deviation of an array
	template<typename type> inline type stddev(const type *FXRESTRICT array, FXuval len, FXuint stride=1, const type *FXRESTRICT _mean=0) throw()
	{
		return sqrt(variance(array, len, stride, _mean));
	}
	template<typename type> inline type stddev(const QMemArray<type> &array, FXuint stride=1, const type *FXRESTRICT _mean=0) throw()
	{
		return stddev(array.data(), array.count(), stride, _mean);
	}

	//! Computes distribution of an array. Returns min, max & bucket size in first three items.
	template<unsigned int buckets, typename type> inline Array<type, buckets+3> distribution(const type *FXRESTRICT array, FXuval len, FXuint stride=1, const type *FXRESTRICT min=0, const type *FXRESTRICT max=0) throw()
	{
		Array<type, buckets+3> ret;
		const type &_min=(min && max) ? *min : ret[0], &_max=(min && max) ? *max : ret[1];
		type &div=ret[2];
		if(!min || !max)
			mean(array, len, stride, &ret[0], &ret[1]);
		div=(_max-_min)/buckets;
		for(FXuval n=0; n<len; n+=stride)
		{
			for(FXuval bucket=0; bucket<buckets; bucket++)
			{
				if(array[n]>=_min+div*bucket && array[n]<_min+div*(bucket+1))
					++ret[3+bucket];
			}
		}
		return ret;
	}
	template<unsigned int buckets, typename type> inline Array<type, buckets+3> distribution(const QMemArray<type> &array, FXuint stride=1, const type *FXRESTRICT min=0, const type *FXRESTRICT max=0) throw()
	{
		return distribution<type, buckets>(array.data(), array.count(), stride, min, max);
	}

} // namespace

} // namespace

#endif
