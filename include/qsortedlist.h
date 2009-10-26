/********************************************************************************
*                                                                               *
*                         An always sorted qptrlist                             *
*                                                                               *
*********************************************************************************
* Copyright (C) 2001,2002,2003 by Niall Douglas.   All Rights Reserved.         *
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

#ifndef QSORTEDLIST_H
#define QSORTEDLIST_H

#include <qptrlist.h>
/*! \file qsortedlist.h
\brief Defines classes which provide sorted lists with a binary search
*/

namespace FX {
/*! \class QSortedList
\ingroup QTL
\brief An always sorted ptr list with binary searching

This is one of probably the first five classes I wrote for Tornado and it's
basically an always sorted FX::QPtrList. The list
is always sorted according to the < and == operators you provide for your list item and
find() performs a binary instead of a linear search. Binary searches are extremely
efficient on sorted data with O(lg n) most compares as compared against O(n) for linear
searches. On a 1024 item list, this means a maximum of ten compares as against 1024 compares!

\note This QSortedList does not bear ANY resemblance to Qt's one of the same name.
This one is MUCH more useful!

\warning You want to be cautious of remove() and take() - this uses the same comparator as the find()
function which means if your comparator is loose for easier searching, often the wrong
item will get deleted. Use removeRef() and takeRef() for these situations - you still get
most of the speed of the binary search but with guaranteed removal of a particular pointer.
*/
template<class type, class allocator=FX::aligned_allocator<type *, 0> > class QSortedListIterator;
template<class type, class allocator=FX::aligned_allocator<type *, 0> > class QSortedList;
template<class type, class allocator> class QSortedList : private QPtrList<type, allocator>
{
	bool findInternal(QSortedListIterator<type, allocator> *itout, int *idx, const type *d) const;
	QSortedListIterator<type, allocator> findRefInternal(const type *d) const;
public:
	explicit QSortedList(bool wantAutoDel=false) : QPtrList<type>(wantAutoDel) {}
	QSortedList(const QSortedList<type, allocator> &o) : QPtrList<type>(o) {}
	~QSortedList()				{ clear(); }
	QSortedList<type, allocator> &operator=(const QSortedList<type, allocator> &l)
			{ return (QSortedList<type, allocator>&)QPtrList<type>::operator=(l); }
	FXADDMOVEBASECLASS(QSortedList, QPtrList<type>)
	bool operator==( const QSortedList<type, allocator> &list ) const
	{ return QPtrList<type>::operator==( list ); }

	using QPtrList<type>::autoDelete;
	using QPtrList<type>::setAutoDelete;
	using QPtrList<type>::count;
	using QPtrList<type>::isEmpty;
	using QPtrList<type>::removeByIter;
	using QPtrList<type>::removeFirst;
	using QPtrList<type>::removeLast;
	using QPtrList<type>::takeByIter;
	using QPtrList<type>::takeFirst;
	using QPtrList<type>::takeLast;
	using QPtrList<type>::clear;
	using QPtrList<type>::contains;
	using QPtrList<type>::containsRef;
	using QPtrList<type>::at;
	using QPtrList<type>::getFirst;
	using QPtrList<type>::getLast;
	using QPtrList<type>::first;
	using QPtrList<type>::last;
	/*! Implemented for you to compare your items using the < and == operators which
	you \b must provide. You may of course reimplement this and hence forego the
	need to implement these operators.
	*/
	virtual int compareItems(type *a, type *b) const;
	//! Removes an item by binary search
	bool remove(const type *d);
	//! Removes an item by binary search and then by pointer value
	bool removeRef(const type *d);
	//! Removes an item by binary search without auto-deletion
	bool take(const type *d);
	//! Removes an item by binary search and then by pointer value without auto-deletion
	bool takeRef(const type *d);
	//! Same as QList::find() except a much more efficient binary search is used
	int find(const type *d) const { int idx; if(findInternal(0, &idx, d)) return idx; else return -1; }
	//! Finds \em d by binary searching, returning an iterator pointing to it or a null iterator if not found.
	QSortedListIterator<type, allocator> findIter(const type *d) const;
	//! Returns a pointer to the item matching \em d or zero if not found
	type *findP(const type *d) const;
	/*! Returns the record after which an insert() would place d if called. This method
	is useful for finding "similar to" items. Always returns a valid index and never
	changes the current list item.
	*/
	int findClosest(const type *d) const { int idx; findInternal(0, &idx, d); return idx; }
	/*! Returns an iterator pointing to the closest item to \em d. Note that the closest
	item can be after the last item causing a null iterator to be returned */
	QSortedListIterator<type, allocator> findClosestIter(const type *d) const;
	/*! Returns a pointer to the closest item. Note that the closest item can be after
	the last item causing a null pointer to be returned */
	type *findClosestP(const type *d) const;
	/*! This is the only way to insert a new item into the list (the other methods
	prepend() or append() are disabled). This method inserts the new item into the
	correctly sorted position.
	*/
	bool insert(const type *d);
	/*! Merges the specified list into this list, emptying the source list. If
	\em exclusive is set, if the item being merged equals an already existing item,
	it is thrown away. */
	void merge(QSortedList<type, allocator> &list, bool exclusive=false);
};

template<class type, class allocator> inline int QSortedList<type, allocator>::compareItems(type *a, type *b) const
{
	if(*a==*b) return 0;
	return (*a<*b) ? -1 : 1;
	//if(*((QSortedListItem<type> *) s1)==*((QSortedListItem<type> *) s2)) return 0;
	//return (*((QSortedListItem<type> *) s1)<*((QSortedListItem<type> *) s2) ? -1 : 1 );
}
template<> inline int QSortedList<void>::compareItems(void *a, void *b) const
{
	if(a==b) return 0;
	return (a<b) ? -1 : 1;
}

/*! \class QSortedListIterator
\ingroup QTL
\brief Provides an iterator for a QSortedList

This iterator works identically to Qt's ones. See their documentation.

\sa QSortedList
\sa QListIterator
*/
template<class type, class allocator> class QSortedListIterator : public QPtrListIterator<type, allocator>
{
public:
	QSortedListIterator() { }
	QSortedListIterator(const QSortedList<type, allocator> &l)
		: QPtrListIterator<type, allocator>((const QPtrList<type, allocator> &) l) {}
	//! Makes the iterator dead (ie; point to nothing)
	QSortedListIterator<type, allocator> &makeDead()
	{
		return static_cast<QSortedListIterator<type, allocator> &>(QPtrListIterator<type, allocator>::makeDead());
	}
};

template<class type, class allocator> inline QSortedListIterator<type, allocator> QSortedList<type, allocator>::findRefInternal(const type *d) const
{
	QSortedListIterator<type, allocator> it;
	if(!findInternal(&it, 0, d)) return it.makeDead();
	QSortedListIterator<type, allocator> it1(it), it2(it);
	type *a;
	// Step backwards while comparitor is equal
	for(--it; (a=it.current()) && !compareItems(const_cast<type *>(d), a); it1=it, --it);
	for(it=it1; (a=it.current()); ++it)
	{
		if(a==d) return it;
		if(it==it2) break;
	}
	// Step forwards while comparitor is equal
	for(; (a=it.current()) && !compareItems(const_cast<type *>(d), a); ++it)
	{
		if(a==d) return it;
	}
	return it.makeDead();
}

template<class type, class allocator> inline bool QSortedList<type, allocator>::remove(const type *d)
{
	QSortedListIterator<type, allocator> it;
	if(!findInternal(&it, 0, d)) return false;
	return QPtrList<type>::removeByIter(it);
}

template<class type, class allocator> inline bool QSortedList<type, allocator>::removeRef(const type *d)
{
	QSortedListIterator<type, allocator> it(findRefInternal(d));
	if(!it.current()) return false;
	return QPtrList<type>::removeByIter(it);
}

template<class type, class allocator> inline bool QSortedList<type, allocator>::take(const type *d)
{
	QSortedListIterator<type, allocator> it;
	if(!findInternal(&it, 0, d)) return false;
	return QPtrList<type>::takeByIter(it);
}

template<class type, class allocator> inline bool QSortedList<type, allocator>::takeRef(const type *d)
{
	QSortedListIterator<type, allocator> it(findRefInternal(d));
	if(!it.current()) return false;
	return QPtrList<type>::takeByIter(it);
}

template<class type, class allocator> inline QSortedListIterator<type, allocator> QSortedList<type, allocator>::findIter(const type *d) const
{
	QSortedListIterator<type, allocator> it;
	if(!findInternal(&it, 0, d))
		it.makeDead();
	return it;
}

template<class type, class allocator> inline type *QSortedList<type, allocator>::findP(const type *d) const
{
	QSortedListIterator<type, allocator> it;
	if(!findInternal(&it, 0, d)) return 0;
	return it.current();
}

template<class type, class allocator> inline QSortedListIterator<type, allocator> QSortedList<type, allocator>::findClosestIter(const type *d) const
{
	QSortedListIterator<type, allocator> it;
	findInternal(&it, 0, d);
	return it;
}

template<class type, class allocator> inline type *QSortedList<type, allocator>::findClosestP(const type *d) const
{
	QSortedListIterator<type, allocator> it;
	findInternal(&it, 0, d);
	return it.current();
}

template<class type, class allocator> inline bool QSortedList<type, allocator>::insert(const type *d)
{
	QSortedListIterator<type, allocator> it;
	findInternal(&it, 0, d);
	return QPtrList<type>::insertAtIter(it, d);
}

template<class type, class allocator> inline bool QSortedList<type, allocator>::findInternal(QSortedListIterator<type, allocator> *itout, int *idx, const type *d) const
{
	int n=0;
	QSortedListIterator<type, allocator> it(*this); it.toFirst();
	int lbound=0, ubound=count()-1, c;
	//if(lbound<0) lbound=0;
	while(lbound<=ubound)
	{
		int result;
		const type *testpoint;
		c=(lbound+ubound)/2;
		if(c-n>0) { testpoint=it+=(c-n); n=c; }
		else if(c-n<0) { testpoint=it-=(n-c); n=c; }
		else testpoint=it.current();
		// Unfortunate that I have to do this, but compareItems() should really be const
		QSortedList<type, allocator> *t=const_cast<QSortedList<type, allocator> *>(this);
		result=t->compareItems(const_cast<type *>(d), const_cast<type *>(testpoint));
		if(result<0)
			ubound=c-1;
		else if(result>0)
			lbound=c+1;
		else
		{
			if(itout) *itout=it;
			if(idx) *idx=n;
			return true;
		}
	}
	if(itout)
	{
		if(lbound!=n) it+=(lbound-n);
		*itout=it;
	}
	if(idx) *idx=lbound;
	return false;
}

template<class type, class allocator> inline void QSortedList<type, allocator>::merge(QSortedList<type, allocator> &list, bool exclusive)
{
	QSortedListIterator<type, allocator> it;
	type *ne;
	for(QSortedListIterator<type, allocator> nit(list); (ne=nit.current());)
	{
		QSortedListIterator<type, allocator> nit2(nit); ++nit;
		if(findInternal(&it, 0, ne) && exclusive)
			QPtrList<type>::removeByIter(nit2);
		else
		{	// Might as well splice as it avoids malloc
			std::list<type *, allocator>::splice(it.int_getIterator(), static_cast<std::list<type *, allocator> &>(list), nit2.int_getIterator());
			//QPtrList<type>::insertAtIter(it, ne);
			//list.takeByIter(nit2);
		}
	}
}

//! Writes the contents of the list to stream \em s
template<class type, class allocator> FXStream &operator<<(FXStream &s, const QSortedList<type, allocator> &i)
{
	FXuint mysize=i.count();
	s << mysize;
	for(QSortedListIterator<type, allocator> it(i); it.current(); ++it)
	{
		s << *it.current();
	}
	return s;
}
//! Reads in a list from stream \em s
template<class type, class allocator> FXStream &operator>>(FXStream &s, QSortedList<type, allocator> &i)
{
	QPtrList<type, allocator> &ii=(QPtrList<type, allocator> &) i;
	FXuint mysize;
	s >> mysize;
	i.clear();
	for(FXuint n=0; n<mysize; n++)
	{
		type *item;
		FXERRHM(item=new type);
		s >> *item;
		ii.append(item);
	}
	return s;
}


} // namespace

#endif
