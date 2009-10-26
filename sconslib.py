#********************************************************************************
#                                                                               *
#                              TnFOX scons library                              *
#                                                                               *
#********************************************************************************
#        Copyright (C) 2003-2006 by Niall Douglas.   All Rights Reserved.       *
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
import string
import datetime

def libAsLA(leaf=False):
    ret=str(DLL)
    ret=ret[:string.find(ret, ".so")]+".la"
    if leaf: ret=ret[string.rfind(ret, '/')+1:]
    return ret

def repositionLibrary(target, source, env):
    # On Darwin must rewrite shared library header with full path
    fullpath=os.path.abspath(str(target[0]))
    print "Repositioning library to "+fullpath
    os.system("install_name_tool -id "+fullpath+" "+fullpath)

def VersionedSharedLibraryName(env, target, version, debug=False):
    ABIversion, interface, sourcev, backwards=version
    if ABIversion: target+="-"+ABIversion
    suffix=""
    if debug: target+="d"
    # Hack for scons misinterpreting '.' in target name
    if sys.platform=="win32":
        suffix+=".dll"
    elif sys.platform=="darwin":
        suffix+=".dylib"
    else:
        suffix+=".so"
        suffix+=".%d.%d.%d" % (interface, sourcev, backwards)
    return target,suffix

