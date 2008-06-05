/********************************************************************************
*                                                                               *
*                                 Tools for maths                               *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2006-2008 by Niall Douglas.   All Rights Reserved.       *
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
		// GCC's alignment support on x86 and x64 is nearly useless :(
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
					iszero&=!a;
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
		};
#undef VECTOR1OP
#undef VECTOR2OP
#undef VECTORP2OP
#undef VECTORFUNC
#undef VECTOR2FUNC


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
	correct alignment.

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
	sqrt(), rcp(), rsqrt(), min(), max(), sum(), dot()</tt>.
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

	For integers on SSE only, multiplication, division, modulus are emulated (slowly)
	as they don't have corresponding SSE instructions available.
	
	See FX::Maths::VectorArray for an array of vectors letting you easily implement
	a matrix. See also the FXVECTOROFVECTORS macro for how to declare to the compiler
	when a vector should be implemented as a sequence of other vectors (this is how
	the SSE specialisations overload specialisations for two power increments) - if
	you want a non-two power size, you'll need to declare the VectorOfVectors
	specialisation manually.
	*/
	template<typename type, unsigned int A> class Vector : public Impl::VectorBase<type, A, Vector<type, A>, Generic::Traits<type>::isArithmetical, Generic::Traits<type>::isInt>
	{
		typedef Impl::VectorBase<type, A, Vector<type, A>, Generic::Traits<type>::isArithmetical, Generic::Traits<type>::isInt> Base;
	public:
		Vector() { }
		//! Use \em d =0 to initialise to zero
		explicit Vector(const type *d) : Base(d) { }
		//! Initialises all members to a certain value
		explicit Vector(const type &d) : Base(d) { }
	};
//! Specialises a FX::Maths::Vector to be implemented as another vector
#define FXVECTOROFVECTORS(VECTORTYPE, ELEMENTS) template<> class Vector<VECTORTYPE::TYPE, ELEMENTS> : public Impl::VectorOfVectors<VECTORTYPE, ELEMENTS/VECTORTYPE::DIMENSION, Vector<VECTORTYPE::TYPE, ELEMENTS> > \
	{ \
		typedef Impl::VectorOfVectors<VECTORTYPE, ELEMENTS/VECTORTYPE::DIMENSION, Vector<VECTORTYPE::TYPE, ELEMENTS> > Base; \
	public: \
		Vector() { } \
		explicit Vector(const VECTORTYPE::TYPE *d) : Base(d) { } \
		explicit Vector(const VECTORTYPE::TYPE &d) : Base(d) { } \
	};
#if 1	// Use to disable SIMD optimised versions
#if (defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))) || (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))
	// The x86 and x64 SSE specialisations
#if defined(_M_X64) || defined(__x86_64__) || (defined(_M_IX86) && _M_IX86_FP>=1) || (defined(__i386__) && defined(__SSE__))
	template<> class Vector<float, 4> : private Impl::TwoPowerMemAligner<16>
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
				t=_mm_shuffle_ps(v, v, 0); break;
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
			return _mm_movemask_ps(a.v)||_mm_movemask_ps(_mm_sub_ps(_mm_setzero_ps(), a.v));
		}
		friend TYPE sum(const Vector &a)
		{
#if _M_IX86_FP>=3 || defined(__SSE3__)
			return int_extract(_mm_hadd_ps(_mm_hadd_ps(a.v, _mm_setzero_ps())));
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
			return int_extract(_mm_dp_ps(a.v, b.v, 0xf1));
#else
			// SSE implementation
			return sum(a*b);
#endif
		}
	};
	typedef Vector<float, 4> int_SSEOptimised_float4;	// Needed as macros don't understand template types :(
	FXVECTOROFVECTORS(int_SSEOptimised_float4, 8);
	FXVECTOROFVECTORS(int_SSEOptimised_float4, 16);
	FXVECTOROFVECTORS(int_SSEOptimised_float4, 32);
	FXVECTOROFVECTORS(int_SSEOptimised_float4, 64);
	FXVECTOROFVECTORS(int_SSEOptimised_float4, 128);
	FXVECTOROFVECTORS(int_SSEOptimised_float4, 256);
