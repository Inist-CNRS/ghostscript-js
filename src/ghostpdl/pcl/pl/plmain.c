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


/* plmain.c */
/* Main program command-line interpreter for PCL interpreters */
#include "string_.h"
#include <stdlib.h> /* atof */
#include "gdebug.h"
#include "gscdefs.h"
#include "gsio.h"
#include "gstypes.h"
#include "gserrors.h"
#include "gsmemory.h"
#include "plalloc.h"
#include "gsmalloc.h"
#include "gsmchunk.h"
#include "gsstruct.h"
#include "gxalloc.h"
#include "gsalloc.h"
#include "gsargs.h"
#include "gp.h"
#include "gsdevice.h"
#include "gxdevice.h"
#include "gxdevsop.h"       /* for gxdso_* */
#include "gxclpage.h"
#include "gdevprn.h"
#include "gsparam.h"
#include "gslib.h"
#include "pjtop.h"
#include "plparse.h"
#include "plplatf.h"
#include "plmain.h"
#include "pltop.h"
#include "pltoputl.h"
#include "plapi.h"
#include "gslibctx.h"
/* includes for the display device */
#include "gdevdevn.h"
#include "gsequivc.h"
#include "gdevdsp.h"
#include "gdevdsp2.h"

#ifdef _Windows
/* FIXME: this is purely because the gsdll.h requires psi/iapi.h and
 * we don't want that required here. But as a couple of Windows specific
 * devices depend upon pgsdll_callback being defined, having a compatible
 * set of declarations here saves having to have different device lists
 * for Ghostscript and the other languages, and as both devices are
 * deprecated, a simple solution seems best = for now.
 */
#ifdef __IBMC__
#define GSPLDLLCALLLINK _System
#else
#define GSPLDLLCALLLINK
#endif

typedef int (* GSPLDLLCALLLINK GS_PL_DLL_CALLBACK) (int, char *, unsigned long);
GS_PL_DLL_CALLBACK pgsdll_callback = NULL;
#endif

/*
 * Define bookeeping for interpreters and devices
 */
typedef struct pl_main_universe_s
{
    gs_memory_t *mem;           /* mem alloc to dealloc devices */
    pl_interp_implementation_t const *const *pdl_implementation;        /* implementations to choose from */
    pl_interp_instance_t *pdl_instance_array[100];      /* parallel to pdl_implementation */
    pl_interp_t *pdl_interp_array[100]; /* parallel to pdl_implementation */
    pl_interp_implementation_t const *curr_implementation;
    pl_interp_instance_t *curr_instance;
    gx_device *curr_device;
} pl_main_universe_t;

/* Include the extern for the device list. */
extern_gs_lib_device_list();

/* Extern for PJL */
extern pl_interp_implementation_t pjl_implementation;

/* Extern for PDL(s): currently in one of: plimpl.c (XL & PCL), */
/* pcimpl.c (PCL only), or pximpl (XL only) depending on make configuration.*/
extern pl_interp_implementation_t const *const pdl_implementation[];    /* zero-terminated list */

/* Define the usage message. */
static const char *pl_usage = "\
Usage: %s [option* file]+...\n\
Options: -dNOPAUSE -E[#] -h -L<PCL|PCLXL> -K<maxK> -l<PCL5C|PCL5E|RTL> -Z...\n\
         -sDEVICE=<dev> -g<W>x<H> -r<X>[x<Y>] -d{First|Last}Page=<#>\n\
         -H<l>x<b>x<r>x<t> -dNOCACHE\n\
         -sOutputFile=<file> (-s<option>=<string> | -d<option>[=<value>])*\n\
         -J<PJL commands>\n";

/* ---------------- Static data for memory management ------------------ */

static gs_gc_root_t device_root;

void pl_print_usage(const pl_main_instance_t *, const char *);

/* ---------------- Forward decls ------------------ */
/* Functions to encapsulate pl_main_universe_t */
/* 0 ok, else -1 error */
int pl_main_universe_init(pl_main_universe_t * universe,        /* universe to init */
                          char *err_str,        /* RETURNS error str if error */
                          gs_memory_t * mem,    /* deallocator for devices */
                          pl_interp_implementation_t const *const pdl_implementation[], /* implementations to choose from */
                          pl_interp_instance_t * pjl_instance,  /* pjl to reference */
                          pl_main_instance_t * inst,    /* instance for pre/post print */
                          pl_page_action_t pl_pre_finish_page,  /* pre-page action */
                          pl_page_action_t pl_post_finish_page  /* post-page action */
    );

/* 0 ok, else -1 error */
int pl_main_universe_dnit(pl_main_universe_t * universe,        /* universe to dnit */
                          char *err_str /* RETRUNS errmsg if error return */
    );

/* rets current interp_instance, 0 if err */
pl_interp_instance_t *pl_main_universe_select(pl_main_universe_t * universe,     /* universe to select from */
                                              char *err_str,     /* RETURNS error str if error */
                                              pl_interp_instance_t * pjl_instance,       /* pjl */
                                              pl_interp_implementation_t const *desired_implementation,  /* impl to select */
                                              pl_main_instance_t * pti,  /* inst contains device */
                                              gs_param_list * params     /* device params to use */
    );

static pl_interp_implementation_t const *pl_auto_sense(const char *name,        /* stream  */
                                                       int buffer_length,       /* length of stream */
                                                       pl_interp_implementation_t const *const impl_array[]     /* implementations to choose from */
    );

static pl_interp_implementation_t const
    *pl_select_implementation(pl_interp_instance_t * pjl_instance,
                              pl_main_instance_t * pmi, pl_top_cursor_t r);

/* Process the options on the command line. */
static FILE *pl_main_arg_fopen(const char *fname, void *ignore_data);

/* Initialize the instance parameters. */
void pl_main_init_instance(pl_main_instance_t * pmi, gs_memory_t * memory);

void pl_main_reinit_instance(pl_main_instance_t * pmi);

/* Process the options on the command line, including making the
   initial device and setting its parameters.  */
int pl_main_process_options(pl_main_instance_t * pmi, arg_list * pal,
                            gs_c_param_list * params,
                            pl_interp_instance_t * pjl_instance,
                            pl_interp_implementation_t const *const
                            impl_array[], char **filename);

/* Find default language implementation */
pl_interp_implementation_t const *pl_auto_sense(const char *buf, int buf_len,
                                                pl_interp_implementation_t
                                                const *const impl_array[]);

static pl_interp_implementation_t const *pl_pjl_select(pl_interp_instance_t *
                                                       pjl_instance,
                                                       pl_interp_implementation_t
                                                       const *const
                                                       impl_array[]);

/* Pre-page portion of page finishing routine */
int                             /* ret 0 if page should be printed, 1 if no print, else -ve error */
    pl_pre_finish_page(pl_interp_instance_t * interp, void *closure);

/* Post-page portion of page finishing routine */
int                             /* ret 0, else -ve error */
    pl_post_finish_page(pl_interp_instance_t * interp, void *closure);

      /* -------------- Read file cursor operations ---------- */
/* Open a read cursor w/specified file */
int pl_main_cursor_open(const gs_memory_t *, pl_top_cursor_t *, const char *,
                        byte *, unsigned);

#ifdef DEBUG
/* Refill from input, avoid extra call level for efficiency */
int pl_main_cursor_next(pl_top_cursor_t * cursor);
#else
#define pl_main_cursor_next(curs) (pl_top_cursor_next(curs))
#endif

