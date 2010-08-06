/********************************************************************************
*                                                                               *
*                             Primary Dialog Button                             *
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

#include "FXApp.h"
#include "FXPrimaryButton.h"
#include "FXDCWindow.h"
#include "QTrans.h"

#define POSITIVECOL FXRGB(135, 255, 135)
#define NEGATIVECOL FXRGB(255, 135, 135)
#define COMBINEDCOL FXRGB(255, 200, 81)
#define  UNKNOWNCOL FXRGB(255, 255, 160)

namespace FX {

long FXApplyResetList::onApply(FXObject *from, FXSelector sel, void *ptr)
{
	FXWindow *w;
	for(QPtrVectorIterator<FXWindow> it(mywidgets); (w=it.current()); ++it)
	{
		w->handle(from, FXSEL(SEL_APPLY, 0), ptr);
	}
	return 1;
}
long FXApplyResetList::onReset(FXObject *from, FXSelector sel, void *ptr)
{
	FXWindow *w;
	for(QPtrVectorIterator<FXWindow> it(mywidgets); (w=it.current()); ++it)
	{
		w->handle(from, FXSEL(SEL_RESET, 0), ptr);
	}
	return 1;
}

FXDEFMAP(FXPrimaryButton) FXPrimaryButtonMap[]={
	FXMAPFUNC(SEL_PAINT,0,FXPrimaryButton::onPaint),
	FXMAPFUNC(SEL_MIDDLEBUTTONPRESS,0,FXButton::onLeftBtnPress),
	FXMAPFUNC(SEL_MIDDLEBUTTONRELEASE,0,FXPrimaryButton::onMiddleBtnRelease),
};
FXIMPLEMENT(FXPrimaryButton,FXButton,FXPrimaryButtonMap,ARRAYNUMBER(FXPrimaryButtonMap))

FXPrimaryButton::FXPrimaryButton(FXComposite *parent,const FXString &text,FXIcon *icon,
	FXObject *tgt,FXSelector sel,FXuint opts,
	FXint x,FXint y,FXint w,FXint h,
	FXint pl,FXint pr,FXint pt,FXint pb,
	FXint sr)
		: FXButton(parent,text,icon,tgt,sel,
		(opts & (PBUTTON_NOAUTOLAYOUT|LAYOUT_CENTER_X)) ? opts : ((opts & ~LAYOUT_RIGHT)|FXWindow::userHandednessLayout()),
		x,y,w,h,
#ifdef BUILDING_TCOMMON
		pl+sr,pr+sr,pt+sr,pb+sr), surcol(UNKNOWNCOL), surwidth(sr)
#else
		pl,pr,pt,pb), surcol(UNKNOWNCOL), surwidth(0)
#endif
{
	if((PBUTTON_POSITIVE|PBUTTON_NEGATIVE)==(opts & (PBUTTON_POSITIVE|PBUTTON_NEGATIVE)))
	{
		surcol=COMBINEDCOL;
	}
	else if(opts & PBUTTON_POSITIVE)
	{
		surcol=POSITIVECOL;
		setHelpTag(QTrans::tr("FXPrimaryButton", "Select this to commit to the action. Alt-select merely applies the effects of committing but does not close the dialog"));
	}
	else if(opts & PBUTTON_NEGATIVE)
	{
		surcol=NEGATIVECOL;
		setHelpTag(QTrans::tr("FXPrimaryButton", "Select this to cancel the action. Alt-select merely resets the dialog to its initial state"));
	}
}

long FXPrimaryButton::onPaint(FXObject*f,FXSelector s,void*ptr)
{
#ifdef BUILDING_TCOMMON
	FXEvent *ev=(FXEvent*)ptr;

	// Start drawing
	FXDCWindow dc(this,ev);
	dc.setForeground(surcol);
	dc.fillRectangle(1,1,width-2,height-2);
	drawSunkenRectangle(dc,0,0,width,height);
	if(surwidth>2) drawSunkenRectangle(dc,1,1,width-2,height-2);
	drawButton(dc,surwidth,surwidth,width-surwidth*2,height-surwidth*2);

	return 1;
#else
	return FXButton::onPaint(f,s,ptr);
#endif
}

long FXPrimaryButton::onMiddleBtnRelease(FXObject*,FXSelector,void *ptr)
{
	bool click=(state==STATE_DOWN);
	if(isEnabled() && (flags & FLAG_PRESSED))
	{
		ungrab();
		if(target && target->handle(this, FXSEL(SEL_MIDDLEBUTTONRELEASE,message), ptr)) return 1;
		flags|=FLAG_UPDATE;
		flags&=~FLAG_PRESSED;
		if(state!=STATE_ENGAGED) setState(STATE_UP);
		if(click && target)
		{
			target->handle(this, FXSEL((options & PBUTTON_NEGATIVE)!=0 ? SEL_RESET : SEL_APPLY, message), (void *)(FXuval) 1);
		}
		return 1;
	}
	return 0;
}

FXColor FXPrimaryButton::surroundColour() const throw()
{
	return surcol;
}

void FXPrimaryButton::setSurroundColour(FXColor c) throw()
{
	surcol=c;
	update();
}

void FXPrimaryButton::save(FXStream &store) const
{
	FXButton::save(store);
	store << surcol << surwidth;
}
void FXPrimaryButton::load(FXStream &store)
{
	FXButton::load(store);
	store >> surcol >> surwidth;
}

} // namespace

