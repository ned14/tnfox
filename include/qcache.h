/********************************************************************************
*                                                                               *
*                           Q C a c h e    T h u n k                            *
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

#ifndef QCACHE_H
#define QCACHE_H

#include "FXLRUCache.h"
#include "qdict.h"

namespace FX {

/*! \file qcache.h
\brief Defines an implementation of Qt's QCache
*/

/*! \class QCache
\brief An implementation of Qt's QCache

\sa FX::QDict
*/
template<class type> class QCache : public FXLRUCache< QDict<type> >
{
	typedef FXLRUCache< QDict<type> > Base;
public:
	//! Constructs an instance with maximum cost \em maxCost and dictionary size \em size with optional case sensitivity
	QCache(FXuint maxCost=100, FXuint size=13, bool caseSensitive=true, bool autodel=false)
		: Base(maxCost, size, autodel)
	{
		setCaseSensitive(caseSensitive);
	}
	using Base::caseSensitive;
	using Base::setCaseSensitive;
};

/*! \class QCacheIterator
\brief An iterator for a QCache
\sa FX::QCache
*/
template<class type> class QCacheIterator : public FXLRUCacheIterator< FXLRUCache< QDict<type> > >
{
public:
	QCacheIterator(const QCache<type> &d) : FXLRUCacheIterator< FXLRUCache< QDict<type> > >(d) { }
};

} // namespace

#endif
