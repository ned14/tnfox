# MSVC config

env['CPPDEFINES']+=["WIN32",
                    "_WINDOWS",
                    "_USRDLL"
                    ]
if debugmode:
    env['CPPDEFINES']+=["_DEBUG"]
else:
    env['CPPDEFINES']+=["NDEBUG"]

# Warnings, Synchronous exceptions, enable RTTI, smallest inlining,
# only functions may alias, types defined before pointers to members used,
# virtual inheritance not used
cppflags=Split('/c /nologo /W3 /EHsc /GR /Ob1 /Ow /vmb /vmm /vd0')
assert architecture=="i486"
cppflags+=[ "/G%d" % i486_version ]
if   i486_SSE==1: cppflags+=[ "/arch:SSE" ]
elif i486_SSE==2: cppflags+=[ "/arch:SSE2" ]
if debugmode:
    cppflags+=["/O1",        # Optimise for small code (code is very, very big otherwise)
               "/MDd"        # Select MSVCRTD.dll
               ]
else:
    cppflags+=["/O1",        # Optimise for small code + COMDAT
               "/Oi",        # Enable intrinsics
               "/MD"         # Select MSVCRT.dll
               ]
env['CPPFLAGS']=cppflags

# Sets version, base address
env['LINKFLAGS']=["/version:"+tnfoxversion,
                  "/BASE:0x52000000",
                  "/SUBSYSTEM:WINDOWS",
                  "/DLL",
                  "/MACHINE:X86",
                  "/OPT:NOWIN98",
                  "/INCREMENTAL:NO",
                  "/LARGEADDRESSAWARE",
                  "/STACK:524288,65536"
                  ]
if debugmode:
    env['LINKFLAGS']+=["/DEBUG",
                       "/OPT:NOREF",
                       "/OPT:NOICF"
                       ]
else:
    env['LINKFLAGS']+=["/RELEASE",
                       "/OPT:REF,ICF"    # You may wish to disable ICF on a slow computer
                       ]

env['LIBS']+=["kernel32"]

# Set where the BPL *objects* live (which is platform dependent)
BPLObjectsHome="../../boost/bin/boost/libs/python/build/boost_python.dll/vc-7_1/"+ternary(debugmode, "debug", "release")+"/threading-multi"
