/********************************************************************************
*                                                                               *
*                 Q t   P o i n t e r   V e c t o r   T h u n k                 *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003 by Niall Douglas.   All Rights Reserved.                   *
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

#ifndef QPTRVECTOR_H
#define QPTRVECTOR_H

#if _MSC_VER==1200
#pragma warning(disable: 4786)
#endif

#include <vector>
#include <algorithm>
#include "fxdefs.h"
#include "FXStream.h"
#include "FXException.h"

namespace FX {

typedef FXuint uint;

/*! \file QPtrVector.h
\brief Defines a thunk of Qt's QPtrVector to the STL

To aid porting of Qt programs to FOX, this file defines a QPtrVector & QVector
via the STL
*/

/*! \class QPtrVector
\ingroup QTL
\brief A thunk of Qt's QPtrVector to the STL

A QPtrVector should be used when random access to elements needs to be quick.
Insertions to and removals from the start of the list in particular are slow.
Iterators are also invalidated whenever an insertion or removal is made. If
you want something different, look into FX::QPtrList. This
list is effectively a FX::QMemArray but with pointers and auto-deletion.

I've gone a bit further than Qt's QPtrVector as I've implemented all the QPtrList
methods too - this was because the QPtrList code was used to make QPtrVector so
it was trivial. Thus you can drop in a QPtrVector where a QPtrList used to be.
I did however remove prepend() & removeFirst() as that's a major performance
hurt with this container - use insert(0, x) or remove(0) instead. Qt doesn't
seem to provide a QPtrVectorIterator, but we have.

Note that all sorts of unpleasantness has been used to get this work, including
\c const_cast<>. Also, because of the nature of templates not being compiled until
they are used, I may not have caught all the compile errors yet :(

\warning This list is much less forgiving of range errors than Qt. Where Qt returns
false and prints a message, this list like the STL throws an exception!
*/
template<class type> class QPtrVectorIterator;
template<class type> class QPtrVector : private std::vector<type *>
{
	bool autodel;
public:
	explicit QPtrVector(bool wantAutoDel=false) : autodel(wantAutoDel), std::vector<type *>() {}
	explicit QPtrVector(std::vector<type *> &l) : autodel(false), std::vector<type *>(l) {}
	~QPtrVector()	{ clear(); }
	FXADDMOVEBASECLASS(QPtrVector, std::vector<type *>)
	//! Returns if auto-deletion is enabled
	bool autoDelete() const { return autodel; }
	//! Sets if auto-deletion is enabled
	void setAutoDelete(bool a) { autodel=a; }

	//! Returns the raw array of pointers
	type **data() const { return &std::vector<type *>::front(); }
	using std::vector<type *>::size;
	//! Returns how many items could be added before memory needs reallocating
	using std::vector<type *>::capacity;
	//! Sets how much over-allocation should be performed to avoid reallocating memory
	using std::vector<type *>::reserve;
	//! Returns the number of items in the list
	uint count() const { return (uint) std::vector<type *>::size(); }
	//! Returns true if the list is empty
	bool isEmpty() const { return std::vector<type *>::empty(); }
	//! Inserts item \em d into the list at index \em i
	bool insert(uint i, const type *d) { FXEXCEPTION_STL1 { std::vector<type *>::insert(std::vector<type *>::begin()+i, const_cast<type *>(d)); } FXEXCEPTION_STL2; return true; }
	//! Inserts item \em d into the list at where iterator \em it points
	bool insertAtIter(QPtrVectorIterator<type> &it, const type *d);
	//! Inserts item \em d into the list in its correct sorted order
	void inSort(const type *d)
	{
		for(typename std::vector<type *>::iterator it=std::vector<type *>::begin(); it!=std::vector<type *>::end(); ++it)
		{
			if(compareItems(*it, const_cast<type *>(d))>=0)
			{
				FXEXCEPTION_STL1 { std::vector<type *>::insert(it, const_cast<type *>(d)); } FXEXCEPTION_STL2;
			}
		}
	}
	//! Appends the item \em d onto the list
	void append(const type *d) { FXEXCEPTION_STL1 { push_back(const_cast<type *>(d)); } FXEXCEPTION_STL2; }
	//! Extends the list to \em no items if not that already
	bool extend(uint no)
	{
		uint cs=(uint) std::vector<type *>::size();
		if(cs<no)
		{
			std::vector<type *>::resize(no);
			return true;
		}
		return false;
	}
	//! Removes the item at index \em i
	bool remove(uint i)
	{
		if(isEmpty()) return false;
		typename std::vector<type *>::iterator it=std::vector<type *>::begin()+i;
		deleteItem(*it);
		erase(it);
		return true;
	}
	//! Removes the specified item \em d via compareItems()
	bool remove(const type *d)
	{
		for(typename std::vector<type *>::iterator it=std::vector<type *>::begin(); it!=std::vector<type *>::end(); ++it)
		{
			if(0==compareItems(*it, const_cast<type *>(d)))
			{
				deleteItem(*it);
				erase(it);
				return true;
			}
		}
		return false;
	}
	//! Removes the specified item \em d via pointer compare (quicker)
	bool removeRef(const type *d)
	{
		for(typename std::vector<type *>::iterator it=std::vector<type *>::begin(); it!=std::vector<type *>::end(); ++it)
		{
			if(*it==d)
			{
				deleteItem(*it);
				erase(it);
				return true;
			}
		}
		return false;
	}
	//! Removes the item pointed to by \em it (quickest)
	bool removeByIter(QPtrVectorIterator<type> &it);
	//! Removes the last item
	bool removeLast()
	{
		if(isEmpty()) return false;
		typename std::vector<type *>::iterator it=--std::vector<type *>::end();
		deleteItem(*it);
		std::vector<type *>::pop_back();
		return true;
	}
	//! Removes the item at index \em i without auto-deletion
	type *take(uint i)
	{
		if(isEmpty()) return 0; // Fails for non-pointer types
		typename std::vector<type *>::iterator it=std::vector<type *>::begin()+i;
		type *ret=*it;
		std::vector<type *>::erase(it);
		return ret;
	}
	//! Removes the specified item \em d via compareItems() without auto-deletion
	bool take(const type *d)
	{
		for(typename std::vector<type *>::iterator it=std::vector<type *>::begin(); it!=std::vector<type *>::end(); ++it)
		{
			if(0==compareItems(*it, const_cast<type *>(d)))
			{
				std::vector<type *>::erase(it);
				return true;
			}
		}
		return false;
	}
	//! Removes the specified item \em d via pointer compare (quicker) without auto-deletion
	bool takeRef(const type *d)
	{
		for(typename std::vector<type *>::iterator it=std::vector<type *>::begin(); it!=std::vector<type *>::end(); ++it)
		{
			if(*it==d)
			{
				std::vector<type *>::erase(it);
				return true;
			}
		}
		return false;
	}
	//! Removes the item pointed to by \em it without auto-deletion (quickest)
	bool takeByIter(QPtrVectorIterator<type> &it);
	//! Removes the last item without auto-deletion
	bool takeLast()
	{
		if(isEmpty()) return false;
		typename std::vector<type *>::iterator it=--std::vector<type *>::end();
		std::vector<type *>::pop_back();
		return true;
	}
	//! Clears the list
	void clear()
	{
		for(typename std::vector<type *>::iterator it=std::vector<type *>::begin(); it!=std::vector<type *>::end(); ++it)
		{
			deleteItem(*it);
		}
		std::vector<type *>::clear();
	}
private:
	struct SortPredicate
	{
		QPtrVector *me;
		SortPredicate(QPtrVector *_me) : me(_me) { }
		bool operator()(type *a, type *b)
		{
			return me->compareItems(a, b)==-1;
		}
	};
public:
	//! Sorts the list using a user supplied callable entity taking two pointers of type \em type
	template<class SortFunc> void sort(SortFunc sortfunc)
	{
		std::sort(std::vector<type *>::begin(), std::vector<type *>::end(), sortfunc);
	}
	//! Sorts the list
	void sort()
	{
		std::sort(std::vector<type *>::begin(), std::vector<type *>::end(), SortPredicate(this));
	}
	//! Returns the index of the position of item \em d via compareItems(), or -1 if not found
	int find(const type *d)
	{
		int idx=0;
		for(typename std::vector<type *>::iterator it=std::vector<type *>::begin(); it!=std::vector<type *>::end(); ++it, ++idx)
		{
			if(0==compareItems(*it, const_cast<type *>(d))) return idx;
		}
		return -1;
	}
	//! Returns the index of the position of item \em d via pointer compare, or -1 if not found
	int findRef(const type *d)
	{
		int idx=0;
		for(typename std::vector<type *>::iterator it=std::vector<type *>::begin(); it!=std::vector<type *>::end(); ++it, ++idx)
		{
			if(*it==d) return idx;
		}
		return -1;
	}
	//! Returns the number of item \em d in the list via compareItems()
	uint contains(const type *d) const
	{
		uint count=0;
		for(typename std::vector<type *>::const_iterator it=std::vector<type *>::begin(); it!=std::vector<type *>::end(); ++it)
		{
			if(0==compareItems(const_cast<type *>(*it), const_cast<type *>(d))) count++;
		}
		return count;
	}
	//! Returns the number of item \em d in the list via pointer compare
	uint containsRef(const type *d) const
	{
		uint count=0;
		for(typename std::vector<type *>::const_iterator it=std::vector<type *>::begin(); it!=std::vector<type *>::end(); ++it)
		{
			if(*it==d) count++;
		}
		return count;
	}
	//! Replaces item at index \em i with \em d
	bool replace(uint i, const type *d, bool callDeleteItem=true)
	{
		if(isEmpty()) return false;
		typename std::vector<type *>::iterator it=std::vector<type *>::begin()+i;
		if(callDeleteItem)
			deleteItem(*it);
		*it=const_cast<type *>(d);
		//list<type *>::erase(list<type *>::begin()+i);
		//list<type *>::insert(list<type *>::begin()+i, d);
		return true;
	}
	//! Replaces item at iterator with \em d
	bool replaceAtIter(QPtrVectorIterator<type> &it, const type *d, bool callDeleteItem=true);
	//! Returns the item at index \em i
	type *at(uint i) const { return std::vector<type *>::empty() ? 0 : *(std::vector<type *>::begin()+i); }
	//! \overload
	type *operator[](uint i) const { return at(i); }
	//! \overload
	type *operator[](int i) const { return at((uint) i); }
	//! Returns the first item in the list
	type *getFirst() const { return std::vector<type *>::empty() ? 0 : std::vector<type *>::front(); }
	//! Returns the last item in the list
	type *getLast() const { return std::vector<type *>::empty() ? 0 : std::vector<type *>::back(); }
	//! Returns the first item in the list
	type *first() { return std::vector<type *>::empty() ? 0 : std::vector<type *>::front(); }
	//! Returns the last item in the list
	type *last() { return std::vector<type *>::empty() ? 0 : std::vector<type *>::back(); }
	//! Compares two items (used by many methods above). Default returns -1 if a < b, +1 if a > b and 0 if a==b
	virtual int compareItems(type *a, type *b) const { return (a<b) ? -1 : (a==b) ? 0 : -1; }

	typename std::vector<type *> &int_vector() { return static_cast<std::vector<type *> &>(*this); }
	typename std::vector<type *>::iterator int_begin() { return std::vector<type *>::begin(); }
	typename std::vector<type *>::iterator int_end() { return std::vector<type *>::end(); }

protected:
	virtual void deleteItem(type *d);
};

