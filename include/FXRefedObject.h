/********************************************************************************
*                                                                               *
*                          Reference counted objects                            *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2002,2003 by Niall Douglas.   All Rights Reserved.       *
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

#ifndef FXREFEDOBJECT_H
#define FXREFEDOBJECT_H

#include "FXException.h"
#include "FXPolicies.h"
#include "qvaluelist.h"

namespace FX {

/*! \file FXRefedObject.h
\brief Defines classes useful for implementing handles to objects
*/

/*! \def FXREFINGOBJECT_DEBUGKNOWALLOC [0|1]
If defined to 1, FX::FXRefingObject stores where in the program source it was created.
Useful for tracking down lost handles to things. By default enabled in debug builds.
To use, do something like the following:
\code
#if FXREFINGOBJECT_DEBUGKNOWALLOC
 #define FXRefingObject(a) FXRefingObject(a, __FILE__, __LINE__)
#endif
\endcode
*/
#ifndef FXREFINGOBJECT_DEBUGKNOWALLOC
 #ifdef DEBUG
  #define FXREFINGOBJECT_DEBUGKNOWALLOC 1
 #else
  #define FXREFINGOBJECT_DEBUGKNOWALLOC 0
 #endif
#endif

class FXRefingObjectBase
{
protected:
#if FXREFINGOBJECT_DEBUGKNOWALLOC
	struct Allocated
	{
		const char *file;
		int lineno;
		Allocated(const char *_file, int _lineno) : file(_file), lineno(_lineno) { }
	} allocated;
	FXRefingObjectBase(const char *_file, int _lineno) : allocated(_file, _lineno) { }
public:
	FXRefingObjectBase &operator=(const FXRefingObjectBase &o)
	{
		if(o.allocated.file)
		{
			allocated.file=o.allocated.file;
			allocated.lineno=o.allocated.lineno;
		}
		return *this;
	}
	const char *int_allocated_file() const throw() { return allocated.file; }
	int int_allocated_lineno() const throw() { return allocated.lineno; }
#else
	FXRefingObjectBase(const char *_file, int _lineno) { }
public:
	const char *int_allocated_file() const throw() { return 0; }
	int int_allocated_lineno() const throw() { return 0; }
#endif
};
template<class type> class FXRefingObject;
namespace FXRefingObjectImpl {
	template<bool mutexed, class type> struct dataHolderI;
	template<class type> class refedObject;
}

namespace FXRefedObjectImpl
{
	template<typename type> class countHolder
	{
		type myrefcount;
		bool mydying;
	protected:
		bool int_increfcount()		// Returns true if not dead
		{
			if(mydying) return false;
			++myrefcount;
			return true;
		}
		bool int_decrefcount()		// Returns false if kill me now please
		{
			if(!--myrefcount && !mydying)
			{
				mydying=true;
				return false;
			}
			return true;
		}
	public:
		countHolder() : myrefcount(0), mydying(false) { }
		countHolder(const countHolder &o) : myrefcount(0), mydying(false) { }
		//! Returns the reference count of this object
		type &refCount() throw() { return myrefcount; }
		//! \overload
		const type &refCount() const throw() { return myrefcount; }
	};
	template<> class countHolder<FXAtomicInt>
	{	/* This specialisation for FXAtomicInt features a serialised checking and
		setting of dying so we can know when you can't create a new reference safely
		in a threadsafe fashion. */
		FXShrdMemMutex myrefcountlock;
		int myrefcount;
		bool mydying;
	protected:
		bool int_increfcount()		// Returns true if not dead
		{
			myrefcountlock.lock();
			if(mydying)
			{
				myrefcountlock.unlock();
				return false;
			}
			++myrefcount;
			myrefcountlock.unlock();
			return true;
		}
		bool int_decrefcount()		// Returns false if kill me now please
		{
			myrefcountlock.lock();
			if(!--myrefcount && !mydying)
			{
				mydying=true;
				myrefcountlock.unlock();
				return false;
			}
			myrefcountlock.unlock();
			return true;
		}
	public:
		countHolder() : myrefcountlock(FXINFINITE), myrefcount(0), mydying(false) { }
		countHolder(const countHolder &o) : myrefcountlock(FXINFINITE), myrefcount(0), mydying(false) { }
		// Deliberately prevent direct alteration
		const int &refCount() const throw() { return myrefcount; }
	};
}

