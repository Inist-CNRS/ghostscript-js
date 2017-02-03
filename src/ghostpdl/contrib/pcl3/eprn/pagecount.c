/******************************************************************************
  File:     $Id: pagecount.c,v 1.6 2000/10/07 17:48:49 Martin Rel $
  Contents: Simple (page) count file facility on UNIX
  Author:   Martin Lottermoser, Greifswaldstrasse 28, 38124 Braunschweig,
            Germany. E-mail: Martin.Lottermoser@t-online.de.

*******************************************************************************
*									      *
*	Copyright (C) 1997, 1998, 2000 by Martin Lottermoser		      *
*	All rights reserved						      *
*									      *
******************************************************************************/

/* This file should be ignored under windows */
#ifdef _MSC_VER
int dummy;
#else

/*****************************************************************************/

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE	500
#endif

#include "std.h"

/* Standard headers */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

/* Specific headers */
#include "pagecount.h"

/*****************************************************************************/

#define ERRPREFIX	"?-E Pagecount module: "
#define WARNPREFIX	"?-W Pagecount module: "

/******************************************************************************

  Function: lock_file

  This function locks the specified file 'f' with a lock of type 'lock_type'.
  'filename' is merely used for error messages.

  The function returns zero on success and issues error messages on stderr
  if it fails.

******************************************************************************/

static int lock_file(const char *filename, FILE *f, int lock_type)
{
  int
    fd,
    rc,
    tries;
  struct flock cmd;

  /* Obtain file descriptor */
  fd = fileno(f);
  if (fd == -1) {
    fprintf(stderr, ERRPREFIX "Cannot obtain file descriptor (%s).\n",
      strerror(errno));
    fclose(f);
    return -1;
  }

  /* Lock the file */
  cmd.l_type = lock_type;
  cmd.l_whence = SEEK_SET;	/* 'start' is interpreted from start of file */
  cmd.l_start = 0;
  cmd.l_len = 0;		/* until EOF */
  tries = 1;
  while ((rc = fcntl(fd, F_SETLK, &cmd)) != 0 && tries < 3) {
    tries++;
    sleep(1);
  }
  if (rc != 0) {
    fprintf(stderr, ERRPREFIX
      "Cannot obtain lock on page count file `%s' after %d attempts.\n",
      filename, tries);
    return -1;
  }

  return 0;
}

/******************************************************************************

  Function: read_count

  This function reads the contents of the open page count file.

******************************************************************************/

static int read_count(const char *filename, FILE *f, unsigned long *count)
{
  if (fscanf(f, "%lu\n", count) != 1) {
    if (feof(f) && !ferror(f)) *count = 0;	/* Empty file */
    else {
      fprintf(stderr, ERRPREFIX "Strange contents in page count file `%s'.\n",
        filename);
      return -1;
    }
  }

  return 0;
}

/******************************************************************************

  Function: pcf_getcount

  This routine reads the page count file 'filename' and returns the count read
  in '*count'. If the file does not exist, the value 0 is assumed.

  The function returns zero on success. On error, a message will have been
  issued on stderr.

******************************************************************************/

int pcf_getcount(const char *filename, unsigned long *count)
{
  FILE *f;

  /* Should we use a page count file? */
  if (filename == NULL || *filename == '\0') return 0;

  /* If the file does not exist, the page count is taken to be zero. */
  if (access(filename, F_OK) != 0) {
    *count = 0;
    return 0;
  }

  /* Open the file */
  if ((f = fopen(filename, "r")) == NULL) {
    fprintf(stderr, ERRPREFIX "Cannot open page count file `%s': %s.\n",
      filename, strerror(errno));
    return -1;
  }

  /* Lock the file for reading (shared lock) */
  if (lock_file(filename, f, F_RDLCK) != 0) {
    fclose(f);
    return 1;
  }

  /* Read the contents */
  if (read_count(filename, f, count) != 0) {
    fclose(f);
    return -1;
  }

  /* Close the file, releasing the lock */
  fclose(f);

  return 0;
}

