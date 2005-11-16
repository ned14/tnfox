/* THourglass.h
Much enhanced wait cursor
(C) 2002, 2003 Niall Douglas
Original version created: Nov 2002
This version created: 4th Feb 2003
*/

/* This code is licensed for use only in testing TnFOX facilities.
It may not be used by other code or incorporated into code without
prior written permission */

#include "THourglass.h"
#include "FXCursor.h"
#include "FXPNGImage.h"
#include "FXWindow.h"
#include "FXFile.h"
#include <qmemarray.h>
#include <qptrvector.h>
#include <qptrdict.h>

#ifdef USE_POSIX
#include <X11/Xlib.h>
#else
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif
#include "FXMemDbg.h"
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace Tn {

#define WIDTH 18
#define HEIGHT 32
#define FRAMES 8
#define HOTSPOT_X 9
#define HOTSPOT_Y 12

static QMutex lock;
static QPtrVector<FXImage> templates(true);
struct WindowInfo
{
	int count, percent;
	FXColor topLED, bottomLED;
	FXApp *app;
	FXWindow *window;
#ifdef WIN32
	DWORD wndthreadid;
#endif
	QThreadPool::handle threadpoolH;
	FXCursor *before;
	struct Frame
	{
		QMemArray<FXColor> image;
		FXCursor cursor;
		Frame(FXApp *app, const FXImage *i) : image(WIDTH*HEIGHT), cursor(app, image.data(), WIDTH, HEIGHT, HOTSPOT_X, HOTSPOT_Y, CURSOR_KEEP)
		{
			memcpy(image.data(), i->getData(), WIDTH*HEIGHT*sizeof(FXColor));
		}
	};
	QPtrVector<Frame> frames;
	int frame;
	WindowInfo(FXWindow *w) : count(0), percent(-1), topLED((FXColor)-1), bottomLED((FXColor)-1),
		app(w ? w->getApp() : FXApp::instance()), window(w), threadpoolH(0), before(0), frames(true), frame(0)
	{
		if(window)
			before=window->getDefaultCursor();
		else
			before=app->getWaitCursor();
#ifdef WIN32
		wndthreadid=GetWindowThreadProcessId((HWND) window->id(), NULL);
#endif
	}
	void defineCursor();
	void setCursor()
	{	// Is the mouse pointer inside our window?
#ifndef WIN32
		Window root,child; int rx,ry,x,y; unsigned int buttons;
		if(!XQueryPointer((Display *) app->getDisplay(),window->id(),&root,&child,&rx,&ry,&x,&y,&buttons)) return;
#else
		POINT p;
		GetCursorPos(&p);
		if((HWND) window->id()!=WindowFromPoint(p)) return;
		// Annoyingly we must attach to the thread in which the window "lives"
		DWORD myid=GetCurrentThreadId();
		if(myid!=wndthreadid)
			AttachThreadInput(myid, wndthreadid, TRUE);
#endif
		if(window)
			window->setDefaultCursor(&frames[frame]->cursor);
		else
			app->setWaitCursor(&frames[frame]->cursor);
#ifndef WIN32
		XFlush((Display *) app->getDisplay());
#else
		if(myid!=wndthreadid)
			AttachThreadInput(GetCurrentThreadId(), wndthreadid, FALSE);
#endif
	}
};
static QPtrDict<WindowInfo> windowlist(13, true);

static void initHourglass(FXApp *app)
{
	if(templates.isEmpty())
	{
		FXString icpath="../../TestSuite/Tn";
		for(FXuint n=0; n<FRAMES; n++)
		{
			FXString ipath=icpath+"/icons/Hourglass%1.png";
			ipath.arg(n);
			FXPNGImage *i;
			FXERRHM(i=new FXPNGImage(app));
			FXRBOp uni=FXRBNew(i);
			templates.append(i);
			uni.dismiss();
			FXFile f(ipath);
			f.open(IO_ReadOnly);
			FXStream s(&f);
			i->loadPixels(s);
			for(u32 y=25; y<32; y++)
			{
				for(u32 x=0; x<WIDTH-4; x++)
					i->getData()[WIDTH*y+x]=FXRGBA(0,0,0,255);
			}
		}
	}
}

#define TEXTCOLOUR FXRGBA(255,255,255,255)
static inline void drawPoint(FXColor *data, int x, int y)
{
	data[WIDTH*y+x]=TEXTCOLOUR;
}
static inline void drawLine(FXColor *data, int x1, int y1, int x2, int y2)
{
	if(x1==x2)
	{
		for(int y=FXMIN(y1,y2); y<=FXMAX(y1,y2); y++)
			data[WIDTH*y+x1]=TEXTCOLOUR;
	}
	else if(y1==y2)
	{
		for(int x=FXMIN(x1,x2); x<=FXMAX(x1,x2); x++)
			data[WIDTH*y1+x]=TEXTCOLOUR;
	}
	else assert(0);
}
static void drawNumber(FXColor *data, int x, int digit)
{	// Would it actually be quicker to bltblt these???
	const int y=0;
	if(1==digit)
	{
		drawPoint(data, x+2,y+1); drawLine(data, x+3,y+0,x+3,y+6);
		return;
	}
	if(4==digit)
	{
		drawLine(data, x+2,y+0,x+2,y+6);
		drawLine(data, x+0,y+4,x+3,y+4);
		drawPoint(data, x+0,y+3); drawPoint(data, x+0,y+2); drawPoint(data, x+1,y+1);
		return;
	}
	if(5==digit || 7==digit)
	{	// Top left and top right
		drawPoint(data, x+0,y+0);
		drawPoint(data, x+3,y+0);
	}
	if(2==digit || 3==digit || 5==digit || 6==digit || 8==digit || 9==digit || 0==digit)
	{
		drawPoint(data, x+0,y+1); 
	}
	if(2==digit || 3==digit || 5==digit || 6==digit || 7==digit || 8==digit || 9==digit || 0==digit)
	{	// Top
		drawPoint(data, x+1,y+0); drawPoint(data, x+2,y+0); 
	}
	if(2==digit || 3==digit || 6==digit || 7==digit || 8==digit || 9==digit || 0==digit) drawPoint(data, x+3,y+1);
	if(2==digit || 3==digit || 7==digit || 8==digit || 9==digit || 0==digit)
	{	// Top right side 2
		drawPoint(data, x+3,y+2);
	}
	if(9==digit || 0==digit)
	{	// Middle right
		drawPoint(data, x+3,y+3);
	}
	if(5==digit || 6==digit || 8==digit || 9==digit || 0==digit)
	{	// Top left side
		drawPoint(data, x+0,y+2);
	}
	if(6==digit || 0==digit)
	{	// Mid left
		drawPoint(data, x+0,y+3);
	}
	if(3==digit || 5==digit || 6==digit || 8==digit || 9==digit)
	{	// Middle
		drawPoint(data, x+1,y+3); drawPoint(data, x+2,y+3);
	}
	if(3==digit || 5==digit || 6==digit || 8==digit || 9==digit || 0==digit)
	{	// Bottom right side
		drawPoint(data, x+3,y+4); drawPoint(data, x+3,y+5);
	}
	if(2==digit || 7==digit)
	{	// Mid cross section
		drawPoint(data, x+2,y+3); drawPoint(data, x+1,y+4);
	}
	if(6==digit || 8==digit || 0==digit)
	{	// Bottom left side 1
		drawPoint(data, x+0,y+4);
	}
	if(2==digit || 3==digit || 5==digit || 6==digit || 7==digit || 8==digit || 9==digit || 0==digit)
	{	// Bottom left side 2
		drawPoint(data, x+0,y+5);
	}
	if(2==digit || 3==digit || 5==digit || 6==digit || 8==digit || 9==digit || 0==digit)
	{	// Bottom
		drawPoint(data, x+1,y+6); drawPoint(data, x+2,y+6);
	}
	if(2==digit || 7==digit)
	{	// Bottom left corner and bottom right corner
		drawPoint(data, x+0,y+6); if(2==digit) drawPoint(data, x+3,y+6);
	}
}


void WindowInfo::defineCursor()
{
	if(frames.isEmpty())
	{
		for(int n=0; n<FRAMES; n++)
		{
			Frame *f;
			FXERRHM(f=new Frame(app, templates[n]));
			FXRBOp unf=FXRBNew(f);
			frames.append(f);
			unf.dismiss();
		}
	}
	u32 n;
	for(n=0; n<frames.count(); n++)		// Destroy existing server side version
	{
		frames[n]->cursor.destroy();
	}
	static const u32 pstart=WIDTH*25, plen=WIDTH*7;
	if(-1==percent)
	{	// Clear the percentage part
		for(u32 n=0; n<frames.count(); n++)
		{
			memset(frames[n]->image.data()+pstart, 0x00, plen*sizeof(FXColor));
		}
	}
	else
	{	// Reset percentage part
		FXColor *data=frames[0]->image.data();
		memcpy(data+pstart, templates[0]->getData()+pstart, plen*sizeof(FXColor));
		int c=percent % 10;
		int b=(percent/10) % 10;
		int a=percent/100;
		data+=pstart;
		drawNumber(data, 9, c);
		if(a || b) drawNumber(data, 4, b);
		if(a) drawNumber(data, -1, a);
		for(u32 n=1; n<frames.count(); n++)
		{
			memcpy(frames[n]->image.data()+pstart, data, plen*sizeof(FXColor));
		}
	}
	FXColor tl=topLED, bl=bottomLED, body=frames[0]->image.data()[WIDTH/2];
	if((FXColor)-1==tl) tl=body; if((FXColor)-1==bl) bl=body;
	for(n=0; n<frames.count(); n++)		// Create new server side version
	{
		FXColor *data=frames[n]->image.data();
		for(int top=3; top<WIDTH-3; top++)
		{
			data[WIDTH*1 +top]=data[WIDTH*2 +top]=tl;
			data[WIDTH*21+top]=data[WIDTH*22+top]=bl;
		}
		frames[n]->cursor.create();
	}
}

static void animateCursor(FXWindow *w)
{
	QMtxHold h(lock);
	WindowInfo *wi=windowlist.find(w);
	if(!wi) return;
	if(wi->frames.isEmpty()) wi->defineCursor();
	wi->setCursor();
	if(++wi->frame>FRAMES-1) wi->frame=0;
	wi->threadpoolH=FXProcess::threadPool().dispatch(Generic::BindFuncN(animateCursor, w), 200);
}

u32 THourglass::count(FXWindow *w)
{
	QMtxHold h(lock);
	WindowInfo *wi=windowlist.find(w);
	if(wi) return wi->count;
	return 0;
}

void THourglass::on(FXWindow *w, u32 wait)
{
	QMtxHold h(lock);
	WindowInfo *wi=windowlist.find(w);
	if(!wi)
	{
		initHourglass(w->getApp());
		FXERRHM(wi=new WindowInfo(w));
		FXRBOp unwi=FXRBNew(wi);
		windowlist.insert(w, wi);
		unwi.dismiss();
		wi->threadpoolH=FXProcess::threadPool().dispatch(Generic::BindFuncN(animateCursor, w), wait);
	}
	wi->count++;
}

void THourglass::off(FXWindow *w)
{
	QMtxHold h(lock);
	WindowInfo *wi=windowlist.find(w);
	if(wi)
	{
		if(!--wi->count)
		{
			FXProcess::threadPool().cancel(wi->threadpoolH);
			if(w)
				w->setDefaultCursor(wi->before);
			else
				wi->app->setWaitCursor(wi->before);
			windowlist.remove(w);
		}
	}
}

void THourglass::smash(FXWindow *w)
{
	QMtxHold h(lock);
	WindowInfo *wi=windowlist.find(w);
	if(wi)
	{
		wi->count=1;
		off(w);
	}
}

int THourglass::percent(FXWindow *w)
{
	QMtxHold h(lock);
	WindowInfo *wi=windowlist.find(w);
	if(wi) return wi->percent;
	return 0;
}

void THourglass::setPercent(FXWindow *w, int percent)
{
	QMtxHold h(lock);
	WindowInfo *wi=windowlist.find(w);
	if(percent<-1) percent=-1;
	if(percent>100) percent=100;
	if(wi && wi->percent!=percent)
	{
		wi->percent=percent;
		wi->defineCursor();
		wi->setCursor();
	}
}

void THourglass::setLED(FXWindow *w, u32 no, FXColor colour)
{
	QMtxHold h(lock);
	WindowInfo *wi=windowlist.find(w);
	if(wi)
	{
		if(0==no)
		{
			if(wi->topLED!=colour)
			{
				wi->topLED=colour;
				wi->defineCursor();
				wi->setCursor();
			}
		}
		else if(1==no)
		{
			if(wi->bottomLED!=colour)
			{
				wi->bottomLED=colour;
				wi->defineCursor();
				wi->setCursor();
			}
		}
	}
}

} // namespace

