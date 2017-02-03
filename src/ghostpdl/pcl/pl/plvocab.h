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


/* plvocab.h */
/* Map between glyph vocabularies */

/* Map a MSL glyph code to a Unicode character code. */
/* Note that the symbol set is required, because some characters map */
/* differently in different symbol sets. */
uint pl_map_MSL_to_Unicode(uint msl, uint symbol_set);

/* Map a Unicode character code to a MSL glyph code similarly. */
uint pl_map_Unicode_to_MSL(uint unicode, uint symbol_set);
