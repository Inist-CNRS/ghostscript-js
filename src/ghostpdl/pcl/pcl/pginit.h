/* Copyright (C) 2001-2012 Artifex Software, Inc.
   All Rights Reserved.

   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied,
   modified or distributed except as expressly authorized under the terms
   of the license contained in the file LICENSE in this distribution.

   Refer to licensing information at http://www.artifex.com or contact
   Artifex Software, Inc.,  7 Mt. Lassen Drive - Suite A-134, San Rafael,
   CA  94903, U.S.A., +1(415)492-9861, for further information.
*/


/* pginit.h - Interface to initialize/reset procedures in pginit.c */

#ifndef pginit_INCLUDED
#define pginit_INCLUDED

#include "gx.h"
#include "pcstate.h"
#include "pcommand.h"
#include "pgmand.h"

/* Reset a set of font parameters to their default values. */
void hpgl_default_font_params(pcl_font_selection_t * pfs);

/* Reset all the fill patterns to solid fill. */
void hpgl_default_all_fill_patterns(hpgl_state_t * pgls);

/* Reset (parts of) the HP-GL/2 state. */
void hpgl_do_reset(pcl_state_t * pcs, pcl_reset_type_t type);

#endif /* pginit_INCLUDED */
