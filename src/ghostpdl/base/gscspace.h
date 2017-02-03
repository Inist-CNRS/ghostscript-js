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


/* Client interface to color spaces */

#ifndef gscspace_INCLUDED
#  define gscspace_INCLUDED

#include "gsmemory.h"
#include "gsiparam.h"
#include "gsrefct.h"

/*
 * Previous versions had a complicated lifecycle discipline for
 * colorspaces. All that is now replaced with a simple allocation and
 * reference counting discipline. All colorspace objects are allocated
 * on the heap and reference counted. References to colorspace objects
 * are pointers. There is no stack allocation, inline allocation inside
 * structs, or copying of contents.
 */

/*
 * Here is the information associated with the various color space
 * structures.  Note that DevicePixel, DeviceN, and the ability to use
 * Separation or DeviceN spaces as the base space of an Indexed space
 * are LanguageLevel 3 additions.  Unfortunately, the terminology for the
 * different levels of generality is confusing and inconsistent for
 * historical reasons.
 *
 * For base spaces:
 *
 * Space        Space parameters                Color parameters
 * -----        ----------------                ----------------
 * DeviceGray   (none)                          1 real [0-1]
 * DeviceRGB    (none)                          3 reals [0-1]
 * DeviceCMYK   (none)                          4 reals [0-1]
 * DevicePixel  depth                           1 int [up to depth bits]
 * CIEBasedDEFG dictionary                      4 reals
 * CIEBasedDEF  dictionary                      3 reals
 * CIEBasedABC  dictionary                      3 reals
 * CIEBasedA    dictionary                      1 real
 *
 * For non-base direct spaces:
 *
 * Space        Space parameters                Color parameters
 * -----        ----------------                ----------------
 *
 * Separation   name, alt_space, tint_xform     1 real [0-1]
 * DeviceN      names, alt_space, tint_xform    N reals
 *
 * For non-direct paint spaces:
 *
 * Space        Space parameters                Color parameters
 * -----        ----------------                ----------------
 * Indexed      base_space, hival, lookup       1 int [0-hival]
 * ICCBased     dictionary, alt_space           1, 3, or 4 reals
 *
 * For non-paint spaces:
 *
 * Space        Space parameters                Color parameters
 * -----        ----------------                ----------------
 * Pattern      colored: (none)                 dictionary
 *              uncolored: base_space dictionary + base space params
 */

/* In moving to a pure ICC based color management every color space
   (or its base space) will be defined by an ICC profile.  All modern
   CMMs provide a handle to a profile structure when handed an
   ICC profile.  This handle will become a new member variable for
   ghostscripts color space structure.  When that profile is initialized
   depends upon the color space type.

   a) If the color space is defined to be ICC based, then the profile handle
      will be obtained during the installation of this space.

   b) If the color space is a defined as deviceRGB, CMYK or Gray color space, then
      the profile handle  will be populated with the handle for the ICC profile that
      is in the ICC manager for that color space type.

   c) If the color space is defined as one of the Postscript or PDF CIE (nonICC)
      color spaces, then the profile handle will only be obtained once a request is made
      to obtain a link that includes this color space.  At that point
      we will convert the nonICC colorspace to an ICC color space.

   d) DevicePixel color space appears to only be used by the image3x code
      for the softmask.  Really this should be a Gray color space that is
      never really linked as we do not want a pure alpha image to be
      colorimetrically managed.  Need to do a bit of code/history review to
      understand what this space is.  It may need to be specially managed.  It never
      is used in a linked transformation.

   e) DeviceN is related somewhat to the colorspaces in b) but a bit different.
      PDF does not provide the capability for definining DeviceN source colors
      with an ICC profile.  XPS and OpenXPS do, and it is likely that SVG will
      do so too.  Also, it may become possible in future PDF versions to define
      DeviceN source colors with and ICC profile.
      To support these cases when creating a deviceN color space, it
      will need to be possible to include the ICC information related to that
      color space.  When it is provided, the handle for that color space will
      be obtained from the CMM similar to the ICC spaces in a) .  Note that the
      output device can effectively be DeviceN, which is the process colorants.
      In this case, we will have the CMM handle for the profile in the ICC manager
      and that will be used for the device's color space.

   f) Indexed and Pattern.  These will not have an ICC CMM handle.  Their base
      space will however.

   g) Separation.  These can have an ICC CMM handle.  The exact capabilities
      depend upon the form of the Named color structure that the CMM is going
      to use.  If it is using a traditional ICC named color profile and the entry
      for the color name is there, then we will have a handle to the named
      color profile.  If the color is not there and the device does not have
      this color in its list of colors that it "understands", then we have
      to use the alternate tint transform.  At the extreme end, we could have
      a named color structure that includes for each named color a 1CLR ICC profile.
      Such a profile maps the tint values to CIELAB values.  In this case,
      the ICC CMM handle for the Separation space would be this profile.  Note that
      XPS has to have an ICC profile for a "Named" Color Space.

*/

