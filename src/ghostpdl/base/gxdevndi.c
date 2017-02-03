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


#include "gx.h"
#include "gsstruct.h"
#include "gsdcolor.h"
#include "gxdevice.h"
#include "gxlum.h"
#include "gxcmap.h"
#include "gxdither.h"
#include "gzht.h"
#include "gxfrac.h"

/*
 * Binary halftoning algorithms.
 *
 * The procedures in this file use halftoning (if necessary)
 * to implement a given device color that has already gone through
 * the transfer function.  There are two major cases: gray and color.
 * Gray halftoning always uses a binary screen.  Color halftoning
 * uses either a fast algorithm with a binary screen that produces
 * relatively poor approximations, or a very slow algorithm with a
 * general colored screen (or screens) that faithfully implements
 * the Adobe specifications.
 */

/* Tables for fast computation of fractional color levels. */
/* We have to put the table before any uses of it because of a bug */
/* in the VAX C compiler. */
/* We have to split up the definition of the table itself because of a bug */
/*  in the IBM AIX 3.2 C compiler. */
static const gx_color_value q0[] = {
    0
};
static const gx_color_value q1[] = {
    0, frac_color_(1, 1)
};
static const gx_color_value q2[] = {
    0, frac_color_(1, 2), frac_color_(2, 2)
};
static const gx_color_value q3[] = {
    0, frac_color_(1, 3), frac_color_(2, 3), frac_color_(3, 3)
};
static const gx_color_value q4[] = {
    0, frac_color_(1, 4), frac_color_(2, 4), frac_color_(3, 4),
    frac_color_(4, 4)
};
static const gx_color_value q5[] = {
    0, frac_color_(1, 5), frac_color_(2, 5), frac_color_(3, 5),
    frac_color_(4, 5), frac_color_(5, 5)
};
static const gx_color_value q6[] = {
    0, frac_color_(1, 6), frac_color_(2, 6), frac_color_(3, 6),
    frac_color_(4, 6), frac_color_(5, 6), frac_color_(6, 6)
};
static const gx_color_value q7[] = {
    0, frac_color_(1, 7), frac_color_(2, 7), frac_color_(3, 7),
    frac_color_(4, 7), frac_color_(5, 7), frac_color_(6, 7), frac_color_(7, 7)
};

/* We export fc_color_quo for the fractional_color macro in gzht.h. */
const gx_color_value *const fc_color_quo[8] = {
    q0, q1, q2, q3, q4, q5, q6, q7
};

/*
 * Render DeviceN possibly by halftoning.
 *  pcolors = pointer to an array color values (as fracs)
 *  pdevc - pointer to device color structure
 *  dev = pointer to device data structure
 *  pht = pointer to halftone data structure
 *  ht_phase  = halftone phase
 *  gray_colorspace = true -> current color space is DeviceGray.
 *  This is part of a kludge to minimize differences in the
 *  regression testing.
 */
