# Copyright (C) 2001-2016 Artifex Software, Inc.
# All Rights Reserved.
#
# This software is provided AS-IS with no warranty, either express or
# implied.
#
# This software is distributed under license and may not be copied,
# modified or distributed except as expressly authorized under the terms
# of the license contained in the file LICENSE in this distribution.
#
# Refer to licensing information at http://www.artifex.com or contact
# Artifex Software, Inc.,  7 Mt. Lassen Drive - Suite A-134, San Rafael,
# CA  94903, U.S.A., +1(415)492-9861, for further information.
#
#
# $Id: msvc32.mak 12087 2011-02-01 11:57:26Z robin $
# makefile for 32-bit Microsoft Visual C++, Windows NT or Windows 95 platform.
#
# All configurable options are surrounded by !ifndef/!endif to allow
# preconfiguration from within another makefile.
#
# Optimization /O2 seems OK with MSVC++ 4.1, but not with 5.0.
# Created 1997-01-24 by Russell Lang from MSVC++ 2.0 makefile.
# Enhanced 97-05-15 by JD
# Common code factored out 1997-05-22 by L. Peter Deutsch.
# Made pre-configurable by JD 6/4/98
# Revised to use subdirectories 1998-11-13 by lpd.

# Note: If you are planning to make self-extracting executables,
# see winint.mak to find out about third-party software you will need.

# If we are building MEMENTO=1, then adjust default debug flags
!if "$(MEMENTO)"=="1"
!ifndef DEBUG
DEBUG=1
!endif
!ifndef TDEBUG
TDEBUG=1
!endif
!ifndef DEBUGSYM
DEBUGSYM=1
!endif
!endif

# If we are building PROFILE=1, then adjust default debug flags
!if "$(PROFILE)"=="1"
!ifndef DEBUG
DEBUG=0
!endif
!ifndef TDEBUG
TDEBUG=0
!endif
!ifndef DEBUGSYM
DEBUGSYM=1
!endif
!endif

# Pick the target architecture file
!if "$(TARGET_ARCH_FILE)"==""
!ifdef WIN64
TARGET_ARCH_FILE=$(GLSRCDIR)\..\arch\windows-x64-msvc.h
!else
!ifdef ARM
TARGET_ARCH_FILE=$(GLSRCDIR)\..\arch\windows-arm-msvc.h
!else
TARGET_ARCH_FILE=$(GLSRCDIR)\..\arch\windows-x86-msvc.h
!endif
!endif
!endif

# ------------------------------- Options ------------------------------- #

###### This section is the only part of the file you should need to edit.

# ------ Generic options ------ #

# Define the directory for the final executable, and the
# source, generated intermediate file, and object directories
# for the graphics library (GL) and the PostScript/PDF interpreter (PS).

!if "$(MEMENTO)"=="1"
DEFAULT_OBJ_DIR=.\$(PRODUCT_PREFIX)memobj
!else
!if "$(PROFILE)"=="1"
DEFAULT_OBJ_DIR=.\$(PRODUCT_PREFIX)profobj
!else
!if "$(DEBUG)"=="1"
DEFAULT_OBJ_DIR=.\$(PRODUCT_PREFIX)debugobj
!else
DEFAULT_OBJ_DIR=.\$(PRODUCT_PREFIX)obj
!endif
!endif
!endif
!ifdef METRO
DEFAULT_OBJ_DIR=$(DEFAULT_OBJ_DIR)rt
!endif
!ifdef WIN64
DEFAULT_OBJ_DIR=$(DEFAULT_OBJ_DIR)64
!endif

!ifndef AUXDIR
AUXDIR=$(DEFAULT_OBJ_DIR)\aux_
!endif

# Note that 32-bit and 64-bit binaries reside in a common directory
# since the names are unique
!ifndef BINDIR
!if "$(MEMENTO)"=="1"
BINDIR=.\membin
!else
!if "$(DEBUG)"=="1"
BINDIR=.\debugbin
!else
!if "$(DEBUGSYM)"=="1"
BINDIR=.\profbin
!else
BINDIR=.\bin
!endif
!endif
!endif
!endif
!ifndef GLSRCDIR
GLSRCDIR=.\base
!endif
!ifndef GLGENDIR
GLGENDIR=$(DEFAULT_OBJ_DIR)
!endif
!ifndef DEVSRCDIR
DEVSRCDIR=.\devices
!endif
!ifndef GLOBJDIR
GLOBJDIR=$(DEFAULT_OBJ_DIR)
!endif
!ifndef DEVGENDIR
DEVGENDIR=$(DEFAULT_OBJ_DIR)
!endif
!ifndef DEVOBJDIR
DEVOBJDIR=$(DEFAULT_OBJ_DIR)
!endif

!ifndef PSSRCDIR
PSSRCDIR=.\psi
!endif
!ifndef PSLIBDIR
PSLIBDIR=.\lib
!endif
!ifndef PSRESDIR
PSRESDIR=.\Resource
!endif
!ifndef PSGENDIR
PSGENDIR=$(DEFAULT_OBJ_DIR)
!endif
!ifndef PSOBJDIR
PSOBJDIR=$(DEFAULT_OBJ_DIR)
!endif
!ifndef SBRDIR
SBRDIR=$(DEFAULT_OBJ_DIR)
!endif

!ifndef EXPATGENDIR
EXPATGENDIR=$(GLGENDIR)
!endif

!ifndef EXPATOBJDIR
EXPATOBJDIR=$(GLGENDIR)
!endif

!ifndef JPEGXR_GENDIR
JPEGXR_GENDIR=$(GLGENDIR)
!endif

!ifndef JPEGXR_OBJDIR
JPEGXR_OBJDIR=$(GLGENDIR)
!endif


!ifndef PCL5SRCDIR
PCL5SRCDIR=.\pcl\pcl
!endif

!ifndef PCL5GENDIR
PCL5GENDIR=.\$(DEFAULT_OBJ_DIR)
!endif

!ifndef PCL5OBJDIR
PCL5OBJDIR=.\$(DEFAULT_OBJ_DIR)
!endif

!ifndef PXLSRCDIR
PXLSRCDIR=.\pcl\pxl
!endif

!ifndef PXLGENDIR
PXLGENDIR=.\$(DEFAULT_OBJ_DIR)
!endif

!ifndef PXLOBJDIR
PXLOBJDIR=.\$(DEFAULT_OBJ_DIR)
!endif

!ifndef PLSRCDIR
PLSRCDIR=.\pcl\pl
!endif

!ifndef PLGENDIR
PLGENDIR=.\$(DEFAULT_OBJ_DIR)
!endif

!ifndef PLOBJDIR
PLOBJDIR=.\$(DEFAULT_OBJ_DIR)
!endif

!ifndef XPSSRCDIR
XPSSRCDIR=.\xps
!endif

!ifndef XPSGENDIR
XPSGENDIR=.\$(DEFAULT_OBJ_DIR)
!endif

!ifndef XPSOBJDIR
XPSOBJDIR=.\$(DEFAULT_OBJ_DIR)
!endif

GPDLSRCDIR=.\gpdl
GPDLGENDIR=.\$(DEFAULT_OBJ_DIR)
GPDLOBJDIR=.\$(DEFAULT_OBJ_DIR)

CONTRIBDIR=.\contrib

# Can we build PCL and XPS
!ifndef BUILD_PCL
BUILD_PCL=0
!if exist ("$(PLSRCDIR)\pl.mak")
BUILD_PCL=1
!endif
!endif

!ifndef BUILD_XPS
BUILD_XPS=0
!if exist ("$(XPSSRCDIR)\xps.mak")
BUILD_XPS=1
!endif
!endif

!ifndef BUILD_GPDL
BUILD_GPDL=0
!if exist ("$(GPDLSRCDIR)\gpdl.mak")
BUILD_GPDL=1
!endif
!endif

PCL_TARGET=
XPS_TARGET=

!if $(BUILD_PCL)
PCL_TARGET=gpcl6
!endif

!if $(BUILD_XPS)
XPS_TARGET=gxps
!endif

# !if $(BUILD_GPDL)
# GPDL_TARGET=gpdl
# !endif

PCL_XPS_TARGETS=$(PCL_TARGET) $(XPS_TARGET) $(GPDL_TARGET)

# Define the root directory for Ghostscript installation.

!ifndef AROOTDIR
AROOTDIR=c:/gs
!endif
!ifndef GSROOTDIR
GSROOTDIR=$(AROOTDIR)/gs$(GS_DOT_VERSION)
!endif

# Define the directory that will hold documentation at runtime.

!ifndef GS_DOCDIR
GS_DOCDIR=$(GSROOTDIR)/doc
!endif

# Define the default directory/ies for the runtime initialization, resource and
# font files.  Separate multiple directories with ';'.
# Use / to indicate directories, not \.
# MSVC will not allow \'s here because it sees '\;' CPP-style as an
# illegal escape.

!ifndef GS_LIB_DEFAULT
GS_LIB_DEFAULT=$(GSROOTDIR)/Resource/Init;$(GSROOTDIR)/lib;$(GSROOTDIR)/Resource/Font;$(AROOTDIR)/fonts
!endif

# Define whether or not searching for initialization files should always
# look in the current directory first.  This leads to well-known security
# and confusion problems, but may be convenient sometimes.

!ifndef SEARCH_HERE_FIRST
SEARCH_HERE_FIRST=0
!endif

# Define the name of the interpreter initialization file.
# (There is no reason to change this.)

!ifndef GS_INIT
GS_INIT=gs_init.ps
!endif

# Choose generic configuration options.

# Setting DEBUG=1 includes debugging features in the build:
# 1. It defines the C preprocessor symbol DEBUG. The latter includes
#    tracing and self-validation code fragments into compilation.
#    Particularly it enables the -Z and -T switches in Ghostscript.
# 2. It compiles code fragments for C stack overflow checks.
# Code produced with this option is somewhat larger and runs
# somewhat slower.

!ifndef DEBUG
DEBUG=0
!endif

# Setting TDEBUG=1 disables code optimization in the C compiler and
# includes symbol table information for the debugger.
# Code is substantially larger and slower.

# NOTE: The MSVC++ 5.0 compiler produces incorrect output code with TDEBUG=0.
# Also MSVC 6 must be service pack >= 3 to prevent INTERNAL COMPILER ERROR

# Default to 0 anyway since the execution times are so much better.
!ifndef TDEBUG
TDEBUG=0
!endif

# Setting DEBUGSYM=1 is only useful with TDEBUG=0.
# This option is for advanced developers. It includes symbol table
# information for the debugger in an optimized (release) build.
# NOTE: The debugging information generated for the optimized code may be
# significantly misleading. For general MSVC users we recommend TDEBUG=1.

!ifndef DEBUGSYM
DEBUGSYM=0
!endif


# We can compile for a 32-bit or 64-bit target
# WIN32 and WIN64 are mutually exclusive.  WIN32 is the default.
!if !defined(WIN32) && (!defined(Win64) || !defined(WIN64))
WIN32=0
!endif

# We can build either 32-bit or 64-bit target on a 64-bit platform
# but the location of the binaries differs. Would be nice if the
# detection of the platform could be automatic.
!ifndef BUILD_SYSTEM
!if "$(PROCESSOR_ARCHITEW6432)"=="AMD64" || "$(PROCESSOR_ARCHITECTURE)"=="AMD64"
BUILD_SYSTEM=64
PGMFILES=$(SYSTEMDRIVE)\Program Files
PGMFILESx86=$(SYSTEMDRIVE)\Program Files (x86)
!else
BUILD_SYSTEM=32
PGMFILES=$(SYSTEMDRIVE)\Program Files
PGMFILESx86=$(SYSTEMDRIVE)\Program Files
!endif
!endif

