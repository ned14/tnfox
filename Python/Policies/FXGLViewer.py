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

import FXGLCanvas

def baseFXGLViewer():
    return "FXGLCanvas"

def applyFXGLViewer(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    FXGLCanvas.applyFXGLCanvas(g, cclass)
    set_policy(cclass.lasso,          return_value_policy(manage_new_object))
    set_policy(cclass.select,         return_value_policy(manage_new_object))
    set_policy(cclass.pick,           return_internal_reference())
    set_policy(cclass.getScene,       return_internal_reference())
    set_policy(cclass.getSelection,   return_internal_reference())
    exclude(cclass.getZSortFunc)
    exclude(cclass.setZSortFunc)
    
def customise(g):
    for key,value in g.items():
        globals()[key]=value
    declaration_code("""static FXMallocHolder<FXGLObject *> *FXGLViewer_lasso(FXGLViewer &c, FXint x1,FXint y1,FXint x2,FXint y2)
{
    return new FXMallocHolder<FXGLObject *>(c.lasso(x1,y1,x2,y2));
}
static FXMallocHolder<FXGLObject *> *FXGLViewer_select(FXGLViewer &c, FXint x,FXint y,FXint w,FXint h)
{
    return new FXMallocHolder<FXGLObject *>(c.select(x,y,w,h));
}""")
    
