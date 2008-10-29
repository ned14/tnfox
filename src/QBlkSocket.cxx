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

#include "QBlkSocket.h"
#include "FXString.h"
#include "QThread.h"
#include "FXException.h"
#include "QTrans.h"
#include "FXProcess.h"
#include "FXRollback.h"
#include "FXErrCodes.h"
#include "FXNetwork.h"
#include "FXACL.h"
#include <assert.h>
#include "sigpipehandler.h"

#ifndef USE_POSIX
#define USE_WINAPI
#include "WinSock2.h"
#include "MSWSock.h"
#include "WindowsGubbins.h"
#include "WS2Tcpip.h"

static const char *decodeWinsockErr(int code)
{	// Annoying that I have to do this in this day and age :(
	switch(code)
	{
	case WSAEINTR:
		return "Interrupted function call";
	case WSAEACCES:
		return "Permission denied";
	case WSAEFAULT:
		return "Bad address";
	case WSAEINVAL:
		return "Invalid argument";
	case WSAEMFILE:
		return "Too many open files";
	case WSAEWOULDBLOCK:
		return "Resource temporarily unavailable";
	case WSAEINPROGRESS:
		return "Operation now in progress";
	case WSAEALREADY:
		return "Operation already in progress";
	case WSAENOTSOCK:
		return "Socket operation on nonsocket";
	case WSAEDESTADDRREQ:
		return "Destination address required";
	case WSAEMSGSIZE:
		return "Message too long";
	case WSAEPROTOTYPE:
		return "Protocol wrong type for socket";
	case WSAENOPROTOOPT:
		return "Bad protocol option";
	case WSAEPROTONOSUPPORT:
		return "Protocol not supported";
	case WSAESOCKTNOSUPPORT:
		return "Socket type not supported";
	case WSAEOPNOTSUPP:
		return "Operation not supported";
	case WSAEPFNOSUPPORT:
		return "Protocol family not supported";
	case WSAEAFNOSUPPORT:
		return "Address family not supported by protocol family";
	case WSAEADDRINUSE:
		return "Address already in use";
	case WSAEADDRNOTAVAIL:
		return "Cannot assign requested address";
	case WSAENETDOWN:
		return "Network is down";
	case WSAENETUNREACH:
		return "Network is unreachable";
	case WSAENETRESET:
		return "Network dropped connection on reset";
	case WSAECONNABORTED:
		return "Software caused connection abort";
	case WSAECONNRESET:
		return "Connection reset by peer";
	case WSAENOBUFS:
		return "No buffer space available";
	case WSAEISCONN:
		return "Socket is already connected";
	case WSAENOTCONN:
		return "Socket is not connected";
	case WSAESHUTDOWN:
		return "Cannot send after socket shutdown";
	case WSAETIMEDOUT:
		return "Connection timed out";
	case WSAECONNREFUSED:
		return "Connection refused";
	case WSAEHOSTDOWN:
		return "Host is down";
	case WSAEHOSTUNREACH:
		return "No route to host";
	case WSAEPROCLIM:
		return "Too many processes";
	case WSASYSNOTREADY:
		return "Network subsystem is unavailable";
	case WSAVERNOTSUPPORTED:
		return "Winsock.dll version out of range";
	case WSANOTINITIALISED:
		return "Successful WSAStartup not yet performed";
	case WSAEDISCON:
		return "Graceful shutdown in progress";
	case WSATYPE_NOT_FOUND:
		return "Class type not found";
	case WSAHOST_NOT_FOUND:
		return "Host not found";
	case WSATRY_AGAIN:
		return "Nonauthoritative host not found";
	case WSANO_RECOVERY:
		return "This is a nonrecoverable error";
	case WSANO_DATA:
		return "Valid name, no data record of requested type";
	}
	return "Unknown";
}

#define FXERRHSKT(exp) { int __res=(int)(exp); if(SOCKET_ERROR==__res) { int __errorcode=WSAGetLastError(); \
	if(WSAENETDOWN==__errorcode || WSAENETRESET==__errorcode || WSAECONNABORTED==__errorcode || WSAECONNRESET==__errorcode || WSAETIMEDOUT==__errorcode) \
		{ FXERRGCONLOST("Connection Lost", 0); } \
	else { FXERRGIO(decodeWinsockErr(__errorcode)); } } }
#endif
#ifdef USE_POSIX
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#ifdef __FreeBSD__
#include <sys/sysctl.h>
#endif

#define FXERRHSKT(exp) { int __res=(exp); if(__res<0) { \
	if(EPIPE==errno) \
		{ FXERRGCONLOST("Connection Lost", 0); } \
	else { FXERRGIO(strerror(errno)); } } }
#endif
#include "tnfxselect.h"

#include "FXMemDbg.h"
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

enum SocketState
{
	NoIP=0,
	IPv4,
	IPv6
};
static SocketState socketsEnabled;
class QBlkSocket_SocketInit
{
public:
	QBlkSocket_SocketInit()
	{
#ifdef USE_WINAPI
		WSADATA wd;
		FXERRH(0==WSAStartup(MAKEWORD(2,2), &wd), "Failed to initialise Winsock library", QBLKSOCKET_NOWINSOCK, 0);
		if(HIBYTE(wd.wVersion)>2 || (2==HIBYTE(wd.wVersion) && LOBYTE(wd.wVersion)>=2))
			socketsEnabled=IPv6;
		else
			socketsEnabled=IPv4;
#endif
#ifdef USE_POSIX
		socketsEnabled=IPv6;
#endif
	}
	~QBlkSocket_SocketInit()
	{
#ifdef USE_WINAPI
		WSACleanup();
#endif
		socketsEnabled=NoIP;
	}
};
FXProcess_StaticInit<QBlkSocket_SocketInit> mystaticinit("QBlkSocket");

