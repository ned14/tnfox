# GCC config

env['CPPDEFINES']+=["USE_POSIX",
                    "HAVE_CONSTTEMPORARIES"    # Workaround lack of non-const temporaries
                    ]
if debugmode:
    env['CPPDEFINES']+=["_DEBUG"]
else:
    env['CPPDEFINES']+=["NDEBUG"]

def CheckGCCHasVisibility(cc):
    cc.Message("Checking for GCC global symbol visibility support...")
    try:
        temp=cc.env['CPPFLAGS']
    except:
        temp=[]
    cc.env['CPPFLAGS']=temp+["-fvisibility=hidden"]
    result=cc.TryCompile('int main(void) { struct __attribute__ ((visibility("default"))) Foo { int foo; } foo; return 0; }\n', '.c')
    cc.env['CPPFLAGS']=temp
    cc.Result(result)
    return result
conf=Configure(env, { "CheckGCCHasVisibility" : CheckGCCHasVisibility } )

if conf.CheckGCCHasVisibility():
    env['CPPFLAGS']+=["-fvisibility=hidden",        # All symbols are hidden unless marked otherwise
                      "-fvisibility-inlines-hidden" # All inlines are always hidden
                     ]
    env['CPPDEFINES']+=["GCC_HASCLASSVISIBILITY"]
else:
    print "Disabling -fvisibility support"

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
           #"-Winvalid-pch",             # Warning if PCH files can't be used
           "-pipe"                      # Use faster pipes
           ]
if debugmode:
    cppflags+=["-g1",                   # Generate minimal debug info
               "-Os"                    # Optimise for small code
               ]
else:
    cppflags+=["-Os",                   # Optimise for small code
               "-fomit-frame-pointer"   # No frame pointer
               ]
env['CPPFLAGS']+=cppflags


# Linkage
env['LINKFLAGS']=["-fPIC",              # Link shared
                  "-pthread"            # Ensure no libc on BSD
                  ]

if debugmode:
    env['LINKFLAGS']+=[]
else:
    env['LINKFLAGS']+=[ #"-O"             # Optimise
                       ]

# Set where the BPL *objects* live (which is platform dependent)
BPLObjectsHome="../../boost/bin/boost/libs/python/build/libboost_python.so/gcc/"+ternary(debugmode, "debug", "release")+"/shared-linkable-true"
