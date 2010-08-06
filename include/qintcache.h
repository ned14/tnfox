/********************************************************************************
*                                                                               *
*                        Q I n t C a c h e    T h u n k                         *
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

#ifndef QINTCACHE_H
#define QINTCACHE_H

#include "FXLRUCache.h"
#include "qintdict.h"

namespace FX {

/*! \file qintcache.h
\brief Defines an implementation of Qt's QIntCache
*/

/*! \class QIntCache
\brief An implementation of Qt's QIntCache

\sa FX::QIntDict
*/
template<class type> class QIntCache : public FXLRUCache< QIntDict<type> >
{
	typedef FXLRUCache< QIntDict<type> > Base;
public:
	//! Constructs an instance with maximum cost \em maxCost and dictionary size \em size
	QIntCache(FXuint maxCost=100, FXuint size=13, bool autodel=false)
		: Base(maxCost, size, autodel) { }
};

/*! \class QIntCacheIterator
\brief An iterator for a QIntCache
\sa FX::QIntCache
*/
template<class type> class QIntCacheIterator : public FXLRUCacheIterator< FXLRUCache< QIntDict<type> > >
{
public:
	QIntCacheIterator(const QIntCache<type> &d) : FXLRUCacheIterator< FXLRUCache< QIntDict<type> > >(d) { }
};

} // namespace

#endif
