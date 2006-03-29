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

import FXIconList
import FXObject

def baseFXFoldingList():
    return "FXIconList"

def applyFXFoldingItem(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    FXObject.applyFXObject(g, cclass)
    set_policy(cclass.getParent,        return_value_policy(reference_existing_object))
    set_policy(cclass.getNext,          return_value_policy(reference_existing_object))
    set_policy(cclass.getPrev,          return_value_policy(reference_existing_object))
    set_policy(cclass.getFirst,         return_value_policy(reference_existing_object))
    set_policy(cclass.getLast,          return_value_policy(reference_existing_object))
    set_policy(cclass.getBelow,         return_value_policy(reference_existing_object))
    set_policy(cclass.getAbove,         return_value_policy(reference_existing_object))
    set_policy(cclass.getOpenIcon,      return_value_policy(reference_existing_object))
    set_policy(cclass.getClosedIcon,    return_value_policy(reference_existing_object))

def applyFXFoldingList(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    FXIconList.applyFXIconList(g, cclass)
    set_policy(cclass.getFirstItem,     return_internal_reference())
    set_policy(cclass.getLastItem,      return_internal_reference())
    set_policy(cclass.insertItem,       return_internal_reference())
    set_policy(cclass.appendItem,       return_internal_reference())
    set_policy(cclass.prependItem,      return_internal_reference())
    set_policy(cclass.moveItem,         return_internal_reference())
    set_policy(cclass.getItemAt,        return_internal_reference())
    set_policy(cclass.findItem,         return_internal_reference())
    set_policy(cclass.findItemByData,   return_internal_reference())
    set_policy(cclass.getItemOpenIcon,  return_internal_reference())
    set_policy(cclass.getItemClosedIcon,return_internal_reference())
    set_policy(cclass.getCurrentItem,   return_internal_reference())
    set_policy(cclass.getAnchorItem,    return_internal_reference())
    set_policy(cclass.getCursorItem,    return_internal_reference())

def customise(g):
    for key,value in g.items():
        globals()[key]=value
    SplitOutput("FX::FXFoldingList")
