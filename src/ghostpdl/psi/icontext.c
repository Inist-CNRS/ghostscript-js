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


/* Context state operations */
#include "ghost.h"
#include "gsstruct.h"		/* for gxalloc.h */
#include "gxalloc.h"
#include "ierrors.h"
#include "stream.h"		/* for files.h */
#include "files.h"
#include "idict.h"
#include "igstate.h"
#include "icontext.h"
#include "interp.h"
#include "isave.h"
#include "dstack.h"
#include "estack.h"
#include "store.h"

/* Declare the GC descriptors for embedded objects. */
extern_st(st_gs_dual_memory);
extern_st(st_ref_stack);
extern_st(st_dict_stack);
extern_st(st_exec_stack);
extern_st(st_op_stack);

/* GC descriptors */
static
CLEAR_MARKS_PROC(context_state_clear_marks)
{
    gs_context_state_t *const pcst = vptr;

    r_clear_attrs(&pcst->stdio[0], l_mark);
    r_clear_attrs(&pcst->stdio[1], l_mark);
    r_clear_attrs(&pcst->stdio[2], l_mark);
    r_clear_attrs(&pcst->error_object, l_mark);
    r_clear_attrs(&pcst->userparams, l_mark);
    r_clear_attrs(&pcst->op_array_table_global.table, l_mark);
    r_clear_attrs(&pcst->op_array_table_local.table, l_mark);
}
static
ENUM_PTRS_WITH(context_state_enum_ptrs, gs_context_state_t *pcst) {
    index -= 10;
    if (index < st_gs_dual_memory_num_ptrs)
        return ENUM_USING(st_gs_dual_memory, &pcst->memory,
                          sizeof(pcst->memory), index);
    index -= st_gs_dual_memory_num_ptrs;
    if (index < st_dict_stack_num_ptrs)
        return ENUM_USING(st_dict_stack, &pcst->dict_stack,
                          sizeof(pcst->dict_stack), index);
    index -= st_dict_stack_num_ptrs;
    if (index < st_exec_stack_num_ptrs)
        return ENUM_USING(st_exec_stack, &pcst->exec_stack,
                          sizeof(pcst->exec_stack), index);
    index -= st_exec_stack_num_ptrs;
    return ENUM_USING(st_op_stack, &pcst->op_stack,
                      sizeof(pcst->op_stack), index);
    }
    ENUM_PTR(0, gs_context_state_t, pgs);
    case 1: ENUM_RETURN_REF(&pcst->stdio[0]);
    case 2: ENUM_RETURN_REF(&pcst->stdio[1]);
    case 3: ENUM_RETURN_REF(&pcst->stdio[2]);
    case 4: ENUM_RETURN_REF(&pcst->error_object);
    case 5: ENUM_RETURN_REF(&pcst->userparams);
    ENUM_PTR(6, gs_context_state_t, op_array_table_global.nx_table);
    ENUM_PTR(7, gs_context_state_t, op_array_table_local.nx_table);
    case 8: ENUM_RETURN_REF(&pcst->op_array_table_global.table);
    case 9: ENUM_RETURN_REF(&pcst->op_array_table_local.table);
ENUM_PTRS_END
static RELOC_PTRS_WITH(context_state_reloc_ptrs, gs_context_state_t *pcst);
    RELOC_PTR(gs_context_state_t, pgs);
    RELOC_USING(st_gs_dual_memory, &pcst->memory, sizeof(pcst->memory));
    /******* WHY DON'T WE CLEAR THE l_mark OF stdio? ******/
    RELOC_REF_VAR(pcst->stdio[0]);
    RELOC_REF_VAR(pcst->stdio[1]);
    RELOC_REF_VAR(pcst->stdio[2]);
    RELOC_REF_VAR(pcst->error_object);
    r_clear_attrs(&pcst->error_object, l_mark);
    RELOC_REF_VAR(pcst->userparams);
    r_clear_attrs(&pcst->userparams, l_mark);
    RELOC_PTR(gs_context_state_t, op_array_table_global.nx_table);
    RELOC_PTR(gs_context_state_t, op_array_table_local.nx_table);
    RELOC_REF_VAR(pcst->op_array_table_global.table);
    r_clear_attrs(&pcst->op_array_table_global.table, l_mark);
    RELOC_REF_VAR(pcst->op_array_table_local.table);
    r_clear_attrs(&pcst->op_array_table_local.table, l_mark);
    RELOC_USING(st_dict_stack, &pcst->dict_stack, sizeof(pcst->dict_stack));
    RELOC_USING(st_exec_stack, &pcst->exec_stack, sizeof(pcst->exec_stack));
    RELOC_USING(st_op_stack, &pcst->op_stack, sizeof(pcst->op_stack));
