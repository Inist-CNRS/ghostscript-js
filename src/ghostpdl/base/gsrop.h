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


/* RasterOp / transparency procedure interface */

#ifndef gsrop_INCLUDED
#  define gsrop_INCLUDED

#include "gsropt.h"

/* Procedural interface */

int gs_setrasterop(gs_gstate *, gs_rop3_t);
gs_rop3_t gs_currentrasterop(const gs_gstate *);
int gs_setsourcetransparent(gs_gstate *, bool);
bool gs_currentsourcetransparent(const gs_gstate *);
int gs_settexturetransparent(gs_gstate *, bool);
bool gs_currenttexturetransparent(const gs_gstate *);

/* Save/restore the combined logical operation. */
gs_logical_operation_t gs_current_logical_op(const gs_gstate *);
int gs_set_logical_op(gs_gstate *, gs_logical_operation_t);

#endif /* gsrop_INCLUDED */
