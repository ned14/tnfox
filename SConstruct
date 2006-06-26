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
if not os.path.exists("lib"):
    os.mkdir("lib")
if not os.path.exists(targetname):
    os.mkdir(targetname)
targetname+="/"+tnfoxname
doConfTests(env)
# This must go after all the conf tests as it's DLL only
if onDarwin:
    env['LINKFLAGS']+=["-compatibility_version", targetversion,
                       "-current_version", targetversion
                      ]

updmunged=env.Command("dont exist", None, ternary(onWindows, "", "python ")+'UpdateMunged.py -d src -c "-f 4 -c include/FXErrCodes.h -t TnFOXTrans.txt"')
objects=[]
if not disableGUI:
    objects+=[env.SharedObject(builddir+"/"+getBase(x), "src/"+x, CPPFLAGS=env['CPPFLAGS']+env['CCWPOOPTS'], CPPDEFINES=env['CPPDEFINES']+["FOXDLL_EXPORTS"]) for x in getTnFOXSources("", False)]
objects+=[env.SharedObject(builddir+"/"+getBase(x), "src/"+x, CPPFLAGS=env['CPPFLAGS']+env['CCWPOOPTS'], CPPDEFINES=env['CPPDEFINES']+["FOXDLL_EXPORTS"]) for x in getTnFOXSources("", True)]
if SQLModule==1: objects.append(sqlmoduleobjs)
if GraphingModule==1: objects.append(graphingmoduleobjs)
for object in objects:
    env.Depends(object, updmunged)

# Unfortunately some stuff is per output DLL and so can't live in config/<tool>.py
linkflags=env['LINKFLAGS'][:]
if onWindows:
    versionrc="src/version.rc"
    versionobj=env.RES(builddir+"/version.res", versionrc)
    objects+=[versionobj]
    if architecture=="x86" or architecture=="x64":
        linkflags+=[ternary(make64bit, "/BASE:0x7ff06200000", "/BASE:0x62000000")]
Clean(targetname, objects)
DLL=VersionedSharedLibrary(env, targetname+ternary(disableGUI, "_noGUI", ""), tnfoxversioninfo, "/usr/local/"+libPathSpec(make64bit), objects, debugmode, GenStaticLib, LINKFLAGS=linkflags)
env.Precious(DLL)
addBind(DLL)
if SQLModule==2:
    linkflags=env['LINKFLAGS'][:]
    if onWindows:
        sqlmoduleobjs+=[versionobj]
        if architecture=="x86" or architecture=="x64":
            linkflags+=[ternary(make64bit, "/BASE:0x7ff06300000", "/BASE:0x63000000")]
    Clean(targetname, sqlmoduleobjs)
    SQLDLL=VersionedSharedLibrary(env, targetname+"_sql", tnfoxversioninfo, "/usr/local/"+libPathSpec(make64bit), sqlmoduleobjs, debugmode, GenStaticLib, LIBS=env['LIBS']+[libtnfox], LINKFLAGS=linkflags)
    env.Depends(SQLDLL, DLL)
    DLL=SQLDLL
    env.Precious(DLL)
    addBind(DLL)
if GraphingModule==2:
    linkflags=env['LINKFLAGS'][:]
    if onWindows:
        graphingmoduleobjs+=[versionobj]
        if architecture=="x86" or architecture=="x64":
            linkflags+=[ternary(make64bit, "/BASE:0x7ff06310000", "/BASE:0x63100000")]
    Clean(targetname, graphingmoduleobjs)
    GraphingDLL=VersionedSharedLibrary(env, targetname+"_graphing", tnfoxversioninfo, "/usr/local/"+libPathSpec(make64bit), graphingmoduleobjs, debugmode, GenStaticLib, LIBS=env['LIBS']+ternary(SQLModule==2, [libtnfox, libtnfoxsql], [libtnfox]), LINKFLAGS=linkflags)
    env.Depends(GraphingDLL, DLL)
    DLL=GraphingDLL
    env.Precious(DLL)
    addBind(DLL)

if onWindows:
    env.MSVSProject("windows/TnFOXProject"+env['MSVSPROJECTSUFFIX'],
                srcs=["../src/"+x for x in getTnFOXSources()]
                    + ["../src/"+x for x in getSQLModuleSources("")]
                    + ["../src/"+x for x in getGraphingModuleSources("")]
                    + ["../src/"+x for x in getTnFOXSources("", True)],
                incs=["../include/"+x for x in getTnFOXIncludes()]
                    + ["../src/"+x for x in getSQLModuleIncludes("")]
                    + ["../src/"+x for x in getGraphingModuleIncludes("")],
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
