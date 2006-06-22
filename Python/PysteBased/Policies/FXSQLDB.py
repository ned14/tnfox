#********************************************************************************
#                                                                               *
#                             TnFOX Pyste policies                              *
#                                                                               *
#********************************************************************************
#        Copyright (C) 2005 by Niall Douglas.   All Rights Reserved.            *
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

def baseTnFXSQLDB():
    return None

def applyTnFXSQLDB(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    set_policy(cclass.Capabilities.setTransactions,         return_self())
    set_policy(cclass.Capabilities.setQueryRows,            return_self())
    set_policy(cclass.Capabilities.setNoTypeConstraints,    return_self())
    set_policy(cclass.Capabilities.setHasBackwardsCursor,   return_self())
    set_policy(cclass.Capabilities.setHasSettableCursor,    return_self())
    set_policy(cclass.Capabilities.setHasStaticCursor,      return_self())
    set_policy(cclass.Capabilities.setAsynchronous,         return_self())

def applyTnFXSQLDBRegistry(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    set_policy(cclass.processRegistry,  return_value_policy(reference_existing_object))

def applyTnFXSQLDBStatement(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    set_policy(cclass.bind,             return_self())
    set_policy(cclass.driver,           return_value_policy(reference_existing_object))

def applyTnFXSQLDBColumn(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    set_policy(cclass.cursor,           return_value_policy(reference_existing_object))
    exclude(cclass.data)

def applyTnFXSQLDBCursor(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    set_policy(cclass.statement,        return_value_policy(reference_existing_object))
    set_policy(cclass.resultsLatch,     return_value_policy(reference_existing_object))
