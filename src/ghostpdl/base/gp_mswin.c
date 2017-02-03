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


/*
 * Microsoft Windows platform support for Ghostscript.
 *
 * Original version by Russell Lang and Maurice Castro with help from
 * Programming Windows, 2nd Ed., Charles Petzold, Microsoft Press;
 * initially created from gp_dosfb.c and gp_itbc.c 5th June 1992.
 */

/* Modified for Win32 & Microsoft C/C++ 8.0 32-Bit, 26.Okt.1994 */
/* by Friedrich Nowak                                           */

/* Original EXE and GSview specific code removed */
/* DLL version must now be used under MS-Windows */
/* Russell Lang 16 March 1996 */

/* prevent gp.h from defining fopen */
#define fopen fopen

#include "windows_.h"
#include "stdio_.h"
#include "string_.h"
#include "memory_.h"
#include "stat_.h"
#include "pipe_.h"
#include <stdlib.h>
#include <stdarg.h>
#include "ctype_.h"
#include <io.h>
#include "malloc_.h"
#include <fcntl.h>
#include <signal.h>
#include "gx.h"

#include "gp.h"
#include "gpcheck.h"
#include "gpmisc.h"
#include "gserrors.h"
#include "gsexit.h"
#include "scommon.h"

#include <shellapi.h>
#include <winspool.h>
#include "gp_mswin.h"
#include "winrtsup.h"

/* Library routines not declared in a standard header */
extern char *getenv(const char *);

/* limits */
#define MAXSTR 255

/* GLOBAL VARIABLE that needs to be removed */
char win_prntmp[MAXSTR];	/* filename of PRN temporary file */

/* GLOBAL VARIABLES - initialised at DLL load time */
HINSTANCE phInstance;
BOOL is_win32s = FALSE;

const LPSTR szAppName = "Ghostscript";
#ifndef METRO
static int is_printer(const char *name);
#endif

/* ====== Generic platform procedures ====== */

/* ------ Initialization/termination (from gp_itbc.c) ------ */

/* Do platform-dependent initialization. */
void
gp_init(void)
{
}

/* Do platform-dependent cleanup. */
void
gp_exit(int exit_status, int code)
{
}

/* Exit the program. */
void
gp_do_exit(int exit_status)
{
    exit(exit_status);
}

/* ------ Printer accessing ------ */

/* Forward references */
#ifndef METRO
static int gp_printfile(const char *, const char *);
#endif

/* Open a connection to a printer.  A null file name means use the */
/* standard printer connected to the machine, if any. */
/* Return NULL if the connection could not be opened. */
FILE *
gp_open_printer(const gs_memory_t *mem,
                      char         fname[gp_file_name_sizeof],
                      int          binary_mode)
{
#ifdef METRO
    return gp_fopen(fname, (binary_mode ? "wb" : "w"));
#else
    if (is_printer(fname)) {
        FILE *pfile;

        /* Open a scratch file, which we will send to the */
        /* actual printer in gp_close_printer. */
        pfile = gp_open_scratch_file(mem, gp_scratch_file_name_prefix,
                                     win_prntmp, "wb");
        return pfile;
    } else if (fname[0] == '|') 	/* pipe */
        return mswin_popen(fname + 1, (binary_mode ? "wb" : "w"));
    else if (strcmp(fname, "LPT1:") == 0)
        return NULL;	/* not supported, use %printer%name instead  */
    else
        return gp_fopen(fname, (binary_mode ? "wb" : "w"));
#endif
}

/* Close the connection to the printer. */
void
gp_close_printer(const gs_memory_t *mem, FILE * pfile, const char *fname)
{
#ifdef METRO
    fclose(pfile);
#else
    fclose(pfile);
    if (!is_printer(fname))
        return;			/* a file, not a printer */

    gp_printfile(win_prntmp, fname);
    unlink(win_prntmp);
#endif
}

