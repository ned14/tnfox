/********************************************************************************
*                                                                               *
*                           Test of embedding Python                            *
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
#include "FXPython.h"

using namespace boost::python;

static void doThreads1(FXPythonInterp *interp, int ino)
{
	FXPython_CtxHold ctxhold(interp);
	py( "class TestThread(FXThread):\n"
		"    def __init__(self, interpno, threadno):\n"
		"        FXThread.__init__(self)\n"
		"        self.interpno=interpno\n"
		"        self.threadno=threadno\n"
		"    def run(self):\n"
		"        while not self.checkForTerminate():\n"
		"            print 'I am TestThread no',self.threadno,'in interpreter',self.interpno\n"
		"            #FXThread.msleep(1)\n"
		"    def cleanup(self):\n"
		"        return 0\n");
	py(FXString("a=TestThread(%1, 1); b=TestThread(%2, 2)\n").arg(ino).arg(ino));;
	py( "a.start(); b.start();");
}
static void doThreads2(FXPythonInterp *interp)
{
	FXPython_CtxHold ctxhold(interp);
	handle<> ah=pyeval("a"), bh=pyeval("b");
	py( "a.requestTermination(); b.requestTermination();\n"
		"a.wait(); b.wait();\n"
		"del a; del b\n"
		"print 'All threads have ended in this interpreter'\n"
		);
	assert(ah.get()->ob_refcnt==1); assert(bh.get()->ob_refcnt==1);
}

namespace FX {
	extern void *testfunc();
}
int main(int argc, char *argv[])
{
	FXProcess myprocess(argc, argv);
	fxmessage("TnFOX Python test:\n"
		      "-=-=-=-=-=-=-=-=-=\n");
	try
	{
		fxmessage("Working with Python %s\n", FXPython::version().text());
		FXPythonInterp mainpython(argc, argv, "main interpreter");
		py("print 'If you can read this, python prints!'");

		void *ret=testfunc();
		printf("Typeinfo of pointer returned from DLL is %p,%p (%s)\n", &typeid(*(FXException *) ret), typeid(*(FXException *) ret).name(), typeid(*(FXException *) ret).name());
		printf("Typeinfo of FXException in EXE is %p,%p (%s)\n", &typeid(FXException), typeid(FXException).name(), typeid(FXException).name());
		printf("Typeinfo of FXPythonException in EXE is %p,%p (%s)\n", &typeid(FXPythonException), typeid(FXPythonException).name(), typeid(FXPythonException).name());

		{
			FXProcess::MappedFileInfoList list=FXProcess::mappedFiles();
			for(FXProcess::MappedFileInfoList::iterator it=list.begin(); it!=list.end(); ++it)
			{
				FXString temp2("  %1 to %2 %3%4%5%6 %7\n");
				temp2.arg((FXulong) (*it).startaddr,-8,16).arg((FXulong) (*it).endaddr,-8,16);
				temp2.arg(((*it).read) ? 'r' : '-').arg(((*it).write) ? 'w' : '-').arg(((*it).execute) ? 'x' : '-').arg(((*it).copyonwrite) ? 'c' : '-');
				temp2.arg((*it).path);
				fxmessage(temp2.text());
			}
		}
		FXERRH_TRY
		{
			py( "print '\\nThrowing a python exception ...'\n"
				"raise IOError, 'A parameter to a python exception'");
			FXPythonException f("Test", 0);
			FXERRH_THROW(f);
		}
		FXERRH_CATCH(FXException &e)
		{
			fxmessage("C++ exception caught: %s\n", e.report().text());
		}
		FXERRH_ENDTRY

		py("print '\\nCalculating some fibonacci numbers ...'");
		py( "def fibGen():\n"
			"    a=1; b=1; yield a\n"
			"    while True:\n"
			"        c=b; b+=a; a=c\n"
			"        yield a\n"
			"def calcFibonacci(no):\n"
			"    it=fibGen()\n"
			"    return [it.next() for n in range(0,no)]\n"
			"print 'First ten fibonacci numbers are',calcFibonacci(10)");
		{
			fxmessage("Invoking fibonacci number calculation via BPL ...\n");
			object calcFibonacci(pyeval("calcFibonacci"));
			list result(calcFibonacci(10));
			const char *resultstr=extract<const char *>(str(result));
			fxmessage("List from C++ is: %s\n", resultstr);
			list slice(result.slice(_, 3));
			resultstr=extract<const char *>(str(slice));
			fxmessage("Slice [:3] from C++ is: %s\n", resultstr);
			object item7(result[7]);
			resultstr=extract<const char *>(str(item7));
			fxmessage("Item [7] from C++ is: %s\n", resultstr);
		}
		{
			py("def printNum(no): print no");
			object printNum(pyeval("printNum"));
			long_ bignum((FXulong)-1);
			fxmessage("\nThe biggest number C++ can handle: "); printNum(bignum);
			bignum*=bignum;
			fxmessage("That number multiplied by itself: "); printNum(bignum);
		}
		fxmessage("\nStarting multiple threads in multiple interpreters ...\n");
		FXPythonInterp ointerp;
		ointerp.unsetContext();
		doThreads1(&mainpython, 0);
		doThreads1(&ointerp, 1);
		fxmessage("Press Return to end\n");
		mainpython.unsetContext();
		getchar();
		//mainpython.setContext();
		doThreads2(&ointerp);
		doThreads2(&mainpython);
	}
	catch(const FXException &e)
	{
		fxmessage("Exception thrown: %s\n", e.report().text());
	}

	fxmessage("All Done!\n");
#ifdef _MSC_VER
	getchar();
#endif
	return 0;
}
