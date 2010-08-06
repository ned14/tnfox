/********************************************************************************
*                                                                               *
*                              2D and 3D Graphing                               *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006 by Niall Douglas.   All Rights Reserved.                   *
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
#ifndef FX_DISABLEGL

#if FX_GRAPHINGMODULE

#ifndef TNFXGRAPH_H
#define TNFXGRAPH_H

#include "FXApp.h"
#include "FXGLObject.h"
#include "FXVec4f.h"
#include "qmemarray.h"

namespace FX {

/*! \file TnFXGraph.h
\brief Defines classes used to draw graphs
*/

/*! \defgroup graphing 2D and 3D Graphing

As of v0.87, TnFOX now has graphing support which allows the display of two
and three dimensional data. Currently, the graphs are limited to basically
a scatter plot of points with optional lines drawn between them - however,
quite a number of familiar graph types can be built from these primitives.

All present (and future) types of graph derive from FX::TnFXGraph with the
exception of FX::TnFXVTKCanvas. FX::TnFXGraph derives from FX::FXGLGroup
and is essentially a series of OpenGL primitives, most especially
FX::FXGLVertices. Each data item plot N is also item N in the GL group
object so you can access and manipulate them if you wish to fine tune their
configuration. Additional items like axes and major/minor marks are appended
to the GL group as subsequent items.

To display the graph on the screen, simply instantiate a FX::FXGLViewer and
set the scene to be the graph. You can see an example in <tt>TestSuite/TestGraphing</tt>.

FX::TnFXVTKCanvas is somewhat different in that it is a subclass of FX::FXGLCanvas
and therefore is more like a FX::FXGLViewer. It effectively "views" a VTK
model and allows the user to interact with it as they would in any normal
VTK application.
*/

class FXGLVertices;
class FXVec3d;
template<class type> class QPtrVector;


/*! \class TnFXGraph
\ingroup graphing
\brief Abstract base class for a graph renderer

This base class for OpenGL graph renderers provides a number of useful facilities.
For one, you can supply either 2D or 3D data and the appropriate subclass which
either reduce or expand it to what is required by a user supplied function.

If you want to set the colour, line width etc, set it via the FX::FXGLVertices
instance representing that item. Note that if you change the data to be shown,
you MUST call itemChanged() to regenerate the OpenGL display lists used to
show the item.
*/
struct TnFXGraphItem;
class FXGRAPHINGMODULEAPI TnFXGraph : public FXGLGroup
{
	FXDECLARE_ABSTRACT(TnFXGraph)
	TnFXGraph(const TnFXGraph &);
	TnFXGraph &operator=(const TnFXGraph &);
	inline FXDLLLOCAL TnFXGraphItem *int_item(FXuint item);
protected:
	QPtrVector<TnFXGraphItem> *items;
	virtual void int_prepareItems(FXGLViewer *viewer)=0;
	TnFXGraph();
public:
	~TnFXGraph();

	//! A function translating a 2d array into a 3d array
	typedef void (*ExpandFunc)(QMemArray<FXVec3f> &out, const QMemArray<FXVec2f> &in);
	//! A function translating a 3d array into a 2d array
	typedef void (*ReduceFunc)(QMemArray<FXVec3f> &out, const QMemArray<FXVec3f> &in);

	//! Sets item title and colour
	FXGLVertices *setItemDetails(FXuint item, const FXString &title, const FXGLColor &colour, FXfloat pointsize=4.0f, FXfloat linesize=2.0f);
	//! Sets the data to be used, with zero unsetting
	FXGLVertices *setItemData(FXuint item, const FXVec2f *data, FXuint elements, ExpandFunc expand=Expand);
	//! Sets the data to be used, with zero unsetting. Automatically fetches the length thereafter
	FXGLVertices *setItemData(FXuint item, const QMemArray<FXVec2f> *data, ExpandFunc expand=Expand);
	//! Sets the data to be used, with zero unsetting
	FXGLVertices *setItemData(FXuint item, const FXVec3f *data, FXuint elements, ReduceFunc reduce=ReduceZ);
	//! Sets the data to be used, with zero unsetting. Automatically fetches the length thereafter
	FXGLVertices *setItemData(FXuint item, const QMemArray<FXVec3f> *data, ReduceFunc reduce=ReduceZ);
	//! Indicates that an item's data should be recalculated
	FXGLVertices *itemChanged(FXuint item);

