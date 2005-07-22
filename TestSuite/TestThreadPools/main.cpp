/********************************************************************************
*                                                                               *
*                              Test of thread pools                             *
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

#define TOTAL 8

static void testcode(const char *msg, int a)
{
	for(int n=0; n<a; n++)
	{
		//QThread::sleep(1);
		fxmessage("%s counts to %d/%d\n", msg, n, a);
	}
}
static FXuint seed;

int main(int argc, char *argv[])
{
	FXProcess myprocess(argc, argv);
	fxmessage("TnFOX Thread Pools test:\n"
		      "-=-=-=-=-=-=-=-=-=-=-=-=\n");
	fxmessage("Starting thread pool ...\n");
	QThreadPool &tp=FXProcess::threadPool();
	FXString txts[TOTAL];
	QThreadPool::handle handles[TOTAL];
	for(int n=0; n<TOTAL; n++)
	{
		txts[n]="No %1";
		txts[n].arg(n);
		fxmessage("Dispatching %s ...\n", txts[n].text());
		handles[n]=tp.dispatch(Generic::BindFuncN(testcode, txts[n].text(), 3));
	}
	for(int n=0; n<TOTAL; n++)
	{
		tp.wait(handles[n]);
	}

	fxmessage("\nNow testing timed jobs ...\n");
	for(int n=0; n<TOTAL; n++)
	{
		txts[n]="No %1";
		txts[n].arg(n);
		FXuint delay=fxrandom(seed) & 0xfff;
		fxmessage("Dispatching %s in %d msecs...\n", txts[n].text(), delay);
		handles[n]=tp.dispatch(Generic::BindFuncN(testcode, txts[n].text(), 3), delay);
	}
	for(int n=0; n<TOTAL; n++)
	{
		tp.wait(handles[n]);
	}

	fxmessage("All Done!\n");
#ifdef _MSC_VER
	getchar();
#endif
	return 0;
}
