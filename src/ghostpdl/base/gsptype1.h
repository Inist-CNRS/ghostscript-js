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


/* Client interface to PatternType 1 Patterns */

#ifndef gsptype1_INCLUDED
#  define gsptype1_INCLUDED

#include "gspcolor.h"
#include "gxbitmap.h"

#ifndef gx_device_color_DEFINED
#  define gx_device_color_DEFINED
typedef struct gx_device_color_s gx_device_color;
#endif
#ifndef gs_color_space_DEFINED
#  define gs_color_space_DEFINED
typedef struct gs_color_space_s gs_color_space;
#endif

/* ---------------- Types and structures ---------------- */

/* PatternType 1 template */

typedef struct gs_pattern1_template_s {
    /*
     * The common template must come first.  It defines type, uid,
     * PatternType, and client_data.
     */
    gs_pattern_template_common;
    int PaintType;
    int TilingType;
    bool uses_transparency;
    gs_rect BBox;
    float XStep;
    float YStep;
    int (*PaintProc) (const gs_client_color *, gs_gstate *);
} gs_pattern1_template_t;

#define private_st_pattern1_template() /* in gspcolor.c */\
  gs_private_st_suffix_add0(st_pattern1_template,\
    gs_pattern1_template_t, "gs_pattern1_template_t",\
    pattern1_template_enum_ptrs, pattern1_template_reloc_ptrs,\
    st_pattern_template)
#define st_pattern1_template_max_ptrs st_pattern_template_max_ptrs

/* Backward compatibility */
typedef gs_pattern1_template_t gs_client_pattern;

/* ---------------- Procedures ---------------- */

/*
 * Construct a PatternType 1 Pattern color space.  If the base space is
 * NULL, the color space can only be used with colored patterns.
 */
extern int gs_cspace_build_Pattern1(
                                    gs_color_space ** ppcspace,
                                    gs_color_space * pbase_cspace,
                                    gs_memory_t * pmem
                                    );

/* Initialize a PatternType 1 pattern. */
void gs_pattern1_init(gs_pattern1_template_t *);

/* Backward compatibility */
#define gs_client_pattern_init(ppat) gs_pattern1_init(ppat)

/*
 * Define versions of make_pattern and get_pattern specifically for
 * PatternType 1 patterns.
 *
 * The gs_memory_t argument for gs_makepattern may be NULL, meaning use the
 * same allocator as for the gs_gstate argument.  Note that gs_makepattern
 * uses rc_alloc_struct_1 to allocate pattern instances.
 */
int gs_makepattern(gs_client_color *, const gs_client_pattern *,
                   const gs_matrix *, gs_gstate *, gs_memory_t *);
const gs_client_pattern *gs_getpattern(const gs_client_color *);

/* Check device color for Pattern Type 1. */
bool gx_dc_is_pattern1_color(const gx_device_color *pdevc);

/* Check device color for Pattern Type 1 with transparency involved */
bool gx_dc_is_pattern1_color_with_trans(const gx_device_color *pdevc);

/* Get transparency pointer */
void * gx_pattern1_get_transptr(const gx_device_color *pdevc);
/* pattern is clist with transparency */
int gx_pattern1_clist_has_trans(const gx_device_color *pdevc);

/* For changing the device color procs when we have a transparency situation */

void gx_set_pattern_procs_trans(gx_device_color *pdevc);
void gx_set_pattern_procs_standard(gx_device_color *pdevc);
bool gx_pattern_procs_istrans(gx_device_color *pdevc);

/* Check device color for clist-based Pattern Type 1. */
bool gx_dc_is_pattern1_color_clist_based(const gx_device_color *pdevc);

/* Get pattern id (type 1 pattern only) */
gs_id gs_dc_get_pattern_id(const gx_device_color *pdevc);

/*
 * Make a pattern from a bitmap or pixmap. The pattern may be colored or
 * uncolored, as determined by the mask operand. This code is intended
 * primarily for use by PCL.
 *
 * By convention, if pmat is null the identity matrix will be used, and if
 * id is no_UniqueID the code will assign a unique id. Thes conventions allow
 * gs_makebitmappattern to be implemented as a macro. Also, if mem is a
 * null pointer, the memory allocator for the graphic state is used.
 *
 * For mask patterns, pix_depth must be 1, while pcspace and white_index are
 * ignored; the polarity of the mask considers ones part of the mask, while
 * zeros are not. For colored patterns pspace must point to an indexed color
 * space and the image must used the canoncial Decode array for this color
 * space. For both cases no interpolation or adjustment is provided.
 *
 * For backwards compatibility, if mask is false, pcspace is null, and
 * pix_depth is 1, the pattern will be rendered with a color space that maps
 * 0 to white and 1 to black.
 *
 * The image must be described by a gx_tile_bitmap structure (this is actually
 * somewhat awkward, but the only option available at the moment), and the
 * pattern step will exactly match the image size. The client need not maintain
 * the gx_tile_bitmap structure after the completion of this call, but the
 * raw image data itself must be kept until the pattern is no longer needed.
 *
 * NB: For proper handling of transparency in PCL, there must be only a single
 *     white value accessed by the pattern image. If the palette contains
 *     multiple white values, the PCL component must remap the image data to
 *     ensure that all white indices are mapped to the single, given white
 *     index.
 */
extern int gs_makepixmappattern(
                                gs_client_color * pcc,
                                const gs_depth_bitmap * pbitmap,
                                bool mask,
                                const gs_matrix * pmat,
                                long id,
                                gs_color_space * pcspace,
                                uint white_index,
                                gs_gstate * pgs,
                                gs_memory_t * mem
                                );

/*
 *  Backwards compatibility feature, to allow the existing
 *  gs_makebitmappattern operation to still function.
 */
extern int gs_makebitmappattern_xform(
                                      gs_client_color * pcc,
                                      const gx_tile_bitmap * ptile,
                                      bool mask,
                                      const gs_matrix * pmat,
                                      long id,
                                      gs_gstate * pgs,
                                      gs_memory_t * mem
                                      );

/*
 * High level pattern support for pixmap patterns, if the interpreter supports them.
 */
extern int pixmap_high_level_pattern(gs_gstate * pgs);

#define gs_makebitmappattern(pcc, tile, mask, pgs, mem)                 \
    gs_makebitmappattern_xform(pcc, tile, mask, 0, no_UniqueID, pgs, mem)

#endif /* gsptype1_INCLUDED */
