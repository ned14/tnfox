/* TFileBySyncDev.h
Permits a file type FXIODevice to be accessed through a FXIODeviceS
(C) 2002, 2004 Niall Douglas
Original version created: July 2002
This version created: 13th Jan 2004
*/

/* This code is licensed for use only in testing TnFOX facilities.
It may not be used by other code or incorporated into code without
prior written permission */

#ifndef _TFileBySyncDev_h_
#define _TFileBySyncDev_h_

#include "tmaster.h"
#include "FXIODeviceS.h"

/*! \file TFileBySyncDev.h
*/

namespace FX { class FXIPCChannel; }
namespace Tn {

/*! \class TFileBySyncDev
\brief Proxies FX::FXIODevice operations over a FX::FXIPCChannel

By default maintains eight buffers of \c pageSize-msgheader blocks. Transfers
over the IPC channel are done a block at a time - therefore \c pageSize-msgheader
is transferred with each transaction. When the file pointer exits a block, it
is marked dirty and will get overwritten by prefetched incoming blocks which
are requested immediately. Therefore this device is \b heavily optimised for
sequential transfers over a pipe transport. It also usefully is not bad for
sockets with three 1536 MTU's required with a 4Kb page size.

There are two types of instantiation - server and client. Server by default
provides default fetchData() and putData() using \em source. You can of course
override these with more complex stuff eg; like the data stream implementation.

A windowed protocol is used to maximise throughput even over high latency
connections. Hence when an exception is thrown it may refer to code executed
some time ago (eg; a "disc full" error coming from readBlock()).

Notes:
\li Uses a "push" mechanism internally.
\li Number of buffers is dynamically >> by FXProcess::memoryFull().
\li Buffers can be asynchronously prefetched.
\li If you will be doing a lot of random-access work, set buffers to one or
max two.
\li Sizing isn't implemented - therefore size() and truncate() doesn't have
any effect (this done with all other metainfo by the data stream code). Also
you can't open(IO_Append).
*/
struct TFileBySyncDevPrivate;
class TFileBySyncDev : public FXIODevice
{
	friend struct TFileBySyncDevPrivate;
	TFileBySyncDevPrivate *p;
	inline void channelInstall() const;
	inline void channelUninstall() const;
	inline void requestData();
	inline void checkErrors();
	TFileBySyncDev(const TFileBySyncDev &);
	TFileBySyncDev &operator=(const TFileBySyncDev &);
public:
	/*! Constructs a server or client instance depending on \em server.
	If a server, you can specifiy a source via \em source */
	TFileBySyncDev(bool server, FXIPCChannel &ctrl, FXIODevice *source=0);
	~TFileBySyncDev();
	//! Returns the IPC channel used as control
	FXIPCChannel &channel() const;
	//! Sets the IPC channel used as control
	void setChannel(FXIPCChannel &ch);
	//! Sets if operational statistics are printed to \c stdout
	void setPrintStatistics(bool v);
	//! Returns the size of the block used
	u32 blockSize() const;
	//! (Client instance only) Returns the number of blocks buffered
	u32 buffers() const;
	//! (Client instance only) Sets the number and size of buffered blocks
	void setTransferParams(u32 no, u32 size);
	/*! (Client instance only) Invalidates client buffers if they are within
	the specified range, returning true if buffers were invalidated */
	bool invalidateBuffers(FXfval offset, FXfval length);
	/*! (Client instance only) Causes the device to report a connection lost
	exception. This is useful if the channel monitor thread exits and another
	thread is trying to read or write using this device */
	void invokeConnectionLost();

	//! (Server instance only) Returns the source i/o device
	FXIODevice *source() const;
	//! (Server instance only) Sets the source i/o device
	void setSource(FXIODevice *source);
protected:
	//! (Server instance only) Called to retrieve data served by this class
	virtual FXuval fetchData(FXuchar *data, FXuval amount, FXfval offset);
	//! (Server instance only) Called to put data
	virtual FXuval putData(FXfval offset, FXuchar *data, FXuval amount);
public:
	bool open(FXuint mode);
	void close();
	void flush();
	//! \warning Always returns 0xffffffffffffffff
	FXfval size() const;
	//! \warning Not implemented
	void truncate(FXfval size);
	FXfval at() const;
	bool at(FXfval newpos);
	FXuval readBlock(char *data, FXuval maxlen);
	FXuval writeBlock(const char *data, FXuval maxlen);
	FXuval readBlockFrom(char *data, FXuval maxlen, FXfval pos);
	FXuval writeBlockTo(FXfval pos, const char *data, FXuval maxlen);
	int ungetch(int c);
};

} // namespace

#endif
