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

#ifndef FXPRIMARYBUTTON_H
#define FXPRIMARYBUTTON_H

#include "FXButton.h"
#include "FXPacker.h"
#include <qptrvector.h>

namespace FX {

/*! \file FXPrimaryButton.h
\brief Defines a primary button within a dialog
*/

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
/*! \class FXApplyResetList
\brief A list of widgets which understand the \c SEL_APPLY and \c SEL_RESET commands

This is a convenience class to make dispatching the above messages easier. It's easiest
to use by multiple inheritance and you simply supply the address of onApply() and
onReset() in your FXDEFMAP(). Adding apply/reset capable widgets is especially easy
with the templated addApplyReset() like as follows:
\code
FXERRHM(tdata=addApplyReset(new TTaggedDataBox(this)));
\endcode

\sa FX::FXPrimaryButton
*/
class FXAPI FXApplyResetList
{
	QPtrVector<FXWindow> mywidgets;
protected:
	FXApplyResetList() { }
public:
	//! Dispatches the \c SEL_APPLY command to all widgets in its list
	long onApply(FXObject*,FXSelector,void*);
	//! Dispatches the \c SEL_RESET command to all widgets in its list
	long onReset(FXObject*,FXSelector,void*);

	//! Adds a widget to the list
	template<typename type> type *addApplyReset(type *widget)
	{
		mywidgets.append(widget);
		return widget;
	}
	//! Removes a widget from the list
	void removeApplyReset(FXWindow *widget) { mywidgets.removeRef(widget); }
	//! Clears the apply/reset list
	void clearApplyReset() { mywidgets.clear(); }
};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

//! Primary button flags
enum
{
	PBUTTON_POSITIVE = 0x20000000,	//!< Pressing the button is a positive action
	PBUTTON_NEGATIVE = 0x40000000,	//!< Pressing the button is a negative action
	PBUTTON_NOAUTOLAYOUT = 0x80000000, //!< Disables auto layout to the left or right
	PBUTTON_NORMAL=BUTTON_NORMAL|LAYOUT_BOTTOM,
	//! Gives a positive action OK-style button
	PBUTTON_OK     = PBUTTON_NORMAL|PBUTTON_POSITIVE,
	//! Gives a negative action Cancel-style button
	PBUTTON_CANCEL = PBUTTON_NORMAL|PBUTTON_NEGATIVE,
	//! Gives a positive action, default initial button
	PBUTTON_DEFOK    = BUTTON_INITIAL|BUTTON_DEFAULT|PBUTTON_OK,
	//! Gives a positive action, default initial button
	PBUTTON_DEFCANCEL= BUTTON_INITIAL|BUTTON_DEFAULT|PBUTTON_CANCEL
};


/*! \class FXPrimaryButton
\brief A primary button within a dialog

Under Tn, all "take action" buttons or what Tn calls "primary buttons" are
colour coded with a variable colour border. This alternative appearence
is disabled so that it appears like a normal FX::FXButton unless BUILDING_TCOMMON
is defined, in which case all TnFOX dialogs start using the colour coding. A
"take action" button is where pressing it does some definitive thing eg; save
a file or close a dialog.

However its alternative functionality remains irrespective of how it's built.
Tn overloads the mouse buttons with primary buttons so that depending on
what mouse button you use, a slightly different action is carried out -
using select (the primary button) does the action whereas alt-select (usually
the middle button) \em applies the action but does not close the dialog.
This appears to the target as either \c SEL_APPLY (for positive action buttons)
or \c SEL_RESET (for negative action buttons). Just for information, under Tn
alt-selecting a cancel button resets the data to what it was when the dialog
was opened (hence the SEL_RESET). \c SEL_COMMAND still gets issued for normal
left-button clicks.

Default help texts are provided for \c PBUTTON_POSITIVE and \c PBUTTON_NEGATIVE
but you'll have supply your own for everything else.

You can safely choose either to use this class or use a normal FXButton in
your code if you so choose. Be aware that future improvements to TnFOX for
disabled users will make use of FXPrimaryButton's being what they are.

\note The default options specify \c LAYOUT_BOTTOM with \c LAYOUT_LEFT or
\c LAYOUT_RIGHT automatically chosen based on the current user's handedness.
Use \c PBUTTON_NOAUTOLAYOUT or \c LAYOUT_CENTER_X to override this behaviour.

<h3>Usage:</h3>
This mostly applies to Tn, but it's useful to illustrate how this button is
intended to be used. Within each dialog there will be a number of widgets which
can receive the \c SEL_APPLY and \c SEL_RESET messages - in this case they
don't take a message id. When received they apply their GUI state to the data
they represent or else restore the GUI to show the original state respectively.

Hence in the target of the FXPrimaryButton you must understand \c SEL_APPLY and
\c SEL_RESET and inform all apply/reset capable widgets. This can be made much
easier for you by using FX::FXApplyResetList.
*/
class FXAPI FXPrimaryButton : public FXButton
{
	FXDECLARE(FXPrimaryButton)
	FXColor surcol;
	FXint surwidth;
	FXPrimaryButton(const FXPrimaryButton &);
	FXPrimaryButton &operator=(const FXPrimaryButton &);
protected:
	FXPrimaryButton() {}
public:
	//! Constructs an instance
	FXPrimaryButton(FXComposite *parent,const FXString &text,FXIcon *icon=NULL,
		FXObject *tgt=NULL,FXSelector sel=0,FXuint opts=PBUTTON_NORMAL,
		FXint x=0,FXint y=0,FXint w=0,FXint h=0,
		FXint pl=DEFAULT_PAD*4,FXint pr=DEFAULT_PAD*4,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD,
#ifdef BUILDING_TCOMMON
		FXint sr=FXMAX(2, DEFAULT_SPACING));
#else
		FXint sr=0);
#endif
public:
	long onPaint(FXObject*,FXSelector,void*);
	long onMiddleBtnRelease(FXObject*,FXSelector,void*);
public:
	//! Returns the colour of the surround
	FXColor surroundColour() const throw();
	//! Sets the colour of the surround
	void setSurroundColour(FXColor c) throw();

	virtual void save(FXStream &store) const;
	virtual void load(FXStream &store);
};

} // namespace

#endif