/* Read back curr file position */
long pl_main_cursor_position(pl_top_cursor_t * cursor);

/* Close read cursor */
void pl_main_cursor_close(pl_top_cursor_t * cursor);

/* return index in gs device list -1 if not found */
static inline int
get_device_index(const gs_memory_t * mem, const char *value)
{
    const gx_device *const *dev_list;
    int num_devs = gs_lib_device_list(&dev_list, NULL);
    int di;

    for (di = 0; di < num_devs; ++di)
        if (!strcmp(gs_devicename(dev_list[di]), value))
            break;
    if (di == num_devs) {
        errprintf(mem, "Unknown device name %s.\n", value);
        return -1;
    }
    return di;
}

static int
close_job(pl_main_universe_t * universe, pl_main_instance_t * pti)
{
    return pl_dnit_job(universe->curr_instance);
}

#ifdef _Windows
GSDLLEXPORT int GSDLLAPI
pl_wchar_to_utf8(char *out, const void *in)
{
    return wchar_to_utf8(out, in);
}
#endif

GSDLLEXPORT int GSDLLAPI
pl_program_family_name(char **str)
{
    *str = (char *)gs_program_family_name();
    return 0;
}

/* ----------- Command-line driver for pl_interp's  ------ */
/*
 * Here is the real main program.
 */
GSDLLEXPORT int GSDLLAPI
pl_main_aux(int argc, char *argv[], void *disp)
{
    gs_memory_t *mem;
    gs_memory_t *pjl_mem;
    pl_main_instance_t inst;
    arg_list args;
    char *filename = NULL;
    char err_buf[256];
    pl_interp_t *pjl_interp;
    pl_interp_instance_t *pjl_instance;
    pl_main_universe_t universe;
    pl_interp_instance_t *curr_instance = 0;
    gs_c_param_list params;
    int (*arg_get_codepoint) (FILE * file, const char **astr) = NULL;
    int code = 0;

    mem = pl_alloc_init();

    pl_platform_init(mem->gs_lib_ctx->fstdout);


    pjl_mem = mem;

    gs_lib_init1(pjl_mem);

    /* Create a memory allocator to allocate various states from */
    {
        /*
         * gs_iodev_init has to be called here (late), rather than
         * with the rest of the library init procedures, because of
         * some hacks specific to MS Windows for patching the
         * stdxxx IODevices.
         */
        extern void gs_iodev_init(gs_memory_t *);

        gs_iodev_init(pjl_mem);
    }

    /* Init the top-level instance */
    gs_c_param_list_write(&params, pjl_mem);
    gs_param_list_set_persistent_keys((gs_param_list *) & params, false);
    pl_main_init_instance(&inst, mem);
#if defined(__WIN32__) && !defined(GS_NO_UTF8)
    arg_get_codepoint = gp_local_arg_encoding_get_codepoint;
#endif
    arg_init(&args, (const char **)argv, argc, pl_main_arg_fopen, NULL,
             arg_get_codepoint, mem);

    /* Create PJL instance */
    if (pl_allocate_interp(&pjl_interp, &pjl_implementation, pjl_mem) < 0
        || pl_allocate_interp_instance(&pjl_instance, pjl_interp,
                                       pjl_mem) < 0) {
        errprintf(mem, "Unable to create PJL interpreter.");
        return -1;
    }

    /* Create PDL instances, etc */
    if (pl_main_universe_init(&universe, err_buf, mem, pdl_implementation,
                              pjl_instance, &inst, &pl_pre_finish_page,
                              &pl_post_finish_page) < 0) {
        errprintf(mem, "%s", err_buf);
        code = -1;
        goto done;
    }
#ifdef DEBUG
    if (gs_debug_c(':'))
        pl_print_usage(&inst, "Start");
#endif

    /* ------ Begin Main LOOP ------- */
    for (;;) {
        /* Process one input file. */
        /* for debugging we test the parser with a small 256 byte
           buffer - for production systems use 8192 bytes */
#ifdef DEBUG
        byte buf[1 << 9];
#else
        byte buf[1 << 13];
#endif
        pl_top_cursor_t r;
        int code = 0;
        bool in_pjl = true;
        bool new_job = false;

        if (pl_init_job(pjl_instance) < 0) {
            errprintf(mem, "Unable to init PJL job.\n");
            code = -1;
            goto done;
        }

        /* Process any new options. May request new device. */
        if (argc == 1 ||
            pl_main_process_options(&inst,
                                    &args,
                                    &params,
                                    pjl_instance, pdl_implementation,
                                    &filename) < 0) {
            /* Print error verbage and return */
            int i;
            const gx_device **dev_list;
            int num_devs =
                gs_lib_device_list((const gx_device * const **)&dev_list,
                                   NULL);
            errprintf(mem, pl_usage, argv[0]);

            if (pl_characteristics(&pjl_implementation)->version)
                errprintf(mem, "Version: %s\n",
                          pl_characteristics(&pjl_implementation)->version);
            if (pl_characteristics(&pjl_implementation)->build_date)
                errprintf(mem, "Build date: %s\n",
                          pl_characteristics(&pjl_implementation)->
                          build_date);
            errprintf(mem, "Devices:");
            for (i = 0; i < num_devs; ++i) {
                if (((i + 1)) % 9 == 0)
                    errprintf(mem, "\n");
                errprintf(mem, " %s", gs_devicename(dev_list[i]));
            }
            errprintf(mem, "\n");

            code = -1;
            goto done;
        }

        if (!filename)
            break;              /* no nore files to process */

        /* If the display device is selected (default), set up the callback */
        if (strcmp(inst.device->dname, "display") == 0) {
            gx_device_display *ddev;

            if (!disp) {
                errprintf(mem,
                          "Display device selected, but no display device configured.\n");
                return -1;
            }
            ddev = (gx_device_display *) inst.device;
            ddev->callback = (display_callback *) disp;
        }

        /* open file for reading - NB we should respect the minimum
           requirements specified by each implementation in the
           characteristics structure */
        if (pl_main_cursor_open(mem, &r, filename, buf, sizeof(buf)) < 0) {
            errprintf(mem, "Unable to open %s for reading.\n", filename);
            code = -1;
            goto done;
        }

#ifdef DEBUG
        if (gs_debug_c(':'))
            dmprintf1(mem, "%% Reading %s:\n", filename);
#endif
        /* pump data thru PJL/PDL until EOD or error */
        new_job = false;
        in_pjl = true;
        for (;;) {
            if_debug1m('I', mem, "[i][file pos=%ld]\n",
                       pl_main_cursor_position(&r));
            /* end of data - if we are not back in pjl the job has
               ended in the middle of the data stream. */
            if (pl_main_cursor_next(&r) <= 0) {
                if_debug0m('I', mem, "End of of data\n");
                if (!in_pjl) {
                    if_debug0m('I', mem,
                               "end of data stream found in middle of job\n");
                    pl_process_eof(curr_instance);
                    if (close_job(&universe, &inst) < 0) {
                        dmprintf(mem, "Unable to deinit PDL job.\n");
                        code = -1;
                        goto done;
                    }
                }
                break;
            }
            if (in_pjl) {
                if_debug0m('I', mem, "Processing pjl\n");
                code = pl_process(pjl_instance, &r.cursor);
                if (code == e_ExitLanguage) {
                    if_debug0m('I', mem, "Exiting pjl\n");
                    in_pjl = false;
                    new_job = true;
                }
            }
            if (new_job) {
                if (mem->gs_lib_ctx->gs_next_id > 0xFF000000) {
                    dmprintf(mem, "Once a year reset the gs_next_id.\n");
                    code = -1;
                    goto done;
                }

                if_debug0m('I', mem, "Selecting PDL\n");
                curr_instance = pl_main_universe_select(&universe, err_buf,
                                                        pjl_instance,
                                                        pl_select_implementation
                                                        (pjl_instance, &inst,
                                                         r), &inst,
                                                        (gs_param_list *) &
                                                        params);
                if (curr_instance == NULL) {
                    errprintf(mem, "%s", err_buf);
                    code = -1;
                    goto done;
                }

                if (pl_init_job(curr_instance) < 0) {
                    dmprintf(mem, "Unable to init PDL job.\n");
                    code = -1;
                    goto done;
                }
                if_debug1m('I', mem, "selected and initializing (%s)\n",
                           pl_characteristics(curr_instance->interp->
                                              implementation)->language);
                new_job = false;
            }
            if (curr_instance) {

                /* Special case when the job resides in a seekable file and
                   the implementation has a function to process a file at a
                   time. */
                if (curr_instance->interp->implementation->proc_process_file
                    && r.strm != mem->gs_lib_ctx->fstdin) {
                    if_debug1m('I', mem, "processing job from file (%s)\n",
                               filename);
                    code = pl_process_file(curr_instance, filename);
                    if (code < 0) {
                        dmprintf1(mem,
                                  "Warning interpreter exited with error code %d\n",
                                  code);
                    }
                    if (close_job(&universe, &inst) < 0) {
                        dmprintf(mem, "Unable to deinit PJL.\n");
                        return -1;
                    }
                    if_debug0m('I', mem,
                               "exiting job and proceeding to next file\n");
                    break;      /* break out of the loop to process the next file */
                }

                code = pl_process(curr_instance, &r.cursor);
                if_debug1m('I', mem, "processing (%s) job\n",
                           pl_characteristics(curr_instance->interp->
                                              implementation)->language);
                if (code == e_ExitLanguage) {
                    in_pjl = true;
                    if_debug1m('I', mem, "exiting (%s) job back to pjl\n",
                               pl_characteristics(curr_instance->interp->
                                                  implementation)->language);
                    if (close_job(&universe, &inst) < 0) {
                        dmprintf(mem, "Unable to deinit PDL job.\n");
                        code = -1;
                        goto done;
                    }
                    if (pl_init_job(pjl_instance) < 0) {
                        dmprintf(mem, "Unable to init PJL job.\n");
                        code = -1;
                        goto done;
                    }
                    pl_renew_cursor_status(&r);
                } else if (code < 0) {  /* error and not exit language */
                    int show = 1;
                    dmprintf1(mem,
                              "Warning interpreter exited with error code %d\n",
                              code);
                    dmprintf(mem, "Flushing to end of job\n");
                    /* flush eoj may require more data */
                    while ((pl_flush_to_eoj(curr_instance, &r.cursor)) == 0) {
                        if (show) {
                            if_debug1m('I', mem, "flushing to eoj for (%s) job\n",
                                       pl_characteristics(curr_instance->interp->
                                                          implementation)->
                                       language);
                            show  = 0;
                        }
                        if (pl_main_cursor_next(&r) <= 0) {
                            if_debug0m('I', mem,
                                       "end of data found while flushing\n");
                            break;
                        }
                    }
                    pl_report_errors(curr_instance, code,
                                     pl_main_cursor_position(&r),
                                     inst.error_report > 0);
                    if (close_job(&universe, &inst) < 0) {
                        dmprintf(mem, "Unable to deinit PJL.\n");
                        code = -1;
                        goto done;
                    }
                    /* Print PDL status if applicable, then dnit PDL job */
                    code = 0;
                    new_job = true;
                    /* go back to pjl */
                    in_pjl = true;
                }
            }
        }
        pl_main_cursor_close(&r);
    }

    /* ----- End Main loop ----- */
    /* If we were accumulating saved-pages, print them now */
    if (inst.saved_pages_test_mode) {
        gx_device *pdev = inst.device;

        if (gx_saved_pages_param_process((gx_device_printer *)pdev,
                                  (byte *)"print normal flush", 18) < 0) {
            errprintf(mem, "Unable to print saved-pages.\n");
            code = -1;
            goto done;
        }
     }
    /* release param list */
    gs_c_param_list_release(&params);
    /* Dnit PDLs */
    if (pl_main_universe_dnit(&universe, err_buf)) {
        errprintf(mem, "%s", err_buf);
        code = -1;
        goto done;
    }
    /* dnit pjl */
    if (pl_deallocate_interp_instance(pjl_instance) < 0
        || pl_deallocate_interp(pjl_interp) < 0) {
        dmprintf(mem, "Unable to close out PJL instance.\n");
        code = -1;
        goto done;
    }

    /* We lost the ability to print peak memory usage with the loss
     * of the memory wrappers.
     */
    arg_finit(&args);

    if (gs_debug_c('A'))
        dmprintf(mem, "Final time");
    pl_platform_dnit(0);
    pl_alloc_finit(mem);
done:
    return code;
}

