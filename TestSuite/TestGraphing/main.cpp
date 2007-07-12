/********************************************************************************
*                                                                               *
*                                 Graphing test                                 *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2006-2007 by Niall Douglas.   All Rights Reserved.       *
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

class Window : public FXMainWindow
{
	FXDECLARE(Window)
	double h;			// integration step size for all fractals
	float scale;		// Scaling factor
	FXVec3d & (Window::*iterator)(FXVec3d &vec) throw();


	// Lorentz
	double Pr;			// Prandtl number
	double r;			// Rayleigh number
	double b;

	// Pendulum
	double omega0;		// Natural angular frequency of undamped oscillator
	double Q;
	double a0;			// External forcing F0/m
	double omega, T0, gamma, T_driving, x, v, t;

	FXAutoPtr<FXGLVisual> vis1, vis2, vis3;
	FXHorizontalFrame *canvas3dFrames;
	FXPacker *canvasFrame1, *canvasFrame2, *canvasFrame3;
	FXGLViewer *graphviewer1, *graphviewer2, *graphviewer3;
	FXRadioButton *lorentzButton, *pendulumButton, *darkerButton, *brighterButton, *rainbowButton;
	FXVec3d state;			// Lorentz state
	QMemArray<FXVec3f> points;
	TnFX3DGraph graph1, graph2;
	TnFX2DGraph graph3;

	FXuint maxPoints, addPoints;
protected:
	Window() { }
public:
	enum
	{
		ID_SHOWVIEWER1=FXMainWindow::ID_LAST,
		ID_SHOWVIEWER2,
		ID_SHOWVIEWER3,
		ID_ANTIALIAS,
		ID_TURBO,
		ID_COPYCAMERA1,
		ID_COPYCAMERA2,
		ID_LORENTZ,
		ID_PENDULUM,
		ID_DARKERGEN,
		ID_BRIGHTERGEN,
		ID_RAINBOWGEN,
		ID_SAVEIMAGE1,
		ID_SAVEIMAGE2,
		ID_SAVEIMAGE3,
		ID_LAST
	};
	Window(FXApp *app) : FXMainWindow(app, "TnFOX Graphing Test", NULL, NULL, DECOR_ALL, 50, 50, 800, 600),
		vis1(new FXGLVisual(app, VISUAL_DOUBLEBUFFER)), vis2(new FXGLVisual(app, VISUAL_DOUBLEBUFFER)),
		vis3(new FXGLVisual(app, VISUAL_DOUBLEBUFFER)), state(0.1, 0, 0),
		maxPoints(20000), addPoints(0)
	{
		h=0.005;
		scale=4;
		iterator=&Window::iterateLorentz;
		// Lorentz
		//Pr=10.0;
		//r=28.0;
		//b=8.0 / 3.0;
		Pr=28.0;
		r=46.92;
		b=4.0;
		// Pendulum
		omega0=1.0;
		Q=2.0;
		a0=1.15; // 1.15, 1.5

		omega=omega0*0.67;
		T0=2*PI/omega0;
		gamma=omega0/Q;
		T_driving=2*PI/omega;
		x=v=t=0;

		FXHorizontalFrame *frame = new FXHorizontalFrame(this, LAYOUT_FILL_X | LAYOUT_FILL_Y);
		FXVerticalFrame *canvasFrames = new FXVerticalFrame(frame, LAYOUT_FILL);
		canvas3dFrames = new FXHorizontalFrame(canvasFrames, LAYOUT_FILL, 0,0,0,0, 0,0,0,0);
		canvasFrame1 = new FXPacker(canvas3dFrames, FRAME_THICK | FRAME_SUNKEN | LAYOUT_FILL | LAYOUT_SIDE_LEFT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		graphviewer1 = new FXGLViewer(canvasFrame1, PtrPtr(vis1), (FXObject *) 0, 0, LAYOUT_FILL_X | LAYOUT_FILL_Y | VIEWER_FOG);
		graphviewer1->setBackgroundColor(FXGLColor(0,0,0));
		canvasFrame2 = new FXPacker(canvas3dFrames, FRAME_THICK | FRAME_SUNKEN | LAYOUT_FILL | LAYOUT_SIDE_RIGHT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		graphviewer2 = new FXGLViewer(canvasFrame2, PtrPtr(vis2), (FXObject *) 0, 0, LAYOUT_FILL_X | LAYOUT_FILL_Y);
		graphviewer2->setBackgroundColor(FXGLColor(0,0,0));
		canvasFrame3 = new FXPacker(canvasFrames, FRAME_THICK | FRAME_SUNKEN | LAYOUT_FILL | LAYOUT_SIDE_BOTTOM, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		graphviewer3 = new FXGLViewer(canvasFrame3, PtrPtr(vis3), (FXObject *) 0, 0, LAYOUT_FILL);
		graphviewer3->setBackgroundColor(FXGLColor(1,1,1));
		graphviewer3->setProjection(FXGLViewer::PARALLEL);

		FXVerticalFrame *controls = new FXVerticalFrame(frame, userHandednessLayout()|LAYOUT_FILL_Y);
		(new FXCheckButton(controls, "Fancy 3d Viewer", this, ID_SHOWVIEWER1));
		(new FXCheckButton(controls, "Quick 3d Viewer", this, ID_SHOWVIEWER2))->setCheck();
		(new FXCheckButton(controls, "2d Viewer", this, ID_SHOWVIEWER3))->setCheck();
		canvasFrame1->hide();
		(new FXCheckButton(controls, "Antialiasing", this, ID_ANTIALIAS));
		(new FXCheckButton(controls, "Use Turbo", this, ID_TURBO));
		FXHorizontalFrame *maxPointsFrame = new FXHorizontalFrame(controls);
		new FXLabel(maxPointsFrame, "Maximum Points:");
		new FXSpinner(maxPointsFrame, 6, new FXDataTarget(maxPoints), FXDataTarget::ID_VALUE, SPIN_NOMAX|FRAME_THICK|FRAME_SUNKEN);
		FXHorizontalFrame *addPointsFrame = new FXHorizontalFrame(controls);
		new FXLabel(addPointsFrame, "Add Points:");
		new FXSpinner(addPointsFrame, 4, new FXDataTarget(addPoints), FXDataTarget::ID_VALUE, SPIN_NOMAX|FRAME_THICK|FRAME_SUNKEN);
		new FXButton(controls, "Copy Camera =>", NULL, this, ID_COPYCAMERA1);
		new FXButton(controls, "Copy Camera <=", NULL, this, ID_COPYCAMERA2);
		FXGroupBox *fractalBox = new FXGroupBox(controls, "Fractal:", FRAME_RIDGE|LAYOUT_FILL_X);
		lorentzButton = new FXRadioButton(fractalBox, "Lorentz",   this, ID_LORENTZ);
		pendulumButton = new FXRadioButton(fractalBox, "Pendulum", this, ID_PENDULUM);
		lorentzButton->setCheck();
		FXGroupBox *colourBox = new FXGroupBox(controls, "Colour Generator:", FRAME_RIDGE|LAYOUT_FILL_X);
		darkerButton = new FXRadioButton(colourBox, "Darker",   this, ID_DARKERGEN);
		brighterButton = new FXRadioButton(colourBox, "Brighter", this, ID_BRIGHTERGEN);
		rainbowButton = new FXRadioButton(colourBox, "Rainbow",  this, ID_RAINBOWGEN);
		rainbowButton->setCheck();
		new FXButton(controls, "Save Image 1", NULL, this, ID_SAVEIMAGE1);
		new FXButton(controls, "Save Image 2", NULL, this, ID_SAVEIMAGE2);
		new FXButton(controls, "Save Image 3", NULL, this, ID_SAVEIMAGE3);

		genPoints(2000);
		
		FXGLVertices *vertices1=graph1.setItemData(0, &points);
		vertices1->setColorGenerator(FXGLVertices::RainbowGenerator);
		vertices1->setColor(FXGLColor(0.2f, 1.0f, 0.2f));

		FXGLVertices *vertices2=graph2.setItemData(0, &points);
		vertices2->setColorGenerator(FXGLVertices::RainbowGenerator);
		vertices2->setColor(FXGLColor(0.2f, 1.0f, 0.2f));

		FXGLVertices *vertices3a=graph3.setItemData(0, &points, &TnFXGraph::ReduceE2y<1, 0, 3>);  // Plot X
		graph3.setItemDetails(0, "X", FXGLColor(1.0f, 0.0f, 0.0f), 2.0f, 0.0f);
		FXGLVertices *vertices3b=graph3.setItemData(1, &points, &TnFXGraph::ReduceE2y<1, 1, 3>);  // Plot Y
		graph3.setItemDetails(1, "Y", FXGLColor(0.0f, 1.0f, 0.0f), 2.0f, 0.0f);
		FXGLVertices *vertices3c=graph3.setItemData(2, &points, &TnFXGraph::ReduceE2y<1, 2, 3>);  // Plot Z
		graph3.setItemDetails(2, "Z", FXGLColor(0.0f, 0.0f, 1.0f), 2.0f, 0.0f);
		graph3.setAxes();
		graph3.setAxesMajor();

		// Want a nice shiny light on viewer 1
		FXLight light;
		graphviewer1->getLight(light);
		light.specular=FXGLColor(1.0f,1.0f,1.0f,1.0f);
		graphviewer1->setLight(light);
		vertices1->setOptions(vertices1->getOptions()|STYLE_SURFACE|SHADING_SMOOTH);
		FXMaterial material;
		material.ambient=FXGLColor(0.0f, 0.0f, 0.0f, 1.0f);
		material.diffuse=FXGLColor(0.0f, 0.0f, 0.0f, 1.0f);
		material.specular=FXGLColor(1.0f, 1.0f, 1.0f, 1.0f);
		material.emission=FXGLColor(0.0f, 0.0f, 0.0f, 1.0f);
		material.shininess=128;
		vertices1->setMaterial(0, material);
		vertices1->setMaterial(1, material);

		graphviewer1->setScene(&graph1);
		graphviewer2->setScene(&graph2);
		graphviewer3->setScene(&graph3);
		graphviewer3->setZoom(1.8);
		getApp()->addTimeout(this, 0, 100);
	}
	FXVec3d &iterateLorentz(FXVec3d &vec) throw()
	{
		double &x0=vec[0], &y0=vec[1], &z0=vec[2], x1, y1, z1;
		x1 = x0 + h * Pr * (y0 - x0);
		y1 = y0 + h * (x0 * (r - z0) - y0);
		z1 = z0 + h * (x0 * y0 - b * z0);
		x0 = x1;
		y0 = y1;
		z0 = z1;
		return vec;
	}
	inline double accel(double x, double v, double t) throw()
	{
		return -omega0*omega0*sin(x)-gamma*v+a0*cos(omega*t);
	}
	double rk4() throw()
	{
		double xk1, vk1, xk2, vk2, xk3, vk3, xk4, vk4;
		xk1=h*v;
		vk1=h*accel(x, v, t);
		xk2=h*(v+vk1/2.0);
		vk2=h*accel(x+xk1/2.0, v+vk1/2.0, t+h/2);
		xk3=h*(v+vk2/2.0);
		vk3=h*accel(x+xk2/2.0, v+vk2/2.0, t+h/2);
		xk4=h*(v+vk3);
		vk4=h*accel(x+xk3, v+vk3, t+h);
		x+=(xk1+2.0*xk2+2.0*xk3+xk4)/6.0;
		v+=(vk1+2.0*vk2+2.0*vk3+vk4)/6.0;
		if(x<-PI) x+=2*PI;
		if(x>PI) x-=2*PI;
		t+=h;
		return x;
	}
	FXVec3d &iteratePendulum(FXVec3d &vec) throw()
	{
		static FXVec3d vecM1(0, 0, 0);
		//vec[2]=vecM1[1];
		//vec[1]=vecM1[0];
		//vec[0]=rk4()-vec[1];
		//vecM1=vec;
		vec[0]=rk4();
		vec[1]=v;
		vec[2]=0;
		//fxmessage("%f, %f\n", x, v);
		return vec;
	}
	void genPoints(FXuint no)
	{
		for(FXuint n=0; n<no; n++)
		{
			((*this).*iterator)(state);
			points.append(FXVec3f((FXfloat) state.x*scale, (FXfloat) state.y*scale, (FXfloat) state.z*scale));
		}
		if(points.count()>maxPoints)
		{
			memmove(points.data(), points.data()+(points.count()-maxPoints), maxPoints*sizeof(FXVec3f));
			points.resize(maxPoints);
		}
		graph1.itemChanged(0);
		graph2.itemChanged(0);
		graph3.itemChanged(0); graph3.itemChanged(1); graph3.itemChanged(2);
		if(canvasFrame1->shown()) graphviewer1->update();
		if(canvasFrame2->shown()) graphviewer2->update();
		if(canvasFrame3->shown()) graphviewer3->update();
	}
	long addPoint(FXObject* sender,FXSelector,void*)
	{
		if(addPoints)
		{
			genPoints(addPoints);
		}
		getApp()->addTimeout(this, 0, 100);
		if(FXProcess::isAutomatedTest())
			close();
		return 1;
	}
	long showViewer(FXObject* sender,FXSelector sel,void*)
	{
		FXPacker *frame=(FXSELID(sel)==ID_SHOWVIEWER1) ? canvasFrame1 : (FXSELID(sel)==ID_SHOWVIEWER2) ? canvasFrame2 : canvasFrame3;
		if(FXSELID(sel)==ID_SHOWVIEWER1 || FXSELID(sel)==ID_SHOWVIEWER2)
			copyCamera(sender, (FXSELID(sel)==ID_SHOWVIEWER1) ? ID_COPYCAMERA2 : ID_COPYCAMERA1, 0);
		if(frame->shown())
			frame->hide();
		else
			frame->show();
		if(canvasFrame1->shown() || canvasFrame2->shown()) canvas3dFrames->show(); else canvas3dFrames->hide();
		frame->recalc();
		return 1;
	}
	long antiAlias(FXObject* sender,FXSelector,void*)
	{
		graphviewer1->setOptions(graphviewer1->getOptions() ^ VIEWER_ANTIALIAS);
		graphviewer1->update();
		graphviewer2->setOptions(graphviewer2->getOptions() ^ VIEWER_ANTIALIAS);
		graphviewer2->update();
		graphviewer3->setOptions(graphviewer3->getOptions() ^ VIEWER_ANTIALIAS);
		graphviewer3->update();
		return 1;
	}
	long turbo(FXObject* sender,FXSelector,void*)
	{
		graphviewer1->setTurboMode(!graphviewer1->getTurboMode());
		graphviewer2->setTurboMode(!graphviewer2->getTurboMode());
		return 1;
	}
	long fractal(FXObject* sender,FXSelector sel,void *ptr)
	{
		if(ptr)
		{
			FXRangef r(-1.0f,1.0f,-1.0f,1.0f,-1.0f,1.0f);
			points.resize(0);
			if(FXSELID(sel)==ID_LORENTZ)
			{
				iterator=&Window::iterateLorentz;
				graph1.setItemDetails(0, "", FXGLColor(0,0,0,0), 4, 2);
				graph2.setItemDetails(0, "", FXGLColor(0,0,0,0), 4, 2);
				state=FXVec3d(0.1, 0, 0);
				h=0.005;
				scale=4;
				genPoints(2000);
				graphviewer1->getScene()->bounds(r);
				graphviewer1->setBounds(r);
				graphviewer2->setBounds(r);
			}
			else
			{
				iterator=&Window::iteratePendulum;
				graph1.setItemDetails(0, "", FXGLColor(0,0,0,0), 1, 0);
				graph2.setItemDetails(0, "", FXGLColor(0,0,0,0), 1, 0);
				h=0.1;
				scale=10;
				genPoints(2000);
				graphviewer1->getScene()->bounds(r);
				graphviewer1->setBounds(r);
				graphviewer2->setBounds(r);
			}
			if(FXSELID(sel)!=ID_LORENTZ) lorentzButton->setCheck(false);
			if(FXSELID(sel)!=ID_PENDULUM) pendulumButton->setCheck(false);
		}
		return 1;
	}
	long colourGen(FXObject* sender,FXSelector sel,void *ptr)
	{
		if(ptr)
		{
			static const FXGLVertices::ColorGeneratorFunc funcs[]={
				FXGLVertices::DarkerGenerator,
				FXGLVertices::BrighterGenerator,
				FXGLVertices::RainbowGenerator
			};
			FXGLVertices *vertices1=static_cast<FXGLVertices *>(graph1.child(0));
			FXGLVertices *vertices2=static_cast<FXGLVertices *>(graph2.child(0));
			vertices1->setColorGenerator(funcs[FXSELID(sel)-ID_DARKERGEN]);
			vertices2->setColorGenerator(funcs[FXSELID(sel)-ID_DARKERGEN]);
			graphviewer1->update();
			graphviewer2->update();
			if(FXSELID(sel)!=ID_DARKERGEN) darkerButton->setCheck(false);
			if(FXSELID(sel)!=ID_BRIGHTERGEN) brighterButton->setCheck(false);
			if(FXSELID(sel)!=ID_RAINBOWGEN) rainbowButton->setCheck(false);
		}
		return 1;
	}
	long copyCamera(FXObject* sender,FXSelector sel,void*)
	{
		FXGLViewer *from=(FXSELID(sel)==ID_COPYCAMERA1) ? graphviewer1 : graphviewer2;
		FXGLViewer *to=(FXSELID(sel)==ID_COPYCAMERA1) ? graphviewer2 : graphviewer1;
		to->setZoom(from->getZoom());
		to->setDistance(from->getDistance());
		to->setOrientation(from->getOrientation());
		to->setCenter(from->getCenter());
		return 1;
	}
	long saveImage(FXObject* sender,FXSelector sel,void*)
	{
		FXGLViewer *from=(FXSELID(sel)==ID_SAVEIMAGE1) ? graphviewer1 : (FXSELID(sel)==ID_SAVEIMAGE2) ? graphviewer2 : graphviewer3;
		FXFileDialog dialog(getApp(), "Save Image to:");
		dialog.setFilename("Image.png");
		dialog.setPattern("*.png");
		FXERRH_TRY
		{
			if(dialog.execute())
			{
				QFile fh(dialog.getFilename());
				// Need to repaint the screen fully first
				getApp()->runWhileEvents();
				fh.open(IO_WriteOnly);
				FXStream s(&fh);
				FXColor *imagedata;
				from->readPixels(imagedata, 0, 0, from->getWidth(), from->getHeight());
				FXPNGImage pngimage(getApp(), NULL, 0, from->getWidth(), from->getHeight());
				pngimage.setData(imagedata);
				pngimage.savePixels(s);
				FXFREE(&imagedata);
			}
		}
		FXERRH_CATCH(FXException &e)
		{
			FXERRH_REPORT(this, e);
		}
		FXERRH_ENDTRY
		return 1;
	}
};


FXDEFMAP(Window) WindowMap[]={
	FXMAPFUNC(SEL_TIMEOUT,0,Window::addPoint),
	FXMAPFUNC(SEL_COMMAND,Window::ID_SHOWVIEWER1, Window::showViewer),
	FXMAPFUNC(SEL_COMMAND,Window::ID_SHOWVIEWER2, Window::showViewer),
	FXMAPFUNC(SEL_COMMAND,Window::ID_SHOWVIEWER3, Window::showViewer),
	FXMAPFUNC(SEL_COMMAND,Window::ID_ANTIALIAS,   Window::antiAlias),
	FXMAPFUNC(SEL_COMMAND,Window::ID_TURBO,       Window::turbo),
	FXMAPFUNCS(SEL_COMMAND,Window::ID_LORENTZ,	  Window::ID_PENDULUM,Window::fractal),
	FXMAPFUNCS(SEL_COMMAND,Window::ID_DARKERGEN,  Window::ID_RAINBOWGEN,Window::colourGen),
	FXMAPFUNC(SEL_COMMAND,Window::ID_COPYCAMERA1, Window::copyCamera),
	FXMAPFUNC(SEL_COMMAND,Window::ID_COPYCAMERA2, Window::copyCamera),
	FXMAPFUNC(SEL_COMMAND,Window::ID_SAVEIMAGE1,  Window::saveImage),
	FXMAPFUNC(SEL_COMMAND,Window::ID_SAVEIMAGE2,  Window::saveImage),
	FXMAPFUNC(SEL_COMMAND,Window::ID_SAVEIMAGE3,  Window::saveImage),
};

FXIMPLEMENT(Window, FXMainWindow, WindowMap, ARRAYNUMBER(WindowMap))


int main( int argc, char *argv[] )
{
	FXProcess myprocess(argc, argv);
	FXApp app("graphingtest", "TnFOX");
	app.init(argc, argv);
	
	Window *win = new Window(&app);
	
	app.create();
	win->show(PLACEMENT_SCREEN);
	
	return app.run();
}
