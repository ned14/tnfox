/********************************************************************************
*                                                                               *
*                                Refed Object test                              *
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

#include "fx.h"
#include <qdict.h>
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

struct Foo : public FXRefedObject<FXAtomicInt>
{
	int number;
	Foo(int no) : number(no)
	{
		fxmessage("Foo %p constructed\n", this);
	}
	~Foo()
	{
		fxmessage("Foo %p destroyed\n", this);
	}
};
typedef FXRefingObject<Foo> FooHold;
typedef FXLRUCache<QDict<FooHold>, FooHold> FooCache;

static void printFoo(Foo *fh)
{
	fxmessage("Foo no %d has refcount %d, ", fh->number, (int) fh->refCount());
}

int main(int argc, char *argv[])
{
	FXProcess myprocess(argc, argv);
	printf("FXRefingObject test\n"
		   "-=-=-=-=-=-=-=-=-=-\n");
	FooCache cache(3);
	struct Foos { Foo *a, *b, *c; void print() { printFoo(a); printFoo(b); printFoo(c); fxmessage("\n"); } } foos;
	FXERRHM(foos.a=new Foo(1));
	FXERRHM(foos.b=new Foo(2));
	FXERRHM(foos.c=new Foo(3));
	foos.print();

	FXLRUCacheIterator<FooCache> it(cache);
	{
		FooHold a(foos.a), b(foos.b), c(foos.c);
		FooHold n(a);
		foos.print();
		n=b;
		foos.print();
		fxmessage("\nInserting into cache ...\n");
		cache.insert("Hello", a);
		cache.insert("Niall", b);
		cache.insert("Does", c);
		foos.print();
		cache.insert("this", n);
		cache.insert("work?", a);
		foos.print();
	}
	foos.print();
	// Should cause deletion
	cache.insert("Heh", foos.a);
	foos.print();
	printf("\n\nTests complete!\n");
#ifdef WIN32
	getchar();
#endif
	return 0;
}
