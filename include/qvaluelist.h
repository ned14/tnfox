/********************************************************************************
*                                                                               *
*                   Q t   V a l u e   L i s t   T h u n k                       *
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

#ifndef QVALUELIST_H
#define QVALUELIST_H

#if _MSC_VER==1200
#pragma warning(disable: 4786)
#endif

#include <list>
#undef Unsorted
#include "fxdefs.h"
#include "FXException.h"
#include "FXPolicies.h"
#include "FXStream.h"

namespace FX {

typedef FXuint uint;

/*! \file qvaluelist.h
\brief Defines a thunk of Qt's QValueList to the STL
*/

/*! \class QValueList
\ingroup QTL
\brief A thunk of Qt's QValueList to the STL

To aid porting of Qt programs to FOX. Since the STL list<> is virtually identical
to Qt's QValueList, I haven't bothered documenting it.

Note that the following are not quite the same:
\li There's no support for Qt's "current list item" system. It's a bad idea for
thread-safe code anyway
\li remove() doesn't return how many items it deleted. Use removeAllOf() instead for this.
\li insert() returns an iterator
*/
template<class type> class QValueList : public std::list<type>
{
public:
	QValueList() : std::list<type>() { }
	QValueList(const std::list<type> &l) : std::list<type>(l) { }
	void remove(const type &d) { std::list<type>::remove(d); }
	uint removeAllOf(const type &d)
	{
		uint count=0;
		for(typename std::list<type>::iterator it=std::list<type>::begin(); it!=std::list<type>::end(); ++it)
		{
			if(*it==d) count++;
		}
		for(uint n=0; n<count; n++)
			std::list<type>::remove(d);
		return count;
	}
	bool isEmpty() const { return std::list<type>::empty(); }
	typename std::list<type>::iterator append(const type &d) { FXEXCEPTION_STL1 { std::list<type>::push_back(d); } FXEXCEPTION_STL2; return --std::list<type>::end(); }
	typename std::list<type>::iterator append(const QValueList &l)
	{
		FXEXCEPTION_STL1 {
			for(typename std::list<type>::const_iterator it=l.begin(); it!=l.end(); ++it)
			{
				std::list<type>::push_back(*it);
			}
		} FXEXCEPTION_STL2;
		return --std::list<type>::end();
	}
	typename std::list<type>::iterator prepend(const type &d) { FXEXCEPTION_STL1 { std::list<type>::push_front(d); } FXEXCEPTION_STL2; return std::list<type>::begin(); }
	typename std::list<type>::iterator remove(typename std::list<type>::iterator it) { typename std::list<type>::iterator itafter=it; ++itafter; std::list<type>::erase(it); return itafter; }
	typename std::list<type>::iterator at(typename std::list<type>::size_type i) { typename std::list<type>::iterator it; for(it=std::list<type>::begin(); i; --i, ++it); return it; }
	typename std::list<type>::const_iterator at(typename std::list<type>::size_type i) const { typename std::list<type>::const_iterator it; for(it=std::list<type>::begin(); i; --i, ++it); return it; }
	type &operator[](typename std::list<type>::size_type i) { return *at(i); }
	const type &operator[](typename std::list<type>::size_type i) const { return *at(i); }
	typename std::list<type>::iterator find(const type &d)
	{
		typename std::list<type>::iterator it;
		for(it=std::list<type>::begin(); it!=std::list<type>::end(); ++it)
			if(*it==d) return it;
		return it;
	}
	typename std::list<type>::const_iterator find(const type &d) const
	{
		typename std::list<type>::const_iterator it;
		for(it=std::list<type>::begin(); it!=std::list<type>::end(); ++it)
			if(*it==d) return it;
		return it;
	}
	typename std::list<type>::iterator find(typename std::list<type>::iterator it, const type &d)
	{
		for(; it!=std::list<type>::end(); ++it)
			if(*it==d) return it;
		return it;
	}
	typename std::list<type>::const_iterator find(typename std::list<type>::const_iterator it, const type &d) const
	{
		for(; it!=std::list<type>::end(); ++it)
			if(*it==d) return it;
		return it;
	}
	int findIndex(const type &d) const
	{
		int idx=0;
		for(typename std::list<type>::const_iterator it=std::list<type>::begin(); it!=std::list<type>::end(); ++it, ++idx)
			if(*it==d) return idx;
		return -1;
	}
	typename std::list<type>::size_type contains(const type &d) const
	{
		typename std::list<type>::size_type count=0;
		for(typename std::list<type>::const_iterator it=std::list<type>::begin(); it!=std::list<type>::end(); ++it)
			if(*it==d) count++;
		return count;
	}
	typename std::list<type>::size_type count() const { return std::list<type>::size(); }
	QValueList<type> &operator+=(const type &d) { std::list<type>::push_back(d); return *this; }

};

// Unfortunately GCC doesn't like templated typedefs (at last one area MSVC wins!)
//template<class type> typedef typename std::list<type>::iterator QValueListIterator;
//template<class type> typedef typename std::list<type>::const_iterator QValueListConstIterator;

