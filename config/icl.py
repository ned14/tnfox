# Intel C++ compiler config

env['CPPDEFINES']+=["WIN32",
                    "_WINDOWS",
                    "_USRDLL"
                    ]
if debugmode:
    env['CPPDEFINES']+=["_DEBUG"]
else:
    env['CPPDEFINES']+=["NDEBUG"]

conf=Configure(env)
env=conf.Finish()

env['CPPPATH']+=[prefixpath+"windows"]

# Warnings, synchronous exceptions, enable RTTI, pool strings,
# ANSI for scoping, ANSI aliasing
cppflags=Split('/c /nologo /W3 /EHsc /GR /GF /Zc:forScope /Qansi_alias')
assert architecture=="i486"
cppflags+=[ "/G%d" % i486_version ]
if   i486_SSE==1: cppflags+=[ "/arch:SSE" ]
elif i486_SSE==2: cppflags+=[ "/arch:SSE2" ]
if debugmode:
    cppflags+=[#"/Od",        # Optimisation off
               #"/O3", "/Ob2",
               #"/Zi",        # Program database debug info
               #"/Qinline_debug_info", # Maximum debug info
               "/Qprof_gen", # Generate an instrumented binary
               #"/Ge",        # Function stack checking
               #"/RTCs",      # More stack checking
               #"/GS",        # Buffer overrun check
               #"/GZ",        # Init all auto vars
               "/MDd"        # Select MSVCRTD.dll
               ]
    #env['CCWPOOPTS']=["/Qipo"]# Interprocedural Optimisation
else:
    cppflags+=["/O3",        # Optimise for fast code
               #"/Zd",        # Line no debug info only
               "/Zi",
               "/Ob2",       # Inline all suitable
               "/MD"         # Select MSVCRT.dll
               ]
    env['CCWPOOPTS']=["/Qprof_use"] # Profile Guided Optimisation
    if i486_version<6:
        cppflags+=["/QaxW"]  # Also produce specialisation for Athlon/Pentium 4
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
    env['LINKFLAGS']+=["/INCREMENTAL:NO",
                       "/NODEFAULTLIB:MSVCRT"
                       ]
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
              "pdh", "netapi32", "secur32"
              ]
