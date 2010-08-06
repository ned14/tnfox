/********************************************************************************
*                                                                               *
*                              Test of DLL facilities                           *
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

#include "fx.h"
#include "TestDLL.h"

static const char mytransfile[]="# TnFOX human language string literal translation file\n"
"# Last updated: Tue, 01 Jul 2003 05:22:14 +0000\n"
"\n"
"version=1:\n"
"\n"
"# If the line below says =Yes, then there are still text literals in here\n"
"# which have not been translated yet. Search for \"!TODO!\" to find them\n"
"\n"
"needsupdating=No:\n"
"\n"
"# Enter the list of all language ids used in the file\n"
"\n"
"langids=\"ES\":\n"
"\n"
"\"This is a test teapot\":\n"
"\tEN: \"Translation worked!\"\n"
"\tES: \"El traducido functiona!\"\n"
"\n";
QTRANS_SETTRANSFILE(mytransfile, sizeof(mytransfile));

namespace Test {

static TestDLL *thetestdll;
TestDLL::TestDLL()
{
	thetestdll=this;
	fxmessage("Test DLL initialised, this is 0x%p, FXProcess is 0x%p\n", this, FXProcess::instance());
}
TestDLL::~TestDLL()
{
	fxmessage("Test DLL destructed\n");
	thetestdll=0;
}

static FXProcess_StaticInit<TestDLL> dllinit("TestDLL");
}

Test::TestDLL *findDLL()
{
	return Test::thetestdll;
}