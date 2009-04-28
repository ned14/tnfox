/********************************************************************************
*                                                                               *
*                 Q t   P o i n t e r   L i s t   T h u n k                     *
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

#ifndef QPTRLIST_H
#define QPTRLIST_H

#if _MSC_VER==1200
#pragma warning(disable: 4786)
#endif

#include "qvaluelist.h"
#include "FXException.h"
#include "FXStream.h"

namespace FX {

typedef FXuint uint;

/*! \file qptrlist.h
\brief Defines a thunk of Qt's QPtrList to the STL

To aid porting of Qt programs to FOX, this file defines a QPtrList & QList
via the STL
*/

/*! \class QPtrList
\ingroup QTL
\brief A thunk of Qt's QPtrList to the STL

A QPtrList should be used when insertions to and removals from the centre of the list
are common. Random access is slow with this list especially to near the end. If you
want fast random access, use FX::QPtrVector. Iterators are not invalidated by
insertions and removals.

Note that all sorts of unpleasantness has been used to get this work, including
\c const_cast<>. Also, because of the nature of templates not being compiled until
they are used, I may not have caught all the compile errors yet :(

\warning This list is much less forgiving of range errors than Qt. Where Qt returns
false and prints a message, this list like the STL throws an exception!
*/
template<class type, class allocator=FX::aligned_allocator<type *, 0> > class QPtrListIterator;
template<class type, class allocator=FX::aligned_allocator<type *, 0> > class QPtrList;
template<class type, class allocator> class QPtrList : protected std::list<type *, allocator>
{
	typedef std::list<type *, allocator> Base;
	bool autodel;
	typename Base::iterator int_idx(uint i)
	{
		typename Base::iterator it=Base::begin();
		while(i--) ++it;
		return it;
	}
public:
	explicit QPtrList(bool wantAutoDel=false) : autodel(wantAutoDel), Base() {}
	explicit QPtrList(Base &l) : autodel(false), Base(l) {}
	~QPtrList()	{ clear(); }
	FXADDMOVEBASECLASS(QPtrList, Base)
	//! Returns if auto-deletion is enabled
	bool autoDelete() const { return autodel; }
	//! Sets if auto-deletion is enabled
	void setAutoDelete(bool a) { autodel=a; }

	//! Returns the number of items in the list
	uint count() const { return (uint) Base::size(); }
	//! Returns true if the list is empty
	bool isEmpty() const { return Base::empty(); }
	//! Inserts item \em d into the list at index \em i
	bool insert(uint i, const type *d) { FXEXCEPTION_STL1 { Base::insert(int_idx(i), d); } FXEXCEPTION_STL2; return true; }
	//! Inserts item \em d into the list at where iterator \em it points
	bool insertAtIter(QPtrListIterator<type, allocator> &it, const type *d);
	//! Inserts item \em d into the list in its correct sorted order
	void inSort(const type *d)
	{
		for(typename Base::iterator it=Base::begin(); it!=Base::end(); ++it)
		{
			if(compareItems(*it, d)>=0)
			{
				FXEXCEPTION_STL1 { Base::insert(it, d); } FXEXCEPTION_STL2;
			}
		}
	}
	//! Prepends item \em d onto the list
	void prepend(const type *d) { FXEXCEPTION_STL1 { Base::push_front(const_cast<type *>(d)); } FXEXCEPTION_STL2; }
	//! Appends the item \em d onto the list
	void append(const type *d) { FXEXCEPTION_STL1 { Base::push_back(const_cast<type *>(d)); } FXEXCEPTION_STL2; }
	//! Removes the item at index \em i
	bool remove(uint i)
	{
		if(isEmpty()) return false;
		typename Base::iterator it=int_idx(i);
		deleteItem(*it);
		Base::erase(it);
		return true;
	}
	//! Removes the specified item \em d via compareItems()
	bool remove(const type *d)
	{
		for(typename Base::iterator it=Base::begin(); it!=Base::end(); ++it)
		{
			if(0==compareItems(*it, const_cast<type *>(d)))
			{
				deleteItem(*it);
				Base::erase(it);
				return true;
			}
		}
		return false;
	}
	//! Removes the specified item \em d via pointer compare (quicker)
	bool removeRef(const type *d)
	{
		for(typename Base::iterator it=Base::begin(); it!=Base::end(); ++it)
		{
			if(*it==d)
			{
				deleteItem(*it);
				Base::erase(it);
				return true;
			}
		}
		return false;
	}
	//! Removes the item pointed to by \em it (quickest)
	bool removeByIter(QPtrListIterator<type, allocator> &it);
	//! Removes the first item
	bool removeFirst()
	{
		if(isEmpty()) return false;
		typename Base::iterator it=Base::begin();
		deleteItem(*it);
		Base::pop_front();
		return true;
	}
	//! Removes the last item
	bool removeLast()
	{
		if(isEmpty()) return false;
		typename Base::iterator it=--Base::end();
		deleteItem(*it);
		Base::pop_back();
		return true;
	}
	//! Removes the item at index \em i without auto-deletion
	type *take(uint i)
	{
		if(isEmpty()) return 0; // Fails for non-pointer types
		typename Base::iterator it=int_idx(i);
		type *ret=*it;
		Base::erase(it);
		return ret;
	}
	//! Removes the specified item \em d via compareItems() without auto-deletion
	bool take(const type *d)
	{
		for(typename Base::iterator it=Base::begin(); it!=Base::end(); ++it)
		{
			if(0==compareItems(*it, const_cast<type *>(d)))
			{
				Base::erase(it);
				return true;
			}
		}
		return false;
	}
	//! Removes the item pointed to by \em it without auto-deletion (quickest)
	bool takeByIter(QPtrListIterator<type, allocator> &it);
	//! Removes the specified item \em d via pointer compare (quicker) without auto-deletion
	bool takeRef(const type *d)
	{
		for(typename Base::iterator it=Base::begin(); it!=Base::end(); ++it)
		{
			if(*it==d)
			{
				Base::erase(it);
				return true;
			}
		}
		return false;
	}
	//! Removes the first item without auto-deletion
	bool takeFirst()
	{
		if(isEmpty()) return false;
		typename Base::iterator it=Base::begin();
		Base::pop_front();
		return true;
	}
	//! Removes the last item without auto-deletion
	bool takeLast()
	{
		if(isEmpty()) return false;
		typename Base::iterator it=--Base::end();
		Base::pop_back();
		return true;
	}
	//! Clears the list
	void clear()
	{
		for(typename Base::iterator it=Base::begin(); it!=Base::end(); ++it)
		{
			deleteItem(*it);
		}
		Base::clear();
	}
private:
	template<class type2> struct swapPolicy
	{
		template<class L, class I> void swap(L &list, I &a, I &b) const
		{	// Simply swap the pointers
			type2 temp=*a;
			*a=*b;
			*b=temp;
		}
	};
	template<class type2> struct comparePolicyFunc
	{
		bool (*comparer)(type2 a, type2 b);
		template<class L, class I> bool compare(L &list, I &a, I &b) const
		{
			return comparer(*a, *b);
		}
	};
	template<class type2> struct comparePolicyMe
	{
		template<class L, class I> bool compare(L &list, I &a, I &b) const
		{
			return ((QPtrList<type2> &) *this).compareItems(*a, *b)==-1;
		}
	};
public:
	//! Sorts the list using a user supplied callable entity taking two pointers of type \em type
	template<typename SortFuncSpec> void sort(SortFuncSpec sortfunc)
	{	// Would use Base::sort but this is faster
		QValueListQSort<type *, Pol::itMove, swapPolicy, comparePolicyFunc> sorter((QValueList<type *> &) *this);
		//sorter.comparer=sortfunc;
		sorter.run();
	}
	//! Sorts the list
	void sort()
	{
		QValueListQSort<type *, Pol::itMove, swapPolicy, comparePolicyMe> sorter((QValueList<type *> &) *this);
		sorter.run();
	}
	//! Returns the index of the position of item \em d via compareItems(), or -1 if not found
	int find(const type *d)
	{
		int idx=0;
		for(typename Base::iterator it=Base::begin(); it!=Base::end(); ++it, ++idx)
		{
			if(0==compareItems(*it, d)) return idx;
		}
		return -1;
	}
	//! Returns the index of the position of item \em d via pointer compare, or -1 if not found
	int findRef(const type *d)
	{
		int idx=0;
		for(typename Base::iterator it=Base::begin(); it!=Base::end(); ++it, ++idx)
		{
			if(*it==d) return idx;
		}
		return -1;
	}
	//! Returns the number of item \em d in the list via compareItems()
	uint contains(const type *d) const
	{
		uint count=0;
		for(typename Base::iterator it=Base::begin(); it!=Base::end(); ++it)
		{
			if(0==compareItems(*it, d)) count++;
		}
		return count;
	}
	//! Returns the number of item \em d in the list via pointer compare
	uint containsRef(const type *d) const
	{
		uint count=0;
		for(typename Base::iterator it=Base::begin(); it!=Base::end(); ++it)
		{
			if(*it==d) count++;
		}
		return count;
	}
	//! Replaces item at index \em i with \em d
	bool replace(uint i, const type *d)
	{
		if(isEmpty()) return false;
		typename Base::iterator it=int_idx(i);
		*it=d;
		//list<type *>::erase(list<type *>::begin()+i);
		//list<type *>::insert(list<type *>::begin()+i, d);
		return true;
	}
	//! Replaces item at iterator with \em d
	bool replaceAtIter(QPtrListIterator<type, allocator> &it, const type *d);
	//! Returns the item at index \em i
	type *at(uint i) { return Base::empty() ? 0 : *int_idx(i); }
	//! Returns the first item in the list
	type *getFirst() const { return Base::empty() ? 0 : Base::front(); }
	//! Returns the last item in the list
	type *getLast() const { return Base::empty() ? 0 : Base::back(); }
	//! Returns the first item in the list
	type *first() { return Base::empty() ? 0 : Base::front(); }
	//! Returns the last item in the list
	type *last() { return Base::empty() ? 0 : Base::back(); }
	//! Compares two items (used by many methods above). Default returns -1 if a < b, +1 if a > b and 0 if a==b
    virtual int compareItems(type *a, type *b) const { return (a<b) ? -1 : (a==b) ? 0 : -1; }

	typename Base &int_list() { return static_cast<Base &>(*this); }
	typename Base::iterator int_begin() { return Base::begin(); }
	typename Base::iterator int_end() { return Base::end(); }

protected:
	virtual void deleteItem(type *d);
};

