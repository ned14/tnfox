# Simple config file, everything else is automatic

debugmode=True
#if os.environ("TNFOX_DEBUG"):
#    debugmode=True
#else:
#    debugmode=False


######## Global build options ########

# Set to true to create a SMP architecture compatible binary (GCC only).
# This is always safe though inefficient on uniprocessor machines.
makeSMPBuild=True

# Set to true to cause inlining of FXAtomicInt and QMutex in all
# source files. This can slightly increase code size and also drags
# in a lot of extra system header files.
inlineMutex=False  # not debugmode

# Set to true to enable the next generation C++0x features on compilers which
# support that (currently GCC v4.3 or later). Note that this is a profound
# setting and changes compilation of all source code. It can yield significant
# code speed & size improvements.
enableCPP0xFeaturesIfAvailable=True

# What to generate as the library
# 0=generate just the DLL, 1=also generate a static library, 2=only generate a static library
GenStaticLib=not (sys.platform=="win32" or sys.platform=="darwin")

# Set to include FOX compatibility layer
FOXCompatLayer=False

# Set to disable inclusion of GUI ie; no FXObject, no FXApp etc. Disables all of the below.
disableGUI=False

# Set to disable inclusion of OpenGL support (required by the graphing functionality)
disableGL=False

# Set to disable inclusion of file and directory dialogs
disableFileDirDialogs=False

# Set to disable inclusion of print dialogs
disablePrintDialogs=False

# Set to disable inclusion of find and replace dialogs
disableFindReplaceDialogs=False

# Set to disable inclusion of menu functionality (required by file & directory dialogs and MDI interface)
disableMenus=False

# Set to disable the MDI interface
disableMDI=False


# How to include SQL functionality (=0 to disable, =1 to include in core, =2 to build as separate DLL)
SQLModule=2

# How to include the graphing functionality (=0 to disable, =1 to include in core, =2 to build as separate DLL)
GraphingModule=2

SeparateTnLibs=(sys.platform=="win32" or sys.platform=="darwin")

######## End Global build options ########



toolset=None             # Let scons pick which compiler/linker to use
toolsprefix=''           # Useful for cross compiling eg; 'arm9tdmi'

# For example, if you wanted to force a certain toolset below:
#if sys.platform!="win32":
#    architecture_version=6
#    x86_SSE=1
#    if sys.platform=="win32":
#        toolset=["icl"]
#    else:
#        toolset=["intelc"]
#toolset=["intelc"]



######## Processor architecture ########

# architecture can be "generic", "x86", "x64", "ppc" or "macosx-ppc"/"macosx-i386"
# architecture_version can be:
#     For x86: =4 for i486, =5 for Pentium, =6 for Pentium Pro/Athlon, =7 for Pentium 4/Athlon XP
#     For x64: =0 for AMD64/EM64T
#     For ppc: =1 for Power1, =2 for Power2 etc until =6 for PowerPC, =7 for 64 bit PowerPC

# The default, unless overriden below, is to choose x86 or x64
if sys.platform=="win32":
    if os.environ.has_key('PROCESSOR_ARCHITECTURE') and os.environ['PROCESSOR_ARCHITECTURE']=="AMD64":
        architecture="x64"
        architecture_version=0
    else:
        architecture="x86"
        architecture_version=7
        x86_SSE=3               # =0 (disable), =1, 2, 3, 4 etc for SSE variants
        x86_3dnow=0             # =0 (disable), =1 (3dnow)
else:
    import platform
    if 'x64' in platform.machine() or 'x86_64' in platform.machine():
        architecture="x64"
        architecture_version=0
    else:
        architecture="x86"
        architecture_version=7
        x86_SSE=3               # =0 (disable), =1, 2, 3, 4 etc for SSE variants
        x86_3dnow=0             # =0 (disable), =1 (3dnow)


# Define for a "generic" build
#architecture="generic"
#architecture_version=0

# Define for a PowerPC build
#architecture="ppc"
#architecture_version=6

# Define for a PowerPC build (Apple MacOS X only)
#architecture="macosx-ppc"
#architecture_version=0

# Define for an Intel build (Apple MacOS X only)
#architecture="macosx-i386"
#architecture_version=0

# Define for an ARM build. You probably want to set toolsprefix too
#architecture="arm"
#architecture_version=4
#toolsprefix="arm-linux-"





######## End Processor architecture ########


PYTHON_INCLUDE=None      # Sets where the python header  files can be found (=None for get from environment)
PYTHON_LIB=None          # Sets where the python library files can be found (=None for get from environment)

tnfoxname="TnFOX"
tnfoxversion="0.89"      # Increment each release the interface is removed from
tnfoxinterfaceidx=0      # Increment each release the interface is added to
tnfoxsourceidx=0         # Increment each release the source has changed
tnfoxbackcompatible=tnfoxinterfaceidx     # Probably correct
tnfoxversioninfo=(tnfoxversion, tnfoxinterfaceidx, tnfoxsourceidx, tnfoxbackcompatible)
libtnfoxname=tnfoxname

print
if debugmode:
    print "Configured for "+architecture+" DEBUG",
else:
    print "Configured for "+architecture+" RELEASE",
if disableGUI:
    print "(no GUI)",
print "build ..."
