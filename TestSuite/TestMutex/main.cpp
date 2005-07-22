/********************************************************************************
*                                                                               *
*                               Mutex speed test                                *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2002-2004 by Niall Douglas.   All Rights Reserved.       *
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

/* On my machine (dual Athlon XP 1700 Win2k):

Original:
    SMP Build, old mutex, 1 thread :  49236829  17793594
    SMP Build, old mutex, 2 threads:   4779657   4513450
Non-SMP Build, old mutex, 1 thread : 116414435  27352297

With fastinc() and fastdec():
    SMP Build, old mutex, 1 thread :  51203277  18389113 (4%,     3.3% )
    SMP Build, old mutex, 2 threads:   4793978   5258452 (0.3%,   16.5%)
Non-SMP Build, old mutex, 1 thread : 103305785  27352297 (-11.3%, 0%)
Non-SMP Build, old mutex, 2 threads:  54929964  10978153

With xchg instead of cmpxchg SMP Build mutex got 5337603
*/

#define MAX_THREADS 2
#define MAX_COUNT 100000000
#define MAX_MUTEX 50000000

static FXAtomicInt count;
static QMutex lock;

class TestAtomicInt : public QThread
{
public:
	FXuint taken;
	TestAtomicInt() : taken(0) { }
	void run()
	{
		FXuint start=FXProcess::getMsCount();
		for(FXuint n=0; n<MAX_COUNT; n++)
			count.fastinc();
		FXuint end=FXProcess::getMsCount();
		taken=end-start;
	}
	void *cleanup()
	{
		return 0;
	}
};
class TestMutex : public QThread
{
public:
	FXuint taken;
	TestMutex() : taken(0) { }
	void run()
	{
		FXuint start=FXProcess::getMsCount();
		for(FXuint n=0; n<MAX_MUTEX; n++)
		{
			lock.lock();
			lock.unlock();
		}
		FXuint end=FXProcess::getMsCount();
		taken=end-start;
	}
	void *cleanup()
	{
		return 0;
	}
};
int main( int argc, char** argv)
{
	FXProcess myprocess(argc, argv);
	int n;
	TestAtomicInt threads1[MAX_THREADS];
	TestMutex     threads2[MAX_THREADS];

	// Without this this test occupies so much of the scheduler I can't debug!
	//lock.setSpinCount(0);

	FXuint taken;
	for(n=0; n<MAX_THREADS; n++)
	{
		threads1[n].start(true);
	}
	taken=0;
	for(n=0; n<MAX_THREADS; n++)
	{
		threads1[n].wait();
		taken+=threads1[n].taken;
	}
	if(taken)
	{
		fxmessage("System can perform %llu atomic increments per second\n", (1000LL*MAX_COUNT*MAX_THREADS)/taken);
		fxmessage("Atomic Int is %u\n", (FXuint) count);
	}

	for(n=0; n<MAX_THREADS; n++)
	{
		threads2[n].start(true);
	}
	taken=0;
	for(n=0; n<MAX_THREADS; n++)
	{
		threads2[n].wait();
		taken+=threads2[n].taken;
	}
	if(taken) fxmessage("System can perform %llu mutex lock/unlocks per second\n", (1000LL*MAX_MUTEX*MAX_THREADS)/taken);
	getchar();
	fxmessage("Exiting!\n");
	return 0;
}
