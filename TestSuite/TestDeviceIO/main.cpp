/********************************************************************************
*                                                                               *
*                              Test of i/o classes                              *
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
#include <qptrlist.h>
#include <qcstring.h>

struct DevInfo
{
	bool isSynch, amSocket;
	FXString name;
	QIODevice *rdev, *wdev;
	DevInfo(const FXString &n, QIODevice *_rdev, QIODevice *_wdev=0) : isSynch(_wdev!=0), amSocket(false), name(n), rdev(_rdev), wdev((_wdev) ? _wdev : _rdev) { }
};

class IOThread : public QThread
{
public:
	DevInfo *reader;
	FXuint crc;
	FXZeroedWait byteCount;
	IOThread(DevInfo *r, int bc) : reader(r), crc(0), byteCount(bc), QThread() { }
	virtual void run()
	{
		char buffer[65536];
		if(reader->amSocket)
		{
			QBlkSocket *rdev=(QBlkSocket *) reader->rdev;
			rdev=rdev->waitForConnection();
			delete reader->rdev;
			reader->rdev=rdev;
		}
		QIODevice *rdev=reader->rdev;
		rdev->open(IO_ReadOnly);
		for(;;)
		{
			FXuval read=rdev->readBlock(buffer, sizeof(buffer));
			crc=fxadler32(crc, (FXuchar *) buffer, read);
			byteCount-=read;
		}
	}
	virtual void *cleanup()
	{
		return 0;
	}
};

int main(int argc, char *argv[])
{
	FXProcess myprocess(argc, argv);
	fxmessage("These tests will leave large files on your hard drive\n"
			  "You will probably want to delete these afterwards\n");
	FXERRH_TRY
	{
		if(1)
		{	// Basic i/o test
			fxmessage("\nGeneral purpose i/o test:\n"
						"-=-=-=-=-=-=-=-=-=-=-=-=-\n");
			FXFile fh("../ReadMe.txt");
			QBuffer bufferh;
			fh.open(IO_ReadOnly);
			bufferh.open(IO_ReadWrite);
			FXStream sbufferh(&bufferh);
			char buffer[32];
			memset(buffer, 0, 32);
			int count=0;
			while(!fh.atEnd())
			{	// Obviously there are much more efficient ways than this
				int read=fh.readBlock(buffer, 5);
				sbufferh << buffer[0]; count++;
				fh.at(fh.at()-read+1);
			}
			fxmessage("Size of main.cpp is %d (reported by filing system: %ld)\n", count, fh.size());
			fh.close();
			int i;
			for(i=5; i<32 && !buffer[i]; i++);
			if(32!=i) fxmessage("Buffer overwrite occurred (failure)\n");

			fxmessage("\nEndian conversion & mixed read-writes test:\n"
					  "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
			QBuffer buffer2h;
			buffer2h.open(IO_ReadWrite);
			FXStream sbuffer2h(&buffer2h);
			sbuffer2h.swapBytes(true);
			bufferh.truncate(bufferh.size() & ~7);
			for(bufferh.at(0); !sbufferh.atEnd(); )
			{
				FXulong c;
				sbufferh >> c;
				sbuffer2h << c;
				sbuffer2h.rewind(8);
				sbuffer2h.swapBytes(false);
				sbuffer2h >> c;
				sbuffer2h.swapBytes(true);
				sbuffer2h << c;
			}
			if(buffer2h.size()!=bufferh.size()*2)
				fxmessage("Endian conversion and mixed read-writes test failed\n");

			fxmessage("\nGZip device test:\n"
					  "-=-=-=-=-=-=-=-=-=-\n");
			FXFile zipped("IOTestOutput.gz");
			QGZipDevice gzipdev(&zipped);
			gzipdev.open(IO_WriteOnly);
			bufferh.at(0);
			buffer2h.at(0);
			FXStream sgzip(&gzipdev);
			sgzip << FXString("The original main.cpp:\n") << bufferh;
			sgzip << FXString("\nNow the mangled copy:\n") << buffer2h;
			sgzip << FXString("\nAnd the end is right NOW");
			fxmessage("Uncompressed data is %ld bytes long\n", gzipdev.size());
			gzipdev.close();
			fxmessage("Data gzipped is %ld bytes long\n", zipped.size());
			zipped.close();
			fxmessage("Test file saved out as '%s' - try testing it with your favourite decompression utility\n", zipped.name().text());
			gzipdev.open(IO_ReadOnly);
			fxmessage("Loaded in and decompressed file (it's %ld bytes), first 256 bytes are:\n", zipped.size());
			for(int i2=0; i2<256; i2++)
			{
				fxmessage("%c",gzipdev.getch());
			}
		}

		if(1)
		{
			const FXuint testsize=128*1024*1024;
			fxmessage("\nPerformance test:\n"
					    "-=-=-=-=-=-=-=-=-\n");
			QPtrList<DevInfo> devs(true);
			if(1) {
				devs.append(new DevInfo("FXFile", new FXFile("BigFile3.txt")));
			}
			if(1) {
				QMemMap *dev=new QMemMap("BigFile2.txt");
				dev->open(IO_WriteOnly);
				dev->truncate(testsize);
				dev->mapIn();
				devs.append(new DevInfo("QMemMap", dev));
			}
			if(1) {
				QBuffer *buff=new QBuffer;
				buff->open(IO_WriteOnly);
				buff->truncate(testsize);
				devs.append(new DevInfo("QBuffer", buff));
			}
			if(1) {
				QLocalPipe *p=new QLocalPipe;
				p->setGranularity(1024*1024);
				devs.append(new DevInfo("QLocalPipe", new QLocalPipe(p->clientEnd()), p));
				p->open(IO_WriteOnly);
			}
			if(1) {
				QPipe *r=new QPipe("TestPipe", true), *w=new QPipe("TestPipe", true);
				w->create(IO_WriteOnly);
				devs.append(new DevInfo("QPipe", r, w));
			}
			if(1) {
				QBlkSocket *server=new QBlkSocket,*w;
				server->create(IO_ReadOnly);
				w=new QBlkSocket(QHOSTADDRESS_LOCALHOST, server->port());
				w->open(IO_WriteOnly);
				DevInfo *di=new DevInfo("FXSocket", server, w);
				di->amSocket=true;
				devs.append(di);
			}
			QBuffer largefile;
			FXStream s(&largefile);
			fxmessage("Writing out test file ...\n");
			largefile.open(IO_WriteOnly);
			largefile.truncate(testsize);
			FXuint origCRC=0;
			{
				FXuint tbuffer[16384/4];
				int n;
				for(n=0; n<16384/4; n++)
					tbuffer[n]=rand();
				for(n=0; n<testsize; n+=16384)
				{
					largefile.writeBlock((char *) tbuffer, 16384);
					origCRC=fxadler32(origCRC, (FXuchar *) tbuffer, 16384);
				}
			}
			largefile.close();
			largefile.open(IO_ReadOnly);
			DevInfo *devi=0;
			for(QPtrListIterator<DevInfo> it(devs); (devi=it.current()); ++it)
			{
				double taken;
				largefile.at(0);
				if(devi->isSynch)
				{
					QIODevice *dev=devi->wdev;
					IOThread *t=new IOThread(devi, testsize);
					t->start(true);
					fxmessage("Writing lots of data to a %s ...\n", devi->name.text());
					FXuint time=FXProcess::getMsCount();
					s >> *dev;
					t->byteCount.wait();
					taken=(FXProcess::getMsCount()-time)/1000.0;
					t->requestTermination();
					t->wait();
					if(t->crc!=origCRC) fxmessage("WARNING: Data read was corrupted!\n");
					delete t;
				}
				else
				{
					QIODevice *dev=devi->rdev;
					dev->open(IO_WriteOnly);
					fxmessage("Reading test file and writing into a %s ...\n", devi->name.text());
					FXuint time=FXProcess::getMsCount();
					s >> *dev;
					dev->close();
					taken=(FXProcess::getMsCount()-time)/1000.0;
				}
				if(devi->wdev!=devi->rdev) FXDELETE(devi->wdev);
				FXDELETE(devi->rdev);
				fxmessage("That took %f seconds, average speed=%dKb/sec\n", taken, (FXuint)(((FXlong)largefile.at()/1024)/taken));
			}
		}

		if(1)
		{
			fxmessage("\nStippled i/o test:\n"
						"-=-=-=-=-=-=-=-=-=\n");
			QMemMap src("../ReadMe.txt"), dest("BigFile.txt");
			src.open(IO_ReadOnly);
			dest.open(IO_WriteOnly);
			dest.truncate(src.size());
			FXuval ps=FXProcess::pageSize();
			FXuval n, blocks=(FXuval) src.size()/ps;
			for(n=0; n<blocks; n++)
			{
				src.mapIn(n*ps, (ps/2)*rand()/RAND_MAX);
				dest.mapIn(n*ps, (ps/2)*rand()/RAND_MAX);
			}
			FXuval r=0;
			FXfval oldpos;
			char buffer[256];
			do
			{
				r=(FXuval)(16ULL*rand()/RAND_MAX);
				oldpos=src.at();
				r=src.readBlock(buffer, r);
				dest.writeBlock(buffer, r);
				//dest.putch(src.getch());
				if(rand()>RAND_MAX/2)
				{
					src.at(oldpos); dest.at(oldpos);
				}
			} while(!src.atEnd());
			src.close(); dest.close();
			src.open(IO_ReadOnly); dest.open(IO_ReadOnly);
			if(src.size()!=dest.size()) fxmessage("Error: File sizes are not the same!\n");
			void *a=src.mapIn(), *b=dest.mapIn();
			if(memcmp(a, b, (size_t) src.size()))
				fxmessage("Error: Files are different!\n");
			else
				fxmessage("Test passed!\n");
		}

		if(0)
		{	// Test of 64 bit file handling
			fxmessage("\n64 bit file handling test:\n"
						"-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
			FXFile largefile("BigFile.txt");
			FXStream slargefile(&largefile);
			largefile.open(IO_ReadWrite);
			largefile.at(((FXuint)-11));
			fxmessage(FXString("File ptr at 0x%1 - warning, next bit may take a while as the OS extends the file by 4Gb\n").arg(largefile.at(), 0, 16).text());
			slargefile << FXString("Hello I am a pretty round teapot six centimetres tall");
			fxmessage(FXString("File ptr now at 0x%1\n").arg(largefile.at(), 0, 16).text());
			largefile.at(0x100000002LL);
			char buffer[64];
			FXfval pos=largefile.at();
			largefile.readBlock(buffer, 64);
			fxmessage("From 4Gb + 2 bytes position, read '%s'\n", buffer);
			largefile.truncate(0x100000002LL);
			largefile.close();
			fxmessage(FXString("File size is 0x%1 (should be two bytes above 4Gb)\n").arg(FXFile::size(largefile.name()), 0, 16).text());
			FXFile::remove(largefile.name());
		}
	}
	FXERRH_CATCH(FXException &e)
	{
		fxerror("Error: %s\n", e.report().text());
	}
	FXERRH_CATCH(...)
	{
		fxerror("Unknown exception\n");
	}
	FXERRH_ENDTRY
	fxmessage("All Done!\n");
#ifdef _MSC_VER
	getchar();
#endif
	return 0;
}
