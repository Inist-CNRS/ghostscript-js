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


/* Client interface to HSB color routines */

#ifndef gshsb_INCLUDED
#  define gshsb_INCLUDED

int gs_sethsbcolor(gs_gstate *, double, double, double),
    gs_currenthsbcolor(const gs_gstate *, float[3]);

#endif /* gshsb_INCLUDED */
