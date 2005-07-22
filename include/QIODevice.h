/********************************************************************************
*                                                                               *
*                                Base i/o device                                *
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

#ifndef QIODEVICE_H
#define QIODEVICE_H

#include "fxdefs.h"

namespace FX {

/*! \file QIODevice.h
\brief Defines classes and values used for i/o
*/

/*! \defgroup fiodevices File type i/o devices
\ingroup categorised

This is a list of all file type i/o devices ie; those which access a flat
patch of contiguous data
*/

class FXACL;

//! A set of flags passed to FX::QIODevice::open() combined bitwise
enum QIODeviceFlags
{
	IO_ReadOnly=	0x0001,		//!< This specifies that the device should be opened for read-only access
	IO_WriteOnly=	0x0002,		//!< This specifies that the device should be opened for write-only access
	IO_ReadWrite=	0x0003,		//!< This specifies that the device should be opened for read-write access
	IO_Append=		0x0004,		//!< This specifies that the device should be opened for append access
	IO_Truncate=	0x0008,		//!< This specifies that the device should be opened and truncated
	/*! This specifies that i/o to & from the device should be CR/LF translated (ie; text)
	\warning Most i/o devices in TnFOX handle this very inefficiently when doing small
	block transfers sometimes even needing to rewind the file pointer by a byte in case
	a newline was missed. If it's a large quantity of text, it may be worth while
	reading in binary and doing translation yourself. */
	IO_Translate=	0x0010,
	/*! This specifies that truncates to smaller than the current file size should
	destroy the data being truncated first. This prevents data "leaking" out of a
	file into the disc's free space. See FX::QIODevice::shredData() */
	IO_ShredTruncate=0x080,
	IO_ModeMask=	0x00ff,		//!< This can be used to mask out the mode() flags

	IO_Raw=			0x0100,		//!< Causes immediate buffer flushes after every write operation
	IO_QuietSocket=	0x0200,		//!< Prevents server sockets from listening (see FX::QBlkSocket)
	IO_DontUnlink=  0x0400,		//!< Prevents creator deleting its entry on close()
	IO_Open=		0x1000,		//!< This is set if the device is currently open
	IO_StateMask=   0xf000		//!< This can be used to mask out the state() flags
};

/*! \class QIODevice
\ingroup fiodevices
\ingroup security
\brief The abstract base class for all i/o classes in TnFOX (Qt compatible)

This is the base class for all TnFOX byte i/o classes. It provides a universal API
for accessing all subclasses and while API compatible with Qt's QIODevice, it
provides enhanced functionality.

CR/LF translation facilities are provided by applyCRLF() and removeCRLF(). These
were broken first time I wrote them and I find it surprising how difficult it
actually is to implement this 100% correctly. They are public as GUI code will
want to apply CR/LF before exporting text to the clipboard and remove it when
importing.

Data destruction is performed by shredData(). This uses an algorithm which
thoroughly shreds the specified region, ensuring that the data cannot be
recovered by any known process. The algorithm used involves XORing the existing
data with pseudo-random data seeded with data from FX::Secure::Randomness,
then writing zeros over the lot. All TnFOX file i/o devices call this on
truncated data when IO_ShredTruncate is specified in the mode. If you wish
to destroy an existing file, open it with IO_ShredTruncate, call truncate(0)
and close before deleting.

QIODevice is extremely straightforward, so I won't bother explaining any more.
What I will say is that there are two types of i/o device in TnFOX: (a) file
and (b) synchronous. File i/o devices (eg; FX::FXFile, FX::QBuffer, FX::QMemMap,
FX::QGZipDevice etc) let you perform reads and writes on a patch of data and
thus they are interchangeable. Synchronous i/o devices (eg; FX::QPipe,
FX::QBlkSocket, FX::QLocalPipe etc) which inherit off FX::QIODeviceS imply
the use of two threads to use them
because data reads in all of them wait for data to be written by the other side.
Things like at() have no meaning and size() returns how much data is waiting
to be read. Synchronous i/o devices are the foundation stone of Inter Process
Communication (IPC) and are used mostly with FX::FXIPCMsg.

All i/o devices in TnFOX are thread-safe and can be used by multiple threads
simultaneously though obviously there is only one current file pointer. You
will have to synchronise that on your own. Furthermore, full access to the
host security mechanism is provided by all i/o devices via permissions().

<h3>Differences from QIODevice</h3>
\li All file position pointers are always at least 64 bit (FXfval).
\li All memory quantities are FXuval so 64 bit memory works fine.
\li Turning raw mode on simply causes a buffer flush at the end of each write
operation. You probably don't want this.
\li Reads and writes can be interchanged without buffer flushes. Why the hell
we still need to do this on modern computers is beyond me, and it's \b especially
beyond me why Qt still needs it.
\li All devices must provide truncate(). Again, why the hell doesn't QIODevice
have this? All devices must also provide readBlockFrom() and writeBlockTo().
\li All i/o errors are reported as exceptions deriving off FX::FXIOException with
the exception of not found errors which are FX::FXNotFoundException
\li getch(), putch() aren't pure virtual

\sa FX::QBuffer, FX::FXFile, FX::QBlkSocket, FX::QPipe, FX::QMemMap
*/
class FXAPI QIODevice
{
public:
	typedef FXfval Offset;
private:
	FXuint mymode;
protected:
	QIODevice(const QIODevice &o) : mymode(o.mymode), ioIndex(o.ioIndex) { }
	QIODevice &operator=(const QIODevice &o) { mymode=o.mymode; ioIndex=o.ioIndex; return *this; }
	FXfval ioIndex;
public:
	QIODevice() : mymode(0), ioIndex(0) { }
	virtual ~QIODevice() { }

