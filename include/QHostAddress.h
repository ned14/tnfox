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


#ifndef QHOSTADDRESS_H
#define QHOSTADDRESS_H

#include "fxdefs.h"

namespace FX {

/*! \file QHostAddress.h
\brief Defines classes used to provide an IP address
*/

class FXString;

//! Simple macro creating a FX::QHostAddress containing \c localhost
#define QHOSTADDRESS_LOCALHOST QHostAddress(0x7f000001)

/*! \class QHostAddress
\brief A container holding an IPv4 or IPv6 address (Qt compatible)

This is a Qt compatible IP address container capable of working transparently
with IPv4 and IPv6 addresses. It has no network knowledge and so cannot
be more intelligent than what the IP specification rules permit (eg; it
can't tell if an IPv4 and IPv6 addresses refer to the same machine).

An IPv4 address is 32 bits long whereas IPv6, a substantial improvement
over IPv4, is up to 128 bits long (though it can vary). A discussion of
IP is beyond the scope of this page, there are hundreds of books and RFC's
about the matter which you should consult.

Some mapping though is provided. IPv4 maps to IPv6 where an IPv4 address
\c AA.BB.CC.DD (if the numbers were in hex) becomes in IPv6 
\c ::FFFF:AABB:CCDD. Also loopback is treated specially:
127.0.0.1 is equivalent to ::1 and vice versa. In the comparison operators,
0.0.0.0 is treated as being equivalent to loopback.
*/
struct QHostAddressPrivate;
class FXAPIR QHostAddress
{
	QHostAddressPrivate *p;
public:
	//! Constructs a null address
	QHostAddress();
	//! Constructs the specified IPv4 address
	QHostAddress(FXuint ip4addr);
	//! Constructs the specified IPv6 address. Must be in network order.
	QHostAddress(const FXuchar *ip6addr);
	QHostAddress(const QHostAddress &o);
	QHostAddress &operator=(const QHostAddress &o);
	~QHostAddress();
	//! Returns true if both addresses are the same by ruleset
	bool operator==(const QHostAddress &o) const;
	//! Returns true if both addresses are not the same
	bool operator!=(const QHostAddress &o) const { return !(*this==o); }

	//! Sets the contents to the specified IPv4 address
	void setAddress(FXuint ip4addr);
	//! Sets the contents to the specified IPv6 address. Must be in network order.
	void setAddress(const FXuchar *ip6addr);
	/*! Sets the contents by parsing a string. If the string is of the format X.X.X.X
	then it is assumed to be an IPv4 descriptor and is set as such. If not, it is
	assumed to be an IPv6 descriptor and is set as such, including expanding "::" sections.
	Returns false if there was an error in parsing.
	*/
	bool setAddress(const FXString &addr);
	//! Returns true if the contents are a null address
	bool isNull() const;
	//! Returns true if the contents contain an IPv4 address
	bool isIp4Addr() const;
	/*! Returns the contents in IPv4 format. Returns zero if the container holds
	an IPv6 address (use isIp4Addr() to check) unless that IPv6 address is a 6to4
	protocol map (::FFFF:...) or loopback (::1) in which case it returns 127.0.0.1 */
	FXuint ip4Addr() const;
	//! Returns true if the contents contain an IPv6 address
	bool isIp6Addr() const;
	/*! Returns the contents in IPv6 format. If the contents are in IPv4 format,
	returns the IPv4 address mapped as an IPv6 using the 6to4 protocol */
	const FXuchar *ip6Addr() const;
	/*! Returns the contents as a string representation. If in IPv4, this is in the
	format X.X.X.X. If in IPv6, this is the standard colon separated IPv6 representation,
	including with zero sections condensed to "::".
	*/
	FXString toString() const;
	//! Returns true if the address is the local machine
	bool isLocalMachine() const;
	friend FXStream &operator<<(FXStream &s, const QHostAddress &i);
	friend FXStream &operator>>(FXStream &s, QHostAddress &i);
};

//! Writes the contents of the IP address to stream \em s
FXStream &operator<<(FXStream &s, const QHostAddress &i);
//! Reads an IP address from stream \em s
FXStream &operator>>(FXStream &s, QHostAddress &i);

} // namespace

#endif
