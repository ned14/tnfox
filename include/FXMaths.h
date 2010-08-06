/********************************************************************************
*                                                                               *
*                                 Tools for maths                               *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2006-2009 by Niall Douglas.   All Rights Reserved.       *
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
#include "FXStream.h"
#include "qmemarray.h"
#include <math.h>
#include "FXVec2f.h"
#include "FXVec2d.h"
#include "FXVec3f.h"
#include "FXVec3d.h"
#include "FXMat3f.h"
#include "FXMat3d.h"
#include "FXVec4f.h"
#include "FXVec4d.h"
#include "FXMat4f.h"
#include "FXMat4d.h"
#if _M_IX86_FP>=3 || defined(__SSE3__)
#include "pmmintrin.h"
#endif
#if _M_IX86_FP>=4 || defined(__SSE4__)
#include "smmintrin.h"
//#include "nmmintrin.h"	// We only use the SSE4.1 dot product so this is unneeded
#endif

namespace FX {

/*! \file FXMaths.h
\brief Defines a number of tools useful for maths
*/

namespace Maths {
	/*! \namespace Maths
	\brief Defines a set of tools for maths

	In the FX::Maths namespace are a number of assorted tools for performing maths
	operations. Each is optimised for as much inline generation as possible via
	metaprogramming, as well as making use of any specialised vector hardware on your
	processor.

	Typically on x86, your compiler will use x87 floating point operations. When SSE1
	is available, it will use SSE1 ops for float operations and when SSE2 is available,
	it will use SSE2 for both float and double operations. You will typically see a
	25% increase in performance with SSE2 over x87, and a \em further 250-400% increase
	when using SSE optimised vectors via FX::Maths::Vector. The Vector implementation
	is future proof, so writing your code with it now will be exponentially faster
	again on newer technology using GPU-like parallel processing.

	Much like FX::Generic::BiggestValue and FX::Generic::SmallestValue, we have here
	FX::Maths::InfinityValue and FX::Maths::NaNValue. This solves a major problem of
	specifying these values as constants in C++ as both have very useful properties
	when working in floating point numbers - for example, if you have a floating point
	number which is invalid, you should set it to NaN so any operations with it
	will propagate the NaN, thereby catching the misuse.
	*/

	//! Returns the minimum of two values
	template<typename type> inline const type &min(const type &a, const type &b) { return (a<b) ? a : b; }
	//! Returns the maximum of two values
	template<typename type> inline const type &max(const type &a, const type &b) { return (a>b) ? a : b; }
	//! Returns the square root of a value
	template<typename type> inline type sqrt(const type &v) { return ::sqrt(v); }
	template<> inline float sqrt<float>(const float &v) { return ::sqrtf(v); }
	//! Returns the reciprocal of a value
	template<typename type> inline type rcp(const type &v) { return 1/v; }
	//! Returns the reciprocal square root of a value
	template<typename type> inline type rsqrt(const type &v) { return 1/sqrt(v); }

	namespace Impl
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
				integer=minus ? 0xfff0000000000000ULL : 0x7ff0000000000000ULL;
			}
			operator const double &()
			{
				return floatingpoint;
			}
		};
		template<typename type, bool minus> struct InfinityValue
		{
			static const CalcInfinity<type, minus> &value()
			{
				static const CalcInfinity<type, minus> foo;
				return foo;
			}
		};

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
			operator const float &() const
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
				integer=0x7fffffffffffffffULL;
			}
			operator const double &() const
			{
				return floatingpoint;
			}
		};
		template<typename type> struct NaNValue
		{
			static const CalcNaN<type> &value()
			{
				static const CalcNaN<type> foo;
				return foo;
			}
		};
		template<unsigned int size> struct TwoPowerMemAligner { };
#if defined(_MSC_VER)
		// Don't assert on compilers not supporting memory alignment
		template<> struct FXMEMALIGNED(4)   TwoPowerMemAligner<4>   { TwoPowerMemAligner() { assert(!((FXuval)this & 3)); } };
		template<> struct FXMEMALIGNED(8)   TwoPowerMemAligner<8>   { TwoPowerMemAligner() { assert(!((FXuval)this & 7)); } };
		template<> struct FXMEMALIGNED(16)  TwoPowerMemAligner<16>  { TwoPowerMemAligner() { assert(!((FXuval)this & 15)); } };
		template<> struct FXMEMALIGNED(32)  TwoPowerMemAligner<32>  { TwoPowerMemAligner() { assert(!((FXuval)this & 31)); } };
		template<> struct FXMEMALIGNED(64)  TwoPowerMemAligner<64>  { TwoPowerMemAligner() { assert(!((FXuval)this & 63)); } };
		template<> struct FXMEMALIGNED(128) TwoPowerMemAligner<128> { TwoPowerMemAligner() { assert(!((FXuval)this & 127)); } };
		template<> struct FXMEMALIGNED(256) TwoPowerMemAligner<256> { TwoPowerMemAligner() { assert(!((FXuval)this & 255)); } };
		template<> struct FXMEMALIGNED(512) TwoPowerMemAligner<512> { TwoPowerMemAligner() { assert(!((FXuval)this & 511)); } };
		template<> struct FXMEMALIGNED(1024) TwoPowerMemAligner<1024> { TwoPowerMemAligner() { assert(!((FXuval)this & 1023)); } };
#elif defined(__GNUC__)
#if __GNUC__<4 || (__GNUC__==4 && __GNUC_MINOR__<3)
#warning Before v4.3 GCC will not allow selective structure alignment - FX::Maths::Vector will not be aligned!
#define FXVECTOR_BUGGYGCCALIGNMENTHACK FXMEMALIGNED(16)
#else
		// GCC's alignment support on x86 and x64 is nearly useless
		// as it won't go above 16 bytes :(
		template<> struct FXMEMALIGNED(4)   TwoPowerMemAligner<4>   { TwoPowerMemAligner() { assert(!((FXuval)this & 3)); } };
		template<> struct FXMEMALIGNED(8)   TwoPowerMemAligner<8>   { TwoPowerMemAligner() { assert(!((FXuval)this & 7)); } };
		template<> struct FXMEMALIGNED(16)  TwoPowerMemAligner<16>  { TwoPowerMemAligner() { assert(!((FXuval)this & 15)); } };
		template<> struct FXMEMALIGNED(32)  TwoPowerMemAligner<32>  { TwoPowerMemAligner() { assert(!((FXuval)this & 15)); } };
		template<> struct FXMEMALIGNED(64)  TwoPowerMemAligner<64>  { TwoPowerMemAligner() { assert(!((FXuval)this & 15)); } };
		template<> struct FXMEMALIGNED(128) TwoPowerMemAligner<128> { TwoPowerMemAligner() { assert(!((FXuval)this & 15)); } };
		template<> struct FXMEMALIGNED(256) TwoPowerMemAligner<256> { TwoPowerMemAligner() { assert(!((FXuval)this & 15)); } };
		template<> struct FXMEMALIGNED(512) TwoPowerMemAligner<512> { TwoPowerMemAligner() { assert(!((FXuval)this & 15)); } };
		template<> struct FXMEMALIGNED(1024) TwoPowerMemAligner<1024> { TwoPowerMemAligner() { assert(!((FXuval)this & 15)); } };
#endif
#endif
#ifndef FXVECTOR_BUGGYGCCALIGNMENTHACK
#define FXVECTOR_BUGGYGCCALIGNMENTHACK
#endif
		template<typename type, unsigned int A, class supertype, bool _isArithmetic, bool _isInteger, typename SIMDType=char> class VectorBase
		{
		protected:
			union
			{
				type data[A];
				SIMDType v;
			};
		public:
			//! The container type
			typedef type TYPE;
			//! The dimension
			static const unsigned int DIMENSION=A;
			//! True if arithmetric
			static const bool isArithmetic=_isArithmetic;
			//! True if integer
			static const bool isInteger=_isInteger;
			VectorBase() { }
			operator const supertype &() const { return static_cast<const supertype &>(*this); }
			VectorBase(const VectorBase &o) { for(FXuint n=0; n<A; n++) data[n]=o.data[n]; }
			VectorBase &operator=(const VectorBase &o) { for(FXuint n=0; n<A; n++) data[n]=o.data[n]; return *this; }
			explicit VectorBase(const type *d) { for(FXuint n=0; n<A; n++) data[n]=d ? d[n] : 0; }
			explicit VectorBase(const type &d) { for(FXuint n=0; n<A; n++) data[n]=d; }
			//! Retrieves a component
			type operator[](unsigned int i) const { assert(i<A); return data[i]; }
			//! Sets a component
			VectorBase &set(unsigned int i, const type &d) { assert(i<A); data[i]=d; return *this; }
		};
