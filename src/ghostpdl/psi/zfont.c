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


/* Generic font operators */
#include "ghost.h"
#include "oper.h"
#include "gsstruct.h"		/* for registering root */
#include "gzstate.h"		/* must precede gxdevice */
#include "gxdevice.h"		/* must precede gxfont */
#include "gxfont.h"
#include "gxfcache.h"
#include "bfont.h"
#include "ialloc.h"
#include "iddict.h"
#include "igstate.h"
#include "iname.h"		/* for name_mark_index */
#include "isave.h"
#include "store.h"
#include "ivmspace.h"
#include "gscencs.h"

/* Forward references */
static int make_font(i_ctx_t *, const gs_matrix *);
static void make_uint_array(os_ptr, const uint *, int);
static int setup_unicode_decoder(i_ctx_t *i_ctx_p, ref *Decoding);

/* Mark a glyph as a PostScript name (if it isn't a CID). */
bool
zfont_mark_glyph_name(const gs_memory_t *mem, gs_glyph glyph, void *ignore_data)
{
    return (glyph >= gs_c_min_std_encoding_glyph || glyph == GS_NO_GLYPH ? false :
            name_mark_index(mem, (uint) glyph));
}

/* Get a global glyph code.  */
static int
zfont_global_glyph_code(const gs_memory_t *mem, gs_const_string *gstr, gs_glyph *pglyph)
{
    ref v;
    int code = name_ref(mem, gstr->data, gstr->size, &v, 0);

    if (code < 0)
        return code;
    *pglyph = (gs_glyph)name_index(mem, &v);
    return 0;
}

/* Initialize the font operators */
static int
zfont_init(i_ctx_t *i_ctx_p)
{
    ifont_dir = gs_font_dir_alloc2(imemory->stable_memory, imemory->non_gc_memory);
    if (ifont_dir == NULL)
        return gs_error_VMerror;
    ifont_dir->ccache.mark_glyph = zfont_mark_glyph_name;
    ifont_dir->global_glyph_code = zfont_global_glyph_code;
    return gs_register_struct_root(imemory, NULL, (void **)&ifont_dir,
                                   "ifont_dir");
}

/* <font> <scale> scalefont <new_font> */
static int
zscalefont(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;
    double scale;
    gs_matrix mat;

    if ((code = real_param(op, &scale)) < 0)
        return code;
    if ((code = gs_make_scaling(scale, scale, &mat)) < 0)
        return code;
    return make_font(i_ctx_p, &mat);
}

/* <font> <matrix> makefont <new_font> */
static int
zmakefont(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;
    gs_matrix mat;

    if ((code = read_matrix(imemory, op, &mat)) < 0)
        return code;
    return make_font(i_ctx_p, &mat);
}

/* <font> setfont - */
int
zsetfont(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_font *pfont;
    int code = font_param(op, &pfont);

    if (code < 0 || (code = gs_setfont(igs, pfont)) < 0)
        return code;
    pop(1);
    return code;
}

/* - currentfont <font> */
static int
zcurrentfont(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    *op = *pfont_dict(gs_currentfont(igs));
    return 0;
}

/* - cachestatus <mark> <bsize> <bmax> <msize> <mmax> <csize> <cmax> <blimit> */
static int
zcachestatus(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    uint status[7];

    gs_cachestatus(ifont_dir, status);
    push(7);
    make_uint_array(op - 6, status, 7);
    return 0;
}

/* <blimit> setcachelimit - */
static int
zsetcachelimit(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    check_int_leu(*op, max_uint);
    gs_setcachelimit(ifont_dir, (uint) op->value.intval);
    pop(1);
    return 0;
}

/* <mark> <size> <lower> <upper> setcacheparams - */
static int
zsetcacheparams(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    uint params[3];
    int i, code;
    os_ptr opp = op;

    for (i = 0; i < 3 && !r_has_type(opp, t_mark); i++, opp--) {
        check_int_leu(*opp, max_uint);
        params[i] = opp->value.intval;
    }
    switch (i) {
        case 3:
            if ((code = gs_setcachesize(igs, ifont_dir, params[2])) < 0)
                return code;
        case 2:
            if ((code = gs_setcachelower(ifont_dir, params[1])) < 0)
                return code;
        case 1:
            if ((code = gs_setcacheupper(ifont_dir, params[0])) < 0)
                return code;
        case 0:;
    }
    return zcleartomark(i_ctx_p);
}

