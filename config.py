# Simple config file, everything else is automatic

debugmode=False
#if os.environ("TNFOX_DEBUG"):
#    debugmode=True
#else:
#    debugmode=False

### Global build options
# Set to true to create a SMP architecture compatible binary. This
# is always safe though inefficient on uniprocessor machines.
makeSMPBuild=True
# Set to true to cause inlining of FXAtomicInt and FXMutex in all
# source files. This can slightly increase code size and also drags
# in a lot of extra system header files.
inlineMutex=False  # not debugmode
GenStaticLib=(sys.platform!="win32")          # 1=generate a static library, 2=only generate a static library
SeparateTnLibs=(sys.platform=="win32")

architecture="x86"       # Can be "x86" or "x64"
# For x86: =4 for i486, =5 for Pentium, =6 for Pentium Pro/Athlon, =7 for Pentium 4/Athlon XP
architecture_version=4           
x86_SSE=0               # =0 (disable), =1 (SSE) or =2 (SSE2)
x86_3dnow=0             # =0 (disable), =1 (3dnow)
#architecture_version=7
#x86_SSE=1               # =0 (disable), =1 (SSE) or =2 (SSE2)
#x86_3dnow=1             # =0 (disable), =1 (3dnow)

#architecture="x64"
# For x64: =0 for AMD64/EM64T
#architecture_version=0

toolset=None             # Let scons pick which compiler/linker to use
#if sys.platform!="win32":
#    architecture_version=6
#    x86_SSE=1
#    if sys.platform=="win32":
#        toolset=["icl"]
#    else:
#        toolset=["intel"]

PYTHON_INCLUDE=None      # Sets where the python header  files can be found (=None for get from environment)
PYTHON_LIB=None          # Sets where the python library files can be found (=None for get from environment)

tnfoxname="TnFOX"
tnfoxversion="0.86"      # Increment each release the interface is removed from
tnfoxinterfaceidx=0      # Increment each release the interface is added to
tnfoxsourceidx=0         # Increment each release the source has changed
tnfoxbackcompatible=tnfoxinterfaceidx     # Probably correct
tnfoxversioninfo=(tnfoxversion, tnfoxinterfaceidx, tnfoxsourceidx, tnfoxbackcompatible)
libtnfoxname=tnfoxname

print
if debugmode:
    print "Configured for "+architecture+" DEBUG build ..."
else:
    print "Configured for "+architecture+" RELEASE build ..."
