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


/* Painting procedures for Ghostscript library */
#include "math_.h"		/* for fabs */
#include "gx.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gsropt.h"		/* for gxpaint.h */
#include "gxfixed.h"
#include "gxmatrix.h"		/* for gs_gstate */
#include "gspaint.h"
#include "gspath.h"
#include "gzpath.h"
#include "gxpaint.h"
#include "gzstate.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gzcpath.h"
#include "gxhldevc.h"
#include "gsutil.h"
#include "gxscanc.h"
#include "gxdevsop.h"
#include "gsicc_cms.h"

/* Define the nominal size for alpha buffers. */
#define abuf_nominal_SMALL 500
#define abuf_nominal_LARGE 2000
#if ARCH_SMALL_MEMORY
#  define abuf_nominal abuf_nominal_SMALL
#else
#ifdef DEBUG
#  define abuf_nominal\
     (gs_debug_c('.') ? abuf_nominal_SMALL : abuf_nominal_LARGE)
#else
#  define abuf_nominal abuf_nominal_LARGE
#endif
#endif

/* Erase the page */
int
gs_erasepage(gs_gstate * pgs)
{
    /*
     * We can't just fill with device white; we must take the
     * transfer function into account.
     */
    int code;

    if ((code = gs_gsave(pgs)) < 0)
        return code;
    if ((code = gs_setgray(pgs, 1.0)) >= 0) {
        /* Fill the page directly, ignoring clipping. */
        code = gs_fillpage(pgs);
    }
    gs_grestore(pgs);
    return code;
}

/* Fill the page with the current color. */
int
gs_fillpage(gs_gstate * pgs)
{
    gx_device *dev = gs_currentdevice(pgs);
    int code;
    gx_cm_color_map_procs *   pprocs;

    pprocs = get_color_mapping_procs_subclass(dev);
    /* If we get here without a valid get_color_mapping_procs, fail */
    if (pprocs == NULL ||
        /* Deliberately use the terminal device here */
        dev_proc(dev, get_color_mapping_procs) ==  gx_error_get_color_mapping_procs) {
        emprintf1(dev->memory,
                  "\n   *** Error: No get_color_mapping_procs for device: %s\n",
                  dev->dname);
        return_error(gs_error_Fatal);
    }
    /* Processing a fill object operation */
    dev_proc(pgs->device, set_graphics_type_tag)(pgs->device, GS_PATH_TAG);

    code = gx_set_dev_color(pgs);
    if (code != 0)
        return code;

    code = (*dev_proc(dev, fillpage))(dev, pgs, gs_currentdevicecolor_inline(pgs));
    if (code < 0)
        return code;

    /* If GrayDetection is set, make sure monitoring is enabled. */
    if (dev->icc_struct != NULL &&
            dev->icc_struct->graydetection && !dev->icc_struct->pageneutralcolor) {
        dev->icc_struct->pageneutralcolor = true;	/* start detecting again */
        code = gsicc_mcm_begin_monitor(pgs->icc_link_cache, dev);
    }
    if (code < 0)
        return code;
    return (*dev_proc(dev, sync_output)) (dev);
}
/*
 * Set up an alpha buffer for a stroke or fill operation.  Return 0
 * if no buffer could be allocated, 1 if a buffer was installed,
 * or the usual negative error code.
 *
 * The fill/stroke code sets up a clipping device if needed; however,
 * since we scale up all the path coordinates, we either need to scale up
 * the clipping region, or do clipping after, rather than before,
 * alpha buffering.  Either of these is a little inconvenient, but
 * the former is less inconvenient.
 */
