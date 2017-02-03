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

/* x-bit-per-pixel "memory" (stored bitmap) device. */
/* This is used as a prototype for a memory device for the planar device
   to make use of in creating its structure for bit depths greater than
   64.  Note that the 256 is a place holder.   */
#include "memory_.h"
#include "gx.h"
#include "gxdevice.h"
#include "gxdevmem.h"		/* semi-public definitions */
#include "gdevmem.h"		/* private definitions */

/* The device descriptor. */
const gx_device_memory mem_x_device =
    mem_device("imagex", 256, 0, NULL, NULL, NULL, NULL, NULL, NULL);
