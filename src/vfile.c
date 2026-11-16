/**
*   VFILE.C     -
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include "vfile.h"
#include "vmisc.h"
#include "db_defs.h"

/**
*  name      FileGetTempDir - Gets pointer to directory named in the TEMP environment variable
*
*  synopsis  tmpDir = FileGetTempDir();
*
*
*  purpose   To provide a temporary directory (TEMP, TMP, or current directory)
*
*
*  returns   tmpDir = pointer to the TEMP directory - must be freed
*
**/
CHAR *FileGetTempDir(VOID)
{
    CHAR *ptr, *tmpDir;
    INT n;

    /*
    * Get TEMP or TMP Environment Variable
    */
    if (((ptr = getenv("TEMP")) != NULL) || ((ptr = getenv("TMP")) != NULL)) {
	n = strlen(ptr) - 1;
	if (*(ptr + n) == '\\')
	    *(ptr + n) = '\0';

	/* verify that this is a valid directory */
	Assert(FileIsDirectory(ptr));

	/*
	* use current directory if this dir does not exist
	*/
	if (!FileIsDirectory(ptr)) {

	    *ptr = '.';
	    *(ptr + 1) = '\0';
	}

	tmpDir = (CHAR *)MemAlloc(n+3);

	sprintf(tmpDir, "%s\\", ptr);
    /*
    * Neither TEMP nor TMP environment variables are set
    */
    } else {

	tmpDir = (CHAR *)MemAlloc(4);
	strcpy(tmpDir, ".\\");
    }

    return(tmpDir);
}


/**
*  name      FileExists - Checks to see if specified file exists
*
*  synopsis  exists = FileExists(fileName)
*            const CHAR *fileName;              name of file to check existance
*
*  purpose   To determine is a specified file exists
*
*
*  returns   exists = TRUE if file exists, FALSE if file does not exist
*
**/
BOOLEAN FileExists(const CHAR *fileName)
{
    FILE *fp;

    /* can't open a file with no name */
    Assert(fileName != NULL);

    if ((fp = fileopn(fileName, "r")) == NULL) {
	return(FALSE);
    } else {
    filecls(fp);
	return(TRUE);
    }
}


/**
*  name      FileFlush - Causes file data in Dos Buffers to be written to disk
*
*  synopsis  errCode = FileFlush(handle)
*            INT handle;                    handle of file to flush
*
*  purpose   To commit a file's data to disk
*
*
*  returns   errCode = error return code
*
**/
ERROR_CODE FileFlush(INT handle)
{
    ERROR_CODE errCode;
    USHORT retVal;

    /* assume no error */
    errCode = ERR_NONE;

    retVal = (USHORT)(_commit(handle) != 0);   /* INT 21h/68h commit file */

    /*
    * if error
    */
    if (retVal)
	errCode = ERR_FILEWRITE;

    return(errCode);
}


/**
*  name      FileTempName - returns a unique file name
*
*  synopsis  fileName = FileTempName(fileName);
*            CHAR *fileName;                buffer to fill with new unique name
*
*  purpose   To provide a temporary file name (uses TEMP directory also)
*
*
*  returns   pointer to buffer containing new unique file name
*
**/
CHAR *FileTempName(CHAR *buf)
{
    CHAR *tmpPtr, *dirPtr, tmpBuf[8];

    /* can't write to a null pointer */
    Assert(buf != NULL);

    /*
    * get temporary directory specified in TEMP environment variable
    * Note: pointer must be freed
    */
    dirPtr = FileGetTempDir();

    sprintf(tmpBuf, "%04x", rand());
    tmpPtr = tempnam(dirPtr, tmpBuf);

    Assert(strlen(tmpBuf) < 5);

    /* free that TEMP directory buffer */
    MemFree(dirPtr);

    /* tell me if something went wrong */
    Assert(tmpPtr != NULL);

    if (tmpPtr != NULL) {

	strcpy(buf, tmpPtr);
	MemFree(tmpPtr);
    }

    /* this is a bug if the file exists */
    Assert(!FileExists(buf));

    return(buf);
}


/**
*  name      FileSize - gets length of file in bytes (vie file handle)
*
*  synopsis  length = FileSize(handle);
*            INT handle;                   file handle
*
*  purpose   To determine the size of a file in bytes
*
*
*  returns   length = size of specified file
*
**/
LONG FileSize(INT handle)
{
    return(filelength(handle));
}


