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


/* "Plain bits" devices to measure rendering time. */

#include "gdevprn.h"
#include "gsparam.h"
#include "gscrd.h"
#include "gscrdp.h"
#include "gxlum.h"
#include "gxdcconv.h"
#include "gdevdcrd.h"
#include "gsutil.h" /* for bittags hack */

/* Define the device parameters. */
#ifndef X_DPI
#  define X_DPI 72
#endif
#ifndef Y_DPI
#  define Y_DPI 72
#endif

/* The device descriptor */
static dev_proc_get_color_mapping_procs(bittag_get_color_mapping_procs);
static dev_proc_map_rgb_color(bittag_rgb_map_rgb_color);
static dev_proc_map_color_rgb(bittag_map_color_rgb);
static dev_proc_put_params(bittag_put_params);
static dev_proc_map_rgb_color(bit_mono_map_color);
#if 0 /* unused */
static dev_proc_map_rgb_color(bit_forcemono_map_rgb_color);
#endif
static dev_proc_map_rgb_color(bitrgb_rgb_map_rgb_color);
static dev_proc_map_color_rgb(bit_map_color_rgb);
static dev_proc_map_cmyk_color(bit_map_cmyk_color);
static dev_proc_get_params(bit_get_params);
static dev_proc_put_params(bit_put_params);
static dev_proc_print_page(bit_print_page);
static dev_proc_print_page(bittags_print_page);
static dev_proc_put_image(bit_put_image);
dev_proc_get_color_comp_index(gx_default_DevRGB_get_color_comp_index);

#define bit_procs(encode_color)\
{	gdev_prn_open,\
        gx_default_get_initial_matrix,\
        NULL,	/* sync_output */\
        /* Since the print_page doesn't alter the device, this device can print in the background */\
        gdev_prn_bg_output_page,\
        gdev_prn_close,\
        encode_color,	/* map_rgb_color */\
        bit_map_color_rgb,	/* map_color_rgb */\
        NULL,	/* fill_rectangle */\
        NULL,	/* tile_rectangle */\
        NULL,	/* copy_mono */\
        NULL,	/* copy_color */\
        NULL,	/* draw_line */\
        NULL,	/* get_bits */\
        bit_get_params,\
        bit_put_params,\
        encode_color,	/* map_cmyk_color */\
        NULL,	/* get_xfont_procs */\
        NULL,	/* get_xfont_device */\
        NULL,	/* map_rgb_alpha_color */\
        gx_page_device_get_page_device,	/* get_page_device */\
        NULL,	/* get_alpha_bits */\
        NULL,	/* copy_alpha */\
        NULL,	/* get_band */\
        NULL,	/* copy_rop */\
        NULL,	/* fill_path */\
        NULL,	/* stroke_path */\
        NULL,	/* fill_mask */\
        NULL,	/* fill_trapezoid */\
        NULL,	/* fill_parallelogram */\
        NULL,	/* fill_triangle */\
        NULL,	/* draw_thin_line */\
        NULL,	/* begin_image */\
        NULL,	/* image_data */\
        NULL,	/* end_image */\
        NULL,	/* strip_tile_rectangle */\
        NULL,	/* strip_copy_rop */\
        NULL,	/* get_clipping_box */\
        NULL,	/* begin_typed_image */\
        NULL,	/* get_bits_rectangle */\
        NULL,	/* map_color_rgb_alpha */\
        NULL,	/* create_compositor */\
        NULL,	/* get_hardware_params */\
        NULL,	/* text_begin */\
        NULL,	/* finish_copydevice */\
        NULL,	/* begin_transparency_group */\
        NULL,	/* end_transparency_group */\
        NULL,	/* begin_transparency_mask */\
        NULL,	/* end_transparency_mask */\
        NULL,	/* discard_transparency_layer */\
        NULL,	/* get_color_mapping_procs */\
        NULL,	/* get_color_comp_index */\
        encode_color,		/* encode_color */\
        bit_map_color_rgb	/* decode_color */\
}

