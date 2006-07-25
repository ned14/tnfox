/* This file was taken from http://sourceforge.net/projects/vtkfox
Resides in the public domain
*/
#if FX_GRAPHINGMODULE


/*
* TnFXVTKCanvas Widget Implementation
* 
* Author: Doug Henry (brilligent@gmail.com)
*/


#include "TnFXVTKCanvas.h"

#ifdef HAVE_VTK
#include <vtkRenderWindow.h>
#include <vtkCommand.h>
#endif


namespace FX {


FXDEFMAP(TnFXVTKCanvas) TnFXVTKCanvasMap[] =
{
    FXMAPFUNC(SEL_PAINT, 0, TnFXVTKCanvas::onPaint),
    FXMAPFUNC(SEL_TIMEOUT, 0, TnFXVTKCanvas::onTimeout),
    FXMAPFUNC(SEL_CONFIGURE, 0, TnFXVTKCanvas::onResize),
    
    FXMAPFUNC(SEL_LEFTBUTTONPRESS, 0, TnFXVTKCanvas::onLeftButtonDown),
    FXMAPFUNC(SEL_LEFTBUTTONRELEASE, 0, TnFXVTKCanvas::onLeftButtonUp),
    FXMAPFUNC(SEL_MIDDLEBUTTONPRESS, 0, TnFXVTKCanvas::onMiddleButtonDown),
    FXMAPFUNC(SEL_MIDDLEBUTTONRELEASE, 0, TnFXVTKCanvas::onMiddleButtonUp),
    FXMAPFUNC(SEL_RIGHTBUTTONPRESS, 0, TnFXVTKCanvas::onRightButtonDown),
    FXMAPFUNC(SEL_RIGHTBUTTONRELEASE, 0, TnFXVTKCanvas::onRightButtonUp),
    FXMAPFUNC(SEL_MOTION, 0, TnFXVTKCanvas::onMotion),
    FXMAPFUNC(SEL_KEYPRESS, 0, TnFXVTKCanvas::onKeyboard)
};
    
FXIMPLEMENT(TnFXVTKCanvas, FXGLCanvas, TnFXVTKCanvasMap, ARRAYNUMBER(TnFXVTKCanvasMap))


/**
 * FOX compatible constructor.  Should match the constructor given in FOX itself for
 * the FXGLCanvas widget.
 */

TnFXVTKCanvas::TnFXVTKCanvas(FXComposite *p, FXGLVisual *vis, FXObject *tgt, FXSelector sel, FXuint opts, FXint x, FXint y, FXint w, FXint h)
		: FXGLCanvas(p, vis, tgt, sel, opts, x, y, w, h)
		, _fxrwi(NULL)
		, _id(NULL)
		, _display(NULL)
{
#ifdef HAVE_VTK
	_fxrwi = new vtkTnFXRenderWindowInteractor(this);
#endif
}


/**
 * FOX compatible constructor.  Should match the constructor given in FOX itself for
 * the FXGLCanvas widget.
 */

TnFXVTKCanvas::TnFXVTKCanvas(FXComposite *p, FXGLVisual *vis, FXGLCanvas *sharegroup, FXObject *tgt, FXSelector sel, FXuint opts, FXint x, FXint y, FXint w, FXint h)
		: FXGLCanvas(p, vis, sharegroup, tgt, sel, opts, x, y, w, h)
		, _fxrwi(NULL)
		, _id(NULL)
		, _display(NULL)
{
#ifdef HAVE_VTK
	_fxrwi = new vtkTnFXRenderWindowInteractor(this);
#endif
}

/**
 * Delete the interactor using the VTK Delete reference counting
 * method.
 */

TnFXVTKCanvas::~TnFXVTKCanvas()
{
#ifdef HAVE_VTK
	_fxrwi->Delete();
#endif
}


/**
 * Get the internally stored interactor.
 * 
 * @return pointer to vtkFOX interactor
 */

vtkTnFXRenderWindowInteractor* TnFXVTKCanvas::interactor() const throw()
{
	return _fxrwi;
}


/**
 * Set the internal interactor, deleting the current interactor if one
 * exists.  This class will maintain control of the interactor (e.g. clean it up).
 * 
 * @param fxrwi pointer to a vtkFOX interactor
 */

void TnFXVTKCanvas::setInteractor(vtkTnFXRenderWindowInteractor *fxrwi) throw()
{
#ifdef HAVE_VTK
	if (fxrwi != NULL)
	{
		_fxrwi->Delete();
		_fxrwi = fxrwi;
		
		_fxrwi->setCanvas(this);
	}
#endif
}


/**
 * Standard FOX create.  The window id and display id are stored
 * for use with VTK.
 */

void TnFXVTKCanvas::create()
{
	FXGLCanvas::create();
	
	_id = (void*)id();
	_display = getApp()->getDisplay();	
}


void TnFXVTKCanvas::render(bool _makeCurrent)
{
#ifdef HAVE_VTK
	if(_makeCurrent) makeCurrent();
	
	_fxrwi->GetRenderWindow()->SetWindowId(_id);
#ifndef WIN32
	_fxrwi->GetRenderWindow()->SetDisplayId(_display);
#endif
	_fxrwi->Render();
	
	if(_makeCurrent) makeNonCurrent();
#endif
}

/**
 * Repaint callback handler.  Sets the window and display ids and then
 * calls the VTK renderer.
 */

long TnFXVTKCanvas::onPaint(FXObject *obj, FXSelector sel, void *data)
{	
#ifdef HAVE_VTK
	render();
#endif		
	return 1;
}


/**
 * Resize callback handler.  Tell VTK that our window size has changed.
 */

long TnFXVTKCanvas::onResize(FXObject *obj, FXSelector sel, void *data)
{
#ifdef HAVE_VTK
	_fxrwi->UpdateSize(getWidth(), getHeight());
#endif	
	return 1;
}


/**
 * Timer callback handler.  Tell VTK that a timer has fired.
 */

long TnFXVTKCanvas::onTimeout(FXObject *obj, FXSelector sel, void *data)
{
#ifdef HAVE_VTK
	_fxrwi->InvokeEvent(vtkCommand::TimerEvent,NULL);
#endif	
	return 1;
}


/**
 * Left mouse button pressed callback handler.  Fires the appropriate
 * VTK event.
 */

long TnFXVTKCanvas::onLeftButtonDown(FXObject *obj, FXSelector sel, void *data)
{
#ifdef HAVE_VTK
	FXEvent *event = (FXEvent*)data;
	
	grab();
	
	_fxrwi->SetEventInformationFlipY(event->win_x, event->win_y);
	_fxrwi->InvokeEvent(vtkCommand::LeftButtonPressEvent, NULL);
	
	setFocus();
#endif	
	return 1;
}


/**
 * Left mouse button released callback handler.  Fires the appropriate
 * VTK event.
 */

long TnFXVTKCanvas::onLeftButtonUp(FXObject *obj, FXSelector sel, void *data)
{
#ifdef HAVE_VTK
	FXEvent *event = (FXEvent*)data;
	
	_fxrwi->SetEventInformationFlipY(event->win_x, event->win_y);
	_fxrwi->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, NULL);
	