!ifndef MSWINSDKPATH
!if exist ("$(PGMFILESx86)\Microsoft SDKs\Windows")
!if exist ("$(PGMFILESx86)\Microsoft SDKs\Windows\v7.1A")
MSWINSDKPATH=$(PGMFILESx86)\Microsoft SDKs\Windows\v7.1A
!else
!if exist ("$(PGMFILESx86)\Microsoft SDKs\Windows\v7.1")
MSWINSDKPATH=$(PGMFILESx86)\Microsoft SDKs\Windows\v7.1
!else
!if exist ("$(PGMFILESx86)\Microsoft SDKs\Windows\v7.0A")
MSWINSDKPATH=$(PGMFILESx86)\Microsoft SDKs\Windows\v7.0A
!else
!if exist ("$(PGMFILESx86)\Microsoft SDKs\Windows\v7.0")
MSWINSDKPATH=$(PGMFILESx86)\Microsoft SDKs\Windows\v7.0
!endif
!endif
!endif
!endif
!else
!if exist ("$(PGMFILES)\Microsoft SDKs\Windows")
!if exist ("$(PGMFILES)\Microsoft SDKs\Windows\v7.1A")
MSWINSDKPATH=$(PGMFILES)\Microsoft SDKs\Windows\v7.1A
!else
!if exist ("$(PGMFILES)\Microsoft SDKs\Windows\v7.1")
MSWINSDKPATH=$(PGMFILES)\Microsoft SDKs\Windows\v7.1
!else
!if exist ("$(PGMFILES)\Microsoft SDKs\Windows\v7.0A")
MSWINSDKPATH=$(PGMFILES)\Microsoft SDKs\Windows\v7.0A
!else
!if exist ("$(PGMFILES)\Microsoft SDKs\Windows\v7.0")
MSWINSDKPATH=$(PGMFILES)\Microsoft SDKs\Windows\v7.0
!endif
!endif
!endif
!endif
!endif
!endif
!endif

XPSPRINTCFLAGS=
XPSPRINT=0

!ifdef MSWINSDKPATH
!if exist ("$(MSWINSDKPATH)\Include\XpsPrint.h")
XPSPRINTCFLAGS=/DXPSPRINT=1 /I"$(MSWINSDKPATH)\Include"
XPSPRINT=1
!endif
!endif

# Define the name of the executable file.

!ifndef GS
!ifdef WIN64
GS=gswin64
PCL=gpcl6win64
XPS=gxpswin64
GPDL=gpdlwin64
!else
!ifdef ARM
GS=gswinARM
PCL=gpcl6winARM
XPS=gxpswinARM
GPDL=gpdlwinARM
!else
GS=gswin32
PCL=gpcl6win32
XPS=gxpswin32
GPDL=gpdlwin32
!endif
!endif
!endif
!ifndef GSCONSOLE
GSCONSOLE=$(GS)c
!endif

!ifndef GSDLL
!ifdef METRO
!ifdef WIN64
GSDLL=gsdll64metro
!else
!ifdef ARM
GSDLL=gsdllARM32metro
!else
GSDLL=gsdll32metro
!endif
!endif
!else
!ifdef WIN64
GSDLL=gsdll64
!else
GSDLL=gsdll32
!endif
!endif
!endif

!ifndef GPCL6DLL
!ifdef METRO
!ifdef WIN64
GPCL6DLL=gpcl6dll64metro
!else
!ifdef ARM
GPCL6DLL=gpcl6dllARM32metro
!else
GPCL6DLL=gpcl6dll32metro
!endif
!endif
!else
!ifdef WIN64
GPCL6DLL=gpcl6dll64
!else
GPCL6DLL=gpcl6dll32
!endif
!endif
!endif

!ifndef GXPSDLL
!ifdef METRO
!ifdef WIN64
GXPSDLL=gxpsdll64metro
!else
!ifdef ARM
GXPSDLL=gxpsdllARM32metro
!else
GXPSDLL=gxpsdll32metro
!endif
!endif
!else
!ifdef WIN64
GXPSDLL=gxpsdll64
!else
GXPSDLL=gxpsdll32
!endif
!endif
!endif

!ifndef GPDLDLL
!ifdef METRO
!ifdef WIN64
GPDLDLL=gpdldll64metro
!else
!ifdef ARM
GPDLDLL=gpdldllARM32metro
!else
GPDLDLL=gpdldll32metro
!endif
!endif
!else
!ifdef WIN64
GPDLDLL=gpdldll64
!else
GPDLDLL=gpdldll32
!endif
!endif
!endif

# To build two small executables and a large DLL use MAKEDLL=1
# To build two large executables use MAKEDLL=0

!ifndef MAKEDLL
!if "$(PROFILE)"=="1"
MAKEDLL=0
!else
MAKEDLL=1
!endif
!endif

# Should we build in the cups device....
!ifdef WITH_CUPS
!if "$(WITH_CUPS)"!="0"
WITH_CUPS=1
!else
WITH_CUPS=0
!endif
!else
WITH_CUPS=0
!endif

# We can't build cups libraries in a Metro friendly way,
# so if building for Metro, disable cups regardless of the
# request
!ifdef METRO
WITH_CUPS=0
!endif

# Define the directory where the FreeType2 library sources are stored.
# See freetype.mak for more information.

!ifdef UFST_BRIDGE
!if "$(UFST_BRIDGE)"=="1"
FT_BRIDGE=0
!endif
!endif

!ifndef FT_BRIDGE
FT_BRIDGE=1
!endif

!ifndef FTSRCDIR
FTSRCDIR=freetype
!endif
!ifndef FT_CFLAGS
FT_CFLAGS=-I$(FTSRCDIR)\include
!endif

!ifdef BITSTREAM_BRIDGE
FT_BRIDGE=0
!endif

# Define the directory where the IJG JPEG library sources are stored,
# and the major version of the library that is stored there.
# You may need to change this if the IJG library version changes.
# See jpeg.mak for more information.

!ifndef JSRCDIR
JSRCDIR=jpeg
!endif

# Define the directory where the PNG library sources are stored,
# and the version of the library that is stored there.
# You may need to change this if the libpng version changes.
# See png.mak for more information.

!ifndef PNGSRCDIR
PNGSRCDIR=libpng
!endif

!ifndef TIFFSRCDIR
TIFFSRCDIR=tiff$(D)
TIFFCONFDIR=$(TIFFSRCDIR)
TIFFCONFIG_SUFFIX=.vc
TIFFPLATFORM=win32
!endif

# Define the directory where the zlib sources are stored.
# See zlib.mak for more information.

!ifndef ZSRCDIR
ZSRCDIR=.\zlib
!endif

# Define which jbig2 library to use
!if !defined(JBIG2_LIB) && (!defined(NO_LURATECH) || "$(NO_LURATECH)" != "1")
!if exist("luratech\ldf_jb2")
JBIG2_LIB=luratech
!endif
!endif

!ifndef JBIG2_LIB
JBIG2_LIB=jbig2dec
!endif

!if "$(JBIG2_LIB)" == "luratech" || "$(JBIG2_LIB)" == "ldf_jb2"
# Set defaults for using the Luratech JB2 implementation
!ifndef JBIG2SRCDIR
# CSDK source code location
JBIG2SRCDIR=luratech\ldf_jb2
!endif
!ifndef JBIG2_CFLAGS
# required compiler flags
!ifdef WIN64
JBIG2_CFLAGS=-DUSE_LDF_JB2 -DWIN64
!else
JBIG2_CFLAGS=-DUSE_LDF_JB2 -DWIN32
!endif
!endif
!else
# Use jbig2dec by default. See jbig2.mak for more information.
!ifndef JBIG2SRCDIR
# location of included jbig2dec library source
JBIG2SRCDIR=jbig2dec
!endif
!endif

# Alternatively, you can build a separate DLL
# and define SHARE_JBIG2=1 in src/winlib.mak

# Define which jpeg2k library to use
!if !defined(JPX_LIB) && (!defined(NO_LURATECH) || "$(NO_LURATECH)" != "1")
!if exist("luratech\lwf_jp2")
JPX_LIB=luratech
!endif
!endif

!ifndef JPX_LIB
JPX_LIB=openjpeg
!endif

# Alternatively, you can build a separate DLL
# and define SHARE_JPX=1 in src/winlib.mak

# Define the directory where the lcms source is stored.
# See lcms.mak for more information
!ifndef LCMSSRCDIR
LCMSSRCDIR=lcms
!endif

# Define the directory where the lcms2 source is stored.
# See lcms2.mak for more information
!ifndef LCMS2SRCDIR
LCMS2SRCDIR=lcms2
!endif

# Define the directory where the ijs source is stored,
# and the process forking method to use for the server.
# See ijs.mak for more information.

!ifndef IJSSRCDIR
SHARE_IJS=0
IJS_NAME=
IJSSRCDIR=ijs
IJSEXECTYPE=win
!endif

# Define the directory where the CUPS library sources are stored,

!ifndef LCUPSSRCDIR
SHARE_LCUPS=0
LCUPS_NAME=
LCUPSSRCDIR=cups
LCUPSBUILDTYPE=win
CUPS_CC=$(CC) $(CFLAGS) -DWIN32
!endif

!ifndef LCUPSISRCDIR
SHARE_LCUPSI=0
LCUPSI_NAME=
LCUPSISRCDIR=cups
CUPS_CC=$(CC) $(CFLAGS) -DWIN32 -DHAVE_BOOLEAN
!endif

!ifndef JPEGXR_SRCDIR
JPEGXR_SRCDIR=.\jpegxr
!endif

!ifndef SHARE_JPEGXR
SHARE_JPEGXR=0
!endif

!ifndef JPEGXR_CFLAGS
JPEGXR_CFLAGS=/TP /DNDEBUG
!endif

!ifndef EXPATSRCDIR
EXPATSRCDIR=.\expat
!endif

!ifndef SHARE_EXPAT
SHARE_EXPAT=0
!endif

!ifndef EXPAT_CFLAGS
EXPAT_CFLAGS=/DHAVE_MEMMOVE
!endif

# Define any other compilation flags.

# support XCFLAGS for parity with the unix makefiles
!ifndef XCFLAGS
XCFLAGS=
!endif

# We now build with unicode support by default. To avoid this, build
# with GS_NO_UTF8=1.
!if "$(GS_NO_UTF8)" != ""
UNICODECFLAGS=/DGS_NO_UTF8
!else
UNICODECFLAGS=
!endif

!ifndef CFLAGS
CFLAGS=
!endif

!ifdef DEVSTUDIO
CFLAGS=$(CFLAGS) /FC
!endif

!if "$(MEMENTO)"=="1"
CFLAGS=$(CFLAGS) -DMEMENTO
!endif

!ifdef METRO
# Ideally the TIF_PLATFORM_CONSOLE define should only be for libtiff,
# but we aren't set up to do that easily
CFLAGS=$(CFLAGS) -DMETRO -DWINAPI_FAMILY=WINAPI_PARTITION_APP -DTIF_PLATFORM_CONSOLE
# WinRT doesn't allow ExitProcess() so we have to suborn it here.
# it shouldn't matter since we actually rely on setjmp()/longjmp() for error handling in libtiff
PNG_CFLAGS=/DExitProcess=exit
!endif

CFLAGS=$(CFLAGS) $(XCFLAGS) $(UNICODECFLAGS)