/*
 * The following macro is used in get_params and put_params to determine the
 * num_components for the current device. It works using the device name
 * character after "bit" which is either '\0', 'r', or 'c'. Any new devices
 * that are added to this module must modify this macro to return the
 * correct num_components. This is needed to support the ForceMono
 * parameter, which alters dev->num_components.
 */
#define REAL_NUM_COMPONENTS(dev) (dev->dname[3] == 'c' ? 4 : \
                                  dev->dname[3] == 'r' ? 3 : 1)
struct gx_device_bit_s {
    gx_device_common;
    gx_prn_device_common;
    int  FirstLine, LastLine;	/* to allow multi-threaded rendering testing */
};
typedef struct gx_device_bit_s gx_device_bit;

static const gx_device_procs bitmono_procs =
bit_procs(bit_mono_map_color);
const gx_device_bit gs_bit_device =
{prn_device_body(gx_device_bit, bitmono_procs, "bit",
                 DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
                 X_DPI, Y_DPI,
                 0, 0, 0, 0,    /* margins */
                 1, 1, 1, 0, 2, 1, bit_print_page)
};

static const gx_device_procs bitrgb_procs =
bit_procs(bitrgb_rgb_map_rgb_color);
const gx_device_bit gs_bitrgb_device =
{prn_device_body(gx_device_bit, bitrgb_procs, "bitrgb",
                 DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
                 X_DPI, Y_DPI,
                 0, 0, 0, 0,	/* margins */
                 3, 4, 1, 1, 2, 2, bit_print_page)
};

static const gx_device_procs bitcmyk_procs =
bit_procs(bit_map_cmyk_color);
const gx_device_bit gs_bitcmyk_device =
{prn_device_body(gx_device_bit, bitcmyk_procs, "bitcmyk",
                 DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
                 X_DPI, Y_DPI,
                 0, 0, 0, 0,	/* margins */
                 4, 4, 1, 1, 2, 2, bit_print_page)
};

static const gx_device_procs bitrgbtags_procs =
    {
        gdev_prn_open,                        /* open_device */
        gx_default_get_initial_matrix,        /* initial_matrix */
        ((void *)0),                        /* sync_output */
        gdev_prn_output_page,                 /* output page */
        gdev_prn_close,                       /* close_device */
        bittag_rgb_map_rgb_color,             /* map rgb color */
        bittag_map_color_rgb,                 /* map color rgb */
        ((void *)0),                        /* fill_rectangle */
        ((void *)0),                        /* tile rectangle */
        ((void *)0),                        /* copy mono */
        ((void *)0),                        /* copy color */
        ((void *)0),                        /* obsolete draw line */
        ((void *)0),                        /* get_bits */
        gdev_prn_get_params,                  /* get params */
        bittag_put_params,                    /* put params */
        bittag_rgb_map_rgb_color,             /* map_cmyk_color */
        ((void *)0),                        /* get_xfonts */
        ((void *)0),                        /* get_xfont_device */
        ((void *)0),                        /* map_rgb_alpha_color */
        gx_page_device_get_page_device,       /* get_page_device */
        ((void *)0),                        /* get_alpha_bits */
        ((void *)0),                        /* copy_alpha */
        ((void *)0),                        /* get_band */
        ((void *)0),                        /* copy_rop */
        ((void *)0),                       /* fill_path */
        ((void *)0),                       /* stroke_path */
        ((void *)0),                       /* fill_mask */
        ((void *)0),                        /* fill_trapezoid */
        ((void *)0),                        /* fill_parallelogram */
        ((void *)0),                        /* fill_triangle */
        ((void *)0),                        /* draw_thin_line */
        ((void *)0),                        /* begin_image */
        ((void *)0),                        /* image_data */
        ((void *)0),                        /* end_image */
        ((void *)0),                        /* strip_tile_rectangle */
        ((void *)0),                        /* strip_copy_rop */
        ((void *)0),                        /* get_clipping_box */
        ((void *)0),                        /* begin_typed_image */
        ((void *)0),                        /* get_bits_rectangle */
        ((void *)0),                        /* map_color_rgb_alpha */
        ((void *)0),                       /* create_compositor */
        ((void *)0),                       /* get_hardware_params */
        ((void *)0),                       /* text_begin */
        ((void *)0),                       /* finish_copydevice */
        ((void *)0),                       /* begin_transparency_group */
        ((void *)0),                       /* end_transparency_group */
        ((void *)0),                       /* begin_transparency_mask */
        ((void *)0),                       /* end_transparency_mask */
        ((void *)0),                       /* discard_transparency_layer */
        bittag_get_color_mapping_procs,      /* get_color_mapping_procs */
        gx_default_DevRGB_get_color_comp_index, /* get_color_comp_index */
        bittag_rgb_map_rgb_color,           /* encode_color */
        bittag_map_color_rgb,               /* decode_color */
        ((void *)0),                        /* pattern_manage */
        ((void *)0),                        /* fill_rectangle_hl_color */
        ((void *)0),                        /* include_color_space */
        ((void *)0),                        /* fill_linear_color_scanline */
        ((void *)0),                        /* fill_linear_color_trapezoid */
        ((void *)0),                        /* fill_linear_color_triangle */
        ((void *)0),                        /* update_spot_equivalent_colors */
        ((void *)0),                        /* ret_devn_params */
        ((void *)0),                        /* fillpage */
        ((void *)0),                        /* push_transparency_state */
        ((void *)0),                        /* pop_transparency_state */
        bit_put_image                        /* put_image */
    };

