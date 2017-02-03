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


/* Image scaling filters */
#include "math_.h"
#include "memory_.h"
#include "stdio_.h"
#include "stdint_.h"
#include "gdebug.h"
#include "strimpl.h"
#include "siscale.h"

/*
 *    Image scaling code is based on public domain code from
 *      Graphics Gems III (pp. 414-424), Academic Press, 1992.
 */

/* ---------------- ImageScaleEncode/Decode ---------------- */

/* Auxiliary structures. */
typedef struct {
    double weight;               /* float or scaled fraction */
} CONTRIB;

typedef struct {
    int index;                  /* index of first element in list of */
    /* contributors */
    int n;                      /* number of contributors */
    /* (not multiplied by stride) */
    int first_pixel;            /* offset of first value in source data */
} CLIST;

/* ImageScaleEncode / ImageScaleDecode */
typedef struct stream_IScale_state_s {
    /* The client sets the params values before initialization. */
    stream_image_scale_state_common;  /* = state_common + params */
    /* The init procedure sets the following. */
    int sizeofPixelIn;          /* bytes per input value, 1 or 2 */
    int sizeofPixelOut;         /* bytes per output value, 1 or 2 */
    void /*PixelIn */ *src;
    void /*PixelOut */ *dst;
    byte *tmp;
    CLIST *contrib;
    CONTRIB *items;
    /* The following are updated dynamically. */
    int src_y;
    uint src_offset, src_size;
    int dst_y;
    int src_y_offset;
    uint dst_offset, dst_size;
    CLIST dst_next_list;        /* for next output value */
    int dst_last_index;         /* highest index used in list */
    /* Vertical filter details */
    int filter_width;
    int max_support;
    double (*filter)(double);
    double min_scale;
    CONTRIB *dst_items; /* ditto */
} stream_IScale_state;

gs_private_st_ptrs6(st_IScale_state, stream_IScale_state,
    "ImageScaleEncode/Decode state",
    iscale_state_enum_ptrs, iscale_state_reloc_ptrs,
    dst, src, tmp, contrib, items, dst_items);

/* ------ Digital filter definition ------ */

/* Mitchell filter definition */
#define Mitchell_support 2
#define Mitchell_min_scale ((Mitchell_support * 2) / (MAX_ISCALE_SUPPORT - 1.01))
#define B (1.0 / 3.0)
#define C (1.0 / 3.0)
static double
Mitchell_filter(double t)
{
    double t2 = t * t;

    if (t < 0)
        t = -t;

    if (t < 1)
        return
            ((12 - 9 * B - 6 * C) * (t * t2) +
             (-18 + 12 * B + 6 * C) * t2 +
             (6 - 2 * B)) / 6;
    else if (t < 2)
        return
            ((-1 * B - 6 * C) * (t * t2) +
             (6 * B + 30 * C) * t2 +
             (-12 * B - 48 * C) * t +
             (8 * B + 24 * C)) / 6;
    else
        return 0;
}

/* Interpolated filter definition */
#define Interp_support 1
#define Interp_min_scale 0
static double
Interp_filter(double t)
{
    if (t < 0)
        t = -t;

    if (t >= 1)
        return 0;
    return 1 + (2*t -3)*t*t;
}

/*
 * The environment provides the following definitions:
 *      double fproc(double t)
 *      double fWidthIn
 *      PixelTmp {min,max,unit}PixelTmp
 */
#define CLAMP(v, mn, mx)\
  (v < mn ? mn : v > mx ? mx : v)

/* ------ Auxiliary procedures ------ */

/* Calculate the support for a given scale. */
/* The value is always in the range 1..max_support (was MAX_ISCALE_SUPPORT). */
static int
Interp_contrib_pixels(double scale)
{
    if (scale == 0.0)
        return 1;
    return (int)(((float)Interp_support) / (scale >= 1.0 ? 1.0 : scale)
                 * 2 + 1.5);
}

static int
Mitchell_contrib_pixels(double scale)
{
    if (scale == 0.0)
        return 1;
    return (int)(((float)Mitchell_support) / (scale >= 1.0 ? 1.0 : max(scale, Mitchell_min_scale))
                 * 2 + 1.5);
}

