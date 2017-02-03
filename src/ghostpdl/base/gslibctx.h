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


#ifndef GSLIBCTX_H
#define GSLIBCTX_H

#include "std.h"
#include "stdio_.h"
#include "gs_dll_call.h"

typedef struct name_table_s *name_table_ptr;

#ifndef gs_fapi_server_DEFINED
#define gs_fapi_server_DEFINED
typedef struct gs_fapi_server_s gs_fapi_server;
#endif

#ifndef gs_font_dir_DEFINED
#  define gs_font_dir_DEFINED
typedef struct gs_font_dir_s gs_font_dir;
#endif
typedef struct gs_lib_ctx_s
{
    gs_memory_t *memory;  /* mem->gs_lib_ctx->memory == mem */
    FILE *fstdin;
    FILE *fstdout;
    FILE *fstderr;
    FILE *fstdout2;		/* for redirecting %stdout and diagnostics */
    bool stdout_is_redirected;	/* to stderr or fstdout2 */
    bool stdout_to_stderr;
    bool stdin_is_interactive;
    void *caller_handle;	/* identifies caller of GS DLL/shared object */
    void *custom_color_callback;  /* pointer to color callback structure */
    int (GSDLLCALL *stdin_fn)(void *caller_handle, char *buf, int len);
    int (GSDLLCALL *stdout_fn)(void *caller_handle, const char *str, int len);
    int (GSDLLCALL *stderr_fn)(void *caller_handle, const char *str, int len);
    int (GSDLLCALL *poll_fn)(void *caller_handle);
    ulong gs_next_id; /* gs_id initialized here, private variable of gs_next_ids() */
    void *top_of_system;  /* use accessor functions to walk down the system
                           * to the desired structure gs_lib_ctx_get_*()
                           */
    name_table_ptr gs_name_table;  /* hack this is the ps interpreters name table
                                    * doesn't belong here
                                    */
    /* Define whether dictionaries expand automatically when full. */
    bool dict_auto_expand;  /* ps dictionary: false level 1 true level 2 or 3 */
    /* A table of local copies of the IODevices */
    struct gx_io_device_s **io_device_table;
    int io_device_table_count;
    int io_device_table_size;
    /* Define the default value of AccurateScreens that affects setscreen
       and setcolorscreen. */
    bool screen_accurate_screens;
    uint screen_min_screen_levels;
    /* real time clock 'bias' value. Not strictly required, but some FTS
     * tests work better if realtime starts from 0 at boot time. */
    long real_time_0[2];

    /* font directory - see gsfont.h */
    gs_font_dir *font_dir;
    /* True if we are emulating CPSI. Ideally this would be in the imager
     * state, but this can't be done due to problems detecting changes in it
     * for the clist based devices. */
    bool CPSI_mode;
    /* Keep the path for the ICCProfiles here so devices and the icc_manager 
     * can get to it. Prevents needing two copies, one in the icc_manager
     * and one in the device */
    char *profiledir;               /* Directory used in searching for ICC profiles */
    int profiledir_len;             /* length of directory name (allows for Unicode) */
    void *cms_context;  /* Opaque context pointer from underlying CMS in use */
    gs_fapi_server **fapi_servers;
    char *default_device_list;
    int gcsignal;
    int scanconverter;
} gs_lib_ctx_t;

enum {
    GS_SCANCONVERTER_OLD = 0,
    GS_SCANCONVERTER_DEFAULT = 1,
    GS_SCANCONVERTER_EDGEBUFFER = 2
};

/** initializes and stores itself in the given gs_memory_t pointer.
 * it is the responsibility of the gs_memory_t objects to copy
 * the pointer to subsequent memory objects.
 */
int gs_lib_ctx_init( gs_memory_t *mem );

/** Called when the lowest level allocator (the one which the lib_ctx was
 * initialised under) is about to be destroyed. The lib_ctx should tidy up
 * after itself. */
void gs_lib_ctx_fin( gs_memory_t *mem );

gs_lib_ctx_t *gs_lib_ctx_get_interp_instance( const gs_memory_t *mem );

void *gs_lib_ctx_get_cms_context( const gs_memory_t *mem );
void gs_lib_ctx_set_cms_context( const gs_memory_t *mem, void *cms_context );

#ifndef GS_THREADSAFE
/* HACK to get at non garbage collection memory pointer
 *
 */
gs_memory_t * gs_lib_ctx_get_non_gc_memory_t(void);
#endif

void gs_lib_ctx_set_icc_directory(const gs_memory_t *mem_gc, const char* pname,
                        int dir_namelen);


/* Sets/Gets the string containing the list of device names we should search
 * to find a suitable default
 */
int
gs_lib_ctx_set_default_device_list(const gs_memory_t *mem, const char* dev_list_str,
                        int list_str_len);

/* Returns a pointer to the string not a new string */
int
gs_lib_ctx_get_default_device_list(const gs_memory_t *mem, char** dev_list_str,
                        int *list_str_len);

#define IS_LIBCTX_STDOUT(mem, f) (f == mem->gs_lib_ctx->fstdout)
#define IS_LIBCTX_STDERR(mem, f) (f == mem->gs_lib_ctx->fstderr)

#endif /* GSLIBCTX_H */
