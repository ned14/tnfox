/********************************************************************************
*                                                                               *
*                        Tools for generic programming                          *
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

#ifndef FXGENERICTOOLS_H
#define FXGENERICTOOLS_H

#include "FXString.h"
#include "FXPolicies.h"
#include "FXException.h"
#include "FXPath.h"
#include <typeinfo>
#include <assert.h>

namespace FX {

/*! \file FXGenericTools.h
\brief Defines a number of tools useful for generic programming
*/


namespace Generic {
/*! \namespace Generic
\ingroup generic
\brief Defines a set of generic tools

Within the FX::Generic namespace are a collection of compile-time programming
tools similar to those provided by the <a href="http://www.boost.org/">Boost</a>
library or <a href="http://sourceforge.net/projects/loki-lib/">Loki</a> libraries.
However what is provided here is very much a subset of either of those two libraries.

If you want a full explanation of how this works and how to use it, please consult
"Modern C++ Design" by Andrei Alexandrescu. These facilities follow that book somewhat
but have also incorporated ideas from Boost and other places.
*/

// MSVC doesn't define type_info within namespace std :(
#ifdef _MSC_VER
typedef ::type_info type_info;
#else
typedef std::type_info type_info;
#endif

template<bool> struct StaticAssert;
template<> struct StaticAssert<true>
{
	StaticAssert(int) { }
};
/*! \ingroup generic
A macro which provides compile-time assertion tests, causing the compiler to issue
a useful message \em msg if the assertion fails. The message must be a legal C++ identifier
though it can start with a number. You also must place it somewhere where code can
be compiled as it relies on a scope to remove a temporarily created variable (which
is optimised away on release builds).
*/
#define FXSTATIC_ASSERT(expr, msg) \
{ \
	FX::Generic::StaticAssert<(expr)!=0> ERROR_##msg(0); \
}

template<typename foo> struct StaticError;

/*! \struct NullType
\ingroup generic
\brief A type representing no type (ie; end of list etc)
*/
struct NullType { };
/*! \struct IntToType
\ingroup generic
\brief A small template which maps any \c int to a type (and thus allows number-based
overloading and type specialisation - and thus compile-time determined code
generation).
*/
template<int n> struct IntToType { enum { value=n }; };
/*! \struct TypeToType
\ingroup generic
\brief A small template which maps any type to a lightweight type suitable for
overloading eg; functions
*/
template<typename type> struct TypeToType { typedef type value; };

/*! \struct select
\ingroup generic
\brief Selects one of two types based upon a boolean
\code
FX::Generic::select<defined(SOME_DEFINE), int, double>::value
\endcode
If SOME_DEFINE is defined, int is chosen. Otherwise double.
*/
template<bool v, typename A, typename B> struct select
{
	typedef A value;
};
template<typename A, typename B> struct select<false, A, B>
{
	typedef B value;
};
/*! \struct sameType
\ingroup generic
\brief Returns true if the two types are the same
*/
template<typename A, typename B> struct sameType
{
	static const bool value=false;
};
template<typename T> struct sameType<T, T>
{
	static const bool value=true;
};
/*! \struct addRef
\ingroup generic
\brief Adds a reference indirection unless type already has one
*/
template<typename type> struct addRef { typedef type &value; };
template<typename type> struct addRef<type &> { typedef type value; };
template<> struct addRef<void> { typedef void value; };
/*! \struct addConstRef
\ingroup generic
\brief Ensures a type is a const reference
*/
template<typename type> struct addConstRef { typedef const type &value; };
template<typename type> struct addConstRef<type &> { typedef typename addConstRef<type>::value value; };
template<typename type> struct addConstRef<const type> { typedef typename addConstRef<type>::value value; };
template<typename type> struct addConstRef<const type &> { typedef const type &value; };
template<> struct addConstRef<void> { typedef void value; };

namespace convertiblePrivate {
	struct TwoChar { char foo[2]; };
	template<typename to, typename from> struct impl
	{
		static from makeFrom();
		static TwoChar test(...);
		static    char test(to);
	};
}
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244)  // possible loss of data
#pragma warning(disable: 4800)  // forcing value to bool true or false
#endif
/*! \struct convertible
\ingroup generic
\brief Returns true if \em from can become \em to
*/
template<typename to, typename from> struct convertible
{
private:
	typedef typename addConstRef<to>::value refTo;
	typedef typename addConstRef<from>::value refFrom;
	typedef convertiblePrivate::impl<refTo, refFrom> impl;
public:
	static const bool value=sizeof(impl::test(impl::makeFrom()))==sizeof(char);
};
template<typename T> struct convertible<void, T> { static const bool value=false; };
template<typename T> struct convertible<T, void> { static const bool value=false; };
template<> struct convertible<void, void> { static const bool value=true; };
#ifdef _MSC_VER
#pragma warning(pop)
#endif
/*! \struct lessIndir
\ingroup generic
\brief Removes if possible a level of indirection from a pointer or reference type

Example:
\li FX::Generic::lessIndir<double **>::value is double *
\li FX::Generic::lessIndir<int *>::value is int
\li FX::Generic::lessIndir<char>::value is char, and FX::Generic::lessIndir<char>::failed is true
*/
template<typename ptr> struct lessIndir
{
	typedef ptr value;
	static const bool failed=true;
};
template<typename ptr> struct lessIndir<ptr *>
{
	typedef ptr value;
	static const bool failed=false;
};
template<typename ptr> struct lessIndir<const ptr *>
{
	typedef ptr value;
	static const bool failed=false;
};
template<typename ptr> struct lessIndir<volatile ptr *>
{
	typedef ptr value;
	static const bool failed=false;
};
template<typename ptr> struct lessIndir<const volatile ptr *>
{
	typedef ptr value;
	static const bool failed=false;
};
template<typename ptr> struct lessIndir<ptr &>
{
	typedef ptr value;
	static const bool failed=false;
};
template<typename ptr> struct lessIndir<const ptr &>
{
	typedef ptr value;
	static const bool failed=false;
};
template<typename ptr> struct lessIndir<volatile ptr &>
{
	typedef ptr value;
	static const bool failed=false;
};
template<typename ptr> struct lessIndir<const volatile ptr &>
{
	typedef ptr value;
	static const bool failed=false;
};
/*! \struct indirs
\ingroup generic
\brief Returns how many levels of indirection this type has eg; int ** is two.
*/
template<typename type> struct indirs { static const int value=0; };
template<typename type> struct indirs<type &> { static const int value=1+indirs<type>::value; };
template<typename type> struct indirs<const type &> { static const int value=1+indirs<type>::value; };
template<typename type> struct indirs<volatile type &> { static const int value=1+indirs<type>::value; };
template<typename type> struct indirs<const volatile type &> { static const int value=1+indirs<type>::value; };
template<typename type> struct indirs<type *> { static const int value=1+indirs<type>::value; };
template<typename type> struct indirs<const type *> { static const int value=1+indirs<type>::value; };
template<typename type> struct indirs<volatile type *> { static const int value=1+indirs<type>::value; };
template<typename type> struct indirs<const volatile type *> { static const int value=1+indirs<type>::value; };
/*! \struct leastIndir
\ingroup generic
\brief Removes all levels of indirection from a pointer or reference type
*/
template<typename ptr> struct leastIndir { typedef ptr value; };
template<typename ptr> struct leastIndir<ptr *> { typedef typename leastIndir<ptr>::value value; };
template<typename ptr> struct leastIndir<const ptr *> { typedef typename leastIndir<ptr>::value value; };
template<typename ptr> struct leastIndir<volatile ptr *> { typedef typename leastIndir<ptr>::value value; };
template<typename ptr> struct leastIndir<const volatile ptr *> { typedef typename leastIndir<ptr>::value value; };
template<typename ptr> struct leastIndir<ptr &> { typedef typename leastIndir<ptr>::value value; };
template<typename ptr> struct leastIndir<const ptr &> { typedef typename leastIndir<ptr>::value value; };
template<typename ptr> struct leastIndir<volatile ptr &> { typedef typename leastIndir<ptr>::value value; };
template<typename ptr> struct leastIndir<const volatile ptr &> { typedef typename leastIndir<ptr>::value value; };

/*! \class typeInfoBase
\brief Base class for FX::Generic::typeInfo
*/
class typeInfoBase
{
	FXString decorated, readable;
public:
	typeInfoBase() { }
	//! Constructs an instance of information about \em ti
	typeInfoBase(const type_info &ti) :
#ifdef _MSC_VER
		decorated(ti.raw_name()), readable(ti.name())
#elif defined(__GNUC__) || defined(__DMC__)
		decorated(ti.name()), readable(fxdemanglesymbol(ti.name()))
#else
#error Method of demangling RTTI unknown
#endif
		{ }
	//! Returns true if empty
	bool operator!() const throw() { return !!decorated.empty(); }
	//! Returns true if identical to the other
	bool operator==(const typeInfoBase &o) const { return (decorated==o.decorated)!=0; }
	//! Returns true if not the same
	bool operator!=(const typeInfoBase &o) const { return (decorated!=o.decorated)!=0; }
	//! Returns the human readable name
	const FXString &name() const { return readable; }
	//! Returns the mangled name
	const FXString &mangled() const { return decorated; }
	//! Returns the name but suitable for use as an identifier (no <>'s)
	FXString asIdentifier() const
	{
		FXString ret(readable);
		ret.substitute('<', '_'); ret.substitute('>', '_'); ret.substitute(',', '_'); ret.substitute(':', '_');
		return ret;
	}
	//! Returns the leaf name suitable for use as an identifier
	FXString asLeafIdentifier() const
	{
		FXint rpos=readable.rfind(':'); if(-1==rpos) rpos=readable.rfind(' ');
		FXString ret(readable.mid(rpos+1));
		return ret;
	}
	// These defined in fxutils.cpp
	friend FXAPI FXStream &operator<<(FXStream &s, const typeInfoBase &i);
	friend FXAPI FXStream &operator>>(FXStream &s, typeInfoBase &i);
};
/*! \struct typeInfo
\ingroup generic
\brief Enhanced version of std::type_info

The C++ \c std::type_info has issues on GCC when working across shared
objects so you won't want to use that one, use this one instead. It
furthermore provides useful conversions into suitable type-unique
strings. Note that passing a type reference may not show the reference
on some platforms - wrap as a parameter to a template to see these.

Obviously this requires an implementation of \c typeid() and \c std::type_info
which return meaningful values for \c raw_name() and \c name(). As the C++
standard does not require it, this may not be the case.
\code
Generic::typeInfoBase &typeinfo=Generic::typeInfo<type>();
\endcode
*/
template<typename type> class typeInfo : public typeInfoBase
{
public:
	typeInfo(const type_info &ti=typeid(type)) : typeInfoBase(ti) { }
};
/*! \class ptr
\ingroup generic
\brief A policy-based smart pointer

I was very much taken by Alexandrescu's policy-based smart pointer and though
the one I offer here is considerably reduced in scope, it can be easily
expanded into the future - and almost certainly shall be.

Until then, you have the following policies available:
\li FX::Pol::deepCopy (requires a member function \c copy() to be available).
\li FX::Pol::refCounted whereby if the reference count should reach zero,
the pointee is deleted (requires a function \c refCount() to return a reference
to some \c int like entity eg; FX::FXAtomicInt). Deletion of the smart pointer
decrements the pointee's reference count.
\li FX::Pol::destructiveCopy which destroys the source smart pointer during
copies.
\li FX::Pol::noCopy which does nothing, thus making the smart pointer behave
like a pointer holder (ie; FX::FXPtrHold).

For all these except where specified, deleting the smart pointer deletes its
pointee.

You'll also want to know about the helper functions:
\li PtrPtr(ptr), dereferences the smart pointer. This means you are about
to access the pointee. If you aren't about to access the pointee, use PtrRef().
\li PtrRef(ptr), returns a reference to the pointer to the pointee unless you
pass a const smart pointer, in which case it returns a const pointer.
\li PtrRelease(ptr), releases the pointer (sets it to zero, returning former
contents)
\li PtrReset(ptr, newptr), is equivalent to setting the smart pointer using
\c operator=

These helper functions work via Koenig lookup and thus do not need
namespace qualification.
<h3>Usage:</h3>
Like Alexandrescu's smart pointer, this smart pointer is not entirely a pointer
replacement. It enables smart pointer semantics which aren't the same as
pointer semantics. Smart pointer semantics enabled by the policies such as
listed above are defeated if you have direct access to the held pointer so
no implicit conversions are provided nor the address-of operator. However,
you can test the smart pointer for equality against other pointers including
other smart pointers and implicit conversion to base classes is correctly
handled.

If you are used to using smart pointers to enable exception-safety, TnFOX
isn't built around that - I personally find it easy to get it wrong when
using smart pointers for that purpose as different policies enable quite
different smart pointer behaviour and well, to me they're all smart pointers!
I prefer to use a pointer holder (FX::FXPtrHold) or \link rollbacks
transaction rollbacks
\endlink
to enable exception-safety - they are much clearer in saying what they're
doing and if you forget them it's obvious when looking at the code. This IMHO
is better than the implicit behaviour of some reconfigurable abstract entity.

\sa FXAutoPtr, FX::FXPtrHold
*/
template<typename type,
	template<class> class ownershipPolicy=FX::Pol::destructiveCopy>
