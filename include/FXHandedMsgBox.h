/********************************************************************************
*                                                                               *
*                            User handed message box                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005-2006 by Niall Douglas.   All Rights Reserved.              *
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

#if !defined(FXHANDEDMSGBOX_H) && !defined(FX_DISABLEGUI)
#define FXHANDEDMSGBOX_H

#include "FXHandedInterface.h"

namespace FX {

/*! \file FXHandedMsgBox.h
\brief Defines a user handed message box
*/

//! Handed message box options
enum FXHandedMsgBoxFlags
{
	HANDEDMSGBOX_BOTTOMWELL		= HANDEDINTERFACE_BOTTOMWELL,		//!< Places the primary button well on the bottom
	HANDEDMSGBOX_SIDEWELL		= HANDEDINTERFACE_SIDEWELL,			//!< Places the primary button well on the side
	HANDEDMSGBOX_OKBUTTON		= HANDEDINTERFACE_OKBUTTON,			//!< Adds an OK button
	HANDEDMSGBOX_DEFOKBUTTON	= HANDEDINTERFACE_DEFOKBUTTON,		//!< Adds a default OK button
	HANDEDMSGBOX_CANCELBUTTON	= HANDEDINTERFACE_CANCELBUTTON,		//!< Adds a Cancel button
	HANDEDMSGBOX_OKCANCELBUTTONS= HANDEDINTERFACE_OKCANCELBUTTONS,	//!< Adds both OK and Cancel buttons
	HANDEDMSGBOX_DEFCANCELBUTTON= HANDEDINTERFACE_DEFCANCELBUTTON,	//!< Adds a default Cancel button
	HANDEDMSGBOX_DEFCANCEL		= HANDEDINTERFACE_DEFCANCEL,		//!< Makes the cancel button the default
	HANDEDMSGBOX_USEYESNO		= HANDEDINTERFACE_USEYESNO,			//!< Uses the text "Yes" instead of "Ok" and "No" instead of "Cancel"
	HANDEDMSGBOX_RETRYBUTTON	= 0x40000000						//!< Adds a Retry button
};

/*! \class FXHandedMsgBox
\brief A user handed message box

Probably the majority use of this class will be via its static functions fatalerror(),
error(), question() and informational() which simply instantiate and call execute().
You can supply your own icon, no icon or use fatalErrorIcon(), errorIcon(), questionIcon()
or informationalIcon().
*/
struct FXHandedMsgBoxPrivate;
class FXAPI FXHandedMsgBox : public FXHandedDialog
{
	FXDECLARE(FXHandedMsgBox)
	FXHandedMsgBoxPrivate *p;
	FXHandedMsgBox(const FXHandedMsgBox &);
	FXHandedMsgBox &operator=(const FXHandedMsgBox &);
	void init(const FXString &text, const FXIcon *icon);
protected:
	FXHandedMsgBox() : p(0) {}
public:
	long onCmdClicked(FXObject*,FXSelector,void*);
	enum
	{
		ID_RETRY=FXHandedDialog::ID_LAST,
		ID_LAST
	};
public:
	//! Constructs a message box to show the specified message
	FXHandedMsgBox(FXApp *a, const FXString &caption, const FXString &text, const FXIcon *icon=informationalIcon(), FXuint opts=DECOR_ALL|HANDEDMSGBOX_DEFOKBUTTON,FXint x=0,FXint y=0,FXint w=200,FXint h=150,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD,FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING);
	//! \overload
	FXHandedMsgBox(FXWindow *owner, const FXString &caption, const FXString &text, const FXIcon *icon=informationalIcon(), FXuint opts=DECOR_ALL|HANDEDMSGBOX_DEFOKBUTTON,FXint x=0,FXint y=0,FXint w=200,FXint h=150,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD,FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING);
	~FXHandedMsgBox();
	//! Returns the text in the message box
	FXString text() const;
	//! Sets the text in the message box
	void setText(const FXString &text);
	/*! \return 0 for cancel operation, 1 for OK, 2 for retry

	Starts a modal execution of the dialog display.
	*/
	virtual FXuint execute(FXuint placement=PLACEMENT_CURSOR);

