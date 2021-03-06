/********************************************************************************
*                                                                               *
*                       Test of filing system monitor                           *
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

static void print(const QFileInfo &fi)
{
	FXString temp("%1 (size %2, modified %3) %4");
	temp.arg(fi.fileName()).arg(fi.sizeAsString()).arg(fi.lastModifiedAsString())
		.arg(fi.permissionsAsString());
	fxmessage("%s\n", temp.text());
}
static void printDir(const QFileInfoList *list)
{
	for(QFileInfoList::const_iterator it=list->begin(); it!=list->end(); ++it)
	{
		print(*it);
	}
}
static int called;
static void handler(FXFSMonitor::Change change, const QFileInfo &oldfi, const QFileInfo &newfi)
{
	called++;
	fxmessage("Item changed to:\n");
	print(newfi);
	if(change.modified)
	{
		fxmessage("Code was modified\n\n");
	}
	if(change.created)
	{
		fxmessage("Code was created\n\n");
	}
	if(change.deleted)
	{
		fxmessage("Code was deleted\n\n");
	}
	if(change.renamed)
	{
		fxmessage("Code was renamed (from %s)\n\n", oldfi.fileName().text());
	}
	if(change.attrib)
	{
		fxmessage("Code was attributes changed\n\n");
	}
	if(change.security)
	{
		fxmessage("Code was security changed\n\n");
	}
}

int main(int argc, char *argv[])
{
	int ret=0;
	FXProcess myprocess(argc, argv);
	fxmessage("TnFOX Filing system Monitor test:\n"
		      "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	fxmessage("List of drives on this system: %s\n", QDir::drives().join(" ").text());
	fxmessage("List of this directory:\n");
	QDir dir(".", "*", QDir::Time|QDir::DirsFirst|QDir::Reversed);
	FXuint before=FXProcess::getMsCount();
	dir.entryList();
	FXuint after1=FXProcess::getMsCount();
	const QFileInfoList *list=dir.entryInfoList();
	FXuint after2=FXProcess::getMsCount();
	fxmessage("  (took %dms for list, %dms for file info fetch)\n", after1-before, after2-after1);
	printDir(list);
	fxmessage("\nMonitoring for changes ...\n");
	FXFSMonitor::add(".", FXFSMonitor::ChangeHandler(handler));
	QThread::sleep(1);
	fxmessage("Making changes ...\n");
	QFile file("MyTestFile.txt");
	fxmessage("About to open file ...\n");
	file.open(IO_ReadWrite);
	file.truncate(1024);
	fxmessage("MyTestFile.txt opened and set to 1024 bytes long\n");
	QThread::sleep(1);
	file.truncate(4096);
	file.close();
	fxmessage("MyTestFile.txt set to 4096 bytes long and closed\n");
	QThread::sleep(1);
	FXFile::rename("MyTestFile.txt", "MyTestFile2.txt");
	fxmessage("File renamed from MyTestFile.txt to MyTestFile2.txt\n");
	QThread::sleep(1);
	QFile::setPermissions("MyTestFile2.txt", FXACL(FXACL::File));
	fxmessage("Changed permissions on MyTestFile2.txt\n");
	QThread::sleep(1);
	FXFile::remove("MyTestFile.txt");
	FXFile::remove("MyTestFile2.txt");
	fxmessage("Deleted MyTestFile.txt and MyTestFile2.txt\n");
	QThread::sleep(5);
	fxmessage("\nChange handler was called %d times\n", called);
	if(called<3)
	{
		fxwarning("FAILED: Change handler should have been called at least three times!\n");
		ret=1;
	}
	fxmessage("\nAll Done!\n");
	if(!myprocess.isAutomatedTest())
		getchar();
	return ret;
}
