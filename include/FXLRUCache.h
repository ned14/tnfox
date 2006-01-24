/********************************************************************************
*                                                                               *
*                    A Generic Least Recently Used Cache                        *
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

#ifndef FXLRUCACHE_H
#define FXLRUCACHE_H

#include "FXProcess.h"
#include "qdictbase.h"
#include <list>

namespace FX {

/*! \file FXLRUCache.h
\brief Defines classes implementing a generic Least Recently Used Cache
*/

namespace FXLRUCacheImpl
{
	template<typename Type, typename TypeAsParam, bool isValue> struct getPtr
	{
		Type item;
		getPtr(TypeAsParam _item) : item(_item) { }
		Type itemPtr() throw() { return item; }
	};
	template<typename Type, typename TypeAsParam> struct getPtr<Type, TypeAsParam, true>
	{
		Type item;
		getPtr(TypeAsParam _item) : item(_item) { }
		Type *itemPtr() throw() { return &item; }
	};
}

/*! \class FXLRUCache
\brief A generic Least Recently Used (LRU) Cache (Qt compatible)

To maximise performance in modern systems which tend to have plenty of free
RAM, caching can be used to speed up cases where possibly the same processing
may need to be repeated. When I wrote this class, I had the Tornado kernel
namespace nodes in mind, but it's generically applicable to any similar
situation.

Some effort is made to replicate the Qt API for QCache, sufficiently that
with suitable <tt>typedef</tt>-ing of the template parameters you can
make a compatible QCache or QIntCache. However anything that FX::QDictBase
can be configured with can also make a FXLRUCache.

One interesting feature of FXLRUCache is that it can handle value objects.
This was done primarily with FX::FXRefingObject in mind but its use exists
outside of that. To enable FXLRUCache as a value-based container, use
the following form:
\code
typedef FXLRUCache<QDict<Foo>, Foo> FooValueCache;
\endcode
Leaving out the second template parameter causes FXLRUCache to use the
dictionary's type which of course is always a pointer. If value-based
semantics are enabled, auto-deletion becomes always enabled.

If dynamic mode is enabled, maxCost() is shifted right by
FXProcess::memoryFull(). By default dynamic mode is enabled.
*/
template<class dictbase, class type=Generic::NullType> class FXLRUCache : protected dictbase
{
	template<class cache> friend class FXLRUCacheIterator;
public:
	typedef typename dictbase::KeyType KeyType;
	typedef typename dictbase::ItemType FundamentalDictType;
protected:
	typedef FundamentalDictType *DictType;
	typedef typename Generic::select<Generic::sameType<type, Generic::NullType>::value, DictType, type>::value Type;
	typedef Generic::TraitsBasic<Type> TypeTraits;
	typedef typename Generic::TraitsBasic<KeyType>::asROParam KeyTypeAsParam;
	typedef typename TypeTraits::asROParam TypeAsParam;
	typedef typename Generic::leastIndir<Type>::value *TypeAsReturn;
	FXuint topmax, maximum, cost;
	bool amDynamic;
	typedef FXLRUCacheImpl::getPtr<Type, TypeAsParam, TypeTraits::isValue> CacheItemBase;
	struct CacheItem : public CacheItemBase
	{
		FXint cost;
		KeyType key;
		typename std::list<CacheItem>::iterator myit;	// Points to this
		CacheItem(FXint _cost, KeyTypeAsParam _key, TypeAsParam _item)
			: CacheItemBase(_item), cost(_cost), key(_key) { }
	};
	std::list<CacheItem> cache;
	struct stats_t
	{
		FXuint inserted, removed, flushed, hits, misses;
		stats_t() : inserted(0), removed(0), flushed(0), hits(0), misses(0) { }
	} stats;
	/*! Called when totalCost() exceeds maxCost() to purge the least recently
	used items. The default implementation works on the basis of the least
	recently looked up via find(), however this is virtual so you can override
	it with a more intelligent mechanism */
	virtual void purgeLFU()
	{
		while(cost>maximum)
		{
			CacheItem *ci=&cache.back();
			remove(ci->key);
			--stats.removed; ++stats.flushed;
		}
	}
private:
	void dynMax()
	{
		maximum=amDynamic ? topmax>>FXProcess::memoryFull() : topmax;
		if(maximum<cost)
			purgeLFU();
	}
public:
	//! Constructs an instance with maximum cost \em maxCost and dictionary size \em size
	FXLRUCache(FXuint maxCost=100, FXuint size=13, bool autodel=false)
		: dictbase(size), topmax(maxCost), maximum(maxCost), cost(0), amDynamic(true)
	{
		dynMax();
		setAutoDelete(autodel);
	}
	using dictbase::autoDelete;
	void setAutoDelete(bool a)
	{
		if(!TypeTraits::isValue) dictbase::setAutoDelete(a);
	}
	using dictbase::count;
	using dictbase::isEmpty;
	using dictbase::size;
	using dictbase::clear;
	using dictbase::resize;
	using dictbase::spread;

	//! Returns if this cache adjusts itself dynamically to memory full
	bool dynamic() const throw() { return amDynamic; }
	//! Sets if this cache adjusts itself dynamically to memory full
	void setDynamic(bool v) const throw() { amDynamic=v; }
	//! Returns the maximum cost permitted by the cache
	FXuint maxCost() const throw() { return maximum; }
	//! Sets the maximum cost permitted. Disposes of items immediately if the new cost warrants it.
	void setMaxCost(FXuint newmax)
	{
		topmax=newmax;
		dynMax();
	}
	//! Returns the total cost of the cache's current contents
	FXuint totalCost() const throw() { return cost; }
	/*! Returns operating statistics about the cache. \em hitrate
	is a percentage of hits versus total lookups and \em churn is
	a percentage of items flushed versus total lookups.
	*/
	void cacheStats(float *hitrate, float *churn) const throw()
	{
		FXuint lookups=stats.hits+stats.misses;
		if(!lookups) lookups=1;
		if(hitrate)
			*hitrate=100.0*stats.hits/lookups;
		if(churn)
			*churn=100.0*stats.flushed/lookups;
	}
	//! Prints some statistics about the hash table (only debug builds)
	void statistics() const
	{
#ifdef DEBUG
		float full, slotsspread, avrgkeysperslot, _spread, hitrate, churn;
		spread(&full, &slotsspread, &avrgkeysperslot, &_spread);
		cacheStats(&hitrate, &churn);
		fxmessage("Dictionary size=%d, items=%d, full=%f%%, slot spread=%f%%, avrg keys per slot=%f, overall spread=%f%%\n"
			      "Cache hit rate=%d%%, churn=%d%%\n", size(), count(), full, slotsspread, avrgkeysperslot, _spread, hitrate, churn);
#endif
	}

	/*! \return False if item's cost is bigger than maxCost()

	Inserts an item \em d into the cache under key \em k with cost \em itemcost.
	If the total cost of all the items in the cache exceed maxCost(), the least
	recently used item is flushed. */
	bool insert(KeyTypeAsParam k, TypeAsParam d, FXint itemcost=1)
	{
		if(cost>maximum) return false;
		FXEXCEPTION_STL1 {
			cache.push_front(CacheItem(itemcost, k, d));
			cache.front().myit=cache.begin();
		} FXEXCEPTION_STL2;
		dictbase::insert(k, (DictType) &cache.front());
		cost+=itemcost; ++stats.inserted;
		dynMax();
		return true;
	}
	//! Removes the item associated with key \em k, deleting if auto-deletion is enabled
	bool remove(KeyTypeAsParam k)
	{
		bool ret=dictbase::remove(k);
		++stats.removed;
		return ret;
	}
	/*! Removes the item associated with key \em k, never deleting except
	when a value-based container (in which case, always returns zero) */
	TypeAsReturn take(KeyTypeAsParam k)
	{
		CacheItem *ci=(CacheItem *) dictbase::take(k);
		++stats.removed;
		cost-=ci->cost;
		TypeAsReturn ret=TypeTraits::isValue ? 0 : ci->itemPtr();
		cache.erase(ci->myit);
		return ret;
	}
	/*! Finds the item associated with key \em k, returning zero if not found.
	Marks the item as most recently used if \em tofront is true.
	*/
	TypeAsReturn find(KeyTypeAsParam k, bool tofront=true) const
	{
		CacheItem *ci=(CacheItem *) dictbase::find(k);
		if(!ci)
		{
			++stats.misses;
			return 0;
		}
		++stats.hits;
		if(tofront)
		{
			cache.splice(cache.begin(), cache, ci->myit);
			// ci->myit=cache.begin();
		}
		return ci->itemPtr();
	}
	//! \overload
	TypeAsReturn operator[](KeyTypeAsParam k) const { return find(k); }

protected:
	virtual void deleteItem(DictType d)
	{
		CacheItem *ci=(CacheItem *) d;
		FXint itemcost=ci->cost;
		if(!TypeTraits::isValue)
			dictbase::deleteItem(ci->itemPtr());
		cache.erase(ci->myit);
		cost-=itemcost;
	}
};

/*! \class FXLRUCacheIterator
\brief An iterator for a FXLRUCache

Unlike other QTL type iterators in TnFOX, this takes the FXLRUCache type
as template parameter.
\sa FX::FXLRUCache
*/
template<class cache> class FXLRUCacheIterator
	: public QDictBaseIterator<typename cache::KeyType, typename cache::FundamentalDictType>
{
public:
	FXLRUCacheIterator(const cache &d) : QDictBaseIterator<typename cache::KeyType, typename cache::FundamentalDictType>(d) { }
	/* NOTE TO SELF: Relies on the compiler putting FXLRUCacheImpl::getPtr at the
	front of CacheItem (undefined behaviour). Likely to be fine on almost any
	compiler as it's not virtual and base classes go first */
};

} // namespace

#endif