class ptr : public ownershipPolicy<type>
{
	typedef ownershipPolicy<type> op;
public:
	//! Constructs a smart pointer pointing to \em data
	ptr(type *data=0) : op(data) { }
	ptr(const ptr &o) : op(o) { }
	//! Resets the pointer to \em data
	ptr &operator=(type *data) { PtrReset(*this, data); return *this; }
	type &operator *() { assert(PtrPtr(*this)); return *PtrPtr(*this); }
	const type &operator *() const { assert(PtrPtr(*this)); return *PtrPtr(*this); }
	type *operator->()
	{
		type *ret=PtrPtr(*this);
		if(!ret)
		{
			assert(ret);
		}
		return ret;
	}
	const type *operator->() const
	{
		const type *ret=PtrPtr(*this);
		if(!ret)
		{
			assert(ret);
		}
		return ret;
	}
	//! Releases the pointer (sets it to zero, returns the former contents)
	friend type *PtrRelease(ptr &p)
	{
		type *ret=PtrRef(p);
		PtrRef(p)=0;
		return ret;
	}
	//! Resets the pointer (deletes the old contents, sets to new contents)
	friend void PtrReset(ptr &p, type *data)
	{
		p=ptr(data);
	}
	template<typename T> bool operator==(const ptr<T> &o) const { return PtrRef(*this)==PtrRef(o); }
	template<typename T> bool operator==(T *o) const { return PtrRef(*this)==o; }
	// Workaround to implement if(sp)
private:
	struct Tester
	{
	private:
		void operator delete(void *);
	};
public:
	operator Tester *() const
	{
		if(!*this) return 0;
		static Tester t;
		return &t;
	}

	// For if(!sp) 
	bool operator!() const { return PtrRef(*this)==0; }
	template<typename T> bool operator!=(const ptr<T> &o) const { return !(*this==o); }
	template<typename T> bool operator!=(T *o) const { return !(*this==o); }
};

/*! \defgroup TL Typelists
\ingroup generic
\brief Provides a list of types at compile-time

Typelists are the core fundamental base upon which all complex metaprogramming
rests. While their use isn't mandatory, a whole pile of things become way,
way easier if you have them. Much of the TnFOX generic tools library uses
typelists to implement itself.

There's not much to say except that like with all other code in the
generic tools library, ::value is always the value where applicable.
TL::item is an item in the list though TL::item is also the list itself
(due to the recursive nature of typelist definitions). The
method naming follows the same style at the QTL, including semantics.

Example:
\code
typedef TL::create<char, short, int>::value Ints;
template<typename type> Handler
{
	static const bool typeIsInt=TL::find<Ints, type>::value>=0 };
};
\endcode
A \b very powerful tool is FX::Generic::TL::instantiateH which couples with
the instance() function below.
*/
/*! \namespace TL
\ingroup TL
\brief Holds compile-time code implementing typelists
*/
namespace TL
{
	/*! \struct item
	\ingroup TL
	\brief A single item within the list
	*/
	template<typename A, typename B> struct item
	{
		//! The value of this item
		typedef A value;
		//! Either the next item in the list, or NullType
		typedef B next;
	};
	/*! \struct create
	\ingroup TL
	\brief Creates a typelist with up to 16 types
	\note Make sure you use the ::value member, not create<> itself
	*/
	template<typename T1 =NullType, typename T2 =NullType, typename T3 =NullType, typename T4 =NullType,
			 typename T5 =NullType, typename T6 =NullType, typename T7 =NullType, typename T8 =NullType,
			 typename T9 =NullType, typename T10=NullType, typename T11=NullType, typename T12=NullType,
			 typename T13=NullType, typename T14=NullType, typename T15=NullType, typename T16=NullType>
	struct create
	{
	private:
		typedef typename create<T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16>::value temp;
	public:
		typedef item<T1, temp> value;
	};
	template<> struct create<> { typedef NullType value; };
	/*! \struct length
	\ingroup TL
	\brief Determines the length of a typelist

	Example:
	\code
	TL::length(TL::create<int,double,float,char>::value)::value is 4
	\endcode
	*/
	template<class typelist> struct length
	{
		StaticError<typelist> ERROR_Not_A_TypeList;
	};
	template<> struct length<NullType> { static const int value=0; };
	template<class A, class B> struct length< item<A, B> >
	{
		static const int value=1+length<B>::value;
	};
	/*! \struct at
	\ingroup TL
	\brief Returns the type at index \em idx, returning NullType if out of bounds
	*/
	template<class typelist, FXuint idx> struct at { typedef NullType value; };
	template<class A, class B> struct at<item<A, B>, 0> { typedef A value; };
	template<class A, class B, FXuint idx> struct at<item<A, B>, idx>
	{
		typedef typename at<B, idx-1>::value value;
	};
	/*! \struct atC
	\ingroup TL
	\brief Returns the type at index \em idx, throwing an error if out of bounds
	*/
	template<class typelist, FXuint idx> struct atC
	{
		StaticError<typelist> ERROR_Out_Of_Bounds_Or_Not_A_TypeList;
	};
	template<class A, class B> struct atC<item<A, B>, 0> { typedef A value; };
	template<class A, class B, FXuint idx> struct atC<item<A, B>, idx>
	{
		typedef typename atC<B, idx-1>::value value;
	};
	/*! \struct find
	\ingroup TL
	\brief Returns the index of the specified type in the list or -1 if not found
	*/
	template<class typelist, typename type> struct find
	{
		StaticError<typelist> ERROR_Not_A_TypeList;
	};
	template<typename type> struct find<NullType, type> { static const int value=-1; };
	template<typename type, class next> struct find<item<type, next>, type> { static const int value=0; };
	template<typename A, class next, typename type> struct find<item<A, next>, type>
	{
	private:
		static const int temp=find<next, type>::value;
	public:
		static const int value=(temp==-1 ? -1 : 1+temp);
	};
	/*! \struct findC
	\ingroup TL
	\brief Returns the index of the specified type in the list, throwing an error if not found
	*/
	template<class typelist, typename type> struct findC
	{
		static const int value=find<typelist, type>::value;
	private:
		typename Generic::select<value==-1, StaticError<typelist>, NullType>::value ERROR_Type_Not_Found;
	};
	/*! \struct findParent
	\ingroup TL
	\brief Returns the index of the first parent class of the specified type in the list or -1 if not found
	*/
	template<class typelist, typename type> struct findParent
	{
		StaticError<typelist> ERROR_Not_A_TypeList;
	};
	template<typename type> struct findParent<NullType, type> { static const int value=-1; };
	template<typename type, class next> struct findParent<item<type, next>, type> { static const int value=0; };
	template<typename A, class next, typename type> struct findParent<item<A, next>, type>
	{
	private:
		static const bool isSubclass=convertible<A, type>::value;
		static const int temp=isSubclass ? 0 : findParent<next, type>::value;
	public:
		static const int value=(temp==-1 ? -1 : 1+temp-isSubclass);
	};
	/*! \struct findParentC
	\ingroup TL
	\brief Returns the index of the specified type in the list, throwing an error if not found
	*/
	template<class typelist, typename type> struct findParentC
	{
		static const int value=findParent<typelist, type>::value;
	private:
		typename Generic::select<value==-1, StaticError<typelist>, NullType>::value ERROR_Type_Not_Found;
	};
	/*! \struct append
	\ingroup TL
	\brief Appends a type or typelist
	*/
	template<class typelist, typename type> struct append
	{
		StaticError<typelist> ERROR_Not_A_TypeList;
	};
	template<> struct append<NullType, NullType> { typedef NullType value; };
	template<typename type> struct append<NullType, type> { typedef typename create<type>::value value; };
	template<typename A, class next> struct append<NullType, item<A, next> >
	{
		typedef item<A, next> value;
	};
	template<typename A, class next, typename type> struct append<item<A,next>, type>
	{
		typedef item<A, typename append<next, type>::value> value;
	};
	/*! \struct remove
	\ingroup TL
	\brief Removes a type from a list
	*/
	template<class typelist, typename type> struct remove
	{
		StaticError<typelist> ERROR_Not_A_TypeList;
	};
	template<typename type> struct remove<NullType, type> { typedef NullType value; };
	template<typename type, class next> struct remove<item<type, next>, type> { typedef next value; };
	template<typename A, class next, typename type> struct remove<item<A, next>, type>
	{
		typedef item<A, typename remove<next, type>::value> value;
	};
	namespace numberRangePrivate {
		template<int no, template<int> class instance, int top> struct Impl
		{
			typedef item<instance<top-no>, typename Impl<no-1, instance, top>::value> value;
		};
		template<template<int> class instance, int top> struct Impl<0, instance, top>
		{
			typedef NullType value;
		};
	}
	/*! \struct numberRange
	\ingroup TL
	\brief Creates a typelist composed of types instantiated by a number
	running from 0 to \em no

	The default instance is FX::Generic::IntToType and thus creates basically
	a type list of numbers. This can be very useful indeed for generating at
	compile time a unique index etc.
	*/
	template<int no, template<int> class instance=IntToType> struct numberRange
	{
		typedef typename numberRangePrivate::Impl<no, instance, no>::value value;
	};
	/*! \struct apply
	\ingroup TL
	\brief Applies each type in a typelist to the template \em instance, returning a typelist

	ie; from a list "type1, type2 ..." it becomes "instance<type1>, instance<type2> ..."
	*/
	template<typename typelist, template<class> class instance> struct apply
	{
		StaticError<typelist> ERROR_Not_A_TypeList;
	};
	template<typename type, class next, template<class> class instance> struct apply<item<type, next>, instance>
	{
		typedef item<instance<type>, typename apply<next, instance>::value> value;
	};
	template<template<class> class instance> struct apply<NullType, instance>
	{
		typedef NullType value;
	};
	/*! \struct filter
	\ingroup TL
	\brief Reduces from a typelist to another based on a binary predicate

	This example outputs a typelist of all those members of a typelist which can be
	converted to an int:
	\code
	template<typename type> struct predicate { static const bool value=Generic::convertible<int, type>::value; };
	typedef Generic::TL::filter<typelist, predicate>::value TypesConvertibleToInt;
	\endcode
	*/
	template<typename typelist, template<class> class filt> struct filter
	{
		StaticError<typelist> ERROR_Not_A_TypeList;
	};
	template<typename type, class next, template<class> class filt> struct filter<item<type, next>, filt>
	{
	private:
		typedef typename filter<next, filt>::value nextfilter;
	public:
		typedef typename select<filt<type>::value, item<type, nextfilter>, nextfilter>::value value;
	};
	template<template<class> class filt> struct filter<NullType, filt>
	{
		typedef NullType value;
	};
	/*! \struct replicate
	\ingroup TL
	\brief Generates a typelist of \em no copies of a type
	*/
	template<int no, typename type> struct replicate
	{
		typedef item<type, typename replicate<no-1, type>::value> value;
	};
	template<typename type> struct replicate<0, type>
	{
		typedef NullType value;
	};

#if defined(_MSC_VER)
#pragma warning(push)
// Disable silly warning about class inheriting off itself
#pragma warning(disable : 4584)
#endif // _MSC_VER

	/*! \struct instanceHolderH
	\ingroup TL
	\brief Default holder for instanceH
	*/
	template<typename type> struct instanceHolderH
	{
		type value;
		//! Constructs an instance
		instanceHolderH() { }
		//! Constructs an instance with parameters
		template<typename P1> instanceHolderH(P1 p1) : value(p1) { }
		//! \overload
		template<typename P1, typename P2> instanceHolderH(P1 p1, P2 p2) : value(p1, p2) { }
		//! \overload
		template<typename P1, typename P2, typename P3> instanceHolderH(P1 p1, P2 p2, P3 p3) : value(p1, p2, p3) { }
	};

