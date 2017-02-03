# Copyright (C) 2001-2012 Artifex Software, Inc.
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
# $Id: lcupsi.mak 11073 2010-04-15 08:30:48Z chrisl $
# makefile for libcupsimage as part of the monolithic gs build.
#
# Users of this makefile must define the following:
#	LCUPSISRCDIR    - the source directory
#	LCUPSIGENDIR    - the generated intermediate file directory
#	LCUPSIOBJBJDIR   - the object file directory
#	LCUPSI_CFLAGS   - The include options for the libcupsimage library
#	SHARE_LCUPSI - 0 to compile in libcupsimage, 1 to link a shared library
#	LCUPSI_LIBS  - if SHARE_LCUPSI=1, the link options for the shared library

# (Rename directories.)
LIBCUPSISRC=$(LCUPSISRCDIR)$(D)libs$(D)filter$(D)
LIBCUPSIGEN=$(LCUPSIGENDIR)$(D)
LIBCUPSIOBJ=$(LCUPSIOBJDIR)$(D)
LCUPSIO_=$(O_)$(LIBCUPSIOBJ)

# NB: we can't use the normal $(CC_) here because msvccmd.mak
# adds /Za which conflicts with the cups source.
LCUPSI_CC=$(CUPS_CC) $(I_)$(LIBCUPSISRC) $(I_)$(LIBCUPSIGEN)$(D)cups $(I_)$(LCUPSISRCDIR)$(D)libs $(I_)$(GLGENDIR) \
         $(I_)$(ZSRCDIR) $(I_)$(PNGSRCDIR) $(I_)$(TIFFSRCDIR) $(I_)$(TIFFCONFDIR) $(I_)$(TI_) 

# Define the name of this makefile.
LCUPSI_MAK=$(GLSRC)lcupsi.mak $(TOP_MAKEFILES)

LIBCUPSI_OBJS =\
    $(LIBCUPSIOBJ)error.$(OBJ) \
    $(LIBCUPSIOBJ)interpret.$(OBJ) \
    $(LIBCUPSIOBJ)raster.$(OBJ)

LIBCUPSI_DEPS	=	\
		$(LIBCUPSISRC)common.h $(LCUPSI_MAK) \
		$(MAKEDIRS)

libcupsi.clean : libcupsi.config-clean libcupsi.clean-not-config-clean

libcupsi.clean-not-config-clean :
	$(EXP)$(ECHOGS_XE) $(LIBCUPSISRC) $(LIBCUPSIOBJ)
	$(RM_) $(LIBCUPSIOBJ)*.$(OBJ)

libcupsi.config-clean :
	$(RMN_) $(LIBCUPSIGEN)$(D)lcupsi*.dev

# instantiate the requested build option (shared or compiled in)
$(LIBCUPSIGEN)lcupsi.dev : $(LIBCUPSIGEN)lcupsi_$(SHARE_LCUPSI).dev\
 $(LCUPSI_MAK) $(MAKEDIRS)
	$(CP_) $(LIBCUPSIGEN)lcupsi_$(SHARE_LCUPSI).dev $(LIBCUPSIGEN)lcupsi.dev

# Define the shared version.
$(LIBCUPSIGEN)lcupsi_1.dev : $(ECHOGS_XE) $(LCUPSI_MAK) \
		$(LCUPSI_MAK) $(MAKEDIRS)
	$(SETMOD) $(LIBCUPSIGEN)lcupsi_1 -link $(LCUPSI_LIBS)

# Define the non-shared version.
$(LIBCUPSIGEN)lcupsi_0.dev : $(ECHOGS_XE) $(LIBCUPSI_OBJS) $(LIBCUPSI_DEPS)
	$(SETMOD) $(LIBCUPSIGEN)lcupsi_0 $(LIBCUPSI_OBJS)

# explicit rules for building the source files
# for simplicity we have every source file depend on all headers

$(LIBCUPSIOBJ)image-bmp.$(OBJ) : $(LIBCUPSISRC)image-bmp.c $(LIBCUPSI_DEPS)
	$(LCUPSI_CC) $(LCUPSIO_)image-bmp.$(OBJ) $(C_) $(LIBCUPSISRC)image-bmp.c

$(LIBCUPSIOBJ)image-colorspace.$(OBJ) : $(LIBCUPSISRC)image-colorspace.c $(LIBCUPSI_DEPS)
	$(LCUPSI_CC) $(LCUPSIO_)image-colorspace.$(OBJ) $(C_) $(LIBCUPSISRC)image-colorspace.c
	