def VersionedSharedLibrary(env, target, version, installpath, sources, debug=False, genstaticlib=False, **args):
    """Produces a versioned shared library on POSIX and Windows platforms.
    version is a tuple of (ABI string, interface rev, source rev,
    backwards compatible level (usually same as interface rev) ).
    installpath is used only on GNU and is /usr/lib or /usr/local/lib etc."""
    target=os.path.abspath(target)
    targetorig=target
    ABIversion, interface, sourcev, backwards=version
    target,suffix=VersionedSharedLibraryName(env, target, version, debug)
    libtool=sys.platform!="win32" and sys.platform!="darwin"
    DLL=env.SharedLibrary(target+suffix, sources, SHLIBSUFFIX=suffix, **args)
    basetarget=str(DLL[0])
    print "basetarget=",basetarget
    stem,ext=os.path.split(targetorig)
    mpos=string.rfind(basetarget, '-')
    lpos=string.rfind(basetarget, os.sep)
    dpos=string.find(basetarget, '.')
    sopos=string.find(basetarget, '.', dpos+1)
    vIpos=string.find(basetarget, '.', sopos+1)
    vSpos=string.find(basetarget, '.', vIpos+1)
    vBpos=string.find(basetarget, '.', vSpos+1)
    LAIname=os.path.join(stem, basetarget[lpos+1:sopos]+".la")
    def genLAIfile(target, source, env):
        print "Generating LAI file at",LAIname
        oh=file(LAIname, "wt")
        try:
            
            oh.write("# "+LAIname+" - a libtool library file\n")
            oh.write("# Generated by scons for libtool ("+datetime.datetime.now().isoformat()+")\n#\n")
            oh.write("# Please DO NOT delete this file!\n# It is necessary for linking the library.\n\n")
            oh.write("# The name that we can dlopen(3).\n")
            oh.write("dlname='"+basetarget[lpos+1:vSpos]+"'\n\n")
            oh.write("# Names of this library.\n")
            oh.write("library_names='"+basetarget[lpos+1:]+" "+basetarget[lpos+1:vSpos]+" "+basetarget[lpos+1:vIpos]+"'\n\n")
            oh.write("# The name of the static archive.\n")
            oh.write("old_library='"+basetarget[lpos+1:sopos]+".a'\n\n")
            oh.write("# Libraries that this one depends upon.\n")
            oh.write("dependency_libs='")
            for libpath in env['LIBPATH']:
                oh.write(" -L"+libpath)
            for lib in env['LIBS']:
                oh.write(" -l"+lib)
            oh.write("'\n\n")
            oh.write("# Version information for libtnfox.\n")
            oh.write("current=%d\nage=%d\nrevision=%d\n\n" % (interface, backwards, sourcev))
            oh.write("# Is this an already installed library?\n")
            oh.write("installed=no\n\n")
            oh.write("# Files to dlopen/dlpreopen\ndlopen=''\ndlpreopen=''\n\n")
            oh.write("# Directory that this library needs to be installed in:\n")
            oh.write("libdir='"+installpath+"'\n")
        finally:
            oh.close()
        libsdir=os.path.join(stem, ".libs")
        try:
            os.mkdir(libsdir)
        except: pass
        LAIloc=os.path.join(libsdir, basetarget[lpos+1:sopos]+".lai")
        try:
            os.unlink(LAIloc)
        except: pass
        os.link(LAIname, LAIloc)

    def dupLibrary(target, source, env):
        target=str(target[0])
        if os.path.getsize(target)<10240: # Has libtool output into .libs?
            return
        basetarget=target[string.rfind(target, os.path.sep)+1:]
        libsbase=os.path.abspath(os.path.join(stem, ".libs"))
        libsloc=os.path.join(libsbase, basetarget)
        print "Linking to", target, "from", libsloc, "libsbase=", libsbase, "basetarget=",basetarget
        try:
            os.mkdir(libsbase)
        except: pass
        try:
            os.unlink(libsloc)
        except: pass
        os.link(target, libsloc)
    def dupLibrary2(target, source, env):
        libsbase=os.path.abspath(os.path.join(stem, ".libs"))
        try:
            os.unlink(os.path.join(libsbase, basetarget[lpos+1:vSpos]))
        except: pass
        os.symlink(basetarget[lpos+1:], os.path.join(libsbase, basetarget[lpos+1:vSpos]))
        try:
            os.unlink(os.path.join(libsbase, basetarget[lpos+1:vIpos]))
        except: pass
        os.symlink(basetarget[lpos+1:], os.path.join(libsbase, basetarget[lpos+1:vIpos]))

    if libtool or genstaticlib:
        LIB=env.StaticLibrary(target+".a", sources, LIBSUFFIX=".a", **args)
        env.Depends(DLL, LIB)
    if genstaticlib==2: DLL=LIB
    if libtool:
        AddPostAction(LIB, Action(dupLibrary))
        AddPreAction(DLL, Action(genLAIfile))
        Clean(DLL, LAIname)
        AddPostAction(DLL, Action(dupLibrary))
        AddPostAction(DLL, Action(dupLibrary2))
        Clean(DLL, os.path.join(stem, ".libs"))
    if onDarwin:
        AddPostAction(DLL, Action(repositionLibrary))
    return DLL


#********************************************************************************

def ternary(val, A, B):
    if val:
        return A
    else:
        return B

def architectureSpec():
    return architecture+"_"+str(architecture_version)

def libPathSpec(make64bit):
    if make64bit and os.path.exists("/usr/lib64"):
        # This system has parallel 32 & 64 bit libraries
        return "lib64"
    else:
        return "lib"

def getBase(file):
    base,ext=os.path.splitext(file)
    base,leaf=os.path.split(base)
    # If it's in a dir other than src, include that
    if len(base)>3:
        leaf=os.path.join(base[ternary(base[:3]=="src", 4, 0):], leaf)
    return leaf
    
def CheckCompilerPtr32(cc):
    cc.Message("Making sure this is really a 32 bit compiler ...")
    result=cc.TryCompile('int main(void)\n{\nint foo[4==sizeof(void *) ? 1 : 0];\n}\n', '.c')
    cc.Result(result)
    return result

def CheckCompilerPtr64(cc):
    cc.Message("Making sure this is really a 64 bit compiler ...")
    result=cc.TryCompile('int main(void)\n{\nint foo[8==sizeof(void *) ? 1 : 0];\n}\n', '.c')
    cc.Result(result)
    return result