const gx_device_bit gs_bitrgbtags_device =
    {
        sizeof(gx_device_bit),
        &bitrgbtags_procs,
        "bitrgbtags",
        0 ,                             /* memory */
        &st_device_printer,
        0 ,                             /* stype_is_dynamic */
        0 ,                             /* finalize */
        { 0 } ,                         /* rc header */
        0 ,                             /* retained */
        0 ,                             /* parent */
        0 ,                             /* child */
        0 ,                             /* subclass data */
        0 ,                             /* PageList */
        0 ,                             /* is open */
        0,                              /* max_fill_band */
        {                               /* color infor */
            3,                          /* max_components */
            3,                          /* num_components */
            GX_CINFO_POLARITY_ADDITIVE, /* polarity */
            32,                         /* depth */
            GX_CINFO_COMP_NO_INDEX,     /* gray index */
            255 ,                         /* max_gray */
            255 ,                         /* max_colors */
            256 ,                         /* dither grays */
            256 ,                         /* dither colors */
            { 1, 1 } ,                  /* antialiasing */
            GX_CINFO_UNKNOWN_SEP_LIN,   /* sep and linear */
            { 16, 8, 0, 0 } ,                    /* comp shift */
            { 8, 8, 8, 8 } ,                     /* comp bits */
            { 0xFF0000, 0x00FF00, 0x0000FF } ,     /* comp mask */
            ( "DeviceRGBT" ),            /* color model name */
            GX_CINFO_OPMODE_UNKNOWN ,   /* overprint mode */
            0,                           /* process comps */
            0                            /* icc_locations */
        },
        {
            ((gx_color_index)(~0)),
            ((gx_color_index)(~0))
        },
        (int)((float)(85) * (X_DPI) / 10 + 0.5),
        (int)((float)(110) * (Y_DPI) / 10 + 0.5),
        0, /* Pad */
        0, /* Align */
        0, /* Num planes */
        0,
        {
            (float)(((((int)((float)(85) * (X_DPI) / 10 + 0.5)) * 72.0 + 0.5) - 0.5) / (X_DPI)) ,
            (float)(((((int)((float)(110) * (Y_DPI) / 10 + 0.5)) * 72.0 + 0.5) - 0.5) / (Y_DPI)) },
        {
            0,
            0,
            0,
            0
        } ,
        0 ,
        { X_DPI, Y_DPI } ,
        { X_DPI, Y_DPI },
        {(float)(-(0) * (X_DPI)),
         (float)(-(0) * (Y_DPI))},
        {(float)((0) * 72.0),
         (float)((0) * 72.0),
         (float)((0) * 72.0),
         (float)((0) * 72.0)},
        0 , /*FirstPage*/
        0 , /*LastPage*/
        0 , /*PageHandlerPushed*/
        0 , /*DisablePageHandler*/
        0 , /*ObjectFilter*/
        0 , /*ObjectHandlerPushed*/
        0 , /*PageCount*/
        0 , /*ShowPageCount*/
        1 , /*NumCopies*/
        0 , /*NumCopiesSet*/
        0 , /*IgnoreNumCopies*/
        0 , /*UseCIEColor*/
        0 , /*LockSafetyParams*/
        0,  /*band_offset_x*/
        0,  /*band_offset_*/
        false, /*BLS_force_memory*/
        {false}, /*sgr*/
        0, /*MaxPatternBitmap*/
        0, /*page_uses_transparency*/
        { MAX_BITMAP, BUFFER_SPACE,
          { BAND_PARAMS_INITIAL_VALUES },
          0/*false*/, /* params_are_read_only */
          BandingAuto /* banding_type */
        }, /*space_params*/
        0, /*icc_struct*/
        GS_UNKNOWN_TAG,         /* this device supports tags */
        {
            gx_default_install,
            gx_default_begin_page,
            gx_default_end_page
        },
        { 0 },
        { 0 },
        { bittags_print_page,
          gx_default_print_page_copies,
          { gx_default_create_buf_device,
            gx_default_size_buf_device,
            gx_default_setup_buf_device,
            gx_default_destroy_buf_device },
          gx_default_get_space_params },
        { 0 },
        0 ,
        0 ,
        0 ,
        -1,
        0 ,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0 ,
        0,
        0,
        0
    };

