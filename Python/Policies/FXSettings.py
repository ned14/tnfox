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

def baseFXSettings():
    return "FXObject"

def applyFXSettings(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    FXObject.applyFXObject(g, cclass)
    exclude(cclass.readFormatEntry)
    exclude(cclass.writeFormatEntry)
    set_policy(cclass.insert,           return_internal_reference())
    set_policy(cclass.replace,          return_internal_reference())
    set_policy(cclass.remove,           return_internal_reference())
    set_policy(cclass.find,             return_internal_reference())
    set_policy(cclass.data,             return_internal_reference())