def CheckCompilerIsBigEndian(cc):
    cc.Message("Is the compiler configured for big endian architecture ...")
    result=cc.TryRun('#include <stdio.h>\nint main(void)\n{\nint foo=1;\nint retcode=((char *) &foo)[0]==1;\nprintf("%d,%d=%d\\n", foo, ((char *) &foo)[0], retcode);\nreturn retcode;\n}\n', '.c')
    cc.Result(result[0])
    #print "Result=",result[0]
    return result[0]

def CheckTnFOXIsBigEndian(cc):
    cc.Message("Is the TnFOX library built for big endian architecture ...")
    result=cc.TryLink('extern void tnfoxbigendian(void);\nint main(void)\n{\ntnfoxbigendian();\nreturn 0;\n}\n', '.c')
    cc.Result(result)
    return result

def CheckTnFOXIsLittleEndian(cc):
    cc.Message("Is the TnFOX library built for little endian architecture ...")
    result=cc.TryLink('extern void tnfoxlittleendian(void);\nint main(void)\n{\ntnfoxlittleendian();\nreturn 0;\n}\n', '.c')
    cc.Result(result)
    return result

def CheckCPP0x_N1720(cc):
    cc.Message("Checking for C++0x feature N1720 (static assertions) ...")
    result=cc.TryCompile('int main(void) { static_assert(true, "Death!"); return 0; }\n', '.cpp')
    cc.Result(result)
    return result
def CheckCPP0x_N2118(cc):
    cc.Message("Checking for C++0x feature N2118 (rvalue references) ...")
    result=cc.TryCompile('struct Foo { Foo(int) { } Foo(Foo &&a) { } };\nint main(void) { Foo foo(0); return 0; }\n', '.cpp')
    cc.Result(result)
    return result

