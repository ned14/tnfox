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

import FXDialogBox
import FXPopup

def baseFXHandedInterface():
    return "FXDialogBox"

def applyFXHandedInterfaceI(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    set_policy(cclass.buttonWell,       return_internal_reference())
    set_policy(cclass.okButton,         return_internal_reference())
    set_policy(cclass.cancelButton,     return_internal_reference())

def applyFXHandedDialog(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    FXDialogBox.applyFXDialogBox(g, cclass)
    applyFXHandedInterfaceI(g, cclass)

def applyFXHandedPopup(g, cclass):
    for key,value in g.items():
        globals()[key]=value
    FXPopup.applyFXPopup(g, cclass)
    applyFXHandedInterfaceI(g, cclass)

def customise(g):
    for key,value in g.items():
        globals()[key]=value
    SplitOutput("FX::FXHandedDialog")
    SplitOutput("FX::FXHandedPopup")