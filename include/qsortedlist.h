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
*/
template<class type> class QSortedListIterator;
template<class type> class QSortedList : private QPtrList<type>
{
	bool findInternal(QSortedListIterator<type> *itout, int *idx, const type *d) const;
public:
	QSortedList(bool wantAutoDel=false) : QPtrList<type>(wantAutoDel) {}
	QSortedList(const QSortedList<type> &o) : QPtrList<type>(o) {}
	~QSortedList()				{ clear(); }
	QSortedList<type> &operator=(const QSortedList<type> &l)
			{ return (QSortedList<type>&)QPtrList<type>::operator=(l); }
	bool operator==( const QSortedList<type> &list ) const
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
	//! Removes an item
	bool remove(const type *d);
	//! Removes an item without auto-deletion
	bool take(const type *d);
	//! Same as QList::find() except a much more efficient binary search is used
	int find(const type *d) const { int idx; if(findInternal(0, &idx, d)) return idx; else return -1; }
	//! Finds \em d by binary searching, returning an iterator pointing to it or a null iterator if not found.
	QSortedListIterator<type> findIter(const type *d) const;
	//! Returns a pointer to the item matching \em d or zero if not found
	type *findP(const type *d) const;
	/*! Returns the record after which an insert() would place d if called. This method
	is useful for finding "similar to" items. Always returns a valid index and never
	changes the current list item.
	*/
	int findClosest(const type *d) const { int idx; findInternal(0, &idx, d); return idx; }
	/*! Returns an iterator pointing to the closest item to \em d. Note that the closest
	item can be after the last item causing a null iterator to be returned */
	QSortedListIterator<type> findClosestIter(const type *d) const;
	/*! Returns a pointer to the closest item. Note that the closest item can be after
	the last item causing a null pointer to be returned */
	type *findClosestP(const type *d) const;
	/*! This is the only way to insert a new item into the list (the other methods
	prepend() or append() are disabled). This method inserts the new item into the
	correctly sorted position.
	*/
	bool insert(const type *d);
};

template<class type> inline int QSortedList<type>::compareItems(type *a, type *b) const
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
template<class type> class QSortedListIterator : public QPtrListIterator<type>
{
public:
	QSortedListIterator() { }
	QSortedListIterator(const QSortedList<type> &l)
		: QPtrListIterator<type>((const QPtrList<type> &) l) {}
};

template<class type> inline bool QSortedList<type>::remove(const type *d)
{
	QSortedListIterator<type> it(*this);
	if(!findInternal(&it, 0, d)) return false;
	return QPtrList<type>::removeByIter(it);
}

template<class type> inline bool QSortedList<type>::take(const type *d)
{
	QSortedListIterator<type> it(*this);
	if(!findInternal(&it, 0, d)) return false;
	return QPtrList<type>::takeByIter(it);
}

template<class type> inline QSortedListIterator<type> QSortedList<type>::findIter(const type *d) const
{
	QSortedListIterator<type> it(*this);
	if(!findInternal(&it, 0, d))
		it.makeDead();
	return it;
}

template<class type> inline type *QSortedList<type>::findP(const type *d) const
{
	QSortedListIterator<type> it(*this);
	if(!findInternal(&it, 0, d)) return 0;
	return it.current();
}

template<class type> inline QSortedListIterator<type> QSortedList<type>::findClosestIter(const type *d) const
{
	QSortedListIterator<type> it(*this);
	findInternal(&it, 0, d);
	return it;
}

template<class type> inline type *QSortedList<type>::findClosestP(const type *d) const
{
	QSortedListIterator<type> it(*this);
	findInternal(&it, 0, d);
	return it.current();
}

template<class type> inline bool QSortedList<type>::insert(const type *d)
{
	QSortedListIterator<type> it(*this);
	findInternal(&it, 0, d);
	return QPtrList<type>::insertAtIter(it, d);
}

template<class type> inline bool QSortedList<type>::findInternal(QSortedListIterator<type> *itout, int *idx, const type *d) const
{
	int n=0;
	QSortedListIterator<type> it(*this); it.toFirst();
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
		QSortedList<type> *t=const_cast<QSortedList<type> *>(this);
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

//! Writes the contents of the list to stream \em s
template<class type> FXStream &operator<<(FXStream &s, const QSortedList<type> &i)
{
	FXuint mysize=i.count();
	s << mysize;
	for(QSortedListIterator<type> it(i); it.current(); ++it)
	{
		s << *it.current();
	}
	return s;
}
//! Reads in a list from stream \em s
template<class type> FXStream &operator>>(FXStream &s, QSortedList<type> &i)
{
	QPtrList<type> &ii=(QPtrList<type> &) i;
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
