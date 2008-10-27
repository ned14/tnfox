/********************************************************************************
*                                                                               *
*                            Q D i c t   T h u n k                              *
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

#ifndef QDICT_H
#define QDICT_H

#include "qdictbase.h"

namespace FX {

/*! \file qdict.h
\brief Defines an implementation of Qt's QDict
*/

/*! \class QDict
\ingroup QTL
\brief An implementation of Qt's QDict

This implements a hash dictionary based on the STL but not using hash_multimap
which comes with an average complexity approaching O(1) - however there is
significantly more overhead to search this than a simple list and so it only
really shines when it contains a lot of items.
It should be near-optimum as when a collision occurs it uses a binary search
through a sorted list to maximise efficiency even when the dictionary is extremely
full. This means this container is suitable for very small hash table sizes or
putting it another way, if the hash table is sized one then the container becomes
a pure binary searched (O(log n) complexity) container.

When choosing a list size, choose a prime number and one not close to a
power of two when possible. Always choose an odd number. See FX::fx2powerprimes().

Like Qt's QDict, ours maintains a list of all iterators traversing it.
The reason a list must be maintained is to handle removals during iteration -
without updating the iterators, the complex internal structure would require
all iterators to become invalid (which would be very inefficient with large
dictionaries never mind tricky to have disparate iterator owners inform each
other of iterator invalidation).

Unlike Qt's QDict, removing items can invalidate iterators if you have lots of
items stored per key. This could be fixed with extra implementational
overhead (ask if you need it).

You may find the QDICTDYNRESIZE() and QDICTDYNRESIZEAGGR() macros useful but
bear in mind iterators are thrown away when resize() is performed.

\note When case insensitive compares are enabled, all keys returned by
QDictIterator will be lower case. This results from std::map not permitting a
user value to be passed to the sort function and thus it can't determine
whether to do a case insensitive comparison :(
*/
template<class type> class QDict : public QDictBase<FXString, type>
{
	typedef QDictBase<FXString, type> Base;
	bool checkcase;
	FXuint hash(const FXString &str) const
	{	// Fast hash as QDictBase does its own hashing
		return str.hash();
		//const FXchar *txt=str.text();
		//const FXchar *end=txt+str.length();
		//FXuint h=0;
		//for(int n=0; end>=txt && n<6; n++, end--)
		//{
		//	h=(h & 0xffffffc0)^(h<<5)^((*end)-32);
		//}
		//return h;
	}
public:
	enum { HasSlowKeyCompare=true };
	//! Creates a hash table indexed by FXString's. Choose a prime for \em size
	explicit QDict(int size=13, bool caseSensitive=true, bool wantAutoDel=false)
		: checkcase(caseSensitive), Base(size, wantAutoDel)
	{
	}
	QDict(const QDict<type> &o) : checkcase(o.checkcase), Base(o) { }
	~QDict() { Base::clear(); }
	FXADDMOVEBASECLASS(QDict, Base)
	//! Returns if case sensitive key comparisons is enabled
	bool caseSensitive() const throw() { return checkcase; }
	//! Sets if case sensitive key comparisons is enabled
	void setCaseSensitive(bool c) throw() { checkcase=c; }
	//! Inserts item \em d into the dictionary under key \em k
	void insert(const FXString &k, const type *d)
	{
		if(checkcase)
			Base::insert(hash(k), k, const_cast<type *>(d));
		else
		{
			FXString key(k);
			key.lower();
			Base::insert(hash(key), key, const_cast<type *>(d));
		}
	}
	//! Replaces item \em d in the dictionary under key \em k
	void replace(const FXString &k, const type *d)
	{
		if(checkcase)
			Base::replace(hash(k), k, const_cast<type *>(d));
		else
		{
			FXString key(k);
			key.lower();
			Base::replace(hash(key), key, const_cast<type *>(d));
		}
	}
	//! Deletes the most recently placed item in the dictionary under key \em k
	bool remove(const FXString &k)
	{
		if(checkcase)
			return Base::remove(hash(k), k);
		else
		{
			FXString key(k);
			key.lower();
			return Base::remove(hash(key), key);
		}
	}
	//! Removes the most recently placed item in the dictionary under key \em k without auto-deletion
	type *take(const FXString &k)
	{
		if(checkcase)
			return Base::take(hash(k), k);
		else
		{
			FXString key(k);
			key.lower();
			return Base::take(hash(key), key);
		}
	}
	//! Finds the most recently placed item in the dictionary under key \em k
	type *find(const FXString &k) const
	{
		if(checkcase)
			return Base::find(hash(k), k);
		else
		{
			FXString key(k);
			key.lower();
			return Base::find(hash(key), key);
		}
	}
	//! \overload
	type *operator[](const FXString &k) const { return find(k); }
protected:
	virtual void deleteItem(type *d);
};

template<class type> inline void QDict<type>::deleteItem(type *d)
{
	if(Base::autoDelete())
	{
		//fxmessage("QDB delete %p\n", d);
		delete d;
	}
}
// Don't delete void *
template<> inline void QDict<void>::deleteItem(void *)
{
}

/*! \class QDictIterator
\ingroup QTL
\brief An iterator for a QDict
*/
template<class type> class QDictIterator : public QDictBaseIterator<FXString, type>
{
public:
	QDictIterator() { }
	QDictIterator(const QDict<type> &d) : QDictBaseIterator<FXString, type>(d) { }
};

//! Writes the contents of the dictionary to stream \em s
template<class type> FXStream &operator<<(FXStream &s, const QDict<type> &i)
{
	FXuint mysize=i.count();
	s << mysize;
	for(QDictIterator<type> it(i); it.current(); ++it)
	{
		s << it.currentKey();
		s << *it.current();
	}
	return s;
}
//! Reads a dictionary from stream \em s
template<class type> FXStream &operator>>(FXStream &s, QDict<type> &i)
{
	FXuint mysize;
	s >> mysize;
	i.clear();
	FXString key;
	for(FXuint n=0; n<mysize; n++)
	{
		type *item;
		FXERRHM(item=new type);
		s >> key;
		s >> *item;
		i.insert(key, item);
	}
	return s;
}

} // namespace

#endif
