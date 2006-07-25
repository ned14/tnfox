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

#include "xincs.h"
#include "FXObjectList.h"
#include "FXVec2f.h"
#include "FXVec3f.h"
#include "FXVec4f.h"
#include "FXRangef.h"
#include "TnFXGraph.h"
#include "FXRollback.h"
#include "FXGLViewer.h"
#include "FXGLVertices.h"
#include "FXGLVisual.h"
#include "FXFont.h"
#include <qptrvector.h>
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

FXIMPLEMENT_ABSTRACT(TnFXGraph, FXGLGroup, NULL, 0)

struct TnFXGraphItem
{
	FXString title;
	FXGLVertices *vertices;
	QMemArray<FXVec2f> *plotData2d;
	QMemArray<FXVec3f> *plotData3d;
	FXAutoPtr< QMemArray<FXVec2f> > int_plotData2d;
	FXAutoPtr< QMemArray<FXVec3f> > int_plotData3d, temp;
	TnFXGraph::ExpandFunc expand;
	TnFXGraph::ReduceFunc reduce;
	TnFXGraphItem() : vertices(0), plotData2d(0), plotData3d(0), expand(0), reduce(0)
	{
	}
};

TnFXGraph::TnFXGraph() : items(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(items=new QPtrVector<TnFXGraphItem>(true));
	unconstr.dismiss();
}

TnFXGraph::~TnFXGraph()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(items);
} FXEXCEPTIONDESTRUCT2; }

inline TnFXGraphItem *TnFXGraph::int_item(FXuint i)
{
    items->extend(i+1);
	if(!(*items)[i])
	{
		TnFXGraphItem *it;
		FXERRHM(it=new TnFXGraphItem);
		FXRBOp unit=FXRBNew(it);
		items->replace(i, it);
		unit.dismiss();
		FXERRHM(it->vertices=new FXGLVertices(0, 0, 0, VERTICES_POINTS|VERTICES_LINES, NULL, 0, 4.0f, 2.0f));
		replace(i, it->vertices);
	}
	return (*items)[i];
}

FXGLVertices *TnFXGraph::setItemDetails(FXuint item, const FXString &title, const FXGLColor &colour, FXfloat pointsize, FXfloat linesize)
{
	TnFXGraphItem *i=int_item(item);
	i->title=title;
	i->vertices->setColor(colour);
	if(pointsize)
		i->vertices->setPointSize(pointsize);
	else
		i->vertices->setOptions(i->vertices->getOptions()&~VERTICES_POINTS);
	if(linesize)
		i->vertices->setLineSize(linesize);
	else
		i->vertices->setOptions(i->vertices->getOptions()&~VERTICES_LINES);
	return i->vertices;
}
FXGLVertices *TnFXGraph::setItemData(FXuint item, const FXVec2f *data, FXuint elements, TnFXGraph::ExpandFunc expand)
{
	TnFXGraphItem *i=int_item(item);
	if(!i->int_plotData2d)
	{
		FXERRHM(i->int_plotData2d=new QMemArray<FXVec2f>);
	}
	i->int_plotData2d->setRawData(const_cast<FXVec2f *>(data), elements, true);
	return setItemData(item, PtrPtr(i->int_plotData2d), expand);
}
FXGLVertices *TnFXGraph::setItemData(FXuint item, const QMemArray<FXVec2f> *data, TnFXGraph::ExpandFunc expand)
{
	TnFXGraphItem *i=int_item(item);
	i->plotData2d=const_cast<QMemArray<FXVec2f> *>(data);
	i->plotData3d=0;
	i->expand=expand;
	return i->vertices;
}
FXGLVertices *TnFXGraph::setItemData(FXuint item, const FXVec3f *data, FXuint elements, TnFXGraph::ReduceFunc reduce)
{
	TnFXGraphItem *i=int_item(item);
	if(!i->int_plotData3d)
	{
		FXERRHM(i->int_plotData3d=new QMemArray<FXVec3f>);
	}
	i->int_plotData3d->setRawData(const_cast<FXVec3f *>(data), elements, true);
	return setItemData(item, PtrPtr(i->int_plotData3d), reduce);
}
FXGLVertices *TnFXGraph::setItemData(FXuint item, const QMemArray<FXVec3f> *data, TnFXGraph::ReduceFunc reduce)
{
	TnFXGraphItem *i=int_item(item);
	i->plotData2d=0;
	i->plotData3d=const_cast<QMemArray<FXVec3f> *>(data);
	i->reduce=reduce;
	return i->vertices;
}
FXGLVertices *TnFXGraph::itemChanged(FXuint item)
{
	TnFXGraphItem *i=int_item(item);
	i->vertices->setModified();
	return i->vertices;
}


