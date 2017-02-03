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


/* Transparency definitions and interface */

#ifndef gstrans_INCLUDED
#  define gstrans_INCLUDED

#include "gstparam.h"
#include "gxcomp.h"
#include "gsmatrix.h"
#include "gxblend.h"

/*
 * Define the operations for the PDF 1.4 transparency compositor.
 */
typedef enum {
    PDF14_PUSH_DEVICE,
    PDF14_POP_DEVICE,
    PDF14_ABORT_DEVICE,
    PDF14_BEGIN_TRANS_GROUP,
    PDF14_END_TRANS_GROUP,
    PDF14_BEGIN_TRANS_MASK,
    PDF14_END_TRANS_MASK,
    PDF14_SET_BLEND_PARAMS,
    PDF14_PUSH_TRANS_STATE,
    PDF14_POP_TRANS_STATE,
    PDF14_PUSH_SMASK_COLOR,
    PDF14_POP_SMASK_COLOR
} pdf14_compositor_operations;

#define PDF14_OPCODE_NAMES \
{\
    "PDF14_PUSH_DEVICE      ",\
    "PDF14_POP_DEVICE       ",\
    "PDF14_ABORT_DEVICE     ",\
    "PDF14_BEGIN_TRANS_GROUP",\
    "PDF14_END_TRANS_GROUP  ",\
    "PDF14_BEGIN_TRANS_MASK ",\
    "PDF14_END_TRANS_MASK   ",\
    "PDF14_SET_BLEND_PARAMS ",\
    "PDF14_PUSH_TRANS_STATE ",\
    "PDF14_POP_TRANS_STATE  ",\
    "PDF14_PUSH_SMASK_COLOR ",\
    "PDF14_POP_SMASK_COLOR  "\
}

/* Bit definitions for serializing PDF 1.4 parameters */
#define PDF14_SET_BLEND_MODE    (1 << 0)
#define PDF14_SET_TEXT_KNOCKOUT (1 << 1)
#define PDF14_SET_SHAPE_ALPHA   (1 << 2)
#define PDF14_SET_OPACITY_ALPHA (1 << 3)
#define PDF14_SET_OVERPRINT		(1 << 4)
#define PDF14_SET_OVERPRINT_MODE (1 << 5)
#define PDF14_SET_OVERPRINT_BLEND (1 << 6)

#ifndef gs_function_DEFINED
typedef struct gs_function_s gs_function_t;
#  define gs_function_DEFINED
#endif

typedef struct gs_transparency_source_s {
    float alpha;		/* constant alpha */
} gs_transparency_source_t;

struct gs_pdf14trans_params_s {
    /* The type of trasnparency operation */
    pdf14_compositor_operations pdf14_op;
    int num_spot_colors;    /* Only for devices which support spot colors. */
    /* Changed parameters flag */
    int changed;
    /* Parameters from the gs_transparency_group_params_t structure */
    bool Isolated;
    bool Knockout;
    bool image_with_SMask;
    gs_rect bbox;
    /*The transparency channel selector */
    gs_transparency_channel_selector_t csel;
    /* Parameters from the gx_transparency_mask_params_t structure */
    gs_transparency_mask_subtype_t subtype;
    bool function_is_identity;
    int Background_components;
    float Background[GS_CLIENT_COLOR_MAX_COMPONENTS];
    int Matte_components;
    float Matte[GS_CLIENT_COLOR_MAX_COMPONENTS];
    float GrayBackground;  /* This is used to determine if the
                              softmask's bbox needs to be adjusted
                              to the parent groups bbox.  Since
                              the soft mask can affect areas
                              outside its own groups bounding
                              box in such a case */
    gs_function_t *transfer_function;
    byte transfer_fn[MASK_TRANSFER_FUNCTION_SIZE];
    /* Individual transparency parameters */
    gs_blend_mode_t blend_mode;
    bool text_knockout;
    gs_transparency_source_t opacity;
    gs_transparency_source_t shape;
    bool mask_is_image;
    gs_matrix ctm;
    bool replacing;
    bool overprint;
    bool overprint_mode;
    bool blendspot;
    bool idle; /* For clist reader.*/
    uint mask_id; /* For clist reader.*/
    int group_color_numcomps;
    gs_transparency_color_t group_color;
    int64_t icc_hash;
    cmm_profile_t *iccprofile;               /* The profile  */
    bool crop_blend_params;  /* This is used when the blend params are updated
                                during a transparency group push */
    bool is_pattern;      /* Needed to detect device push and pop for clist pattern */
};

#ifndef gs_pdf14trans_params_DEFINED
#define gs_pdf14trans_params_DEFINED
typedef struct gs_pdf14trans_params_s gs_pdf14trans_params_t;
#endif

/*
 * The PDF 1.4 transparency compositor structure. This is exactly analogous to
 * other compositor structures, consisting of a the compositor common elements
 * and the PDF 1.4 transparency specific parameters.
 */
