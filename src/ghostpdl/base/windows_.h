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


/* Wrapper for windows.h */

#ifndef windows__INCLUDED
#  define windows__INCLUDED

/* This is a good place to define GS_NO_UTF8 */

#define STRICT
#ifdef METRO
#include <winapifamily.h>
#endif
#include <windows.h>
#include <process.h>

/* After including windows.h, we have to undef bool for VS 2014 and later */
#if defined(_MSC_VER) && _MSC_VER>=1900
#  undef bool
   typedef int bool;
#endif

/* Unicode/UTF-8 wrappers that we provide */
BOOL gp_OpenPrinter(char *device, LPHANDLE printer);
#ifndef GS_NO_UTF8
int utf8_to_wchar(wchar_t *out, const char *in);
int wchar_to_utf8(char *out, const wchar_t *in);
#endif

#ifdef __WATCOMC__
typedef RGBQUAD FAR * LPRGBQUAD;
        /* Watcom's _beginthread takes an extra stack_bottom argument. */
#  define BEGIN_THREAD(proc, stksize, data)\
     _beginthread(proc, NULL, stksize, data)
#else
#  define BEGIN_THREAD(proc, stksize, data)\
     _beginthread(proc, stksize, data)
        /* Define null equivalents of the Watcom 32-to-16-bit glue. */
#  define AllocAlias16(ptr) ((DWORD)(ptr))
#  define FreeAlias16(dword)	/* */
#  define MK_FP16(fp32) ((DWORD)(fp32))
#  define MK_FP32(fp16) (fp16)
#  define GetProc16(proc, ptype) (proc)
#  define ReleaseProc16(cbp)	/* */
#endif

/* Substitute for special "far" library procedures under Win32. */
#ifdef __WIN32__
#  undef _fstrtok
#  define _fstrtok(str, set) strtok(str, set)
#endif

#if defined(__BORLANDC__)
#  define exception_code() __exception_code
#endif

#endif /* windows__INCLUDED */