void TnFXGraph::bounds(FXRangef& box)
{
	int_prepareItems(0);
	FXGLGroup::bounds(box);
}
void TnFXGraph::draw(FXGLViewer *viewer)
{
	if(items->count() && ((*items)[0]->vertices->getOptions() & (SHADING_SMOOTH|SHADING_FLAT))) viewer->getApp()->beginWaitCursor();
	int_prepareItems(viewer);
	FXGLGroup::draw(viewer);
	if(items->count() && ((*items)[0]->vertices->getOptions() & (SHADING_SMOOTH|SHADING_FLAT))) viewer->getApp()->endWaitCursor();
}
void TnFXGraph::hit(FXGLViewer *viewer)
{
	int_prepareItems(viewer);
	FXGLGroup::hit(viewer);
}


//***************************************************************************************************


FXIMPLEMENT(TnFX2DGraph, TnFXGraph, NULL, 0)

struct TnFX2DGraphPrivate
{
	struct Axes
	{
		FXVec4f range;
		FXVec3f data[4];
		FXGLVertices *vertices;
		Axes() : vertices(0)
		{
			memset(data, 0, sizeof(data));
		}
	} axes;
	struct Majors
	{
		FXfloat granx, grany;
		const FXFont *font;
		FXfloat maxwidth, maxheight;
		FXGLVertices *vertices;
		QMemArray<FXVec3f> data;
		FXuint charlists;
		Majors() : granx(0), grany(0), font(0), maxwidth(0), maxheight(0), vertices(0), charlists(0) { }
		~Majors()
		{
			if(charlists)
				glDeleteLists(charlists, 14);
		}
	} majors;
};

TnFX2DGraph::TnFX2DGraph() : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new TnFX2DGraphPrivate);
	init();
	unconstr.dismiss();
}

TnFX2DGraph::TnFX2DGraph(const QMemArray<FXVec2f> *data, ExpandFunc expand) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new TnFX2DGraphPrivate);
	init();
	setItemData(0, data, expand);
	unconstr.dismiss();
}

TnFX2DGraph::TnFX2DGraph(const QMemArray<FXVec3f> *data, ReduceFunc reduce) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new TnFX2DGraphPrivate);
	init();
	setItemData(0, data, reduce);
	unconstr.dismiss();
}

TnFX2DGraph::~TnFX2DGraph()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

void TnFX2DGraph::init()
{
}