/* Pre-calculate filter contributions for a row or a column. */
/* Return the highest input pixel index used. */
static int
calculate_contrib(
        /* Return weight list parameters in contrib[0 .. size-1]. */
                     CLIST * contrib,
        /* Store weights in items[0 .. contrib_pixels(scale)*size-1]. */
        /* (Less space than this may actually be needed.) */
                     CONTRIB * items,
        /* The output image is scaled by 'scale' relative to the input. */
                     double scale,
        /* Start generating weights for input pixel 'starting_output_index'. */
                     int starting_output_index,
        /* Offset of input subimage (data) from the input image start. */
                     int src_y_offset,
        /* Entire output image size. */
                     int dst_size,
        /* Entire input image size. */
                     int src_size,
        /* Generate 'size' weight lists. */
                     int size,
        /* Limit pixel indices to 'limit', for clamping at the edges */
        /* of the image. */
                     int limit,
        /* Wrap pixel indices modulo 'modulus'. */
                     int modulus,
        /* Successive pixel values are 'stride' distance apart -- */
        /* normally, the number of color components. */
                     int stride,
        /* The unit of output is 'rescale_factor' times the unit of input. */
                     double rescale_factor,
        /* The filters width */
                     int fWidthIn,
        /* The filter to use */
                     double (*fproc)(double),
        /* minimum scale factor to use */
                     double min_scale
)
{
    double WidthIn, fscale;
    bool squeeze;
    int npixels;
    int i, j;
    int last_index = -1;

    if_debug1('w', "[w]calculate_contrib scale=%lg\n", scale);
    if (scale < 1.0) {
        double clamped_scale = max(scale, min_scale);
        WidthIn = ((double)fWidthIn) / clamped_scale;
        fscale = 1.0 / clamped_scale;
        squeeze = true;
    } else {
        WidthIn = (double)fWidthIn;
        fscale = 1.0;
        squeeze = false;
    }
    npixels = (int)(WidthIn * 2 + 1);

    for (i = 0; i < size; ++i) {
        /* Here we need :
           double scale = (double)dst_size / src_size;
           float dst_offset_fraction = floor(dst_offset) - dst_offset;
           double center = (starting_output_index  + i + dst_offset_fraction + 0.5) / scale - 0.5;
           int left = (int)ceil(center - WidthIn);
           int right = (int)floor(center + WidthIn);
           We can't compute 'right' in floats because float arithmetics is not associative.
           In older versions tt caused a 1 pixel bias of image bands due to
           rounding direction appears to depend on src_y_offset. So compute in rationals.
           Since pixel center fall to half integers, we subtract 0.5
           in the image space and add 0.5 in the device space.
         */
        int dst_y_offset_fraction_num = (int)((int64_t)src_y_offset * dst_size % src_size) * 2 <= src_size
                        ? -(int)((int64_t)src_y_offset * dst_size % src_size)
                        : src_size - (int)((int64_t)src_y_offset * dst_size % src_size);
        int center_denom = dst_size * 2;
        int64_t center_num = /* center * center_denom * 2 = */
            (starting_output_index  + i) * (int64_t)src_size * 2 + src_size + dst_y_offset_fraction_num * 2 - dst_size;
        int left = (int)ceil((center_num - WidthIn * center_denom) / center_denom);
        int right = (int)floor((center_num + WidthIn * center_denom) / center_denom);
        double center = (double)center_num / center_denom;
#define clamp_pixel(j) (j < 0 ? 0 : j >= limit ? limit - 1 : j)
        int first_pixel = clamp_pixel(left);
        int last_pixel = clamp_pixel(right);
        CONTRIB *p;

        if_debug4('w', "[w]i=%d, i+offset=%lg scale=%lg center=%lg : ", starting_output_index + i,
                starting_output_index + i + (double)src_y_offset / src_size * dst_size, scale, center);
        if (last_pixel > last_index)
            last_index = last_pixel;
        contrib[i].first_pixel = (first_pixel % modulus) * stride;
        contrib[i].n = last_pixel - first_pixel + 1;
        contrib[i].index = i * npixels;
        p = items + contrib[i].index;
        for (j = 0; j < npixels; ++j)
            p[j].weight = 0;
        if (squeeze) {
            double sum = 0;
            for (j = left; j <= right; ++j)
                sum += fproc((center - j) / fscale) / fscale;
            for (j = left; j <= right; ++j) {
                double weight = fproc((center - j) / fscale) / fscale / sum;
                int n = clamp_pixel(j);
                int k = n - first_pixel;

                p[k].weight +=
                    (float) (weight * rescale_factor);
                if_debug2('w', " %d %f", k, (float)p[k].weight);
            }

        } else {
            double sum = 0;
            for (j = left; j <= right; ++j)
                sum += fproc(center - j);
            for (j = left; j <= right; ++j) {
                double weight = fproc(center - j) / sum;
                int n = clamp_pixel(j);
                int k = n - first_pixel;

                p[k].weight +=
                    (float) (weight * rescale_factor);
                if_debug2('w', " %d %f", k, (float)p[k].weight);
            }
        }
        if_debug0('w', "\n");
    }
    return last_index;
}