	ungrab();
#endif	
	return 1;
}


/**
 * Middle mouse button pressed callback handler.  Fires the appropriate
 * VTK event.
 */

long TnFXVTKCanvas::onMiddleButtonDown(FXObject *obj, FXSelector sel, void *data)
{
#ifdef HAVE_VTK
	FXEvent *event = (FXEvent*)data;
	
	grab();
	
	_fxrwi->SetEventInformationFlipY(event->win_x, event->win_y);
	_fxrwi->InvokeEvent(vtkCommand::MiddleButtonPressEvent, NULL);
	
	setFocus();
#endif	
	return 1;
}


/**
 * Middle mouse button released callback handler.  Fires the appropriate
 * VTK event.
 */

long TnFXVTKCanvas::onMiddleButtonUp(FXObject *obj, FXSelector sel, void *data)
{
#ifdef HAVE_VTK
	FXEvent *event = (FXEvent*)data;
	
	_fxrwi->SetEventInformationFlipY(event->win_x, event->win_y);
	_fxrwi->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, NULL);
	
	ungrab();
#endif	
	return 1;
}


/**
 * Right mouse button pressed callback handler.  Fires the appropriate
 * VTK event.
 */

long TnFXVTKCanvas::onRightButtonDown(FXObject *obj, FXSelector sel, void *data)
{
#ifdef HAVE_VTK
	FXEvent *event = (FXEvent*)data;
	
	grab();
	
	_fxrwi->SetEventInformationFlipY(event->win_x, event->win_y);
	_fxrwi->InvokeEvent(vtkCommand::RightButtonPressEvent, NULL);
	
	setFocus();
#endif	
	return 1;
}


/**
 * Right mouse button released callback handler.  Fires the appropriate
 * VTK event.
 */