static void
cmyk_cs_to_rgb_cm(gx_device * dev, frac c, frac m, frac y, frac k, frac out[])
{
    color_cmyk_to_rgb(c, m, y, k, NULL, out, dev->memory);
}

static void
private_rgb_cs_to_rgb_cm(gx_device * dev, const gs_gstate *pgs,
                                  frac r, frac g, frac b, frac out[])
{
    out[0] = r;
    out[1] = g;
    out[2] = b;
}

static void
gray_cs_to_rgb_cm(gx_device * dev, frac gray, frac out[])
{
    out[0] = out[1] = out[2] = gray;
}

static const gx_cm_color_map_procs bittag_DeviceRGB_procs = {
    gray_cs_to_rgb_cm, private_rgb_cs_to_rgb_cm, cmyk_cs_to_rgb_cm
};

static const gx_cm_color_map_procs *
bittag_get_color_mapping_procs(const gx_device *dev)
{
    return &bittag_DeviceRGB_procs;
}

static gx_color_index
bittag_rgb_map_rgb_color(gx_device * dev, const gx_color_value cv[])
{
    return
        gx_color_value_to_byte(cv[2]) +
        (((uint) gx_color_value_to_byte(cv[1])) << 8) +
        (((ulong) gx_color_value_to_byte(cv[0])) << 16) +
        ((ulong)(dev->graphics_type_tag & ~GS_DEVICE_ENCODES_TAGS) << 24);
}

static int
bittag_map_color_rgb(gx_device * dev, gx_color_index color, gx_color_value cv[4])
{
    int depth = 24;
    int ncomp = 3;
    int bpc = depth / ncomp;
    uint mask = (1 << bpc) - 1;

#define cvalue(c) ((gx_color_value)((ulong)(c) * gx_max_color_value / mask))

    gx_color_index cshift = color;
    cv[2] = cvalue(cshift & mask);
    cshift >>= bpc;
    cv[1] = cvalue(cshift & mask);
    cshift >>= bpc;
    cv[0] = cvalue(cshift & mask);
    return 0;
#undef cvalue
}

/* Map gray to color. */
/* Note that 1-bit monochrome is a special case. */
static gx_color_index
bit_mono_map_color(gx_device * dev, const gx_color_value cv[])
{
    int bpc = dev->color_info.depth;
    int drop = sizeof(gx_color_value) * 8 - bpc;
    gx_color_value gray = cv[0];

    return (bpc == 1 ? gx_max_color_value - gray : gray) >> drop;
}

