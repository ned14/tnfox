#********************************************************************************
#                                                                               *
#                            TnFOX tests make file                              *
#                                                                               *
#********************************************************************************
#        Copyright (C) 2003-2007 by Niall Douglas.   All Rights Reserved.       *
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
dir,name=os.path.split(os.getcwd())
if "windows" in dir or "/media/Share" in dir:
    # Replace with Unix equiv
    dir="/home/ned/"+dir[dir.find("Tornado"):]
if "/Volumes/DATA" in dir:
    # Replace with MacOS X equiv
    dir="/Users/ned/Documents/"+dir[dir.find("Tornado"):]
execfile(dir+"/../sconslib.py")
init(globals(), dir+"/../", dir+"/")
targetname=dir+"/../lib/"+architectureSpec()+"/"+name
if globals().has_key('DoConfTests'):
    doConfTests(env, os.path.normpath(dir+"/..")+"/")

env['CPPPATH']+=[ ".",
                 "../../include",
                 "../../../boost",
                 "../../Python"
                 ]
if PYTHON_INCLUDE:
    env['CPPPATH'].append(PYTHON_INCLUDE)
else:
    try:
        if wantPython: pass
    except:
        wantPython=False
    try:
        env['CPPPATH'].append(os.environ["PYTHON_INCLUDE"])
    except:
        if wantPython: raise IOError, "You need to define PYTHON_INCLUDE and PYTHON_LIB for this test"

suffix=ternary(GenStaticLib==2, ".a", ternary(onWindows, env['LIBSUFFIX'], ternary(onWindows or onDarwin, env['SHLIBSUFFIX'], ".la")))
env['LINKFLAGS']+=[os.path.normpath(dir+"/../lib/"+architectureSpec()+"/"+env['LIBPREFIX']+libtnfox+suffix)]
if SQLModule==2: env['LINKFLAGS']+=[os.path.normpath(dir+"/../lib/"+architectureSpec()+"/"+env['LIBPREFIX']+libtnfoxsql+suffix)]
if GraphingModule==2: env['LINKFLAGS']+=[os.path.normpath(dir+"/../lib/"+architectureSpec()+"/"+env['LIBPREFIX']+libtnfoxgraphing+suffix)]
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
            env['LINKFLAGS']+=[ os.path.abspath(dir+"/../lib/"+architectureSpec()+"/TnFOX.so") ]
except: pass

# Some sanity checks
assert os.path.exists("../../include/fxdefs.h")
conf=Configure(env, { "CheckTnFOXIsBigEndian" : CheckTnFOXIsBigEndian, "CheckTnFOXIsLittleEndian" : CheckTnFOXIsLittleEndian } )
foxbigendiandef=filter(lambda defn: defn[0]=="FOX_BIGENDIAN", env['CPPDEFINES'])[0]
TnFOXIsBigEndian=conf.CheckTnFOXIsBigEndian()
TnFOXIsLittleEndian=conf.CheckTnFOXIsLittleEndian()
assert TnFOXIsBigEndian or TnFOXIsLittleEndian
if not ternary(foxbigendiandef[1], TnFOXIsBigEndian, TnFOXIsLittleEndian):
    # Hmm our FOX_BIGENDIAN test is wrong! This happens on coLinux due to a bug in cofs not running executables properly
    print "WARNING: FOX_BIGENDIAN definition does not match that of TnFOX library! Fixing ..."
    env['CPPDEFINES'].remove(foxbigendiandef)
    env['CPPDEFINES'].append(("FOX_BIGENDIAN", 1-foxbigendiandef[1]))
env=conf.Finish()

objects=[env.StaticObject(builddir+"/main", "main.cpp")] #+ [env.StaticObject(builddir+"/gcLink", "../gcLink.cc")]
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
try:
    env.Execute(postbuildexecute)
except: pass
targetfrombase=targetname[3:]
Return("targetfrombase")