GSDLLEXPORT int GSDLLAPI
pl_main(int argc, char *argv[]
    )
{
    return pl_main_aux(argc, argv, NULL);
}

/* --------- Functions operating on pl_main_universe_t ----- */
/* Init main_universe from pdl_implementation */
int                             /* 0 ok, else -1 error */
pl_main_universe_init(pl_main_universe_t * universe,    /* universe to init */
                      char *err_str,    /* RETURNS error str if error */
                      gs_memory_t * mem,        /* deallocator for devices */
                      pl_interp_implementation_t const *const pdl_implementation[],     /* implementations to choose from */
                      pl_interp_instance_t * pjl_instance,      /* pjl to
                                                                   reference */
                      pl_main_instance_t * inst,        /* instance for pre/post print */
                      pl_page_action_t pl_pre_finish_page,      /* pre-page action */
                      pl_page_action_t pl_post_finish_page      /* post-page action */
    )
{
    int index;

    /* 0-init everything */
    memset(universe, 0, sizeof(*universe));
    universe->pdl_implementation = pdl_implementation;
    universe->mem = mem;
    mem->gs_lib_ctx->top_of_system = universe;
    inst->device_memory = mem;

    /* Create & init PDL all instances. Could do this lazily to save memory, */
    /* but for now it's simpler to just create all instances up front. */
    for (index = 0; pdl_implementation[index] != 0; ++index) {
        pl_interp_instance_t *instance;
        int code;

        code = pl_allocate_interp(&universe->pdl_interp_array[index],
                                  pdl_implementation[index], mem);
        if (code >= 0) {
            /* Whatever instance we allocate here will become the current
             * instance during initialisation; this allows init files to be
             * successfully read etc. */
            code = pl_allocate_interp_instance(&universe->curr_instance,
                                               universe->
                                               pdl_interp_array[index], mem);
        }
        universe->pdl_instance_array[index] = universe->curr_instance;
        if (code < 0) {
            if (err_str)
                gs_sprintf(err_str, "Unable to create %s interpreter.\n",
                        pl_characteristics(pdl_implementation[index])->
                        language);
            goto pmui_err;
        }

        instance = universe->pdl_instance_array[index];
        if (pl_set_client_instance(instance, pjl_instance, PJL_CLIENT) < 0 ||
            pl_set_client_instance(instance, universe->pdl_instance_array[0],
                                   PCL_CLIENT)
            || pl_set_pre_page_action(instance, pl_pre_finish_page, inst) < 0
            || pl_set_post_page_action(instance, pl_post_finish_page,
                                       inst) < 0
            || pl_get_device_memory(instance, &inst->device_memory) < 0) {
            if (err_str)
                gs_sprintf(err_str, "Unable to init %s interpreter.\n",
                        pl_characteristics(pdl_implementation[index])->
                        language);
            goto pmui_err;
        }
    }
    return 0;

  pmui_err:
    pl_main_universe_dnit(universe, 0);
    return -1;
}

