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

def baseFXFoldingList():
    return "FXIconList"

def applyFXFoldingList(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    FXIconList.applyFXIconList(g, cclass)
    set_policy(cclass.getFirstItem,     return_internal_reference())
    set_policy(cclass.getLastItem,      return_internal_reference())
    set_policy(cclass.addItemFirst,     return_internal_reference())
    set_policy(cclass.addItemLast,      return_internal_reference())
    set_policy(cclass.addItemAfter,     return_internal_reference())
    set_policy(cclass.addItemBefore,    return_internal_reference())
    set_policy(cclass.moveItemAfter,    return_internal_reference())
    set_policy(cclass.moveItemBefore,   return_internal_reference())
    set_policy(cclass.getItemAt,        return_internal_reference())
    set_policy(cclass.findItem,         return_internal_reference())
    set_policy(cclass.getCurrentItem,   return_internal_reference())
    set_policy(cclass.getItemOpenIcon,  return_internal_reference())
    set_policy(cclass.getItemClosedIcon,return_internal_reference())
    set_policy(cclass.getAnchorItem,    return_internal_reference())
    set_policy(cclass.getCursorItem,    return_internal_reference())