#ifndef METRO
/* Dialog box to select printer port */
DLGRETURN CALLBACK
SpoolDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPSTR entry;

    switch (message) {
        case WM_INITDIALOG:
            entry = (LPSTR) lParam;
            while (*entry) {
                SendDlgItemMessage(hDlg, SPOOL_PORT, LB_ADDSTRING, 0, (LPARAM) entry);
                entry += lstrlen(entry) + 1;
            }
            SendDlgItemMessage(hDlg, SPOOL_PORT, LB_SETCURSEL, 0, (LPARAM) 0);
            return TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case SPOOL_PORT:
                    if (HIWORD(wParam) == LBN_DBLCLK)
                        PostMessage(hDlg, WM_COMMAND, IDOK, 0L);
                    return FALSE;
                case IDOK:
                    EndDialog(hDlg, 1 + (int)SendDlgItemMessage(hDlg, SPOOL_PORT, LB_GETCURSEL, 0, 0L));
                    return TRUE;
                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    return TRUE;
            }
    }
    return FALSE;
}

/* return TRUE if queue looks like a valid printer name */
int
is_spool(const char *queue)
{
    char *prefix = "\\\\spool";	/* 7 characters long */
    int i;

    for (i = 0; i < 7; i++) {
        if (prefix[i] == '\\') {
            if ((*queue != '\\') && (*queue != '/'))
                return FALSE;
        } else if (tolower(*queue) != prefix[i])
            return FALSE;
        queue++;
    }
    if (*queue && (*queue != '\\') && (*queue != '/'))
        return FALSE;
    return TRUE;
}

static int
is_printer(const char *name)
{
    /* is printer if no name given */
    if (strlen(name) == 0)
        return TRUE;

    /* is printer if name prefixed by \\spool\ */
    if (is_spool(name))
        return TRUE;

    return FALSE;
}

/******************************************************************/
/* Print File to port or queue */
/* port==NULL means prompt for port or queue with dialog box */

/* This is messy because of past support for old version of Windows. */

/* Win95, WinNT: Use OpenPrinter, WritePrinter etc. */
#ifndef METRO
static int gp_printfile_win32(const char *filename, char *port);
#endif

/*
 * Valid values for pmport are:
 *   ""
 *      action: Use default queue
 *   "\\spool\printer name"
 *      action: send to printer using WritePrinter.
 *              Using "%printer%printer name" is preferred
 *   "\\spool"
 *      action: prompt for queue name then send to printer using WritePrinter.
 *              THIS IS CURRENTLY BROKEN
 */
/* Print File */
static int
gp_printfile(const char *filename, const char *pmport)
{
    if (strlen(pmport) == 0) {	/* get default printer */
        char buf[256];
        char *p;

        /* WinNT stores default printer in registry and win.ini */
        /* Win95 stores default printer in win.ini */
#ifdef GS_NO_UTF8
        GetProfileString("windows", "device", "", buf, sizeof(buf));
#else
        wchar_t wbuf[512];
        int l;

        GetProfileStringW(L"windows", L"device", L"",  wbuf, sizeof(wbuf));
        l = wchar_to_utf8(NULL, wbuf);
        if (l < 0 || l > sizeof(buf))
            return_error(gs_error_undefinedfilename);
        wchar_to_utf8(buf, wbuf);
#endif
        if ((p = strchr(buf, ',')) != NULL)
            *p = '\0';
        return gp_printfile_win32(filename, buf);
    } else if (is_spool(pmport)) {
        if (strlen(pmport) >= 8)
            return gp_printfile_win32(filename, (char *)pmport + 8);
        else
            return gp_printfile_win32(filename, (char *)NULL);
    } else
        return_error(gs_error_undefinedfilename);
}

#define PRINT_BUF_SIZE 16384u
#define PORT_BUF_SIZE 4096

