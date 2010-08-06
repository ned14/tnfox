/********************************************************************************
*                                                                               *
*                 O p e n G L   V e r t i c e s   O b j e c t                   *
*                                                                               *
*********************************************************************************
* Copyright (C) 2006 by Niall Douglas.   All Rights Reserved.                   *
*********************************************************************************
* This library is free software; you can redistribute it and/or                 *
* modify it under the terms of the GNU Lesser General Public                    *
* License as published by the Free Software Foundation; either                  *
* version 2.1 of the License, or (at your option) any later version.            *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU             *
* Lesser General Public License for more details.                               *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public              *
* License along with this library; if not, write to the Free Software           *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.    *
*********************************************************************************
* $Id: FXGLVertices.h                                                           *
********************************************************************************/
#ifndef FX_DISABLEGL

#if FX_GRAPHINGMODULE

#ifndef FXGLVERTICES_H
#define FXGLVERTICES_H

#ifndef FXGLSHAPE_H
#include "FXGLShape.h"
#endif

namespace FX {


// Vertices drawing options
enum {
  VERTICES_POINTS          = 0x00000100,     // Draw each point
  VERTICES_LINES           = 0x00000200,     // Draw as set of connected lines
  VERTICES_LOOPLINES       = 0x00000400,     // Connect start to end
  VERTICES_LINEITEMS       = 0x00000800,     // Draw as set of separate lines
  VERTICES_NODEPTHTEST     = 0x00001000      // Disable the depth test during rendition
  };


/**
* OpenGL Vertices Object. Plots a series of points optionally connected by lines.
* If STYLE_SURFACE is enabled, renders points as spheres and lines as cylinders
* thus allowing full shading and material use.
*/
class FXGRAPHINGMODULEAPI FXGLVertices : public FXGLShape {
  FXDECLARE(FXGLVertices)
public:
  typedef void (*ColorGeneratorFunc)(FXGLColor &color, FXGLVertices *obj, FXGLViewer *viewer, FXuint vertex, void *&data);

protected:
  FXuchar modified;
  FXuint displayLists;
  FXuint vertexNumber;
  FXVec3f *vertices;
  FXfloat pointSize, lineSize;
  FXGLColor color;
  ColorGeneratorFunc colorGenerator;
  void *colorGeneratorData;
protected:
  FXGLVertices();
private:
  friend class FXGLCircle;
  inline void renderPoints(FXGLViewer *viewer, bool isHit, bool complex);
  inline void renderLines(FXGLViewer *viewer, bool isHit, bool complex);
  inline void render(FXGLViewer *viewer, bool isHit, bool complex);
  virtual void drawshape(FXGLViewer*);
public:
  /// Construct with specified origin and options
  FXGLVertices(FXfloat x,FXfloat y,FXfloat z,FXuint opts=VERTICES_POINTS, FXVec3f *vertices=NULL, FXuint no=0, FXfloat pointSize=4.0f, FXfloat lineSize=1.0f);

  /// Copy constructor
  FXGLVertices(const FXGLVertices& orig);

  ~FXGLVertices();

  /// Sets if the representation of the vertices need recomputing
  void setModified(bool v=true) { modified=3*v; }

  /// Returns the number of vertices
  FXuint getNumberOfVertices() const { return vertexNumber; }

  /// Sets the number of vertices
  void setNumberOfVertices(FXuint vertices);

  /// Returns the vertices
  FXVec3f *getVertices() const { return vertices; }

  /// Sets the vertices
  void setVertices(FXVec3f *verts, FXuint no);

  /// Returns the point size
  FXfloat getPointSize() const { return pointSize; }

  /// Sets the point size
  void setPointSize(FXfloat ps);

  /// Returns the line size
  FXfloat getLineSize() const { return lineSize; }

  /// Sets the line size
  void setLineSize(FXfloat ps);

  /// Returns the color
  const FXGLColor &getColor() const { return color; }

  /// Sets the color
  void setColor(const FXGLColor &col);

  /// Sets a color generation function
  void setColorGenerator(ColorGeneratorFunc func, void *data=0);

  /// Called by the viewer to get bounds for this object
  virtual void bounds(FXRangef& box);

  /// Draw this object in a viewer
  virtual void draw(FXGLViewer* viewer);

  /// Draw this object for hit-testing purposes
  virtual void hit(FXGLViewer* viewer);

  /// Copy this object
  virtual FXGLObject* copy();

  /// Save shape to a stream
  virtual void save(FXStream& store) const;

  /// Load shape from a stream
  virtual void load(FXStream& store);

public:
  /// Rainbow color generator. Generates RGB sweep
  static void RainbowGenerator(FXGLColor &color, FXGLVertices *obj, FXGLViewer *viewer, FXuint vertex, void *&data);

  /// Brighter color generator. Goes from black up to getColor()
  static void BrighterGenerator(FXGLColor &color, FXGLVertices *obj, FXGLViewer *viewer, FXuint vertex, void *&data);

  /// Darker color generator. Goes from getColor() to black
  static void DarkerGenerator(FXGLColor &color, FXGLVertices *obj, FXGLViewer *viewer, FXuint vertex, void *&data);

  };


/**
* OpenGL Circle Object. Renders a set of vertices to draw a circle.
* Transforms a single set of statically created vertices for each
* instance, so rendition is very fast.
*/
class FXGRAPHINGMODULEAPI FXGLCircle : public FXGLShape {
	float radius;
	FXMat4f transformation;
protected:
	virtual void drawshape(FXGLViewer*);
public:
	FXGLCircle() { }
	/// Construct with specified origin and radius
	FXGLCircle(float x, float y, float z, float r, FXuint options=0);
	/// Get transformation
	const FXMat4f &getTransformation() const { return transformation; }
	/// Set transformation
	void setTransformation(const FXMat4f &t) { transformation=t; }

	virtual void bounds(FXRangef& box);
	virtual void draw(FXGLViewer* viewer);
	virtual FXGLObject* copy();
	virtual void save(FXStream& store) const;
	virtual void load(FXStream& store);
};

}

#endif
#endif
#endif