namespace Pol {
	/*! \struct unknownReferrers
	\brief Policy specifying that the refed object does not know its referrers
	\sa FX::FXRefedObject
	*/
	struct unknownReferrers
	{
	protected:
		struct ReferrerEntry { };
		unknownReferrers() { }
		void int_addReferrer(FXRefingObjectBase *r) { }
		void int_removeReferrer(FXRefingObjectBase *r) { }
		const QValueList<ReferrerEntry> *int_referrers() const { return 0; }
	};
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
	/*! \struct knowReferrers
	\brief Policy specifying that the refed object should know its referrers
	\sa FX::FXRefedObject
	*/
	struct knowReferrers
	{
	protected:
		struct ReferrerEntry
		{
			FXRefingObjectBase *ref;
			FXulong threadId;
			ReferrerEntry(FXRefingObjectBase *_ref) : ref(_ref), threadId(FXThread::id()) { }
			bool operator==(const ReferrerEntry &o) const throw() { return ref==o.ref; }
		};
		QValueList<ReferrerEntry> referrers;
		knowReferrers() { }
		void int_addReferrer(FXRefingObjectBase *r)
		{
			referrers.push_back(ReferrerEntry(r));
		}
		void int_removeReferrer(FXRefingObjectBase *r)
		{
			referrers.remove(ReferrerEntry(r));
		}
		const QValueList<ReferrerEntry> *int_referrers() const { return &referrers; }
	};
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	/*! \struct hasLastUsed
	\brief Policy specifying that the refed object maintains when it was last accessed
	\sa FX::FXRefedObject
	*/
	struct hasLastUsed
	{
	private:
		mutable FXuint lastusedtimestamp;
	protected:
		hasLastUsed() : lastusedtimestamp(0) { }
	public:
		/*! Returns the FXProcess::getMsCount() timestamp of the last time this
		object was accessed by a FXRefingObject
		*/
		FXuint lastUsed() const throw() { return lastusedtimestamp; }
	};
}
/*! \class FXRefedObject
\brief The parent class of something which is reference counted

\param intType The type of the container maintaining the reference count. Usually
\c int or FX::FXAtomicInt
\param lastUsed Can be FX::Pol::hasLastUsed
\param referrersPolicy Either FX::Pol::unknownReferrers or FX::Pol::knowReferrers

This is the base class for something which is reference counted - when the count
reaches zero, the virtual method noMoreReferrers() is called which by default calls
<tt>delete this</tt>. You of course should override this with anything you like.
noMoreReferrers() will never be called more than once and once it has been called,
even if the object is not deleted all attempts to open a new reference upon it
will return a null reference. If you don't want this, specialise FX::FXRefedObjectImpl::countHolder
(see the header file)

A specialisation exists for FX::FXAtomicInt which implements an atomic reference
count (despite not actually using FX::FXAtomicInt directly to do it). 
\sa FX::FXRefingObject
*/
template<typename intType,
	class lastUsed=Pol::None0,
	class referrersPolicy=Pol::unknownReferrers> class FXRefedObject : public FXRefedObjectImpl::countHolder<intType>, public lastUsed, private referrersPolicy
{
	template<class type> friend class FXRefingObjectImpl::refedObject;
	template<bool mutexed, class type> friend struct FXRefingObjectImpl::dataHolderI;
	typedef FXRefedObject MyFXRefedObjectSpec;
public:
	FXRefedObject() { }
	virtual ~FXRefedObject() { }
	FXRefedObject(const FXRefedObject &o) : lastUsed(o), referrersPolicy() { /* new instance */ }
protected:
	//! Called when the reference count reaches zero
	virtual void noMoreReferrers() { delete this; }
	using referrersPolicy::ReferrerEntry;
	using referrersPolicy::int_referrers;
};

namespace FXRefingObjectImpl {
	template<bool mutexed, class type> struct dataHolderI
	{
		type *data;
		dataHolderI(type *p) : data(p) { }
		inline void addRef();
		inline void delRef();
	};
	template<class type> struct dataHolderI<true, type>
	{
		type *data;
		dataHolderI(type *p) : data(p) { }
		inline void addRef();
		inline void delRef();
	};
	template<class type> struct dataHolder
		: public dataHolderI<Generic::convertible<FXMutex &, type &>::value, type>
	{
		dataHolder(type *p) : dataHolderI<Generic::convertible<FXMutex &, type &>::value, type>(p) { }
	};
	template<bool hasLastUsed, class type> class lastUsedI : protected dataHolder<type>
	{
	protected:
		lastUsedI(type *p) : dataHolder<type>(p) { }
		type *accessData() const throw() { return dataHolder<type>::data; }
	};
	template<class type> class lastUsedI<true, type> : protected dataHolder<type>
	{
	protected:
		lastUsedI(type *p) : dataHolder<type>(p) { }
		type *accessData() const
		{
			if(dataHolder<type>::data) dataHolder<type>::data->lastusedtimestamp=FXProcess::getMsCount();
			return dataHolder<type>::data;
		}
	};
	template<class type> class refedObject
		: private lastUsedI<Generic::convertible<Pol::hasLastUsed &, type &>::value, type>
	{
		template<bool, class> friend struct FXRefingObjectImpl::dataHolderI;
		typedef lastUsedI<Generic::convertible<Pol::hasLastUsed &, type &>::value, type> Base;
		void inc()
		{
			if(Base::data)
			{
				if(Base::data->int_increfcount())
					Base::addRef();
				else
					Base::data=0;
			}
		}
		void dec()
		{
			if(Base::data)
			{
				Base::delRef();
				if(!Base::data->int_decrefcount())
					static_cast<typename type::MyFXRefedObjectSpec *>(Base::data)->noMoreReferrers();
				Base::data=0;			// No longer usuable
			}
		}
	public:
		refedObject(type *d) : Base(d) { inc(); }
		refedObject(const refedObject &o) : Base(o) { inc(); }
		refedObject &operator=(const refedObject &o)
		{
			dec();
			Base::data=o.data;
			inc();
			return *this;
		}
		~refedObject()
		{
			dec();
		}
		friend type *PtrPtr(refedObject &p) { return p.accessData(); }
		friend const type *PtrPtr(const refedObject &p) { return p.accessData(); }
		friend type *&PtrRef(refedObject &p) { return p.data; }
		friend const type *PtrRef(const refedObject &p) { return p.data; }
	};

} // namespace

