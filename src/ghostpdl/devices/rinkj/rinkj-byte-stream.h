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


/* Bytestream abstraction for Rinkj driver. */

typedef struct _RinkjByteStream RinkjByteStream;

struct _RinkjByteStream {
  int (*write) (RinkjByteStream *self, const char *buf, int size);
};

int
rinkj_byte_stream_write (RinkjByteStream *bs, const char *buf, int size);

int
rinkj_byte_stream_puts (RinkjByteStream *bs, const char *str);

int
rinkj_byte_stream_printf (RinkjByteStream *bs, const char *fmt, ...);

int
rinkj_byte_stream_close (RinkjByteStream *bs);

RinkjByteStream *
rinkj_byte_stream_file_new (FILE *f);