/* Apply filter to zoom horizontally from src to tmp. */
static void
zoom_x(byte * tmp, const void /*PixelIn */ *src, int sizeofPixelIn,
       int skip, int tmp_width, int Colors, const CLIST * contrib,
       const CONTRIB * items)
{
    int c, i;

    contrib += skip;
    tmp += Colors * skip;

    for (c = 0; c < Colors; ++c) {
        byte *tp = tmp + c;
        const CLIST *clp = contrib;

        if_debug1('W', "[W]zoom_x color %d:", c);
        if (sizeofPixelIn == 1) {
            const byte *raster = (const byte *)src + c;

            for ( i = 0; i < tmp_width; tp += Colors, ++clp, ++i ) {
                double weight = 0;
                int pixel, j = clp->n;
                const byte *pp = raster + clp->first_pixel;
                const CONTRIB *cp = items + clp->index;

                switch ( Colors ) {
                  case 1:
                      for ( ; j > 0; pp += 1, ++cp, --j )
                          weight += *pp * cp->weight;
                      break;
                  case 3:
                      for ( ; j > 0; pp += 3, ++cp, --j )
                          weight += *pp * cp->weight;
                      break;
                  default:
                      for ( ; j > 0; pp += Colors, ++cp, --j )
                          weight += *pp * cp->weight;
                }
                pixel = (int)(weight + 0.5);
                if_debug1('W', " %g", weight);
                *tp = (byte)CLAMP(pixel, 0, 255);
            }
        } else {                /* sizeofPixelIn == 2 */
            const bits16 *raster = (const bits16 *)src + c;
            for ( i = 0; i < tmp_width; tp += Colors, ++clp, ++i ) {
                double weight = 0;
                int pixel, j = clp->n;
                const bits16 *pp = raster + clp->first_pixel;
                const CONTRIB *cp = items + clp->index;

                switch ( Colors ) {
                  case 1:
                      for ( ; j > 0; pp += 1, ++cp, --j )
                          weight += *pp * cp->weight;
                      break;
                  case 3:
                      for ( ; j > 0; pp += 3, ++cp, --j )
                          weight += *pp * cp->weight;
                      break;
                  default:
                      for ( ; j > 0; pp += Colors, ++cp, --j )
                          weight += *pp * cp->weight;
                }
                pixel = (int)(weight + 0.5);
                if_debug1('W', " %g", weight);
                *tp = (byte)CLAMP(pixel, 0, 255);
            }
        }
        if_debug0('W', "\n");
    }
}

/*
 * Apply filter to zoom vertically from tmp to dst.
 * This is simpler because we can treat all columns identically
 * without regard to the number of samples per pixel.
 */
static void
zoom_y(void /*PixelOut */ *dst, int sizeofPixelOut, uint MaxValueOut,
       const byte * tmp, int skip, int WidthOut, int Stride,
       int Colors, const CLIST * contrib, const CONTRIB * items)
{
    int kn = Stride * Colors;
    int width = WidthOut * Colors;
    int cn = contrib->n;
    int first_pixel = contrib->first_pixel;
    const CONTRIB *cbp = items + contrib->index;
    int kc;
    int max_weight = MaxValueOut;

    if_debug0('W', "[W]zoom_y: ");

    skip *= Colors;
    width += skip;
    if (sizeofPixelOut == 1) {
        for ( kc = skip; kc < width; ++kc ) {
            double weight = 0;
            const byte *pp = &tmp[kc + first_pixel];
            int pixel, j = cn;
            const CONTRIB *cp = cbp;

            for ( ; j > 0; pp += kn, ++cp, --j )
                weight += *pp * cp->weight;
            pixel = (int)(weight + 0.5);
            if_debug1('W', " %x", pixel);
            ((byte *)dst)[kc] = (byte)CLAMP(pixel, 0, max_weight);
        }
    } else {                    /* sizeofPixelOut == 2 */
        for ( kc = skip; kc < width; ++kc ) {
            double weight = 0;
            const byte *pp = &tmp[kc + first_pixel];
            int pixel, j = cn;
            const CONTRIB *cp = cbp;

            for ( ; j > 0; pp += kn, ++cp, --j )
                weight += *pp * cp->weight;
            pixel = (int)(weight + 0.5);
            if_debug1('W', " %x", pixel);
            ((bits16 *)dst)[kc] = (bits16)CLAMP(pixel, 0, max_weight);
        }
    }
    if_debug0('W', "\n");
}

