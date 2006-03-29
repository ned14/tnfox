/********************************************************************************
*                                                                               *
*              E x c e p t i o n  H a n d l i n g  D i a l o g                  *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002-2006 by Niall Douglas.   All Rights Reserved.              *
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

#if !defined(FXEXCEPTIONBOX_H) && !defined(FX_DISABLEGUI)
#define FXEXCEPTIONBOX_H

#include "FXHandedInterface.h"

namespace FX {

/*! \file FXExceptionDialog.h
\brief Defines a dialog which can report an FXException to the user
*/

/*! \class FXExceptionDialog
\brief A dialog which reports an FXException to the user

Usually you will use this via FXERRH_REPORT().
*/
class FXException;
class FXIcon;
class FXExceptionDetails;
class FXAPI FXExceptionDialog : public FXHandedDialog
{
	FXDECLARE(FXExceptionDialog)
	friend class FXExceptionDetails;
	FXException *e;
	FXExceptionDetails *details;
	FXExceptionDialog(const FXExceptionDialog &);
	FXExceptionDialog &operator=(const FXExceptionDialog &);
	FXDLLLOCAL void init();
protected:
	FXExceptionDialog() {}
public:
	long onCmdClicked(FXObject*,FXSelector,void*);
	long onCmdFullDetail(FXObject*,FXSelector,void*);
	enum
	{
		ID_RETRY=FXDialogBox::ID_LAST,
		ID_QUIT,
		ID_FULLDETAIL,
		ID_LAST
	};
	/*! Constructs a dialog box to show the specified error
	*/
	FXExceptionDialog(FXApp *a, FXException &e, FXuint opts=DECOR_ALL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD,FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING);
	//! \overload
	FXExceptionDialog(FXWindow *owner, FXException &e, FXuint opts=DECOR_ALL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD,FXint hs=DEFAULT_SPACING,FXint vs=DEFAULT_SPACING);
	~FXExceptionDialog();
	virtual void create();
	/*! \return 0 for cancel operation, 1 for retry

	Starts a modal execution of the dialog display. Internally quits the event loop if the user requested that.
	*/
	virtual FXuint execute(FXuint placement=PLACEMENT_CURSOR);

};

} // namespace

#endif
