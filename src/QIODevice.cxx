/********************************************************************************
*                                                                               *
*                                Base i/o device                                *
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

#include "QIODeviceS.h"
#include "FXStream.h"
#include "FXACL.h"
#include "FXException.h"
#include "QTrans.h"
#include "QThread.h"
#include "FXSecure.h"
#include "QBuffer.h"
#include <qcstring.h>
#include <string.h>
#include "FXErrCodes.h"
#ifdef USE_POSIX
#include <unistd.h>
#include "tnfxselect.h"
#else
#include "WindowsGubbins.h"
#endif
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

FXfval QIODevice::at() const
{
	return ioIndex;
}

bool QIODevice::at(FXfval newpos)
{
	ioIndex=newpos;
	return true;
}

bool QIODevice::atEnd() const
{
	return at()>=size();
}

const FXACL &QIODevice::permissions() const
{
	static QMutex lock;
	static FXACL perms;
	QMtxHold lh(lock);
	if(perms.count()) return perms;
	// Fake a default
	perms.append(FXACL::Entry(FXACLEntity::owner(), 0, FXACL::Permissions().setAll()));
	return perms;
}
void QIODevice::setPermissions(const FXACL &perms)
{
	FXERRG(QTrans::tr("QIODevice", "You cannot set the permissions on this device"), QIODEVICE_BADPERMISSIONS, 0);
}

FXuval QIODevice::readLine(char *data, FXuval maxlen)
{
	FXuval count=0;
	int c;
	do
	{
		c=getch();
		if(c>=0) data[count]=(char) c;
	} while(-1!=c && data[count++]!='\n');
	data[count]=0;
	return count;
}

int QIODevice::getch()
{
	char ret=0;
	if(readBlock(&ret, 1))
		return (int) ret;
	else
		return -1;
}

int QIODevice::putch(int c)
{
	char val=(char) c;
	if(writeBlock(&val, 1))
		return c;
	else
		return -1;
}



QIODevice::UnicodeType QIODevice::determineUnicodeType(FXuchar *data, FXuval len) throw()
{
#if 1
	// Always returns NoTranslation on v1.4.x based versions
	return NoTranslation;
#else
	QByteArray _data(data, len, true);
	QBuffer dev(_data);
	FXStream s(&dev);
	char buffer[12];
	int goods[6], bads[6];
	dev.open(IO_ReadOnly);
	for(int u=UTF32LE; u!=NoTranslation; u--)
	{
		int &good=goods[u], &bad=bads[u];
		FXuval inc=u & 6;
		if(!inc) inc=1;
		good=bad=0;
		s.setBigEndian(!(u & 1));
		for(FXuval n=0; n<len-8; n+=inc)
		{
			dev.at(n);
			if(4==inc)
			{
				FXwchar *c=(FXwchar *) buffer;
				s >> c[0];
				// By UTF-32 spec the top eleven bits must be clear
				if(c[0] & 0xFFE00000) bad+=inc; else good+=inc;
			}
			else if(2==inc)
			{
				FXushort *c=(FXushort *) buffer;
				s >> c[0] >> c[1];
				if(!c[0] || !isutfvalid((FXnchar *) c))
					bad+=inc;
				else
				{
					FXint len=wcinc((FXnchar *)(data+n), 0);
					good+=inc;
					if(len>2)
						good+=inc*4;
					if(!(c[0] & 0xff00))	// Prefer high byte empty
						good+=1;
					n+=len-inc;
				}
			}
			else
			{
				if(!data[n] || !isutfvalid((char *) data+n))
					bad+=inc;
				else
				{
					FXint len=wcinc((char *) data+n, 0);
					good+=inc;
					if(len>1)
						good+=inc<<(1+len);
					n+=len-inc;
				}
			}
		}
		//if(!bad) return (UnicodeType) u;
	}
	UnicodeType ret=NoTranslation;
	goods[ret]=0;
	bads[ret]=Generic::BiggestValue<int>::value;
	// Choose that with the lowest bads
	for(int n=0; n<=UTF32LE; n++)
	{
		if((double)bads[n]/(goods[n]+0.0000001)<0.1 && bads[n]<=bads[ret] && goods[n]>goods[ret]) ret=(QIODevice::UnicodeType) n;
	}
#ifdef DEBUG
	static const char *utfs[]={ "NoTranslation", "UTF8", "UTF16BE", "UTF16LE", "UTF32BE", "UTF32LE" };
	fxmessage("QIODevice::determineUnicodeType(%p, %u) returns %s\n", data, (FXuint) len, utfs[ret]);
#endif
	return ret;
#endif
}

FXuval QIODevice::applyCRLF(FXuchar *output, const FXuchar *input, FXuval outputlen, FXuval &inputlen, QIODevice::CRLFType crlftype, QIODevice::UnicodeType utftype)
{
	if(Default==crlftype)
#ifdef WIN32
		crlftype=MSDOS;
#elif defined(USE_MACOSX)
		crlftype=MacOS;
#elif defined(USE_POSIX)
		crlftype=Unix;
#else
#error Unknown system
#endif
	if(Unix==crlftype && NoTranslation==utftype)
	{
		inputlen=FXMIN(outputlen, inputlen);
		memcpy(output, input, inputlen);
		return inputlen;
	}
	FXuval o=0, i=0;
	for(;;)
	{
		FXint inlen=1, outlen=1;
		/*if(UTF16 & utftype)
		{
			inlen=wclen((FXchar *) input+i);
			outlen=ncslen((FXchar *) input+i, inlen);
		}
		else if(UTF32 & utftype)
		{
			inlen=wclen((FXchar *) input+i);
			outlen=sizeof(FXwchar);
		}*/
		if(i+inlen>inputlen) break;
		if(o+outlen>outputlen) break;
		if(Unix!=crlftype && 10==input[i])
		{
			if(NoTranslation==utftype || UTF8==utftype)
				output[o]=13;
			/*else if(UTF16 & utftype)
			{
				FXnchar *t=(FXnchar *)(output+o);
				*t=13;
				if(FOX_BIGENDIAN==(utftype & 1))
					fxendianswap2(t);
			}
			else if(UTF32==utftype)
			{
				FXwchar *t=(FXwchar *)(output+o);
				*t=13;
				if(FOX_BIGENDIAN==(utftype & 1))
					fxendianswap4(t);
			}*/
			if(MSDOS==crlftype)
			{
				if(o+outlen+outlen>outputlen)
					break;
				o+=outlen;
				if(NoTranslation==utftype || UTF8==utftype)
					output[o]=10;
				/*else if(UTF16 & utftype)
				{
					FXnchar *t=(FXnchar *)(output+o);
					*t=10;
					if(FOX_BIGENDIAN==(utftype & 1))
						fxendianswap2(t);
				}
				else if(UTF32 & utftype)
				{
					FXwchar *t=(FXwchar *)(output+o);
					*t=10;
					if(FOX_BIGENDIAN==(utftype & 1))
						fxendianswap4(t);
				}*/
			}
		}
		/*else if(UTF16 & utftype)
		{
			FXnchar *t=(FXnchar *)(output+o);
			utf2ncs(t, (FXchar *) input+i, inlen);
			if(FOX_BIGENDIAN==(utftype & 1))
			{
				fxendianswap2(t);
				if(outlen>2)
					fxendianswap2(t+1);
			}
		}
		else if(UTF32 & utftype)
		{
			FXwchar *t=(FXwchar *)(output+o);
			*t=wc((FXchar *) input+i);
			if(FOX_BIGENDIAN==(utftype & 1))
				fxendianswap4(t);
		}*/
		else output[o]=input[i];
		i+=inlen; o+=outlen;
	}
	inputlen=i;
	return o;
}

