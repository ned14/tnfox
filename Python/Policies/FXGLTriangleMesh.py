#********************************************************************************
#                                                                               *
#                             TnFOX Pyste policies                              *
#                                                                               *
#********************************************************************************
#        Copyright (C) 2003 by Niall Douglas.   All Rights Reserved.            *
#       NOTE THAT I DO NOT PERMIT ANY OF MY CODE TO BE PROMOTED TO THE GPL      *
#********************************************************************************
# This code is free software; you can redistribute it and/or modify it under    *
# the terms of the GNU Library General Public License v2.1 as published by the  *
# Free Software Foundation EXCEPT that clause 3 does not apply ie; you may not  *
# "upgrade" this code to the GPL without my prior written permission.           *
# Please consult the file "License_Addendum2.txt" accompanying this file.       *
#                                                                               *
# This code is distributed in the hope that it will be useful,                  *
# but WITHOUT ANY WARRANTY; without even the implied warranty of                *
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                          *
#********************************************************************************

import FXGLShape

def baseFXGLTriangleMesh():
    return "FXGLShape"

def applyFXGLTriangleMesh(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    FXGLShape.applyFXGLShape(g, cclass)
    set_policy(cclass.getVertexBuffer,              return_internal_reference())
    set_policy(cclass.getColorBuffer,               return_internal_reference())
    set_policy(cclass.getNormalBuffer,              return_internal_reference())
    set_policy(cclass.getTextureCoordBuffer,        return_internal_reference())

def customise(g):
    for key,value in g.items():
        globals()[key]=value
    declaration_code("""DEFINE_MAKECARRAYITER(FXGLTriangleMesh, FX::FXfloat, getVertexBuffer, (), 3*c.getVertexNumber())
DEFINE_MAKECARRAYITER(FXGLTriangleMesh, FX::FXfloat, getColorBuffer, (), 4*c.getVertexNumber())
DEFINE_MAKECARRAYITER(FXGLTriangleMesh, FX::FXfloat, getNormalBuffer, (), 3*c.getVertexNumber())
DEFINE_MAKECARRAYITER(FXGLTriangleMesh, FX::FXfloat, getTextureCoordBuffer, (), 2*c.getVertexNumber())""")
    
