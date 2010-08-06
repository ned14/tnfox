/********************************************************************************
*                                                                               *
*                        Inter Process Communication                            *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003-2009 by Niall Douglas.   All Rights Reserved.       *
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

#ifndef FXIPC_H
#define FXIPC_H

#include "FXException.h"
#include "QThread.h"
#include "FXStream.h"

namespace FX {

/*! \file FXIPC.h
\brief Defines classes used in providing inter-process communication
*/

class QIODeviceS;

/*! \defgroup IPC Inter Process Communication & Data Transport

TnFOX provides a lightweight but powerful IPC framework based around message
passing. It has been designed primarily with efficiency in mind - not only
in terms of execution, but especially in terms of maintainence and extensibility.
The IPC framework can use any FX::QIODeviceS eg; FX::QPipe, FX::QBlkSocket,
FX::QLocalPipe or FX::QSSLDevice and via threads can provide full portable
asynchronous i/o (ie; you can send a message, go do something else and get
notified when its acknowledgement returns).

Messages can be of any size though generally they are unsuited for very large
blocks of data (consider using a FX::QMemMap if machine-local). If you are
streaming over an external connection (socket) then you'd usually want to
chop them up into lots of smaller messages. For maximum flexibility there are
broadcast type messages (one way) and synchronous messages (send and wait
for acknowledgement message). Both outbound and acknowledgement messages can
contain arbitrary C++ objects of any kind so long as they are serialisable
using FX::FXStream. Endian translation is automatically performed where
necessary. Out-of-order sequences are supported as are multiple independent
transactions via one IPC channel (eg; by multiple threads). The data can be
CRC checked for unreliable connections and GZip compressed for very slow ones.

As you might be guessing by now, the IPC framework was designed as a generic
solution for almost every conceivable IPC need. You could just as easily
implement a maximum efficiency windowed file transfer protocol as a simple
ping-type broadcast. The upcoming Tn library will use the exact same
framework for kernel/client communication, data stream implementation,
distributed processing and inter-kernel communication. All these applications
require quite different configurations of messaging protocol - this IPC
framework makes it all quite easy whilst maintaining maximum speed.

While only a subset of CORBA's functionality is provided directly here, these
classes along with the rest of TnFOX make it easy to build a CORBA-like
system whilst remaining much more efficient and easier to use. Indeed, Tn does
precisely this and benchmarks have shown a roughly 20-50% improvement over CORBA.

If you want to interoperate with a web server, you might find the FX::FXFCGI
class useful - this implements a FastCGI interface suitable for a web service
providing daemon. If you would like to work with a child process, FX::QChildProcess
provides a standard Unix-style simple i/o interface.

<h3>Implementation:</h3>
TnFOX uses a rather unconventional mechanism of defining IPC messages -
this has resulted from lessons learned during my previous attempt in Tornado
which while it worked, it began to become a maintainence headache as it grew
(think large \c switch() statements).
This solution is far more flexible and was inspired by
<a href="http://www.nedprod.com/Niall_stuff/MarshallClineConversation.html">
a conversation with Marshall Cline</a> (my thanks to him).

An IPC message registry defines a \em namespace of unique messages for a
particular IPC channel. Both ends of the IPC channel must use a broadly
compatible mapping of message C++ types to unique 32 bit integers. However
to allow for one end to be much newer than the other, the namespace is
not contiguous but is broken up into \em chunks of fixed start which are
denoted by FX::FXIPCMsgChunk. All messages are versioned, so you can even
maintain the unique message code whilst appending to the format.

When you construct a FXIPCMsgChunk, you enter the mapping of
codes to types and deendianisation routine into the registry where it
can be used to correctly identify, unpack and debug received messages.
The unique code of each message within the chunk is determined at compile-time
and it is very easy to add new messages with minimal recompiling. Destroying
a FXIPCMsgChunk removes it from the registry - you can use this to implement
security by literally removing the ability for certain functionality to be
called. The main intention though was that DLLs could be loaded in which extend
the messaging namespace and unloaded again.

All FX::FXIPCMsgRegistry's include by default the FX::FXIPCMsgChunkStandard
chunk. This includes the following messages:
\li FX::FXIPCMsg_Disconnect
\li FX::FXIPCMsg_Unhandled
\li FX::FXIPCMsg_ErrorOccurred

These messages are partially acted upon internally by FX::FXIPCChannel, usually
sufficiently that you don't need to do anything yourself. See FX::FXIPCChannel
for implementation notes for that class.

<h3>Usage:</h3>
To define a chunk you must follow the following pattern:
\code
typedef FXIPCMsgChunkCodeAlloc<base of chunk, true> MyChunkBegin;
struct FXIPCMsg_MyMsg1 : public FXIPCMsg
{
   typedef FXIPCMsgChunkCodeAlloc<MyChunkBegin::code, hasAck> id;
   typedef FXIPCMsgRegister<id, FXIPCMsg_MyMsg1> regtype;
   int foo;
   FXIPCMsg_MyMsg1(int _foo) : FXIPCMsg(id::code), foo(_foo) { }
   void   endianise(FXStream &ds) const { ds << foo; }
   void deendianise(FXStream &ds)       { ds >> foo; }
};
struct FXIPCMsg_MyMsg2 : public FXIPCMsg
{
   typedef FXIPCMsgChunkCodeAlloc<FXIPCMsg_MyMsg1::id::nextcode, hasAck> id;
   typedef FXIPCMsgRegister<id, FXIPCMsg_MyMsg2> regtype;
...
typedef FXIPCMsgChunk<Generic::TL::create<
      FXIPCMsg_MyMsg1::regtype,
      FXIPCMsg_MyMsg2::regtype,
      ...
   >::value> MyMsgChunk;
\endcode
While it's a bit of a mouthful, copy & paste makes it easy enough. Trust me,
it's better than an enum of all type codes as the list breaks one hundred long!

<h2>More advanced usage:</h2>
The IPC framework can also tunnel messages across an IPC channel bridge ie;
you send a message, the remote end receives it and resends it down another
IPC channel all whilst ensuring that message acks route themselves backwards.
If you want to do this, please ask on the mailing list for advice as it seemed
too complex to document in detail for most users. On the other hand, it is
quite straightforward if you think about it for a while. Tn's kernel implements
precisely this mechanism to link together components running in disparate
processes.

Using the IPC framework asynchronously is demonstrated by FX::TnFXSQLDB_ipc
and the example code in the Tn directory inside the test suite.

<h3>Example code:</h3>
If you want some examples of usage, see the source for FX::TnFXSQLDBServer and
FX::TnFXSQLDB_ipc. Also see \c TestIPC and \c TestSQLDB in the test suite.
*/

