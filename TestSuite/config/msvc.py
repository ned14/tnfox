# MSVC config

env['CPPDEFINES']+=["WIN32",
                    "_WINDOWS"
                    ]
if debugmode:
    env['CPPDEFINES']+=["_DEBUG"]
else:
    env['CPPDEFINES']+=["NDEBUG"]

# Warnings, synchronous exceptions, enable RTTI, pool strings and ANSI for scoping
cppflags=Split('/c /nologo /W3 /EHsc /GR /GF /Zc:forScope')
assert architecture=="i486"
cppflags+=[ "/G%d" % i486_version ]
if   i486_SSE==1: cppflags+=[ "/arch:SSE" ]
elif i486_SSE==2: cppflags+=[ "/arch:SSE2" ]
if debugmode:
    cppflags+=["/Od",        # Optimisation off
               "/ZI",        # Edit & Continue debug info
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
                  "/MACHINE:X86",
                  "/DEBUG",
                  "/OPT:NOWIN98",
                  "/LARGEADDRESSAWARE"
                  ]
if debugmode:
    env['LINKFLAGS']+=["/INCREMENTAL"]
else:
    env['LINKFLAGS']+=["/INCREMENTAL:NO",
                       "/OPT:REF",
                       "/OPT:ICF",
                       "/RELEASE",
                       ]
env['LIBS']+=["kernel32", "user32", libtnfox ]