/* - currentcacheparams <mark> <size> <lower> <upper> */
static int
zcurrentcacheparams(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    uint params[3];

    params[0] = gs_currentcachesize(ifont_dir);
    params[1] = gs_currentcachelower(ifont_dir);
    params[2] = gs_currentcacheupper(ifont_dir);
    push(4);
    make_mark(op - 3);
    make_uint_array(op - 2, params, 3);
    return 0;
}

/* <font> .registerfont - */
static int
zregisterfont(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_font *pfont;
    int code = font_param(op, &pfont);

    if (code < 0)
        return code;
    pfont->is_resource = true;
    pop(1);
    return 0;
}

/* <Decoding> .setupUnicodeDecoder - */
static int
zsetupUnicodeDecoder(i_ctx_t *i_ctx_p)
{   /* The allocation mode must be global. */
    os_ptr op = osp;
    int code;

    check_type(*op, t_dictionary);
    code = setup_unicode_decoder(i_ctx_p, op);
    if (code < 0)
        return code;
    pop(1);
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zfont_op_defs[] =
{
    {"0currentfont", zcurrentfont},
    {"2makefont", zmakefont},
    {"2scalefont", zscalefont},
    {"1setfont", zsetfont},
    {"0cachestatus", zcachestatus},
    {"1setcachelimit", zsetcachelimit},
    {"1setcacheparams", zsetcacheparams},
    {"0currentcacheparams", zcurrentcacheparams},
    {"1.registerfont", zregisterfont},
    {"1.setupUnicodeDecoder", zsetupUnicodeDecoder},
    op_def_end(zfont_init)
};

/* ------ Subroutines ------ */

/* Validate a font parameter. */
int
font_param(const ref * pfdict, gs_font ** ppfont)
{	/*
         * Check that pfdict is a read-only dictionary, that it has a FID
         * entry whose value is a fontID, and that the fontID points to a
         * gs_font structure whose associated PostScript dictionary is
         * pfdict.
         */
    ref *pid;
    gs_font *pfont;
    const font_data *pdata;

    check_type(*pfdict, t_dictionary);
    if (dict_find_string(pfdict, "FID", &pid) <= 0 ||
        !r_has_type(pid, t_fontID)
        )
        return_error(gs_error_invalidfont);
    pfont = r_ptr(pid, gs_font);
    if (pfont == 0)
        return_error(gs_error_invalidfont);	/* unregistered font */
    pdata = pfont->client_data;
    if (!obj_eq(pfont->memory, &pdata->dict, pfdict))
        return_error(gs_error_invalidfont);
    *ppfont = pfont;
    return 0;
}

/* Add the FID entry to a font dictionary. */
/* Note that i_ctx_p may be NULL. */
int
add_FID(i_ctx_t *i_ctx_p, ref * fp /* t_dictionary */ , gs_font * pfont,
        gs_ref_memory_t *imem)
{
    ref fid;

    make_tav(&fid, t_fontID,
             a_readonly | imemory_space(imem) | imemory_new_mask(imem),
             pstruct, (void *)pfont);
    return (i_ctx_p ? idict_put_string(fp, "FID", &fid) :
            dict_put_string(fp, "FID", &fid, NULL));
}

/* Make a transformed font (common code for makefont/scalefont). */
static int
make_font(i_ctx_t *i_ctx_p, const gs_matrix * pmat)
{
    os_ptr op = osp;
    os_ptr fp = op - 1;
    gs_font *oldfont, *newfont;
    int code;
    ref *pencoding = 0;

    code = font_param(fp, &oldfont);
    if (code < 0)
        return code;
    {
        uint space = ialloc_space(idmemory);

        ialloc_set_space(idmemory, r_space(fp));
        if (dict_find_string(fp, "Encoding", &pencoding) > 0 &&
            !r_is_array(pencoding)
            )
            code = gs_note_error(gs_error_invalidfont);
        else {
                /*
                 * Temporarily substitute the new dictionary
                 * for the old one, in case the Encoding changed.
                 */
            ref olddict;

            olddict = *pfont_dict(oldfont);
            *pfont_dict(oldfont) = *fp;
            code = gs_makefont(ifont_dir, oldfont, pmat, &newfont);
            *pfont_dict(oldfont) = olddict;
        }
        ialloc_set_space(idmemory, space);
    }
    if (code < 0)
        return code;
    /*
     * We have to allow for the possibility that the font's Encoding
     * is different from that of the base font.  Note that the
     * font_data of the new font was simply copied from the old one.
     */
    if (pencoding != 0 &&
        !obj_eq(imemory, pencoding, &pfont_data(newfont)->Encoding)
        ) {
        if (newfont->FontType == ft_composite)
            return_error(gs_error_rangecheck);
        /* We should really do validity checking here.... */
        ref_assign(&pfont_data(newfont)->Encoding, pencoding);
        lookup_gs_simple_font_encoding((gs_font_base *) newfont);
    }
    *fp = *pfont_dict(newfont);
    pop(1);
    return 0;
}
/* Create the transformed font dictionary. */
/* This is the make_font completion procedure for all non-composite fonts */
/* created at the interpreter level (see build_gs_simple_font in zbfont.c.) */
int
zbase_make_font(gs_font_dir * pdir, const gs_font * oldfont,
                const gs_matrix * pmat, gs_font ** ppfont)
{
    /*
     * We must call gs_base_make_font so that the XUID gets copied
     * if necessary.
     */
    int code = gs_base_make_font(pdir, oldfont, pmat, ppfont);

    if (code < 0)
        return code;
    return zdefault_make_font(pdir, oldfont, pmat, ppfont);
}
int
zdefault_make_font(gs_font_dir * pdir, const gs_font * oldfont,
                   const gs_matrix * pmat, gs_font ** ppfont)
{
    gs_font *newfont = *ppfont;
    gs_memory_t *mem = newfont->memory;
    /* HACK: we know this font was allocated by the interpreter. */
    gs_ref_memory_t *imem = (gs_ref_memory_t *)mem;
    ref *fp = pfont_dict(oldfont);
    font_data *pdata;
    ref newdict, newmat, scalemat;
    uint dlen = dict_maxlength(fp);
    uint mlen = dict_length(fp) + 3;	/* FontID, OrigFont, ScaleMatrix */
    int code;

    if (dlen < mlen)
        dlen = mlen;
    if ((pdata = gs_alloc_struct(mem, font_data, &st_font_data,
                                 "make_font(font_data)")) == 0
        )
        return_error(gs_error_VMerror);
    /*
     * This dictionary is newly created: it's safe to pass NULL as the
     * dstack pointer to dict_copy and dict_put_string.
     */
    if ((code = dict_alloc(imem, dlen, &newdict)) < 0 ||
        (code = dict_copy(fp, &newdict, NULL)) < 0 ||
        (code = gs_alloc_ref_array(imem, &newmat, a_all, 12,
                                   "make_font(matrices)")) < 0
        )
        return code;
    refset_null_new(newmat.value.refs, 12, imemory_new_mask(imem));
    ref_assign(&scalemat, &newmat);
    r_set_size(&scalemat, 6);
    scalemat.value.refs += 6;
    /*
     * Create the scaling matrix.  We could do this several different
     * ways: by "dividing" the new FontMatrix by the base FontMatrix, by
     * multiplying the current scaling matrix by a ScaleMatrix kept in
     * the gs_font, or by multiplying the current scaling matrix by the
     * ScaleMatrix from the font dictionary.  We opt for the last of
     * these.
     */
    {
        gs_matrix scale, prev_scale;
        ref *ppsm;

        if (!(dict_find_string(fp, "ScaleMatrix", &ppsm) > 0 &&
              read_matrix(mem, ppsm, &prev_scale) >= 0 &&
              gs_matrix_multiply(pmat, &prev_scale, &scale) >= 0)
            )
            scale = *pmat;
        write_matrix_new(&scalemat, &scale, imem);
    }
    r_clear_attrs(&scalemat, a_write);
    r_set_size(&newmat, 6);
    write_matrix_new(&newmat, &newfont->FontMatrix, imem);
    r_clear_attrs(&newmat, a_write);
    if ((code = dict_put_string(&newdict, "FontMatrix", &newmat, NULL)) < 0 ||
        (code = dict_put_string(&newdict, "OrigFont", pfont_dict(oldfont->base), NULL)) < 0 ||
        (code = dict_put_string(&newdict, "ScaleMatrix", &scalemat, NULL)) < 0 ||
        (code = add_FID(NULL, &newdict, newfont, imem)) < 0
        )
        return code;
    newfont->client_data = pdata;
    *pdata = *pfont_data(oldfont);
    pdata->dict = newdict;
    r_clear_attrs(dict_access_ref(&newdict), a_write);
    return 0;
}

/* Convert an array of (unsigned) integers to stack form. */
static void
make_uint_array(register os_ptr op, const uint * intp, int count)
{
    int i;

    for (i = 0; i < count; i++, op++, intp++)
        make_int(op, *intp);
}

/* Remove scaled font and character cache entries that would be */
/* invalidated by a restore. */
static bool
purge_if_name_removed(const gs_memory_t *mem, cached_char * cc, void *vsave)
{
    return alloc_name_index_is_since_save(mem, cc->code, vsave);
}

/* Remove entries from font and character caches. */
int
font_restore(const alloc_save_t * save)
{

    gs_memory_t *smem = gs_save_any_memory(save);
    gs_font_dir *pdir = smem->gs_lib_ctx->font_dir;
    int code;

    if (pdir == 0)		/* not initialized yet */
        return 0;

    /* Purge original (unscaled) fonts. */

    {
        gs_font *pfont;

otop:
        for (pfont = pdir->orig_fonts; pfont != 0;
             pfont = pfont->next
            ) {
            if (alloc_is_since_save((char *)pfont, save)) {
                code = gs_purge_font(pfont);
                if (code < 0)
                    return code;
                goto otop;
            }
        }
    }

    /* Purge cached scaled fonts. */

    {
        gs_font *pfont;

top:
        for (pfont = pdir->scaled_fonts; pfont != 0;
             pfont = pfont->next
            ) {
            if (alloc_is_since_save((char *)pfont, save)) {
                code = gs_purge_font(pfont);
                if (code < 0)
                    return code;
                goto top;
            }
        }
    }

    /* Purge xfonts and uncached scaled fonts. */

    {
        cached_fm_pair *pair;
        uint n;

        for (pair = pdir->fmcache.mdata, n = pdir->fmcache.mmax;
             n > 0; pair++, n--
            )
            if (!fm_pair_is_free(pair)) {
#if 0
                /* We disabled this code portion because
                   gx_add_fm_pair now copied xvalues
                   into a stable memory.
                 */
                if ((uid_is_XUID(&pair->UID) &&
                     alloc_is_since_save((char *)pair->UID.xvalues,
                                         save))
                    ) {
                    code = gs_purge_fm_pair(pdir, pair, 0);
                    if (code < 0)
                        return code;
                    continue;
                }
#endif
                if (pair->font != 0 &&
                    alloc_is_since_save((char *)pair->font, save)
                    ) {
                    if (!uid_is_valid(&pair->UID))
                        gs_clean_fm_pair(pdir, pair);
                    /* Don't discard pairs with a surviving UID. */
                    pair->font = 0;
                }
                if (pair->xfont != 0 &&
                    alloc_is_since_save((char *)pair->xfont, save)
                    ) {
                    code = gs_purge_fm_pair(pdir, pair, 1);
                    if (code < 0)
                        return code;
                }
            }
    }

    /* Purge characters with names about to be removed. */
    /* We only need to do this if any new names have been created */
    /* since the save. */

    if (alloc_any_names_since_save(save))
        gx_purge_selected_cached_chars(pdir, purge_if_name_removed,
                                       (void *)save);
    return 0;
}

/* ------ Font procedures for PostScript fonts ------ */

/* font_info procedure */
static bool
zfont_info_has(const ref *pfidict, const char *key, gs_const_string *pmember)
{
    ref *pvalue;

    if (dict_find_string(pfidict, key, &pvalue) > 0 &&
        r_has_type(pvalue, t_string)
        ) {
        pmember->data = pvalue->value.const_bytes;
        pmember->size = r_size(pvalue);
        return true;
    }
    return false;
}
int
zfont_info(gs_font *font, const gs_point *pscale, int members,
           gs_font_info_t *info)
{
    int code = gs_default_font_info(font, pscale, members &
                    ~(FONT_INFO_COPYRIGHT | FONT_INFO_NOTICE |
                      FONT_INFO_FAMILY_NAME | FONT_INFO_FULL_NAME),
                                    info);
    const ref *pfdict;
    ref *pfontinfo, *pvalue;

    if (code < 0)
        return code;
    pfdict = &pfont_data(font)->dict;
    if (dict_find_string(pfdict, "FontInfo", &pfontinfo) <= 0 ||
        !r_has_type(pfontinfo, t_dictionary))
        return 0;
    if ((members & FONT_INFO_COPYRIGHT) &&
        zfont_info_has(pfontinfo, "Copyright", &info->Copyright))
        info->members |= FONT_INFO_COPYRIGHT;
    if ((members & FONT_INFO_NOTICE) &&
        zfont_info_has(pfontinfo, "Notice", &info->Notice))
        info->members |= FONT_INFO_NOTICE;
    if ((members & FONT_INFO_FAMILY_NAME) &&
        zfont_info_has(pfontinfo, "FamilyName", &info->FamilyName))
        info->members |= FONT_INFO_FAMILY_NAME;
    if ((members & FONT_INFO_FULL_NAME) &&
        zfont_info_has(pfontinfo, "FullName", &info->FullName))
        info->members |= FONT_INFO_FULL_NAME;
    if ((members & FONT_INFO_EMBEDDING_RIGHTS)
        && (dict_find_string(pfontinfo, "FSType", &pvalue) > 0)) {
        info->EmbeddingRights = pvalue->value.intval;
        info->members |= FONT_INFO_EMBEDDING_RIGHTS;
    }
    return code;
}

/* -------------------- Utilities --------------*/

typedef struct gs_unicode_decoder_s {
    ref data;
} gs_unicode_decoder;

/* GC procedures */
static
CLEAR_MARKS_PROC(unicode_decoder_clear_marks)
{   gs_unicode_decoder *const pptr = vptr;

    r_clear_attrs(&pptr->data, l_mark);
}
static
ENUM_PTRS_WITH(unicode_decoder_enum_ptrs, gs_unicode_decoder *pptr) return 0;
case 0:
ENUM_RETURN_REF(&pptr->data);
ENUM_PTRS_END
static RELOC_PTRS_WITH(unicode_decoder_reloc_ptrs, gs_unicode_decoder *pptr);
RELOC_REF_VAR(pptr->data);
r_clear_attrs(&pptr->data, l_mark);
RELOC_PTRS_END

gs_private_st_complex_only(st_unicode_decoder, gs_unicode_decoder,\
    "unicode_decoder", unicode_decoder_clear_marks, unicode_decoder_enum_ptrs,
    unicode_decoder_reloc_ptrs, 0);

/* Get the Unicode value for a glyph. */
const ref *
zfont_get_to_unicode_map(gs_font_dir *dir)
{
    const gs_unicode_decoder *pud = (gs_unicode_decoder *)dir->glyph_to_unicode_table;

    return (pud == NULL ? NULL : &pud->data);
}

static int
setup_unicode_decoder(i_ctx_t *i_ctx_p, ref *Decoding)
{
    gs_unicode_decoder *pud = gs_alloc_struct(imemory, gs_unicode_decoder,
                             &st_unicode_decoder, "setup_unicode_decoder");
    if (pud == NULL)
        return_error(gs_error_VMerror);
    ref_assign_new(&pud->data, Decoding);
    ifont_dir->glyph_to_unicode_table = pud;
    return 0;
}
