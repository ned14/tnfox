/********************************************************************************
*                                                                               *
*                   G e n e r i c   Q D i c t   T h u n k                       *
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

#ifndef QDICTBASE_H
#define QDICTBASE_H

#include "FXException.h"
#include "FXStream.h"
#include "FXProcess.h"
#include <utility>
#include <vector>
#include <map>

namespace FX {

/*! \file qdictbase.h
\brief Defines classes used in implementing QTL dictionary classes
*/

typedef FXuint uint;

/* ned 19th Oct 2003: Looked through hash_multimap but was not convinced at
all - seems to be hard to get the last element of a group of elements
attached to a key. I may be missing something, but this class is /the/ most
important performance-wise to Tn which uses it very heavily indeed, so I'm
going to opt for my own implementation after all. In the end, right now
(2003) hash_multimap is not part of the C++ STL and it's likely to get
replaced with something similar but different.
*/
template<typename keytype, class type> class QDictBaseIterator;

/*! \class QDictBase
\brief Base implementation class for all QTL dictionary classes

Some notes on observed behaviour deduced by real-world testing:
\li Dictionary is a \c std::vector of \c std::map of item key to
pair of key hash and \c std::vector of pointers to items. Therefore,
each entry minimally consumes an entry in \c std::map, a whole \c std::vector
plus a copy of the key.
\li Lookup time as well as insertion and deletion time should approach
O(1) when dictionary table is 50% greater or more than item count. As \c
std::map is O(log N), it approaches that as dictionary table size becomes
relatively lower.
\li Empirical testing shows an interesting abberation on MSVC (Dinkumware
STL) and GCC (SGI STL) - it's about 40% quicker to insert & delete
within a \c std::map than to add a new entry (which involves constructing a new
\c std::vector and more importantly, copy constructing it a number of times).
If the default memory allocator is used on Win32 the difference gets much worse.
\li dictionaryBias() is lookups less insert/removes - therefore to be biased
in favour of lookups they must outnumber inserts by twice.
*/
template<class keytype, class type> class QDictBase
{
	friend class QDictBaseIterator<keytype, type>;
private:
	typedef std::vector<type *> itemlist; // Simple array of pointers to items
	typedef std::pair<FXuint, itemlist> hashitemlist; // Tuple of (key hash, item list)
protected:
	typedef std::map<keytype, hashitemlist> keyitemlist; // Map of key values to item list
	typedef typename keyitemlist::value_type keyitem;
private:
	typedef std::vector<keyitemlist> dictionary; // Hash-indexed slots
	bool autodel;
	dictionary dict;
	FXuint items, mysize;
	mutable FXuint inserts, lookups;
	typedef std::vector<QDictBaseIterator<keytype, type> *> iteratorlist;
	iteratorlist iterators;
	inline FXuint mkIdx(FXuint hash, FXuint size) const
	{	// 0x9E3779B9 being the fibonacci constant (excellent spread)
		return ((hash*0x9E3779B9)>>8) % size;
	}
	inline FXuint mkSize(FXuint size) const
	{
		//FXuint ret=1;
		//while(ret<size) ret<<=1;
		//return ret;
		if(size<1) size=1;
		return size;
	}
public:
	enum { HasSlowKeyCompare=false };
	//! The type of the key
	typedef keytype KeyType;
	//! The type of the container's item
	typedef type ItemType;
	QDictBase(uint _size, bool wantAutoDel=false) : autodel(wantAutoDel), dict(mkSize(_size)), items(0), mysize(mkSize(_size)), inserts(0), lookups(0) { }
	inline ~QDictBase();
	QDictBase(const QDictBase<keytype, type> &o) : autodel(o.autodel), dict(o.dict), items(o.items), mysize(o.mysize), inserts(o.inserts), lookups(o.lookups) { }
	QDictBase<keytype, type> &operator=(const QDictBase<keytype, type> &o)
	{
		clear();
		autodel=o.autodel; dict=o.dict; items=o.items; mysize=o.mysize; inserts=o.inserts; lookups=o.lookups;
		return (*this);
	}
	//! Returns if auto-deletion is enabled
	bool autoDelete() const { return autodel; }
	//! Sets if auto-deletion is enabled
	void setAutoDelete(bool a) { autodel=a; }

	//! Returns the number of items in the list
	uint count() const { return items; }
	//! Returns true if the list is empty
	bool isEmpty() const { return !items; }
	//! Returns the size of the hash table
	uint size() const { return mysize; }
protected:
	keyitem *findKey(FXuint h, const keytype &k)
	{
		keyitemlist &kil=dict[mkIdx(h, mysize)];
		typename keyitemlist::iterator it=kil.find(k);
		if(it==kil.end()) return 0;
		else return &(*it);
	}
	const keyitem *findKey(FXuint h, const keytype &k) const
	{
		const keyitemlist &kil=dict[mkIdx(h, mysize)];
		typename keyitemlist::const_iterator it=kil.find(k);
		if(it==kil.end()) return 0;
		else return &(*it);
	}
	void insert(FXuint h, const keytype &k, type *d)
	{
		keyitemlist &kil=dict[mkIdx(h, mysize)];
		typename keyitemlist::iterator it=kil.find(k);
		FXEXCEPTION_STL1 {
			if(it==kil.end())
			{
				std::pair<typename keyitemlist::iterator, bool> point=kil.insert(keyitem(k, hashitemlist(h, itemlist())));
				it=point.first;
			}
			keyitem &ki=*it;
			itemlist &il=ki.second.second;
			il.push_back(d);
		} FXEXCEPTION_STL2;
		items++;
		inserts++;
	}
	void replace(FXuint h, const keytype &k, type *d)
	{
		remove(h, k);
		insert(h, k, d);
	}
	inline bool remove(FXuint h, const keytype &k);
	type *take(FXuint h, const keytype &k)
	{
		keyitem *ki=findKey(h, k);
		if(!ki) return 0;
		itemlist &il=ki->second.second;
		if(il.empty()) return 0;
		type *ret=il.back();
		il.pop_back();
		items--;
		inserts++;
		return ret;
	}
	type *find(FXuint h, const keytype &k) const
	{
		lookups++;
		const keyitem *ki=findKey(h, k);
		if(!ki) return 0;
		const itemlist &il=ki->second.second;
		if(il.empty()) return 0;
		return il.back();
	}
public:
	//! Clears the list of items, auto-deleting if enabled
	void clear()
	{
		for(typename dictionary::iterator itdict=dict.begin(); itdict!=dict.end(); ++itdict)
		{
			keyitemlist &kil=*itdict;
			for(typename keyitemlist::iterator itkil=kil.begin(); itkil!=kil.end(); ++itkil)
			{
				keyitem &ki=*itkil;
				itemlist &il=ki.second.second;
				for(typename itemlist::iterator itlist=il.begin(); itlist!=il.end(); ++itlist)
				{
					deleteItem(*itlist);
				}
				il.clear();
			}
			kil.clear();
		}
		items=0;
	}
	//! Resizes the hash table (see QDICTDYNRESIZE()). Invalidates all iterators.
	inline void resize(uint newsize);
	/*! Returns statistics useful for dynamic balancing of the table.
	If full exceeds 100%, then there are more items than slots and the table is
	no longer performing at maximum efficiency. If slotsspread is less than 50%
	then the hash function is broken. Ideally for performance purposes avrgkeysperslot
	should be near 1.0 and finally spread is slotsspread divided by avrgskeysperslot
	which is an overall indication of spread and thus efficiency of the table. Of
	course maximum efficiency is pointless for very large sets of data where avrgkeysperslot
	could be as much as eight - but slotsspread should always be near 100%
	*/
	void spread(float *full, float *slotsspread, float *avrgkeysperslot, float *spread) const
	{
		if(full) *full=(float) 100.0*items/mysize;
		if(slotsspread || avrgkeysperslot || spread)
		{
			FXuint slotsused=0, keys=0;
			for(typename dictionary::const_iterator itdict=dict.begin(); itdict!=dict.end(); ++itdict)
			{
				const keyitemlist &kil=(*itdict);
				if(!kil.empty())
				{
					slotsused++;
					keys+=kil.size();
				}
			}
			if(slotsspread) *slotsspread=(float) 100.0*slotsused/FXMIN(mysize, items);
			if(avrgkeysperslot) *avrgkeysperslot=(float) keys/slotsused;
			if(spread) *spread=((float) 100.0*slotsused/FXMIN(mysize, items))/((float) keys/slotsused);
		}
	}
	//! Prints some statistics about the hash table (only debug builds)
	void statistics() const
	{
#ifdef DEBUG
		float full, slotsspread, avrgkeysperslot, _spread;
		spread(&full, &slotsspread, &avrgkeysperslot, &_spread);
		fxmessage("Dictionary size=%d, items=%d, full=%f%%, slot spread=%f%%, avrg keys per slot=%f, overall spread=%f%%\n", mysize, items, full, slotsspread, avrgkeysperslot, _spread);
#endif
	}
	/*! Returns a number indicating how biased between lookups and insert/removals
	the usage of this dictionary has been so far. Is negative for more insert/removals
	than lookups and positive for the opposite. */
	FXint dictionaryBias() const throw() { return (FXint)(lookups-inserts); }
protected:
	virtual void deleteItem(type *d);
};

