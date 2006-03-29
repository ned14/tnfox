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

def baseFXACL():
    return None

def applyFXACL(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    exclude(cclass.Permissions.read)
    exclude(cclass.Permissions.write)
    exclude(cclass.Permissions.execute)
    exclude(cclass.Permissions.append)
    exclude(cclass.Permissions.copyonwrite)
    exclude(cclass.Permissions.reserved2)
    exclude(cclass.Permissions.list)
    exclude(cclass.Permissions.createfiles)
    exclude(cclass.Permissions.createdirs)
    exclude(cclass.Permissions.traverse)
    exclude(cclass.Permissions.deletefiles)
    exclude(cclass.Permissions.deletedirs)
    exclude(cclass.Permissions.reserved1)
    exclude(cclass.Permissions.readattrs)
    exclude(cclass.Permissions.writeattrs)
    exclude(cclass.Permissions.readperms)
    exclude(cclass.Permissions.writeperms)
    exclude(cclass.Permissions.takeownership)
    exclude(cclass.Permissions.reserved3)
    exclude(cclass.Permissions.amTn)
    exclude(cclass.Permissions.custom)
    
    set_policy(cclass.Permissions.setRead,         return_self())
    set_policy(cclass.Permissions.setWrite,        return_self())
    set_policy(cclass.Permissions.setExecute,      return_self())
    set_policy(cclass.Permissions.setAppend,       return_self())
    set_policy(cclass.Permissions.setCopyOnWrite,  return_self())
    set_policy(cclass.Permissions.setList,         return_self())
    set_policy(cclass.Permissions.setCreateFiles,  return_self())
    set_policy(cclass.Permissions.setCreateDirs,   return_self())
    set_policy(cclass.Permissions.setTraverse,     return_self())
    set_policy(cclass.Permissions.setDeleteFiles,  return_self())
    set_policy(cclass.Permissions.setDeleteDirs,   return_self())
    set_policy(cclass.Permissions.setReadAttrs,    return_self())
    set_policy(cclass.Permissions.setWriteAttrs,   return_self())
    set_policy(cclass.Permissions.setReadPerms,    return_self())
    set_policy(cclass.Permissions.setWritePerms,   return_self())
    set_policy(cclass.Permissions.setTakeOwnership, return_self())
    set_policy(cclass.Permissions.setGenRead,      return_self())
    set_policy(cclass.Permissions.setGenWrite,     return_self())
    set_policy(cclass.Permissions.setGenExecute,   return_self())
    set_policy(cclass.Permissions.setAll,          return_self())

    set_policy(cclass.owner,        return_internal_reference())
    exclude(cclass.int_toWin32SecurityDescriptor)

    exclude(cclass.ACLSupport.perOwnerGroup)
    exclude(cclass.ACLSupport.perEntity)
    exclude(cclass.ACLSupport.hasInheritance)
    
