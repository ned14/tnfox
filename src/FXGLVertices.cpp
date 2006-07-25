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

#include "xincs.h"
#include "FXObjectList.h"
#include "FXVec2f.h"
#include "FXVec3f.h"
#include "FXVec4f.h"
#include "FXRangef.h"
#include "FXGLViewer.h"
#include "FXGLVertices.h"
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif


namespace FX {


// Object implementation
FXIMPLEMENT(FXGLVertices,FXGLShape,NULL,0)


FXGLVertices::FXGLVertices(){
  modified=0;
  displayLists=0;
  vertexNumber=0;
  vertices=NULL;
  pointSize=4.0f;
  lineSize=1.0f;
  colorGenerator=NULL;
  colorGeneratorData=NULL;
  }

// Construct with specified origin and options
FXGLVertices::FXGLVertices(FXfloat x,FXfloat y,FXfloat z,FXuint opts,FXVec3f *vert,FXuint no,FXfloat ps,FXfloat ls)
: FXGLShape(x,y,z,opts) {
  modified=0;
  displayLists=0;
  vertexNumber=no;
  vertices=vert;
  pointSize=ps;
  lineSize=ls;
  colorGenerator=NULL;
  colorGeneratorData=NULL;
  }


// Copy constructor
FXGLVertices::FXGLVertices(const FXGLVertices& orig){
  modified=orig.modified;
  displayLists=0;
  vertexNumber=orig.vertexNumber;
  FXMEMDUP(&vertices,orig.vertices,FXVec3f,3*orig.vertexNumber);
  pointSize=orig.pointSize;
  lineSize=orig.lineSize;
  colorGenerator=orig.colorGenerator;
  colorGeneratorData=orig.colorGeneratorData;
  }


FXGLVertices::~FXGLVertices(){
#ifdef HAVE_GL_H
  if(displayLists){
    glDeleteLists(displayLists, 3);
    }
#endif
  }

// Sets the number of vertices
void FXGLVertices::setNumberOfVertices(FXuint vertices){
  vertexNumber=vertices;
  modified=3;
  }


// Sets the vertices
void FXGLVertices::setVertices(FXVec3f *verts, FXuint no){
  vertices=verts;
  vertexNumber=no;
  modified=3;
  }


// Sets the point size
void FXGLVertices::setPointSize(float ps){
  pointSize=ps;
  modified=3;
  }


// Sets the line size
void FXGLVertices::setLineSize(float ps){
  lineSize=ps;
  modified=3;
  }


// Sets the color
void FXGLVertices::setColor(const FXGLColor &col){
  color=col;
  modified=3;
  }


// Sets a color generation function
void FXGLVertices::setColorGenerator(ColorGeneratorFunc func, void *data){
  colorGenerator=func;
  colorGeneratorData=data;
  modified=3;
  }


// Called by the viewer to get bounds for this object
void FXGLVertices::bounds(FXRangef& box){
  if(vertexNumber){
    range.lower.x=range.upper.x=vertices[0].x;
    range.lower.y=range.upper.y=vertices[0].y;
    range.lower.z=range.upper.z=vertices[0].z;
    for(FXuint n=1; n<vertexNumber; n++){
      range.lower.x=FXMIN(range.lower.x, vertices[n].x);
      range.lower.y=FXMIN(range.lower.y, vertices[n].y);
      range.lower.z=FXMIN(range.lower.z, vertices[n].z);
      range.upper.x=FXMAX(range.upper.x, vertices[n].x);
      range.upper.y=FXMAX(range.upper.y, vertices[n].y);
      range.upper.z=FXMAX(range.upper.z, vertices[n].z);
      }
    }
  box.lower.x=position.x+range.lower.x; box.upper.x=position.x+range.upper.x;
  box.lower.y=position.y+range.lower.y; box.upper.y=position.y+range.upper.y;
  box.lower.z=position.z+range.lower.z; box.upper.z=position.z+range.upper.z;
  }


inline void FXGLVertices::renderPoints(FXGLViewer *viewer, bool isHit, bool complex){
#ifdef HAVE_GL_H
  FXGLColor col(color);
  FXuint inc=(!(options & (SHADING_SMOOTH|SHADING_FLAT)) && viewer->doesTurbo()) ? 4 : 1;
  for(FXuint n=0; n<vertexNumber; n+=inc){
    if(!isHit){
      if(colorGenerator) for(FXuint c=0; c<inc; c++) colorGenerator(col, this, viewer, n, colorGeneratorData);
      if(complex && (options & (SHADING_SMOOTH|SHADING_FLAT))){
        glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,&col.r);
        }
       else
        glColor4fv(&col.r);
      }
    if(complex){
      glPushMatrix();
      glTranslatef(vertices[n].x,vertices[n].y,vertices[n].z);
      glCallList(displayLists+2);
      glPopMatrix();
      }
    else{
      glVertex3fv(&vertices[n].x);
      }
    }
#endif
  }

