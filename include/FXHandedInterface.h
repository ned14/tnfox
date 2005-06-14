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

#ifndef FXHANDEDINTERFACE_H
#define FXHANDEDINTERFACE_H

#include "FXDialogBox.h"
#include "FXPopup.h"
#include "FXPacker.h"
#include "FXPrimaryButton.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4355)	// "this" used in base member init
#endif

namespace FX {

/*! \file FXHandedInterface.h
\brief Defines a user handed dialog
*/

//! Handed dialog options
enum FXHandedInterfaceFlags
{
	HANDEDINTERFACE_BOTTOMWELL		=0,				//!< Places the primary button well on the bottom
	HANDEDINTERFACE_SIDEWELL		=0x08000000,	//!< Places the primary button well on the side
	HANDEDINTERFACE_OKBUTTON		=0x10000000,	//!< Adds an OK button
	HANDEDINTERFACE_DEFOKBUTTON		=0x10000000,	//!< Adds a default OK button
	HANDEDINTERFACE_CANCELBUTTON	=0x20000000,	//!< Adds a Cancel button
	HANDEDINTERFACE_OKCANCELBUTTONS	=0x30000000,	//!< Adds both OK and Cancel buttons
	HANDEDINTERFACE_DEFCANCELBUTTON	=0x60000000,	//!< Adds a default Cancel button
	HANDEDINTERFACE_DEFCANCEL		=0x40000000		//!< Makes the cancel button the default
	// 0x80000000 used by FXHandedMsgBox
};

/*! \class FXHandedInterfaceI
\brief A user handed dialog type interface with a primary button well

Tn's dialogs coexist as popups and traditional dialogs, so this class provides the
common functionality as a base class suitable for a dialog or popup with a primary button well aligned to the
bottom and a side. Which side depends on the current user's handedness and
so using this as a base class saves you a quantity of work. You should still
test all your applications with both handedness settings (via command-line
override). If you choose to add any extra buttons to the well, remember to
use FX::FXWindow::userHandednessLayout() in its options if it's going into
a bottom well (if side then always LAYOUT_BOTTOM too)

The layout performed by this class is not particularly intelligent - it
either goes bottom-up or handedness-sideways. The well is first allocated as
small as it can go in the direction of the placement but filling in the
perpendicular direction and all child widgets are then given an equal division
of the remaining space but all filling in the perpendicular direction. These
fills are compulsory - if you want something different, insert a packer and
get it to do the management. If anyone has call for improving this layout,
tell me and I'll fix it but I think what I've supplied is fine for almost
every possible use of this dialog.

If the remaining windows require more space than is available, the button
well slides out of view. This is to enable small screen use. If you need
this to happen earlier eg; your content is in a scroll area then post
\c FXSEL(SEL_COMMAND,ID_DOCKWELL) when required.

You can also have Ok and Cancel buttons added automatically to the primary
button well which also operate the apply/reset protocol (see FX::FXPrimaryButton).
These buttons are automatically translated into the user's language and have
suitable help texts set for them.

This class inherits FX::FXApplyResetList for you and points the messages
at that - so all you have to do is add your apply/reset capable widgets to
the list via addApplyReset(). Furthermore by default FXHandedInterface's message
handlers issue an apply after a \c SEL_COMMAND from an Ok button so for most
cases, you can just create your child widgets and everything else is taken
care of for you.

\note Under Tn, you are NOT permitted to create a modal dialog so execute()
isn't present. If you must use modal dialogs look at the source of
FX::FXExceptionDialog for how it's done.
*/
struct FXHandedInterfaceIPrivate;
class FXAPI FXHandedInterfaceI
{
	FXHandedInterfaceIPrivate *p;
	FXHandedInterfaceI(const FXHandedInterfaceI &);
	FXHandedInterfaceI &operator=(const FXHandedInterfaceI &);
protected:
	FXHandedInterfaceI() : p(0) {}
	enum
	{
		ID_CANCEL=FXShell::ID_LAST,		//!< Send to cancel the interface
		ID_ACCEPT,						//!< Send to accept the interface
		ID_BUTTONWELL,
		ID_DOCKWELL,					//!< Send to dock the button well
		ID_UNDOCKWELL,					//!< Send to undock the button well
		ID_CHOICE,
		ID_LAST=ID_CHOICE+1000
	};
	long onCmdChoice(FXObject*,FXSelector,void*);
	long onButtonWell(FXObject*,FXSelector,void*);
	long onCmdDockWell(FXObject*,FXSelector,void*);
	long onCmdUndockWell(FXObject*,FXSelector,void*);
	long onConfigure(FXObject*,FXSelector,void*);