// Don't delete void *
template<> inline void QPtrList<void>::deleteItem(void *)
{
}

template<class type, class allocator> inline void QPtrList<type, allocator>::deleteItem(type *d)
{
    if(autodel) delete d;
}

/*! \class QPtrListIterator
\ingroup QTL
\brief An iterator for a QPtrList
*/
template<class type, class allocator> class QPtrListIterator : private std::list<type *, allocator>::iterator
{
	typedef std::list<type *, allocator> Base;
	mutable bool dead;
	QPtrList<type, allocator> *mylist;
protected:
	type *retptr() const
	{
		if(dead) return 0;
		typename Base::iterator &me=const_cast<QPtrListIterator<type, allocator> &>(*this);
		if(mylist->int_end()==me) { dead=true; return 0; }
		return *me;
	}
public:
	typename Base::iterator &int_getIterator() { return static_cast<typename Base::iterator &>(*this); }
	QPtrListIterator() : dead(true), mylist(0) { }
	//! Construct an iterator to the specified QPtrList
	QPtrListIterator(const QPtrList<type, allocator> &l) : dead(false), mylist(&const_cast<QPtrList<type, allocator> &>(l)), Base::iterator(const_cast<QPtrList<type, allocator> &>(l).int_begin()) { retptr(); }
	QPtrListIterator(const QPtrListIterator<type, allocator> &l) : dead(l.dead), mylist(l.mylist), Base::iterator(l) { }
	QPtrListIterator<type, allocator> &operator=(const QPtrListIterator<type, allocator> &it)
	{
		dead=it.dead; mylist=it.mylist;
		typename Base::iterator &me=*this;
		me=it;
		return *this;
	}
	bool operator==(const QPtrListIterator &o) const { return static_cast<typename Base::iterator const &>(*this)==o; }
	bool operator!=(const QPtrListIterator &o) const { return static_cast<typename Base::iterator const &>(*this)!=o; }
	bool operator<(const QPtrListIterator &o) const { return static_cast<typename Base::iterator const &>(*this)<o; }
	bool operator>(const QPtrListIterator &o) const { return static_cast<typename Base::iterator const &>(*this)>o; }
	//! Returns the number of items in the list this iterator references
	uint count() const   { return mylist->count(); }
	//! Returns true if the list this iterator references is empty
	bool isEmpty() const { return mylist->isEmpty(); }
	//! Returns true if this iterator is at the start of its list
	bool atFirst() const
	{
		typename Base::iterator &me=const_cast<QPtrListIterator<type, allocator> &>(*this); 
		return mylist->int_begin()==me;
	}
	//! Returns true if this iterator is at the end of its list
	bool atLast() const
	{
		typename Base::iterator next=*this;
		++next;
		return mylist->int_end()==next;
	}
	//! Sets the iterator to point to the first item in the list, then returns that item
	type *toFirst()
	{
		typename Base::iterator &me=const_cast<QPtrListIterator<type, allocator> &>(*this); 
		me=mylist->int_begin(); dead=false;
		return retptr();
	}
	//! Sets the iterator to point to the last item in the list, then returns that item
	type *toLast()
	{
		typename Base::iterator &me=const_cast<QPtrListIterator<type, allocator> &>(*this); 
		me=mylist->int_end(); dead=false;
		if(!mylist->isEmpty()) --me;
		return retptr();
	}
	//! Makes the iterator dead (ie; point to nothing)
	QPtrListIterator<type, allocator> &makeDead()
	{
		typename Base::iterator &me=const_cast<QPtrListIterator<type, allocator> &>(*this);
		me=mylist->int_end();
		dead=true;
		return *this;
	}
	//! Returns what the iterator points to
	type *operator*() const { return retptr(); }
	//! Returns the item this iterator points to
	type *current() const { return retptr(); }
	//! Increments the iterator
	type *operator++()
	{
		if(!dead)
		{
			typename Base::iterator &me=int_getIterator(); ++me;
		}
		return retptr();
	}
	//! Increments the iterator
	type *operator+=(uint j)
	{
		if(!dead)
		{
			typename Base::iterator &me=*this;
			typename Base::iterator myend=mylist->int_end();
			for(uint n=0; n<j && me!=myend; n++)
				++me;
		}
		return retptr();
	}
	//! Decrements the iterator
	type *operator--()
	{
		if(!dead)
		{
			typename Base::iterator &me=*this;
			if(mylist->int_begin()==me)
			{
				me=mylist->int_end();
				dead=true;
			}
			else --me;
		}
		return retptr();
	}
	//! Decrements the iterator
	type *operator-=(uint j)
	{
		if(!dead)
		{
			typename Base::iterator &me=*this;
			typename Base::iterator myend=mylist->int_begin();
			for(uint n=0; n<j && !dead; n++)
			{
				if(myend==me)
				{
					me=mylist->int_end();
					dead=true;
				}
				else --me;
			}
		}
		return retptr();
	}
};