/******************************************************************************

  Function: pcf_inccount

  This function opens the page count file 'filename' and increases the number
  stored there by 'by'. If the file does not exist it will be created and
  treated as if had contained 0.

  The function returns zero on success and issues error messages on stderr if
  it fails.

******************************************************************************/

int pcf_inccount(const char *filename, unsigned long by)
{
  FILE *f;
  int rc;
  unsigned long count;

  /* Should we use a page count file? */
  if (filename == NULL || *filename == '\0') return 0;

  /* Open the file. We need to access the old contents: this excludes the "w",
     "a" and "w+" modes. The operation should create the file if it doesn't
     exist: this excludes the "r" and "r+" modes. Hence the only choice is "a+".
     Note that this procedure makes it unavoidable to accept an empty file as
     being valid. This is, however, anyway necessary because of the fopen() and
     fcntl() calls being not in one transaction.
  */
  if ((f = fopen(filename, "a+")) == NULL) {
    fprintf(stderr, ERRPREFIX "Cannot open page count file `%s': %s.\n",
      filename, strerror(errno));
    return 1;
  }

  /* Lock the file for writing (exclusively) */
  if (lock_file(filename, f, F_WRLCK) != 0) {
    fclose(f);
    return 1;
  }

  /* Reposition on the beginning. fopen() with "a" as above opens the file at
     EOF. */
  if (fseek(f, 0L, SEEK_SET) != 0) {
    fprintf(stderr, ERRPREFIX "fseek() failed on `%s': %s.\n",
      filename, strerror(errno));
    fclose(f);
    return 1;
  }

  /* Read the contents */
  if (read_count(filename, f, &count) != 0) {
    fclose(f);
    return -1;
  }

  /* Rewrite the file */
  rc = 0;
  {
    FILE *f1 = fopen(filename, "w");

    if (f1 == NULL) {
      fprintf(stderr, ERRPREFIX
        "Error opening page count file `%s' a second time: %s.\n",
        filename, strerror(errno));
      rc = 1;
    }
    else {
      if (fprintf(f1, "%lu\n", count + by) < 0) {
        fprintf(stderr, ERRPREFIX "Error writing to `%s': %s.\n",
          filename, strerror(errno));
        rc = -1;
      }
      if (fclose(f1) != 0) {
        fprintf(stderr,
          ERRPREFIX "Error closing `%s' after writing: %s.\n",
          filename, strerror(errno));
        rc = -1;
      }
    }
  }

  /* Close the file (this releases the lock) */
  if (fclose(f) != 0)
    fprintf(stderr, WARNPREFIX "Error closing `%s': %s.\n",
      filename, strerror(errno));

  return rc;
}

#ifdef TEST
/******************************************************************************

  Function: main

  This routine can be used for testing.

******************************************************************************/

int main(int argc, char **argv)
{
  const char *filename = "pages.count";
  int rc;
  unsigned long
    by = 1,
    count = 0;

  if (argc > 1) {
    if (sscanf(argv[1], "%lu", &by) != 1) {
      fprintf(stderr, ERRPREFIX "Illegal number: `%s'.\n", argv[1]);
      exit(EXIT_FAILURE);
    }
    if (argc > 2) filename = argv[2];
    if (argc > 3) {
      fprintf(stderr, ERRPREFIX "Too many arguments.\n");
      exit(EXIT_FAILURE);
    }
  }

  rc = pcf_getcount(filename, &count);
  if (rc == 0)
    printf("Initial count returned by pcf_getcount(): %lu.\n", count);
  else fprintf(stderr, "? Error from pcf_getcount(), return code is %d.\n", rc);

  rc = pcf_inccount(filename, by);

  exit(rc == 0? EXIT_SUCCESS: EXIT_FAILURE);
}

#endif	/* TEST */

#endif /* _MSC_VER */