// Don't delete void *
template<> inline void QPtrVector<void>::deleteItem(void *)
{
}

template<class type> inline void QPtrVector<type>::deleteItem(type *d)
{
    if(autodel) delete d;
}

/*! \class QPtrVectorIterator
\ingroup QTL
\brief An iterator for a QPtrVector
*/
template<class type> class QPtrVectorIterator
{	// std::vector::iterator is a ptr on some implementations, so play it safe
	typename std::vector<type *>::iterator me;
	mutable bool dead;
	QPtrVector<type> *myvector;
protected:
	type *retptr() const
	{
		if(dead) return 0;
		if(myvector->int_end()==me) { dead=true; return 0; }
		return *me;
	}
public:
	typename std::vector<type *>::iterator &int_getIterator() { return me; }
	QPtrVectorIterator() : dead(true), myvector(0) { }
	//! Construct an iterator to the specified QPtrVector
	QPtrVectorIterator(const QPtrVector<type> &l) : dead(false), myvector(&const_cast<QPtrVector<type> &>(l)), me(const_cast<QPtrVector<type> &>(l).int_begin()) { }
	QPtrVectorIterator(const QPtrVectorIterator<type> &l) : dead(l.dead), myvector(l.myvector), me(l) { }
	QPtrVectorIterator<type> &operator=(const QPtrVectorIterator<type> &it)
	{
		dead=it.dead; myvector=it.myvector;
		me=it.me;
		return *this;
	}
	bool operator==(const QPtrVectorIterator &o) const { return me==o.me; }
	bool operator!=(const QPtrVectorIterator &o) const { return me!=o.me; }
	bool operator<(const QPtrVectorIterator &o) const { return me<o.me; }
	bool operator>(const QPtrVectorIterator &o) const { return me>o.me; }
	//! Returns the number of items in the list this iterator references
	uint count() const   { return myvector->count(); }
	//! Returns true if the list this iterator references is empty
	bool isEmpty() const { return myvector->isEmpty(); }
	//! Returns true if this iterator is at the start of its list
	bool atFirst() const
	{
		return myvector->int_begin()==me;
	}
	//! Returns true if this iterator is at the end of its vector
	bool atLast() const
	{
		typename std::vector<type *>::iterator next(me);
		++next;
		return myvector->int_end()==next;
	}
	//! Sets the iterator to point to the first item in the vector, then returns that item
	type *toFirst()
	{
		me=myvector->int_begin(); dead=false;
		return retptr();
	}
	//! Sets the iterator to point to the last item in the vector, then returns that item
	type *toLast()
	{
		me=myvector->int_end(); dead=false;
		if(!myvector->isEmpty()) --me;
		return retptr();
	}
	//! Makes the iterator dead (ie; point to nothing)
	QPtrVectorIterator<type> &makeDead()
	{
		me=myvector->int_end();
		dead=true;
		return *this;
	}
	//! Returns what the iterator points to
	operator type *() const { return retptr(); }
	//! Returns what the iterator points to
	type *operator*() { return retptr(); }
	//! Returns the item this iterator points to
	type *current() const { return retptr(); }
	//! Returns the item this iterator points to
	type *operator()() { return retptr(); }
	//! Increments the iterator
	type *operator++()
	{
		++me;
		return retptr();
	}
	//! Increments the iterator
	type *operator+=(uint j)
	{
		typename std::vector<type *>::difference_type left=myvector->int_end()-me;
		if(j>left) dead=true; else me+=j;
		return retptr();
	}
	//! Decrements the iterator
	type *operator--()
	{
		if(myvector->int_begin()==me) dead=true; else --me;
		return retptr();
	}
	//! Decrements the iterator
	type *operator-=(uint j)
	{
		typename std::vector<type *>::difference_type left=me-myvector->int_begin();
		if(j>left+1) dead=true; else me-=j;
		return retptr();
	}
};

