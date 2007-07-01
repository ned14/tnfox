/********************************************************************************
*                                                                               *
*                               Test of FXNetwork                               *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003-2007 by Niall Douglas.   All Rights Reserved.       *
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

#include "fx.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
	FXProcess myprocess(argc, argv);
	char buffer[256];
	for(;;)
	{
		fxmessage("\nPlease enter a fully qualified name:\n");
		if(myprocess.isAutomatedTest())
		{
			fxmessage("www.google.com\n");
			strcpy(buffer, "www.google.com");
		}
		else
			gets(buffer);
		if(!buffer[0]) break;
		QHostAddress addr=FXNetwork::dnsLookup(FXString(buffer));
		fxmessage("\nIn IP that is: %s\n", addr.toString().text());
		QHostAddress addr6(addr.ip6Addr());
		fxmessage("In IPv6 that is: %s\n", addr6.toString().text());
		FXString name=FXNetwork::dnsReverseLookup(addr);
		fxmessage("Reverse lookup says: %s\n", name.text());
		if(myprocess.isAutomatedTest())
			break;
	}
	return 0;
}
