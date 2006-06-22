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

# Set to disable SQL functionality
disableSQL=False

# Set to disable the graphing functionality
disableGraphing=False

SeparateTnLibs=(sys.platform=="win32" or sys.platform=="darwin")

######## End Global build options ########




######## Processor architecture ########

architecture="x86"       # Can be "x86" or "x64"
# For x86: =4 for i486, =5 for Pentium, =6 for Pentium Pro/Athlon, =7 for Pentium 4/Athlon XP
#architecture_version=4           
#x86_SSE=0               # =0 (disable), =1 (SSE) or =2 (SSE2)
#x86_3dnow=0             # =0 (disable), =1 (3dnow)
architecture_version=7
x86_SSE=1               # =0 (disable), =1 (SSE) or =2 (SSE2)
x86_3dnow=1             # =0 (disable), =1 (3dnow)

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

######## End Processor architecture ########


PYTHON_INCLUDE=None      # Sets where the python header  files can be found (=None for get from environment)
PYTHON_LIB=None          # Sets where the python library files can be found (=None for get from environment)

tnfoxname="TnFOX"
tnfoxversion="0.87"      # Increment each release the interface is removed from
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