# 1 --> Use 64 bits for gx_color_index.  This is required only for
# non standard devices or DeviceN process color model devices.
USE_LARGE_COLOR_INDEX=0

!if $(USE_LARGE_COLOR_INDEX) == 1
# Definitions to force gx_color_index to 64 bits
LARGEST_UINTEGER_TYPE=unsigned __int64
GX_COLOR_INDEX_TYPE=$(LARGEST_UINTEGER_TYPE)

CFLAGS=$(CFLAGS) /DGX_COLOR_INDEX_TYPE="$(GX_COLOR_INDEX_TYPE)"
!endif

# -W3 generates too much noise.
!ifndef WARNOPT
WARNOPT=-W2
!endif

#
# Do not edit the next group of lines.

#!include $(COMMONDIR)\msvcdefs.mak
#!include $(COMMONDIR)\pcdefs.mak
#!include $(COMMONDIR)\generic.mak
!include $(GLSRCDIR)\version.mak
# The following is a hack to get around the special treatment of \ at
# the end of a line.
NUL=
DD=$(GLGENDIR)\$(NUL)
GLD=$(GLGENDIR)\$(NUL)
PSD=$(PSGENDIR)\$(NUL)

!ifdef SBR
SBRFLAGS=/FR$(SBRDIR)\$(NUL)
!endif

# ------ Platform-specific options ------ #

# Define which major version of MSVC is being used
# (currently, 4, 5, 6, 7, and 8 are supported).
# Define the minor version of MSVC, currently only
# used for Microsoft Visual Studio .NET 2003 (7.1)

#MSVC_VERSION=6
#MSVC_MINOR_VERSION=0

# Make a guess at the version of MSVC in use
# This will not work if service packs change the version numbers.

!if defined(_NMAKE_VER) && !defined(MSVC_VERSION)
!if "$(_NMAKE_VER)" == "162"
MSVC_VERSION=5
!endif
!if "$(_NMAKE_VER)" == "6.00.8168.0"
MSVC_VERSION=6
!endif
!if "$(_NMAKE_VER)" == "7.00.9466"
MSVC_VERSION=7
!endif
!if "$(_NMAKE_VER)" == "7.00.9955"
MSVC_VERSION=7
!endif
!if "$(_NMAKE_VER)" == "7.10.3077"
MSVC_VERSION=7
MSVC_MINOR_VERSION=1
!endif
!if "$(_NMAKE_VER)" == "8.00.40607.16"
MSVC_VERSION=8
!endif
!if "$(_NMAKE_VER)" == "8.00.50727.42"
MSVC_VERSION=8
!endif
!if "$(_NMAKE_VER)" == "8.00.50727.762"
MSVC_VERSION=8
!endif
!if "$(_NMAKE_VER)" == "9.00.21022.08"
MSVC_VERSION=9
!endif
!if "$(_NMAKE_VER)" == "9.00.30729.01"
MSVC_VERSION=9
!endif
!if "$(_NMAKE_VER)" == "10.00.30319.01"
MSVC_VERSION=10
!endif
!if "$(_NMAKE_VER)" == "11.00.50522.1"
MSVC_VERSION=11
!endif
!if "$(_NMAKE_VER)" == "11.00.50727.1"
MSVC_VERSION=11
!endif
!if "$(_NMAKE_VER)" == "11.00.60315.1"
MSVC_VERSION=11
!endif
!if "$(_NMAKE_VER)" == "11.00.60610.1"
MSVC_VERSION=11
!endif
!if "$(_NMAKE_VER)" == "12.00.21005.1"
MSVC_VERSION=12
!endif
!if "$(_NMAKE_VER)" == "14.00.23506.0"
MSVC_VERSION=14
!endif
!endif

!ifndef MSVC_VERSION
MSVC_VERSION=6
!endif
!ifndef MSVC_MINOR_VERSION
MSVC_MINOR_VERSION=0
!endif

# Define the drive, directory, and compiler name for the Microsoft C files.
# COMPDIR contains the compiler and linker (normally \msdev\bin).
# MSINCDIR contains the include files (normally \msdev\include).
# LIBDIR contains the library files (normally \msdev\lib).
# COMP is the full C compiler path name (normally \msdev\bin\cl).
# COMPCPP is the full C++ compiler path name (normally \msdev\bin\cl).
# COMPAUX is the compiler name for DOS utilities (normally \msdev\bin\cl).
# RCOMP is the resource compiler name (normallly \msdev\bin\rc).
# LINK is the full linker path name (normally \msdev\bin\link).
# Note that when MSINCDIR and LIBDIR are used, they always get a '\' appended,
#   so if you want to use the current directory, use an explicit '.'.

!if $(MSVC_VERSION) == 4
! ifndef DEVSTUDIO
DEVSTUDIO=c:\msdev
! endif
COMPBASE=$(DEVSTUDIO)
SHAREDBASE=$(DEVSTUDIO)
!endif

!if $(MSVC_VERSION) == 5
! ifndef DEVSTUDIO
DEVSTUDIO=C:\Program Files\Devstudio
! endif
!if "$(DEVSTUDIO)"==""
COMPBASE=
SHAREDBASE=
!else
COMPBASE=$(DEVSTUDIO)\VC
SHAREDBASE=$(DEVSTUDIO)\SharedIDE
!endif
!endif

!if $(MSVC_VERSION) == 6
! ifndef DEVSTUDIO
DEVSTUDIO=C:\Program Files\Microsoft Visual Studio
! endif
!if "$(DEVSTUDIO)"==""
COMPBASE=
SHAREDBASE=
!else
COMPBASE=$(DEVSTUDIO)\VC98
SHAREDBASE=$(DEVSTUDIO)\Common\MSDev98
!endif
!endif

!if $(MSVC_VERSION) == 7
! ifndef DEVSTUDIO
!if $(MSVC_MINOR_VERSION) == 0
DEVSTUDIO=C:\Program Files\Microsoft Visual Studio .NET
!else
DEVSTUDIO=C:\Program Files\Microsoft Visual Studio .NET 2003
!endif
! endif
!if "$(DEVSTUDIO)"==""
COMPBASE=
SHAREDBASE=
!else
COMPBASE=$(DEVSTUDIO)\Vc7
SHAREDBASE=$(DEVSTUDIO)\Vc7
!endif
!endif

!if $(MSVC_VERSION) == 8
! ifndef DEVSTUDIO
!if $(BUILD_SYSTEM) == 64
DEVSTUDIO=C:\Program Files (x86)\Microsoft Visual Studio 8
!else
DEVSTUDIO=C:\Program Files\Microsoft Visual Studio 8
!endif
! endif
!if "$(DEVSTUDIO)"==""
COMPBASE=
SHAREDBASE=
!else
COMPBASE=$(DEVSTUDIO)\VC
SHAREDBASE=$(DEVSTUDIO)\VC
!ifdef WIN64
COMPDIR64=$(COMPBASE)\bin\amd64
LINKLIBPATH=/LIBPATH:"$(COMPBASE)\lib\amd64" /LIBPATH:"$(COMPBASE)\PlatformSDK\Lib\AMD64"
!endif
!endif
!endif

!if $(MSVC_VERSION) == 9
! ifndef DEVSTUDIO
!if $(BUILD_SYSTEM) == 64
DEVSTUDIO=C:\Program Files (x86)\Microsoft Visual Studio 9.0
!else
DEVSTUDIO=C:\Program Files\Microsoft Visual Studio 9.0
!endif
! endif
!if "$(DEVSTUDIO)"==""
COMPBASE=
SHAREDBASE=
!else
# There are at least 4 different values:
# "v6.0"=Vista, "v6.0A"=Visual Studio 2008,
# "v6.1"=Windows Server 2008, "v7.0"=Windows 7
! ifdef MSSDK
RCDIR=$(MSSDK)\bin
! else
RCDIR=C:\Program Files\Microsoft SDKs\Windows\v6.0A\bin
! endif
COMPBASE=$(DEVSTUDIO)\VC
SHAREDBASE=$(DEVSTUDIO)\VC
!ifdef WIN64
COMPDIR64=$(COMPBASE)\bin\amd64
LINKLIBPATH=/LIBPATH:"$(COMPBASE)\lib\amd64" /LIBPATH:"$(COMPBASE)\PlatformSDK\Lib\AMD64"
!endif
!endif
!endif

!if $(MSVC_VERSION) == 10
! ifndef DEVSTUDIO
!if $(BUILD_SYSTEM) == 64
DEVSTUDIO=C:\Program Files (x86)\Microsoft Visual Studio 10.0
!else
DEVSTUDIO=C:\Program Files\Microsoft Visual Studio 10.0
!endif
! endif
!if "$(DEVSTUDIO)"==""
COMPBASE=
SHAREDBASE=
!else
# There are at least 4 different values:
# "v6.0"=Vista, "v6.0A"=Visual Studio 2008,
# "v6.1"=Windows Server 2008, "v7.0"=Windows 7
! ifdef MSSDK
!  ifdef WIN64
RCDIR=$(MSSDK)\bin\x64
!  else
RCDIR=$(MSSDK)\bin
!  endif
! else
!ifdef WIN64
RCDIR=C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0\bin
!else
RCDIR=C:\Program Files\Microsoft SDKs\Windows\v7.0\bin
!endif
! endif
COMPBASE=$(DEVSTUDIO)\VC
SHAREDBASE=$(DEVSTUDIO)\VC
!ifdef WIN64
!if $(BUILD_SYSTEM) == 64
COMPDIR64=$(COMPBASE)\bin\amd64
LINKLIBPATH=/LIBPATH:"$(MSSDK)\lib\x64" /LIBPATH:"$(COMPBASE)\lib\amd64"
!else
COMPDIR64=$(COMPBASE)\bin\x86_amd64
LINKLIBPATH=/LIBPATH:"$(COMPBASE)\lib\amd64" /LIBPATH:"$(COMPBASE)\PlatformSDK\Lib\x64"
!endif
!endif
!endif
!endif

!if $(MSVC_VERSION) == 11
! ifndef DEVSTUDIO
!if $(BUILD_SYSTEM) == 64
DEVSTUDIO=C:\Program Files (x86)\Microsoft Visual Studio 11.0
!else
DEVSTUDIO=C:\Program Files\Microsoft Visual Studio 11.0
!endif
! endif
!if "$(DEVSTUDIO)"==""
COMPBASE=
SHAREDBASE=
!else
# There are at least 4 different values:
# "v6.0"=Vista, "v6.0A"=Visual Studio 2008,
# "v6.1"=Windows Server 2008, "v7.0"=Windows 7
! ifdef MSSDK
!  ifdef WIN64
RCDIR=$(MSSDK)\bin\x64
!  else
RCDIR=$(MSSDK)\bin
!  endif
! else
!if $(BUILD_SYSTEM) == 64
RCDIR=C:\Program Files (x86)\Windows Kits\8.0\bin\x64
!else
RCDIR=C:\Program Files\Windows Kits\8.0\bin\x86
!endif
! endif
COMPBASE=$(DEVSTUDIO)\VC
SHAREDBASE=$(DEVSTUDIO)\VC
!ifdef WIN64
!if $(BUILD_SYSTEM) == 64
COMPDIR64=$(COMPBASE)\bin\x86_amd64
LINKLIBPATH=/LIBPATH:"$(MSSDK)\lib\x64" /LIBPATH:"$(COMPBASE)\lib\amd64"
!else
COMPDIR64=$(COMPBASE)\bin\x86_amd64
LINKLIBPATH=/LIBPATH:"$(COMPBASE)\lib\amd64" /LIBPATH:"$(COMPBASE)\PlatformSDK\Lib\x64"
!endif
!endif
!endif
!endif

