/********************************************************************************
*                                                                               *
*                          Filter device applying .bz2                          *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2006 by Niall Douglas.   All Rights Reserved.            *
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

#ifndef QBZIP2DEVICE_H
#define QBZIP2DEVICE_H

#include "QIODevice.h"

namespace FX {

/*! \file QBZip2Device.h
\brief Defines classes used in translating .bz2 files
*/

/*! \class QBZip2Device
\ingroup fiodevices
\brief Provides a filter i/o device which transparently translates .bz2 files

Much along the lines of FX::QGZipDevice, this takes a QIODevice which accesses
the .bz2 file and provides the decompressed version to anything using it. You can use
it for reading or writing, though unless seeking is enabled during construction,
you may only perform linear reading or writing.

<h3>Implementation notes:</h3>
If seeking is enabled, the class internally decompresses to a QBuffer on open()
and all work is done to and from this buffer. Only on close() or flush() is the
data in the buffer bzipped back to the bz2data source. This obviously implies
the usage of RAM to the size of the decompressed data. If you will not be
performing any seeking, you can save this memory usage. Note that if seeking
is disabled, the following functions do not work or return default values:
size(), truncate(), at(FXfval), atEnd(), readBlockFrom(), writeBlockTo(),
ungetch().

The source if not already open is opened on open() - if being reopened
it does not reset the file pointer so ensure it's at the right place.
After open() it leaves the file pointer pointing after the .bz2 data.

On close() or flush() when seeking is enabled, the file pointer is first set to zero and after
writing a truncate() is issued to the source to remove any extraneous data.
If opened as read only or write only, the device behaves correctly and
doesn't issue anything incorrect (or at least it shouldn't!). QBZip2Device
never closes the source when you close() it.

\note Currently this class does not implement CR/LF translation nor UTF
translation when seeking is disabled.

\note You need the bzip2 library available for this to work. If it's missing
an exception is generated if you try to use it.

This class uses the bzip2 library (C) 1996-2002 Julian R Seward
*/
struct QBZip2DevicePrivate;
class FXAPIR QBZip2Device : public QIODevice
{
	QBZip2DevicePrivate *p;
	QBZip2Device(const QBZip2Device &);
	QBZip2Device &operator=(const QBZip2Device &);
public:
	//! Constructs an instance, with \em enableSeeking determining if seeking can be performed
	QBZip2Device(QIODevice *gzdata=0, int compression=9, bool enableSeeking=true);
	~QBZip2Device();
	//! Returns the device being used as .bz2 source
	QIODevice *BZ2Data() const;
	//! Sets the device being usied as .bz2 source
	void setBZ2Data(QIODevice *gzdata);

	virtual bool open(FXuint mode);
	virtual void close();
	virtual void flush();
	virtual FXfval size() const;
	virtual void truncate(FXfval size);
	virtual FXfval at() const;
	virtual bool at(FXfval newpos);
	virtual bool atEnd() const;
	virtual FXuval readBlock(char *data, FXuval maxlen);
	virtual FXuval writeBlock(const char *data, FXuval maxlen);
	virtual FXuval readBlockFrom(char *data, FXuval maxlen, FXfval pos);
	virtual FXuval writeBlockTo(FXfval pos, const char *data, FXuval maxlen);
	virtual int getch();
	virtual int putch(int c);
	virtual int ungetch(int c);
};

} // namespace

#endif
