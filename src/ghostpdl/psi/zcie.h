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


/* $Id: zcie.h  $ */
/* Definitions for CIEBased color spaces */

#ifndef zcie_INCLUDED
#  define zcie_INCLUDED

int cieaspace(i_ctx_t *i_ctx_p, ref *CIEdict, ulong dictkey);
int cieabcspace(i_ctx_t *i_ctx_p, ref *CIEDict, ulong dictkey);
int ciedefspace(i_ctx_t *i_ctx_p, ref *CIEDict, ulong dictkey);
int ciedefgspace(i_ctx_t *i_ctx_p, ref *CIEDict, ulong dictkey);

#endif /* zcie_INCLUDED */
