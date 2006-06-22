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

import TnFXSQLDB

def baseTnFXSQLDB_ipc():
    return "TnFXSQLDB"

def applyTnFXSQLDB_ipc(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    TnFXSQLDB.applyTnFXSQLDB(g, cclass)
    set_policy(cclass.setIsAsynchronous,       return_self())
    set_policy(cclass.setPrefetching,          return_self())
    
def applyTnFXSQLDBServer(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    set_policy(cclass.addDatabase,             return_self())
    