/*! \class QValueListIterator
\ingroup QTL
\brief Iterator for a QValueList

\sa FX::QValueList
*/
template<class type> class QValueListIterator : public std::list<type>::iterator
{
public:
	QValueListIterator() : std::list<type>::iterator() { }
	QValueListIterator(const QValueListIterator &o) : std::list<type>::iterator(o) { }
};
/*! \class QValueListConstIterator
\ingroup QTL
\brief Const iterator for a QValueList

\sa FX::QValueList
*/
template<class type> class QValueListConstIterator : public std::list<type>::const_iterator
{
public:
	QValueListConstIterator() : std::list<type>::const_iterator() { }
	QValueListConstIterator(const QValueListConstIterator &o) : std::list<type>::const_iterator(o) { }
};

//! Writes the contents of the list to stream \em s
template<class type> FXStream &operator<<(FXStream &s, const QValueList<type> &i)
{
	FXuint mysize=i.count();
	s << mysize;
	for(typename QValueList<type>::const_iterator it=i.begin(); it!=i.end(); ++it)
	{
		s << *it;
	}
	return s;
}
//! Reads in a list from stream \em s
template<class type> FXStream &operator>>(FXStream &s, QValueList<type> &i)
{
	FXuint mysize;
	s >> mysize;
	i.clear();
	for(FXuint n=0; n<mysize; n++)
	{
		type item;
		s >> item;
		i.append(item);
	}
	return s;
}

//! Bit flags determining the type of sort to be performed by a FX::QValueListQSort
enum QVLQSortType
{
	QVLQSortDefault=0,		//!< Whatever specified in the constructor
	QVLQSortInsertion=1,	//!< Insertion sort
	QVLQSortMerge=2,		//!< Merge sort
	QVLQSortQuick=4,		//!< Quick sort

	QVLQSortStable=256		//!< Prevents use of algorithms which reorder equal elements
};
/*! \class QValueListQSort
\ingroup QTL
\brief Lets you quickly sort a QValueList with custom swapper

This is a fully generic sorting routine which permits compile-time polymorphic instantiation
of custom move, swap and compare routines letting you enable a whole new
world of sorting lists eg; sorting multiple lists which are in the same order
simultaneously by combining iterators to each entry within each into a core list
(see below). This particular implementation mixes three algorithms to achieve
the best performance - small lists (<12) are insertion sorted, medium lists (40,000)
are merge sorted and large lists are quick sorted. With optimisation
enabled, the outputted code is surprisingly slim and efficient.

In fact, for std::list this sort implementation is substantially more efficient than the
STL's as it completely avoids copy construction which can take a significant
amount of time. Items are swapped and moved by relinking their nodes within
the linked list which is perhaps about six pointer exchanges.

Unfortunately there is precious little code on the internet showing how
to do an inplace sort on a \c std::list via only iterators - they seem to
assume that \c std::sort does all you'll ever need. The trouble with iterators
is that when you move them, the relative ordering of the range alters as
well as the old iterator itself becoming invalid. Hence I came up with
my own solution which for small lists does a form of insertion sort whereby
the list is scanned for items in the wrong place which when found cause
another search to find where best to put it. Thus the best case is O(n)
compares with zero moves. However, because this algorithm does not shift
elements down or up like an insertion sort, it's worst case is O(n^2) compares
with O(n) moves.

What about average case? Well, I'm not too hot on this, but the average
permutations for a sequence is n*(n-1)/4 therefore that's the number of
times another half list scan is caused. Hence, average case is also O(n^2)
compares with O(n/2) moves. This sort is stable (does not reorder identical elements).

For medium sized lists, the merge algorithm is used. This has the advantage
of requiring only O(n log n) for both average and worst-case scenarios as
well as the sort output being created sequentially (thus very useful for
sorting bigger-than-memory data). The disadvantage is that it costs up
to half the list to store temporary data (I've chosen a maximum of 640Kb here
or 40,000 nodes) as well becoming quite inefficient in travelling the linked
list nodes in order to calculate the median.

Thus for larger lists, the quick sort algorithm is used which doesn't have
the need for temporary space nor needs to traverse the linked list pointers
as it chooses the first item as pivot. This obviously means you will get
worst-case performance on nearly sorted data ie; O(n^2) whereas its
average case is O(n log n). Unfortunately, quick sort is an unstable sort
whereby identical elements will get reordered.

\note As yet, merge sorting remains unimplemented. Either a quick or insertion
sort is chosen as appropriate.

<h3>Usage:</h3>
If you have three interdependent lists in the same order and want to sort
them, usually it's best to consider merging those lists somehow. If that's
impossible or unwise, then you'll need to sort them all at the same time.

A good example is FX::FXDir which this class was originally written for.
It maintains a QStringList of directory names and creates the QFileInfoList
for those same names only on demand as this is an expensive operation (often
in the order of milliseconds). If however it's already generated, it's
wasteful to throw them away just because the sort order has changed -
therefore, FXDir uses this class to sort the string list and if the
FXFileInfo list is also available, then that too.

Best thing to do is to look at its source (one of the many benefits of
open source). I should point out that the indirection item list which
holds iterators pointing into the string and file info classes must
take care during swaps to not swap itself, only its client data. This
particular problem soaked up over a day of my time as I couldn't figure
out what was going wrong. Hence I mention it here now and hope to save
you all some time! :)

\sa FX::QValueList, FX::Pol::itMove, FX::Pol::itSwap, FX::Pol::itCompare
*/
template<class type,
	template<class> class movePolicy=Pol::itMove,
	template<class> class swapPolicy=Pol::itSwap,
	template<class> class comparePolicy=Pol::itCompare>