static inline float roundaxisdown(float val, float gran) throw()
{
	return gran*floorf(val/gran);
}
static inline float roundaxisup(float val, float gran) throw()
{
	return gran*ceilf(val/gran);
}
void TnFX2DGraph::int_prepareItems(FXGLViewer *viewer)
{	// Plot as 2D
	TnFXGraphItem *i;
	FXint end=0;
	for(QPtrVectorIterator<TnFXGraphItem> it(*items); (i=it.current()); ++it)
	{
		if(i->plotData3d)
		{
			if(!i->temp)
			{
				FXERRHM(i->temp=new QMemArray<FXVec3f>);
			}
			i->reduce(*i->temp, *i->plotData3d);
		}
		else if(i->plotData2d)
		{
			if(!i->temp)
			{
				FXERRHM(i->temp=new QMemArray<FXVec3f>);
			}
			Expand(*i->temp, *i->plotData2d);
		}
		if(i->vertices->getVertices()!=i->temp->data() || i->vertices->getNumberOfVertices()!=i->temp->count())
			i->vertices->setVertices(i->temp->data(), i->temp->count());
		replace(end++, i->vertices);
	}
	// Add axes and labels if wanted
	if(viewer && (p->axes.vertices || p->majors.vertices))
	{
		FXRangef total(0, 0, 0, 0, 0, 0), temp;
		for(QPtrVectorIterator<TnFXGraphItem> it(*items); (i=it.current()); ++it)
		{
			i->vertices->bounds(temp);
			total.include(temp);
		}
		if(p->majors.vertices)
		{	// Ensure 6 chars and 1 char between each major point
			float worldPix=(float) viewer->worldPix();
			p->majors.maxwidth=50;
			p->majors.maxheight=20;
			if(p->majors.font)
			{
				p->majors.maxwidth=(float) p->majors.font->getTextWidth("000000");
				p->majors.maxheight=(float) p->majors.font->getTextHeight("0");
			}

			float distx=worldPix*p->majors.maxwidth, disty=worldPix*p->majors.maxheight;
			float xwidth=total.upper.x-total.lower.x, ywidth=total.upper.y-total.lower.y, granx=p->majors.granx, grany=p->majors.grany;
			if(!granx)
				granx=roundaxisup(distx, 10*ceilf(fabsf(xwidth/200.0f)));
			if(!grany)
				grany=roundaxisup(disty, 10*ceilf(fabsf(ywidth/200.0f)));
			total.lower.x=(Generic::BiggestValue<float>::value==p->axes.range.x) ? roundaxisdown(total.lower.x, granx) : p->axes.range.x;
			total.upper.x=(Generic::BiggestValue<float>::value==p->axes.range.y) ? roundaxisup  (total.upper.x, granx) : p->axes.range.y;
			total.lower.y=(Generic::BiggestValue<float>::value==p->axes.range.z) ? roundaxisdown(total.lower.y, grany) : p->axes.range.z;
			total.upper.y=(Generic::BiggestValue<float>::value==p->axes.range.w) ? roundaxisup  (total.upper.y, grany) : p->axes.range.w;
			xwidth=total.upper.x-total.lower.x;
			ywidth=total.upper.y-total.lower.y;
			p->majors.data.resize((FXuint)(4+2*(xwidth/granx)+2*(ywidth/grany)));
			FXVec3f *data=p->majors.data.data();
			FXuint no=4;
			float marksize=4*worldPix;
			float mb=(total.lower.y<0) ? -marksize : 0;
			float mt=(total.upper.y>0) ?  marksize : 0;
			for(float n=total.lower.x; n<total.upper.x; n+=granx, no++)
			{
				*data++=FXVec3f(n, mb, 0);
				*data++=FXVec3f(n, mt, 0);
			}
			*data++=FXVec3f(total.upper.x, mb, 0);
			*data++=FXVec3f(total.upper.x, mt, 0);
			float mr=(total.lower.x<0) ?  marksize : 0;
			float ml=(total.upper.x>0) ? -marksize : 0;
			for(float n=total.lower.y; n<total.upper.y; n+=grany, no++)
			{
				*data++=FXVec3f(ml, n, 0);
				*data++=FXVec3f(mr, n, 0);
				//fxmessage("y=%f, lower=%f, upper=%f, gran=%f\n", n, total.lower.y, total.upper.y, grany);
			}
			*data++=FXVec3f(ml, total.upper.y, 0);
			*data++=FXVec3f(mr, total.upper.y, 0);
			p->majors.data.resize(no*2);
			if(p->majors.vertices->getVertices()!=p->majors.data.data() || p->majors.vertices->getNumberOfVertices()!=p->majors.data.count())
				p->majors.vertices->setVertices(p->majors.data.data(), p->majors.data.count());
			p->majors.vertices->setModified();
			replace(end++, p->majors.vertices);
			// We may need to generate fonts for the axis labels
			if(p->majors.font && !p->majors.charlists)
			{
				static const char digits[]="0123456789+-.e";
				p->majors.charlists=glGenLists(14);
				glUseFXFont(const_cast<FXFont *>(p->majors.font), digits[0], 10, p->majors.charlists);
				glUseFXFont(const_cast<FXFont *>(p->majors.font), digits[10], 1, p->majors.charlists+10);
				glUseFXFont(const_cast<FXFont *>(p->majors.font), digits[11], 1, p->majors.charlists+11);
				glUseFXFont(const_cast<FXFont *>(p->majors.font), digits[12], 1, p->majors.charlists+12);
				glUseFXFont(const_cast<FXFont *>(p->majors.font), digits[13], 1, p->majors.charlists+13);
			}
		}
		if(p->axes.vertices)
		{
			p->axes.data[0].x=total.lower.x;
			p->axes.data[1].x=total.upper.x;
			p->axes.data[2].y=total.lower.y;
			p->axes.data[3].y=total.upper.y;
			p->axes.vertices->setModified();
			replace(end++, p->axes.vertices);
		}
	}

	for(; end<no(); end++)
		remove(end);
}


FXGLVertices *TnFX2DGraph::setAxes(const FXVec4f &range, FXfloat lineSize)
{
	if(!lineSize)
	{
		if(p->axes.vertices)
		{
			remove(p->axes.vertices);
			FXDELETE(p->axes.vertices);
		}
	}
	else
	{
		p->axes.range=range;
		if(!p->axes.vertices)
		{
			FXERRHM(p->axes.vertices=new FXGLVertices(0,0,0, VERTICES_LINES|VERTICES_LINEITEMS, p->axes.data, 4, 0, lineSize));
			p->axes.vertices->setColor(FXGLColor(0,0,0));
			append(p->axes.vertices);
		}
		else
			p->axes.vertices->setLineSize(lineSize);
	}
	return p->axes.vertices;
}