def init(cglobals, prefixpath="", platprefix="", targetversion=0, tcommonopts=0):
    prefixpath=os.path.abspath(prefixpath)+"/"
    platprefix=os.path.abspath(platprefix)+"/"
    print prefixpath, platprefix
    varsset={}
    execfile(prefixpath+"config.py", globals(), varsset)    # Sets debugmode, version, libtnfoxname
    for key,value in varsset.items():
        globals()[key]=value
    global FOXCompatLayer, disableGL, disableFileDirDialogs, disableFindReplaceDialogs, disableMDI
    global disableMenus
    global SQLModule, GraphingModule
    if targetversion==0: targetversion=tnfoxversion
    compiler=ARGUMENTS.get("compiler", None)
    global toolset, toolsprefix
    if compiler:
        toolset=[compiler]
    if toolset:
        env=Environment(tools=toolset)
    else:
        env=Environment()
    for tool in env['TOOLS']:
        platformconfig=platprefix+"config/"+tool+".py"
        if os.path.isfile(platformconfig): break
    if not os.path.isfile(platformconfig):
        raise IOError, "No platform config file for any of "+repr(env['TOOLS'])+" was found"
    #env['CC']='icc'
    #env['CXX']='icpc'
    #env['LINK']='icpc'
    #env['ENV']['INCLUDE']=os.environ['INCLUDE']
    #env['ENV']['LIB']=os.environ['LIB']
    #env['ENV']['PATH']=os.environ['PATH']
    #platformconfig=platprefix+"config/intelc.py"
    
    # Force scons to always use absolute paths in everything (helps debuggers to find source files)
    #print env.Dump()
    env['CCCOMFLAGS']= env['CCCOMFLAGS'].replace('$SOURCES','$SOURCES.abspath')
    #env['LINKCOM']   = env['LINKCOM'].replace('$SOURCES','$SOURCES.abspath')

    print "Using platform configuration",platformconfig,"..."
    onWindows=(env['PLATFORM']=="win32" or env['PLATFORM']=="win64")
    onLinux=('linux' in sys.platform)
    onBSD=('bsd' in sys.platform)
    onDarwin=(env['PLATFORM']=="darwin")

    # Force scons job number to be the processors on this machine
    num_jobs=None
    if onWindows:
        num_jobs=1+int(os.environ['NUMBER_OF_PROCESSORS'])
    elif os.sysconf_names.has_key('SC_NPROCESSORS_ONLN'):
        num_jobs=1+int(os.sysconf('SC_NPROCESSORS_ONLN'))
    if num_jobs:
        print "Setting jobs to",num_jobs,"(use -j <n> to override)"
        env.SetOption('num_jobs', num_jobs)
        del num_jobs

    if onWindows:
        #print env['ENV']
        #print "Resetting $(INCLUDE) to",os.environ['INCLUDE']
        env['ENV']['INCLUDE']=os.environ['INCLUDE']
        #print "Resetting $(LIB) to",os.environ['LIB']
        env['ENV']['LIB']=os.environ['LIB']
        #print "Resetting $(PATH) to",os.environ['PATH']
        env['ENV']['PATH']=os.environ['PATH']
    if tcommonopts:
        libtcommon,libtcommonsuffix=VersionedSharedLibraryName(env, tncommonname+ternary(disableGUI, "_noGUI", ""), tncommonversioninfo, debug=debugmode)
        libtcommon2,libtcommonsuffix=VersionedSharedLibraryName(env, tncommonname+"Tn"+ternary(disableGUI, "_noGUI", ""), tncommonversioninfo, debug=debugmode)
    else:
        libtnfox,libtnfoxsuffix=VersionedSharedLibraryName(env, tnfoxname+ternary(disableGUI, "_noGUI", ""), tnfoxversioninfo, debug=debugmode)
    if debugmode:
        builddir="Debug_"+tool+"_"+architectureSpec()
    else:
        builddir="Release_"+tool+"_"+architectureSpec()
    env['CPPDEFINES']=[ ]
    if GenStaticLib!=2: env['CPPDEFINES']=[ "FOXDLL" ]
    env['CPPPATH']=[ prefixpath+"include" ]
    env['CPPFLAGS']=[ ]
    env['LIBPATH']=[ prefixpath+"lib/"+architectureSpec() ]
    env['LIBS']=[ ]
    env['CCWPOOPTS']=[ ]
    env['LINKFLAGS']=[ ]
    make64bit=(architecture=="x64")
    execfile(platformconfig)

    # WARNING: You must NOT touch FXDISABLE_GLOBALALLOCATORREPLACEMENTS here as it breaks
    # the python bindings. Alter it at the top of fxmemoryops.h

    # Very rarely, scons won't create the object file output directory and
    # thus causes both the following tests to fail. Hence preempt the problem
    if not os.path.exists(builddir):
        os.mkdir(builddir)
    # Configure options always taken by testing
    confA=Configure(env, { "CheckCompilerPtr32" : CheckCompilerPtr32, "CheckCompilerPtr64" : CheckCompilerPtr64, "CheckCompilerIsBigEndian" : CheckCompilerIsBigEndian,
                           "CheckCPP0x_N1720" : CheckCPP0x_N1720, "CheckCPP0x_N2118" : CheckCPP0x_N2118 } )
    if make64bit:
        assert confA.CheckCompilerPtr64()
    else:
        assert confA.CheckCompilerPtr32()
    if confA.CheckCompilerIsBigEndian():
        env['CPPDEFINES']+=[("FOX_BIGENDIAN",1)]
    else:
        env['CPPDEFINES']+=[("FOX_BIGENDIAN",0)]
    if confA.CheckCPP0x_N1720(): env['CPPDEFINES']+=["HAVE_CPP0XSTATICASSERT"]
    if confA.CheckCPP0x_N2118(): env['CPPDEFINES']+=["HAVE_CPP0XRVALUEREFS", "__GXX_EXPERIMENTAL_CXX0X__"]
    env=confA.Finish()
    del confA

    # Library config
    if tcommonopts:
        # Force config for Tn
        if tcommonopts==2:
            env['CPPDEFINES']+=["BUILDING_TCOMMON"]
        FOXCompatLayer=False
        disableFileDirDialogs=True
        disableFindReplaceDialogs=True
        disableMenus=True
        disableMDI=True
        SQLModule=1
        GraphingModule=1
    if make64bit: env['CPPDEFINES']+=["FX_IS64BITPROCESSOR"]
    if makeSMPBuild: env['CPPDEFINES']+=["FX_SMPBUILD"]
    if inlineMutex: env['CPPDEFINES']+=["FXINLINE_MUTEX_IMPLEMENTATION"]
    if FOXCompatLayer: env['CPPDEFINES']+=["FX_FOXCOMPAT"]
    if disableGUI:
        env['CPPDEFINES']+=[("FX_DISABLEGUI", 1)]
        GraphingModule=0
    
    if not disableFileDirDialogs or not disableMDI: disableMenus=False
    if GraphingModule: disableGL=False
    if disableGL: env['CPPDEFINES']+=[("FX_DISABLEGL", 1)]
    if disableFileDirDialogs: env['CPPDEFINES']+=[("FX_DISABLEFILEDIRDIALOGS", 1)]
    if disablePrintDialogs: env['CPPDEFINES']+=[("FX_DISABLEPRINTDIALOGS", 1)]
    if disableFindReplaceDialogs: env['CPPDEFINES']+=[("FX_DISABLEFINDREPLACEDIALOGS", 1)]
    if disableMenus: env['CPPDEFINES']+=[("FX_DISABLEMENUS", 1)]
    if disableMDI: env['CPPDEFINES']+=[("FX_DISABLEMDI", 1)]

    if onWindows:
        # Seems to need this on some installations
        env['ENV']['TMP']=os.environ['TMP']

    if SQLModule:
        env['CPPDEFINES']+=[("FX_SQLMODULE", SQLModule)]
        SQLModuleSources=getSQLModuleSources(prefixpath)
        sqlmoduleobjs=[]
        if os.path.exists(prefixpath+"src/sqlite/sqlite3.h"):
            env['CPPDEFINES']+=[("HAVE_SQLITE3_H", 1)]
            env['CPPPATH']+=[prefixpath+"src/sqlite"]
            sqlmoduleobjs+=[env.SharedObject(builddir+"/sqlite/sqlite3", prefixpath+"src/sqlite/sqlite3.c",
                 CPPDEFINES=env['CPPDEFINES']+[("SQLITE_OMIT_UTF16", 1), ("SQLITE_THREADSAFE", 1), ("HAVE_USLEEP", 1)])]
        else:
            print "SQLite not found in TnFOX sources, disabling support!\n"
            SQLModuleSources=filter(lambda obj: "TnFXSQLDB_sqlite" in obj, SQLModuleSources)
        sqlmoduleobjs+=[env.SharedObject(builddir+"/sqlmodule/"+getBase(x), prefixpath+"src/"+x, CPPDEFINES=env['CPPDEFINES']+[ternary(SQLModule==2, "FX_SQLMODULE_EXPORTS", "FOXDLL_EXPORTS")]) for x in SQLModuleSources]
        libtnfoxsql,libtnfoxsqlsuffix=VersionedSharedLibraryName(env, tnfoxname+"_sql", tnfoxversioninfo, debug=debugmode)
        del SQLModuleSources
    
    if GraphingModule:
        env['CPPDEFINES']+=[("FX_GRAPHINGMODULE", GraphingModule)]
        if os.path.exists(prefixpath+"../VTK/Common/vtkVersion.h"):
            vtkpath=os.path.normpath(prefixpath+"../VTK")
            env['CPPDEFINES']+=[("HAVE_VTK", 1)]
            env['CPPPATH']+=[vtkpath, vtkpath+"/Common", vtkpath+"/Filtering", vtkpath+"/Graphics",
                vtkpath+"/Hybrid", vtkpath+"/Imaging", vtkpath+"/IO", vtkpath+"/Rendering"]
            if debugmode:
                if os.path.exists(vtkpath+"/bin/debug"):
                    vtkbinpath=vtkpath+"/bin/debug"
                elif os.path.exists(vtkpath+"/bin/relwithdebinfo"):
                    vtkbinpath=vtkpath+"/bin/relwithdebinfo"
                elif os.path.exists(vtkpath+"/bin/release"):
                    vtkbinpath=vtkpath+"/bin/release"
                else:
                    raise RuntimeError, "VTK doesn't appear to have been built!"
            else:
                if os.path.exists(vtkpath+"/bin/release"):
                    vtkbinpath=vtkpath+"/bin/release"
                elif os.path.exists(vtkpath+"/bin/relwithdebinfo"):
                    vtkbinpath=vtkpath+"/bin/relwithdebinfo"
                elif os.path.exists(vtkpath+"/bin/debug"):
                    vtkbinpath=vtkpath+"/bin/debug"
                else:
                    raise RuntimeError, "VTK doesn't appear to have been built!"
            env['LIBS']+=[vtkbinpath+"/vtkCommon", vtkbinpath+"/vtkFiltering", vtkbinpath+"/vtkGraphics",
                vtkbinpath+"/vtkHybrid", vtkbinpath+"/vtkImaging", vtkbinpath+"/vtkIO", vtkbinpath+"/vtkRendering"]
            del vtkpath, vtkbinpath
        else:
            print "Local VTK not found"
        graphingmoduleobjs=[env.SharedObject(builddir+"/graphingmodule/"+getBase(x), prefixpath+"src/"+x, CPPDEFINES=env['CPPDEFINES']+[ternary(GraphingModule==2, "FX_GRAPHINGMODULE_EXPORTS", "FOXDLL_EXPORTS"), "HAVE_GL_H", "HAVE_GLU_H"]) for x in getGraphingModuleSources(prefixpath)]
        libtnfoxgraphing,libtnfoxgraphingsuffix=VersionedSharedLibraryName(env, tnfoxname+"_graphing", tnfoxversioninfo, debug=debugmode)

    for key,value in locals().items():
        cglobals[key]=value