struct FXDLLLOCAL QBlkSocketPrivate : public QMutex
{
	QBlkSocket::Type type;
	bool unique, amServer, connected, monitoring;
	FXint maxPending;
	struct Req_t
	{
		QHostAddress addr;
		FXushort port;
	} req;
	struct Mine_t
	{
		QHostAddress addr;
		FXushort port;
	} mine;
	struct Peer_t
	{
		QHostAddress addr;
		FXushort port;
	} peer;
#ifdef USE_WINAPI
	SOCKET handle;
	OVERLAPPED olr, olw;
#endif
#ifdef USE_POSIX
	int handle;
#endif
	QBlkSocketPrivate(QBlkSocket::Type _type, FXushort port) : type(_type), unique(false), amServer(false),
		connected(false), monitoring(false), maxPending(50), handle(0), QMutex()
	{
		req.port=port; mine.port=0; peer.port=0;
#ifdef USE_WINAPI
		memset(&olr, 0, sizeof(olr));
		memset(&olw, 0, sizeof(olw));
#endif
	}
	QBlkSocketPrivate(const QBlkSocketPrivate &o, int h) : type(o.type), unique(o.unique), amServer(o.amServer),
		connected(o.connected), monitoring(o.monitoring), maxPending(o.maxPending), handle(h), req(o.req), mine(o.mine), QMutex()
	{
		peer.port=0;
#ifdef USE_WINAPI
		memset(&olr, 0, sizeof(olr));
		memset(&olw, 0, sizeof(olw));
#endif
	}
};

void *QBlkSocket::int_getOSHandle() const
{
#ifdef USE_WINAPI
	return (void *) p->olr.hEvent;
#endif
#ifdef USE_POSIX
	return (void *) p->handle;
#endif
}

QBlkSocket::QBlkSocket(QBlkSocket::Type type, FXushort port) : p(0), QIODeviceS()
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRH(NoIP!=socketsEnabled, QTrans::tr("QBlkSocket", "Sockets are not enabled on this machine"), QBLKSOCKET_NOSOCKETS, 0);
	FXERRHM(p=new QBlkSocketPrivate(type, port));
	unconstr.dismiss();
}

QBlkSocket::QBlkSocket(const QHostAddress &addr, FXushort port, QBlkSocket::Type type) : p(0), QIODeviceS()
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRH(NoIP!=socketsEnabled, QTrans::tr("QBlkSocket", "Sockets are not enabled on this machine"), QBLKSOCKET_NOSOCKETS, 0);
	FXERRH(!(IPv6!=socketsEnabled && addr.isIp6Addr()), QTrans::tr("QBlkSocket", "This machine cannot perform IPv6"), QBLKSOCKET_NOIPV6, 0);
	FXERRHM(p=new QBlkSocketPrivate(type, port));
	p->req.addr=addr;
	unconstr.dismiss();
}

QBlkSocket::QBlkSocket(const FXString &addrname, FXushort port, QBlkSocket::Type type) : p(0), QIODeviceS()
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRH(NoIP!=socketsEnabled, QTrans::tr("QBlkSocket", "Sockets are not enabled on this machine"), QBLKSOCKET_NOSOCKETS, 0);
	QHostAddress addr=FXNetwork::dnsLookup(addrname);
	FXERRH(!(IPv6!=socketsEnabled && addr.isIp6Addr()), QTrans::tr("QBlkSocket", "This machine cannot perform IPv6"), QBLKSOCKET_NOIPV6, 0);
	FXERRHM(p=new QBlkSocketPrivate(type, port));
	p->req.addr=addr;
	unconstr.dismiss();
}