#if 0 /* unused */
/* Map RGB to gray shade. */
/* Only used in CMYK mode when put_params has set ForceMono=1 */
static gx_color_index
bit_forcemono_map_rgb_color(gx_device * dev, const gx_color_value cv[])
{
    gx_color_value color;
    int bpc = dev->color_info.depth / 4;	/* This function is used in CMYK mode */
    int drop = sizeof(gx_color_value) * 8 - bpc;
    gx_color_value gray, red, green, blue;
    red = cv[0]; green = cv[1]; blue = cv[2];
    gray = red;
    if ((red != green) || (green != blue))
        gray = (red * (unsigned long)lum_red_weight +
             green * (unsigned long)lum_green_weight +
             blue * (unsigned long)lum_blue_weight +
             (lum_all_weights / 2))
                / lum_all_weights;

    color = (gx_max_color_value - gray) >> drop;	/* color is in K channel */
    return color;
}
#endif

gx_color_index
bitrgb_rgb_map_rgb_color(gx_device * dev, const gx_color_value cv[])
{
    if (dev->color_info.depth == 24)
        return gx_color_value_to_byte(cv[2]) +
            ((uint) gx_color_value_to_byte(cv[1]) << 8) +
            ((ulong) gx_color_value_to_byte(cv[0]) << 16);
    else {
        COLROUND_VARS;
        /* The following needs special handling to avoid bpc=5 when depth=16 */
        int bpc = dev->color_info.depth == 16 ? 4 : dev->color_info.depth / 3;
        COLROUND_SETUP(bpc);

        return (((COLROUND_ROUND(cv[0]) << bpc) +
                 COLROUND_ROUND(cv[1])) << bpc) +
               COLROUND_ROUND(cv[2]);
    }
}

/* Map color to RGB.  This has 3 separate cases, but since it is rarely */
/* used, we do a case test rather than providing 3 separate routines. */
static int
bit_map_color_rgb(gx_device * dev, gx_color_index color, gx_color_value cv[4])
{
    int depth = dev->color_info.depth;
    int ncomp = REAL_NUM_COMPONENTS(dev);
    int bpc = depth / ncomp;
    uint mask = (1 << bpc) - 1;

#define cvalue(c) ((gx_color_value)((ulong)(c) * gx_max_color_value / mask))

    switch (ncomp) {
        case 1:		/* gray */
            cv[0] = cv[1] = cv[2] =
                (depth == 1 ? (color ? 0 : gx_max_color_value) :
                 cvalue(color));
            break;
        case 3:		/* RGB */
            {
                gx_color_index cshift = color;

                cv[2] = cvalue(cshift & mask);
                cshift >>= bpc;
                cv[1] = cvalue(cshift & mask);
                cv[0] = cvalue(cshift >> bpc);
            }
            break;
        case 4:		/* CMYK */
            /* Map CMYK back to RGB. */
            {
                gx_color_index cshift = color;
                uint c, m, y, k;

                k = cshift & mask;
                cshift >>= bpc;
                y = cshift & mask;
                cshift >>= bpc;
                m = cshift & mask;
                c = cshift >> bpc;
                /* We use our improved conversion rule.... */
                cv[0] = cvalue((mask - c) * (mask - k) / mask);
                cv[1] = cvalue((mask - m) * (mask - k) / mask);
                cv[2] = cvalue((mask - y) * (mask - k) / mask);
            }
            break;
    }
    return 0;
#undef cvalue
}

/* Map CMYK to color. */
static gx_color_index
bit_map_cmyk_color(gx_device * dev, const gx_color_value cv[])
{
    int bpc = dev->color_info.depth / 4;
    int drop = sizeof(gx_color_value) * 8 - bpc;
    gx_color_index color =
    (((((((gx_color_index) cv[0] >> drop) << bpc) +
        (cv[1] >> drop)) << bpc) +
      (cv[2] >> drop)) << bpc) +
    (cv[3] >> drop);

    return (color == gx_no_color_index ? color ^ 1 : color);
}