/*! \struct FXIPCMsg
\ingroup IPC
\brief A base IPC message

Some fields are marked optional - this means they are set externally to FXIPCMsg
eg; by FX::FXIPCChannel which sets them all. You must define
\code
void   endianise(FXStream &s) const;
void deendianise(FXStream &s);
\endcode
... in your subclasses of FXIPCMsg, the first serialising your message's contents
to stream \em s and the latter unserialising them. They aren't virtual because
(a) to avoid virtual table overheads (b) endianise() is per-derived-type and so
using a templated type extractor we can pass the endianise() method then and (c)
per-derived-type deendianise() is registered with FX::FXIPCMsgRegistry. Therefore
there is no useful reason to make them virtual.

All FXIPCMsg subclasses must provide a default constructor or else the chunk
register code won't compile.

You can use this structure yourself - or more likely let FX::FXIPCChannel do it
all for you. There is an optional routing field which isn't used unless you set
it to something.
*/
struct FXIPCMsg
{	// This gets memcpy()'ed so don't add objects
	friend class FXIPCChannel;
	friend class FXIPCChannelIndirector;
	enum Flags				// NOTE TO SELF: May not exceed a byte
	{
		FlagsWantAck=1,		//!< If unset and this message has an ack, don't bother serialising the ack
		FlagsGZipped=2,		//!< If set, data has been run through a FX::QGZipDevice
		FlagsHasRouting=4,	//!< If set, msg contains routing number
		FlagsIsBigEndian=8	//!< If set, the sender was big endian
	};
private:
	// The following are sent in this order (total: 18 bytes)
	FXuint len;				//!< +0  Length of the message (total, inclusive)
	FXuint crc;				//!< +4  Optional fxadler32() of message data after this point till end
	FXuint type;			//!< +8  Unique type code
	FXuint myid;			//!< +12 Optional unique non-zero id
	FXuchar mymsgrev;		//!< +16 Revision number of the message
	FXuchar myflags;		//!< +17 Bitwise combination of Flags (MUST be a byte)
	FXuint myrouting;		//!< +18 Optional routing info (see FlagsHasRouting)
private:	// The following members are not sent
	FXuchar *myoriginaldata;
protected:
	FXIPCMsg(FXuint _type, FXuchar _msgrev=0) : len(0), crc(0), type(_type), myid(0), mymsgrev(_msgrev), myflags(FOX_BIGENDIAN*FlagsIsBigEndian), myrouting(0), myoriginaldata(0) { }
	FXIPCMsg(FXuint _type, FXuint _id, FXuchar _msgrev=0) : len(0), crc(0), type(_type), myid(_id), mymsgrev(_msgrev), myflags(FOX_BIGENDIAN*FlagsIsBigEndian), myrouting(0), myoriginaldata(0) { }
	FXIPCMsg(const FXIPCMsg &o, FXuint _type=0) : len(o.len), crc(o.crc), type(_type!=0 ? _type : o.type), myid(o.myid), mymsgrev(o.mymsgrev), myflags(o.myflags), myrouting(o.myrouting), myoriginaldata(o.myoriginaldata) { }
#ifdef FOXPYTHONDLL
public:
	virtual ~FXIPCMsg() { }
#else
	~FXIPCMsg() { } // stops sliced destructs
#endif
public:
	//! True if this message is identical to the other message
	bool operator==(const FXIPCMsg &o) const throw() { return len==o.len && crc==o.crc && type==o.type && myid==o.myid && mymsgrev==o.mymsgrev && myflags==o.myflags && myrouting==o.myrouting; }
	//! True if this message is not identical to the other message
	bool operator!=(const FXIPCMsg &o) const throw() { return !(*this==o); }
	//! The minimum possible size of a header
	static const FXuint minHeaderLength=18;
	//! The maximum possible size of a header
	static const FXuint maxHeaderLength=22;
	//! Length of the header only when serialised
	int headerLength() const throw() { return minHeaderLength+((myflags & FlagsHasRouting) ? sizeof(FXuint) : 0); }
	//! Returns the length of the message in bytes. Unset until sent or received.
	FXuint length() const throw() { return len; }
	//! Returns the type of the message
	FXuint msgType() const throw() { return type; }
	//! Returns true if the message has an ack
	bool hasAck() const throw() { return (type & 1)==0; }
	//! Returns the internal unique id count of the message.
	FXuint msgId() const throw() { return myid; }
	//! Sets the internal unique id count of the message
	void setMsgId(FXuint id) throw() { myid=id; }
	//! Returns true if the sender of this message is waiting for the ack
	bool wantsAck() const throw() { return (myflags & FlagsWantAck)!=0; }
	//! Returns true if the data sent in this message is GZipped
	bool gzipped() const throw() { return (myflags & FlagsGZipped)!=0; }
	//! Sets if the data sent in this message is GZipped. Use only on very slow connections.
	void setGZipped(bool v) throw() { if(v) myflags|=FlagsGZipped; else myflags&=~FlagsGZipped; }
	//! Returns true if the message has routing
	bool hasRouting() const throw() { return (myflags & FlagsHasRouting)!=0; }
	//! Returns the routing number of this message
	FXuint routing() const throw() { return myrouting; }
	//! Sets the routing number of this message
	void setRouting(FXuint no) throw() { myrouting=no; myflags|=FlagsHasRouting; }
	//! Returns true if the message was serialised by a big endian architecture
	bool inBigEndian() const throw() { return (myflags & FlagsIsBigEndian)!=0; }
	/*! Returns a pointer to the serialised data from which this message was
	constructed (if it wasn't generated this way, then zero). FX::FXIPCChannel
	only sets this when a received message is known, not an ack and only for
	the duration of normal message handling - it is reset to zero if you detach
	a message to prevent you using the wrong data (as the buffer gets overwritten). */
	FXuchar *originalData() const throw() { return myoriginaldata; }
	//! Dumps the message header to stream \em s. Does not send subclass data - you need to use this as part of a larger call
	void write(FXStream &s) const { s << len << crc << type << myid << mymsgrev << myflags; if(myflags & FlagsHasRouting) s << myrouting; }
	//! Unpacks the message header from stream \em s.
	void read(FXStream &s) { s >> len >> crc >> type >> myid >> mymsgrev >> myflags; if(myflags & FlagsHasRouting) s >> myrouting; }
private:
	// Default constructor so GCC is happy
	FXDLLLOCAL FXIPCMsg();
};
//! Strawman type for containers of FX::FXIPCMsg
struct FXIPCMsgHolder : public FXIPCMsg
{
};
#ifdef __GNUC__
// GCC gets confused by the protected destructor when in Generic::convertible
namespace Generic {
	template<> struct convertible<int, FXIPCMsg> { enum { value=false }; };
}
#endif

