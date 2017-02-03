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

/* Default device implementation */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gsstruct.h"
#include "gxobj.h"
#include "gserrors.h"
#include "gsropt.h"
#include "gxcomp.h"
#include "gxdevice.h"
#include "gxdevsop.h"
#include "gdevp14.h"        /* Needed to patch up the procs after compositor creation */
#include "gstrans.h"        /* For gs_pdf14trans_t */
#include "gxgstate.h"       /* for gs_image_state_s */


/* defined in gsdpram.c */
int gx_default_get_param(gx_device *dev, char *Param, void *list);

/* ---------------- Default device procedures ---------------- */

/*
 * Set a color model polarity to be additive or subtractive. In either
 * case, indicate an error (and don't modify the polarity) if the current
 * setting differs from the desired and is not GX_CINFO_POLARITY_UNKNOWN.
 */
static void
set_cinfo_polarity(gx_device * dev, gx_color_polarity_t new_polarity)
{
#ifdef DEBUG
    /* sanity check */
    if (new_polarity == GX_CINFO_POLARITY_UNKNOWN) {
        dmprintf(dev->memory, "set_cinfo_polarity: illegal operand\n");
        return;
    }
#endif
    /*
     * The meory devices assume that single color devices are gray.
     * This may not be true if SeparationOrder is specified.  Thus only
     * change the value if the current value is unknown.
     */
    if (dev->color_info.polarity == GX_CINFO_POLARITY_UNKNOWN)
        dev->color_info.polarity = new_polarity;
}

static gx_color_index
(*get_encode_color(gx_device *dev))(gx_device *, const gx_color_value *)
{
    dev_proc_encode_color(*encode_proc);

    /* use encode_color if it has been provided */
    if ((encode_proc = dev_proc(dev, encode_color)) == 0) {
        if (dev->color_info.num_components == 1                          &&
            dev_proc(dev, map_rgb_color) != 0) {
            set_cinfo_polarity(dev, GX_CINFO_POLARITY_ADDITIVE);
            encode_proc = gx_backwards_compatible_gray_encode;
        } else  if ( (dev->color_info.num_components == 3    )           &&
             (encode_proc = dev_proc(dev, map_rgb_color)) != 0  )
            set_cinfo_polarity(dev, GX_CINFO_POLARITY_ADDITIVE);
        else if ( dev->color_info.num_components == 4                    &&
                 (encode_proc = dev_proc(dev, map_cmyk_color)) != 0   )
            set_cinfo_polarity(dev, GX_CINFO_POLARITY_SUBTRACTIVE);
    }

    /*
     * If no encode_color procedure at this point, the color model had
     * better be monochrome (though not necessarily bi-level). In this
     * case, it is assumed to be additive, as that is consistent with
     * the pre-DeviceN code.
     *
     * If this is not the case, then the color model had better be known
     * to be separable and linear, for there is no other way to derive
     * an encoding. This is the case even for weakly linear and separable
     * color models with a known polarity.
     */
    if (encode_proc == 0) {
        if (dev->color_info.num_components == 1 && dev->color_info.depth != 0) {
            set_cinfo_polarity(dev, GX_CINFO_POLARITY_ADDITIVE);
            if (dev->color_info.max_gray == (1 << dev->color_info.depth) - 1)
                encode_proc = gx_default_gray_fast_encode;
            else
                encode_proc = gx_default_gray_encode;
            dev->color_info.separable_and_linear = GX_CINFO_SEP_LIN;
        } else if (dev->color_info.separable_and_linear == GX_CINFO_SEP_LIN) {
            gx_color_value  max_gray = dev->color_info.max_gray;
            gx_color_value  max_color = dev->color_info.max_color;

            if ( (max_gray & (max_gray + 1)) == 0  &&
                 (max_color & (max_color + 1)) == 0  )
                /* NB should be gx_default_fast_encode_color */
                encode_proc = gx_default_encode_color;
            else
                encode_proc = gx_default_encode_color;
        }
    }

    return encode_proc;
}

/*
 * Determine if a color model has the properties of a DeviceRGB
 * color model. This procedure is, in all likelihood, high-grade
 * overkill, but since this is not a performance sensitive area
 * no harm is done.
 *
 * Since there is little benefit to checking the values 0, 1, or
 * 1/2, we use the values 1/4, 1/3, and 3/4 in their place. We
 * compare the results to see if the intensities match to within
 * a tolerance of .01, which is arbitrarily selected.
 */

static bool
is_like_DeviceRGB(gx_device * dev)
{
    const gx_cm_color_map_procs *   cm_procs;
    frac                            cm_comp_fracs[3];
    int                             i;

    if ( dev->color_info.num_components != 3                   ||
         dev->color_info.polarity != GX_CINFO_POLARITY_ADDITIVE  )
        return false;

    cm_procs = get_color_mapping_procs_subclass(dev);
    if (cm_procs == 0 || cm_procs->map_rgb == 0)
        return false;

    /* check the values 1/4, 1/3, and 3/4 */
    map_rgb_subclass(cm_procs, dev, 0, frac_1 / 4, frac_1 / 3, 3 * frac_1 / 4,cm_comp_fracs);

    /* verify results to .01 */
    cm_comp_fracs[0] -= frac_1 / 4;
    cm_comp_fracs[1] -= frac_1 / 3;
    cm_comp_fracs[2] -= 3 * frac_1 / 4;
    for ( i = 0;
           i < 3                            &&
           -frac_1 / 100 < cm_comp_fracs[i] &&
           cm_comp_fracs[i] < frac_1 / 100;
          i++ )
        ;
    return i == 3;
}

/*
 * Similar to is_like_DeviceRGB, but for DeviceCMYK.
 */
static bool
is_like_DeviceCMYK(gx_device * dev)
{
    const gx_cm_color_map_procs *   cm_procs;
    frac                            cm_comp_fracs[4];
    int                             i;

    if ( dev->color_info.num_components != 4                      ||
         dev->color_info.polarity != GX_CINFO_POLARITY_SUBTRACTIVE  )
        return false;
    cm_procs = get_color_mapping_procs_subclass(dev);
    if (cm_procs == 0 || cm_procs->map_cmyk == 0)
        return false;

    /* check the values 1/4, 1/3, 3/4, and 1/8 */

    map_cmyk_subclass( cm_procs, dev,
                        frac_1 / 4,
                        frac_1 / 3,
                        3 * frac_1 / 4,
                        frac_1 / 8,
                        cm_comp_fracs );

    /* verify results to .01 */
    cm_comp_fracs[0] -= frac_1 / 4;
    cm_comp_fracs[1] -= frac_1 / 3;
    cm_comp_fracs[2] -= 3 * frac_1 / 4;
    cm_comp_fracs[3] -= frac_1 / 8;
    for ( i = 0;
           i < 4                            &&
           -frac_1 / 100 < cm_comp_fracs[i] &&
           cm_comp_fracs[i] < frac_1 / 100;
          i++ )
        ;
    return i == 4;
}

/*
 * Two default decode_color procedures to use for monochrome devices.
 * These will make use of the map_color_rgb routine, and use the first
 * component of the returned value or its inverse.
 */
static int
gx_default_1_add_decode_color(
    gx_device *     dev,
    gx_color_index  color,
    gx_color_value  cv[1] )
{
    gx_color_value  rgb[3];
    int             code = dev_proc(dev, map_color_rgb)(dev, color, rgb);

    cv[0] = rgb[0];
    return code;
}

static int
gx_default_1_sub_decode_color(
    gx_device *     dev,
    gx_color_index  color,
    gx_color_value  cv[1] )
{
    gx_color_value  rgb[3];
    int             code = dev_proc(dev, map_color_rgb)(dev, color, rgb);

    cv[0] = gx_max_color_value - rgb[0];
    return code;
}

