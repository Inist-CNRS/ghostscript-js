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


/* A Rinkj module encapsulating ETS screening. */

RinkjDevice *
rinkj_screen_eb_new (RinkjDevice *dev_out);

void
rinkj_screen_eb_set_scale (RinkjDevice *self, double xscale, double yscale);

void
rinkj_screen_eb_set_gamma (RinkjDevice *self, int plane, double gamma, double max);

void
rinkj_screen_eb_set_lut (RinkjDevice *self, int plane, const double *lut);