#define VECTOR1OP(op)  VectorBase operator op () const                    { VectorBase ret; for(FXuint n=0; n<A; n++) ret.data[n]=op Base::data[n]; return ret; }
#define VECTOR2OP(op)  VectorBase operator op (const VectorBase &o) const { VectorBase ret; for(FXuint n=0; n<A; n++) ret.data[n]=Base::data[n] op o.data[n]; return ret; }
#define VECTORP2OP(op) VectorBase &operator op (const VectorBase &o)      {                 for(FXuint n=0; n<A; n++) Base::data[n] op o.data[n]; return *this; }
#define VECTORFUNC(op) friend VectorBase op(const VectorBase &o)          { VectorBase ret; for(FXuint n=0; n<A; n++) ret.data[n]= Maths::op (o.data[n]); return ret; }
#define VECTOR2FUNC(op) friend VectorBase op(const VectorBase &a, const VectorBase &b) { VectorBase ret; for(FXuint n=0; n<A; n++) ret.data[n]= op (a.data[n], b.data[n]); return ret; }
		template<typename type, unsigned int A, class supertype, bool isInteger, typename SIMDType> class VectorBase<type, A, supertype, true, isInteger, SIMDType> : private TwoPowerMemAligner<sizeof(type)*A>, public VectorBase<type, A, supertype, false, false, SIMDType>
		{
			typedef VectorBase<type, A, supertype, false, false, SIMDType> Base;
		public:
			static const bool isArithmetic=true;
			VectorBase() { }
			explicit VectorBase(const type *d) : Base(d) { }
			explicit VectorBase(const type &d) : Base(d) { }
			// Arithmetic (int & fp)
			VECTOR1OP(+) VECTOR1OP(-) VECTOR1OP(!)
			 VECTOR2OP(+)    VECTOR2OP(-)   VECTOR2OP(*)   VECTOR2OP(/)
			VECTORP2OP(+=) VECTORP2OP(-=) VECTORP2OP(*=) VECTORP2OP(/=)
			// Logical (int & fp)
			VECTOR2OP( == ) VECTOR2OP( != ) VECTOR2OP( < )  VECTOR2OP( <= ) VECTOR2OP( > )  VECTOR2OP( >= )
			VECTOR2OP( && ) VECTOR2OP( || )

			VECTORFUNC(sqrt) VECTORFUNC(rcp) VECTORFUNC(rsqrt)
			VECTOR2FUNC(min) VECTOR2FUNC(max)
			//! Returns true if all elements are zero
			friend bool isZero(const VectorBase &a)
			{
				bool iszero=true;
				for(FXuint n=0; n<A && iszero; n++)
					iszero=iszero && !a.data[n];
				return iszero;
			}
			//! Returns the sum of the elements
			friend type sum(const VectorBase &a)
			{
				type ret=0;
				for(FXuint n=0; n<A; n++)
					ret+=a.data[n];
				return ret;
			}
			//! Returns the dot product
			friend type dot(const VectorBase &a, const VectorBase &b)
			{
				type ret=0;
				for(FXuint n=0; n<A; n++)
					ret+=a.data[n]*b.data[n];
				return ret;
			}
		};
		template<typename type, unsigned int A, class supertype, typename SIMDType> class VectorBase<type, A, supertype, true, true, SIMDType> : public VectorBase<type, A, supertype, true, false, SIMDType>
		{
			typedef VectorBase<type, A, supertype, true, false, SIMDType> Base;
		public:
			static const bool isArithmetic=true;
			static const bool isInteger=true;
			VectorBase() { }
			explicit VectorBase(const type *d) : Base(d) { }
			explicit VectorBase(const type &d) : Base(d) { }

			// Arithmetic (int only)
			 VECTOR2OP(%)   VECTOR2OP(&)   VECTOR2OP(|)   VECTOR2OP(^)   VECTOR2OP(<<)   VECTOR2OP(>>)  VECTOR1OP(~)
			VECTORP2OP(%=) VECTORP2OP(&=) VECTORP2OP(|=) VECTORP2OP(^=) VECTORP2OP(<<=) VECTORP2OP(>>=)

			//! Bitwise shifts the entire vector left
			friend VectorBase lshiftvec(const VectorBase &a, int shift)
			{
				VectorBase ret;
				FXuint offset=(shift/8)/sizeof(type);
				shift-=(offset*sizeof(type))*8;
				for(FXint n=A-1; n>=0; n--)
					ret.data[n]=(a.data[n-offset]<<shift)|((0==n) ? 0 : (a.data[n-offset-1]>>(8*sizeof(type)-shift)));
				return ret;
			}
			//! Bitwise shifts the entire vector right
			friend VectorBase rshiftvec(const VectorBase &a, int shift)
			{
				VectorBase ret;
				FXuint offset=(shift/8)/sizeof(type);
				shift-=(offset*sizeof(type))*8;
				for(FXuint n=0; n<A; n++)
					ret.data[n]=(a.data[n+offset]>>shift)|((A==n+offset+1) ? 0 : (a.data[n+offset+1]<<(8*sizeof(type)-shift)));
				return ret;
			}
		};
#undef VECTOR1OP
#undef VECTOR2OP
#undef VECTORP2OP
#undef VECTORFUNC
#undef VECTOR2FUNC

		// Used to provide implicit conversions between these and FOX classes
		template<class base, typename type, class equivtype> class EquivType : public base
		{
		public:
			EquivType() { }
			template<typename F> explicit EquivType(const F &d) : base(d) { }
			EquivType &operator=(const equivtype &o) { return *this=*((const EquivType *)&o); }
			operator equivtype &() { return *((equivtype *)this); }
			operator const equivtype &() const { return *((const equivtype *)this); }
		};
		template<class base, typename type> class EquivType<base, type, void> : public base
		{
		public:
			EquivType() { }
			template<typename F> explicit EquivType(const F &d) : base(d) { }
		};

		// Used to conglomerate multiple SIMD vector ops
		template<class vectortype, unsigned int N, class supertype, bool _isArithmetic=vectortype::isArithmetic, bool _isInteger=vectortype::isInteger> class VectorOfVectors
		{
		protected:
			vectortype vectors[N];
		public:
			//! The container type
			typedef typename vectortype::TYPE TYPE;
			//! The dimension
			static const unsigned int DIMENSION=vectortype::DIMENSION*N;
			//! True if arithmetric
			static const bool isArithmetic=_isArithmetic;
			//! True if integer
			static const bool isInteger=_isInteger;
			VectorOfVectors() { }
			operator const supertype &() const { return static_cast<const supertype &>(*this); }
			VectorOfVectors(const VectorOfVectors &o)
			{
				for(FXuint n=0; n<N; n++)
					vectors[n]=o.vectors[n];
			}
			VectorOfVectors &operator=(const VectorOfVectors &o)
			{
				for(FXuint n=0; n<N; n++)
					vectors[n]=o.vectors[n];
				return *this;
			}
			explicit VectorOfVectors(const TYPE *d)
			{
				for(FXuint n=0; n<N; n++)
					vectors[n]=vectortype(FXOFFSETPTR(d, n*sizeof(vectortype)));
			}
			explicit VectorOfVectors(const TYPE &d)
			{
				for(FXuint n=0; n<N; n++)
					vectors[n]=vectortype(d);
			}
			//! Retrieves a component
			TYPE operator[](unsigned int i) const { assert(i<DIMENSION); return vectors[i/vectortype::DIMENSION][i%vectortype::DIMENSION]; }
			//! Sets a component
			VectorOfVectors &set(unsigned int i, const TYPE &d) { assert(i<DIMENSION); vectors[i/vectortype::DIMENSION].set(i%vectortype::DIMENSION, d); return *this; }
		};
#define VECTOR1OP(op)  VectorOfVectors operator op () const                         { VectorOfVectors ret; for(FXuint n=0; n<N; n++) ret.vectors[n]=op Base::vectors[n]; return ret; }
#define VECTOR2OP(op)  VectorOfVectors operator op (const VectorOfVectors &o) const { VectorOfVectors ret; for(FXuint n=0; n<N; n++) ret.vectors[n]=Base::vectors[n] op o.vectors[n]; return ret; }
#define VECTORP2OP(op) VectorOfVectors &operator op (const VectorOfVectors &o)      {                      for(FXuint n=0; n<N; n++) Base::vectors[n] op o.vectors[n]; return *this; }
#define VECTORFUNC(op) friend VectorOfVectors op(const VectorOfVectors &o)          { VectorOfVectors ret; for(FXuint n=0; n<N; n++) ret.vectors[n]= op (o.vectors[n]); return ret; }
#define VECTOR2FUNC(op) friend VectorOfVectors op(const VectorOfVectors &a, const VectorOfVectors &b) { VectorOfVectors ret; for(FXuint n=0; n<N; n++) ret.vectors[n]= op (a.vectors[n], b.vectors[n]); return ret; }
		template<class vectortype, unsigned int N, class supertype, bool isInteger> class VectorOfVectors<vectortype, N, supertype, true, isInteger> : public VectorOfVectors<vectortype, N, supertype, false, false>
		{
		protected:
			typedef VectorOfVectors<vectortype, N, supertype, false, false> Base;
		public:
			typedef typename vectortype::TYPE TYPE;
			VectorOfVectors() { }
			explicit VectorOfVectors(const TYPE *d) : Base(d) { }
			explicit VectorOfVectors(const TYPE &d) : Base(d) { }
			// Arithmetic (int & fp)
			VECTOR1OP(+) VECTOR1OP(-) VECTOR1OP(!)
			 VECTOR2OP(+)    VECTOR2OP(-)   VECTOR2OP(*)   VECTOR2OP(/)
			VECTORP2OP(+=) VECTORP2OP(-=) VECTORP2OP(*=) VECTORP2OP(/=)
			// Logical (int & fp)
			VECTOR2OP( == ) VECTOR2OP( != ) VECTOR2OP( < )  VECTOR2OP( <= ) VECTOR2OP( > )  VECTOR2OP( >= )
			VECTOR2OP( && ) VECTOR2OP( || )

			VECTORFUNC(sqrt) VECTORFUNC(rcp) VECTORFUNC(rsqrt)
			VECTOR2FUNC(min) VECTOR2FUNC(max)
			//! Returns true if all elements are zero
			friend bool isZero(const VectorOfVectors &a)
			{
				bool iszero=true;
				for(FXuint n=0; n<N && iszero; n++)
					iszero&=isZero(a.vectors[n]);
				return iszero;
			}
			//! Returns the sum of the elements
			friend TYPE sum(const VectorOfVectors &a)
			{
				TYPE ret=0;
				for(FXuint n=0; n<N; n++)
					ret+=sum(a.vectors[n]);
				return ret;
			}
			//! Returns the dot product
			friend TYPE dot(const VectorOfVectors &a, const VectorOfVectors &b)
			{
				TYPE ret=0;
				for(FXuint n=0; n<N; n++)
					ret+=dot(a.vectors[n], b.vectors[n]);
				return ret;
			}
		};
		template<class vectortype, unsigned int N, class supertype> class VectorOfVectors<vectortype, N, supertype, true, true> : public VectorOfVectors<vectortype, N, supertype, true, false>
		{
		protected:
			typedef VectorOfVectors<vectortype, N, supertype, true, false> Base;
		public:
			typedef typename vectortype::TYPE TYPE;
			VectorOfVectors() { }
			explicit VectorOfVectors(const TYPE *d) : Base(d) { }
			explicit VectorOfVectors(const TYPE &d) : Base(d) { }

			// Arithmetic (int only)
			 VECTOR2OP(%)   VECTOR2OP(&)   VECTOR2OP(|)   VECTOR2OP(^)   VECTOR2OP(<<)   VECTOR2OP(>>)  VECTOR1OP(~)
			VECTORP2OP(%=) VECTORP2OP(&=) VECTORP2OP(|=) VECTORP2OP(^=) VECTORP2OP(<<=) VECTORP2OP(>>=)

			VECTOR2FUNC(lshiftvec) VECTOR2FUNC(rshiftvec)
		};
