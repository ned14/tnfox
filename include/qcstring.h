/********************************************************************************
*                                                                               *
*                    Q t   B y t e   A r r a y   t h u n k                      *
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

#ifndef QCSTRING_H
#define QCSTRING_H

#include <qmemarray.h>

namespace FX {

/*! \file qcstring.h
\brief Defines a thunk of Qt's QByteArray to the STL
*/

/*! \class QByteArray
\ingroup QTL
\brief A thunk of Qt's QByteArray to the STL

To aid porting of Qt programs to FOX.

\note Qt's QByteArray is QMemArray<char> whereas ours is QMemArray<unsigned char>.
Why? Because byte data should be unsigned!

\sa QMemArray
*/
class QByteArray : public QMemArray<unsigned char>
{
public:
	//! Constructs an empty array of \em type
	QByteArray() : QMemArray<unsigned char>() { }
	//! Constructs an array of \em type \em size long
	QByteArray(FXuval size) : QMemArray<unsigned char>(size) { }
	//! Constructs an array using an external array
	QByteArray(unsigned char *a, uint n, bool noDeleteExtArray=true) : QMemArray<unsigned char>(a, n, noDeleteExtArray) { }
};

} // namespace

#endif
