/********************************************************************************
*                                                                               *
*                             IP address container                              *
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

#include "fxdefs.h"
#include "FXHostAddress.h"
#include "FXString.h"
#include "FXException.h"
#include "FXStream.h"
#include "FXRollback.h"
#include <string.h>
#include <stdio.h>

namespace FX {

struct FXDLLLOCAL FXHostAddressPrivate
{
	bool isIPv6, isLoopback, isNull;
	FXuint IPv4;
	FXuchar IPv6[16];
	FXHostAddressPrivate() : isIPv6(false), isLoopback(false), isNull(true), IPv4(0)
	{
		memset(IPv6, 0, sizeof(IPv6));
	}
};

FXHostAddress::FXHostAddress() : p(0)
{
	FXERRHM(p=new FXHostAddressPrivate);
}

FXHostAddress::FXHostAddress(FXuint ip4addr) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new FXHostAddressPrivate);
	setAddress(ip4addr);
	unconstr.dismiss();
}

FXHostAddress::FXHostAddress(const FXuchar *ip6addr) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new FXHostAddressPrivate);
	setAddress(ip6addr);
	unconstr.dismiss();
}

FXHostAddress::FXHostAddress(const FXHostAddress &o) : p(0)
{
	FXERRHM(p=new FXHostAddressPrivate(*o.p));
}

FXHostAddress &FXHostAddress::operator=(const FXHostAddress &o)
{
	*p=*o.p;
	return *this;
}

FXHostAddress::~FXHostAddress()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

bool FXHostAddress::operator==(const FXHostAddress &o) const
{
	if((p->isLoopback || p->isNull) && (o.p->isLoopback || o.p->isNull)) return true;
	if(!p->isIPv6 && !o.p->isIPv6) return (p->IPv4==o.p->IPv4);
	// Compare the IPv6 as IPv4 sets the IPv6
	return (0==memcmp(p->IPv6, o.p->IPv6, sizeof(p->IPv6)));
}

void FXHostAddress::setAddress(FXuint ip4addr)
{
	p->isIPv6=false;
	p->isLoopback=(0x7f000001==ip4addr);
	p->isNull=(0==ip4addr);
	p->IPv4=ip4addr;
	memset(p->IPv6, 0, sizeof(p->IPv6));
	if(p->isLoopback)
		p->IPv6[15]=1;
	else
	{
		p->IPv6[10]=0xff; p->IPv6[11]=0xff;
		p->IPv6[12]=(FXuchar)((ip4addr>>24) & 0xff);
		p->IPv6[13]=(FXuchar)((ip4addr>>16) & 0xff);
		p->IPv6[14]=(FXuchar)((ip4addr>>8) & 0xff);
		p->IPv6[15]=(FXuchar)((ip4addr) & 0xff);
	}
}

void FXHostAddress::setAddress(const FXuchar *ip6addr)
{
	p->isIPv6=true;
	p->isLoopback=true;
	p->isNull=true;
	for(int n=0; n<16; n++)
	{
		register FXuchar c=ip6addr[n];
		if(p->isLoopback)
		{
			if(n<15)
			{
				if(c) p->isLoopback=false;
			}
			else if(1!=c) p->isLoopback=false;
		}
		if(c) p->isNull=false;
		p->IPv6[n]=c;
	}
	if(p->isLoopback) p->IPv4=0x7f000001;
	else if(0xff==p->IPv6[10] && 0xff==p->IPv6[11])
	{	// A IPv6 to IPv4 mapping
		p->IPv4=(p->IPv6[12]<<24)|(p->IPv6[13]<<16)|(p->IPv6[14]<<8)|(p->IPv6[15]);
	}
	else p->IPv4=0;
}

bool FXHostAddress::setAddress(const FXString &str)
{
	if(3==str.count('.'))
	{	// IPv4
		int a,b,c,d;
		int ret=sscanf(str.text(), "%d.%d.%d.%d", &a,&b,&c,&d);
		if(EOF==ret) return false;
		if(a<0 || a>255) return false;
		if(b<0 || b>255) return false;
		if(c<0 || c>255) return false;
		if(d<0 || d>255) return false;
		FXuint addr=(a<<24)|(b<<16)|(c<<8)|d;
		setAddress(addr);
	}
	else
	{	// IPv6
		FXuchar buffer[8];
		memset(buffer, 0, sizeof(buffer));
		int doublecolons=str.find("::");
		FXString before, after;
		if(doublecolons>=0)
		{
			before=str.left(doublecolons);
			after=str.mid(doublecolons+2, str.length());
		}
		else before=str;
		int idx=0;
		bool ok=true;
		for(int bidx=0; bidx<before.length();)
		{
			FXuint val=before.mid(bidx, before.length()).toUInt(&ok, 16);
			if(!ok) return false;
			buffer[idx++]=(FXuchar)((val>>8) & 0xff); buffer[idx++]=(FXuchar)((val) & 0xff);
			bidx=before.find(':', bidx); if(-1==bidx) bidx=before.length();
		}
		idx=7;
		for(int aidx=after.length(); aidx>0;)
		{
			aidx=after.rfind(':', aidx); if(-1==aidx) aidx=0;
			FXuint val=after.mid(aidx, after.length()).toUInt(&ok, 16);
			if(!ok) return false;
			buffer[idx--]=(FXuchar)((val) & 0xff); buffer[idx--]=(FXuchar)((val>>8) & 0xff);
		}
		setAddress((FXuchar *) buffer);
	}
	return true;
}

bool FXHostAddress::isNull() const
{
	return p->isNull;
}

bool FXHostAddress::isIp4Addr() const
{
	return !p->isIPv6;
}

FXuint FXHostAddress::ip4Addr() const
{
	return p->IPv4;
}

bool FXHostAddress::isIp6Addr() const
{
	return p->isIPv6;
}

const FXuchar *FXHostAddress::ip6Addr() const
{
	return p->IPv6;
}

FXString FXHostAddress::toString() const
{
	FXString ret;
	if(p->isIPv6)
	{
		ret="%1:%2:%3:%4:%5:%6:%7:%8";
		for(int idx=0; idx<16; idx+=2)
		{
			FXushort addr=(p->IPv6[idx]<<8)|(p->IPv6[idx+1]);
			ret.arg(addr,0,16);
		}
		// Collapse the biggest sequence of zeros from the end if possible
		FXString zeros("0:0:0:0:0:0:0:0");
		for(; zeros.length()>=3; zeros.length(zeros.length()-2))
		{
			int zidx=ret.rfind(zeros);
			if(zidx>=0)
			{
				ret=ret.left(zidx)+":"+ret.mid(zidx+zeros.length(), ret.length());
				break;
			}
		}
	}
	else
	{
		char buffer[20];
		FXuchar *addr=(FXuchar *) &p->IPv4;
		sprintf(buffer, "%d.%d.%d.%d", addr[3], addr[2], addr[1], addr[0]);
		ret=buffer;
	}
	return ret;
}

bool FXHostAddress::isLocalMachine() const
{
	return p->isLoopback || p->isNull;
}

FXStream &operator<<(FXStream &s, const FXHostAddress &i)
{
	FXuchar v6=i.p->isIPv6;
	s << v6;
	if(v6)
		s.writeRawBytes(i.p->IPv6, sizeof(i.p->IPv6));
	else
		s << i.p->IPv4;
	return s;
}

FXStream &operator>>(FXStream &s, FXHostAddress &i)
{
	FXuchar v6;
	s >> v6;
	if(v6)
	{
		s.readRawBytes(i.p->IPv6, sizeof(i.p->IPv6));
		i.setAddress(i.p->IPv6);
	}
	else
	{
		s >> i.p->IPv4;
		i.setAddress(i.p->IPv4);
	}
	return s;
}

} // namespace
