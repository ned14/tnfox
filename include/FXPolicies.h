/********************************************************************************
*                                                                               *
*                                Policy classes                                 *
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

#ifndef FXPOLICIES_H
#define FXPOLICIES_H

#include "FXException.h"

namespace FX {

/*! \file FXPolicies.h
\brief Defines a number of policy classes used elsewhere in TnFOX
*/

class QMtxHold;

namespace Pol {

/*! \class None0
\brief Policy taking no types specifying that there is no policy here
*/
class None0
{
protected:
	None0() { }
	~None0() { }
};
/*! \class None1
\brief Policy taking one type specifying that there is no policy here
*/
template<class type> class None1
{
protected:
	None1() { }
	~None1() { }
};

/*! \class deepCopy
\brief Policy specifying that an object pointee should be deep copied
\note Requires that the object provides a \c copy() method which is
usually virtual.

\sa FX::Generic::ptr
*/
template<class type> class deepCopy
{
protected:
	type *data;
public:
	deepCopy(type *d) : data(d) { }
	deepCopy(const deepCopy &o) : data(o.data->copy()) { FXERRHM(data); }
	deepCopy &operator=(const deepCopy &o)
	{
		FXDELETE(data);
		FXERRHM(data=o.data->copy());
		return *this;
	}
	~deepCopy() { FXDELETE(data); }
	friend type *PtrPtr(deepCopy &p) { return p.data; }
	friend const type *PtrPtr(const deepCopy &p) { return p.data; }
	friend type *&PtrRef(deepCopy &p) { return p.data; }
	friend const type *PtrRef(const deepCopy &p) { return p.data; }
};

/*! \class refCounted
\brief Policy specifying that an object pointee should be reference counted
\note Requires that the object provides a \c refCount() method which
returns a reference to an int-like entity eg; an int itself or FX::FXAtomicInt

\sa FX::Generic::ptr
*/
template<class type> class refCounted
{
protected:
	type *data;
public:
	refCounted(type *d) : data(d) { ++data->refCount(); }
	refCounted(const refCounted &o) : data(o.data) { ++data->refCount(); }
	refCounted &operator=(const refCounted &o)
	{
		if(!--data->refCount()) FXDELETE(data);
		data=o.data;
		++data->refCount();
		return *this;
	}
	~refCounted()
	{
		if(!--data->refCount()) FXDELETE(data);
	}
	friend type *PtrPtr(refCounted &p) { return p.data; }
	friend const type *PtrPtr(const refCounted &p) { return p.data; }
	friend type *&PtrRef(refCounted &p) { return p.data; }
	friend const type *PtrRef(const refCounted &p) { return p.data; }
};

/*! \class destructiveCopy
\brief Policy specifying that an object pointee should be copied destructively

\sa FX::Generic::ptr
*/
template<class type> class destructiveCopy
{
protected:
	mutable type *data;
public:
	destructiveCopy(type *d) : data(d) { }
	destructiveCopy(const destructiveCopy &o) : data(o.data) { o.data=0; }
	destructiveCopy &operator=(const destructiveCopy &o)
	{
		FXDELETE(data);
		data=o.data;
		o.data=0;
		return *this;
	}
	~destructiveCopy() { FXDELETE(data); }
	friend type *PtrPtr(destructiveCopy &p) { return p.data; }
	friend const type *PtrPtr(const destructiveCopy &p) { return p.data; }
	friend type *&PtrRef(destructiveCopy &p) { return p.data; }
	friend const type *PtrRef(const destructiveCopy &p) { return p.data; }
};

/*! \class noCopy
\brief Policy specifying that an object pointee should not be copied

\sa FX::Generic::ptr
*/
template<class type> class noCopy
{
protected:
	type *data;
public:
	noCopy(type *d) : data(d) { }
	~noCopy() { FXDELETE(data); }
	friend type *PtrPtr(noCopy &p) { return p.data; }
	friend const type *PtrPtr(const noCopy &p) { return p.data; }
	friend type *&PtrRef(noCopy &p) { return p.data; }
	friend const type *PtrRef(const noCopy &p) { return p.data; }
};

/*! \struct itMove
\brief Policy specifying how to move iterator referenced data

This moves the item at \em from to \em to. \em from
is set to the newly inserted item on exit.
*/
template<class type> struct itMove
{
	template<class L, class I> void move(L &list, I to, I &from) const
	{
		list.splice(to, list, from);
		from=--to;
	}
};
/*! \struct itSwap
\brief Policy specifying how to swap iterator referenced data

This swaps two items by relinking surrounding nodes. Both
\em a and \em b are set to the newly inserted items on exit.
*/
template<class type> struct itSwap
{
	template<class L, class I> void swap(L &list, I &a, I &b) const
	{
		I _a=a, _b=b; ++_a; ++_b;
		if(_b==a)
		{
			list.splice(b, list, a);
			a=--_a; b=--_a;
		}
		else if(_a==b)
		{
			list.splice(a, list, b);
			b=--_b; a=--_b;
		}
		else
		{
			list.splice(b, list, a);
			list.splice(_a, list, b);
			a=--_a; b=--_b;
		}
	}
};
/*! \struct itCompare
\brief Policy specifying how to compare iterator referenced data

This compares two items by dereferencing them and calling the <
operator
*/
template<class type> struct itCompare
{
	template<class L, class I> bool compare(L &list, I &a, I &b) const
	{
		return *a<*b;
	}
};
/*! \struct itRevCompare
\brief Policy specifying how to compare iterator referenced data

This compares two items by dereferencing them and calling the >
operator
*/
template<class type> struct itRevCompare
{
	template<class L, class I> bool compare(L &list, I &a, I &b) const
	{
		return *a>*b;
	}
};

} // namespace

} // namespace

#endif