int
gx_render_device_DeviceN(frac * pcolor,
        gx_device_color * pdevc, gx_device * dev,
        gx_device_halftone * pdht, const gs_int_point * ht_phase)
{
    uint max_value[GS_CLIENT_COLOR_MAX_COMPONENTS];
    frac dither_check = 0;
    uint int_color[GS_CLIENT_COLOR_MAX_COMPONENTS];
    gx_color_value vcolor[GS_CLIENT_COLOR_MAX_COMPONENTS];
    int i;
    int num_colors = dev->color_info.num_components;
    uint l_color[GS_CLIENT_COLOR_MAX_COMPONENTS];

    for (i=0; i<num_colors; i++) {
        max_value[i] = (dev->color_info.gray_index == i) ?
             dev->color_info.dither_grays - 1 :
             dev->color_info.dither_colors - 1;
    }

    for (i = 0; i < num_colors; i++) {
        unsigned long hsize = pdht && i <= pdht->num_comp ?
                (unsigned) pdht->components[i].corder.num_levels
                : 1;
        unsigned long nshades = hsize * max_value[i] + 1;
        long shade = pcolor[i] * nshades / (frac_1_long + 1);
        int_color[i] = shade / hsize;
        l_color[i] = shade % hsize;
        if (max_value[i] < MIN_CONTONE_LEVELS)
            dither_check |= l_color[i];
    }

#ifdef DEBUG
    if (gs_debug_c('c')) {
        dmlprintf1(dev->memory, "[c]ncomp=%d ", num_colors);
        for (i = 0; i < num_colors; i++)
            dmlprintf1(dev->memory, "0x%x, ", pcolor[i]);
        dmlprintf(dev->memory, "-->   ");
        for (i = 0; i < num_colors; i++)
            dmlprintf2(dev->memory, "%x+0x%x, ", int_color[i], l_color[i]);
        dmlprintf(dev->memory, "\n");
    }
#endif

    /* Check for no dithering required */
    if (!dither_check) {
        for (i = 0; i < num_colors; i++)
            vcolor[i] = fractional_color(int_color[i], max_value[i]);
        color_set_pure(pdevc, dev_proc(dev, encode_color)(dev, vcolor));
        return 0;
    }

    /* Use the slow, general colored halftone algorithm. */

    for (i = 0; i < num_colors; i++)
        _color_set_c(pdevc, i, int_color[i], l_color[i]);
    gx_complete_halftone(pdevc, num_colors, pdht);

    color_set_phase_mod(pdevc, ht_phase->x, ht_phase->y,
                            pdht->lcm_width, pdht->lcm_height);

    /* Determine if we are using only one component */
    if (!(pdevc->colors.colored.plane_mask &
         (pdevc->colors.colored.plane_mask - 1))) {
        /* We can reduce this color to a binary halftone or pure color. */
        return gx_devn_reduce_colored_halftone(pdevc, dev);
    }

    return 1;
}

/* Reduce a colored halftone to a binary halftone or pure color. */
/* This routine is called when only one component is being halftoned. */
int
gx_devn_reduce_colored_halftone(gx_device_color *pdevc, gx_device *dev)
{
    int planes = pdevc->colors.colored.plane_mask;
    int num_colors = dev->color_info.num_components;
    uint max_value[GS_CLIENT_COLOR_MAX_COMPONENTS];
    uint b[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_value v[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_index c0, c1;
    int i;

    for (i = 0; i < num_colors; i++) {
        max_value[i] = (dev->color_info.gray_index == i) ?
             dev->color_info.dither_grays - 1 :
             dev->color_info.dither_colors - 1;
        b[i] = pdevc->colors.colored.c_base[i];
        v[i] = fractional_color(b[i], max_value[i]);
    }
    c0 = dev_proc(dev, encode_color)(dev, v);

    if (planes == 0) {
        /*
         * Use a pure color.  This case is unlikely, but it can occur if
         * (and only if) the difference of each component from the nearest
         * device color is less than one halftone level.
         */
        color_set_pure(pdevc, c0);
        return 0;
    } else {
        /* Use a binary color. */
        int i = 0;
        uint bi;
        const gx_device_halftone *pdht = pdevc->colors.colored.c_ht;
        /*
         * NB: the halftone orders are all set up for an additive color
         *     space.  To use these work with a subtractive color space, it is
         *     necessary to invert both the color level and the color
         *     pair. Note that if the original color was provided an
         *     additive space, this will reverse (in an approximate sense)
         *     the color conversion performed to express the color in
         *     subtractive space.
         */
        bool invert = dev->color_info.polarity == GX_CINFO_POLARITY_SUBTRACTIVE;
        uint level;

        /* Convert plane mask bit position to component number */
        /* Determine i = log2(planes);  This works for powers of two */
        while (planes > 7) {
            i += 3;
            planes >>= 3;
        }
        i += planes >> 1;  /* log2 for 1,2,4 */

        bi = b[i] + 1;
        v[i] = fractional_color(bi, max_value[i]);
        level = pdevc->colors.colored.c_level[i];
        c1 = dev_proc(dev, encode_color)(dev, v);
        if (invert) {
            level = pdht->components[i].corder.num_levels - level;
            color_set_binary_halftone_component(pdevc, pdht, i, c1, c0, level);
        } else
            color_set_binary_halftone_component(pdevc, pdht, i, c0, c1, level);

        return 1;
    }
}
