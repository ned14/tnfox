/********************************************************************************
*                                                                               *
*                            QStringList implementation                         *
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

#ifndef QSTRINGLIST_H
#define QSTRINGLIST_H

#include "FXString.h"
#include "qvaluelist.h"

namespace FX {

/*! \file qstringlist.h
\brief Defines a thunk of Qt's QStringList to the STL
*/

/*! \class QStringList
\brief A thunk of Qt's QStringList to the STL

This is a very incomplete implementation, but I have little call for anything
more. If you want more, email me.
*/
class QStringList : public QValueList<FXString>
{
public:
	QStringList() : QValueList<FXString>() { }
	QStringList(const QStringList &l) : QValueList<FXString>(l) { }
	FXString &operator[](size_type i) { return *at(i); }
	const FXString &operator[](size_type i) const { return *at(i); }
	QStringList &operator+=(const FXString &d) { QValueList<FXString>::operator+=(d); return *this; }
	//! Joins together the members of the list separated by \em sep
	FXString join(const FXString &sep) const
	{
		bool first=true;
		FXString ret;
		for(const_iterator it=begin(); it!=end(); ++it)
		{
			if(!first) ret+=sep;
			ret+=*it;
			first=false;
		}
		return ret;
	}
};

} // namespace

#endif
