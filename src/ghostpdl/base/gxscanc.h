/* Copyright (C) 2001-2006 Artifex Software, Inc.
   All Rights Reserved.

   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied, modified
   or distributed except as expressly authorized under the terms of that
   license.  Refer to licensing information at http://www.artifex.com/
   or contact Artifex Software, Inc.,  7 Mt. Lassen Drive - Suite A-134,
   San Rafael, CA  94903, U.S.A., +1(415)492-9861, for further information.
*/

/* $Id: gxpath.h 8022 2007-06-05 22:23:38Z giles $ */
/* Fixed-point path procedures */
/* Requires gxfixed.h */

#ifndef gxscanc_INCLUDED
#  define gxscanc_INCLUDED

#include "gxpath.h"
#include "gxdevice.h"

/* The routines and types in this interface use */
/* device, rather than user, coordinates, and fixed-point, */
/* rather than floating, representation. */

/* Opaque type for an edgebuffer */
typedef struct gx_edgebuffer_s gx_edgebuffer;

struct gx_edgebuffer_s {
    int  base;
    int  height;
    int  xmin;
    int  xmax;
    int *index;
    int *table;
};

/* "Pixel centre" scanline routines */
int
gx_scan_convert(gx_device     * pdev,
                gx_path       * path,
          const gs_fixed_rect * rect,
                gx_edgebuffer * edgebuffer,
                fixed           flatness);

int
gx_filter_edgebuffer(gx_device       * pdev,
                     gx_edgebuffer   * edgebuffer,
                     int               rule);

int
gx_fill_edgebuffer(gx_device       * pdev,
             const gx_device_color * pdevc,
                   gx_edgebuffer   * edgebuffer,
                   int               log_op);

/* "Any Part of a Pixel" (app) scanline routines */
int
gx_scan_convert_app(gx_device     * pdev,
                    gx_path       * path,
              const gs_fixed_rect * rect,
                    gx_edgebuffer * edgebuffer,
                    fixed           flatness);

int
gx_filter_edgebuffer_app(gx_device       * pdev,
                         gx_edgebuffer   * edgebuffer,
                         int               rule);

int
gx_fill_edgebuffer_app(gx_device       * pdev,
                 const gx_device_color * pdevc,
                       gx_edgebuffer   * edgebuffer,
                       int               log_op);

/* "Pixel centre" trapezoid routines */
int
gx_scan_convert_tr(gx_device     * pdev,
                   gx_path       * path,
             const gs_fixed_rect * rect,
                   gx_edgebuffer * edgebuffer,
                   fixed           flatness);

int
gx_filter_edgebuffer_tr(gx_device       * pdev,
                        gx_edgebuffer   * edgebuffer,
                        int               rule);

int
gx_fill_edgebuffer_tr(gx_device       * pdev,
                const gx_device_color * pdevc,
                      gx_edgebuffer   * edgebuffer,
                      int               log_op);

/* "Any Part of a Pixel" (app) trapezoid routines */
int
gx_scan_convert_tr_app(gx_device     * pdev,
                       gx_path       * path,
                 const gs_fixed_rect * rect,
                       gx_edgebuffer * edgebuffer,
                       fixed           flatness);

int
gx_filter_edgebuffer_tr_app(gx_device       * pdev,
                            gx_edgebuffer   * edgebuffer,
                            int               rule);

int
gx_fill_edgebuffer_tr_app(gx_device       * pdev,
                    const gx_device_color * pdevc,
                          gx_edgebuffer   * edgebuffer,
                          int               log_op);

/* Equivalent to filling it full of 0's */
void gx_edgebuffer_init(gx_edgebuffer * edgebuffer);

void gx_edgebuffer_fin(gx_device     * pdev,
                       gx_edgebuffer * edgebuffer);


#endif /* gxscanc_INCLUDED */
