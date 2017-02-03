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


/* Level 1 extended color operators for Ghostscript library */
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gsutil.h"		/* for gs_next_ids */
#include "gsccolor.h"
#include "gxcspace.h"
#include "gxdcconv.h"
#include "gxdevice.h"		/* for gx_color_index */
#include "gxcmap.h"
#include "gzstate.h"
#include "gscolor1.h"
#include "gscolor2.h"
#include "gxhttype.h"
#include "gzht.h"

/* Imports from gscolor.c */
void load_transfer_map(gs_gstate *, gx_transfer_map *, double);

/* Imported from gsht.c */
void gx_set_effective_transfer(gs_gstate *);

/* Force a parameter into the range [0.0..1.0]. */
#define FORCE_UNIT(p) (p < 0.0 ? 0.0 : p > 1.0 ? 1.0 : p)

/* setcmykcolor */
int
gs_setcmykcolor(gs_gstate * pgs, double c, double m, double y, double k)
{
    gs_color_space      *pcs;
    int                 code;

    pcs = gs_cspace_new_DeviceCMYK(pgs->memory);
    if (pcs == NULL)
        return_error(gs_error_VMerror);
    if ((code = gs_setcolorspace(pgs, pcs)) >= 0) {
       gs_client_color *pcc = gs_currentcolor_inline(pgs);

        cs_adjust_color_count(pgs, -1); /* not strictly necessary */
        pcc->paint.values[0] = FORCE_UNIT(c);
        pcc->paint.values[1] = FORCE_UNIT(m);
        pcc->paint.values[2] = FORCE_UNIT(y);
        pcc->paint.values[3] = FORCE_UNIT(k);
        pcc->pattern = 0;		/* for GC */
        gx_unset_dev_color(pgs);
    }
    rc_decrement_only_cs(pcs, "gs_setcmykcolor");
    return code;
}

/* setblackgeneration */
/* Remap=0 is used by the interpreter. */
int
gs_setblackgeneration(gs_gstate * pgs, gs_mapping_proc proc)
{
    return gs_setblackgeneration_remap(pgs, proc, true);
}
int
gs_setblackgeneration_remap(gs_gstate * pgs, gs_mapping_proc proc, bool remap)
{
    rc_unshare_struct(pgs->black_generation, gx_transfer_map,
                      &st_transfer_map, pgs->memory,
                      return_error(gs_error_VMerror),
                      "gs_setblackgeneration");
    pgs->black_generation->proc = proc;
    pgs->black_generation->id = gs_next_ids(pgs->memory, 1);
    if (remap) {
        load_transfer_map(pgs, pgs->black_generation, 0.0);
        gx_unset_dev_color(pgs);
    }
    return 0;
}

/* currentblackgeneration */
gs_mapping_proc
gs_currentblackgeneration(const gs_gstate * pgs)
{
    return pgs->black_generation->proc;
}

/* setundercolorremoval */
/* Remap=0 is used by the interpreter. */
int
gs_setundercolorremoval(gs_gstate * pgs, gs_mapping_proc proc)
{
    return gs_setundercolorremoval_remap(pgs, proc, true);
}
int
gs_setundercolorremoval_remap(gs_gstate * pgs, gs_mapping_proc proc, bool remap)
{
    rc_unshare_struct(pgs->undercolor_removal, gx_transfer_map,
                      &st_transfer_map, pgs->memory,
                      return_error(gs_error_VMerror),
                      "gs_setundercolorremoval");
    pgs->undercolor_removal->proc = proc;
    pgs->undercolor_removal->id = gs_next_ids(pgs->memory, 1);
    if (remap) {
        load_transfer_map(pgs, pgs->undercolor_removal, -1.0);
        gx_unset_dev_color(pgs);
    }
    return 0;
}

/* currentundercolorremoval */
gs_mapping_proc
gs_currentundercolorremoval(const gs_gstate * pgs)
{
    return pgs->undercolor_removal->proc;
}

/* setcolortransfer */
/* Remap=0 is used by the interpreter. */
int
gs_setcolortransfer_remap(gs_gstate * pgs, gs_mapping_proc red_proc,
                          gs_mapping_proc green_proc,
                          gs_mapping_proc blue_proc,
                          gs_mapping_proc gray_proc, bool remap)
{
    gx_transfer *ptran = &pgs->set_transfer;
    gx_transfer old;
    gs_id new_ids = gs_next_ids(pgs->memory, 4);
    gx_device * dev = pgs->device;

    old = *ptran;
    rc_unshare_struct(ptran->gray, gx_transfer_map, &st_transfer_map,
                      pgs->memory, goto fgray, "gs_setcolortransfer");
    rc_unshare_struct(ptran->red, gx_transfer_map, &st_transfer_map,
                      pgs->memory, goto fred, "gs_setcolortransfer");
    rc_unshare_struct(ptran->green, gx_transfer_map, &st_transfer_map,
                      pgs->memory, goto fgreen, "gs_setcolortransfer");
    rc_unshare_struct(ptran->blue, gx_transfer_map, &st_transfer_map,
                      pgs->memory, goto fblue, "gs_setcolortransfer");
    ptran->gray->proc = gray_proc;
    ptran->gray->id = new_ids;
    ptran->red->proc = red_proc;
    ptran->red->id = new_ids + 1;
    ptran->green->proc = green_proc;
    ptran->green->id = new_ids + 2;
    ptran->blue->proc = blue_proc;
    ptran->blue->id = new_ids + 3;
    ptran->red_component_num =
        gs_color_name_component_number(dev, "Red", 3, ht_type_colorscreen);
    ptran->green_component_num =
        gs_color_name_component_number(dev, "Green", 5, ht_type_colorscreen);
    ptran->blue_component_num =
        gs_color_name_component_number(dev, "Blue", 4, ht_type_colorscreen);
    ptran->gray_component_num =
        gs_color_name_component_number(dev, "Gray", 4, ht_type_colorscreen);
    if (remap) {
        load_transfer_map(pgs, ptran->red, 0.0);
        load_transfer_map(pgs, ptran->green, 0.0);
        load_transfer_map(pgs, ptran->blue, 0.0);
        load_transfer_map(pgs, ptran->gray, 0.0);
        gx_set_effective_transfer(pgs);
        gx_unset_dev_color(pgs);
    } else
        gx_set_effective_transfer(pgs);
    return 0;
  fblue:
    rc_assign(ptran->green, old.green, "setcolortransfer");
  fgreen:
    rc_assign(ptran->red, old.red, "setcolortransfer");
  fred:
    rc_assign(ptran->gray, old.gray, "setcolortransfer");
  fgray:
    return_error(gs_error_VMerror);
}
int
gs_setcolortransfer(gs_gstate * pgs, gs_mapping_proc red_proc,
                    gs_mapping_proc green_proc, gs_mapping_proc blue_proc,
                    gs_mapping_proc gray_proc)
{
    return gs_setcolortransfer_remap(pgs, red_proc, green_proc,
                                     blue_proc, gray_proc, true);
}

/* currentcolortransfer */
void
gs_currentcolortransfer(const gs_gstate * pgs, gs_mapping_proc procs[4])
{
    const gx_transfer *ptran = &pgs->set_transfer;

    procs[0] = ptran->red->proc;
    procs[1] = ptran->green->proc;
    procs[2] = ptran->blue->proc;
    procs[3] = ptran->gray->proc;
}
