/********************************************************************************
*                                                                               *
*                           Threading framework test                            *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2002,2003 by Niall Douglas.   All Rights Reserved.       *
*   NOTE THAT I NIALL DOUGLAS DO NOT PERMIT ANY OF MY CODE USED UNDER THE GPL   *
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
#include <stdlib.h>
#include <assert.h>

/* FXRWMutex is the one stress-tested because it utilises all other parts of
the threading classes. Any failures in anything show up here readily, though
it's no use for finding the problem!

For successful test completion, program should exit after two Return presses
cleanly. There appears to be some issue with OutputDebugString() on Win32,
multiple threads and the heap - I get frequently enough hangs in OutputDebugString()
and since it's not just in this code, I'm guessing it's Microsoft's problem.
*/

#define MAX_THREADS 8

static int rnd(int max)
{
	return (int) (rand()*max/RAND_MAX);
}

static struct SharedData
{
	FXRWMutex lock;
	FXString data;
	FXAtomicInt nest;
} shareddata;
static FXThreadLocalStorage<SharedData> dataptr;

class TestThread : public FXThread
{
	int myidx, mycount;
	SharedData *data;
public:
	TestThread(SharedData *_data, int _myidx) : data(_data), myidx(_myidx), mycount(0), FXThread() {}
	void run()
	{
		if(!dataptr) dataptr=data;
		assert(current()==this);
		for(;;)
		{
			SharedData *sd=dataptr;
			//fxmessage("%d",myidx);
			mycount++;
			assert(sd==&shareddata);
			checkForTerminate();
			bool write=rnd(MAX_THREADS/2)==myidx/2;
			{
				FXThread_DTHold dthold;
				FXMtxHold h(sd->lock, write);
				if(write && ++sd->nest>1)
				{
					FXERRG("FAILURE: Permitting multiple writes", 0, FXERRH_ISDEBUG);
				}
				fxmessage(FXString("Thread %1 says data is %2\n").arg(myidx).arg(sd->data).text());
				if(write)
				{
					sd->data.truncate(rnd(16));
					sd->data.insert(rnd(sd->data.length()-1), FXString("Thread %1").arg(myidx));
					fxmessage(FXString("=> Thread %1 altered data to %2\n").arg(myidx).arg(sd->data).text());
				}
				if(write) --sd->nest;
			}
			//FXThread::msleep(100+rnd(50));
		}
	}
	void *cleanup()
	{
		fxmessage(FXString("Thread %1 got %2 iterations\n").arg(myidx).arg(mycount).text());
		return 0;
	}
};
int main( int argc, char** argv)
{
	FXProcess myprocess(argc, argv);
	int n;
	TestThread *threads[MAX_THREADS];
	FXString desc;
	fxmessage(FXString("OS ver=%1\n").arg(FXProcess::hostOSDescription()).text());
	fxmessage(FXString("Main mutex=%1\n").arg((FXuint) &shareddata.lock, 0, 16).text());
	fxmessage("\nPress Return to begin closing down the threads\n\n");

	// Without this this test occupies so much of the scheduler I can't debug!
	shareddata.lock.setSpinCount(0);

	for(n=0; n<MAX_THREADS; n++)
	{
		threads[n]=new TestThread(&shareddata, n);
	}
	for(n=0; n<MAX_THREADS; n++)
	{
		threads[n]->start();
	}
	getchar();
	for(n=0; n<MAX_THREADS; n++)
	{
		threads[n]->requestTermination();
		//threads[n]->wait(FXINFINITE);
	}
	bool alldone;
	do
	{
		alldone=true;
		for(n=0; n<MAX_THREADS; n++)
		{
			if(threads[n])
			{
				alldone=false;
				if(threads[n]->wait(1)) FXDELETE(threads[n]);
			}
		}
	} while(!alldone);
	fxmessage("\nPress Return to end\n");
	getchar();
	fxmessage("Exiting!\n");
	return 0;
}
