/********************************************************************************
*                                                                               *
*                          Q P t r D i c t   T h u n k                          *
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

#ifndef QPTRDICT_H
#define QPTRDICT_H

#include "qdictbase.h"

namespace FX {

/*! \file qptrdict.h
\brief Defines an implementation of Qt's QPtrDict
*/

/*! \class QPtrDict
\brief An implementation of Qt's QPtrDict

\sa FX::QDict
*/
template<class type> class QPtrDict : public QDictBase<FXuval, type>
{
	FXuval conv(void *v) const
	{
		return reinterpret_cast<FXuval>(v);
	}
public:
	//! Creates a hash table indexed by void pointers. Choose a prime for \em size
	QPtrDict(int size=13, bool wantAutoDel=false) : QDictBase<FXuval, type>(size, wantAutoDel) { }
	//! Inserts item \em d into the dictionary under key \em k
	void insert(void *k, const type *d)
	{
		FXuval k_=conv(k);
		QDictBase<FXuval, type>::insert((FXuint) k_, k_, const_cast<type *>(d));
	}
	//! Replaces item \em d in the dictionary under key \em k
	void replace(void *k, const type *d)
	{
		FXuval k_=conv(k);
		QDictBase<FXuval, type>::replace((FXuint) k_, k_, const_cast<type *>(d));
	}
	//! Deletes the most recently placed item in the dictionary under key \em k
	bool remove(void *k)
	{
		FXuval k_=conv(k);
		return QDictBase<FXuval, type>::remove((FXuint) k_, k_);
	}
	//! Removes the most recently placed item in the dictionary under key \em k without auto-deletion
	type *take(void *k)
	{
		FXuval k_=conv(k);
		return QDictBase<FXuval, type>::take((FXuint) k_, k_);
	}
	//! Finds the most recently placed item in the dictionary under key \em k
	type *find(void *k) const
	{
		FXuval k_=conv(k);
		return QDictBase<FXuval, type>::find((FXuint) k_, k_);
	}
	//! \overload
	type *operator[](void *k) const { return find(k); }
};

/*! \class QPtrDictIterator
\brief An iterator for a QPtrDict
*/
template<class type> class QPtrDictIterator : public QDictBaseIterator<FXuval, type>
{
public:
	QPtrDictIterator() { }
	QPtrDictIterator(const QPtrDict<type> &d) : QDictBaseIterator<FXuval, type>(d) { }
};

//! Writes the contents of the dictionary to stream \em s
template<class type> FXStream &operator<<(FXStream &s, const QPtrDict<type> &i)
{
	FXuint mysize=i.count();
	s << mysize;
	for(QPtrDictIterator<type> it(i); it.current(); ++it)
	{
		s << (FXulong) it.currentKey();
		s << *it.current();
	}
	return s;
}
//! Reads a dictionary from stream \em s
template<class type> FXStream &operator>>(FXStream &s, QPtrDict<type> &i)
{
	FXuint mysize;
	s >> mysize;
	i.clear();
	FXulong key;
	for(FXuint n=0; n<mysize; n++)
	{
		type *item;
		FXERRHM(item=new type);
		s >> key;
		s >> *item;
		i.insert((void *)(FXuval) key, item);
	}
	return s;
}

} // namespace

#endif
