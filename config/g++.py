# GCC config

env['CPPDEFINES']+=["USE_POSIX",
                    "HAVE_CONSTTEMPORARIES"    # Workaround lack of non-const temporaries
                    ]
if onDarwin:
    # You only get the thread cancelling pthread implementation this way
    env['CPPDEFINES']+=[("_APPLE_C_SOURCE", 1)]
if debugmode:
    env['CPPDEFINES']+=["_DEBUG"]
else:
    env['CPPDEFINES']+=["NDEBUG"]

# Include the standard library locations (scons gets these wrong on mixed 32/64 bit)
env['LIBPATH']+=["/usr/"+libPathSpec(make64bit)]
env['LIBPATH']+=["/usr/local/"+libPathSpec(make64bit)]
# Include Kerberos
env['CPPPATH']+=["/usr/kerberos/include"]
# Include X11
env['CPPPATH']+=["/usr/X11R6/include"]
env['LIBPATH']+=["/usr/X11R6/"+libPathSpec(make64bit)]

# Warnings
cppflags=Split('-Wformat -Wno-reorder -Wno-non-virtual-dtor')
if architecture=="x86":
    if x86_3dnow!=0:
          cppflagsopts=["i486", "k6-2",    "athlon",     "athlon-4" ]
    else: cppflagsopts=["i486", "pentium", "pentiumpro", "pentium-m" ]
    cppflags+=["-m32", "-march="+cppflagsopts[architecture_version-4] ]
    if x86_SSE!=0:
        cppflags+=["-mfpmath="+ ["387", "sse"][x86_SSE!=0] ]
        if x86_SSE>1: cppflags+=["-msse%d" % x86_SSE]
        else: cppflags+=["-msse"]
elif architecture=="x64":
    #cppflagsopts=["athlon64"]
    cppflags+=["-m64", "-mfpmath=sse", "-msse2"] #, "-march="+cppflagsopts[architecture_version] ]
elif architecture=="ppc":
    cppflagsopts=["power", "power2", "power3", "power4", "power5", "powerpc", "powerpc64"]
    cppflags+=["-mcpu="+cppflagsopts[architecture_version-1] ]
elif architecture=="macosx-ppc":
    cppflags+=["-arch", "ppc"]
elif architecture=="macosx-i386":
    cppflags+=["-arch", "i386"]

cppflags+=["-fexceptions",              # Enable exceptions
           "-fstrict-aliasing",         # Always enable strict aliasing
           "-fargument-noalias",        # Arguments may alias globals but not each other
           "-Wstrict-aliasing",         # Warn about bad aliasing
           "-ffast-math",               # Lose FP precision in favour of speed
           #"-pg",                       # Perform profiling
           #"-finstrument-functions",    # Other form of profiling
           "-pipe"                      # Use faster pipes
           ]
if debugmode:
    cppflags+=["-O0",                   # No optimisation
               #"-fmudflapth",           # Do memory access checking (doesn't work on Apple MacOS X)
               "-ggdb"                  # Best debug info (better than -g on MacOS X)
               ]
else:
    cppflags+=["-O2",                   # Optimise for fast code
               #"-ftree-vectorize",      # Use vectorisation
               #"-ftree-loop-linear",    # Further optimise loops
               #"-ftree-loop-im",        # Remove loop invariant code
               #"-fivopts",              # Tree induction variable optimisation
               #"-fno-default-inline",
               #"-fno-inline-functions",
               #"-fno-inline",
               #"-finline-limit=0",
               #"-ggdb",
               "-fomit-frame-pointer"   # No frame pointer
               ]
    env['CCWPOOPTS']=[
               #"-fno-default-inline",
               #"-fno-inline-functions"
               #"-fno-inline",
               #"-finline-limit=0"
               ]
env['CPPFLAGS']+=cppflags

# Linkage
env['LINKFLAGS']+=[# "-Wl,--allow-multiple-definition", # You may need this when cross-compiling
                   #"-pg",                             # Profile
                   ternary(make64bit, "-m64", "-m32")
                  ]
if not onDarwin:
    env['LINKFLAGS']+=["-rdynamic"]                   # Keep function names for backtracing
if architecture=="macosx-ppc":
    env['LINKFLAGS']+=["-arch", "ppc"]
elif architecture=="macosx-i386":
    env['LINKFLAGS']+=["-arch", "i386"]

if debugmode:
    env['LINKFLAGS']+=[]
else:
    env['LINKFLAGS']+=["-O3"            # Optimise
                       ]


#if onDarwin:
    # Link against Saturn profiling library
    #env['LIBS']+=[ "Saturn" ]


# Include system libs (mandatory)
env['CPPDEFINES']+=[("STDC_HEADERS",1),
                    ("HAVE_SYS_TYPES_H",1),
                    ("HAVE_STDLIB_H",1),
                    ("HAVE_STRING_H",1),
                    ("HAVE_STRINGS_H",1),
                    ("HAVE_UNISTD_H",1),
                    ("TIME_WITH_SYS_TIME",1),
                    ("HAVE_SYS_WAIT_H",1),
                    ("HAVE_DIRENT_H",1),
                    ("HAVE_SYS_PARAM_H",1),
                    ("HAVE_SYS_SELECT_H",1)
                    ]
env['LIBS']+=[ "m", "stdc++" ]


def CheckGCCHasVisibility(cc):
    cc.Message("Checking for GCC global symbol visibility support...")
    try:
        temp=cc.env['CPPFLAGS']
    except:
        temp=[]
    cc.env['CPPFLAGS']=temp+["-fvisibility=hidden"]
    result=cc.TryCompile('struct __attribute__ ((visibility("default"))) Foo { int foo; };\nint main(void) { Foo foo; return 0; }\n', '.cpp')
    cc.env['CPPFLAGS']=temp
    cc.Result(result)
    return result