def getTnFOXSources(prefix="", getWPOfiles=False):
    filelist=os.listdir(prefix+"src")
    # These aren't to be WPOed
    dontWPO=["FXExceptionDialog.cxx", "FXFunctorTarget.cxx", "FXHandedInterface.cxx", "FXHandedMsgBox.cxx", "FXPrimaryButton.cxx", "TnFXApp.cxx"]
    # These are to be WPOed
    doWPO=["fxascii.cpp", "FXColorNames.cpp", "FXDir.cpp", "FXFile.cpp", "fxfilematch.cpp", "FXIO.cpp", "fxio.cpp", "fxparsegeometry.cpp", "FXPath.cpp", "FXStat.cpp", "FXSystem.cpp", "fxunicode.cpp", "fxutils.cpp", "vsscanf.cpp"]
    idx=0
    while idx<len(filelist):
        type=filelist[idx][-4:]
        if getWPOfiles:
            if (type!=".cxx" or filelist[idx] in dontWPO) and filelist[idx] not in doWPO:
                del filelist[idx]
            elif not onWindows and filelist[idx]=="vsscanf.cpp":
                del filelist[idx]   # Causes issues with glibc
            else: idx+=1
        else:
            if (type!=".cpp" and filelist[idx] not in dontWPO) or filelist[idx] in doWPO:
                del filelist[idx]
            elif not onWindows and filelist[idx]=="vsscanf.cpp":
                del filelist[idx]   # Causes issues with glibc
            else: idx+=1
    sqlsrcs=getSQLModuleSources(prefix)
    filelist=filter(lambda src: not src in sqlsrcs, filelist)
    graphingsrcs=getGraphingModuleSources(prefix)
    filelist=filter(lambda src: not src in graphingsrcs, filelist)
    return filelist