template<class type, class allocator> inline bool QPtrList<type, allocator>::insertAtIter(QPtrListIterator<type, allocator> &it, const type *d)
{
	FXEXCEPTION_STL1 { Base::insert(it.int_getIterator(), const_cast<type *>(d));} FXEXCEPTION_STL2;
	return true;
}

template<class type, class allocator> inline bool QPtrList<type, allocator>::removeByIter(QPtrListIterator<type, allocator> &it)
{
	deleteItem(*it.int_getIterator());
	Base::erase(it.int_getIterator());
	return true;
}

template<class type, class allocator> inline bool QPtrList<type, allocator>::takeByIter(QPtrListIterator<type, allocator> &it)
{
	Base::erase(it.int_getIterator());
	return true;
}

template<class type, class allocator> inline bool QPtrList<type, allocator>::replaceAtIter(QPtrListIterator<type, allocator> &it, const type *d)
{
	*it.int_getIterator()=const_cast<type *>(d);
	return true;
}

//! For Qt 2.x compatibility
#define QList QPtrList
//! For Qt 2.x compatibility
#define QListIterator QPtrListIterator

//! Writes the contents of the list to stream \em s
template<class type, class allocator> FXStream &operator<<(FXStream &s, const QPtrList<type, allocator> &i)
{
	FXuint mysize=i.count();
	s << mysize;
	for(QPtrListIterator<type, allocator> it(i); it.current(); ++it)
	{
		s << *it.current();
	}
	return s;
}
//! Reads in a list from stream \em s
template<class type, class allocator> FXStream &operator>>(FXStream &s, QPtrList<type, allocator> &i)
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

