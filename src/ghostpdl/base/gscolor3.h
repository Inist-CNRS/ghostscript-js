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


/* Client interface to LanguageLevel 3 color facilities */

#ifndef gscolor3_INCLUDED
#  define gscolor3_INCLUDED

/* Smooth shading */
#ifndef gs_shading_t_DEFINED
#  define gs_shading_t_DEFINED
typedef struct gs_shading_s gs_shading_t;
#endif

int gs_setsmoothness(gs_gstate *, double);
float gs_currentsmoothness(const gs_gstate *);
int gs_shfill(gs_gstate *, const gs_shading_t *);

#endif /* gscolor3_INCLUDED */