/*! \class FXIPCMsgRegistry
\ingroup IPC
\brief A registry of known IPC messages for a particular IPC channel
*/
struct FXIPCMsgRegistryPrivate;
class FXAPI FXIPCMsgRegistry
{
public:
	template<class codealloc, class msgtype> friend struct FXIPCMsgRegister;
	typedef void (*deendianiseSpec)(FXIPCMsg *, FXStream &);
	typedef FXIPCMsg *(*makeMsgSpec)();
	typedef void (*delMsgSpec)(FXIPCMsg *);
private:
	FXuint magic;
	FXIPCMsgRegistryPrivate *p;
	void int_register(FXuint code, deendianiseSpec deendianise, makeMsgSpec makeMsg, delMsgSpec delMsg, Generic::typeInfoBase &ti);
	void int_deregister(FXuint code);
	FXIPCMsgRegistry(const FXIPCMsgRegistry &);
	FXIPCMsgRegistry &operator=(const FXIPCMsgRegistry &);
public:
	FXIPCMsgRegistry();
	~FXIPCMsgRegistry();
	//! True if registry is valid
	bool isValid() const { return magic==*(FXuint *)"PCMR"; }
	//! Returns true if code is known
	bool lookup(FXuint code) const;
	//! Returns the deendianise and make functions of the code
	bool lookup(deendianiseSpec &deendianise, makeMsgSpec &makeMsg, delMsgSpec &delMsg, FXuint code) const;
	//! Returns a textualised version of the code (mostly used for debugging)
	const FXString &decodeType(FXuint code) const;
};

template<unsigned int basecode, bool _hasAck> struct FXIPCMsgChunkCodeAlloc
{
	enum { hasAck=_hasAck, code=basecode+(_hasAck ? 0 : 1), nextcode=basecode+2 };
};
/*! \class FXIPCMsgChunk
\ingroup IPC
\brief A chunk within an IPC message registry
*/
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4355) // this used in base member init
#endif
template<typename msgtypelist> class FXIPCMsgChunk
{
	FXIPCMsgChunk(const FXIPCMsgChunk &);
	FXIPCMsgChunk &operator=(const FXIPCMsgChunk &);
protected:
	FXIPCMsgRegistry *myregistry;
	typedef msgtypelist MsgTypeList;
	Generic::TL::instantiateH<msgtypelist> registrants;
public:
	FXIPCMsgChunk(FXIPCMsgRegistry *registry) : myregistry(registry), registrants(myregistry) { }
	//! Returns the registry this chunk refers to
	FXIPCMsgRegistry &registry() const throw() { return *myregistry; }
	enum { BaseCode=Generic::TL::at<msgtypelist, 0>::value::MsgType::id::code };
	//! Returns the base code of this chunk
	FXuint baseCode() const throw() { return BaseCode; }
	enum { EndCode=Generic::TL::at<msgtypelist, Generic::TL::length<msgtypelist>::value-1>::value::MsgType::id::code };
	//! Returns the end code of this chunk
	FXuint endCode() const throw() { return EndCode; }
};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

/*! \struct FXIPCMsgRegister
\ingroup IPC
\brief Actual implementation of registering a message with the registry
*/
template<class codealloc, class msgtype> struct FXIPCMsgRegister
{
	FXIPCMsgRegistry &registry;
	Generic::typeInfo<msgtype> typeinfo;
	typedef msgtype MsgType;
	static void   endianise(FXIPCMsg *msg, FXStream &s) { static_cast<msgtype *>(msg)->endianise(s); }
	static void deendianise(FXIPCMsg *msg, FXStream &s) { static_cast<msgtype *>(msg)->deendianise(s); }
	static FXIPCMsg *makeMsg() { return new msgtype; }
	static void delMsg(FXIPCMsg *ptr) { delete static_cast<msgtype *>(ptr); }
	FXIPCMsgRegister(FXIPCMsgRegistry *_registry) : registry(*_registry)
	{
		registry.int_register(codealloc::code, &deendianise, &makeMsg, &delMsg, typeinfo);
	}
	~FXIPCMsgRegister()
	{
		registry.int_deregister(codealloc::code);
	}
private:
	FXIPCMsgRegister(const FXIPCMsgRegister &);
	FXIPCMsgRegister &operator=(const FXIPCMsgRegister &);
};

