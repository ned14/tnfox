# Simple config file, everything else is automatic

debugmode=False
#if os.environ("TNFOX_DEBUG"):
#    debugmode=True
#else:
#    debugmode=False
make64bit=False          # Set to true if to use a 64 bit memory model
GenStaticLib=False       # Set to true to always generate a static library
SeparateTnLibs=False

architecture="i486"      # Can currently only be "i486"
# =4 for i486, =5 for Pentium, =6 for Pentium Pro/Athlon,
# =7 for Pentium 4/Athlon XP, =8 for AMD64
i486_version=4           
i486_SSE=0               # =0 (disable), =1 (SSE) or =2 (SSE2)
i486_3dnow=0             # =0 (disable), =1 (3dnow)
#i486_version=6           
#i486_SSE=1               # =0 (disable), =1 (SSE) or =2 (SSE2)
#i486_3dnow=1             # =0 (disable), =1 (3dnow)

toolset=None             # Let scons pick which compiler/linker to use
#if sys.platform=="win32":
#    i486_version=6
#    i486_SSE=1
#    toolset=["icl"]
#else:
#    toolset=["intel"]

tnfoxname="TnFOX"
tnfoxversion="0.80"      # Increment each release the interface is removed from
tnfoxinterfaceidx=0      # Increment each release the interface is added to
tnfoxsourceidx=0         # Increment each release the source has changed
tnfoxbackcompatible=tnfoxinterfaceidx     # Probably correct
tnfoxversioninfo=(tnfoxversion, tnfoxinterfaceidx, tnfoxsourceidx, tnfoxbackcompatible)
libtnfoxname=tnfoxname

print
if make64bit:
    if debugmode:
        print "Configured for 64 bit DEBUG build ..."
    else:
        print "Configured for 64 bit RELEASE build ..."
else:
    if debugmode:
        print "Configured for 32 bit DEBUG build ..."
    else:
        print "Configured for 32 bit RELEASE build ..."
