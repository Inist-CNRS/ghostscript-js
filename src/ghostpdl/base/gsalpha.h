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


/* API for alpha value in graphics state */

#ifndef gsalpha_INCLUDED
#  define gsalpha_INCLUDED

/*
 * This tiny little file is separate so that it can be included by
 * gsstate.c for initializing the alpha value, even in configurations
 * that don't have full alpha support.
 */

/* Set/read alpha value. */
int gs_setalpha(gs_gstate *, double);
float gs_currentalpha(const gs_gstate *);

#endif /* gsalpha_INCLUDED */
