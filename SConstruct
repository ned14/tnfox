#********************************************************************************
#                                                                               *
#                                 TnFOX make file                               *
#                                                                               *
#********************************************************************************
#        Copyright (C) 2003-2005 by Niall Douglas.   All Rights Reserved.       *
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

EnsurePythonVersion(2,3)
execfile("sconslib.py")
init(globals())
if "hgfs/D" in os.getcwd() or "kate/D" in os.getcwd():
    raise IOError, "You don't want to run me on the Windows share!"
targetname="lib/"+architectureSpec()
if not os.path.exists(targetname):
    os.mkdir(targetname)
targetname+="/"+tnfoxname
env['CPPDEFINES']+=[ "FOXDLL_EXPORTS" ]
doConfTests(env)

updmunged=env.Command("dont exist", None, ternary(onWindows, "", "python ")+'UpdateMunged.py -d src -c "-f 4 -c include/FXErrCodes.h -t TnFOXTrans.txt"')
objects=[]
if not disableGUI:
    objects+=[env.SharedObject(builddir+"/"+getBase(x), "src/"+x, CPPFLAGS=env['CPPFLAGS']+env['CCWPOOPTS']) for x in getTnFOXSources("", False)]
objects+=[env.SharedObject(builddir+"/"+getBase(x), "src/"+x, CPPFLAGS=env['CPPFLAGS']+env['CCWPOOPTS']) for x in getTnFOXSources("", True)]
if libsqlite: objects.append(libsqlite)
for object in objects:
    env.Depends(object, updmunged)
if onWindows:
    versionrc="src/version.rc"
    objects+=[env.RES(builddir+"/version.res", versionrc)]
Clean(targetname, objects)
DLL=VersionedSharedLibrary(env, targetname+ternary(disableGUI, "_noGUI", ""), tnfoxversioninfo, "/usr/local/"+libPathSpec(make64bit), objects, debugmode, GenStaticLib)
env.Precious(DLL)
addBind(DLL)

if onWindows:
    env.MSVSProject("windows/TnFOXProject"+env['MSVSPROJECTSUFFIX'],
                srcs=["../src/"+x for x in getTnFOXSources()],
                incs=["../include/"+x for x in getTnFOXIncludes()],
                localincs=["../config.py", "../config/msvc.py", "../config/g++.py"],
                resources="../"+versionrc,
                misc=["../"+x for x in ["ChangeLog.txt", "License.txt", "License_Addendum.txt",
                      "License_Addendum2.txt", "Readme.txt", "ReadmePython.txt",
                      "TnFOXTrans.txt", "Todo.txt"]],
                buildtarget=DLL,
                variant=["Release","Debug"][debugmode])
    env.Alias("msvcproj", "windows/TnFOXProject"+env['MSVSPROJECTSUFFIX'])
env.Alias("tnfox", DLL)
env.Alias("python", env.Command("python_", DLL, "cd Python && scons"))
env.Alias("tests",  env.Command("tests_", DLL, "cd TestSuite && scons"))
env.Alias("all", ["tnfox", "python", "tests"])
def runSynopsis(target, source, env):
    syns=genSynopsis("include", getTnFOXIncludes())
    callSynopsis(syns)
env.Command("synopsis", None, runSynopsis)
# env.Command("install", DLL, "chown root::root "+libAsLA()+" && chmod a+s "+libAsLA()+" && libtool --mode=install cp "+libAsLA()+" /usr/local/"+libPathSpec(make64bit))
env.Command("install", DLL, "libtool --mode=install cp "+libAsLA()+" /usr/local/"+libPathSpec(make64bit))
env.Command("uninstall", DLL, "libtool --mode=uninstall rm /usr/local/"+libPathSpec(make64bit)+"/"+libAsLA(True))
Help("""TnFOX make options:
  scons tnfox     : Make TnFOX DLL
  scons python    : Make TnFOX Python bindings DLL
  scons tests     : Make all tests
  scons all       : Make everything
  scons install   : Install shared libraries (POSIX & root only)
  scons uninstall : Uninstall shared libraries (POSIX & root only)
  scons msvcproj  : Make MSVC compatible project file (Windows only)

Debug and 64 bit modes set by config.py - change before use.
""")
def printhelp(target=None, source=None, env=None):
    print "    *** Try scons -h for help on what targets you can build"
    print "    *** Try scons -H for help on scons functionality"
    return 1
env.Alias("help", env.Command("help", None, printhelp))
Default(DLL)