!if $(MSVC_VERSION) == 12
! ifndef DEVSTUDIO
!  if $(BUILD_SYSTEM) == 64
DEVSTUDIO=C:\Program Files (x86)\Microsoft Visual Studio 12.0
!  else
DEVSTUDIO=C:\Program Files\Microsoft Visual Studio 12.0
!  endif
! endif
! if "$(DEVSTUDIO)"==""
COMPBASE=
SHAREDBASE=
! else
# There are at least 4 different values:
# "v6.0"=Vista, "v6.0A"=Visual Studio 2008,
# "v6.1"=Windows Server 2008, "v7.0"=Windows 7
!  ifdef MSSDK
!   ifdef WIN64
RCDIR=$(MSSDK)\bin\x64
!   else
RCDIR=$(MSSDK)\bin
!   endif
!  else
!   if $(BUILD_SYSTEM) == 64
RCDIR=C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Bin
!   else
RCDIR=C:\Program Files\Microsoft SDKs\Windows\v7.1A\Bin
!   endif
!  endif
COMPBASE=$(DEVSTUDIO)\VC
SHAREDBASE=$(DEVSTUDIO)\VC
!  ifdef WIN64
!   if $(BUILD_SYSTEM) == 64
COMPDIR64=$(COMPBASE)\bin\x86_amd64
LINKLIBPATH=/LIBPATH:"$(COMPBASE)\lib\amd64"
!   else
COMPDIR64=$(COMPBASE)\bin\x86_amd64
LINKLIBPATH=/LIBPATH:"$(COMPBASE)\lib\amd64" /LIBPATH:"$(COMPBASE)\PlatformSDK\Lib\x64"
!   endif
!  endif
! endif
!endif

!if $(MSVC_VERSION) == 14
! ifndef DEVSTUDIO
!  if $(BUILD_SYSTEM) == 64
DEVSTUDIO=C:\Program Files (x86)\Microsoft Visual Studio 14.0
!  else
DEVSTUDIO=C:\Program Files\Microsoft Visual Studio 14.0
!  endif
! endif
! if "$(DEVSTUDIO)"==""
COMPBASE=
SHAREDBASE=
! else
# There are at least 4 different values:
# "v6.0"=Vista, "v6.0A"=Visual Studio 2008,
# "v6.1"=Windows Server 2008, "v7.0"=Windows 7
!  ifdef MSSDK
!   ifdef WIN64
RCDIR=$(MSSDK)\bin\x64
!   else
RCDIR=$(MSSDK)\bin
!   endif
!  else
!   if $(BUILD_SYSTEM) == 64
RCDIR=C:\Program Files (x86)\Microsoft SDKs\Windows\v10.0A\Bin
!   else
RCDIR=C:\Program Files\Microsoft SDKs\Windows\v10.0A\Bin
!   endif
!  endif
COMPBASE=$(DEVSTUDIO)\VC
SHAREDBASE=$(DEVSTUDIO)\VC
!  ifdef WIN64
!   if $(BUILD_SYSTEM) == 64
COMPDIR64=$(COMPBASE)\bin\x86_amd64
LINKLIBPATH=/LIBPATH:"$(COMPBASE)\lib\amd64"
!   else
COMPDIR64=$(COMPBASE)\bin\x86_amd64
LINKLIBPATH=/LIBPATH:"$(COMPBASE)\lib\amd64" /LIBPATH:"$(COMPBASE)\PlatformSDK\Lib\x64"
!   endif
!  endif
! endif
!endif

!if "$(ARM)"=="1"
VCINSTDIR=$(VS110COMNTOOLS)..\..\VC\

!ifndef WINSDKVER
WINSDKVER=8.0
!endif

!ifndef WINSDKDIR
WINSDKDIR=$(VS110COMNTOOLS)..\..\..\Windows Kits\$(WINSDKVER)\
!endif

COMPAUX__="$(VCINSTDIR)\bin\cl.exe"
COMPAUXCFLAGS=/I"$(VCINSTDIR)\INCLUDE" /I"$(VCINSTDIR)\ATLMFC\INCLUDE" \
/I"$(WINSDKDIR)\include\shared" /I"$(WINSDKDIR)\include\um" \
/I"$(WINDSKDIR)include\winrt"

COMPAUXLDFLAGS=/LIBPATH:"$(VCINSTDIR)\LIB" \
/LIBPATH:"$(VCINSTDIR)\ATLMFC\LIB" \
/LIBPATH:"$(WINSDKDIR)\lib\win8\um\x86"

COMPAUX=$(COMPAUX__) $(COMPAUXCFLAGS)

!else

COMPAUXLDFLAGS=""

!endif

# Some environments don't want to specify the path names for the tools at all.
# Typical definitions for such an environment would be:
#   MSINCDIR= LIBDIR= COMP=cl COMPAUX=cl RCOMP=rc LINK=link
# COMPDIR, LINKDIR, and RCDIR are irrelevant, since they are only used to
# define COMP, LINK, and RCOMP respectively, but we allow them to be
# overridden anyway for completeness.
!ifndef COMPDIR
!if "$(COMPBASE)"==""
COMPDIR=
!else
!ifdef WIN64
COMPDIR=$(COMPDIR64)
!else
COMPDIR=$(COMPBASE)\bin
!endif
!endif
!endif

!ifndef LINKDIR
!if "$(COMPBASE)"==""
LINKDIR=
!else
!ifdef WIN64
LINKDIR=$(COMPDIR64)
!else
LINKDIR=$(COMPBASE)\bin
!endif
!endif
!endif

!ifndef RCDIR
!if "$(SHAREDBASE)"==""
RCDIR=
!else
RCDIR=$(SHAREDBASE)\bin
!endif
!endif

!ifndef MSINCDIR
!if "$(COMPBASE)"==""
MSINCDIR=
!else
MSINCDIR=$(COMPBASE)\include
!endif
!endif

!ifndef LIBDIR
!if "$(COMPBASE)"==""
LIBDIR=
!else
!ifdef WIN64
LIBDIR=$(COMPBASE)\lib\amd64
!else
LIBDIR=$(COMPBASE)\lib
!endif
!endif
!endif

!ifndef COMP
!if "$(COMPDIR)"==""
COMP=cl
!else
COMP="$(COMPDIR)\cl"
!endif
!endif
!ifndef COMPCPP
COMPCPP=$(COMP)
!endif
!ifndef COMPAUX
!ifdef WIN64
COMPAUX=$(COMP)
!else
COMPAUX=$(COMP)
!endif
!endif

!ifndef RCOMP
!if "$(RCDIR)"==""
RCOMP=rc
!else
RCOMP="$(RCDIR)\rc"
!endif
!endif

!ifndef LINK
!if "$(LINKDIR)"==""
LINK=link
!else
LINK="$(LINKDIR)\link"
!endif
!endif

# nmake does not have a form of .BEFORE or .FIRST which can be used
# to specify actions before anything else is done.  If LIB and INCLUDE
# are not defined then we want to define them before we link or
# compile.  Here is a kludge which allows us to to do what we want.
# nmake does evaluate preprocessor directives when they are encountered.
# So the desired set statements are put into dummy preprocessor
# directives.
!ifndef INCLUDE
!if "$(MSINCDIR)"!=""
!if [set INCLUDE=$(MSINCDIR)]==0
!endif
!endif
!endif
!ifndef LIB
!if "$(LIBDIR)"!=""
!if [set LIB=$(LIBDIR)]==0
!endif
!endif
!endif

!ifndef LINKLIBPATH
LINKLIBPATH=
!endif

# Define the processor architecture. (i386, ppc, alpha)

!ifndef CPU_FAMILY
CPU_FAMILY=i386
#CPU_FAMILY=ppc
#CPU_FAMILY=alpha  # not supported yet - we need someone to tweak
!endif

# Define the processor (CPU) type. Allowable values depend on the family:
#   i386: 386, 486, 586
#   ppc: 601, 604, 620
#   alpha: not currently used.

!ifndef CPU_TYPE
CPU_TYPE=486
#CPU_TYPE=601
!endif

# Define special features of CPUs

# We'll assume that if you have an x86 machine, you've got a modern
# enough one to have SSE2 instructions. If you don't, then predefine
# DONT_HAVE_SSE2 when calling this makefile
!ifndef ARM
!if "$(CPU_FAMILY)" == "i386"
!ifndef DONT_HAVE_SSE2
!ifndef HAVE_SSE2
!message **************************************************************
!message * Assuming that target has SSE2 instructions available. If   *
!message * this is NOT the case, define DONT_HAVE_SSE2 when building. *
!message **************************************************************
!endif
HAVE_SSE2=1
CFLAGS=$(CFLAGS) /DHAVE_SSE2
# add "/D__SSE__" here, but causes crashes just now
JPX_SSE_CFLAGS=
!endif
!endif
!endif

# Define the .dev module that implements thread and synchronization
# primitives for this platform.  Don't change this unless you really know
# what you're doing.

!ifndef SYNC
SYNC=winsync
!endif

# Luratech jp2 flags depend on the compiler version
#
!if "$(JPX_LIB)" == "luratech" || "$(JPX_LIB)" == "lwf_jp2"
# Set defaults for using the Luratech JP2 implementation
!ifndef JPXSRCDIR
# CSDK source code location
JPXSRCDIR=luratech\lwf_jp2
!endif
!ifndef JPX_CFLAGS
# required compiler flags
!ifdef WIN64
JPX_CFLAGS=-DUSE_LWF_JP2 -DWIN64 -DNO_ASSEMBLY
!else
JPX_CFLAGS=-DUSE_LWF_JP2 -DWIN32 -DNO_ASSEMBLY
!endif
!endif
!endif

# OpenJPEG compiler flags
#
!if "$(JPX_LIB)" == "openjpeg"
!ifndef JPXSRCDIR
JPXSRCDIR=openjpeg
!endif
!ifndef JPX_CFLAGS
!ifdef WIN64
JPX_CFLAGS=-DUSE_OPENJPEG_JP2 -DUSE_JPIP $(JPX_SSE_CFLAGS) -DWIN64
!else
JPX_CFLAGS=-DUSE_OPENJPEG_JP2 -DUSE_JPIP $(JPX_SSE_CFLAGS) -DWIN32
!endif
!else
JPX_CFLAGS = $JPX_CFLAGS -DUSE_JPIP -DUSE_OPENJPEG_JP2
!endif
!endif

# ------ Devices and features ------ #

# Choose the language feature(s) to include.  See gs.mak for details.

!ifndef FEATURE_DEVS
# Choose the language feature(s) to include.  See gs.mak for details.

# if it's included, $(PSD)gs_pdfwr.dev should always be one of the last in the list
PSI_FEATURE_DEVS=$(PSD)psl3.dev $(PSD)pdf.dev $(PSD)dpsnext.dev $(PSD)epsf.dev $(PSD)ttfont.dev \
                 $(PSD)jbig2.dev $(PSD)jpx.dev $(PSD)fapi_ps.dev $(GLD)winutf8.dev $(PSD)gs_pdfwr.dev


PCL_FEATURE_DEVS=$(PLOBJDIR)/pl.dev $(PLOBJDIR)/pjl.dev $(PXLOBJDIR)/pxl.dev $(PCL5OBJDIR)/pcl5c.dev \
             $(PCL5OBJDIR)/hpgl2c.dev