	virtual void bounds(FXRangef& box);
	virtual void draw(FXGLViewer *viewer);
	virtual void hit(FXGLViewer *viewer);
public:
	//! Expands a 2d array into a 3d array by setting of Z
	template<FXint zval> static void Expand(QMemArray<FXVec3f> &out, const QMemArray<FXVec2f> &in)
	{
		out.resize(in.count());
		for(FXuint n=0; n<in.count(); n++)
		{
			out[n].x=in[n].x;
			out[n].y=in[n].y;
			out[n].z=zval;
		}
	}
	//! Expands a 2d array into a 3d array by setting of Z to zero
	static void Expand(QMemArray<FXVec3f> &out, const QMemArray<FXVec2f> &in)
	{
		out.resize(in.count());
		for(FXuint n=0; n<in.count(); n++)
		{
			out[n].x=in[n].x;
			out[n].y=in[n].y;
			out[n].z=0.0f;
		}
	}
	//! Translates a 3d array into a 2d array by elimination of a coordinate, stepping \em stepe at the end and \em stepm in the middle
	template<int offset, int stepe, int stepm> static void ReduceE(QMemArray<FXVec3f> &out, const QMemArray<FXVec3f> &in)
	{
		out.resize(in.count()*3/(stepe+stepm));
		FXfloat *FXRESTRICT o=(FXfloat *) out.data();
		const FXfloat *FXRESTRICT i=(const FXfloat *) in.data();
		for(FXuint n=offset; n<in.count()*3; n+=stepe)
		{
			*o++=i[n];
			*o++=i[n+stepm];
			*o++=0;
		}
	}
	//! Translates a 3d array into a 2d array by elimination of two coordinates
	template<int yinc, int offset, int step> static void ReduceE2x(QMemArray<FXVec3f> &out, const QMemArray<FXVec3f> &in)
	{
		out.resize(in.count()*3/step);
		FXfloat y=0;
		FXfloat *FXRESTRICT o=(FXfloat *) out.data();
		const FXfloat *FXRESTRICT i=(const FXfloat *) in.data();
		for(FXuint n=offset; n<in.count()*3; n+=step)
		{
			*o++=i[n];
			*o++=y;
			*o++=0;
			y+=yinc;
		}
	}
	//! Translates a 3d array into a 2d array by elimination of two coordinates
	template<int xinc, int offset, int step> static void ReduceE2y(QMemArray<FXVec3f> &out, const QMemArray<FXVec3f> &in)
	{
		out.resize(in.count()*3/step);
		FXfloat x=0;
		FXfloat *FXRESTRICT o=(FXfloat *) out.data();
		const FXfloat *FXRESTRICT i=(const FXfloat *) in.data();
		for(FXuint n=offset; n<in.count()*3; n+=step)
		{
			*o++=x;
			x+=xinc;
			*o++=i[n];
			*o++=0;
		}
	}
	//! Translates a 3d array into a 2d array by division of Z
	static void ReduceZ(QMemArray<FXVec3f> &out, const QMemArray<FXVec3f> &in)
	{
		out.resize(in.count());
		for(FXuint n=0; n<in.count(); n++)
		{
			out[n].x=in[n].x/in[n].z;
			out[n].y=in[n].y/in[n].z;
			out[n].z=0;
		}
	}
};


/*! \class TnFX2DGraph
\ingroup graphing
\brief Renders a 2D graph

In addition to the standard facilities offered by FX::TnFXGraph, TnFX2DGraph
also provides optional axes and major indicators with labels.
*/
struct TnFX2DGraphPrivate;
class FXGRAPHINGMODULEAPI TnFX2DGraph : public TnFXGraph
{
	FXDECLARE(TnFX2DGraph)
	TnFX2DGraphPrivate *p;
	TnFX2DGraph(const TnFX2DGraph &);
	TnFX2DGraph &operator=(const TnFX2DGraph &);
	void init();
	virtual void int_prepareItems(FXGLViewer *viewer);
	void int_drawLabels(FXGLViewer *viewer);
public:
	//! Constructs an instance
	TnFX2DGraph();
	TnFX2DGraph(const QMemArray<FXVec2f> *data, ExpandFunc expand=Expand);
	TnFX2DGraph(const QMemArray<FXVec3f> *data, ReduceFunc reduce=ReduceZ);
	~TnFX2DGraph();

	//! Sets axes. Defaults set automatic ranging. Set \em lineSize to 0 to remove.
	FXGLVertices *setAxes(const FXVec4f &range=FXVec4f(Generic::BiggestValue<float>::value, Generic::BiggestValue<float>::value, Generic::BiggestValue<float>::value, Generic::BiggestValue<float>::value), FXfloat lineSize=4.0f);
	//! Sets major marks with optional text (set zero for no text).
	FXGLVertices *setAxesMajor(FXfloat granx=0, FXfloat grany=0, FXfloat lineSize=2.0f, const FXFont *font=FXApp::instance()->getNormalFont());

	virtual void bounds(FXRangef& box);
	virtual void draw(FXGLViewer *viewer);
	virtual void hit(FXGLViewer *viewer);
};


/*! \class TnFX3DGraph
\ingroup graphing
\brief Renders a 3D graph

The graphs can come in one of two formats: quick and pretty. The quick scales up
to 500,000 points on typical 3D hardware without a problem whereas the pretty
scales up to 100,000 on typical 3D hardware. As rendering all the points can take
too long to allow smooth animation during user manipulation, the graph offers a
turbo mode which quarters the number of points being plotted during animation.
See FX::FXGLViewer::setTurbo().

Pretty mode can be enabled by specifying the option \c STYLE_SURFACE|SHADING_SMOOTH
on the FX::FXGLVertices underlying the item being rendered. You may also want to
set up a specular light as well as a colour generation function.
*/
struct TnFX3DGraphPrivate;
class FXGRAPHINGMODULEAPI TnFX3DGraph : public TnFXGraph
{
	FXDECLARE(TnFX3DGraph)
	TnFX3DGraphPrivate *p;
	TnFX3DGraph(const TnFX3DGraph &);
	TnFX3DGraph &operator=(const TnFX3DGraph &);
	void init();
	virtual void int_prepareItems(FXGLViewer *viewer);
public:
	//! Constructs an instance
	TnFX3DGraph();
	TnFX3DGraph(const QMemArray<FXVec2f> *data, ExpandFunc expand=Expand);
	TnFX3DGraph(const QMemArray<FXVec3f> *data, ReduceFunc reduce=ReduceZ);
	~TnFX3DGraph();
};

}

#endif
#endif
#endif
