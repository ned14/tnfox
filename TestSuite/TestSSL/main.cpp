/********************************************************************************
*                                                                               *
*                              Test of secure i/o                               *
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
	FXIODevice *rdev, *wdev;
	FXSSLDevice rssl, wssl;
	DevInfo(const FXString &n, FXIODevice *_rdev, FXIODevice *_wdev=0)
		: isSynch(_wdev!=0), amSocket(false), name(n), rdev(_rdev), wdev((_wdev) ? _wdev : _rdev)
	{
		rssl.setEncryptedDev(rdev);
		wssl.setEncryptedDev(wdev);
	}
};

class IOThread : public FXThread
{
public:
	DevInfo *reader;
	FXuint crc;
	FXZeroedWait byteCount;
	IOThread(DevInfo *r, int bc) : reader(r), crc(0), byteCount(bc), FXThread() { }
	virtual void run()
	{
		char buffer[65536];
		if(reader->amSocket)
		{
			FXBlkSocket *rdev=(FXBlkSocket *) reader->rdev;
			rdev=rdev->waitForConnection();
			delete reader->rdev;
			reader->rdev=rdev;
			reader->rssl.setEncryptedDev(reader->rdev);
		}
		FXIODeviceS *rdev=&reader->rssl;
		rdev->create(IO_ReadWrite);
		for(;;)
		{
			FXuval read=rdev->readBlock(buffer, sizeof(buffer));
			crc=fxadler32(crc, (FXuchar *) buffer, read);
			byteCount-=read;
		}
	}
	virtual void *cleanup()
	{
		reader->rssl.close();
		return 0;
	}
};

static void dumpSSLStats(FXSSLDevice &d)
{
	FXString msg("SSL used %1 at %2 bits (%3)\n");
	msg.arg(d.cipherName()).arg(d.cipherBits()).arg(d.cipherDescription());
	fxmessage("%s", msg.text());
}

int main(int argc, char *argv[])
{
	FXProcess myprocess(argc, argv);
	fxmessage("Encrypted device i/o test\n"
			  "-=-=-=-=-=-=-=-=-=-=-=-=-\n");
	FXERRH_TRY
	{
		if(1)
		{
			const FXuint testsize=16*1024*1024;
			fxmessage("\nPerformance test:\n"
					    "-=-=-=-=-=-=-=-=-\n");
			FXSSLKey key(128, FXSSLKey::AES);
			//key.generateFromText("Niall");
			FXSSLPKey pkey(1024, FXSSLPKey::RSA);
			key.setAsymmetricKey(&pkey);
			QPtrList<DevInfo> devs(true);
			if(1) {
				FXFile *dev=new FXFile("BigFile3.bin");
				dev->open(IO_WriteOnly|IO_ShredTruncate);
				devs.append(new DevInfo("FXFile", dev));
			}
			if(1) {
				FXMemMap *dev=new FXMemMap("BigFile2.bin");
				dev->open(IO_WriteOnly|IO_ShredTruncate);
				dev->truncate(testsize);
				dev->mapIn();
				devs.append(new DevInfo("FXMemMap", dev));
			}
			if(1) {
				FXBuffer *buff=new FXBuffer;
				buff->open(IO_WriteOnly|IO_ShredTruncate);
				buff->truncate(testsize);
				devs.append(new DevInfo("FXBuffer", buff));
			}
			if(0) {
				FXLocalPipe *p=new FXLocalPipe;
				p->setGranularity(1024*1024);
				p->create(IO_ReadWrite);
				devs.append(new DevInfo("FXLocalPipe", new FXLocalPipe(p->clientEnd()), p));
			}
			if(1) {
				FXPipe *r=new FXPipe("TestPipe", true), *w=new FXPipe("TestPipe", true);
				r->create(IO_ReadWrite);
				devs.append(new DevInfo("FXPipe", r, w));
			}
			if(1) {
				FXBlkSocket *server=new FXBlkSocket,*w;
				server->create(IO_ReadWrite);
				w=new FXBlkSocket(FXHOSTADDRESS_LOCALHOST, server->port());
				DevInfo *di=new DevInfo("FXSocket", server, w);
				di->amSocket=true;
				devs.append(di);
			}
			FXBuffer largefile;
			FXStream s(&largefile);
			fxmessage("Writing out test file ...\n");
			largefile.open(IO_WriteOnly);
			largefile.truncate(testsize);
			FXuint origCRC=0;
			{
				FXuchar tbuffer[16384];
				int n;
				for(n=0; n<16384; n++)
					tbuffer[n]=32+(n % 0x60);
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
					FXIODeviceS *dev=&devi->wssl;
					dev->open(IO_ReadWrite);
					IOThread *t=new IOThread(devi, testsize);
					t->start(true);
					fxmessage("Writing lots of data to a %s ...\n", devi->name.text());
					FXuint time=FXProcess::getMsCount();
					s >> *dev;
					/*{	// Empty anything received
						FXuchar buffer[4096];
						if(!dev->atEnd()) dev->readBlock(buffer, 4096);
					}*/
					t->byteCount.wait();
					taken=(FXProcess::getMsCount()-time)/1000.0;
					dumpSSLStats(devi->wssl);
					t->requestTermination();
					dev->close();
					t->wait();
					if(t->crc!=origCRC) fxmessage("WARNING: Data read was corrupted!\n");
					delete t;
					fxmessage("That took %f seconds, average speed=%dKb/sec\n", taken, (FXuint)(((FXlong)largefile.at()/1024)/taken));
				}
				else
				{
					FXIODevice *dev=&devi->rssl;
					FXSSLKey ekey=key;
					if(ekey.asymmetricKey()) ekey.setAsymmetricKey(&ekey.asymmetricKey()->publicKey());
					devi->rssl.setKey(ekey);
					fxmessage("Reading test file and writing into a %s ...\n", devi->name.text());
					FXuint time=FXProcess::getMsCount();
					dev->open(IO_WriteOnly|IO_ShredTruncate);
					s >> *dev;
					dev->close();
					taken=(FXProcess::getMsCount()-time)/1000.0;
					fxmessage("Encryption took %f seconds, average speed=%dKb/sec\n", taken, (FXuint)(((FXlong)largefile.at()/1024)/taken));
					//key.generateFromText("Niall");
					//key.asymmetricKey()->generate();
					devi->rssl.setKey(key);
					FXBuffer temp;
					FXStream ts(&temp);
					temp.open(IO_ReadWrite);
					time=FXProcess::getMsCount();
					dev->open(IO_ReadOnly);
					ts << *dev;
					FXuchar *tempdata=temp.buffer().data();
					dev->close();
					taken=(FXProcess::getMsCount()-time)/1000.0;
					fxmessage("Decryption took %f seconds, average speed=%dKb/sec\n", taken, (FXuint)(((FXlong)largefile.at()/1024)/taken));
					if(largefile.size()!=temp.size() || memcmp(largefile.buffer().data(), temp.buffer().data(), (size_t) largefile.size()))
						fxmessage("WARNING: Decrypted data not same as original\n");
				}
				if(devi->wdev!=devi->rdev) FXDELETE(devi->wdev);
				FXDELETE(devi->rdev);
			}
		}
	}
	FXERRH_CATCH(FXException &e)
	{
		fxerror("Error: %s\n", e.report().text());
	}
	/*FXERRH_CATCH(...)
	{
		fxerror("Unknown exception\n");
	}*/
	FXERRH_ENDTRY
	fxmessage("All Done!\n");
#ifdef _MSC_VER
	getchar();
#endif
	return 0;
}
