/********************************************************************************
*                                                                               *
*                        Base synchronous i/o device                            *
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

#ifndef QIODEVICES_H
#define QIODEVICES_H

#include "QIODevice.h"

namespace FX {

/*! \file QIODeviceS.h
\brief Defines classes and values used for synchronous i/o
*/

/*! \defgroup siodevices Synchronous i/o devices
\ingroup categorised

This is a list of all synchronous i/o devices ie; those which transfer
data between two conceptual ends. These are characterised by reads and
writes blocking if the other end hasn't supplied/read data yet.
*/

/*! \class QIODeviceS
\ingroup siodevices
\brief The abstract base class for all synchronous i/o classes in TnFOX

You'll also want to see FX::QIODevice. All subclasses of this base class
are i/o devices which provide a synchronous functionality.

You can wait for data to become available on any one of a number of QIODeviceS's
using the static method waitForData().

*/
class FXAPI QIODeviceS : public QIODevice
{
protected:
	QIODeviceS(const QIODeviceS &o) : QIODevice(o) { }
public:
	QIODeviceS() : QIODevice() { }
	virtual bool isSynchronous() const { return true; }

	//!	Creates the server side of the device
	virtual bool create(FXuint mode=IO_ReadWrite)=0;
	//! Creates the client side of the device
	virtual bool open(FXuint mode=IO_ReadWrite)=0;
	//! Does nothing as synchronous devices can't be truncated
	virtual void truncate(FXfval) { }
	//! Returns 0 because synchronous devices don't have a current file pointer
	virtual FXfval at() const { return 0; }
	//! Returns false because you can't set the current file pointer on a synchronous device
	virtual bool at(FXfval) { return false; }
	//! Default implementation returning true if size() is zero
	virtual bool atEnd() const { return size()==0; }
	// NOTE: Next two are defined in QIODevice.cxx
	//! Default implementation throws an exception
	virtual FXuval readBlockFrom(char *data, FXuval maxlen, FXfval pos);
	//! Default implementation throws an exception
	virtual FXuval writeBlockTo(FXfval pos, const char *data, FXuval maxlen);
public:
	/*! Waits for more data to become available for reading on any one or
	more of an array of QIODeviceS's specified by \em list. Precisely which
	are those left in the array \em signalled if it's not NULL with empty
	members being zero.
	\warning Beware race conditions caused by waiting on i/o devices which
	can be read asynchronously by other threads
	\note This is a thread cancellation point
	*/
	static bool waitForData(QIODeviceS **signalled, FXuint no, QIODeviceS **list, FXuint waitfor=FXINFINITE);
	//! Returns the maximum number of QIODeviceS's which can be waited for at once
	static FXuint waitForDataMax() throw();
private:
	friend class QSSLDevice;
	virtual FXDLLLOCAL void *int_getOSHandle() const=0;
};

} // namespace

#endif
