# MSVC config

env['CPPDEFINES']+=["WIN32",
                    "_WINDOWS",
                    "_USRDLL"
                    ]
if debugmode:
    env['CPPDEFINES']+=["_DEBUG"]
else:
    env['CPPDEFINES']+=["NDEBUG"]

def CheckMSVC71(cc):
    cc.Message("Checking for MSVC7.1 ...")
    result=cc.TryCompile('#if !defined(_MSC_VER) || _MSC_VER<1310\n#error Too old!\n#endif\n', '.cpp')
    cc.Result(result)
    return result
def CheckMSVC80(cc):
    cc.Message("Checking for MSVC8.0 ...")
    result=cc.TryCompile('#if !defined(_MSC_VER) || _MSC_VER<1400\n#error Too old!\n#endif\n', '.cpp')
    cc.Result(result)
    return result
conf=Configure(env, { "CheckMSVC71" : CheckMSVC71, "CheckMSVC80" : CheckMSVC80 } )
MSVCVersion=0
if conf.CheckMSVC71():
    MSVCVersion=710
if conf.CheckMSVC80():
    MSVCVersion=800
assert MSVCVersion>0
env=conf.Finish()

env['CPPPATH']+=[prefixpath+"windows"]
if not os.path.exists(builddir):
    os.mkdir(builddir)

# Warnings, synchronous exceptions, enable RTTI, smallest inlining,
# wchar_t native, types defined before pointers to members used
cppflags=Split('/c /nologo /W3 /EHsc /GR /Ob1 /Ow /Zc:wchar_t /vmb /vmm')
if MSVCVersion==710:
    cppflags+=[ "/Ow"                          # Only functions may alias
              ]
    if architecture=="x86":
        cppflags+=[ "/G%d" % architecture_version ] # Optimise for given processor revision
else:
    # Stop the stupid STDC function deprecated warnings
    env['CPPDEFINES']+=[("_CRT_SECURE_NO_DEPRECATE",1), ("_SECURE_SCL_THROWS", 1)]
    if not debugmode:
        # Prevent checked iterators on release builds
        env['CPPDEFINES']+=[("_SECURE_SCL",0)]
        cppflags+=["/GS-",       # Disable buffer overrun check
                   ]
if architecture=="x86":
    if   x86_SSE==1: cppflags+=[ "/arch:SSE" ]
    elif x86_SSE==2: cppflags+=[ "/arch:SSE2" ]
if debugmode:
    cppflags+=["/O1",        # Optimise for small code (code is very, very big otherwise)
               "/Zd",        # Line number debug info
               "/MDd"        # Select MSVCRTD.dll
               ]
else:
    cppflags+=["/O1",        # Optimise for small code + COMDAT
               "/Oi",        # Enable intrinsics
               "/MD"         # Select MSVCRT.dll
               ]
env['CPPFLAGS']=cppflags

# Sets version, base address
env['LINKFLAGS']=["/version:"+targetversion,
                  "/SUBSYSTEM:WINDOWS",
                  "/DLL",
                  "/DEBUG",
                  "/OPT:NOWIN98",
                  "/INCREMENTAL:NO",      # Incremental linking is just broken on all versions of MSVC
                  "/STACK:524288,65536"
                  ]
if MSVCVersion>=800:
    env['LINKFLAGS']+=["/NXCOMPAT"]
if architecture=="x86" or architecture=="x64":
    if make64bit:
        env['LINKFLAGS']+=["/MACHINE:X64", "/BASE:0x7ff06000000"]
    else:
        env['LINKFLAGS']+=["/MACHINE:X86", "/BASE:0x52000000", "/LARGEADDRESSAWARE"]
if debugmode:
    env['LINKFLAGS']+=["/OPT:NOREF",
                       "/OPT:NOICF"
                       ]
else:
    env['LINKFLAGS']+=["/RELEASE",
                       "/OPT:REF,ICF"    # You may wish to disable ICF on a slow computer
                       ]

env['LIBS']+=["kernel32"]

# Set where the BPL *objects* live (which is platform dependent)
BPLObjectsHome="../../boost/bin/boost/libs/python/build/boost_python.dll/vc-7_1/"+ternary(debugmode, "debug", "release")+"/threading-multi"
