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

def baseFXObjectList():
    return None

def applyFXObjectList(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    set_policy(cclass.list,         return_internal_reference())
    set_policy(cclass.assign,       return_self())
    set_policy(cclass.insert,       return_self())
    set_policy(cclass.prepend,      return_self())
    set_policy(cclass.append,       return_self())
    set_policy(cclass.replace,      return_self())
    set_policy(cclass.remove,       return_self())
    set_policy(cclass.clear,        return_self())

def customise(g):
    for key,value in g.items():
        globals()[key]=value
    declaration_code("DEFINE_MAKECARRAYITER(FXObjectList, FX::FXObject *, list, (), c.no())")
    