pl_interp_instance_t *
get_interpreter_from_memory(const gs_memory_t * mem)
{
    pl_main_universe_t *universe =
        (pl_main_universe_t *) mem->gs_lib_ctx->top_of_system;
    return universe->curr_instance;
}

/* Undo pl_main_universe_init */
int                             /* 0 ok, else -1 error */
pl_main_universe_dnit(pl_main_universe_t * universe,    /* universe to dnit */
                      char *err_str     /* RETRUNS errmsg if error return */
    )
{
    int index;

    /* Deselect last-selected device */
    if (universe->curr_instance
        && pl_remove_device(universe->curr_instance) < 0) {
        if (err_str)
            gs_sprintf(err_str, "Unable to close out PDL instance.\n");
        return -1;
    }

    /* dnit interps */
    for (index = 0;
         universe->pdl_implementation[index] != 0;
         ++index, universe->curr_instance =
         universe->pdl_instance_array[index])
        if ((universe->pdl_instance_array[index]
             && pl_deallocate_interp_instance(universe->
                                              pdl_instance_array[index]) < 0)
            || (universe->pdl_interp_array[index]
                && pl_deallocate_interp(universe->pdl_interp_array[index]) <
                0)) {
            if (err_str)
                gs_sprintf(err_str, "Unable to close out %s instance.\n",
                        pl_characteristics(universe->
                                           pdl_implementation[index])->
                        language);
            return -1;
        }

    /* close and dealloc the device if selected. */
    if (universe->curr_device) {
        gs_closedevice(universe->curr_device);
        gs_unregister_root(universe->curr_device->memory, &device_root,
                           "pl_main_universe_select");
        gx_device_retain(universe->curr_device, false);
        universe->curr_device = NULL;
    }

    return 0;
}

/* Select new device and/or implementation, deselect one one (opt) */
pl_interp_instance_t *          /* rets current interp_instance, 0 if err */
pl_main_universe_select(pl_main_universe_t * universe,  /* universe to select from */
                        char *err_str,  /* RETURNS error str if error */
                        pl_interp_instance_t * pjl_instance,    /* pjl */
                        pl_interp_implementation_t const *desired_implementation,       /* impl to select */
                        pl_main_instance_t * pti,       /* inst contains device */
                        gs_param_list * params  /* device params to set */
    )
{
    int params_are_set = 0;
    /* requesting the device in the main instance */
    gx_device *desired_device = pti->device;

    /* If new interpreter/device is different, deselect it from old interp */
    if ((universe->curr_implementation
         && universe->curr_implementation != desired_implementation)
        || (universe->curr_device && universe->curr_device != desired_device)) {
        if (universe->curr_instance
            && pl_remove_device(universe->curr_instance) < 0) {
            if (err_str)
                strcpy(err_str,
                       "Unable to deselect device from interp instance.\n");
            return 0;
        }
        if (universe->curr_device && universe->curr_device != desired_device) {
            /* Here, we close the device. Note that this is not an absolute */
            /* requirement: we could have a pool of open devices & select them */
            /* into interp_instances as needed. The reason we force a close */
            /* here is that multiple *async* devices would need coordination */
            /* since an async device is not guaranteed to have completed */
            /* rendering until it is closed. So, we close devices here to */
            /* avoid things like intevermingling of output streams. */
            if (gs_closedevice(universe->curr_device) < 0) {
                if (err_str)
                    strcpy(err_str, "Unable to close device.\n");
                return 0;
            } else {
                /* Delete the device. */
                gs_unregister_root(universe->curr_device->memory,
                                   &device_root, "pl_main_universe_select");
                gs_free_object(universe->curr_device->memory,
                               universe->curr_device,
                               "pl_main_universe_select(gx_device)");
                universe->curr_device = 0;
            }
        }
    }

    /* Switch to/select new interperter if indicated. */
    /* Here, we assume that instances of all interpreters are open & ready */
    /* to go. If memory were scarce, we could dynamically destroy/create */
    /* interp_instances here (or even de/init the entire interp for greater */
    /* memory savings). */
    if ((!universe->curr_implementation
         || universe->curr_implementation != desired_implementation)
        || !universe->curr_device) {
        int index;

        /* Select/change PDL if needed */
        if (!universe->curr_implementation
            || universe->curr_implementation != desired_implementation) {
            /* find instance corresponding to implementation */
            for (index = 0;
                 desired_implementation !=
                 universe->pdl_implementation[index]; ++index);
            universe->curr_instance = universe->pdl_instance_array[index];
            universe->curr_implementation = desired_implementation;
        }

        /* Open a new device if needed. */
        if (!universe->curr_device) {   /* remember that curr_device==0 if we closed it above */
            /* Set latest params into device BEFORE setting into device. */
            /* Do this here because PCL5 will do some 1-time initializations based */
            /* on device geometry when pl_set_device, below, selects the device. */
            if (gs_putdeviceparams(desired_device, params) < 0) {
                strcpy(err_str, "Unable to set params into device.\n");
                return 0;
            }
            params_are_set = 1;

            if (gs_opendevice(desired_device) < 0) {
                if (err_str)
                    strcpy(err_str, "Unable to open new device.\n");
                return 0;
            } else
                universe->curr_device = desired_device;
        }

        /* NB fix me, these parameters should not be passed this way */
        universe->curr_instance->pcl_personality = pti->pcl_personality;
        universe->curr_instance->interpolate = pti->interpolate;
        universe->curr_instance->nocache = pti->nocache;
        universe->curr_instance->page_set_on_command_line =
            pti->page_set_on_command_line;
        universe->curr_instance->res_set_on_command_line =
            pti->res_set_on_command_line;
        universe->curr_instance->high_level_device = pti->high_level_device;
        universe->curr_instance->scanconverter = pti->scanconverter;
        universe->curr_instance->piccdir = pti->piccdir;
        universe->curr_instance->pdefault_gray_icc = pti->pdefault_gray_icc;
        universe->curr_instance->pdefault_rgb_icc = pti->pdefault_rgb_icc;
        universe->curr_instance->pdefault_cmyk_icc = pti->pdefault_cmyk_icc;


        /* Select curr/new device into PDL instance */
        if (pl_set_device(universe->curr_instance, universe->curr_device) < 0) {
            if (err_str)
                strcpy(err_str,
                       "Unable to install device into PDL interp.\n");
            return 0;
        }
    }

    /* Set latest params into device. Write them all in case any changed */
    if (!params_are_set
        && gs_putdeviceparams(universe->curr_device, params) < 0) {
        strcpy(err_str, "Unable to set params into device.\n");
        return 0;
    }
    return universe->curr_instance;
}

