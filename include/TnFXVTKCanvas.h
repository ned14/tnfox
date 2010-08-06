/* This file was taken from http://sourceforge.net/projects/vtkfox
Resides in the public domain
*/
#if FX_GRAPHINGMODULE

#ifndef TNFXVTKCANVAS_H
#define TNFXVTKCANVAS_H

#include "FXGLCanvas.h"
#include "FXGenericTools.h"
#if HAVE_VTK
#include <vtkRenderWindowInteractor.h>
#endif

namespace FX {

/*! \file TnFXVTKCanvas.h
\brief Defines classes used to work with the Visualisation Toolkit (VTK)
*/

/*! \defgroup graphing Visualisation Toolkit (VTK) Support
*/

class vtkTnFXRenderWindowInteractor;

/*! \class TnFXVTKHold
\ingroup graphing
\brief Destructive smart pointer holding a vtkObject

This class simply ensures that the vtkObject is correctly destroyed on destruction.
*/
template<class type> class TnFXVTKHold : public Generic::ptr<type, Pol::destructiveCopyNoDelete>
{
public:
	TnFXVTKHold(type *p=0) : Generic::ptr<type, Pol::destructiveCopyNoDelete>(p) { }
	~TnFXVTKHold()
	{
		if(PtrPtr(*this)) PtrRelease(*this)->Delete();
	}
	TnFXVTKHold &operator=(const TnFXVTKHold &o)
	{
		if(PtrPtr(*this)) PtrRelease(*this)->Delete();
		Generic::ptr<type, Pol::destructiveCopyNoDelete>::data=o.data;
		o.data=0;
		return *this;
	}
};

/*! \class TnFXVTKCanvas
\ingroup graphing
\brief TnFOX to VTK interface

This class is intended to be a drop-in replacement for FX::FXGLCanvas, with
the exception that it allows for VTK rendering.  This class holds an
instance of FX::vtkTnFXRenderWindowInteractor, mapping all appropriate functions
and events, to allow for proper VTK use.

Author: Doug Henry (brilligent@gmail.com)
*/
class FXGRAPHINGMODULEAPI TnFXVTKCanvas : public FXGLCanvas
{
	FXDECLARE(TnFXVTKCanvas)

	vtkTnFXRenderWindowInteractor *_fxrwi;
	void *_id;
	void *_display;
protected:
	TnFXVTKCanvas() {}
	
	// not implemented (should cause compile error if object is copied)
	TnFXVTKCanvas(const TnFXVTKCanvas& FVC);
	TnFXVTKCanvas& operator=(const TnFXVTKCanvas& FVC);
public:
	//! Constructs an instance
	TnFXVTKCanvas(FXComposite *p, FXGLVisual *vis, FXObject *tgt = NULL, FXSelector sel = 0, FXuint opts = 0, FXint x = 0, FXint y = 0, FXint w = 0, FXint h = 0);
	TnFXVTKCanvas(FXComposite *p, FXGLVisual *vis, FXGLCanvas *sharegroup, FXObject *tgt=NULL, FXSelector sel=0, FXuint opts=0, FXint x=0, FXint y=0, FXint w=0, FXint h=0);
	~TnFXVTKCanvas();

	//! Returns the interactor
	vtkTnFXRenderWindowInteractor* interactor() const throw();
	//! Sets the interactor
	void setInteractor(vtkTnFXRenderWindowInteractor *fxrwi) throw();
	
	/*! Renders the VTK image onto the screen. If \em makeCurrent is true,
	sets and restores the OpenGL context around the call */
	virtual void render(bool makeCurrent=true);

	virtual void create();

public:
	long onPaint(FXObject *obj, FXSelector sel, void *data);
	long onTimeout(FXObject *obj, FXSelector sel, void *data);
	long onResize(FXObject *obj, FXSelector sel, void *data);
	
	long onLeftButtonDown(FXObject *obj, FXSelector sel, void *data);
	long onLeftButtonUp(FXObject *obj, FXSelector sel, void *data);
	long onMiddleButtonDown(FXObject *obj, FXSelector sel, void *data);
	long onMiddleButtonUp(FXObject *obj, FXSelector sel, void *data);
	long onRightButtonDown(FXObject *obj, FXSelector sel, void *data);
	long onRightButtonUp(FXObject *obj, FXSelector sel, void *data);
	long onMotion(FXObject *obj, FXSelector sel, void *data);
	long onKeyboard(FXObject *obj, FXSelector sel, void *data);
};

/*! \class vtkTnFXRenderWindowInteractor
\ingroup graphing
\brief VTK to TnFOX interface

This class is used to "close the loop" with VTK.  The FX::TnFXVTKCanvas widget
handles the fox side and translates events to VTK.  This class uses
the canvas to update its own information.

Normally you will never need to create one of these on your own as it is usually
done for you. However, you will probably use it to adjust the properties of its
base class vtkRenderWindowInteractor

Author: Doug Henry (brilligent@gmail.com)
*/
#ifdef HAVE_VTK

// Suppress warning about vtkRenderWindowInteractor not being a suitable base class
#ifdef _MSC_VER
#if _MSC_VER >= 1200
#pragma warning( push )
#endif
#pragma warning( disable : 4275 )
#endif

class FXGRAPHINGMODULEAPI vtkTnFXRenderWindowInteractor : public vtkRenderWindowInteractor
{
public:
	//! Constructs an instance
	vtkTnFXRenderWindowInteractor(TnFXVTKCanvas *canvas=0);
	~vtkTnFXRenderWindowInteractor();

	//! Returns the canvas in use
	TnFXVTKCanvas *canvas() const throw();
	//! Sets the canvas to use
	void setCanvas(TnFXVTKCanvas *canvas) throw();
	
	virtual void Initialize();
	virtual void Enable();
	virtual void Disable();
	virtual void Start();
	virtual void SetRenderWindow(vtkRenderWindow *renderer);
	virtual void UpdateSize(int w, int h);
	virtual void TerminateApp();
	virtual int CreateTimer(int timertype);
	virtual int DestroyTimer(void);

	
	virtual char const* GetClassNameW() const { return "vtkTnFXRenderWindowInteractor"; }
	
protected:
	// not implemented (should cause compile error)
	vtkTnFXRenderWindowInteractor(const vtkTnFXRenderWindowInteractor& RWI);
	vtkTnFXRenderWindowInteractor& operator=(const vtkTnFXRenderWindowInteractor& RWI);
	
private:
	TnFXVTKCanvas *_canvas;
};

// Restore previous warning levels
#ifdef _MSC_VER
#if _MSC_VER >= 1200
#pragma warning( pop )
#endif
#endif

#endif

}

#endif
#endif
