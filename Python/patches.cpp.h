/********************************************************************************
*                                                                               *
*                           Python embedding support                            *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003-2006 by Niall Douglas.   All Rights Reserved.       *
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
#include <qmemarray.h>
#include "CArrays.h"

static inline void FXApp_init(FX::FXApp &app, int argc, boost::python::list argv, unsigned char connect=TRUE)
{
	using namespace FX;
	int n, size=PyList_Size(argv.ptr());
	static QMemArray<const char *> array;
	array.resize(size+1);
	for(n=0; n<size; n++)
	{
		array[n]=PyString_AsString(PyList_GetItem(argv.ptr(), n));
	}
	array[n]=0;
	app.init(argc, (char **)(array.data()), connect);
}
static inline void FXApp_init2(FX::FXApp &app, int argc, boost::python::list argv)
{
	FXApp_init(app, argc, argv);
}

DEFINE_MAKECARRAYITER(FXApp, const FX::FXchar *, getArgv, (), c.getArgc())

DEFINE_MAKECARRAYITER(FXImage, FX::FXColor, getData, (), (c.getWidth()*c.getHeight()))

DEFINE_MAKECARRAYITER(FXBitmap, FX::FXuchar, getData, (), (c.getWidth()*c.getHeight()/8))