/* ------- Functions related to pl_main_instance_t ------ */

/* Initialize the instance parameters. */
void
pl_main_init_instance(pl_main_instance_t * pti, gs_memory_t * mem)
{
    pti->memory = mem;
    {
        int i;

        for (i = 0; i < countof(pti->spaces.memories.indexed); ++i)
            pti->spaces.memories.indexed[i] = 0;
        pti->spaces.memories.named.local =
            pti->spaces.memories.named.global = (gs_ref_memory_t *) mem;
    }

    pti->error_report = -1;
    pti->pause = true;
    pti->device = 0;
    pti->implementation = 0;
    gp_get_realtime(pti->base_time);
    pti->page_count = 0;
    pti->interpolate = false;
    pti->nocache = false;
    pti->page_set_on_command_line = false;
    pti->res_set_on_command_line = false;
    pti->high_level_device = false;
    pti->saved_pages_test_mode = false;
    pti->scanconverter = GS_SCANCONVERTER_DEFAULT;
    pti->piccdir = NULL;
    pti->pdefault_gray_icc = NULL;
    pti->pdefault_rgb_icc = NULL;
    pti->pdefault_cmyk_icc = NULL;
    strncpy(&pti->pcl_personality[0], "PCL",
            sizeof(pti->pcl_personality) - 1);
}

/* -------- Command-line processing ------ */

/* Create a default device if not already defined. */
static int
pl_top_create_device(pl_main_instance_t * pti, int index, bool is_default)
{
    int code = 0;

    if (!is_default || !pti->device) {
        const gx_device *dev;
        /* We assume that nobody else changes pti->device,
           and this function is called from this module only.
           Due to that device_root is always consistent with pti->device,
           and it is regisrtered if and only if pti->device != NULL.
         */
        if (pti->device != NULL) {
            pti->device = NULL;
            gs_unregister_root(pti->device_memory, &device_root,
                               "pl_main_universe_select");
        }

        if (index == -1) {
            dev = gs_getdefaultlibdevice(pti->device_memory);
        }
        else {
            const gx_device **list;
            gs_lib_device_list((const gx_device * const **)&list, NULL);
            dev = list[index];
        }
        code = gs_copydevice(&pti->device, dev, pti->device_memory);

        if (pti->device != NULL)
            gs_register_struct_root(pti->device_memory, &device_root,
                                    (void **)&pti->device,
                                    "pl_top_create_device");
        /* NB this needs a better solution */
        if (!strcmp(gs_devicename(pti->device), "pdfwrite") ||
            !strcmp(gs_devicename(pti->device), "ps2write"))
            pti->high_level_device = true;
    }
    return code;
}

/* Process the options on the command line. */
static FILE *
pl_main_arg_fopen(const char *fname, void *ignore_data)
{
    return gp_fopen(fname, "r");
}

static void
set_debug_flags(const char *arg, char *flags)
{
    byte value = (*arg == '-' ? (++arg, 0) : 0xff);

    while (*arg)
        flags[*arg++ & 127] = value;
}

/*
 * scan floats delimited by spaces, tabs and/or 'x'.  Return the
 * number of arguments parsed which will be less than or equal to the
 * number of arguments requested in the parameter arg_count.
 */
static int
parse_floats(gs_memory_t * mem, uint arg_count, char *arg, float *f)
{
    int float_index = 0;
    char *tok, *l = NULL;
    /* copy the input because strtok() steps on the string */
    char *s = arg_copy(arg, mem);
    if (s == NULL)
        return -1;
    
    /* allow 'x', tab or spaces to delimit arguments */
    tok = gs_strtok(s, " \tx", &l);
    while (tok != NULL && float_index < arg_count) {
        f[float_index++] = atof(tok);
        tok = gs_strtok(NULL, " \tx", &l);
    }

    gs_free_object(mem, s, "parse_floats()");

    /* return the number of args processed */
    return float_index;
}

static int check_for_special_int(pl_main_instance_t * pmi, const char *arg, int b)
{
    if (!strncmp(arg, "BATCH", 5))
        return (b == 0) ? 0 : gs_note_error(gs_error_rangecheck);
    if (!strncmp(arg, "NOPAUSE", 6)) {
        pmi->pause = !b;
        return 0;
    }
    if (!strncmp(arg, "DOINTERPOLATE", 13)) {
        pmi->interpolate = !!b;
        return 0;
    }
    if (!strncmp(arg, "NOCACHE", 7)) {
        pmi->nocache = !!b;
        return 0;
    }
    if (!strncmp(arg, "SCANCONVERTERTYPE", 17)) {
        pmi->scanconverter = b;
        return 0;
    }
    return 1;
}

static int check_for_special_float(pl_main_instance_t * pmi, const char *arg, float f)
{
    if (!strncmp(arg, "BATCH", 5) ||
        !strncmp(arg, "NOPAUSE", 6) ||
        !strncmp(arg, "DOINTERPOLATE", 13) ||
        !strncmp(arg, "NOCACHE", 7) ||
        !strncmp(arg, "SCANCONVERTERTYPE", 17)) {
        return gs_note_error(gs_error_rangecheck);
    }
    return 1;
}

static int check_for_special_str(pl_main_instance_t * pmi, const char *arg, gs_param_string *f)
{
    if (!strncmp(arg, "BATCH", 5) ||
        !strncmp(arg, "NOPAUSE", 6) ||
        !strncmp(arg, "DOINTERPOLATE", 13) ||
        !strncmp(arg, "NOCACHE", 7) ||
        !strncmp(arg, "SCANCONVERTERTYPE", 17)) {
        return gs_note_error(gs_error_rangecheck);
    }
    return 1;
}

