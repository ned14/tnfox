# MSVC config

env['CPPDEFINES']+=["WIN32",
                    "_WINDOWS",
                    "UNICODE"
                    ]
if debugmode:
    env['CPPDEFINES']+=["DEBUG"]
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
confM=Configure(env, { "CheckMSVC71" : CheckMSVC71, "CheckMSVC80" : CheckMSVC80 } )
MSVCVersion=0
if confM.CheckMSVC71():
    MSVCVersion=710
if confM.CheckMSVC80():
    MSVCVersion=800
assert MSVCVersion>0
env=confM.Finish()
del confM
# Warnings, synchronous exceptions, enable RTTI, pool strings, separate COMDAT per function,
# ANSI for scoping, wchar_t native, types defined before pointers to members used
cppflags=Split('/W3 /EHsc /GR /GF /Gy /Zc:forScope /Zc:wchar_t /vmb /vmm')
if MSVCVersion==710:
    cppflags+=[ "/Ow",                         # Only functions may alias
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
    env['CPPDEFINES']+=[("_CRT_SECURE_NO_DEPRECATE",1)]
    if not debugmode:
        # Prevent checked iterators on release builds
        env['CPPDEFINES']+=[("_SECURE_SCL",0)]
        cppflags+=[#"/GS-",       # Disable buffer overrun check (disabled as code is actually faster with it on)
                   ]
if architecture=="x86":
    if   x86_SSE==1: cppflags+=[ "/arch:SSE" ]
    elif x86_SSE>=2: cppflags+=[ "/arch:SSE2" ]
    if x86_SSE>=3: env['CPPDEFINES']+=[("__SSE3__", 1)]
    if x86_SSE>=4: env['CPPDEFINES']+=[("__SSE4__", 1)]
if debugmode:
    if env.GetOption('num_jobs')>1:
        cppflags+=["/Z7"]    # Put debug info into .obj files
    else:
        cppflags+=["/Zi"]    # Program database debug info
    cppflags+=["/w34701",    # Report uninitialized variable use
               "/Od",        # Optimisation off
               #"/O2", "/Oy-",
               "/RTC1",      # Stack and uninit run time checks
               "/RTCc",      # Smaller data type without cast run time check
               "/GS",        # Buffer overrun check
               "/MDd"        # Select MSVCRTD.dll
               ]
else:
    if env.GetOption('num_jobs')>1:
        cppflags+=["/Z7"]    # Put debug info into .obj files
    else:
        cppflags+=["/Zi"]    # Program database debug info
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
    env['LINKFLAGS']+=["/NXCOMPAT", "/DYNAMICBASE", "/MANIFEST", "/TSAWARE"]
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