static int
scale_paths(gs_gstate * pgs, int log2_scale_x, int log2_scale_y, bool do_path)
{
    /*
     * Because of clip and clippath, any of path, clip_path, and view_clip
     * may be aliases for each other.  The only reliable way to detect
     * this is by comparing the segments pointers.  Note that we must
     * scale the non-segment parts of the paths even if the segments are
     * aliased.
     */
    const gx_path_segments *seg_clip =
        (pgs->clip_path->path_valid ? pgs->clip_path->path.segments : 0);
    const gx_clip_rect_list *list_clip = pgs->clip_path->rect_list;
    const gx_path_segments *seg_view_clip;
    const gx_clip_rect_list *list_view_clip;
    const gx_path_segments *seg_effective_clip =
        (pgs->effective_clip_path->path_valid ?
         pgs->effective_clip_path->path.segments : 0);
    const gx_clip_rect_list *list_effective_clip =
        pgs->effective_clip_path->rect_list;

    gx_cpath_scale_exp2_shared(pgs->clip_path, log2_scale_x, log2_scale_y,
                               false, false);
    if (pgs->view_clip != 0 && pgs->view_clip != pgs->clip_path) {
        seg_view_clip =
            (pgs->view_clip->path_valid ? pgs->view_clip->path.segments : 0);
        list_view_clip = pgs->view_clip->rect_list;
        gx_cpath_scale_exp2_shared(pgs->view_clip, log2_scale_x, log2_scale_y,
                                   list_view_clip == list_clip,
                                   seg_view_clip && seg_view_clip == seg_clip);
    } else
        seg_view_clip = 0, list_view_clip = 0;
    if (pgs->effective_clip_path != pgs->clip_path &&
        pgs->effective_clip_path != pgs->view_clip
        )
        gx_cpath_scale_exp2_shared(pgs->effective_clip_path, log2_scale_x,
                                   log2_scale_y,
                                   list_effective_clip == list_clip ||
                                   list_effective_clip == list_view_clip,
                                   seg_effective_clip &&
                                   (seg_effective_clip == seg_clip ||
                                    seg_effective_clip == seg_view_clip));
    if (do_path) {
        const gx_path_segments *seg_path = pgs->path->segments;

        gx_path_scale_exp2_shared(pgs->path, log2_scale_x, log2_scale_y,
                                  seg_path == seg_clip ||
                                  seg_path == seg_view_clip ||
                                  seg_path == seg_effective_clip);
    }
    return 0;
}
static void
scale_dash_pattern(gs_gstate * pgs, double scale)
{
    int i;

    for (i = 0; i < pgs->line_params.dash.pattern_size; ++i)
        pgs->line_params.dash.pattern[i] *= scale;
    pgs->line_params.dash.offset *= scale;
    pgs->line_params.dash.pattern_length *= scale;
    pgs->line_params.dash.init_dist_left *= scale;
    if (pgs->line_params.dot_length_absolute)
        pgs->line_params.dot_length *= scale;
}
static int
alpha_buffer_init(gs_gstate * pgs, fixed extra_x, fixed extra_y, int alpha_bits, 
                  bool devn)
{
    gx_device *dev = gs_currentdevice_inline(pgs);
    int log2_alpha_bits = ilog2(alpha_bits);
    gs_fixed_rect bbox;
    gs_int_rect ibox;
    uint width, raster, band_space;
    uint height;
    gs_log2_scale_point log2_scale;
    gs_memory_t *mem;
    gx_device_memory *mdev;

    log2_scale.x = log2_scale.y = log2_alpha_bits;
    gx_path_bbox(pgs->path, &bbox);
    ibox.p.x = fixed2int(bbox.p.x - extra_x) - 1;
    ibox.p.y = fixed2int(bbox.p.y - extra_y) - 1;
    ibox.q.x = fixed2int_ceiling(bbox.q.x + extra_x) + 1;
    ibox.q.y = fixed2int_ceiling(bbox.q.y + extra_y) + 1;
    width = (ibox.q.x - ibox.p.x) << log2_scale.x;
    raster = bitmap_raster(width);
    band_space = raster << log2_scale.y;
    height = (abuf_nominal / band_space) << log2_scale.y;
    if (height == 0)
        height = 1 << log2_scale.y;
    mem = pgs->memory;
    mdev = gs_alloc_struct(mem, gx_device_memory, &st_device_memory,
                           "alpha_buffer_init");
    if (mdev == 0)
        return 0;		/* if no room, don't buffer */
    /* We may have to update the marking parameters if we have a pdf14 device
       as our target.  Need to do while dev is still active in pgs */
    if (dev_proc(dev, dev_spec_op)(dev, gxdso_is_pdf14_device, NULL, 0) > 0) {
        gs_update_trans_marking_params(pgs);
    }
    gs_make_mem_abuf_device(mdev, mem, dev, &log2_scale,
                            alpha_bits, ibox.p.x << log2_scale.x, devn);
    mdev->width = width;
    mdev->height = height;
    mdev->bitmap_memory = mem;
    if ((*dev_proc(mdev, open_device)) ((gx_device *) mdev) < 0) {
        /* No room for bits, punt. */
        gs_free_object(mem, mdev, "alpha_buffer_init");
        return 0;
    }
    gx_set_device_only(pgs, (gx_device *) mdev);
    scale_paths(pgs, log2_scale.x, log2_scale.y, true);
    return 1;
}

/* Release an alpha buffer. */
static int
alpha_buffer_release(gs_gstate * pgs, bool newpath)
{
    gx_device_memory *mdev =
        (gx_device_memory *) gs_currentdevice_inline(pgs);
    int code = (*dev_proc(mdev, close_device)) ((gx_device *) mdev);

    if (code >= 0)
        scale_paths(pgs, -mdev->log2_scale.x, -mdev->log2_scale.y,
                !(newpath && !gx_path_is_shared(pgs->path)));
    /* Reference counting will free mdev. */
    gx_set_device_only(pgs, mdev->target);
    return code;
}

