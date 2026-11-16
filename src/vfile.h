/**
*   VFILE.H     - Header file for standard file routines
*
*
*   Description -
*
*
*
*
*   (c) Copyright 1994 by Brian Weed for Vanguard Information Systems
*   All rights reserved.
*
*
*   Revision History:
*
*   Version     Date        Author      Comments
*   -------     --------    ------      --------------------------------
*   1.00a       02-17-94     BCW        Created this file
*
**/
#ifndef VFILE_H
#define VFILE_H

#include <stdio.h>
#include <dos.h>          /* struct find_t */
#include "stdinc.h"

/*
* Don't mangle these function names
*/
#ifdef __cplusplus
extern "C" {
#endif

/*
* prototypes
*/
CHAR        *FileGetTempDir(void);
BOOLEAN      FileExists(const CHAR *);
ERROR_CODE   FileFlush(INT);
CHAR        *FileTempName(CHAR *);
LONG         FileSize(INT);
LONG         FileLength(const CHAR *);
BOOLEAN      FileFindFirst(const CHAR *, INT, struct find_t *);
BOOLEAN      FileFindNext(struct find_t *);
BOOLEAN      FileIsDirectory(const CHAR *);
ERROR_CODE   FileDelete(const CHAR *);
ERROR_CODE   FilePrint(const CHAR *, FILE *);

#ifdef __cplusplus
}
#endif

#endif  /* VFILE_H */
