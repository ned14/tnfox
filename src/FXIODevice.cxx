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

#include "FXIODeviceS.h"
#include "FXStream.h"
#include "FXACL.h"
#include "FXException.h"
#include "FXTrans.h"
#include "FXThread.h"
#include "FXSecure.h"
#include <string.h>
#include "FXErrCodes.h"
#ifdef USE_POSIX
#include <unistd.h>
#else
#include "WindowsGubbins.h"
#endif
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

FXfval FXIODevice::at() const
{
	return ioIndex;
}

bool FXIODevice::at(FXfval newpos)
{
	ioIndex=newpos;
	return true;
}

bool FXIODevice::atEnd() const
{
	return at()>=size();
}

const FXACL &FXIODevice::permissions() const
{
	static FXMutex lock;
	static FXACL perms;
	FXMtxHold lh(lock);
	if(perms.count()) return perms;
	// Fake a default
	perms.append(FXACL::Entry(FXACLEntity::owner(), 0, FXACL::Permissions().setAll()));
	return perms;
}
void FXIODevice::setPermissions(const FXACL &perms)
{
	FXERRG(FXTrans::tr("FXIODevice", "You cannot set the permissions on this device"), FXIODEVICE_BADPERMISSIONS, 0);
}

FXuval FXIODevice::readLine(char *data, FXuval maxlen)
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

int FXIODevice::getch()
{
	char ret=0;
	if(readBlock(&ret, 1))
		return (int) ret;
	else
		return -1;
}

int FXIODevice::putch(int c)
{
	char val=(char) c;
	if(writeBlock(&val, 1))
		return c;
	else
		return -1;
}

FXuval FXIODevice::applyCRLF(bool &midNL, FXuchar *output, const FXuchar *input, FXuval outputlen, FXuval &inputlen, FXIODevice::CRLFType type)
{
	if(Default==type)
#ifdef WIN32
		type=MSDOS;
#elif defined(USE_MACOSX)
		type=MacOS;
#elif defined(USE_POSIX)
		type=Unix;
#else
#error Unknown system
#endif
	if(Unix==type)
	{
		FXuval tocopy=FXMIN(outputlen, inputlen);
		memcpy(output, input, tocopy);
		return tocopy;
	}
	FXuval o=0, i=0;
	midNL=false;
	for(; o<outputlen && i<inputlen; o++, i++)
	{
		if(10==input[i])
		{
			output[o]=13;
			if(o+1==outputlen)
			{
				midNL=true;
				inputlen=i;
				return o;
			}
			if(MSDOS==type) output[++o]=10;
		}
		else output[o]=input[i];
	}
	inputlen=i;
	return o;
}

FXuval FXIODevice::removeCRLF(bool &midNL, FXuchar *output, const FXuchar *input, FXuval len)
{
	FXuval writeidx=0;
	midNL=false;
	for(FXuval n=0; n<len; n++, writeidx++)
	{
		if(13==input[n])
		{
			if(n+1==len)
			{
				midNL=true;
				return writeidx;
			}
			if(10==input[n+1])
			{	// MS-DOS style
				output[writeidx]=input[++n];
			}
			else
			{	// Mac style
				output[writeidx]=10;
			}
		}
		else output[writeidx]=input[n];
	}
	return writeidx;
}

FXfval FXIODevice::shredData(FXfval offset, FXfval len)
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

FXStream &operator<<(FXStream &s, FXIODevice &i)
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

FXStream &operator>>(FXStream &s, FXIODevice &i)
{
	char buffer[256*1024];
	FXuval read;
	FXIODevice *sdev=s.device();
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

FXuval FXIODeviceS::readBlockFrom(char *data, FXuval maxlen, FXfval pos)
{
	FXERRGNOTSUPP("readBlockFrom not supported for synchronous i/o devices");
}

FXuval FXIODeviceS::writeBlockTo(FXfval pos, const char *data, FXuval maxlen)
{
	FXERRGNOTSUPP("readBlockFrom not supported for synchronous i/o devices");
}

bool FXIODeviceS::waitForData(FXIODeviceS **signalled, FXuint no, FXIODeviceS **list, FXuint waitfor)
{
	if(signalled) signalled[0]=0;
#ifndef USE_POSIX
	HANDLE hlist[MAXIMUM_WAIT_OBJECTS];
	FXERRH(no<MAXIMUM_WAIT_OBJECTS, "MAXIMUM_WAIT_OBJECTS exceeded", 0, FXERRH_ISDEBUG);
	for(FXuint n=0; n<no; n++)
	{
		hlist[n]=list[n]->int_getOSHandle();
		FXERRH(hlist[n], FXTrans::tr("FXIODeviceS", "Either i/o device is not open or not supported"), 0, FXERRH_ISDEBUG);
	}
	hlist[no]=FXThread::int_cancelWaiterHandle();
	DWORD ret=WaitForMultipleObjects(no+1, hlist, FALSE, (waitfor==FXINFINITE) ? INFINITE : waitfor);
	if(ret==WAIT_TIMEOUT) return false;
	if(WAIT_OBJECT_0+no==ret)
	{
		FXThread::current()->checkForTerminate();
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
	struct timeval *tv=0, _tv;
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
		int fd=(int) list[n]->int_getOSHandle();
		FXERRH(fd, FXTrans::tr("FXIODeviceS", "Either i/o device is not open or not supported"), 0, FXERRH_ISDEBUG);
		FD_SET(fd, &fds);
		if(fd>maxfd) maxfd=fd;
	}
	int ret=::select(maxfd+1, &fds, 0, 0, tv);
	if(ret<0) { FXERRHOS(ret); return false; }
	if(!ret) return false;
	if(signalled)
	{
		FXuint oidx=0;
		for(FXuint n=0; n<no; n++)
		{
			int fd=(int) list[n]->int_getOSHandle();
			if(FD_ISSET(fd, &fds))
				signalled[oidx++]=list[n];
		}
		signalled[oidx]=0;
	}
	return true;
#endif
}
FXuint FXIODeviceS::waitForDataMax() throw()
{
#ifndef USE_POSIX
	return MAXIMUM_WAIT_OBJECTS-1;
#else
	return FD_SETSIZE;
#endif
}

} // namespace