char *
get_queues(void)
{
    int i;
    DWORD count, needed;
    PRINTER_INFO_1 *prinfo;
    char *enumbuffer;
    char *buffer;
    char *p;

    /* enumerate all available printers */
    EnumPrinters(PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_LOCAL, NULL, 1, NULL, 0, &needed, &count);
    if (needed == 0) {
        /* no printers */
        enumbuffer = malloc(4);
        if (enumbuffer == (char *)NULL)
            return NULL;
        memset(enumbuffer, 0, 4);
        return enumbuffer;
    }
    enumbuffer = malloc(needed);
    if (enumbuffer == (char *)NULL)
        return NULL;
    if (!EnumPrinters(PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_LOCAL, NULL, 1, (LPBYTE) enumbuffer, needed, &needed, &count)) {
        char buf[256];

        free(enumbuffer);
        gs_sprintf(buf, "EnumPrinters() failed, error code = %d", GetLastError());
        MessageBox((HWND) NULL, buf, szAppName, MB_OK | MB_ICONSTOP);
        return NULL;
    }
    prinfo = (PRINTER_INFO_1 *) enumbuffer;
    if ((buffer = malloc(PORT_BUF_SIZE)) == (char *)NULL) {
        free(enumbuffer);
        return NULL;
    }
    /* copy printer names to single buffer */
    p = buffer;
    for (i = 0; i < count; i++) {
        if (strlen(prinfo[i].pName) + 1 < (PORT_BUF_SIZE - (p - buffer))) {
            strcpy(p, prinfo[i].pName);
            p += strlen(p) + 1;
        }
    }
    *p = '\0';			/* double null at end */
    free(enumbuffer);
    return buffer;
}

/* return TRUE if queuename available */
/* return FALSE if cancelled or error */
/* if queue non-NULL, use as suggested queue */
BOOL
get_queuename(char *portname, const char *queue)
{
    char *buffer;
    char *p;
    int i, iport;

    buffer = get_queues();
    if (buffer == NULL)
        return FALSE;
    if ((queue == (char *)NULL) || (strlen(queue) == 0)) {
        /* PROMPTING FOR A QUEUE IS CURRENTLY BROKEN */
        /* select a queue */
        iport = DialogBoxParam(phInstance, "QueueDlgBox", (HWND) NULL, SpoolDlgProc, (LPARAM) buffer);
        if (!iport) {
            free(buffer);
            return FALSE;
        }
        p = buffer;
        for (i = 1; i < iport && strlen(p) != 0; i++)
            p += lstrlen(p) + 1;
        /* prepend \\spool\ which is used by Ghostscript to distinguish */
        /* real files from queues */
        strcpy(portname, "\\\\spool\\");
        strcat(portname, p);
    } else {
        strcpy(portname, "\\\\spool\\");
        strcat(portname, queue);
    }

    free(buffer);
    return TRUE;
}
#endif

BOOL gp_OpenPrinter(char *port, LPHANDLE printer)
{
#ifdef METRO
    return FALSE;
#else
#ifdef GS_NO_UTF8
    return OpenPrinter(port, printer, NULL);
#else
    BOOL opened;
    wchar_t *uni = malloc(utf8_to_wchar(NULL, port) * sizeof(wchar_t));
    if (uni)
        utf8_to_wchar(uni, port);
    opened = OpenPrinterW(uni, printer, NULL);
    free(uni);
    return opened;
#endif
#endif
}

