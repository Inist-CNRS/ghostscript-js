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


/* Client interface to FormType 1 Forms */

typedef struct gs_form_template_s {
    gs_rect BBox;
    gs_matrix form_matrix;
    gs_matrix CTM;
    gx_clip_path * pcpath;
    int FormID;
} gs_form_template_t;

