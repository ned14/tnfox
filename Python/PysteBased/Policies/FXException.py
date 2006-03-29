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

def baseFXException():
    return None

def applyFXException(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    set_policy(cclass.nested,     return_internal_reference())
    exclude(cclass.int_enableNestedExceptionFramework)

def applyFXRangeException(g, cclass):
    applyFXException(g, cclass)

def applyFXPointerException(g, cclass):
    applyFXException(g, cclass)

def applyFXResourceException(g, cclass):
    applyFXException(g, cclass)

def applyFXMemoryException(g, cclass):
    applyFXException(g, cclass)

def applyFXNotSupportedException(g, cclass):
    applyFXException(g, cclass)

def applyFXNotFoundException(g, cclass):
    applyFXException(g, cclass)

def applyFXIOException(g, cclass):
    applyFXException(g, cclass)

def applyFXConnectionLostException(g, cclass):
    applyFXException(g, cclass)

def applyFXNoPermissionException(g, cclass):
    applyFXException(g, cclass)

def applyFXWindowException(g, cclass):
    applyFXException(g, cclass)

def applyFXImageException(g, cclass):
    applyFXException(g, cclass)

def applyFXFontException(g, cclass):
    applyFXException(g, cclass)

def customise(g):
    for key,value in g.items():
        globals()[key]=value
    declaration_code("""namespace boost { namespace python { namespace indexing {
    template<> struct value_traits<FX::FXException> : public value_traits<int>
    {
        BOOST_STATIC_CONSTANT (bool, equality_comparable = false);
        BOOST_STATIC_CONSTANT (bool, less_than_comparable = false);
    };
} } }""")
    module_code("    RegisterConvQValueList<FX::FXException>();\n")
    
