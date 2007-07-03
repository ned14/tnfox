/********************************************************************************
*                                                                               *
*                      Test of human language translation                       *
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
"\"There are %1 pretty round teapots\":\n"
"\tEN: \"There are %1 pretty round teapots (embedded copy)\"\n"
"\tES: \"Hay %1 teteras redondas bonitas\"\n"
"\t%1=1:\n"
"\t\tEN: \"There is %1 pretty round teapot\"\n"
"\t\tES: \"Hay %1 tetera redonda bonita\"\n"
"\n";
QTRANS_SETTRANSFILE(mytransfile, sizeof(mytransfile));

int main(int argc, char *argv[])
{
	FXProcess myprocess(argc, argv);
	int n;
	const char txt[]="There are %1 pretty round teapots";
	fxmessage("The current language is '%s' in region '%s'\n", QTrans::language().text(), QTrans::country().text());
	QTrans::overrideLanguage("en");
	fxmessage("Translating '%s' as language 'en' ...\n", txt);
	for(n=0; n<5; n++)
	{
		FXString transed=QTrans::tr("main", txt).arg(n);
		fxmessage("%s\n",transed.text());
	}
	QTrans::overrideLanguage("es");
	fxmessage("Translating '%s' as language 'es' ...\n", txt);
	for(n=0; n<5; n++)
	{
		FXString transed=QTrans::tr("main", txt).arg(n);
		fxmessage("%s\n",transed.text());
	}

	fxmessage("All Done!\n");
#ifdef _MSC_VER
	if(!myprocess.isAutomatedTest())
		getchar();
#endif
	return 0;
}