// Don't delete void *
/*template<class keytype> inline void QDictBase<keytype, void>::deleteItem(void *)
{
}*/
template<class keytype, class type> inline void QDictBase<keytype, type>::deleteItem(type *d)
{
    if(autodel) delete d;
}

/*! \class QDictBaseIterator
\brief An iterator for a FX::QDictBase
*/
template<typename keytype, class type> class QDictBaseIterator
{
	friend class QDictBase<keytype, type>;
	QDictBase<keytype, type> *mydict;
	typename QDictBase<keytype, type>::dictionary::iterator itdict;
	typename QDictBase<keytype, type>::keyitemlist::iterator itkil;
	typename QDictBase<keytype, type>::itemlist::iterator itil;
	void int_next(int j=1)
	{
		bool firstiter=true;
		typename QDictBase<keytype, type>::dictionary::iterator itdictend=mydict->dict.end();
		while(itdict!=itdictend)
		{
			typename QDictBase<keytype, type>::keyitemlist::iterator itkilend=(*itdict).end();
			if(!firstiter) itkil=(*itdict).begin();
			while(itkil!=itkilend)
			{
				typename QDictBase<keytype, type>::itemlist::iterator itilend=(*itkil).second.second.end();
				if(!firstiter) itil=(*itkil).second.second.begin();
				while(itil!=itilend)
				{
					if(!j) return;
					if(firstiter) ++itil;
					j--;
				}
				++itkil;
				firstiter=false;
			}
			++itdict;
			firstiter=false;
		}
	}
	void int_removeFromDict() const
	{
		for(typename QDictBase<keytype, type>::iteratorlist::iterator it=mydict->iterators.begin(); it!=mydict->iterators.end(); ++it)
		{
			if(this==*it)
			{
				mydict->iterators.erase(it);
				break;
			}
		}
	}
protected:
	type *retptr() const
	{
		if(!mydict || mydict->dict.end()==itdict) return 0;
		if((*itdict).end()==itkil) return 0;
		if((*itkil).second.second.end()==itil) return 0;
		return *itil;
	}
public:
	QDictBaseIterator() : mydict(0) { }
	QDictBaseIterator(const QDictBase<keytype, type> &d)
		: mydict(const_cast<QDictBase<keytype, type> *>(&d))
	{
		toFirst();
		mydict->iterators.push_back(this);
	}
	QDictBaseIterator(const QDictBaseIterator &o) : mydict(o.mydict), itdict(o.itdict), itkil(o.itkil), itil(o.itil)
	{
		if(mydict)
			mydict->iterators.push_back(this);
	}
	~QDictBaseIterator()
	{
		if(mydict)
			int_removeFromDict();
	}
	//! Returns the key associated with what this iterator points to
	const keytype &currentKey() const
	{
		typename QDictBase<keytype, type>::keyitem &ki=*itkil;
		return ki.first;
	}
	QDictBaseIterator &operator=(const QDictBaseIterator &it)
	{
		if(mydict)
			int_removeFromDict();
		mydict=it.mydict;
		itdict=it.itdict;
		itkil=it.itkil;
		itil=it.itil;
		if(mydict)
			mydict->iterators.push_back(this);
		return *this;
	}
	//! Returns the number of items in the list this iterator references
	uint count() const   { return mydict ? mydict->count() : 0; }
	//! Returns true if the list this iterator references is empty
	bool isEmpty() const { return mydict ? mydict.isEmpty() : true; }
	//! Sets the iterator to point to the first item in the list, then returns that item
	type *toFirst()
	{
		if(!mydict) return 0;
		itdict=mydict->dict.begin();
		itkil=(*itdict).begin();
		if((*itdict).end()!=itkil)
			itil=(*itkil).second.second.begin();
		else int_next();
		return retptr();
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
		return this->operator+=(1);
	}
	//! Increments the iterator
	type *operator+=(uint j)
	{
		if(!retptr()) return 0;
		int_next(j);
		return retptr();
	}
};