typedef FXIPCMsgChunkCodeAlloc<0, true> FXIPCMsgChunkStandardBegin;
/*! \struct FXIPCMsg_Disconnect
\ingroup IPC
\brief Received when the other end of the IPC channel disconnects

You should also trap FX::FXException_ConnectionLost for unexpected connection
losses. Indeed you should probably call similar code in both circumstances.
*/
struct FXIPCMsg_Disconnect : public FXIPCMsg
{
	typedef FXIPCMsgChunkCodeAlloc<FXIPCMsgChunkStandardBegin::code, false> id;
	typedef FXIPCMsgRegister<id, FXIPCMsg_Disconnect> regtype;
	FXIPCMsg_Disconnect() : FXIPCMsg(id::code) { }
	void   endianise(FXStream &ds) const { }
	void deendianise(FXStream &ds)       { }
};
/*! \struct FXIPCMsg_Unhandled
\ingroup IPC
\brief Received when a message with an ack was not handled

Messages without acks are silently ignored - however messages with acks not
handled by the channel (eg; because they are not recognised) are replied to
with this message
*/
struct FXIPCMsg_Unhandled : public FXIPCMsg
{
	typedef FXIPCMsgChunkCodeAlloc<FXIPCMsg_Disconnect::id::nextcode, false> id;
	typedef FXIPCMsgRegister<id, FXIPCMsg_Unhandled> regtype;
	FXuchar cause;	// 0 for normal, 1 for msg not known/registered
	FXIPCMsg_Unhandled(FXuint _id=0, FXuchar _cause=0) : FXIPCMsg(id::code, _id), cause(_cause) { }
	void   endianise(FXStream &ds) const { ds << cause; }
	void deendianise(FXStream &ds)       { ds >> cause; }
};
/*! \struct FXIPCMsg_ErrorOccurred
\ingroup IPC
\brief Received when an exception was thrown by the other end of the IPC channel

Normally speaking FX::FXIPCChannel silently throws a FX::FXException this
end with the same contents as the remote exception. This behaviour can be disabled.
*/
struct FXIPCMsg_ErrorOccurred : public FXIPCMsg
{
	typedef FXIPCMsgChunkCodeAlloc<FXIPCMsg_Unhandled::id::nextcode, false> id;
	typedef FXIPCMsgRegister<id, FXIPCMsg_ErrorOccurred> regtype;
	FXString message;
	FXuint code;
	FXuint flags;
	FXIPCMsg_ErrorOccurred() : FXIPCMsg(id::code), code(0), flags(0) { }
	FXIPCMsg_ErrorOccurred(FXuint _id, FXException &e) : FXIPCMsg(id::code, _id)
	{
#ifdef DEBUG
		message=e.report();
#else
		message=e.message();
#endif
		code=e.code();
		flags=e.flags();
	}
	void   endianise(FXStream &ds) const { ds << message << code << flags; }
	void deendianise(FXStream &ds)       { ds >> message >> code >> flags; }
};
typedef FXIPCMsgChunk<Generic::TL::create<
		FXIPCMsg_Disconnect::regtype,
		FXIPCMsg_Unhandled::regtype,
		FXIPCMsg_ErrorOccurred::regtype
	>::value> FXIPCMsgChunkStandard;


