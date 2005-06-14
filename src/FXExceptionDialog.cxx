/********************************************************************************
*                                                                               *
*              E x c e p t i o n  H a n d l i n g  D i a l o g                  *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2003 by Niall Douglas.   All Rights Reserved.              *
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

#include "fxdefs.h"
#include "FXApp.h"
#include "FXExceptionDialog.h"
#include "FXHandedMsgBox.h"
#include "FXException.h"
#include "FXVerticalFrame.h"
#include "FXHorizontalFrame.h"
#include "FXGIFIcon.h"
#include "FXText.h"
#include "FXPrimaryButton.h"
#include "FXProcess.h"
#include "FXTrans.h"
#include "FXIODevice.h"
#include "FXRollback.h"
#include <stdlib.h>
#include <string.h>
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

// Map
FXDEFMAP(FXExceptionDialog) FXExceptionDialogMap[]={
	FXMAPFUNC(SEL_COMMAND, FXExceptionDialog::ID_CANCEL, FXExceptionDialog::onCmdClicked),
	FXMAPFUNC(SEL_COMMAND, FXExceptionDialog::ID_RETRY,  FXExceptionDialog::onCmdClicked),
	FXMAPFUNC(SEL_COMMAND, FXExceptionDialog::ID_QUIT,   FXExceptionDialog::onCmdClicked),
	FXMAPFUNC(SEL_COMMAND, FXExceptionDialog::ID_FULLDETAIL, FXExceptionDialog::onCmdFullDetail)
};

// Object implementation
FXIMPLEMENT(FXExceptionDialog,FXHandedDialog,FXExceptionDialogMap,ARRAYNUMBER(FXExceptionDialogMap))


class FXExceptionDetails : public FXHandedDialog
{
	FXDECLARE(FXExceptionDetails)
	FXExceptionDialog *myowner;
	FXExceptionDetails(const FXExceptionDetails &);
	FXExceptionDetails &operator=(const FXExceptionDetails &);
protected:
	FXExceptionDetails() {}
public:
	enum
	{
		ID_COPYTOCLIPBOARD=FXHandedDialog::ID_LAST,
		ID_LAST
	};
	FXExceptionDetails(FXExceptionDialog *owner,  FXuint opts=DECOR_ALL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD,FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING)
		: myowner(owner), FXHandedDialog(owner, FXTrans::tr("FXExceptionDetails", "Full error report"), opts|HANDEDINTERFACE_OKBUTTON, x,y,w,h, pl,pr,pt,pb, hs,vs)
	{
		FXText *text;
		FXERRHM(text=   new FXText(this, NULL, 0, TEXT_READONLY|TEXT_WORDWRAP|LAYOUT_FILL));
		text->setText(myowner->e->report());

		FXERRHM(new FXButton(buttonWell(), tr("&Copy to clipboard"), NULL, this, ID_COPYTOCLIPBOARD, BUTTON_NORMAL|FXWindow::userHandednessLayout()|LAYOUT_CENTER_Y));
		resize(420,256);
	}
	long onClipboardRequest(FXObject *sender, FXSelector sel, void *ptr)
	{
		const FXString &txt=myowner->e->report();
		FXuchar *buff;
		FXMALLOC(&buff, FXuchar, (txt.length()+1)*2);
		bool midNL;
		FXuval inputlen=txt.length();
		FXuval actuallen=FXIODevice::applyCRLF(midNL, buff, (FXuchar *) txt.text(), (txt.length()+1)*2, inputlen);
		setDNDData(FROM_CLIPBOARD, stringType, (FXuchar *) buff, actuallen+1);
		return 1;
	}
	long onCmdCopy(FXObject *sender, FXSelector sel, void *ptr)
	{
		if(!acquireClipboard(&stringType, 1)) getApp()->beep();
		return 1;
	}
};
FXDEFMAP(FXExceptionDetails) FXExceptionDetailsMap[]={
	FXMAPFUNC(SEL_CLIPBOARD_REQUEST, 0, FXExceptionDetails::onClipboardRequest),
	FXMAPFUNC(SEL_COMMAND, FXExceptionDetails::ID_COPYTOCLIPBOARD, FXExceptionDetails::onCmdCopy)
};
FXIMPLEMENT(FXExceptionDetails,FXHandedDialog,FXExceptionDetailsMap,ARRAYNUMBER(FXExceptionDetailsMap));


long FXExceptionDialog::onCmdClicked(FXObject *sender, FXSelector sel, void *ptr)
{
	getApp()->stopModal(this, FXSELID(sel));
	hide();
	return 1;
}

long FXExceptionDialog::onCmdFullDetail(FXObject *sender, FXSelector sel, void *ptr)
{
	if(!details)
	{
		FXERRHM(details=new FXExceptionDetails(this));
		details->create();
	}
	details->show(PLACEMENT_CURSOR);
	return 1;
}

FXExceptionDialog::FXExceptionDialog(FXApp *a, FXException &_e,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs)
	: e(&_e), details(0),
	FXHandedDialog(a, (_e.isFatal() ? FXTrans::tr("FXExceptionDialog", "Fatal Error from %1") : FXTrans::tr("FXExceptionDialog", "Error from %1")).arg(a->getAppName()),
	opts|HANDEDINTERFACE_DEFCANCELBUTTON,x,y,w,h,pl,pr,pt,pb,hs,vs)
{
	init();
}

FXExceptionDialog::FXExceptionDialog(FXWindow *owner, FXException &_e,FXuint opts,FXint x,FXint y,FXint w,FXint h,FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs)
	: e(&_e), details(0),
	FXHandedDialog(owner, (_e.isFatal() ? FXTrans::tr("FXExceptionDialog", "Fatal Error from %1") : FXTrans::tr("FXExceptionDialog", "Error from %1")).arg(owner->getApp()->getAppName()),
	opts|HANDEDINTERFACE_DEFCANCELBUTTON,x,y,w,h,pl,pr,pt,pb,hs,vs)
{
	init();
}

void FXExceptionDialog::init()
{
	FXRBOp unconstr=FXRBConstruct(this);
	setHelpTag(tr("This is asking you what to do about the listed error"));
	FXVerticalFrame *top;
	FXHorizontalFrame *top2;
	FXText *text;
	FXERRHM(top=    new FXVerticalFrame(this, FRAME_RIDGE|LAYOUT_FILL));

	FXERRHM(top2=new FXHorizontalFrame(top, LAYOUT_FILL, 0,0,0,0, 0,0,0,0, 0,0));
	FXERRHM(new FXLabel(top2, FXString(), const_cast<FXIcon *>(e->isFatal() ? FXHandedMsgBox::fatalErrorIcon() : FXHandedMsgBox::errorIcon())));

	FXERRHM(text=new FXText(top2, NULL, 0, TEXT_READONLY|TEXT_WORDWRAP|LAYOUT_FILL));
	text->setBackColor(getBackColor());
	text->setText(e->message());
	text->setHelpTag(tr("This describes the error which has occurred"));

	FXERRHM(new FXButton(top, tr("&Full detail"), NULL, this, ID_FULLDETAIL, BUTTON_NORMAL|LAYOUT_BOTTOM|FXWindow::userHandednessLayout()));

	FXPrimaryButton *pb=cancelButton(), *rb=0, *qb=0;
	if((e->flags() & FXERRH_ISFATAL)) pb->disable();
	pb->setHelpTag(tr("Press to cancel the operation and return to the program"));

	FXERRHM(rb=new FXPrimaryButton(buttonWell(), tr("&Retry"), NULL, this, ID_RETRY, PBUTTON_OK));
	if((e->flags() & (FXERRH_ISNORETRY|FXERRH_ISFATAL))) rb->disable();
	rb->setHelpTag(tr("Press to attempt to retry the operation"));

	FXERRHM(qb=new FXPrimaryButton(buttonWell(), tr("&Quit"), NULL, this, ID_QUIT, PBUTTON_NORMAL));
	qb->setHelpTag(tr("Press to immediately quit the program"));

	if(pb->isEnabled()) pb->setFocus(); else qb->setFocus();
	resize(360, 240);
	unconstr.dismiss();
}

FXExceptionDialog::~FXExceptionDialog()
{ FXEXCEPTIONDESTRUCT1 {
	destroy();
} FXEXCEPTIONDESTRUCT2; }

void FXExceptionDialog::create()
{
	const_cast<FXIcon *>(e->isFatal() ? FXHandedMsgBox::fatalErrorIcon() : FXHandedMsgBox::errorIcon())->create();
	FXHandedDialog::create();
	getApp()->errorBeep();
}

FXuint FXExceptionDialog::execute(FXuint placement)
{
	create();
	show(placement);
	switch(getApp()->runModalFor(this))
	{
	case ID_CANCEL:
		return 0;
	case ID_RETRY:
		return 1;
	case ID_QUIT:
		{
			getApp()->exit(1);
			FXProcess::exit(1);
		}
	}
	return 0;
}

} // namespace