template<class keytype, class type> inline QDictBase<keytype, type>::~QDictBase()
{
	clear();
	for(typename iteratorlist::iterator it=iterators.begin(); it!=iterators.end(); ++it)
	{
		(*it)->mydict=0;
	}
}

template<class keytype, class type> inline bool QDictBase<keytype, type>::remove(FXuint h, const keytype &k)
{
	keyitemlist &kil=dict[mkIdx(h, mysize)];
	typename keyitemlist::iterator it=kil.find(k);
	if(it==kil.end()) return false;
	itemlist &il=(*it).second.second;
	if(il.empty()) return false;
	for(typename iteratorlist::iterator it2=iterators.begin(); it2!=iterators.end(); ++it2)
	{
		QDictBaseIterator<keytype, type> *dictit=*it2;
		if(dictit->itil==--il.end())
		{	// Advance the iterator
			++(*dictit);
		}
	}
	deleteItem(il.back());
	il.pop_back();
	items--;
	inserts++;
	if(il.empty())
	{	// Delete the key entry
		kil.erase(it);
	}
	return true;
}

template<class keytype, class type> inline void QDictBase<keytype, type>::resize(uint newsize)
{
	newsize=mkSize(newsize);
	if(mysize!=newsize)
	{
		FXEXCEPTION_STL1 {
			dictionary newdict(newsize);
			for(typename dictionary::iterator itdict=dict.begin(); itdict!=dict.end(); ++itdict)
			{
				keyitemlist &kil=*itdict;
				for(typename keyitemlist::iterator itkil=kil.begin(); itkil!=kil.end(); ++itkil)
				{
					keyitem &ki=*itkil;
					hashitemlist &hil=ki.second;
					FXuint h=hil.first;
					itemlist &il=hil.second;

					keyitemlist &nkil=newdict[mkIdx(h, newsize)];
					// Really could do with move semantics here :(
					nkil.insert(keyitem(ki.first, hashitemlist(h, il)));
				}
			}
			dict=newdict;
			mysize=newsize;
			// Kill all iterators
			for(typename iteratorlist::iterator it2=iterators.begin(); it2!=iterators.end(); ++it2)
			{
				QDictBaseIterator<keytype, type> *dictit=*it2;
				dictit->mydict=0;
			}
		} FXEXCEPTION_STL2;
	}
}