/*! \class FXIPCChannel
\ingroup IPC
\brief Base class for an IPC channel over an arbitrary FX::QIODeviceS

While you could use FX::FXIPCMsg manually, chances are you'll use this base
class which provides almost everything you need to implement a TnFOX IPC
channel. While it was written with running a monitor thread to dispatch
incoming messages asynchronously in mind, you can in fact just not start
the thread and call \c doReception(false) manually. Note that if you do
this, you must send messages without waiting and retrieve their acks after
the doReception() - also see ackedMsgs().

The two most interesting functions for average users will be sendMsg() and
getMsgAck(). sendMsg() endianises all the requisite data and sets all header
items of FXIPCMsg for you before writing to the transport device. If
unreliable() is set, a slight amount of overhead is incurred to calculate
the adler32 checksum of the message contents. If compression() is set, a
\b great amount of overhead is incurred by compressing the message data
using an internal FX::QGZipDevice - it makes no sense to use this except
when the transport is very, very slow indeed (eg; a modem dialup). Note
that one end can be compressed and the other not as indeed one end can
be unreliable and the other not.

In order to be most efficient, FXIPCChannel in general extends on
demand - but doesn't ever reduce the allocation. It trades off a small
amount of execution speed for unlocking itself when sending data so
that another thread can receive and dispatch incoming data during sends,
thus improving performance. As of v0.80 of TnFOX, FXIPCChannel self-optimises
endian conversion so that when like endian architectures are conversing,
conversion is totally optimised away even when a message tunneller is of
a different endian to the source and destination. You can still force
one kind of endian or another which is especially useful for dumping
message exchanges into a format you can read later.

Because of exception safety requirements, thread termination is disabled
during most of doReception() - all except the first read from the device
which is the one which normally does most of the waiting. After the initial
header, the reading of the rest of the message cannot be interrupted.
Sending also disables thread termination during the actual device write
but it is enabled during getMsgAck() which again is where most of the waiting
would be done. requestTermination() is not available publicly in order to
encourage you to use requestClose() whenever possible - however it can be
accessed by a method in your subclass.

\note Automatic retries for damaged messages on unreliable connections
are currently not implemented.

FXIPCChannel was designed to operate connections with untrusted partners
eg; on a public server where a malicious connectee may try to cause
misoperation. The most obvious technique is to deliberately malform
messages and here the responsibility for hardening is shifted entirely
onto you - you must ensure your messages' endianise() and deendianise()
can handle garbage being thrown at them (to aid in testing this, use
setGarbageMessageCount() which causes every N-th message's contents to
be garbled). maxMsgSize() is 64Kb by default (throwing an exception if
a message is sent or received in excess of this) - this prevents an
attacker specifying overly long messages and chewing up connections or
bandwidth. Note that currently peerUntrusted() does nothing, but may
do in a future release.

<h3>Usage:</h3>
You must subclass FXIPCChannel, implementing msgReceived() which is called
to handle non-acknowledgement messages (acknowledgement messages are
deendianised directly into the ack passed to getMsgAck()). msgReceived()
is called in the context of the monitor thread and with the FXIPCChannel
unlocked. The form of msgReceived() is obvious:
\code
HandledCode msgReceived(FXIPCMsg *rawmsg)
{
   switch(rawmsg->msgType())
   {
      case MyMsg::id::code:
      {
         MyMsg *i=(MyMsg *) rawmsg;
         // Do processing ...
         if(i->wantsAck())
         {
            MyMsgAck ia(i->msgId());
            // Maybe set members in ia?
            sendMsg(ia);
         }
         return Handled;
      }
...
   return NotHandled;
}
\endcode
\note Stack space for the monitor thread is only 128Kb and as minimising
latency is so important for good performance, anything more than trivial
processing should be outsourced to a thread pool via invokeMsgHandler().
More importantly you cannot do sendMsg() from the handler with a wait
as it not only massively increases latency, it also creates the easy
possibility of deadlock.

In order to outsource message processing, there is a facility to detach
the message you have received so you can perform the process and return
the ack in your own good time. Simply return \c HandledAsync after dispatching
the heavy processing to a FX::QThreadPool, passing the message pointer
which must now be deleted by you. Ensure you don't throw an exception
before returning \c HandledAsync in this case as FXIPCChannel will delete
it on you.

If you throw a FX::FXException or subclass during msgReceived(), it
is automatically translated into a FX::FXIPCMsg_ErrorOccurred and sent
if and only if the message you were handling has an acknowledgement.
If you handle the message asynchronously, you must do this yourself
if you don't use invokeMsgHandler().
The error is then thrown on the opposite end of the channel in the
thread which called getMsgAck(). An exception to this operation is
throwing a FX::FXConnectionLostException which is causes doReception()
to return false ie; closes the channel.

Something you should note if the situation should arise is that if
you are newer code and send a message which the other end of the IPC
channel does not recognise, you will see a FX::FXException thrown by
sendMsg() or getMsgAck() with the code FXIPCCHANNEL_UNHANDLED if that
message has an ack. You should trap this situation with a FXERRH_TRY() etc.

<h3>Message tunnelling:</h3>
From v0.80 of TnFOX some provision is also made for FXIPCChannel subclasses to tunnel messages
from one remote end to another remote end and vice versa, even messages
whose internal format is totally opaque (they must be a FX::FXIPCMsg subclass
however). Each FXIPCMsg has an optional 32 bit routing number which is
up to you to set so that it identifies the true destination of the message.
It is assumed in this explanation that each end of a FXIPCChannel has its own
independent set of routing numbers and thus four routing numbers would be
required to operate a tunnel (this needn't be true - it's up to how you implement
routing).

At the start of your msgReceived() you will need to see if the message's
routing number is known (usually via a FX::QIntDict lookup). The lookup
will yield the destination FXIPCChannel subclass instance & destination
routing plus a mapping table of message id's within the destination FXIPCChannel
back to message id's within the source FXIPCChannel. You then restamp
the header of the message with the correct info for the destination channel
and send it using restampMsgAndSend() and the FX::FXIPCMsg::originalData()
method (this being more efficient than reserialising the message but also
dependent on both channels using the same endianness). If the message has an
ack, before restamping you should allocate a new message id using makeUniqueMsgId()
in the \b destination channel and add a mapping of that id back to the id used
by the message you received from the source. You then return \c HandledAsync.

If you can be absolutely sure that you will never post to yourself and
no more than the above is required, it is safe to post directly from within
the msgReceived() handler so long as the destination channel isn't choked. If
however you will be performing some work (eg; validating the message or
substantially altering it) you will get better performance and avoid deadlocks
by outsourcing the tunnelling to a thread pool using invokeMsgHandler().

Handling the routing of acks back through the tunnel can be tricky. Your
subclass should implement lonelyMsgAckReceived() and from its message id see
if any messages were tunnelled through using that message id. From this you
can derive the original message id and routing within the original source
channel and thus restamp and send it as previously.

Remember that FXIPCChannel never uses FXIPCMsg::routing() so you can use that
for anything you like. And if you want to tunnel messages which have no registration
within the carrying FXIPCChannel subclass, you should implement unknownMsgReceived().
*/
struct FXIPCChannelPrivate;
template<class type> class QPtrVector;
class FXAPI FXIPCChannel : public QMutex,
#if !defined(BOOST_PYTHON_SOURCE) && !defined(FX_RUNNING_PYSTE)
/* We must subvert normal access protection for the BPL bindings as pyste doesn't
understand yet and it's easier to do this than fix pyste */
	protected QThread
{
#else
	public QThread
{
#endif
	FXIPCChannelPrivate *p;
	FXIPCChannel(const FXIPCChannel &);
	FXIPCChannel &operator=(const FXIPCChannel &);
public:
	//! Constructs an instance using namespace \em registry working with device \em dev and naming the monitor thread \em threadname
	FXIPCChannel(FXIPCMsgRegistry &registry, QIODeviceS *dev, bool peerUntrusted=false, QThreadPool *threadPool=0, const char *threadname="IPC channel monitor");
	~FXIPCChannel();
	//! Returns the message registry the channel uses
	FXIPCMsgRegistry &registry() const;
	//! Sets the message registry the channel uses
	void setRegistry(FXIPCMsgRegistry &registry);
	//! Returns the device providing the transport for the channel
	QIODeviceS *device() const;
	//! Sets the device providing the transport for the channel
	void setDevice(QIODeviceS *dev);
	//! Returns the thread pool the channel uses
	QThreadPool *threadPool() const;
	//! Sets the thread pool for the channel to use
	void setThreadPool(QThreadPool *threadPool);
	//! Returns if CRC checking is enabled for this channel
	bool unreliable() const;
	//! Sets if CRC checking is enabled for this channel
	void setUnreliable(bool v);
	//! Returns if compression is enabled for this channel
	bool compression() const;
	//! Sets if compression is enabled for this channel
	void setCompression(bool v);
	//! Returns if channel is active
	bool active() const;
	//! Resets the channel to its initial state
	void reset();
	//! The types of endian conversion which can be performed
	enum EndianConversionKinds
	{
		AlwaysLittleEndian,		//!< The channel operates exclusively in little endian
		AlwaysBigEndian,		//!< The channel operates exclusively in big endian
		AutoEndian				//!< Incoming messages only are endian converted on demand (default)
	};
	//! Returns what kind of endian conversion the channel performs
	EndianConversionKinds endianConversion() const;
	//! Sets what kind of endian conversion the channel performs
	void setEndianConversion(EndianConversionKinds kind);
	//! Returns if FX::FXIPCMsg_ErrorOccurred messages are translated into thrown exceptions
	bool errorTranslation() const;
	//! Sets if FX::FXIPCMsg_ErrorOccurred messages are translated into thrown exceptions
	void setErrorTranslation(bool v);
	//! Returns true if this channel is in untrusted mode
	bool peerUntrusted() const;
	//! Sets if this channel is with an untrusted partner
	void setPeerUntrusted(bool v);
	//! Returns the maximum permitted message size (=0 for no limit)
	FXuint maxMsgSize() const;
	//! Sets the maximum permitted message size (=0 for no limit)
	void setMaxMsgSize(FXuint newsize);
	//! Returns if the channel's garbage message count (used for hardening)
	FXuint garbageMessageCount() const;
	//! Sets this channel's garbage message count (used for hardening)
	void setGarbageMessageCount(FXuint newsize);
	//! Sets if operational statistics are printed to \c stdout
	void setPrintStatistics(bool v);
	//! Causes the channel to close
	void requestClose();

	// From QThread
	using QThread::name;
	using QThread::wait;
	using QThread::start;
	using QThread::finished;
	using QThread::running;
	using QThread::inCleanup;
	using QThread::isValid;
	using QThread::setAutoDelete;
	using QThread::creator;
	using QThread::priority;
	using QThread::setPriority;
	using QThread::processorAffinity;
	using QThread::setProcessorAffinity;
	using QThread::addCleanupCall;
	using QThread::removeCleanupCall;

	//! Specifies how the message was handled
	enum HandledCode
	{
		NotHandled=0,	//!< Not handled
		Handled=1,		//!< Handled
		HandledAsync=2	//!< Handled asynchronously. Message pointer becomes yours
	};
	/*! Called by subclass' run() to perform message receiving, returning true when
	a message is received and dispatched. */
	bool doReception(FXuint waitfor=FXINFINITE);
	/*! Returns a list of messages which have been acked. Not normally used when
	running the monitor thread as it notifies waiters by signalling a wait condition
	*/
	QPtrVector<FXIPCMsgHolder> ackedMsgs() const;
	//! The type of the message filter functor
	typedef Generic::Functor<Generic::TL::create<HandledCode, FXIPCMsg *>::value> MsgFilterSpec;
	/*! Installs a handler which gets called before msgReceived(). You can modify the
	message if you wish or handle it on your own
	*/
	void installPreMsgReceivedFilter(MsgFilterSpec filter);
	//! Removes a previously installed filter
	bool removePreMsgReceivedFilter(MsgFilterSpec filter);

	typedef void (*endianiseSpec)(FXIPCMsg *, FXStream &);
	//! Message send implementation. Use sendMsg() in preference to this where possible.
	bool sendMsgI(FXIPCMsg *FXRESTRICT msgack, FXIPCMsg *FXRESTRICT msg, endianiseSpec endianise, FXuint waitfor);
	/*! \return True if the ack message was received within the specified timeout
	\param msgack Pointer to an already constructed corresponding ack message.
	If zero, we don't care about ack (throw it away)
	\param msg Pointer to the message to be sent
	\param waitfor Specifies how long to wait for the ack message. If zero, returns
	immediately.

	Used to send a message and wait for an ack. If the message does not have an ack,
	returns immediately. You must preserve msg until its ack is received. If the
	transport send throws a FX::FXConnectionLostException sendMsg() asks the monitor
	thread to exit before rethrowing the exception. sendMsg() adjusts itself to
	use less memory when FXProcess::memoryFull() is true.
	\note If the channel suddenly closes before an ack to a message you sent arrives,
	sendMsg() and getMsgAck() throw a FX::FXConnectionLostException.
	*/
	template<class msgacktype, class msgtype> bool sendMsg(msgacktype *FXRESTRICT msgack, msgtype *FXRESTRICT msg, FXuint waitfor=FXINFINITE)
	{
		FXSTATIC_ASSERT(!msgtype::id::hasAck || msgtype::id::code+1==msgacktype::id::code, AckMsg_Not_Ack_Of_Msg);
		return sendMsgI(msgack, msg, &msgtype::regtype::endianise, waitfor);
	}
	//! \overload
	template<class msgacktype, class msgtype> bool sendMsg(msgacktype &msgack, msgtype &msg, FXuint waitfor=FXINFINITE)
	{
		FXSTATIC_ASSERT(!msgtype::id::hasAck || msgtype::id::code+1==msgacktype::id::code, AckMsg_Not_Ack_Of_Msg);
		return sendMsgI(&msgack, &msg, &msgtype::regtype::endianise, waitfor);
	}
	//! \overload
	template<class msgtype> bool sendMsg(msgtype *msg)
	{
		FXSTATIC_ASSERT(!msgtype::id::hasAck, Cannot_Send_Msgs_With_Ack);
		return sendMsgI(0, msg, &msgtype::regtype::endianise, 0);
	}
	//! \overload
	template<class msgtype> bool sendMsg(msgtype &msg)
	{
		FXSTATIC_ASSERT(!msgtype::id::hasAck, Cannot_Send_Msgs_With_Ack);
		return sendMsgI(0, &msg, &msgtype::regtype::endianise, 0);
	}
	/*! This is used to poll for the receipt of the ack to a previously
	sent message and uses the same parameters as sendMsg(). You must pass
	identical parameters to sendMsg() except for \em waitfor. Internally
	sendMsg() actually calls this function.
	*/
	bool getMsgAck(FXIPCMsg *FXRESTRICT msgack, FXIPCMsg *FXRESTRICT msg, FXuint waitfor=FXINFINITE);
	//! \overload
	bool getMsgAck(FXIPCMsg &msgack, FXIPCMsg &msg, FXuint waitfor=FXINFINITE)
	{
		return getMsgAck(&msgack, &msg, waitfor);
	}
	/*! Allocates and returns a unique message id. This id is guaranteed to not be
	in use by any pending message acknowledgements */
	FXuint makeUniqueMsgId();
	/*! Restamps a serialised message's header and sends. Used to implement channel-to-channel
	message tunnelling even when messages are unknown to the tunnel implementation. Both
	\em rawmsg and \em msgheader are updated on exit. If the message has an ack you
	must set a new unique message id beforehand to ensure no conflicts occur. */
	bool restampMsgAndSend(FXuchar *rawmsg, FXIPCMsg *msgheader);
protected:
	/*! Called by an asynchronous handler of a message to indicate that the
	message has been handled in some fashion. */
	void doAsyncHandled(FXIPCMsg *msg, HandledCode handled);
	/*! Called by an asynchronous handler of a message to indicate that an
	error occurred during the handling of the message. */
	void doAsyncHandled(FXIPCMsg *msg, FXException &e);
	/*!	You must define this in your subclass to handle/dispatch received messages.
	Called in the context of the internal monitor thread and as there is limited
	stack space plus requirements for low latency, consider dispatching heavier
	processing to a FX::QThreadPool */
	virtual HandledCode msgReceived(FXIPCMsg *msg)=0;
	/*! Called when a message of unknown type is received. Default action is to return
	an \c NotHandled code which causes a FX::FXIPCMsg_Unhandled to be returned to the
	sender if that message has an ack. Note that because the type is unknown, the
	message is not deserialised and so only the header is valid.
	\warning You cannot return \c HandledAsync from this function
	*/
	virtual HandledCode unknownMsgReceived(FXIPCMsg *msgheader, FXuchar *buffer);
	/*! Called when a message with a non-zero id is received but the id is not within
	the list of messages awaiting acks (this being an impossible case during normal
	operation). Note that a malicious attacker could simply set the message id on a
	normal message to cause this function to be called. If your subclass of FXIPCChannel
	implements channel-to-channel message tunnelling you will probably want to implement
	this. The default returns \c NotHandled and it is called before the message received
	filters. */
	virtual HandledCode lonelyMsgAckReceived(FXIPCMsg *msgheader, FXuchar *buffer);
	/*! Forces a channel close. Does not notify other end of close but does not close
	transport device either */
	void forceClose();
	/*! Outsources message handling to the channel's threadpool (used to perform
	significant amounts of processing). Simply do <tt>return invokeMsgHandler(mymethod, msg);</tt>
	and even if your method throws an exception it will be handled correctly. Your method
	should be of the spec <tt>HandledCode subclass::method(msgtype *msg)</tt> */
	template<typename fntype, typename msgtype> HandledCode invokeMsgHandler(fntype fnptr, msgtype *msg)
	{
		typedef typename Generic::FnInfo<fntype>::objectType objType;
		FXSTATIC_ASSERT((Generic::convertible<FXIPCChannel, objType>::value), Must_Be_Member_Function_Of_FXIPCChannel_subclass);
		typedef typename Generic::TL::create<void, fntype, FXAutoPtr<msgtype>, Generic::BoundFunctorV *>::value functtype;
		FXAutoPtr< Generic::BoundFunctor<functtype> > callv;
		// MSVC7.1 has issues with type deduction of int_handlerIndirect so we must help it
		void (FXIPCChannel::*mythunk)(fntype, FXAutoPtr<msgtype>, Generic::BoundFunctorV *)=&FXIPCChannel::int_handlerIndirect<objType, fntype, msgtype>;
		FXERRHM(callv=new Generic::BoundFunctor<functtype>(Generic::Functor<functtype>(this, mythunk)));
		Generic::TL::instance<0>(callv->parameters()).value=fnptr;
		Generic::TL::instance<1>(callv->parameters()).value=msg;
		Generic::TL::instance<2>(callv->parameters()).value=PtrPtr(callv);
		int_addMsgHandler(PtrPtr(callv));
		PtrRelease(callv);
		return HandledAsync;
	}
	//! Default implementation merely calls <tt>while(active()) doReception();</tt>
	virtual void run() { while(active()) doReception(); }
	//! You can override this, but you must call this implementation too
	virtual void *cleanup();
private:
	FXDLLLOCAL HandledCode invokeMsgReceived(MsgFilterSpec *what, FXIPCMsg *msg, FXIPCMsg &tmsg);
	inline FXDLLLOCAL void removeAck(void *ae);
	inline FXDLLLOCAL FXuint int_makeUniqueMsgId();
	Generic::BoundFunctorV *int_addMsgHandler(FXAutoPtr<Generic::BoundFunctorV> v);
	bool int_removeMsgHandler(Generic::BoundFunctorV *v);
	template<typename myrealtype, typename fntype, typename msgtype> void int_handlerIndirect(fntype fnptr, FXAutoPtr<msgtype> msg, Generic::BoundFunctorV *v)
	{	// Called in context of other thread
		int_removeMsgHandler(v);
		FXERRH_TRY
		{
			doAsyncHandled(PtrPtr(msg), (static_cast<myrealtype &>(*this).*fnptr)(PtrPtr(msg)));
		}
		FXERRH_CATCH(FXException &e)
		{
			if(msg->hasAck())
				doAsyncHandled(PtrPtr(msg), e);
			// Else I suppose I just throw it away?
		}
		FXERRH_ENDTRY
	}
};


/*! \class FXIPCChannelIndirector
\brief Base class indirecting messages to be sent over a subclass of a
FX::FXIPCChannel

FX::FXIPCChannel can't know what kind of application you have put it to eg;
how messages are routed, how you implemented message tunnelling etc.
Therefore, to aid code genericity, this little class inspects via
metaprogramming how you've subclassed FX::FXIPCChannel so that other TnFOX
code can invoke the correct underlying code.
*/
class FXIPCChannelIndirector
{
	FXIPCChannel *mychannel;
	FXuint myMsgChunk, myrouting;
	bool (FXIPCChannel::*sendMsgI)(FXIPCMsg *FXRESTRICT msgack, FXIPCMsg *FXRESTRICT msg, FXIPCChannel::endianiseSpec endianise, FXuint waitfor);
	bool (FXIPCChannel::*getMsgAckI)(FXIPCMsg *FXRESTRICT msgack, FXIPCMsg *FXRESTRICT msg, FXuint waitfor);
	/*void resetChunkAdd(FXIPCMsg *msg) const throw()
	{
		msg->type-=myMsgChunk;
	}*/
	inline void configMsg(FXIPCMsg *msg) const throw()
	{
		msg->type+=myMsgChunk;
		if(myrouting) msg->setRouting(myrouting);
	}
protected:
	FXIPCChannelIndirector() : mychannel(0), myMsgChunk(0), myrouting(0), sendMsgI(0), getMsgAckI(0) { }
	//! Returns the channel this is using
	FXIPCChannel *channel() const throw() { return mychannel; }
	//! Returns the message chunk this is using
	FXuint msgChunk() const throw() { return myMsgChunk; }
	//! Returns the routing this is using
	FXuint msgRouting() const throw() { return myrouting; }
	/*! Sets indirector to use the specified channel and message chunk,
	applying \em routing to all sent messages. */
	template<class msgchunk, class channel> void setIPCChannel(channel *_channel, FXuint addToMsgType=msgchunk::BaseCode, FXuint _routing=0)
	{
		mychannel=_channel;
		myMsgChunk=addToMsgType;
		myrouting=_routing;
		sendMsgI=&channel::sendMsgI;
		getMsgAckI=&channel::getMsgAck;
	}
	/*! Sends a message via the previously set channel, applying the previously specified routing
	and adding the previously specified message chunk. See FX::FXIPCChannel::sendMsg() */
	template<class msgacktype, class msgtype> bool sendMsg(msgacktype *FXRESTRICT msgack, msgtype *FXRESTRICT msg, FXuint waitfor=FXINFINITE)
	{
		FXSTATIC_ASSERT(!msgtype::id::hasAck || msgtype::id::code+1==msgacktype::id::code, AckMsg_Not_Ack_Of_Msg);
		configMsg(msg);
		//FXRBOp unconfig=FXRBObj(*this, &FXIPCChannelIndirector::resetChunkAdd, msg);
		return ((*mychannel).*sendMsgI)(msgack, msg, &msgtype::regtype::endianise, waitfor);
	}
	//! \overload
	template<class msgacktype, class msgtype> bool sendMsg(msgacktype &msgack, msgtype &msg, FXuint waitfor=FXINFINITE)
	{
		FXSTATIC_ASSERT(!msgtype::id::hasAck || msgtype::id::code+1==msgacktype::id::code, AckMsg_Not_Ack_Of_Msg);
		configMsg(&msg);
		//FXRBOp unconfig=FXRBObj(*this, &FXIPCChannelIndirector::resetChunkAdd, &msg);
		return ((*mychannel).*sendMsgI)(&msgack, &msg, &msgtype::regtype::endianise, waitfor);
	}
	//! \overload
	template<class msgtype> bool sendMsg(msgtype *msg)
	{
		FXSTATIC_ASSERT(!msgtype::id::hasAck, Cannot_Send_Msgs_With_Ack);
		configMsg(msg);
		//FXRBOp unconfig=FXRBObj(*this, &FXIPCChannelIndirector::resetChunkAdd, msg);
		return ((*mychannel).*sendMsgI)(0, msg, &msgtype::regtype::endianise, 0);
	}
	//! \overload
	template<class msgtype> bool sendMsg(msgtype &msg)
	{
		FXSTATIC_ASSERT(!msgtype::id::hasAck, Cannot_Send_Msgs_With_Ack);
		configMsg(&msg);
		//FXRBOp unconfig=FXRBObj(*this, &FXIPCChannelIndirector::resetChunkAdd, &msg);
		return ((*mychannel).*sendMsgI)(0, &msg, &msgtype::regtype::endianise, 0);
	}
	//! Gets the ack for a message previously sent using sendMsg(). See FX::FXIPCChannel::getMsgAck().
	bool getMsgAck(FXIPCMsg *FXRESTRICT msgack, FXIPCMsg *FXRESTRICT msg, FXuint waitfor=FXINFINITE)
	{
		return ((*mychannel).*getMsgAckI)(msgack, msg, waitfor);
	}
	//! \overload
	bool getMsgAck(FXIPCMsg &msgack, FXIPCMsg &msg, FXuint waitfor=FXINFINITE)
	{
		return ((*mychannel).*getMsgAckI)(&msgack, &msg, waitfor);
	}
};


} // namespace

#endif