#ifndef HAVE_CPP0XRVALUEREFS
#ifdef HAVE_CONSTTEMPORARIES
QBlkSocket::QBlkSocket(const QBlkSocket &other) : p(other.p), QIODeviceS(other)
{
	QBlkSocket &o=const_cast<QBlkSocket &>(other);
#else
QBlkSocket::QBlkSocket(QBlkSocket &o) : p(o.p), QIODeviceS(o)
{
#endif
#else
QBlkSocket::QBlkSocket(QBlkSocket &&o) : p(std::move(o.p)), QIODeviceS(std::forward<QIODeviceS>(o))
{
#endif
	o.p=0;
	o.setFlags(0);
}

QBlkSocket::QBlkSocket(const QBlkSocket &o, int h) : p(0), QIODeviceS(o)
{
	FXERRHM(p=new QBlkSocketPrivate(*o.p, h));
}

QBlkSocket::~QBlkSocket()
{ FXEXCEPTIONDESTRUCT1 {
	close();
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

QBlkSocket::Type QBlkSocket::type() const
{
	return p->type;
}

void QBlkSocket::setType(Type type)
{
	p->type=type;
}

const QHostAddress &QBlkSocket::address() const
{
	return p->mine.addr;
}

FXushort QBlkSocket::port() const
{
	return p->mine.port;
}

QHostAddress QBlkSocket::peerAddress() const
{
	QMtxHold h(p);
	return p->peer.addr;
}

FXushort QBlkSocket::peerPort() const
{
	return p->peer.port;
}

const QHostAddress &QBlkSocket::requestedAddress() const
{
	return p->req.addr;
}

FXushort QBlkSocket::requestedPort() const
{
	return p->req.port;
}

void QBlkSocket::setRequestedAddressAndPort(const QHostAddress &reqAddr, FXushort port)
{
	QMtxHold h(p);
	p->req.addr=reqAddr;
	p->req.port=port;
}

bool QBlkSocket::isUnique() const
{
	QMtxHold h(p);
	return p->unique;
}

void QBlkSocket::setUnique(bool a)
{
	QMtxHold h(p);
	p->unique=a;
	p->mine.port=0;
}

FXuval QBlkSocket::receiveBufferSize() const
{
	QMtxHold h(p);
	if(isOpen())
	{
		int val=0;
		socklen_t valsize=sizeof(val);
		FXERRHSKT(::getsockopt(p->handle, SOL_SOCKET, SO_RCVBUF, (char *) &val, &valsize));
		return val;
	}
	return 0;
}

void QBlkSocket::setReceiveBufferSize(FXuval newsize)
{
	QMtxHold h(p);
	if(isOpen())
	{
		int val=(int) newsize;
		FXERRHSKT(::setsockopt(p->handle, SOL_SOCKET, SO_RCVBUF, (char *) &val, sizeof(val)));
	}
}

FXuval QBlkSocket::sendBufferSize() const
{
	QMtxHold h(p);
	if(isOpen())
	{
		int val=0;
		socklen_t valsize=sizeof(val);
		FXERRHSKT(::getsockopt(p->handle, SOL_SOCKET, SO_SNDBUF, (char *) &val, &valsize));
		return val;
	}
	return 0;
}

void QBlkSocket::setSendBufferSize(FXuval newsize)
{
	QMtxHold h(p);
	if(isOpen())
	{
		int val=(int) newsize;
		FXERRHSKT(::setsockopt(p->handle, SOL_SOCKET, SO_SNDBUF, (char *) &val, sizeof(val)));
	}
}

FXuval QBlkSocket::maxDatagramSize() const
{
	QMtxHold h(p);
	if(isOpen())
	{
#ifdef USE_WINAPI
		unsigned int val=0;
		socklen_t valsize=sizeof(val);
		FXERRHSKT(::getsockopt(p->handle, SOL_SOCKET, SO_MAX_MSG_SIZE, (char *) &val, &valsize));
		return val;
#endif
#ifdef USE_POSIX
#ifdef __linux__
		// Couldn't find any other way, so determined by trial and error
		return 65507;
#endif
#ifdef __FreeBSD__
		int val;
		size_t vallen=sizeof(val);
		FXERRHOS(sysctlbyname("net.inet.udp.maxdgram", &val, &vallen, NULL, 0));
		return val;
#endif
#endif
	}
	return 0;
}

FXint QBlkSocket::maxPending() const
{
	QMtxHold h(p);
	return p->maxPending;
}

void QBlkSocket::setMaxPending(FXint newp)
{
	QMtxHold h(p);
	p->maxPending=newp;
	if(isOpen())
	{
		FXERRHSKT(::listen(p->handle, p->maxPending));
	}
}

bool QBlkSocket::addressReusable() const
{
	QMtxHold h(p);
	if(isOpen())
	{
		int val=0;
		socklen_t valsize=sizeof(val);
		FXERRHSKT(::getsockopt(p->handle, SOL_SOCKET, SO_REUSEADDR, (char *) &val, &valsize));
		return val!=0;
	}
	return true;
}

void QBlkSocket::setAddressReusable(bool newar)
{
	QMtxHold h(p);
	if(isOpen())
	{
		int val=(int) newar;
		FXERRHSKT(::setsockopt(p->handle, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof(val)));
	}
}

bool QBlkSocket::keepAlive() const
{
	QMtxHold h(p);
	if(isOpen())
	{
		int val=0;
		socklen_t valsize=sizeof(val);
		FXERRHSKT(::getsockopt(p->handle, SOL_SOCKET, SO_KEEPALIVE, (char *) &val, &valsize));
		return val!=0;
	}
	return false;
}

void QBlkSocket::setKeepAlive(bool newar)
{
	QMtxHold h(p);
	if(isOpen())
	{
		int val=(int) newar;
		FXERRHSKT(::setsockopt(p->handle, SOL_SOCKET, SO_KEEPALIVE, (char *) &val, sizeof(val)));
	}
}

FXint QBlkSocket::lingerPeriod() const
{
	QMtxHold h(p);
	if(isOpen())
	{
		struct linger l;
		socklen_t valsize=sizeof(l);
		FXERRH(Stream==p->type, "Only available for stream sockets", QBLKSOCKET_NOTSTREAM, 0);
		FXERRHSKT(::getsockopt(p->handle, SOL_SOCKET, SO_LINGER, (char *) &l, &valsize));
		return l.l_onoff ? l.l_linger : -1;
	}
	return 0;
}

void QBlkSocket::setLingerPeriod(FXint period)
{
	QMtxHold h(p);
	if(isOpen())
	{
		struct linger l;
		l.l_onoff=period>=0;
		l.l_linger=(unsigned short) period;
		FXERRH(Stream==p->type, "Only available for stream sockets", QBLKSOCKET_NOTSTREAM, 0);
		FXERRHSKT(::setsockopt(p->handle, SOL_SOCKET, SO_LINGER, (char *) &l, sizeof(l)));
	}
}

bool QBlkSocket::usingNagles() const
{
	QMtxHold h(p);
	if(isOpen())
	{
		int val=0;
		socklen_t valsize=sizeof(val);
		FXERRH(Stream==p->type, "Only available for stream sockets", QBLKSOCKET_NOTSTREAM, 0);
		struct protoent *pe=getprotobyname("ip");
		if(!pe) FXERRHSKT(-1);
		FXERRHSKT(::getsockopt(p->handle, pe->p_proto, TCP_NODELAY, (char *) &val, &valsize));
		return val!=0;
	}
	return true;
}

void QBlkSocket::setUsingNagles(bool newar)
{
	QMtxHold h(p);
	if(isOpen())
	{
		int val=(int) newar;
		FXERRH(Stream==p->type, "Only available for stream sockets", QBLKSOCKET_NOTSTREAM, 0);
		struct protoent *pe=getprotobyname("ip");
		if(!pe) FXERRHSKT(-1);
		FXERRHSKT(::setsockopt(p->handle, pe->p_proto, TCP_NODELAY, (char *) &val, sizeof(val)));
	}
}

bool QBlkSocket::connected() const
{
	QMtxHold h(p);
	return p->connected;
}

static inline sockaddr *makeSockAddr(int &salen, sockaddr_in6 &sa6, const QHostAddress &addr, FXushort port)
{
	if(addr.isIp4Addr())
	{
		sockaddr_in *sa=(sockaddr_in *) &sa6;
		salen=sizeof(sockaddr_in);
		sa->sin_family=AF_INET;
		sa->sin_port=htons(port);
		sa->sin_addr.s_addr=htonl(addr.ip4Addr());
		// Some systems define a sa_len member at the top of the structure
		if((void *) &sa->sin_family>(void *) sa)
			*(char *)(sa)=salen;
	}
	if(addr.isIp6Addr())
	{
		salen=sizeof(sockaddr_in6);
		sa6.sin6_family=AF_INET6;
		sa6.sin6_port=htons(port);
		sa6.sin6_flowinfo=0;
		memcpy(sa6.sin6_addr.s6_addr, addr.ip6Addr(), 16);
		//sa6.sin6_scope_id=0;
		// Some systems define a sin6_len member at the top of the structure
		if((void *) &sa6.sin6_family>(void *) &sa6)
			*(char *)(&sa6)=salen;
	}
	return (sockaddr *) &sa6;
}

static inline void readSockAddr(QHostAddress &addr, FXushort &port, sockaddr_in6 *sa6)
{
	if(AF_INET6==sa6->sin6_family)
	{	// It's IPv6
		addr.setAddress(sa6->sin6_addr.s6_addr);
		port=ntohs(sa6->sin6_port);
	}
	else if(AF_INET==sa6->sin6_family)
	{	// It's IPv4
		sockaddr_in *sa=(sockaddr_in *) sa6;
		addr.setAddress(ntohl(sa->sin_addr.s_addr));
		port=ntohs(sa->sin_port);
	}
	else { assert(0); }
}

void QBlkSocket::fillInAddrs(bool incPeer)
{
	{	// Mine
		sockaddr_in6 sa6={0};
		socklen_t salen=sizeof(sa6);
		FXERRHSKT(getsockname(p->handle, (sockaddr *) &sa6, &salen));
		readSockAddr(p->mine.addr, p->mine.port, &sa6);
	}
	if(incPeer)
	{	// Peer
		sockaddr_in6 sa6={0};
		socklen_t salen=sizeof(sa6);
		FXERRHSKT(getpeername(p->handle, (sockaddr *) &sa6, &salen));
		readSockAddr(p->peer.addr, p->peer.port, &sa6);
	}
}

void QBlkSocket::zeroAddrs()
{
	p->mine.addr=p->peer.addr=QHostAddress();
	p->mine.port=p->peer.port=0;
}

void QBlkSocket::setupSocket()
{	// Called to "fix up" any newly created socket handles
#ifdef SO_NOSIGPIPE
	// Use FreeBSD extension to disable broken pipe signals
	int val=1;
	FXERRHSKT(::setsockopt(p->handle, SOL_SOCKET, SO_NOSIGPIPE, (char *) &val, sizeof(val)));
#endif
}

bool QBlkSocket::create(FXuint mode)
{
	QMtxHold h(p);
	QThread_DTHold dth;
	close();
	FXERRH(p->mine.addr.isLocalMachine() || p->mine.addr.isNull(), "Server sockets must be local", QBLKSOCKET_NONLOCALCREATE, FXERRH_ISDEBUG);
	FXERRHSKT(p->handle=::socket(p->mine.addr.isIp6Addr() ? PF_INET6 : PF_INET, (p->type==Datagram) ? SOCK_DGRAM : SOCK_STREAM, 0));
#ifdef USE_WINAPI
	if(mode & IO_ReadOnly)
	{
		FXERRHWIN(INVALID_HANDLE_VALUE!=(p->olr.hEvent=CreateEvent(NULL, TRUE, FALSE, NULL)));
	}
#endif
#ifdef USE_POSIX
	FXERRHOS(::fcntl(p->handle, F_SETFD, ::fcntl(p->handle, F_GETFD, 0)|FD_CLOEXEC));
#endif
	setupSocket();
	if(Stream==p->type)
	{
		struct linger l;
		l.l_onoff=1;
		l.l_linger=5;
		FXERRHSKT(::setsockopt(p->handle, SOL_SOCKET, SO_LINGER, (char *) &l, sizeof(l)));
	}
	sockaddr_in6 sa6={0};
	int salen;
	sockaddr *sa=makeSockAddr(salen, sa6, p->req.addr, p->req.port);
	FXERRHSKT(::bind(p->handle, sa, salen));
	if(Stream==p->type && !(mode & IO_QuietSocket))
	{
		FXERRHSKT(::listen(p->handle, p->maxPending));
	}
	fillInAddrs(false);
	p->amServer=true;
	p->connected=false;
	setFlags((mode & IO_ModeMask)|IO_Open);
	return true;
}

bool QBlkSocket::open(FXuint mode)
{
	QMtxHold h(p);
	if(isOpen())
	{	// I keep fouling myself up here, so assertion check
		if(QIODevice::mode()!=mode) FXERRGIO(QTrans::tr("QBlkSocket", "Device reopen has different mode"));
	}
	else
	{
		QThread_DTHold dth;
		FXERRHSKT(p->handle=::socket(p->mine.addr.isIp6Addr() ? PF_INET6 : PF_INET, (p->type==Datagram) ? SOCK_DGRAM : SOCK_STREAM, 0));
#ifdef USE_WINAPI
		if(mode & IO_ReadOnly)
		{
			FXERRHWIN(INVALID_HANDLE_VALUE!=(p->olr.hEvent=CreateEvent(NULL, TRUE, FALSE, NULL)));
		}
		if(mode & IO_WriteOnly)
		{
			FXERRHWIN(INVALID_HANDLE_VALUE!=(p->olw.hEvent=CreateEvent(NULL, TRUE, FALSE, NULL)));
		}
#endif
#ifdef USE_POSIX
		FXERRHOS(::fcntl(p->handle, F_SETFD, ::fcntl(p->handle, F_GETFD, 0)|FD_CLOEXEC));
#endif
		setupSocket();
		if(Stream==p->type)
		{
			struct linger l;
			l.l_onoff=1;
			l.l_linger=5;
			FXERRHSKT(::setsockopt(p->handle, SOL_SOCKET, SO_LINGER, (char *) &l, sizeof(l)));
			sockaddr_in6 sa6={0};
			int salen;
			sockaddr *sa=makeSockAddr(salen, sa6, p->req.addr, p->req.port);
			h.unlock();
			int ret=::connect(p->handle, sa, salen);
			h.relock();
			FXERRHSKT(ret);
			fillInAddrs(true);
		}
		p->amServer=false;
		p->connected=true;
#ifdef USE_WINAPI
		if(mode & IO_ReadOnly)
		{	// Monitor inbound (start a read reading no data) so event object is correct
			DWORD ret=ReadFile((HANDLE) p->handle, 0, 0, 0, &p->olr);
			assert(ret==0 && GetLastError()==ERROR_IO_PENDING);
			p->monitoring=true;
		}
#endif
		setFlags((mode & IO_ModeMask)|IO_Open);
	}
	return true;
}

void QBlkSocket::close()
{
	if(p)
	{
		QMtxHold h(p);
		QThread_DTHold dth;
		if(p->connected)
		{	// Ignore any errors
#ifdef USE_WINAPI
			::shutdown(p->handle, SD_BOTH);
#endif
#ifdef USE_POSIX
			::shutdown(p->handle, SHUT_RDWR);
#endif
			p->connected=false;
		}
		if(p->handle)
		{	// This will block for lingerPeriod
			h.unlock();
#ifdef USE_WINAPI
			FXERRHSKT(::closesocket(p->handle));
#endif
#ifdef USE_POSIX
			FXERRHSKT(::close(p->handle));
#endif
			p->handle=0;
			h.relock();
		}
#ifdef USE_WINAPI
		if(p->olr.hEvent)
		{
			FXERRHWIN(CloseHandle(p->olr.hEvent));
			p->olr.hEvent=0;
		}
		if(p->olw.hEvent)
		{
			FXERRHWIN(CloseHandle(p->olw.hEvent));
			p->olw.hEvent=0;
		}
#endif
		zeroAddrs();
		setFlags(0);
	}
}

void QBlkSocket::flush()
{
#if 0
	// Can't implement this on Windows :(
	if(isOpen())
	{
#ifdef USE_WINAPI
		FXERRHWIN(FlushFileBuffers((HANDLE) p->handle));
#endif
#ifdef USE_POSIX
		FXERRHSKT(::fsync(p->handle));
#endif
	}
#endif
}

bool QBlkSocket::reset()
{
	close();
	return (p->amServer) ? create(flags()) : open(flags());
}

FXfval QBlkSocket::size() const
{
	QMtxHold h(p);
	unsigned long waiting=0;
	if(isOpen())
	{
		QThread_DTHold dth;
#ifdef USE_WINAPI
		FXERRHSKT(::ioctlsocket(p->handle, FIONREAD, &waiting));
#endif
#ifdef USE_POSIX
		FXERRHSKT(::ioctl(p->handle, FIONREAD, &waiting));
#endif
	}
	return (FXfval) waiting;
}

const FXACL &QBlkSocket::permissions() const
{
	static QMutex lock;
	static FXACL perms;
	QMtxHold lh(lock);
	if(perms.count()) return perms;
	perms.append(FXACL::Entry(FXACLEntity::everything(), 0, FXACL::Permissions().setAll()));
	return perms;
}

FXuval QBlkSocket::readBlock(char *data, FXuval maxlen)
{
	QMtxHold h(p);
	if(!QIODevice::isReadable()) FXERRGIO(QTrans::tr("QBlkSocket", "Not open for reading"));
	if(isOpen())
	{
		FXuval readed=(FXuval) -1;
#ifdef USE_WINAPI
		{
			WSABUF wsabuf; wsabuf.buf=data; wsabuf.len=(u_long) maxlen;
			DWORD _readed=0, flags=0;
			sockaddr_in6 sa6={0};
			socklen_t salen=sizeof(sa6);
			if(p->monitoring)
			{
				FXERRHWIN(CancelIo((HANDLE) p->handle));
				p->monitoring=false;
			}
			int ret=SOCKET_ERROR;
			if(Stream==p->type)
				ret=WSARecv(p->handle, &wsabuf, 1, &_readed, &flags, &p->olr, NULL);
			else if(Datagram==p->type)
				ret=WSARecvFrom(p->handle, &wsabuf, 1, &_readed, &flags, (sockaddr *) &sa6, &salen, &p->olr, NULL);
			if(!ret && !_readed)
			{	// Indicates graceful closure ie; connection lost
				FXERRGCONLOST("Connection closed", 0);
			}
			if(SOCKET_ERROR==ret)
			{
				if(WSA_IO_PENDING==WSAGetLastError())
				{
					h.unlock();
					HANDLE hs[2]; hs[0]=p->olr.hEvent; hs[1]=QThread::int_cancelWaiterHandle();
					DWORD ret=WaitForMultipleObjects(2, hs, FALSE, INFINITE);
					h.relock();
					if(WAIT_OBJECT_0+1==ret)
					{	// There appears to be no way to cancel overlapping i/o on sockets, so here's best attempt
						FXERRHSKT(::shutdown(p->handle, SD_BOTH));
						FXERRHSKT(::closesocket(p->handle));
						p->handle=0;
						p->connected=false;
						h.unlock();
						QThread::current()->checkForTerminate();
					}
					else if(WAIT_OBJECT_0!=ret)
					{ FXERRHSKT(ret); }
					else if(!WSAGetOverlappedResult(p->handle, &p->olr, &_readed, FALSE, &flags))
					{ FXERRHSKT(SOCKET_ERROR); }
				}
				else { FXERRHSKT(ret); }
			}
			if(Datagram==p->type && _readed)
				readSockAddr(p->peer.addr, p->peer.port, &sa6);
			readed=(FXuval) _readed;
		}
#endif
#ifdef USE_POSIX
		h.unlock();
		if(Stream==p->type)
		{
#ifdef __APPLE__
			// Mac OS X has such inconsistent thread cancellation support :(
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(p->handle, &fds);
			tnfxselect(p->handle+1, &fds, 0, 0, NULL);
#endif
			/* 31st Jan 2005 ned: Finally fixed segfault on Linux 2.6 kernels when
			library was being using dynamically. For some odd reason it doesn't like
			being thread cancelled during a recv(), so I've moved permanently to
			read() which FreeBSD needed anyway */
			readed=::read(p->handle, data, maxlen);
			//h.relock();
		}
		else if(Datagram==p->type)
		{
			sockaddr_in6 sa6={0};
			socklen_t salen=sizeof(sa6);
#ifdef __FreeBSD__
			// Unfortunately recvfrom is not a thread cancellation point on FreeBSD, so
			// we use select() to do it for us
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(p->handle, &fds);
			::select(p->handle+1, &fds, 0, 0, NULL);
#endif
			readed=::recvfrom(p->handle, data, maxlen, 0, (sockaddr *) &sa6, &salen);
			h.relock();
			if((FXuval)-1!=readed)
				readSockAddr(p->peer.addr, p->peer.port, &sa6);
		}
		FXERRHSKT(readed);
#endif
		return readed;
	}
	return 0;
}

FXuval QBlkSocket::writeBlock(const char *data, FXuval maxlen)
{
	QMtxHold h(p);
	if(!isWriteable()) FXERRGIO(QTrans::tr("QBlkSocket", "Not open for writing"));
	if(isOpen())
	{
		FXuval written=(FXuval) -1;
#ifdef USE_WINAPI
		{
			WSABUF wsabuf; wsabuf.buf=(char *) data; wsabuf.len=(u_long) maxlen;
			DWORD _written=0, flags=0;
			int ret=SOCKET_ERROR;
			if(Stream==p->type)
				ret=WSASend(p->handle, &wsabuf, 1, &_written, flags, &p->olw, NULL);
			else if(Datagram==p->type)
			{
				sockaddr_in6 sa6={0};
				int salen;
				sockaddr *sa=makeSockAddr(salen, sa6, p->req.addr, p->req.port);
				ret=WSASendTo(p->handle, &wsabuf, 1, &_written, flags, sa, salen, &p->olw, NULL);
			}
			if(SOCKET_ERROR==ret)
			{
				if(WSA_IO_PENDING==WSAGetLastError())
				{
					h.unlock();
					HANDLE hs[2]; hs[0]=p->olw.hEvent; hs[1]=QThread::int_cancelWaiterHandle();
					DWORD ret=WaitForMultipleObjects(2, hs, FALSE, INFINITE);
					h.relock();
					if(WAIT_OBJECT_0+1==ret)
					{	// There appears to be no way to cancel overlapping i/o on sockets, so here's best attempt
						FXERRHSKT(::shutdown(p->handle, SD_BOTH));
						FXERRHSKT(::closesocket(p->handle));
						p->handle=0;
						p->connected=false;
						h.unlock();
						QThread::current()->checkForTerminate();
					}
					else if(WAIT_OBJECT_0!=ret)
					{ FXERRHSKT(ret); }
					else if(!WSAGetOverlappedResult(p->handle, &p->olw, &_written, FALSE, &flags))
					{ FXERRHSKT(SOCKET_ERROR); }
				}
				else { FXERRHSKT(ret); }
			}
			written=(FXuval) _written;
		}
#endif
#ifdef USE_POSIX
		QIODeviceS_SignalHandler::lockWrite();
		h.unlock();
		if(Stream==p->type)
		{
#ifdef __linux__
			// send() is a cancellable point on Linux
			written=::send(p->handle, data, maxlen, MSG_NOSIGNAL);
#else
			// send() isn't a cancellable point on all platforms, so use write()
			written=::write(p->handle, data, maxlen);
#endif
		}
		else if(Datagram==p->type)
		{
			sockaddr_in6 sa6={0};
			int salen;
			sockaddr *sa=makeSockAddr(salen, sa6, p->req.addr, p->req.port);
			written=::sendto(p->handle, data, maxlen,
#ifdef __linux__
				MSG_NOSIGNAL,
#else
				0,
#endif
				sa, salen);
		}
		h.relock();
		FXERRHSKT(written);
		if(QIODeviceS_SignalHandler::unlockWrite())		// Nasty this
			FXERRGCONLOST("Broken socket", 0);
#endif
		if(isRaw()) flush();
		return written;
	}
	return 0;
}

FXuval QBlkSocket::writeBlock(const char *data, FXuval maxlen, const QHostAddress &addr, FXushort port)
{
	QMtxHold h(p);
	if(Datagram!=p->type) FXERRGIO(QTrans::tr("QBlkSocket", "Not a datagram socket"));
	if(!isWriteable()) FXERRGIO(QTrans::tr("QBlkSocket", "Not open for writing"));
	if(isOpen())
	{
		FXuval written=(FXuval) -1;
#ifdef USE_WINAPI
		{
			WSABUF wsabuf; wsabuf.buf=(char *) data; wsabuf.len=(u_long) maxlen;
			DWORD _written=0, flags=0;
			int ret=SOCKET_ERROR;
			if(Stream==p->type)
				ret=WSASend(p->handle, &wsabuf, 1, &_written, flags, &p->olw, NULL);
			else if(Datagram==p->type)
			{
				sockaddr_in6 sa6={0};
				int salen;
				sockaddr *sa=makeSockAddr(salen, sa6, addr, port);
				ret=WSASendTo(p->handle, &wsabuf, 1, &_written, flags, sa, salen, &p->olw, NULL);
			}
			if(SOCKET_ERROR==ret)
			{
				if(WSA_IO_PENDING==WSAGetLastError())
				{
					h.unlock();
					HANDLE hs[2]; hs[0]=p->olw.hEvent; hs[1]=QThread::int_cancelWaiterHandle();
					DWORD ret=WaitForMultipleObjects(2, hs, FALSE, INFINITE);
					h.relock();
					if(WAIT_OBJECT_0+1==ret)
					{	// There appears to be no way to cancel overlapping i/o on sockets, so here's best attempt
						FXERRHSKT(::shutdown(p->handle, SD_BOTH));
						FXERRHSKT(::closesocket(p->handle));
						p->handle=0;
						p->connected=false;
						h.unlock();
						QThread::current()->checkForTerminate();
					}
					else if(WAIT_OBJECT_0!=ret)
					{ FXERRHSKT(ret); }
					else if(!WSAGetOverlappedResult(p->handle, &p->olw, &_written, FALSE, &flags))
					{ FXERRHSKT(SOCKET_ERROR); }
				}
				else { FXERRHSKT(ret); }
			}
			written=(FXuval) _written;
		}
#endif
#ifdef USE_POSIX
		QIODeviceS_SignalHandler::lockWrite();
		h.unlock();
		if(Stream==p->type)
		{
#ifdef __linux__
			// send() is a cancellable point on Linux
			written=::send(p->handle, data, maxlen, MSG_NOSIGNAL);
#else
			// send() isn't a cancellable point on all platforms, so use write()
			written=::write(p->handle, data, maxlen);
#endif
		}
		else if(Datagram==p->type)
		{
			sockaddr_in6 sa6={0};
			int salen;
			sockaddr *sa=makeSockAddr(salen, sa6, addr, port);
			written=::sendto(p->handle, data, maxlen,
#ifdef __linux__
							 MSG_NOSIGNAL,
#else
							 0,
#endif
							 sa, salen);
		}
		h.relock();
		FXERRHSKT(written);
		if(QIODeviceS_SignalHandler::unlockWrite())		// Nasty this
			FXERRGCONLOST("Broken socket", 0);
#endif
		if(isRaw()) flush();
		return written;
	}
	return 0;
}

int QBlkSocket::ungetch(int c)
{
	return -1;
}

#if 0
	for(;;)
	{	// Most annoyingly WaitForMultipleObjects doesn't register accept pending as signalled :(
		struct timeval tv;
		tv.tv_sec=0;
		tv.tv_usec=100*1000;	// tenth second poll
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(p->handle, &fds);
		int ret=::select(p->handle+1, &fds, 0, 0, &tv); 
		if(ret) break;
		if(WAIT_OBJECT_0==WaitForSingleObject(QThread::int_cancelWaiterHandle(), 0))
		{	// There appears to be no way to cancel overlapping i/o on sockets, so here's best attempt
			h.relock();
			FXERRHSKT(::shutdown(p->handle, SD_BOTH));
			FXERRHSKT(::closesocket(p->handle));
			p->handle=0;
			p->connected=false;
			h.unlock();
			QThread::current()->checkForTerminate();
		}
	}
#endif

QBlkSocket *QBlkSocket::waitForConnection(FXuint waitfor)
{
	QMtxHold h(p);
	FXERRH(isOpen(), "Server socket isn't open", QBLKSOCKET_NOTOPEN, 0);
	FXERRH(Stream==p->type, "Only available for stream sockets", QBLKSOCKET_NOTSTREAM, 0);
	sockaddr_in6 sa6={0}, *sa6addr; sa6addr=&sa6;
	socklen_t salen=sizeof(sa6);
#ifdef USE_WINAPI
	// This is made considerably harder by AcceptEx() being too low level :(
	SOCKET newskt;
	FXERRHSKT(newskt=::socket(p->mine.addr.isIp6Addr() ? PF_INET6 : PF_INET, (p->type==Datagram) ? SOCK_DGRAM : SOCK_STREAM, 0));
	FXRBOp unnewskt=FXRBFunc(::closesocket, newskt);
	const int MaxSockAddr=16+sizeof(sockaddr_in6);
	char buffer[2*MaxSockAddr];
	DWORD recd=0, flags=0;
	if(!AcceptEx(p->handle, newskt, buffer, 0, 16+MaxSockAddr, 16+MaxSockAddr, &recd, &p->olr))
	{
		if(WSA_IO_PENDING==WSAGetLastError())
		{
			h.unlock();
			HANDLE hs[2]; hs[0]=p->olr.hEvent; hs[1]=QThread::int_cancelWaiterHandle();
			DWORD ret=WaitForMultipleObjects(2, hs, FALSE, (FXINFINITE==waitfor) ? INFINITE : waitfor);
			h.relock();
			if(WAIT_OBJECT_0+1==ret)
			{	// There appears to be no way to cancel overlapping i/o on sockets, so here's best attempt
				::closesocket(newskt);
				unnewskt.dismiss();
				FXERRHSKT(::closesocket(p->handle));
				p->handle=0;
				p->connected=false;
				h.unlock();
				QThread::current()->checkForTerminate();
			}
			else if(WAIT_TIMEOUT==ret)
				return 0;
			else if(WAIT_OBJECT_0!=ret)
			{ FXERRHSKT(ret); }
			else if(!(ret=WSAGetOverlappedResult(p->handle, &p->olr, &recd, FALSE, &flags)))
			{ FXERRHSKT(ret); }
		}
		else { FXERRHSKT(0); }
	}
	{
		SOCKADDR *foo=0;
		socklen_t foolen=sizeof(sa6);
		GetAcceptExSockaddrs(buffer, 0, 16+MaxSockAddr, 16+MaxSockAddr, &foo, &foolen, (SOCKADDR **) &sa6addr, &salen);
	}
	FXERRHSKT(setsockopt(newskt, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&p->handle, sizeof(p->handle)));
	FXAutoPtr<QBlkSocket> ret;
	FXERRHM(ret=new QBlkSocket(*this, (int) newskt));
	unnewskt.dismiss();
	if(isReadable())
	{
		FXERRHWIN(INVALID_HANDLE_VALUE!=(ret->p->olr.hEvent=CreateEvent(NULL, TRUE, FALSE, NULL)));
	}
	if(isWriteable())
	{
		FXERRHWIN(INVALID_HANDLE_VALUE!=(ret->p->olw.hEvent=CreateEvent(NULL, TRUE, FALSE, NULL)));
	}
#endif
#ifdef USE_POSIX
	h.unlock();
	struct ::timeval *tv=0, _tv;
	if(waitfor!=FXINFINITE)
	{
		_tv.tv_sec=waitfor/1000;
		_tv.tv_usec=(waitfor % 1000)*1000;
		tv=&_tv;
	}
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(p->handle, &fds);
	if(!tnfxselect(p->handle+1, &fds, 0, 0, tv)) return 0;
	int newskt=::accept(p->handle, (sockaddr *) sa6addr, &salen);
	h.relock();
	FXERRHSKT(newskt);
	FXAutoPtr<QBlkSocket> ret;
	FXERRHM(ret=new QBlkSocket(*this, newskt));
	ret->setupSocket();
#endif
	readSockAddr(ret->p->peer.addr, ret->p->peer.port, sa6addr);
	ret->p->connected=true;
	return PtrRelease(ret);
}

FXuval QBlkSocket::waitForMore(int msecs, bool *timeout)
{
	QMtxHold h(p);
	if(!QIODevice::isReadable()) FXERRGIO(QTrans::tr("QBlkSocket", "Not open for reading"));
	if(isOpen())
	{
		QThread_DTHold dth;
		struct ::timeval tv;
		tv.tv_sec=msecs/1000;
		tv.tv_usec=(msecs % 1000)*1000;
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(p->handle, &fds);
		h.unlock();
		int ret=tnfxselect((int) p->handle+1, &fds, 0, 0, &tv); 
		h.relock();
		if(timeout) *timeout=(!ret) ? true : false;
		return (FXuval) size();
	}
	return 0;
}

//**********************************************************************************************

bool FXNetwork::hasIPv6()
{
	return IPv6==socketsEnabled;
}

FXString FXNetwork::hostname()
{
	char buffer[1024];
	FXERRHSKT(::gethostname(buffer, sizeof(buffer)));
	return buffer;
}

QHostAddress FXNetwork::dnsLookup(const FXString &name)
{
	QHostAddress ret;
	FXushort port;
	QThread_DTHold dth;
#ifdef USE_WINAPI
	hostent *he=::gethostbyname(name.text());
	if(!he) return ret;
	sockaddr_in sa={0};
	sa.sin_family=PF_INET;
	sa.sin_addr.s_addr=*(FXuint *) he->h_addr_list[0];
	readSockAddr(ret, port, (sockaddr_in6 *) &sa);
#endif
#if USE_POSIX
	// MS Platform SDK 2003 has the right header files and links fine, but the DLL is missing the code :(
	addrinfo *res=0;
	int errcode=::getaddrinfo(name.text(), NULL, NULL, &res);
	if(EAI_NONAME==errcode
#ifdef EAI_NODATA
		// EAI_NODATA has been obsoleted on newer platforms
		|| EAI_NODATA==errcode
#endif
		) return ret;
	if(errcode)
	{
		FXERRGIO(FXString(gai_strerror(errcode)));
	}
	if(res)
	{
		addrinfo *a=0;
		for(a=res; a; (a=a->ai_next))
		{
			if(PF_INET6==a->ai_family) break;
		}
		if(!a)
		{
			for(a=res; a; (a=a->ai_next))
			{
				if(PF_INET==a->ai_family) break;
			}
		}
		if(a) readSockAddr(ret, port, (sockaddr_in6 *) a->ai_addr);
	}
	freeaddrinfo(res);
#endif
	return ret;
}

FXString FXNetwork::dnsReverseLookup(const QHostAddress &addr)
{
	QThread_DTHold dth;
#ifdef USE_WINAPI
	FXuint ip4=htonl(addr.ip4Addr());
	hostent *he=::gethostbyaddr((const char *) &ip4, sizeof(ip4), PF_INET);
	if(!he) return FXString();
	return FXString(he->h_name);
#endif
#if USE_POSIX
	// MS Platform SDK 2003 has the right header files and links fine, but the DLL is missing the code :(
	sockaddr_in6 sa6={0};
	int salen;
	sockaddr *sa=makeSockAddr(salen, sa6, addr, 0);
	char buffer[NI_MAXHOST];
	int errcode=::getnameinfo(sa, salen, buffer, sizeof(buffer), NULL, 0, NI_NAMEREQD);
	if(errcode)
	{
#ifdef USE_WINAPI
		if(WSATRY_AGAIN==WSAGetLastError() || WSAHOST_NOT_FOUND==WSAGetLastError()) return FXString();
#endif
#ifdef USE_POSIX
		if(EAI_AGAIN==errcode || EAI_NONAME==errcode) return FXString();
#endif
		FXERRGIO(FXString(gai_strerror(errcode)));
	}
	return FXString(buffer);
#endif
}

}
