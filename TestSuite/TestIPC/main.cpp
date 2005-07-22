/********************************************************************************
*                                                                               *
*                                   IPC test                                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003 by Niall Douglas.   All Rights Reserved.                   *
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
#include "../Tn/TFileBySyncDev.cxx"

class TestChannel : public FXIPCChannel
{
	FXIPCMsgRegistry myregistry;
public:
	TestChannel(QIODeviceS *dev) : FXIPCChannel(myregistry, dev) { }
protected:
	HandledCode msgReceived(FXIPCMsg *rawmsg)
	{
		return NotHandled;
	}
};

int main(int argc, char *argv[])
{
	FXProcess myprocess(argc, argv);
	char devtype[8], server[8];
	bool amServer;
	int choice=0;
	do
	{
		fxmessage("YOU MUST RUN TWO INSTANCES OF ME AT THE SAME TIME!!!\nBe server (1) or client (2):\n");
		scanf("%s", server);
		amServer=('1'==server[0]);
	} while('1'!=server[0] && '2'!=server[0]);
	fxmessage(amServer ? "Server\n" : "Client\n");
	do
	{
		fxmessage("What should I use for the transport? (S: Socket, P: Pipe\n"
			"    E: Encrypted Socket):\n");
		scanf("%s", devtype);
		if('s'==devtype[0] || 'S'==devtype[0]) choice=1;
		else if('p'==devtype[0] || 'P'==devtype[0]) choice=2;
		else if('e'==devtype[0] || 'E'==devtype[0]) choice=3;
	} while(!choice);
	FXPtrHold<QIODeviceS> realtransport;
	FXPtrHold<QIODeviceS> transport;
	if(1==choice || 3==choice)
	{
		if(1==choice) fxmessage("Socket\n");
		if(amServer)
		{
			QBlkSocket server(QBlkSocket::Stream, (FXushort) 12345);
			server.create(IO_ReadWrite);
			transport=server.waitForConnection();
		}
		else
		{
			FXERRHM(transport=new QBlkSocket(QHOSTADDRESS_LOCALHOST, 12345));
			transport->open(IO_ReadWrite);
		}
	}
	if(2==choice)
	{
		fxmessage("Pipe\n");
		FXERRHM(transport=new QPipe("TestIPC", true));
		if(amServer)
			transport->create(IO_ReadWrite);
		else
			transport->open(IO_ReadWrite);
	}
	if(3==choice)
	{
		fxmessage("Encrypted socket\n");
		realtransport=transport;
		transport=0;
		FXERRHM(transport=new QSSLDevice(realtransport));
		if(amServer)
			transport->create(IO_ReadWrite);
		else
			transport->open(IO_ReadWrite);
	}

	TestChannel ch(transport);
	ch.setUnreliable(false);
	ch.setCompression(false);
	ch.setPrintStatistics(false);
	FXERRH_TRY
	{
		const int round=4096; // 4066 for roundness test
		if(amServer)
		{
			QMemMap testfile("BigFile2.txt");
			testfile.open(IO_ReadWrite);
			testfile.truncate(4*4096*round);	// 4Mb
			char *data=(char *) testfile.mapIn();
			FXuint before=FXProcess::getMsCount();
			memset(data, 'N', (size_t) testfile.size());
			testfile.flush();
			FXuint after=FXProcess::getMsCount();
			fxmessage("Wrote raw file at %fKb/sec\n", (1000.0*testfile.size()/(after-before))/1024);
			before=FXProcess::getMsCount();
			FXulong foo=0;
			FXfval testfilelen=testfile.size();
			for(FXfval n=0; n<testfilelen; n+=8)
			{
				foo+=*(FXulong *)&data[n];
			}
			after=FXProcess::getMsCount();
			fxmessage("Read raw file at %fKb/sec\n", (1000.0*testfile.size()/(after-before))/1024);
			int cnt=0;
			for(char *ptr=data; ptr<data+testfilelen-64;)
			{
				sprintf(ptr, "There are %d bottles on the wall!\r\n", cnt++);
				ptr=strchr(ptr,0);
			}
			fxmessage("Running server ...\n");
			Tn::TFileBySyncDev server(true, ch, &testfile);
			server.setPrintStatistics(false);
			ch.start(true);
			ch.wait();
		}
		else
		{
			Tn::TFileBySyncDev client(false, ch);
			client.setPrintStatistics(false);
			ch.start(true);
			Generic::BoundFunctorV *cancelclient=ch.addCleanupCall(Generic::BindObjN(client, &Tn::TFileBySyncDev::invokeConnectionLost), true);
			client.open(IO_ReadWrite);
			char buffer[65536];
			FXuval read=0, justread;
			FXuint before=FXProcess::getMsCount();
			do
			{
				read+=justread=client.readBlock(buffer, 65536);
			} while(justread);
			FXuint after=FXProcess::getMsCount();
			fxmessage("Read through TFileBySyncDev at %fKb/sec\n", (1000.0*read/(after-before))/1024);
			fxmessage("*** Moving to 4096\n");
			client.at(round);
			before=FXProcess::getMsCount();
			for(FXuint n=0; n<read/65536; n++)
			{
				//fxmessage("Writing %u of %u ...\n", n*65536, read);
				client.writeBlock(buffer, 65536);
			}
			fxmessage("*** Flushing output\n");
			client.flush();
			//fxmessage("*** Synchronising\n");
			//client.at(0);
			//client.readBlock(buffer, 1);
			after=FXProcess::getMsCount();
			fxmessage("Wrote through TFileBySyncDev at %fKb/sec\n", (1000.0*read/(after-before))/1024);
			client.close();
			ch.requestClose();
			ch.wait();
			ch.removeCleanupCall(cancelclient);
		}
	}
	FXERRH_CATCH(FXException &e)
	{
		fxmessage("\n\nException %s\n", e.report().text());
		ch.requestClose();
		ch.wait();
		return 1;
	}
	FXERRH_ENDTRY;

	printf("\n\nTests complete!\n");
#ifdef WIN32
	getchar();
#endif
	return 0;
}
