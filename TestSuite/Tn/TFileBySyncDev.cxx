/* TFileBySyncDev.cxx
Permits a file type QIODevice to be accessed through a QIODeviceS
(C) 2002, 2003 Niall Douglas
Original version created: July 2002
This version created: 13th Jan 2003
*/

/* This code is licensed for use only in testing TnFOX facilities.
It may not be used by other code or incorporated into code without
prior written permission */

#include "TFileBySyncDev.h"
#include "FXIPC.h"
#include <qptrvector.h>
#include <qvaluelist.h>
#include <qcstring.h>
#include "FXMemDbg.h"
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace Tn {

typedef FXIPCMsgChunkCodeAlloc<0x80, true> TFBSDChunkBegin;
//! Used to send client an error (more slimline than FXIPCMsg_ErrorOccurred)
struct TFBSD_Error : public FXIPCMsg
{
	typedef FXIPCMsgChunkCodeAlloc<TFBSDChunkBegin::code, false> id;
	typedef FXIPCMsgRegister<id, TFBSD_Error> regtype;
	u64 offset;
	u32 amount;
	u32 code;
	FXString message;
	TFBSD_Error() : FXIPCMsg(id::code), offset(0), amount(0), code(0) { }
	TFBSD_Error(u64 _offset, u32 _amount, FXException &e)
		: FXIPCMsg(id::code), offset(_offset), amount(_amount), code(e.code()), message(e.message()) { }
	void   endianise(FXStream &ds) const { ds << code << message; }
	void deendianise(FXStream &ds)       { ds >> code >> message; }
};
//! Used to request a data fill
struct TFBSD_Read : public FXIPCMsg
{
	typedef FXIPCMsgChunkCodeAlloc<TFBSD_Error::id::nextcode, false> id;
	typedef FXIPCMsgRegister<id, TFBSD_Read> regtype;
	u64 offset;
	u32 amount;
	u32 buffers;
	u32 blockSize;
	TFBSD_Read(u64 _offset=0, u32 _amount=0, u32 _buffers=0, u32 _blockSize=0)
		: FXIPCMsg(id::code), offset(_offset), amount(_amount), buffers(_buffers), blockSize(_blockSize) { }
	void   endianise(FXStream &ds) const { ds << offset << amount << buffers << blockSize; }
	void deendianise(FXStream &ds)       { ds >> offset >> amount >> buffers >> blockSize; }
};
//! Used to push data from one side to the other
struct TFBSD_Data : public FXIPCMsg
{
	typedef FXIPCMsgChunkCodeAlloc<TFBSD_Read::id::nextcode, false> id;
	typedef FXIPCMsgRegister<id, TFBSD_Data> regtype;
	u64 offset;
	bool datamine;	// Not sent
	FXuchar *data;	// Not sent
	u32 amount;
	int headerLength() const throw() { return FXIPCMsg::headerLength()+sizeof(u64)+sizeof(u32); }
	TFBSD_Data(u64 _offset=0, FXuchar *_data=0, u32 _amount=0)
		: FXIPCMsg(id::code), offset(_offset), datamine(false), data(_data), amount(_amount) { }
	void   endianise(FXStream &ds) const { ds << offset << amount; ds.writeRawBytes(data, amount); }
	void deendianise(FXStream &ds)
	{
		ds >> offset >> amount;
		if(!data) { FXERRHM(data=new FXuchar[amount]); datamine=true; } // data deleted manually
		ds.readRawBytes(data, amount);
	}
private:
	TFBSD_Data(const TFBSD_Data &);
	TFBSD_Data &operator=(const TFBSD_Data &);
};
typedef FXIPCMsgChunk<Generic::TL::create<
		TFBSD_Error::regtype,
		TFBSD_Read::regtype,
		TFBSD_Data::regtype
   >::value> TFBSDMsgChunk;



struct TFileBySyncDevPrivate : public QMutex
{
	TFileBySyncDev *parent;
	FXIPCChannel *ctrl;
	bool amServer, printstats;
	QIODevice *source;
	TFBSDMsgChunk *chunk;

	// Client only
	QPtrVector<TFBSD_Error> errors;
	struct Buffer
	{
		FXfval offset;
		s32 len;		// len is -1 if more data to come
		bool empty, pending, modified;
		QByteArray data;
		Buffer() : offset(0), len(0), empty(true), pending(false), modified(false) { }
	};
	QValueList<Buffer> buffers;
	QWaitCondition buffersReady;

	// Server only
	FXfval lastSuccessfulOffset;

	// Both client and server
	u32 maxBuffers, blockSize;
	TFileBySyncDevPrivate(TFileBySyncDev *i, bool server, FXIPCChannel &_ctrl, QIODevice *_source)
		: QMutex(), parent(i), ctrl(&_ctrl), amServer(server), printstats(false), source(_source), chunk(0), errors(true),
		lastSuccessfulOffset(0),
		maxBuffers(0), blockSize(0) {
#ifdef DEBUG
			printstats=true;
#endif
		}
	FXIPCChannel::HandledCode msgReceived(FXIPCMsg *msg);
	inline u32 idealBuffers() const
	{
		u32 ret=maxBuffers>>FXProcess::memoryFull();
		if(ret<1) ret=1;
		return ret;
	}
	inline void dynchkBuffers()
	{
		u32 target=idealBuffers();
		if(target!=buffers.count())
		{
			bool extending=target>buffers.count();
			if(printstats)
				fxmessage("Due to memory full change, dynamically adjusting buffers from %u to %u\n", (u32) buffers.count(), target);
			buffers.resize(target);
			if(extending)
			{
				FXfval offset=parent->ioIndex;
				for(QValueList<Buffer>::iterator it=buffers.begin(); it!=buffers.end(); ++it)
				{
					Buffer &b=*it;
					if(!b.empty) offset=b.offset;
					else
					{
						offset+=blockSize;
						b.offset=offset;
					}
				}
			}
		}
	}
	void resizeBuffers(u32 no, u32 size)
	{
		if(no!=maxBuffers)
		{
			maxBuffers=no;
			dynchkBuffers();
			if(size!=blockSize)
			{
				FXfval offset=parent->ioIndex;
				blockSize=size;
				for(QValueList<Buffer>::iterator it=buffers.begin(); it!=buffers.end(); ++it)
				{
					Buffer &b=*it;
					b.offset=offset; offset+=blockSize;
					b.len=0; b.empty=true; b.pending=false; b.modified=false;
					b.data.resize(size);
				}
			}
		}
	}
	inline void retireBuffer()
	{	// lock held on entry
		Buffer &b=buffers.front();
		if(b.modified)
		{
			TFBSD_Data i(b.offset, b.data.data(), FXMIN(blockSize, (u32)b.len));
			QMtxHold h(this, QMtxHold::UnlockAndRelock);
			if(printstats)
				fxmessage("Sending modified buffer offset=%d with %d bytes\n", (u32) i.offset, i.amount);
			ctrl->sendMsg(i);
			b.modified=false;
		}
		if(printstats)
			fxmessage("Retiring buffer offset=%d\n", (u32) b.offset);
		b.empty=true;
#ifdef DEBUG
		memset(b.data.data(), 0, b.data.size());
#endif
		b.offset=buffers.back().offset+blockSize;
		buffers.splice(buffers.end(), buffers, buffers.begin());
		if(buffers.front().empty && buffersReady.signalled())
			buffersReady.reset();
	}
};

FXIPCChannel::HandledCode TFileBySyncDevPrivate::msgReceived(FXIPCMsg *msg)
{
	u64 offset=0;
	u32 amount=0;
	FXERRH_TRY
	{
		switch(msg->msgType())
		{
		case TFBSD_Error::id::code:
			{	// Received by the client only
				TFBSD_Error *m=(TFBSD_Error *) msg;
				QMtxHold h(this);
				if(amServer)
					return FXIPCChannel::Handled;
				else
				{
					errors.append(m);
					return FXIPCChannel::HandledAsync;
				}
			}
		case TFBSD_Read::id::code:
			{	// Received by the server only
				TFBSD_Read *m=(TFBSD_Read *) msg;
				QMtxHold h(this);
				blockSize=m->blockSize;
				maxBuffers=m->buffers;
				h.unlock();
				if(printstats)
					fxmessage("Received request for offset=%d of %d bytes\n", (u32) m->offset, m->amount);
				QByteArray buffer(blockSize);
				FXuchar *data=buffer.data();
				for(offset=m->offset; offset<m->offset+m->amount; offset+=blockSize)
				{
					if((amount=(u32) parent->fetchData(data, blockSize, offset))
						|| lastSuccessfulOffset+blockSize==offset)
					{
						if(amount) lastSuccessfulOffset=offset;
						TFBSD_Data i(offset, data, amount);
						ctrl->sendMsg(i);
					}
					else break;
				}
				return FXIPCChannel::Handled;
			}
		case TFBSD_Data::id::code:
			{	// Received by both server and client
				TFBSD_Data *m=(TFBSD_Data *) msg;
				FXRBOp deldata=FXRBNewA(m->data);
				if(amServer)
					parent->putData(m->offset, m->data, m->amount);
				else
				{
					QMtxHold h(this);
					QValueList<Buffer>::iterator it=buffers.begin();
					for(; it!=buffers.end(); ++it)
					{
						if(m->offset==(*it).offset) break;
					}
					if(it!=buffers.end())
					{
						Buffer &b=*it;
						if(printstats)
							fxmessage("Filling buffer offset=%d with %d bytes\n", (u32) b.offset, m->amount);
						if(m->amount>blockSize) m->amount=blockSize;
						b.len=(m->amount==blockSize) ? -1 : m->amount;
						b.empty=false; b.pending=false; b.modified=false;
						memcpy(b.data.data(), m->data, m->amount);
						if(-1!=b.len) memset(b.data.data()+m->amount, 0, blockSize-m->amount);
						if(it==buffers.begin()) buffersReady.wakeAll();
					}
					else
					{
						if(printstats)
							fxmessage("WARNING: Received unwanted buffer offset=%d with %d bytes\n", (u32) m->offset, m->amount);
					}
				}
				return FXIPCChannel::Handled;
			}
		}
	}
	FXERRH_CATCH(FXException &e)
	{
		TFBSD_Error i(offset, amount, e);
		ctrl->sendMsg(i);
		return FXIPCChannel::Handled;
	}
	FXERRH_ENDTRY;
	return FXIPCChannel::NotHandled;
}

TFileBySyncDev::TFileBySyncDev(bool server, FXIPCChannel &ctrl, QIODevice *source) : p(0)
{	// Server
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new TFileBySyncDevPrivate(this, server, ctrl, source));
	channelInstall();
	if(!server) p->resizeBuffers(8, FXProcess::pageSize()-TFBSD_Data().headerLength()-sizeof(u32));
	unconstr.dismiss();
}
inline void TFileBySyncDev::channelInstall() const
{
	FXERRHM(p->chunk=new TFBSDMsgChunk(&p->ctrl->registry()));
	FXIPCChannel::MsgFilterSpec mfs(p, &TFileBySyncDevPrivate::msgReceived);
	p->ctrl->installPreMsgReceivedFilter(std::move(mfs));
}
inline void TFileBySyncDev::channelUninstall() const
{
	if(p->chunk)
	{
		FXIPCChannel::MsgFilterSpec mfs(p, &TFileBySyncDevPrivate::msgReceived);
		p->ctrl->removePreMsgReceivedFilter(std::move(mfs));
		FXDELETE(p->chunk);
	}
}
TFileBySyncDev::~TFileBySyncDev()
{ FXEXCEPTIONDESTRUCT1 {
	close();
	channelUninstall();
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }
inline void TFileBySyncDev::requestData()
{	// Do NOT hold lock when calling this
	if(isOpen())
	{
		QMtxHold h(p);
		FXfval offset=(FXfval)-1, amount=0;
		p->dynchkBuffers();
		QValueList<TFileBySyncDevPrivate::Buffer>::iterator it=--p->buffers.end();
		bool cont=false;
		while((*it).empty && !(*it).pending && it!=p->buffers.begin())
		{
			--it;
			cont=true;
		}
		if(!cont) return;	// early exit
		for(; it!=p->buffers.end(); ++it)
		{
			TFileBySyncDevPrivate::Buffer &b=*it;
			if(b.empty && !b.pending)
			{
				if(offset>b.offset) offset=b.offset;
				amount+=p->blockSize;
				b.pending=true;
			}
		}
		if(amount)
		{
			TFBSD_Read i(offset, (u32) amount, (u32) p->buffers.count(), p->blockSize);
			h.unlock();
			p->ctrl->sendMsg(i);
		}
	}
}
inline void TFileBySyncDev::checkErrors()
{
	if(p->errors.isEmpty()) return;
	TFBSD_Error *err=p->errors.getFirst();
	FXRBOp remove=FXRBObj(p->errors, &QPtrVector<TFBSD_Error>::removeRef, err);
	FXERRG(err->message, err->code, FXERRH_ISFROMOTHER);
}
FXIPCChannel &TFileBySyncDev::channel() const
{
	QMtxHold h(p);
	return *p->ctrl;
}
void TFileBySyncDev::setChannel(FXIPCChannel &ch)
{
	QMtxHold h(p);
	if(&ch!=p->ctrl)
	{
		channelUninstall();
		p->ctrl=&ch;
		channelInstall();
	}
}
void TFileBySyncDev::setPrintStatistics(bool v)
{
	p->printstats=v;
}
u32 TFileBySyncDev::blockSize() const
{
	QMtxHold h(p);
	return p->blockSize;
}
u32 TFileBySyncDev::buffers() const
{
	QMtxHold h(p);
	return p->maxBuffers;
}
void TFileBySyncDev::setTransferParams(u32 no, u32 size)
{
	QMtxHold h(p);
	if(!p->source)
	{
		p->resizeBuffers(no, size);
		h.unlock();
		requestData();
	}
}
bool TFileBySyncDev::invalidateBuffers(FXfval offset, FXfval length)
{
	bool ret=false;
	QMtxHold h(p);
	checkErrors();
	FXfval start=offset, end=offset+length;
	for(QValueList<TFileBySyncDevPrivate::Buffer>::iterator it=p->buffers.begin(); it!=p->buffers.end(); ++it)
	{
		TFileBySyncDevPrivate::Buffer &b=*it;
		if((b.offset>=start && b.offset<end) || (b.offset+p->blockSize>=start && b.offset+p->blockSize<end))
		{
			if(b.modified) flush();
			b.empty=true; b.pending=false; assert(!b.modified);
			ret=true;
		}
	}
	h.unlock();
	requestData();
	return ret;
}
void TFileBySyncDev::invokeConnectionLost()
{
	QMtxHold h(p);
	TFBSD_Error *e;
	FXERRHM(e=new TFBSD_Error);
	FXRBOp une=FXRBNew(e);
	e->code=FXEXCEPTION_CONNECTIONLOST;
	e->message="Connection Lost";
	p->errors.append(e);
	une.dismiss();
	p->buffersReady.wakeAll();
}

QIODevice *TFileBySyncDev::source() const
{
	QMtxHold h(p);
	return p->source;
}
void TFileBySyncDev::setSource(QIODevice *source)
{
	QMtxHold h(p);
	p->source=source;
}

FXuval TFileBySyncDev::fetchData(FXuchar *data, FXuval amount, FXfval offset)
{
	assert(p->source);
	return p->source->readBlockFrom(data, amount, offset);
}
FXuval TFileBySyncDev::putData(FXfval offset, FXuchar *data, FXuval amount)
{
	assert(p->source);
	//fxmessage("Putting block of %u at offset %u\n", amount, (FXuval) offset);
	return p->source->writeBlockTo(offset, data, amount);
}

bool TFileBySyncDev::open(FXuint mode)
{
	if(isOpen())
	{	// I keep fouling myself up here, so assertion check
		if(QIODevice::mode()!=mode) FXERRGIO(QTrans::tr("TFileBySyncDev", "Device reopen has different mode"));
	}
	else
	{
		FXERRH(!(mode & IO_Append), "You cannot use IO_Append with TFileBySyncDev::open()", 0, FXERRH_ISDEBUG);
		QMtxHold h(p);
		setFlags((mode & IO_ModeMask)|IO_Open);
		ioIndex=0;
		h.unlock();
		requestData();
	}
	return true;
}
void TFileBySyncDev::close()
{
	if(isOpen())
	{
		flush();
		QMtxHold h(p);
		setFlags(0);
		invalidateBuffers(0, (FXfval)-1);
		ioIndex=0;
	}
}
void TFileBySyncDev::flush()
{
	if(isOpen())
	{
		QMtxHold h(p);
		checkErrors();
		TFileBySyncDevPrivate::Buffer &b=p->buffers.front();
		if(b.modified)
		{
			TFBSD_Data i(b.offset, b.data.data(), FXMIN(p->blockSize, (u32)b.len));
			p->ctrl->sendMsg(i);
			b.modified=false;
		}
		h.unlock();
		p->ctrl->device()->flush();
	}
}
FXfval TFileBySyncDev::size() const
{
	return (FXfval)-1;
}
void TFileBySyncDev::truncate(FXfval size)
{
	FXERRGNOTSUPP("Cannot truncate a TFileBySyncDev");
}
FXfval TFileBySyncDev::at() const
{
	return ioIndex;
}
bool TFileBySyncDev::at(FXfval newpos)
{
	if(isOpen() && ioIndex!=newpos)
	{
		QMtxHold h(p);
		checkErrors();
		TFileBySyncDevPrivate::Buffer &b=p->buffers.front();
		if(b.modified && (newpos<b.offset || newpos>=b.offset+p->blockSize))
			p->retireBuffer();
		FXfval pos=newpos;
		if(newpos>=b.offset && newpos<p->buffers.back().offset+p->blockSize)
		{	// Salvage some of the buffers
			while(true)
			{
				b=p->buffers.front();
				if(newpos<b.offset || newpos>=b.offset+p->blockSize)
					p->retireBuffer();
				else break;
			}
		}
		// Resync buffer offsets
		else for(QValueList<TFileBySyncDevPrivate::Buffer>::iterator it=p->buffers.begin(); it!=p->buffers.end(); ++it)
		{
			TFileBySyncDevPrivate::Buffer &b=*it;
			b.offset=pos; b.len=0; pos+=p->blockSize;
			b.empty=true; b.pending=false; assert(!b.modified);
#ifdef DEBUG
			memset(b.data.data(), 0, p->blockSize);
#endif
		}
		ioIndex=newpos;
		h.unlock();
		requestData();
	}
	return isOpen();
}
FXuval TFileBySyncDev::readBlock(char *data, FXuval maxlen)
{
	if(!isReadable()) FXERRGIO(QTrans::tr("TFileBySyncDev", "Not open for reading"));
	if(isOpen())
	{
		QMtxHold h(p);
		checkErrors();
		FXuval read=0;
		while(maxlen)
		{
			while(p->buffers.front().empty)
			{
				h.unlock();
				if(!p->buffers.front().pending)
					requestData();
				p->buffersReady.wait();
				h.relock();
				checkErrors();
			}
			TFileBySyncDevPrivate::Buffer &b=p->buffers.front();
			bool isMore=-1==b.len;
			FXuval bleft=FXMIN((FXuval)b.len, p->blockSize);
			FXuval boffset=(FXuval)(ioIndex-b.offset);
			if(!isMore && bleft-boffset==0) break;
			FXuval toread=(FXuval)FXMIN(bleft-boffset, maxlen);
			memcpy(data, b.data.data()+boffset, toread);
			ioIndex+=toread; data+=toread; maxlen-=toread; read+=toread;
			if(ioIndex==b.offset+bleft && -1==b.len)
			{
				p->retireBuffer();
				h.unlock();
				requestData();
				h.relock();
				checkErrors();
			}
		}
		return read;
	}
	return 0;
}
FXuval TFileBySyncDev::writeBlock(const char *data, FXuval maxlen)
{
	if(!isWriteable()) FXERRGIO(QTrans::tr("TFileBySyncDev", "Not open for writing"));
	if(isOpen())
	{
		QMtxHold h(p);
		checkErrors();
		FXuval written=0;
		while(maxlen)
		{
			while(p->buffers.front().empty)
			{
				h.unlock();
				if(!p->buffers.front().pending)
					requestData();
				p->buffersReady.wait();
				h.relock();
				checkErrors();
			}
			TFileBySyncDevPrivate::Buffer &b=p->buffers.front();
			assert(ioIndex>=b.offset && ioIndex<b.offset+p->blockSize);
			FXuval boffset=(FXuval)(ioIndex-b.offset);
			FXuval togo=FXMIN(p->blockSize-boffset, maxlen);
			memcpy(b.data.data()+boffset, data, togo);
			b.modified=true;
			if(b.len!=-1)
			{
				if(p->blockSize==(b.len=(s32)(boffset+togo)))
					b.len=-1;
			}
			ioIndex+=togo; data+=togo; maxlen-=togo; written+=togo;
			if(p->blockSize==boffset+togo)
			{
				p->retireBuffer();
				h.unlock();
				requestData();
				h.relock();
				TFileBySyncDevPrivate::Buffer &b=p->buffers.front();
				if(maxlen && b.empty)
				{
					b.empty=false;
					b.len=0;
				}
				checkErrors();
			}
		}
		return written;
	}
	return 0;
}
FXuval TFileBySyncDev::readBlockFrom(char *data, FXuval maxlen, FXfval pos)
{
	FXERRGNOTSUPP("TFileBySyncDev::readBlockFrom not supported");
	return 0;
}
FXuval TFileBySyncDev::writeBlockTo(FXfval pos, const char *data, FXuval maxlen)
{
	FXERRGNOTSUPP("TFileBySyncDev::writeBlockTo not supported");
	return 0;
}

int TFileBySyncDev::ungetch(int c)
{
	if(isOpen())
	{
		QMtxHold h(p);
		TFileBySyncDevPrivate::Buffer &b=p->buffers.front();
		FXuval boffset=(FXuval)(ioIndex-b.offset);
		if(!boffset) return -1;	// FIXME
		ioIndex--;
		b.data.data()[ioIndex-b.offset]=(FXuchar) c;
		return c;
	}
	return -1;
}

} // namespace

