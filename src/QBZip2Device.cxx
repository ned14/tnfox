/********************************************************************************
*                                                                               *
*                          Filter device applying .bz2                          *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2006 by Niall Douglas.   All Rights Reserved.            *
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

#include "QBZip2Device.h"
#include "FXException.h"
#include "QBuffer.h"
#include "QThread.h"
#include "QTrans.h"
#include <qcstring.h>
#include "FXErrCodes.h"
#ifdef HAVE_BZ2LIB_H
#include "bzlib.h"
#endif
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

#ifdef HAVE_BZ2LIB_H

#define FXERRHBZ2(ret) { int _ret=(ret); if(_ret<0) FXERRGIO(decodeBZ2Err(_ret)); }

static const char *decodeBZ2Err(int code)
{
	switch(code)
	{
	case BZ_SEQUENCE_ERROR:
		return "BZip2 sequence error";
	case BZ_PARAM_ERROR:
		return "BZip2 parameter error";
	case BZ_MEM_ERROR:
		return "BZip2 failed to allocate memory";
	case BZ_DATA_ERROR:
		return "BZip2 data integrity failure";
	case BZ_DATA_ERROR_MAGIC:
		return "BZip2 bad magic header";
	case BZ_IO_ERROR:
		return "BZip2 i/o error";
	case BZ_UNEXPECTED_EOF:
		return "BZip2 unexpected end of file";
	case BZ_OUTBUFF_FULL:
		return "BZip2 output buffer full";
	case BZ_CONFIG_ERROR:
		return "BZip2 configuration error";
	}
	return "Unknown BZip2 error";
}
#endif

struct FXDLLLOCAL QBZip2DevicePrivate : public QMutex
{
	QIODevice *src;
	int compression;
	bool enableSeeking;
	QBuffer uncomp;
#ifdef HAVE_BZ2LIB_H
	char inbuffer[16384];
	bz_stream inh, outh;
#endif
	QBZip2DevicePrivate(QIODevice *_src, int _compression, bool _enableSeeking) : src(_src), compression(_compression), enableSeeking(_enableSeeking) { }
};

QBZip2Device::QBZip2Device(QIODevice *src, int compression, bool enableSeeking) : p(0)
{
	FXERRHM(p=new QBZip2DevicePrivate(src, compression, enableSeeking));
}

QBZip2Device::~QBZip2Device()
{ FXEXCEPTIONDESTRUCT1 {
	if(p)
	{
		close();
		FXDELETE(p);
	}
} FXEXCEPTIONDESTRUCT2; }

QIODevice *QBZip2Device::BZ2Data() const
{
	return p->src;
}

void QBZip2Device::setBZ2Data(QIODevice *src)
{
	p->src=src;
}

bool QBZip2Device::open(FXuint mode)
{
	QMtxHold h(p);
	if(isOpen())
	{	// I keep fouling myself up here, so assertion check
		if(QIODevice::mode()!=mode) FXERRGIO(QTrans::tr("QBZip2Device", "Device reopen has different mode"));
	}
	else
	{
#ifndef HAVE_BZ2LIB_H
		FXERRG("This TnFOX was built without bz2 support", QBZIP2DEVICE_NOBZIP2LIB, FXERRH_ISDEBUG);
#else
		//if(mode & IO_Truncate) FXERRG("Cannot truncate with this device", QBZIP2DEVICE_CANTTRUNCATE, FXERRH_ISDEBUG);
		FXERRH(p->src, "Need to set a source device before opening", QBZIP2DEVICE_MISSINGSOURCE, FXERRH_ISDEBUG);
		if(p->src->isClosed()) p->src->open(mode & ~IO_Translate);
		memset(&p->inh, 0, sizeof(p->inh));
		memset(&p->outh, 0, sizeof(p->outh));
		if(p->compression<1) p->compression=1;
		if(p->compression>9) p->compression=9;
		if(p->enableSeeking)
			p->uncomp.open(IO_ReadWrite);

		if(mode & IO_ReadOnly)
		{
			if(!p->src->atEnd())
			{
				int ret=BZ2_bzDecompressInit(&p->inh,
#ifdef DEBUG
					1,
#else
					0,
#endif
					0);
				FXERRHBZ2(ret);
				if(p->enableSeeking)
				{
					static const FXuval BlockSize=65536;
					FXuval offset=0;
					p->uncomp.buffer().resize(0);
					do
					{
						p->inh.next_in=p->inbuffer;
						p->inh.avail_in+=(FXuint)p->src->readBlock(p->inbuffer+p->inh.avail_in, sizeof(p->inbuffer)-p->inh.avail_in);
						p->uncomp.buffer().resize(offset+BlockSize);
						p->inh.next_out=(char *) p->uncomp.buffer().data()+offset;
						p->inh.avail_out=BlockSize;
						FXERRHBZ2(ret=BZ2_bzDecompress(&p->inh));
						memmove(p->inbuffer, p->inh.next_in, p->inh.avail_in);
						offset+=BlockSize-p->inh.avail_out;
					} while(BZ_STREAM_END!=ret);
					FXERRHBZ2(BZ2_bzDecompressEnd(&p->inh));
					p->uncomp.buffer().resize(offset);
					if(mode & IO_Translate)
					{
						if(!(mode & IO_NoAutoUTF))
						{
							FXuchar *data=p->uncomp.buffer().data();
							FXuval datasize=p->uncomp.buffer().size();
							// Have a quick peek to see what kind of text it is
							setUnicodeTranslation(determineUnicodeType(data, FXMIN(64, datasize)));
						}
						// Translate
						QByteArray temp(p->uncomp.buffer().size());
						temp.swap(p->uncomp.buffer());
						FXuval out=0, in=temp.size();
						for(FXuval idx=0; idx<temp.size(); )
						{
							in=temp.size()-idx;
							out+=removeCRLF(p->uncomp.buffer().data()+out, temp.data()+idx, p->uncomp.buffer().size()-out, in, unicodeTranslation());
							idx+=in;
							if(p->uncomp.buffer().size()-out<8)
								p->uncomp.buffer().resize(out+16384);
						}
						p->uncomp.buffer().resize(out);
					}
				}
			}
		}
		if((mode & IO_WriteOnly) && !p->enableSeeking)
		{
			int ret=BZ2_bzCompressInit(&p->outh, p->compression,
#ifdef DEBUG
				1,
#else
				0,
#endif
				0);
			FXERRHBZ2(ret);
		}
		setFlags((mode & IO_ModeMask)|IO_Open);
#endif
		ioIndex=0;
	}
	return true;
}

void QBZip2Device::close()
{
#ifdef HAVE_BZ2LIB_H
	QMtxHold h(p);
	if(isOpen())
	{
		flush();
		if(p->enableSeeking)
		{
			p->uncomp.truncate(0);
			p->uncomp.close();
		}
		else
		{
			if(isReadable())
				FXERRHBZ2(BZ2_bzDecompressEnd(&p->inh));
			if(isWriteable())
			{
				int ret;
				char outbuffer[16384];
				p->outh.avail_in=0;
				do
				{
					p->outh.next_out=outbuffer;
					p->outh.avail_out=sizeof(outbuffer);
					FXERRHBZ2(ret=BZ2_bzCompress(&p->outh, BZ_FINISH));
					p->src->writeBlock(outbuffer, p->outh.next_out-outbuffer);
				} while(BZ_STREAM_END!=ret);
				FXERRHBZ2(BZ2_bzCompressEnd(&p->outh));
			}
		}
		setFlags(0);
	}
#endif
}

void QBZip2Device::flush()
{
#ifdef HAVE_BZ2LIB_H
	QMtxHold h(p);
	if(isOpen() && isWriteable() && p->enableSeeking)
	{
		char *data=(char *) p->uncomp.buffer().data();
		FXuval datalen=p->uncomp.buffer().size();
		p->src->at(0);
		int ret=BZ2_bzCompressInit(&p->outh, p->compression,
#ifdef DEBUG
			1,
#else
			0,
#endif
			0);
		FXERRHBZ2(ret);
		char outbuffer[16384];
		for(FXuval offset=0; offset<datalen && ret!=BZ_STREAM_END;)
		{
			FXuval in=0;
			FXchar buffer[4096];
			if(isTranslated())
			{
				in=datalen-offset;
				p->outh.next_in=buffer;
				p->outh.avail_in=(FXuint)applyCRLF((FXuchar *) buffer, (FXuchar *) data+offset, sizeof(buffer), in, crlfFormat(), unicodeTranslation());
			}
			else
			{
				p->outh.next_in=data+offset;
				p->outh.avail_in=(FXuint)(datalen-offset);
			}
			do
			{
				p->outh.next_out=outbuffer;
				p->outh.avail_out=sizeof(outbuffer);
				FXERRHBZ2(ret=BZ2_bzCompress(&p->outh, BZ_RUN));
				p->src->writeBlock(outbuffer, p->outh.next_out-outbuffer);
			} while(p->outh.avail_in);
			// We must assume excellent compression of unicode text here
			offset=isTranslated() ? offset+in : p->outh.next_in-data;
		}
		do
		{
			p->outh.next_in=0;
			p->outh.next_out=outbuffer;
			p->outh.avail_out=sizeof(outbuffer);
			FXERRHBZ2(ret=BZ2_bzCompress(&p->outh, BZ_FINISH));
			p->src->writeBlock(outbuffer, p->outh.next_out-outbuffer);
		} while(ret!=BZ_STREAM_END);
		FXERRHBZ2(BZ2_bzCompressEnd(&p->outh));
		p->src->truncate(p->src->at());
		//p->src->flush();
	}
#endif
}

FXfval QBZip2Device::size() const
{
#ifdef HAVE_BZ2LIB_H
	QMtxHold h(p);
	if(p->enableSeeking)
		return p->uncomp.size();
	else
		return (FXfval)-1;
#else
	return 0;
#endif
}

void QBZip2Device::truncate(FXfval size)
{
#ifdef HAVE_BZ2LIB_H
	QMtxHold h(p);
	if(p->enableSeeking)
	{
		if((mode() & IO_ShredTruncate) && size<p->uncomp.size())
			shredData(size);
		p->uncomp.truncate(size);
	}
	else
		FXERRG(QTrans::tr("QBZip2Device", "Seeking not enabled"), QBZIP2DEVICE_NOTSEEKABLE, 0);
#endif
}

FXfval QBZip2Device::at() const
{
#ifdef HAVE_BZ2LIB_H
	QMtxHold h(p);
	if(p->enableSeeking)
		return p->uncomp.at();
	else
		return ioIndex;
#else
	return 0;
#endif
}

bool QBZip2Device::at(FXfval newpos)
{
#ifdef HAVE_BZ2LIB_H
	QMtxHold h(p);
	if(p->enableSeeking)
		return p->uncomp.at(newpos);
	else
		FXERRG(QTrans::tr("QBZip2Device", "Seeking not enabled"), QBZIP2DEVICE_NOTSEEKABLE, 0);
#endif
	return false;
}

bool QBZip2Device::atEnd() const
{
#ifdef HAVE_BZ2LIB_H
	QMtxHold h(p);
	if(p->enableSeeking)
		return p->uncomp.atEnd();
	else
		FXERRG(QTrans::tr("QBZip2Device", "Seeking not enabled"), QBZIP2DEVICE_NOTSEEKABLE, 0);
#endif
	return true;
}

FXuval QBZip2Device::readBlock(char *data, FXuval maxlen)
{
#ifdef HAVE_BZ2LIB_H
	QMtxHold h(p);
	if(p->enableSeeking)
		return p->uncomp.readBlock(data, maxlen);
	int ret;
	p->inh.next_out=data;
	p->inh.avail_out=(FXuint)maxlen;
	do
	{
		p->inh.next_in=p->inbuffer;
		p->inh.avail_in+=(FXuint)p->src->readBlock(p->inbuffer+p->inh.avail_in, sizeof(p->inbuffer)-p->inh.avail_in);
		FXERRHBZ2(ret=BZ2_bzDecompress(&p->inh));
		memmove(p->inbuffer, p->inh.next_in, p->inh.avail_in);
	} while(BZ_STREAM_END!=ret && p->inh.avail_out);
	ioIndex+=maxlen-p->inh.avail_out;
	return maxlen-p->inh.avail_out;
#else
	return 0;
#endif
}

FXuval QBZip2Device::writeBlock(const char *data, FXuval maxlen)
{
#ifdef HAVE_BZ2LIB_H
	QMtxHold h(p);
	if(p->enableSeeking)
		return p->uncomp.writeBlock(data, maxlen);
	int ret;
	char outbuffer[16384];
	p->outh.next_in=(char *) data;
	p->outh.avail_in=(FXuint)maxlen;
	do
	{
		p->outh.next_out=outbuffer;
		p->outh.avail_out=sizeof(outbuffer);
		FXERRHBZ2(ret=BZ2_bzCompress(&p->outh, BZ_RUN));
		p->src->writeBlock(outbuffer, p->outh.next_out-outbuffer);
	} while(BZ_STREAM_END!=ret && p->outh.avail_in);
	ioIndex+=maxlen-p->outh.avail_in;
	return maxlen-p->outh.avail_in;
#else
	return 0;
#endif
}

FXuval QBZip2Device::readBlockFrom(char *data, FXuval maxlen, FXfval pos)
{
#ifdef HAVE_BZ2LIB_H
	QMtxHold h(p);
	if(p->enableSeeking)
	{
		p->uncomp.at(pos);
		return p->uncomp.readBlock(data, maxlen);
	}
	FXERRG(QTrans::tr("QBZip2Device", "Seeking not enabled"), QBZIP2DEVICE_NOTSEEKABLE, 0);
#endif
	return 0;
}
FXuval QBZip2Device::writeBlockTo(FXfval pos, const char *data, FXuval maxlen)
{
#ifdef HAVE_BZ2LIB_H
	QMtxHold h(p);
	if(p->enableSeeking)
	{
		p->uncomp.at(pos);
		return p->uncomp.writeBlock(data, maxlen);
	}
	FXERRG(QTrans::tr("QBZip2Device", "Seeking not enabled"), QBZIP2DEVICE_NOTSEEKABLE, 0);
#endif
	return 0;
}

int QBZip2Device::getch()
{
#ifdef HAVE_BZ2LIB_H
	QMtxHold h(p);
	if(p->enableSeeking)
		return p->uncomp.getch();
	char ret;
	return (1==readBlock(&ret, 1)) ? ret : -1;
#else
	return -1;
#endif
}

int QBZip2Device::putch(int c)
{
#ifdef HAVE_BZ2LIB_H
	QMtxHold h(p);
	if(p->enableSeeking)
		return p->uncomp.putch(c);
	char _c=(char) c;
	return (1==writeBlock(&_c, 1)) ? _c : -1;
#else
	return -1;
#endif
}

int QBZip2Device::ungetch(int c)
{
#ifdef HAVE_BZ2LIB_H
	QMtxHold h(p);
	if(p->enableSeeking)
		return p->uncomp.ungetch(c);
	FXERRG(QTrans::tr("QBZip2Device", "Seeking not enabled"), QBZIP2DEVICE_NOTSEEKABLE, 0);
#endif
	return -1;
}

} // namespace
