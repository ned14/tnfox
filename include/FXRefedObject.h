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

class FXRefingObjectBase {};
template<class type> class FXRefingObject;
namespace FXRefingObjectImpl {
	template<bool mutexed, class type> struct dataHolderI;
	template<class type> class refedObject;
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
			FXuint threadId;
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
\sa FX::FXRefingObject
*/
template<typename intType,
	class lastUsed=Pol::None0,
	class referrersPolicy=Pol::unknownReferrers> class FXRefedObject : public lastUsed, private referrersPolicy
{
	template<class type> friend class FXRefingObjectImpl::refedObject;
	template<bool mutexed, class type> friend struct FXRefingObjectImpl::dataHolderI;
	bool dying;
	intType myrefcount;
public:
	FXRefedObject() : dying(false), myrefcount(0) { }
	virtual ~FXRefedObject() { }
	FXRefedObject(const FXRefedObject &o) : lastUsed(o), referrersPolicy(), dying(false), myrefcount(0) { /* new instance */ }
protected:
	//! Called when the reference count reaches zero
	virtual void noMoreReferrers() { delete this; }
	using referrersPolicy::ReferrerEntry;
	using referrersPolicy::int_referrers;
public:
	//! Returns the reference count of this object
	intType &refCount() throw() { return myrefcount; }
	//! \overload
	const intType &refCount() const throw() { return myrefcount; }
};

namespace FXRefingObjectImpl {
	template<bool mutexed, class type> struct dataHolderI
	{
		type *data;
		dataHolderI(type *p) : data(p) { }
		void addRef() { data->int_addReferrer((FXRefingObjectBase *) this); }
		void delRef() { data->int_removeReferrer((FXRefingObjectBase *) this); }
	};
	template<class type> struct dataHolderI<true, type>
	{
		type *data;
		dataHolderI(type *p) : data(p) { }
		void addRef()
		{
			FXMtxHold h(data);
			data->int_addReferrer((FXRefingObjectBase *) this);
		}
		void delRef()
		{
			FXMtxHold h(data);
			data->int_removeReferrer((FXRefingObjectBase *) this);
		}
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
		typedef lastUsedI<Generic::convertible<Pol::hasLastUsed &, type &>::value, type> Base;
		void inc()
		{
			if(Base::data)
			{
				++Base::data->refCount();
				Base::addRef();
			}
		}
		void dec()
		{
			if(Base::data)
			{
				Base::delRef();
				if(!--Base::data->refCount() && !Base::data->dying)
				{
					Base::data->dying=true;
					Base::data->noMoreReferrers();
					Base::data=0;
				}
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
	FXRefingObject(type *data=0) : Generic::ptr<type, FXRefingObjectImpl::refedObject>(data) { }
	explicit FXRefingObject(FXAutoPtr<type> &ptr) : Generic::ptr<type, FXRefingObjectImpl::refedObject>(ptr) { }
	FXRefingObject(const FXRefingObject &o) : Generic::ptr<type, FXRefingObjectImpl::refedObject>(o) { }
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

} // namespace

#endif
