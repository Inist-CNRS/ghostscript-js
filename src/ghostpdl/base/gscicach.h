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


/* A color index conversion cache. */

#ifndef gscicach_INCLUDED
#  define gscicach_INCLUDED

#include "gxdevcli.h" /* For frac31. */

#ifndef gs_color_index_cache_DEFINED
#  define gs_color_index_cache_DEFINED
typedef struct gs_color_index_cache_s gs_color_index_cache_t;
#endif

gs_color_index_cache_t *gs_color_index_cache_create(gs_memory_t *memory,
                const gs_color_space *direct_space, gx_device *dev, gs_gstate *pgs, bool need_frac, gx_device *trans_dev);
void gs_color_index_cache_destroy(gs_color_index_cache_t *this);

int gs_cached_color_index(gs_color_index_cache_t *this, const float *paint_values, gx_device_color *pdevc, frac31 *frac_values);

#endif /* gscicach_INCLUDED */