#undef VECTOR1OP
#undef VECTOR2OP
#undef VECTORP2OP
#undef VECTORFUNC
#undef VECTOR2FUNC
	}
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
	template<> inline bool isNaN<float>(float val) throw() { union { float f; FXuint i; } v; v.f=val; return v.i==NaNValue<float>::value().integer; }
	template<> inline bool isNaN<double>(double val) throw() { union { double f; FXulong i; } v; v.f=val; return v.i==NaNValue<double>::value().integer; }

	/*! \class Vector
	\brief A SIMD based N dimensional vector

	This is a generic vector of size \em A of \em type components. Specialisations have
	been provided for SIMD quantities on platforms which support those, so for example
	on Intel SSE platforms four floats will map to a \c __m128 SSE register. Where the
	size is a power of two up to 1Kb, the compiler is asked to memory align
	the vector and an assert is added to ensure instantiation will not succeed without
	correct alignment. If doing your own memory allocation, make sure to align to sixteen
	bytes: FX::malloc and FX::calloc both have an alignment parameter, and you can use
	FX::aligned_allocator<T, 16> for STL containers.

	\warning GCC on x86 and x64 cannot currently align stack allocated variables any
	better than 16 bytes. Therefore the assertion checks are reduced to a check for
	16 byte alignment on GCC.

	Furthermore, processor SIMD is used for ALL two power sized
	vectors up to 1Kb (256 floats or 128 doubles) so a vector of sixteen floats will be
	implemented as four lots of \c __m128 SSE operations on current processor technology.
	This means that you can write now for upcoming vector processors eg; Intel's upcoming
	Advanced Vector Extensions (AVX) which use a GPU-like parallel processing engine (Larrabee)
	to process blocks of 256-1024 bit vectors (8 to 32 floats) at once. For this same reason,
	operands available for this class have been kept minimal - \c sin() probably is too big
	for a simple math processor.

	\warning This use of combining multiple SIMD vectors to implement a bigger vector
	does not currently optimise well with most compilers. Both GCC v4.2 and MSVC9 refuse
	to realise that more than one SIMD op can be performed in parallel and force everything
	through xmm0. Intel's C++ compiler does do the right thing, but forces a load & store
	via memory between ops which is entirely unnecessary and I haven't found a way to
	prevent this (it seems to think memory may get clobbered). This SIMD problem is known
	to the GCC and MSVC authors and imminently upcoming versions fix this register
	allocation problem.

	All of the standard arithmetic, logical and comparison operators are provided but
	they are only defined according to what FX::Generic::TraitsBasic<type> says. If the
	type is not an arithmetical one, no operators are defined; if floating point, then
	the standard arithmetic ones; if integer, then the standard plus logical operators.
	When arithmetical, the additional friend functions have been provided: <tt>isZero(),
	min(), max(), sum(), dot()</tt>; for floating-point only: <tt>sqrt(), rcp(), rsqrt()</tt>;
	for integer only: <tt>lshiftvec(), rshiftvec()</tt>.
	These can be invoked via Koenig lookup so you can use them as though they were in
	the C library.

	FOX provides hardwired versions of this class in the forms of FX::FXVec2f, FX::FXVec2d,
	FX::FXVec3f, FX::FXVec3d, FX::FXVec4f, FX::FXVec4d. These are nothing like as fast,
	and also they are designed in a highly SIMD unfriendly way - FX::Maths::Vector was
	deliberately designed with an inconvenient API to force high performance programming.

	<h3>Implementation:</h3>
	The following combinations have been optimised:
	\li When compiled with SSE support, Vector<float, 4> uses \c __m128
	\li When compiled with SSE2 support, Vector<double, 2> uses \c __m128d.
	Vector<FXushort|FXshort,8>, Vector<FXuint|FXint,4> both use \c __m128i (SSE2
	does not have a full set of instructions for 16 chars nor 2 long long's).
	\li When compiled with SSE3 support, sum() uses horizontal adding.
	\li When compiled with SSE4 support, dot() uses the direct SSE4.1 instruction.

	For double precision on SSE2 only, rcp(), rsqrt() are no faster (nor slower) than
	doing it manually - only on SSE do they have special instructions.

	For integers on SSE2 only, multiplication, division, modulus, min(), max() are emulated (slowly)
	as they don't have corresponding SSE instructions available. For SSE4 only,
	multiplication, min(), max() is SSE optimised.

	Note that the SSE2 optimised bit shift ignores all but the lowest member - for
	future compatibility you should set all members of the shift quantity to be
	identical. lshiftvec() and rshiftvec() treat the entire vector as higher indexed members
	being higher bits. On little endian machines, this leads to shifts occurring
	within their member types going "the wrong way" and then leaping to the next
	member. \em Usually, you want this. \b Only bit shifts which are multiples of
	eight are accelerated on SSE2.

	See FX::Maths::Array and FX::Maths::Matrix for a static array letting you easily implement
	a matrix. See also the FXVECTOROFVECTORS macro for how to declare to the compiler
	when a vector should be implemented as a sequence of other vectors (this is how
	the SSE specialisations overload specialisations for two power increments) - if
	you want a non-two power size, you'll need to declare the VectorOfVectors
	specialisation manually.
	*/
	template<typename type, unsigned int A> class Vector : public Impl::EquivType<Impl::VectorBase<type, A, Vector<type, A>, Generic::Traits<type>::isArithmetical, Generic::Traits<type>::isInt>, type, void>
	{
		typedef Impl::EquivType<Impl::VectorBase<type, A, Vector<type, A>, Generic::Traits<type>::isArithmetical, Generic::Traits<type>::isInt>, type, void> Base;
	public:
		Vector() { }
		//! Initialises from an array
		explicit Vector(const type *d) : Base(d) { }
		//! Initialises all members to a certain value
		explicit Vector(const type &d) : Base(d) { }
	};
	// Map to FOX types
#define DEFINEVECTOREQUIV(type, A, equivtype) \
	template<> class Vector<type, A> : public Impl::EquivType<Impl::VectorBase<type, A, Vector<type, A>, Generic::Traits<type>::isArithmetical, Generic::Traits<type>::isInt>, type, void> \
	{ \
		typedef Impl::EquivType<Impl::VectorBase<type, A, Vector<type, A>, Generic::Traits<type>::isArithmetical, Generic::Traits<type>::isInt>, type, void> Base; \
	public: \
		Vector() { } \
		explicit Vector(const type *d) : Base(d) { } \
		explicit Vector(const type &d) : Base(d) { } \
	};
	DEFINEVECTOREQUIV(FXuchar,  8, FXulong)
	DEFINEVECTOREQUIV(FXushort, 4, FXulong)
	DEFINEVECTOREQUIV(FXuint,   2, FXulong)
	DEFINEVECTOREQUIV(FXuchar,  4, FXuint)
	DEFINEVECTOREQUIV(FXushort, 2, FXuint)
	DEFINEVECTOREQUIV(FXuchar,  2, FXushort)
	DEFINEVECTOREQUIV(float, 2, FXVec2f)
	DEFINEVECTOREQUIV(float, 3, FXVec3f)
	typedef Vector<float, 2> Vector2f;
	typedef Vector<float, 3> Vector3f;
	typedef Vector<float, 4> Vector4f;
	DEFINEVECTOREQUIV(double, 3, FXVec3d)
	typedef Vector<double, 2> Vector2d;
	typedef Vector<double, 3> Vector3d;
	typedef Vector<double, 4> Vector4d;

	template<typename type, unsigned int A> FXStream &operator<<(FXStream &s, const Vector<type, A> &v)
	{
		for(FXuint n=0; n<A; n++) s << v[n];
		return s;
	}
	template<typename type, unsigned int A> FXStream &operator>>(FXStream &s, Vector<type, A> &v)
	{
		type t;
		for(FXuint n=0; n<A; n++) { s >> t; v.set(n, t); }
		return s;
	}
//! Specialises a FX::Maths::Vector to be implemented as another vector
#define FXVECTOROFVECTORS(VECTORTYPE, ELEMENTS, equivtype) template<> class Vector<VECTORTYPE::TYPE, ELEMENTS> : public Impl::EquivType<Impl::VectorOfVectors<VECTORTYPE, ELEMENTS/VECTORTYPE::DIMENSION, Vector<VECTORTYPE::TYPE, ELEMENTS> >, VECTORTYPE::TYPE, equivtype> \
	{ \
		typedef Impl::EquivType<Impl::VectorOfVectors<VECTORTYPE, ELEMENTS/VECTORTYPE::DIMENSION, Vector<VECTORTYPE::TYPE, ELEMENTS> >, VECTORTYPE::TYPE, equivtype> Base; \
	public: \
		Vector() { } \
		explicit Vector(const VECTORTYPE::TYPE *d) : Base(d) { } \
		explicit Vector(const VECTORTYPE::TYPE &d) : Base(d) { } \
	};


#if 1	// Use to disable SIMD optimised versions
#if (defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))) || (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))
	// The x86 and x64 SSE specialisations