def getTnFOXIncludes(prefix=""):
    filelist=os.listdir(prefix+"include")
    filelist=filter(lambda src: src[-2:]==".h", filelist)
    sqlsrcs=getSQLModuleIncludes(prefix)
    filelist=filter(lambda src: not src in sqlsrcs, filelist)
    graphingsrcs=getGraphingModuleIncludes(prefix)
    filelist=filter(lambda src: not src in graphingsrcs, filelist)
    return filelist
def getSQLModuleSources(prefix=""):
    filelist=os.listdir(prefix+"src")
    filelist=filter(lambda src: "TnFXSQL" in src, filelist)
    return filelist
def getSQLModuleIncludes(prefix=""):
    filelist=os.listdir(prefix+"include")
    filelist=filter(lambda src: "TnFXSQL" in src, filelist)
    return filelist
def getGraphingModuleSources(prefix=""):
    filelist=os.listdir(prefix+"src")
    filelist=filter(lambda src: "TnFXVTKCanvas" in src or "TnFXGraph" in src or "FXGL" in src, filelist)
    return filelist
def getGraphingModuleIncludes(prefix=""):
    filelist=os.listdir(prefix+"include")
    filelist=filter(lambda src: "TnFXVTKCanvas" in src or "TnFXGraph" in src or "FXGL" in src, filelist)
    return filelist
   
