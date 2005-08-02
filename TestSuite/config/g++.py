# GCC config

env['CPPDEFINES']+=["USE_POSIX",
                    "HAVE_CONSTTEMPORARIES"    # Workaround lack of non-const temporaries
                    ]
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
env['LIBS']+=["Xext", "X11"]

# Warnings
cppflags=Split('-Wformat -Wno-reorder -Wno-non-virtual-dtor')
if architecture=="x86":
    if x86_3dnow!=0:
          cppflagsopts=["i486", "k6-2",    "athlon",     "athlon-4" ]
    else: cppflagsopts=["i486", "pentium", "pentiumpro", "pentium4" ]
    cppflags+=["-m32", "-march="+cppflagsopts[architecture_version-4] ]
    if x86_SSE!=0:
        cppflags+=["-mfpmath="+ ["387", "sse"][x86_SSE!=0] ]
        if x86_SSE>1: cppflags+=["-msse%d" % x86_SSE]
        else: cppflags+=["-msse"]
elif architecture=="x64":
    cppflagsopts=["athlon64"]
    cppflags+=["-m64", "-march="+cppflagsopts[architecture_version] ]
else:
    raise IOError, "Unknown architecture type"
cppflags+=["-fexceptions",              # Enable exceptions
           "-pipe"                      # Use faster pipes
           ]
if debugmode:
    cppflags+=["-O0",                   # No optimisation
               "-g"                     # Debug info
               ]
else:
    cppflags+=["-O2",                   # Optimise for fast code
               #"-fno-default-inline",
               #"-fno-inline-functions",
               #"-fno-inline",
               #"-finline-limit=0",
               #"-g",
               "-fomit-frame-pointer"   # No frame pointer
               ]
env['CPPFLAGS']+=cppflags


# Use libtool for linking
if "freebsd" in sys.platform:
    env['LINK']="libtool --tag=CXX --mode=link g++"
else: env['LINK']="libtool --mode=link g++"
env['LINKFLAGS']+=["-Wl,--allow-multiple-definition", # You may need this when cross-compiling
                   ternary(make64bit, "-m64", "-m32")
                  ]

if debugmode:
    env['LINKFLAGS']+=[ #"-static"        # Don't use shared objects
                       ]
else:
    env['LINKFLAGS']+=[ # "-O"             # Optimise
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
conf=Configure(env, { "CheckGCCHasVisibility" : CheckGCCHasVisibility } )

if conf.CheckGCCHasVisibility():
    env['CPPFLAGS']+=["-fvisibility=hidden",        # All symbols are hidden unless marked otherwise
                      "-fvisibility-inlines-hidden" # All inlines are always hidden
                     ]
    env['CPPDEFINES']+=["GCC_HASCLASSVISIBILITY"]
else:
    print "Disabling -fvisibility support"

env=conf.Finish()

