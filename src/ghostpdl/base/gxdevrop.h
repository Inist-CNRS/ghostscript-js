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


/* Extension of gxdevice.h for RasterOp */

#ifndef gxdevrop_INCLUDED
#  define gxdevrop_INCLUDED

/* Define an unaligned implementation of [strip_]copy_rop. */
dev_proc_copy_rop(gx_copy_rop_unaligned);
dev_proc_strip_copy_rop(gx_strip_copy_rop_unaligned);

#endif /* gxdevrop_INCLUDED */