FXuval QIODevice::removeCRLF(FXuchar *output, const FXuchar *input, FXuval outputlen, FXuval &inputlen, QIODevice::UnicodeType utftype)
{
	FXuval o=0, i=0;
	FXwchar thischar=0, nextchar=0;
	assert(output!=input);		// No longer safe unlike previous versions of this function
	for( ; o<outputlen && i<inputlen; )
	{
		FXint thischarlen=0;
		if(NoTranslation==utftype || UTF8==utftype)
		{
			thischarlen=1;
			thischar=input[i];
			nextchar=input[i+1];
		}
		/*else if(UTF16 & utftype)
		{
			FXnchar temp[2];
			temp[0]=((FXnchar *)(input+i))[0];
			temp[1]=((FXnchar *)(input+i))[1];
			if(FOX_BIGENDIAN==(utftype & 1))
			{
				fxendianswap2(&temp[0]);
				fxendianswap2(&temp[1]);
			}
			thischarlen=wclen(temp)*sizeof(FXnchar);
			if(i+thischarlen>inputlen) break;
			thischar=wc(temp);
			nextchar=wc(temp+1);
		}
		else if(UTF32 & utftype)
		{
			thischar=((FXwchar *)(input+i))[0];
			nextchar=((FXwchar *)(input+i))[1];
			if(FOX_BIGENDIAN==(utftype & 1))
			{
				fxendianswap4(&thischar);
				fxendianswap4(&nextchar);
			}
			thischarlen=sizeof(FXwchar);
			if(i+thischarlen>inputlen) break;
		}*/

		if(13==thischar)
		{
			if(i+thischarlen>=inputlen)
				break;
			if(10==nextchar)
			{	// Gobble the next char
				i+=thischarlen;
			}
			output[o++]=10;
		}
		/*else if((UTF16|UTF32) & utftype)
		{
			FXint towrite=utfslen(&thischar, 1);
			if(o+towrite>outputlen) break;
			o+=wc2utfs((FXchar *) output+o, &thischar, 1);
		}*/
		else
			output[o++]=thischar;
		i+=thischarlen;
	}
	inputlen=i;
	return o;
}