#define arg_heap_copy(str) arg_copy(str, pmi->memory)
int
pl_main_process_options(pl_main_instance_t * pmi, arg_list * pal,
                        gs_c_param_list * params,
                        pl_interp_instance_t * pjl_instance,
                        pl_interp_implementation_t const *const impl_array[],
                        char **filename)
{
    int code = 0;
    bool help = false;
    char *arg;

    gs_c_param_list_write_more(params);
    while ((code = arg_next(pal, (const char **)&arg, pmi->memory)) > 0 && *arg == '-') { /* just - read from stdin */
        if (arg[1] == '\0')
            break;
        arg += 2;
        switch (arg[-1]) {
            case '-':
                if (strcmp(arg, "debug") == 0) {
                    gs_debug_flags_list(pmi->memory);
                    break;
                } else if (strncmp(arg, "debug=", 6) == 0) {
                    gs_debug_flags_parse(pmi->memory, arg + 6);
                    break;
                } else if (strncmp(arg, "saved-pages=", 12) == 0) {
                    gx_device *pdev = pmi->device;
                    gx_device_printer *ppdev = (gx_device_printer *)pdev;

                    /* open the device if not yet open */
                    if (pdev->is_open == 0 && (code = gs_opendevice(pdev)) < 0) {
                        return code;
                    }
                    if (dev_proc(pdev, dev_spec_op)(pdev, gxdso_supports_saved_pages, NULL, 0) == 0) {
                        errprintf(pmi->memory, "   --saved-pages not supported by the '%s' device.\n",
                                  pdev->dname);
                        return -1;
                    }
                    code = gx_saved_pages_param_process(ppdev, (byte *)arg+12, strlen(arg+12));
                    if (code > 0) {
                        /* erase the page */
                        gx_color_index color;
                        gx_color_value rgb_white[3] = { 65535, 65535, 65535 };
                        gx_color_value cmyk_white[4] = { 0, 0, 0, 0 };

                        if (pdev->color_info.polarity == GX_CINFO_POLARITY_ADDITIVE)
                            color = dev_proc(pdev, map_rgb_color)(pdev, rgb_white);
                        else
                            color = dev_proc(pdev, map_cmyk_color)(pdev, cmyk_white);
                        code = dev_proc(pdev, fill_rectangle)(pdev, 0, 0, pdev->width, pdev->height, color);
                    }
                    if (code < 0)
                        return code;
                    break;
                /* The following code is only to allow regression testing of saved-pages */
                } else if (strncmp(arg, "saved-pages-test", 16) == 0) {
                    gx_device *pdev = pmi->device;

                    if ((code = gs_opendevice(pdev)) < 0)
                        return code;

                    if (dev_proc(pdev, dev_spec_op)(pdev, gxdso_supports_saved_pages, NULL, 0) == 0) {
                        errprintf(pmi->memory, "   --saved-pages-test not supported by the '%s' device.\n",
                                  pdev->dname);
                        break;			/* just ignore it */
                    }
                    code = gx_saved_pages_param_process((gx_device_printer *)pdev, (byte *)"begin", 5);
                    if (code > 0) {
                        /* erase the page */
                        gx_color_index color;
                        gx_color_value rgb_white[3] = { 65535, 65535, 65535 };
                        gx_color_value cmyk_white[4] = { 0, 0, 0, 0 };

                        if (pdev->color_info.polarity == GX_CINFO_POLARITY_ADDITIVE)
                            color = dev_proc(pdev, map_rgb_color)(pdev, rgb_white);
                        else
                            color = dev_proc(pdev, map_cmyk_color)(pdev, cmyk_white);
                        code = dev_proc(pdev, fill_rectangle)(pdev, 0, 0, pdev->width, pdev->height, color);
                    }
                    if (code < 0)
                        return code;
                    pmi->saved_pages_test_mode = true;
                    break;
                }
                /* FALLTHROUGH */
            default:
                dmprintf1(pmi->memory, "Unrecognized switch: %s\n", arg);
                return -1;
            case '\0':
                /* read from stdin - must be last arg */
                continue;
            case 'd':
            case 'D':
                {
                    /* We're setting a device parameter to a non-string value. */
                    char *eqp = strchr(arg, '=');
                    const char *value;
                    int vi;
                    float vf;
                    bool bval = true;
                    char buffer[128];

                    if (eqp || (eqp = strchr(arg, '#')))
                        value = eqp + 1;
                    else {
                        /* -dDefaultBooleanIs_TRUE */
                        code = check_for_special_int(pmi, arg, (int)bval);
                        if (code == 1)
                            code =
                                param_write_bool((gs_param_list *) params,
                                                 arg_heap_copy(arg), &bval);
                        continue;
                    }

                    if (value && value[0] == '/') {
                        gs_param_string str;

                        strncpy(buffer, arg, eqp - arg);
                        buffer[eqp - arg] = '\0';
                        param_string_from_transient_string(str, value + 1);
                        code = check_for_special_str(pmi, arg, &str);
                        if (code == 1)
                            code = param_write_name((gs_param_list *) params,
                                                    arg_heap_copy(buffer), &str);
                        continue;
                    }
                    /* Search for a non-decimal 'radix' number */
                    else if (strchr(value, '#')) {
                        int base, number = 0;
                        char *val = strchr(value, '#');

                        *val++ = 0x00;
                        sscanf(value, "%d", &base);
                        if (base < 2 || base > 36) {
                            dmprintf1(pmi->memory, "Value out of range %s",
                                      value);
                            return -1;
                        }
                        while (*val) {
                            if (*val >= '0' && *val <= '9') {
                                number = number * base + (*val - '0');
                            } else {
                                if (*val >= 'A' && *val <= 'Z') {
                                    number = number * base + (*val - 'A');
                                } else {
                                    if (*val >= 'a' && *val <= 'z') {
                                        number = number * base + (*val - 'a');
                                    } else {
                                        dmprintf1(pmi->memory,
                                                  "Value out of range %s",
                                                  val);
                                        return -1;
                                    }
                                }
                            }
                            val++;
                        }
                        strncpy(buffer, arg, eqp - arg);
                        buffer[eqp - arg] = '\0';
                        code = check_for_special_int(pmi, arg, number);
                        if (code == 1)
                            code =
                                param_write_int((gs_param_list *) params,
                                                arg_heap_copy(buffer), &number);
                    } else if ((!strchr(value, '.')) &&
                               /* search for an int (no decimal), if fail try a float */
                               (sscanf(value, "%d", &vi) == 1)) {
                        /* Here we have an int -- check for a scaling suffix */
                        char suffix = eqp[strlen(eqp) - 1];

                        switch (suffix) {
                            case 'k':
                            case 'K':
                                vi *= 1024;
                                break;
                            case 'm':
                            case 'M':
                                vi *= 1024 * 1024;
                                break;
                            case 'g':
                            case 'G':
                                /* caveat emptor: more than 2g will overflow */
                                /* and really should produce a 'real', so don't do this */
                                vi *= 1024 * 1024 * 1024;
                                break;
                            default:
                                break;  /* not a valid suffix or last char was digit */
                        }
                        /* create a null terminated string for the key */
                        strncpy(buffer, arg, eqp - arg);
                        buffer[eqp - arg] = '\0';
                        code = check_for_special_int(pmi, arg, vi);
                        if (code == 1)
                            code =
                                param_write_int((gs_param_list *) params,
                                                arg_heap_copy(buffer), &vi);
                    } else if (sscanf(value, "%f", &vf) == 1) {
                        /* create a null terminated string.  NB duplicated code. */
                        strncpy(buffer, arg, eqp - arg);
                        buffer[eqp - arg] = '\0';
                        code = check_for_special_float(pmi, arg, vf);
                        if (code == 1)
                            code =
                                param_write_float((gs_param_list *) params,
                                                  arg_heap_copy(buffer), &vf);
                    } else if (!strcmp(value, "true")) {
                        /* bval = true; */
                        strncpy(buffer, arg, eqp - arg);
                        buffer[eqp - arg] = '\0';
                        code = check_for_special_int(pmi, arg, (int)bval);
                        if (code == 1)
                            code =
                                param_write_bool((gs_param_list *) params,
                                                 arg_heap_copy(buffer), &bval);
                    } else if (!strcmp(value, "false")) {
                        bval = false;
                        strncpy(buffer, arg, eqp - arg);
                        buffer[eqp - arg] = '\0';
                        code = check_for_special_int(pmi, arg, (int)bval);
                        if (code == 1)
                            code =
                                param_write_bool((gs_param_list *) params,
                                                 arg_heap_copy(buffer), &bval);
                    } else {
                        dmprintf(pmi->memory,
                                 "Usage for -d is -d<option>=[<integer>|<float>|true|false]\n");
                        continue;
                    }
                }
                break;
            case 'E':
                if (*arg == 0)
                    gs_debug['#'] = 1;
                else
                    sscanf(arg, "%d", &pmi->error_report);
                break;
            case 'g':
                {
                    int geom[2];
                    gs_param_int_array ia;

                    if (sscanf(arg, "%ux%u", &geom[0], &geom[1]) != 2) {
                        dmprintf(pmi->memory,
                                 "-g must be followed by <width>x<height>\n");
                        return -1;
                    }
                    ia.data = geom;
                    ia.size = 2;
                    ia.persistent = false;
                    code =
                        param_write_int_array((gs_param_list *) params,
                                              "HWSize", &ia);
                    pmi->page_set_on_command_line = true;
                }
                break;
            case 'H':
                {
                    float hwmarg[4];
                    gs_param_float_array fa;
                    uint sz = countof(hwmarg);
                    uint parsed = parse_floats(pmi->memory, sz, arg, hwmarg);
                    if (parsed != sz) {
                        dmprintf(pmi->memory,
                                 "-H must be followed by <left>x<bottom>x<right>x<top>\n");
                        return -1;
                    }
                    fa.data = hwmarg;
                    fa.size = parsed;
                    fa.persistent = false;
                    code =
                        param_write_float_array((gs_param_list *) params,
                                              ".HWMargins", &fa);
                }
                break;
            case 'm':
                {
                    float marg[2];
                    gs_param_float_array fa;
                    uint sz = countof(marg);
                    uint parsed = parse_floats(pmi->memory, sz, arg, marg);
                    if (parsed != sz) {
                        dmprintf(pmi->memory,
                                 "-m must be followed by <left>x<bottom>\n");
                        return -1;
                    }
                    fa.data = marg;
                    fa.size = parsed;
                    fa.persistent = false;
                    code =
                        param_write_float_array((gs_param_list *) params,
                                                "Margins", &fa);

                }
                break;
            case 'h':
                help = true;
                goto out;
                /* job control line follows - PJL */
            case 'j':
            case 'J':
                /* set up the read cursor and send it to the pjl parser */
                {
                    stream_cursor_read cursor;
                    /* PJL lines have max length of 80 character + null terminator */
                    byte buf[512];
                    /* length of arg + newline (expected by PJL parser) + null */
                    int buf_len = strlen(arg) + 2;
                    if ((buf_len) > sizeof(buf)) {
                        dmprintf(pmi->memory, "pjl sequence too long\n");
                        return -1;
                    }

                    /* replace ";" with newline for the PJL parser and
                       concatenate NULL. */
                    {
                        int i;

                        for (i = 0; i < buf_len - 2; i++) {
                            if (arg[i] == ';')
                                buf[i] = '\n';
                            else
                                buf[i] = arg[i];
                        }
                        buf[i] = '\n';
                        i++;
                        buf[i] = '\0';
                    }
                    /* starting pos for pointer is always one position back */
                    cursor.ptr = buf - 1;
                    /* set the end of data pointer */
                    cursor.limit = cursor.ptr + buf_len;
                    /* process the pjl */
                    code = pl_process(pjl_instance, &cursor);

                    /* we expect the language to exit properly otherwise
                       there was some sort of problem */
                    if (code != e_ExitLanguage)
                        return (code == 0 ? -1 : code);
                    else
                        code = 0;
                }
                break;
            case 'K':          /* max memory in K */
                {
                    int maxk;
#ifdef HEAP_ALLOCATOR_ONLY
                    gs_malloc_memory_t *rawheap =
                        (gs_malloc_memory_t *)
                        gs_malloc_wrapped_contents(pmi->memory);
#else
                    gs_malloc_memory_t *rawheap =
                        (gs_malloc_memory_t *) gs_memory_chunk_target(pmi->
                                                                      memory)->
                        non_gc_memory;
#endif
                    if (sscanf(arg, "%d", &maxk) != 1) {
                        dmprintf(pmi->memory,
                                 "-K must be followed by a number\n");
                        return -1;
                    }
                    rawheap->limit = (long)maxk << 10;
                }
                break;
            case 'o':
                {
                    const char *adef;
                    gs_param_string str;

                    if (arg[0] == 0) {
                        code = arg_next(pal, (const char **)&adef, pmi->memory);
                        if (code < 0)
                            break;
                    } else
                        adef = arg;
                    param_string_from_transient_string(str, adef);
                    code =
                        param_write_string((gs_param_list *) params,
                                           "OutputFile", &str);
                    pmi->pause = false;
                    break;
                }
            case 'L':          /* language */
                {
                    int index;

                    for (index = 0; impl_array[index] != 0; ++index)
                        if (!strcmp(arg,
                                    pl_characteristics(impl_array[index])->
                                    language))
                            break;
                    if (impl_array[index] != 0)
                        pmi->implementation = impl_array[index];
                    else {
                        dmprintf(pmi->memory,
                                 "Choose language in -L<language> from: ");
                        for (index = 0; impl_array[index] != 0; ++index)
                            dmprintf1(pmi->memory, "%s ",
                                      pl_characteristics(impl_array[index])->
                                      language);
                        dmprintf(pmi->memory, "\n");
                        return -1;
                    }
                    break;
                }
            case 'l':          /* personality */
                {
                    if (!strcmp(arg, "RTL") || !strcmp(arg, "PCL5E") ||
                        !strcmp(arg, "PCL5C"))
                        strcpy(pmi->pcl_personality, arg);
                    else
                        dmprintf(pmi->memory,
                                 "PCL personality must be RTL, PCL5E or PCL5C\n");
                }
                break;
            case 'r':
                {
                    float res[2];
                    gs_param_float_array fa;
                    uint sz = countof(res);
                    uint parsed = parse_floats(pmi->memory, sz, arg, res);
                    switch (parsed) {
                        default:
                            dmprintf(pmi->memory,
                                     "-r must be followed by <res> or <xres>x<yres>\n");
                            return -1;
                        case 1:        /* -r<res> */
                            res[1] = res[0];
                        case 2:        /* -r<xres>x<yres> */
                            ;
                    }
                    fa.data = res;
                    fa.size = sz;
                    fa.persistent = false;
                    code =
                        param_write_float_array((gs_param_list *) params,
                                                "HWResolution", &fa);
                    if (code == 0)
                        pmi->res_set_on_command_line = true;
                }
                break;
            case 's':
            case 'S':
                {               /* We're setting a device or user parameter to a string. */
                    char *eqp;
                    const char *value;
                    gs_param_string str;

                    eqp = strchr(arg, '=');
                    if (!(eqp || (eqp = strchr(arg, '#')))) {
                        dmprintf(pmi->memory,
                                 "Usage for -s is -s<option>=<string>\n");
                        return -1;
                    }
                    value = eqp + 1;
                    if (!strncmp(arg, "DEVICE", 6)) {
                        int code = pl_top_create_device(pmi,
                                                        get_device_index(pmi->
                                                                         memory,
                                                                         value),
                                                        false);

                        if (code < 0)
                            return code;
                        /* check for icc settings */
                    } else
                        if (!strncmp
                            (arg, "DefaultGrayProfile",
                             strlen("DefaultGrayProfile"))) {
                        pmi->pdefault_gray_icc = arg_heap_copy(value);
                    } else
                        if (!strncmp
                            (arg, "DefaultRGBProfile",
                             strlen("DefaultRGBProfile"))) {
                        pmi->pdefault_rgb_icc = arg_heap_copy(value);
                    } else
                        if (!strncmp
                            (arg, "DefaultCMYKProfile",
                             strlen("DefaultCMYKProfile"))) {
                        pmi->pdefault_cmyk_icc = arg_heap_copy(value);
                    } else
                        if (!strncmp
                            (arg, "ICCProfileDir", strlen("ICCProfileDir"))) {
                        pmi->piccdir = arg_heap_copy(value);
                    } else {
                        char buffer[128];

                        strncpy(buffer, arg, eqp - arg);
                        buffer[eqp - arg] = '\0';
                        param_string_from_transient_string(str, value);
                        code =
                            param_write_string((gs_param_list *) params,
                                               buffer, &str);
                    }
                }
                break;
            case 'Z':
                set_debug_flags(arg, gs_debug);
                break;
        }
    }
  out:if (help) {
        arg_finit(pal);
        gs_c_param_list_release(params);
        return -1;
    }
    gs_c_param_list_read(params);
    pl_top_create_device(pmi, -1, true); /* create default device if needed */

    /* The last argument wasn't a switch filename else NULL */
    *filename = arg;
    return 0;
}

