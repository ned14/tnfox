/********************************************************************************
*                                                                               *
*                        i/o device working with memory                         *
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

#include <qcstring.h>
#include "QBuffer.h"
#include "FXException.h"
#include "QThread.h"
#include "QTrans.h"
#include "FXRollback.h"
#include "FXErrCodes.h"
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

struct FXDLLLOCAL QBufferPrivate : public QMutex
{
	bool mine;
	QByteArray *buffer;
	QBufferPrivate() : mine(false), buffer(0), QMutex() { }
};

QBuffer::QBuffer(FXuval len) : p(0), QIODevice()
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QBufferPrivate);
	if(len)
	{
		FXERRHM(p->buffer=new QByteArray(len));
		p->mine=true;
	}
	unconstr.dismiss();
}

QBuffer::QBuffer(QByteArray &buffer) : p(0), QIODevice()
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QBufferPrivate);
	setBuffer(buffer);
	unconstr.dismiss();
}

QBuffer::~QBuffer()
{ FXEXCEPTIONDESTRUCT1 {
	if(p)
	{
		close();
		if(p->mine) FXDELETE(p->buffer);
		FXDELETE(p);
	}
} FXEXCEPTIONDESTRUCT2; }

QByteArray &QBuffer::buffer() const
{
	if(!p->buffer)
	{
		QMtxHold h(p);
		if(!p->buffer)
		{
			FXERRHM(p->buffer=new QByteArray);
			p->mine=true;
		}
	}
	return *p->buffer;
}

void QBuffer::setBuffer(QByteArray &buffer)
{
	QMtxHold h(p);
	if(p->mine) FXDELETE(p->buffer);
	p->mine=false;
	p->buffer=&buffer;
}

bool QBuffer::open(FXuint mode)
{
	QMtxHold h(p);
	mode&=~IO_Translate;
	if(isOpen())
	{	// I keep fouling myself up here, so assertion check
		if(QIODevice::mode()!=mode) FXERRGIO(QTrans::tr("QBuffer", "Device reopen has different mode"));
	}
	else
	{
		if(!p->buffer)
		{
			if(!(mode & IO_WriteOnly))
			{
				FXERRGIO(QTrans::tr("QBuffer", "Buffer does not exist"));
			}
			FXERRHM(p->buffer=new QByteArray);
			p->mine=true;
		}
		if(mode & IO_Truncate) p->buffer->resize(0);
		setFlags((mode & IO_ModeMask)|IO_Open);
		ioIndex=(mode & IO_Append) ? p->buffer->size() : 0;
	}
	return true;
}

void QBuffer::close()
{
	QMtxHold h(p);
	if(isOpen())
	{
		ioIndex=0;
		setFlags(0);
	}
}

void QBuffer::flush()
{
}

FXfval QBuffer::size() const
{
	// QMtxHold h(p);	can do without
	return p->buffer->size();
}

void QBuffer::truncate(FXfval size)
{
	QMtxHold h(p);
	if(!isWriteable()) FXERRGIO(QTrans::tr("QBuffer", "Not open for writing"));
	if(isOpen())
	{
		if((mode() & IO_ShredTruncate) && size<p->buffer->size())
			shredData(size);
		if(ioIndex>size) at(size);
		p->buffer->truncate((FXuint) size);
	}
}

FXuval QBuffer::readBlock(char *data, FXuval maxlen)
{
	QMtxHold h(p);
	if(!isReadable()) FXERRGIO(QTrans::tr("QBuffer", "Not open for reading"));
	if(isOpen() && ioIndex<p->buffer->size())
	{
		FXuval left=(FXuval)(p->buffer->size()-ioIndex);
		FXuval read=FXMIN(left, maxlen);
		memcpy(data, &p->buffer->data()[ioIndex], read);
		ioIndex+=read;
		return read;
	}
	return 0;
}

FXuval QBuffer::writeBlock(const char *data, FXuval maxlen)
{
	QMtxHold h(p);
	if(!isWriteable()) FXERRGIO(QTrans::tr("QBuffer", "Not open for writing"));
	if(isOpen())
	{
		FXuval buffersize=p->buffer->size();
		FXuval left=(FXuval)(buffersize-ioIndex);
		if(left<maxlen || ioIndex>buffersize)
			p->buffer->resize(buffersize+maxlen-left);
		memcpy(&p->buffer->data()[ioIndex], data, maxlen);
		ioIndex+=maxlen;
		return maxlen;
	}
	return 0;
}

FXuval QBuffer::readBlockFrom(char *data, FXuval maxlen, FXfval pos)
{
	QMtxHold h(p);
	ioIndex=pos;
	return readBlock(data, maxlen);
}
FXuval QBuffer::writeBlockTo(FXfval pos, const char *data, FXuval maxlen)
{
	QMtxHold h(p);
	ioIndex=pos;
	return writeBlock(data, maxlen);
}


int QBuffer::getch()
{
	QMtxHold h(p);
	if(!isReadable()) FXERRGIO(QTrans::tr("QBuffer", "Not open for reading"));
	if(isOpen() && ioIndex<p->buffer->size())
	{
		FXuval left=(FXuval)(p->buffer->size()-ioIndex);
		if(left<1) return -1;
		return p->buffer->data()[ioIndex++];
	}
	return -1;
}

int QBuffer::putch(int c)
{
	QMtxHold h(p);
	if(!isWriteable()) FXERRGIO(QTrans::tr("QBuffer", "Not open for writing"));
	if(isOpen())
	{
		FXuval buffersize=p->buffer->size();
		FXuval left=(FXuval)(buffersize-ioIndex);
		if(left<1 || ioIndex>buffersize)
			p->buffer->resize(buffersize+1-left);
		p->buffer->data()[ioIndex++]=(char) c;
		return c;
	}
	return -1;
}

int QBuffer::ungetch(int c)
{
	QMtxHold h(p);
	if(isOpen())
	{
		if(0==ioIndex)
		{	// TODO: shuffle entire array downwards
			return -1;
		}
		p->buffer->data()[ioIndex--]=(char) c;
		return c;
	}
	return -1;
}

FXStream &operator<<(FXStream &s, const QBuffer &i)
{
	QMtxHold h(i.p);
	return s.writeRawBytes(i.buffer().data(), (FXuval) i.size());
}

FXStream &operator>>(FXStream &s, QBuffer &i)
{
	QMtxHold h(i.p);
	FXuval len=(FXuval) s.device()->size();
	FXERRH(len<((FXuint)-1), "Cannot read a file larger than a FXuint", QBUFFER_FILETOOBIG, FXERRH_ISDEBUG);
	i.buffer().resize((FXuint) len);
	i.at(0);
	return s.readRawBytes(i.buffer().data(), len);
}


}
