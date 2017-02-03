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


/* Internal definitions for CIE color implementation */
/* Requires gxcspace.h */

#ifndef gxcie_INCLUDED
#  define gxcie_INCLUDED

#include "gscie.h"
#include "gsnamecl.h"

/*
 * These color space implementation procedures are defined in gscie.c or
 * gsciemap.c, and referenced from the color space structures in gscscie.c.
 */
/*
 * We use CIExxx rather than CIEBasedxxx in some places because
 * gcc under VMS only retains 23 characters of procedure names,
 * and DEC C truncates all identifiers at 31 characters.
 */

/* Defined in gscie.c */

cs_proc_init_color(gx_init_CIE);
cs_proc_restrict_color(gx_restrict_CIEDEFG);
cs_proc_install_cspace(gx_install_CIEDEFG);
cs_proc_restrict_color(gx_restrict_CIEDEF);
cs_proc_install_cspace(gx_install_CIEDEF);
cs_proc_restrict_color(gx_restrict_CIEABC);
cs_proc_install_cspace(gx_install_CIEABC);
cs_proc_restrict_color(gx_restrict_CIEA);
cs_proc_install_cspace(gx_install_CIEA);

/*
 * Initialize (just enough of) an gs_gstate so that "concretizing" colors
 * using this gs_gstate will do only the CIE->XYZ mapping.  This is a
 * semi-hack for the PDF writer.
 */
extern	int	gx_cie_to_xyz_alloc(gs_gstate **,
                                    const gs_color_space *, gs_memory_t *);
extern	void	gx_cie_to_xyz_free(gs_gstate *);
extern int gx_cie_to_xyz_alloc2(gs_color_space * pcs, gs_gstate * pgs);

/* Defined in gsciemap.c */

/*
 * Test whether a CIE rendering has been defined; ensure that the joint
 * caches are loaded.  Note that the procedure may return 1 if no rendering
 * has been defined.
 */
int gx_cie_check_rendering(const gs_color_space * pcs, frac * pconc, const gs_gstate * pgs);

/*
 * Do the common remapping operation for CIE color spaces. Returns the
 * number of components of the concrete color space (3 if RGB, 4 if CMYK).
 * This simply calls a procedure variable stored in the joint caches
 * structure.
 */
extern  int     gx_cie_remap_finish( cie_cached_vector3,
                                     frac *, float *,
                                     const gs_gstate *,
                                     const gs_color_space * );
/* Make sure the prototype matches the one defined in gscie.h. */
extern GX_CIE_REMAP_FINISH_PROC(gx_cie_remap_finish);

/*
 * Define the real remap_finish procedure.  Except for CIE->XYZ mapping,
 * this is what is stored in the remap_finish member of the joint caches.
 */
extern GX_CIE_REMAP_FINISH_PROC(gx_cie_real_remap_finish);
/*
 * Define the remap_finish procedure for CIE->XYZ mapping.
 */
extern GX_CIE_REMAP_FINISH_PROC(gx_cie_xyz_remap_finish);

cs_proc_concretize_color(gx_concretize_CIEDEFG);
cs_proc_concretize_color(gx_concretize_CIEDEF);
cs_proc_concretize_color(gx_concretize_CIEABC);
#if ENABLE_CUSTOM_COLOR_CALLBACK
cs_proc_remap_color(gx_remap_IndexedSpace);
#endif
cs_proc_remap_color(gx_remap_CIEDEF);
cs_proc_remap_color(gx_remap_CIEDEFG);
cs_proc_remap_color(gx_remap_CIEA);
cs_proc_remap_color(gx_remap_CIEABC);
cs_proc_concretize_color(gx_concretize_CIEA);

/* Defined in gscscie.c */

/* GC routines exported for gsicc.c */
extern_st(st_cie_common);
extern_st(st_cie_common_elements_t);

/* set up the common default values for a CIE color space */
extern  void    gx_set_common_cie_defaults( gs_cie_common *,
                                            void *  client_data );

/* Load the common caches for a CIE color space */
extern  void    gx_cie_load_common_cache(gs_cie_common *, gs_gstate *);

/* Complete loading of the common caches */
extern  void    gx_cie_common_complete(gs_cie_common *);

/* "indirect" color space installation procedure */
cs_proc_install_cspace(gx_install_CIE);

/* allocate and initialize the common part of a cie color space */
extern  void *  gx_build_cie_space( gs_color_space **           ppcspace,
                                    const gs_color_space_type * pcstype,
                                    gs_memory_type_ptr_t        stype,
                                    gs_memory_t *               pmem );

/*
 * Determine the concrete space which underlies a CIE based space. For all
 * device independent color spaces, this is dependent on the current color
 * rendering dictionary, rather than the current color space. This procedure
 * is exported for use by gsicc.c to implement ICCBased color spaces.
 */
cs_proc_concrete_space(gx_concrete_space_CIE);

/* Special operations used in the creation of ICC color spaces from PS
   spaces.  These are used to map from PS color to CIEXYZ */
int gx_psconcretize_CIEDEFG(const gs_client_color * pc, const gs_color_space * pcs,
                      frac * pconc, float * xyz, const gs_gstate * pgs);
int gx_psconcretize_CIEDEF(const gs_client_color * pc, const gs_color_space * pcs,
                     frac * pconc, float * xyz, const gs_gstate * pgs);
int gx_psconcretize_CIEABC(const gs_client_color * pc, const gs_color_space * pcs,
                     frac * pconc, float * xyz, const gs_gstate * pgs);
int gx_psconcretize_CIEA(const gs_client_color * pc, const gs_color_space * pcs,
                     frac * pconc, float * xyz, const gs_gstate * pgs);
bool check_range(gs_range *ranges, int num_colorants);
bool check_cie_range( const gs_color_space * pcs );
gs_range* get_cie_range( const gs_color_space * pcs );
bool rescale_cie_colors(const gs_color_space * pcs, gs_client_color *cc);
#endif /* gxcie_INCLUDED */