	FXHandedInterfaceI(FXApplyResetList *arl, FXShell *window, FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD,FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING);
	FXHandedInterfaceI(FXApplyResetList *arl, FXShell *window, FXHandedInterfaceI &o);
	~FXHandedInterfaceI();
public:
	//! Returns the button well
	FXPacker *buttonWell() const throw();
	//! Returns the Ok button
	FXPrimaryButton *okButton() const throw();
	//! Returns the Cancel button
	FXPrimaryButton *cancelButton() const throw();
	//! Returns true if the well is docked
	bool isButtonWellDocked() const throw();
protected:
	FXint getDefaultWidth();
	FXint getDefaultHeight();
	void layout();
};

class FXHandedPopup;
/*! \class FXHandedDialog
\brief A user handed dialog

See FX::FXHandedInterfaceI
*/
class FXAPI FXHandedDialog : public FXTopWindow, public FXApplyResetList, public FXHandedInterfaceI
{
	FXDECLARE(FXHandedDialog)
	FXHandedDialog(const FXHandedDialog &);
	FXHandedDialog &operator=(const FXHandedDialog &);
	static FXuint int_filter(FXuint opts, FXuint newopts)
	{
		opts&=~0x000f0000;	// Remove popup bits
		return opts|newopts;
	}
protected:
	FXHandedDialog() { }
public:
	enum
	{
		ID_CANCEL=FXShell::ID_LAST,		//!< Send to cancel the interface
		ID_ACCEPT,						//!< Send to accept the interface
		ID_BUTTONWELL,
		ID_DOCKWELL,					//!< Send to dock the button well
		ID_UNDOCKWELL,					//!< Send to undock the button well
		ID_CHOICE,
		ID_LAST=ID_CHOICE+1000
	};
	long onCmdChoice(FXObject*a,FXSelector b,void*c) { return FXHandedInterfaceI::onCmdChoice(a,b,c); }
	long onButtonWell(FXObject*a,FXSelector b,void*c) { return FXHandedInterfaceI::onButtonWell(a,b,c); }
	long onCmdDockWell(FXObject*a,FXSelector b,void*c) { return FXHandedInterfaceI::onCmdDockWell(a,b,c); }
	long onCmdUndockWell(FXObject*a,FXSelector b,void*c) { return FXHandedInterfaceI::onCmdUndockWell(a,b,c); }
	long onConfigure(FXObject*a,FXSelector b,void*c) { return FXHandedInterfaceI::onConfigure(a,b,c); }
	long onCmdAccept(FXObject*,FXSelector,void*);
	long onCmdCancel(FXObject*,FXSelector,void*);
	long onKeyPress(FXObject*,FXSelector,void*);
	long onKeyRelease(FXObject*,FXSelector,void*);
	//! Constructs a handed dialog with optional primary buttons
	FXHandedDialog(FXApp *a, const FXString &name, FXuint opts=HANDEDINTERFACE_BOTTOMWELL|DECOR_ALL,FXint x=0,FXint y=0,FXint w=0,FXint h=0, FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD, FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING)
		: FXTopWindow(a, name, NULL, NULL, opts, x,y,w,h, 0,0,0,0, 0,0), FXHandedInterfaceI(this, this, pl,pr,pt,pb, hs,vs) { }
	//! \overload
	FXHandedDialog(FXWindow *owner, const FXString &name, FXuint opts=HANDEDINTERFACE_BOTTOMWELL|DECOR_ALL,FXint x=0,FXint y=0,FXint w=0,FXint h=0, FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD, FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING)
		: FXTopWindow(owner, name, NULL, NULL, opts, x,y,w,h, 0,0,0,0, 0,0), FXHandedInterfaceI(this, this, pl,pr,pt,pb, hs,vs) { }

