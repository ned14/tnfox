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
MSVCVersion=0
if conf.CheckMSVC71():
    MSVCVersion=710
if conf.CheckMSVC80():
    MSVCVersion=800
assert MSVCVersion>0
env=conf.Finish()
# Warnings, synchronous exceptions, enable RTTI, pool strings and ANSI for scoping, wchar_t native
cppflags=Split('/c /nologo /W3 /EHsc /GR /GF /Zc:forScope /Zc:wchar_t')
if MSVCVersion==710:
    cppflags+=[ "/Ow",                        # Only functions may alias
                "/Fd"+builddir+"/vc70.pdb"     # Set PDB location
              ]
    if architecture=="x86":
        cppflags+=[ "/G%d" % architecture_version ] # Optimise for given processor revision
else:
    cppflags+=[ "/fp:fast",                    # Fastest floating-point performance
                ###"/Gm",                         # Minimum rebuild (seriously broken on MSVC8)
                "/Fd"+builddir+"/vc80.pdb"     # Set PDB location
              ]
    # Stop the stupid STDC function deprecated warnings
    env['CPPDEFINES']+=[("_CRT_SECURE_NO_DEPRECATE",1), ("_SECURE_SCL_THROWS", 1)]
    if not debugmode:
        # Prevent checked iterators on release builds
        env['CPPDEFINES']+=[("_SECURE_SCL",0)]
        cppflags+=["/GS-",       # Disable buffer overrun check
                   ]
if architecture=="x86":
    if   x86_SSE==1: cppflags+=[ "/arch:SSE" ]
    elif x86_SSE>=2: cppflags+=[ "/arch:SSE2" ]
    if x86_SSE>=3: env['CPPDEFINES']+=[("__SSE3__", 1)]
    if x86_SSE>=4: env['CPPDEFINES']+=[("__SSE4__", 1)]
if debugmode:
    cppflags+=["/Od",        # Optimisation off
               "/Zi",        # Program database debug info
               #"/O2", "/Oy-",
               "/RTC1",      # Stack and uninit run time checks
               "/RTCc",      # Smaller data type without cast run time check
               "/MDd",       # Select MSVCRTD.dll
               "/GS"         # Buffer overrun check
               ]
else:
    cppflags+=["/O2",        # Optimise for fast code
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
                  "/STACK:524288,65536"
                  ]
if MSVCVersion>=800:
    env['LINKFLAGS']+=["/NXCOMPAT"]
if architecture=="x86" or architecture=="x64":
    if make64bit:
        env['LINKFLAGS']+=["/MACHINE:X64"]
    else:
        env['LINKFLAGS']+=["/MACHINE:X86", "/LARGEADDRESSAWARE"]
if debugmode:
    env['LINKFLAGS']+=["/INCREMENTAL"]
else:
    env['LINKFLAGS']+=["/INCREMENTAL:NO",
                       "/OPT:REF",
                       "/OPT:ICF",
                       "/RELEASE",
                       ]
env['LIBS']+=["kernel32", "user32", "gdi32"]
#env['LIBS']+=["advapi32", "shell32", "gdi32",
#              "winspool",
#              "delayimp", "wsock32", "ws2_32", "psapi", "dbghelp",
#              "pdh", "netapi32", "secur32", "userenv"
#              ]
#if not disableGUI:
#    env['LIBS']+=["comctl32"]