static int
bittag_put_params(gx_device * pdev, gs_param_list * plist)
{
    pdev->graphics_type_tag |= GS_DEVICE_ENCODES_TAGS;		/* the bittags devices use tags in the color */
    return gdev_prn_put_params(pdev, plist);
}
/* Get parameters.  We provide a default CRD. */
static int
bit_get_params(gx_device * pdev, gs_param_list * plist)
{
    int code, ecode;
    /*
     * The following is a hack to get the original num_components.
     * See comment above.
     */
    int real_ncomps = REAL_NUM_COMPONENTS(pdev);
    int ncomps = pdev->color_info.num_components;
    int forcemono = (ncomps == real_ncomps ? 0 : 1);

    /*
     * Temporarily set num_components back to the "real" value to avoid
     * confusing those that rely on it.
     */
    pdev->color_info.num_components = real_ncomps;

    ecode = gdev_prn_get_params(pdev, plist);
    code = sample_device_crd_get_params(pdev, plist, "CRDDefault");
    if (code < 0)
            ecode = code;
    if ((code = param_write_int(plist, "ForceMono", &forcemono)) < 0) {
        ecode = code;
    }
    if ((code = param_write_int(plist, "FirstLine", &((gx_device_bit *)pdev)->FirstLine)) < 0) {
        ecode = code;
    }
    if ((code = param_write_int(plist, "LastLine", &((gx_device_bit *)pdev)->LastLine)) < 0) {
        ecode = code;
    }

    /* Restore the working num_components */
    pdev->color_info.num_components = ncomps;

    return ecode;
}

