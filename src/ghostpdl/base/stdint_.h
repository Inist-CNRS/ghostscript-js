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


/* Generic substitute for stdint.h */

#ifndef stdint__INCLUDED
#  define stdint__INCLUDED

/*
 * ensure our standard defines have been included
 */
#include "std.h"

/* Define some stdint.h types. The jbig2dec headers and ttf bytecode
 * interpreter require these and they're generally useful to have around
 * now that there's a standard.
 */

/* Some systems are guaranteed to have stdint.h
 * but don't use the autoconf detection
 */
#ifndef HAVE_STDINT_H
# ifdef __MACOS__
#   define HAVE_STDINT_H
# endif
#endif

/* try a generic header first */
#if defined(HAVE_STDINT_H)
# include <stdint.h>
# define STDINT_TYPES_DEFINED
#elif defined(SYS_TYPES_HAS_STDINT_TYPES)
  /* std.h will have included sys/types.h */
# define STDINT_TYPES_DEFINED
#endif

/* try platform-specific defines */
#ifndef STDINT_TYPES_DEFINED
# if defined(__WIN32__) /* MSVC currently doesn't provide C99 headers */
   typedef signed char             int8_t;
   typedef short int               int16_t;
   typedef int                     int32_t;
   typedef __int64                 int64_t;
   typedef unsigned char           uint8_t;
   typedef unsigned short int      uint16_t;
   typedef unsigned int            uint32_t;
   typedef unsigned __int64        uint64_t;
#  define STDINT_TYPES_DEFINED
# elif defined(__VMS) || defined(__osf__) /* OpenVMS and Tru64 provide these types in inttypes.h */
#  include <inttypes.h>
#  define STDINT_TYPES_DEFINED
# elif defined(__CYGWIN__)
#  include <sys/types.h>  /* Old Cygwin has some of C99 types here. */
   typedef unsigned char uint8_t;
   typedef unsigned short uint16_t;
   typedef unsigned int uint32_t;
   typedef unsigned long long uint64_t;
#  define STDINT_TYPES_DEFINED
# endif
   /* other archs may want to add defines here,
      or use the fallbacks in std.h */
#endif

/* fall back to tests based on arch.h */
#ifndef STDINT_TYPES_DEFINED
/* 8 bit types */
# if ARCH_SIZEOF_CHAR == 1
typedef signed char int8_t;
typedef unsigned char uint8_t;
# endif
/* 16 bit types */
# if ARCH_SIZEOF_SHORT == 2
typedef signed short int16_t;
typedef unsigned short uint16_t;
# else
#  if ARCH_SIZEOF_INT == 2
typedef signed int int16_t;
typedef unsigned int uint16_t;
#  endif
# endif
/* 32 bit types */
# if ARCH_SIZEOF_INT == 4
typedef signed int int32_t;
typedef unsigned int uint32_t;
# else
#  if ARCH_SIZEOF_LONG == 4
typedef signed long int32_t;
typedef unsigned long uint32_t;
#  else
#   if ARCH_SIZEOF_SHORT == 4
typedef signed short int32_t;
typedef unsigned short uint32_t;
#   endif
#  endif
# endif
/* 64 bit types */
# if ARCH_SIZEOF_INT == 8
typedef signed int int64_t;
typedef unsigned int uint64_t;
# else
#  if ARCH_SIZEOF_LONG == 8
typedef signed long int64_t;
typedef unsigned long uint64_t;
#  else
#   ifdef ARCH_SIZEOF_LONG_LONG
#    if ARCH_SIZEOF_LONG_LONG == 8
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
#    endif
#   endif
#  endif
# endif

#  define STDINT_TYPES_DEFINED
#endif /* STDINT_TYPES_DEFINED */

#if defined(HAVE_INTTYPES_H) && HAVE_INTTYPES_H == 1
# include <inttypes.h>
#else
# if defined(__WIN32__)
#  define PRId32 "I32d"
#  define PRId64 "I64d"
#  define PRIi32 "I32i"
#  define PRIi64 "I64i"
#  define PRIu32 "I32u"
#  define PRIu64 "I64u"
#  define PRIx64 "I64x"
# endif
#endif

/* Even if we have inttypes.h, these may not be defined */
# ifndef PRId32
#  define PRId32 "d"
# endif

# ifndef PRId64
#  define PRId64 "lld"
# endif

# ifndef PRIi32
#  define PRIi32 "i"
# endif

# ifndef PRIi64
#  define PRIi64 "lli"
# endif

# ifndef PRIu32
#  define PRIu32 "u"
# endif

# ifndef PRIu64
#  define PRIu64 "llu"
# endif

# ifndef PRIx64
#  define PRIx64 "llx"
# endif

#endif /* stdint__INCLUDED */