/*
 * A default decode_color procedure for DeviceCMYK color models.
 *
 * There is no generally accurate way of decode a DeviceCMYK color using
 * the map_color_rgb method. Unfortunately, there are many older devices
 * employ the DeviceCMYK color model but don't provide a decode_color
 * method. The code below works on the assumption of full undercolor
 * removal and black generation. This may not be accurate, but is the
 * best that can be done in the general case without other information.
 */
static int
gx_default_cmyk_decode_color(
    gx_device *     dev,
    gx_color_index  color,
    gx_color_value  cv[4] )
{
    /* The device may have been determined to be 'separable'. */
    if (dev->color_info.separable_and_linear == GX_CINFO_SEP_LIN)
        return gx_default_decode_color(dev, color, cv);
    else {
        int i, code = dev_proc(dev, map_color_rgb)(dev, color, cv);
        gx_color_value min_val = gx_max_color_value;

        for (i = 0; i < 3; i++) {
            if ((cv[i] = gx_max_color_value - cv[i]) < min_val)
                min_val = cv[i];
        }
        for (i = 0; i < 3; i++)
            cv[i] -= min_val;
        cv[3] = min_val;

        return code;
    }
}

/*
 * Special case default color decode routine for a canonical 1-bit per
 * component DeviceCMYK color model.
 */
static int
gx_1bit_cmyk_decode_color(
    gx_device *     dev,
    gx_color_index  color,
    gx_color_value  cv[4] )
{
    cv[0] = ((color & 0x8) != 0 ? gx_max_color_value : 0);
    cv[1] = ((color & 0x4) != 0 ? gx_max_color_value : 0);
    cv[2] = ((color & 0x2) != 0 ? gx_max_color_value : 0);
    cv[3] = ((color & 0x1) != 0 ? gx_max_color_value : 0);
    return 0;
}

static int
(*get_decode_color(gx_device * dev))(gx_device *, gx_color_index, gx_color_value *)
{
    /* if a method has already been provided, use it */
    if (dev_proc(dev, decode_color) != 0)
        return dev_proc(dev, decode_color);

    /*
     * If a map_color_rgb method has been provided, we may be able to use it.
     * Currently this will always be the case, as a default value will be
     * provided this method. While this default may not be correct, we are not
     * introducing any new errors by using it.
     */
    if (dev_proc(dev, map_color_rgb) != 0) {

        /* if the device has a DeviceRGB color model, use map_color_rgb */
        if (is_like_DeviceRGB(dev))
            return dev_proc(dev, map_color_rgb);

        /* If separable ande linear then use default */
        if ( dev->color_info.separable_and_linear == GX_CINFO_SEP_LIN )
            return &gx_default_decode_color;

        /* gray devices can be handled based on their polarity */
        if ( dev->color_info.num_components == 1 &&
             dev->color_info.gray_index == 0       )
            return dev->color_info.polarity == GX_CINFO_POLARITY_ADDITIVE
                       ? &gx_default_1_add_decode_color
                       : &gx_default_1_sub_decode_color;

        /*
         * There is no accurate way to decode colors for cmyk devices
         * using the map_color_rgb procedure. Unfortunately, this cases
         * arises with some frequency, so it is useful not to generate an
         * error in this case. The mechanism below assumes full undercolor
         * removal and black generation, which may not be accurate but are
         * the  best that can be done in the general case in the absence of
         * other information.
         *
         * As a hack to handle certain common devices, if the map_rgb_color
         * routine is cmyk_1bit_map_color_rgb, we provide a direct one-bit
         * decoder.
         */
        if (is_like_DeviceCMYK(dev)) {
            if (dev_proc(dev, map_color_rgb) == cmyk_1bit_map_color_rgb)
                return &gx_1bit_cmyk_decode_color;
            else
                return &gx_default_cmyk_decode_color;
        }
    }

    /*
     * The separable and linear case will already have been handled by
     * code in gx_device_fill_in_procs, so at this point we can only hope
     * the device doesn't use the decode_color method.
     */
    if (dev->color_info.separable_and_linear == GX_CINFO_SEP_LIN )
        return &gx_default_decode_color;
    else
        return &gx_error_decode_color;
}

/*
 * If a device has a linear and separable encode color function then
 * set up the comp_bits, comp_mask, and comp_shift fields.  Note:  This
 * routine assumes that the colorant shift factor decreases with the
 * component number.  See check_device_separable() for a general routine.
 */
void
set_linear_color_bits_mask_shift(gx_device * dev)
{
    int i;
    byte gray_index = dev->color_info.gray_index;
    gx_color_value max_gray = dev->color_info.max_gray;
    gx_color_value max_color = dev->color_info.max_color;
    int num_components = dev->color_info.num_components;

#define comp_bits (dev->color_info.comp_bits)
#define comp_mask (dev->color_info.comp_mask)
#define comp_shift (dev->color_info.comp_shift)
    comp_shift[num_components - 1] = 0;
    for ( i = num_components - 1 - 1; i >= 0; i-- ) {
        comp_shift[i] = comp_shift[i + 1] +
            ( i == gray_index ? ilog2(max_gray + 1) : ilog2(max_color + 1) );
    }
    for ( i = 0; i < num_components; i++ ) {
        comp_bits[i] = ( i == gray_index ?
                         ilog2(max_gray + 1) :
                         ilog2(max_color + 1) );
        comp_mask[i] = (((gx_color_index)1 << comp_bits[i]) - 1)
                                               << comp_shift[i];
    }
#undef comp_bits
#undef comp_mask
#undef comp_shift
}

/* Determine if a number is a power of two.  Works only for integers. */
#define is_power_of_two(x) ((((x) - 1) & (x)) == 0)

/*
 * This routine attempts to determine if a device's encode_color procedure
 * produces gx_color_index values which are 'separable'.  A 'separable' value
 * means two things.  Each colorant has a group of bits in the gx_color_index
 * value which is associated with the colorant.  These bits are separate.
 * I.e. no bit is associated with more than one colorant.  If a colorant has
 * a value of zero then the bits associated with that colorant are zero.
 * These criteria allows the graphics library to build gx_color_index values
 * from the colorant values and not using the encode_color routine. This is
 * useful and necessary for overprinting, halftoning more
 * than four colorants, and the fast shading logic.  However this information
 * is not setup by the default device macros.  Thus we attempt to derive this
 * information.
 *
 * This routine can be fooled.  However it usually errors on the side of
 * assuing that a device is not separable.  In this case it does not create
 * any new problems.  In theory it can be fooled into believing that a device
 * is separable when it is not.  However we do not know of any real cases that
 * will fool it.
 */
