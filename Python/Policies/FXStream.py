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

def baseFXStream():
    return None

def applyFXStream(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    set_policy(cclass.device,           return_value_policy(reference_existing_object))
    set_policy(cclass.readBytes,        return_self())
    set_policy(cclass.readRawBytes,     return_self())
    set_policy(cclass.writeBytes,       return_self())
    set_policy(cclass.writeRawBytes,    return_self())
    set_policy(cclass.container,        return_value_policy(reference_existing_object))
    set_policy(cclass.save,             return_self())
    set_policy(cclass.load,             return_self())
    set_policy(cclass.saveObject,       return_self())
    set_policy(cclass.loadObject,       return_self())
    set_policy(cclass.addObject,        return_self())