/*! \class FXRefingObject
\brief A handle to a reference counted object

\param type The type of the data to be reference counted. This must inherit
FX::FXRefedObject

FX::FXRefingObject and FX::FXRefedObject are a pair of classes which denote
a handle and reference counted object respectively. The basic premise is that
a FXRefedObject subclass maintains a count incremented by the construction
of a corresponding FXRefingObject pointing to it and decremented by the same's
destruction. If the count reaches zero, the virtual method
FX::FXRefedObject::noMoreReferrers() is called which typically deletes the object.

In v0.6 of TnFOX the old implementation of this taken from Tornado was replaced
with a much superior version based on FX::Generic::ptr. This has enabled a much
greater degree of self-determination - now FXRefingObject configures itself
automatically according to how you configure its FXRefedObject - you no longer
need to reproduce the policies. Also when \em type inherits a FX::FXMutex that
mutex is automatically locked around modifications to the referrers list if
enabled.

The basic form is as follows:
\code
class Foo : public FXRefedObject<FXAtomicInt>, public FXMutex
{
   bool foo();
};
typedef FXRefingObject<Foo> FooHold;
...
bool f(Foo *f)
{
   FooHold fh(f);
   return fh->foo();
}
\endcode
In addition to the usual \c PtrPtr(), \c PtrRef(), \c PtrRelease() and \c PtrReset()
helper functions there is a \c PtrDetach() which returns a handle referring to a \c new
copy constructed instance of the pointee.

FXRefingObject can work with FX::FXLRUCache to implement a LRU caching
system of reference counted objects.
\sa FX::FXRefedObject, FX::FXLRUCache
*/

template<class type> class FXRefingObject
	: public FXRefingObjectBase, public Generic::ptr<type, FXRefingObjectImpl::refedObject>
{
public:
	//! Constructs an instance holding a reference to \em data
	FXRefingObject(type *data=0, const char *file=0, int lineno=0)
		: FXRefingObjectBase(file, lineno), Generic::ptr<type, FXRefingObjectImpl::refedObject>(data) { }
	explicit FXRefingObject(FXAutoPtr<type> &ptr)
		: FXRefingObjectBase(0, 0), Generic::ptr<type, FXRefingObjectImpl::refedObject>(ptr) { }
#if FXREFINGOBJECT_DEBUGKNOWALLOC
 #if 1
	// Prefer original info over where copied
	FXRefingObject(const FXRefingObject &o, const char *file=0, int lineno=0)
		: FXRefingObjectBase(o.allocated.file ? o.allocated.file : file, o.allocated.file ? o.allocated.lineno : lineno), Generic::ptr<type, FXRefingObjectImpl::refedObject>(o) { }
 #else
	// Prefer where copied over original info
	FXRefingObject(const FXRefingObject &o, const char *file=0, int lineno=0)
		: FXRefingObjectBase(file ? file : o.allocated.file, file ? lineno : o.allocated.lineno), Generic::ptr<type, FXRefingObjectImpl::refedObject>(o) { }
 #endif
#else
	FXRefingObject(const FXRefingObject &o, const char *file=0, int lineno=0)
		: FXRefingObjectBase(0, 0), Generic::ptr<type, FXRefingObjectImpl::refedObject>(o) { }
#endif
	/*! Makes a copy of the referenced object and returns a holder pointing to
	the new copy
	*/
	friend FXRefingObject PtrDetach(FXRefingObject &p)
	{
		FXAutoPtr<type> v;
		type *pv=PtrPtr(p);
		assert(pv);
		FXERRHM(v=new type(*pv));
		return FXRefingObject(v);
	}
};

namespace FXRefingObjectImpl
{	// Now that we know FXRefingObject's relationship to its policy classes, we can static_cast<>
	template<bool mutexed, class type> inline void dataHolderI<mutexed, type>::addRef()
	{
		data->int_addReferrer(static_cast<FXRefingObject<type> *>(this));
	}
	template<bool mutexed, class type> inline void dataHolderI<mutexed, type>::delRef()
	{
		data->int_removeReferrer(static_cast<FXRefingObject<type> *>(this));
	}
	template<class type> inline void dataHolderI<true, type>::addRef()
	{
		FXMtxHold h(data);
		data->int_addReferrer(static_cast<FXRefingObject<type> *>(this));
	}
	template<class type> inline void dataHolderI<true, type>::delRef()
	{
		FXMtxHold h(data);
		data->int_removeReferrer(static_cast<FXRefingObject<type> *>(this));
	}
}

} // namespace

#endif
