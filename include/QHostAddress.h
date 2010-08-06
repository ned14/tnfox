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

#include "FXMaths.h"
#include "qdictbase.h"

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

See FX::QHostAddressDict for a linear access time dictionary of QHostAddresses.
You may also wish to consult FX::Maths::Vector<> for enhanced bit operations
for when working with IPv6 addresses.
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
	QHostAddress(const Maths::Vector<FXuchar, 16> &ip6addr);
	//! Constructs the specified IPv6 address. Must be in network order.
	QHostAddress(const FXuchar *ip6addr);
	QHostAddress(const QHostAddress &o);
	QHostAddress &operator=(const QHostAddress &o);
	~QHostAddress();
	//! Returns true if the contents is a null address
	bool operator!() const { return isNull(); }
	//! Returns true if both addresses are the same by ruleset
	bool operator==(const QHostAddress &o) const;
	//! Returns true if both addresses are not the same
	bool operator!=(const QHostAddress &o) const { return !(*this==o); }
	//! Returns true if this address is less than the other by ruleset
	bool operator<(const QHostAddress &o) const;
	//! Returns true if this address is greater than the other by ruleset
	bool operator>(const QHostAddress &o) const;
	//! Returns true if this address is less than or equal to the other by ruleset
	bool operator<=(const QHostAddress &o) const { return !(*this>o); }
	//! Returns true if this address is greater than or equal to the other by ruleset
	bool operator>=(const QHostAddress &o) const { return !(*this<o); }
	//! Returns one address bitwise ANDed with another
	QHostAddress operator&(const QHostAddress &o) const;
	//! Returns one address bitwise ORed with another
	QHostAddress operator|(const QHostAddress &o) const;
	//! Returns one address bitwise XORed with another
	QHostAddress operator^(const QHostAddress &o) const;

	//! Sets the contents to the specified IPv4 address
	void setAddress(FXuint ip4addr);
	//! Sets the contents to the specified IPv6 address. Must be in network order.
	void setAddress(const Maths::Vector<FXuchar, 16> &ip6addr);
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
	returns the IPv4 address mapped as an IPv6 using the 6to4 protocol.
	*/
	const Maths::Vector<FXuchar, 16> &ip6Addr() const;
	/*! Returns the contents in IPv6 format. If the contents are in IPv4 format,
	returns the IPv4 address mapped as an IPv6 using the 6to4 protocol.
	*/
	const FXuchar *ip6AddrData() const;
	/*! Returns the contents as a string representation. If in IPv4, this is in the
	format X.X.X.X. If in IPv6, this is the standard colon separated IPv6 representation,
	including with zero sections condensed to "::".
	*/
	FXString toString() const;
	//! Returns true if the address is the local machine
	bool isLocalMachine() const;

	//! Writes the contents of the IP address to stream \em s
	friend FXAPIR FXStream &operator<<(FXStream &s, const QHostAddress &i);
	//! Reads an IP address from stream \em s
	friend FXAPIR FXStream &operator>>(FXStream &s, QHostAddress &i);
};

template<class type> class QHostAddressDict : public QDictBase<QHostAddress, type>
{
	typedef QDictBase<QHostAddress, type> Base;
	FXuint hash(const QHostAddress &a) const
	{
		if(a.isIp4Addr()) return a.ip4Addr();
		const FXuchar *txt=a.ip6AddrData();
		const FXuchar *end=txt+16;
		FXuint h=0;
		for(; end>=txt; end--)
			h=(h & 0xffffff80)^(h<<8)^(*end);
		return h;
	}
public:
	enum { HasSlowKeyCompare=true };
	//! Creates a hash table indexed by QHostAddress's. Choose a prime for \em size
	explicit QHostAddressDict(int size=13, bool wantAutoDel=false)
		: Base(size, wantAutoDel)
	{
	}
	QHostAddressDict(const QHostAddressDict<type> &o) : Base(o) { }
	~QHostAddressDict() { Base::clear(); }
	FXADDMOVEBASECLASS(QHostAddressDict, Base)
	//! Inserts item \em d into the dictionary under key \em k
	void insert(const QHostAddress &k, const type *d)
	{
		Base::insert(hash(k), k, const_cast<type *>(d));
	}
	//! Replaces item \em d in the dictionary under key \em k
	void replace(const QHostAddress &k, const type *d)
	{
		Base::replace(hash(k), k, const_cast<type *>(d));
	}
	//! Deletes the most recently placed item in the dictionary under key \em k
	bool remove(const QHostAddress &k)
	{
		return Base::remove(hash(k), k);
	}
	//! Removes the most recently placed item in the dictionary under key \em k without auto-deletion
	type *take(const QHostAddress &k)
	{
		return Base::take(hash(k), k);
	}
	//! Finds the most recently placed item in the dictionary under key \em k
	type *find(const QHostAddress &k) const
	{
		return Base::find(hash(k), k);
	}
	//! \overload
	type *operator[](const QHostAddress &k) const { return find(k); }
protected:
	virtual void deleteItem(type *d);
};

template<class type> inline void QHostAddressDict<type>::deleteItem(type *d)
{
	if(Base::autoDelete())
	{
		//fxmessage("QDB delete %p\n", d);
		delete d;
	}
}
// Don't delete void *
template<> inline void QHostAddressDict<void>::deleteItem(void *)
{
}

/*! \class QHostAddressIterator
\ingroup QTL
\brief An iterator for a QHostAddress
*/
template<class type> class QHostAddressDictIterator : public QDictBaseIterator<QHostAddress, type>
{
public:
	QHostAddressDictIterator() { }
	QHostAddressDictIterator(const QHostAddressDict<type> &d) : QDictBaseIterator<QHostAddress, type>(d) { }
};

//! Writes the contents of the dictionary to stream \em s
template<class type> FXStream &operator<<(FXStream &s, const QHostAddressDict<type> &i)
{
	FXuint mysize=i.count();
	s << mysize;
	for(QHostAddressDictIterator<type> it(i); it.current(); ++it)
	{
		s << it.currentKey();
		s << *it.current();
	}
	return s;
}
//! Reads a dictionary from stream \em s
template<class type> FXStream &operator>>(FXStream &s, QHostAddressDict<type> &i)
{
	FXuint mysize;
	s >> mysize;
	i.clear();
	QHostAddress key;
	for(FXuint n=0; n<mysize; n++)
	{
		type *item;
		FXERRHM(item=new type);
		s >> key;
		s >> *item;
		i.insert(key, item);
	}
	return s;
}

} // namespace

#endif
