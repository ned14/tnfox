/********************************************************************************
*                                                                               *
*                                Event Loops test                               *
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
#include "FXMemDbg.h"
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

static FXuint seed;

class Thread : public TnFXAppEventLoop
{
	FXDECLARE(Thread)
public:
	FXDataTargetI<FXuint> count;
    FXMainWindow *mainwnd;
	FXTimer *timer;
	FXDial *dial;
	FXProgressBar *progress;
	FXListBox *listbox;
	FXList *list;
	Thread() : TnFXAppEventLoop("Test Thread", (TnFXApp *) TnFXApp::instance()), mainwnd(0), timer(0)
	{
		*count=0;
	}
	enum
	{
		ID_TOGGLECOUNTER=TnFXAppEventLoop::ID_LAST,
		ID_TIMERTICK,
		ID_THROWERROR,
		ID_MSGBOX,
		ID_LAST
	};
	FXint execute(FXApp *app)
	{
	    mainwnd=new FXMainWindow(app, "Foo");
		FXString txt;
		txt.format("I am event loop 0x%p running in thread %d!", app->getEventLoop(), FXThread::id());
		FXVerticalFrame *vf=new FXVerticalFrame(mainwnd, LAYOUT_FILL);
		FXLabel *label=new FXLabel(vf, txt.text());
		FXHorizontalFrame *hf1=new FXHorizontalFrame(vf, LAYOUT_FILL_X);
	    new FXButton(hf1, "Toggle timer", 0, this, ID_TOGGLECOUNTER);
	    new FXButton(hf1, "Throw error", 0, this, ID_THROWERROR);
		new FXButton(hf1, "Show message", 0, this, ID_MSGBOX);

		FXHorizontalFrame *hf2=new FXHorizontalFrame(vf, LAYOUT_FILL_X);
		dial=new FXDial(hf2, &count, FXDataTarget::ID_VALUE, LAYOUT_FILL_Y);
		dial->setRange(0, 100);
		dial->setHeight(256);
		progress=new FXProgressBar(hf2, &count, FXDataTarget::ID_VALUE, LAYOUT_FILL_X);
		progress->showNumber();

		listbox=new FXListBox(vf, &count, FXDataTarget::ID_VALUE, LAYOUT_FILL_X);
		list=new FXList(vf, &count, FXDataTarget::ID_VALUE, LAYOUT_FILL_X);
		for(int n=0; n<=100; n++)
		{
			listbox->appendItem(FXString("I am item %1").arg(n));
			list->appendItem(FXString("I am item %1").arg(n));
		}

		mainwnd->recalc();
        mainwnd->resize(320,256);
		mainwnd->move(50*(fxrandom(seed)>>29), 50*(fxrandom(seed)>>29));
        mainwnd->show();
	    app->create();
        return app->run();
	}
	long onCmdToggleCounter(FXObject *from, FXSelector sel, void *data)
	{
		if(timer)
		{
			getApp()->removeTimeout(timer);
			timer=0;
		}
		else
		{
			timer=getApp()->addTimeout(this, ID_TIMERTICK, 1);
		}
		return 1;
	}
	long onCmdTimerTick(FXObject *from, FXSelector sel, void *data)
	{
		*count=*count>=100 ? 0 : *count+1;
		timer=getApp()->addTimeout(this, ID_TIMERTICK, 10);
		return 1;
	}
	long onCmdThrowError(FXObject *from, FXSelector sel, void *data)
	{
		FXERRH_TRY
		{
			FXERRG("A test error message", 0, 0);
		}
		FXERRH_CATCH(FXException &e)
		{
			FXERRH_REPORT(mainwnd, e);
		}
		FXERRH_ENDTRY;
		return 1;
	}
	long onCmdMsgBox(FXObject *from, FXSelector sel, void *data)
	{
		FXMessageBox::question(mainwnd, MBOX_YES_NO_CANCEL, "Hello", "How are you today?");
		return 1;
	}
};
FXDEFMAP(Thread) ThreadMap[]={
	FXMAPFUNC(SEL_COMMAND,Thread::ID_TOGGLECOUNTER,Thread::onCmdToggleCounter),
	FXMAPFUNC(SEL_TIMEOUT,Thread::ID_TIMERTICK,    Thread::onCmdTimerTick),
	FXMAPFUNC(SEL_COMMAND,Thread::ID_THROWERROR,   Thread::onCmdThrowError),
	FXMAPFUNC(SEL_COMMAND,Thread::ID_MSGBOX,       Thread::onCmdMsgBox),
};

FXIMPLEMENT(Thread,TnFXAppEventLoop,ThreadMap,ARRAYNUMBER(ThreadMap))

int main(int argc, char *argv[])
{
	FXProcess myprocess(argc, argv);
	TnFXApp app("TestEventLoops");
	app.init(argc,argv);
	printf("FXEventLoop test\n"
		   "-=-=-=-=-=-=-=-=\n");
	const int threads=6;
	Thread ths[threads];
	//fxTraceLevel=101;
	for(int n=1; n<threads; n++)
	{
		ths[n].start();
	}
	fxmessage("Close all windows to end ...\n");
	app.run(ths[0]);
	for(int n=0; n<threads; n++)
	{
		ths[n].wait();
	}
	printf("\n\nTests complete!\n");
#ifdef WIN32
	getchar();
#endif
	return 0;
}