FXGLVertices *TnFX2DGraph::setAxesMajor(FXfloat granx, FXfloat grany, FXfloat lineSize, const FXFont *font)
{
	if(!lineSize)
	{
		if(p->majors.vertices)
		{
			remove(p->majors.vertices);
			FXDELETE(p->majors.vertices);
		}
	}
	else
	{
		p->majors.granx=granx;
		p->majors.grany=grany;
		p->majors.font=font;
		if(!p->majors.vertices)
		{
			FXERRHM(p->majors.vertices=new FXGLVertices(0,0,0, VERTICES_LINES|VERTICES_LINEITEMS, p->majors.data.data(), 0, 0, lineSize));
			p->majors.vertices->setColor(FXGLColor(0,0,0));
			append(p->majors.vertices);
		}
		else
			p->majors.vertices->setLineSize(lineSize);
		if(p->majors.charlists)
		{
			glDeleteLists(p->majors.charlists, 11);
			p->majors.charlists=0;
		}
	}
	return p->majors.vertices;
}


void TnFX2DGraph::int_drawLabels(FXGLViewer *viewer)
{
	if(p->majors.vertices && p->majors.charlists)
	{
		float worldPix=(float) viewer->worldPix();
		FXGLColor col(0,0,0,1);
        glColor4fv(&col.r);
		for(FXuint n=0; n<p->majors.data.count(); n+=2)
		{
			const FXVec3f *v=&p->majors.data[n];
			if(fabsf(v[0].x)<1e-30f || fabsf(v[0].y)<1e-30f) continue;
			FXString text(FXString::number(v[0].x==v[1].x ? v[0].x : v[0].y));
			float width=(float) p->majors.font->getTextWidth(text), height=(float) p->majors.font->getTextHeight(text);
			glPushMatrix();
			glTranslatef(v[0].x, v[0].y, v[0].z);
			if(v[0].x==v[1].x)
				glTranslatef(worldPix*(-width/2), worldPix*(-height), 0);
			else
				glTranslatef(worldPix*(-10-width), worldPix*(-height/2), 0);
			glScalef(16*worldPix, 16*worldPix, 0);
			for(int c=0; c<text.length(); c++)
			{
				int cidx=('+'==text[c] ? 10 : '-'==text[c] ? 11 : '.'==text[c] ? 12 : ('e'==text[c] || 'E'==text[c]) ? 13 : text[c]-'0');
				glCallList(p->majors.charlists+cidx);
			}
			glPopMatrix();
		}
	}
}
void TnFX2DGraph::bounds(FXRangef& box)
{
	TnFXGraph::bounds(box);
	if(p->majors.vertices && p->majors.charlists)
	{
		box.include(-10-p->majors.maxwidth, -p->majors.maxheight, 0);
	}
}
void TnFX2DGraph::draw(FXGLViewer *viewer)
{	// Give best 2D results
	glHint(GL_POLYGON_SMOOTH_HINT,GL_NICEST);
	glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
	glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POINT_SMOOTH);

	TnFXGraph::draw(viewer);
	int_drawLabels(viewer);
}
void TnFX2DGraph::hit(FXGLViewer *viewer)
{
	TnFXGraph::hit(viewer);
	int_drawLabels(viewer);
}


//***************************************************************************************************

FXIMPLEMENT(TnFX3DGraph, TnFXGraph, NULL, 0)

struct TnFX3DGraphPrivate
{
};

TnFX3DGraph::TnFX3DGraph() : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new TnFX3DGraphPrivate);
	init();
	unconstr.dismiss();
}

TnFX3DGraph::TnFX3DGraph(const QMemArray<FXVec2f> *data, ExpandFunc expand) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new TnFX3DGraphPrivate);
	init();
	setItemData(0, data, expand);
	unconstr.dismiss();
}

TnFX3DGraph::TnFX3DGraph(const QMemArray<FXVec3f> *data, ReduceFunc reduce) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new TnFX3DGraphPrivate);
	init();
	setItemData(0, data, reduce);
	unconstr.dismiss();
}

TnFX3DGraph::~TnFX3DGraph()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

void TnFX3DGraph::init()
{
}


void TnFX3DGraph::int_prepareItems(FXGLViewer *viewer)
{	// Plot as 3D
	QMemArray<FXVec3f> *plotData=0;
	TnFXGraphItem *i;
	for(QPtrVectorIterator<TnFXGraphItem> it(*items); (i=it.current()); ++it)
	{
		if(i->plotData3d)
			plotData=i->plotData3d;
		else if(i->plotData2d)
		{
			if(!i->temp)
			{
				FXERRHM(i->temp=new QMemArray<FXVec3f>);
			}
			i->expand(*i->temp, *i->plotData2d);
			plotData=PtrPtr(i->temp);
		}
		if(i->vertices->getVertices()!=plotData->data() || i->vertices->getNumberOfVertices()!=plotData->count())
			i->vertices->setVertices(plotData->data(), plotData->count());
	}
}

}
#endif
#endif
