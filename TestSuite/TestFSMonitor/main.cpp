/********************************************************************************
*                                                                               *
*                       Test of filing system monitor                           *
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

static void print(const FXFileInfo &fi)
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
static void handler(FXFSMonitor::Change change, const FXFileInfo &oldfi, const FXFileInfo &newfi)
{
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
	FXProcess myprocess(argc, argv);
	fxmessage("TnFOX Filing system Monitor test:\n"
		      "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	fxmessage("List of drives on this system: %s\n", FXDir::drives().join(" ").text());
	fxmessage("List of this directory:\n");
	FXDir dir(".", "*", FXDir::Time|FXDir::DirsFirst|FXDir::Reversed);
	FXuint before=FXProcess::getMsCount();
	dir.entryList();
	FXuint after1=FXProcess::getMsCount();
	const QFileInfoList *list=dir.entryInfoList();
	FXuint after2=FXProcess::getMsCount();
	fxmessage("  (took %dms for list, %dms for file info fetch)\n", after1-before, after2-after1);
	printDir(list);
	fxmessage("\nMonitoring for changes ...\n");
	FXFSMonitor::add(".", FXFSMonitor::ChangeHandler(handler));
	FXThread::sleep(1);
	fxmessage("Making changes ...\n");
	FXFile file("MyTestFile.txt");
	file.open(IO_ReadWrite);
	file.truncate(1024);
	FXThread::sleep(1);
	file.truncate(4096);
	file.close();
	FXThread::sleep(1);
	FXFile::move("MyTestFile.txt", "MyTestFile2.txt");
	FXThread::sleep(1);
	FXFile::setPermissions("MyTestFile2.txt", FXACL(FXACL::File));
	FXThread::sleep(1);
	FXFile::remove("MyTestFile.txt");
	FXFile::remove("MyTestFile2.txt");
	fxmessage("\nAll Done!\n");
#ifdef _MSC_VER
	getchar();
#endif
	return 0;
}
