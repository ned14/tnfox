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
#include "FXBuffer.h"
#include "FXException.h"
#include "FXThread.h"
#include "FXTrans.h"
#include "FXRollback.h"
#include "FXErrCodes.h"
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

struct FXDLLLOCAL FXBufferPrivate : public FXMutex
{
	bool mine;
	QByteArray *buffer;
	FXBufferPrivate() : mine(false), buffer(0), FXMutex() { }
};

FXBuffer::FXBuffer(FXuval len) : p(0), FXIODevice()
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new FXBufferPrivate);
	if(len)
	{
		FXERRHM(p->buffer=new QByteArray(len));
		p->mine=true;
	}
	unconstr.dismiss();
}

FXBuffer::FXBuffer(QByteArray &buffer) : p(0), FXIODevice()
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new FXBufferPrivate);
	setBuffer(buffer);
	unconstr.dismiss();
}

FXBuffer::~FXBuffer()
{ FXEXCEPTIONDESTRUCT1 {
	if(p)
	{
		close();
		if(p->mine) FXDELETE(p->buffer);
		FXDELETE(p);
	}
} FXEXCEPTIONDESTRUCT2; }

QByteArray &FXBuffer::buffer() const
{
	if(!p->buffer)
	{
		FXMtxHold h(p);
		if(!p->buffer)
		{
			FXERRHM(p->buffer=new QByteArray);
			p->mine=true;
		}
	}
	return *p->buffer;
}

void FXBuffer::setBuffer(QByteArray &buffer)
{
	FXMtxHold h(p);
	if(p->mine) FXDELETE(p->buffer);
	p->mine=false;
	p->buffer=&buffer;
}

bool FXBuffer::open(FXuint mode)
{
	FXMtxHold h(p);
	mode&=~IO_Translate;
	if(isOpen())
	{	// I keep fouling myself up here, so assertion check
		if(FXIODevice::mode()!=mode) FXERRGIO(FXTrans::tr("FXBuffer", "Device reopen has different mode"));
	}
	else
	{
		if(!p->buffer)
		{
			if(!(mode & IO_WriteOnly))
			{
				FXERRGIO(FXTrans::tr("FXBuffer", "Buffer does not exist"));
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

void FXBuffer::close()
{
	FXMtxHold h(p);
	if(isOpen())
	{
		ioIndex=0;
		setFlags(0);
	}
}

void FXBuffer::flush()
{
}

FXfval FXBuffer::size() const
{
	// FXMtxHold h(p);	can do without
	return p->buffer->size();
}

void FXBuffer::truncate(FXfval size)
{
	FXMtxHold h(p);
	if(!isWriteable()) FXERRGIO(FXTrans::tr("FXBuffer", "Not open for writing"));
	if(isOpen())
	{
		if((mode() & IO_ShredTruncate) && size<p->buffer->size())
			shredData(size);
		if(ioIndex>size) at(size);
		p->buffer->truncate((FXuint) size);
	}
}

FXuval FXBuffer::readBlock(char *data, FXuval maxlen)
{
	FXMtxHold h(p);
	if(!isReadable()) FXERRGIO(FXTrans::tr("FXBuffer", "Not open for reading"));
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

FXuval FXBuffer::writeBlock(const char *data, FXuval maxlen)
{
	FXMtxHold h(p);
	if(!isWriteable()) FXERRGIO(FXTrans::tr("FXBuffer", "Not open for writing"));
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

FXuval FXBuffer::readBlockFrom(char *data, FXuval maxlen, FXfval pos)
{
	FXMtxHold h(p);
	ioIndex=pos;
	return readBlock(data, maxlen);
}
FXuval FXBuffer::writeBlockTo(FXfval pos, const char *data, FXuval maxlen)
{
	FXMtxHold h(p);
	ioIndex=pos;
	return writeBlock(data, maxlen);
}


int FXBuffer::getch()
{
	FXMtxHold h(p);
	if(!isReadable()) FXERRGIO(FXTrans::tr("FXBuffer", "Not open for reading"));
	if(isOpen() && ioIndex<p->buffer->size())
	{
		FXuval left=(FXuval)(p->buffer->size()-ioIndex);
		if(left<1) return -1;
		return p->buffer->data()[ioIndex++];
	}
	return -1;
}

int FXBuffer::putch(int c)
{
	FXMtxHold h(p);
	if(!isWriteable()) FXERRGIO(FXTrans::tr("FXBuffer", "Not open for writing"));
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

int FXBuffer::ungetch(int c)
{
	FXMtxHold h(p);
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

FXStream &operator<<(FXStream &s, const FXBuffer &i)
{
	FXMtxHold h(i.p);
	return s.writeRawBytes(i.buffer().data(), (FXuval) i.size());
}

FXStream &operator>>(FXStream &s, FXBuffer &i)
{
	FXMtxHold h(i.p);
	FXuval len=(FXuval) s.device()->size();
	FXERRH(len<((FXuint)-1), "Cannot read a file larger than a FXuint", FXBUFFER_FILETOOBIG, FXERRH_ISDEBUG);
	i.buffer().resize((FXuint) len);
	i.at(0);
	return s.readRawBytes(i.buffer().data(), len);
}


}