RELOC_PTRS_END
public_st_context_state();

static int
no_reschedule(i_ctx_t **pi_ctx_p)
{
    return (gs_error_invalidcontext);
}

/* Allocate the state of a context. */
int
context_state_alloc(gs_context_state_t ** ppcst,
                    const ref *psystem_dict,
                    const gs_dual_memory_t * dmem)
{
    gs_ref_memory_t *mem = dmem->space_local;
    gs_context_state_t *pcst = *ppcst;
    int code;
    int i;

    if (pcst == 0) {
        pcst = gs_alloc_struct((gs_memory_t *) mem, gs_context_state_t,
                               &st_context_state, "context_state_alloc");
        if (pcst == 0)
            return_error(gs_error_VMerror);
    }
    code = gs_interp_alloc_stacks(mem, pcst);
    if (code < 0)
        goto x0;
    /*
     * We have to initialize the dictionary stack early,
     * for far-off references to systemdict.
     */
    pcst->dict_stack.system_dict = *psystem_dict;
    pcst->dict_stack.min_size = 0;
    pcst->dict_stack.userdict_index = 0;
    pcst->pgs = int_gstate_alloc(dmem);
    if (pcst->pgs == 0) {
        code = gs_note_error(gs_error_VMerror);
        goto x1;
    }
    pcst->memory = *dmem;
    pcst->language_level = 1;
    make_false(&pcst->array_packing);
    make_int(&pcst->binary_object_format, 0);
    pcst->nv_page_count = 0;
    pcst->rand_state = rand_state_initial;
    pcst->usertime_total = 0;
    pcst->keep_usertime = false;
    pcst->in_superexec = 0;
    pcst->plugin_list = 0;
    make_t(&pcst->error_object, t__invalid);
    {	/*
         * Create an empty userparams dictionary of the right size.
         * If we can't determine the size, pick an arbitrary one.
         */
        ref *puserparams;
        uint size;
        ref *system_dict = &pcst->dict_stack.system_dict;

        if (dict_find_string(system_dict, "userparams", &puserparams) >= 0)
            size = dict_length(puserparams);
        else
            size = 300;
        code = dict_alloc(pcst->memory.space_local, size, &pcst->userparams);
        if (code < 0)
            goto x2;
        /* PostScript code initializes the user parameters. */
    }
    pcst->scanner_options = 0;
    pcst->LockFilePermissions = false;
    pcst->starting_arg_file = false;
    pcst->RenderTTNotdef = true;
    /* Create and initialize an invalid (closed) stream. */
    /* Initialize the stream for the sake of the GC, */
    /* and so it can act as an empty input stream. */
    {
        stream *s;

        s = (stream*)gs_alloc_bytes_immovable(mem->non_gc_memory->stable_memory,
                                              sizeof(*s),
                                              "context_state_alloc");
        if (s == NULL) {
            code = gs_error_VMerror;
            goto x3;
        }
        pcst->invalid_file_stream = s;

        s_init(s, NULL);
        sread_string(s, NULL, 0);
        s->next = s->prev = 0;
        s_init_no_id(s);
    }
    /* The initial stdio values are bogus.... */
    make_file(&pcst->stdio[0], a_readonly | avm_invalid_file_entry, 1,
              pcst->invalid_file_stream);
    make_file(&pcst->stdio[1], a_all | avm_invalid_file_entry, 1,
              pcst->invalid_file_stream);
    make_file(&pcst->stdio[2], a_all | avm_invalid_file_entry, 1,
              pcst->invalid_file_stream);
    for (i = countof(pcst->memory.spaces_indexed); --i >= 0;)
        if (dmem->spaces_indexed[i] != 0)
            ++(dmem->spaces_indexed[i]->num_contexts);
    /* The number of interpreter "ticks" between calls on the time_slice_proc.
     * Currently, the clock ticks before each operator, and at each
     * procedure return. */
    pcst->time_slice_ticks = 0x7fff;
    pcst->reschedule_proc = no_reschedule;
    pcst->time_slice_proc = no_reschedule;
    *ppcst = pcst;
    return 0;
  x3:/* No need to delete dictionary here, as gc will do it for us. */
  x2:gs_gstate_free(pcst->pgs);
  x1:gs_interp_free_stacks(mem, pcst);
  x0:if (*ppcst == 0)
        gs_free_object((gs_memory_t *) mem, pcst, "context_state_alloc");
    return code;
}