/* Opaque types for a graphics state stuff */
#ifndef gs_gstate_DEFINED
#  define gs_gstate_DEFINED
typedef struct gs_gstate_s gs_gstate;
#endif

#ifndef gsicc_link_DEFINED
typedef struct gsicc_link_s gsicc_link_t;
#  define gsicc_link_DEFINED
#endif


/* Define ICC profile structure type */
#ifndef cmm_profile_DEFINED
#define cmm_profile_DEFINED
typedef struct cmm_profile_s cmm_profile_t;
#endif

/*
 * Define the abstract type for color space objects.
 */
#ifndef gs_color_space_DEFINED
#  define gs_color_space_DEFINED
typedef struct gs_color_space_s gs_color_space;
#endif

/*
 * Define color space type indices.  NOTE: PostScript code (gs_res.ps,
 * gs_ll3.ps) and the color space substitution code (gscssub.[hc] and its
 * clients) assumes values 0-2 for DeviceGray/RGB/CMYK respectively.
 */
typedef enum {

    /* Supported in all configurations */
    gs_color_space_index_DeviceGray = 0,
    gs_color_space_index_DeviceRGB,

    /* Supported in extended Level 1, and in Level 2 and above */
    gs_color_space_index_DeviceCMYK,

    /* Supported in LanguageLevel 3 only */
    gs_color_space_index_DevicePixel,
    gs_color_space_index_DeviceN,

    /* Supported in Level 2 and above only */
    /* DEC C truncates identifiers at 32 characters, so.... */
    gs_color_space_index_CIEDEFG,
    gs_color_space_index_CIEDEF,
    gs_color_space_index_CIEABC,
    gs_color_space_index_CIEA,
    gs_color_space_index_Separation,
    gs_color_space_index_Indexed,
    gs_color_space_index_Pattern,

    /* Supported in PDF 1.3 and later only */
    gs_color_space_index_ICC

} gs_color_space_index;

/* We define the names only for debugging printout. */
#define GS_COLOR_SPACE_TYPE_NAMES\
  "DeviceGray", "DeviceRGB", "DeviceCMYK", "DevicePixel", "DeviceN",\
  "CIEBasedDEFG", "CIEBasedDEF", "CIEBasedABC", "CIEBasedA",\
  "Separation", "Indexed", "Pattern", "ICCBased"

/* Define an abstract type for color space types (method structures). */
typedef struct gs_color_space_type_s gs_color_space_type;

/*
 * Parameters for "small" base color spaces. Of the small base color spaces,
 * only DevicePixel and CIE spaces have parameters: see gscie.h for the
 * structure definitions for CIE space parameters.
 *
 * It would be possible to save an allocation by placing the CIE
 * params inside the colorspace struct, rather than a pointer to a
 * separate struct, but we'll keep it the way it is for now.
 */
typedef struct gs_device_pixel_params_s {
    int depth;
} gs_device_pixel_params;
typedef struct gs_cie_a_s gs_cie_a;
typedef struct gs_cie_abc_s gs_cie_abc;
typedef struct gs_cie_def_s gs_cie_def;
typedef struct gs_cie_defg_s gs_cie_defg;

#ifndef gs_device_n_map_DEFINED
#  define gs_device_n_map_DEFINED
typedef struct gs_device_n_map_s gs_device_n_map;
#endif

#ifndef gs_device_n_attributes_DEFINED
#  define gs_device_n_attributes_DEFINED
typedef struct gs_device_n_attributes_s gs_device_n_attributes;
#endif

/*
 * Non-base direct color spaces: Separation and DeviceN.
 * These include a base alternative color space.
 */
typedef ulong gs_separation_name;	/* BOGUS */

/*
 * Define callback function for graphics library to ask
 * interpreter about character string representation of
 * component names.  This is used for comparison of component
 * names with similar objects like ProcessColorModel colorant
 * names.
 */
typedef int (gs_callback_func_get_colorname_string)
     (const gs_memory_t *mem, gs_separation_name colorname, unsigned char **ppstr, unsigned int *plen);

typedef enum { SEP_NONE, SEP_ALL, SEP_OTHER } separation_type;

typedef struct gs_separation_params_s {
    gs_separation_name sep_name;
    gs_device_n_map *map;
    separation_type sep_type;
    bool use_alt_cspace;
    gs_callback_func_get_colorname_string *get_colorname_string;
} gs_separation_params;

typedef struct gs_device_n_params_s {
    gs_separation_name *names;
    uint num_components;
    gs_device_n_map *map;
    gs_device_n_attributes *colorants;
    bool use_alt_cspace;
    gs_callback_func_get_colorname_string *get_colorname_string;
} gs_device_n_params;

/* Define an abstract type for the client color space data */
typedef struct client_color_space_data_s client_color_space_data_t;

