/********************************************************************************
*                                                                               *
*                              Test of i/o classes                              *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003-2008 by Niall Douglas.   All Rights Reserved.       *
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
	bool isSynch, amSocket, isUnreliable;
	FXString name;
	QIODevice *rdev, *wdev;
	FXuval maxchunk;
	DevInfo(const FXString &n, QIODevice *_rdev, QIODevice *_wdev=0) : isSynch(_wdev!=0), amSocket(false), isUnreliable(false), name(n), rdev(_rdev), wdev((_wdev) ? _wdev : _rdev), maxchunk(65536) { }
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
			byteCount-=(int) read;
			//fxmessage("r%u, togo=%u\n", (FXuint) read, (FXuint) byteCount);
		}
	}
	virtual void *cleanup()
	{
		return 0;
	}
};

int main(int argc, char *argv[])
{
	int ret=0;
	FXProcess myprocess(argc, argv);
	fxmessage("These tests will leave large files on your hard drive\n"
			  "You will probably want to delete these afterwards\n");
	FXERRH_TRY
	{
		if(1)
		{	// Basic i/o test
			fxmessage("\nGeneral purpose i/o test:\n"
						"-=-=-=-=-=-=-=-=-=-=-=-=-\n");
			QFile fh("../../ReadMe.txt");
			QBuffer bufferh;
			fh.open(IO_ReadOnly|IO_Translate);
			bufferh.open(IO_ReadWrite);
			FXStream sbufferh(&bufferh);
			char buffer[32];
			memset(buffer, 0, 32);
			int count=0, read;
			for(;;)
			{	// Obviously there are much more efficient ways than this
				if(!(read=(int) fh.readBlock(buffer, 5))) break;
				sbufferh << buffer[0]; count++;
				fh.at(fh.at()-read+1);
			}
			fxmessage("Size of ReadMe.txt is %d (reported by filing system: %ld)\n", count, fh.size());
			fh.close();
			int i;
			for(i=5; i<32 && !buffer[i]; i++);
			if(32!=i) fxerror("Buffer overwrite occurred (failure)\n");

			fxmessage("\nEndian conversion & mixed read-writes test:\n"
					  "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
			QBuffer buffer2h;
			buffer2h.open(IO_ReadWrite);
			FXStream sbuffer2h(&buffer2h);
			sbuffer2h.swapBytes(true);
			bufferh.truncate(bufferh.size() & ~7);
			for(bufferh.at(0); !sbufferh.atEnd(); )
			{
				union { FXulong c3; FXuchar p[8]; };
				FXulong c, c2;
				sbufferh >> c;
				c3=c2=c;
				fxendianswap(c2);
				{
					FXuchar t;
					t=p[0]; p[0]=p[7]; p[7]=t;
					t=p[1]; p[1]=p[6]; p[6]=t;
					t=p[2]; p[2]=p[5]; p[5]=t;
					t=p[3]; p[3]=p[4]; p[4]=t;
				}
				if(c2!=c3)
					fxerror("fxendianswap8() isn't working!\n");
				sbuffer2h << c;
				sbuffer2h.rewind(8);
				sbuffer2h.swapBytes(false);
				sbuffer2h >> c;
				sbuffer2h.swapBytes(true);
				sbuffer2h << c;
			}
			if(buffer2h.size()!=bufferh.size()*2)
				fxerror("Endian conversion and mixed read-writes test failed\n");

			QBuffer temp;
			temp.open(IO_ReadWrite);
			FXStream stemp(&temp);
			bufferh.at(0);
			buffer2h.at(0);
			stemp.writeRawBytes("The original ReadMe.txt:\n", 22) << bufferh;
			stemp.writeRawBytes("\nNow the mangled copy:\n", 23) << buffer2h;
			for(int n=0; n<100000; n++)
			{
				FXString t("I am a happy cow %1\n"); t.arg(n);
				stemp.writeRawBytes(t.text(), t.length());
				if(n % 10000==0)
					fxmessage("Generating test text (%d) ...\n", n);
			}
			stemp.writeRawBytes("\nAnd the end is right NOW", 25);
			fxmessage("Uncompressed data is %u bytes long\n", (FXuint) temp.size());

			fxmessage("\nGZip device test:\n"
					    "-=-=-=-=-=-=-=-=-\n");
			QFile zipped("../IOTestOutput.gz");
			QGZipDevice gzipdev(&zipped);
			gzipdev.open(IO_WriteOnly|IO_Translate);
			FXStream sgzip(&gzipdev);
			temp.at(0);
			sgzip << temp;
			gzipdev.close();
			fxmessage("Data gzipped is %u bytes long\n", (FXuint) zipped.size());
			zipped.close();
			fxmessage("Test file saved out as '%s' - try testing it with your favourite decompression utility\n", zipped.name().text());
			gzipdev.open(IO_ReadOnly|IO_Translate);
			fxmessage("Loaded in and decompressed file (it's %u bytes)\n", (FXuint) gzipdev.size());
			if(gzipdev.size()!=temp.size())
				fxwarning("FAILED, original size is not same as decompressed size!\n");
			{
				char buffer[16384];
				FXuval idx=0, read;
				do
				{
					if(memcmp(buffer, temp.buffer().data()+idx, read=gzipdev.readBlock(buffer, sizeof(buffer))))
						fxerror("FAILED, original data is not same as decompressed data at %u!\n", (FXuint) idx);
					idx+=read;
				} while(read);
			}

			fxmessage("\nBZip2 device test:\n"
					    "-=-=-=-=-=-=-=-=-\n");
			QFile bzipped("../IOTestOutput.bz2");
			QBZip2Device bzip2dev(&bzipped);
			bzip2dev.open(IO_WriteOnly|IO_Translate);
			FXStream sbzip(&bzip2dev);
			temp.at(0);
			sbzip << temp;
			bzip2dev.close();
			fxmessage("Data bzip2ed is %u bytes long\n", (FXuint) bzipped.size());
			bzipped.close();
			fxmessage("Test file saved out as '%s' - try testing it with your favourite decompression utility\n", bzipped.name().text());
			bzip2dev.open(IO_ReadOnly|IO_Translate);
			fxmessage("Loaded in and decompressed file (it's %u bytes)\n", (FXuint) bzip2dev.size());
			if(bzip2dev.size()!=temp.size())
				fxwarning("FAILED, original size is not same as decompressed size!\n");
			{
				char buffer[1024];
				FXuval idx=0, read;
				do
				{
					char *orig=(char *) temp.buffer().data()+idx;
					read=bzip2dev.readBlock(buffer, sizeof(buffer));
					for(FXuval n=0; n<read; n++)
					{
						if(orig[n]!=buffer[n])
							fxerror("FAILED, original data is not same as decompressed data at %u!\n", (FXuint)(idx+n));
					}
					idx+=read;
				} while(read);
			}
		}

		if(1)
		{
			const FXuint testsize=128*1024*1024;
			fxmessage("\nPerformance test:\n"
					    "-=-=-=-=-=-=-=-=-\n");
			QPtrList<DevInfo> devs(true);
			if(1) {
				devs.append(new DevInfo("QFile", new QFile("../BigFile3.txt")));
			}
			if(1) {
				QMemMap *dev=new QMemMap("../BigFile2.txt");
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
				DevInfo *di=new DevInfo("QBlkSocket(Stream)", server, w);
				di->amSocket=true;
				devs.append(di);
			}
			if(0) {
				QBlkSocket *server=new QBlkSocket(QBlkSocket::Datagram),*w;
				server->create(IO_ReadOnly);
				w=new QBlkSocket(QHOSTADDRESS_LOCALHOST, server->port(), QBlkSocket::Datagram);
				w->open(IO_WriteOnly);
				DevInfo *di=new DevInfo("QBlkSocket(Datagram)", server, w);
				di->isUnreliable=true;
				di->maxchunk=w->maxDatagramSize();
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
					QByteArray temp(devi->maxchunk);
					t->start(true);
					// Make definitely sure reader is ready
					QThread::msleep(100);
					fxmessage("Writing lots of data to a %s ...\n", devi->name.text());
					FXuint time=FXProcess::getMsCount();
					for(FXuval n=0; n<testsize; n+=temp.size())
					{
						dev->writeBlock(temp.data(), largefile.readBlock((char *) temp.data(), temp.size()));
						//fxmessage("w%u\n", (FXuint) temp.size());
						if(devi->isUnreliable)
							QThread::msleep(1);
					}
					t->byteCount.wait();
					taken=(FXProcess::getMsCount()-time)/1000.0;
					t->requestTermination();
					t->wait();
					if(t->crc!=origCRC) fxerror("WARNING: Data read was corrupted!\n");
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
			QMemMap src("../../ReadMe.txt"), dest("../BigFile.txt");
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
			if(src.size()!=dest.size()) fxerror("Error: File sizes are not the same!\n");
			void *a=src.mapIn(), *b=dest.mapIn();
			if(memcmp(a, b, (size_t) src.size()))
				fxerror("Error: Files are different!\n");
			else
				fxmessage("Test passed!\n");
		}

		if(0)
		{	// Test of 64 bit file handling
			fxmessage("\n64 bit file handling test:\n"
						"-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
			QFile largefile("../BigFile.txt");
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
			fxmessage(FXString("File size is 0x%1 (should be two bytes above 4Gb)\n").arg(FXStat::size(largefile.name()), 0, 16).text());
			FXFile::remove(largefile.name());
		}

		if(1)
		{
			fxmessage("\nUTF translation test:\n"
						"-=-=-=-=-=-=-=-=-=-=-\n");
			QFile utffile("../../UTF-16BE-demo.txt"), outfile("../BigFile.txt");
			char buffer[32768], buffer2[32768];
			FXuval read, written, totalread=0;
			utffile.open(IO_ReadOnly|IO_Translate);
			outfile.open(IO_ReadWrite|IO_Truncate|IO_Translate);
			do
			{
				//memset(buffer, 0xff, sizeof(buffer));
				totalread+=(read=utffile.readBlock(buffer, 64));
				written=outfile.writeBlock(buffer, read);
				//fxmessage("Read %u bytes, written %u bytes, diff=%d\n", (FXuint) read, (FXuint) written, read!=written);
			} while(read);
			utffile.at(0);
			outfile.at(0);
			read=utffile.readBlock(buffer, sizeof(buffer));
			fxmessage("Read %u bytes from original\n", (FXuint) read);
			written=outfile.readBlock(buffer2, sizeof(buffer2));
			fxmessage("Read %u bytes from copy\n", (FXuint) written);
			if(totalread!=read)
				fxerror("FAILED: Segmented reads should yield identical length as block reads (%u vs %u)!\n", (FXuint) totalread, (FXuint) read);
			if(read!=written)
				fxerror("FAILED: Source is not same file size as copy!\n");
			else if(memcmp(buffer, buffer2, read))
				fxerror("FAILED: Source is not same as copy!\n");
			else
			{	// Stipple it
				FXuval r;
				FXfval oldpos1, oldpos2;
				fxmessage("Trying stipple i/o ...\n");
				utffile.at(0);
				outfile.at(0);
				do
				{
					r=(FXuval)(16ULL*rand()/RAND_MAX);
					oldpos1=utffile.at(); oldpos2=outfile.at();
					read=utffile.readBlock(buffer, r);
					written=outfile.writeBlock(buffer, read);
					//fxmessage("Read %u bytes of %u, written %u bytes, diff=%d\n", (FXuint) read, (FXuint) r, (FXuint) written, read!=written);
					if(rand()>RAND_MAX/2)
					{
						//fxmessage("Rewinding to %u, %u\n", (FXuint) oldpos1, (FXuint) oldpos2);
						utffile.at(oldpos1); outfile.at(oldpos2);
					}
				} while(oldpos1<totalread);
				utffile.at(0);
				outfile.at(0);
				read=utffile.readBlock(buffer, sizeof(buffer));
				written=outfile.readBlock(buffer2, sizeof(buffer2));
				if(totalread!=written)
					fxerror("FAILED: Segmented writes should yield identical length as block reads (%u vs %u)!\n", (FXuint) totalread, (FXuint) written);
				if(read!=written)
					fxerror("FAILED: Source is not same file size as copy!\n");
				else if(memcmp(buffer, buffer2, read))
					fxerror("FAILED: Source is not same as copy!\n");
				else
					fxmessage("Test passed!\n");
			}
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
	if(!myprocess.isAutomatedTest())
		getchar();
#endif
	return ret;
}
