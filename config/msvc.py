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
assert conf.CheckMSVC71()
MSVCVersion=710
if conf.CheckMSVC80():
    MSVCVersion=800
env=conf.Finish()

env['CPPPATH']+=[prefixpath+"windows"]

# Warnings, synchronous exceptions, enable RTTI, pool strings, ANSI for scoping,
# types defined before pointers to members used
cppflags=Split('/c /nologo /W3 /EHsc /GR /GF /Zc:forScope /vmb /vmm')
assert architecture=="i486"
if MSVCVersion==710:
    cppflags+=[ "/Ow",                   # Only functions may alias
                "/G%d" % i486_version    # Optimise for given processor revision
              ]
else:
    # Stop the stupid STDC function deprecated warnings
    env['CPPDEFINES']+=[("_CRT_SECURE_NO_DEPRECATE",1)]
if   i486_SSE==1: cppflags+=[ "/arch:SSE" ]
elif i486_SSE==2: cppflags+=[ "/arch:SSE2" ]
if debugmode:
    cppflags+=["/w34701",    # Report uninitialized variable use
               "/Od",        # Optimisation off
               #"/O2", "/Oy-",
               "/Zi",        # Program database debug info
               ####"/GL",
               "/Gm",        # Minimum rebuild
               "/RTC1",      # Stack and uninit run time checks
               ####"/RTCc",      # Smaller data type without cast run time check
               "/GS",        # Buffer overrun check
               "/MDd",       # Select MSVCRTD.dll
               "/Fd"+builddir+"/vc70.pdb" # Set PDB location
               ]
    #env['CCWPOOPTS']=["/W4"] # Maximum warnings for TnFOX files only
else:
    cppflags+=["/O2",        # Optimise for fast code
               "/Zd",        # Line no debug info only
               #"/Zi", #"/Og-",
               "/MD"         # Select MSVCRT.dll
               ]
    #env['CCWPOOPTS']=["/Og-"]
env['CPPFLAGS']=cppflags

# Sets version, base address
env['LINKFLAGS']=["/version:"+targetversion,
                  "/BASE:0x60000000",
                  "/SUBSYSTEM:WINDOWS",
                  "/MAP:"+builddir+"\\TnFOXdll.map",
                  "/DLL",
                  "/MACHINE:X86",
                  "/DEBUG",
                  "/OPT:NOWIN98",
                  "/LARGEADDRESSAWARE",
                  "/STACK:524288,65536"
                  ]
if debugmode:
    if MSVCVersion==710:
        env['LINKFLAGS']+=["/INCREMENTAL:NO"]
    else:
        env['LINKFLAGS']+=["/INCREMENTAL"]
    env['LINKFLAGS']+=["/NODEFAULTLIB:MSVCRT"]
else:
    env['LINKFLAGS']+=["/INCREMENTAL:NO",
                       "/OPT:REF",
                       "/OPT:ICF",
                       "/RELEASE",
                       "/DELAYLOAD:opengl32.dll",
                       "/DELAYLOAD:glu32.dll"
                       ]
env['LIBS']+=["kernel32", "user32", "gdi32", "advapi32", "shell32",
              "comctl32", "winspool",
              "delayimp", "wsock32", "ws2_32", "psapi", "dbghelp",
              "pdh", "netapi32", "secur32", "userenv"
              ]