/* ------ Stream implementation ------ */

/* Forward references */
static void s_IScale_release(stream_state * st);

/* Calculate the weights for an output row. */
static void
calculate_dst_contrib(stream_IScale_state * ss, int y)
{
    uint row_size = ss->params.WidthOut * ss->params.spp_interp;
    int last_index =
    calculate_contrib(&ss->dst_next_list, ss->dst_items,
                      (double)ss->params.EntireHeightOut / ss->params.EntireHeightIn,
                      y, ss->src_y_offset, ss->params.EntireHeightOut, ss->params.EntireHeightIn,
                      1, ss->params.HeightIn, ss->max_support, row_size,
                      (double)ss->params.MaxValueOut / 255, ss->filter_width,
                      ss->filter, ss->min_scale);
    int first_index_mod = ss->dst_next_list.first_pixel / row_size;

    if_debug2m('w', ss->memory, "[W]calculate_dst_contrib for y = %d, y+offset=%d\n", y, y + ss->src_y_offset);
    ss->dst_last_index = last_index;
    last_index %= ss->max_support;
    if (last_index < first_index_mod) {         /* Shuffle the indices to account for wraparound. */
        CONTRIB *shuffle = &ss->dst_items[ss->max_support];
        int i;

        for (i = 0; i < ss->max_support; ++i) {
            shuffle[i].weight =
                (i <= last_index ?
                 ss->dst_items[i + ss->max_support - first_index_mod].weight :
                 i >= first_index_mod ?
                 ss->dst_items[i - first_index_mod].weight :
                 0);
            if_debug1m('W', ss->memory, " %f", shuffle[i].weight);
        }
        memcpy(ss->dst_items, shuffle, ss->max_support * sizeof(CONTRIB));
        ss->dst_next_list.n = ss->max_support;
        ss->dst_next_list.first_pixel = 0;
    }
    if_debug0m('W', ss->memory, "\n");
}

/* Set default parameter values (actually, just clear pointers). */
static void
s_IScale_set_defaults(stream_state * st)
{
    stream_IScale_state *const ss = (stream_IScale_state *) st;

    ss->src = 0;
    ss->dst = 0;
    ss->tmp = 0;
    ss->contrib = 0;
    ss->items = 0;
}

typedef struct filter_defn_s {
    double  (*filter)(double);
    int     filter_width;
    int     (*contrib_pixels)(double scale);
    double  min_scale;
} filter_defn_s;