def doConfTests(env, prefixpath=""):
    conf=Configure(env)
    def checkLib(conf, name, header, prefix=""):
        capsname=string.upper(name)
        if os.path.exists(prefixpath+"windows/lib"+name+"/lib"+name+ternary(make64bit, "64", "32")+".lib"):
            print "Found "+capsname+" library"
            conf.env['CPPDEFINES']+=["HAVE_"+capsname+"_H"]
            conf.env['CPPPATH']+=[prefixpath+"windows/"+prefix+"lib"+name]
            conf.env['LIBS']+=[prefixpath+"windows/lib"+name+"/lib"+name+ternary(make64bit, "64", "32")]
        elif conf.CheckLibWithHeader(name, ["stdio.h", header], "c"):
            conf.env['CPPDEFINES']+=["HAVE_"+capsname+"_H"]
        else:
            print capsname+" library not found, disabling support"
    haveZLib=False
    if os.path.exists(prefixpath+"windows/zlib/zlib"+ternary(make64bit, "64", "32")+".lib"):
        print "Found ZLib"
        conf.env['CPPDEFINES']+=["HAVE_ZLIB_H"]
        conf.env['CPPPATH']+=[prefixpath+"windows/zlib"]
        conf.env['LIBS']+=[prefixpath+"windows/zlib/zlib"+ternary(make64bit, "64", "32")]
        haveZLib=True
    elif conf.CheckLibWithHeader("z", "zlib.h", "c"):
        conf.env['CPPDEFINES']+=["HAVE_ZLIB_H"]
        haveZLib=True
    else:
        print "ZLib library not found, disabling support"
    if haveZLib:
        checkLib(conf, "tiff", "tiff.h", "libtiff/")
        checkLib(conf, "png", "png.h")
        # Keep jpeg BELOW tiff and png as on some systems they include the jpeg routines
        checkLib(conf, "jpeg", "jpeglib.h")
    if os.path.exists(prefixpath+"windows/libbzip2/libbzip2"+ternary(make64bit, "64", "32")+".lib"):
        print "Found BZip2 library"
        conf.env['CPPDEFINES']+=["HAVE_BZ2LIB_H"]
        conf.env['CPPPATH']+=[prefixpath+"windows/libbzip2"]
        conf.env['LIBS']+=[prefixpath+"windows/libbzip2/libbzip2"+ternary(make64bit, "64", "32")]
    elif conf.CheckLibWithHeader("bz2", "bzlib.h", "c"):
        conf.env['CPPDEFINES']+=["HAVE_BZ2LIB_H"]
    else:
        print "BZip2 library not found, disabling support"

    if os.path.exists(prefixpath+"../openssl/inc32/openssl"):
        print "Found OpenSSL library"
        conf.env['CPPDEFINES']+=["HAVE_OPENSSL"]
        conf.env['CPPPATH']+=[prefixpath+"../openssl/inc32"]
        conf.env['LIBPATH']+=[prefixpath+"../openssl/out32"]
        conf.env['LIBS']+=["libeay"+ternary(make64bit, "64", "32"), "ssleay"+ternary(make64bit, "64", "32")]
    elif conf.CheckLibWithHeader("ssl", ["openssl/ssl.h"], "c", "SSL_library_init();"):
        conf.env['CPPDEFINES']+=["HAVE_OPENSSL"]
        conf.env['LIBS']+=["crypto"]
    else:
        print "OpenSSL library not found, disabling support"

    if conf.CheckLibWithHeader(ternary(onWindows, "opengl32", "GL"), ternary(onWindows, ["windows.h"], [])+["GL/gl.h"], "c", "glBegin(GL_POINTS);", True):
        conf.env['CPPDEFINES']+=["HAVE_GL_H",
                                 "SUN_OGL_NO_VERTEX_MACROS",
                                 "HPOGL_SUPPRESS_FAST_API"]
        if conf.CheckCXXHeader(ternary(onWindows, ["windows.h"], [])+["GL/glu.h"]):
            conf.env['CPPDEFINES']+=["HAVE_GLU_H"]
            conf.env['LIBS']+=[ternary(onWindows, "glu32", "GLU")]
        else:
            print "GLU library not found, disabling support"
    else:
        print "GL library not found, disabling support"

    if not os.path.exists(prefixpath+"../VTK/Common/vtkVersion.h"):
        if conf.CheckLibWithHeader("vtkCommon", ["vtk/Common/vtkVersion.h"], "c++", "vtkVersion::GetVTKVersion();"):
            env['CPPDEFINES']+=[("HAVE_VTK", 1)]
            env['CPPPATH']+=["vtk", "vtk/Common", "vtk/Filtering", "vtk/Graphics"
                "vtk/Hybrid", "vtk/Imaging", "vtk/IO", "vtk/Rendering"]
            env['LIBS']+=["vtkFiltering", "vtkGraphics",
                "vtkHybrid", "vtkImaging", "vtkIO", "vtkRendering"]

    env=conf.Finish()

def addBind(DLL):
    if onWindows:
        def runBind(target, source, env):
            stem, leaf=os.path.split(str(target[0]))
            oldpath=os.getcwd()
            try:
                os.chdir(stem)
                os.system('bind -u '+leaf)
            finally:
                os.chdir(oldpath)
        AddPostAction(DLL, Action(runBind))

def genSynopsis(headersloc, headers):
    syns=["Synopsis_syn/"+x[:string.find(x, '.')]+".syn" for x in headers]
    for idx in range(0, len(headers)):
        os.system("synopsis -p Cxx -D__GNUC__=1 -I"+headersloc+" -o "+syns[idx]+" "+headersloc+"/"+headers[idx])
    return syns

def callSynopsis(syns):
    print "Linking synopsis AST files"
    cmd="synopsis \"-Wl,comment_processors(['qt','java','summary'])\" -o Synopsis_syn/combined.syn "+string.join(syns, " ")
    #print cmd
    os.system(cmd)
    print "Generating HTML"
    os.system("synopsis -f HTML -o Synopsis Synopsis_syn/combined.syn")

    
