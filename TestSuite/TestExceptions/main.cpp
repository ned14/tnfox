/********************************************************************************
*                                                                               *
*                         Exception framework test                              *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003-2007 by Niall Douglas.   All Rights Reserved.              *
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

class BadObject
{
public:
	~BadObject()
	{	// NOTE: This code is normally automatically inserted for you by CppMunge.py
		FXException::int_incDestructorCnt();
		try
		{
			FXERRG("From BadObject", 0, 0);
		}
		catch(FXException &e)
		{
			if(FXException::int_nestedException(e)) throw;
		}
		FXException::int_decDestructorCnt();
	}
};

int main(int argc, char *argv[])
{
	int ret=0;
	FXProcess myprocess(argc, argv);
	FXApp app("TestExceptions");
	app.init(argc,argv);
	FXMainWindow *mainwnd=new FXMainWindow(&app, "Foo");
	app.create();

	printf("Test 1: Throwing basic exception: ");
	FXERRH_TRY
	{
		FXERRH(false, "Assertion test", 0, 0);
	}
	FXERRH_CATCH(FXException &e)
	{
		printf(FXString("Caught exception, message=%1\n").arg(e.message()).text());
	}
	FXERRH_ENDTRY

	printf("\n\nTest 2: Throwing exception during exception throw: ");
	FXERRH_TRY
	{
		BadObject o1, o2;
		FXERRG("First exception", 0, 0);
	}
	FXERRH_CATCH(FXException &e)
	{
		printf("Caught exception, Nesting count=%d, fatal flag=%d\n", e.nestedLen(), e.isFatal());
		printf("\nFull report: %s\n", e.report().text());
		if(e.nestedLen()!=2 || !e.isFatal()) ret=1;
	}
	FXERRH_ENDTRY

	printf("\n\nTest 3: Retrying an error: ");
	FXERRH_TRY
	{
		BadObject o1, o2;
		FXERRG("A test exception. Try Retry to retry me. There should also be two other exceptions thrown during the handling of this exception", 0, 0);
	}
	FXERRH_CATCH(FXException &e)
	{
		printf("Caught exception, now reporting to user\n");
		if(e.nestedLen()!=2 || !e.isFatal()) ret=1;
		e.setFatal(false);
		if(FXProcess::isAutomatedTest())
		{
			FX::FXExceptionDialog f(&app, e);
			f.place(PLACEMENT_CURSOR);
			f.hide();
		}
		else
			FXERRH_REPORT(&app, e);
	}
	FXERRH_ENDTRY
	printf("User chose to cancel\n");

	app.runModalWhileShown(mainwnd);
	app.exit(0);

	printf("\n\nTests complete!\n");
#ifdef WIN32
	if(!FXProcess::isAutomatedTest())
		getchar();
#endif
	return ret;
}