	namespace Private
	{
		template<int idx, class instance> struct instanceHHolder : public instance
		{	// Hold an indexed instantiation of a typelist member
			instanceHHolder() : instance() { }
			template<typename P1> explicit instanceHHolder(P1 p1) : instance(p1) { }
			template<typename P1, typename P2> explicit instanceHHolder(P1 p1, P2 p2) : instance(p1, p2) { }
			template<typename P1, typename P2, typename P3> explicit instanceHHolder(P1 p1, P2 p2, P3 p3) : instance(p1, p2, p3) { }
		};
	}
	/*! \struct instantiateH
	\ingroup TL
	\brief Horizontally instantiates a container containing a \em holder for each item in a typelist

	This is an amazingly powerful call. For each item in the typelist, class
	\em instance is instantiated with that item and all instances are inherited
	from to form a container of them all. Thus:
	\code
	typedef TL::create<int, double, char>::value mylist;
	template<typename type> struct Holder
	{
		type value;
	};
	TL::instantiateH<mylist, Holder> mycontainer;
	// mycontainer now holds an int, double and char but they are all called "value"
	// To ease disambiguation, use the helper function instance()
	TL::instance<0>(mycontainer).value=5;                                   // Sets the int to 5
	TL::instance<1>(mycontainer).value=TL::instance<0>(mycontainer).value;  // And so on ...
	\endcode
	To make things even more powerful, you can specify construction parameters for your instance
	by constructing instantiateH with any parameters you choose - these are passed by \b value
	to your instance's constructor. They unfortunately must be by value as Traits needs typelists
	and thus cannot be defined before it.

	One caveat is that since each \em instance is a base class, if it contains any
	virtual functions then the space occupied by the virtual tables can quickly
	multiply. FX::Generic::TL::instantiateV helps you here at the cost of a more
	complex \em instance.
	*/
	template<typename typelist, template<class> class instance=instanceHolderH, int idx=0> struct instantiateH
	{
		StaticError<typelist> ERROR_Not_A_TypeList;
	};
	template<typename type, class next, template<class> class instance, int idx> struct instantiateH<item<type, next>, instance, idx>
		: public Private::instanceHHolder<idx, instance<type> >, public instantiateH<next, instance, idx+1>
	{
		//! Construct instances using their default constructor
		instantiateH() : Private::instanceHHolder<idx, instance<type> >(), instantiateH<next, instance, idx+1>() { }
		//! Construct instances with parameter <tt>p1</tt>
		template<typename P1> explicit instantiateH(P1 p1)
			: Private::instanceHHolder<idx, instance<type> >(p1), instantiateH<next, instance, idx+1>(p1) { }
		//! Construct instances with parameters <tt>p1, p2</tt>
		template<typename P1, typename P2> explicit instantiateH(P1 p1, P2 p2)
			: Private::instanceHHolder<idx, instance<type> >(p1, p2), instantiateH<next, instance, idx+1>(p1, p2) { }
		//! Construct instances with parameters <tt>p1, p2, p3</tt>
		template<typename P1, typename P2, typename P3> explicit instantiateH(P1 p1, P2 p2, P3 p3)
			: Private::instanceHHolder<idx, instance<type> >(p1, p2, p3), instantiateH<next, instance, idx+1>(p1, p2, p3) { }
	};
	template<template<class> class instance, int idx> struct instantiateH<NullType, instance, idx>
	{
		instantiateH() { }
		template<typename P1> explicit instantiateH(P1 p1) { }
		template<typename P1, typename P2> explicit instantiateH(P1 p1, P2 p2) { }
		template<typename P1, typename P2, typename P3> explicit instantiateH(P1 p1, P2 p2, P3 p3) { }
	};

	namespace Private
	{
		// TODO: volatile specialisations & instantiateV
		template<int i, class container> struct accessInstantiateH
		{
			StaticError<container> ERROR_Not_An_InstantiateH_Container;
		};
		template<int i, typename typelist, template<class> class instance> struct accessInstantiateH<i, instantiateH<typelist, instance, 0> >
		{
			typedef instance<typename atC<typelist, i>::value> IdxType;
			typedef instantiateH<typelist, instance, 0> containerType;
			static IdxType &Get(containerType &c)
			{
				return static_cast<instanceHHolder<i, IdxType> &>(c);
			}
		};
		template<int i, typename typelist, template<class> class instance> struct accessInstantiateH<i, const instantiateH<typelist, instance, 0> >
		{
			typedef instance<typename atC<typelist, i>::value> IdxTypeO;
			typedef const IdxTypeO IdxType;
			typedef const instantiateH<typelist, instance, 0> containerType;
			static IdxType &Get(const containerType &c)
			{
				return static_cast<const instanceHHolder<i, IdxTypeO> &>(c);
			}
		};
	}
	/*! \ingroup TL
	Helper function for disambiguating instanced members of an instantiated
	typelist. Simply specify the original index of the type within the list
	to access. Remember it returns a reference to the \em instance template
	parameter to instantiateH.
	*/
	template<int i, class container> typename Private::accessInstantiateH<i, container>::IdxType &instance(container &c)
	{
		return Private::accessInstantiateH<i, container>::Get(c);
	}
#if defined(_MSC_VER)
#pragma warning(pop) 
#endif
}
#define FXTYPELIST1(P1) FX::Generic::TL::list<P1, FX::Generic::NullType>
#define FXTYPELIST2(P1,P2) FX::Generic::TL::list<P1, FXTYPELIST1(P2) >
#define FXTYPELIST3(P1,P2,P3) FX::Generic::TL::list<P1, FXTYPELIST2(P2,P3) >
#define FXTYPELIST4(P1,P2,P3,P4) FX::Generic::TL::list<P1, FXTYPELIST3(P2,P3,P4) >
#define FXTYPELIST5(P1,P2,P3,P4,P5) FX::Generic::TL::list<P1, FXTYPELIST4(P2,P3,P4,P5) >
#define FXTYPELIST6(P1,P2,P3,P4,P5,P6) FX::Generic::TL::list<P1, FXTYPELIST5(P2,P3,P4,P5,P6) >
#define FXTYPELIST7(P1,P2,P3,P4,P5,P6,P7) FX::Generic::TL::list<P1, FXTYPELIST6(P2,P3,P4,P5,P6,P7) >
#define FXTYPELIST8(P1,P2,P3,P4,P5,P6,P7,P8) FX::Generic::TL::list<P1, FXTYPELIST7(P2,P3,P4,P5,P6,P7,P8) >
#define FXTYPELIST9(P1,P2,P3,P4,P5,P6,P7,P8,P9) FX::Generic::TL::list<P1, FXTYPELIST8(P2,P3,P4,P5,P6,P7,P8,P9) >
#define FXTYPELIST10(P1,P2,P3,P4,P5,P6,P7,P8,P9,P10) \
	FX::Generic::TL::list<P1, FXTYPELIST9(P2,P3,P4,P5,P6,P7,P8,P9,P10) >
#define FXTYPELIST11(P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11) \
	FX::Generic::TL::list<P1, FXTYPELIST10(P2,P3,P4,P5,P6,P7,P8,P9,P10,P11) >
#define FXTYPELIST12(P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12) \
	FX::Generic::TL::list<P1, FXTYPELIST11(P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12) >
#define FXTYPELIST13(P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13) \
	FX::Generic::TL::list<P1, FXTYPELIST12(P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13) >
#define FXTYPELIST14(P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14) \
	FX::Generic::TL::list<P1, FXTYPELIST13(P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14) >
#define FXTYPELIST15(P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15) \
	FX::Generic::TL::list<P1, FXTYPELIST14(P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15) >
#define FXTYPELIST16(P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16) \
	FX::Generic::TL::list<P1, FXTYPELIST15(P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16) >

/*! \struct FnInfo
\ingroup generic
\brief Extracts type information from a function pointer type

Use as follows:
\code
class Test
{
	int *foo(double);
};
// Need to convert the function pointer into a function pointer type
// using template argument deduction
template<typename fn> void GetFnInfo(fn funcPtr)
{
	typedef FX::Generic::FnInfo<fn> fooInfo;
	typedef fooInfo::resultType result;		// result becomes "int *"
	typedef fooInfo::objectType object;		// object becomes "class Test"
	typedef fooInfo::par1Type par1;			// par1 becomes "double"
}
...
GetFnInfo(&Test::foo);
\endcode
Up to four parameters are supported. You can also read ::isConst which
becomes true when the function pointer type refers to a const function and
::arity for how many parameters there are. ::objectType becomes NullType when
the function pointer is not a member function of a class. ::asList is
a typelist with the first item the result type and subsequent items the
parameters.

If the function pointer type is not a pointer to code, ::arity becomes -1
and resultType becomes the type passed as template parameter.
*/
template<typename fn> struct FnInfo
{
	typedef fn resultType;
	typedef NullType objectType;
	static const bool isConst=false;
	static const int arity=-1;
	typedef typename TL::create<resultType>::value asList;
};
template<typename R, typename O> struct FnInfo<R (O::*)()>
{
	typedef R resultType;
	typedef O objectType;
	static const bool isConst=false;
	static const int arity=0;
	typedef typename TL::create<resultType>::value asList;
};
template<typename R, typename O> struct FnInfo<R (O::*)() const>
{
	typedef R resultType;
	typedef O objectType;
	static const bool isConst=true;
	static const int arity=0;
	typedef typename TL::create<resultType>::value asList;
};
template<typename R> struct FnInfo<R (*)(void)>
{
	typedef R resultType;
	typedef NullType objectType;
	static const bool isConst=false;
	static const int arity=0;
	typedef typename TL::create<resultType>::value asList;
};
template<typename R, typename O, typename P1> struct FnInfo<R (O::*)(P1)>
{
	typedef R resultType;
	typedef O objectType;
	typedef P1 par1Type;
	static const bool isConst=false;
	static const int arity=1;
	typedef typename TL::create<resultType, par1Type>::value asList;
};
template<typename R, typename O, typename P1> struct FnInfo<R (O::*)(P1) const>
{
	typedef R resultType;
	typedef O objectType;
	typedef P1 par1Type;
	static const bool isConst=true;
	static const int arity=1;
	typedef typename TL::create<resultType, par1Type>::value asList;
};
template<typename R, typename P1> struct FnInfo<R (*)(P1)>
{
	typedef R resultType;
	typedef NullType objectType;
	typedef P1 par1Type;
	static const bool isConst=false;
	static const int arity=1;
	typedef typename TL::create<resultType, par1Type>::value asList;
};
template<typename R, typename O, typename P1, typename P2> struct FnInfo<R (O::*)(P1, P2)>
{
	typedef R resultType;
	typedef O objectType;
	typedef P1 par1Type;
	typedef P2 par2Type;
	static const bool isConst=false;
	static const int arity=2;
	typedef typename TL::create<resultType, par1Type, par2Type>::value asList;
};
template<typename R, typename O, typename P1, typename P2> struct FnInfo<R (O::*)(P1, P2) const>
{
	typedef R resultType;
	typedef O objectType;
	typedef P1 par1Type;
	typedef P2 par2Type;
	static const bool isConst=true;
	static const int arity=2;
	typedef typename TL::create<resultType, par1Type, par2Type>::value asList;
};
template<typename R, typename P1, typename P2> struct FnInfo<R (*)(P1, P2)>
{
	typedef R resultType;
	typedef NullType objectType;
	typedef P1 par1Type;
	typedef P2 par2Type;
	static const bool isConst=false;
	static const int arity=2;
	typedef typename TL::create<resultType, par1Type, par2Type>::value asList;
};
template<typename R, typename O, typename P1, typename P2, typename P3> struct FnInfo<R (O::*)(P1, P2, P3)>
{
	typedef R resultType;
	typedef O objectType;
	typedef P1 par1Type;
	typedef P2 par2Type;
	typedef P3 par3Type;
	static const bool isConst=false;
	static const int arity=3;
	typedef typename TL::create<resultType, par1Type, par2Type, par3Type>::value asList;
};
template<typename R, typename O, typename P1, typename P2, typename P3> struct FnInfo<R (O::*)(P1, P2, P3) const>
{
	typedef R resultType;
	typedef O objectType;
	typedef P1 par1Type;
	typedef P2 par2Type;
	typedef P3 par3Type;
	static const bool isConst=true;
	static const int arity=3;
	typedef typename TL::create<resultType, par1Type, par2Type, par3Type>::value asList;
};
template<typename R, typename P1, typename P2, typename P3> struct FnInfo<R (*)(P1, P2, P3)>
{
	typedef R resultType;
	typedef NullType objectType;
	typedef P1 par1Type;
	typedef P2 par2Type;
	typedef P3 par3Type;
	static const bool isConst=false;
	static const int arity=3;
	typedef typename TL::create<resultType, par1Type, par2Type, par3Type>::value asList;
};
template<typename R, typename O, typename P1, typename P2, typename P3, typename P4> struct FnInfo<R (O::*)(P1, P2, P3, P4)>
{
	typedef R resultType;
	typedef O objectType;
	typedef P1 par1Type;
	typedef P2 par2Type;
	typedef P3 par3Type;
	typedef P4 par4Type;
	static const bool isConst=false;
	static const int arity=4;
	typedef typename TL::create<resultType, par1Type, par2Type, par3Type, par4Type>::value asList;
};
template<typename R, typename O, typename P1, typename P2, typename P3, typename P4> struct FnInfo<R (O::*)(P1, P2, P3, P4) const>
{
	typedef R resultType;
	typedef O objectType;
	typedef P1 par1Type;
	typedef P2 par2Type;
	typedef P3 par3Type;
	typedef P4 par4Type;
	static const bool isConst=true;
	static const int arity=4;
	typedef typename TL::create<resultType, par1Type, par2Type, par3Type, par4Type>::value asList;
};
template<typename R, typename P1, typename P2, typename P3, typename P4> struct FnInfo<R (*)(P1, P2, P3, P4)>
{
	typedef R resultType;
	typedef NullType objectType;
	typedef P1 par1Type;
	typedef P2 par2Type;
	typedef P3 par3Type;
	typedef P4 par4Type;
	static const bool isConst=false;
	static const int arity=4;
	typedef typename TL::create<resultType, par1Type, par2Type, par3Type, par4Type>::value asList;
};
namespace FnFromListPrivate {
	template<typename list, int pars> struct impl;
	template<typename list> struct impl<list, 0> { typedef typename TL::at<list, 0>::value (*value)(); };
	template<typename list> struct impl<list, 1>
	{ typedef typename TL::at<list, 0>::value (*value)(typename TL::at<list, 1>::value); };
	template<typename list> struct impl<list, 2>
	{ typedef typename TL::at<list, 0>::value (*value)(typename TL::at<list, 1>::value, typename TL::at<list, 2>::value); };
	template<typename list> struct impl<list, 3>
	{ typedef typename TL::at<list, 0>::value (*value)(typename TL::at<list, 1>::value, typename TL::at<list, 2>::value, typename TL::at<list, 3>::value); };
	template<typename list> struct impl<list, 4>
	{ typedef typename TL::at<list, 0>::value (*value)(typename TL::at<list, 1>::value, typename TL::at<list, 2>::value, typename TL::at<list, 3>::value, typename TL::at<list, 4>::value); };
}
/*! \struct FnFromList
\ingroup generic
\brief Converts a typelist into a function pointer type

Whereby you get a typelist of the form [ret, par1, par2 ...] eg; from FX::Generic::FnInfo<>::asList,
you can turn it back into a function pointer type ie; <tt>ret (*)(par1, par2 ...)</tt>
*/
template<typename list> struct FnFromList
{
public:
	typedef typename FnFromListPrivate::impl<list, TL::length<list>::value-1>::value value;
};

