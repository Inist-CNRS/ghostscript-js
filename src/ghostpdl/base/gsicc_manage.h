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


/*  Header for the ICC Manager */

#ifndef gsiccmanage_INCLUDED
#  define gsiccmanage_INCLUDED

#define ICC_DUMP 0

/* Define the default ICC profiles in the file system */
#define OI_PROFILE        "\xffOIProfile" /* Keyword to indicate use of OI profile */
#define DEFAULT_GRAY_ICC  "default_gray.icc"
#define DEFAULT_RGB_ICC   "default_rgb.icc"
#define DEFAULT_CMYK_ICC  "default_cmyk.icc"
#define LAB_ICC           "lab.icc"
#define SMASK_GRAY_ICC    "ps_gray.icc"
#define SMASK_RGB_ICC     "ps_rgb.icc"
#define SMASK_CMYK_ICC    "ps_cmyk.icc"
#define GRAY_TO_K         "gray_to_k.icc"
#define DEFAULT_DIR_ICC   "%rom%iccprofiles/"
#define MAX_DEFAULT_ICC_LENGTH 17

/* Key names for special common canned profiles.
   These are found in some image file formats as
   a magic number.   */

#define GSICC_STANDARD_PROFILES_KEYS\
  "srgb", "sgray"
#define GSICC_STANDARD_PROFILES\
  "srgb.icc", "sgray.icc"
#define GSICC_NUMBER_STANDARD_PROFILES 2
/* This enumeration has to be in sync with GSICC_SRCGTAG_KEYS */
typedef enum {
    COLOR_TUNE,
    GRAPHIC_CMYK,
    IMAGE_CMYK,
    TEXT_CMYK,
    GRAPHIC_RGB,
    IMAGE_RGB,
    TEXT_RGB,
    GRAPHIC_GRAY,
    IMAGE_GRAY,
    TEXT_GRAY
} gsicc_srcgtagkey_t;

#define GSICC_SRCTAG_NOCM "None"
#define GSICC_SRCTAG_REPLACE "Replace"

#define GSICC_SRCGTAG_KEYS\
  "ColorTune", "Graphic_CMYK", "Image_CMYK", "Text_CMYK",\
  "Graphic_RGB", "Image_RGB", "Text_RGB",\
  "Graphic_GRAY", "Image_GRAY", "Text_GRAY"
#define GSICC_NUM_SRCGTAG_KEYS 10

#include "gsicc_cms.h"

/* Prototypes */
void gsicc_profile_reference(cmm_profile_t *icc_profile, int delta);
void gsicc_extract_profile(gs_graphics_type_tag_t graphics_type_tag,
                       cmm_dev_profile_t *profile_struct,
                       cmm_profile_t **profile, gsicc_rendering_param_t *render_cond);
void gsicc_get_srcprofile(gsicc_colorbuffer_t data_cs,
                     gs_graphics_type_tag_t graphics_type_tag,
                     cmm_srcgtag_profile_t *srcgtag_profile,
                     cmm_profile_t **profile, gsicc_rendering_param_t *render_cond);
int gsicc_getsrc_channel_count(cmm_profile_t *icc_profile);
int gsicc_init_iccmanager(gs_gstate * pgs);
int gsicc_init_gs_colors(gs_gstate *pgs);
void  gsicc_profile_serialize(gsicc_serialized_profile_t *profile_data,
                              cmm_profile_t *iccprofile);
int gsicc_set_device_profile_intent(gx_device *dev,
                                    gsicc_rendering_intents_t intent,
                                    gsicc_profile_types_t profile_type);
int gsicc_set_device_blackptcomp(gx_device *dev,
                                    gsicc_blackptcomp_t blackptcomp,
                                    gsicc_profile_types_t profile_type);
int gsicc_set_device_blackpreserve(gx_device *dev,
                                   gsicc_blackpreserve_t blackpreserve,
                                   gsicc_profile_types_t profile_type);
int gsicc_set_devicen_equiv_colors(gx_device *dev, const gs_gstate * pgs,
                                    cmm_profile_t *profile);
int gsicc_set_device_profile_colorants(gx_device *dev, char *name_str);
int gsicc_init_device_profile_struct(gx_device * dev,  char *profile_name,
                                     gsicc_profile_types_t profile_type);
int gsicc_set_profile(gsicc_manager_t *icc_manager, const char *pname,
                      int namelen, gsicc_profile_t defaulttype);
int gsicc_set_srcgtag_struct(gsicc_manager_t *icc_manager, const char* pname,
                            int namelen);
