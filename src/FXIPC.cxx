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
#include "FXTrans.h"
#include "FXRollback.h"
#include "FXBuffer.h"
#include "FXProcess.h"
#include "FXGZipDevice.h"
#include "FXIODeviceS.h"
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

struct FXDLLLOCAL FXIPCMsgRegistryPrivate : public FXMutex
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
	FXMtxHold h(p);
	FXERRH(p->msgs.find(code)==0, FXTrans::tr("FXIPCMsgRegistry", "Message already registered in this registry"), FXIPCMSGREGISTRY_MSGALREADYREGED, 0);
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
	FXMtxHold h(p);
	assert(p->msgs.find(code));
	p->msgs.remove(code);
}

bool FXIPCMsgRegistry::lookup(FXuint code) const
{
	FXMtxHold h(p);
	return p->msgs.find(code)!=0;
}

bool FXIPCMsgRegistry::lookup(FXIPCMsgRegistry::deendianiseSpec &deendianise, FXIPCMsgRegistry::makeMsgSpec &makeMsg, FXIPCMsgRegistry::delMsgSpec &delMsg, FXuint code) const
{
	FXMtxHold h(p);
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
	FXMtxHold h(p);
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
	FXIODeviceS *dev;
	bool unreliable, compressed, errorTrans, quit, noquitmsg, peerUntrusted, printstats;
	FXIPCChannel::EndianConversionKinds endianConversion;
	FXuint maxMsgSize, garbageMessageCount;
	FXBuffer buffer;
	FXGZipDevice *compressedbuffer;
	FXStream endianiser;
	QPtrList<FXWaitCondition> wcsFree;
	struct AckEntry
	{
		FXIPCMsg *msg, *ack;
		FXWaitCondition *wc;
		FXuint timesent;
		FXException *erroroccurred;
		AckEntry(FXIPCMsg *_msg, FXIPCMsg *_ack, FXWaitCondition *_wc)
			: msg(_msg), ack(_ack), wc(_wc), timesent(FXProcess::getMsCount()), erroroccurred(0) { }
		~AckEntry() { FXDELETE(wc); }
	};
	QIntDict<AckEntry> msgs;
	FXuint msgidcount;
	FXuint monitorThreadId;
	QPtrVector<FXIPCChannel::MsgFilterSpec> premsgfilters;
	FXThreadPool *threadPool;
	QPtrList<Generic::BoundFunctorV> msgHandlings;
	FXIPCChannelPrivate(FXIPCMsgRegistry *_registry, FXIODeviceS *_dev, bool _peerUntrusted, FXThreadPool *_threadPool)
		: registry(_registry), dev(_dev), unreliable(false), compressed(false), errorTrans(true),
		quit(false), noquitmsg(false), peerUntrusted(_peerUntrusted), printstats(false), endianConversion(FXIPCChannel::AutoEndian),
		maxMsgSize(65536), garbageMessageCount(0), buffer(pageSize), compressedbuffer(0), endianiser(&buffer), wcsFree(true),
		msgs(1, true), msgidcount(0), monitorThreadId(0), premsgfilters(true), threadPool(_threadPool), msgHandlings(true)
	{
		buffer.open(IO_ReadWrite);
//#ifdef DEBUG
//		printstats=true;
//#endif
	}
};