conf=Configure(env, { "CheckGCCHasVisibility" : CheckGCCHasVisibility } )
nothreads=True                # NOTE: Needs to be first to force use of kse over c_r on FreeBSD
if conf.CheckCHeader("pthread.h"):
    oldcpppath=conf.env['CPPPATH'][:]
    oldlibpath=conf.env['LIBPATH'][:]
    conf.env['CPPPATH'][0:0]=["/usr/include/nptl"]
    conf.env['LIBPATH'][0:0]=["/usr/"+libPathSpec(make64bit)+"/nptl"]
    if conf.CheckLibWithHeader("pthread", "pthread.h", "C", "pthread_setaffinity_np(0,0,0);"):
        nothreads=False
        conf.env['CPPDEFINES']+=[("HAVE_NPTL",1)]
    else:
        conf.env['CPPPATH']=oldcpppath
        conf.env['LIBPATH']=oldlibpath
        if conf.CheckLibWithHeader("pthread", "pthread.h", "C", "pthread_create(0,0,0,0);") or conf.CheckLibWithHeader("kse", "pthread.h", "C", "pthread_create(0,0,0,0);"):
            nothreads=False
            print "Note that thread processor affinity functionality will not be present!"
if nothreads:
    conf.env['CPPDEFINES']+=["FXDISABLE_THREADS"]
    print "Disabling thread support"
else:
    conf.env['CPPDEFINES']+=[("XTHREADS",1)]
    conf.env['CPPDEFINES']+=[("FOX_THREAD_SAFE",1)]
del nothreads

if not disableGUI:
    if not conf.CheckLib("X11", "XOpenDisplay"):
        raise AssertionError, "TnFOX requires X11"
    if not conf.CheckLib("Xext", "XShmAttach"):
        raise AssertionError, "TnFOX requires X11"

    if conf.CheckCHeader(["X11/Xlib.h", "X11/Xcursor/Xcursor.h"]):
        conf.env['CPPDEFINES']+=[("HAVE_XCURSOR_H",1)]
        conf.env['LIBS']+=["Xcursor"]
    else:
        print "Disabling 32 bit colour cursor support"
    
    conf.env.ParseConfig("/usr/X11R6/bin/xft-config --cflags --libs")
    if not make64bit:
        # Annoyingly xft-config adds lib64 on 64 bit platforms
        for n in range(0, len(conf.env['LIBPATH'])):
            if "lib64" in conf.env['LIBPATH'][n]:
                print "   NOTE: Removing unneccessary library path", conf.env['LIBPATH'][n]
                del conf.env['LIBPATH'][n]
                break
    if conf.CheckCHeader(["X11/Xlib.h", "X11/Xft/Xft.h"]):
        conf.env['CPPDEFINES']+=[("HAVE_XFT_H",1)]
    else:
        # If freetype-config failed, try a boilerplate location
        oldcpppath=conf.env['CPPPATH'][:]
        conf.env['CPPPATH']+=["/usr/X11R6/include/freetype2"]
        if conf.CheckCHeader(["X11/Xlib.h", "X11/Xft/Xft.h"]):
            conf.env['CPPDEFINES']+=[("HAVE_XFT_H",1)]
            conf.env['LIBS']+=["Xft", "fontconfig", "freetype"]
        else:
            conf.env['CPPPATH']=oldcpppath
            print "Disabling anti-aliased fonts support"

if not conf.CheckLib("rt", "shm_open") and not conf.CheckLib("c", "shm_open"):
    raise AssertionError, "TnFOX requires POSIX shared memory support"

if conf.CheckCHeader(["X11/Xlib.h", "sys/shm.h", "sys/ipc.h", "X11/extensions/XShm.h"]):
    conf.env['CPPDEFINES']+=["HAVE_XSHM_H"]
else:
    print "Disabling shared memory support"
    
if conf.CheckLib("dl", "dlopen") or conf.CheckLib("c", "dlopen"):
    conf.env['CPPDEFINES']+=[("HAVE_LIBDL",1)]
else:
    print "Disabling DLL support"

if conf.CheckLibWithHeader("cups", "cups/cups.h", "c"):
    conf.env['CPPDEFINES']+=[("HAVE_CUPS_H",1)]
else:
    print "Disabling CUPS support"
    
if conf.CheckLibWithHeader("pam", "security/pam_appl.h", "c"):
    conf.env['CPPDEFINES']+=[("HAVE_PAM",1)]
elif conf.CheckLibWithHeader("pam", "pam/pam_appl.h", "c"):
    conf.env['CPPDEFINES']+=[("HAVE_PAM",2)]
else:
    print "Disabling PAM support"

# Linux and FreeBSD get crypt() from libcrypt
conf.CheckLib("crypt", "crypt")

if conf.CheckGCCHasVisibility():
    if not debugmode:
        env['CPPFLAGS']+=["-fvisibility=hidden",        # All symbols are hidden unless marked otherwise
                          "-fvisibility-inlines-hidden" # All inlines are always hidden
                         ]
        env['CPPDEFINES']+=["GCC_HASCLASSVISIBILITY"]
else:
    print "Disabling -fvisibility support"

if conf.CheckLibWithHeader("fam", "fam.h", "c"):
    conf.env['CPPDEFINES']+=[("HAVE_FAM",1)]
else:
    print "Disabling FAM support"

env=conf.Finish()