cmm_profile_t* gsicc_get_profile_handle_file(const char* pname, int namelen,
                                             gs_memory_t *mem);
int gsicc_init_profile_info(cmm_profile_t *profile);
int gsicc_initialize_default_profile(cmm_profile_t *icc_profile);
gsicc_manager_t* gsicc_manager_new(gs_memory_t *memory);
cmm_profile_t* gsicc_profile_new(stream *s, gs_memory_t *memory,
                                 const char* pname, int namelen);
int gsicc_clone_profile(cmm_profile_t *source, cmm_profile_t **destination,
                        gs_memory_t *memory);
int gsicc_set_gscs_profile(gs_color_space *pcs, cmm_profile_t *icc_profile,
                           gs_memory_t * mem);
cmm_profile_t* gsicc_get_gscs_profile(gs_color_space *gs_colorspace,
                                      gsicc_manager_t *icc_manager);
void gsicc_init_hash_cs(cmm_profile_t *picc_profile, gs_gstate *pgs);
gcmmhprofile_t gsicc_get_profile_handle_clist(cmm_profile_t *picc_profile,
                                              gs_memory_t *memory);
gcmmhprofile_t gsicc_get_profile_handle_buffer(unsigned char *buffer,
                                               int profile_size,
                                               gs_memory_t *memory);
gsicc_smask_t* gsicc_new_iccsmask(gs_memory_t *memory);
int gsicc_initialize_iccsmask(gsicc_manager_t *icc_manager);
unsigned int gsicc_getprofilesize(unsigned char *buffer);
void gscms_set_icc_range(cmm_profile_t **icc_profile);
cmm_profile_t* gsicc_read_serial_icc(gx_device * dev, int64_t icc_hashcode);
cmm_profile_t* gsicc_finddevicen(const gs_color_space *pcs,
                                 gsicc_manager_t *icc_manager);
gs_color_space_index gsicc_get_default_type(cmm_profile_t *profile_data);
bool gsicc_profile_from_ps(cmm_profile_t *profile_data);
cmm_dev_profile_t* gsicc_new_device_profile_array(gs_memory_t *memory);
void gs_setoverrideicc(gs_gstate *pgs, bool value);
bool gs_currentoverrideicc(const gs_gstate *pgs);
cmm_profile_t* gsicc_set_iccsmaskprofile(const char *pname, int namelen,
                                         gsicc_manager_t *icc_manager,
                                         gs_memory_t *mem);
int gsicc_set_device_profile(gx_device * pdev, gs_memory_t * mem,
                             char *file_name, gsicc_profile_types_t defaulttype);
char* gsicc_get_dev_icccolorants(cmm_dev_profile_t *dev_profile);
void gsicc_setrange_lab(cmm_profile_t *profile);
/* system and user params */
void gs_currentdevicenicc(const gs_gstate * pgs, gs_param_string * pval);
int gs_setdevicenprofileicc(const gs_gstate * pgs, gs_param_string * pval);
void gs_currentdefaultgrayicc(const gs_gstate * pgs, gs_param_string * pval);
int gs_setdefaultgrayicc(const gs_gstate * pgs, gs_param_string * pval);
void gs_currenticcdirectory(const gs_gstate * pgs, gs_param_string * pval);
int gs_seticcdirectory(const gs_gstate * pgs, gs_param_string * pval);
void gs_currentsrcgtagicc(const gs_gstate * pgs, gs_param_string * pval);
int gs_setsrcgtagicc(const gs_gstate * pgs, gs_param_string * pval);
void gs_currentdefaultrgbicc(const gs_gstate * pgs, gs_param_string * pval);
int gs_setdefaultrgbicc(const gs_gstate * pgs, gs_param_string * pval);
void gs_currentnamedicc(const gs_gstate * pgs, gs_param_string * pval);
int gs_setnamedprofileicc(const gs_gstate * pgs, gs_param_string * pval);
void gs_currentdefaultcmykicc(const gs_gstate * pgs, gs_param_string * pval);
int gs_setdefaultcmykicc(const gs_gstate * pgs, gs_param_string * pval);
void gs_currentlabicc(const gs_gstate * pgs, gs_param_string * pval);
int gs_setlabicc(const gs_gstate * pgs, gs_param_string * pval);
int gsicc_get_device_class(cmm_profile_t *icc_profile);

#if ICC_DUMP
static void dump_icc_buffer(int buffersize, char filename[],byte *Buffer);
#endif
#endif