FXIPCChannel::FXIPCChannel(FXIPCMsgRegistry &registry, FXIODeviceS *dev, bool peerUntrusted, FXThreadPool *threadPool, const char *threadname)
	: FXThread(threadname, false, 128*1024, FXThread::InProcess), p(0)
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
	FXMtxHold h(this);
	Generic::BoundFunctorV *callv;
	for(QPtrListIterator<Generic::BoundFunctorV> it(p->msgHandlings); (callv=it.current());)
	{
		if(FXThreadPool::Cancelled==p->threadPool->cancel(callv, false))
		{
			QPtrListIterator<Generic::BoundFunctorV> it2=it;
			++it;
			p->msgHandlings.removeByIter(it2);
		}
		else ++it;
	}
	h.unlock();
	while(!p->msgHandlings.isEmpty()) FXThread::yield();
	FXDELETE(p->compressedbuffer);
	assert(p->msgs.isEmpty());
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }
FXIPCMsgRegistry &FXIPCChannel::registry() const
{
	FXMtxHold h(this);
	return *p->registry;
}
void FXIPCChannel::setRegistry(FXIPCMsgRegistry &registry)
{
	FXMtxHold h(this);
	p->registry=&registry;
}
FXIODeviceS *FXIPCChannel::device() const
{
	FXMtxHold h(this);
	return p->dev;
}
void FXIPCChannel::setDevice(FXIODeviceS *dev)
{
	FXMtxHold h(this);
	p->dev=dev;
}
FXThreadPool *FXIPCChannel::threadPool() const
{
	FXMtxHold h(this);
	return p->threadPool;
}
void FXIPCChannel::setThreadPool(FXThreadPool *threadPool)
{
	FXMtxHold h(this);
	p->threadPool=threadPool;
}
bool FXIPCChannel::unreliable() const
{
	FXMtxHold h(this);
	return p->unreliable;
}
void FXIPCChannel::setUnreliable(bool v)
{
	FXMtxHold h(this);
	p->unreliable=v;
}
bool FXIPCChannel::compression() const
{
	FXMtxHold h(this);
	return p->compressed;
}
void FXIPCChannel::setCompression(bool v)
{
	FXMtxHold h(this);
	p->compressed=v;
}
bool FXIPCChannel::active() const
{
	FXMtxHold h(this);
	return !p->quit;
}
void FXIPCChannel::reset()
{
	FXMtxHold h(this);
	p->quit=false; p->noquitmsg=false;
	p->monitorThreadId=0;
}
FXIPCChannel::EndianConversionKinds FXIPCChannel::endianConversion() const
{
	FXMtxHold h(this);
	return p->endianConversion;
}
void FXIPCChannel::setEndianConversion(FXIPCChannel::EndianConversionKinds kind)
{
	FXMtxHold h(this);
	p->endianConversion=kind;
	if(AlwaysLittleEndian==kind)
		p->endianiser.setByteOrder(FXStream::LittleEndian);
	else if(AlwaysBigEndian==kind)
		p->endianiser.setByteOrder(FXStream::BigEndian);
}
bool FXIPCChannel::errorTranslation() const
{
	FXMtxHold h(this);
	return p->errorTrans;
}
void FXIPCChannel::setErrorTranslation(bool v)
{
	FXMtxHold h(this);
	p->errorTrans=v;
}
bool FXIPCChannel::peerUntrusted() const
{
	FXMtxHold h(this);
	return p->peerUntrusted;
}
void FXIPCChannel::setPeerUntrusted(bool v)
{
	FXMtxHold h(this);
	p->peerUntrusted=v;
}
FXuint FXIPCChannel::maxMsgSize() const
{
	FXMtxHold h(this);
	return p->maxMsgSize;
}
void FXIPCChannel::setMaxMsgSize(FXuint newsize)
{
	FXMtxHold h(this);
	p->maxMsgSize=newsize;
}
FXuint FXIPCChannel::garbageMessageCount() const
{
	FXMtxHold h(this);
	return p->garbageMessageCount;
}
void FXIPCChannel::setGarbageMessageCount(FXuint newsize)
{
	FXMtxHold h(this);
	p->garbageMessageCount=newsize;
}
void FXIPCChannel::setPrintStatistics(bool v)
{
	FXMtxHold h(this);
	p->printstats=v;
}
void FXIPCChannel::requestClose()
{
	if(!p->noquitmsg && p->dev)
	{
		FXIPCMsg_Disconnect byebye;
		sendMsg(byebye);
	}
	forceClose();
}