	//! Returns the flags of this device
	FXuint flags() const { return mymode; }
	/*! Returns the mode of this device \sa QIODeviceOpenFlags */
	FXuint mode() const { return mymode & IO_ModeMask; }
	/*! Returns the state of this device \sa QIODeviceStateFlags */
	FXuint state() const { return mymode & IO_StateMask; }
	//! Returns true if the device is buffered
	bool isBuffered() const { return IO_Raw!=(mymode & IO_Raw); }
	//! Returns true if the device is unbuffered
	bool isRaw() const { return IO_Raw==(mymode & IO_Raw); }
	//! Returns true if the device is LR/CF translated
	bool isTranslated() const { return IO_Translate==(mymode & IO_Translate); }
	//! Returns true if the device is readable
	bool isReadable() const { return IO_ReadOnly==(mymode & IO_ReadOnly); }
	//! Returns true if the device is writeable
	bool isWriteable() const { return IO_WriteOnly==(mymode & IO_WriteOnly); }
	//! \overload
	bool isWritable() const { return isWriteable(); }
	//! Returns true if the device is readable & writeable
	bool isReadWrite() const { return IO_ReadWrite==(mymode & IO_ReadWrite); }
	//! Returns true if the device is closed
	bool isClosed() const { return IO_Open!=(mymode & IO_Open); }
	//! \overload
	bool isInactive() const { return isClosed(); }
	//! Returns true if the device is opened
	bool isOpen() const { return IO_Open==(mymode & IO_Open); }
	//! Returns true if this device is a synchronous device
	virtual bool isSynchronous() const { return false; }

	//! Opens the device for the specified access
	virtual bool open(FXuint mode)=0;
	//! Closes the device
	virtual void close()=0;
	//! Flushes the device's write buffer
	virtual void flush()=0;
	//! Returns the size of the data being accessed by the device
	virtual FXfval size() const=0;
	/*! Truncates the data to the specified size. Extends the file if necessary &
	doesn't affect the current file pointer unless it is beyond the new file size,
	in which case it is moved to the end of the new file
	*/
	virtual void truncate(FXfval size)=0;
	/*! Returns the current file pointer within the device
	\note With IO_Translate enabled, this increments faster than the return values from either
	readBlock() or writeBlock()
	*/
	virtual FXfval at() const;
	/*! Sets the current file pointer
	\note With IO_Translate enabled, setting to anything other than a previously read at() can
	be hazardous.
	*/
	virtual bool at(FXfval newpos);
	//! Returns true if there is no more data available to be read from the device
	virtual bool atEnd() const;
	//! Returns the ACL for this device
	virtual const FXACL &permissions() const;
	//! Sets the ACL for this device
	virtual void setPermissions(const FXACL &);