/**
*  name      FileLength - gets length of file in bytes (via file name)
*
*  synopsis  length = FileLength(fileName);
*            const CHAR *fileName          name of file to get length for
*
*  purpose   To determine the size of a file in bytes
*
*
*  returns   length = size of specified file (or -1 if not exist or error)
*
**/
LONG FileLength(const CHAR *fileName)
{
    struct find_t nextFile;

    /* can't use null pointer */
    Assert(fileName != NULL);

    if (!FileFindFirst(fileName, 0, &nextFile))
	nextFile.size = -1;

    return(nextFile.size);
}


/**
*
*  name      FileFindFirst - Find first file matching searchPath
*
*  synopsis  errCode = FileWrite(searchPath, attribs, nextFile)
*            CHAR *searchPath;               path and filespec to search
*            INT attribs;                    attributes for matching files
*            find_t nextFile;             structure to hold found file
*
*  purpose   To search for the first file matching 'searchPath'
*
*  returns   found = TRUE if a match was found, FALSE if none found
*
**/
BOOLEAN FileFindFirst(const CHAR *searchPath, INT attribs, struct find_t *nextFile)
{
    struct find_t temp;

    /* can't access a null pointer */
    Assert(searchPath != NULL);

    if (!_dos_findfirst(searchPath, attribs, &temp)) {

	if (nextFile != NULL)
	    memcpy(nextFile, &temp, sizeof(struct find_t));

	return(TRUE);
    }

    return(FALSE);
}


/**
*
*  name      FileFindNext - searches for next file matching previous spec
*
*  synopsis  errCode = FileFindNext(nextFile)
*            find_t nextFile;             structure to hold found file
*
*  purpose   To search for the next file matching 'searchPath'
*
*  returns   found = TRUE if a match was found, FALSE if none found
*
**/
BOOLEAN FileFindNext(struct find_t *nextFile)
{
    if (nextFile != NULL) {

	if (!_dos_findnext(nextFile)) {

	    return(TRUE);
	}
    }

    return(FALSE);
}


/**
*
*  name      FileIsDirectory - checks if 'path' is a valid directory
*
*  synopsis  errCode = FileIsDirectory(path)
*            const CHAR *path             directory to check the existance of
*
*  purpose   To determine if 'path' exists
*
*
*  returns   exists = TRUE if specified entry is a directory, else FALSE
*
**/
BOOLEAN FileIsDirectory(const CHAR *path)
{
    CHAR buf[128];
//    FILE *fp;

    /* can't access null pointer */
    Assert(path != NULL);
    Assert(strlen(path) < 128);

    strncpy(buf, path, 128);

    /*
    * if no backslash on end then append one
    */
    if (buf[strlen(buf) - 1] != 0x5C)
	strcat(buf, "\\");

    /* the following code tries to open a NUL file in that dir,
       which causes a bug which leading to the fclose doesn't really
       close the file!! */
    buf[strlen(buf) - 1] = '\0'; /* no trailing backslash */
    if(access(buf, 0) == 0) 
	return(TRUE);
    else
	return(FALSE);

#ifdef old
    /*
    * try to open the NUL device in this directory
    */
    strcat(buf, "NUL");
    if ((fp = fileopn(buf, "r")) != NULL) {

	/* must be a valid directory */
    filecls(fp);
	return(TRUE);
    }

    /* not a valid directory */
    return(FALSE);
#endif
}


/**
*
* name      FileDelete - delete specified file
*
* synopsis  errCode = FileDelete(fileName)
*           const CHAR *fileName              file to delete
*
* purpose   To delete a file
*
*
* returns   errCode - error return code
*
**/
ERROR_CODE FileDelete(const CHAR *fileName)
{
    ERROR_CODE errCode;

    /* assume no error */
    errCode = ERR_NONE;

    if (remove(fileName) == -1)
	errCode = ERR_FILEDEL;

    return(errCode);
}


/**
*
* name      FilePrint - prints specified file to specified device
*
* synopsis  errCode = FilePrint(fileName, printer)
*           const CHAR *fileName              file to print
*           FILE *printer                     destination device to print to
*
* purpose   To print a file - adds a form feed at the end
*
*
* returns   errCode - error return code
*
**/
ERROR_CODE FilePrint(const CHAR *fileName, FILE *printer)
{
    FILE *fp;
    INT i;
    ERROR_CODE errCode;

    /* assume no error */
    errCode = ERR_NONE;

    /* can't use a nul pointer */
    Assert(fileName != NULL);
    Assert(printer != NULL);

    /* file must exist to print it */
    Assert(FileExists(fileName));

    if ((fp = fileopn(fileName, "rb")) == NULL) {
	errCode = ERR_FILEOPEN;
    } else {

	while (!feof(fp)) {
	    i = fgetc(fp);
	    fputc(i, printer);
	}

    filecls(fp);

	/*
	* write a form feed
	*/
	fputc(12, printer);
	fflush(printer);
    }

    return(errCode);
}
