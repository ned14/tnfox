/********************************************************************************
*                                                                               *
*                                Memory Pool test                               *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003 by Niall Douglas.   All Rights Reserved.                   *
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

#include <qptrvector.h>
#include "fx.h"
#ifdef _MSC_VER
#include <crtdbg.h>
#endif

//#define USE_LOCALMALLOC
//#define USE_WIN32DIRECT

#ifdef USE_WIN32DIRECT
#include <windows.h>
#endif

static FXuint seed;
static FXAtomicInt allocs;

class Thread : public FXThread
{
public:
	int number;
	volatile bool expanding;
	QPtrVector<void> alloclist;
	Thread() : number(0), expanding(true) { }
	void alloc()
	{
		void *ptr;
		FXuint size;
#ifdef USE_LOCALMALLOC
#ifdef USE_WIN32DIRECT
		alloclist.append((ptr=HeapAlloc(GetProcessHeap(), 0, (size=fxrandom(seed)>>23))));
#else
		alloclist.append((ptr=::malloc((size=fxrandom(seed)>>23))));
#endif
#else
		alloclist.append((ptr=FXMemoryPool::glmalloc((size=fxrandom(seed)>>23))));
#endif
		//fxmessage("Thread %d allocated %d at 0x%p\n", FXThread::id(), size, ptr);
		allocs++;
		//_ASSERTE( _CrtCheckMemory( ) );
	}
	void free()
	{
		void *ptr=alloclist.getLast();
		alloclist.removeLast();
		//fxmessage("Thread %d freed 0x%p\n", FXThread::id(), ptr);
#ifdef USE_LOCALMALLOC
#ifdef USE_WIN32DIRECT
		HeapFree(GetProcessHeap(), 0, ptr);
#else
		::free(ptr);
#endif
#else
		FXMemoryPool::glfree(ptr);
#endif
		//_ASSERTE( _CrtCheckMemory( ) );
	}
	void run()
	{
		for(int n=0; n<100; n++)
		{
			alloc();
		}
		do
		{
			FXuint testpoint=((FXuint)-1)/49;
			if(expanding) testpoint*=25;
			else testpoint*=24;
			if(fxrandom(seed)<testpoint)
				alloc();
			else free();
		} while(!alloclist.isEmpty());
	}
	void *cleanup()
	{
		return 0;
	}
};

int main(int argc, char *argv[])
{
	FXProcess myprocess(argc, argv);
	printf("FXMemoryPool test\n"
		   "-=-=-=-=-=-=-=-=-\n");
	const int threads=5, totalallocs=20000000;
	//const int threads=5, totalallocs=2000000;
	Thread ths[threads];
	fxmessage("*** Allocating\n");
	FXuint start=FXProcess::getMsCount();
	for(int n=0; n<threads; n++)
	{
		ths[n].start();
	}
	while(allocs<totalallocs)
	{
		FXThread::sleep(1);
		fxmessage("*** Stats: %s\n", FXMemoryPool::glstatsAsString().text());
	}
	fxmessage("*** Deallocating\n");
	fxmessage("*** Stats: %s\n", FXMemoryPool::glstatsAsString().text());
	for(int n=0; n<threads; n++)
	{
		ths[n].expanding=false;
	}
	for(int fin=0; fin<threads;)
	{
		fin=0;
		for(int n=0; n<threads; n++)
		{
			fin+=ths[n].wait(1000);
			fxmessage("*** Stats: %s\n", FXMemoryPool::glstatsAsString().text());
		}
	}
	FXuint end=FXProcess::getMsCount();
	fxmessage("\n%d allocs & frees done in %f seconds\n"
		"   (=%f ops/sec)\n\n", allocs*2, (end-start)/1000.0, (allocs*2)/((end-start)/1000.0));
	fxmessage("*** Trimming\n");
	FXMemoryPool::gltrim(0);
	fxmessage("*** Stats: %s\n", FXMemoryPool::glstatsAsString().text());
#ifdef _MSC_VER
	_ASSERTE( _CrtCheckMemory( ) );
#endif
	printf("\n\nTests complete!\n");
#ifdef WIN32
	getchar();
#endif
	return 0;
}
