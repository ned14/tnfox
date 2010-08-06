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
env['LINKFLAGS']+=[# "-Wl,--allow-multiple-definition", # You may need this when cross-compiling
                   #"-pg",                             # Profile
                   ternary(make64bit, "-m64", "-m32")
                  ]

if debugmode:
    env['LINKFLAGS']+=[]
else:
    env['LINKFLAGS']+=["-O3"            # Optimise
                       ]

# Set where the BPL *objects* live (which is platform dependent)
BPLObjectsHome="../../boost/bin/boost/libs/python/build/libboost_python.so/gcc/"+ternary(debugmode, "debug", "release")+"/shared-linkable-true"