/*! Useful macro which dynamically resizes a FX::QDictBase subclass
according to a number of runtime factors:
\li If there aren't many items in the container (up to 64 or 16 if using
a string key), uses a table size of one (ie; causes pure binary search). Depending
on compile time defines will do the same if the container is doing more
inserts/removals than lookups (via QDictBase::dictionaryBias()).
\li If memory load is low (0), will only shrink when dictionary contents
fall below one quarter of the dictionary table size.

Otherwise it ensures dictionary table size is twice that of contents
and is a prime number (via FX::fx2powerprimes) shifted right by memory load.
*/
#ifdef DEBUG
#define QDICTDYNRESIZE(dict) FX::QDictByMemLoadResize(dict, __FILE__, __LINE__)
#else
#define QDICTDYNRESIZE(dict) FX::QDictByMemLoadResize(dict)
#endif

template<class dicttype> inline bool QDictByMemLoadResize(dicttype &dict, const char *file=0, int lineno=0)
{
	FXuint memload=FXProcess::memoryFull(), dictcount=dict.count(), dictsize=dict.size();
	FXuint newsize=dictsize;
	if(dictcount<=16+48*dicttype::HasSlowKeyCompare
#ifndef FXDISABLE_QDICTSLOWINSERTSOPT
		|| dict.dictionaryBias()<16 /* slush factor */)
#else
		)