/* either the (1) implementation has been selected on the command line or
   (2) it has been selected in PJL or (3) we need to auto sense. */
static pl_interp_implementation_t const *
pl_select_implementation(pl_interp_instance_t * pjl_instance,
                         pl_main_instance_t * pmi, pl_top_cursor_t r)
{
    /* Determine language of file to interpret. We're making the incorrect */
    /* assumption that any file only contains jobs in one PDL. The correct */
    /* way to implement this would be to have a language auto-detector. */
    pl_interp_implementation_t const *impl;

    if (pmi->implementation)
        return pmi->implementation;     /* was specified as cmd opt */
    /* select implementation */
    if ((impl = pl_pjl_select(pjl_instance, pdl_implementation)) != 0)
        return impl;
    /* lookup string in name field for each implementation */
    return pl_auto_sense((const char *)r.cursor.ptr + 1,
                         (r.cursor.limit - r.cursor.ptr), pdl_implementation);
}

/* Find default language implementation */
static pl_interp_implementation_t const *
pl_pjl_select(pl_interp_instance_t * pjl_instance, pl_interp_implementation_t const *const impl_array[] /* implementations to choose from */
    )
{
    pjl_envvar_t *language;
    pl_interp_implementation_t const *const *impl;

    language = pjl_proc_get_envvar(pjl_instance, "language");
    for (impl = impl_array; *impl != 0; ++impl) {
        if (!strcmp(pl_characteristics(*impl)->language, language))
            return *impl;
    }
    /* Defaults to NULL */
    return 0;
}