XPS_FEATURE_DEVS=$(XPSOBJDIR)/pl.dev $(XPSOBJDIR)/xps.dev

FEATURE_DEVS=$(GLD)pipe.dev $(GLD)gsnogc.dev $(GLD)htxlib.dev $(GLD)psl3lib.dev $(GLD)psl2lib.dev \
             $(GLD)dps2lib.dev $(GLD)path1lib.dev $(GLD)patlib.dev $(GLD)psl2cs.dev $(GLD)rld.dev $(GLD)gxfapiu$(UFST_BRIDGE).dev\
             $(GLD)ttflib.dev  $(GLD)cielib.dev $(GLD)pipe.dev $(GLD)htxlib.dev $(GLD)sdct.dev $(GLD)libpng.dev\
	     $(GLD)seprlib.dev $(GLD)translib.dev $(GLD)cidlib.dev $(GLD)psf0lib.dev $(GLD)psf1lib.dev\
             $(GLD)psf2lib.dev $(GLD)lzwd.dev $(GLD)sicclib.dev $(GLD)mshandle.dev $(GLD)mspoll.dev \
             $(GLD)ramfs.dev $(GLD)sjpx.dev $(GLD)sjbig2.dev


!ifndef METRO
FEATURE_DEVS=$(FEATURE_DEVS) $(PSD)msprinter.dev $(GLD)pipe.dev
!endif
!endif

# Choose whether to compile the .ps initialization files into the executable.
# See gs.mak for details.

!ifndef COMPILE_INITS
COMPILE_INITS=1
!endif

# Choose whether to store band lists on files or in memory.
# The choices are 'file' or 'memory'.

!ifndef BAND_LIST_STORAGE
BAND_LIST_STORAGE=file
!endif

# Choose which compression method to use when storing band lists in memory.
# The choices are 'lzw' or 'zlib'.

!ifndef BAND_LIST_COMPRESSOR
BAND_LIST_COMPRESSOR=zlib
!endif

# Choose the implementation of file I/O: 'stdio', 'fd', or 'both'.
# See gs.mak and sfxfd.c for more details.

!ifndef FILE_IMPLEMENTATION
FILE_IMPLEMENTATION=stdio
!endif

# Choose the implementation of stdio: '' for file I/O and 'c' for callouts
# See gs.mak and ziodevs.c/ziodevsc.c for more details.

!ifndef STDIO_IMPLEMENTATION
STDIO_IMPLEMENTATION=c
!endif

# Choose the device(s) to include.  See devs.mak for details,
# devs.mak and contrib.mak for the list of available devices.

!ifndef DEVICE_DEVS
!ifdef METRO
DEVICE_DEVS=
!else
# $(DD)mswindll.dev 
DEVICE_DEVS=$(DD)display.dev $(DD)mswinpr2.dev $(DD)ijs.dev $(DD)mswindll.dev 
!endif
DEVICE_DEVS2=$(DD)epson.dev $(DD)eps9high.dev $(DD)eps9mid.dev $(DD)epsonc.dev $(DD)ibmpro.dev
DEVICE_DEVS3=$(DD)deskjet.dev $(DD)djet500.dev $(DD)laserjet.dev $(DD)ljetplus.dev $(DD)ljet2p.dev
DEVICE_DEVS4=$(DD)cdeskjet.dev $(DD)cdjcolor.dev $(DD)cdjmono.dev $(DD)cdj550.dev
DEVICE_DEVS5=$(DD)uniprint.dev $(DD)djet500c.dev $(DD)declj250.dev $(DD)lj250.dev
DEVICE_DEVS6=$(DD)st800.dev $(DD)stcolor.dev $(DD)bj10e.dev $(DD)bj200.dev
DEVICE_DEVS7=$(DD)t4693d2.dev $(DD)t4693d4.dev $(DD)t4693d8.dev $(DD)tek4696.dev
DEVICE_DEVS8=$(DD)pcxmono.dev $(DD)pcxgray.dev $(DD)pcx16.dev $(DD)pcx256.dev $(DD)pcx24b.dev $(DD)pcxcmyk.dev
DEVICE_DEVS9=$(DD)pbm.dev $(DD)pbmraw.dev $(DD)pgm.dev $(DD)pgmraw.dev $(DD)pgnm.dev $(DD)pgnmraw.dev $(DD)pkmraw.dev
DEVICE_DEVS10=$(DD)tiffcrle.dev $(DD)tiffg3.dev $(DD)tiffg32d.dev $(DD)tiffg4.dev $(DD)tifflzw.dev $(DD)tiffpack.dev
DEVICE_DEVS11=$(DD)bmpmono.dev $(DD)bmpgray.dev $(DD)bmp16.dev $(DD)bmp256.dev $(DD)bmp16m.dev $(DD)tiff12nc.dev $(DD)tiff24nc.dev $(DD)tiff48nc.dev $(DD)tiffgray.dev $(DD)tiff32nc.dev $(DD)tiff64nc.dev $(DD)tiffsep.dev $(DD)tiffsep1.dev $(DD)tiffscaled.dev $(DD)tiffscaled8.dev $(DD)tiffscaled24.dev $(DD)tiffscaled32.dev $(DD)tiffscaled4.dev
DEVICE_DEVS12=$(DD)bit.dev $(DD)bitrgb.dev $(DD)bitcmyk.dev
DEVICE_DEVS13=$(DD)pngmono.dev $(DD)pngmonod.dev $(DD)pnggray.dev $(DD)png16.dev $(DD)png256.dev $(DD)png16m.dev $(DD)pngalpha.dev $(DD)fpng.dev $(DD)psdcmykog.dev
DEVICE_DEVS14=$(DD)jpeg.dev $(DD)jpeggray.dev $(DD)jpegcmyk.dev
DEVICE_DEVS15=$(DD)pdfwrite.dev $(DD)ps2write.dev $(DD)eps2write.dev $(DD)txtwrite.dev $(DD)pxlmono.dev $(DD)pxlcolor.dev $(DD)xpswrite.dev $(DD)inkcov.dev $(DD)ink_cov.dev
DEVICE_DEVS16=$(DD)bbox.dev $(DD)plib.dev $(DD)plibg.dev $(DD)plibm.dev $(DD)plibc.dev $(DD)plibk.dev $(DD)plan.dev $(DD)plang.dev $(DD)planm.dev $(DD)planc.dev $(DD)plank.dev
!if "$(WITH_CUPS)" == "1"
DEVICE_DEVS16=$(DEVICE_DEVS16) $(DD)cups.dev
!endif
# Overflow for DEVS3,4,5,6,9
DEVICE_DEVS17=$(DD)ljet3.dev $(DD)ljet3d.dev $(DD)ljet4.dev $(DD)ljet4d.dev
DEVICE_DEVS18=$(DD)pj.dev $(DD)pjxl.dev $(DD)pjxl300.dev $(DD)jetp3852.dev $(DD)r4081.dev
DEVICE_DEVS19=$(DD)lbp8.dev $(DD)m8510.dev $(DD)necp6.dev $(DD)bjc600.dev $(DD)bjc800.dev
DEVICE_DEVS20=$(DD)pnm.dev $(DD)pnmraw.dev $(DD)ppm.dev $(DD)ppmraw.dev $(DD)pamcmyk32.dev $(DD)pamcmyk4.dev $(DD)pnmcmyk.dev $(DD)pam.dev
DEVICE_DEVS21=$(DD)spotcmyk.dev $(DD)devicen.dev $(DD)bmpsep1.dev $(DD)bmpsep8.dev $(DD)bmp16m.dev $(DD)bmp32b.dev $(DD)psdcmyk.dev $(DD)psdrgb.dev $(DD)cp50.dev $(DD)gprf.dev
!endif
CONTRIB_DEVS=$(DD)pcl3.dev $(DD)hpdjplus.dev $(DD)hpdjportable.dev $(DD)hpdj310.dev $(DD)hpdj320.dev $(DD)hpdj340.dev $(DD)hpdj400.dev $(DD)hpdj500.dev $(DD)hpdj500c.dev $(DD)hpdj510.dev $(DD)hpdj520.dev $(DD)hpdj540.dev $(DD)hpdj550c.dev $(DD)hpdj560c.dev $(DD)hpdj600.dev $(DD)hpdj660c.dev $(DD)hpdj670c.dev $(DD)hpdj680c.dev $(DD)hpdj690c.dev $(DD)hpdj850c.dev $(DD)hpdj855c.dev $(DD)hpdj870c.dev $(DD)hpdj890c.dev $(DD)hpdj1120c.dev $(DD)cdj670.dev $(DD)cdj850.dev $(DD)cdj880.dev $(DD)cdj890.dev $(DD)cdj970.dev $(DD)cdj1600.dev $(DD)cdnj500.dev $(DD)chp2200.dev $(DD)lips3.dev $(DD)lxm3200.dev $(DD)lex2050.dev $(DD)lxm3200.dev $(DD)lex5700.dev $(DD)lex7000.dev $(DD)oki4w.dev $(DD)gdi.dev $(DD)samsunggdi.dev $(DD)dl2100.dev $(DD)la50.dev $(DD)la70.dev $(DD)la75.dev $(DD)la75plus.dev $(DD)ln03.dev $(DD)xes.dev $(DD)md2k.dev $(DD)md5k.dev $(DD)lips4.dev $(DD)bj10v.dev $(DD)bj10vh.dev $(DD)md50Mono.dev $(DD)md50Eco.dev $(DD)md1xMono.dev $(DD)lp2000.dev $(DD)escpage.dev $(DD)npdl.dev $(DD)rpdl.dev $(DD)fmpr.dev $(DD)fmlbp.dev $(DD)jj100.dev $(DD)lbp310.dev $(DD)lbp320.dev $(DD)mj700v2c.dev $(DD)mj500c.dev $(DD)mj6000c.dev $(DD)mj8000c.dev $(DD)pr201.dev $(DD)pr150.dev $(DD)pr1000.dev $(DD)pr1000_4.dev $(DD)lips2p.dev $(DD)bjc880j.dev $(DD)mag16.dev $(DD)mag256.dev $(DD)bjcmono.dev $(DD)bjcgray.dev $(DD)bjccmyk.dev $(DD)bjccolor.dev

!if "$(WITH_CONTRIB)" == "1"
DEVICE_DEVS16=$(DEVICE_DEVS16) $(CONTRIB_DEVS)
!endif

# FAPI compilation options :
UFST_CFLAGS=-DMSVC

BITSTREAM_CFLAGS=

# ---------------------------- End of options ---------------------------- #

# Define the name of the makefile -- used in dependencies.

MAKEFILE=$(PSSRCDIR)\msvc32.mak
TOP_MAKEFILES=$(MAKEFILE) $(GLSRCDIR)\msvccmd.mak $(GLSRCDIR)\msvctail.mak $(GLSRCDIR)\winlib.mak $(PSSRCDIR)\winint.mak

# Define the files to be removed by `make clean'.
# nmake expands macros when encountered, not when used,
# so this must precede the !include statements.

BEGINFILES2=$(GLGENDIR)\lib.rsp\
 $(GLOBJDIR)\*.exp $(GLOBJDIR)\*.ilk $(GLOBJDIR)\*.pdb $(GLOBJDIR)\*.lib\
 $(BINDIR)\*.exp $(BINDIR)\*.ilk $(BINDIR)\*.pdb $(BINDIR)\*.lib obj.pdb\
 obj.idb $(GLOBJDIR)\gs.pch $(SBRDIR)\*.sbr $(GLOBJDIR)\cups\*.h

