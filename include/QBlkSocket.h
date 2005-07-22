/********************************************************************************
*                                                                               *
*                          Network socket i/o device                            *
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


#ifndef QBLKSOCKET_H
#define QBLKSOCKET_H
#include "QIODeviceS.h"
#include "QHostAddress.h"

namespace FX {

/*! \file QBlkSocket.h
\brief Defines classes used to provide a portable network socket
*/

class FXString;

/*! \class QBlkSocket
\ingroup siodevices
\brief A synchronous TCP or UDP network socket (Qt compatible)

This is a \em synchronous network socket class capable of providing a TCP
connection or UDP packet-based transfers. Synchronous means that unlike most
socket implementations there is no posting of messages to notify you of
events - instead a much simpler and elegant structure is required of having
a unique thread wait on input and it can handle and/or dispatch incoming data.

It is also almost API compatible with Qt's QSocketDevice which is Qt's
core socket class. Unlike QSocketDevice, QBlkSocket can only behave in
a blocking fashion.

Furthermore, QBlkSocket provides a full IPv6 interface as well as IPv4
facilities. All you need to do is set an IPv6 address in FX:QHostAddress
and the rest is taken care of for you.

The send and receive buffer sizes are important. If the send buffer size
becomes too full to allow insertion of a writeBlock(), writeBlock() will
stall until enough data has been sent. This may take seconds and occasionally
minutes. The receive buffer size is also important because it limits the
maximum data length you can pass to readBlock() which will never return
more than readBufferSize() bytes. On Win32, the default read and write
buffer sizes appear to be 8192 bytes which is a little small. On Linux on
FreeBSD, they are a much more reasonable 49152 bytes.

To create the server end of a socket, instantiate a QBlkSocket just as
type and optionally port (if port is zero, it chooses any free port which isn't
really useful for server sockets) and call
create(). This will bind the socket to any local network adaptor which is chosen by
whichever adaptor the incoming connection is using and can be retrieved
after connection by address() and port(). If your server socket is for
localhost communication only you should use the other constructor specifying
the loopback address \c 127.0.0.1 or \c ::1 (QHOSTADDRESS_LOCALHOST) - this on many systems internally
optimises the connection, making it much faster. Lastly if you want a client
socket connecting to some server socket, specify the address and port you want
to connect to and call open().

Stream type can be either QBlkSocket::Stream or QBlkSocket::Datagram. The
former uses TCP over IP and is a connection-based reliable full-duplex
general-purpose data transport protocol. The latter uses UDP over IP and is a
connection-less unreliable packet protocol - unreliable means your packets 
may not receive their destination nor in the same order as sending, however
its ability to send status updates much more efficiently than TCP guarantees
its usefulness.

For stream-based server sockets you should create() the socket in your
monitoring thread, then call waitForConnection() which upon a client
connect, returns a new instance of the server socket with a connection
to the client. You should at this stage launch another thread to handle
monitoring that connection and return to waitForConnection() again.

A full discussion of the issues surrounding TCP, UDP, IPv4 and IPv6 is
beyond the scope of this class documentation, but there are plenty of
man pages, MSVC help files, books and RFC's available. Note that by default
socket linger is enabled and set to five seconds.

Default security on sockets is full public access and you cannot change
this. If you want to control security, consider using FX::QPipe which
is more efficient anyway.

Lastly FX::FXConnectionLostException with error code FXEXCEPTION_CONNECTIONLOST
is thrown should a connection unexpectedly terminate. You will probably
want to trap this for client sockets - for server sockets, you just destroy
the instance.

\warning Do not do multiple reads from multiple threads at the same time.
Nor multiple writes from multiple threads (you can read in one thread and
write in another concurrently fine). This is due to a limitation in the Windows
implementation.

<h4>Differences from Qt:</h4>
First off, as usual, errors are returned as exceptions rather than by error().
Secondly, a lot of the complexity in QSocketDevice can be avoided as this
socket always blocks - thus some API's return default values. I've also
removed the methods allowing direct manipulation of the socket handle
as code should need to know nothing about it (and this is the biggest
break from QSocketDevice's API). Lastly, I've extended the API where
it makes sense to make it easier to use.

<h3>Usage:</h3>
To create a TCP server socket on port 12345:
\code
QBlkSocket server(QBlkSocket::Stream, (FXushort) 12345);
server.create(IO_ReadWrite);
\endcode
To wait for someone to connect to your TCP server socket:
\code
FXPtrHold<QBlkSocket> transport=server.waitForConnection();
FXProcess::threadPool().dispatch(Generic::BindFuncN(clientHandler, transport));
transport=0;
\endcode
\sa FX:FXNetwork
*/

struct QBlkSocketPrivate;
class FXAPIR QBlkSocket : public QIODeviceS
{
	QBlkSocketPrivate *p;
	QBlkSocket &operator=(const QBlkSocket &);
	FXDLLLOCAL void fillInAddrs(bool incPeer);
	FXDLLLOCAL void zeroAddrs();
	FXDLLLOCAL void setupSocket();
	QBlkSocket(const QBlkSocket &o, int h);
	virtual FXDLLLOCAL void *int_getOSHandle() const;
public:
	//! The types of connection you can have
	enum Type
	{
		Stream=0,	//!< A connection based reliable socket (TCP)
		Datagram	//!< A packet based unreliable socket (UDP)
	};
	//! Constructs a socket on the local machine on port \em port (you still need to call create())
	QBlkSocket(Type type=Stream, FXushort port=0);
	//! Constructs a socket to connect to \em addr (you still need to call open()) or a server on localhost
	QBlkSocket(const QHostAddress &addr, FXushort port, Type type=Stream);
	//! Constructs a socket to connect to whatever addr resolves to by DNS (you still need to call open()) or a server on localhost
	QBlkSocket(const FXString &addr, FXushort port, Type type=Stream);
	//! Destructive copy constructor. Best to not use explicitly.
#ifndef HAVE_MOVECONSTRUCTORS
#ifdef HAVE_CONSTTEMPORARIES
	QBlkSocket(const QBlkSocket &o);
#else
	QBlkSocket(QBlkSocket &o);
#endif
#else
private:
	QBlkSocket(const QBlkSocket &);	// disable copy constructor
public:
	QBlkSocket(QBlkSocket &&o);
#endif
	~QBlkSocket();