/* Load the interpreter state from a context. */
int
context_state_load(gs_context_state_t * i_ctx_p)
{
    gs_ref_memory_t *lmem = iimemory_local;
    ref *system_dict = systemdict;
    uint space = r_space(system_dict);
    dict_stack_t *dstack = &idict_stack;
    int code;

    /*
     * Disable save checking, and space check for systemdict, while
     * copying dictionaries.
     */
    alloc_set_not_in_save(idmemory);
    r_set_space(system_dict, avm_max);
    /*
     * Switch references from systemdict to local objects.
     * userdict.localdicts holds these objects.  We could optimize this by
     * only doing it if we're changing to a different local VM relative to
     * the same global VM, but the cost is low enough relative to other
     * things that we don't bother.
     */
    {
        ref_stack_t *rdstack = &dstack->stack;
        const ref *puserdict =
            ref_stack_index(rdstack, ref_stack_count(rdstack) - 1 -
                            dstack->userdict_index);
        ref *plocaldicts;

        if (dict_find_string(puserdict, "localdicts", &plocaldicts) > 0 &&
            r_has_type(plocaldicts, t_dictionary)
            ) {
            dict_copy(plocaldicts, system_dict, dstack);
        }
    }
    /*
     * Set systemdict.userparams to the saved copy, and then
     * set the actual user parameters.  Note that we must disable both
     * space checking and save checking while doing this.  Also,
     * we must do this after copying localdicts (if required), because
     * userparams also appears in localdicts.
     */
    code = dict_put_string(system_dict, "userparams", &i_ctx_p->userparams,
                           dstack);
    if (code >= 0)
        code = set_user_params(i_ctx_p, &i_ctx_p->userparams);
    r_set_space(system_dict, space);
    if (lmem->save_level > 0)
        alloc_set_in_save(idmemory);
    estack_clear_cache(&iexec_stack);
    dstack_set_top(&idict_stack);
    return code;
}

/* Store the interpreter state in a context. */
int
context_state_store(gs_context_state_t * pcst)
{
    ref_stack_cleanup(&pcst->dict_stack.stack);
    ref_stack_cleanup(&pcst->exec_stack.stack);
    ref_stack_cleanup(&pcst->op_stack.stack);
    /*
     * The user parameters in systemdict.userparams are kept
     * up to date by PostScript code, but we still need to save
     * systemdict.userparams to get the correct l_new flag.
     */
    {
        ref *puserparams;
        /* We need i_ctx_p for access to the d_stack. */
        i_ctx_t *i_ctx_p = pcst;

        if (dict_find_string(systemdict, "userparams", &puserparams) < 0)
            return_error(gs_error_Fatal);
        pcst->userparams = *puserparams;
    }
    return 0;
}

/* Free the contents of the state of a context, always to its local VM. */
/* Return a mask of which of its VMs, if any, we freed. */
int
context_state_free(gs_context_state_t * pcst)
{
    gs_ref_memory_t *mem = pcst->memory.space_local;
    int freed = 0;
    int i;

    /* Invalid file stream is always in static space, so needs to be done
     * separately. */
    gs_free_object(mem->non_gc_memory->stable_memory, pcst->invalid_file_stream, "context_state_alloc");
    /*
     * If this context is the last one referencing a particular VM
     * (local / global / system), free the entire VM space;
     * otherwise, just free the context-related structures.
     */
    for (i = countof(pcst->memory.spaces_indexed); --i >= 0;) {
        if (pcst->memory.spaces_indexed[i] != 0 &&
            !--(pcst->memory.spaces_indexed[i]->num_contexts)
            ) {
/****** FREE THE ENTIRE SPACE ******/
            freed |= 1 << i;
        }
    }
    /*
     * If we freed any spaces at all, we must have freed the local
     * VM where the context structure and its substructures were
     * allocated.
     */
    if (freed)
        return freed;
    {
        gs_gstate *pgs = pcst->pgs;

        gs_grestoreall(pgs);
        /* Patch the saved pointer so we can do the last grestore. */
        {
            gs_gstate *saved = gs_gstate_saved(pgs);

            gs_gstate_swap_saved(saved, saved);
        }
        gs_grestore(pgs);
        gs_gstate_swap_saved(pgs, (gs_gstate *) 0);
        gs_gstate_free(pgs);
    }
/****** FREE USERPARAMS ******/
    gs_interp_free_stacks(mem, pcst);
    return 0;
}
