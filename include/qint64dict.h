/********************************************************************************
*                                                                               *
*                       Q I n t 6 4 D i c t   T h u n k                         *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2004 by Niall Douglas.   All Rights Reserved.            *
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

#ifndef QINT64DICT_H
#define QINT64DICT_H

#include "qdictbase.h"

namespace FX {

/*! \file qint64dict.h
\brief Defines a QIntDict with 64 bit keys
*/

/*! \class QInt64Dict
\ingroup QTL
\brief A QIntDict with 64 bit keys

\sa FX::QDict
*/
template<class type> class QInt64Dict : public QDictBase<FXlong, type>
{
	typedef QDictBase<FXlong, type> Base;
	inline FXuint hash(FXlong k) const throw() { return (FXuint)((k>>32)^(k & 0xffffffff)); }
public:
	//! Creates a hash table indexed by FXlong's. Choose a prime for \em size
	explicit QInt64Dict(int size=13, bool wantAutoDel=false) : Base(size, wantAutoDel) { }
	~QInt64Dict() { Base::clear(); }
	FXADDMOVEBASECLASS(QInt64Dict, Base)
	//! Inserts item \em d into the dictionary under key \em k
	void insert(FXlong k, const type *d)
	{
		Base::insert(hash(k), k, const_cast<type *>(d));
	}
	//! Replaces item \em d in the dictionary under key \em k
	void replace(FXlong k, const type *d)
	{
		Base::replace(hash(k), k, const_cast<type *>(d));
	}
	//! Deletes the most recently placed item in the dictionary under key \em k
	bool remove(FXlong k)
	{
		return Base::remove(hash(k), k);
	}
	//! Removes the most recently placed item in the dictionary under key \em k without auto-deletion
	type *take(FXlong k)
	{
		return Base::take(hash(k), k);
	}
	//! Finds the most recently placed item in the dictionary under key \em k
	type *find(FXlong k) const
	{
		return Base::find(hash(k), k);
	}
	//! \overload
	type *operator[](FXlong k) const { return find(k); }
protected:
	virtual void deleteItem(type *d);
};

template<class type> inline void QInt64Dict<type>::deleteItem(type *d)
{
	if(Base::autoDelete())
	{
		//fxmessage("QDB delete %p\n", d);
		delete d;
	}
}
// Don't delete void *
template<> inline void QInt64Dict<void>::deleteItem(void *)
{
}

/*! \class QInt64DictIterator
\ingroup QTL
\brief An iterator for a QInt64Dict
*/
template<class type> class QInt64DictIterator : public QDictBaseIterator<FXlong, type>
{
public:
	QInt64DictIterator() { }
	QInt64DictIterator(const QInt64Dict<type> &d) : QDictBaseIterator<FXlong, type>(d) { }
};

//! Writes the contents of the dictionary to stream \em s
template<class type> FXStream &operator<<(FXStream &s, const QInt64Dict<type> &i)
{
	FXuint mysize=i.count();
	s << mysize;
	for(QInt64DictIterator<type> it(i); it.current(); ++it)
	{
		s << it.currentKey();
		s << *it.current();
	}
	return s;
}
//! Reads a dictionary from stream \em s
template<class type> FXStream &operator>>(FXStream &s, QInt64Dict<type> &i)
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
		i.insert(key, item);
	}
	return s;
}

} // namespace

#endif
