/********************************************************************************
*                                                                               *
*                        Inter Process Communication                            *
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

#include "FXIPC.h"
#include "QTrans.h"
#include "FXRollback.h"
#include "QBuffer.h"
#include "FXProcess.h"
#include "QGZipDevice.h"
#include "QPipe.h"
#include "FXErrCodes.h"
#include <qintdict.h>
#include <qptrvector.h>
#include <qcstring.h>
#include <qptrlist.h>
#include <assert.h>
#include "FXMemDbg.h"
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

struct FXDLLLOCAL FXIPCMsgRegistryPrivate : public QMutex
{
	struct Entry
	{
		FXuint code;
		FXIPCMsgRegistry::deendianiseSpec deendianise;
		FXIPCMsgRegistry::makeMsgSpec makeMsg;
		FXIPCMsgRegistry::delMsgSpec delMsg;
		Generic::typeInfoBase &ti;
		Entry(FXuint _code, FXIPCMsgRegistry::deendianiseSpec _deendianise, FXIPCMsgRegistry::makeMsgSpec _makeMsg, FXIPCMsgRegistry::delMsgSpec _delMsg, Generic::typeInfoBase &_ti)
			: code(_code), deendianise(_deendianise), makeMsg(_makeMsg), delMsg(_delMsg), ti(_ti) { }
	};
	QIntDict<Entry> msgs;
	FXIPCMsgChunkStandard *stdchunk;
	FXIPCMsgRegistryPrivate() : msgs(13, true), stdchunk(0) { }
};

FXIPCMsgRegistry::FXIPCMsgRegistry() : p(0)
{
	FXRBOp unconstruct=FXRBConstruct(this);
	FXERRHM(p=new FXIPCMsgRegistryPrivate);
	// Add the standard chunk
	FXERRHM(p->stdchunk=new FXIPCMsgChunkStandard(this));
	magic=*(FXuint *)"PCMR";
	unconstruct.dismiss();
}

FXIPCMsgRegistry::~FXIPCMsgRegistry()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p->stdchunk);
	magic=0;
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

void FXIPCMsgRegistry::int_register(FXuint code, FXIPCMsgRegistry::deendianiseSpec deendianise, FXIPCMsgRegistry::makeMsgSpec makeMsg, FXIPCMsgRegistry::delMsgSpec delMsg, Generic::typeInfoBase &ti)
{
	QMtxHold h(p);
	FXERRH(p->msgs.find(code)==0, QTrans::tr("FXIPCMsgRegistry", "Message already registered in this registry"), FXIPCMSGREGISTRY_MSGALREADYREGED, 0);
	FXIPCMsgRegistryPrivate::Entry *e;
	FXERRHM(e=new FXIPCMsgRegistryPrivate::Entry(code, deendianise, makeMsg, delMsg, ti));
	FXRBOp unnew=FXRBNew(e);
	p->msgs.insert(code, e);
	unnew.dismiss();
	if((p->msgs.count()*3)/2>p->msgs.size())
		p->msgs.resize(fx2powerprimes(p->msgs.size()*2)[0]);
}

void FXIPCMsgRegistry::int_deregister(FXuint code)
{
	assert(isValid());
	QMtxHold h(p);
	assert(p->msgs.find(code));
	p->msgs.remove(code);
}

bool FXIPCMsgRegistry::lookup(FXuint code) const
{
	QMtxHold h(p);
	return p->msgs.find(code)!=0;
}

bool FXIPCMsgRegistry::lookup(FXIPCMsgRegistry::deendianiseSpec &deendianise, FXIPCMsgRegistry::makeMsgSpec &makeMsg, FXIPCMsgRegistry::delMsgSpec &delMsg, FXuint code) const
{
	QMtxHold h(p);
	FXIPCMsgRegistryPrivate::Entry *e=p->msgs.find(code);
	if(!e)
	{
		deendianise=0; makeMsg=0; delMsg=0;
		return false;
	}
	deendianise=e->deendianise;
	makeMsg=e->makeMsg;
	delMsg=e->delMsg;
	return true;
}

const FXString &FXIPCMsgRegistry::decodeType(FXuint code) const
{
	QMtxHold h(p);
	FXIPCMsgRegistryPrivate::Entry *e=p->msgs.find(code);
	if(e)
		return e->ti.name();
	else
	{
		static FXString unknown("?");
		return unknown;
	}
}

//*******************************************************************************************

static FXuint pageSize=FXProcess::pageSize();
struct FXDLLLOCAL FXIPCChannelPrivate
{
	FXIPCMsgRegistry *registry;
	QIODeviceS *dev;
	bool unreliable, compressed, errorTrans, quit, noquitmsg, peerUntrusted, printstats;
	FXIPCChannel::EndianConversionKinds endianConversion;
	FXuint maxMsgSize, garbageMessageCount, sendMsgSize;
	QBuffer buffer;
	QGZipDevice *compressedbuffer;
	FXStream endianiser;
	QPtrList<QWaitCondition> wcsFree;
	struct AckEntry
	{
		FXIPCMsg *msg, *ack;
		QWaitCondition *wc;
		FXuint timesent;
		FXException *erroroccurred;
		AckEntry(FXIPCMsg *_msg, FXIPCMsg *_ack, QWaitCondition *_wc)
			: msg(_msg), ack(_ack), wc(_wc), timesent(FXProcess::getMsCount()), erroroccurred(0) { }
		~AckEntry() { FXDELETE(wc); }
	};
	QIntDict<AckEntry> msgs;
	FXuint msgidcount;
	FXulong monitorThreadId;
	QPtrVector<FXIPCChannel::MsgFilterSpec> premsgfilters;
	QThreadPool *threadPool;
	QPtrList<Generic::BoundFunctorV> msgHandlings;
	FXIPCChannelPrivate(FXIPCMsgRegistry *_registry, QIODeviceS *_dev, bool _peerUntrusted, QThreadPool *_threadPool)
		: registry(_registry), dev(_dev), unreliable(false), compressed(false), errorTrans(true),
		quit(false), noquitmsg(false), peerUntrusted(_peerUntrusted), printstats(false), endianConversion(FXIPCChannel::AutoEndian),
		maxMsgSize(65536), garbageMessageCount(0), sendMsgSize(pageSize), buffer(pageSize), compressedbuffer(0), endianiser(&buffer),
		wcsFree(true), msgs(1, true), msgidcount(0), monitorThreadId(0), premsgfilters(true), threadPool(_threadPool), msgHandlings(true)
	{
		buffer.open(IO_ReadWrite);
//#ifdef DEBUG
//		printstats=true;
//#endif
	}
};

FXIPCChannel::FXIPCChannel(FXIPCMsgRegistry &registry, QIODeviceS *dev, bool peerUntrusted, QThreadPool *threadPool, const char *threadname)
	: QThread(threadname, false, 128*1024, QThread::InProcess), p(0)
{
	FXERRHM(p=new FXIPCChannelPrivate(&registry, dev, peerUntrusted, threadPool));
}
FXIPCChannel::~FXIPCChannel()
{ FXEXCEPTIONDESTRUCT1 {
	if(running())
	{
		requestTermination();
		wait();
	}
	QMtxHold h(this);
	Generic::BoundFunctorV *callv;
	for(QPtrListIterator<Generic::BoundFunctorV> it(p->msgHandlings); (callv=it.current());)
	{
		if(QThreadPool::Cancelled==p->threadPool->cancel(callv, false))
		{
			QPtrListIterator<Generic::BoundFunctorV> it2=it;
			++it;
			p->msgHandlings.removeByIter(it2);
		}
		else ++it;
	}
	h.unlock();
	while(!p->msgHandlings.isEmpty()) QThread::yield();
	FXDELETE(p->compressedbuffer);
	assert(p->msgs.isEmpty());
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }
FXIPCMsgRegistry &FXIPCChannel::registry() const
{
	// QMtxHold h(this); can do without
	return *p->registry;
}
void FXIPCChannel::setRegistry(FXIPCMsgRegistry &registry)
{
	// QMtxHold h(this); can do without
	p->registry=&registry;
}
QIODeviceS *FXIPCChannel::device() const
{
	// QMtxHold h(this); can do without
	return p->dev;
}
void FXIPCChannel::setDevice(QIODeviceS *dev)
{
	// QMtxHold h(this); can do without
	p->dev=dev;
}
QThreadPool *FXIPCChannel::threadPool() const
{
	// QMtxHold h(this); can do without
	return p->threadPool;
}
void FXIPCChannel::setThreadPool(QThreadPool *threadPool)
{
	// QMtxHold h(this); can do without
	p->threadPool=threadPool;
}
bool FXIPCChannel::unreliable() const
{
	// QMtxHold h(this); can do without
	return p->unreliable;
}
void FXIPCChannel::setUnreliable(bool v)
{
	// QMtxHold h(this); can do without
	p->unreliable=v;
}
bool FXIPCChannel::compression() const
{
	// QMtxHold h(this); can do without
	return p->compressed;
}
void FXIPCChannel::setCompression(bool v)
{
	// QMtxHold h(this); can do without
	p->compressed=v;
}
bool FXIPCChannel::active() const
{
	// QMtxHold h(this); can do without
	return !p->quit;
}
void FXIPCChannel::reset()
{
	QMtxHold h(this);
	p->quit=false; p->noquitmsg=false;
	p->monitorThreadId=0;
}
FXIPCChannel::EndianConversionKinds FXIPCChannel::endianConversion() const
{
	// QMtxHold h(this); can do without
	return p->endianConversion;
}
void FXIPCChannel::setEndianConversion(FXIPCChannel::EndianConversionKinds kind)
{
	QMtxHold h(this);
	p->endianConversion=kind;
	if(AlwaysLittleEndian==kind)
		p->endianiser.setByteOrder(FXStream::LittleEndian);
	else if(AlwaysBigEndian==kind)
		p->endianiser.setByteOrder(FXStream::BigEndian);
}
bool FXIPCChannel::errorTranslation() const
{
	QMtxHold h(this);
	return p->errorTrans;
}
void FXIPCChannel::setErrorTranslation(bool v)
{
	QMtxHold h(this);
	p->errorTrans=v;
}
bool FXIPCChannel::peerUntrusted() const
{
	QMtxHold h(this);
	return p->peerUntrusted;
}
void FXIPCChannel::setPeerUntrusted(bool v)
{
	QMtxHold h(this);
	p->peerUntrusted=v;
}
FXuint FXIPCChannel::maxMsgSize() const
{
	QMtxHold h(this);
	return p->maxMsgSize;
}
void FXIPCChannel::setMaxMsgSize(FXuint newsize)
{
	QMtxHold h(this);
	p->maxMsgSize=newsize;
}
FXuint FXIPCChannel::garbageMessageCount() const
{
	QMtxHold h(this);
	return p->garbageMessageCount;
}
void FXIPCChannel::setGarbageMessageCount(FXuint newsize)
{
	QMtxHold h(this);
	p->garbageMessageCount=newsize;
}
void FXIPCChannel::setPrintStatistics(bool v)
{
	QMtxHold h(this);
	p->printstats=v;
}
void FXIPCChannel::requestClose()
{
	if(!p->noquitmsg && p->dev)
	{
		FXIPCMsg_Disconnect byebye;
		sendMsg(byebye);
	}
	forceClose();	// must be after the above!
}

bool FXIPCChannel::doReception(FXuint waitfor)
{
	if(p->quit) return false;
	if(!waitfor && p->dev->atEnd()) return false;
	FXERRH_TRY
	{
		QByteArray &data=p->buffer.buffer();
		FXStream endianiser(&p->buffer);
		endianiser.setByteOrder(p->endianiser.byteOrder());
		FXuval read=0;
#ifdef DEBUG
		memset(data.data(), 0, data.size());
#endif
		// The thread cancellable portion
		while(read<FXIPCMsg::minHeaderLength)
		{
			FXuval justread=0;
			if(waitfor!=FXINFINITE && p->dev->atEnd())
			{
				if(!QIODeviceS::waitForData(0, 1, &p->dev, waitfor)) return false;
			}
			read+=(justread=p->dev->readBlock(data.data()+read, data.size()-read));
			if(!justread) return true;
		}
		//fxmessage("Thread %u channel read=%d bytes\n", (FXuint) QThread::id(), read);
		{
			QThread_DTHold dthold;
			FXIPCMsg tmsg(0);
			QMtxHold h(this);
			if(!p->monitorThreadId) p->monitorThreadId=QThread::id();
			while(!p->quit)
			{
#ifdef DEBUG
				memset(&tmsg, 0, sizeof(FXIPCMsg));
#endif
				while(read<FXIPCMsg::minHeaderLength)
				{
					FXuval justread=0;
					if(waitfor!=FXINFINITE && p->dev->atEnd())
					{
						if(!QIODeviceS::waitForData(0, 1, &p->dev, waitfor)) return false;
					}
					read+=(justread=p->dev->readBlock(data.data()+read, data.size()-read));
					if(!justread) return true;
				}
				p->buffer.at(0);
				if(AutoEndian==p->endianConversion)		// Receiver translates
					endianiser.setByteOrder((data.data()[17] & FXIPCMsg::FlagsIsBigEndian) ? FXStream::BigEndian : FXStream::LittleEndian);
				tmsg.read(endianiser);
				assert(tmsg.length()>=FXIPCMsg::minHeaderLength);
				assert(tmsg.msgType());
				if(p->maxMsgSize && tmsg.length()>p->maxMsgSize)
				{
					FXERRG(QTrans::tr("FXIPCChannel", "Maximum message size exceeded (%1 with limit of %2)").arg(tmsg.length()).arg(p->maxMsgSize), FXIPCCHANNEL_MSGTOOBIG, 0);
				}
				if(p->buffer.size()<tmsg.length())
					p->buffer.truncate(tmsg.length());
				FXint togo=(FXint)(tmsg.length()-read);
				while(togo>0)
				{
					h.unlock();
					if(waitfor!=FXINFINITE && p->dev->atEnd())
					{
						if(!QIODeviceS::waitForData(0, 1, &p->dev, waitfor)) return false;
					}
					read+=p->dev->readBlock(data.data()+read, togo);
					//fxmessage("Thread %u channel read=%d bytes\n", (FXuint) QThread::id(), read);
					h.relock();
					togo=(FXint)(tmsg.length()-read);
				}
				if(tmsg.crc)
				{
					FXuint crc=fxadler32(1, data.data()+2*sizeof(FXuint), tmsg.length()-2*sizeof(FXuint));
					FXERRH(crc==tmsg.crc, QTrans::tr("FXIPCChannel", "Message failed CRC check"), FXIPCCHANNEL_BADMESSAGE, 0);
				}
				//fxmessage("IPCRead %s", fxdump8(data.data(), tmsg.length()).text());
				FXIPCMsgRegistry::deendianiseSpec deendianise;
				FXIPCMsgRegistry::makeMsgSpec makeMsg;
				FXIPCMsgRegistry::delMsgSpec delMsg;
				if(p->registry->lookup(deendianise, makeMsg, delMsg, tmsg.msgType()))
				{
					FXIPCMsg *msg=0;
					bool msgdelpls=false;
					FXIPCChannelPrivate::AckEntry *ae=tmsg.hasAck() ? 0 : p->msgs.find(tmsg.msgId());
					if(ae)
					{
						assert(ae->ack);
						if(ae->msg->msgType()+1!=tmsg.msgType())
						{
							if(tmsg.msgType()>FXIPCMsg_ErrorOccurred::id::code)
							{
#ifdef DEBUG
								fxmessage("WARNING: Thread %u received ack 0x%x (%s) with ack id %u which doesn't match msg 0x%x (%s)!\n",
									(FXuint) QThread::id(), tmsg.msgType(), p->registry->decodeType(tmsg.msgType()).text(), tmsg.msgId(),
									ae->msg->msgType(), p->registry->decodeType(ae->msg->msgType()).text());
#if 1
								QValueList<std::pair<FXuint, FXIPCChannelPrivate::AckEntry *> > acks;
								FXIPCChannelPrivate::AckEntry *ae2;
								for(QIntDictIterator<FXIPCChannelPrivate::AckEntry> it(p->msgs); (ae2=it.current()); ++it)
									acks.push_back(std::make_pair(it.currentKey(), ae2));
								acks.sort();
								for(QValueList<std::pair<FXuint, FXIPCChannelPrivate::AckEntry *> >::const_iterator it=acks.begin(); it!=acks.end(); ++it)
								{
									fxmessage("  Ack %u (0x%x %s id %u)\n", it->first, it->second->msg->msgType(), p->registry->decodeType(it->second->msg->msgType()).text(), it->second->msg->msgId());
								}
#endif
#endif
								ae=0;
							}
						}
						else msg=ae->ack;
					}
					if(!msg)
					{	// Either new message, ack not wanted or msg was not what we expected
						FXERRHM(msg=makeMsg());
						msgdelpls=true;
					}
					assert(msg);
					memcpy(msg, &tmsg, sizeof(FXIPCMsg));
					if(msgdelpls) msg->myoriginaldata=data.data();
					// Annoying you can't define local functions :(
					struct Undo { static void call(bool msgdelpls, FXIPCMsgRegistry::delMsgSpec delMsg, FXIPCMsg *msg)
						{
							if(msgdelpls) delMsg(msg);
						} };
					FXRBOp unmsg=FXRBFunc(Undo::call, msgdelpls, delMsg, msg);
					if(tmsg.gzipped())
					{
						if(!p->compressedbuffer)
						{
							FXERRHM(p->compressedbuffer=new QGZipDevice);
						}
						p->compressedbuffer->setGZData(&p->buffer);
						p->compressedbuffer->open(IO_ReadOnly);
						FXRBOp unopen=FXRBObj(*p->compressedbuffer, &QGZipDevice::close);
						FXStream compressed(p->compressedbuffer);
						deendianise(msg, compressed);
					}
					else
					{
						deendianise(msg, endianiser);
					}
					if(p->printstats)
						fxmessage("Thread %u received msg 0x%x (%s), len=%d bytes\n", (FXuint) QThread::id(), msg->msgType(), p->registry->decodeType(msg->msgType()).text(), msg->length());

					if(FXIPCMsg_Disconnect::id::code==msg->msgType())
					{
						p->quit=true;
						p->noquitmsg=true;
					}
					else if(FXIPCMsg_Unhandled::id::code==msg->msgType())
					{
						if(ae)
						{
							FXIPCMsg_Unhandled *uh=(FXIPCMsg_Unhandled *) msg;
							FXERRHM(ae->erroroccurred=new FXException(0, 0, QTrans::tr("FXIPCChannel", "Unhandled message"), FXIPCCHANNEL_UNHANDLED, FXERRH_ISFROMOTHER));
						}
					}
					else if(FXIPCMsg_ErrorOccurred::id::code==msg->msgType())
					{
						if(p->errorTrans && ae)
						{
							FXIPCMsg_ErrorOccurred *eo=(FXIPCMsg_ErrorOccurred *) msg;
							FXERRHM(ae->erroroccurred=new FXException(0, 0, eo->message, eo->code, (eo->flags&~FXERRH_ISFATAL)|FXERRH_ISFROMOTHER));
						}
					}
					if(ae)
					{
						ae->wc->wakeAll();
						// getMsgAck() calls removeAck()
					}
					else
					{
						HandledCode handled=NotHandled;
						if(!msg->hasAck() && msg->msgId())
						{	// A lonely ack?
							h.unlock();
							handled=lonelyMsgAckReceived(msg, data.data());
							h.relock();
						}
						if(NotHandled==handled)
						{
							MsgFilterSpec *mf;
							for(QPtrVectorIterator<MsgFilterSpec> it(p->premsgfilters); (mf=it.current()); ++it)
							{
								h.unlock();
								HandledCode filterhandled=invokeMsgReceived(mf, msg, tmsg);
								if(NotHandled!=filterhandled)
								{
									handled=filterhandled;
									break;
								}
								h.relock();
							}
						}
						if(NotHandled==handled)
						{
							h.unlock();
							handled=invokeMsgReceived(0, msg, tmsg);
						}
						if(NotHandled==handled && FXIPCMsg_Disconnect::id::code==msg->msgType()) handled=Handled;
						switch(handled)
						{
						case NotHandled:
							{
#ifndef DEBUG
								if(p->printstats)
#endif
									fxmessage("Thread %u msg 0x%x (%s) not handled\n", (FXuint) QThread::id(), msg->msgType(), p->registry->decodeType(msg->msgType()).text());
								if(tmsg.hasAck())
								{
									FXIPCMsg_Unhandled badmsg(tmsg.msgId());
									sendMsg(badmsg);
								}
								// Fall through
							}
						case Handled:
							{
								break;
							}
						case HandledAsync:
							{	// Don't delete msg
								assert(msgdelpls);
								msg->myoriginaldata=0;
								unmsg.dismiss();
								break;
							}
						}
						h.relock();
					}
				}
				else
				{	// If has ack, reply we don't know this
#ifndef DEBUG
					if(p->printstats)
#endif
						fxmessage("Thread %u msg 0x%x (%s) unknown\n", (FXuint) QThread::id(), tmsg.msgType(), p->registry->decodeType(tmsg.msgType()).text());
					HandledCode handled=unknownMsgReceived(&tmsg, data.data());
					if(NotHandled==handled && tmsg.hasAck())
					{
						FXIPCMsg_Unhandled badmsg(tmsg.msgId(), 1);
						sendMsg(badmsg);
					}
				}
				read-=tmsg.length();
				if(read)
				{
					memmove(data.data(), data.data()+tmsg.length(), read);
#ifdef DEBUG
					memset(data.data()+read, 0, tmsg.length());
#endif
					//fxmessage("Thread %u channel loop read=%d bytes\n", (FXuint) QThread::id(), read);
				}
				else break;
			}
		}
	}
	FXERRH_CATCH(FXConnectionLostException &)
	{
		p->noquitmsg=true;
		p->quit=true;
	}
	FXERRH_ENDTRY;
	return !p->quit;
}
FXIPCChannel::HandledCode FXIPCChannel::unknownMsgReceived(FXIPCMsg *msgheader, FXuchar *data)
{
	return NotHandled;
}
FXIPCChannel::HandledCode FXIPCChannel::lonelyMsgAckReceived(FXIPCMsg *msgheader, FXuchar *buffer)
{
	return NotHandled;
}
FXIPCChannel::HandledCode FXIPCChannel::invokeMsgReceived(FXIPCChannel::MsgFilterSpec *what, FXIPCMsg *msg, FXIPCMsg &tmsg)
{
	FXERRH_TRY
	{
		return what ? (*what)(msg) : msgReceived(msg);
	}
	FXERRH_CATCH(FXConnectionLostException &)
	{	// Quit
		p->noquitmsg=true;
		p->quit=true;
		return Handled;
	}
	FXERRH_CATCH(FXException &e)
	{
		if(tmsg.hasAck() && tmsg.wantsAck())
		{
			FXIPCMsg_ErrorOccurred errocc(tmsg.msgId(), e);
			sendMsg(errocc);
			return Handled;
		}
		else throw;
	}
	FXERRH_ENDTRY;
	return NotHandled;
}
QPtrVector<FXIPCMsgHolder> FXIPCChannel::ackedMsgs() const
{
	QPtrVector<FXIPCMsgHolder> ret;
	QMtxHold h(this);
	FXIPCChannelPrivate::AckEntry *ae;
	for(QIntDictIterator<FXIPCChannelPrivate::AckEntry> it(p->msgs); (ae=it.current()); ++it)
	{
		if(ae->wc->signalled()) ret.append((FXIPCMsgHolder *) ae->msg);
	}
	return ret;
}
void FXIPCChannel::installPreMsgReceivedFilter(FXIPCChannel::MsgFilterSpec filter)
{
	QMtxHold h(this);
	MsgFilterSpec *mf;
	FXERRHM(mf=new MsgFilterSpec(filter));
	FXRBOp unnew=FXRBNew(mf);
	p->premsgfilters.append(mf);
	unnew.dismiss();
}
bool FXIPCChannel::removePreMsgReceivedFilter(FXIPCChannel::MsgFilterSpec filter)
{
	QMtxHold h(this);
	MsgFilterSpec *mf;
	for(QPtrVectorIterator<MsgFilterSpec> it(p->premsgfilters); (mf=it.current()); ++it)
	{
		if(*mf==filter)
		{
			p->premsgfilters.removeByIter(it);
			return true;
		}
	}
	return false;
}

inline void FXIPCChannel::removeAck(void *_ae)
{	// Lock is held on entry
	if(!_ae) return;
	FXIPCChannelPrivate::AckEntry *ae=(FXIPCChannelPrivate::AckEntry *) _ae;
	ae->wc->reset();
	p->wcsFree.append(ae->wc); ae->wc=0;
	p->msgs.remove(ae->msg->msgId());
	QDICTDYNRESIZEAGGR(p->msgs);
}
inline FXuint FXIPCChannel::int_makeUniqueMsgId()
{	// Lock is held on entry
	register FXuint ret;
	do
	{
		ret=++p->msgidcount;
	} while(!ret || p->msgs.find(ret));
	return ret;
}
bool FXIPCChannel::sendMsgI(FXIPCMsg *msgack, FXIPCMsg *msg, FXIPCChannel::endianiseSpec endianise, FXuint waitfor)
{
	FXERRH(msg->hasAck() || (!msgack && !waitfor), "Can't wait for ack if the message doesn't have one", 0, FXERRH_ISDEBUG);
	//FXERRH(running(), "Communications monitor thread appears not to be running", 0, FXERRH_ISDEBUG);
	assert(!endianise || p->registry->lookup(msg->msgType()));
	{
		QThread_DTHold dthold;
		QMtxHold h(this);
		FXIPCChannelPrivate::AckEntry *ae=0;
		if(msg->hasAck())
		{
			if(msgack)
			{
				msg->myflags|=FXIPCMsg::FlagsWantAck;		// Tell other end we do want the ack
				msg->myid=int_makeUniqueMsgId();
				QWaitCondition *wc=p->wcsFree.getFirst(); p->wcsFree.takeFirst();
				if(!wc)
				{
					FXERRHM(wc=new QWaitCondition);
				}
				FXRBOp unwc=FXRBNew(wc);
				FXERRHM(ae=new FXIPCChannelPrivate::AckEntry(msg, msgack, wc));
				unwc.dismiss();
				FXRBOp unae=FXRBNew(ae);
				p->msgs.insert(msg->myid, ae);
				unae.dismiss();
				QDICTDYNRESIZEAGGR(p->msgs);
			}
		}
		FXRBOp unackentry=FXRBObj(*this, &FXIPCChannel::removeAck, ae);
		QBuffer buffer(p->sendMsgSize);
		buffer.open(IO_ReadWrite);
		p->endianiser.setDevice(&buffer);
		switch(p->endianConversion)
		{
		case AlwaysLittleEndian:
			msg->myflags&=~FXIPCMsg::FlagsIsBigEndian;
			break;
		case AlwaysBigEndian:
			msg->myflags|=FXIPCMsg::FlagsIsBigEndian;
			break;
		case AutoEndian:
			p->endianiser.swapBytes(0);			// Always disable conversion
		}
		QByteArray &data=buffer.buffer();
#ifdef DEBUG
		memset(data.data(), 0, data.size());
#endif
		if(endianise)
		{
			if(p->compressed)
			{	// If using this, execution speed is hardly a priority
				if(!p->compressedbuffer)
				{
					FXERRHM(p->compressedbuffer=new QGZipDevice);
				}
				p->compressedbuffer->setGZData(&buffer);
				p->compressedbuffer->open(IO_WriteOnly);
				FXRBOp unopen=FXRBObj(*p->compressedbuffer, &QGZipDevice::close);
				FXStream compressed(p->compressedbuffer);
				endianise(msg, compressed);
				p->compressedbuffer->close();
				unopen.dismiss();
				FXuval compressedlen=(FXuval) buffer.size();
				buffer.truncate(compressedlen+msg->headerLength());
				memmove(data.data()+msg->headerLength(), data.data(), compressedlen);
				msg->myflags|=FXIPCMsg::FlagsGZipped;
				buffer.at(compressedlen+msg->headerLength());
			}
			else
			{
				buffer.at(msg->headerLength());
				endianise(msg, p->endianiser);
			}
		} else buffer.at(msg->headerLength());
		// NOTE TO SELF: Keep consistent with restampMsgAndSend()
		FXuval len=(FXuval) buffer.at();
		msg->len=(FXuint) len;
		assert(msg->length()>=FXIPCMsg::minHeaderLength);
		assert(msg->msgType());
		if(p->maxMsgSize && msg->length()>p->maxMsgSize)
		{
			FXERRG(QTrans::tr("FXIPCChannel", "Maximum message size exceeded (%1 with limit of %2)").arg(msg->length()).arg(p->maxMsgSize), FXIPCCHANNEL_MSGTOOBIG, 0);
		}
		if(msg->length()>p->sendMsgSize) p->sendMsgSize=msg->length();
		if(p->garbageMessageCount && (msg->myid % p->garbageMessageCount)==0)
		{	// Trash the message
			FXuint seed=*(FXuint *)(data.data()+msg->headerLength());
			for(FXuval n=msg->headerLength(); n<msg->len; n+=4)
			{
				*(FXuint *)(data.data()+n)=fxrandom(seed);
			}
		}
		buffer.at(0);
		msg->write(p->endianiser);
		if(p->unreliable)
		{
			msg->crc=fxadler32(1, data.data()+2*sizeof(FXuint), len-2*sizeof(FXuint));
			buffer.at(0);
			msg->write(p->endianiser);
		}
#ifdef DEBUG
		p->endianiser.setDevice(0);
#endif
		h.unlock();
		if(p->printstats)
			fxmessage("Thread %u sending msg 0x%x (%s), len=%d bytes\n", (FXuint) QThread::id(), msg->msgType(), p->registry->decodeType(msg->msgType()).text(), (int) len);
		assert(p->dev);
		FXERRH_TRY
		{
			FXuval written=0, writ;
			//fxmessage("IPCWrite %s", fxdump8(data.data(), len).text());
#if defined(DEBUG) && 0
			{	// Ensure the message is identical when deserialised
				buffer.at(0);
				p->endianiser.setDevice(&buffer);
				FXIPCMsg tmsg(0);
				tmsg.read(p->endianiser);
				FXIPCMsgRegistry::deendianiseSpec deendianise;
				FXIPCMsgRegistry::makeMsgSpec makeMsg;
				FXIPCMsgRegistry::delMsgSpec delMsg;
				if(p->registry->lookup(deendianise, makeMsg, delMsg, tmsg.msgType()))
				{
					FXIPCMsg *msg2=makeMsg();
					FXRBOp unmsg2=FXRBFunc(delMsg, msg2);
					memcpy(msg2, &tmsg, sizeof(FXIPCMsg));
					deendianise(msg2, p->endianiser);
					p->endianiser.setDevice(0);
					assert(*msg2==*msg);
				}
			}
#endif
			while(written+=(writ=p->dev->writeBlock(data.data()+written, len-written)), written<len)
			{
				fxmessage("Partial write of %u bytes, %u to go\n", (FXuint) writ, (FXuint)(len-written));
			}
		}
		FXERRH_CATCH(FXConnectionLostException &)
		{	// Quit
			p->quit=true;
			throw;
		}
		FXERRH_ENDTRY;
		unackentry.dismiss();
	}
	if(waitfor)
		return getMsgAck(msgack, msg, waitfor);
	return true;
}
bool FXIPCChannel::getMsgAck(FXIPCMsg *msgack, FXIPCMsg *msg, FXuint waitfor)
{
	QMtxHold h(this);
	FXIPCChannelPrivate::AckEntry *ae=p->msgs.find(msg->msgId());
	FXERRH(ae, "Message not found in awaiting ack list", 0, FXERRH_ISDEBUG);
	if(p->monitorThreadId==QThread::id())
	{
		FXERRH(!waitfor, "Message monitor thread may only poll msg ack, not wait for it", 0, FXERRH_ISDEBUG);
	}
	h.unlock();
	if(ae->wc->wait(waitfor))
	{	// Duplicated in doHandle()
		h.relock();
		if(p->printstats)
			fxmessage("Operation 0x%x (%s) took %d ms\n", ae->msg->msgType(),
				p->registry->decodeType(ae->msg->msgType()).text(), FXProcess::getMsCount()-ae->timesent);
		FXException *e=ae->erroroccurred;
		removeAck(ae);
		if(e)
		{
			FXRBOp une=FXRBNew(e);
			FXERRH_THROW(*e);
		}
		return true;
	}
	return false;
}

FXuint FXIPCChannel::makeUniqueMsgId()
{
	QMtxHold h(this);
	return int_makeUniqueMsgId();
}
bool FXIPCChannel::restampMsgAndSend(FXuchar *rawmsg, FXIPCMsg *msgheader)
{
	// length, type, myid, mymsgrev, myflags and myrouting all already set
	// NOTE TO SELF: Keep in sync with sendMsgI() above
	QByteArray data; data.setRawData(rawmsg, msgheader->length());
	FXRBOp unsetrawdata=FXRBObj(data, &QByteArray::resetRawData, rawmsg, msgheader->length());
	QBuffer buffer(data);
	QThread_DTHold dthold;
	buffer.open(IO_ReadWrite);
	QMtxHold h(this);
	p->endianiser.setDevice(&buffer);
	// Use endian of original message
	if(msgheader->inBigEndian())
		p->endianiser.setByteOrder(FXStream::BigEndian);
	else
		p->endianiser.setByteOrder(FXStream::LittleEndian);
	assert(msgheader->length()>=FXIPCMsg::minHeaderLength);
	assert(msgheader->msgType());
	if(p->maxMsgSize && msgheader->length()>p->maxMsgSize)
	{
		FXERRG(QTrans::tr("FXIPCChannel", "Maximum message size exceeded (%1 with limit of %2)").arg(msgheader->length()).arg(p->maxMsgSize), FXIPCCHANNEL_MSGTOOBIG, 0);
	}
	msgheader->write(p->endianiser);
	if(p->unreliable)
	{
		msgheader->crc=fxadler32(1, data.data()+2*sizeof(FXuint), msgheader->length()-2*sizeof(FXuint));
		buffer.at(0);
		msgheader->write(p->endianiser);
	}
#ifdef DEBUG
	p->endianiser.setDevice(0);
#endif
	h.unlock();
	if(p->printstats)
		fxmessage("Thread %u resending msg 0x%x (%s), len=%d bytes\n", (FXuint) QThread::id(), msgheader->msgType(), p->registry->decodeType(msgheader->msgType()).text(), (int) msgheader->length());
	assert(p->dev);
	FXERRH_TRY
	{
		FXuval written=0, writ, len=msgheader->length();
		//fxmessage("IPCWrite %s", fxdump8(data.data(), len).text());
		while(written+=(writ=p->dev->writeBlock(data.data()+written, len-written)), written<len)
		{
			fxmessage("Partial write of %u bytes, %u to go\n", (FXuint) writ, (FXuint)(len-written));
		}
	}
	FXERRH_CATCH(FXConnectionLostException &)
	{	// Quit
		p->quit=true;
		throw;
	}
	FXERRH_ENDTRY;
	return true;
}

void FXIPCChannel::doAsyncHandled(FXIPCMsg *msg, HandledCode handled)
{
	if(NotHandled==handled)
	{
#ifndef DEBUG
		if(p->printstats)
#endif
			fxmessage("Thread %u msg 0x%x (%s) not handled\n", (FXuint) QThread::id(), msg->msgType(), p->registry->decodeType(msg->msgType()).text());
		if(msg->hasAck())
		{
			FXIPCMsg_Unhandled badmsg(msg->msgId());
			sendMsg(badmsg);
		}
	}
}
void FXIPCChannel::doAsyncHandled(FXIPCMsg *msg, FXException &e)
{
	FXERRH(msg->hasAck(), "Message needs ack to return exception to sender", 0, FXERRH_ISDEBUG);
	FXIPCMsg_ErrorOccurred errocc(msg->msgId(), e);
	sendMsg(errocc);
}

void FXIPCChannel::forceClose()
{
	p->quit=true;
	p->noquitmsg=true;
}

void *FXIPCChannel::cleanup()
{	// Tell all things waiting on an ack that no more connection
	QMtxHold h(this);
	FXIPCChannelPrivate::AckEntry *ae;
	for(QIntDictIterator<FXIPCChannelPrivate::AckEntry> it(p->msgs); (ae=it.current()); ++it)
	{
		FXERRHM(ae->erroroccurred=new FXConnectionLostException("Channel closed", 0));
		ae->wc->wakeAll();
	}
	h.unlock();
//#ifndef __linux__
	// Send quit message for graceful shutdown
	if(!p->noquitmsg && p->registry->isValid())
	{
#ifdef USE_POSIX
		/* 29th January 2005 ned: I've had problems with this code on POSIX since day
		one when the device is a pipe. QPipe lazily creates the write side of the pipe
		which causes a stall until the read end reads on POSIX and the problem was that
		various systems don't seem to like this being done in a cancellation handler much.
		So, having failed to come up with a solution, if it's a pipe we simply SIGPIPE it */
		QPipe *mypipe=dynamic_cast<QPipe *>(p->dev);
		if(mypipe)
		{	// Don't send the message
			return 0;
			//mypipe->int_hack_makeWriteNonblocking();
		}
#endif
#ifdef DEBUG
		fxmessage("Attempting to send Disconnect message to other end of IPC connection\n");
#endif
		FXERRH_TRY
		{
			FXIPCMsg_Disconnect byebye;
			sendMsg(byebye);
		}
		FXERRH_CATCH(...)
		{	// sink all - it's not important
		}
		FXERRH_ENDTRY;
#ifdef DEBUG
		fxmessage("Succeeded in sending Disconnect message to other end of IPC connection!\n");
#endif
	}
//#endif
	return 0;
}

Generic::BoundFunctorV *FXIPCChannel::int_addMsgHandler(FXAutoPtr<Generic::BoundFunctorV> v)
{
	QMtxHold h(this);
	assert(p->threadPool);
	p->threadPool->dispatch(PtrPtr(v));
	p->msgHandlings.append(PtrPtr(v));
	return PtrRelease(v);
}
bool FXIPCChannel::int_removeMsgHandler(Generic::BoundFunctorV *v)
{
	QMtxHold h(this);
	return p->msgHandlings.takeRef(v);
}


} // namespace