#if defined(_M_X64) || defined(__x86_64__) || (defined(_M_IX86) && _M_IX86_FP>=1) || (defined(__i386__) && defined(__SSE__))
#define FXVECTOR_SPECIALISEDFLOAT4
	template<> class FXVECTOR_BUGGYGCCALIGNMENTHACK Vector<float, 4> : private Impl::TwoPowerMemAligner<16>
	{
	public:
		typedef float TYPE;
		static const unsigned int DIMENSION=4;
		static const bool isArithmetic=true;
		static const bool isInteger=false;
	private:
		typedef __m128 SSETYPE;
		SSETYPE v;
		static TYPE int_extract(SSETYPE v, unsigned int i)
		{
			SSETYPE t;
			switch(i)
			{
			case 0:
				t=v; /*t=_mm_shuffle_ps(v, v, 0);*/ break;
			case 1:
				t=_mm_shuffle_ps(v, v, 1); break;
			case 2:
				t=_mm_shuffle_ps(v, v, 2); break;
			case 3:
				t=_mm_shuffle_ps(v, v, 3); break;
			}
			TYPE ret;
			_mm_store_ss(&ret, t);
			return ret;
		}
		static SSETYPE int_ones()
		{
			static SSETYPE v=_mm_set1_ps(1);
			return v;
		}
		static SSETYPE int_not()
		{
			static SSETYPE v=_mm_cmpeq_ps(int_ones(), int_ones());
			return v;
		}
		static SSETYPE int_notones()
		{
			static SSETYPE v=_mm_xor_ps(int_ones(), int_not());
			return v;
		}
	public:
		Vector() { }
		Vector(const SSETYPE &_v) { v=_v; }
		Vector(const Vector &o) { v=o.v; }
		Vector &operator=(const Vector &o) { v=o.v; return *this; }

		Vector(const FXVec4f &o) { v=_mm_loadu_ps((float*)&o); }
		Vector &operator=(const FXVec4f &o) { v=_mm_loadu_ps((float*)&o); return *this; }
		operator FXVec4f &() { return *((FXVec4f *)this); }
		operator const FXVec4f &() const { return *((const FXVec4f *)this); }

		explicit Vector(const TYPE *d)
		{
			if(!d) v=_mm_setzero_ps();
			else v=_mm_loadu_ps(d);
		}
		explicit Vector(const TYPE &d)
		{
			v=_mm_set1_ps(d);
		}
		TYPE operator[](unsigned int i) const
		{
			return int_extract(v, i);
		}
		Vector &set(unsigned int i, const TYPE &d)
		{
			union { TYPE f[DIMENSION]; SSETYPE v; } t;
			assert(i<DIMENSION);
			t.v=v;
			t.f[i]=d;
			v=t.v;
			return *this;
		}
		Vector operator +() const { return *this; }
		Vector operator -() const { Vector ret((TYPE *)0); ret-=*this; return ret; }
		Vector operator !() const { return _mm_and_ps(int_ones(), _mm_cmpneq_ps(_mm_setzero_ps(), v)); }
#define VECTOR2OP(op, sseop)  Vector operator op (const Vector &o) const { return _mm_ ##sseop## _ps(v, o.v); }
#define VECTORP2OP(op, sseop) Vector &operator op (const Vector &o)      { v=_mm_ ##sseop## _ps(v, o.v); return *this; }
#define VECTORFUNC(op) friend Vector op(const Vector &o)                 { return _mm_ ##op## _ps(o.v); }
#define VECTOR2FUNC(op) friend Vector op(const Vector &a, const Vector &b) { return _mm_ ##op## _ps(a.v, b.v); }
		 VECTOR2OP(+,  add)  VECTOR2OP(-,  sub)  VECTOR2OP(*,  mul)  VECTOR2OP(/,  div)
		VECTORP2OP(+=, add) VECTORP2OP(-=, sub) VECTORP2OP(*=, mul) VECTORP2OP(/=, div)

#undef VECTOR2OP
#define VECTOR2OP(op, sseop)  Vector operator op (const Vector &o) const { return _mm_and_ps(int_ones(), _mm_cmp ##sseop## _ps(v, o.v)); }
		VECTOR2OP( == , eq) VECTOR2OP( != , neq) VECTOR2OP( < , lt)  VECTOR2OP( <= , le) VECTOR2OP( > , gt)  VECTOR2OP( >= , ge)
#undef VECTOR2OP
#define VECTOR2OP(op, sseop)  Vector operator op (const Vector &o) const { return _mm_and_ps(int_ones(), _mm_cmpneq_ps(_mm_setzero_ps(), _mm_ ##sseop## _ps(v, o.v))); }
		VECTOR2OP( && , and) VECTOR2OP( || , or)

		VECTORFUNC(sqrt) VECTORFUNC(rcp) VECTORFUNC(rsqrt)
		VECTOR2FUNC(min) VECTOR2FUNC(max)
#undef VECTOR1OP
#undef VECTOR2OP
#undef VECTORP2OP
#undef VECTORFUNC
#undef VECTOR2FUNC
		friend bool isZero(const Vector &a)
		{	// We can use a cunning trick here
			return !_mm_movemask_ps(a.v)&&!_mm_movemask_ps(_mm_sub_ps(_mm_setzero_ps(), a.v));
		}
		friend TYPE sum(const Vector &a)
		{
#if _M_IX86_FP>=3 || defined(__SSE3__)
			SSETYPE v=_mm_hadd_ps(a.v, a.v);
			return int_extract(_mm_hadd_ps(v, v), 0);
#elif 1
			// This is actually the same speed as two hadd's on my machine, but
			// hadd is supposed to be quicker on newer processors
			SSETYPE tempA = _mm_shuffle_ps(a.v,a.v, _MM_SHUFFLE(2,0,2,0));
			SSETYPE tempB = _mm_shuffle_ps(a.v,a.v, _MM_SHUFFLE(3,1,3,1));
			SSETYPE tempC = _mm_add_ps(tempB, tempA);
			tempA = _mm_shuffle_ps(tempC,tempC, _MM_SHUFFLE(2,0,2,0));
			tempB = _mm_shuffle_ps(tempC,tempC, _MM_SHUFFLE(3,1,3,1));
			tempC = _mm_add_ss(tempB, tempA);
			return int_extract(tempC, 0);
#else
			FXMEMALIGNED(16) TYPE f[DIMENSION];
			_mm_store_ps(f, a.v);
			return f[0]+f[1]+f[2]+f[3];
#endif
		}
		friend TYPE dot(const Vector &a, const Vector &b)
		{
#if _M_IX86_FP>=4 || defined(__SSE4__)
			// Use the SSE4.1 instruction
			return int_extract(_mm_dp_ps(a.v, b.v, 0xf1), 0);
#else
			// SSE implementation
			return sum(a*b);
#endif
		}
		friend inline SSETYPE &GetSSEVal(Vector &a) { return a.v; }
		friend inline const SSETYPE &GetSSEVal(const Vector &a) { return a.v; }
	};
	typedef Vector<float, 4> int_SSEOptimised_float4;	// Needed as macros don't understand template types :(
	FXVECTOROFVECTORS(int_SSEOptimised_float4, 8, void);
	FXVECTOROFVECTORS(int_SSEOptimised_float4, 16, void);
	FXVECTOROFVECTORS(int_SSEOptimised_float4, 32, void);
	FXVECTOROFVECTORS(int_SSEOptimised_float4, 64, void);
	FXVECTOROFVECTORS(int_SSEOptimised_float4, 128, void);
	FXVECTOROFVECTORS(int_SSEOptimised_float4, 256, void);
