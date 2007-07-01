/********************************************************************************
*                                                                               *
*                               SQL Database test                               *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2005-2007 by Niall Douglas.   All Rights Reserved.       *
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


#include "QFileInfo.h"
#include "TnFXSQLDB_ipc.h"
#include "FXFile.h"
#include "QPipe.h"
#include <stdio.h>
#include <assert.h>
#include "FXMemDbg.h"
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

using namespace FX;

#if defined(USE_POSIX)
#include "TnFXSQLDB_sqlite3.h"
extern void pullSQLite3IntoEXE()
{
	TnFXSQLDB_sqlite3 foo("Hello");
}
#endif

#ifdef _MSC_VER
#include "../../windows/WindowsGubbins.h"
extern "C" void _cexit(void);
static void ForceDLLDeinit(const char *module)
{
	try
	{
		typedef BOOL (WINAPI *dllMainSpec)(HINSTANCE, DWORD, LPVOID);
		HMODULE moduleaddr=GetModuleHandle(module);
		FXERRHWIN(moduleaddr);
		PIMAGE_DOS_HEADER dosHeader=(PIMAGE_DOS_HEADER) moduleaddr;
		PIMAGE_NT_HEADERS ntHeader=(PIMAGE_NT_HEADERS) FXOFFSETPTR(dosHeader, dosHeader->e_lfanew);
		dllMainSpec dllMain=(dllMainSpec) FXOFFSETPTR(moduleaddr, ntHeader->OptionalHeader.AddressOfEntryPoint);
		// Fake call of DLL_PROCESS_DETACH
		dllMain(moduleaddr, DLL_PROCESS_DETACH, (LPVOID) 1);
	}
	catch(FXException &e)
	{
		fxwarning("Exception %s\n", e.report().text());
	}
}
#endif


static const char dbname[]="TestSQLDB.sqlite3";

static const char *numbers1[]  ={ "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine" };
static const char *numbers10[] ={ "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen" };
static const char *numbers100[]={ 0, 0, "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety" };
static inline FXString numberTo100(FXuint no)
{
	FXString ret;
	if(no>=20)
	{
		ret.append(numbers100[no/10]);
		no=no % 10;
		if(no)
		{
			ret.append('-');
			ret.append(numbers1[no]);
		}
		return ret;
	}
	if(no>=10)
	{
		ret.append(numbers10[no-10]);
		return ret;
	}
	ret.append(numbers1[no]);
	return ret;
}
static inline FXString numberTo1000(FXuint no)
{
	FXString ret;
	if(no>=100)
	{
		ret.append(numbers1[no/100]);
		ret.append(" hundred");
		no=no % 100;
		if(no)
		{
			ret.append(" and ");
			ret.append(numberTo100(no));
		}
	}
	else
		ret.append(numberTo100(no));
	return ret;
}
static FXString NumDescr(FXuint no)
{
	FXString ret;
	if(no>=1000)
	{	// Thousands
		ret.append(numberTo1000(no/1000)+" thousand, ");
		no=no % 1000;
	}
	ret.append(numberTo1000(no));
	return ret;
}

typedef TnFXSQLDBIPCMsgs<100>::Chunk MyMsgChunk;
static class MyRegistry : public FXIPCMsgRegistry
{
	MyMsgChunk mychunk;
public:
	MyRegistry() : mychunk(this)
	{
	}
} myregistry;
class MyChannel : public FXIPCChannel
{
public:
	MyChannel(QIODeviceS *dev, const char *name="TestSQLDB client") : FXIPCChannel(myregistry, dev, false, 0, name)
	{
		//setPrintStatistics(true);
		setUnreliable(true);
	}
	~MyChannel()
	{
		if(running())
		{
			requestTermination();
			wait();
		}
	}
	virtual HandledCode msgReceived(FXIPCMsg *msg)
	{
		return NotHandled;
	}
};
class MyServer : public MyChannel
{
public:
	TnFXSQLDBServer server;
	MyServer(QIODeviceS *dev) : MyChannel(dev, "TestSQLDB server")
	{
		setPrintStatistics(false);
	}
	~MyServer()
	{
		requestTermination();
		wait();
	}
	virtual HandledCode msgReceived(FXIPCMsg *msg)
	{
		return server.handleMsg(msg);
	}
};

static void printdbsize()
{
	QFileInfo mydbinfo(dbname);
	fxmessage("SQLite3 database is now %s long\n", mydbinfo.sizeAsString().text());
}

static void insert(TnFXSQLDB *db, FXuint no)
{
	TnFXSQLDBStatementRef s=db->prepare("INSERT INTO 'test' ('value', 'text') VALUES(:value, :text);");
	for(FXuint n=0; n<no; n++)
	{
		s->bind(":value", n);
		s->bind(":text", NumDescr(n));
		s->immediate();
		//fxmessage("Inserted %u=%s\n", n, NumDescr(n).text());
	}
}

template<typename type> struct PrintCPPType
{
	static void Do()
	{
		fxmessage("%s", Generic::typeInfo<type>().name().text());
	}
};

class FooString : public FXString
{
};

int main( int argc, char** argv)
{ {
	FXProcess myprocess(argc, argv);
	FXulong maxulong=Generic::BiggestValue<FXulong>::value;
	FXlong   maxlong=Generic::BiggestValue<FXlong>::value;
	FXlong   minlong=Generic::BiggestValue<FXlong, true>::value;
	fxmessage("%s", FXString("64 bit ranges: %1, %3 to %2\n").arg(maxulong).arg(maxlong).arg(minlong).text());

	FXuint maxuint=Generic::BiggestValue<FXuint>::value;
	FXint   maxint=Generic::BiggestValue<FXint>::value;
	FXint   minint=Generic::BiggestValue<FXint, true>::value;
	fxmessage("%s", FXString("32 bit ranges: %1, %3 to %2\n").arg(maxuint).arg(maxint).arg(minint).text());

	FXushort maxushort=Generic::BiggestValue<FXushort>::value;
	FXshort   maxshort=Generic::BiggestValue<FXshort>::value;
	FXshort   minshort=Generic::BiggestValue<FXshort, true>::value;
	fxmessage("%s", FXString("16 bit ranges: %1, %3 to %2\n").arg(maxushort).arg(maxshort).arg(minshort).text());

#if 0
	printf("hasSerialise=%d, %d, %d\n", Generic::hasSerialise<const char *>::value, Generic::hasSerialise<Generic::NullType>::value, Generic::hasSerialise<FXuint>::value);
	printf("hasSerialise=%d, %d, %d\n", Generic::hasSerialise<FXString>::value, Generic::hasSerialise<FooString>::value, Generic::hasSerialise<FXIconDict *>::value);

	printf("hasDeserialise=%d, %d, %d\n", Generic::hasDeserialise<const char *>::value, Generic::hasDeserialise<Generic::NullType>::value, Generic::hasDeserialise<FXuint>::value);
	printf("hasDeserialise=%d, %d, %d\n", Generic::hasDeserialise<FXString>::value, Generic::hasDeserialise<FooString>::value, Generic::hasDeserialise<FXIconDict *>::value);
#endif

	if(0)
	{	// Little string test
		fxmessage("\nFXString cached inserts test:\n"
					"-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
		static const int argsno=10, runs=100;
		FXString orig, args[argsno];
		FXuint seed=78787878;
		for(int n=0; n<64; n++) orig.append((FXchar)('a'+(('Z'-'A')*(fxrandom(seed) & 0xff))/255));
		for(int run=0; run<runs; run++)
		{
			FXString copy(orig);
			for(int n=0; n<argsno; n++)
			{
				FXint s=(copy.length()*(fxrandom(seed) & 0xffff))/65535;
				FXint e, e2=(copy.length()*(fxrandom(seed) & 0xffff))/(65535*8);
				if(copy[s]=='%') s++;
				if(copy[s-1]=='%') s++;
				for(e=0; e<e2 && s+e<copy.length() && copy[s+e]!='%'; e++);
				args[n]=copy.mid(s, e);
				copy.replace(s, e, "%"+FXString::number(n));
			}
			FXString fixd(copy);
			FXMEMDBG_TESTHEAP;
			for(int n=0; n<argsno; n++)
			{
				fixd.arg(args[n]);
				FXMEMDBG_TESTHEAP;
			}
			if(orig!=fixd)
			{
				fxmessage("Orig=%s\n", orig.text());
				fxmessage("Fixd=%s\n", fixd.text());
				fxmessage("Copy=%s\n", copy.text());
				for(int n=0; n<argsno; n++)
				{
					fxmessage("arg%d=%s\n", n, args[n].text());
				}
				assert(0);
				fxerror("Inserted into string does not equal original!\n");
			}
		}
	}
	FXERRH_TRY
	{
		fxmessage("\nSQL Database test:\n"
					"-=-=-=-=-=-=-=-=-=\n");
		FXFile::remove(dbname);
		QPipe mypipeS("TestSQLDBPipe"), mypipeC("TestSQLDBPipe");
		MyServer myserver(&mypipeS);
		MyChannel mychannel(&mypipeC);
		FXAutoPtr<TnFXSQLDB> mydb;
		if(0)	// Testing IPC tests SQLite!
		{
			mydb=TnFXSQLDBRegistry::make("SQLite3", dbname);
			FXERRH(mydb, "Couldn't make a SQLite3 driver!!", 0, FXERRH_ISDEBUG);
		}
		else
		{
			mypipeS.create();
			mypipeC.open();
			myserver.start();
			mychannel.start();
			mydb=TnFXSQLDBRegistry::make("IPC", FXString("SQLite3:")+dbname);
			FXERRH(mydb, "Couldn't make an IPC driver!!", 0, FXERRH_ISDEBUG);
			TnFXSQLDB_ipc *db=dynamic_cast<TnFXSQLDB_ipc *>(PtrPtr(mydb));
			db->setIPCChannel<MyMsgChunk>(&mychannel);
			//db->setIsAsynchronous();
			myserver.server.setIPCChannel<MyMsgChunk>(&myserver);
			myserver.server.addDatabase("SQLite3", dbname);
		}
		mydb->open();
		mydb->immediate("PRAGMA auto_vacuum=1; PRAGMA synchronous=OFF;");	// Enable auto-vacuuming and disable sync
		mydb->immediate("CREATE TABLE test(id INTEGER PRIMARY KEY, 'value' INTEGER, 'text' VARCHAR(256), 'when' TIMESTAMP DEFAULT CURRENT_TIMESTAMP);");
		printdbsize();

		FXulong begin, end;
		TnFXSQLDBStatementRef s;

		fxmessage("\nInserting 1000 records without a transaction ...\n");
		begin=FXProcess::getNsCount();
		insert(PtrPtr(mydb), 1000);
		mydb->synchronise();
		end=FXProcess::getNsCount();
		printdbsize();
		fxmessage("Took %lf secs (%lf per second)\n", (end-begin)/1000000000.0, 1000/((end-begin)/1000000000.0));

		fxmessage("\nInserting 5000 records with a transaction ...\n");
		begin=FXProcess::getNsCount();
		mydb->immediate("BEGIN TRANSACTION;");
		insert(PtrPtr(mydb), 5000);
		mydb->immediate("END TRANSACTION;");
		mydb->synchronise();
		end=FXProcess::getNsCount();
		fxmessage("Took %lf secs (%lf per second)\n", (end-begin)/1000000000.0, 5000/((end-begin)/1000000000.0));
		printdbsize();

		fxmessage("\nDeleting 5,000 records without a transaction ...\n");
		begin=FXProcess::getNsCount();
		mydb->immediate("DELETE FROM 'test' WHERE id<=5000;");
		mydb->synchronise();
		end=FXProcess::getNsCount();
		fxmessage("Took %lf secs (%lf per second)\n", (end-begin)/1000000000.0, 5000/((end-begin)/1000000000.0));
		printdbsize();

		fxmessage("\nSelecting all records with 'twenty' in them ...\n");
		begin=FXProcess::getNsCount();
		int n=0;
		for(TnFXSQLDBCursorRef c=mydb->execute("SELECT text FROM 'test' WHERE text LIKE '%twenty%';"); !c->atEnd(); c->next(), n++)
		{
			if(!n)
				end=FXProcess::getNsCount();
			TnFXSQLDB::SQLDataType datatype;
			FXint size;
			c->type(datatype, size, 0);
			fxmessage("Entry %d (header type %s(%d), colname=%s): %s\n", c->at(), TnFXSQLDB::sql92TypeAsString(datatype), size, c->header(0)->get<const char *>(), c->data(0)->get<const char *>());
		}
		fxmessage("Took %lf secs (%lf per second)\n", (end-begin)/1000000000.0, n/((end-begin)/1000000000.0));
		printdbsize();

		FXMEMDBG_TESTHEAP;
		fxmessage("\nInserting ReadMe.txt ...\n");
		FXFile fh("../../ReadMe.txt");
		fh.open(IO_ReadOnly);
		begin=FXProcess::getNsCount();
		TnFXSQLDBStatementRef stmt=mydb->prepare("INSERT INTO 'test' ('value', 'text') VALUES(?2, ?1);");
		stmt->bind(1, 0xdeadbeef);		// Test a top bit set value to test overflow upcasting
		stmt->bind(0, fh);
		stmt->immediate();
		mydb->synchronise();
		end=FXProcess::getNsCount();
		fxmessage("Took %lf secs (%lf per second)\n", (end-begin)/1000000000.0, 5000/((end-begin)/1000000000.0));
		printdbsize();

		FXMEMDBG_TESTHEAP;
		fxmessage("\nReading and comparing to ReadMe.txt (size=%u bytes)...\n", (FXuint) fh.size());
		fh.at(0);
		{
			begin=FXProcess::getNsCount();
			TnFXSQLDBCursorRef c=mydb->execute("SELECT * FROM 'test' WHERE value=3735928559;");
			end=FXProcess::getNsCount();
			fxmessage("SELECT took %lf secs (%lf per second)\n", (end-begin)/1000000000.0, 1/((end-begin)/1000000000.0));
			FXERRH(!c->atEnd(), "There should be some records returned!", 0, FXERRH_ISDEBUG);
			fxmessage("There were %u columns returned (there should be 4)\n", c->columns());
			for(FXuint n=0; n<c->columns(); n++)
			{
				begin=FXProcess::getNsCount();
				TnFXSQLDBColumnRef header=c->header(n);
				TnFXSQLDBColumnRef data=c->data(n);
				end=FXProcess::getNsCount();
				fxmessage("\nGetting header & data took %lf secs (%lf per second)\n", (end-begin)/1000000000.0, 1/((end-begin)/1000000000.0));
				fxmessage("Column %u,%d is called '%s' SQL type %s size=%d\n    C++ type=", header->column(), header->row(),
					header->get<const char *>(), TnFXSQLDB::sql92TypeAsString(data->type()), data->size());
				TnFXSQLDB::toCPPType<PrintCPPType>(data->type());
				switch(data->type())
				{
				case TnFXSQLDB::VarChar:
					fxmessage(" contents=%s\n", data->get<const char *>());
					break;
				case TnFXSQLDB::BigInt:
					fxmessage(FXString(" contents=%1 (0x%2)\n").arg(data->get<FXint>()).arg(data->get<FXint>(), 0, 16).text());
					break;
				case TnFXSQLDB::Timestamp:
					fxmessage(" contents=%s\n", data->get<FXTime>().asString().text());
					break;
				case TnFXSQLDB::BLOB:
					{
						QBuffer b;
						b.open(IO_ReadWrite);
						data->get(b);
						FXString temp((FXchar *) b.buffer().data(), 64);
						fxmessage(" contents (first 64 bytes of %u bytes)=\n%s\n", (FXuint) b.size(), temp.text());
						break;
					}
				default:
					fxmessage(" contents=Null\n");
					break;
				}
			}
		}
		//mychannel.setPrintStatistics(true);
		FXMEMDBG_TESTHEAP;
	}
	FXERRH_CATCH(FXException &e)
	{
		fxerror("%s\n", e.report().text());
	}
	FXERRH_ENDTRY

	fxmessage("\nAll Done!\n");
	if(!myprocess.isAutomatedTest())
		getchar();
	fxmessage("Exiting!\n");
	}
#if defined(_MSC_VER) && 0
	fxmessage("Force deiniting myself ...\n"); 
	_cexit();
	fxmessage("Force deiniting TnFOX ...\n");
	ForceDLLDeinit("TnFOX-0.86d.dll");
#endif
	return 0;
}