#endif
#if defined(_M_X64) || defined(__x86_64__) || (defined(_M_IX86) && _M_IX86_FP>=2) || (defined(__i386__) && defined(__SSE2__))
	template<> class Vector<double, 2> : private Impl::TwoPowerMemAligner<16>
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
				t=_mm_shuffle_pd(v, v, 0); break;
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
			return _mm_movemask_pd(a.v)||_mm_movemask_pd(_mm_sub_pd(_mm_setzero_pd(), a.v));
		}
		friend TYPE sum(const Vector &a)
		{
#if _M_IX86_FP>=3 || defined(__SSE3__)
			return int_extract(_mm_hadd_pd(_mm_hadd_pd(a.v, _mm_setzero_pd())));
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
			return int_extract(_mm_dp_pd(a.v, b.v, 0xf1));
#else
			// SSE implementation
			return sum(a*b);
#endif
		}
	};
	typedef Vector<double, 2> int_SSEOptimised_double2;	// Needed as macros don't understand template types :(
	FXVECTOROFVECTORS(int_SSEOptimised_double2, 4);
	FXVECTOROFVECTORS(int_SSEOptimised_double2, 8);
	FXVECTOROFVECTORS(int_SSEOptimised_double2, 16);
	FXVECTOROFVECTORS(int_SSEOptimised_double2, 32);
	FXVECTOROFVECTORS(int_SSEOptimised_double2, 64);
	FXVECTOROFVECTORS(int_SSEOptimised_double2, 128);


	// Now for the integer vectors, we can save ourselves some work by reusing Impl::VectorBase
#define VECTOR2OP_A(op, sseop, sseending)  Vector operator op (const Vector &o) const { return _mm_ ##sseop## _ ##sseending (v, o.v); }
#define VECTORP2OP_A(op, sseop, sseending) Vector &operator op (const Vector &o)      { v=_mm_ ##sseop## _ ##sseending (v, o.v); return *this; }
#define VECTOR2OP_B(op, sseop, sseending)  Vector operator op (const Vector &o) const { return _mm_ ##sseop## _si128(v, o.v); }
#define VECTORP2OP_B(op, sseop, sseending) Vector &operator op (const Vector &o)      { v=_mm_ ##sseop## _si128(v, o.v); return *this; }
#define VECTOR2OP_C(op, sseop, sseending)  Vector operator op (const Vector &o) const { return _mm_and_si128(int_ones(), _mm_cmp ##sseop## _ ##sseending (v, o.v)); }
#define VECTOR2OP_D(op, sseop, sseending)  Vector operator op (const Vector &o) const { return _mm_andnot_si128(_mm_cmp ##sseop## _ ##sseending (v, o.v), int_ones()); }
#define VECTOR2OP_E(op, sseop, sseending)  Vector operator op (const Vector &o) const { return _mm_xor_si128(int_ones(), _mm_cmpeq_ ##sseending (_mm_setzero_si128(), _mm_ ##sseop## _si128(v, o.v))); }
#define VECTORINTEGER(vint, vsize, sseending) \
	template<> class Vector<vint, vsize> : public Impl::VectorBase<vint, vsize, Vector<vint, vsize>, true, true, __m128i> \
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
		 VECTOR2OP_A(+,  add, sseending)  VECTOR2OP_A(-,  sub, sseending)  VECTOR2OP_A(<<,  sll, sseending)  VECTOR2OP_A(>>,  sra, sseending) \
		VECTORP2OP_A(+=, add, sseending) VECTORP2OP_A(-=, sub, sseending) VECTORP2OP_A(<<=, sll, sseending) VECTORP2OP_A(>>=, sra, sseending) \
		    \
		 VECTOR2OP_B( & ,  and, sseending)   VECTOR2OP_B( | ,  or, sseending)   VECTOR2OP_B( ^ ,  xor, sseending) \
		VECTORP2OP_B( &= , and, sseending)  VECTORP2OP_B( |= , or, sseending)  VECTORP2OP_B( ^= , xor, sseending)  \
 \
		VECTOR2OP_C( == , eq, sseending)  VECTOR2OP_C( < , lt, sseending)  VECTOR2OP_C( > , gt, sseending)   \
		VECTOR2OP_D( != , eq, sseending)  VECTOR2OP_D( <= , gt, sseending) VECTOR2OP_D( >= , lt, sseending) \
		VECTOR2OP_E( && , and, sseending) VECTOR2OP_E( || , or, sseending) \
 \
		friend bool isZero(const Vector &a) \
		{	/* We can use a cunning trick here */ \
			return 65535==_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_setzero_si128(), a.v)); \
		} \
	};