#endif
#if defined(_M_X64) || defined(__x86_64__) || (defined(_M_IX86) && _M_IX86_FP>=2) || (defined(__i386__) && defined(__SSE2__))
#define FXVECTOR_SPECIALISEDDOUBLE2
	template<> class FXVECTOR_BUGGYGCCALIGNMENTHACK Vector<double, 2> : private Impl::TwoPowerMemAligner<16>
	{
	public:
		typedef double TYPE;
		static const unsigned int DIMENSION=2;
		static const bool isArithmetic=true;
		static const bool isInteger=false;
	private:
		typedef __m128d SSETYPE;
		SSETYPE v;
		static TYPE int_extract(SSETYPE v, unsigned int i)
		{
			SSETYPE t;
			switch(i)
			{
			case 0:
				t=v; /*_mm_shuffle_pd(v, v, 0);*/ break;
			case 1:
				t=_mm_shuffle_pd(v, v, 1); break;
			}
			TYPE ret;
			_mm_store_sd(&ret, t);
			return ret;
		}
		static SSETYPE int_ones()
		{
			static SSETYPE v=_mm_set1_pd(1);
			return v;
		}
		static SSETYPE int_not()
		{
			static SSETYPE v=_mm_cmpeq_pd(int_ones(), int_ones());
			return v;
		}
		static SSETYPE int_notones()
		{
			static SSETYPE v=_mm_xor_pd(int_ones(), int_not());
			return v;
		}
	public:
		Vector() { }
		Vector(const SSETYPE &_v) { v=_v; }
		Vector(const Vector &o) { v=o.v; }
		Vector &operator=(const Vector &o) { v=o.v; return *this; }

		Vector(const FXVec2d &o) { v=_mm_loadu_pd((double*)&o); }
		Vector &operator=(const FXVec2d &o) { v=_mm_loadu_pd((double*)&o); return *this; }
		operator FXVec2d &() { return *((FXVec2d *)this); }
		operator const FXVec2d &() const { return *((const FXVec2d *)this); }

		explicit Vector(const TYPE *d)
		{
			if(!d) v=_mm_setzero_pd();
			else v=_mm_loadu_pd(d);
		}
		explicit Vector(const TYPE &d)
		{
			v=_mm_set1_pd(d);
		}
		TYPE operator[](unsigned int i) const
		{
			return int_extract(v, i);
		}
		Vector &set(unsigned int i, const TYPE &d)
		{
			union { TYPE f[DIMENSION]; SSETYPE v; } t;
			assert(i<DIMENSION);
			t.v=v;
			t.f[i]=d;
			v=t.v;
			return *this;
		}
		Vector operator +() const { return *this; }
		Vector operator -() const { Vector ret((TYPE *)0); ret-=*this; return ret; }
		Vector operator !() const { return _mm_and_pd(int_ones(), _mm_cmpneq_pd(_mm_setzero_pd(), v)); }
#define VECTOR2OP(op, sseop)  Vector operator op (const Vector &o) const { return _mm_ ##sseop## _pd(v, o.v); }
#define VECTORP2OP(op, sseop) Vector &operator op (const Vector &o)      { v=_mm_ ##sseop## _pd(v, o.v); return *this; }
#define VECTORFUNC(op) friend Vector op(const Vector &o)                 { return _mm_ ##op## _pd(o.v); }
#define VECTOR2FUNC(op) friend Vector op(const Vector &a, const Vector &b) { return _mm_ ##op## _pd(a.v, b.v); }
		 VECTOR2OP(+,  add)  VECTOR2OP(-,  sub)  VECTOR2OP(*,  mul)  VECTOR2OP(/,  div)
		VECTORP2OP(+=, add) VECTORP2OP(-=, sub) VECTORP2OP(*=, mul) VECTORP2OP(/=, div)

#undef VECTOR2OP
#define VECTOR2OP(op, sseop)  Vector operator op (const Vector &o) const { return _mm_and_pd(int_ones(), _mm_cmp ##sseop## _pd(v, o.v)); }
		VECTOR2OP( == , eq) VECTOR2OP( != , neq) VECTOR2OP( < , lt)  VECTOR2OP( <= , le) VECTOR2OP( > , gt)  VECTOR2OP( >= , ge)
#undef VECTOR2OP
#define VECTOR2OP(op, sseop)  Vector operator op (const Vector &o) const { return _mm_and_pd(int_ones(), _mm_cmpneq_pd(_mm_setzero_pd(), _mm_ ##sseop## _pd(v, o.v))); }
		VECTOR2OP( && , and) VECTOR2OP( || , or)

		VECTORFUNC(sqrt)
		VECTOR2FUNC(min) VECTOR2FUNC(max)
#undef VECTOR1OP
#undef VECTOR2OP
#undef VECTORP2OP
#undef VECTORFUNC
#undef VECTOR2FUNC
		friend Vector rcp(const Vector &o) { return _mm_div_pd(_mm_set1_pd(1), o.v); }
		friend Vector rsqrt(const Vector &o) { return _mm_div_pd(_mm_set1_pd(1), _mm_sqrt_pd(o.v)); }
		friend bool isZero(const Vector &a)
		{	// We can use a cunning trick here
			return !_mm_movemask_pd(a.v)&&!_mm_movemask_pd(_mm_sub_pd(_mm_setzero_pd(), a.v));
		}
		friend TYPE sum(const Vector &a)
		{
#if _M_IX86_FP>=3 || defined(__SSE3__)
			return int_extract(_mm_hadd_pd(a.v, a.v), 0);
#elif 1
			SSETYPE tempA = _mm_shuffle_pd(a.v,a.v, _MM_SHUFFLE2(0,0));
			SSETYPE tempB = _mm_shuffle_pd(a.v,a.v, _MM_SHUFFLE2(1,1));
			return int_extract(_mm_add_sd(tempB, tempA), 0);
#else
			FXMEMALIGNED(16) TYPE f[DIMENSION];
			_mm_store_pd(f, a.v);
			return f[0]+f[1];
#endif
		}
		friend TYPE dot(const Vector &a, const Vector &b)
		{
#if _M_IX86_FP>=4 || defined(__SSE4__)
			// Use the SSE4.1 instruction
			return int_extract(_mm_dp_pd(a.v, b.v, 0xf1), 0);
#else
			// SSE implementation
			return sum(a*b);
#endif
		}
		friend inline SSETYPE &GetSSEVal(Vector &a) { return a.v; }
		friend inline const SSETYPE &GetSSEVal(const Vector &a) { return a.v; }
	};
	typedef Vector<double, 2> int_SSEOptimised_double2;	// Needed as macros don't understand template types :(
	FXVECTOROFVECTORS(int_SSEOptimised_double2, 4, FXVec4d);
	FXVECTOROFVECTORS(int_SSEOptimised_double2, 8, void);
	FXVECTOROFVECTORS(int_SSEOptimised_double2, 16, void);
	FXVECTOROFVECTORS(int_SSEOptimised_double2, 32, void);
	FXVECTOROFVECTORS(int_SSEOptimised_double2, 64, void);
	FXVECTOROFVECTORS(int_SSEOptimised_double2, 128, void);


	// Now for the integer vectors, we can save ourselves some work by reusing Impl::VectorBase