#ifndef METRO
/* True Win32 method, using OpenPrinter, WritePrinter etc. */
static int
gp_printfile_win32(const char *filename, char *port)
{
    DWORD count;
    char *buffer;
    char portname[MAXSTR];
    FILE *f;
    HANDLE printer;
    DOC_INFO_1 di;
    DWORD written;

    if (!get_queuename(portname, port))
        return FALSE;
    port = portname + 8;	/* skip over \\spool\ */

    if ((buffer = malloc(PRINT_BUF_SIZE)) == (char *)NULL)
        return FALSE;

    if ((f = gp_fopen(filename, "rb")) == (FILE *) NULL) {
        free(buffer);
        return FALSE;
    }
    /* open a printer */
    if (!gp_OpenPrinter(port, &printer)) {
        char buf[256];

        gs_sprintf(buf, "OpenPrinter() failed for \042%s\042, error code = %d", port, GetLastError());
        MessageBox((HWND) NULL, buf, szAppName, MB_OK | MB_ICONSTOP);
        free(buffer);
        return FALSE;
    }
    /* from here until ClosePrinter, should AbortPrinter on error */

    di.pDocName = szAppName;
    di.pOutputFile = NULL;
    di.pDatatype = "RAW";	/* for available types see EnumPrintProcessorDatatypes */
    if (!StartDocPrinter(printer, 1, (LPBYTE) & di)) {
        char buf[256];

        gs_sprintf(buf, "StartDocPrinter() failed, error code = %d", GetLastError());
        MessageBox((HWND) NULL, buf, szAppName, MB_OK | MB_ICONSTOP);
        AbortPrinter(printer);
        free(buffer);
        return FALSE;
    }
    /* copy file to printer */
    while ((count = fread(buffer, 1, PRINT_BUF_SIZE, f)) != 0) {
        if (!WritePrinter(printer, (LPVOID) buffer, count, &written)) {
            free(buffer);
            fclose(f);
            AbortPrinter(printer);
            return FALSE;
        }
    }
    fclose(f);
    free(buffer);

    if (!EndDocPrinter(printer)) {
        char buf[256];

        gs_sprintf(buf, "EndDocPrinter() failed, error code = %d", GetLastError());
        MessageBox((HWND) NULL, buf, szAppName, MB_OK | MB_ICONSTOP);
        AbortPrinter(printer);
        return FALSE;
    }
    if (!ClosePrinter(printer)) {
        char buf[256];

        gs_sprintf(buf, "ClosePrinter() failed, error code = %d", GetLastError());
        MessageBox((HWND) NULL, buf, szAppName, MB_OK | MB_ICONSTOP);
        return FALSE;
    }
    return TRUE;
}

/******************************************************************/
/* MS Windows has popen and pclose in stdio.h, but under different names.
 * Unfortunately MSVC5 and 6 have a broken implementation of _popen,
 * so we use own.  Our implementation only supports mode "wb".
 */
