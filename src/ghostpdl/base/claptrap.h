#ifndef CLAPTRAP_H
#define CLAPTRAP_H

/* Copyright (C) 2015-16 Artifex Software, Inc.
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

#include "std.h"
#include "stdpre.h"
#include "gsmemory.h"

#ifndef restrict
#define restrict __restrict
#endif /* restrict */
#ifndef inline
#define inline __inline
#endif /* inline */

typedef struct ClapTrap ClapTrap;

typedef int (ClapTrap_LineFn)(void *arg, unsigned char *buf);

ClapTrap *ClapTrap_Init(gs_memory_t     *mem,
                        int              width,
                        int              height,
                        int              num_comps,
                        const int       *comp_order,
                        int              max_x_offset,
                        int              max_y_offset,
                        ClapTrap_LineFn *get_line,
                        void            *get_line_arg);

void ClapTrap_Fin(gs_memory_t *mem,
                  ClapTrap *trapper);

int ClapTrap_GetLine(ClapTrap      * restrict trapper,
                     unsigned char * restrict buffer);

int ClapTrap_GetLinePlanar(ClapTrap       * restrict trapper,
                           unsigned char ** restrict buffer);

#endif /* CLAPTRAP_H */
