#! /usr/bin/env python
#********************************************************************************
#                                                                               *
#                           TnFOX Python bindings test                          *
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

import sys
import os
sys.path=[os.path.abspath("../../lib")]+sys.path # In case TnFOX isn't installed
from TnFOX import *

from mainwindow import *
myapp=FXApp("TestPython", "Niall Douglas")
args=sys.argv +["TestPython", "-tracelevel","999999"]
myapp.init(len(args), args)
main=MainWindow(myapp, "TnFOX Python test")
myapp.create()
main.show()

# Test QValueList translation
memmaps=FXProcess.mappedFiles()
main.addText("\nList of mapped in files:\n")
for mapping in memmaps:
    txt="  %08x to %08x  " % (mapping.startaddr, mapping.endaddr)
    if mapping.read: txt+="r"
    else: txt+="-"
    if mapping.write: txt+="w"
    else: txt+="-"
    if mapping.execute: txt+="x"
    else: txt+="-"
    if mapping.copyonwrite: txt+="c"
    else: txt+="-"
    txt+=" %s\n" % mapping.path
    main.addText(txt)

# main loop
retcode=myapp.run()

# Delete everything before exit
del memmaps
del main
del myapp
#sys.exit(retcode)