typedef struct gs_pdf14trans_s {
    gs_composite_common;
    gs_pdf14trans_params_t  params;
} gs_pdf14trans_t;

/* Access transparency-related graphics state elements. */
int gs_setblendmode(gs_gstate *, gs_blend_mode_t);
gs_blend_mode_t gs_currentblendmode(const gs_gstate *);
int gs_setopacityalpha(gs_gstate *, double);
float gs_currentopacityalpha(const gs_gstate *);
int gs_setshapealpha(gs_gstate *, double);
float gs_currentshapealpha(const gs_gstate *);
int gs_settextknockout(gs_gstate *, bool);
bool gs_currenttextknockout(const gs_gstate *);

/*
 * We have to abbreviate the procedure name because procedure names are
 * only unique to 23 characters on VMS.
 */
int gs_push_pdf14trans_device(gs_gstate * pgs, bool is_pattern);

int gs_pop_pdf14trans_device(gs_gstate * pgs, bool is_pattern);

int gs_abort_pdf14trans_device(gs_gstate * pgs);

void gs_trans_group_params_init(gs_transparency_group_params_t *ptgp);

int gs_update_trans_marking_params(gs_gstate * pgs);

int gs_begin_transparency_group(gs_gstate * pgs,
                                const gs_transparency_group_params_t *ptgp,
                                const gs_rect *pbbox);

int gs_end_transparency_group(gs_gstate *pgs);

void gs_trans_mask_params_init(gs_transparency_mask_params_t *ptmp,
                               gs_transparency_mask_subtype_t subtype);

int gs_begin_transparency_mask(gs_gstate *pgs,
                               const gs_transparency_mask_params_t *ptmp,
                               const gs_rect *pbbox, bool mask_is_image);

int gs_end_transparency_mask(gs_gstate *pgs,
                             gs_transparency_channel_selector_t csel);

/*
 * Imager level routines for the PDF 1.4 transparency operations.
 */
int gx_begin_transparency_group(gs_gstate * pgs, gx_device * pdev,
                                const gs_pdf14trans_params_t * pparams);

int gx_end_transparency_group(gs_gstate * pgs, gx_device * pdev);

int gx_begin_transparency_mask(gs_gstate * pgs, gx_device * pdev,
                                const gs_pdf14trans_params_t * pparams);

int gx_end_transparency_mask(gs_gstate * pgs, gx_device * pdev,
                                const gs_pdf14trans_params_t * pparams);

int gx_abort_trans_device(gs_gstate * pgs, gx_device * pdev);


/* These are used for watching for q Smask Q events.  We need to
   send special compositor commands to keep the bands in sync
   with the current softmask during clist rendering.  Like the
   other transparency operations the gs functions occur on the
   clist writer side and the gx functions occur on the
   clist reader side */

int gs_push_transparency_state(gs_gstate *pgs);

int gs_pop_transparency_state(gs_gstate *pgs, bool force);

int gx_push_transparency_state(gs_gstate * pgs, gx_device * pdev);

int gx_pop_transparency_state(gs_gstate * pgs, gx_device * pdev);

/*
 * Verify that a compositor data structure is for the PDF 1.4 compositor.
 */
int gs_is_pdf14trans_compositor(const gs_composite_t * pct);

/*
 * Estimate the amount of space that will be required by the PDF 1.4
 * transparency buffers for doing the blending operations.  These buffers
 * use 8 bits per component plus one or two 8 bit alpha component values.
 * In theory there can be a large number of these buffers required.  However
 * we do not know the required number of buffers, the required numbe of
 * alpha chanels, or the number of components for the blending operations.
 * (This information is determined later as the data streams are parsed.)
 * For now we are simply assuming that we will have three buffers with five
 * eight bit values.  This is a hack but not too unreasonable.  However
 * since it is a hack, we may exceed our desired buffer space while
 * processing the file.
 */
#define NUM_PDF14_BUFFERS 3     /* totally a random guess */
#define NUM_ALPHA_CHANNELS 1    /* common, but doesn't include possible tag, shape or group alpha */
#define NUM_COLOR_CHANNELS 4    /* CMYK is most common 'worst case' */
#define BITS_PER_CHANNEL 8      /* currently pdf14 device is always 8-bit */
/* The estimated size of an individual PDF 1.4 buffer row (in bits) */
#define ESTIMATED_PDF14_ROW_SIZE(width, target_num_components) ((width) * BITS_PER_CHANNEL\
        * (NUM_ALPHA_CHANNELS + NUM_COLOR_CHANNELS))
/* The estimated size of one row in all PDF 1.4 buffers (in bits) */
#define ESTIMATED_PDF14_ROW_SPACE(width, target_num_components) \
        (NUM_PDF14_BUFFERS * ESTIMATED_PDF14_ROW_SIZE(width, target_num_components))

#endif /* gstrans_INCLUDED */