/* Initialize the filter. */
static int
do_init(stream_state        *st,
        const filter_defn_s *horiz,
        const filter_defn_s *vert)
{
    stream_IScale_state *const ss = (stream_IScale_state *) st;
    gs_memory_t *mem = ss->memory;

    ss->sizeofPixelIn = ss->params.BitsPerComponentIn / 8;
    ss->sizeofPixelOut = ss->params.BitsPerComponentOut / 8;

    ss->src_y = 0;
    ss->src_size = 
        ss->params.WidthIn * ss->sizeofPixelIn * ss->params.spp_interp;
    ss->src_offset = 0;
    ss->dst_y = 0;
    ss->src_y_offset = ss->params.src_y_offset;
    ss->dst_size = 
        ss->params.WidthOut * ss->sizeofPixelOut * ss->params.spp_interp;
    ss->dst_offset = 0;

    /* create intermediate image to hold horizontal zoom */
    ss->max_support  = vert->contrib_pixels((double)ss->params.EntireHeightOut/
                                            ss->params.EntireHeightIn);
    ss->filter_width = vert->filter_width;
    ss->filter       = vert->filter;
    ss->min_scale    = vert->min_scale;
    ss->tmp = (byte *) gs_alloc_byte_array(mem,
                                           ss->max_support,
                                           (ss->params.WidthOut *
                                            ss->params.spp_interp),
                                           "image_scale tmp");
    ss->contrib = (CLIST *) gs_alloc_byte_array(mem,
                                                max(ss->params.WidthOut,
                                                    ss->params.HeightOut),
                                                sizeof(CLIST),
                                                "image_scale contrib");
    ss->items = (CONTRIB *)
                    gs_alloc_byte_array(mem,
                                        (horiz->contrib_pixels(
                                            (double)ss->params.EntireWidthOut /
                                            ss->params.EntireWidthIn) *
                                         ss->params.WidthOut),
                                         sizeof(CONTRIB),
                                         "image_scale contrib[*]");
    ss->dst_items = (CONTRIB *) gs_alloc_byte_array(mem,
                                                    ss->max_support*2,
                                                    sizeof(CONTRIB), "image_scale contrib_dst[*]");
    /* Allocate buffers for 1 row of source and destination. */
    ss->dst = 
        gs_alloc_byte_array(mem, ss->params.WidthOut * ss->params.spp_interp,
                            ss->sizeofPixelOut, "image_scale dst");
    ss->src = 
        gs_alloc_byte_array(mem, ss->params.WidthIn * ss->params.spp_interp,
                            ss->sizeofPixelIn, "image_scale src");
    if (ss->tmp == 0 || ss->contrib == 0 || ss->items == 0 ||
        ss->dst_items == 0 || ss->dst == 0 || ss->src == 0
        ) {
        s_IScale_release(st);
        return ERRC;
/****** WRONG ******/
    }
#ifdef PACIFY_VALGRIND
    /* When we are scaling a subrectangle of an image, we calculate
     * the subrectangle, so that it's slightly larger than it needs
     * to be. Some of these 'extra' pixels are calculated using
     * bogus values (i.e. ones we don't bother copying/scaling into
     * the line buffer). These cause valgrind to be upset. To avoid
     * this, we preset the buffer to known values. */
    memset((byte *)ss->tmp, 0,
           ss->max_support * ss->params.WidthOut * ss->params.spp_interp);
#endif
    /* Pre-calculate filter contributions for a row. */
    calculate_contrib(ss->contrib, ss->items,
                      (double)ss->params.EntireWidthOut / ss->params.EntireWidthIn,
                      0, 0, ss->params.WidthOut, ss->params.WidthIn,
                      ss->params.WidthOut, ss->params.WidthIn, ss->params.WidthIn,
                      ss->params.spp_interp, 255. / ss->params.MaxValueIn,
                      horiz->filter_width, horiz->filter, horiz->min_scale);

    /* Prepare the weights for the first output row. */
    calculate_dst_contrib(ss, 0);

    return 0;

}

static const filter_defn_s Mitchell_defn =
{
    Mitchell_filter,
    Mitchell_support,
    Mitchell_contrib_pixels,
    Mitchell_min_scale
};

static const filter_defn_s Interp_defn =
{
    Interp_filter,
    Interp_support,
    Interp_contrib_pixels,
    Interp_min_scale
};

static int
s_IScale_init(stream_state * st)
{
    stream_IScale_state *const ss = (stream_IScale_state *) st;
    const filter_defn_s *horiz = &Mitchell_defn;
    const filter_defn_s *vert  = &Mitchell_defn;

    /* By default we use the mitchell filter, but if we are scaling down
     * (either on the horizontal or the vertical axis) then use the simple
     * interpolation filter for that axis. */
    if (ss->params.EntireWidthOut < ss->params.EntireWidthIn)
        horiz = &Interp_defn;
    if (ss->params.EntireHeightOut < ss->params.EntireHeightIn)
        vert = &Interp_defn;

    return do_init(st, horiz, vert);
}

