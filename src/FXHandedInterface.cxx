/********************************************************************************
*                                                                               *
*                              User handed interface                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004 by Niall Douglas.   All Rights Reserved.                   *
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

#include "FXHandedInterface.h"
#include "fxkeys.h"
#include "FXHorizontalFrame.h"
#include "FXVerticalFrame.h"
#include "FXRollback.h"
#include <assert.h>
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

struct FXDLLLOCAL FXHandedInterfaceIPrivate
{
	FXApplyResetList *arl;
	FXShell *me;
	FXint padleft, padright, padtop, padbottom, hspacing, vspacing;
	FXPacker *buttonwell;
	FXPrimaryButton *okButton, *cancelButton;
	bool docked;
	bool docktimeractive;
	FXint dockdone, dockspeed;
	FXHandedInterfaceIPrivate(FXApplyResetList *_arl, FXShell *_me, FXint pl,FXint pr,FXint pt,FXint pb, FXint hs,FXint vs)
		: arl(_arl), me(_me), padleft(pl), padright(pr), padtop(pt), padbottom(pb),
		hspacing(hs), vspacing(vs), buttonwell(0), okButton(0), cancelButton(0),
		docked(false), docktimeractive(false), dockdone(100), dockspeed(0) { }
	~FXHandedInterfaceIPrivate()
	{
	}
};

long FXHandedInterfaceI::onCmdChoice(FXObject*,FXSelector,void*)
{	// Do nothing (stop going to FXPopup)
	return 1;
}

long FXHandedInterfaceI::onButtonWell(FXObject*,FXSelector sel,void*)
{
	if(p->docked)
	{
		switch(FXSELTYPE(sel))
		{
		case SEL_ENTER:
			{
				p->dockdone=96;
				p->buttonwell->setBackColor(p->me->getApp()->getBaseColor());
				p->me->recalc();
				break;
			}
		case SEL_LEAVE:
			{
				p->dockdone=4;
				p->me->layout();
				p->buttonwell->setBackColor(FXRGB(255,255,0));
				break;
			}
		}
	}
	return 1;
}
long FXHandedInterfaceI::onCmdDockWell(FXObject*,FXSelector sel,void*)
{
	FXShell *me=p->me;
	switch(FXSELTYPE(sel))
	{
	case SEL_COMMAND:
		{
			if(p->docked) return 1;
			p->docked=true;
			if(p->docktimeractive)
			{
				me->getEventLoop()->removeTimeout(me, ID_DOCKWELL);
				p->docktimeractive=false;
			}
			p->dockspeed=0;
		} // fall through
	case SEL_TIMEOUT:
		{
			p->docktimeractive=false;
			if(++p->dockspeed>16) p->dockspeed=16;
			p->dockdone-=p->dockspeed;
			if(p->dockdone<4)
			{
				p->dockdone=4;
				p->buttonwell->setBackColor(FXRGB(255,255,0));
			}
			else
			{
				me->getEventLoop()->addTimeout(me, ID_DOCKWELL, 25);
				p->docktimeractive=true;
			}
			me->recalc();
			return 1;
		}
	}
	return 0;
}
long FXHandedInterfaceI::onCmdUndockWell(FXObject*,FXSelector sel,void*)
{
	FXShell *me=p->me;
	switch(FXSELTYPE(sel))
	{
	case SEL_COMMAND:
		{
			if(!p->docked) return 1;
			p->docked=false;
			if(p->docktimeractive)
			{
				me->getEventLoop()->removeTimeout(me, ID_DOCKWELL);
				p->docktimeractive=false;
			}
			p->dockspeed=0;
			p->buttonwell->setBackColor(me->getApp()->getBaseColor());
		} // fall through
	case SEL_TIMEOUT:
		{
			p->docktimeractive=false;
			if(++p->dockspeed>16) p->dockspeed=16;
			p->dockdone+=p->dockspeed;
			if(p->dockdone>=100)
			{
				p->dockdone=100;
			}
			else
			{
				p->docktimeractive=true;
				me->getEventLoop()->addTimeout(me, ID_UNDOCKWELL, 25);
			}
			me->recalc();
			return 1;
		}
	}
	return 0;
}

long FXHandedInterfaceI::onConfigure(FXObject *from, FXSelector sel, void *ptr)
{
	FXShell *me=p->me;
	me->onConfigure(from, sel, ptr);
	if(!me->shown()) return 1;
	if(me->options & HANDEDINTERFACE_SIDEWELL)
	{
		FXint idealwidth=getDefaultWidth();
		if(!p->docked && me->width<idealwidth)
			me->handle(me, FXSEL(SEL_COMMAND, ID_DOCKWELL), 0);
		if(p->docked && me->width>idealwidth)
			me->handle(me, FXSEL(SEL_COMMAND, ID_UNDOCKWELL), 0);
	}
	else
	{
		FXint idealheight=getDefaultHeight();
		if(!p->docked && me->height<idealheight)
			me->handle(me, FXSEL(SEL_COMMAND, ID_DOCKWELL), 0);
		if(p->docked && me->height>idealheight)
			me->handle(me, FXSEL(SEL_COMMAND, ID_UNDOCKWELL), 0);
	}
	return 1;
}

FXHandedInterfaceI::FXHandedInterfaceI(FXApplyResetList *arl, FXShell *me, FXint pl,FXint pr,FXint pt,FXint pb,FXint hs,FXint vs)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new FXHandedInterfaceIPrivate(arl, me, pl,pr,pt,pb, hs,vs));
	if(me->options & HANDEDINTERFACE_SIDEWELL)
	{
		FXint pl=0,pr=0;
		if(FXWindow::userHandednessLayout() & LAYOUT_RIGHT) pl=DEFAULT_SPACING; else pr=DEFAULT_SPACING;
		FXERRHM(p->buttonwell=new FXVerticalFrame(me, PACK_UNIFORM_WIDTH|LAYOUT_FILL_Y, 0,0,0,0, pl,pr,0,0, DEFAULT_PAD,DEFAULT_PAD));
	}
	else
	{
		FXERRHM(p->buttonwell=new FXHorizontalFrame(me, LAYOUT_FILL_X, 0,0,0,0, 0,0,DEFAULT_SPACING,0, DEFAULT_PAD,DEFAULT_PAD));
	}
	p->buttonwell->setTarget(me);
	p->buttonwell->setSelector(ID_BUTTONWELL);
	p->buttonwell->enable();
	if(me->options & HANDEDINTERFACE_OKBUTTON)
	{
		FXERRHM(p->okButton=new FXPrimaryButton(p->buttonwell, FXTrans::tr("FXHandedInterface", "&Ok"), NULL, me, ID_ACCEPT, ((me->options & HANDEDINTERFACE_DEFCANCEL) ? PBUTTON_OK : PBUTTON_DEFOK)));
		p->okButton->setHelpTag(FXTrans::tr("FXHandedInterface", "Select to apply the changes and return to the program. Alt-select to apply without closing the dialog."));
		if(!(me->options & HANDEDINTERFACE_DEFCANCEL)) p->okButton->setFocus();
	}
	if(me->options & HANDEDINTERFACE_CANCELBUTTON)
	{
		FXERRHM(p->cancelButton=new FXPrimaryButton(p->buttonwell, FXTrans::tr("FXHandedInterface", "&Cancel"), NULL, me, ID_CANCEL, ((me->options & HANDEDINTERFACE_DEFCANCEL) ? PBUTTON_DEFCANCEL : PBUTTON_CANCEL)));
		p->cancelButton->setHelpTag(FXTrans::tr("FXHandedInterface", "Select to cancel the operation and return to the program. Alt-select to reset the contents of the dialog."));
		if(me->options & HANDEDINTERFACE_DEFCANCEL) p->cancelButton->setFocus();
	}
	unconstr.dismiss();
}

static void retarget(FXWindow *tree, FXWindow *from, FXWindow *to)
{
	for(register FXWindow *child=tree->getFirst(); child; child=child->getNext())
	{
		if(child->getTarget()==from)
			child->setTarget(to);
		if(child->getFirst()) retarget(child, from, to);
	}
}
FXHandedInterfaceI::FXHandedInterfaceI(FXApplyResetList *arl, FXShell *window, FXHandedInterfaceI &o) : p(o.p)
{
	o.p=0; p->arl=arl;
	window->create();
	while(FXWindow *child=p->me->getFirst())
	{
		child->reparent(window);
	}
	retarget(window, p->me, window);
	p->me=window;
}

FXHandedInterfaceI::~FXHandedInterfaceI()
{ FXEXCEPTIONDESTRUCT1 {
	if(p)
	{
		if(p->docktimeractive)
		{
			p->me->getEventLoop()->removeTimeout(p->me, ID_DOCKWELL);
			p->docktimeractive=false;
		}
		FXDELETE(p);
	}
} FXEXCEPTIONDESTRUCT2; }
FXPacker *FXHandedInterfaceI::buttonWell() const throw()
{
	return p->buttonwell;
}
FXPrimaryButton *FXHandedInterfaceI::okButton() const throw()
{
	return p->okButton;
}
FXPrimaryButton *FXHandedInterfaceI::cancelButton() const throw()
{
	return p->cancelButton;
}
bool FXHandedInterfaceI::isButtonWellDocked() const throw()
{
	return p->docked;
}

FXint FXHandedInterfaceI::getDefaultWidth()
{
	FXint ret=0;
	for(FXWindow *child=p->me->getFirst(); child; child=child->getNext())
	{
		FXint childw=child->getDefaultWidth()+p->hspacing;
		if(p->me->options & HANDEDINTERFACE_SIDEWELL)
			ret+=childw;
		else
		{
			if(childw>ret) ret=childw;
		}
	}
	return ret+p->padleft+p->padright;
}

FXint FXHandedInterfaceI::getDefaultHeight()
{
	FXint ret=0;
	for(FXWindow *child=p->me->getFirst(); child; child=child->getNext())
	{
		FXint childh=child->getDefaultHeight()+p->vspacing;
		if(p->me->options & HANDEDINTERFACE_SIDEWELL)
		{
			if(childh>ret) ret=childh;
		}
		else
			ret+=childh;
	}
	return ret+p->padtop+p->padbottom;
}

void FXHandedInterfaceI::layout()
{
	FXShell *me=p->me;
	FXWindow *child=me->getFirst();
	FXint children=me->numChildren(), x=p->padleft, y=p->padtop, w=me->getWidth()-p->padleft-p->padright, h=me->getHeight()-p->padtop-p->padbottom;
	assert(child==p->buttonwell);
	FXint childw=child->getDefaultWidth(), childh=child->getDefaultHeight();
	if(me->options & HANDEDINTERFACE_SIDEWELL)
	{	// First child to user handedness
		FXint wellw=(p->dockdone*childw)/100;
		if(wellw<1) wellw=1;
		if(FXWindow::userHandednessLayout() & LAYOUT_RIGHT)
		{
			child->position(p->padleft+w-wellw, y, childw, h);
		}
		else
		{
			child->position(x-childw+wellw, y, childw, h);
			x+=childw+p->hspacing;
		}
		w-=wellw;
		FXint portion=(w/(children-1))-p->hspacing;
		for(child=child->getNext(); child; x+=portion+p->hspacing, child=child->getNext())
		{
			child->position(x, y, portion, h);
		}
	}
	else
	{	// Bottom upwards
		FXint wellh=(p->dockdone*childh)/100;
		if(wellh<1) wellh=1;
		child->position(x, p->padtop+h-wellh, w, childh);
		h-=wellh;
		FXint portion=(h/(children-1))-p->vspacing;
		for(child=child->getNext(); child; y+=portion+p->vspacing, child=child->getNext())
		{
			child->position(x, y, w, portion);
		}
	}
}


FXDEFMAP(FXHandedDialog) FXHandedDialogMap[]={
	FXMAPFUNC(SEL_CONFIGURE,	0,	FXHandedDialog::onConfigure),
	FXMAPFUNC(SEL_TIMEOUT,		FXHandedDialog::ID_DOCKWELL,	FXHandedDialog::onCmdDockWell),
	FXMAPFUNC(SEL_TIMEOUT,		FXHandedDialog::ID_UNDOCKWELL,	FXHandedDialog::onCmdUndockWell),
	FXMAPFUNC(SEL_KEYPRESS,		0,	FXHandedDialog::onKeyPress),
	FXMAPFUNC(SEL_KEYRELEASE,	0,	FXHandedDialog::onKeyRelease),
	FXMAPFUNC(SEL_CLOSE,		0,	FXHandedDialog::onCmdCancel),
	FXMAPFUNC(SEL_RESET,		FXHandedDialog::ID_CANCEL,	FXHandedDialog::onReset),
	FXMAPFUNC(SEL_APPLY,		FXHandedDialog::ID_ACCEPT,	FXHandedDialog::onApply),
	FXMAPFUNC(SEL_COMMAND,		FXHandedDialog::ID_CANCEL,	FXHandedDialog::onCmdCancel),
	FXMAPFUNC(SEL_COMMAND,		FXHandedDialog::ID_ACCEPT,	FXHandedDialog::onCmdAccept),
	FXMAPFUNC(SEL_ENTER,		FXHandedDialog::ID_BUTTONWELL,FXHandedDialog::onButtonWell),
	FXMAPFUNC(SEL_LEAVE,		FXHandedDialog::ID_BUTTONWELL,FXHandedDialog::onButtonWell),
	FXMAPFUNC(SEL_COMMAND,		FXHandedDialog::ID_DOCKWELL,	FXHandedDialog::onCmdDockWell),
	FXMAPFUNC(SEL_COMMAND,		FXHandedDialog::ID_UNDOCKWELL,	FXHandedDialog::onCmdUndockWell),
	FXMAPFUNCS(SEL_COMMAND,		FXHandedDialog::ID_CHOICE,	FXHandedDialog::ID_CHOICE+999, FXHandedDialog::onCmdChoice),
};
FXIMPLEMENT(FXHandedDialog,FXTopWindow,FXHandedDialogMap,ARRAYNUMBER(FXHandedDialogMap))

long FXHandedDialog::onCmdAccept(FXObject *from, FXSelector sel, void *ptr)
{
	handle(from, FXSEL(SEL_APPLY, FXSELID(sel)), ptr);
	getApp()->stopModal(this,TRUE);
	hide();
	return 1;
}
long FXHandedDialog::onCmdCancel(FXObject*,FXSelector,void*)
{
	getApp()->stopModal(this,FALSE);
	hide();
	return 1;
}

long FXHandedDialog::onKeyPress(FXObject* sender,FXSelector sel,void* ptr)
{
	if(FXTopWindow::onKeyPress(sender,sel,ptr)) return 1;
	if(((FXEvent*)ptr)->code==KEY_Escape)
	{
		handle(this,FXSEL(SEL_COMMAND,ID_CANCEL),NULL);
		return 1;
	}
	return 0;
}
long FXHandedDialog::onKeyRelease(FXObject* sender,FXSelector sel,void* ptr)
{
	if(FXTopWindow::onKeyRelease(sender,sel,ptr)) return 1;
	if(((FXEvent*)ptr)->code==KEY_Escape)
		return 1;
	return 0;
}


FXDEFMAP(FXHandedPopup) FXHandedPopupMap[]={
	FXMAPFUNC(SEL_CONFIGURE,	0,	FXHandedPopup::onConfigure),
	FXMAPFUNC(SEL_TIMEOUT,		FXHandedPopup::ID_DOCKWELL,	FXHandedPopup::onCmdDockWell),
	FXMAPFUNC(SEL_TIMEOUT,		FXHandedPopup::ID_UNDOCKWELL,FXHandedPopup::onCmdUndockWell),
	FXMAPFUNC(SEL_KEYPRESS,		0,	FXHandedPopup::onKeyPress),
	FXMAPFUNC(SEL_KEYRELEASE,	0,	FXHandedPopup::onKeyRelease),
	FXMAPFUNC(SEL_CLOSE,		0,	FXHandedPopup::onCmdCancel),
	FXMAPFUNC(SEL_RESET,		FXHandedPopup::ID_CANCEL,	FXHandedPopup::onReset),
	FXMAPFUNC(SEL_APPLY,		FXHandedPopup::ID_ACCEPT,	FXHandedPopup::onApply),
	FXMAPFUNC(SEL_COMMAND,		FXHandedPopup::ID_CANCEL,	FXHandedPopup::onCmdCancel),
	FXMAPFUNC(SEL_COMMAND,		FXHandedPopup::ID_ACCEPT,	FXHandedPopup::onCmdAccept),
	FXMAPFUNC(SEL_ENTER,		FXHandedPopup::ID_BUTTONWELL,FXHandedPopup::onButtonWell),
	FXMAPFUNC(SEL_LEAVE,		FXHandedPopup::ID_BUTTONWELL,FXHandedPopup::onButtonWell),
	FXMAPFUNC(SEL_COMMAND,		FXHandedPopup::ID_DOCKWELL,	FXHandedPopup::onCmdDockWell),
	FXMAPFUNC(SEL_COMMAND,		FXHandedPopup::ID_UNDOCKWELL,FXHandedPopup::onCmdUndockWell),
	FXMAPFUNCS(SEL_COMMAND,		FXHandedPopup::ID_CHOICE,	FXHandedPopup::ID_CHOICE+999, FXHandedPopup::onCmdChoice),
};
FXIMPLEMENT(FXHandedPopup,FXPopup,FXHandedPopupMap,ARRAYNUMBER(FXHandedPopupMap))

long FXHandedPopup::onCmdAccept(FXObject *from, FXSelector sel, void *ptr)
{
	handle(from, FXSEL(SEL_APPLY, FXSELID(sel)), ptr);
    handle(this,FXSEL(SEL_COMMAND,ID_UNPOST),NULL);
	return 1;
}
long FXHandedPopup::onCmdCancel(FXObject*,FXSelector,void*)
{
    handle(this,FXSEL(SEL_COMMAND,ID_UNPOST),NULL);
	return 1;
}

long FXHandedPopup::onKeyPress(FXObject* sender,FXSelector sel,void* ptr)
{
	FXEvent* event=(FXEvent*)ptr;
	if(event->code==KEY_Escape || event->code==KEY_Cancel)
	{
		handle(this,FXSEL(SEL_COMMAND,ID_CANCEL),NULL);
		return 1;
	}
	return 0;
}
long FXHandedPopup::onKeyRelease(FXObject* sender,FXSelector sel,void* ptr)
{
	FXEvent* event=(FXEvent*)ptr;
	if(event->code==KEY_Escape || event->code==KEY_Cancel)
		return 1;
	return 0;
}

} // namespace