/* Set parameters.  We allow setting the number of bits per component. */
/* Also, ForceMono=1 forces monochrome output from RGB/CMYK devices. */
static int
bit_put_params(gx_device * pdev, gs_param_list * plist)
{
    gx_device_color_info save_info;
    int ncomps = pdev->color_info.num_components;
    int real_ncomps = REAL_NUM_COMPONENTS(pdev);
    int v;
    int ecode = 0;
    int code;
    /* map to depths that we actually have memory devices to support */
    static const byte depths[4 /* ncomps - 1 */][16 /* bpc - 1 */] = {
        {1, 2, 0, 4, 8, 0, 0, 8, 0, 0, 0, 16, 0, 0, 0, 16}, /* ncomps = 1 */
        {0}, /* ncomps = 2, not supported */
        {4, 8, 0, 16, 16, 0, 0, 24, 0, 0, 0, 40, 0, 0, 0, 48}, /* ncomps = 3 (rgb) */
        {4, 8, 0, 16, 32, 0, 0, 32, 0, 0, 0, 48, 0, 0, 0, 64}  /* ncomps = 4 (cmyk) */
    };
    /* map back from depth to the actual bits per component */
    static int real_bpc[17] = { 0, 1, 2, 2, 4, 4, 4, 4, 8, 8, 8, 8, 12, 12, 12, 12, 16 };
    int bpc = real_bpc[pdev->color_info.depth / real_ncomps];
    const char *vname;
    int FirstLine = ((gx_device_bit *)pdev)->FirstLine;
    int LastLine = ((gx_device_bit *)pdev)->LastLine;

    /*
     * Temporarily set num_components back to the "real" value to avoid
     * confusing those that rely on it.
     */
    pdev->color_info.num_components = real_ncomps;

    if ((code = param_read_int(plist, (vname = "GrayValues"), &v)) != 1 ||
        (code = param_read_int(plist, (vname = "RedValues"), &v)) != 1 ||
        (code = param_read_int(plist, (vname = "GreenValues"), &v)) != 1 ||
        (code = param_read_int(plist, (vname = "BlueValues"), &v)) != 1
        ) {
        if (code < 0)
            ecode = code;
        else
            switch (v) {
                case   2: bpc = 1; break;
                case   4: bpc = 2; break;
                case  16: bpc = 4; break;
                case 256: bpc = 8; break;
                case 4096: bpc = 12; break;
                case 65536: bpc = 16; break;
                default:
                    param_signal_error(plist, vname,
                                       ecode = gs_error_rangecheck);
            }
    }

    switch (code = param_read_int(plist, (vname = "ForceMono"), &v)) {
    case 0:
        if (v == 1) {
            ncomps = 1;
            break;
        }
        else if (v == 0) {
            ncomps = real_ncomps;
            break;
        }
        code = gs_error_rangecheck;
    default:
        ecode = code;
        param_signal_error(plist, vname, ecode);
    case 1:
        break;
    }
    if (ecode < 0)
        return ecode;
    switch (code = param_read_int(plist, (vname = "FirstLine"), &v)) {
    case 0:
        FirstLine = v;
        break;
    default:
        ecode = code;
        param_signal_error(plist, vname, ecode);
    case 1:
        break;
    }
    if (ecode < 0)
        return ecode;

    switch (code = param_read_int(plist, (vname = "LastLine"), &v)) {
    case 0:
        LastLine = v;
        break;
    default:
        ecode = code;
        param_signal_error(plist, vname, ecode);
    case 1:
        break;
    }
    if (ecode < 0)
        return ecode;

    /*
     * Save the color_info in case gdev_prn_put_params fails, and for
     * comparison.  Note that depth is computed from real_ncomps.
     */
    save_info = pdev->color_info;
    pdev->color_info.depth = depths[real_ncomps - 1][bpc - 1];
    pdev->color_info.max_gray = pdev->color_info.max_color =
        (pdev->color_info.dither_grays =
         pdev->color_info.dither_colors =
         (1 << bpc)) - 1;
    ecode = gdev_prn_put_params(pdev, plist);
    if (ecode < 0) {
        pdev->color_info = save_info;
        return ecode;
    }
    /* Now restore/change num_components. This is done after other	*/
    /* processing since it is used in gx_default_put_params		*/
    pdev->color_info.num_components = ncomps;
    if (pdev->color_info.depth != save_info.depth ||
        pdev->color_info.num_components != save_info.num_components
        ) {
        gs_closedevice(pdev);
    }
    /* Reset the map_cmyk_color procedure if appropriate. */
    if (dev_proc(pdev, map_cmyk_color) == cmyk_1bit_map_cmyk_color ||
        dev_proc(pdev, map_cmyk_color) == cmyk_8bit_map_cmyk_color ||
        dev_proc(pdev, map_cmyk_color) == bit_map_cmyk_color) {
        set_dev_proc(pdev, map_cmyk_color,
                     pdev->color_info.depth == 4 ? cmyk_1bit_map_cmyk_color :
                     pdev->color_info.depth == 32 ? cmyk_8bit_map_cmyk_color :
                     bit_map_cmyk_color);
    }
    /* Reset the separable and linear shift, masks, bits. */
    set_linear_color_bits_mask_shift(pdev);
    pdev->color_info.separable_and_linear = GX_CINFO_SEP_LIN;
    ((gx_device_bit *)pdev)->FirstLine = FirstLine;
    ((gx_device_bit *)pdev)->LastLine = LastLine;

    return 0;
}

/* Send the page to the printer. */
static int
bit_print_page(gx_device_printer * pdev, FILE * prn_stream)
{				/* Just dump the bits on the file. */
    /* If the file is 'nul', don't even do the writes. */
    int line_size = gdev_mem_bytes_per_scan_line((gx_device *) pdev);
    byte *in = gs_alloc_bytes(pdev->memory, line_size, "bit_print_page(in)");
    byte *data;
    int nul = !strcmp(pdev->fname, "nul") || !strcmp(pdev->fname, "/dev/null");
    int lnum = ((gx_device_bit *)pdev)->FirstLine >= pdev->height ?  pdev->height - 1 :
                 ((gx_device_bit *)pdev)->FirstLine;
    int bottom = ((gx_device_bit *)pdev)->LastLine >= pdev->height ?  pdev->height - 1 :
                 ((gx_device_bit *)pdev)->LastLine;
    int line_count = any_abs(bottom - lnum);
    int i, step = lnum > bottom ? -1 : 1;

    if (in == 0)
        return_error(gs_error_VMerror);
    if ((lnum == 0) && (bottom == 0))
        line_count = pdev->height - 1;		/* default when LastLine == 0, FirstLine == 0 */
    for (i = 0; i <= line_count; i++, lnum += step) {
        gdev_prn_get_bits(pdev, lnum, in, &data);
        if (!nul)
            fwrite(data, 1, line_size, prn_stream);
    }
    gs_free_object(pdev->memory, in, "bit_print_page(in)");
    return 0;
}

