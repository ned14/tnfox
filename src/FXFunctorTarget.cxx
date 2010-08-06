/********************************************************************************
*                                                                               *
*                                  Functor Target                               *
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

#include "FXFunctorTarget.h"

namespace FX {

// Keep similar to FXIMPLEMENT()
const FXMetaClass FXFunctorTarget::metaClass("FXFunctorTarget",FXFunctorTarget::manufacture,&FXObject::metaClass,NULL,0,sizeof(FXObject::FXMapEntry));

long FXFunctorTarget::handle(FXObject* sender,FXSelector sel,void* ptr)
{
	return handler(sender, sel, ptr);
}

} // namespace

