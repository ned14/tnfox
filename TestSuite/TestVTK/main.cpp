/********************************************************************************
*                                                                               *
*                              VTK Graphing test                                *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2006 by Niall Douglas.   All Rights Reserved.            *
*   NOTE THAT I NIALL DOUGLAS DO NOT PERMIT ANY OF MY CODE USED UNDER THE GPL   *
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


#include "fx.h"
#include "fx3d.h"
#include <vtkInteractorStyleJoystickCamera.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkPolyDataMapper.h>
#include <vtkSphereSource.h>
#include <vtkEarthSource.h>
#include <vtkProperty.h>
#include <vtkCamera.h>
#include <vtkLight.h>


class Window : public FXMainWindow
{
	FXDECLARE(Window)
	FXAutoPtr<FXGLVisual> vis;
protected:
	Window() { }
public:
	Window(FXApp *app) : FXMainWindow(app, "TnFOX VTK Test", NULL, NULL, DECOR_ALL, 50, 50, 800, 600),
		vis(new FXGLVisual(app, VISUAL_DOUBLEBUFFER))
	{
		FXHorizontalFrame *frame = new FXHorizontalFrame(this, LAYOUT_FILL_X | LAYOUT_FILL_Y);
		FXPacker *canvasFrame = new FXPacker(frame, FRAME_THICK | FRAME_SUNKEN | LAYOUT_FILL_X | LAYOUT_FILL_Y, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		TnFXVTKCanvas *canvas = new TnFXVTKCanvas(canvasFrame, PtrPtr(vis), (FXObject *) 0, 0, LAYOUT_FILL_X | LAYOUT_FILL_Y);
		create_pipeline(canvas->interactor());
	}
	void create_pipeline(vtkTnFXRenderWindowInteractor *rwi)
	{
		TnFXVTKHold<vtkRenderer> ren = vtkRenderer::New();
		ren->SetBackground(0, 0, 0);
		
		TnFXVTKHold<vtkRenderWindow> renWindow = vtkRenderWindow::New();
		renWindow->AddRenderer(PtrPtr(ren));
		
		TnFXVTKHold<vtkInteractorStyleJoystickCamera> style = vtkInteractorStyleJoystickCamera::New();
		rwi->SetInteractorStyle(PtrPtr(style));
			
		rwi->SetRenderWindow(PtrPtr(renWindow));
		rwi->Initialize();

		TnFXVTKHold<vtkSphereSource> sphereSource = vtkSphereSource::New();
		sphereSource->SetThetaResolution(100);
		sphereSource->SetPhiResolution(50);
		sphereSource->SetRadius(1);
		TnFXVTKHold<vtkPolyDataMapper> sphereMapper = vtkPolyDataMapper::New();
		sphereMapper->SetInput(sphereSource->GetOutput());
		TnFXVTKHold<vtkActor> sphereActor = vtkActor::New();
		sphereActor->SetMapper(PtrPtr(sphereMapper));
		sphereActor->GetProperty()->SetColor(0,0,1);
		sphereActor->GetProperty()->SetAmbient(0.1);
		sphereActor->GetProperty()->SetDiffuse(0.5);
		sphereActor->GetProperty()->SetSpecular(1.0);
		sphereActor->GetProperty()->SetSpecularPower(50.0);
		ren->AddActor(PtrPtr(sphereActor));

		TnFXVTKHold<vtkEarthSource> earthSource = vtkEarthSource::New();
		earthSource->SetOnRatio(1);
		earthSource->SetRadius(1);
		TnFXVTKHold<vtkPolyDataMapper> earthMapper = vtkPolyDataMapper::New();
		earthMapper->SetInput(earthSource->GetOutput());
		TnFXVTKHold<vtkActor> earthActor = vtkActor::New();
		earthActor->SetMapper(PtrPtr(earthMapper));
		ren->AddActor(PtrPtr(earthActor));

		TnFXVTKHold<vtkLight> light = vtkLight::New();
		light->SetFocalPoint(1.875,0.6125,0);
		light->SetPosition(0.875,1.6125,1);
		ren->AddLight(PtrPtr(light));

		ren->GetActiveCamera()->SetFocalPoint(0,0,0);
		ren->GetActiveCamera()->SetPosition(0,0,1);
		ren->GetActiveCamera()->SetViewUp(0,1,0);
		ren->GetActiveCamera()->ParallelProjectionOn();
		ren->ResetCamera();
		ren->GetActiveCamera()->SetParallelScale(1.5);
	}
};


FXIMPLEMENT(Window, FXMainWindow, NULL, 0)


int main( int argc, char *argv[] )
{
	FXProcess myprocess(argc, argv);
	FXApp app("vtktest", "vtkFOX");
	app.init(argc, argv);
	
	Window *win = new Window(&app);
	
	app.create();
	win->show(PLACEMENT_SCREEN);
	
	return app.run();
}
