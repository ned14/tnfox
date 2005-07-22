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
#include "DLL/TestDLL.h"
#include "qvaluelist.h"

static void runTest(const char *prefix)
{
	const char *teststring="This is a test teapot";
	fxmessage("%s: The translation of '%s' is '%s'\n", prefix, teststring,
		FXString(QTrans::tr("main", teststring)).text());
	// Test providers code
	QTrans::ProvidedInfoList provided=QTrans::provided();
	fxmessage("QTrans::provided reports:\n");
	for(QTrans::ProvidedInfoList::iterator it=provided.begin(); it!=provided.end(); ++it)
	{
		fxmessage("Module: %s Language: %s Country: %s\n",
			(*it).module.text(), (*it).language.text(), (*it).country.text());
	}
}

int main(int argc, char *argv[])
{
	FXProcess myprocess(argc, argv);
	fxmessage("TnFOX DLL test:\n"
		      "-=-=-=-=-=-=-=-\n");
	FXERRH_TRY
	{
		runTest("Before");

		FXProcess::dllHandle *dllh=new FXProcess::dllHandle(FXProcess::dllLoad("MyTestDLL"));
		typedef Generic::TL::create<Test::TestDLL *>::value TestDLLspec;
		Generic::Functor<TestDLLspec> findDLL=dllh->resolve<TestDLLspec>("findDLL");
		Test::TestDLL *testdll=findDLL();
		fxmessage("findDLL() returns 0x%p\n", testdll);
		// Test that translations loaded by DLL's are found
		runTest("\nWith DLL loaded");
		FXDELETE(dllh);
		// Test that translations loaded by kicked out DLL's are NOT found
		runTest("\nWith DLL unloaded");
		fxmessage("This should be the same as the first one!\n\n");
	}
	FXERRH_CATCH(FXException &e)
	{
		fxmessage("Exception: %s\n", e.report().text());
		return 1;
	}
	FXERRH_ENDTRY

	fxmessage("All Done!\n");
#ifdef _MSC_VER
	getchar();
#endif
	return 0;
}
