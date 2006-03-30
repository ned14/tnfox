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

def VersionedSharedLibraryName(env, target, version, debug=False):
    ABIversion, interface, sourcev, backwards=version
    if ABIversion: target+="-"+ABIversion
    suffix=""
    if debug: target+="d"
    # Hack for scons misinterpreting '.' in target name
    if sys.platform=="win32":
        suffix+=".dll"
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
    libtool=sys.platform!="win32"
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
    
def init(cglobals, prefixpath="", platprefix="", targetversion=0, tcommonopts=0):
    prefixpath=os.path.abspath(prefixpath)+"/"
    platprefix=os.path.abspath(platprefix)+"/"
    #print prefixpath, platprefix
    execfile(prefixpath+"config.py")    # Sets debugmode, version, libtnfoxname
    for key,value in locals().items():
        globals()[key]=value
    if targetversion==0: targetversion=tnfoxversion
    compiler=ARGUMENTS.get("compiler", None)
    global toolset
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
    print "Using platform configuration",platformconfig,"..."
    onWindows=(env['PLATFORM']=="win32" or env['PLATFORM']=="win64")
    if tcommonopts:
        libtcommon,libtcommonsuffix=VersionedSharedLibraryName(env, tncommonname+ternary(disableGUI, "_noGUI", ""), tncommonversioninfo, debug=debugmode)
        libtcommon2,libtcommonsuffix=VersionedSharedLibraryName(env, tncommonname+"Tn"+ternary(disableGUI, "_noGUI", ""), tncommonversioninfo, debug=debugmode)
    else:
        libtnfox,libtnfoxsuffix=VersionedSharedLibraryName(env, tnfoxname+ternary(disableGUI, "_noGUI", ""), tnfoxversioninfo, debug=debugmode)
    if debugmode:
        builddir="Debug_"+tool+"_"+architectureSpec()
    else:
        builddir="Release_"+tool+"_"+architectureSpec()
    env['CPPDEFINES']=[ "FOXDLL" ]
    env['CPPPATH']=[ prefixpath+"include" ]
    env['CPPFLAGS']=[ ]
    env['LIBPATH']=[ ]
    env['LIBS']=[ ]
    env['CCWPOOPTS']=[ ]
    env['LINKFLAGS']=[ ]
    make64bit=(architecture=="x64")
    execfile(platformconfig)
    env['CPPDEFINES']+=[("FOX_BIGENDIAN",ternary(sys.byteorder=="big", 1, 0))]
    if make64bit: env['CPPDEFINES']+=["FX_IS64BITPROCESSOR"]
    if makeSMPBuild: env['CPPDEFINES']+=["FX_SMPBUILD"]
    if inlineMutex: env['CPPDEFINES']+=["FXINLINE_MUTEX_IMPLEMENTATION"]
    if tcommonopts!=2 and FOXCompatLayer: env['CPPDEFINES']+=["FX_FOXCOMPAT"]
    if disableGUI: env['CPPDEFINES']+=[("FX_DISABLEGUI", 1)]
    # WARNING: You must NOT touch FXDISABLE_GLOBALALLOCATORREPLACEMENTS here as it breaks
    # the python bindings. Alter it at the top of fxmemoryops.h
    if tcommonopts==2: env['CPPDEFINES']+=["BUILDING_TCOMMON"]
    if onWindows:
        # Seems to need this on some installations
        env['ENV']['TMP']=os.environ['TMP']

    if os.path.exists(prefixpath+"src/sqlite/sqlite3.h"):
        env['CPPDEFINES']+=[("HAVE_SQLITE3_H", 1)]
        env['CPPPATH']+=[prefixpath+"src/sqlite"]
        # Generate a static library of SQLite and add to ourselves
        filelist=os.listdir(prefixpath+"src/sqlite/src")
        idx=0
        while idx<len(filelist):
            if filelist[idx][-2:]!=".c" or filelist[idx][:3]=="os_":
                del filelist[idx]
            else: idx+=1
        del idx
        if onWindows:
            filelist.append("os_win.c")
        else:
            filelist.append("os_unix.c")
        # Compile SQLite threadsafe with UTF16 support removed
        objects=[env.SharedObject(builddir+"/sqlite/"+getBase(x), prefixpath+"src/sqlite/src/"+x,
            CPPDEFINES=env['CPPDEFINES']+[("SQLITE_OMIT_UTF16", 1), ("THREADSAFE", 1), ("HAVE_USLEEP", 1)]) for x in filelist]
        del filelist
        libsqlite=objects # StaticLibrary(builddir+"/sqlite", objects)
        del objects
    else:
        print "SQLite not found in TnFOX sources, disabling support!\n"
        libsqlite=None

    for key,value in locals().items():
        cglobals[key]=value

def getTnFOXSources(prefix="", getWPOfiles=False):
    filelist=os.listdir(prefix+"src")
    # These aren't to be WPOed
    dontWPO=["FXExceptionDialog.cxx", "FXFunctorTarget.cxx", "FXHandedInterface.cxx", "FXHandedMsgBox.cxx", "FXPrimaryButton.cxx", "TnFXApp.cxx"]
    # These are to be WPOed
    doWPO=["fxfilematch.cpp", "fxutils.cpp", "vsscanf.cpp"]
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
    return filelist
def getTnFOXIncludes(prefix=""):
    filelist=os.listdir(prefix+"include")
    idx=0
    while idx<len(filelist):
        type=filelist[idx][-2:]
        if type!=".h":
            del filelist[idx]
        else: idx+=1
    return filelist
   
def CheckCompilerPtr32(cc):
    cc.Message("Making sure this is really a 32 bit compiler ...")
    result=cc.TryCompile('int main(void)\n{\nint foo[4==sizeof(void *) ? 1 : 0];\n}\n', '.cpp')
    cc.Result(result)
    return result

def CheckCompilerPtr64(cc):
    cc.Message("Making sure this is really a 64 bit compiler ...")
    result=cc.TryCompile('int main(void)\n{\nint foo[8==sizeof(void *) ? 1 : 0];\n}\n', '.cpp')
    cc.Result(result)
    return result

def doConfTests(env, prefixpath=""):
    conf=Configure(env, { "CheckCompilerPtr32" : CheckCompilerPtr32, "CheckCompilerPtr64" : CheckCompilerPtr64 } )
    if make64bit:
        assert conf.CheckCompilerPtr64()
    else:
        assert conf.CheckCompilerPtr32()
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
        checkLib(conf, "jpeg", "jpeglib.h")
        checkLib(conf, "png", "png.h")
        checkLib(conf, "tiff", "tiff.h", "libtiff/")
    if os.path.exists(prefixpath+"windows/libbzip2/libbzip2"+ternary(make64bit, "64", "32")+".lib"):
        print "Found BZip2 library"
        conf.env['CPPDEFINES']+=["HAVE_BZ2LIB_H"]
        conf.env['CPPPATH']+=[prefixpath+"windows/libbzip2"]
        conf.env['LIBS']+=[prefixpath+"windows/libbzip2/libbzip2"+ternary(make64bit, "64", "32")]
    elif conf.CheckLibWithHeader("bzip2", "bzlib.h", "c"):
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

    
