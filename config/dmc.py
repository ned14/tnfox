# Digital Mars config

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

# Enable new[]/delete[], bool, exceptions, RTTI, function level linking,
# strings in code seg, switch tables in code seg, Win32 binary, DLL
cppflags=Split('-Aa -Ab -Ae -Ar -Nc -Ns -R -mn -WD')
if debugmode:
    cppflags+=["-g",         # Debug info on
               "-s",         # Stack overflow checking
               "-S",         # Always generate stack frame
               "-gp"         # Pointer validations
               ]
else:
    cppflags+=["-o+all",     # Optimise fully
               "-gl",        # Line no debug info only
               "-5"          # Pentium code please
               ]
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
                  "/LARGEADDRESSAWARE"
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
              "pdh"
              ]
