/********************************************************************************
*                                                                               *
*                        Miscellaneous network support                          *
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

#ifndef FXNETWORK_H
#define FXNETWORK_H
#include "FXHostAddress.h"

namespace FX {

/*! \file FXNetwork.h
\brief Defines classes used to provide miscellaneous network services
*/

class FXString;

/*! \class FXNetwork
\brief Provides miscellaneous network facilities

Not a lot to say other than that this class provides various miscellaneous
network services such as DNS lookups. More will be added with time (eg;
querying of local machines via Samba or MS Networking)
*/
class FXAPIR FXNetwork
{
	FXNetwork();
	FXNetwork(const FXNetwork &);
	FXNetwork &operator=(const FXNetwork &);
public:
	//! Returns true if IPv6 is supported on this machine
	static bool hasIPv6();
	//! Returns the host name of this machine
	static FXString hostname();
	/*! Performs a translation of textual name to IP address by querying a DNS
	provider on the network. This call can take seconds and so should be called
	from a worker thread. IPv6 addresses are preferred to IPv4 where possible.
	If the name isn't found due to not existing or no DNS server available, a
	null address is returned - however an exception is still thrown for errors.
	*/
	static FXHostAddress dnsLookup(const FXString &name);
	/*! Performs a translation of an IP address to a textual name by querying a
	DNS provider on the network. This call can especially take a long time. If
	there is no name associated with the IP address or no DNS server is
	available, a null string is returned - however an exception is still thrown
	for errors.
	*/
	static FXString dnsReverseLookup(const FXHostAddress &addr);
};

} // namespace

#endif
