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
# Include Kerberos
env['CPPPATH']+=["/usr/kerberos/include"]
# Include X11
env['CPPPATH']+=["/usr/X11R6/include"]

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
env['CPPFLAGS']+=cppflags


# Use libtool for linking
if not onDarwin:
    if "freebsd" in sys.platform:
        env['LINK']="libtool --tag=CXX --mode=link g++"
    else: env['LINK']="libtool --mode=link g++"
env['LINKFLAGS']+=[# "-Wl,--allow-multiple-definition", # You may need this when cross-compiling
                   #"-pg",                             # Profile
                   ternary(make64bit, "-m64", "-m32")
                  ]
if not onDarwin:
    env['LINKFLAGS']+=[ "-rdynamic"]                  # Keep function names for backtracing
if architecture=="macosx-ppc":
    env['LINKFLAGS']+=["-arch", "ppc"]
elif architecture=="macosx-i386":
    env['LINKFLAGS']+=["-arch", "i386"]

if debugmode:
    env['LINKFLAGS']+=[ # "-static"        # Don't use shared objects
                       ]
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
env['LIBS']+=["m"]


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
def CheckGCCHasCPP0xFeatures(cc):
    cc.Message("Checking if GCC can enable C++0x features ...")
    try:
        temp=cc.env['CPPFLAGS']
    except:
        temp=[]
    cc.env['CPPFLAGS']=temp+["-std=c++0x"]
    result=cc.TryCompile('struct Foo { static const int gee=5; Foo(const char *) { } Foo(Foo &&a) { } };\nint main(void) { Foo foo(__func__); static_assert(Foo::gee==5, "Death!"); return 0; }\n', '.cpp')
    cc.env['CPPFLAGS']=temp
    cc.Result(result)
    return result
conf=Configure(env, { "CheckGCCHasVisibility" : CheckGCCHasVisibility, "CheckGCCHasCPP0xFeatures" : CheckGCCHasCPP0xFeatures } )

# Disabled to allow GCC backtrace support
#if conf.CheckGCCHasVisibility():
#    env['CPPFLAGS']+=["-fvisibility=hidden",        # All symbols are hidden unless marked otherwise
#                      "-fvisibility-inlines-hidden" # All inlines are always hidden
#                     ]
#    env['CPPDEFINES']+=["GCC_HASCLASSVISIBILITY"]
#else:
#    print "Disabling -fvisibility support"

if enableCPP0xFeaturesIfAvailable and conf.CheckGCCHasCPP0xFeatures():
    env['CPPFLAGS']+=["-std=c++0x"]
env['CPPDEFINES']+=["HAVE_CONSTTEMPORARIES"]

env=conf.Finish()