static int do_fill(gs_gstate *pgs, int rule)
{
    int code, abits, acode, rcode = 0;
    bool devn;

    /* We need to distinguish text from vectors to set the object tag.

       To make that determination, we check for the show graphics state being stored
       in the current graphics state. This works even in the case of a glyph from a
       Type 3 Postscript/PDF font which has multiple, nested gsave/grestore pairs in
       the BuildGlyph/BuildChar procedure. Also, it works in the case of operating
       without a glyph cache or bypassing the cache because the glyph is too large or
       the cache being already full.

       Note that it doesn't work for a construction like:
       "(xyz) true charpath fill/stroke"
       where the show machinations have completed before we get to the fill operation.
       This has implications for how we handle PDF text rendering modes 1 and 2. To
       handle that, we'll have to add a flag to the path structure, or to the path
       segment structure (depending on how fine grained we require it to be).
     */
    if (pgs->show_gstate == NULL) {
        dev_proc(pgs->device, set_graphics_type_tag)(pgs->device, GS_PATH_TAG);
    }
    else {
        dev_proc(pgs->device, set_graphics_type_tag)(pgs->device, GS_TEXT_TAG);
    }
    code = gx_set_dev_color(pgs);
    if (code != 0)
        return code;
    code = gs_gstate_color_load(pgs);
    if (code < 0)
        return code;
    abits = 0;
    {
        gx_device_color *col = gs_currentdevicecolor_inline(pgs);
        devn = color_is_devn(col);
        if (color_is_pure(col) || devn)
            abits = alpha_buffer_bits(pgs);
    }
    if (abits > 1) {
        acode = alpha_buffer_init(pgs, pgs->fill_adjust.x,
                                  pgs->fill_adjust.y, abits, devn);
        if (acode < 0)
            return acode;
    } else
        acode = 0;
    code = gx_fill_path(pgs->path, gs_currentdevicecolor_inline(pgs), pgs, rule,
                        pgs->fill_adjust.x, pgs->fill_adjust.y);
    if (acode > 0)
        rcode = alpha_buffer_release(pgs, code >= 0);
    if (code >= 0 && rcode < 0)
        code = rcode;

    return code;
}

/* Fill the current path using a specified rule. */
static int
fill_with_rule(gs_gstate * pgs, int rule)
{
    int code;

    /* If we're inside a charpath, just merge the current path */
    /* into the parent's path. */
    if (pgs->in_charpath)
        code = gx_path_add_char_path(pgs->show_gstate->path, pgs->path,
                                     pgs->in_charpath);
            /* If we're rendering a glyph cached, the show machinery decides
             * whether to actually image it on the output or not, but uncached
             * will render directly to the output, so for text rendering
             * mode 3, we have to short circuit it here, but keep the
             * current point
             */
    else if (gs_is_null_device(pgs->device)
            || (pgs->show_gstate && pgs->text_rendering_mode == 3
            && pgs->in_cachedevice == CACHE_DEVICE_NOT_CACHING)) {
        /* Handle separately to prevent gs_gstate_color_load - bug 688308. */
        gs_newpath(pgs);
        code = 0;
    } else {
        code = do_fill(pgs, rule);
        if (code >= 0)
            gs_newpath(pgs);
    }
    return code;
}
/* Fill using the winding number rule */
int
gs_fill(gs_gstate * pgs)
{
    pgs->device->sgr.stroke_stored = false;
    return fill_with_rule(pgs, gx_rule_winding_number);
}
/* Fill using the even/odd rule */
int
gs_eofill(gs_gstate * pgs)
{
    pgs->device->sgr.stroke_stored = false;
    return fill_with_rule(pgs, gx_rule_even_odd);
}