//VECTORINTEGER(FXuchar, 16, epi8)
//VECTORINTEGER(FXchar,  16, epi8)
VECTORINTEGER(FXushort, 8, epi16)
VECTORINTEGER(FXshort,  8, epi16)
VECTORINTEGER(FXuint,   4, epi32)
VECTORINTEGER(FXint,    4, epi32)
//VECTORINTEGER(FXulong,  2, epi64)
//VECTORINTEGER(FXlong,   2, epi64)
#undef VECTOR2OP_A
#undef VECTORP2OP_A
#undef VECTOR2OP_B
#undef VECTORP2OP_B
#undef VECTOR2OP_C
#undef VECTOR2OP_D
#undef VECTOR2OP_E
#undef VECTORINTEGER
#define VECTORINTEGER(vint, vsize) \
	typedef Vector<vint, vsize> int_SSEOptimised_##vint; \
	FXVECTOROFVECTORS(int_SSEOptimised_##vint, vsize*2); \
	FXVECTOROFVECTORS(int_SSEOptimised_##vint, vsize*4); \
	FXVECTOROFVECTORS(int_SSEOptimised_##vint, vsize*8); \
	FXVECTOROFVECTORS(int_SSEOptimised_##vint, vsize*16); \
	FXVECTOROFVECTORS(int_SSEOptimised_##vint, vsize*32); \
	FXVECTOROFVECTORS(int_SSEOptimised_##vint, vsize*64);
VECTORINTEGER(FXushort, 8)
VECTORINTEGER(FXshort,  8)
VECTORINTEGER(FXuint,   4)
VECTORINTEGER(FXint,    4)
#undef VECTORINTEGER
#endif
#endif
#endif
	/*! \class VectoryArray
	\brief A static array of vectors
	*/
	template<typename type, unsigned int A, unsigned int B> class VectorArray
	{
	protected:
		Vector<type, A> data[B];
	public:
		//! The base type
		typedef type BASETYPE;
		//! The base dimension
		static const unsigned int BASEDIMENSION=A;
		//! The container type
		typedef Vector<type, A> VECTORTYPE;
		//! The base dimension
		static const unsigned int VECTORDIMENSION=B;
		VectorArray() { }
		explicit VectorArray(const type **d) { for(FXuint b=0; b<B; b++) data[b]=TYPE(d ? d[b] : 0); }
		Vector<type, A> &operator[](unsigned int i) { assert(i<B); return data[i]; }
		const Vector<type, A> &operator[](unsigned int i) const { assert(i<B); return data[i]; }
	};

	namespace FRandomnessPrivate
	{
	}
	/*! \class FRandomness
	\brief A fast quality source of pseudo entropy

	Unlike FX::Secure::PRandomness, this class provides a much faster but
	not cryptographically secure pseudo random generator. In this it is much
	like FX::fxrandom(), except that instead of a 2^32 period this one has a
	mathematically proven 2^216091 period and is considered a much higher quality
	generator. Also, unlike FX::Secure::PRandomness, this class doesn't require
	the OpenSSL library to be compiled in.

	Most use will by via FX::Maths::SysRandSrc which is a mutex protected static
	instance which can be used safely by multiple threads.

	More specifically, this class is an implementation of the Mersenne Twister.
	Previous to v0.88 of TnFOX, this was a generic 64 bit implementation as
	detailed at http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html. Since
	v0.88, a SIMD-based improved version has been added (you can reenable
	the old version by modifying an \c #ifdef in FXMaths.h).

	You should be aware that each instance consumes about 2.5Kb of internal state
	and thus shouldn't be copied around nor constructed & destructed repeatedly.

	\code
	\endcode
	*/
#if 1
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
#else
	class FRandomness
	{
		w128_t sfmt[N];
	public:
		//! Constructs, using seed \em seed
		FRandomness(FXulong seed) throw() : mti(NN+1)
		{
		}

        //! Generates a random number on [0, 2^64-1]-interval
		FXulong int64() throw()
		{
			FXulong r;
			r = psfmt64[idx / 2];
			idx += 2;
			return r;
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
	template<unsigned int buckets, typename type> inline Vector<type, buckets+3> distribution(const QMemArray<type> &array, FXuint stride=1, const type *FXRESTRICT min=0, const type *FXRESTRICT max=0) throw()
	{
		return distribution<type, buckets>(array.data(), array.count(), stride, min, max);
	}

} // namespace

} // namespace

#endif