/*! \struct IntegralLists
\ingroup generic
\brief A series of typelists of kinds of integral type
*/
struct IntegralLists
{
	//! All unsigned integer types
	typedef TL::create<unsigned char, FXushort, FXuint, unsigned long, FXulong>::value unsignedInts;
	//! All signed integer types
	typedef TL::create<signed char, FXshort, FXint, signed long, FXlong>::value signedInts;
	//! Other kinds of integer
	typedef TL::create<bool, char>::value otherInts;
	//! Floating point types
	typedef TL::create<FXfloat, FXdouble>::value floats;

	//! All integer types
	typedef TL::append<TL::append<unsignedInts, signedInts>::value, otherInts>::value Ints;
	//! All arithmetical types
	typedef TL::append<TL::append<unsignedInts, signedInts>::value, floats>::value Arithmetical;
	//! All types which are signed
	typedef TL::append<signedInts, floats>::value Signeds;
	//! All integral types
	typedef TL::append<Ints, floats>::value All;

	//! Smallest unsigned int
	typedef TL::at<unsignedInts, 0>::value smallestUnsignedInt;
	//! Smallest signed int
	typedef TL::at<signedInts,   0>::value smallestSignedInt;
	//! Smallest floating point
	typedef TL::at<floats,       0>::value smallestFloat;
	//! Biggest unsigned int
	typedef TL::at<unsignedInts, TL::length<unsignedInts>::value-1>::value biggestUnsignedInt;
	//! Biggest signed int
	typedef TL::at<signedInts,   TL::length<signedInts>::value-1>::value biggestSignedInt;
	//! Biggest floating point
	typedef TL::at<floats,       TL::length<floats>::value-1>::value biggestFloat;
};

/*! \struct BiggestValue
\ingroup generic
\brief Returns the biggest positive or negative value which can be stored in an integral type
*/
template<typename type, bool minus=false> struct BiggestValue
{
private:
	static const int UnsignedIntIdx=TL::find<IntegralLists::unsignedInts, type>::value;
	static const int SignedIntIdx  =TL::find<IntegralLists::signedInts, type>::value;
	static const int intIdx=(-1==UnsignedIntIdx) ? SignedIntIdx : UnsignedIntIdx;
	typedef typename TL::atC<IntegralLists::unsignedInts, intIdx>::value typeAsUnsigned;
	static const typeAsUnsigned maxUnsignedValue=(typeAsUnsigned)-1;
	static const typeAsUnsigned maxUnsignedValueL1=(typeAsUnsigned)((-1==UnsignedIntIdx) ? maxUnsignedValue<<1 : maxUnsignedValue);
	static const typeAsUnsigned maxSignedValue=(typeAsUnsigned)((-1==UnsignedIntIdx) ? maxUnsignedValueL1>>1 : maxUnsignedValue);
public:
	static const type value=(type)(minus ? ~maxSignedValue : maxSignedValue);
};
#if defined(_MSC_VER) && _MSC_VER<=1400
template<bool minus> struct BiggestValue<float, minus>
{	// Not supported on <=MSVC80
};
template<bool minus> struct BiggestValue<double, minus>
{	// Not supported on <=MSVC80
};
#else
template<> struct BiggestValue<float, false>
{
	static const float value=3.402823466e+38F;
};
template<> struct BiggestValue<float, true>
{
	static const float value=1.175494351e-38F;
};
template<> struct BiggestValue<double, false>
{
	static const double value=1.7976931348623158e+308;
};
template<> struct BiggestValue<double, true>
{
	static const double value=2.2250738585072014e-308;
};
#endif

/*! \defgroup ClassTraits Class Traits
\ingroup generic
\brief Provides user-definable traits
*/
/*! \namespace ClassTraits
\ingroup ClassTraits
\brief Holds user-definable traits classes and structures can possess
*/
namespace ClassTraits
{
	struct POD;
	/*! \struct combine
	\ingroup ClassTraits
	\brief Combines class traits when specialising a type
	\sa FX::Generic::ClassTraits::has
	*/
	template<class T1=NullType> struct combine
	{
		typedef typename TL::create<T1>::value pars;
		static const bool PODness=TL::find<pars, POD>::value>=0;
	};
	/*! \struct has
	\ingroup ClassTraits
	\brief Permits you to specify qualities of a type

	This class works with FX::Generic::TraitsBasic & FX::Generic::Traits. To use,
	specialise the traits in question for your type eg;
	\code
	namespace FX { namespace Generic { namespace ClassTraits {
		template<> struct has<MyType> : public combine<POD> {};
	} } };
	\endcode
	You can use:
	\li POD, means your type is fine being memcpyed and memcmped
	*/
	template<class type> struct has : public combine<> { };
}
/*! \class Traits
\ingroup generic
\brief Determines qualities of a type via introspection

This is an amazingly useful class. It is divided into two parts, FX::Generic::TraitsBasic
and FX::Generic::Traits with the basic base class providing all the commonly used (and
least compiler demanding) traits. If you can, use TraitsBasic where possible.

You may also find FX::Generic::FnInfo useful for working with function pointers when
isFunctionPtr is true.
Traits provided by TraitsBasic:
\li isVoid, if true then all rest below are irrelevent
\li isPtr, true when type is a pointer. False if reference to a pointer.
\li isRef, true when type is a reference
\li isPtrToCode, true when type points to code
\li isMemberPtr, true when type is a member function pointer
\li isFunctionPtr, true when type is a function pointer
\li isValue, true when type is a value (therefore neither of the above)
\li isIndirect, true when type is an indirection to real data (the previous is false)
\li isConst, true when type is const
\li isVolatile, true when type is volatile

\li isArray, true when the base type is an array
\li isFloat, true when base type is a floating point value
\li isInt, true when base type is an integer
\li isSigned, true when base type is signed
\li isUnsigned, true when base type is unsigned
\li isArithmetical, true when base type can be used to do maths
\li isIntegral or isBasic, true when base type is one of the basic or integral types
\li holdsData, true when the previous is false and is not a function pointer.
True for enums even though they are equivalent to an int. True for arrays.
\li isPOD, true when the base type is Plain Old Data (POD) and therefore can be
memcpyed or memcmped. See notes below.

Furthermore, the following types are defined:
\li baseType: This is the type less any indirection.
\li asConstParam: This is the type plus const, but only where that makes sense ie;
integral types remain non-const
\li asRWParam: This is the best type for passing this type as a parameter.
\li asROParam: Same as asRWParam but const where it makes sense ie; values remain
as-is, pointers become const pointers and other types (structs, classes, enums
etc.) become const references.

More advanced traits not provided by TraitsBasic (note that these require more
compiler time and require fully known types ie; not just predeclarations):
\li isEnum, true when base type is an enum (note: depends on an implicit conversion
to \c int being available, which may be provided by a class operator. May give
false positives if \c sizeof(type)==sizeof(enum) and provides <tt>operator int()</tt>)
\li isPolymorphic, true when the base type contains virtual members (note: depends on
the compiler saying a class with a virtual member is bigger)

What is the best type for passing as a parameter? Well basic types like int
and pointers should be passed as-is along with enums whereas structures,
classes, arrays etc. should be passed by reference for efficiency. asROParam
only applies const when the parameter type permits altering of external data
(except for enums, just in case isEnum is misdiagnosed). Convertibility is
always maintained (ie; so the compiler can always convert your type to asRWParam
or asROParam) so this is ideal for compile-time parameter specification.
<h3>Notes:</h3>
isPOD currently only becomes true if isBasic is true as there is no way
currently in the ISO C++ standard to determine if a class is POD. You can
manually specify PODness using FX::Generic::ClassTraits.
*/
namespace TraitsHelper
{
	template<typename par> struct isVoidI { static const bool value=false; };
	template<> struct isVoidI<void> { static const bool value=true; };
	template<typename par> struct polyA : public par
	{
		char foo[64];
	};
	template<typename par> struct polyB : public par
	{
		char foo[64];
		virtual ~polyB() { }
	};
	template<bool isComplex, typename par> struct isPolymorphicI { static const bool value=false; };
	template<typename par> struct isPolymorphicI<true, par>
	{
		static const bool value=sizeof(polyA<par>)==sizeof(polyB<par>);
	};
}
template<typename type> class TraitsBasic
{
protected:
	template<typename par> struct isVoidI { static const bool value=TraitsHelper::isVoidI<par>::value; };
	template<typename par> struct isPtrI { static const bool value=false; };
	template<typename par> struct isPtrI<par *> { static const bool value=true; };
	template<typename par> struct isRefI { static const bool value=false; };
	template<typename par> struct isRefI<par &> { static const bool value=true; };
	template<typename par> struct isArrayI { static const bool value=false; };
	template<typename T, unsigned int len> struct isArrayI<T[len]> { static const bool value=true; };
	template<typename T, unsigned int len> struct isArrayI<T const[len]> { static const bool value=true; };
	template<typename T, unsigned int len> struct isArrayI<T volatile[len]> { static const bool value=true; };
	template<typename T, unsigned int len> struct isArrayI<T const volatile[len]> { static const bool value=true; };
	template<typename par> struct isConstI { static const bool value=false; };
	template<typename par> struct isConstI<const par> { static const bool value=true; };
	template<typename par> struct isVolatileI { static const bool value=false; };
	template<typename par> struct isVolatileI<volatile par> { static const bool value=true; };
	typedef FnInfo<type> fnInfo;
	template<bool wantConst, typename par> struct addConstI { typedef par value; };
	template<typename par> struct addConstI<true, par> { typedef const par value; };
public:
	static const bool isVoid=isVoidI<type>::value;
	static const bool isPtr=isPtrI<type>::value;
	static const bool isRef=isRefI<type>::value;
	static const bool isPtrToCode=fnInfo::arity>=0;
	static const bool isMemberPtr=isPtrToCode && !sameType<typename fnInfo::objectType, NullType>::value;
	static const bool isFunctionPtr=isPtrToCode && sameType<typename fnInfo::objectType, NullType>::value;
	static const bool isValue=!isPtr && !isRef && !isPtrToCode;
	static const bool isIndirect=!isValue;
	static const bool isConst=isConstI<type>::value;
	static const bool isVolatile=isVolatileI<type>::value;

	typedef typename leastIndir<type>::value baseType;
	static const bool isArray=isArrayI<baseType>::value;
	static const bool isFloat=TL::find<IntegralLists::floats, baseType>::value>=0;
	static const bool isInt=TL::find<IntegralLists::Ints, baseType>::value>=0;
	static const bool isSigned=TL::find<IntegralLists::Signeds, baseType>::value>=0;
	static const bool isUnsigned=TL::find<IntegralLists::unsignedInts, baseType>::value>=0;
	static const bool isArithmetical=TL::find<IntegralLists::Arithmetical, baseType>::value>=0;
	static const bool isIntegral=TL::find<IntegralLists::All, baseType>::value>=0;
	static const bool isBasic=isIntegral;
	static const bool holdsData=!isIntegral && !isPtrToCode;

	static const bool isPOD=isBasic || ClassTraits::has<type>::PODness;

	typedef typename select<isIntegral || isIndirect, type, typename addRef<type>::value >::value asRWParam;
	typedef typename addConstI<!isIntegral && !isRef, type>::value asConstParam;
private:
	typedef typename TL::create<asRWParam, asConstParam, typename addRef<asConstParam>::value>::value ROParams;
public:
	typedef typename TL::at<ROParams, ((isValue && isBasic) || isRef) ? 0 : (isIndirect) ? 1 : 2>::value asROParam;
};
template<typename type> class Traits : public TraitsBasic<type>
{
	enum TestEnum {};
	template<bool isCodePtr, typename par> struct isEnumSizeI { static const bool value=false; };
	template<typename par> struct isEnumSizeI<false, par> { static const bool value=sizeof(TestEnum)==sizeof(par); };
private:
	static const bool int_baseTypeIsVoid=TraitsHelper::isVoidI<typename TraitsBasic<type>::baseType>::value;
	static const bool int_isEnumSize=isEnumSizeI<TraitsBasic<type>::isPtrToCode || int_baseTypeIsVoid, typename TraitsBasic<type>::baseType>::value;
	static const bool int_isConvertibleToInt=!TraitsBasic<type>::isPtrToCode && convertible<int, typename TraitsBasic<type>::baseType>::value;
public:
	static const bool isEnum=TraitsBasic<type>::holdsData && !TraitsBasic<type>::isArray && int_isEnumSize && int_isConvertibleToInt;
	static const bool isPolymorphic=TraitsHelper::isPolymorphicI<TraitsBasic<type>::holdsData && !TraitsBasic<type>::isArray && !isEnum && !int_baseTypeIsVoid, typename TraitsBasic<type>::baseType>::value;
};