/*! \class QQuickList
\brief A FX::QPtrList with move semantics

This is a low overhead list which copies destructively and always has
auto-deletion enabled. Ideal for a list being passed around a lot.
*/
template<class type, class allocator=FX::aligned_allocator<type *, 0> > class QQuickListIterator;
template<class type, class allocator=FX::aligned_allocator<type *, 0> > class QQuickList;
template<class type, class allocator> class QQuickList : protected QPtrList<type, allocator>
{
	friend class QQuickListIterator<type, allocator>;
public:
	//! Creates an instance
	QQuickList() : QPtrList<type, allocator>(true) { }
	//! Destructively copies a list very quickly
#ifndef HAVE_CPP0XRVALUEREFS
#ifdef HAVE_CONSTTEMPORARIES
	QQuickList(const QQuickList &_o) : QPtrList<type, allocator>(true)
	{
		QQuickList &o=const_cast<QQuickList &>(_o);
#else
	QQuickList(QQuickList &o) : QPtrList<type, allocator>(true)
	{
#endif
#else
private:
	QQuickList(const QQuickList &);		// disable copy constructor
public:
	QQuickList(QQuickList &&o) : QPtrList<type, allocator>(true)
	{
#endif
		Base::splice(Base::begin(), o, o.begin(), o.end());
	}
#ifndef HAVE_CPP0XRVALUEREFS
	QQuickList &operator=(QQuickList &o)
#else
private:
	QQuickList &operator=(QQuickList &o);
public:
	QQuickList &&operator=(QQuickList &&o)
#endif
	{
		clear();
		Base::splice(Base::begin(), o, o.begin(), o.end());
		return *this;
	}
	//! Destructively copies a FX::QPtrList very quickly
#ifndef HAVE_CPP0XRVALUEREFS
#ifdef HAVE_CONSTTEMPORARIES
	explicit QQuickList(const QPtrList<type, allocator> &_o) : QPtrList<type, allocator>(true)
	{
		QPtrList<type, allocator> &o=const_cast<QPtrList<type, allocator> &>(_o);
#else
	explicit QQuickList(QPtrList<type, allocator> &o) : QPtrList<type, allocator>(true)
	{
#endif
#else
	explicit QQuickList(QPtrList<type, allocator> &&o) : QPtrList<type, allocator>(true)
	{
#endif
		Base::splice(Base::begin(), o, o.begin(), o.end());
	}
	//! Returns the list as a FX::QPtrList
	const QPtrList<type, allocator> &asPtrList() const throw()
	{
		return static_cast<QPtrList<type, allocator> &>(*this);
	}
	using QPtrList<type, allocator>::count;
	using QPtrList<type, allocator>::isEmpty;
	using QPtrList<type, allocator>::insert;
	using QPtrList<type, allocator>::insertAtIter;
	using QPtrList<type, allocator>::inSort;
	using QPtrList<type, allocator>::prepend;
	using QPtrList<type, allocator>::append;
	using QPtrList<type, allocator>::remove;
	using QPtrList<type, allocator>::removeRef;
	using QPtrList<type, allocator>::removeByIter;
	using QPtrList<type, allocator>::removeFirst;
	using QPtrList<type, allocator>::removeLast;
	using QPtrList<type, allocator>::take;
	using QPtrList<type, allocator>::takeRef;
	using QPtrList<type, allocator>::clear;
	using QPtrList<type, allocator>::sort;
	using QPtrList<type, allocator>::find;
	using QPtrList<type, allocator>::findRef;
	using QPtrList<type, allocator>::contains;
	using QPtrList<type, allocator>::containsRef;
	using QPtrList<type, allocator>::replace;
	using QPtrList<type, allocator>::at;
	using QPtrList<type, allocator>::getFirst;
	using QPtrList<type, allocator>::getLast;
	using QPtrList<type, allocator>::first;
	using QPtrList<type, allocator>::last;
	using QPtrList<type, allocator>::compareItems;
	using QPtrList<type, allocator>::int_list;
	using QPtrList<type, allocator>::int_begin;
	using QPtrList<type, allocator>::int_end;

	friend FXStream &operator<<(FXStream &s, const QQuickList<type> &_i)
	{
		const QPtrList<type, allocator> &i=_i;
		s << i;
		return s;
	}
	friend FXStream &operator>>(FXStream &s, QQuickList<type> &_i)
	{
		QPtrList<type, allocator> &i=_i;
		s >> i;
		return s;
	}
};

/*! \class QQuickListIterator
\brief An iterator for a FX::QQuickList
*/
template<class type, class allocator> class QQuickListIterator : public QPtrListIterator<type, allocator>
{
public:
	QQuickListIterator(const QQuickList<type, allocator> &l) : QPtrListIterator<type, allocator>(l) { }
};

} // namespace

#endif