FILE *mswin_popen(const char *cmd, const char *mode)
{
    SECURITY_ATTRIBUTES saAttr;
#ifdef GS_NO_UTF8
    STARTUPINFO siStartInfo;
#else
    STARTUPINFOW siStartInfo;
#endif
    PROCESS_INFORMATION piProcInfo;
    HANDLE hPipeTemp = INVALID_HANDLE_VALUE;
    HANDLE hChildStdinRd = INVALID_HANDLE_VALUE;
    HANDLE hChildStdinWr = INVALID_HANDLE_VALUE;
    HANDLE hChildStdoutWr = INVALID_HANDLE_VALUE;
    HANDLE hChildStderrWr = INVALID_HANDLE_VALUE;
    HANDLE hProcess = GetCurrentProcess();
    int handle = 0;
#ifdef GS_NO_UTF8
    char *command = NULL;
#else
    wchar_t *command = NULL;
#endif
    FILE *pipe = NULL;

    if (strcmp(mode, "wb") != 0)
        return NULL;

    /* Set the bInheritHandle flag so pipe handles are inherited. */
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    /* Create anonymous inheritable pipes for STDIN for child.
     * First create a noninheritable duplicate handle of our end of
     * the pipe, then close the inheritable handle.
     */
    if (handle == 0)
        if (!CreatePipe(&hChildStdinRd, &hPipeTemp, &saAttr, 0))
            handle = -1;
    if (handle == 0) {
        if (!DuplicateHandle(hProcess, hPipeTemp,
            hProcess, &hChildStdinWr, 0, FALSE /* not inherited */,
            DUPLICATE_SAME_ACCESS))
            handle = -1;
        CloseHandle(hPipeTemp);
    }
    /* Create inheritable duplicate handles for our stdout/err */
    if (handle == 0)
        if (!DuplicateHandle(hProcess, GetStdHandle(STD_OUTPUT_HANDLE),
            hProcess, &hChildStdoutWr, 0, TRUE /* inherited */,
            DUPLICATE_SAME_ACCESS))
            handle = -1;
    if (handle == 0)
        if (!DuplicateHandle(hProcess, GetStdHandle(STD_ERROR_HANDLE),
            hProcess, &hChildStderrWr, 0, TRUE /* inherited */,
            DUPLICATE_SAME_ACCESS))
            handle = -1;

    /* Set up members of STARTUPINFO structure. */
    memset(&siStartInfo, 0, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(siStartInfo);
    siStartInfo.dwFlags = STARTF_USESTDHANDLES;
    siStartInfo.hStdInput = hChildStdinRd;
    siStartInfo.hStdOutput = hChildStdoutWr;
    siStartInfo.hStdError = hChildStderrWr;

    if (handle == 0) {
#ifdef GS_NO_UTF8
        command = (char *)malloc(strlen(cmd)+1);
        if (command)
            strcpy(command, cmd);
#else
        command = (wchar_t *)malloc(sizeof(wchar_t)*utf8_to_wchar(NULL, cmd));
        if (command)
            utf8_to_wchar(command, cmd);
#endif
        else
            handle = -1;
    }

    if (handle == 0)
#ifdef GS_NO_UTF8
        if (!CreateProcess(NULL,
#else
        if (!CreateProcessW(NULL,
#endif
            command,  	   /* command line                       */
            NULL,          /* process security attributes        */
            NULL,          /* primary thread security attributes */
            TRUE,          /* handles are inherited              */
            0,             /* creation flags                     */
            NULL,          /* environment                        */
            NULL,          /* use parent's current directory     */
            &siStartInfo,  /* STARTUPINFO pointer                */
            &piProcInfo))  /* receives PROCESS_INFORMATION  */
        {
            handle = -1;
        }
        else {
            CloseHandle(piProcInfo.hProcess);
            CloseHandle(piProcInfo.hThread);
            handle = _open_osfhandle((long)hChildStdinWr, 0);
        }

    if (hChildStdinRd != INVALID_HANDLE_VALUE)
        CloseHandle(hChildStdinRd);	/* close our copy */
    if (hChildStdoutWr != INVALID_HANDLE_VALUE)
        CloseHandle(hChildStdoutWr);	/* close our copy */
    if (hChildStderrWr != INVALID_HANDLE_VALUE)
        CloseHandle(hChildStderrWr);	/* close our copy */
    if (command)
        free(command);

    if (handle < 0) {
        if (hChildStdinWr != INVALID_HANDLE_VALUE)
            CloseHandle(hChildStdinWr);
    }
    else {
        pipe = _fdopen(handle, "wb");
        if (pipe == NULL)
            _close(handle);
    }
    return pipe;
}
#endif

/* ------ File naming and accessing ------ */

/* Create and open a scratch file with a given name prefix. */
/* Write the actual file name at fname. */
static FILE *
gp_open_scratch_file_generic(const gs_memory_t *mem,
                             const char        *prefix,
                                   char        *fname,
                             const char        *mode,
                                   int          remove)
{
    UINT n;
    DWORD l;
    HANDLE hfile = INVALID_HANDLE_VALUE;
    int fd = -1;
    FILE *f = NULL;
    char sTempDir[_MAX_PATH];
    char sTempFileName[_MAX_PATH];
#if defined(METRO) || !defined(GS_NO_UTF8)
    wchar_t wTempDir[_MAX_PATH];
    wchar_t wTempFileName[_MAX_PATH];
    wchar_t wPrefix[_MAX_PATH];
#endif

    memset(fname, 0, gp_file_name_sizeof);
    if (!gp_file_name_is_absolute(prefix, strlen(prefix))) {
        int plen = _MAX_PATH;

        /* gp_gettmpdir will always return a UTF8 encoded string unless
         * GS_NO_UTF8 is defined, due to the windows specific version of
         * gp_getenv being called (in gp_wgetv.c, not gp_getnv.c) */
        if (gp_gettmpdir(sTempDir, &plen) != 0) {
#ifdef METRO
            /* METRO always uses UTF8 for 'ascii' - there is no concept of
             * local encoding, so GS_NO_UTF8 makes no difference here. */
            l = GetTempPathWRT(_MAX_PATH, wTempDir);
            l = wchar_to_utf8(sTempDir, wTempDir);
#elif defined(GS_NO_UTF8)
            l = GetTempPathA(_MAX_PATH, sTempDir);
#else
            GetTempPathW(_MAX_PATH, wTempDir);
            l = wchar_to_utf8(sTempDir, wTempDir);
#endif
        } else
            l = strlen(sTempDir);
    } else {
        strncpy(sTempDir, prefix, _MAX_PATH);
        prefix = "";
        l = strlen(sTempDir);
    }
    /* Fix the trailing terminator so GetTempFileName doesn't get confused */
    if (sTempDir[l-1] == '/')
        sTempDir[l-1] = '\\';		/* What Windoze prefers */

    if (l <= _MAX_PATH) {
#ifdef METRO
        utf8_to_wchar(wTempDir, sTempDir);
        utf8_to_wchar(wPrefix, prefix);
        n = GetTempFileNameWRT(wTempDir, wPrefix, wTempFileName);
        n = wchar_to_utf8(sTempFileName, wTempFileName);
#elif defined(GS_NO_UTF8)
        n = GetTempFileNameA(sTempDir, prefix, 0, sTempFileName);
#else
        utf8_to_wchar(wTempDir, sTempDir);
        utf8_to_wchar(wPrefix, prefix);
        GetTempFileNameW(wTempDir, wPrefix, 0, wTempFileName);
        n = wchar_to_utf8(sTempFileName, wTempFileName);
#endif
        if (n == 0) {
            /* If 'prefix' is not a directory, it is a path prefix. */
            int l = strlen(sTempDir), i;

            for (i = l - 1; i > 0; i--) {
                uint slen = gs_file_name_check_separator(sTempDir + i, l, sTempDir + l);

                if (slen > 0) {
                    sTempDir[i] = 0;
                    i += slen;
                    break;
                }
            }
            if (i > 0) {
#ifdef METRO
                utf8_to_wchar(wPrefix, sTempDir + i);
                GetTempFileNameWRT(wTempDir, wPrefix, wTempFileName);
                n = wchar_to_utf8(sTempFileName, wTempFileName);
#elif defined(GS_NO_UTF8)
                n = GetTempFileNameA(sTempDir, sTempDir + i, 0, sTempFileName);
#else
                utf8_to_wchar(wPrefix, sTempDir + i);
                GetTempFileNameW(wTempDir, wPrefix, 0, wTempFileName);
                n = wchar_to_utf8(sTempFileName, wTempFileName);
#endif
            }
        }
        if (n != 0) {
#ifdef GS_NO_UTF8
            hfile = CreateFile(sTempFileName,
                               GENERIC_READ | GENERIC_WRITE | DELETE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL | (remove ? FILE_FLAG_DELETE_ON_CLOSE : 0),
                               NULL);
#else
            int len = utf8_to_wchar(NULL, sTempFileName);
            wchar_t *uni = (len > 0 ? malloc(sizeof(wchar_t)*len) : NULL);
            if (uni == NULL)
                hfile = INVALID_HANDLE_VALUE;
            else {
                utf8_to_wchar(uni, sTempFileName);
#ifdef METRO
                hfile = CreateFile2(uni,
                                    GENERIC_READ | GENERIC_WRITE | DELETE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    CREATE_ALWAYS | (remove ? FILE_FLAG_DELETE_ON_CLOSE : 0),
                                    NULL);
#else
                hfile = CreateFileW(uni,
                                    GENERIC_READ | GENERIC_WRITE | DELETE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL, CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL | (remove ? FILE_FLAG_DELETE_ON_CLOSE : 0),
                                    NULL);
#endif
                free(uni);
            }
#endif
        }
    }
    if (hfile != INVALID_HANDLE_VALUE) {
        /* Associate a C file handle with an OS file handle. */
        fd = _open_osfhandle((long)hfile, 0);
        if (fd == -1)
            CloseHandle(hfile);
        else {
            /* Associate a C file stream with C file handle. */
            f = fdopen(fd, mode);
            if (f == NULL)
                _close(fd);
        }
    }
    if (f != NULL) {
        if ((strlen(sTempFileName) < gp_file_name_sizeof))
            strncpy(fname, sTempFileName, gp_file_name_sizeof - 1);
        else {
            /* The file name is too long. */
            fclose(f);
            f = NULL;
        }
    }
    if (f == NULL)
        emprintf1(mem, "**** Could not open temporary file '%s'\n", fname);
    return f;
}

FILE *
gp_open_scratch_file(const gs_memory_t *mem,
                     const char        *prefix,
                           char        *fname,
                     const char        *mode)
{
    return gp_open_scratch_file_generic(mem, prefix, fname, mode, 0);
}

FILE *
gp_open_scratch_file_rm(const gs_memory_t *mem,
                        const char        *prefix,
                              char        *fname,
                        const char        *mode)
{
    return gp_open_scratch_file_generic(mem, prefix, fname, mode, 1);
}

/* Open a file with the given name, as a stream of uninterpreted bytes. */
FILE *
gp_fopen(const char *fname, const char *mode)
{
#ifdef GS_NO_UTF8
    return fopen(fname, mode);
#else
    int len = utf8_to_wchar(NULL, fname);
    wchar_t *uni;
    wchar_t wmode[4];
    FILE *file;

    if (len <= 0)
        return NULL;
    uni = malloc(len*sizeof(wchar_t));
    if (uni == NULL)
        return NULL;
    utf8_to_wchar(uni, fname);
    utf8_to_wchar(wmode, mode);
    file = _wfopen(uni, wmode);
    free(uni);
    return file;
#endif
}

int gp_stat(const char *path, struct _stat *buf)
{
#ifdef GS_NO_UTF8
    return stat(path, buf);
#else
    int len = utf8_to_wchar(NULL, path);
    wchar_t *uni;
    int ret;

    if (len <= 0)
        return -1;
    uni = malloc(len*sizeof(wchar_t));
    if (uni == NULL)
        return -1;
    utf8_to_wchar(uni, path);
    ret = _wstat(uni, buf);
    free(uni);
    return ret;
#endif
}

/* test whether gp_fdup is supported on this platform  */
int gp_can_share_fdesc(void)
{
    return 1;
}

/* Create a second open FILE on the basis of a given one */
FILE *gp_fdup(FILE *f, const char *mode)
{
    int fd = fileno(f);
    if (fd < 0)
        return NULL;

    fd = dup(fd);
    if (fd < 0)
        return NULL;

    return fdopen(fd, mode);
}

/* Read from a specified offset within a FILE into a buffer */
int gp_fpread(char *buf, uint count, int64_t offset, FILE *f)
{
    OVERLAPPED overlapped;
    DWORD ret;
    HANDLE hnd = (HANDLE)_get_osfhandle(fileno(f));

    if (hnd == INVALID_HANDLE_VALUE)
        return -1;

    memset(&overlapped, 0, sizeof(OVERLAPPED));
    overlapped.Offset = (DWORD)offset;
    overlapped.OffsetHigh = (DWORD)(offset >> 32);

    if (!ReadFile((HANDLE)hnd, buf, count, &ret, &overlapped))
        return -1;

    return ret;
}

/* Write to a specified offset within a FILE from a buffer */
int gp_fpwrite(char *buf, uint count, int64_t offset, FILE *f)
{
    OVERLAPPED overlapped;
    DWORD ret;
    HANDLE hnd = (HANDLE)_get_osfhandle(fileno(f));

    if (hnd == INVALID_HANDLE_VALUE)
        return -1;

    memset(&overlapped, 0, sizeof(OVERLAPPED));
    overlapped.Offset = (DWORD)offset;
    overlapped.OffsetHigh = (DWORD)(offset >> 32);

    if (!WriteFile((HANDLE)hnd, buf, count, &ret, &overlapped))
        return -1;

    return ret;
}

/* ------ Font enumeration ------ */

 /* This is used to query the native os for a list of font names and
  * corresponding paths. The general idea is to save the hassle of
  * building a custom fontmap file.
  */

void *gp_enumerate_fonts_init(gs_memory_t *mem)
{
    return NULL;
}

int gp_enumerate_fonts_next(void *enum_state, char **fontname, char **path)
{
    return 0;
}

void gp_enumerate_fonts_free(void *enum_state)
{
}

/* --------- 64 bit file access ----------- */
/* MSVC versions before 8 doen't provide big files.
   MSVC 8 doesn't distinguish big and small files,
   but provide special positioning functions
   to access data behind 4GB.
   Currently we support 64 bits file access with MSVC only.
 */

FILE *gp_fopen_64(const char *filename, const char *mode)
{
    return gp_fopen(filename, mode);
}

FILE *gp_open_scratch_file_64(const gs_memory_t *mem,
                              const char        *prefix,
                                    char         fname[gp_file_name_sizeof],
                              const char        *mode)
{
    return gp_open_scratch_file(mem, prefix, fname, mode);
}

FILE *gp_open_printer_64(const gs_memory_t *mem,
                               char         fname[gp_file_name_sizeof],
                               int          binary_mode)
{
#ifdef METRO
    return NULL;
#else
    /* Assuming gp_open_scratch_file_64 is same as gp_open_scratch_file -
       see the body of gp_open_printer. */
    return gp_open_printer(mem, fname, binary_mode);
#endif
}

#if defined(_MSC_VER) && _MSC_VER < 1400
    int64_t _ftelli64( FILE *);
    int _fseeki64( FILE *, int64_t, int);
#endif

gs_offset_t gp_ftell_64(FILE *strm)
{
#if !defined(_MSC_VER)
    return ftell(strm);
#elif _MSC_VER < 1200
    return ftell(strm);
#else
    return (int64_t)_ftelli64(strm);
#endif
}

int gp_fseek_64(FILE *strm, gs_offset_t offset, int origin)
{
#if !defined(_MSC_VER)
    return fseek(strm, offset, origin);
#elif _MSC_VER < 1200
    long offset1 = (long)offset;

    if (offset != offset1)
        return -1;
    return fseek(strm, offset1, origin);
#else
    return _fseeki64(strm, offset, origin);
#endif
}

bool gp_fseekable (FILE *f)
{
    struct __stat64 s;
    int fno;
    
    fno = fileno(f);
    if (fno < 0)
        return(false);
    
    if (_fstat64(fno, &s) < 0)
        return(false);

    return((bool)S_ISREG(s.st_mode));
}

/* -------------------------  _snprintf -----------------------------*/

#if defined(_MSC_VER) && _MSC_VER>=1900 /* VS 2014 and later have (finally) snprintf */
#  define STDC99
#else
/* Microsoft Visual C++ 2005  doesn't properly define snprintf,
   which is defined in the C standard ISO/IEC 9899:1999 (E) */

int snprintf(char *buffer, size_t count, const char *format, ...)
{
    if (count > 0) {
        va_list args;
        int n;

        va_start(args, format);
        n = _vsnprintf(buffer, count, format, args);
        buffer[count - 1] = 0;
        va_end(args);
        return n;
    } else
        return 0;
}
#endif