FXfval QIODevice::shredData(FXfval offset, FXfval len)
{
	FXfval cpos=at(), idx;
	FXuchar buffer[16384];
	FXuint seed=0;
	FXuval read;
	memset(buffer, 0, sizeof(buffer));
	at(offset);
	for(idx=0; idx<len; idx+=read)
	{
		FXfval pos=at();
		if(!(read=readBlock(buffer, (FXuval) FXMIN(sizeof(buffer), len-idx)))) break;
		at(pos);
		Secure::PRandomness::readBlock((FXuchar *) &seed, sizeof(FXuint));
		FXuval bufflen=read/sizeof(FXuint)+(read & 3) ? 1 : 0;
		for(FXuval n=0; n<bufflen; n++)
		{
			buffer[n]^=fxrandom(seed);
		}
		writeBlock(buffer, read);
		flush();
	}
	len=idx;
	at(offset);
	memset(buffer, 0, sizeof(buffer));
	for(idx=0; idx<len; idx+=sizeof(buffer))
	{
		writeBlock(buffer, FXMIN(sizeof(buffer), (FXuval)(len-idx)));
		flush();
	}
	at(cpos);
	return len;
}

FXStream &operator<<(FXStream &s, QIODevice &i)
{
	FXfval currentpos=i.at();
	char buffer[256*1024];
	FXuval read;
	i.at(0);
	while((read=i.readBlock(buffer, sizeof(buffer))))
	{
		s.writeRawBytes(buffer, read);
	}
	i.at(currentpos);
	return s;
}

