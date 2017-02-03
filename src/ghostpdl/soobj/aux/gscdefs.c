/* Copyright (C) 2001-2015 Artifex Software, Inc.
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


/* Configuration scalars */

#include "std.h"
#include "gscdefs.h"		/* interface */
#include "gconfigd.h"		/* for #defines */

/* ---------------- Miscellaneous system parameters ---------------- */

/* All of these can be set in the makefile. */
/* Normally they are all const; see gscdefs.h for more information. */

#ifndef GS_BUILDTIME
#  define GS_BUILDTIME\
        0			/* should be set in the makefile */
#endif
const long gs_buildtime = GS_BUILDTIME;

#ifndef GS_COPYRIGHT
#  define GS_COPYRIGHT\
        "Copyright (C) 2016 Artifex Software, Inc.  All rights reserved."
#endif
const char *const gs_copyright = GS_COPYRIGHT;

#ifndef GS_PRODUCTFAMILY
#  define GS_PRODUCTFAMILY\
        "GPL Ghostscript"
#endif
const char *const gs_productfamily = GS_PRODUCTFAMILY;

const char *
gs_program_family_name(void)
{
    return gs_productfamily;
}

#ifndef GS_PRODUCT
#  define GS_PRODUCT\
        GS_PRODUCTFAMILY ""
#endif
const char *const gs_product = GS_PRODUCT;

const char *
gs_program_name(void)
{
    return gs_product;
}

/* GS_REVISION must be defined in the makefile. */
const long gs_revision = GS_REVISION;

long
gs_revision_number(void)
{
    return gs_revision;
}

/* GS_REVISIONDATE must be defined in the makefile. */
const long gs_revisiondate = GS_REVISIONDATE;

#ifndef GS_SERIALNUMBER
#  define GS_SERIALNUMBER\
        42			/* a famous number */
#endif
const long gs_serialnumber = GS_SERIALNUMBER;

/* ---------------- Installation directories and files ---------------- */

/* Here is where the library search path, the name of the */
/* initialization file, and the doc directory are defined. */
/* Define the documentation directory (only used in help messages). */
const char *const gs_doc_directory = GS_DOCDIR;

/* Define the default library search path. */
const char *const gs_lib_default_path = GS_LIB_DEFAULT;

/* Define the interpreter initialization file. */
const char *const gs_init_file = GS_INIT;

/* Define the default devices list. */
#ifndef GS_DEV_DEFAULT
#define GS_DEV_DEFAULT ""
#endif
const char *const gs_dev_defaults = GS_DEV_DEFAULT;
