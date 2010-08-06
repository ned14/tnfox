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

#if defined(PYPP_BUILDING_FXAPP) || defined(PYPP_BUILDING_FXIMAGE) || defined(PYPP_BUILDING_FXBITMAP) || defined(PYPP_BUILDING_FXGLTRIANGLEMESH) || defined(PYPP_BUILDING_FXGLVIEWER)

#include <boost/python/suite/indexing/iterator_range.hpp>
#include <boost/python/suite/indexing/container_suite.hpp>

/* This truly evil looking macro would have been impossible without the invaluable
advice of Raoul Gough, the author of the Boost.Python indexing_suite
*/
#define DEFINE_MAKECARRAYITER(contType, memberType, getArrayFunction, getArrayFunctionPars, getArrayLengthFunction) \
	static inline boost::python::indexing::iterator_range<memberType *> \
	contType##_##getArrayFunction (::FX::##contType &c) \
	{ \
		using namespace boost::python; \
		typedef indexing::iterator_range<memberType *> IterPair; \
		class_<IterPair>( #contType "_" #getArrayFunction "Indirect", \
			init<memberType *, memberType *>()) \
			.def(indexing::container_suite<IterPair>()); \
		memberType *data=(&c)->getArrayFunction getArrayFunctionPars; \
		return IterPair(data, data+getArrayLengthFunction); \
	}


#if defined(PYPP_BUILDING_FXAPP)

DEFINE_MAKECARRAYITER(FXApp, const FX::FXchar *, getArgv, (), c.getArgc())

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

#endif // defined(PYPP_BUILDING_FXAPP)

#if defined(PYPP_BUILDING_FXIMAGE)
DEFINE_MAKECARRAYITER(FXImage, FX::FXColor, getData, (), (c.getWidth()*c.getHeight()))
#endif

#if defined(PYPP_BUILDING_FXBITMAP)
DEFINE_MAKECARRAYITER(FXBitmap, FX::FXuchar, getData, (), (c.getWidth()*c.getHeight()/8))
#endif

#if defined(PYPP_BUILDING_FXGLTRIANGLEMESH)
DEFINE_MAKECARRAYITER(FXGLTriangleMesh, FX::FXfloat, getVertexBuffer, (), 3*c.getVertexNumber())
DEFINE_MAKECARRAYITER(FXGLTriangleMesh, FX::FXfloat, getColorBuffer, (), 4*c.getVertexNumber())
DEFINE_MAKECARRAYITER(FXGLTriangleMesh, FX::FXfloat, getNormalBuffer, (), 3*c.getVertexNumber())
DEFINE_MAKECARRAYITER(FXGLTriangleMesh, FX::FXfloat, getTextureCoordBuffer, (), 2*c.getVertexNumber())
#endif

// To be fixed later
// DEFINE_MAKECARRAYITER(FXObjectList, FX::FXObject *, list, (), c.no())

#if defined(PYPP_BUILDING_FXGLVIEWER)
static inline FXMallocHolder<FX::FXGLObject *> *FXGLViewer_lasso(FX::FXGLViewer &c, FX::FXint x1,FX::FXint y1,FX::FXint x2,FX::FXint y2)
{
	return new FXMallocHolder<FX::FXGLObject *>(c.lasso(x1,y1,x2,y2));
}
static inline FXMallocHolder<FX::FXGLObject *> *FXGLViewer_select(FX::FXGLViewer &c, FX::FXint x,FX::FXint y,FX::FXint w,FX::FXint h)
{
	return new FXMallocHolder<FX::FXGLObject *>(c.select(x,y,w,h));
}
#endif

#endif // defined(PYPP_BUILDING_FXAPP) || defined(PYPP_BUILDING_FXIMAGE) || defined(PYPP_BUILDING_FXBITMAP) || defined(PYPP_BUILDING_FXGLTRIANGLEMESH) || defined(PYPP_BUILDING_FXGLVIEWER)