$(LIBCUPSIOBJ)image-gif.$(OBJ) : $(LIBCUPSISRC)image-gif.c $(LIBCUPSI_DEPS)
	$(LCUPSI_CC) $(LCUPSIO_)image-gif.$(OBJ) $(C_) $(LIBCUPSISRC)image-gif.c
	
$(LIBCUPSIOBJ)image-jpeg.$(OBJ) : $(LIBCUPSISRC)image-jpeg.c $(LIBCUPSI_DEPS)
	$(LCUPSI_CC) $(LCUPSIO_)image-jpeg.$(OBJ) $(C_) $(LIBCUPSISRC)image-jpeg.c
	
$(LIBCUPSIOBJ)image-photocd.$(OBJ) : $(LIBCUPSISRC)image-photocd.c $(LIBCUPSI_DEPS)
	$(LCUPSI_CC) $(LCUPSIO_)image-photocd.$(OBJ) $(C_) $(LIBCUPSISRC)image-photocd.c
	
$(LIBCUPSIOBJ)image-pix.$(OBJ) : $(LIBCUPSISRC)image-pix.c $(LIBCUPSI_DEPS)
	$(LCUPSI_CC) $(LCUPSIO_)image-pix.$(OBJ) $(C_) $(LIBCUPSISRC)image-pix.c
	
$(LIBCUPSIOBJ)image-png.$(OBJ) : $(LIBCUPSISRC)image-png.c $(LIBCUPSI_DEPS)
	$(LCUPSI_CC) $(LCUPSIO_)image-png.$(OBJ) $(C_) $(LIBCUPSISRC)image-png.c
	
$(LIBCUPSIOBJ)image-pnm.$(OBJ) : $(LIBCUPSISRC)image-pnm.c $(LIBCUPSI_DEPS)
	$(LCUPSI_CC) $(LCUPSIO_)image-pnm.$(OBJ) $(C_) $(LIBCUPSISRC)image-pnm.c
	
$(LIBCUPSIOBJ)image-sgi.$(OBJ) : $(LIBCUPSISRC)image-sgi.c $(LIBCUPSI_DEPS)
	$(LCUPSI_CC) $(LCUPSIO_)image-sgi.$(OBJ) $(C_) $(LIBCUPSISRC)image-sgi.c
	
$(LIBCUPSIOBJ)image-sgilib.$(OBJ) : $(LIBCUPSISRC)image-sgilib.c $(LIBCUPSI_DEPS)
	$(LCUPSI_CC) $(LCUPSIO_)image-sgilib.$(OBJ) $(C_) $(LIBCUPSISRC)image-sgilib.c
	
$(LIBCUPSIOBJ)image-sun.$(OBJ) : $(LIBCUPSISRC)image-sun.c $(LIBCUPSI_DEPS)
	$(LCUPSI_CC) $(LCUPSIO_)image-sun.$(OBJ) $(C_) $(LIBCUPSISRC)image-sun.c
	
$(LIBCUPSIOBJ)image-tiff.$(OBJ) : $(LIBCUPSISRC)image-tiff.c $(LIBCUPSI_DEPS)
	$(LCUPSI_CC) $(LCUPSIO_)image-tiff.$(OBJ) $(C_) $(LIBCUPSISRC)image-tiff.c
	
$(LIBCUPSIOBJ)image-zoom.$(OBJ) : $(LIBCUPSISRC)image-zoom.c $(LIBCUPSI_DEPS)
	$(LCUPSI_CC) $(LCUPSIO_)image-zoom.$(OBJ) $(C_) $(LIBCUPSISRC)image-zoom.c
	
$(LIBCUPSIOBJ)image.$(OBJ) : $(LIBCUPSISRC)image.c $(LIBCUPSI_DEPS)
	$(LCUPSI_CC) $(LCUPSIO_)image.$(OBJ) $(C_) $(LIBCUPSISRC)image.c
	
$(LIBCUPSIOBJ)error.$(OBJ) : $(LIBCUPSISRC)error.c $(LIBCUPSI_DEPS)
	$(LCUPSI_CC) $(LCUPSIO_)error.$(OBJ) $(C_) $(LIBCUPSISRC)error.c
	
$(LIBCUPSIOBJ)interpret.$(OBJ) : $(LIBCUPSISRC)interpret.c $(LIBCUPSI_DEPS)
	$(LCUPSI_CC) $(LCUPSIO_)interpret.$(OBJ) $(C_) $(LIBCUPSISRC)interpret.c
	
$(LIBCUPSIOBJ)raster.$(OBJ) : $(LIBCUPSISRC)raster.c $(LIBCUPSI_DEPS)
	$(LCUPSI_CC) $(LCUPSIO_)raster.$(OBJ) $(C_) $(LIBCUPSISRC)raster.c
	
