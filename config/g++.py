# GCC config

env['CPPDEFINES']+=["USE_POSIX",
                    "HAVE_CONSTTEMPORARIES"    # Workaround lack of non-const temporaries
                    ]
if debugmode:
    env['CPPDEFINES']+=["_DEBUG"]
else:
    env['CPPDEFINES']+=["NDEBUG"]

# Include Kerberos
env['CPPPATH']+=["/usr/kerberos/include"]
# Include X11
env['CPPPATH']+=["/usr/X11R6/include"]
env['LIBPATH']+=["/usr/X11R6/lib"]
env['LIBS']+=["Xext", "X11"]

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
    if conf.CheckLib("pthread", "pthread_create", "pthread.h") or conf.CheckLib("kse", "pthread_create", "pthread.h"):
        conf.env['CPPDEFINES']+=[("XTHREADS",1)]
        conf.env['CPPDEFINES']+=[("FOX_THREAD_SAFE",1)]
        nothreads=False
if nothreads:
    conf.env['CPPDEFINES']+=["FXDISABLE_THREADS"]
    print "Disabling thread support"
del nothreads

if not conf.CheckLib("rt", "shm_open") and not conf.CheckLib("c", "shm_open"):
    raise AssertionError, "TnFOX requires POSIX shared memory support"

if conf.CheckCHeader(["X11/Xlib.h", "sys/shm.h", "sys/ipc.h", "X11/extensions/XShm.h"]):
    conf.env['CPPDEFINES']+=["HAVE_XSHM_H"]
else:
    print "Disabling shared memory support"
    
if conf.CheckCHeader(["X11/Xlib.h", "X11/Xcursor/Xcursor.h"]):
    conf.env['CPPDEFINES']+=[("HAVE_XCURSOR_H",1)]
    conf.env['LIBS']+=["Xcursor"]
else:
   print "Disabling 32 bit colour cursor support"
    
conf.env.ParseConfig("freetype-config --cflags --libs")
if conf.CheckCHeader(["X11/Xlib.h", "X11/Xft/Xft.h"]):
   conf.env['CPPDEFINES']+=[("HAVE_XFT_H",1)]
   conf.env['LIBS']+=["Xft"]
else:
   print "Disabling anti-aliased fonts support"

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
else:
    print "Disabling PAM support"

if conf.CheckGCCHasVisibility():
    env['CPPFLAGS']+=["-fvisibility=hidden",        # All symbols are hidden unless marked otherwise
                      "-fvisibility-inlines-hidden" # All inlines are always hidden
                     ]
    env['CPPDEFINES']+=["GCC_HASCLASSVISIBILITY"]
else:
    print "Disabling -fvisibility support"

if conf.CheckLibWithHeader("fam", "fam.h", "c"):
    pass
else:
    raise "TnFOX::FXFSMonitor needs the FAM library"

env=conf.Finish()

# Warnings
cppflags=Split('-Wformat -Wno-reorder -Wno-non-virtual-dtor')
if architecture=="i486":
    if i486_3dnow!=0:
          cppflagsopts=["i486", "k6-2",    "athlon",     "athlon-4", "athlon64" ]
    else: cppflagsopts=["i486", "pentium", "pentiumpro", "pentium4", None ]
    cppflags+=["-march="+cppflagsopts[i486_version-4] ]
    if i486_SSE!=0:
        cppflags+=["-mfpmath="+ ["387", "sse"][i486_SSE!=0] ]
        if i486_SSE>1: cppflags+=["-msse%d" % i486_SSE]
        else: cppflags+=["-msse"]
    if make64bit: cppflags+=["-m64"]
else:
    cppflags+=[ "-march="+architecture ]
cppflags+=["-fexceptions",              # Enable exceptions
           "-pipe"                      # Use faster pipes
           ]
if debugmode:
    cppflags+=["-O0",                   # No optimisation
               "-g"                     # Debug info
               ]
else:
    cppflags+=["-O2",                   # Optimise for fast code
               "-fomit-frame-pointer"   # No frame pointer
               #"-fno-default-inline",
               #"-fno-inline-functions",
               #"-fno-inline",
               #"-finline-limit=0",
               #"-g"
               ]
env['CPPFLAGS']+=cppflags


# Linkage
env['LINKFLAGS']=[ #"-Wl,--version-script=gnuld.script" # Specify symbol exports
                  ]

if debugmode:
    env['LINKFLAGS']+=[]
else:
    env['LINKFLAGS']+=["-O"             # Optimise
                       ]

# Include system libs (mandatory)
env['CPPDEFINES']+=[("STDC_HEADERS",1),
                    ("HAVE_SYS_TYPES_H",1),
                    ("HAVE_SYS_STAT_H",1),
                    ("HAVE_STDLIB_H",1),
                    ("HAVE_STRING_H",1),
                    ("HAVE_MEMORY_H",1),
                    ("HAVE_STRINGS_H",1),
                    ("HAVE_INTTYPES_H",1),
                    ("HAVE_STDINT_H",1),
                    ("HAVE_UNISTD_H",1),
                    ("HAVE_DLFCN_H",1),
                    ("TIME_WITH_SYS_TIME",1),
                    ("HAVE_SYS_WAIT_H",1),
                    ("HAVE_DIRENT_H",1),
                    ("HAVE_UNISTD_H",1),
                    ("HAVE_SYS_PARAM_H",1),
                    ("HAVE_SYS_SELECT_H",1)
                    ]
env['LIBS']+=["m", "stdc++", "crypt" ]