	//! Returns the type of the socket
	Type type() const;
	//! Sets the type of the socket. Closes the socket if already open
	void setType(Type type);
	//! The address of the socket. This will be null until a connection is made.
	const QHostAddress &address() const;
	//! The port of the socket. This will be null until a connection is made.
	FXushort port() const;
	//! The address of what's connected to the socket. This will be null until a connection is made.
	const QHostAddress &peerAddress() const;
	//! The port of what's connected to the socket. This will be null until a connection is made.
	FXushort peerPort() const;
	//! Returns true if this socket uses a unique port
	bool isUnique() const;
	//! Set if you want a socket created using a unique port
	void setUnique(bool a);
	//! Returns the receive buffer size. Use only after opening.
	FXuval receiveBufferSize() const;
	//! Sets the receive buffer size. Use only after opening.
	void setReceiveBufferSize(FXuval newsize);
	//! Returns the send buffer size. Use only after opening.
	FXuval sendBufferSize() const;
	//! Sets the send buffer size. Use only after opening.
	void setSendBufferSize(FXuval newsize);
	//! Returns the maximum number of pending connections permitted. Defaults to 50.
	FXint maxPending() const;
	//! Sets the maximum number of pending connections
	void setMaxPending(FXint newp);
	//! \overload
	bool listen(int newp) { setMaxPending(newp); return true; }
	//! Returns true if this socket's address is reusable (never if isUnique() is true). Use only after opening.
	bool addressReusable() const;
	//! Sets if this socket's address is reusable (never if isUnique() is true). Use only after opening.
	void setAddressReusable(bool newar);
	//! Returns true if this socket constantly validates its connection. Use only after opening.
	bool keepAlive() const;
	//! Sets if this socket constantly validates its connection. Use only after opening.
	void setKeepAlive(bool newar);
	//! Returns the period in seconds the socket will wait to send remaining data when closing
	FXint lingerPeriod() const;
	//! Sets the period in seconds the socket will wait to send remaining data when closing
	void setLingerPeriod(FXint period);
	//! Returns true if this socket is using Nagle's algorithm. Use only after opening.
	bool usingNagles() const;
	//! Sets if this socket is using Nagle's algorithm. Use only after opening.
	void setUsingNagles(bool newar);
	//! Returns true if the socket is connected to something
	bool connected() const;

	/*!	Creates the socket on the local machine so that others can connect to it. If
	you specify IO_QuietSocket then the socket is not opened for connections - use
	listen() to open it for connections later */
	bool create(FXuint mode=IO_ReadWrite);
	//! Opens a connection to the address previously set. This make take some time.
	bool open(FXuint mode=IO_ReadWrite);
	//! Closes the connection and/or socket
	void close();
	//! A noop due to lack of support on all host OS's
	void flush();
	//! Resets the socket for use after an error
	bool reset();

	//! Returns the amount of data waiting to be read. May return \c ((FXfval)-1) if there is data but it's unknown how much
	FXfval size() const;
	/*! \overload
	\deprecated For Qt compatibility only
	*/
	FXDEPRECATEDEXT FXfval bytesAvailable() const { return size(); }
	//! Returns an all public access ACL
	virtual const FXACL &permissions() const;

	/*! \return The number of bytes read (which may be less than requested)
	\param data Pointer to buffer to receive data
	\param maxlen Maximum number of bytes to read

	Reads a block of data from the socket into the given buffer. Will wait forever
	until requested amount of data has been read if necessary. Is compatible
	with thread cancellation in FX::QThread on all platforms. 
	*/
	FXuval readBlock(char *data, FXuval maxlen);
	/*! \return The number of bytes written.
	\param data Pointer to buffer of data to send
	\param len Number of bytes to send

	Writes a block of data from the given buffer to the socket. Depending on the
	size of the data being written and what's currently in the socket's output queue,
	the call may block until sufficient data has been sent before returning. On
	any TCP/IP connection, this may be in the order of seconds.
	*/
	FXuval writeBlock(const char *data, FXuval maxlen);
	/*! \overload
	Useful for UDP packet sends
	*/
	FXuval writeBlock(const char *data, FXuval maxlen, const QHostAddress &addr, FXushort port);
	//! Tries to unread a character. Unsupported for sockets.
	int ungetch(int);
	/*! \return A new'ed instance of QBlkSocket for the new connection or 0 if timed out

	Waits for a client to connect to a server socket, spawning a new instance
	of itself for the new connection. The original socket (ie; the one you
	call waitForConnection() upon) is unaffected.
	*/
	QBlkSocket *waitForConnection(FXuint waitfor=FXINFINITE);
public:
	//! \deprecated For Qt compatibility only
	FXDEPRECATEDEXT bool blocking() const { return true; }
	//! \deprecated For Qt compatibility only
	FXDEPRECATEDEXT void setBlocking(bool) { }
	//! \deprecated For Qt compatibility only
	FXDEPRECATEDEXT FXuval waitForMore(int msecs, bool *timeout=0);
};

} // namespace

#endif