FXStream &operator>>(FXStream &s, QIODevice &i)
{
	char buffer[256*1024];
	FXuval read;
	QIODevice *sdev=s.device();
	i.at(0);
	while((read=sdev->readBlock(buffer, sizeof(buffer))))
	{
		i.writeBlock(buffer, read);
	}
	i.truncate(i.at());
	i.at(0);
	return s;
}

//***************************************************************************************

FXuval QIODeviceS::readBlockFrom(char *data, FXuval maxlen, FXfval pos)
{
	FXERRGNOTSUPP("readBlockFrom not supported for synchronous i/o devices");
}

FXuval QIODeviceS::writeBlockTo(FXfval pos, const char *data, FXuval maxlen)
{
	FXERRGNOTSUPP("readBlockFrom not supported for synchronous i/o devices");
}

bool QIODeviceS::waitForData(QIODeviceS **signalled, FXuint no, QIODeviceS **list, FXuint waitfor)
{
	if(signalled) signalled[0]=0;
#ifndef USE_POSIX
	HANDLE hlist[MAXIMUM_WAIT_OBJECTS];
	FXERRH(no<MAXIMUM_WAIT_OBJECTS, "MAXIMUM_WAIT_OBJECTS exceeded", 0, FXERRH_ISDEBUG);
	for(FXuint n=0; n<no; n++)
	{
		hlist[n]=list[n]->int_getOSHandle();
		FXERRH(hlist[n], QTrans::tr("QIODeviceS", "Either i/o device is not open or not supported"), 0, FXERRH_ISDEBUG);
	}
	hlist[no]=QThread::int_cancelWaiterHandle();
	DWORD ret=WaitForMultipleObjects(no+1, hlist, FALSE, (waitfor==FXINFINITE) ? INFINITE : waitfor);
	if(ret==WAIT_TIMEOUT) return false;
	if(WAIT_OBJECT_0+no==ret)
	{
		QThread::current()->checkForTerminate();
		return false;
	}
	if(ret>=WAIT_OBJECT_0 && ret<WAIT_OBJECT_0+no)
	{
		if(signalled)
		{
			FXuint oidx=0;
			for(FXuint n=0; n<no; n++)
			{
				if(WAIT_OBJECT_0==WaitForSingleObject(hlist[n], 0))
					signalled[oidx++]=list[n];
			}
			signalled[oidx]=0;
		}
		return true;
	}
	FXERRHWIN(ret);
	return false;
#else
	struct ::timeval *tv=0, _tv;
	if(waitfor!=FXINFINITE)
	{
		_tv.tv_sec=waitfor/1000;
		_tv.tv_usec=(waitfor % 1000)*1000;
		tv=&_tv;
	}
	fd_set fds;
	FD_ZERO(&fds);
	FXERRH(no<=FD_SETSIZE, "FD_SETSIZE exceeded", 0, FXERRH_ISDEBUG);
	int maxfd=0;
	for(FXuint n=0; n<no; n++)
	{
		int fd=(int)(FXuval) list[n]->int_getOSHandle();
		FXERRH(fd, QTrans::tr("QIODeviceS", "Either i/o device is not open or not supported"), 0, FXERRH_ISDEBUG);
		FD_SET(fd, &fds);
		if(fd>maxfd) maxfd=fd;
	}
	int ret=tnfxselect(maxfd+1, &fds, 0, 0, tv);
	if(ret<0) { FXERRHOS(ret); return false; }
	if(!ret) return false;
	if(signalled)
	{
		FXuint oidx=0;
		for(FXuint n=0; n<no; n++)
		{
			int fd=(int)(FXuval) list[n]->int_getOSHandle();
			if(FD_ISSET(fd, &fds))
				signalled[oidx++]=list[n];
		}
		signalled[oidx]=0;
	}
	return true;
#endif
}
FXuint QIODeviceS::waitForDataMax() throw()
{
#ifndef USE_POSIX
	return MAXIMUM_WAIT_OBJECTS-1;
#else
	return FD_SETSIZE;
#endif
}

} // namespace