#endif
	{	// Better performance if we leave it purely to std::map
		newsize=1;
	}
	else if(memload || dictcount>=dictsize/2 || (dictsize>=15 && dictcount<=dictsize/4))
	{	// 15*(4+4+12+4)=15*24=360 bytes
		const FXuint *primes=fx2powerprimes(dictcount*2);
		newsize=primes[memload];
	}
	if(newsize!=dictsize)
	{
#ifdef DEBUG
		fxmessage("QDICTDYNRESIZE at %s:%d resizing %p from %u to %u (load %d)\n", file, lineno, &(dict), dictsize, newsize, memload);
#endif
		dict.resize(newsize);
		return true;
	}
	return false;
}

/*! Useful macro which dynamically resizes a FX::QDictBase subclass
similarly to QDICTDYNRESIZE but more aggressively for speed:
\li Table is only ever shrunk when memory is loaded and even then,
only when contents fall below one eighth that of the table size.
\li Table size is always double the contents or higher
*/
#ifdef DEBUG
#define QDICTDYNRESIZEAGGR(dict) FX::QDictByMemLoadResizeAggr(dict, __FILE__, __LINE__)
#else
#define QDICTDYNRESIZEAGGR(dict) FX::QDictByMemLoadResizeAggr(dict)
#endif

template<class dicttype> inline bool QDictByMemLoadResizeAggr(dicttype &dict, const char *file=0, int lineno=0)
{
	FXuint memload=FXProcess::memoryFull(), dictcount=dict.count(), dictsize=dict.size();
	FXuint newsize=dictsize;
	if(dictcount<=16+48*dicttype::HasSlowKeyCompare
#ifndef FXDISABLE_QDICTSLOWINSERTSOPT
		|| dict.dictionaryBias()<16 /* slush factor */)
#else
		)
#endif
	{	// Better performance if we leave it purely to std::map
		newsize=1;
	}
	else if(dictcount>=dictsize/2 || (memload && dictsize>=31 && dictcount<=dictsize/8))
	{	// 31*(4+4+12+4)=31*24=744 bytes
		const FXuint *primes=fx2powerprimes(dictcount*2);
		newsize=primes[0];
	}
	if(newsize!=dictsize)
	{
#ifdef DEBUG
		fxmessage("QDICTDYNRESIZEAGGR at %s:%d resizing %p from %u to %u (load %d)\n", file, lineno, &(dict), dictsize, newsize, memload);
#endif
		dict.resize(newsize);
		return true;
	}
	return false;
}

} // namespace

#endif