!ifdef BSCFILE
BEGINFILES2=$(BEGINFILES2) $(BSCFILE)
!endif

!include $(GLSRCDIR)\msvccmd.mak
# *romfs.mak must precede lib.mak
!include $(PSSRCDIR)\psromfs.mak

!if $(BUILD_PCL)
!include $(PLSRCDIR)\plromfs.mak
!endif
!if $(BUILD_XPS)
!include $(XPSSRCDIR)\xpsromfs.mak
!endif

!include $(GLSRCDIR)\winlib.mak

!if $(BUILD_PCL)
!include $(PLSRCDIR)\pl.mak
!include $(PCL5SRCDIR)\pcl.mak
!include $(PCL5SRCDIR)\pcl_top.mak
!include $(PXLSRCDIR)\pxl.mak
!endif

!if $(BUILD_XPS)
!include $(XPSSRCDIR)\xps.mak
!endif

!if $(BUILD_GPDL)
!include $(GPDLSRCDIR)\gpdl.mak
!endif

!include $(GLSRCDIR)\msvctail.mak
!include $(PSSRCDIR)\winint.mak
# ----------------------------- Main program ------------------------------ #

GSCONSOLE_XE=$(BINDIR)\$(GSCONSOLE).exe
GSDLL_DLL=$(BINDIR)\$(GSDLL).dll
GSDLL_OBJS=$(PSOBJ)gsdll.$(OBJ) $(GLOBJ)gp_msdll.$(OBJ)

GPCL6DLL_DLL=$(BINDIR)\$(GPCL6DLL).dll
GXPSDLL_DLL=$(BINDIR)\$(GXPSDLL).dll
GPDLDLL_DLL=$(BINDIR)\$(GPDLDLL).dll

INT_ARCHIVE_SOME=$(GLOBJ)gconfig.$(OBJ) $(GLOBJ)gscdefs.$(OBJ)
INT_ARCHIVE_ALL=$(PSOBJ)imainarg.$(OBJ) $(PSOBJ)imain.$(OBJ) $(GLOBJ)iconfig.$(OBJ) \
 $(INT_ARCHIVE_SOME)

!if $(TDEBUG) != 0
$(PSGEN)lib.rsp: $(TOP_MAKEFILES)
	echo /NODEFAULTLIB:LIBC.lib > $(PSGEN)lib.rsp
	echo /NODEFAULTLIB:LIBCMT.lib >> $(PSGEN)lib.rsp
!ifdef METRO
	echo kernel32.lib runtimeobject.lib rpcrt4.lib >> $(PSGEN)lib.rsp
!else
	echo LIBCMTD.lib >> $(PSGEN)lib.rsp
!endif
!else
$(PSGEN)lib.rsp: $(TOP_MAKEFILES)
	echo /NODEFAULTLIB:LIBC.lib > $(PSGEN)lib.rsp
	echo /NODEFAULTLIB:LIBCMTD.lib >> $(PSGEN)lib.rsp
!ifdef METRO
	echo kernel32.lib runtimeobject.lib rpcrt4.lib >> $(PSGEN)lib.rsp
!else
	echo LIBCMT.lib >> $(PSGEN)lib.rsp
!endif
!endif

# a bit naff - find some way to combine above and this....
!if $(TDEBUG) != 0
$(PCLGEN)pcllib.rsp: $(TOP_MAKEFILES)
	echo /NODEFAULTLIB:LIBC.lib > $(PCLGEN)pcllib.rsp
	echo /NODEFAULTLIB:LIBCMT.lib >> $(PCLGEN)pcllib.rsp
!ifdef METRO
	echo kernel32.lib runtimeobject.lib rpcrt4.lib >> $(PCLGEN)pcllib.rsp
!else
	echo LIBCMTD.lib >> $(PCLGEN)pcllib.rsp
!endif
!else
$(PCLGEN)pcllib.rsp: $(TOP_MAKEFILES)
	echo /NODEFAULTLIB:LIBC.lib > $(PCLGEN)pcllib.rsp
	echo /NODEFAULTLIB:LIBCMTD.lib >> $(PCLGEN)pcllib.rsp
!ifdef METRO
	echo kernel32.lib runtimeobject.lib rpcrt4.lib >> $(PCLGEN)pcllib.rsp
!else
	echo LIBCMT.lib >> $(PCLGEN)pcllib.rsp
!endif
!endif

!if $(TDEBUG) != 0

$(XPSGEN)xpslib.rsp: $(TOP_MAKEFILES)
	echo /NODEFAULTLIB:LIBC.lib > $(XPSGEN)xpslib.rsp
	echo /NODEFAULTLIB:LIBCMT.lib >> $(XPSGEN)xpslib.rsp
!ifdef METRO
	echo kernel32.lib runtimeobject.lib rpcrt4.lib >> $(XPSGEN)xpslib.rsp
!else
	echo LIBCMTD.lib >> $(XPSGEN)xpslib.rsp
!endif
!else
$(XPSGEN)xpslib.rsp: $(TOP_MAKEFILES)
	echo /NODEFAULTLIB:LIBC.lib > $(XPSGEN)xpslib.rsp
	echo /NODEFAULTLIB:LIBCMTD.lib >> $(XPSGEN)xpslib.rsp
!ifdef METRO
	echo kernel32.lib runtimeobject.lib rpcrt4.lib >> $(XPSGEN)xpslib.rsp
!else
	echo LIBCMT.lib >> $(XPSGEN)xpslib.rsp
!endif
!endif

!if $(TDEBUG) != 0

$(GPDLGEN)gpdllib.rsp: $(TOP_MAKEFILES)
	echo /NODEFAULTLIB:LIBC.lib > $(XPSGEN)gpdllib.rsp
	echo /NODEFAULTLIB:LIBCMT.lib >> $(XPSGEN)gpdllib.rsp
!ifdef METRO
	echo kernel32.lib runtimeobject.lib rpcrt4.lib >> $(XPSGEN)gpdllib.rsp
!else
	echo LIBCMTD.lib >> $(XPSGEN)gpdllib.rsp
!endif
!else
$(GPDLGEN)gpdllib.rsp: $(TOP_MAKEFILES)
	echo /NODEFAULTLIB:LIBC.lib > $(XPSGEN)gpdllib.rsp
	echo /NODEFAULTLIB:LIBCMTD.lib >> $(XPSGEN)gpdllib.rsp
!ifdef METRO
	echo kernel32.lib runtimeobject.lib rpcrt4.lib >> $(XPSGEN)gpdllib.rsp
!else
	echo LIBCMT.lib >> $(XPSGEN)gpdllib.rsp
!endif
!endif


!if $(MAKEDLL)
# The graphical small EXE loader
!ifdef METRO
# For METRO build only the dll
$(GS_XE): $(GSDLL_DLL)

!else
$(GS_XE): $(GSDLL_DLL)  $(DWOBJ) $(GSCONSOLE_XE) $(GLOBJ)gp_wutf8.$(OBJ) $(TOP_MAKEFILES)
	echo /SUBSYSTEM:WINDOWS > $(PSGEN)gswin.rsp
!if "$(PROFILE)"=="1"
	echo /PROFILE >> $(PSGEN)gswin.rsp 
!endif
!ifdef WIN64
	echo /DEF:$(PSSRCDIR)\dwmain64.def /OUT:$(GS_XE) >> $(PSGEN)gswin.rsp
!else
	echo /DEF:$(PSSRCDIR)\dwmain32.def /OUT:$(GS_XE) >> $(PSGEN)gswin.rsp
!endif
	$(LINK) $(LCT) @$(PSGEN)gswin.rsp $(DWOBJ) $(LINKLIBPATH) @$(LIBCTR) $(GS_OBJ).res $(GLOBJ)gp_wutf8.$(OBJ)
	del $(PSGEN)gswin.rsp
!endif

# The console mode small EXE loader
$(GSCONSOLE_XE): $(OBJC) $(GS_OBJ).res $(PSSRCDIR)\dw64c.def $(PSSRCDIR)\dw32c.def $(GLOBJ)gp_wutf8.$(OBJ) $(TOP_MAKEFILES)
	echo /SUBSYSTEM:CONSOLE > $(PSGEN)gswin.rsp
!if "$(PROFILE)"=="1"
	echo /PROFILE >> $(PSGEN)gswin.rsp
!endif
!ifdef WIN64
	echo  /DEF:$(PSSRCDIR)\dw64c.def /OUT:$(GSCONSOLE_XE) >> $(PSGEN)gswin.rsp
!else
	echo  /DEF:$(PSSRCDIR)\dw32c.def /OUT:$(GSCONSOLE_XE) >> $(PSGEN)gswin.rsp
!endif
	$(LINK) $(LCT) @$(PSGEN)gswin.rsp $(OBJC) $(LINKLIBPATH) @$(LIBCTR) $(GS_OBJ).res $(GLOBJ)gp_wutf8.$(OBJ)

# The big DLL
$(GSDLL_DLL): $(ECHOGS_XE) $(gs_tr) $(GS_ALL) $(DEVS_ALL) $(GSDLL_OBJS) $(GSDLL_OBJ).res $(PSGEN)lib.rsp \
              $(PSOBJ)gsromfs$(COMPILE_INITS).$(OBJ) $(TOP_MAKEFILES)
	echo Linking $(GSDLL)  $(GSDLL_DLL) $(METRO)
	echo /DLL /DEF:$(PSSRCDIR)\$(GSDLL).def /OUT:$(GSDLL_DLL) > $(PSGEN)gswin.rsp
!if "$(PROFILE)"=="1"
	echo /PROFILE >> $(PSGEN)gswin.rsp
!endif
	$(LINK) $(LCT) @$(PSGEN)gswin.rsp $(GSDLL_OBJS) @$(gsld_tr) $(PSOBJ)gsromfs$(COMPILE_INITS).$(OBJ) @$(PSGEN)lib.rsp $(LINKLIBPATH) @$(LIBCTR) $(GSDLL_OBJ).res
	del $(PSGEN)gswin.rsp

$(GPCL6DLL_DLL): $(ECHOGS_XE) $(GSDLL_OBJ).res $(LIBCTR) $(LIB_ALL) $(PCL_DEVS_ALL) $(PCLGEN)pcllib.rsp \
                $(PCLOBJ)pclromfs$(COMPILE_INITS).$(OBJ) $(ld_tr) $(pcl_tr) $(MAIN_OBJ) $(TOP_OBJ) \
                $(XOBJS) $(INT_ARCHIVE_SOME) $(TOP_MAKEFILES)
	echo Linking $(GPCL6DLL)  $(GPCL6DLL_DLL) $(METRO)
	copy $(ld_tr) $(PCLGEN)gpclwin.tr
	$(ECHOGS_XE) -a $(PCLGEN)gpclwin.tr -n -R $(pcl_tr)
	echo $(MAIN_OBJ) $(TOP_OBJ) $(INT_ARCHIVE_SOME) $(XOBJS) >> $(PCLGEN)gpclwin.tr
	echo $(PCLOBJ)pclromfs$(COMPILE_INITS).$(OBJ) >> $(PCLGEN)gpclwin.tr
	echo /DLL /DEF:$(PLSRCDIR)\$(GPCL6DLL).def /OUT:$(GPCL6DLL_DLL) > $(PCLGEN)gpclwin.rsp
!if "$(PROFILE)"=="1"
	echo /PROFILE >> $(PSGEN)gpclwin.rsp
