/********************************************************************************
*                                                                               *
*                                  Functor Target                               *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004-2006 by Niall Douglas.   All Rights Reserved.              *
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

#if !defined(FXFUNCTORTARGET_H) && !defined(FX_DISABLEGUI)
#define FXFUNCTORTARGET_H

#include "FXObject.h"

namespace FX {

/*! \file FXFunctorTarget.h
\brief Defines classes used in invoking functors
*/

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

/*! \class FXFunctorTarget
\brief A FOX messaging target which invokes a FX::Generic::Functor

When writing generic code, it's a pain to create FX::FXObject derived classes
as these require static data meaning you must labouriously write FXIMPLEMENT()'s.
For the simple case of invoking some arbitrary code when a button is pressed
for example, instantiate one of these with the required functor and set as the
messaging target.

No filtering of the messages is performed. All messages sent to this target
are sent to the functor.

You may also wish to investigate the TnFOX enhancements to FX::FXDataTarget
including FX::FXDataTargetI.
*/
class FXAPI FXFunctorTarget : public FXObject
{	// Keep similar to FXDECLARE()
public:
	static const FX::FXMetaClass metaClass;
	static FX::FXObject* manufacture() { return new FXFunctorTarget; }
	virtual long handle(FX::FXObject* sender,FX::FXSelector sel,void* ptr);
	virtual const FX::FXMetaClass* getMetaClass() const { return &metaClass; }

	// The type of the functor
	typedef Generic::Functor<Generic::TL::create<long, FXObject *, FXSelector, void *>::value> MsgHandlerSpec;
private:
	MsgHandlerSpec handler;
public:
	enum
	{
		ID_LAST=1
	};
	FXFunctorTarget() {}
	//! Constructs a messaging target calling \em functor
	FXFunctorTarget(MsgHandlerSpec functor) : handler(std::move(functor)) { }
	//! Returns the functor called
	const MsgHandlerSpec &functor() const { return handler; }
	//! Sets the functor called
	void setFunctor(MsgHandlerSpec functor) { handler=functor; }
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

} // namespace

#endif