	//! Converts (destructively) from a handed popup to a handed dialog
	explicit inline FXHandedDialog(FXHandedPopup &o, const FXString &name, FXuint newopts=DECOR_ALL);

	virtual FXint getDefaultWidth() { return FXHandedInterfaceI::getDefaultWidth(); }
	virtual FXint getDefaultHeight() { return FXHandedInterfaceI::getDefaultHeight(); }
	virtual void layout() { FXHandedInterfaceI::layout(); flags&=~FLAG_DIRTY; }
};
/*! \class FXHandedPopup
\brief A user handed popup

See FX::FXHandedInterfaceI
*/
class FXAPI FXHandedPopup : public FXPopup, public FXApplyResetList, public FXHandedInterfaceI
{
	FXDECLARE(FXHandedPopup)
	friend class FXHandedDialog;
	FXHandedPopup(const FXHandedPopup &);
	FXHandedPopup &operator=(const FXHandedPopup &);
protected:
	FXHandedPopup() { }
public:
	enum
	{
		ID_CANCEL=FXShell::ID_LAST,		//!< Send to cancel the interface
		ID_ACCEPT,						//!< Send to accept the interface
		ID_BUTTONWELL,
		ID_DOCKWELL,					//!< Send to dock the button well
		ID_UNDOCKWELL,					//!< Send to undock the button well
		ID_CHOICE,
		ID_LAST=ID_CHOICE+1000
	};
	long onCmdChoice(FXObject*a,FXSelector b,void*c) { return FXHandedInterfaceI::onCmdChoice(a,b,c); }
	long onButtonWell(FXObject*a,FXSelector b,void*c) { return FXHandedInterfaceI::onButtonWell(a,b,c); }
	long onCmdDockWell(FXObject*a,FXSelector b,void*c) { return FXHandedInterfaceI::onCmdDockWell(a,b,c); }
	long onCmdUndockWell(FXObject*a,FXSelector b,void*c) { return FXHandedInterfaceI::onCmdUndockWell(a,b,c); }
	long onConfigure(FXObject*a,FXSelector b,void*c) { return FXHandedInterfaceI::onConfigure(a,b,c); }
	long onCmdAccept(FXObject*,FXSelector,void*);
	long onCmdCancel(FXObject*,FXSelector,void*);
	long onKeyPress(FXObject*,FXSelector,void*);
	long onKeyRelease(FXObject*,FXSelector,void*);
	//! Constructs a handed popup with optional primary buttons
	FXHandedPopup(FXWindow *owner, FXuint opts=HANDEDINTERFACE_BOTTOMWELL|FRAME_RAISED|FRAME_THICK,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD, FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING)
		: FXPopup(owner, opts, x,y,w,h), FXHandedInterfaceI(this, this, pl,pr,pt,pb, hs,vs) { }

	virtual FXint getDefaultWidth() { return FXHandedInterfaceI::getDefaultWidth(); }
	virtual FXint getDefaultHeight() { return FXHandedInterfaceI::getDefaultHeight(); }
	virtual void layout() { FXHandedInterfaceI::layout(); flags&=~FLAG_DIRTY; }
};

inline FXHandedDialog::FXHandedDialog(FXHandedPopup &o, const FXString &name, FXuint newopts)
	: FXApplyResetList(o), FXTopWindow(o.getOwner(), name, NULL, NULL, int_filter(o.options, newopts), o.xpos,o.ypos,o.width,o.height, 0,0,0,0, 0,0), FXHandedInterfaceI(this, this, o)
{
	o.clearApplyReset();
}

} // namespace

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