!endif
	$(LINK) $(LCT) @$(PCLGEN)gpclwin.rsp $(GPCL6DLL_OBJS) @$(PCLGEN)gpclwin.tr @$(PSGEN)pcllib.rsp $(LINKLIBPATH) @$(LIBCTR) $(GSDLL_OBJ).res
	del $(PCLGEN)gpclwin.rsp

$(GPCL_XE): $(GPCL6DLL_DLL) $(DWMAINOBJS) $(GS_OBJ).res $(TOP_MAKEFILES)
	echo /SUBSYSTEM:CONSOLE > $(PCLGEN)gpclwin.rsp
!if "$(PROFILE)"=="1"
	echo /PROFILE >> $(PCLGEN)gpclwin.rsp
!endif
!ifdef WIN64
	echo  /OUT:$(GPCL_XE) >> $(PCLGEN)gpclwin.rsp
!else
	echo  /OUT:$(GPCL_XE) >> $(PCLGEN)gpclwin.rsp
!endif
	$(LINK) $(LCT) @$(PCLGEN)gpclwin.rsp $(DWMAINOBJS) $(BINDIR)\$(GPCL6DLL).lib $(LINKLIBPATH) @$(LIBCTR) $(GS_OBJ).res
	del $(PXLGEN)gpclwin.rsp


$(GXPSDLL_DLL): $(ECHOGS_XE) $(GSDLL_OBJ).res $(LIBCTR) $(LIB_ALL) $(XPS_DEVS_ALL) $(XPSGEN)xpslib.rsp \
                $(XPSOBJ)xpsromfs$(COMPILE_INITS).$(OBJ) $(ld_tr) $(xps_tr) $(MAIN_OBJ) $(XPS_TOP_OBJS) \
                $(XOBJS) $(INT_ARCHIVE_SOME) $(TOP_MAKEFILES)
	echo Linking $(GXPSDLL)  $(GXPSDLL_DLL) $(METRO)
	copy $(ld_tr) $(XPSGEN)gxpswin.tr
	$(ECHOGS_XE) -a $(XPSGEN)gxpswin.tr -n -R $(xps_tr)
	echo $(MAIN_OBJ) $(XPS_TOP_OBJS) $(INT_ARCHIVE_SOME) $(XOBJS) >> $(XPSGEN)gxpswin.tr
	echo $(PCLOBJ)xpsromfs$(COMPILE_INITS).$(OBJ) >> $(XPSGEN)gxpswin.tr
	echo /DLL /DEF:$(PLSRCDIR)\$(GXPSDLL).def /OUT:$(GXPSDLL_DLL) > $(XPSGEN)gxpswin.rsp
!if "$(PROFILE)"=="1"
	echo /PROFILE >> $(XPSGEN)gxpswin.rsp
!endif
	$(LINK) $(LCT) @$(XPSGEN)gxpswin.rsp $(GXPSDLL_OBJS) @$(XPSGEN)gxpswin.tr @$(XPSGEN)xpslib.rsp $(LINKLIBPATH) @$(LIBCTR) $(GSDLL_OBJ).res
	del $(PCLGEN)gxpswin.rsp

$(GXPS_XE): $(GXPSDLL_DLL) $(DWMAINOBJS) $(GS_OBJ).res $(TOP_MAKEFILES)
	echo /SUBSYSTEM:CONSOLE > $(XPSGEN)gxpswin.rsp
!if "$(PROFILE)"=="1"
	echo /PROFILE >> $(XPSGEN)gxpswin.rsp
!endif
!ifdef WIN64
	echo  /OUT:$(GXPS_XE) >> $(XPSGEN)gxpswin.rsp
!else
	echo  /OUT:$(GXPS_XE) >> $(XPSGEN)gxpswin.rsp
!endif
	$(LINK) $(LCT) @$(XPSGEN)gxpswin.rsp $(DWMAINOBJS) $(BINDIR)\$(GXPSDLL).lib $(LINKLIBPATH) @$(LIBCTR) $(GS_OBJ).res
	del $(XPSGEN)gxpswin.rsp



$(GPDLDLL_DLL): $(ECHOGS_XE) $(GSDLL_OBJ).res $(LIBCTR) $(LIB_ALL) $(PCL_DEVS_ALL) $(XPS_DEVS_ALL) $(GS_ALL) \
                $(GPDLGEN)gpdllib.rsp $(GPDLOBJ)pdlromfs$(COMPILE_INITS).$(OBJ) \
                $(ld_tr) $(gpdl_tr) $(MAIN_OBJ) $(XPS_TOP_OBJS) $(GPDL_PSI_TOP_OBJS) $(PCL_PXL_TOP_OBJS) $(PSI_TOP_OBJ) $(XPS_TOP_OBJ) \
		$(REALMAIN_OBJ) $(MAIN_OBJ) $(XOBJS) $(INT_ARCHIVE_SOME) $(TOP_MAKEFILES)
	echo Linking $(GPDLDLL)  $(GPDLDLL_DLL) $(METRO)
	copy $(gpdlld_tr) $(GPDLGEN)gpdlwin.tr
	echo $(MAIN_OBJ) $(GPDL_PSI_TOP_OBJS) $(PCL_PXL_TOP_OBJS) $(PSI_TOP_OBJ) $(XPS_TOP_OBJ) $(XOBJS) >> $(GPDLGEN)gpdlwin.tr
	echo $(PCLOBJ)pdlromfs$(COMPILE_INITS).$(OBJ) >> $(GPDLGEN)gpdlwin.tr
	echo /DLL /DEF:$(PLSRCDIR)\$(GPDLDLL).def /OUT:$(GPDLDLL_DLL) > $(GPDLGEN)gpdlwin.rsp
!if "$(PROFILE)"=="1"
	echo /PROFILE >> $(GPDLGEN)gpdlwin.rsp
!endif
	$(LINK) $(LCT) @$(GPDLGEN)gpdlwin.rsp $(GPDLDLL_OBJS) @$(GPDLGEN)gpdlwin.tr @$(GPDLGEN)gpdllib.rsp $(LINKLIBPATH) @$(LIBCTR) $(GSDLL_OBJ).res
	del $(GPDLGEN)gpdlwin.rsp

$(GPDL_XE): $(GPDLDLL_DLL) $(DWMAINOBJS) $(GS_OBJ).res $(TOP_MAKEFILES)
	echo /SUBSYSTEM:CONSOLE > $(GPDLGEN)gpdlwin.rsp
!if "$(PROFILE)"=="1"
	echo /PROFILE >> $(XPSGEN)gpdlwin.rsp
!endif
!ifdef WIN64
	echo  /OUT:$(GPDL_XE) >> $(GPDLGEN)gpdlwin.rsp
!else
	echo  /OUT:$(GPDL_XE) >> $(GPDLGEN)gpdlwin.rsp
!endif
	$(LINK) $(LCT) @$(GPDLGEN)gpdlwin.rsp $(DWMAINOBJS) $(BINDIR)\$(GPDLDLL).lib $(LINKLIBPATH) @$(LIBCTR) $(GS_OBJ).res
	del $(GPDLGEN)gpdlwin.rsp


!else
# The big graphical EXE
$(GS_XE): $(GSCONSOLE_XE) $(GS_ALL) $(DEVS_ALL) $(GSDLL_OBJS) $(DWOBJNO) $(GSDLL_OBJ).res $(PSSRCDIR)\dwmain32.def\
		$(ld_tr) $(gs_tr) $(PSSRCDIR)\dwmain64.def $(PSGEN)lib.rsp $(PSOBJ)gsromfs$(COMPILE_INITS).$(OBJ) \
                $(TOP_MAKEFILES)
	copy $(gsld_tr) $(PSGEN)gswin.tr
	echo $(PSOBJ)gsromfs$(COMPILE_INITS).$(OBJ) >> $(PSGEN)gswin.tr
	echo $(PSOBJ)dwnodll.obj >> $(PSGEN)gswin.tr
	echo $(GLOBJ)dwimg.obj >> $(PSGEN)gswin.tr
	echo $(PSOBJ)dwmain.obj >> $(PSGEN)gswin.tr
	echo $(GLOBJ)dwtext.obj >> $(PSGEN)gswin.tr
	echo $(GLOBJ)dwreg.obj >> $(PSGEN)gswin.tr
!ifdef WIN64
	echo /DEF:$(PSSRCDIR)\dwmain64.def /OUT:$(GS_XE) > $(PSGEN)gswin.rsp
!else
	echo /DEF:$(PSSRCDIR)\dwmain32.def /OUT:$(GS_XE) > $(PSGEN)gswin.rsp
!endif
!if "$(PROFILE)"=="1"
	echo /PROFILE >> $(PSGEN)gswin.rsp
!endif
	$(LINK) $(LCT) @$(PSGEN)gswin.rsp $(GLOBJ)gsdll @$(PSGEN)gswin.tr $(LINKLIBPATH) @$(LIBCTR) @$(PSGEN)lib.rsp $(GSDLL_OBJ).res $(DWTRACE)
	del $(PSGEN)gswin.tr
	del $(PSGEN)gswin.rsp

# The big console mode EXE
$(GSCONSOLE_XE): $(ECHOGS_XE) $(gs_tr) $(GS_ALL) $(DEVS_ALL) $(GSDLL_OBJS) $(OBJCNO) $(GS_OBJ).res $(PSSRCDIR)\dw64c.def $(PSSRCDIR)\dw32c.def \
		$(PSGEN)lib.rsp $(PSOBJ)gsromfs$(COMPILE_INITS).$(OBJ) $(TOP_MAKEFILES)
	copy $(gsld_tr) $(PSGEN)gswin.tr
	echo $(PSOBJ)gsromfs$(COMPILE_INITS).$(OBJ) >> $(PSGEN)gswin.tr
	echo $(PSOBJ)dwnodllc.obj >> $(PSGEN)gswin.tr
	echo $(GLOBJ)dwimg.obj >> $(PSGEN)gswin.tr
	echo $(PSOBJ)dwmainc.obj >> $(PSGEN)gswin.tr
	echo $(PSOBJ)dwreg.obj >> $(PSGEN)gswin.tr
	echo /SUBSYSTEM:CONSOLE > $(PSGEN)gswin.rsp
!ifdef WIN64
	echo /DEF:$(PSSRCDIR)\dw64c.def /OUT:$(GSCONSOLE_XE) >> $(PSGEN)gswin.rsp
!else
	echo /DEF:$(PSSRCDIR)\dw32c.def /OUT:$(GSCONSOLE_XE) >> $(PSGEN)gswin.rsp
!endif
	$(LINK) $(LCT) @$(PSGEN)gswin.rsp $(GLOBJ)gsdll @$(PSGEN)gswin.tr $(LINKLIBPATH) @$(LIBCTR) @$(PSGEN)lib.rsp $(GS_OBJ).res $(DWTRACE)
	del $(PSGEN)gswin.rsp
	del $(PSGEN)gswin.tr