void
check_device_separable(gx_device * dev)
{
    int i, j;
    gx_device_color_info * pinfo = &(dev->color_info);
    int num_components = pinfo->num_components;
    byte comp_shift[GX_DEVICE_COLOR_MAX_COMPONENTS];
    byte comp_bits[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_index comp_mask[GX_DEVICE_COLOR_MAX_COMPONENTS];
    gx_color_index color_index;
    gx_color_index current_bits = 0;
    gx_color_value colorants[GX_DEVICE_COLOR_MAX_COMPONENTS] = { 0 };

    /* If this is already known then we do not need to do anything. */
    if (pinfo->separable_and_linear != GX_CINFO_UNKNOWN_SEP_LIN)
        return;
    /* If there is not an encode_color_routine then we cannot proceed. */
    if (dev_proc(dev, encode_color) == NULL)
        return;
    /*
     * If these values do not check then we should have an error.  However
     * we do not know what to do so we are simply exitting and hoping that
     * the device will clean up its values.
     */
    if (pinfo->gray_index < num_components &&
        (!pinfo->dither_grays || pinfo->dither_grays != (pinfo->max_gray + 1)))
            return;
    if ((num_components > 1 || pinfo->gray_index != 0) &&
        (!pinfo->dither_colors || pinfo->dither_colors != (pinfo->max_color + 1)))
        return;
    /*
     * If dither_grays or dither_colors is not a power of two then we assume
     * that the device is not separable.  In theory this not a requirement
     * but it has been true for all of the devices that we have seen so far.
     * This assumption also makes the logic in the next section easier.
     */
    if (!is_power_of_two(pinfo->dither_grays)
                    || !is_power_of_two(pinfo->dither_colors))
        return;
    /*
     * Use the encode_color routine to try to verify that the device is
     * separable and to determine the shift count, etc. for each colorant.
     */
    color_index = dev_proc(dev, encode_color)(dev, colorants);
    if (color_index != 0)
        return;		/* Exit if zero colorants produce a non zero index */
    for (i = 0; i < num_components; i++) {
        /* Check this colorant = max with all others = 0 */
        for (j = 0; j < num_components; j++)
            colorants[j] = 0;
        colorants[i] = gx_max_color_value;
        color_index = dev_proc(dev, encode_color)(dev, colorants);
        if (color_index == 0)	/* If no bits then we have a problem */
            return;
        if (color_index & current_bits)	/* Check for overlapping bits */
            return;
        current_bits |= color_index;
        comp_mask[i] = color_index;
        /* Determine the shift count for the colorant */
        for (j = 0; (color_index & 1) == 0 && color_index != 0; j++)
            color_index >>= 1;
        comp_shift[i] = j;
        /* Determine the bit count for the colorant */
        for (j = 0; color_index != 0; j++) {
            if ((color_index & 1) == 0) /* check for non-consecutive bits */
                return;
            color_index >>= 1;
        }
        comp_bits[i] = j;
        /*
         * We could verify that the bit count matches the dither_grays or
         * dither_colors values, but this is not really required unless we
         * are halftoning.  Thus we are allowing for non equal colorant sizes.
         */
        /* Check for overlap with other colorant if they are all maxed */
        for (j = 0; j < num_components; j++)
            colorants[j] = gx_max_color_value;
        colorants[i] = 0;
        color_index = dev_proc(dev, encode_color)(dev, colorants);
        if (color_index & comp_mask[i])	/* Check for overlapping bits */
            return;
    }
    /* If we get to here then the device is very likely to be separable. */
    pinfo->separable_and_linear = GX_CINFO_SEP_LIN;
    for (i = 0; i < num_components; i++) {
        pinfo->comp_shift[i] = comp_shift[i];
        pinfo->comp_bits[i] = comp_bits[i];
        pinfo->comp_mask[i] = comp_mask[i];
    }
    /*
     * The 'gray_index' value allows one colorant to have a different number
     * of shades from the remainder.  Since the default macros only guess at
     * an appropriate value, we are setting its value based upon the data that
     * we just determined.  Note:  In some cases the macros set max_gray to 0
     * and dither_grays to 1.  This is not valid so ignore this case.
     */
    for (i = 0; i < num_components; i++) {
        int dither = 1 << comp_bits[i];

        if (pinfo->dither_grays != 1 && dither == pinfo->dither_grays) {
            pinfo->gray_index = i;
            break;
        }
    }
}
#undef is_power_of_two

/* Fill in NULL procedures in a device procedure record. */
void
gx_device_fill_in_procs(register gx_device * dev)
{
    gx_device_set_procs(dev);
    fill_dev_proc(dev, open_device, gx_default_open_device);
    fill_dev_proc(dev, get_initial_matrix, gx_default_get_initial_matrix);
    fill_dev_proc(dev, sync_output, gx_default_sync_output);
    fill_dev_proc(dev, output_page, gx_default_output_page);
    fill_dev_proc(dev, close_device, gx_default_close_device);
    /* see below for map_rgb_color */
    fill_dev_proc(dev, map_color_rgb, gx_default_map_color_rgb);
    /* NOT fill_rectangle */
    fill_dev_proc(dev, tile_rectangle, gx_default_tile_rectangle);
    fill_dev_proc(dev, copy_mono, gx_default_copy_mono);
    fill_dev_proc(dev, copy_color, gx_default_copy_color);
    fill_dev_proc(dev, obsolete_draw_line, gx_default_draw_line);
    fill_dev_proc(dev, get_bits, gx_default_get_bits);
    fill_dev_proc(dev, get_params, gx_default_get_params);
    fill_dev_proc(dev, put_params, gx_default_put_params);
    /* see below for map_cmyk_color */
    fill_dev_proc(dev, get_xfont_procs, gx_default_get_xfont_procs);
    fill_dev_proc(dev, get_xfont_device, gx_default_get_xfont_device);
    fill_dev_proc(dev, map_rgb_alpha_color, gx_default_map_rgb_alpha_color);
    fill_dev_proc(dev, get_page_device, gx_default_get_page_device);
    fill_dev_proc(dev, get_alpha_bits, gx_default_get_alpha_bits);
    fill_dev_proc(dev, copy_alpha, gx_default_copy_alpha);
    fill_dev_proc(dev, get_band, gx_default_get_band);
    fill_dev_proc(dev, copy_rop, gx_default_copy_rop);
    fill_dev_proc(dev, fill_path, gx_default_fill_path);
    fill_dev_proc(dev, stroke_path, gx_default_stroke_path);
    fill_dev_proc(dev, fill_mask, gx_default_fill_mask);
    fill_dev_proc(dev, fill_trapezoid, gx_default_fill_trapezoid);
    fill_dev_proc(dev, fill_parallelogram, gx_default_fill_parallelogram);
    fill_dev_proc(dev, fill_triangle, gx_default_fill_triangle);
    fill_dev_proc(dev, draw_thin_line, gx_default_draw_thin_line);
    fill_dev_proc(dev, begin_image, gx_default_begin_image);
    /*
     * We always replace get_alpha_bits, image_data, and end_image with the
     * new procedures, and, if in a DEBUG configuration, print a warning if
     * the definitions aren't the default ones.
     */
#ifdef DEBUG
#  define CHECK_NON_DEFAULT(proc, default, procname)\
    BEGIN\
        if ( dev_proc(dev, proc) != NULL && dev_proc(dev, proc) != default )\
            dmprintf2(dev->memory, "**** Warning: device %s implements obsolete procedure %s\n",\
                     dev->dname, procname);\
    END
#else
#  define CHECK_NON_DEFAULT(proc, default, procname)\
    DO_NOTHING
#endif
    CHECK_NON_DEFAULT(get_alpha_bits, gx_default_get_alpha_bits,
                      "get_alpha_bits");
    set_dev_proc(dev, get_alpha_bits, gx_default_get_alpha_bits);
    CHECK_NON_DEFAULT(image_data, gx_default_image_data, "image_data");
    set_dev_proc(dev, image_data, gx_default_image_data);
    CHECK_NON_DEFAULT(end_image, gx_default_end_image, "end_image");
    set_dev_proc(dev, end_image, gx_default_end_image);
#undef CHECK_NON_DEFAULT
    fill_dev_proc(dev, strip_tile_rectangle, gx_default_strip_tile_rectangle);
    fill_dev_proc(dev, strip_copy_rop, gx_default_strip_copy_rop);
    fill_dev_proc(dev, strip_copy_rop2, gx_default_strip_copy_rop2);
    fill_dev_proc(dev, strip_tile_rect_devn, gx_default_strip_tile_rect_devn);
    fill_dev_proc(dev, get_clipping_box, gx_default_get_clipping_box);
    fill_dev_proc(dev, begin_typed_image, gx_default_begin_typed_image);
    fill_dev_proc(dev, get_bits_rectangle, gx_default_get_bits_rectangle);
    fill_dev_proc(dev, map_color_rgb_alpha, gx_default_map_color_rgb_alpha);
    fill_dev_proc(dev, create_compositor, gx_default_create_compositor);
    fill_dev_proc(dev, get_hardware_params, gx_default_get_hardware_params);
    fill_dev_proc(dev, text_begin, gx_default_text_begin);
    fill_dev_proc(dev, finish_copydevice, gx_default_finish_copydevice);

    set_dev_proc(dev, encode_color, get_encode_color(dev));
    if (dev->color_info.num_components == 3)
        set_dev_proc(dev, map_rgb_color, dev_proc(dev, encode_color));
    if (dev->color_info.num_components == 4)
        set_dev_proc(dev, map_cmyk_color, dev_proc(dev, encode_color));

    if ( dev->color_info.separable_and_linear == GX_CINFO_SEP_LIN ) {
        fill_dev_proc(dev, encode_color, gx_default_encode_color);
        fill_dev_proc(dev, map_cmyk_color, gx_default_encode_color);
        fill_dev_proc(dev, map_rgb_color, gx_default_encode_color);
    } else {
        /* if it isn't set now punt */
        fill_dev_proc(dev, encode_color, gx_error_encode_color);
        fill_dev_proc(dev, map_cmyk_color, gx_error_encode_color);
        fill_dev_proc(dev, map_rgb_color, gx_error_encode_color);
    }

    /*
     * Fill in the color mapping procedures and the component index
     * assignment procedure if they have not been provided by the client.
     *
     * Because it is difficult to provide default encoding procedures
     * that handle level inversion, this code needs to check both
     * the number of components and the polarity of color model.
     */
    switch (dev->color_info.num_components) {
    case 1:     /* DeviceGray or DeviceInvertGray */
        if (dev_proc(dev, get_color_mapping_procs) == NULL) {
            /*
             * If not gray then the device must provide the color
             * mapping procs.
             */
            if (dev->color_info.polarity == GX_CINFO_POLARITY_ADDITIVE) {
                fill_dev_proc( dev,
                           get_color_mapping_procs,
                           gx_default_DevGray_get_color_mapping_procs );
            } else
                fill_dev_proc(dev, get_color_mapping_procs, gx_error_get_color_mapping_procs);
        }
        fill_dev_proc( dev,
                       get_color_comp_index,
                       gx_default_DevGray_get_color_comp_index );
        break;

    case 3:
        if (dev_proc(dev, get_color_mapping_procs) == NULL) {
            if (dev->color_info.polarity == GX_CINFO_POLARITY_ADDITIVE) {
                fill_dev_proc( dev,
                           get_color_mapping_procs,
                           gx_default_DevRGB_get_color_mapping_procs );
                fill_dev_proc( dev,
                           get_color_comp_index,
                           gx_default_DevRGB_get_color_comp_index );
            } else {
                fill_dev_proc(dev, get_color_mapping_procs, gx_error_get_color_mapping_procs);
                fill_dev_proc(dev, get_color_comp_index, gx_error_get_color_comp_index);
            }
        }
        break;

    case 4:
        fill_dev_proc(dev, get_color_mapping_procs, gx_default_DevCMYK_get_color_mapping_procs);
        fill_dev_proc(dev, get_color_comp_index, gx_default_DevCMYK_get_color_comp_index);
        break;
    default:		/* Unknown color model - set error handlers */
        if (dev_proc(dev, get_color_mapping_procs) == NULL) {
            fill_dev_proc(dev, get_color_mapping_procs, gx_error_get_color_mapping_procs);
            fill_dev_proc(dev, get_color_comp_index, gx_error_get_color_comp_index);
        }
    }

    set_dev_proc(dev, decode_color, get_decode_color(dev));
    fill_dev_proc(dev, map_color_rgb, gx_default_map_color_rgb);
    fill_dev_proc(dev, get_profile, gx_default_get_profile);
    fill_dev_proc(dev, set_graphics_type_tag, gx_default_set_graphics_type_tag);

    /*
     * If the device is known not to support overprint mode, indicate this now.
     * Note that we do not insist that a device be use a strict DeviceCMYK
     * encoding; any color model that is subtractive and supports the cyan,
     * magenta, yellow, and black color components will do. We defer a more
     * explicit check until this information is explicitly required.
     */
    if ( dev->color_info.opmode == GX_CINFO_OPMODE_UNKNOWN          &&
         (dev->color_info.num_components < 4                     ||
          dev->color_info.polarity == GX_CINFO_POLARITY_ADDITIVE ||
          dev->color_info.gray_index == GX_CINFO_COMP_NO_INDEX     )  )
        dev->color_info.opmode = GX_CINFO_OPMODE_NOT;

    fill_dev_proc(dev, fill_rectangle_hl_color, gx_default_fill_rectangle_hl_color);
    fill_dev_proc(dev, include_color_space, gx_default_include_color_space);
    fill_dev_proc(dev, fill_linear_color_scanline, gx_default_fill_linear_color_scanline);
    fill_dev_proc(dev, fill_linear_color_trapezoid, gx_default_fill_linear_color_trapezoid);
    fill_dev_proc(dev, fill_linear_color_triangle, gx_default_fill_linear_color_triangle);
    fill_dev_proc(dev, update_spot_equivalent_colors, gx_default_update_spot_equivalent_colors);
    fill_dev_proc(dev, ret_devn_params, gx_default_ret_devn_params);
    fill_dev_proc(dev, fillpage, gx_default_fillpage);
    if ((dev_proc(dev, copy_planes) != NULL) &&
        (dev_proc(dev, get_bits_rectangle) != NULL)) {
        fill_dev_proc(dev, copy_alpha_hl_color, gx_default_copy_alpha_hl_color);
    }

    /* NOT push_transparency_state */
    /* NOT pop_transparency_state */
    /* NOT put_image */
    fill_dev_proc(dev, dev_spec_op, gx_default_dev_spec_op);
    /* NOT copy_planes */
    fill_dev_proc(dev, process_page, gx_default_process_page);
}

int
gx_default_open_device(gx_device * dev)
{
    /* Initialize the separable status if not known. */
    check_device_separable(dev);
    return 0;
}

/* Get the initial matrix for a device with inverted Y. */
/* This includes essentially all printers and displays. */
/* Supports LeadingEdge, but no margins or viewports */
void
gx_default_get_initial_matrix(gx_device * dev, register gs_matrix * pmat)
{
    /* NB this device has no paper margins */
    double fs_res = dev->HWResolution[0] / 72.0;
    double ss_res = dev->HWResolution[1] / 72.0;

    switch(dev->LeadingEdge & LEADINGEDGE_MASK) {
    case 1: /* 90 degrees */
        pmat->xx = 0;
        pmat->xy = -ss_res;
        pmat->yx = -fs_res;
        pmat->yy = 0;
        pmat->tx = (float)dev->width;
        pmat->ty = (float)dev->height;
        break;
    case 2: /* 180 degrees */
        pmat->xx = -fs_res;
        pmat->xy = 0;
        pmat->yx = 0;
        pmat->yy = ss_res;
        pmat->tx = (float)dev->width;
        pmat->ty = 0;
        break;
    case 3: /* 270 degrees */
        pmat->xx = 0;
        pmat->xy = ss_res;
        pmat->yx = fs_res;
        pmat->yy = 0;
        pmat->tx = 0;
        pmat->ty = 0;
        break;
    default:
    case 0:
        pmat->xx = fs_res;
        pmat->xy = 0;
        pmat->yx = 0;
        pmat->yy = -ss_res;
        pmat->tx = 0;
        pmat->ty = (float)dev->height;
        /****** tx/y is WRONG for devices with ******/
        /****** arbitrary initial matrix ******/
        break;
    }
}
/* Get the initial matrix for a device with upright Y. */
/* This includes just a few printers and window systems. */
void
gx_upright_get_initial_matrix(gx_device * dev, register gs_matrix * pmat)
{
    pmat->xx = dev->HWResolution[0] / 72.0;	/* x_pixels_per_inch */
    pmat->xy = 0;
    pmat->yx = 0;
    pmat->yy = dev->HWResolution[1] / 72.0;	/* y_pixels_per_inch */
    /****** tx/y is WRONG for devices with ******/
    /****** arbitrary initial matrix ******/
    pmat->tx = 0;
    pmat->ty = 0;
}

int
gx_default_sync_output(gx_device * dev)
{
    return 0;
}

int
gx_default_output_page(gx_device * dev, int num_copies, int flush)
{
    int code = dev_proc(dev, sync_output)(dev);

    if (code >= 0)
        code = gx_finish_output_page(dev, num_copies, flush);
    return code;
}

int
gx_default_close_device(gx_device * dev)
{
    return 0;
}

const gx_xfont_procs *
gx_default_get_xfont_procs(gx_device * dev)
{
    return NULL;
}

gx_device *
gx_default_get_xfont_device(gx_device * dev)
{
    return dev;
}

gx_device *
gx_default_get_page_device(gx_device * dev)
{
    return NULL;
}
gx_device *
gx_page_device_get_page_device(gx_device * dev)
{
    return dev;
}

int
gx_default_get_alpha_bits(gx_device * dev, graphics_object_type type)
{
    return (type == go_text ? dev->color_info.anti_alias.text_bits :
            dev->color_info.anti_alias.graphics_bits);
}

int
gx_default_get_band(gx_device * dev, int y, int *band_start)
{
    return 0;
}

void
gx_default_get_clipping_box(gx_device * dev, gs_fixed_rect * pbox)
{
    pbox->p.x = 0;
    pbox->p.y = 0;
    pbox->q.x = int2fixed(dev->width);
    pbox->q.y = int2fixed(dev->height);
}
void
gx_get_largest_clipping_box(gx_device * dev, gs_fixed_rect * pbox)
{
    pbox->p.x = min_fixed;
    pbox->p.y = min_fixed;
    pbox->q.x = max_fixed;
    pbox->q.y = max_fixed;
}

int
gx_no_create_compositor(gx_device * dev, gx_device ** pcdev,
                        const gs_composite_t * pcte,
                        gs_gstate * pgs, gs_memory_t * memory,
                        gx_device *cdev)
{
    return_error(gs_error_unknownerror);	/* not implemented */
}
int
gx_default_create_compositor(gx_device * dev, gx_device ** pcdev,
                             const gs_composite_t * pcte,
                             gs_gstate * pgs, gs_memory_t * memory,
                             gx_device *cdev)
{
    return pcte->type->procs.create_default_compositor
        (pcte, pcdev, dev, pgs, memory);
}
int
gx_null_create_compositor(gx_device * dev, gx_device ** pcdev,
                          const gs_composite_t * pcte,
                          gs_gstate * pgs, gs_memory_t * memory,
                          gx_device *cdev)
{
    *pcdev = dev;
    return 0;
}

/*
 * Default handler for creating a compositor device when writing the clist. */
int
gx_default_composite_clist_write_update(const gs_composite_t *pcte, gx_device * dev,
                gx_device ** pcdev, gs_gstate * pgs, gs_memory_t * mem)
{
    *pcdev = dev;		/* Do nothing -> return the same device */
    return 0;
}

/* Default handler for adjusting a compositor's CTM. */
int
gx_default_composite_adjust_ctm(gs_composite_t *pcte, int x0, int y0, gs_gstate *pgs)
{
    return 0;
}

/*
 * Default check for closing compositor.
 */
gs_compositor_closing_state
gx_default_composite_is_closing(const gs_composite_t *this, gs_composite_t **pcte, gx_device *dev)
{
    return COMP_ENQUEUE;
}

/*
 * Default check whether a next operation is friendly to the compositor.
 */
bool
gx_default_composite_is_friendly(const gs_composite_t *this, byte cmd0, byte cmd1)
{
    return false;
}

/*
 * Default handler for updating the clist device when reading a compositing
 * device.
 */
int
gx_default_composite_clist_read_update(gs_composite_t *pxcte, gx_device * cdev,
                gx_device * tdev, gs_gstate * pgs, gs_memory_t * mem)
{
    return 0;			/* Do nothing */
}

/*
 * Default handler for get_cropping returns no cropping.
 */
int
gx_default_composite_get_cropping(const gs_composite_t *pxcte, int *ry, int *rheight,
                                  int cropping_min, int cropping_max)
{
    return 0;			/* No cropping. */
}

int
gx_default_finish_copydevice(gx_device *dev, const gx_device *from_dev)
{
    /* Only allow copying the prototype. */
    return (from_dev->memory ? gs_note_error(gs_error_rangecheck) : 0);
}

int
gx_default_dev_spec_op(gx_device *pdev, int dev_spec_op, void *data, int size)
{
    switch(dev_spec_op) {
        case gxdso_form_begin:
        case gxdso_form_end:
        case gxdso_pattern_can_accum:
        case gxdso_pattern_start_accum:
        case gxdso_pattern_finish_accum:
        case gxdso_pattern_load:
        case gxdso_pattern_shading_area:
        case gxdso_pattern_is_cpath_accum:
        case gxdso_pattern_handles_clip_path:
        case gxdso_is_pdf14_device:
        case gxdso_supports_devn:
        case gxdso_supports_hlcolor:
        case gxdso_supports_saved_pages:
        case gxdso_needs_invariant_palette:
            return 0;
        case gxdso_pattern_shfill_doesnt_need_path:
            return (pdev->procs.fill_path == gx_default_fill_path);
        case gxdso_is_std_cmyk_1bit:
            return (pdev->procs.map_cmyk_color == cmyk_1bit_map_cmyk_color);
        case gxdso_interpolate_antidropout:
            return pdev->color_info.use_antidropout_downscaler;
        case gxdso_interpolate_threshold:
            if ((pdev->color_info.num_components == 1 &&
                 pdev->color_info.max_gray < 15) ||
                (pdev->color_info.num_components > 1 &&
                 pdev->color_info.max_color < 15)) {
                /* If we are a limited color device (i.e. we are halftoning)
                 * then only interpolate if we are upscaling by at least 4 */
                return 4;
            }
            return 0; /* Otherwise no change */
        case gxdso_get_dev_param:
            {
                dev_param_req_t *request = (dev_param_req_t *)data;
                return gx_default_get_param(pdev, request->Param, request->list);
            }
    }
    return_error(gs_error_undefined);
}

int
gx_default_fill_rectangle_hl_color(gx_device *pdev,
    const gs_fixed_rect *rect,
    const gs_gstate *pgs, const gx_drawing_color *pdcolor,
    const gx_clip_path *pcpath)
{
    return_error(gs_error_rangecheck);
}

int
gx_default_include_color_space(gx_device *pdev, gs_color_space *cspace,
        const byte *res_name, int name_length)
{
    return 0;
}

/*
 * If a device wants to determine an equivalent color for its spot colors then
 * it needs to implement this method.  See comments at the start of
 * src/gsequivc.c.
 */
int
gx_default_update_spot_equivalent_colors(gx_device *pdev, const gs_gstate * pgs)
{
    return 0;
}

/*
 * If a device wants to determine implement support for spot colors then
 * it needs to implement this method.
 */
gs_devn_params *
gx_default_ret_devn_params(gx_device *pdev)
{
    return NULL;
}

int
gx_default_process_page(gx_device *dev, gx_process_page_options_t *options)
{
    gs_int_rect rect;
    int code = 0;
    void *buffer = NULL;

    /* Possible future improvements in here could be given by us dividing the
     * page up into n chunks, and spawning a thread per chunk to do the
     * process_fn call on. n could be given by NumRenderingThreads. This
     * would give us multi-core advantages even without clist. */
    if (options->init_buffer_fn) {
        code = options->init_buffer_fn(options->arg, dev, dev->memory, dev->width, dev->height, &buffer);
        if (code < 0)
            return code;
    }

    rect.p.x = 0;
    rect.p.y = 0;
    rect.q.x = dev->width;
    rect.q.y = dev->height;
    if (options->process_fn)
        code = options->process_fn(options->arg, dev, dev, &rect, buffer);
    if (code >= 0 && options->output_fn)
        code = options->output_fn(options->arg, dev, buffer);

    if (options->free_buffer_fn)
        options->free_buffer_fn(options->arg, dev, dev->memory, buffer);

    return code;
}

/* ---------------- Default per-instance procedures ---------------- */

int
gx_default_install(gx_device * dev, gs_gstate * pgs)
{
    return 0;
}

int
gx_default_begin_page(gx_device * dev, gs_gstate * pgs)
{
    return 0;
}

int
gx_default_end_page(gx_device * dev, int reason, gs_gstate * pgs)
{
    return (reason != 2 ? 1 : 0);
}

void
gx_default_set_graphics_type_tag(gx_device *dev, gs_graphics_type_tag_t graphics_type_tag)
{
    /* set the tag but carefully preserve GS_DEVICE_ENCODES_TAGS */
    dev->graphics_type_tag = (dev->graphics_type_tag & GS_DEVICE_ENCODES_TAGS) | graphics_type_tag;
}

/* ---------------- Device subclassing procedures ---------------- */

/* Non-obvious code. The 'dest_procs' is the 'procs' memory occupied by the original device that we decided to subclass,
 * 'src_procs' is the newly allocated piece of memory, to whch we have already copied the content of the
 * original device (including the procs), prototype is the device structure prototype for the subclassing device.
 * Here we copy the methods from the prototype to the original device procs memory *but* if the original (src_procs)
 * device had a NULL method, we make the new device procs have a NULL method too.
 * The reason for ths is ugly, there are some places in the graphics library which explicitly check for
 * a device having a NULL method and take different code paths depending on the result.
 * Now in general we expect subclassing devices to implement *every* method, so if we didn't copy
 * over NULL methods present in the original source device then the code path could be inappropriate for
 * that underlying (now subclassed) device.
 */
int gx_copy_device_procs(gx_device_procs *dest_procs, gx_device_procs *src_procs, gx_device_procs *prototype_procs)
{
    if (src_procs->open_device != NULL)
        dest_procs->open_device = prototype_procs->open_device;
    if (src_procs->get_initial_matrix != NULL)
        dest_procs->get_initial_matrix = prototype_procs->get_initial_matrix;
    if (src_procs->sync_output != NULL)
        dest_procs->sync_output = prototype_procs->sync_output;
    if (src_procs->output_page != NULL)
        dest_procs->output_page = prototype_procs->output_page;
    if (src_procs->close_device != NULL)
        dest_procs->close_device = prototype_procs->close_device;
    if (src_procs->map_rgb_color != NULL)
        dest_procs->map_rgb_color = prototype_procs->map_rgb_color;
    if (src_procs->map_color_rgb != NULL)
        dest_procs->map_color_rgb = prototype_procs->map_color_rgb;
    if (src_procs->fill_rectangle != NULL)
        dest_procs->fill_rectangle = prototype_procs->fill_rectangle;
    if (src_procs->tile_rectangle != NULL)
        dest_procs->tile_rectangle = prototype_procs->tile_rectangle;
    if (src_procs->copy_mono != NULL)
        dest_procs->copy_mono = prototype_procs->copy_mono;
    if (src_procs->copy_color != NULL)
        dest_procs->copy_color = prototype_procs->copy_color;
    if (src_procs->obsolete_draw_line != NULL)
        dest_procs->obsolete_draw_line = prototype_procs->obsolete_draw_line;
    if (src_procs->get_bits != NULL)
        dest_procs->get_bits = prototype_procs->get_bits;
    if (src_procs->get_params != NULL)
        dest_procs->get_params = prototype_procs->get_params;
    if (src_procs->put_params != NULL)
        dest_procs->put_params = prototype_procs->put_params;
    if (src_procs->map_cmyk_color != NULL)
        dest_procs->map_cmyk_color = prototype_procs->map_cmyk_color;
    if (src_procs->get_xfont_procs != NULL)
        dest_procs->get_xfont_procs = prototype_procs->get_xfont_procs;
    if (src_procs->get_xfont_device != NULL)
        dest_procs->get_xfont_device = prototype_procs->get_xfont_device;
    if (src_procs->map_rgb_alpha_color != NULL)
        dest_procs->map_rgb_alpha_color = prototype_procs->map_rgb_alpha_color;
    if (src_procs->get_page_device != NULL)
        dest_procs->get_page_device = prototype_procs->get_page_device;
    if (src_procs->get_alpha_bits != NULL)
        dest_procs->get_alpha_bits = prototype_procs->get_alpha_bits;
    if (src_procs->copy_alpha != NULL)
        dest_procs->copy_alpha = prototype_procs->copy_alpha;
    if (src_procs->get_band != NULL)
        dest_procs->get_band = prototype_procs->get_band;
    if (src_procs->copy_rop != NULL)
        dest_procs->copy_rop = prototype_procs->copy_rop;
    if (src_procs->fill_path != NULL)
        dest_procs->fill_path = prototype_procs->fill_path;
    if (src_procs->stroke_path != NULL)
        dest_procs->stroke_path = prototype_procs->stroke_path;
    if (src_procs->fill_mask != NULL)
        dest_procs->fill_mask = prototype_procs->fill_mask;
    if (src_procs->fill_trapezoid != NULL)
        dest_procs->fill_trapezoid = prototype_procs->fill_trapezoid;
    if (src_procs->fill_parallelogram != NULL)
        dest_procs->fill_parallelogram = prototype_procs->fill_parallelogram;
    if (src_procs->fill_triangle != NULL)
        dest_procs->fill_triangle = prototype_procs->fill_triangle;
    if (src_procs->draw_thin_line != NULL)
        dest_procs->draw_thin_line = prototype_procs->draw_thin_line;
    if (src_procs->begin_image != NULL)
        dest_procs->begin_image = prototype_procs->begin_image;
    if (src_procs->image_data != NULL)
        dest_procs->image_data = prototype_procs->image_data;
    if (src_procs->end_image != NULL)
        dest_procs->end_image = prototype_procs->end_image;
    if (src_procs->strip_tile_rectangle != NULL)
        dest_procs->strip_tile_rectangle = prototype_procs->strip_tile_rectangle;
    if (src_procs->strip_copy_rop != NULL)
        dest_procs->strip_copy_rop = prototype_procs->strip_copy_rop;
    if (src_procs->get_clipping_box != NULL)
        dest_procs->get_clipping_box = prototype_procs->get_clipping_box;
    if (src_procs->begin_typed_image != NULL)
        dest_procs->begin_typed_image = prototype_procs->begin_typed_image;
    if (src_procs->get_bits_rectangle != NULL)
        dest_procs->get_bits_rectangle = prototype_procs->get_bits_rectangle;
    if (src_procs->map_color_rgb_alpha != NULL)
        dest_procs->map_color_rgb_alpha = prototype_procs->map_color_rgb_alpha;
    if (src_procs->create_compositor != NULL)
        dest_procs->create_compositor = prototype_procs->create_compositor;
    if (src_procs->get_hardware_params != NULL)
        dest_procs->get_hardware_params = prototype_procs->get_hardware_params;
    if (src_procs->text_begin != NULL)
        dest_procs->text_begin = prototype_procs->text_begin;
    if (src_procs->finish_copydevice != NULL)
        dest_procs->finish_copydevice = prototype_procs->finish_copydevice;
    if (src_procs->begin_transparency_group != NULL)
        dest_procs->begin_transparency_group = prototype_procs->begin_transparency_group;
    if (src_procs->end_transparency_group != NULL)
        dest_procs->end_transparency_group = prototype_procs->end_transparency_group;
    if (src_procs->discard_transparency_layer != NULL)
        dest_procs->discard_transparency_layer = prototype_procs->discard_transparency_layer;
    if (src_procs->get_color_mapping_procs != NULL)
        dest_procs->get_color_mapping_procs = prototype_procs->get_color_mapping_procs;
    if (src_procs->get_color_comp_index != NULL)
        dest_procs->get_color_comp_index = prototype_procs->get_color_comp_index;
    if (src_procs->encode_color != NULL)
        dest_procs->encode_color = prototype_procs->encode_color;
    if (src_procs->decode_color != NULL)
        dest_procs->decode_color = prototype_procs->decode_color;
    if (src_procs->pattern_manage != NULL)
        dest_procs->pattern_manage = prototype_procs->pattern_manage;
    if (src_procs->fill_rectangle_hl_color != NULL)
        dest_procs->fill_rectangle_hl_color = prototype_procs->fill_rectangle_hl_color;
    if (src_procs->include_color_space != NULL)
        dest_procs->include_color_space = prototype_procs->include_color_space;
    if (src_procs->fill_linear_color_scanline != NULL)
        dest_procs->fill_linear_color_scanline = prototype_procs->fill_linear_color_scanline;
    if (src_procs->fill_linear_color_trapezoid != NULL)
        dest_procs->fill_linear_color_trapezoid = prototype_procs->fill_linear_color_trapezoid;
    if (src_procs->fill_linear_color_triangle != NULL)
        dest_procs->fill_linear_color_triangle = prototype_procs->fill_linear_color_triangle;
    if (src_procs->update_spot_equivalent_colors != NULL)
        dest_procs->update_spot_equivalent_colors = prototype_procs->update_spot_equivalent_colors;
    if (src_procs->ret_devn_params != NULL)
        dest_procs->ret_devn_params = prototype_procs->ret_devn_params;
    if (src_procs->fillpage != NULL)
        dest_procs->fillpage = prototype_procs->fillpage;
    if (src_procs->push_transparency_state != NULL)
        dest_procs->push_transparency_state = prototype_procs->push_transparency_state;
    if (src_procs->pop_transparency_state != NULL)
        dest_procs->pop_transparency_state = prototype_procs->pop_transparency_state;
    if (src_procs->put_image != NULL)
        dest_procs->put_image = prototype_procs->put_image;
    if (src_procs->dev_spec_op != NULL)
        dest_procs->dev_spec_op = prototype_procs->dev_spec_op;
    if (src_procs->copy_planes != NULL)
        dest_procs->copy_planes = prototype_procs->copy_planes;
    if (src_procs->get_profile != NULL)
        dest_procs->get_profile = prototype_procs->get_profile;
    if (src_procs->set_graphics_type_tag != NULL)
        dest_procs->set_graphics_type_tag = prototype_procs->set_graphics_type_tag;
    if (src_procs->strip_copy_rop2 != NULL)
        dest_procs->strip_copy_rop2 = prototype_procs->strip_copy_rop2;
    if (src_procs->strip_tile_rect_devn != NULL)
        dest_procs->strip_tile_rect_devn = prototype_procs->strip_tile_rect_devn;
    if (src_procs->copy_alpha_hl_color != NULL)
        dest_procs->copy_alpha_hl_color = prototype_procs->copy_alpha_hl_color;
    if (src_procs->process_page != NULL)
        dest_procs->process_page = prototype_procs->process_page;
    return 0;
}

int gx_device_subclass(gx_device *dev_to_subclass, gx_device *new_prototype, unsigned int private_data_size)
{
    gx_device *child_dev;
    void *psubclass_data;
    gs_memory_struct_type_t *a_std;
    int dynamic = dev_to_subclass->stype_is_dynamic;
    char *ptr, *ptr1;

    /* If this happens we are stuffed, as there is no way to get hold
     * of the original device's stype structure, which means we cannot
     * allocate a replacement structure. Abort if so.
     * Also abort if the new_prototype device struct is too large.
     */
    if (!dev_to_subclass->stype ||
        dev_to_subclass->stype->ssize < new_prototype->params_size)
        return_error(gs_error_VMerror);

    /* We make a 'stype' structure for our new device, and copy the old stype into it
     * This means our new device will always have the 'stype_is_dynamic' flag set
     */
    a_std = (gs_memory_struct_type_t *)
        gs_alloc_bytes_immovable(dev_to_subclass->memory->non_gc_memory, sizeof(*a_std),
                                 "gs_device_subclass(stype)");
    if (!a_std)
        return_error(gs_error_VMerror);
    *a_std = *dev_to_subclass->stype;
    a_std->ssize = dev_to_subclass->params_size;

    /* Allocate a device structure for the new child device */
    child_dev = gs_alloc_struct_immovable(dev_to_subclass->memory, gx_device, a_std,
                                        "gs_device_subclass(device)");
    if (child_dev == 0) {
        gs_free_const_object(dev_to_subclass->memory->non_gc_memory, a_std, "gs_device_subclass(stype)");
        return_error(gs_error_VMerror);
    }

    /* Make sure all methods are filled in, note this won't work for a forwarding device
     * so forwarding devices will have to be filled in before being subclassed. This doesn't fill
     * in the fill_rectangle proc, that gets done in the ultimate device's open proc.
     */
    gx_device_fill_in_procs(dev_to_subclass);
    memcpy(child_dev, dev_to_subclass, dev_to_subclass->stype->ssize);
    child_dev->stype = a_std;
    child_dev->stype_is_dynamic = 1;

    psubclass_data = (void *)gs_alloc_bytes(dev_to_subclass->memory->non_gc_memory, private_data_size, "subclass memory for subclassing device");
    if (psubclass_data == 0){
        gs_free_const_object(dev_to_subclass->memory->non_gc_memory, a_std, "gs_device_subclass(stype)");
        gs_free(dev_to_subclass->memory, child_dev, 1, dev_to_subclass->stype->ssize, "free subclass memory for subclassing device");
        return_error(gs_error_VMerror);
    }
    memset(psubclass_data, 0x00, private_data_size);

    gx_copy_device_procs(&dev_to_subclass->procs, &child_dev->procs, &new_prototype->procs);
    dev_to_subclass->procs.fill_rectangle = new_prototype->procs.fill_rectangle;
    dev_to_subclass->procs.copy_planes = new_prototype->procs.copy_planes;
    dev_to_subclass->finalize = new_prototype->finalize;
    dev_to_subclass->dname = new_prototype->dname;
    if (dev_to_subclass->icc_struct)
        rc_increment(dev_to_subclass->icc_struct);
    if (dev_to_subclass->PageList)
        rc_increment(dev_to_subclass->PageList);

    /* In case the new device we're creating has already been initialised, copy
     * its additional data.
     */
    ptr = ((char *)dev_to_subclass) + sizeof(gx_device);
    ptr1 = ((char *)new_prototype) + sizeof(gx_device);
    memcpy(ptr, ptr1, new_prototype->params_size - sizeof(gx_device));

    /* We have to patch up the "type" parameters that the memory manage/garbage
     * collector will use, as well.
     */
    (((obj_header_t *)dev_to_subclass) - 1)->o_type = new_prototype->stype;

    /* If the original device's stype structure was dynamically allocated, we need
     * to 'fixup' the contents, it's procs need to point to the new device's procs
     * for instance.
     */
    if (dynamic) {
        if (new_prototype->stype) {
            a_std = (gs_memory_struct_type_t *)dev_to_subclass->stype;
            *a_std = *new_prototype->stype;
        } else {
            gs_free_const_object(child_dev->memory->non_gc_memory, dev_to_subclass->stype,
                             "unsubclass");
            dev_to_subclass->stype = NULL;
            dev_to_subclass->stype_is_dynamic = 0;
        }
    }

    dev_to_subclass->subclass_data = psubclass_data;
    dev_to_subclass->child = child_dev;
    if (child_dev->parent) {
        dev_to_subclass->parent = child_dev->parent;
        child_dev->parent->child = dev_to_subclass;
    }
    child_dev->parent = dev_to_subclass;

    return 0;
}

int gx_device_unsubclass(gx_device *dev)
{
    void *psubclass_data;
    gx_device *parent, *child;
    gs_memory_struct_type_t *a_std = 0;
    int dynamic;

    /* This should not happen... */
    if (!dev)
        return 0;

    child = dev->child;
    psubclass_data = dev->subclass_data;
    parent = dev->parent;
    dynamic = dev->stype_is_dynamic;

    /* If ths device's stype is dynamically allocated, keep a copy of it
     * in case we might need it.
     */
    if (dynamic) {
        a_std = (gs_memory_struct_type_t *)dev->stype;
        if (child)
            *a_std = *child->stype;
    }

    /* If ths device has any private storage, free it now */
    if (psubclass_data)
        gs_free_object(dev->memory->non_gc_memory, psubclass_data, "subclass memory for first-last page");

    /* Copy the child device into ths device's memory */
    if (child)
        memcpy(dev, child, child->stype->ssize);

    /* How can we have a subclass device with no child ? Simples; when we hit the end of job
     * restore, the devices are not freed in device chain order. To make sure we don't end up
     * following stale pointers, when a device is freed we remove it from the chain and update
     * any danlging poitners to NULL. When we later free the remaining devices its possible that
     * their child pointer can then be NULL.
     */
    if (child) {
        if (child->icc_struct)
            rc_decrement(child->icc_struct, "gx_unsubclass_device, icc_struct");
        if (child->PageList)
            rc_decrement(child->PageList, "gx_unsubclass_device, PageList");
        /* we cannot afford to free the child device if its stype is not dynamic because
         * we can't 'null' the finalise routine, and we cannot permit the device to be finalised
         * because we have copied it up one level, not discarded it.
         * (this shouldn't happen! Child devices are always created with a dynamic stype)
         * If this ever happens garbage collecton will eventually clean up the memory.
         */
        if (child->stype_is_dynamic) {
            gs_memory_struct_type_t *b_std = (gs_memory_struct_type_t *)child->stype;
            /* We definitley do *not* want to finalise the device, we just copied it up a level */
            b_std->finalize = 0;
            gs_free_object(dev->memory, child, "gx_unsubclass_device(device)");
        }
    }
    if(child)
        child->parent = dev;
    dev->parent = parent;

    /* If this device has a dynamic stype, we wnt to keep using it, but we copied
     * the stype pointer from the child when we copied the rest of the device. So
     * we update the stype pointer with the saved pointer to this device's stype.
     */
    if (dynamic) {
        dev->stype = a_std;
        dev->stype_is_dynamic = 1;
    } else {
        dev->stype_is_dynamic = 0;
    }

    return 0;
}

int gx_update_from_subclass(gx_device *dev)
{
    if (!dev->child)
        return 0;

    memcpy(&dev->color_info, &dev->child->color_info, sizeof(gx_device_color_info));
    memcpy(&dev->cached_colors, &dev->child->cached_colors, sizeof(gx_device_cached_colors_t));
    dev->max_fill_band = dev->child->max_fill_band;
    dev->width = dev->child->width;
    dev->height = dev->child->height;
    dev->pad = dev->child->pad;
    dev->log2_align_mod = dev->child->log2_align_mod;
    dev->max_fill_band = dev->child->max_fill_band;
    dev->is_planar = dev->child->is_planar;
    dev->LeadingEdge = dev->child->LeadingEdge;
    memcpy(&dev->ImagingBBox, &dev->child->ImagingBBox, sizeof(dev->child->ImagingBBox));
    dev->ImagingBBox_set = dev->child->ImagingBBox_set;
    memcpy(&dev->MediaSize, &dev->child->MediaSize, sizeof(dev->child->MediaSize));
    memcpy(&dev->HWResolution, &dev->child->HWResolution, sizeof(dev->child->HWResolution));
    memcpy(&dev->Margins, &dev->child->Margins, sizeof(dev->child->Margins));
    memcpy(&dev->HWMargins, &dev->child->HWMargins, sizeof(dev->child->HWMargins));
    dev->FirstPage = dev->child->FirstPage;
    dev->LastPage = dev->child->LastPage;
    dev->PageCount = dev->child->PageCount;
    dev->ShowpageCount = dev->child->ShowpageCount;
    dev->NumCopies = dev->child->NumCopies;
    dev->NumCopies_set = dev->child->NumCopies_set;
    dev->IgnoreNumCopies = dev->child->IgnoreNumCopies;
    dev->UseCIEColor = dev->child->UseCIEColor;
    dev->LockSafetyParams= dev->child->LockSafetyParams;
    dev->band_offset_x = dev->child->band_offset_y;
    dev->sgr = dev->child->sgr;
    dev->MaxPatternBitmap = dev->child->MaxPatternBitmap;
    dev->page_uses_transparency = dev->child->page_uses_transparency;
    memcpy(&dev->space_params, &dev->child->space_params, sizeof(gdev_space_params));
    dev->graphics_type_tag = dev->child->graphics_type_tag;

    return 0;
}

int gx_subclass_create_compositor(gx_device *dev, gx_device **pcdev, const gs_composite_t *pcte,
    gs_gstate *pgs, gs_memory_t *memory, gx_device *cdev)
{
    pdf14_clist_device *p14dev;
    generic_subclass_data *psubclass_data;
    int code = 0;

    p14dev = (pdf14_clist_device *)dev;
    psubclass_data = p14dev->target->subclass_data;

    dev->procs.create_compositor = psubclass_data->saved_compositor_method;

    if (gs_is_pdf14trans_compositor(pcte) != 0 && strncmp(dev->dname, "pdf14clist", 10) == 0) {
        const gs_pdf14trans_t * pdf14pct = (const gs_pdf14trans_t *) pcte;

        switch (pdf14pct->params.pdf14_op) {
            case PDF14_POP_DEVICE:
                {
                    pdf14_clist_device *p14dev = (pdf14_clist_device *)dev;
                    gx_device *subclass_device;

                    p14dev->target->color_info = p14dev->saved_target_color_info;
                    if (p14dev->target->child)
                        p14dev->target->child->color_info = p14dev->saved_target_color_info;

                    p14dev->target->child->procs.encode_color = p14dev->saved_target_encode_color;
                    p14dev->target->child->procs.decode_color = p14dev->saved_target_decode_color;
                    p14dev->target->child->procs.get_color_mapping_procs = p14dev->saved_target_get_color_mapping_procs;
                    p14dev->target->child->procs.get_color_comp_index = p14dev->saved_target_get_color_comp_index;

                    pgs->get_cmap_procs = p14dev->save_get_cmap_procs;
                    gx_set_cmap_procs(pgs, p14dev->target);

                    subclass_device = p14dev->target;
                    p14dev->target = p14dev->target->child;

                    code = dev->procs.create_compositor(dev, pcdev, pcte, pgs, memory, cdev);

                    p14dev->target = subclass_device;

                    return code;
                }
                break;
            default:
                code = dev->procs.create_compositor(dev, pcdev, pcte, pgs, memory, cdev);
                break;
        }
    } else {
        code = dev->procs.create_compositor(dev, pcdev, pcte, pgs, memory, cdev);
    }
    dev->procs.create_compositor = gx_subclass_create_compositor;
    return code;
}
