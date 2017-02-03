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

/* Definitions for interpreter support for file objects */
/* Requires stream.h */

#ifndef files_INCLUDED
#  define files_INCLUDED

/*
 * File objects store a pointer to a stream in value.pfile.
 * A file object is valid if its "size" matches the read_id or write_id
 * (as appropriate) in the stream it points to.  This arrangement
 * allows us to detect closed files reliably, while allowing us to
 * reuse closed streams for new files.
 */
#define fptr(pref) (pref)->value.pfile
#define make_file(pref,a,id,s)\
  make_tasv(pref,t_file,a,id,pfile,s)

/* The stdxxx files.  We have to access them through procedures, */
/* because they might have to be opened when referenced. */
int zget_stdin(i_ctx_t *, stream **);
int zget_stdout(i_ctx_t *, stream **);
int zget_stderr(i_ctx_t *, stream **);
/* Test whether a stream is stdin. */
bool zis_stdin(const stream *);

/* Define access to the stdio refs for operators. */
#define ref_stdio (i_ctx_p->stdio)
#define ref_stdin ref_stdio[0]
#define ref_stdout ref_stdio[1]
#define ref_stderr ref_stdio[2]
/* An invalid (closed) file. */
#define avm_invalid_file_entry avm_foreign
/* Make an invalid file object. */
void make_invalid_file(i_ctx_t *,ref *);

/*
 * If a file is open for both reading and writing, its read_id, write_id,
 * and stream procedures and modes reflect the current mode of use;
 * an id check failure will switch it to the other mode.
 */
int file_switch_to_read(const ref *);

#define check_read_file(ctx, svar,op)\
  BEGIN\
    check_read_type(*(op), t_file);\
    check_read_known_file(ctx, svar, op, return);\
  END
#define check_read_known_file(ctx,svar,op,error_return)\
  check_read_known_file_else(svar, op, error_return, svar = (ctx->invalid_file_stream))
#define check_read_known_file_else(svar,op,error_return,invalid_action)\
  BEGIN\
    svar = fptr(op);\
    if (svar->read_id != r_size(op)) {\
        if (svar->read_id == 0 && svar->write_id == r_size(op)) {\
            int fcode = file_switch_to_read(op);\
\
            if (fcode < 0)\
                 error_return(fcode);\
        } else {\
            invalid_action;	/* closed or reopened file */\
        }\
    }\
  END
int file_switch_to_write(const ref *);

#define check_write_file(svar,op)\
  BEGIN\
    check_write_type(*(op), t_file);\
    check_write_known_file(svar, op, return);\
  END
#define check_write_known_file(svar,op,error_return)\
  BEGIN\
    svar = fptr(op);\
    if ( svar->write_id != r_size(op) )\
        {	int fcode = file_switch_to_write(op);\
                if ( fcode < 0 ) error_return(fcode);\
        }\
  END

/* Data exported by zfile.c. */
        /* for zfilter.c and ziodev.c */
extern const uint file_default_buffer_size;

#ifndef gs_file_path_ptr_DEFINED
#  define gs_file_path_ptr_DEFINED
typedef struct gs_file_path_s *gs_file_path_ptr;
#endif

/* Procedures exported by zfile.c. */
        /* for imainarg.c */
FILE *lib_fopen(const gs_file_path_ptr pfpath, const gs_memory_t *mem, const char *);

        /* for imain.c */
int
lib_file_open(gs_file_path_ptr, const gs_memory_t *, i_ctx_t *,
                       const char *, uint, char *, int, uint *, ref *pfile);

        /* for imain.c */
#ifndef gs_ref_memory_DEFINED
#  define gs_ref_memory_DEFINED
typedef struct gs_ref_memory_s gs_ref_memory_t;
#endif
int file_read_string(const byte *, uint, ref *, gs_ref_memory_t *);

        /* for os_open in ziodev.c */
#ifdef iodev_proc_fopen		/* in gxiodev.h */
int file_open_stream(const char *, uint, const char *, uint, stream **,
                     gx_io_device *, iodev_proc_fopen_t, gs_memory_t *);
#endif

        /* for zfilter.c */
int filter_open(const char *, uint, ref *, const stream_procs *,
                const stream_template *, const stream_state *,
                gs_memory_t *);

        /* for zfileio.c */
void make_stream_file(ref *, stream *, const char *);

        /* for ziodev.c */
int file_close_file(stream *);

        /* for gsmain.c, interp.c */
int file_close(ref *);

        /* for zfproc.c, ziodev.c */
stream *file_alloc_stream(gs_memory_t *, client_name_t);

/* Procedures exported by zfileio.c. */
        /* for ziodev.c */
int zreadline_from(stream *s, gs_string *buf, gs_memory_t *bufmem,
                   uint *pcount, bool *pin_eol);

/* Procedures exported by zfileio.c. */
        /* for zfile.c */
int zfilelineedit(i_ctx_t *i_ctx_p);

#endif /* files_INCLUDED */
