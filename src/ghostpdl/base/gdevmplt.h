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


/* Common definitions for PCL monochrome palette device */

#ifndef gdevmplt_INCLUDED
#  define gdevmplt_INCLUDED

typedef struct gx_device_s gx_device_mplt;

/* Initialize device. */
void gx_device_pcl_mono_palette_init(gx_device_mplt * dev);

typedef struct {
    subclass_common;
    gx_cm_color_map_procs pcl_mono_procs;
    gx_cm_color_map_procs *device_cm_procs;
} pcl_mono_palette_subclass_data;

extern_st(st_device_mplt);

#endif /* gdevmplt_INCLUDED */
