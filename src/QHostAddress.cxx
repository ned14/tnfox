/********************************************************************************
*                                                                               *
*                             IP address container                              *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003-2009 by Niall Douglas.   All Rights Reserved.       *
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
#include "QHostAddress.h"
#include "FXString.h"
#include "FXException.h"
#include "FXStream.h"
#include "FXRollback.h"
#include <string.h>
#include <stdio.h>
#include "FXMemDbg.h"
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

#ifdef __GNUC__
#warning QHostAddress and QHostAddressDict are under construction!
#endif
#ifdef _MSC_VER
#pragma message(__FILE__ ": WARNING: QHostAddress and QHostAddressDict are under construction!")
#endif

namespace FX {

struct FXDLLLOCAL QHostAddressPrivate
{
	bool isIPv6, isLoopback, isNull;
	FXuint IPv4;
	Maths::Vector<FXuchar, 16> IPv6;
	QHostAddressPrivate() : isIPv6(false), isLoopback(false), isNull(true), IPv4(0), IPv6((FXuchar *) 0) { }
};
static FXuchar IPv6LoopbackData[]={ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 };
static Maths::Vector<FXuchar, 16> IPv6Zero((FXuchar *) 0), IPv6Loopback(IPv6LoopbackData);

QHostAddress::QHostAddress() : p(0)
{
	FXERRHM(p=new QHostAddressPrivate);
}

QHostAddress::QHostAddress(FXuint ip4addr) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QHostAddressPrivate);
	setAddress(ip4addr);
	unconstr.dismiss();
}

QHostAddress::QHostAddress(const Maths::Vector<FXuchar, 16> &ip6addr) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QHostAddressPrivate);
	setAddress(ip6addr);
	unconstr.dismiss();
}

QHostAddress::QHostAddress(const FXuchar *ip6addr) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QHostAddressPrivate);
	setAddress(ip6addr);
	unconstr.dismiss();
}

QHostAddress::QHostAddress(const QHostAddress &o) : p(0)
{
	FXERRHM(p=new QHostAddressPrivate(*o.p));
}

QHostAddress &QHostAddress::operator=(const QHostAddress &o)
{
	*p=*o.p;
	return *this;
}

QHostAddress::~QHostAddress()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

bool QHostAddress::operator==(const QHostAddress &o) const
{
	if(p->isNull || o.p->isNull) return (p->isNull && o.p->isNull);
	if(p->isLoopback || o.p->isLoopback) return (p->isLoopback && o.p->isLoopback);
	if(!p->isIPv6 && !o.p->isIPv6) return (p->IPv4==o.p->IPv4);
	// Compare the IPv6 as IPv4 sets the IPv6
	return 4==sum(p->IPv6==o.p->IPv6);
}

bool QHostAddress::operator<(const QHostAddress &o) const
{
	if(!p->isIPv6 && !o.p->isIPv6) return (p->IPv4<o.p->IPv4);
	// Compare the IPv6 as IPv4 sets the IPv6
	return memcmp(&p->IPv6, &o.p->IPv6, sizeof(p->IPv6))<0;
}

bool QHostAddress::operator>(const QHostAddress &o) const
{
	if(!p->isIPv6 && !o.p->isIPv6) return (p->IPv4>o.p->IPv4);
	// Compare the IPv6 as IPv4 sets the IPv6
	return memcmp(&p->IPv6, &o.p->IPv6, sizeof(p->IPv6))>0;
}

QHostAddress QHostAddress::operator&(const QHostAddress &o) const
{
	if(!p->isIPv6 && !o.p->isIPv6) return (p->IPv4 & o.p->IPv4);
	// Compare the IPv6 as IPv4 sets the IPv6
	return QHostAddress(p->IPv6 & o.p->IPv6);
}

QHostAddress QHostAddress::operator|(const QHostAddress &o) const
{
	if(!p->isIPv6 && !o.p->isIPv6) return (p->IPv4 | o.p->IPv4);
	// Compare the IPv6 as IPv4 sets the IPv6
	return QHostAddress(p->IPv6 | o.p->IPv6);
}

QHostAddress QHostAddress::operator^(const QHostAddress &o) const
{
	if(!p->isIPv6 && !o.p->isIPv6) return (p->IPv4 ^ o.p->IPv4);
	// Compare the IPv6 as IPv4 sets the IPv6
	return QHostAddress(p->IPv6 ^ o.p->IPv6);
}

void QHostAddress::setAddress(FXuint ip4addr)
{
	p->isIPv6=false;
	p->isLoopback=(0x7f000001==ip4addr);
	p->isNull=(0==ip4addr);
	p->IPv4=ip4addr;
	p->IPv6=p->isLoopback ? IPv6Loopback : IPv6Zero;
	if(!p->isLoopback)
	{
		p->IPv6.set(10, 0xff); p->IPv6.set(11, 0xff);
		p->IPv6.set(12, (FXuchar)((ip4addr>>24) & 0xff));
		p->IPv6.set(13, (FXuchar)((ip4addr>>16) & 0xff));
		p->IPv6.set(14, (FXuchar)((ip4addr>>8) & 0xff));
		p->IPv6.set(15, (FXuchar)((ip4addr) & 0xff));
	}
}

void QHostAddress::setAddress(const Maths::Vector<FXuchar, 16> &ip6addr)
{
	p->isIPv6=true;
	p->isLoopback=true;
	p->isNull=true;
	p->IPv6=ip6addr;
	p->isNull=isZero(p->IPv6);
	p->isLoopback=(4==sum(ip6addr==IPv6Loopback));
	if(p->isLoopback) p->IPv4=0x7f000001;
	else if(0xff==p->IPv6[10] && 0xff==p->IPv6[11])
	{	// A IPv6 to IPv4 mapping
		p->IPv4=(p->IPv6[12]<<24)|(p->IPv6[13]<<16)|(p->IPv6[14]<<8)|(p->IPv6[15]);
	}
	else p->IPv4=0;
}

void QHostAddress::setAddress(const FXuchar *ip6addr)
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
		p->IPv6.set(n, c);
	}
	if(p->isLoopback) p->IPv4=0x7f000001;
	else if(0xff==p->IPv6[10] && 0xff==p->IPv6[11])
	{	// A IPv6 to IPv4 mapping
		p->IPv4=(p->IPv6[12]<<24)|(p->IPv6[13]<<16)|(p->IPv6[14]<<8)|(p->IPv6[15]);
	}
	else p->IPv4=0;
}

bool QHostAddress::setAddress(const FXString &str)
{
	if(3==str.contains('.'))
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
		FXuchar buffer[16];
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
			if(!ok || idx>14) return false;
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

bool QHostAddress::isNull() const
{
	return p->isNull;
}

bool QHostAddress::isIp4Addr() const
{
	return !p->isIPv6;
}

FXuint QHostAddress::ip4Addr() const
{
	return p->IPv4;
}

bool QHostAddress::isIp6Addr() const
{
	return p->isIPv6;
}

const Maths::Vector<FXuchar, 16> &QHostAddress::ip6Addr() const
{
	return p->IPv6;
}

const FXuchar *QHostAddress::ip6AddrData() const
{
	return (const FXuchar *) &p->IPv6;
}

FXString QHostAddress::toString() const
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

bool QHostAddress::isLocalMachine() const
{
	return p->isLoopback || p->isNull;
}

FXStream &operator<<(FXStream &s, const QHostAddress &i)
{
	FXuchar v6=i.p->isIPv6;
	s << v6;
	if(v6)
		s << i.p->IPv6;
	else
		s << i.p->IPv4;
	return s;
}

FXStream &operator>>(FXStream &s, QHostAddress &i)
{
	FXuchar v6;
	s >> v6;
	if(v6)
	{
		s >> i.p->IPv6;
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