long TnFXVTKCanvas::onRightButtonUp(FXObject *obj, FXSelector sel, void *data)
{
#ifdef HAVE_VTK
	FXEvent *event = (FXEvent*)data;
	
	_fxrwi->SetEventInformationFlipY(event->win_x, event->win_y);
	_fxrwi->InvokeEvent(vtkCommand::RightButtonReleaseEvent, NULL);
	
	ungrab();
#endif
	return 1;
}


/**
 * Mouse motion callback handler.  Fires the appropriate
 * VTK event.
 */

long TnFXVTKCanvas::onMotion(FXObject *obj, FXSelector sel, void *data)
{
#ifdef HAVE_VTK
	FXEvent *event = (FXEvent*)data;
	
	_fxrwi->SetEventInformationFlipY(event->win_x, event->win_y);
	_fxrwi->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
#endif
	return 1;
}


/**
 * Key press callback handler.  Fires the appropriate
 * VTK event.
 */

long TnFXVTKCanvas::onKeyboard(FXObject *obj, FXSelector sel, void *data)
{
#ifdef HAVE_VTK
	FXEvent *event = (FXEvent*)data;
	
	//fxmessage("keypress: %s %d %d\n", event->text.text(), event->state, event->code);
	
	if (event->text != FXString::null)
	{
		_fxrwi->SetEventInformationFlipY(event->win_x, event->win_y, event->state & CONTROLMASK, event->state & SHIFTMASK, event->code, 1);
		_fxrwi->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
		_fxrwi->InvokeEvent(vtkCommand::CharEvent, NULL);
	}
#endif
	return 1;
}


//************************************************************************************************

#ifdef HAVE_VTK

vtkTnFXRenderWindowInteractor::vtkTnFXRenderWindowInteractor(TnFXVTKCanvas *canvas)
		: vtkRenderWindowInteractor()
		, _canvas(canvas)
{}


vtkTnFXRenderWindowInteractor::~vtkTnFXRenderWindowInteractor()
{}


TnFXVTKCanvas *vtkTnFXRenderWindowInteractor::canvas() const throw()
{
	return _canvas;
}

void vtkTnFXRenderWindowInteractor::setCanvas(TnFXVTKCanvas *canvas) throw()
{
	if (canvas != NULL)
		_canvas = canvas;
}


void vtkTnFXRenderWindowInteractor::Initialize()
{
	if (!RenderWindow)
	{
		vtkErrorMacro( << "vtkTnFXRenderWindowInteractor::Initialize has no render window");
	}
	
	else
	{
		int *size = RenderWindow->GetSize();
		Enable();
		
		Size[0] = size[0];
		Size[1] = size[1];
		
		Initialized = 1;
	}
}


void vtkTnFXRenderWindowInteractor::Enable()
{
	if (!Enabled)
	{
		Enabled = 1;
		
		Modified();
	}
}


void vtkTnFXRenderWindowInteractor::Disable()
{
	if (Enabled)
	{
		Enabled = 0;
		
		Modified();
	}
}


void vtkTnFXRenderWindowInteractor::Start()
{
	vtkErrorMacro( << "vtkTnFXRenderWindowInteractor::Start() interactor cannot control event loop.");
}


void vtkTnFXRenderWindowInteractor::SetRenderWindow(vtkRenderWindow *renderer)
{
	vtkRenderWindowInteractor::SetRenderWindow(renderer);
	
	if (RenderWindow)
	{
		RenderWindow->SetSize(_canvas->getWidth(), _canvas->getHeight());
	}
}


void vtkTnFXRenderWindowInteractor::UpdateSize(int w, int h)
{
	if (RenderWindow != NULL)
	{
		if ((w != Size[0]) || (h != Size[1]))
		{
			Size[0] = w;
			Size[1] = h;
			
			RenderWindow->SetSize(w, h);
			
			int *pos = RenderWindow->GetPosition();
			
			if ((pos[0] != _canvas->getX()) || (pos[1] != _canvas->getY()))
			{
				RenderWindow->SetPosition(_canvas->getX(), _canvas->getY());
			}
		}
	}
}


void vtkTnFXRenderWindowInteractor::TerminateApp()
{}


int vtkTnFXRenderWindowInteractor::CreateTimer(int timertype)
{
	_canvas->getApp()->addTimeout(_canvas, 0, 10);
	return 1;
}


int vtkTnFXRenderWindowInteractor::DestroyTimer(void)
{
	_canvas->getApp()->removeTimeout(_canvas, 0);
	return 1;
}

#endif

}

#endif