#define VECTOR2OP_A(op, sseop, sseending)  Vector operator op (const Vector &o) const { return _mm_ ##sseop## _ ##sseending (v, o.v); }
#define VECTORP2OP_A(op, sseop, sseending) Vector &operator op (const Vector &o)      { v=_mm_ ##sseop## _ ##sseending (v, o.v); return *this; }
#define VECTOR2OP_A2(op, sseop, sseending)  Vector operator op (const Vector &o) const { return _mm_ ##sseop## _ ##sseending (v, _mm_cvtsi32_si128(_mm_cvtsi128_si32(o.v))); }
#define VECTORP2OP_A2(op, sseop, sseending) Vector &operator op (const Vector &o)      { v=_mm_ ##sseop## _ ##sseending (v, _mm_cvtsi32_si128(_mm_cvtsi128_si32(o.v))); return *this; }
#define VECTOR2OP_B(op, sseop, sseending)  Vector operator op (const Vector &o) const { return _mm_ ##sseop## _si128(v, o.v); }
#define VECTORP2OP_B(op, sseop, sseending) Vector &operator op (const Vector &o)      { v=_mm_ ##sseop## _si128(v, o.v); return *this; }
#define VECTOR2OP_C(op, sseop, sseending)  Vector operator op (const Vector &o) const { return _mm_and_si128(int_ones(), _mm_cmp ##sseop## _ ##sseending (v, o.v)); }
#define VECTOR2OP_D(op, sseop, sseending)  Vector operator op (const Vector &o) const { return _mm_andnot_si128(_mm_cmp ##sseop## _ ##sseending (v, o.v), int_ones()); }
#define VECTOR2OP_E(op, sseop, sseending)  Vector operator op (const Vector &o) const { return _mm_xor_si128(int_ones(), _mm_cmpeq_ ##sseending (_mm_setzero_si128(), _mm_ ##sseop## _si128(v, o.v))); }
#define VECTOR2FUNC(op, sseop, sseending)  friend Vector op(const Vector &a, const Vector &b) { return _mm_ ##sseop## _ ##sseending (a.v, b.v); }
#if _M_IX86_FP>=4 || defined(__SSE4__)
#define VECTOR2OP_A_SSE4ONLY(op, sseop, sseending)  VECTOR2OP_A(op, sseop, sseending)
#define VECTORP2OP_A_SSE4ONLY(op, sseop, sseending) VECTORP2OP_A(op, sseop, sseending)
#define VECTOR2FUNC_SSE4ONLY(op, sseop, sseending) VECTOR2FUNC(op, sseop, sseending)
#define VECTORISZERO 1==_mm_testc_si128(a.v, _mm_setzero_si128())
#else
#define VECTOR2OP_A_SSE4ONLY(op, sseop, sseending)
#define VECTORP2OP_A_SSE4ONLY(op, sseop, sseending)
#define VECTOR2FUNC_SSE4ONLY(op, sseop, sseending)
#define VECTORISZERO 65535==_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_setzero_si128(), a.v))
#endif
#define VECTORINTEGER(vint, vsize, sseending, signage1, signage2) \
	template<> class FXVECTOR_BUGGYGCCALIGNMENTHACK Vector<vint, vsize> : public Impl::VectorBase<vint, vsize, Vector<vint, vsize>, true, true, __m128i> \
	{ \
	private: \
		typedef Impl::VectorBase<vint, vsize, Vector<vint, vsize>, true, true, __m128i> Base; \
		typedef __m128i SSETYPE; \
		static SSETYPE int_ones() \
		{ \
			static SSETYPE v=_mm_set1_ ##sseending (1); \
			return v; \
		} \
		static SSETYPE int_not() \
		{ \
			static SSETYPE v=_mm_cmpeq_ ##sseending (int_ones(), int_ones()); \
			return v; \
		} \
		static SSETYPE int_notones() \
		{ \
			static SSETYPE v=_mm_xor_si128(int_ones(), int_not()); \
			return v; \
		} \
	public: \
		Vector() { } \
		Vector(const SSETYPE &_v) { v=_v; } \
		Vector(const Vector &o) { v=o.v; } \
		Vector &operator=(const Vector &o) { v=o.v; return *this; } \
		explicit Vector(const TYPE *d) \
		{ \
			if(!d) v=_mm_setzero_si128(); \
			else v=_mm_loadu_si128((SSETYPE *) d); \
		} \
		explicit Vector(const TYPE &d) \
		{ \
			v=_mm_set1_ ##sseending (d); \
		} \
		Vector operator +() const { return *this; } \
		Vector operator -() const { Vector ret((TYPE *)0); ret-=*this; return ret; } \
		Vector operator !() const { return _mm_andnot_si128(_mm_cmpeq_ ##sseending (_mm_setzero_si128(), v), int_ones()); } \
		Vector operator ~() const { return _mm_xor_si128(int_not(), v); } \
		VECTOR2OP_A(+,  add, sseending)  VECTOR2OP_A(-,  sub, sseending)   VECTOR2OP_A2(<<,  sll, sseending)  VECTOR2OP_A2(>>,  sr ## signage1, sseending) \
		VECTORP2OP_A(+=, add, sseending) VECTORP2OP_A(-=, sub, sseending) VECTORP2OP_A2(<<=, sll, sseending) VECTORP2OP_A2(>>=, sr ## signage1, sseending) \
		VECTOR2OP_A_SSE4ONLY(*, mullo, sseending) \
		VECTORP2OP_A_SSE4ONLY(*, mullo, sseending) \
 \
		 VECTOR2OP_B( & ,  and, sseending)   VECTOR2OP_B( | ,  or, sseending)   VECTOR2OP_B( ^ ,  xor, sseending) \
		VECTORP2OP_B( &= , and, sseending)  VECTORP2OP_B( |= , or, sseending)  VECTORP2OP_B( ^= , xor, sseending)  \
 \
		VECTOR2OP_C( == , eq, sseending)  VECTOR2OP_C( < , lt, sseending)  VECTOR2OP_C( > , gt, sseending)   \
		VECTOR2OP_D( != , eq, sseending)  VECTOR2OP_D( <= , gt, sseending) VECTOR2OP_D( >= , lt, sseending) \
		VECTOR2OP_E( && , and, sseending) VECTOR2OP_E( || , or, sseending) \
 \
		VECTOR2FUNC_SSE4ONLY(min, min, signage2) VECTOR2FUNC_SSE4ONLY(max, max, signage2) \
		friend bool isZero(const Vector &a) \
		{ \
			return VECTORISZERO; \
		} \
		friend Vector lshiftvec(const Vector &a, int shift) \
		{ \
			if(shift&7) \
				return lshiftvec(static_cast<const Vector::Base &>(a), shift); \
			else switch(shift/8) \
			{ \
				case 0:  return a; \
				case 1:  return _mm_slli_si128(a.v, 1); \
				case 2:  return _mm_slli_si128(a.v, 2); \
				case 3:  return _mm_slli_si128(a.v, 3); \
				case 4:  return _mm_slli_si128(a.v, 4); \
				case 5:  return _mm_slli_si128(a.v, 5); \
				case 6:  return _mm_slli_si128(a.v, 6); \
				case 7:  return _mm_slli_si128(a.v, 7); \
				case 8:  return _mm_slli_si128(a.v, 8); \
				case 9:  return _mm_slli_si128(a.v, 9); \
				case 10: return _mm_slli_si128(a.v, 10); \
				case 11: return _mm_slli_si128(a.v, 11); \
				case 12: return _mm_slli_si128(a.v, 12); \
				case 13: return _mm_slli_si128(a.v, 13); \
				case 14: return _mm_slli_si128(a.v, 14); \
				case 15: return _mm_slli_si128(a.v, 15); \
			} \
			return _mm_setzero_si128(); \
		} \
		friend Vector rshiftvec(const Vector &a, int shift) \
		{ \
			if(shift&7) \
				return rshiftvec(static_cast<const Vector::Base &>(a), shift); \
			else switch(shift/8) \
			{ \
				case 0:  return a; \
				case 1:  return _mm_srli_si128(a.v, 1); \
				case 2:  return _mm_srli_si128(a.v, 2); \
				case 3:  return _mm_srli_si128(a.v, 3); \
				case 4:  return _mm_srli_si128(a.v, 4); \
				case 5:  return _mm_srli_si128(a.v, 5); \
				case 6:  return _mm_srli_si128(a.v, 6); \
				case 7:  return _mm_srli_si128(a.v, 7); \
				case 8:  return _mm_srli_si128(a.v, 8); \
				case 9:  return _mm_srli_si128(a.v, 9); \
				case 10: return _mm_srli_si128(a.v, 10); \
				case 11: return _mm_srli_si128(a.v, 11); \
				case 12: return _mm_srli_si128(a.v, 12); \
				case 13: return _mm_srli_si128(a.v, 13); \
				case 14: return _mm_srli_si128(a.v, 14); \
				case 15: return _mm_srli_si128(a.v, 15); \
			} \
			return _mm_setzero_si128(); \
		} \
		friend inline SSETYPE &GetSSEVal(Vector &a) { return a.v; } \
		friend inline const SSETYPE &GetSSEVal(const Vector &a) { return a.v; } \
	};
//VECTORINTEGER(FXuchar, 16, epi8)
//VECTORINTEGER(FXchar,  16, epi8)
VECTORINTEGER(FXushort, 8, epi16, l, epu16)
VECTORINTEGER(FXshort,  8, epi16, a, epi16)
VECTORINTEGER(FXuint,   4, epi32, l, epu32)
VECTORINTEGER(FXint,    4, epi32, a, epi32)
//VECTORINTEGER(FXulong,  2, epi64)
//VECTORINTEGER(FXlong,   2, epi64)
#undef VECTOR2OP_A
#undef VECTORP2OP_A
#undef VECTOR2OP_A2
#undef VECTORP2OP_A2
#undef VECTOR2OP_B
#undef VECTORP2OP_B
#undef VECTOR2OP_C
#undef VECTOR2OP_D
#undef VECTOR2OP_E
#undef VECTOR2FUNC
#undef VECTOR2OP_A_SSE4ONLY
#undef VECTORP2OP_A_SSE4ONLY
#undef VECTOR2FUNC_SSE4ONLY
#undef VECTORISZERO
#undef VECTORINTEGER
#define VECTORINTEGER(vint, vsize) \
	typedef Vector<vint, vsize> int_SSEOptimised_##vint; \
	FXVECTOROFVECTORS(int_SSEOptimised_##vint, vsize*2, void); \
	FXVECTOROFVECTORS(int_SSEOptimised_##vint, vsize*4, void); \
	FXVECTOROFVECTORS(int_SSEOptimised_##vint, vsize*8, void); \
	FXVECTOROFVECTORS(int_SSEOptimised_##vint, vsize*16, void); \
	FXVECTOROFVECTORS(int_SSEOptimised_##vint, vsize*32, void); \
	FXVECTOROFVECTORS(int_SSEOptimised_##vint, vsize*64, void);
VECTORINTEGER(FXushort, 8)
VECTORINTEGER(FXshort,  8)
VECTORINTEGER(FXuint,   4)
VECTORINTEGER(FXint,    4)
#undef VECTORINTEGER
#endif
#endif
#endif

#ifndef FXVECTOR_SPECIALISEDFLOAT4
	DEFINEVECTOREQUIV(float, 4, FXVec4f)
#endif
#ifndef FXVECTOR_SPECIALISEDDOUBLE2
	DEFINEVECTOREQUIV(double, 2, FXVec2d)
	DEFINEVECTOREQUIV(double, 4, FXVec4d)
#endif
#undef DEFINEVECTOREQUIV

	/*! \class Array
	\brief A fixed-length array

	This is useful for denoting compile-time fixed length arrays. It emulates the STL
	vector class so you can iterate it. If you are thinking of a vector, use
	FX::Maths::Vector instead and if you are thinking of a matrix, use FX::Maths::Matrix
	instead. That said, an Array<Vector<float, 64>, 64> can make sense in certain
	circumstances.
	*/
	template<typename type, unsigned int A> class Array
	{
		type data[A];
	public:
		typedef type value_type;
		typedef value_type &reference;
		typedef const value_type &const_reference;
		typedef value_type *iterator;
		typedef const value_type *const_iterator;
		size_t max_size() const { return A; }

		Array() { }
		explicit Array(const type *d) { for(FXuint b=0; b<A; b++) data[b]=d ? type(d[b]) : type(); }
		bool operator==(const Array &o) const { for(FXuint b=0; b<A; b++) if(data[b]!=o.data[b]) return false; return true; }
		bool operator!=(const Array &o) const { for(FXuint b=0; b<A; b++) if(data[b]!=o.data[b]) return true; return false; }

		reference at(int i) { assert(i<A); return data[i]; }
		const_reference at(int i) const { assert(i<A); return data[i]; }
		reference operator[](int i) { return at(i); }
		const_reference operator[](int i) const { return at(i); }

		reference front() { return data[0]; }
		const_reference front() const { return data[0]; }
		reference back() { return data[A-1]; }
		const_reference back() const { return data[A-1]; }
		iterator begin() { return &data[0]; }
		const_iterator begin() const { return &data[0]; }
		iterator end() { return &data[A]; }
		const_iterator end() const { return &data[A]; }
	};

	/*! \class Matrix
	\brief A fixed-length matrix

	This is the class to use when you want a SIMD optimised matrix. Internally it
	will specialise itself for when it is two power items wide (not high) to the same level
	as FX::Maths::Vector. If you don't want the constraints of SIMD, you can always
	use a FX::FXMat3f etc. It emulates the STL vector class so you can iterate it.

	Note that a Matrix<Vector<float, 4>, 4, 4> works and is good for working with tensors.
	*/
	namespace Impl {
		template<typename type, unsigned int A, unsigned int B, bool isFP> class MatrixIt;
		// Yet to be implemented
		template<typename type, unsigned int N, unsigned int A, unsigned int B> class MatrixIt<Vector<type, N>, A, B, false>;
		// The floating point specialisation
		template<typename type, unsigned int A, unsigned int B> class MatrixIt<type, A, B, true>
		{
		protected:
			Vector<type, A> data[B];
		public:
			typedef Vector<type, A> VECTORTYPE;
			static const unsigned int WIDTH=A, HEIGHT=B;
			MatrixIt() { }
			explicit MatrixIt(const type *d)
			{
				for(FXuint b=0; b<B; b++)
					data[b]=VECTORTYPE(d ? d+b*A : 0);
			}
			explicit MatrixIt(const type (*d)[A])
			{
				for(FXuint b=0; b<B; b++)
					data[b]=VECTORTYPE(d ? d[b] : 0);
			}
			bool operator==(const MatrixIt &o) const
			{
				for(FXuint b=0; b<B; b++)
					if(isZero(data[b]==o.data[b]))
						return false;
				return true;
			}
			bool operator!=(const MatrixIt &o) const
			{
				for(FXuint b=0; b<B; b++)
					if(isZero(data[b]==o.data[b]))
						return true;
				return false;
			}
			MatrixIt inline operator *(const MatrixIt &_x) const;
			template<typename type2, unsigned int A2, unsigned int B2> friend inline MatrixIt<type2, A2, B2, true> transpose(const MatrixIt<type2, A2, B2, true> &v);
		};
		template<typename type, unsigned int A, unsigned int B> MatrixIt<type, A, B, true> inline MatrixIt<type, A, B, true>::operator *(const MatrixIt<type, A, B, true> &_x) const
		{	// We can make heavy use of the dot product here
			MatrixIt<type, A, B, true> ret, x(transpose(_x));
			for(FXuint b=0; b<B; b++)
				for(FXuint a=0; a<A; a++)
					ret.data[b].set(a, dot(data[b], x.data[a]));
			return ret;
		}
		template<typename type, unsigned int A, unsigned int B> inline MatrixIt<type, A, B, true> transpose(const MatrixIt<type, A, B, true> &v)
		{	// No SIMD available to swap bytes around and endian conversion isn't big enough :(
			MatrixIt<type, A, B, true> ret;
			for(FXuint b=0; b<B; b++)
				for(FXuint a=0; a<A; a++)
					ret.data[b].set(a, v.data[a][b]);
			return ret;
		}
#ifdef FXVECTOR_SPECIALISEDFLOAT4
		template<> inline MatrixIt<float, 4, 4, true> transpose(const MatrixIt<float, 4, 4, true> &v)
		{
			MatrixIt<float, 4, 4, true> ret(v);
			_MM_TRANSPOSE4_PS(GetSSEVal(ret.data[0]), GetSSEVal(ret.data[1]), GetSSEVal(ret.data[2]), GetSSEVal(ret.data[3]));
			return ret;
		}
#if _M_IX86_FP>=4 || defined(__SSE4__)
		template<> MatrixIt<float, 4, 4, true> inline MatrixIt<float, 4, 4, true>::operator *(const MatrixIt<float, 4, 4, true> &_x) const
		{	// The SSE4 fast dot() makes the normal algorithm worthwhile
			MatrixIt<float, 4, 4, true> ret;
			__m128 x[4];
			for(FXuint b=0; b<4; b++)
				x[b]=GetSSEVal(_x.data[b]);
			_MM_TRANSPOSE4_PS(x[0], x[1], x[2], x[3]);
			for(FXuint b=0; b<4; b++)
				for(FXuint a=0; a<4; a++)
					ret.data[b].set(a, dot(data[b], x[a]));
			return ret;
		}
#else
		template<> MatrixIt<float, 4, 4, true> inline MatrixIt<float, 4, 4, true>::operator *(const MatrixIt<float, 4, 4, true> &_x) const
		{	// In pure SSE
			MatrixIt<float, 4, 4, true> ret;
			__m128 acc, temp, row, x[4];
			for(FXuint b=0; b<4; b++)
				x[b]=GetSSEVal(_x.data[b]);
			// To avoid horizontal adding, accumulate vertically
			for(FXuint b=0; b<4; b++)
			{
				row=GetSSEVal(data[b]);
				temp=_mm_shuffle_ps(row, row, _MM_SHUFFLE(0,0,0,0)); acc=_mm_mul_ps(temp, x[0]);
				temp=_mm_shuffle_ps(row, row, _MM_SHUFFLE(1,1,1,1)); acc=_mm_add_ps(acc, _mm_mul_ps(temp, x[1]));
				temp=_mm_shuffle_ps(row, row, _MM_SHUFFLE(2,2,2,2)); acc=_mm_add_ps(acc, _mm_mul_ps(temp, x[2]));
				temp=_mm_shuffle_ps(row, row, _MM_SHUFFLE(3,3,3,3)); acc=_mm_add_ps(acc, _mm_mul_ps(temp, x[3]));
				ret.data[b]=acc;
			}
			return ret;
		}
#endif
#endif
		template<typename type, unsigned int A, unsigned int B> class MatrixI : public MatrixIt<type, A, B, Generic::Traits<type>::isFloat>
		{
		protected:
			typedef MatrixIt<type, A, B, Generic::Traits<type>::isFloat> Base;
		public:
			typedef type value_type;
			typedef value_type &reference;
			typedef const value_type &const_reference;
			typedef value_type *iterator;
			typedef const value_type *const_iterator;
			size_t max_size() const { return A*B; }

			MatrixI() { }
			MatrixI(const Base &o) : Base(o) { }
			explicit MatrixI(const type *d) : Base(d) { }
			explicit MatrixI(const type (*d)[A]) : Base(d) { }

			value_type at(int a, int b) { assert(a<A && b<B); return Base::data[b][a]; }
			value_type at(int a, int b) const { assert(a<A && b<B); return Base::data[b][a]; }
			value_type operator[](int i) { return at(i%A, i/A); }
			value_type operator[](int i) const { return at(i%A, i/A); }

			reference front() { return Base::data[0]; }
			const_reference front() const { return Base::data[0]; }
			reference back() { return Base::data[B-1][A-1]; }
			const_reference back() const { return Base::data[B-1][A-1]; }
			iterator begin() { return &Base::data[0]; }
			const_iterator begin() const { return &Base::data[0]; }
			iterator end() { return &Base::data[B][0]; }
			const_iterator end() const { return &Base::data[B][0]; }
		};
	}
	template<typename type, unsigned int A, unsigned int B> class Matrix : public Impl::EquivType<Impl::MatrixI<type, A, B>, type, void>
	{
	protected:
		typedef Impl::EquivType<Impl::MatrixI<type, A, B>, type, void> Base;
	public:
		Matrix() { }
		Matrix(const Base &o) : Base(o) { }
		Matrix(const typename Base::Base &o) : Base(o) { }
		explicit Matrix(const type *d) : Base(d) { }
		explicit Matrix(const type (*d)[A]) : Base(d) { }
	};
#define DEFINEMATRIXEQUIV(type, no, equivtype) \
	template<> class Matrix<type, no, no> : public Impl::EquivType<Impl::MatrixI<type, no, no>, type, equivtype> \
	{ \
		typedef Impl::EquivType<Impl::MatrixI<type, no, no>, type, equivtype> Base; \
	public: \
		Matrix() { } \
		Matrix(const Base &o) : Base(o) { } \
		Matrix(const Base::Base &o) : Base(o) { } \
		explicit Matrix(const type *d) : Base(d) { } \
		explicit Matrix(const type (*d)[no]) : Base(d) { } \
		Matrix(const equivtype &o) : Base((const type *)&o) { } \
	};
	DEFINEMATRIXEQUIV(float, 3, FXMat3f)
	DEFINEMATRIXEQUIV(float, 4, FXMat4f)
	typedef Matrix<float, 3, 3> Matrix3f;
	typedef Matrix<float, 4, 4> Matrix4f;
	DEFINEMATRIXEQUIV(double, 3, FXMat3d)
	DEFINEMATRIXEQUIV(double, 4, FXMat4d)
	typedef Matrix<double, 3, 3> Matrix3d;
	typedef Matrix<double, 4, 4> Matrix4d;
#undef DEFINEMATRIXEQUIV

	/*! \class FRandomness
	\brief A fast quality source of pseudo entropy

	Unlike FX::Secure::PRandomness, this class provides a much faster but
	not cryptographically secure pseudo random generator. In this it is much
	like FX::fxrandom(), except that instead of a 2^32 period this one has a
	mathematically proven 2^216091 period and is considered a \b much higher quality
	generator. Also, unlike FX::Secure::PRandomness, this class doesn't require
	the OpenSSL library to be compiled in.

	Most use will by via FX::Maths::SysRandSrc which is a mutex protected static
	instance which can be used safely by multiple threads.

	More specifically, this class is an implementation of the Mersenne Twister.
	Previous to v0.88 of TnFOX, this was a generic 64 bit implementation as
	detailed at http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html. Since
	v0.88, a SIMD-based improved version has been added (the old version is still
	used instead on big-endian machines or x86 with no SSE).

	You should be aware that each instance consumes about 2.5Kb of internal state
	and thus shouldn't be copied around nor constructed & destructed repeatedly.
	The old pre-v0.88 version could generate around 1Gb/sec of randomness on a
	3.0Ghz Core 2 processor. The new SIMD version can more than double that,
	even more so again when compiled with Intel's C++ compiler.

	\code
	\endcode
	*/
#if FOX_BIGENDIAN || (defined(_M_IX86) && _M_IX86_FP==0) || (defined(__i386__) && !defined(__SSE__))
	/*
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
	*/
	class FRandomness
	{
		static const FXint NN=312;
		static const FXint MM=156;
		static const FXulong MATRIX_A=0xB5026F5AA96619E9ULL;
		static const FXulong UM=0xFFFFFFFF80000000ULL;	/* Most significant 33 bits */
		static const FXulong LM=0x7FFFFFFFULL;			/* Least significant 31 bits */

		FXulong mt[NN];									/* The array for the state vector */
		int mti;
	public:
		//! Indicates if implemented using SIMD
		static const bool usingSIMD=false;
		//! Constructs, using seed \em seed
		FRandomness(FXulong seed) throw() : mti(NN+1)
		{
			mt[0] = seed;
			for (mti=1; mti<NN; mti++)
				mt[mti] =  (6364136223846793005ULL * (mt[mti-1] ^ (mt[mti-1] >> 62)) + mti);
		}

		FRandomness(FXuchar *_seed, FXuval len) throw() : mti(NN+1)
		{
			FXulong *init_key=(FXulong *) _seed, key_length=len;
			unsigned long long i, j, k;
			mt[0] = 19650218ULL;
			for (mti=1; mti<NN; mti++)
				mt[mti] =  (6364136223846793005ULL * (mt[mti-1] ^ (mt[mti-1] >> 62)) + mti);
			i=1; j=0;
			k = (NN>key_length ? NN : key_length);
			for (; k; k--) {
				mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 62)) * 3935559000370003845ULL))
					+ init_key[j] + j; /* non linear */
				i++; j++;
				if (i>=NN) { mt[0] = mt[NN-1]; i=1; }
				if (j>=key_length) j=0;
			}
			for (k=NN-1; k; k--) {
				mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 62)) * 2862933555777941757ULL))
					- i; /* non linear */
				i++;
				if (i>=NN) { mt[0] = mt[NN-1]; i=1; }
			}

			mt[0] = 1ULL << 63; /* MSB is 1; assuring non-zero initial array */
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

		//! Generates lots of random data (make sure it's 16 byte aligned!)
		void fill(FXuchar *d, FXuval len) throw()
		{
			for(FXuint n=0; n<(FXuint)len/8; n++)
				((FXulong *)d)[n]=int64();
		}

		//! generates a random number between 0.0 and 1.0
		double real1() throw()
		{
			return (int64() >> 11) * (1.0/9007199254740991.0);
		}

		//! generates a random number between 0.0 and 0.99999999999999988897769753748435
		double real2() throw()
		{
			return (int64() >> 11) * (1.0/9007199254740992.0);
		}

		//! generates a random number between 1.1102230246251565404236316680908e-16 and 0.99999999999999988897769753748435
		double real3() throw()
		{
			return ((int64() >> 12) + 0.5) * (1.0/4503599627370496.0);
		}
	};
#else
	class FRandomness
	{
		static const int MEXP=19937;
		static const int N=(MEXP / 128 + 1);	// = 156
		typedef Vector<FXuint, 4> w128_t;
		// From SFMT=params19937.h
		static const int N32=(N * 4);
		static const int POS1=122, SL1=18, SR1=11, SL2=1, SR2=1;
		static const FXuint *MASK() throw() { static const FXMEMALIGNED(16) FXuint d[4]={0xdfffffefU, 0xddfecb7fU, 0xbffaffffU, 0xbffffff6U}; return d; }
		static const FXuint *PARITY() throw() { static const FXMEMALIGNED(16) FXuint d[4]={0x00000001U, 0x00000000U, 0x00000000U, 0x13c9e684U}; return d; }

		w128_t sfmt[N];
		FXuval idx;
		FXuint *psfmt32;
		FXulong *psfmt64;

		inline w128_t do_recursion(const w128_t &a, const w128_t &b, const w128_t &c, const w128_t &d) throw()
		{
			w128_t z(rshiftvec(c, SR2*8));
			z^=a;
			z^=d<<w128_t(SL1);
			z^=lshiftvec(a, SL2*8);
			w128_t y(b>>w128_t(SR1));
			y&=w128_t(MASK());
			z^=y;
			return z;
		}
		inline void gen_rand_all() throw()
		{
			int i;
			w128_t r, r1, r2;

			r1 = sfmt[N - 2];
			r2 = sfmt[N - 1];
			for (i = 0; i < N - POS1; i++)
			{
				sfmt[i] = r = do_recursion(sfmt[i], sfmt[i + POS1], r1, r2);
				r1 = r2;
				r2 = r;
			}
			for (; i < N; i++)
			{
				sfmt[i] = r = do_recursion(sfmt[i], sfmt[i + POS1 - N], r1, r2);
				r1 = r2;
				r2 = r;
			}
		}
		inline void gen_rand_array(w128_t *array, FXuint size) throw()
		{
			FXuint i, j;
			w128_t r, r1, r2;

			r1 = sfmt[N - 2];
			r2 = sfmt[N - 1];
			for (i = 0; i < N - POS1; i++)
			{
				array[i] = r = do_recursion(sfmt[i], sfmt[i + POS1], r1, r2);
				r1 = r2;
				r2 = r;
			}
			for (; i < (FXuint) N; i++)
			{
				array[i] = r = do_recursion(sfmt[i], array[i + POS1 - N], r1, r2);
				r1 = r2;
				r2 = r;
			}
			/* main loop */
			for (; i < size - N; i++)
			{
				array[i] = r = do_recursion(array[i - N], array[i + POS1 - N], r1, r2);
				r1 = r2;
				r2 = r;
			}
			int limit=2*N-size;
			for(j = 0; (int) j < limit; j++)
			{
				sfmt[j]=array[j + size - N];
			}
			for (; i < size; i++, j++)
			{
				sfmt[j] = array[i] = r = do_recursion(array[i - N], array[i + POS1 - N], r1, r2);
				r1 = r2;
				r2 = r;
			}
		}
		void period_certification() throw()
		{
			int inner = 0;
			int i, j;
			FXuint work;
			static const w128_t parity(PARITY());

			for (i = 0; i < 4; i++)
				inner ^= psfmt32[i] & parity[i];
			for (i = 16; i > 0; i >>= 1)
				inner ^= inner >> i;
			inner &= 1;
			/* check OK */
			if (inner == 1) {
				return;
			}
			/* check NG, and modification */
			for (i = 0; i < 4; i++) {
				work = 1;
				for (j = 0; j < 32; j++) {
					if ((work & parity[i]) != 0) {
						psfmt32[i] ^= work;
						return;
					}
					work = work << 1;
				}
			}
		}
		inline FXuint func1(FXuint x) throw() {
			return (x ^ (x >> 27)) * (FXuint)1664525UL;
		}
		inline FXuint func2(FXuint x) throw() {
			return (x ^ (x >> 27)) * (FXuint)1566083941UL;
		}

		void init_by_array(FXuint *init_key, FXuint key_length) throw()
		{
			FXuint i, j, count;
			FXuint r;
			int lag;
			int mid;
			int size = N * 4;

			if (size >= 623) {
				lag = 11;
			} else if (size >= 68) {
				lag = 7;
			} else if (size >= 39) {
				lag = 5;
			} else {
				lag = 3;
			}
			mid = (size - lag) / 2;

			memset(sfmt, 0x8b, sizeof(sfmt));
			if (key_length + 1 > (FXuint) N32) {
				count = key_length + 1;
			} else {
				count = N32;
			}
			r = func1(psfmt32[0] ^ psfmt32[mid] ^ psfmt32[N32 - 1]);
			psfmt32[mid] += r;
			r += key_length;
			psfmt32[mid + lag] += r;
			psfmt32[0] = r;

			count--;
			for (i = 1, j = 0; (j < count) && (j < key_length); j++) {
				r = func1(psfmt32[i] ^ psfmt32[(i + mid) % N32] ^ psfmt32[(i + N32 - 1) % N32]);
				psfmt32[(i + mid) % N32] += r;
				r += init_key[j] + i;
				psfmt32[(i + mid + lag) % N32] += r;
				psfmt32[i] = r;
				i = (i + 1) % N32;
			}
			for (; j < count; j++) {
				r = func1(psfmt32[i] ^ psfmt32[(i + mid) % N32] ^ psfmt32[(i + N32 - 1) % N32]);
				psfmt32[(i + mid) % N32] += r;
				r += i;
				psfmt32[(i + mid + lag) % N32] += r;
				psfmt32[i] = r;
				i = (i + 1) % N32;
			}
			for (j = 0; j < (FXuint) N32; j++) {
				r = func2(psfmt32[i] + psfmt32[(i + mid) % N32] + psfmt32[(i + N32 - 1) % N32]);
				psfmt32[(i + mid) % N32] ^= r;
				r -= i;
				psfmt32[(i + mid + lag) % N32] ^= r;
				psfmt32[i] = r;
				i = (i + 1) % N32;
			}

			idx = N32;
			period_certification();
		}
	public:
		//! Indicates if implemented using SIMD
		static const bool usingSIMD=true;
		//! Constructs, using seed \em seed
		FRandomness(FXulong seed) throw() : idx(0), psfmt32((FXuint *)&sfmt), psfmt64((FXulong *)&sfmt)
		{
			init_by_array((FXuint*)&seed, 2);
			/*int i;

			psfmt32[0] = (FXuint) seed;
			for (i = 1; i < N32; i++) {
				psfmt32[i] = 1812433253UL * (psfmt32[i - 1] ^ (psfmt32[i - 1] >> 30)) + i;
			}
			idx = N32;
			period_certification();*/
		}
		//! Constructs, using seed \em seed
		FRandomness(FXuchar *seed, FXuval len) throw() : idx(0), psfmt32((FXuint *)&sfmt), psfmt64((FXulong *)&sfmt)
		{
			init_by_array((FXuint*)seed, (FXuint)(len/4));
		}

		//! Generates a random number on [0, 2^64-1]-interval
		FXulong int64() throw()
		{
			FXulong r;
			assert(idx % 2 == 0);

			if (idx >= (FXuint) N32)
			{
				gen_rand_all();
				idx = 0;
			}
			r = psfmt64[idx / 2];
			idx += 2;
			return r;
		}

		//! Generates lots of random data (make sure it's 16 byte aligned!)
		void fill(FXuchar *d, FXuval len) throw()
		{
			gen_rand_array((w128_t *) d, (FXuint)(len/sizeof(w128_t)));
		}

		//! generates a random number between 0.0 and 1.0
		double real1() throw()
		{
			return (int64() >> 11) * (1.0/9007199254740991.0);
		}

		//! generates a random number between 0.0 and 0.99999999999999988897769753748435
		double real2() throw()
		{
			return (int64() >> 11) * (1.0/9007199254740992.0);
		}

		//! generates a random number between 1.1102230246251565404236316680908e-16 and 0.99999999999999988897769753748435
		double real3() throw()
		{
			return ((int64() >> 12) + 0.5) * (1.0/4503599627370496.0);
		}
	};
#endif

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
		return stddevs*y*::sqrt(-2.0*log(r2)/r2);
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
		return stddevs*y*::sqrt(-2.0*log(r2)/r2);
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
	template<typename type, class allocator> inline type mean(const QMemArray<type, allocator> &array, FXuint stride=1, type *FXRESTRICT min=0, type *FXRESTRICT max=0, type *FXRESTRICT mode=0) throw()
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
	template<typename type, class allocator> inline type variance(const QMemArray<type, allocator> &array, FXuint stride=1, const type *FXRESTRICT _mean=0) throw()
	{
		return variance(array.data(), array.count(), stride, _mean);
	}

	//! Computes the standard deviation of an array
	template<typename type> inline type stddev(const type *FXRESTRICT array, FXuval len, FXuint stride=1, const type *FXRESTRICT _mean=0) throw()
	{
		return sqrt(variance(array, len, stride, _mean));
	}
	template<typename type, class allocator> inline type stddev(const QMemArray<type, allocator> &array, FXuint stride=1, const type *FXRESTRICT _mean=0) throw()
	{
		return stddev(array.data(), array.count(), stride, _mean);
	}

	//! Computes distribution of an array. Returns min, max & bucket size in first three items.
	template<unsigned int buckets, typename type> inline Vector<type, buckets+3> distribution(const type *FXRESTRICT array, FXuval len, FXuint stride=1, const type *FXRESTRICT min=0, const type *FXRESTRICT max=0) throw()
	{
		Vector<type, buckets+3> ret;
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
	template<unsigned int buckets, typename type, class allocator> inline Vector<type, buckets+3> distribution(const QMemArray<type, allocator> &array, FXuint stride=1, const type *FXRESTRICT min=0, const type *FXRESTRICT max=0) throw()
	{
		return distribution<type, buckets>(array.data(), array.count(), stride, min, max);
	}

} // namespace

} // namespace

#endif
