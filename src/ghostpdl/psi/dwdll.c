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



/* dwdll.c */

#define STRICT
#include <windows.h>
#include <string.h>
#include <stdio.h>

#include "stdpre.h"
#include "gpgetenv.h"
#include "gscdefs.h"
#define GSREVISION gs_revision

#define GSDLLEXPORT
#define GSDLLAPI CALLBACK
#define GSDLLCALL

#include "dwdll.h"

#ifdef METRO
#ifdef _WIN64
static const char name[] = "gsdll64metro.dll";
#else
static const char name[] = "gsdll32metro.dll";
#endif
#else
#ifdef _WIN64
static const char name[] = "gsdll64.dll";
#else
static const char name[] = "gsdll32.dll";
#endif
#endif

int load_dll(GSDLL *gsdll, char *last_error, int len)
{
char fullname[1024];
char *p;
int length;
gsapi_revision_t rv;

    /* Don't load if already loaded */
    if (gsdll->hmodule)
        return 0;

    /* First try to load DLL from the same directory as EXE */
    GetModuleFileName(GetModuleHandle(NULL), fullname, sizeof(fullname));
    if ((p = strrchr(fullname,'\\')) != (char *)NULL)
        p++;
    else
        p = fullname;
    *p = '\0';
    strcat(fullname, name);
    gsdll->hmodule = LoadLibrary(fullname);

    /* Next try to load DLL with name in registry or environment variable */
    if (gsdll->hmodule < (HINSTANCE)HINSTANCE_ERROR) {
        length = sizeof(fullname);
        if (gp_getenv("GS_DLL", fullname, &length) == 0)
            gsdll->hmodule = LoadLibrary(fullname);
    }

    /* Finally try the system search path */
    if (gsdll->hmodule < (HINSTANCE)HINSTANCE_ERROR)
        gsdll->hmodule = LoadLibrary(name);

    if (gsdll->hmodule < (HINSTANCE)HINSTANCE_ERROR) {
        /* Failed */
        DWORD err = GetLastError();
        sprintf(fullname, "Can't load DLL, LoadLibrary error code %ld", err);
        strncpy(last_error, fullname, len-1);
        gsdll->hmodule = (HINSTANCE)0;
        return 1;
    }

    /* DLL is now loaded */
    /* Get pointers to functions */
    gsdll->revision = (PFN_gsapi_revision) GetProcAddress(gsdll->hmodule,
        "gsapi_revision");
    if (gsdll->revision == NULL) {
        strncpy(last_error, "Can't find gsapi_revision\n", len-1);
        unload_dll(gsdll);
        return 1;
    }
    /* check DLL version */
    if (gsdll->revision(&rv, sizeof(rv)) != 0) {
        sprintf(fullname, "Unable to identify Ghostscript DLL revision - it must be newer than needed.\n");
        strncpy(last_error, fullname, len-1);
        unload_dll(gsdll);
        return 1;
    }
    if (rv.revision != GSREVISION) {
        sprintf(fullname, "Wrong version of DLL found.\n  Found version %ld\n  Need version  %ld\n", rv.revision, GSREVISION);
        strncpy(last_error, fullname, len-1);
        unload_dll(gsdll);
        return 1;
    }

    /* continue loading other functions */
    gsdll->new_instance = (PFN_gsapi_new_instance) GetProcAddress(gsdll->hmodule,
        "gsapi_new_instance");
    if (gsdll->new_instance == NULL) {
        strncpy(last_error, "Can't find gsapi_new_instance\n", len-1);
        unload_dll(gsdll);
        return 1;
    }

    gsdll->delete_instance = (PFN_gsapi_delete_instance) GetProcAddress(gsdll->hmodule,
        "gsapi_delete_instance");
    if (gsdll->delete_instance == NULL) {
        strncpy(last_error, "Can't find gsapi_delete_instance\n", len-1);
        unload_dll(gsdll);
        return 1;
    }

    gsdll->set_stdio = (PFN_gsapi_set_stdio) GetProcAddress(gsdll->hmodule,
        "gsapi_set_stdio");
    if (gsdll->set_stdio == NULL) {
        strncpy(last_error, "Can't find gsapi_set_stdio\n", len-1);
        unload_dll(gsdll);
        return 1;
    }

    gsdll->set_poll = (PFN_gsapi_set_poll) GetProcAddress(gsdll->hmodule,
        "gsapi_set_poll");
    if (gsdll->set_poll == NULL) {
        strncpy(last_error, "Can't find gsapi_set_poll\n", len-1);
        unload_dll(gsdll);
        return 1;
    }

    gsdll->set_display_callback = (PFN_gsapi_set_display_callback)
        GetProcAddress(gsdll->hmodule, "gsapi_set_display_callback");
    if (gsdll->set_display_callback == NULL) {
        strncpy(last_error, "Can't find gsapi_set_display_callback\n", len-1);
        unload_dll(gsdll);
        return 1;
    }

    gsdll->get_default_device_list = (PFN_gsapi_get_default_device_list)
        GetProcAddress(gsdll->hmodule, "gsapi_get_default_device_list");
    if (gsdll->get_default_device_list == NULL) {
        strncpy(last_error, "Can't find gsapi_get_default_device_list\n", len-1);
        unload_dll(gsdll);
        return 1;
    }

    gsdll->set_default_device_list = (PFN_gsapi_set_default_device_list)
        GetProcAddress(gsdll->hmodule, "gsapi_set_default_device_list");
    if (gsdll->set_default_device_list == NULL) {
        strncpy(last_error, "Can't find gsapi_set_default_device_list\n", len-1);
        unload_dll(gsdll);
        return 1;
    }

    gsdll->init_with_args = (PFN_gsapi_init_with_args)
        GetProcAddress(gsdll->hmodule, "gsapi_init_with_args");
    if (gsdll->init_with_args == NULL) {
        strncpy(last_error, "Can't find gsapi_init_with_args\n", len-1);
        unload_dll(gsdll);
        return 1;
    }

    gsdll->set_arg_encoding = (PFN_gsapi_set_arg_encoding)
        GetProcAddress(gsdll->hmodule, "gsapi_set_arg_encoding");
    if (gsdll->set_arg_encoding == NULL) {
        strncpy(last_error, "Can't find gsapi_set_arg_encoding\n", len-1);
        unload_dll(gsdll);
        return 1;
    }

    gsdll->run_string = (PFN_gsapi_run_string) GetProcAddress(gsdll->hmodule,
        "gsapi_run_string");
    if (gsdll->run_string == NULL) {
        strncpy(last_error, "Can't find gsapi_run_string\n", len-1);
        unload_dll(gsdll);
        return 1;
    }

    gsdll->exit = (PFN_gsapi_exit) GetProcAddress(gsdll->hmodule,
        "gsapi_exit");
    if (gsdll->exit == NULL) {
        strncpy(last_error, "Can't find gsapi_exit\n", len-1);
        unload_dll(gsdll);
        return 1;
    }

    return 0;
}

void unload_dll(GSDLL *gsdll)
{
    /* Set functions to NULL to prevent use */
    gsdll->revision = NULL;
    gsdll->new_instance = NULL;
    gsdll->delete_instance = NULL;
    gsdll->init_with_args = NULL;
    gsdll->run_string = NULL;
    gsdll->exit = NULL;
    gsdll->set_stdio = NULL;
    gsdll->set_poll = NULL;
    gsdll->set_display_callback = NULL;

    if (gsdll->hmodule != (HINSTANCE)NULL)
            FreeLibrary(gsdll->hmodule);
    gsdll->hmodule = NULL;
}