bool FXIPCChannel::doReception(FXuint waitfor)
{
	if(p->quit) return false;
	if(!waitfor && p->dev->atEnd()) return false;
	FXERRH_TRY
	{
		QByteArray &data=p->buffer.buffer();
		FXuval read=0;
#ifdef DEBUG
		memset(data.data(), 0, data.size());
#endif
		while(read<FXIPCMsg::minHeaderLength)
		{
			FXuval justread=0;
			if(waitfor!=FXINFINITE && p->dev->atEnd())
			{
				if(!FXIODeviceS::waitForData(0, 1, &p->dev, waitfor)) return false;
			}
			read+=(justread=p->dev->readBlock(data.data()+read, pageSize-read));
			if(!justread) return true;
		}
		//fxmessage("Thread %d channel read=%d bytes\n", FXThread::id(), read);
		{
			FXThread_DTHold dthold;
			FXIPCMsg tmsg(0);
			FXMtxHold h(this);
			if(!p->monitorThreadId) p->monitorThreadId=FXThread::id();
			while(!p->quit)
			{
				p->buffer.at(0);
				p->endianiser.setDevice(&p->buffer);
				if(AutoEndian==p->endianConversion)		// Receiver translates
					p->endianiser.setByteOrder(((FXIPCMsg *) data.data())->inBigEndian() ? FXStream::BigEndian : FXStream::LittleEndian);
				tmsg.read(p->endianiser);
				assert(tmsg.length()>=FXIPCMsg::minHeaderLength);
				assert(tmsg.msgType());
				if(p->maxMsgSize && tmsg.length()>p->maxMsgSize)
				{
					FXERRG(FXTrans::tr("FXIPCChannel", "Maximum message size exceeded"), FXIPCCHANNEL_MSGTOOBIG, 0);
				}
				if(p->buffer.size()<tmsg.length())
					p->buffer.truncate(tmsg.length());
				FXint togo=tmsg.length()-read;
				while(togo>0)
				{
					h.unlock();
					if(waitfor!=FXINFINITE && p->dev->atEnd())
					{
						if(!FXIODeviceS::waitForData(0, 1, &p->dev, waitfor)) return false;
					}
					read+=p->dev->readBlock(data.data()+read, togo);
					//fxmessage("Thread %d channel read=%d bytes\n", FXThread::id(), read);
					h.relock();
					togo=tmsg.length()-read;
				}
				if(tmsg.crc)
				{
					FXuint crc=fxadler32(1, data.data()+2*sizeof(FXuint), tmsg.length()-2*sizeof(FXuint));
					FXERRH(crc=tmsg.crc, FXTrans::tr("FXIPCChannel", "Message failed CRC check"), FXIPCCHANNEL_BADMESSAGE, 0);
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
						if(ae->msg->msgType()+1!=tmsg.msgType())
						{
							if(tmsg.msgType()>FXIPCMsg_ErrorOccurred::id::code)
								ae=0;
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
					p->endianiser.setDevice(&p->buffer);
					if(tmsg.gzipped())
					{
						if(!p->compressedbuffer)
						{
							FXERRHM(p->compressedbuffer=new FXGZipDevice);
						}
						p->compressedbuffer->setGZData(&p->buffer);
						p->compressedbuffer->open(IO_ReadOnly);
						FXRBOp unopen=FXRBObj(*p->compressedbuffer, &FXGZipDevice::close);
						FXStream compressed(p->compressedbuffer);
						deendianise(msg, compressed);
					}
					else
					{
						deendianise(msg, p->endianiser);
					}
#ifdef DEBUG
					p->endianiser.setDevice(0);
#endif
					if(p->printstats)
						fxmessage("Thread %d received msg 0x%x (%s), len=%d bytes\n", FXThread::id(), msg->msgType(), p->registry->decodeType(msg->msgType()).text(), msg->length());

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
							FXERRHM(ae->erroroccurred=new FXException(0, 0, FXTrans::tr("FXIPCChannel", "Unhandled message"), FXIPCCHANNEL_UNHANDLED, FXERRH_ISFROMOTHER));
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
								if(p->printstats)
									fxmessage("Thread %d msg 0x%x (%s) not handled\n", FXThread::id(), msg->msgType(), p->registry->decodeType(msg->msgType()).text());
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
					if(p->printstats)
						fxmessage("Thread %d msg 0x%x (%s) unknown\n", FXThread::id(), tmsg.msgType(), p->registry->decodeType(tmsg.msgType()).text());
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
					//fxmessage("Thread %d channel loop read=%d bytes\n", FXThread::id(), read);
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
		if(tmsg.hasAck())
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
	FXMtxHold h(this);
	FXIPCChannelPrivate::AckEntry *ae;
	for(QIntDictIterator<FXIPCChannelPrivate::AckEntry> it(p->msgs); (ae=it.current()); ++it)
	{
		if(ae->wc->signalled()) ret.append((FXIPCMsgHolder *) ae->msg);
	}
	return ret;
}
void FXIPCChannel::installPreMsgReceivedFilter(FXIPCChannel::MsgFilterSpec filter)
{
	FXMtxHold h(this);
	MsgFilterSpec *mf;
	FXERRHM(mf=new MsgFilterSpec(filter));
	FXRBOp unnew=FXRBNew(mf);
	p->premsgfilters.append(mf);
	unnew.dismiss();
}
bool FXIPCChannel::removePreMsgReceivedFilter(FXIPCChannel::MsgFilterSpec filter)
{
	FXMtxHold h(this);
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
		FXThread_DTHold dthold;
		FXMtxHold h(this);
		FXIPCChannelPrivate::AckEntry *ae=0;
		if(msg->hasAck())
		{
			if(msgack) msg->myflags|=FXIPCMsg::FlagsWantAck;		// Tell other end to please bother serialising ack
			msg->myid=int_makeUniqueMsgId();
			FXWaitCondition *wc=p->wcsFree.getFirst(); p->wcsFree.takeFirst();
			if(!wc)
			{
				FXERRHM(wc=new FXWaitCondition);
			}
			FXRBOp unwc=FXRBNew(wc);
			FXERRHM(ae=new FXIPCChannelPrivate::AckEntry(msg, msgack, wc));
			unwc.dismiss();
			FXRBOp unae=FXRBNew(ae);
			p->msgs.insert(msg->myid, ae);
			unae.dismiss();
			QDICTDYNRESIZEAGGR(p->msgs);
		}
		FXRBOp unackentry=FXRBObj(*this, &FXIPCChannel::removeAck, ae);
		FXBuffer buffer(pageSize);
		buffer.open(IO_ReadWrite);
		p->endianiser.setDevice(&buffer);
		switch(p->endianConversion)
		{
		case AlwaysLittleEndian:
			p->endianiser.setByteOrder(FXStream::LittleEndian);
			msg->myflags&=~FXIPCMsg::FlagsIsBigEndian;
			break;
		case AlwaysBigEndian:
			p->endianiser.setByteOrder(FXStream::BigEndian);
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
					FXERRHM(p->compressedbuffer=new FXGZipDevice);
				}
				p->compressedbuffer->setGZData(&buffer);
				p->compressedbuffer->open(IO_WriteOnly);
				FXRBOp unopen=FXRBObj(*p->compressedbuffer, &FXGZipDevice::close);
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
		FXuval len;
		msg->len=len=(FXuint) buffer.at();
		assert(msg->length()>=FXIPCMsg::minHeaderLength);
		assert(msg->msgType());
		if(p->maxMsgSize && msg->length()>p->maxMsgSize)
		{
			FXERRG(FXTrans::tr("FXIPCChannel", "Maximum message size exceeded"), FXIPCCHANNEL_MSGTOOBIG, 0);
		}
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
			fxmessage("Thread %d sending msg 0x%x (%s), len=%d bytes\n", FXThread::id(), msg->msgType(), p->registry->decodeType(msg->msgType()).text(), (int) len);
		assert(p->dev);
		FXERRH_TRY
		{
			//fxmessage("IPCWrite %s", fxdump8(data.data(), len).text());
			p->dev->writeBlock(data.data(), len);
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
	FXMtxHold h(this);
	FXIPCChannelPrivate::AckEntry *ae=p->msgs.find(msg->msgId());
	FXERRH(ae, "Message not found in awaiting ack list", 0, FXERRH_ISDEBUG);
	if(p->monitorThreadId==FXThread::id())
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
	FXMtxHold h(this);
	return int_makeUniqueMsgId();
}
bool FXIPCChannel::restampMsgAndSend(FXuchar *rawmsg, FXIPCMsg *msgheader)
{
	// length, type, myid, mymsgrev, myflags and myrouting all already set
	// NOTE TO SELF: Keep in sync with sendMsgI() above
	QByteArray data; data.setRawData(rawmsg, msgheader->length());
	FXRBOp unsetrawdata=FXRBObj(data, &QByteArray::resetRawData, rawmsg, msgheader->length());
	FXBuffer buffer(data);
	FXThread_DTHold dthold;
	buffer.open(IO_ReadWrite);
	FXMtxHold h(this);
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
		FXERRG(FXTrans::tr("FXIPCChannel", "Maximum message size exceeded"), FXIPCCHANNEL_MSGTOOBIG, 0);
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
		fxmessage("Thread %d resending msg 0x%x (%s), len=%d bytes\n", FXThread::id(), msgheader->msgType(), p->registry->decodeType(msgheader->msgType()).text(), (int) msgheader->length());
	assert(p->dev);
	FXERRH_TRY
	{
		//fxmessage("IPCWrite %s", fxdump8(data.data(), len).text());
		p->dev->writeBlock(data.data(), msgheader->length());
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
		if(p->printstats)
			fxmessage("Thread %d msg 0x%x (%s) not handled\n", FXThread::id(), msg->msgType(), p->registry->decodeType(msg->msgType()).text());
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
	FXMtxHold h(this);
	FXIPCChannelPrivate::AckEntry *ae;
	for(QIntDictIterator<FXIPCChannelPrivate::AckEntry> it(p->msgs); (ae=it.current()); ++it)
	{
		FXERRHM(ae->erroroccurred=new FXConnectionLostException("Channel closed", 0));
		ae->wc->wakeAll();
	}
	h.unlock();
	/* 8th March 2004 ned: Strangely on Linux RH9 trying to do any fifo i/o from
	within a cleanup handler causes immediate thread exit. Sockets, files etc. are
	all fine - just fifos :( */
#ifndef __linux__
	// Send quit message
	if(!p->noquitmsg && p->registry->isValid())
	{
		FXERRH_TRY
		{
			FXIPCMsg_Disconnect byebye;
			sendMsg(byebye);
		}
		FXERRH_CATCH(...)
		{	// sink all - it's not important
		}
		FXERRH_ENDTRY;
	}
#endif
	return 0;
}

Generic::BoundFunctorV *FXIPCChannel::int_addMsgHandler(FXAutoPtr<Generic::BoundFunctorV> v)
{
	FXMtxHold h(this);
	assert(p->threadPool);
	p->threadPool->dispatch(PtrPtr(v));
	p->msgHandlings.append(PtrPtr(v));
	return PtrRelease(v);
}
bool FXIPCChannel::int_removeMsgHandler(Generic::BoundFunctorV *v)
{
	FXMtxHold h(this);
	return p->msgHandlings.takeRef(v);
}


} // namespace
