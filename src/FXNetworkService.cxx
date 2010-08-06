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

#include "FXNetworkService.h"
#include "FXLRUCache.h"
#include "FXMaths.h"
#include "FXRollback.h"
#include "FXPtrHold.h"
#include "qmemarray.h"
#include "FXMemDbg.h"
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

	//QDICTDYNRESIZE(dict)

struct FXNetworkServicePrivate
{
	QBlkSocket *serversocket;
	FXNetworkService::NewClientSpec newclientv;
	QThreadPool *dispatchpool;
	FXuint maxclients, maxclientsperip;
	float maxattemptspermin;
	FXTime banperiod;
	QMemArray<FXNetworkService::IPMask> banmasks;
	FXLRUCache<QHostAddressDict<FXNetworkServiceClient> > clients;
	FXNetworkServicePrivate(QBlkSocket *_serversocket, FXNetworkService::NewClientSpec _newclientv, QThreadPool *_dispatch, FXuint lrucachesize, FXuint _maxclients)
		: serversocket(_serversocket), newclientv(std::move(_newclientv)), dispatchpool(_dispatch), maxclients(_maxclients), maxclientsperip(8),
		maxattemptspermin(1), banperiod(FXTime::micsPerHour), clients(lrucachesize, 1, true) { }
};

FXNetworkService::FXNetworkService(QBlkSocket *serversocket, FXNetworkService::NewClientSpec newclientv, QThreadPool *dispatch,
								   FXuint lrucachesize, FXuint maxclients, bool acceptExternalClients, const char *threadname)
: QThread(threadname), p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new FXNetworkServicePrivate(serversocket, std::move(newclientv), dispatch, lrucachesize, maxclients));
	if(!acceptExternalClients)
	{
		QHostAddress mask, XOR(QHOSTADDRESS_LOCALHOST);
		p->banmasks.push_back(IPMask(mask, XOR));
	}
	unconstr.dismiss();
}

FXNetworkService::~FXNetworkService()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

QBlkSocket *FXNetworkService::serverSocket() const throw()
{
	return p->serversocket;
}

void FXNetworkService::setServerSocket(QBlkSocket *s)
{
	QMtxHold h(this);
	p->serversocket=s;
}

QThreadPool *FXNetworkService::dispatchPool() const throw()
{
	return p->dispatchpool;
}

void FXNetworkService::setDispatchPool(QThreadPool *pool)
{
	QMtxHold h(this);
	p->dispatchpool=pool;
}

FXuint FXNetworkService::maxClients() const throw()
{
	return p->maxclients;
}

void FXNetworkService::setMaxClients(FXuint no)
{
	QMtxHold h(this);
	p->maxclients=no;
}

FXuint FXNetworkService::maxClientsPerIP() const throw()
{
	return p->maxclientsperip;
}

void FXNetworkService::setMaxClientsPerIP(FXuint no)
{
	QMtxHold h(this);
	p->maxclientsperip=no;
}

float FXNetworkService::maxClientAttemptsPerMinute() const throw()
{
	return p->maxattemptspermin;
}

void FXNetworkService::setMaxClientAttemptsPerMinute(float max)
{
	QMtxHold h(this);
	p->maxattemptspermin=max;
}

FXTime FXNetworkService::banPeriod() const
{
	QMtxHold h(this);
	return p->banperiod;
}

void FXNetworkService::setBanPeriod(const FXTime &period)
{
	QMtxHold h(this);
	p->banperiod=period;
}

QMemArray<FXNetworkService::IPMask> FXNetworkService::bannedIPMasks() const
{
	QMtxHold h(this);
	return p->banmasks;
}

void FXNetworkService::setBannedIPMasks(const QMemArray<FXNetworkService::IPMask> &list)
{
	QMtxHold h(this);
	p->banmasks=list;
}

QHostAddressDict<FXNetworkServiceClient> FXNetworkService::IPClientRecords() const
{
	QMtxHold h(this);
	return QHostAddressDict<FXNetworkServiceClient>((const QHostAddressDict<FXNetworkServiceClient> &) p->clients);
}

FXNetworkServiceClient FXNetworkService::IPClientRecord(const QHostAddress &a) const
{
	QMtxHold h(this);
	FXNetworkServiceClient *ret=p->clients[a];
	return ret ? *ret : FXNetworkServiceClient();
}

FXLRUCache< QHostAddressDict<FXNetworkServiceClient> > &FXNetworkService::rawIPClientRecords()
{
	return p->clients;
}

QThreadPool::handle FXNetworkService::newClientUpcall(FXNetworkService *service, QBlkSocket *skt, FXNetworkServiceClient &record, Action &action, NewClientSpec &newclientv)
{
	action=REFUSE;
	return 0;
}

void FXNetworkService::run()
{
	QMtxHold h(this);
	FXPtrHold<QBlkSocket> newsocket;
	for(;;)
	{
		Action action=ACCEPTED;
		h.unlock();
		newsocket=p->serversocket->waitForConnection();
		h.relock();
		FXTime now=FXTime::now();
		QHostAddress sktIP=newsocket->peerAddress();
#ifdef DEBUG
		fxmessage("FXNetworkService: Received connection to %s:%u from %s:%u\n",
			newsocket->requestedAddress().toString().text(), newsocket->requestedPort(),
			newsocket->peerAddress().toString().text(), newsocket->peerPort());
#endif
		FXNetworkServiceClient *client=p->clients.find(sktIP);
		if(!client)
		{
			FXERRHM(client=new FXNetworkServiceClient);
			FXRBOp unclient=FXRBNew(client);
			client->firstSeen=client->lastSeen=now;
			p->clients.insert(sktIP, client);
			unclient.dismiss();
		}
		if(client->bannedUntil.value && client->bannedUntil<now)
			action=BAN;
		if(!action)
		{	// Mask and XOR with ban list
			for(QMemArray<FXNetworkService::IPMask>::const_iterator it=p->banmasks.begin(); it!=p->banmasks.end(); ++it)
			{
				QHostAddress t=(sktIP & it->mask)^it->XOR;
				if(!!t) { action=BAN; break; }
			}
		}
		// Take DDoS measures
		if(!action && client->refusedCount/((float)(now.value-client->firstSeen.value)/FXTime::micsPerMinute)>=p->maxattemptspermin)
			action=BAN;
		if(!action && client->threadhs.count()>=p->maxclientsperip)
			action=REFUSE;
		QThreadPool::handle threadh=newClientUpcall(this, newsocket, *client, action, p->newclientv);
		if(threadh) client->threadhs.push_back(threadh);
		client->lastSeen=now;
#ifdef DEBUG
		fxmessage("FXNetworkService: Action on %s:%u is %d\n",
			newsocket->peerAddress().toString().text(), newsocket->peerPort(), action);
#endif
		if(action)
		{
			client->refusedCount++;
			if(BAN==action) client->bannedUntil=now+p->banperiod;
			newsocket->close();
			delete static_cast<QBlkSocket *>(newsocket);
		}
		newsocket=0;
		if(DELETERECORD==action)
			p->clients.remove(sktIP);
	}
}

void *FXNetworkService::cleanup()
{
	return 0;
}

} // namespace
