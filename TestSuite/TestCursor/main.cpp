/********************************************************************************
*                                                                               *
*                                  Cursors test                                 *
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
#define BUILDING_TCLIENT
#include "../Tn/THourglass.cxx"

class TestWindow : public FXMainWindow
{
	FXDECLARE(TestWindow)
protected:
	TestWindow(){}
public:
	FXGroupBox *a, *b;
	FXTimer *timer;
	enum
	{
		ID_AREA1=FXMainWindow::ID_LAST,
		ID_AREA2,
		ID_TIMER,
		ID_LAST
	};
	TestWindow(FXApp *app, const FXString &title) : FXMainWindow(app, title)
	{
		FXVerticalFrame *fr=new FXVerticalFrame(this, LAYOUT_FILL);
		a=new FXGroupBox(fr, "Area 1", LAYOUT_FILL);
		
		FXHorizontalFrame *hr=new FXHorizontalFrame(fr, LAYOUT_FILL_X);
		new FXButton(hr, "Toggle Area 1", NULL, this, ID_AREA1);
		new FXButton(hr, "Toggle Area 2", NULL, this, ID_AREA2);
		
		b=new FXGroupBox(fr, "Area 2", LAYOUT_FILL);
		resize(320,256);
		recalc();
		
		timer=app->addTimeout(this, ID_TIMER, 100);
	}
	~TestWindow()
	{
		getApp()->removeTimeout(timer);
		Tn::THourglass::smash(a);
		Tn::THourglass::smash(b);
	}
	long onCmdArea1(FXObject*,FXSelector,void*)
	{
		if(!Tn::THourglass::count(a))
			Tn::THourglass::on(a);
		else
			Tn::THourglass::off(a);
		return 1;
	}
	long onCmdArea2(FXObject*,FXSelector,void*)
	{
		if(!Tn::THourglass::count(b))
			Tn::THourglass::on(b);
		else
			Tn::THourglass::off(b);
		return 1;
	}
	long onTimer(FXObject*,FXSelector,void*)
	{
		static FXuint val;
		if(Tn::THourglass::count(a))
		{
			Tn::THourglass::setPercent(a, val);
			Tn::THourglass::setReadingLED(a, (val & 1)!=0);
			if(++val>100) val=0;
			Tn::THourglass::setWritingLED(a, (val & 1)!=0);
		}
		timer=getApp()->addTimeout(this, ID_TIMER, 1000);
		return 1;
	}
};
FXDEFMAP(TestWindow) TestWindowMap[]={
  FXMAPFUNC(SEL_COMMAND,TestWindow::ID_AREA1,TestWindow::onCmdArea1),
  FXMAPFUNC(SEL_COMMAND,TestWindow::ID_AREA2,TestWindow::onCmdArea2),
  FXMAPFUNC(SEL_TIMEOUT,TestWindow::ID_TIMER,TestWindow::onTimer),
};
FXIMPLEMENT(TestWindow,FXMainWindow,TestWindowMap,ARRAYNUMBER(TestWindowMap))

int main(int argc, char *argv[])
{
	FXProcess myprocess(argc, argv);
	FXApp app("TestCursor");
	app.init(argc,argv);
	TestWindow *mainwnd=new TestWindow(&app, "Tn Hourglass test");
    mainwnd->show();

	app.create();
	app.runModalWhileShown(mainwnd);
	app.exit(0);

	printf("\n\nTests complete!\n");
	return 0;
}