	/*! Reads up to the specified quantity of bytes into the buffer, returning how much was actually read
	\note With IO_Translate enabled, this routine regularly returns less read than maxlen (as the CR's are
	stripped out).
	*/
	virtual FXuval readBlock(char *data, FXuval maxlen)=0;
	//! \overload
	FXuval readBlock(FXuchar *data, FXuval maxlen) { return readBlock((char *) data, maxlen); }
	/*! Writes up to the specified quantity of bytes from the buffer, returning
	how much was actually written. Note that less being written due to error is
	returned as an exception, but some devices may write less in a non-error situation.
	*/
	virtual FXuval writeBlock(const char *data, FXuval maxlen)=0;
	//! \overload
	FXuval writeBlock(const FXuchar *data, FXuval maxlen) { return writeBlock((char *) data, maxlen); }
	//! Reads data until an end-of-line or \em maxlen is exceeded
	virtual FXuval readLine(char *data, FXuval maxlen);
	/*! Combines an at() and readBlock() together. Can be much more efficient than those two operations
	individually with some i/o devices, plus it's synchronous and thus threadsafe */
	virtual FXuval readBlockFrom(char *data, FXuval maxlen, FXfval pos)=0;
	//! \overload
	FXuval readBlockFrom(FXuchar *data, FXuval maxlen, FXfval pos) { return readBlockFrom((char *) data, maxlen, pos); }
	/*! Combines an at() and writeBlock() together. Can be much more efficient than those two operations
	individually with some i/o devices, plus it's synchronous and thus threadsafe */
	virtual FXuval writeBlockTo(FXfval pos, const char *data, FXuval maxlen)=0;
	//! \overload
	FXuval writeBlockTo(FXfval pos, const FXuchar *data, FXuval maxlen) { return writeBlockTo(pos, (char *) data, maxlen); }

	//! Reads a single byte. Returns -1 for no data found.
	virtual int getch();
	//! Writes a single byte.
	virtual int putch(int c);
	//! Pushes back a byte to the read buffer
	virtual int ungetch(int c)=0;
public:
	//! The type of CR/LF encoding you want
	enum CRLFType
	{
		Default=0,			//!< Uses the host OS format
		Unix=1,				//!< Uses ASCII 10 to delimit lines
		MacOS=2,			//!< Uses ASCII 13 to delimit lines
		MSDOS=3				//!< Uses ASCII 13,10 to delimit lines
	};
	/*! Applies CR/LF translation returning characters in output. Ensure
	outputlen is bigger if translating to MSDOS. If outputlen runs out
	just as an ASCII 10 is half way through being converted to a CR,LF
	then midNL becomes true and you should restart the translation from
	the last byte in this batch prepended onto the next batch. Whatever
	the case inputlen is written back with the total input read.
	*/
	static FXuval applyCRLF(bool &midNL, FXuchar *output, const FXuchar *input, FXuval outputlen, FXuval &inputlen, CRLFType type=Default);
	/*! Removes CR/LF translation intelligently (ie; self-adjusts to MS-DOS, Unix
	and MacOS formats) returning output length. Safe for placement use ie; output=input.
	Returns bytes output and if midNL goes true then the last byte in the input
	could be half a newline - thus you should restart the translation from
	the last byte in this batch prepended onto the next batch.
	*/
	static FXuval removeCRLF(bool &midNL, FXuchar *output, const FXuchar *input, FXuval len);
	/*! Destroys the \em len bytes of data from offset \em offset into the file.
	Restores the file pointer afterwards and returns how much data was
	shredded before end of file if encountered. You must have the device
	open for both reading and writing for this call to succeed. */
	FXfval shredData(FXfval offset, FXfval len=(FXfval)-1);
protected:
	//! Sets the flags
	void setFlags(int f) { mymode=f; }
	//! Sets the mode
	void setMode(int m) { mymode=(mymode & ~IO_ModeMask)|m; }
	//! Sets the state
	void setState(int s) { mymode=(mymode & ~IO_StateMask)|s; }
	friend FXAPI FXStream &operator<<(FXStream &s, QIODevice &i);
	friend FXAPI FXStream &operator>>(FXStream &s, QIODevice &i);
};

/*! Appends the contents of an i/o device to stream \em s
\warning This operation is not thread-safe
*/
FXAPI FXStream &operator<<(FXStream &s, QIODevice &i);
/*! Reads all available contents of the stream \em s to an i/o device, replacing
its current contents and resetting the file pointer to the start
\warning This operation is not thread-safe
*/
FXAPI FXStream &operator>>(FXStream &s, QIODevice &i);


} // namespace

#endif
