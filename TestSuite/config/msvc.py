# MSVC config

env['CPPDEFINES']+=["WIN32",
                    "_WINDOWS"
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
assert conf.CheckMSVC71()
MSVCVersion=710
if conf.CheckMSVC80():
    MSVCVersion=800
env=conf.Finish()
# Warnings, synchronous exceptions, enable RTTI, pool strings and ANSI for scoping, wchar_t native
cppflags=Split('/c /nologo /W3 /EHsc /GR /GF /Zc:forScope /Zc:wchar_t')
assert architecture=="x86" or architecture=="x64"
if MSVCVersion==710:
    cppflags+=[ "/Ow",                        # Only functions may alias
                "/G%d" % architecture_version # Optimise for given processor revision
              ]
else:
    # Stop the stupid STDC function deprecated warnings
    env['CPPDEFINES']+=[("_CRT_SECURE_NO_DEPRECATE",1)]
if architecture=="x86":
    if   x86_SSE==1: cppflags+=[ "/arch:SSE" ]
    elif x86_SSE==2: cppflags+=[ "/arch:SSE2" ]
if debugmode:
    cppflags+=["/Od",        # Optimisation off
               "/Zi",        # Program database debug info
               #"/O2", "/Oy-",
               "/Gm",        # Minimum rebuild
               "/RTC1",      # Stack and uninit run time checks
               "/RTCc",      # Smaller data type without cast run time check
               "/MDd",       # Select MSVCRTD.dll
               "/GS",        # Buffer overrun check
               "/Fd"+builddir+"/vc70.pdb" # Set PDB location
               ]
else:
    cppflags+=["/O2",        # Optimise for fast code
               #"/Zd",        # Line no debug info only
               "/Zi",
               "/Ob2",       # Inline all suitable
               "/Oi",        # Enable intrinsics
               "/MD"         # Select MSVCRT.dll
               ]
env['CPPFLAGS']=cppflags

# Sets version, base address
env['LINKFLAGS']=["/version:"+tnfoxversion,
                  "/SUBSYSTEM:CONSOLE",
                  "/DEBUG",
                  "/OPT:NOWIN98",
                  "/STACK:524288,65536"
                  ]
if make64bit:
    # This seems to be missing
    env['LINKFLAGS']+=["/FORCE:UNRESOLVED"]
else:
    env['LINKFLAGS']+=["/BASE:0x60000000", "/LARGEADDRESSAWARE"]
if debugmode:
    env['LINKFLAGS']+=["/INCREMENTAL"]
else:
    env['LINKFLAGS']+=["/INCREMENTAL:NO",
                       "/OPT:REF",
                       "/OPT:ICF",
                       "/RELEASE",
                       ]
env['LIBS']+=["kernel32", "user32", libtnfox ]
