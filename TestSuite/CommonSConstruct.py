#********************************************************************************
#                                                                               *
#                            TnFOX tests make file                              *
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

import os
import sys
execfile("../../sconslib.py")
init(globals(), "../../", "../")
dir,name=os.path.split(os.getcwd())
if "hgfs" in dir: dir="/home/ned/Tornado/TClient/TnFOX/TestSuite"
if "kate/D" in dir: dir="/usr/home/ned/Tornado/TClient/TnFOX/TestSuite"
targetname=dir+"/../lib/"+name

env['CPPDEFINES']+=[ "FOXDLL" ]
env['CPPPATH']+=[ ".",
                 "../../include",
                 "../../../boost",
                 "../../Python"
                 ]
env['LIBPATH']+=[dir+"/../lib"]
if PYTHON_INCLUDE:
    env['CPPPATH'].append(PYTHON_INCLUDE)
else:
    try:
        env['CPPPATH'].append(os.environ["PYTHON_INCLUDE"])
    except:
        try:
            if wantPython: raise IOError, "You need to define PYTHON_INCLUDE and PYTHON_LIB for this test"
        except: pass

if not onWindows: # Can't put in g++.py as dir isn't defined there
    env['LINKFLAGS']+=[os.path.normpath(dir+"/../lib/lib"+libtnfox+".la")] #, "-static" ] #, "/lib/libselinux.so.1"]
try:
    if wantPython:
        if PYTHON_LIB:
            env['LINKFLAGS'].append(PYTHON_LIB)
        else:
            env['LINKFLAGS'].append(os.environ["PYTHON_LIB"])
        if onWindows:
            env['LIBS']+=[ "TnFOX" ]
        else:
            env['LIBS']+=[ "util" ]
            env['LINKFLAGS']+=[ os.path.abspath(dir+"/../lib/TnFOX.so") ]
except: pass
assert os.path.exists("../../include/fxdefs.h")

objects=[env.StaticObject(builddir+"/main", "main.cpp")]
Clean(targetname, objects)
EXE=env.Program(targetname, objects)
env.Precious(EXE)

if onWindows:
    env.MSVSProject("project"+env['MSVSPROJECTSUFFIX'],
                srcs=["main.cpp"],
                localincs=["../../config.py"],
                buildtarget=EXE,
                variant=["Release","Debug"][debugmode])
    env.Alias("msvcproj", "project"+env['MSVSPROJECTSUFFIX'])
Default(EXE)
targetfrombase=targetname[3:]
Return("targetfrombase")