/*
 * Non-direct paint space: Indexed space.
 *
 * Note that for indexed color spaces, hival is the highest support index,
 * which is one less than the number of entries in the palette (as defined
 * in PostScript).
 */

typedef struct gs_indexed_map_s gs_indexed_map;

typedef struct gs_indexed_params_s {
    int hival;			/* num_entries - 1 */
    int n_comps;
    union {
        gs_const_string table;	/* size is implicit */
        gs_indexed_map *map;
    } lookup;
    bool use_proc;		/* 0 = use table, 1 = use proc & map */
} gs_indexed_params;

/*
 * Pattern parameter set. This may contain an instances of a paintable
 * color space. The boolean indicates if this is the case.
 */
typedef struct gs_pattern_params_s {
    bool has_base_space; /* {csrc} can't we just NULL-check the base_space? */
} gs_pattern_params;

/* id's 1 through 4 are reserved for static colorspaces; thus, dynamically
   assigned id's must begin at 5. */
#define cs_DeviceGray_id 1
#define cs_DeviceRGB_id 3
#define cs_DeviceCMYK_id 4

/*
 * The colorspace object. For pattern and indexed colorspaces, the
 * base_space refers to the underlying colorspace. For separation,
 * deviceN, and icc colorspaces, base_space refers to the alternate
 * colorspace (referred to as alt_space in previous versions of the
 * code).
 */
struct gs_color_space_s {
    const gs_color_space_type *type;
    rc_header                  rc;
    gs_id                      id;
    gs_color_space             *base_space;
    gs_color_space             *icc_equivalent;
    client_color_space_data_t  *pclient_color_space_data;
    cmm_profile_t              *cmm_icc_profile_data;
    union {
        gs_device_pixel_params   pixel;
        gs_cie_defg *            defg;
        gs_cie_def *             def;
        gs_cie_abc *             abc;
        gs_cie_a *               a;
        gs_separation_params     separation;
        gs_device_n_params       device_n;
        gs_indexed_params        indexed;
        gs_pattern_params        pattern;

    } params;
};

                                        /*extern_st(st_color_space); *//* in gxcspace.h */
#define public_st_color_space()	/* in gscspace.c */  \
    gs_public_st_composite_final( st_color_space,         \
                                  gs_color_space,         \
                                  "gs_color_space",       \
                                  color_space_enum_ptrs,  \
                                  color_space_reloc_ptrs, \
                                  gs_cspace_final         \
                            )

/* ---------------- Procedures ---------------- */

/* Constructors for simple device color spaces. These return NULL on
   VMerror. */

gs_color_space *gs_cspace_new_DeviceGray(gs_memory_t *mem);
gs_color_space *gs_cspace_new_DeviceRGB(gs_memory_t *mem);
gs_color_space *gs_cspace_new_DeviceCMYK(gs_memory_t *mem);
gs_color_space *gs_cspace_new_ICC(gs_memory_t *pmem, gs_gstate * pgs, 
                                  int components);

/* ------ Accessors ------ */

/* Get the index of a color space. */
gs_color_space_index gs_color_space_get_index(const gs_color_space *);

/* Tell if the space is CIE or ICC based */
bool gs_color_space_is_CIE(const gs_color_space * pcs);
bool gs_color_space_is_ICC(const gs_color_space * pcs);
bool gs_color_space_is_PSCIE(const gs_color_space * pcs);
int gs_colorspace_set_icc_equivalent(gs_color_space *pcs, bool *islab,
                                     gs_memory_t *memory);

/* Get the number of components in a color space. */
int gs_color_space_num_components(const gs_color_space *);

/*
 * Test whether two color spaces are equal.  Note that this test is
 * conservative: if it returns true, the color spaces are definitely
 * equal, while if it returns false, they might still be equivalent.
 */
bool gs_color_space_equal(const gs_color_space *pcs1,
                          const gs_color_space *pcs2);

/* Restrict a color to its legal range. */
#ifndef gs_client_color_DEFINED
#  define gs_client_color_DEFINED
typedef struct gs_client_color_s gs_client_color;
#endif
void gs_color_space_restrict_color(gs_client_color *, const gs_color_space *);

/*
 * Get the base space of an Indexed or uncolored Pattern color space, or the
 * alternate space of a Separation or DeviceN space.  Return NULL if the
 * color space does not have a base/alternative color space.
 */
const gs_color_space *gs_cspace_base_space(const gs_color_space * pcspace);

/* Abstract the rc_increment and rc_decrement for color spaces so that we also rc_increment
   the ICC profile if there is one associated with the color space */

void rc_increment_cs(gs_color_space *pcs);

void rc_decrement_cs(gs_color_space *pcs, const char *cname);

void rc_decrement_only_cs(gs_color_space *pcs, const char *cname);

void cs_adjust_counts_icc(gs_gstate *pgs, int delta);

/* backwards compatibility */
#define gs_color_space_indexed_base_space(pcspace)\
    gs_cspace_base_space(pcspace)

#endif /* gscspace_INCLUDED */