static int
do_stroke(gs_gstate * pgs)
{
    int code, abits, acode, rcode = 0;
    bool devn;

    /* We need to distinguish text from vectors to set the object tag.

       To make that determination, we check for the show graphics state being stored
       in the current graphics state. This works even in the case of a glyph from a
       Type 3 Postscript/PDF font which has multiple, nested gsave/grestore pairs in
       the BuildGlyph/BuildChar procedure. Also, it works in the case of operating
       without a glyph cache or bypassing the cache because the glyph is too large or
       the cache being already full.

       Note that it doesn't work for a construction like:
       "(xyz) true charpath fill/stroke"
       where the show machinations have completed before we get to the fill operation.
       This has implications for how we handle PDF text rendering modes 1 and 2. To
       handle that, we'll have to add a flag to the path structure, or to the path
       segment structure (depending on how fine grained we require it to be).
     */
    if (pgs->show_gstate == NULL) {
        dev_proc(pgs->device, set_graphics_type_tag)(pgs->device, GS_PATH_TAG);
    }
    else {
        dev_proc(pgs->device, set_graphics_type_tag)(pgs->device, GS_TEXT_TAG);
    }
    code = gx_set_dev_color(pgs);
    if (code != 0)
        return code;
    code = gs_gstate_color_load(pgs);
    if (code < 0)
        return code;
    abits = 0;
    {
        gx_device_color *col = gs_currentdevicecolor_inline(pgs);
        devn = color_is_devn(col);
        if (color_is_pure(col) || devn)
            abits = alpha_buffer_bits(pgs);
    }
    if (abits > 1) {
        /*
         * Expand the bounding box by the line width.
         * This is expensive to compute, so we only do it
         * if we know we're going to buffer.
         */
        float xxyy = fabs(pgs->ctm.xx) + fabs(pgs->ctm.yy);
        float xyyx = fabs(pgs->ctm.xy) + fabs(pgs->ctm.yx);
        float scale = (float)(1 << (abits / 2));
        float orig_width = gs_currentlinewidth(pgs);
        float new_width = orig_width * scale;
        fixed extra_adjust =
                float2fixed(max(xxyy, xyyx) * new_width / 2);
        float orig_flatness = gs_currentflat(pgs);
        gx_path spath;

        /* Scale up the line width, dash pattern, and flatness. */
        if (extra_adjust < fixed_1)
            extra_adjust = fixed_1;
        acode = alpha_buffer_init(pgs,
                                  pgs->fill_adjust.x + extra_adjust,
                                  pgs->fill_adjust.y + extra_adjust,
                                  abits, devn);
        if (acode < 0)
            return acode;
        gs_setlinewidth(pgs, new_width);
        scale_dash_pattern(pgs, scale);
        gs_setflat(pgs, orig_flatness * scale);
        /*
         * The alpha-buffer device requires that we fill the
         * entire path as a single unit.
         */
        gx_path_init_local(&spath, pgs->memory);
        code = gx_stroke_add(pgs->path, &spath, pgs, false);
        gs_setlinewidth(pgs, orig_width);
        scale_dash_pattern(pgs, 1.0 / scale);
        if (code >= 0)
            code = gx_fill_path(&spath, gs_currentdevicecolor_inline(pgs), pgs,
                                gx_rule_winding_number,
                                pgs->fill_adjust.x,
                                pgs->fill_adjust.y);
        gs_setflat(pgs, orig_flatness);
        gx_path_free(&spath, "gs_stroke");
        if (acode > 0)
            rcode = alpha_buffer_release(pgs, code >= 0);
    } else
        code = gx_stroke_fill(pgs->path, pgs);
    if (code >= 0 && rcode < 0)
        code = rcode;
    return code;
}

/* Stroke the current path */
int
gs_stroke(gs_gstate * pgs)
{
    int code;

    /*
     * If we're inside a charpath, just merge the current path
     * into the parent's path.
     */
    if (pgs->in_charpath) {
        if (pgs->in_charpath == cpm_true_charpath) {
            /*
             * A stroke inside a true charpath should do the
             * equivalent of strokepath.
             */
            code = gs_strokepath(pgs);
            if (code < 0)
                return code;
        }
        code = gx_path_add_char_path(pgs->show_gstate->path, pgs->path,
                                     pgs->in_charpath);
        if (code < 0)
            return code;
    }
    if (gs_is_null_device(pgs->device)) {
        /* Handle separately to prevent gs_gstate_color_load. */
        gs_newpath(pgs);
        code = 0;
    } else {
        code = do_stroke(pgs);
        if (code >= 0)
            gs_newpath(pgs);
    }
    return code;
}

/* Compute the stroked outline of the current path */
static int
gs_strokepath_aux(gs_gstate * pgs, bool traditional)
{
    gx_path spath;
    int code;

    gx_path_init_local(&spath, pgs->path->memory);
    code = gx_stroke_add(pgs->path, &spath, pgs, traditional);
    if (code < 0) {
        gx_path_free(&spath, "gs_strokepath");
        return code;
    }
    pgs->device->sgr.stroke_stored = false;
    code = gx_path_assign_free(pgs->path, &spath);
    if (code < 0)
        return code;
    /* NB: needs testing with PCL */
    if (gx_path_is_void(pgs->path))
        pgs->current_point_valid = false;
    else {
        gx_setcurrentpoint(pgs, fixed2float(spath.position.x), fixed2float(spath.position.y));
    }
    return 0;

}

int
gs_strokepath(gs_gstate * pgs)
{
    return gs_strokepath_aux(pgs, true);
}

int
gs_strokepath2(gs_gstate * pgs)
{
    return gs_strokepath_aux(pgs, false);
}