class QValueListQSort : public movePolicy<type>, public swapPolicy<type>, public comparePolicy<type>
{
	static const int insertionCutOff=12, mergeCutOff=12; //40000;
public:
	typedef QValueList<type> list;
	typedef typename list::iterator iterator;
protected:
	typedef std::pair<int, iterator> intit;
	mutable list &mylist;
	int sorttype;
public:
	//! Constructs a new instance. Setting no sort type sets them all.
	QValueListQSort(QValueList<type> &list, int _sorttype=QVLQSortDefault)
		: mylist(list), sorttype(_sorttype) { }
	//! Runs the sort
	void run(int _sorttype=QVLQSortDefault)
	{
		if(QVLQSortDefault!=sorttype) sorttype=_sorttype;
		if(!sorttype) sorttype=QVLQSortInsertion|QVLQSortMerge|QVLQSortQuick;
		int s=(int) mylist.size();
		if(s>mergeCutOff && (sorttype & QVLQSortQuick) && !(sorttype & QVLQSortStable))
		{
			intit l=std::make_pair(0, mylist.begin());
			intit u=std::make_pair(s, mylist.end());
			qsort(l, u);
		}
		//else if(s>insertionCutOff && (sorttype & QVLQSortMerge))
		//{
		//	msort();
		//}
		else
		{
			iterator l=mylist.begin(), u=mylist.end();
			isort(l, u);
		}
	}
private:
	void isort(iterator &lb, iterator &ub)
	{	// Niall's bastard iterator based insertion sort
		for(iterator it=lb;;)
		{
			iterator prev=it;
			if(++it==ub) break;
			if(compare(mylist, it, prev))
			{
				bool found=false;
				iterator find=lb;
				for(; find!=ub; ++find)
				{
					if(find!=it && compare(mylist, it, find))
					{
						bool atStart=(find==lb);
						move(mylist, find, it);
						if(atStart) lb=it;
						found=true;
						break;
					}
				}
				if(!found) move(mylist, find, it);
			}
		}
	}
	inline void merge(int len, intit &_lb, intit &_m, intit &_ub)
	{
		bool updlb=true, updm=true, updub=true;
		iterator lb=_lb.second, m=_m.second, ub=_ub.second;
		while(true)
		{
			// To be completed
		}
	}
	void msort(intit &_lb, intit &_ub)
	{
		int len=(_ub.first-_lb.first);
		if(len<=2) return;
		int mididx=len>>1;
		iterator it=_lb.second;
		int idx;
		for(idx=_lb.first; idx<mididx; ++it, ++idx);
		intit m=std::make_pair(idx, it);
		msort(_lb, m);
		msort(m, _ub);
		merge(len, _lb, m, _ub);
	}
	inline intit partition(intit &_lb, intit &_ub)
	{
		bool updlb=true;
		int lbidx=_lb.first, ubidx=_ub.first;
		iterator lb(_lb.second), ub(_ub.second);
		while(true)
		{
			while(lbidx<ubidx && compare(mylist, _lb.second, (--ubidx, --ub)));
			while(lbidx<ubidx && compare(mylist, (updlb=false, ++lbidx, ++lb), _lb.second));
			if(lbidx>=ubidx) break;
			swap(mylist, lb, ub);
			if(updlb) _lb.second=lb;
		}
		if(_lb.second!=ub)
		{
			swap(mylist, _lb.second, ub);
		}
		return std::make_pair(ubidx, ub);
	}
	void qsort(intit &_lb, intit &_ub)
	{
		bool updlb=true, updub=true;
		intit lb(_lb), ub(_ub);
		while(lb.first<ub.first)
		{
			if(ub.first-lb.first<=insertionCutOff)
			{
				isort(lb.second, ub.second);
				if(updlb) _lb.second=lb.second;
				if(updub) _ub.second=ub.second;
				return;
			}
			intit i=partition(lb, ub);
			if(updlb) _lb.second=lb.second;
			if(updub) _ub.second=ub.second;
			int r1=i.first-lb.first, r2=ub.first-i.first-1;
			if(r1<=r2)
			{
				if(r1>0)
				{
					qsort(lb, i);
					if(updlb) _lb.second=lb.second;
				}
				lb.first=i.first+1; lb.second=++i.second; updlb=false;
			}
			else
			{
				++i.first; ++i.second;
				if(r2>0)
				{
					qsort(i, ub);
					if(updub) _ub.second=ub.second;
				}
				ub.first=i.first-1; ub.second=--i.second; updub=false;
			}
		}
	}
};

} // namespace

#endif