$(GPCL_XE): $(ECHOGS_XE) $(LIBCTR) $(LIB_ALL) $(WINMAINOBJS) $(PCL_DEVS_ALL) $(PCLGEN)pcllib.rsp \
                $(PCLOBJ)pclromfs$(COMPILE_INITS).$(OBJ) \
		$(ld_tr) $(pcl_tr) $(MAIN_OBJ) $(TOP_OBJ) $(XOBJS) $(INT_ARCHIVE_SOME) \
                $(TOP_MAKEFILES)
	copy $(ld_tr) $(PCLGEN)gpclwin.tr
	$(ECHOGS_XE) -a $(PCLGEN)gpclwin.tr -n -R $(pcl_tr)
	echo $(WINMAINOBJS) $(MAIN_OBJ) $(TOP_OBJ) $(INT_ARCHIVE_SOME) $(XOBJS) >> $(PCLGEN)gpclwin.tr
	echo $(PCLOBJ)pclromfs$(COMPILE_INITS).$(OBJ) >> $(PCLGEN)gpclwin.tr
	echo /SUBSYSTEM:CONSOLE > $(PCLGEN)pclwin.rsp
        echo /OUT:$(GPCL_XE) >> $(PCLGEN)pclwin.rsp
	$(LINK) $(LCT) @$(PCLGEN)pclwin.rsp @$(PCLGEN)gpclwin.tr $(LINKLIBPATH) @$(LIBCTR) @$(PCLGEN)pcllib.rsp
        del $(PCLGEN)pclwin.rsp
        del $(PCLGEN)gpclwin.tr

$(GXPS_XE): $(ECHOGS_XE) $(LIBCTR) $(LIB_ALL) $(WINMAINOBJS) $(XPS_DEVS_ALL) $(XPSGEN)xpslib.rsp \
                $(XPS_TOP_OBJS) $(XPSOBJ)xpsromfs$(COMPILE_INITS).$(OBJ) \
		$(ld_tr) $(xps_tr) $(MAIN_OBJ) $(XOBJS) $(INT_ARCHIVE_SOME) \
                $(TOP_MAKEFILES)
	copy $(ld_tr) $(XPSGEN)gxpswin.tr
	$(ECHOGS_XE) -a $(PCLGEN)gxpswin.tr -n -R $(xps_tr)
	echo $(WINMAINOBJS) $(MAIN_OBJ) $(XPS_TOP_OBJS) $(INT_ARCHIVE_SOME) $(XOBJS) >> $(XPSGEN)gxpswin.tr
	echo $(PCLOBJ)xpsromfs$(COMPILE_INITS).$(OBJ) >> $(XPSGEN)gxpswin.tr
	echo /SUBSYSTEM:CONSOLE > $(XPSGEN)xpswin.rsp
        echo /OUT:$(GXPS_XE) >> $(XPSGEN)xpswin.rsp
	$(LINK) $(LCT) @$(XPSGEN)xpswin.rsp @$(XPSGEN)gxpswin.tr $(LINKLIBPATH) @$(LIBCTR) @$(XPSGEN)xpslib.rsp
        del $(XPSGEN)xpswin.rsp
        del $(XPSGEN)gxpswin.tr

$(GPDL_XE): $(ECHOGS_XE) $(ld_tr) $(gpdl_tr) $(LIBCTR) $(LIB_ALL) $(WINMAINOBJS) $(XPS_DEVS_ALL) $(PCL_DEVS_ALL) $(GS_ALL) \
                $(GPDLGEN)gpdllib.rsp $(GPDLOBJ)pdlromfs$(COMPILE_INITS).$(OBJ) \
                $(GPDL_PSI_TOP_OBJS) $(PCL_PXL_TOP_OBJS) $(PSI_TOP_OBJ) $(XPS_TOP_OBJ) \
		$(MAIN_OBJ) $(XOBJS) $(INT_ARCHIVE_SOME) \
                $(TOP_MAKEFILES)
	copy $(gpdlld_tr) $(GPDLGEN)gpdlwin.tr
	echo $(WINMAINOBJS) $(MAIN_OBJ) $(GPDL_PSI_TOP_OBJS) $(PCL_PXL_TOP_OBJS) $(PSI_TOP_OBJ) $(XPS_TOP_OBJ) $(XOBJS) >> $(GPDLGEN)gpdlwin.tr
	echo $(PCLOBJ)pdlromfs$(COMPILE_INITS).$(OBJ) >> $(GPDLGEN)gpdlwin.tr
	echo /SUBSYSTEM:CONSOLE > $(GPDLGEN)gpdlwin.rsp
        echo /OUT:$(GPDL_XE) >> $(GPDLGEN)gpdlwin.rsp
	$(LINK) $(LCT) @$(GPDLGEN)gpdlwin.rsp @$(GPDLGEN)gpdlwin.tr $(LINKLIBPATH) @$(LIBCTR) @$(GPDLGEN)gpdllib.rsp
	del $(GPDLGEN)gpdlwin.rsp
	del $(GPDLGEN)gpdlwin.tr
!endif

# ---------------------- Debug targets ---------------------- #
# Simply set some definitions and call ourselves back         #

!ifdef WIN64
WINDEFS=WIN64= BUILD_SYSTEM="$(BUILD_SYSTEM)" PGMFILES="$(PGMFILES)" PGMFILESx86="$(PGMFILESx86)"
!else
WINDEFS=BUILD_SYSTEM="$(BUILD_SYSTEM)" PGMFILES="$(PGMFILES)" PGMFILESx86="$(PGMFILESx86)"
!endif

DEBUGDEFS=DEBUG=1 TDEBUG=1

debug:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(DEBUGDEFS) $(WINDEFS)

gsdebug:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(DEBUGDEFS) $(WINDEFS) gs

gpcl6debug:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(DEBUGDEFS) $(WINDEFS) gpcl6

gxpsdebug:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(DEBUGDEFS) $(WINDEFS) gxps

gpdldebug:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(DEBUGDEFS) $(WINDEFS) gpdl

debugclean:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(DEBUGDEFS) $(WINDEFS) clean

debugbsc:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(DEBUGDEFS) $(WINDEFS) bsc

# --------------------- Memento targets --------------------- #
# Simply set some definitions and call ourselves back         #

MEMENTODEFS=$(DEBUGDEFS) MEMENTO=1

memento-target:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(MEMENTODEFS) $(WINDEFS)

gsmemento:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(MEMENTODEFS) $(WINDEFS) gs

gpcl6memento:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(MEMENTODEFS) $(WINDEFS) gpcl6

gxpsmemento:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(MEMENTODEFS) $(WINDEFS) gxps

gpdlmemento:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(MEMENTODEFS) $(WINDEFS) gpdl

mementoclean:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(MEMENTODEFS) $(WINDEFS) clean

mementobsc:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(MEMENTODEFS) $(WINDEFS) bsc

# --------------------- Profile targets --------------------- #
# Simply set some definitions and call ourselves back         #

PROFILEDEFS=PROFILE=1

profile:
profile-target:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(PROFILEDEFS) $(WINDEFS)

gsprofile:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(PROFILEDEFS) $(WINDEFS) gs

gpcl6profile:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(PROFILEDEFS) $(WINDEFS) gpcl6

gxpsprofile:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(PROFILEDEFS) $(WINDEFS) gxps

gpdlprofile:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(PROFILEDEFS) $(WINDEFS) gpdl

profileclean:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(PROFILEDEFS) $(WINDEFS) clean

profilebsc:
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" FT_BRIDGE=$(FT_BRIDGE) $(PROFILEDEFS) $(WINDEFS) bsc



# ---------------------- UFST targets ---------------------- #
# Simply set some definitions and call ourselves back        #

!ifndef UFST_ROOT
UFST_ROOT=C:\ufst
!endif

UFST_ROMFS_ARGS=-b \
   -P $(UFST_ROOT)/fontdata/mtfonts/pcl45/mt3/ -d fontdata/mtfonts/pcl45/mt3/ pcl___xj.fco plug__xi.fco wd____xh.fco \
   -P $(UFST_ROOT)/fontdata/mtfonts/pclps2/mt3/ -d fontdata/mtfonts/pclps2/mt3/ pclp2_xj.fco \
   -c -P $(PSSRCDIR)/../lib/ -d Resource/Init/ FAPIconfig-FCO

UFSTROMFONTDIR=\\\"%%%%%rom%%%%%fontdata/\\\"
UFSTDISCFONTDIR="$(UFST_ROOT)/fontdata"

UFSTBASEDEFS=UFST_BRIDGE=1 FT_BRIDGE=1 UFST_ROOT="$(UFST_ROOT)" UFST_ROMFS_ARGS="$(UFST_ROMFS_ARGS)" UFSTFONTDIR="$(UFSTFONTDIR)" UFSTROMFONTDIR="$(UFSTROMFONTDIR)"

!ifdef WIN64
UFSTDEBUGDEFS=BINDIR=.\ufstdebugbin GLGENDIR=.\ufstdebugobj64 GLOBJDIR=.\ufstdebugobj64 PSLIBDIR=.\lib PSGENDIR=.\ufstdebugobj64 PSOBJDIR=.\ufstdebugobj64 DEBUG=1 TDEBUG=1 SBRDIR=.\ufstdebugobj64
UFSTDEFS=BINDIR=.\ufstbin GLGENDIR=.\ufstobj64 GLOBJDIR=.\ufstobj64 PSLIBDIR=.\lib PSGENDIR=.\ufstobj64 PSOBJDIR=.\ufstobj64 SBRDIR=.\ufstobj64
!else
UFSTDEBUGDEFS=BINDIR=.\ufstdebugbin GLGENDIR=.\ufstdebugobj GLOBJDIR=.\ufstdebugobj PSLIBDIR=.\lib PSGENDIR=.\ufstdebugobj PSOBJDIR=.\ufstdebugobj DEBUG=1 TDEBUG=1 SBRDIR=.\ufstdebugobj
UFSTDEFS=BINDIR=.\ufstbin GLGENDIR=.\ufstobj GLOBJDIR=.\ufstobj PSLIBDIR=.\lib PSGENDIR=.\ufstobj PSOBJDIR=.\ufstobj SBRDIR=.\ufstobj
!endif

ufst-lib:
#	Could make this call a makefile in the ufst code?
#	cd $(UFST_ROOT)\rts\lib
#	nmake -f makefile.artifex fco_lib.a if_lib.a psi_lib.a tt_lib.a

ufst-debug: ufst-lib
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" $(UFSTBASEDEFS) $(UFSTDEBUGDEFS) UFST_CFLAGS="$(UFST_CFLAGS)" $(WINDEFS)

ufst-debugclean: ufst-lib
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" $(UFSTBASEDEFS) $(UFSTDEBUGDEFS) UFST_CFLAGS="$(UFST_CFLAGS)" $(WINDEFS) clean

ufst-debugbsc: ufst-lib
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" $(UFSTBASEDEFS) $(UFSTDEBUGDEFS) UFST_CFLAGS="$(UFST_CFLAGS)" $(WINDEFS) bsc

ufst: ufst-lib
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" $(UFSTBASEDEFS) $(UFSTDEFS) UFST_CFLAGS="$(UFST_CFLAGS)" $(WINDEFS)

ufst-clean: ufst-lib
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" $(UFSTBASEDEFS) $(UFSTDEFS) UFST_CFLAGS="$(UFST_CFLAGS)" $(WINDEFS) clean

ufst-bsc: ufst-lib
	nmake -f $(MAKEFILE) DEVSTUDIO="$(DEVSTUDIO)" $(UFSTBASEDEFS) $(UFSTDEFS) UFST_CFLAGS="$(UFST_CFLAGS)" $(WINDEFS) bsc

#----------------------- Individual Product Targets --------------------#
gs:$(GS_XE) $(GSCONSOLE_XE)
	$(NO_OP)

gpcl6:$(GPCL_XE)
	$(NO_OP)

gxps:$(GXPS_XE)
	$(NO_OP)

gpdl:$(GPDL_XE)
	$(NO_OP)


# ---------------------- Browse information step ---------------------- #

bsc:
	bscmake /o $(SBRDIR)\ghostscript.bsc /v $(GLOBJDIR)\*.sbr

# end of makefile