/* Find default language implementation */
static pl_interp_implementation_t const *
pl_auto_sense(const char *name, /* stream  */
              int buffer_length,        /* length of stream */
              pl_interp_implementation_t const *const impl_array[]      /* implementations to choose from */
    )
{
    /* Lookup this string in the auto sense field for each implementation */
    pl_interp_implementation_t const *const *impl;

    for (impl = impl_array; *impl != 0; ++impl) {
        if (buffer_length >=
            (strlen(pl_characteristics(*impl)->auto_sense_string)))
            if (!strncmp
                (pl_characteristics(*impl)->auto_sense_string, name,
                 (strlen(pl_characteristics(*impl)->auto_sense_string))))
                return *impl;
    }
    /* Defaults to PCL */
    return impl_array[0];
}

/* Print memory and time usage. */
void
pl_print_usage(const pl_main_instance_t * pti, const char *msg)
{
    long utime[2];

    gp_get_realtime(utime);
    dmprintf3(pti->memory, "%% %s time = %g, pages = %d\n",
              msg, utime[0] - pti->base_time[0] +
              (utime[1] - pti->base_time[1]) / 1000000000.0, pti->page_count);
}

/* Log a string to console, optionally wait for input */
static void
pl_log_string(const gs_memory_t * mem, const char *str, int wait_for_key)
{
    errwrite(mem, str, strlen(str));
    if (wait_for_key)
        (void)fgetc(mem->gs_lib_ctx->fstdin);
}

/* Pre-page portion of page finishing routine */
int                             /* ret 0 if page should be printed, 1 if no print, else -ve error */
pl_pre_finish_page(pl_interp_instance_t * interp, void *closure)
{
    pl_main_instance_t *pti = (pl_main_instance_t *) closure;

    /* up the page count */
    ++(pti->page_count);

    /* print the page */
    return 0;
}

/* Post-page portion of page finishing routine */
int                             /* ret 0, else -ve error */
pl_post_finish_page(pl_interp_instance_t * interp, void *closure)
{
    pl_main_instance_t *pti = (pl_main_instance_t *) closure;

    if (pti->pause) {
        char strbuf[256];

        gs_sprintf(strbuf, "End of page %d, press <enter> to continue.\n",
                pti->page_count);
        pl_log_string(pti->memory, strbuf, 1);
    } else if (gs_debug_c(':'))
        pl_print_usage(pti, "render done :");
    return 0;
}

/* ---------------- Stubs ---------------- */
/* Error termination, called back from plplatf.c */
/* Only called back if abnormal termination */
void
pl_exit(int exit_status)
{
    gp_do_exit(exit_status);
}

/* -------------- Read file cursor operations ---------- */
/* Open a read cursor w/specified file */
int                             /* returns 0 ok, else -ve error code */
pl_main_cursor_open(const gs_memory_t * mem, pl_top_cursor_t * cursor,  /* cursor to init/open */
                    const char *fname,  /* name of file to open */
                    byte * buffer,      /* buffer to use for reading */
                    unsigned buffer_length      /* length of *buffer */
    )
{
    /* try to open file */
    if (fname[0] == '-' && fname[1] == 0)
        cursor->strm = mem->gs_lib_ctx->fstdin;
    else
        cursor->strm = gp_fopen(fname, "rb");
    if (!cursor->strm)
        return gs_error_ioerror;

    return pl_top_cursor_init(cursor, cursor->strm, buffer, buffer_length);
}

#ifdef DEBUG
/* Refill from input */
int                             /* rets 1 ok, else 0 EOF, -ve error */
pl_main_cursor_next(pl_top_cursor_t * cursor    /* cursor to operate on */
    )
{
    return pl_top_cursor_next(cursor);
}
#endif /* DEBUG */

/* Read back curr file position */
long                            /* offset from beginning of file */
pl_main_cursor_position(pl_top_cursor_t * cursor        /* cursor to operate on */
    )
{
    return (long)ftell(cursor->strm)
        - (cursor->cursor.limit - cursor->cursor.ptr);
}

/* Close read cursor */
void
pl_main_cursor_close(pl_top_cursor_t * cursor   /* cursor to operate on */
    )
{
    pl_top_cursor_dnit(cursor);
    fclose(cursor->strm);
}


/* plmain accessors */
bool
pl_get_nocache(pl_interp_instance_t *instance)
{
    return instance->nocache;
}


bool
pl_get_interpolation(pl_interp_instance_t * instance)
{
    return instance->interpolate;
}


bool
pl_get_scanconverter(pl_interp_instance_t * instance)
{
    return instance->scanconverter;
}