inline void FXGLVertices::renderLines(FXGLViewer *viewer, bool isHit, bool complex){
#ifdef HAVE_GL_H
  FXGLColor col(color);
  GLUquadricObj* quad=0;
  if(complex){
    quad=gluNewQuadric();
    gluQuadricDrawStyle(quad,(GLenum)GLU_FILL);
  }
  FXuint inc=(!(options & (SHADING_SMOOTH|SHADING_FLAT)) && viewer->doesTurbo()) ? 4 : 1;
  for(FXuint n=0; n<vertexNumber-complex; n+=inc){
    if(!isHit){
      if(colorGenerator) for(FXuint c=0; c<inc; c++) colorGenerator(col, this, viewer, n, colorGeneratorData);
      if(complex && (options & (SHADING_SMOOTH|SHADING_FLAT))){
        glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,&col.r);
        }
       else
        glColor4fv(&col.r);
      }
    if(complex){
      FXVec3f vector=vertices[n+1]-vertices[n];
      FXfloat len=veclen(vector);
      if(!(options & VERTICES_POINTS) || len>=pointSize*0.8f){
        glPushMatrix();
        glTranslatef(vertices[n].x,vertices[n].y,vertices[n].z);
        FXVec3f zaxis(0.0f,0.0f,vector.z<0 ? 1.0f : -1.0f);
        // Cylinder points in Z direction, so we need to rotate so it follows vector
        FXVec3f vectornormal(vecnormalize(vector));
        FXVec3f axis(vectornormal^zaxis);
        FXfloat angle=((FXfloat) RTOD)*asinf(veclen(axis));
        //FXfloat dotproduct=vectornormal*axis;
        if(vector.z<0) angle+=180;
        glRotatef(angle,axis.x,axis.y,axis.z);
        gluCylinder(quad,lineSize/2,lineSize/2,len,8,8);
        glPopMatrix();
        }
      }
    else{
      glVertex3fv(&vertices[n].x);
      }
    }
  if(complex){
    gluDeleteQuadric(quad);
  }
#endif
  }

inline void FXGLVertices::render(FXGLViewer *viewer, bool isHit, bool complex){
#ifdef HAVE_GL_H
  if(!displayLists) displayLists=glGenLists(3);
  if(modified & (1<<viewer->doesTurbo())){
    //fxmessage("modified=%u\n", modified);
    if(complex){
      // Render a sphere into a display list as it's the same for all points
      GLUquadricObj* quad=0;
      glNewList(displayLists+2, GL_COMPILE);
      quad=gluNewQuadric();
      gluQuadricDrawStyle(quad,(GLenum)GLU_FILL);
      gluSphere(quad,pointSize/2,12,12);
      gluDeleteQuadric(quad);
      glEndList();
      }
    glNewList(displayLists+viewer->doesTurbo(), GL_COMPILE);
    if(vertexNumber){
      if(complex){
        if(options & VERTICES_POINTS)
          renderPoints(viewer, isHit, complex);
        if(options & VERTICES_LINES)
          renderLines(viewer, isHit, complex);
        }
      else{
        if(options & VERTICES_POINTS){
          if(!isHit) glPointSize(pointSize);
          glBegin(GL_POINTS);
          renderPoints(viewer, isHit, complex);
          glEnd();
        }
        if(options & VERTICES_LINES){
          if(!isHit) glLineWidth(lineSize);
          glBegin((options & VERTICES_LOOPLINES) ? GL_LINE_LOOP : (options & VERTICES_LINEITEMS) ? GL_LINES : GL_LINE_STRIP);
          renderLines(viewer, isHit, complex);
          glEnd();
          }
        }
      }
    glEndList();
    modified&=~(1<<viewer->doesTurbo());
    }
  glCallList(displayLists+viewer->doesTurbo());
#endif
  }