template<class type> inline bool QPtrVector<type>::insertAtIter(QPtrVectorIterator<type> &it, const type *d)
{
	FXEXCEPTION_STL1 { std::vector<type *>::insert(it.int_getIterator(), const_cast<type *>(d));} FXEXCEPTION_STL2;
	return true;
}

template<class type> inline bool QPtrVector<type>::removeByIter(QPtrVectorIterator<type> &it)
{
	deleteItem(*it.int_getIterator());
	std::vector<type *>::erase(it.int_getIterator());
	return true;
}

template<class type> inline bool QPtrVector<type>::takeByIter(QPtrVectorIterator<type> &it)
{
	std::vector<type *>::erase(it.int_getIterator());
	return true;
}

template<class type> inline bool QPtrVector<type>::replaceAtIter(QPtrVectorIterator<type> &it, const type *d, bool callDeleteItem)
{
	if(callDeleteItem)
		deleteItem(*it);
	*it.int_getIterator()=const_cast<type *>(d);
	return true;
}

//! For Qt 2.x compatibility
#define QVector QPtrVector
//! For Qt 2.x compatibility
#define QVectorIterator QPtrVectorIterator

//! Writes the contents of the vector to stream \em s
template<class type> FXStream &operator<<(FXStream &s, const QPtrVector<type> &i)
{
	FXuint mysize=i.count();
	s << mysize;
	for(QPtrVectorIterator<type> it(i); it.current(); ++it)
	{
		s << *it.current();
	}
	return s;
}
//! Reads in a vector from stream \em s
template<class type> FXStream &operator>>(FXStream &s, QPtrVector<type> &i)
{
	FXuint mysize;
	s >> mysize;
	i.clear();
	for(uint n=0; n<mysize; n++)
	{
		type *item;
		FXERRHM(item=new type);
		s >> *item;
		i.append(item);
	}
	return s;
}

} // namespace

#endif

