/********************************************************************************
*                                                                               *
*                          Q I n t D i c t   T h u n k                          *
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

#ifndef QINTDICT_H
#define QINTDICT_H

#include "qdictbase.h"

namespace FX {

/*! \file qintdict.h
\brief Defines an implementation of Qt's QIntDict
*/

/*! \class QIntDict
\ingroup QTL
\brief An implementation of Qt's QIntDict

\sa FX::QDict
*/
template<class type> class QIntDict : public QDictBase<FXint, type>
{
public:
	//! Creates a hash table indexed by FXint's. Choose a prime for \em size
	explicit QIntDict(int size=13, bool wantAutoDel=false) : QDictBase<FXint, type>(size, wantAutoDel) { }
	~QIntDict() { QDictBase<FXint, type>::clear(); }
	//! Inserts item \em d into the dictionary under key \em k
	void insert(FXint k, const type *d)
	{
		QDictBase<FXint, type>::insert(k, k, const_cast<type *>(d));
	}
	//! Replaces item \em d in the dictionary under key \em k
	void replace(FXint k, const type *d)
	{
		QDictBase<FXint, type>::replace(k, k, const_cast<type *>(d));
	}
	//! Deletes the most recently placed item in the dictionary under key \em k
	bool remove(FXint k)
	{
		return QDictBase<FXint, type>::remove(k, k);
	}
	//! Removes the most recently placed item in the dictionary under key \em k without auto-deletion
	type *take(FXint k)
	{
		return QDictBase<FXint, type>::take(k, k);
	}
	//! Finds the most recently placed item in the dictionary under key \em k
	type *find(FXint k) const
	{
		return QDictBase<FXint, type>::find(k, k);
	}
	//! \overload
	type *operator[](FXint k) const { return find(k); }
protected:
	virtual void deleteItem(type *d);
};

template<class type> inline void QIntDict<type>::deleteItem(type *d)
{
	if(QDictBase<FXint, type>::autoDelete())
	{
		//fxmessage("QDB delete %p\n", d);
		delete d;
	}
}
// Don't delete void *
template<> inline void QIntDict<void>::deleteItem(void *)
{
}

/*! \class QIntDictIterator
\ingroup QTL
\brief An iterator for a QIntDict
*/
template<class type> class QIntDictIterator : public QDictBaseIterator<FXint, type>
{
public:
	QIntDictIterator() { }
	QIntDictIterator(const QIntDict<type> &d) : QDictBaseIterator<FXint, type>(d) { }
};

//! Writes the contents of the dictionary to stream \em s
template<class type> FXStream &operator<<(FXStream &s, const QIntDict<type> &i)
{
	FXuint mysize=i.count();
	s << mysize;
	for(QIntDictIterator<type> it(i); it.current(); ++it)
	{
		s << (FXlong) it.currentKey();
		s << *it.current();
	}
	return s;
}
//! Reads a dictionary from stream \em s
template<class type> FXStream &operator>>(FXStream &s, QIntDict<type> &i)
{
	FXuint mysize;
	s >> mysize;
	i.clear();
	FXlong key;
	for(FXuint n=0; n<mysize; n++)
	{
		type *item;
		FXERRHM(item=new type);
		s >> key;
		s >> *item;
		i.insert((FXint) key, item);
	}
	return s;
}

} // namespace

#endif