	//! Issues a fatal error message box
	static FXuint fatalerror(FXApp *a, const FXString &caption, const FXString &text, FXuint opts=DECOR_ALL|HANDEDMSGBOX_DEFOKBUTTON,FXint x=0,FXint y=0,FXint w=200,FXint h=150,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD,FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING)
	{
		return FXHandedMsgBox(a, caption, text, fatalErrorIcon(), opts, x,y,w,h, pl,pr,pt,pb, hs,vs).execute();
	}
	//! Issues a fatal error message box
	static FXuint fatalerror(FXWindow *window, const FXString &caption, const FXString &text, FXuint opts=DECOR_ALL|HANDEDMSGBOX_DEFOKBUTTON,FXint x=0,FXint y=0,FXint w=200,FXint h=150,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD,FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING)
	{
		return FXHandedMsgBox(window, caption, text, fatalErrorIcon(), opts, x,y,w,h, pl,pr,pt,pb, hs,vs).execute();
	}
	//! Issues an error message box
	static FXuint error(FXApp *a, const FXString &caption, const FXString &text, FXuint opts=DECOR_ALL|HANDEDMSGBOX_DEFOKBUTTON,FXint x=0,FXint y=0,FXint w=200,FXint h=150,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD,FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING)
	{
		return FXHandedMsgBox(a, caption, text, errorIcon(), opts, x,y,w,h, pl,pr,pt,pb, hs,vs).execute();
	}
	//! Issues an error message box
	static FXuint error(FXWindow *window, const FXString &caption, const FXString &text, FXuint opts=DECOR_ALL|HANDEDMSGBOX_DEFOKBUTTON,FXint x=0,FXint y=0,FXint w=200,FXint h=150,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD,FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING)
	{
		return FXHandedMsgBox(window, caption, text, errorIcon(), opts, x,y,w,h, pl,pr,pt,pb, hs,vs).execute();
	}
	//! Issues a question message box
	static FXuint question(FXApp *a, const FXString &caption, const FXString &text, FXuint opts=DECOR_ALL|HANDEDMSGBOX_DEFOKBUTTON|HANDEDMSGBOX_OKCANCELBUTTONS|HANDEDMSGBOX_USEYESNO,FXint x=0,FXint y=0,FXint w=200,FXint h=150,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD,FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING)
	{
		return FXHandedMsgBox(a, caption, text, questionIcon(), opts, x,y,w,h, pl,pr,pt,pb, hs,vs).execute();
	}
	//! Issues a question message box
	static FXuint question(FXWindow *window, const FXString &caption, const FXString &text, FXuint opts=DECOR_ALL|HANDEDMSGBOX_DEFOKBUTTON|HANDEDMSGBOX_OKCANCELBUTTONS|HANDEDMSGBOX_USEYESNO,FXint x=0,FXint y=0,FXint w=200,FXint h=150,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD,FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING)
	{
		return FXHandedMsgBox(window, caption, text, questionIcon(), opts, x,y,w,h, pl,pr,pt,pb, hs,vs).execute();
	}
	//! Issues an informational message box
	static FXuint informational(FXApp *a, const FXString &caption, const FXString &text, FXuint opts=DECOR_ALL|HANDEDMSGBOX_DEFOKBUTTON,FXint x=0,FXint y=0,FXint w=200,FXint h=150,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD,FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING)
	{
		return FXHandedMsgBox(a, caption, text, informationalIcon(), opts, x,y,w,h, pl,pr,pt,pb, hs,vs).execute();
	}
	//! Issues an informational message box
	static FXuint informational(FXWindow *window, const FXString &caption, const FXString &text, FXuint opts=DECOR_ALL|HANDEDMSGBOX_DEFOKBUTTON,FXint x=0,FXint y=0,FXint w=200,FXint h=150,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD,FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING)
	{
		return FXHandedMsgBox(window, caption, text, informationalIcon(), opts, x,y,w,h, pl,pr,pt,pb, hs,vs).execute();
	}

	//! Returns a fatal error icon (mushroom cloud)
	static const FXIcon *fatalErrorIcon();
	//! Returns a normal error icon (exclamation mark)
	static const FXIcon *errorIcon();
	//! Returns a question icon (question mark)
	static const FXIcon *questionIcon();
	//! Returns an informational icon (envelope)
	static const FXIcon *informationalIcon();
};

}

#endif
