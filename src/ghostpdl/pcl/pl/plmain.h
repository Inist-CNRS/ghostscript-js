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


/* plmain.h */
/* Interface to main program utilities for PCL interpreters */

#ifndef plmain_INCLUDED
#  define plmain_INCLUDED

#include "gsargs.h"
#include "gsgc.h"

/*
 * Define the parameters for running the interpreter.
 */
#ifndef gx_device_DEFINED
#  define gx_device_DEFINED
typedef struct gx_device_s gx_device;
#endif

/*
 * Define the parameters for running the interpreter.
 */
typedef struct pl_main_instance_s
{
    /* The following are set at initialization time. */
    gs_memory_t *memory;
    gs_memory_t *device_memory;
    long base_time[2];          /* starting time */
    int error_report;           /* -E# */
    bool pause;                 /* -dNOPAUSE => false */
    int first_page;             /* -dFirstPage= */
    int last_page;              /* -dLastPage= */
    gx_device *device;
    vm_spaces spaces;           /* spaces for "ersatz" garbage collector */

    pl_interp_implementation_t const *implementation; /*-L<Language>*/
    /* The following are updated dynamically. */
    int page_count;             /* # of pages printed */

    char pcl_personality[6];    /* a character string to set pcl's
                                   personality - rtl, pcl5c, pcl5e, and
                                   pcl == default.  NB doesn't belong here. */
    bool interpolate;
    bool nocache;
    bool page_set_on_command_line;
    bool res_set_on_command_line;
    bool high_level_device;
#ifndef OMIT_SAVED_PAGES_TEST
    bool saved_pages_test_mode;
#endif
    int scanconverter;
    /* we have to store these in the main instance until the languages
       state is sufficiently initialized to set the parameters. */
    char *piccdir;
    char *pdefault_gray_icc;
    char *pdefault_rgb_icc;
    char *pdefault_cmyk_icc;
} pl_main_instance_t;

/* initialize gs_stdin, gs_stdout, and gs_stderr.  Eventually the gs
   library should provide an interface for doing this */
void pl_main_init_standard_io(void);

/* Initialize the instance parameters. */
void pl_main_init(pl_main_instance_t * pmi, gs_memory_t * memory);

/* Allocate and initialize the first graphics state. */
#ifndef gs_gstate_DEFINED
#  define gs_gstate_DEFINED
typedef struct gs_gstate_s gs_gstate;
#endif
int pl_main_make_gstate(pl_main_instance_t * pmi, gs_gstate ** ppgs);

#ifdef DEBUG
/* Print memory and time usage. */
void pl_print_usage(const pl_main_instance_t * pmi, const char *msg);
#endif

/* Finish a page, possibly printing usage statistics and/or pausing. */
int pl_finish_page(pl_main_instance_t * pmi, gs_gstate * pgs,
                   int num_copies, int flush);

/* Main instance accessors */
bool pl_get_nocache(pl_interp_instance_t * instance);
bool pl_get_interpolation(pl_interp_instance_t * instance);
int pl_get_scanconverter(pl_interp_instance_t * instance);

#endif /* plmain_INCLUDED */