/* Process a buffer.  Note that this handles Encode and Decode identically. */
static int
s_IScale_process(stream_state * st, stream_cursor_read * pr,
                 stream_cursor_write * pw, bool last)
{
    stream_IScale_state *const ss = (stream_IScale_state *) st;

    /* Check whether we need to deliver any output. */

  top:
    ss->params.Active = (ss->src_y >= ss->params.TopMargin &&
                         ss->src_y <= ss->params.TopMargin + ss->params.PatchHeightIn);

    while (ss->src_y > ss->dst_last_index) {  /* We have enough horizontally scaled temporary rows */
        /* to generate a vertically scaled output row. */
        uint wleft = pw->limit - pw->ptr;

        if (ss->dst_y == ss->params.HeightOut)
            return EOFC;
        if (wleft == 0)
            return 1;
        if (ss->dst_offset == 0) {
            byte *row;

            if (wleft >= ss->dst_size) {        /* We can scale the row directly into the output. */
                row = pw->ptr + 1;
                pw->ptr += ss->dst_size;
            } else {            /* We'll have to buffer the row. */
                row = ss->dst;
            }
            /* Apply filter to zoom vertically from tmp to dst. */
            if (ss->params.Active)
                zoom_y(row, /* Where to scale to */
                       ss->sizeofPixelOut, /* 1 (8 bit) or 2 (16bit) output */
                       ss->params.MaxValueOut, /* output value scale */
                       ss->tmp, /* Line buffer */
                       ss->params.LeftMarginOut, /* Skip */
                       ss->params.PatchWidthOut, /* How many pixels to produce */
                       ss->params.WidthOut, /* Stride */
                       ss->params.spp_interp, /* Color count */
                       &ss->dst_next_list, ss->dst_items);
            /* Idiotic C coercion rules allow T* and void* to be */
            /* inter-assigned freely, but not compared! */
            if ((void *)row != ss->dst)         /* no buffering */
                goto adv;
        }
        {                       /* We're delivering a buffered output row. */
            uint wcount = ss->dst_size - ss->dst_offset;
            uint ncopy = min(wleft, wcount);

            if (ss->params.Active)
                memcpy(pw->ptr + 1, (byte *) ss->dst + ss->dst_offset, ncopy);
            pw->ptr += ncopy;
            ss->dst_offset += ncopy;
            if (ncopy != wcount)
                return 1;
            ss->dst_offset = 0;
        }
        /* Advance to the next output row. */
      adv:++ss->dst_y;
        if (ss->dst_y != ss->params.HeightOut)
            calculate_dst_contrib(ss, ss->dst_y);
    }

    /* Read input data and scale horizontally into tmp. */

    {
        uint rleft = pr->limit - pr->ptr;
        uint rcount = ss->src_size - ss->src_offset;

        if (rleft == 0)
            return 0;           /* need more input */
        if (ss->src_y >= ss->params.HeightIn)
            return ERRC;
        if (rleft >= rcount) {  /* We're going to fill up a row. */
            const byte *row;

            if (ss->src_offset == 0) {  /* We have a complete row.  Read the data */
                /* directly from the input. */
                row = pr->ptr + 1;
            } else {            /* We're buffering a row in src. */
                row = ss->src;
                if (ss->params.Active)
                    memcpy((byte *) ss->src + ss->src_offset, pr->ptr + 1,
                           rcount);
                ss->src_offset = 0;
            }
            /* Apply filter to zoom horizontally from src to tmp. */
            if_debug2('w', "[w]zoom_x y = %d to tmp row %d\n",
                      ss->src_y, (ss->src_y % ss->max_support));
            if (ss->params.Active)
                zoom_x(/* Where to scale to (dst line address in tmp buffer) */
                       ss->tmp + (ss->src_y % ss->max_support) *
                       ss->params.WidthOut * ss->params.spp_interp,
                       row, /* Where to scale from */
                       ss->sizeofPixelIn, /* 1 (8 bit) or 2 (16bit) */
                       ss->params.LeftMarginOut, /* Line skip */
                       ss->params.PatchWidthOut, /* How many pixels to produce */
                       ss->params.spp_interp, /* Color count */
                       ss->contrib, ss->items);
            pr->ptr += rcount;
            ++(ss->src_y);
            goto top;
        } else {                /* We don't have a complete row.  Copy data to src buffer. */
            if (ss->params.Active)
                memcpy((byte *) ss->src + ss->src_offset, pr->ptr + 1, rleft);
            ss->src_offset += rleft;
            pr->ptr += rleft;
            return 0;
        }
    }
}

/* Release the filter's storage. */
static void
s_IScale_release(stream_state * st)
{
    stream_IScale_state *const ss = (stream_IScale_state *) st;
    gs_memory_t *mem = ss->memory;

    gs_free_object(mem, (void *)ss->src, "image_scale src");    /* no longer const */
    ss->src = 0;
    gs_free_object(mem, ss->dst, "image_scale dst");
    ss->dst = 0;
    gs_free_object(mem, ss->items, "image_scale contrib[*]");
    ss->items = 0;
    gs_free_object(mem, ss->items, "image_scale contrib_dst[*]");
    ss->dst_items = 0;
    gs_free_object(mem, ss->contrib, "image_scale contrib");
    ss->contrib = 0;
    gs_free_object(mem, ss->tmp, "image_scale tmp");
    ss->tmp = 0;
}

/* Stream template */
const stream_template s_IScale_template = {
    &st_IScale_state, s_IScale_init, s_IScale_process, 1, 1,
    s_IScale_release, s_IScale_set_defaults
};
