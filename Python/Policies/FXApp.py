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

import FXObject

def baseFXApp():
    return "FXObject"

def applyFXApp(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    FXObject.applyFXObject(g, cclass)
    exclude(cclass.dispatchEvent)
    exclude(cclass.getNextEvent)
    exclude(cclass.getDisplay)
    set_policy(cclass.getArgv,          return_internal_reference())
    #set_policy(cclass.getEventLoop,     return_internal_reference())
    set_policy(cclass.getEventLoop,     return_value_policy(return_opaque_pointer))
    set_policy(cclass.getPrimaryEventLoop,return_internal_reference())
    set_policy(cclass.getDefaultVisual, return_internal_reference())
    set_policy(cclass.getMonoVisual,    return_internal_reference())
    set_policy(cclass.getRootWindow,    return_internal_reference())
    set_policy(cclass.getCursorWindow,  return_internal_reference())
    set_policy(cclass.getFocusWindow,   return_internal_reference())
    set_policy(cclass.getPopupWindow,   return_internal_reference())
    set_policy(cclass.findWindowWithId, return_internal_reference())
    set_policy(cclass.findWindowAt,     return_internal_reference())
    set_policy(cclass.addTimeout,       return_value_policy(return_opaque_pointer))
    set_policy(cclass.removeTimeout,    return_value_policy(return_opaque_pointer))
    set_policy(cclass.addChore,         return_value_policy(return_opaque_pointer))
    set_policy(cclass.removeChore,      return_value_policy(return_opaque_pointer))
    set_policy(cclass.getModalWindow,   return_internal_reference())
    set_policy(cclass.reg,              return_internal_reference())
    set_policy(cclass.getDragWindow,    return_internal_reference())
    set_policy(cclass.instance,         return_value_policy(reference_existing_object))
    set_policy(cclass.getNormalFont,    return_internal_reference())
    set_policy(cclass.getWaitCursor,    return_internal_reference())
    set_policy(cclass.getDefaultCursor, return_internal_reference())

def customise(g):
    for key,value in g.items():
        globals()[key]=value
    declaration_code("DEFINE_MAKECARRAYITER(FXApp, const FX::FXchar *, getArgv, (), c.getArgc())\n")
    declaration_code("""
static void FXApp_init(FXApp &app, int argc, list argv, unsigned char connect=TRUE)
{
	int n, size=PyList_Size(argv.ptr());
	static QMemArray<const char *> array;
	array.resize(size+1);
	for(n=0; n<size; n++)
	{
		array[n]=PyString_AsString(PyList_GetItem(argv.ptr(), n));
	}
	array[n]=0;
	app.init(argc, (char **)(array.data()), connect);
}
static void FXApp_init2(FXApp &app, int argc, list argv)
{
	FXApp_init(app, argc, argv);
}""")
    
