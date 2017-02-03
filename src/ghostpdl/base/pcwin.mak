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
# makefile for PC window system (MS Windows and OS/2) -specific device
# drivers.

# Define the name of this makefile.
PCWIN_MAK=$(GLSRC)pcwin.mak $(TOP_MAKEFILES)

# We have to isolate these in their own file because the MS Windows code
# requires special compilation switches, different from all other files
# and platforms.

### -------------------- The MS-Windows 3.n DLL ------------------------- ###

gp_mswin_h=$(GLSRC)gp_mswin.h
gsdll_h=$(GLSRC)gsdll.h
gsdllwin_h=$(GLSRC)gsdllwin.h

gdevmswn_h=$(DEVSRC)gdevmswn.h $(GDEVH)\
 $(dos__h) $(memory__h) $(string__h) $(windows__h)\
 $(gp_mswin_h)

# This is deprecated and requires the interpreter / PSSRCDIR.
$(GLOBJ)gdevmswn.$(OBJ): $(DEVSRC)gdevmswn.c $(gdevmswn_h) $(gp_h) $(gpcheck_h)\
 $(gsdll_h) $(gsdllwin_h) $(gsparam_h) $(gdevpccm_h) $(PCWIN_MAK)
	$(GLCCWIN) -I$(PSSRCDIR) -I$(DEVSRCDIR) $(GLO_)gdevmswn.$(OBJ) $(C_) $(DEVSRC)gdevmswn.c

$(GLOBJ)gdevmsxf.$(OBJ): $(DEVSRC)gdevmsxf.c $(ctype__h) $(math__h) $(memory__h) $(string__h)\
 $(gdevmswn_h) $(gsstruct_h) $(gsutil_h) $(gxxfont_h) $(PCWIN_MAK)
	$(GLCCWIN) $(GLO_)gdevmsxf.$(OBJ) $(C_) $(DEVSRC)gdevmsxf.c

# An implementation using a DIB filled by an image device.
# This is deprecated and requires the interpreter / PSSRCDIR.
$(GLOBJ)gdevwdib.$(OBJ): $(DEVSRC)gdevwdib.c\
 $(gdevmswn_h) $(gsdll_h) $(gsdllwin_h) $(gxdevmem_h) $(PCWIN_MAK)
	$(GLCCWIN) -I$(PSSRCDIR) $(GLO_)gdevwdib.$(OBJ) $(C_) $(DEVSRC)gdevwdib.c

mswindll1_=$(GLOBJ)gdevmswn.$(OBJ) $(GLOBJ)gdevmsxf.$(OBJ) $(GLOBJ)gdevwdib.$(OBJ)
mswindll2_=$(GLOBJ)gdevemap.$(OBJ) $(GLOBJ)gdevpccm.$(OBJ)
mswindll_=$(mswindll1_) $(mswindll2_)
$(GLGEN)mswindll.dev: $(mswindll_) $(PCWIN_MAK)
	$(SETDEV) $(GLGEN)mswindll $(mswindll1_)
	$(ADDMOD) $(GLGEN)mswindll $(mswindll2_)

### -------------------- The MS-Windows DDB 3.n printer ----------------- ###

mswinprn_=$(GLOBJ)gdevwprn.$(OBJ) $(GLOBJ)gdevmsxf.$(OBJ)
$(DD)mswinprn.dev: $(mswinprn_) $(PCWIN_MAK)
	$(SETDEV) $(DD)mswinprn $(mswinprn_)

$(GLOBJ)gdevwprn.$(OBJ): $(GLSRC)gdevwprn.c $(gdevmswn_h) $(gp_h) $(PCWIN_MAK)
	$(GLCCWIN) $(GLO_)gdevwprn.$(OBJ) $(C_) $(GLSRC)gdevwprn.c

### -------------------- The MS-Windows DIB 3.n printer ----------------- ###

mswinpr2_=$(GLOBJ)gdevwpr2.$(OBJ)
$(DD)mswinpr2.dev: $(mswinpr2_) $(GLD)page.dev $(PCWIN_MAK)
	$(SETPDEV) $(DD)mswinpr2 $(mswinpr2_)

$(GLOBJ)gdevwpr2.$(OBJ): $(DEVSRC)gdevwpr2.c $(PDEVH) $(windows__h)\
 $(gdevpccm_h) $(gp_h) $(gp_mswin_h) $(gsicc_manage_h) $(PCWIN_MAK)
	$(GLCCWIN) $(GLO_)gdevwpr2.$(OBJ) $(C_) $(DEVSRC)gdevwpr2.c

### --------------------------- The OS/2 printer ------------------------ ###

os2prn_=$(GLOBJ)gdevos2p.$(OBJ)
$(DD)os2prn.dev: $(os2prn_) $(GLD)page.dev $(PCWIN_MAK)
	$(SETPDEV) $(DD)os2prn $(os2prn_)

$(GLOBJ)gdevos2p.$(OBJ): $(GLSRC)gdevos2p.c $(gp_h) $(gdevpccm_h) $(gdevprn_h) $(gscdefs_h) $(PCWIN_MAK)
	$(GLCC) $(GLO_)gdevos2p.$(OBJ) $(C_) $(GLSRC)gdevos2p.c