namespace FunctorHelper {
	template<typename parslist> struct ImplBaseBase
	{
		typedef typename TL::at<parslist, 0>::value R;
		typedef typename TL::at<parslist, 1>::value P1base;
		typedef typename TraitsBasic<P1base>::asROParam P1;
		typedef typename TL::at<parslist, 2>::value P2base;
		typedef typename TraitsBasic<P2base>::asROParam P2;
		typedef typename TL::at<parslist, 3>::value P3base;
		typedef typename TraitsBasic<P3base>::asROParam P3;
		typedef typename TL::at<parslist, 4>::value P4base;
		typedef typename TraitsBasic<P4base>::asROParam P4;
		virtual ~ImplBaseBase() { }
		virtual ImplBaseBase *copy() const=0;
	};
	template<typename parslist, int pars> struct ImplBaseOp;
	template<typename parslist> struct ImplBaseOp<parslist, 0> : public ImplBaseBase<parslist>
	{	virtual bool operator==(const ImplBaseOp &) const=0; virtual typename ImplBaseBase<parslist>::R operator()()=0; };
	template<typename parslist> struct ImplBaseOp<parslist, 1> : public ImplBaseBase<parslist>
	{	virtual bool operator==(const ImplBaseOp &) const=0; virtual typename ImplBaseBase<parslist>::R operator()(typename ImplBaseBase<parslist>::P1 p1)=0; };
	template<typename parslist> struct ImplBaseOp<parslist, 2> : public ImplBaseBase<parslist>
	{	virtual bool operator==(const ImplBaseOp &) const=0; virtual typename ImplBaseBase<parslist>::R operator()(typename ImplBaseBase<parslist>::P1 p1, typename ImplBaseBase<parslist>::P2 p2)=0; };
	template<typename parslist> struct ImplBaseOp<parslist, 3> : public ImplBaseBase<parslist>
	{	virtual bool operator==(const ImplBaseOp &) const=0; virtual typename ImplBaseBase<parslist>::R operator()(typename ImplBaseBase<parslist>::P1 p1, typename ImplBaseBase<parslist>::P2 p2, typename ImplBaseBase<parslist>::P3 p3)=0; };
	template<typename parslist> struct ImplBaseOp<parslist, 4> : public ImplBaseBase<parslist>
	{	virtual bool operator==(const ImplBaseOp &) const=0; virtual typename ImplBaseBase<parslist>::R operator()(typename ImplBaseBase<parslist>::P1 p1, typename ImplBaseBase<parslist>::P2 p2, typename ImplBaseBase<parslist>::P3 p3, typename ImplBaseBase<parslist>::P4 p4)=0; };

	template<typename parentfunctor, typename fn> class ImplFn : public parentfunctor::ImplBase
	{
		typedef typename parentfunctor::ImplBase::R  R;
		typedef typename parentfunctor::ImplBase::P1 P1;
		typedef typename parentfunctor::ImplBase::P2 P2;
		typedef typename parentfunctor::ImplBase::P3 P3;
		typedef typename parentfunctor::ImplBase::P4 P4;
		fn fnptr;
	public:
		ImplFn(fn _fnptr) : fnptr(_fnptr) { }
		ImplFn(const ImplFn &o) : fnptr(o.fnptr) { }
		ImplFn *copy() const { return new ImplFn(*this); }
		bool operator==(typename parentfunctor::ImplBase const &o) const { return fnptr==static_cast<const ImplFn &>(o).fnptr; }
		R operator()() { return fnptr(); }
		R operator()(P1 p1) { return fnptr(p1); }
		R operator()(P1 p1, P2 p2) { return fnptr(p1, p2); }
		R operator()(P1 p1, P2 p2, P3 p3) { return fnptr(p1, p2, p3); }
		R operator()(P1 p1, P2 p2, P3 p3, P4 p4) { return fnptr(p1, p2, p3, p4); }
	};
	template<typename parentfunctor, class obj, typename fn> class ImplMemFn : public parentfunctor::ImplBase
	{
		typedef typename parentfunctor::ImplBase::R  R;
		typedef typename parentfunctor::ImplBase::P1 P1;
		typedef typename parentfunctor::ImplBase::P2 P2;
		typedef typename parentfunctor::ImplBase::P3 P3;
		typedef typename parentfunctor::ImplBase::P4 P4;
		obj &objinst;
		fn fnptr;
	public:
		ImplMemFn(obj &_objinst, fn _fnptr) : objinst(_objinst), fnptr(_fnptr) { }
		ImplMemFn(const ImplMemFn &o) : objinst(o.objinst), fnptr(o.fnptr) { }
		ImplMemFn *copy() const { return new ImplMemFn(*this); }
		bool operator==(typename parentfunctor::ImplBase const &o) const { return &objinst==&static_cast<const ImplMemFn &>(o).objinst && fnptr==static_cast<const ImplMemFn &>(o).fnptr; }
		R operator()() { return (objinst.*fnptr)(); }
		R operator()(P1 p1) { return (objinst.*fnptr)(p1); }
		R operator()(P1 p1, P2 p2) { return (objinst.*fnptr)(p1, p2); }
		R operator()(P1 p1, P2 p2, P3 p3) { return (objinst.*fnptr)(p1, p2, p3); }
		R operator()(P1 p1, P2 p2, P3 p3, P4 p4) { return (objinst.*fnptr)(p1, p2, p3, p4); }
	};
}