/* For tags device go ahead and add in the size so that we can strip and create
   proper ppm outputs for various dimensions and not be restricted to 72dpi when
   using the tag viewer */
static int
bittags_print_page(gx_device_printer * pdev, FILE * prn_stream)
{				/* Just dump the bits on the file. */
    /* If the file is 'nul', don't even do the writes. */
    int line_size = gdev_mem_bytes_per_scan_line((gx_device *) pdev);
    byte *in = gs_alloc_bytes(pdev->memory, line_size, "bit_print_page(in)");
    byte *data;
    int nul = !strcmp(pdev->fname, "nul") || !strcmp(pdev->fname, "/dev/null");
    int lnum = ((gx_device_bit *)pdev)->FirstLine >= pdev->height ?  pdev->height - 1 :
                 ((gx_device_bit *)pdev)->FirstLine;
    int bottom = ((gx_device_bit *)pdev)->LastLine >= pdev->height ?  pdev->height - 1 :
                 ((gx_device_bit *)pdev)->LastLine;
    int line_count = any_abs(bottom - lnum);
    int i, step = lnum > bottom ? -1 : 1;

    if (in == 0)
        return_error(gs_error_VMerror);

    fprintf(prn_stream, "P6\n%d %d\n255\n", pdev->width, pdev->height);
    if ((lnum == 0) && (bottom == 0))
        line_count = pdev->height - 1;		/* default when LastLine == 0, FirstLine == 0 */
    for (i = 0; i <= line_count; i++, lnum += step) {
        gdev_prn_get_bits(pdev, lnum, in, &data);
        if (!nul)
            fwrite(data, 1, line_size, prn_stream);
    }
    gs_free_object(pdev->memory, in, "bit_print_page(in)");
    return 0;
}

static int
bit_put_image(gx_device *pdev, const byte *buffer, int num_chan, int xstart,
              int ystart, int width, int height, int row_stride,
              int plane_stride, int alpha_plane_index, int tag_plane_index)
{
    gx_device_memory *pmemdev = (gx_device_memory *)pdev;
    byte *buffer_prn;
    int yend = ystart + height;
    int xend = xstart + width;
    int x, y, k;
    int src_position, des_position;

    if (alpha_plane_index != 0)
        return 0;                /* we don't want alpha, return 0 to ask for the    */
                                 /* pdf14 device to do the alpha composition        */
    /* Eventually, the pdf14 device might be chunky pixels, punt for now */
    if (plane_stride == 0)
        return 0;
    if (num_chan != 3 || tag_plane_index <= 0)
            return_error(gs_error_unknownerror);        /* can't handle these cases */
    /* Drill down to get the appropriate memory buffer pointer */
    buffer_prn = pmemdev->base;
    /* Now go ahead and fill */
    for ( y = ystart; y < yend; y++ ) {
        src_position = (y - ystart) * row_stride;
        des_position = y * pmemdev->raster + xstart * 4;
        for ( x = xstart; x < xend; x++ ) {
            /* Tag data first, then RGB */
            buffer_prn[des_position] =
                buffer[src_position + tag_plane_index * plane_stride];
                des_position += 1;
            for ( k = 0; k < 3; k++) {
                buffer_prn[des_position] =
                    buffer[src_position + k * plane_stride];
                    des_position += 1;
            }
            src_position += 1;
        }
    }
    return height;        /* we used all of the data */
}
