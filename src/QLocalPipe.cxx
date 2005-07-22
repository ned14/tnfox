/********************************************************************************
*                                                                               *
*                            Local pipe i/o device                              *
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

#include <qptrlist.h>
#include "QLocalPipe.h"
#include "QThread.h"
#include "FXException.h"
#include "QTrans.h"
#include "FXRollback.h"
#include "FXMemDbg.h"
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

// Defines the buffer chunk size
#define CHUNKSIZE (64*1024)

struct Buffer
{
	FXuval rptr, wptr;
	int datachunks;
	QPtrList<FXuchar> data;
	FXAtomicInt newdatawaiters;
	QWaitCondition newdata, empty;
	Buffer() : rptr(0), wptr(0), datachunks(0), newdata(true)
	{
		newChunk(CHUNKSIZE);
	}
	void newChunk(FXuval chunksize)
	{
		FXuchar *chunk;
		FXERRHM(chunk=(FXuchar *) malloc(chunksize));
		FXRBOp unalloc=FXRBAlloc(chunk);
		data.append(chunk);
		unalloc.dismiss();
		datachunks++;
	}
	void delChunk()
	{
		free(data.first());
		data.removeFirst();
		datachunks--;
	}
	~Buffer()
	{
		for(QPtrListIterator<FXuchar> it(data); it.current(); ++it)
			free(it.current());
		data.clear();
	}
};
#define MAGIC (*(FXuint *)"LCPI")
struct FXDLLLOCAL QLocalPipePrivate : public QMutex
{
	FXuint magic;
	FXAtomicInt clients;
	Buffer A, B;
	FXuval granularity;
	QLocalPipePrivate() : magic(MAGIC), granularity(CHUNKSIZE), QMutex() { }
	~QLocalPipePrivate() { magic=0; }
	Buffer &readBuffer(QLocalPipe *t) { return (t->creator) ? B : A; }
	Buffer &writeBuffer(QLocalPipe *t) { return (t->creator) ? A : B; }
};

void *QLocalPipe::int_getOSHandle() const
{
	return 0;
}

QLocalPipe::QLocalPipe() : p(0), creator(true), QIODeviceS()
{
	FXERRHM(p=new QLocalPipePrivate);
}

// Creates the client end
QLocalPipe::QLocalPipe(const QLocalPipe &o) : p(o.p), creator(false), QIODeviceS(o)
{
	setFlags(0);
}

QLocalPipe::~QLocalPipe()
{ FXEXCEPTIONDESTRUCT1 {
	close();
	if(creator) { FXDELETE(p); }
	else p=0;
} FXEXCEPTIONDESTRUCT2; }

FXuval QLocalPipe::granularity() const
{
	if(p && MAGIC==p->magic)
	{
		FXMtxHold h(p);
		return p->granularity;
	}
	return 0;
}

void QLocalPipe::setGranularity(FXuval newval)
{
	if(p && MAGIC==p->magic && isClosed())
	{
		FXMtxHold h(p);
		p->granularity=newval;
		p->A.delChunk();
		p->B.delChunk();
		p->A.newChunk(p->granularity);
		p->B.newChunk(p->granularity);
	}
}

bool QLocalPipe::open(FXuint mode)
{
	FXMtxHold h(p);
	if(isOpen())
	{	// I keep fouling myself up here, so assertion check
		if(QIODevice::mode()!=mode) FXERRGIO(QTrans::tr("QLocalPipe", "Device reopen has different mode"));
	}
	else
	{
		setFlags((mode & IO_ModeMask)|IO_Open);
		++p->clients;
	}
	return true;
}

void QLocalPipe::close()
{
	if(p && MAGIC==p->magic && isOpen())
	{
		FXMtxHold h(p);
		Buffer &b=p->writeBuffer(this);
		b.data.clear();
		b.rptr=b.wptr=0;
		--p->clients;
	}
	setFlags(0);
}

void QLocalPipe::flush()
{
	if(isOpen() && isWriteable())
	{
		if(!p || MAGIC!=p->magic) FXERRGCONLOST("Connection Lost", 0);
		p->writeBuffer(this).empty.wait();
	}
}

FXfval QLocalPipe::size() const
{
	if(isOpen())
	{
		if(!p || MAGIC!=p->magic) return 0;
		FXMtxHold h(p);
		Buffer &b=p->readBuffer(const_cast<QLocalPipe *>(this));
		FXfval waiting=(b.datachunks-1)*p->granularity;
		waiting+=b.wptr-b.rptr;
		return waiting;
	}
	return 0;
}

void QLocalPipe::truncate(FXfval) { }
FXfval QLocalPipe::at() const { return 0; }
bool QLocalPipe::at(FXfval) { return false; }
bool QLocalPipe::atEnd() const { return size()==0; }

FXuval QLocalPipe::readBlock(char *data, FXuval maxlen)
{
	FXMtxHold h(p);
	if(!isReadable()) FXERRGIO(QTrans::tr("QLocalPipe", "Not open for reading"));
	if(isOpen() && maxlen)
	{
		if(!p || MAGIC!=p->magic || p->clients<=1) FXERRGCONLOST("Connection Lost", 0);
		Buffer &b=p->readBuffer(this);
		FXuval waiting;
		bool repeat;
		do
		{
			repeat=false;
			waiting=(b.datachunks-1)*p->granularity;
			waiting+=b.wptr-b.rptr;
			if(waiting<maxlen)
			{
				b.newdata.reset();
				++b.newdatawaiters;
				h.unlock();
				repeat=b.newdata.wait((!waiting) ? FXINFINITE : 0);
				h.relock();
				--b.newdatawaiters;
			}
		} while(repeat);
		FXuval readed=FXMIN(maxlen, waiting);
		for(FXuval n=0; n<readed;)
		{
			size_t len=FXMIN(readed-n, (p->granularity-b.rptr));
			memcpy(data+n, b.data.first()+b.rptr, len);
			b.rptr+=len; n+=len;
			if(p->granularity==b.rptr)
			{
				b.delChunk();
				b.rptr=0;
			}
		}
		if(1==b.datachunks && b.rptr==b.wptr)
		{
			b.rptr=b.wptr=0;
			if(!b.empty.signalled()) b.empty.wakeAll();
		}
		return readed;
	}
	return 0;
}

FXuval QLocalPipe::writeBlock(const char *data, FXuval maxlen)
{
	FXMtxHold h(p);
	if(!isWriteable()) FXERRGIO(QTrans::tr("QLocalPipe", "Not open for writing"));
	if(isOpen() && maxlen)
	{
		if(!p || MAGIC!=p->magic) FXERRGCONLOST("Connection Lost", 0);
		Buffer &b=p->writeBuffer(this);
		for(FXuval n=0; n<maxlen;)
		{
			FXuval len=FXMIN(maxlen, (p->granularity-b.wptr));
			memcpy(b.data.last()+b.wptr, data+n, len);
			b.wptr+=len; n+=len;
			if(p->granularity==b.wptr)
			{
				b.newChunk(p->granularity);
				b.wptr=0;
			}
		}
		if(b.empty.signalled()) b.empty.reset();
		if(b.newdatawaiters) b.newdata.wakeAll();
		if(isRaw()) flush();
		return maxlen;
	}
	return 0;
}

int QLocalPipe::ungetch(int c)
{
	return -1;
}

}