/*! \class Functor
\ingroup generic
\brief Represents a callable API

Functors are a compile-time abstraction of some callable entity ie;
basically an enhanced function or member function pointer. Their real
usefulness comes in with \em binding whereby a functor can be bound
with a set of arguments to form an arbitrary call to some API. This
facility is used throughout TnFOX as a much improved form of callback
plus within the dynamic linkage facilities. Up to four arbitrary
parameters are currently supported (this is easily extended, just ask).

With any decent optimising compiler, the considerable complexity of the
functor implementation boils down to two virtual method calls. Functor
itself only occupies 4 + 8 bytes.

This functor operates with move semantics - a copy means destruction
of the original. If you want a real copy, call copy(). You may find
the helper function FX::Generic::BindFunctor of use when binding
a functor's arguments.

\sa FX::Generic::BoundFunctor, FX::FXProcess
*/
template<typename parslist> class Functor
{
public:	// Has to be public unfortunately so it can be inherited off
	typedef typename FunctorHelper::ImplBaseOp<parslist, TL::length<parslist>::value-1> ImplBase;
private:
	ImplBase *fnimpl;
	typedef typename FnFromList<parslist>::value ParsListAsFnType;
public:
	typedef parslist ParsList;
	typedef typename ImplBase::R  R;
	typedef typename ImplBase::P1 P1;
	typedef typename ImplBase::P2 P2;
	typedef typename ImplBase::P3 P3;
	typedef typename ImplBase::P4 P4;
	//! Constructs a null functor
	Functor() : fnimpl(0) { }
	//! Constructs a functor calling a C-style function
	template<typename fn> explicit Functor(fn fnptr) : fnimpl(0)
	{ FXERRHM((fnimpl=new FunctorHelper::ImplFn<Functor, fn>(fnptr))); }
	//! Constructs a functor calling a member function of an object
	template<typename obj, typename fn> Functor(obj &objinst, fn fnptr) : fnimpl(0)
	{ FXERRHM((fnimpl=new FunctorHelper::ImplMemFn<Functor, obj, fn>(objinst, fnptr))); }
	//! Constructs a functor calling a member function of an object pointer
	template<typename obj, typename fn> Functor(obj *objinst, fn fnptr) : fnimpl(0)
	{ FXERRHM((fnimpl=new FunctorHelper::ImplMemFn<Functor, obj, fn>(*objinst, fnptr))); }
	struct void_ {};
	/*! Constructs a functor from a raw address calling a C-style function. This
	uses <tt>Functor::void_ *</tt> to avoid the pointer consuming qualities of <tt>void *</tt> */
	explicit Functor(void_ *fnptr) : fnimpl(0)
	{ FXERRHM((fnimpl=new FunctorHelper::ImplFn<Functor, ParsListAsFnType>((ParsListAsFnType) fnptr))); }
	//! Constructs a functor with a predefined implementation. Note: takes ownership of pointer.
	explicit Functor(ImplBase *_fnimpl) : fnimpl(_fnimpl) { }
#ifndef HAVE_MOVECONSTRUCTORS
#ifdef HAVE_CONSTTEMPORARIES
	Functor(const Functor &other) : fnimpl(other.fnimpl)
	{
		Functor &o=const_cast<Functor &>(other);
#else
	Functor(Functor &o) : fnimpl(o.fnimpl)
	{
#endif
#else
private:
	Functor(const Functor &);		// disable copy constructor
public:
	Functor(Functor &&o) : fnimpl(o.fnimpl)
	{
#endif
		o.fnimpl=0;
	}
	Functor &operator=(Functor &o)
	{
		FXDELETE(fnimpl);
		fnimpl=o.fnimpl;
		o.fnimpl=0;
		return *this;
	}
	~Functor() { FXDELETE(fnimpl); }
	//! Returns a copy of this functor
	Functor copy() const
	{
		ImplBase *newimpl(static_cast<ImplBase *>(fnimpl->copy()));
		FXERRHM(newimpl);
		return Functor(newimpl);
	}
	//! Compares two bound functors
	bool operator==(const Functor &o) const { return *fnimpl==*o.fnimpl; }
	//! Negation compares two bound functors
	bool operator!=(const Functor &o) const { return !(*fnimpl==*o.fnimpl); }
	// Workaround to implement if(sp)
private:
	struct Tester
	{
	private:
		void operator delete(void *);
	};
public:
	operator Tester *() const
	{
		if(!*this) return 0;
		static Tester t;
		return &t;
	}
	// For if(!sp) 
	bool operator!() const { return !fnimpl; }
	//! Calls what the functor points to
	R operator()() { return (*fnimpl)(); }
	//! \overload
	R operator()(P1 p1) { return (*fnimpl)(p1); }
	//! \overload
	R operator()(P1 p1, P2 p2) { return (*fnimpl)(p1, p2); }
	//! \overload
	R operator()(P1 p1, P2 p2, P3 p3) { return (*fnimpl)(p1, p2, p3); }
	//! \overload
	R operator()(P1 p1, P2 p2, P3 p3, P4 p4) { return (*fnimpl)(p1, p2, p3, p4); }
};
/*! \class BoundFunctorV
\ingroup generic
\brief A call to a specific API with specific arguments, throwing away the return
\note You can't copy a BoundFunctorV, but you can a BoundFunctor
*/
class BoundFunctorV
{
	virtual void callV()=0;
protected:
	BoundFunctorV() {}
	BoundFunctorV(const BoundFunctorV &) {}
	BoundFunctorV &operator=(const BoundFunctorV &) { return *this; }
public:
	virtual ~BoundFunctorV() { }
	//! Calls the functor with the bound arguments, throwing away the return
	void operator()() { callV(); }
};

/*! \class BoundFunctor
\ingroup generic
\brief A call to a specific API with specific arguments

The source for this class is illustrative of the power of the compile-time
metaprogramming facilities TnFOX provides - it was surprisingly easy to write,
though I'm sure MSVC7.1's good compliance with the ISO C++ spec has made things
a world easier. I had expected to be spending a week hacking away at it, not a
day!

This class is used throughout TnFOX wherever a callback is required. It permits
arbitrary parameters to be passed to the callback in a type-safe fashion and thus
is a great improvement over the <tt>void *data</tt> method C-style callbacks
traditionally use - you no longer need to cast the "user data pointer" to some
structure & extract the parameters, nor allocate & maintain a separate user data
structure - all this is now taken care of for you by the compiler. Compile-time
introspection is used to ensure that parameters are not copy constructed except
when being stored within the binding - however, the helper friend functions do require
an extra copy construction at the point of creation.

Since at the time of writing (Nov 2003) no C++ compiler implements the \c export
C++ facility, it greatly eases things if object code does not need to know about
the types used by some bound functor. Thus you can take a reference or pointer
to BoundFunctor's base class, BoundFunctorV which permits functor invocation but
with throwing away of the return (as you can't know its type). This comes at the
cost of an extra virtual method indirection at runtime. BoundFunctor varies in
size according to what the bound parameters require to be stored, but it's
typically small - from twelve bytes onwards - usually the custom code generated
by the compiler is more (though with a decent optimising compiler, it's
near-perfect - no more than a Functor call).

Note that unlike Functor, BoundFunctor when copied makes a true new copy plus
passing a Functor to its constructor does not destroy what's passed to it. Thus
you can use one Functor to construct many bound calls to the same thing but with
different parameters.

In the future, the ability to bind some of the arguments but leaving the
remainder to be filled will be added. It's not difficult - just I haven't had
call for it yet.
<h3>Usage:</h3>
You'll almost certainly want to use one of the convenience functions
FX::Generic::BindFunctor, FX::Generic::BindFunc or FX::Generic::BindObj eg;
\code
int main(int argc, char **argv);
FX::Generic::Functor<FX::Generic::TL::create<int, int, char **>::value> mainf(main);
FX::Generic::BoundFunctorV &mainfb=FX::Generic::BindFunctor(mainf, 4, { "Niall", "is", "a", "teapot", 0 });
mainf(2, { "Hello", "World", 0 });  // Calls main(2, { "Hello", "World", 0 })
mainfb();                           // Calls main(4, { "Niall", "is", "a", "teapot", 0 })
\endcode
You can collapse functor generation and binding into one at the cost of
extra copy construction:
\code
FX::Generic::BoundFunctorV &mainfb=FX::Generic::BindObj(main, 4, { "Niall", "is", "a", "teapot", 0 });
\endcode
You may be wondering about the naming scheme I used ie; BindFunctor and BindFunc
look awfully similar. This is because I originally had Bind() which automatically
distinguished between member functions and ordinary functions but it proved too
much for our compiler. Thus I chose the same naming scheme as FX::FXRBFunc and
FX::FXRBObj but I must admit I get confused at times too - however, I can't
think of anything else better which is still descriptive :(

\sa FX::Generic::Functor, FX::Generic::BoundFunctorV
*/
template<typename parslist> class BoundFunctor : public BoundFunctorV
{
	typedef typename TL::at<parslist, 0>::value R;
	typedef typename TraitsBasic<typename TL::at<parslist, 1>::value>::asROParam P1;
	typedef typename TraitsBasic<typename TL::at<parslist, 2>::value>::asROParam P2;
	typedef typename TraitsBasic<typename TL::at<parslist, 3>::value>::asROParam P3;
	typedef typename TraitsBasic<typename TL::at<parslist, 4>::value>::asROParam P4;
	Functor<parslist> myfunctor;
	typedef typename parslist::next realparslist;
	TL::instantiateH<realparslist> parvals;
	// Stupid GCC won't permit explicit specialisation inside a class
	// and we can't partially specialise functions :(
	R call(IntToType<0>) { return myfunctor(); }
	R call(IntToType<1>) { return myfunctor(TL::instance<0>(parvals).value); }
	R call(IntToType<2>) { return myfunctor(TL::instance<0>(parvals).value, TL::instance<1>(parvals).value); }
	R call(IntToType<3>) { return myfunctor(TL::instance<0>(parvals).value, TL::instance<1>(parvals).value, TL::instance<2>(parvals).value); }
	R call(IntToType<4>) { return myfunctor(TL::instance<0>(parvals).value, TL::instance<1>(parvals).value, TL::instance<2>(parvals).value, TL::instance<3>(parvals).value); }
public:
	//! Constructs a bound functor directly from an instantiated typelist
	template<typename fn> BoundFunctor(fn fnptr, TL::instantiateH<realparslist> &_parvals)
		: myfunctor(fnptr), parvals(_parvals), BoundFunctorV() { }
	//! Constructs a bound functor directly from an instantiated typelist
	template<typename obj, typename fn> BoundFunctor(obj &objinst, fn fnptr, TL::instantiateH<realparslist> &_parvals)
		: myfunctor(objinst, fnptr), parvals(_parvals), BoundFunctorV() { }
	//! Constructs a bound functor calling \em _functor with no parameters
#if defined(__INTEL_COMPILER) && __INTEL_COMPILER<=800
	// Avoid usage of templated copy constructor (bug in ICC)
	BoundFunctor(const Functor<parslist> &_functor)
	{
		Functor<parslist> functor=_functor.copy();
		myfunctor=functor;
#else
	BoundFunctor(const Functor<parslist> &_functor) : myfunctor(_functor.copy())
	{
#endif
	}
	//! Constructs a bound functor calling \em _functor with one parameter
#if defined(__INTEL_COMPILER) && __INTEL_COMPILER<=800
	// Avoid usage of templated copy constructor (bug in ICC)
	BoundFunctor(const Functor<parslist> &_functor, P1 p1)
	{
		Functor<parslist> functor=_functor.copy();
		myfunctor=functor;
#else
	BoundFunctor(const Functor<parslist> &_functor, P1 p1) : myfunctor(_functor.copy())
	{
#endif
		TL::instance<0>(parvals).value=p1;
	}
	//! Constructs a bound functor calling \em _functor with two parameters
#if defined(__INTEL_COMPILER) && __INTEL_COMPILER<=800
	// Avoid usage of templated copy constructor (bug in ICC)
	BoundFunctor(const Functor<parslist> &_functor, P1 p1, P2 p2)
	{
		Functor<parslist> functor=_functor.copy();
		myfunctor=functor;
#else
	BoundFunctor(const Functor<parslist> &_functor, P1 p1, P2 p2) : myfunctor(_functor.copy())
	{
#endif
		TL::instance<0>(parvals).value=p1;
		TL::instance<1>(parvals).value=p2;
	}
	//! Constructs a bound functor calling \em _functor with three parameters
#if defined(__INTEL_COMPILER) && __INTEL_COMPILER<=800
	// Avoid usage of templated copy constructor (bug in ICC)
	BoundFunctor(const Functor<parslist> &_functor, P1 p1, P2 p2, P3 p3)
	{
		Functor<parslist> functor=_functor.copy();
		myfunctor=functor;
#else
	BoundFunctor(const Functor<parslist> &_functor, P1 p1, P2 p2, P3 p3) : myfunctor(_functor.copy())
	{
#endif
		TL::instance<0>(parvals).value=p1;
		TL::instance<1>(parvals).value=p2;
		TL::instance<2>(parvals).value=p3;
	}
	//! Constructs a bound functor calling \em _functor with four parameters
#if defined(__INTEL_COMPILER) && __INTEL_COMPILER<=800
	// Avoid usage of templated copy constructor (bug in ICC)
	BoundFunctor(const Functor<parslist> &_functor, P1 p1, P2 p2, P3 p3, P4 p4)
	{
		Functor<parslist> functor=_functor.copy();
		myfunctor=functor;
#else
	BoundFunctor(const Functor<parslist> &_functor, P1 p1, P2 p2, P3 p3, P4 p4) : myfunctor(_functor.copy())
	{
#endif
		TL::instance<0>(parvals).value=p1;
		TL::instance<1>(parvals).value=p2;
		TL::instance<2>(parvals).value=p3;
		TL::instance<3>(parvals).value=p4;
	}
	BoundFunctor(const BoundFunctor &o) : myfunctor(o.myfunctor.copy()), parvals(o.parvals), BoundFunctorV(o) { }
	BoundFunctor &operator=(const BoundFunctor &o)
	{
		myfunctor=o.myfunctor.copy();
		parvals=o.parvals;
		return *this;
	}
	//! Returns the functor called by this binding
	Functor<parslist> &functor() { return myfunctor; }
	//! Returns the parameters the functor is called with
	TL::instantiateH<realparslist> &parameters() { return parvals; }
	//! Invokes the functor with its bound parameters
	R operator()() { return call(IntToType<TL::length<parslist>::value-1>()); }
private:
	void callV() { (*this)(); }

	// MSVC compiles wrong code if these are defined :(
	//template<typename F> friend BoundFunctor<typename F::ParsList> BindFunctor(F &functor);
	//template<typename F> friend BoundFunctor<typename F::ParsList> *BindFunctorN(F &functor);
	//template<typename fn> friend BoundFunctor<typename FnInfo<fn>::asList> BindFunc(fn fnptr);
	//template<typename fn> friend BoundFunctor<typename FnInfo<fn>::asList> *BindFuncN(fn fnptr);
	//template<typename obj, typename fn> friend BoundFunctor<typename FnInfo<fn>::asList> BindObj(obj &objinst, fn fnptr);
	//template<typename obj, typename fn> friend BoundFunctor<typename FnInfo<fn>::asList> *BindObjN(obj &objinst, fn fnptr);
};
#ifdef DOXYGEN_SHOULD_SKIP_THIS
/*! \ingroup generic
Binds a functor to a set of parameters and returns the binding
*/
BoundFunctor BindFunctor(FX::Generic::Functor &functor [, par1 [, par2 ...]]);
/*! \ingroup generic
Binds a functor to a set of parameters and returns the binding as a newed pointer
*/
BoundFunctor *BindFunctorN(FX::Generic::Functor &functor [, par1 [, par2 ...]]);
#else
template<typename F> BoundFunctor<typename F::ParsList>
	BindFunctor(F &functor)
{
	return BoundFunctor<typename F::ParsList>(functor);
}
template<typename F> BoundFunctor<typename F::ParsList> *
	BindFunctorN(F &functor)
{
	return new BoundFunctor<typename F::ParsList>(functor);
}
template<typename F> BoundFunctor<typename F::ParsList>
	BindFunctor(F &functor, typename TraitsBasic<typename F::P1>::asROParam p1)
{
	return BoundFunctor<typename F::ParsList>(functor, p1);
}
template<typename F> BoundFunctor<typename F::ParsList> *
	BindFunctorN(F &functor, typename TraitsBasic<typename F::P1>::asROParam p1)
{
	return new BoundFunctor<typename F::ParsList>(functor, p1);
}
template<typename F> BoundFunctor<typename F::ParsList>
	BindFunctor(F &functor, typename TraitsBasic<typename F::P1>::asROParam p1,
	typename TraitsBasic<typename F::P2>::asROParam p2)
{
	return BoundFunctor<typename F::ParsList>(functor, p1, p2);
}
template<typename F> BoundFunctor<typename F::ParsList> *
	BindFunctorN(F &functor, typename TraitsBasic<typename F::P1>::asROParam p1,
	typename TraitsBasic<typename F::P2>::asROParam p2)
{
	return new BoundFunctor<typename F::ParsList>(functor, p1, p2);
}
template<typename F> BoundFunctor<typename F::ParsList>
	BindFunctor(F &functor, typename TraitsBasic<typename F::P1>::asROParam p1,
	typename TraitsBasic<typename F::P2>::asROParam p2, typename TraitsBasic<typename F::P3>::asROParam p3)
{
	return BoundFunctor<typename F::ParsList>(functor, p1, p2, p3);
}
template<typename F> BoundFunctor<typename F::ParsList> *
	BindFunctorN(F &functor, typename TraitsBasic<typename F::P1>::asROParam p1,
	typename TraitsBasic<typename F::P2>::asROParam p2, typename TraitsBasic<typename F::P3>::asROParam p3)
{
	return new BoundFunctor<typename F::ParsList>(functor, p1, p2, p3);
}
template<typename F> BoundFunctor<typename F::ParsList>
	BindFunctor(F &functor, typename TraitsBasic<typename F::P1>::asROParam p1,
	typename TraitsBasic<typename F::P2>::asROParam p2, typename TraitsBasic<typename F::P3>::asROParam p3,
	typename TraitsBasic<typename F::P4>::asROParam p4)
{
	return BoundFunctor<typename F::ParsList>(functor, p1, p2, p3, p4);
}
template<typename F> BoundFunctor<typename F::ParsList> *
	BindFunctorN(F &functor, typename TraitsBasic<typename F::P1>::asROParam p1,
	typename TraitsBasic<typename F::P2>::asROParam p2, typename TraitsBasic<typename F::P3>::asROParam p3,
	typename TraitsBasic<typename F::P4>::asROParam p4)
{
	return new BoundFunctor<typename F::ParsList>(functor, p1, p2, p3, p4);
}
#endif
#ifdef DOXYGEN_SHOULD_SKIP_THIS
/*! \ingroup generic
Binds a function to a set of parameters and returns the binding
*/
BoundFunctor<> BindFunc(functptr [, par1 [, par2 ...]]);
/*! \ingroup generic
Binds a function to a set of parameters and returns the binding as a newed pointer
*/
BoundFunctor<> BindFuncN(functptr [, par1 [, par2 ...]]);
/*! \ingroup generic
Binds a member function of a specific object to a set of parameters and returns the binding
*/
BoundFunctor<> BindObj(obj &, memfunctptr [, par1 [, par2 ...]]);
/*! \ingroup generic
Binds a member function of a specific object to a set of parameters and returns the binding as a newed pointer
*/
BoundFunctor<> BindObjN(obj &, memfunctptr [, par1 [, par2 ...]]);
#else
namespace Bind {
	template<int pars, typename fn> struct Impl
	{
		typedef typename FnInfo<fn>::asList fnspec;
		TL::instantiateH<typename fnspec::next> parvals;
		void checkParms()
		{
			FXSTATIC_ASSERT(TL::length<fnspec>::value-1==pars, Function_Spec_Not_Equal_To_Parameters_Specified);
		}
		Impl() {}
		template<typename P1> Impl(P1 p1)
		{
			TL::instance<0>(parvals).value=p1;
		}
		template<typename P1, typename P2> Impl(P1 p1, P2 p2)
		{
			TL::instance<0>(parvals).value=p1;
			TL::instance<1>(parvals).value=p2;
		}
		template<typename P1, typename P2, typename P3> Impl(P1 p1, P2 p2, P3 p3)
		{
			TL::instance<0>(parvals).value=p1;
			TL::instance<1>(parvals).value=p2;
			TL::instance<2>(parvals).value=p3;
		}
		template<typename P1, typename P2, typename P3, typename P4> Impl(P1 p1, P2 p2, P3 p3, P4 p4)
		{
			TL::instance<0>(parvals).value=p1;
			TL::instance<1>(parvals).value=p2;
			TL::instance<2>(parvals).value=p3;
			TL::instance<3>(parvals).value=p4;
		}
	};
}
template<typename fn> BoundFunctor<typename FnInfo<fn>::asList>
	BindFunc(fn fnptr)
{
	typedef typename Bind::Impl<0, fn> Impl;
	Impl impl;
	return BoundFunctor<typename Impl::fnspec>(fnptr, impl.parvals);
}
template<typename fn> BoundFunctor<typename FnInfo<fn>::asList> *
	BindFuncN(fn fnptr)
{
	typedef typename Bind::Impl<0, fn> Impl;
	Impl impl;
	return new BoundFunctor<typename Impl::fnspec>(fnptr, impl.parvals);
}
template<typename fn, typename P1> BoundFunctor<typename FnInfo<fn>::asList>
	BindFunc(fn fnptr, P1 p1)
{
	typedef typename Bind::Impl<1, fn> Impl;
	Impl impl(p1);
	return BoundFunctor<typename Impl::fnspec>(fnptr, impl.parvals);
}
template<typename fn, typename P1> BoundFunctor<typename FnInfo<fn>::asList> *
	BindFuncN(fn fnptr, P1 p1)
{
	typedef typename Bind::Impl<1, fn> Impl;
	Impl impl(p1);
	return new BoundFunctor<typename Impl::fnspec>(fnptr, impl.parvals);
}
template<typename fn, typename P1, typename P2> BoundFunctor<typename FnInfo<fn>::asList>
	BindFunc(fn fnptr, P1 p1, P2 p2)
{
	typedef typename Bind::Impl<2, fn> Impl;
	Impl impl(p1, p2);
	return BoundFunctor<typename Impl::fnspec>(fnptr, impl.parvals);
}
template<typename fn, typename P1, typename P2> BoundFunctor<typename FnInfo<fn>::asList> *
	BindFuncN(fn fnptr, P1 p1, P2 p2)
{
	typedef typename Bind::Impl<2, fn> Impl;
	Impl impl(p1, p2);
	return new BoundFunctor<typename Impl::fnspec>(fnptr, impl.parvals);
}
template<typename fn, typename P1, typename P2, typename P3> BoundFunctor<typename FnInfo<fn>::asList>
	BindFunc(fn fnptr, P1 p1, P2 p2, P3 p3)
{
	typedef typename Bind::Impl<3, fn> Impl;
	Impl impl(p1, p2, p3);
	return BoundFunctor<typename Impl::fnspec>(fnptr, impl.parvals);
}
template<typename fn, typename P1, typename P2, typename P3> BoundFunctor<typename FnInfo<fn>::asList> *
	BindFuncN(fn fnptr, P1 p1, P2 p2, P3 p3)
{
	typedef typename Bind::Impl<3, fn> Impl;
	Impl impl(p1, p2, p3);
	return new BoundFunctor<typename Impl::fnspec>(fnptr, impl.parvals);
}
template<typename fn, typename P1, typename P2, typename P3, typename P4> BoundFunctor<typename FnInfo<fn>::asList>
	BindFunc(fn fnptr, P1 p1, P2 p2, P3 p3, P4 p4)
{
	typedef typename Bind::Impl<4, fn> Impl;
	Impl impl(p1, p2, p3, p4);
	return BoundFunctor<typename Impl::fnspec>(fnptr, impl.parvals);
}
template<typename fn, typename P1, typename P2, typename P3, typename P4> BoundFunctor<typename FnInfo<fn>::asList> *
	BindFuncN(fn fnptr, P1 p1, P2 p2, P3 p3, P4 p4)
{
	typedef typename Bind::Impl<3, fn> Impl;
	Impl impl(p1, p2, p3, p4);
	return new BoundFunctor<typename Impl::fnspec>(fnptr, impl.parvals);
}
template<typename obj, typename fn> BoundFunctor<typename FnInfo<fn>::asList>
	BindObj(obj &objinst, fn fnptr)
{
	typedef typename Bind::Impl<0, fn> Impl;
	Impl impl;
	return BoundFunctor<typename Impl::fnspec>(objinst, fnptr, impl.parvals);
}
template<typename obj, typename fn> BoundFunctor<typename FnInfo<fn>::asList> *
	BindObjN(obj &objinst, fn fnptr)
{
	typedef typename Bind::Impl<0, fn> Impl;
	Impl impl;
	return new BoundFunctor<typename Impl::fnspec>(objinst, fnptr, impl.parvals);
}
template<typename obj, typename fn, typename P1> BoundFunctor<typename FnInfo<fn>::asList>
	BindObj(obj &objinst, fn fnptr, P1 p1)
{
	typedef typename Bind::Impl<1, fn> Impl;
	Impl impl(p1);
	return BoundFunctor<typename Impl::fnspec>(objinst, fnptr, impl.parvals);
}
template<typename obj, typename fn, typename P1> BoundFunctor<typename FnInfo<fn>::asList> *
	BindObjN(obj &objinst, fn fnptr, P1 p1)
{
	typedef typename Bind::Impl<1, fn> Impl;
	Impl impl(p1);
	return new BoundFunctor<typename Impl::fnspec>(objinst, fnptr, impl.parvals);
}
template<typename obj, typename fn, typename P1, typename P2> BoundFunctor<typename FnInfo<fn>::asList>
	BindObj(obj &objinst, fn fnptr, P1 p1, P2 p2)
{
	typedef typename Bind::Impl<2, fn> Impl;
	Impl impl(p1, p2);
	return BoundFunctor<typename Impl::fnspec>(objinst, fnptr, impl.parvals);
}
template<typename obj, typename fn, typename P1, typename P2> BoundFunctor<typename FnInfo<fn>::asList> *
	BindObjN(obj &objinst, fn fnptr, P1 p1, P2 p2)
{
	typedef typename Bind::Impl<2, fn> Impl;
	Impl impl(p1, p2);
	return new BoundFunctor<typename Impl::fnspec>(objinst, fnptr, impl.parvals);
}
template<typename obj, typename fn, typename P1, typename P2, typename P3> BoundFunctor<typename FnInfo<fn>::asList>
	BindObj(obj &objinst, fn fnptr, P1 p1, P2 p2, P3 p3)
{
	typedef typename Bind::Impl<3, fn> Impl;
	Impl impl(p1, p2, p3);
	return BoundFunctor<typename Impl::fnspec>(objinst, fnptr, impl.parvals);
}
template<typename obj, typename fn, typename P1, typename P2, typename P3> BoundFunctor<typename FnInfo<fn>::asList> *
	BindObjN(obj &objinst, fn fnptr, P1 p1, P2 p2, P3 p3)
{
	typedef typename Bind::Impl<3, fn> Impl;
	Impl impl(p1, p2, p3);
	return new BoundFunctor<typename Impl::fnspec>(objinst, fnptr, impl.parvals);
}
template<typename obj, typename fn, typename P1, typename P2, typename P3, typename P4> BoundFunctor<typename FnInfo<fn>::asList>
	BindObj(obj &objinst, fn fnptr, P1 p1, P2 p2, P3 p3, P4 p4)
{
	typedef typename Bind::Impl<4, fn> Impl;
	Impl impl(p1, p2, p3, p4);
	return BoundFunctor<typename Impl::fnspec>(objinst, fnptr, impl.parvals);
}
template<typename obj, typename fn, typename P1, typename P2, typename P3, typename P4> BoundFunctor<typename FnInfo<fn>::asList> *
	BindObjN(obj &objinst, fn fnptr, P1 p1, P2 p2, P3 p3, P4 p4)
{
	typedef typename Bind::Impl<4, fn> Impl;
	Impl impl(p1, p2, p3, p4);
	return new BoundFunctor<typename Impl::fnspec>(objinst, fnptr, impl.parvals);
}
#endif

namespace TL
{
	namespace dynamicAtHelper
	{
		template<typename fnspeclist, typename typelist, template<class> class instance> struct Impl
		{
			typedef typename FnFromList<fnspeclist>::value fnspec;
			static fnspec getArray(FXuint idx)
			{
				FXSTATIC_ASSERT(length<typelist>::value<=16, DynamicAt_Maximum_Exceeded);
				typedef instance<NullType> nullinst;
				if(idx>=length<typelist>::value)
					return &nullinst::Do;
				typedef typename apply<typelist, instance>::value instancedtypelist;
				typedef typename replicate<16-length<typelist>::value, instance<NullType> >::value instancedtypelistend;
				typedef typename append<instancedtypelist, instancedtypelistend>::value atypelist;

				static const fnspec mytable[16]={
					&at<atypelist, 0>::value::Do,			&at<atypelist, 1>::value::Do,
					&at<atypelist, 2>::value::Do,			&at<atypelist, 3>::value::Do,
					&at<atypelist, 4>::value::Do,			&at<atypelist, 5>::value::Do,
					&at<atypelist, 6>::value::Do,			&at<atypelist, 7>::value::Do,
					&at<atypelist, 8>::value::Do,			&at<atypelist, 9>::value::Do,
					&at<atypelist, 10>::value::Do,			&at<atypelist, 11>::value::Do,
					&at<atypelist, 12>::value::Do,			&at<atypelist, 13>::value::Do,
					&at<atypelist, 14>::value::Do,			&at<atypelist, 15>::value::Do
					};
				return mytable[idx];
			}
		};
	}
	/*! \struct dynamicAt
	\ingroup TL
	\brief Assembles code to call some templated code with the type from a typelist

	This generates at compile-time a jump table within the read-only section of the binary consisting
	of addresses of the static function <tt>instance<type>::Do</tt> where type is the
	type within the typelist from index 0 to the length of the typelist. At run-time
	the index specified is used to lookup the address of the function and to call it
	with any set of parameters. This allows you to very efficiently call type specialised
	code where the type is known only at run time and is a boon for writing modular
	extensible code which is both type safe and efficient. It also totally removes the
	bloat associated with sets of if...else statements as there is one routine & table reused
	by all instances of usage throughout a binary.

	If the search does not match (eg; the index is out of range), \em instance::Do instantiated with
	\c NullType is called. Note that like instantiateH parameters are always passed by value.

	Usage looks like as follows:
	\code
	template<typename type> struct Impl { static void Do(<pars>); }
	Generic::TL::dynamicAt<typelist, Impl>(typeIdx, <pars>);
	\endcode
	Note that there is a maximum of 16 entries per jump table as I couldn't figure out how
	to make it dynamically generated at compile time to any length. If anyone can advise,
	I'd be most grateful. In the meantime, you can handle > 16 entries by inheriting as many
	dynamicAt's as you require into a helper class and at construction, invoking those you
	don't need using the magic idx number which disables execution.
	*/
	template<typename typelist, template<class> class instance> struct dynamicAt
	{
		static const FXuint MaxEntries=16;			//!< Currently the maximum members permitted in a typelist
		static const FXuint DisableMagicIdx=1<<28;	//!< The magic idx value to disable dispatch
		//! Invoke \em instance with the type \em idx in the typelist
		dynamicAt(FXuint idx)
		{
			typedef typename create<void>::value parslist;
			if(DisableMagicIdx!=idx)
				dynamicAtHelper::Impl<parslist, typelist, instance>::getArray(idx)();
		}
		//! Invoke \em instance with the type \em idx in the typelist with parameters
		template<typename P1> dynamicAt(FXuint idx, P1 p1)
		{
			typedef typename create<void, P1>::value parslist;
			if(DisableMagicIdx!=idx)
				dynamicAtHelper::Impl<parslist, typelist, instance>::getArray(idx)(p1);
		}
		//! \overload
		template<typename P1, typename P2> dynamicAt(FXuint idx, P1 p1, P2 p2)
		{
			typedef typename create<void, P1, P2>::value parslist;
			if(DisableMagicIdx!=idx)
				dynamicAtHelper::Impl<parslist, typelist, instance>::getArray(idx)(p1,p2);
		}
		//! \overload
		template<typename P1, typename P2, typename P3> dynamicAt(FXuint idx, P1 p1, P2 p2, P3 p3)
		{
			typedef typename create<void, P1, P2, P3>::value parslist;
			if(DisableMagicIdx!=idx)
				dynamicAtHelper::Impl<parslist, typelist, instance>::getArray(idx)(p1,p2,p3);
		}
		//! \overload
		template<typename P1, typename P2, typename P3, typename P4> dynamicAt(FXuint idx, P1 p1, P2 p2, P3 p3, P4 p4)
		{
			typedef typename create<void, P1, P2, P3, P4>::value parslist;
			if(DisableMagicIdx!=idx)
				dynamicAtHelper::Impl<parslist, typelist, instance>::getArray(idx)(p1,p2,p3,p4);
		}

	};
}

/*! \struct MapBools
\ingroup generic
\brief Mapper of C++ bools to a bitfield

This generates customised code at compile-time mapping a series of \c bool's to
an unsigned integer suitable for serialisation and deserialisation through
a FX::FXStream. The correct integer (8, 16, 32 or 64 bit) is chosen automatically.
To use simply do:
\code
struct Foo { bool a,b,c; } foo;
FX::FXStream s;
FX::Generic::MapBools<3> boolmap(&foo.a);
s << boolmap;
s >> boolmap;
\endcode
*/
template<int len> struct MapBools
{
	typedef typename Generic::select<(len>sizeof(FXuchar)*8),
				typename Generic::select<(len>sizeof(FXushort)*8),
					typename Generic::select<(len>sizeof(FXuint)*8),
						StaticError< IntToType<len> >,
						FXulong>::value,
					FXushort>::value,
				FXuchar>::value
			holdtype;
	bool *base;
	MapBools(const bool *_base) throw() : base(const_cast<bool *>(_base)) { }

	// Avoid variable bit shifts as they're slow on x86
	friend inline FXStream &operator<<(FXStream &s, const MapBools<len> i) throw()
	{
		holdtype val=0;
		for(int n=len-1; n>=0; n--)
		{
			val=(val<<1)|((FXuchar)i.base[n]);
		}
		s << val;
		return s;
	}
	friend inline FXStream &operator>>(FXStream &s, MapBools<len> i) throw()
	{
		holdtype val; s >> val;
		for(int n=0; n<len; n++)
		{
			i.base[n]=(bool)(val & 1); val>>=1;
		}
		return s;
	}
};

/*! \class DoUndo
\ingroup generic
\brief Performs an action on construction and another action on destruction

This is a very useful class for generically calling some function on construction
and then some other on destruction (and thus is exception-safe). Obviously it
is intended for usage on the stack as an auto-scoped instance. During its lifetime you
can temporarily undo() and redo() the action.

Note that there are two forms of DoUndo - fast and slow. The fast takes
pointers to a function or member function which has no parameters and thus
costs one virtual method call per invocation. The slow takes a FX::Generic::BoundFunctor
and thus costs two virtual method calls per invocation. Obviously there are two
invocations per scope.

For the fast, passing \c void as the object type gets you a non-member function
invocation. The default parameters of all the template parameters gives you the slow
type. With those taking a bound functor (slow type), DoUndo takes ownership of the pointer
you pass.

\sa FX::FXRBObj, FX::FXRBFunc
*/
template<class obj=NullType, typename doaddr=NullType, typename undoaddr=NullType> class DoUndo
{
	bool done;
	obj *instance;
	doaddr doa;
	undoaddr undoa;
public:
	//! Constructs a do/undo instance, doing the action
	DoUndo(obj *_instance, doaddr _doa, undoaddr _undoa) : done(false), instance(_instance), doa(_doa), undoa(_undoa) { redo(); }
	//! Destructs a do/undo instance, undoing the action
	~DoUndo() { undo(); }
	//! Undoes the action
	void undo()
	{
		if(done)
		{
			(*instance.*undoa)();
			done=false;
		}
	}
	//! Redoes the action
	void redo()
	{
		if(!done)
		{
			(*instance.*doa)();
			done=true;
		}
	}
};
template<typename doaddr, typename undoaddr> class DoUndo<void, doaddr, undoaddr>
{
	bool done;
	doaddr doa;
	undoaddr undoa;
public:
	DoUndo(doaddr _doa, undoaddr _undoa) : done(false), doa(_doa), undoa(_undoa) { redo(); }
	~DoUndo() { undo(); }
	void undo()
	{
		if(done)
		{
			(undoa)();
			done=false;
		}
	}
	void redo()
	{
		if(!done)
		{
			(doa)();
			done=true;
		}
	}
};
template<> class DoUndo<NullType, NullType, NullType>
{
	bool done;
	BoundFunctorV *do_, *undo_;
public:
	DoUndo(BoundFunctorV *_do_, BoundFunctorV *_undo_)
		: done(false), do_(_do_), undo_(_undo_) { redo(); }
	~DoUndo() { undo(); FXDELETE(do_); FXDELETE(undo_); }
	void undo()
	{
		if(done)
		{
			(*undo_)();
			done=false;
		}
	}
	void redo()
	{
		if(!done)
		{
			(*do_)();
			done=true;
		}
	}
};

} // namespace

/*! Defined as a FX::Generic::ptr<FX::Pol::destructiveCopy> which means it works
semantically the same as std::auto_ptr. If you see this in a
parameter list, it means that the API takes ownership of the pointer - deleting it
if there's an exception as well as deleting it when done with the pointer. Usually
functions accepting a FXAutoPtr check their input for zero, thus you can use \c new
directly inside the API eg;
\code
type *foo(FXAutoPtr<type> ptr);
...
type *handle=foo(new type);
\endcode
*/
#define FXAutoPtr FX::Generic::ptr	// TODO: FIXME when templated typedefs get implemented

/*! \class FXUnicodify
\brief Converts a UTF-8 format FX::FXString into a unicode string suitable for the
host operating system

\parameter fastlen How much stack space to use before resorting to the allocator

On POSIX, it is assumed that the host OS also uses UTF-8 and thus returns the FXString
as-is. On Windows, if \c UNICODE is defined, then it converts to UTF-16 otherwise it
returns the FXString as-is. In all cases the terminating null of the source FXString
is preserved.

If you are converting a path which can utilise the \\\\?\\ escape sequence to enable
32,768 character long paths, pass true for the \em isPath parameter to the constructor.

Example:
\code
CreateFile(FXUnicodify<>(path, true).buffer(), ...
\endcode
*/
#if defined(WIN32) && defined(UNICODE)
template<size_t fastlen=2048> class FXUnicodify
{
	bool myIsPath;
	FXnchar stkbuff[fastlen], *mybuffer;
	FXAutoPtr<FXnchar> membuff;
	FXint bufflen;
	void doConv(const FXString &_str)
	{
		FXString str(_str);
		if(myIsPath)
		{
			str=FXPath::absolute(str);
			str.prepend("\\\\?\\");
			// Also all \ must be /
			str.substitute('/', '\\');
		}
		FXint strlen=str.length()+1;	// Outputted buffer will always be shorter than this
		if(strlen>sizeof(stkbuff))
			FXERRHM(membuff=mybuffer=new FXnchar[strlen]);
		else mybuffer=stkbuff;
		bufflen=utf2ncs(mybuffer, str.text(), strlen)*sizeof(FXnchar);
	}
public:
	//! Constructs an instance
	FXUnicodify(bool isPath=false) : myIsPath(isPath), mybuffer(0), bufflen(0) { }
	//! Constructs an instance converting \em str, prepending \c \\\\?\\ to enable 32,768 character paths if \em isPath is set
	FXUnicodify(const FXString &str, bool isPath=false) : myIsPath(isPath), mybuffer(0), bufflen(0) { doConv(str); }
	//! Returns the converted string
	const FXnchar *buffer() const throw() { return mybuffer; }
	//! Returns the converted string
	const FXnchar *buffer(const FXString &str) { if(!mybuffer) doConv(str); return mybuffer; }
	//! Returns the length of the converted string in bytes
	FXint length() const throw() { return bufflen; }
	//! Returns the length of the converted string in bytes
	FXint length(const FXString &str) { if(!mybuffer) doConv(str); return bufflen; }
};
#else
template<size_t fastlen=2048> class FXUnicodify
{
	const FXchar *mybuffer;
	FXint bufflen;
public:
	FXUnicodify(bool isPath=false) : mybuffer(0) { }
	FXUnicodify(const FXString &str, bool isPath=false) : mybuffer(str.text()), bufflen(str.length()) { }
	const FXchar *buffer() const throw() { return mybuffer; }
	const FXchar *buffer(const FXString &str) const throw() { return str.text(); }
	FXint length() const throw() { return bufflen; }
	FXint length(const FXString &str) const throw() { return str.length(); }
};
#endif

} // namespace

#endif
