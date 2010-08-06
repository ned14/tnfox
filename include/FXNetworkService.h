/********************************************************************************
*                                                                               *
*                                Network Services                               *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2009 by Niall Douglas.   All Rights Reserved.            *
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

#ifndef FXNETWORKSERVICE_H
#define FXNETWORKSERVICE_H

#include "FXTime.h"
#include "QThread.h"
#include "FXStream.h"
#include "QBlkSocket.h"
#include <qvaluelist.h>

namespace FX {

/*! \file FXNetworkService.h
\brief Defines classes used in providing network services
*/

/*! \struct FXNetworkServiceClient
\ingroup IPC
\brief A network service client record
*/
struct FXNetworkServiceClient
{
	FXTime firstSeen;			//!< The first time this client was seen
	FXTime lastSeen;			//!< The most recent time this client was seen
	FXTime bannedUntil;			//!< Until when this client will be banned
	FXuint refusedCount;		//!< Number of times this client has been refused
	QValueList<QThreadPool::handle> threadhs;	//!< Threadpool threads currently working with this IP
	void *data;					//!< Third party data pointer
	FXNetworkServiceClient() : data(0) { }
};
/*! \class FXNetworkService
\ingroup IPC
\brief Provides a scalable resilient network service

FXNetworkService is designed to operate a front-end network service interface which
is capable of scaling excellently with load and providing a few of the more common
anti-DDoS measures and of course hooks to add your own custom anti-DDoS measures. It
does not directly support anything other than a FX::QBlkSocket as you should always try
to avoid using SSL on a public facing port as it greatly increases your susceptibility
to server CPU exhaustion (you can of course always subsequently negotiate a SSL
connection after you have verified identity - this is what Tn does). Of course, you
can hack in a FX::QSSLDevice with a little bit of work if you want to.

Ban masks are supported whereby entire IP classes can be banned, as well as client
throttling, per-IP record keeping (based on a LRU cache which will self-adjust
according to free memory) and per-IP attempt throttling. FXNetworkService basically
takes away most of the pain and drudgery of dealing with potentially hostile external
networks and lets you get on with the service implementation.

The \em acceptExternalClients constructor parameter when false simply adds a ban mask
on all non-localhost addresses. Ban masks work through the AND and XOR technique
whereby an incoming IP address is ANDed with the mask and then XORed with the XOR,
and if the remaining value is zero then the IP is accepted. FX::Maths::Vector<> is
used so SIMD will be used on machines supporting it to perform the bit operations.
*/
struct FXNetworkServicePrivate;
class FXAPIR FXNetworkService : protected QMutex, public QThread
{
	FXNetworkServicePrivate *p;
	FXNetworkService(const FXNetworkService &);
	FXNetworkService &operator=(const FXNetworkService &);
public:
	//! Actions to be taken
	enum Action
	{
		ACCEPTED=0,		//!< This client is being accepted
		REFUSE,			//!< This client is to have its connection cut
		BAN,			//!< Add this client to the banned list
		DELETERECORD	//!< Destroy this client's record
	};
	//! The API of the new client function
	typedef Generic::Functor<Generic::TL::create<Action, FXNetworkService *, QBlkSocket *>::value> NewClientSpec;
	//! Constructs an instance monitoring \em serversocket, dispatching \em newclientv to threads in \em dispatch with maximum clients=0 being the number of threads in the thread pool.
	FXNetworkService(QBlkSocket *serversocket, NewClientSpec newclientv, QThreadPool *dispatch=0, FXuint lrucachesize=1000, FXuint maxclients=0, bool acceptExternalClients=true, const char *threadname="Network Service Monitor Thread");
	~FXNetworkService();

	//! Returns the server socket
	QBlkSocket *serverSocket() const throw();
	//! Sets the server socket
	void setServerSocket(QBlkSocket *s);
	//! Returns the thread pool used for dispatch
	QThreadPool *dispatchPool() const throw();
	//! Sets the thread pool used for dispatch
	void setDispatchPool(QThreadPool *pool);
	//! Returns the maximum number of clients permitted
	FXuint maxClients() const throw();
	//! Sets the maximum number of clients permitted (use 0 for same as thread pool, use FXINFINITE for infinite)
	void setMaxClients(FXuint no=0);
	//! Returns the maximum number of clients per IP address permitted (default is 8)
	FXuint maxClientsPerIP() const throw();
	//! Sets the maximum number of clients per IP address permitted
	void setMaxClientsPerIP(FXuint no=8);
	//! Returns the maximum number of client attempts per minute allowed before banning (default is 1.0)
	float maxClientAttemptsPerMinute() const throw();
	//! Sets the maximum number of client attempts per minute allowed before banning
	void setMaxClientAttemptsPerMinute(float max=1.0);
	//! Returns the ban period (default is one hour)
	FXTime banPeriod() const;
	//! Sets the ban period
	void setBanPeriod(const FXTime &period=FXTime(FXTime::micsPerHour));
	//! Banned IP mask
	struct IPMask { QHostAddress mask, XOR; IPMask(const QHostAddress &_mask, const QHostAddress &_xor) : mask(_mask), XOR(_xor) { } };
	//! Returns a list of banned IP \b masks
	QMemArray<IPMask> bannedIPMasks() const;
	//! Sets a list of banned IP \b masks
	void setBannedIPMasks(const QMemArray<IPMask> &list);
	//! Returns a list of IP client records
	QHostAddressDict<FXNetworkServiceClient> IPClientRecords() const;
	//! Returns the client record for the specified IP
	FXNetworkServiceClient IPClientRecord(const QHostAddress &a) const;
protected:
	//! The raw LRU cache of IP records. Make SURE you lock the mutex before use!
	FXLRUCache< QHostAddressDict<FXNetworkServiceClient> > &rawIPClientRecords();
	//! Overload this to replace or extend new client processing. Lock is already held on entry.
	virtual QThreadPool::handle newClientUpcall(FXNetworkService *service, QBlkSocket *skt, FXNetworkServiceClient &record, Action &action, NewClientSpec &newclientv);

	virtual void run();
	virtual void *cleanup();
};


} // namespace

#endif