void FXGLVertices::drawshape(FXGLViewer *viewer){
  render(viewer, false, true);
  }


// Draw this object in a viewer
void FXGLVertices::draw(FXGLViewer* viewer){
  // If we need lighting, set that up otherwise render directly
  if((options & STYLE_SURFACE) && !viewer->doesTurbo())
    FXGLShape::draw(viewer);
  else{
    glPushAttrib(GL_CURRENT_BIT|GL_POINT_BIT|GL_LINE_BIT);
    glPushMatrix();

    // Object position
    glTranslatef(position[0],position[1],position[2]);

    render(viewer, false, false);

    // Restore attributes and matrix
    glPopMatrix();
    glPopAttrib();
    }
  }


// Draw this object for hit-testing purposes
void FXGLVertices::hit(FXGLViewer* viewer){
  glPushAttrib(GL_CURRENT_BIT|GL_POINT_BIT|GL_LINE_BIT);
  glPushMatrix();

  // Object position
  glTranslatef(position[0],position[1],position[2]);

  render(viewer, true, false);

  // Restore attributes and matrix
  glPopMatrix();
  glPopAttrib();
  }


// Copy this object
FXGLObject* FXGLVertices::copy(){
  return new FXGLVertices(*this);
  }


// Save object to stream
void FXGLVertices::save(FXStream& store) const {
  FXGLShape::save(store);
  store << vertexNumber;
  store.save((FXfloat *) vertices, 3*vertexNumber);
  store << pointSize << color;
  }


// Load object from stream
void FXGLVertices::load(FXStream& store){
  FXGLShape::load(store);
  store >> vertexNumber;
  FXMALLOC(&vertices, FXVec3f, vertexNumber);
  store.load((FXfloat *) vertices, 3*vertexNumber);
  store >> pointSize >> color;
  }


// Rainbow color generator
void FXGLVertices::RainbowGenerator(FXGLColor &color, FXGLVertices *obj, FXGLViewer *viewer, FXuint vertex, void *&data){
  struct MyData{
    FXfloat scol[4], dcol[3];
    } *mydata=(MyData *) data;
  if(!vertex){
    if(!data){
      mydata=(MyData *) (data=malloc(sizeof(MyData)));
      }
    mydata->scol[0]=mydata->scol[1]=mydata->scol[2]=mydata->scol[3]=1.0f;
    mydata->dcol[0]=-0.002f; mydata->dcol[1]=-0.003f; mydata->dcol[2]=-0.001f;
    }
  mydata->scol[0]+=mydata->dcol[0];
  mydata->scol[1]+=mydata->dcol[1];
  mydata->scol[2]+=mydata->dcol[2];
  if(mydata->scol[0]<0.05f || mydata->scol[0]>=1.0f) mydata->dcol[0]=-mydata->dcol[0];
  if(mydata->scol[1]<0.05f || mydata->scol[1]>=1.0f) mydata->dcol[1]=-mydata->dcol[1];
  if(mydata->scol[2]<0.05f || mydata->scol[2]>=1.0f) mydata->dcol[2]=-mydata->dcol[2];
  color.r=mydata->scol[0]; color.g=mydata->scol[1]; color.b=mydata->scol[2]; color.a=mydata->scol[3];
  if(obj->getNumberOfVertices()-1==vertex){
    free(data);
    data=0;
    }
  }


// Brighter color generator. Goes from black up to getColor()
void FXGLVertices::BrighterGenerator(FXGLColor &color, FXGLVertices *obj, FXGLViewer *viewer, FXuint vertex, void *&data){
  const FXGLColor &base=obj->getColor();
  FXuint vertices=obj->getNumberOfVertices()-1;
  color.r=base.r*vertex/vertices;
  color.g=base.g*vertex/vertices;
  color.b=base.b*vertex/vertices;
  color.a=base.a;
  }

// Darker color generator. Goes from getColor() to black
void FXGLVertices::DarkerGenerator(FXGLColor &color, FXGLVertices *obj, FXGLViewer *viewer, FXuint vertex, void *&data){
  const FXGLColor &base=obj->getColor();
  FXuint vertices=obj->getNumberOfVertices()-1;
  color.r=base.r*(vertices-vertex)/vertices;
  color.g=base.g*(vertices-vertex)/vertices;
  color.b=base.b*(vertices-vertex)/vertices;
  color.a=base.a;
  }


}

#endif
