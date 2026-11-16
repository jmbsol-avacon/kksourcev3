/**
*   FILE.C      -
*
*
*   Description -
*
*
*
*
*   (c) Copyright 1994 by Brian Weed for Automation Solutions, Inc.
*   All rights reserved.
*
*
*   Revision History:
*
*   Version     Date        Author      Comments
*   -------     --------    ------      --------------------------------
*   3.00a       09-07-93     BCW        Revision 3
*
**/
#include <stdlib.h>
#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <dos.h>
#include <share.h>
#include <errno.h>
#include "db_defs.h"
#include "vfile.h"


#define BUF_SIZE 16384

extern CHAR *tempDir;

/* local prototypes */
VOID copydata(INT, INT, LONG);

/***************************************************************************
FileCopyH - copies two open files by file handle.  (leaves open)
***************************************************************************/
VOID FileCopyH(INT source, INT dest)
{
    /*
    * seek to begining and copy entire file by file handles
    */
    FileSeek(source, 0, SEEK_SET);
    FileSeek(dest, 0, SEEK_SET);
    copydata(source, dest, FileSize(source));
}


/***************************************************************************
copydata - copies a block from one file to another file at the current file
           pointer positons.
***************************************************************************/
VOID copydata(INT source, INT dest, LONG size)
{
    LONG getsize;
    CHAR *buf;

    /*
    * allocate an IO buffer
    */
    if (size > 0) {

        buf = (CHAR *)s_alloc(min(BUF_SIZE, size));

        /*
        * copy the file in BUF_SIZE chunks
        */
        while (!eof(source) && (size > 0)) {

            if ((getsize = size) >= BUF_SIZE)
                getsize = BUF_SIZE;

            size -= getsize;
            FileRead(source, buf, getsize);
            FileWrite(dest, buf, getsize);
        }
        /* free the IO buffer */
        s_free(buf);
    }
}


/***************************************************************************
FileAppend - Appends fname1 to the end of fname2
***************************************************************************/
VOID FileAppend(const CHAR *sourceName, const CHAR *destName)
{
    INT source, dest;

    /*
    * open source and destination files
    */
    source = FileOpen(sourceName);
    dest = FileOpen(destName);

    /* seek to end of destination file (destName) */
    FileSeek(dest, 0, SEEK_END);

    /* append source to the back of dest */
    copydata(source, dest, FileSize(source));

    /*
    * close both files
    */
    FileClose(dest);
    FileClose(source);
}

/***************************************************************************
FileCopy - copies files specified by name (must not be open)
***************************************************************************/
VOID FileCopy(const CHAR *sourceName, const CHAR *destName)
{
    INT source, dest;

    /*
    * open source and destination files
    */
    source = FileOpen(sourceName);
    dest = FileOpen(destName);

    /* copy the file by file handle */
    copydata(source, dest, FileSize(source));

    /*
    * close both files
    */
    FileClose(dest);
    FileClose(source);
}


/****************************************************************************
diskspace - Returns Disk Free Space.
        drive - Drive to check free space (A=1,B=2,C=3...)
****************************************************************************/
LONG diskspace(INT drive)
{
    struct diskfree_t df;

    if (_dos_getdiskfree(drive, &df)) {
        message(M_SCR|M_ERR|M_KEY, "Could not Read Drive %c: - hit any key", drive+64);
        return(-1L);
    } else {
        return((LONG)((LONG)df.avail_clusters*(LONG)df.bytes_per_sector*(LONG)df.sectors_per_cluster));
    }
}


/***************************************************************************
FileWrite - the write command with error message.
***************************************************************************/
INT FileWrite(INT handle, VOID *buf, INT length)
{
    /* can't read from a null pointer */
    Assert(buf != NULL);

    while (write(handle, buf, length) == -1) {
        if (errno == EACCES) {
            message(M_SCR, "File locked - retrying...");
            if (Control_C())
                quit(-1);
        } else {
            message(M_SCR|M_ERR|M_KEY, "Write Error: %s - hit any key", strerror(errno));
            return(-1);
        }
    }
    return(0);
}

/***************************************************************************
FileRead - the read command with error message.
***************************************************************************/
INT FileRead(INT handle, VOID *buf, INT length)
{
    /* can't write to a null pointer */
    Assert(buf != NULL);

    while (read(handle, buf, length) == -1) {
        if (errno == EACCES) {
            message(M_SCR, "File locked - retrying. . .");
            if (Control_C())
                quit(-1);
        } else {
            message(M_SCR|M_ERR|M_KEY, "Read Error: %s - hit any key", strerror(errno));
            return(-1);
        }
    }
    return(0);
}

/*************************************************************************
FileLock - Locks Normal files by file handle
*************************************************************************/
INT FileLock(INT handle)
{
    while (lock(handle, 0, sizeof(INT)) == -1) {
        if (errno == EACCES) {
            message(M_SCR, "File busy - please wait. . .");
            if (Control_C())
                quit(0);

        } else {
            return(-1);
        }
    }
    return(1);
}

/************************************************************************
FileUnlock - Unlocks Normal files by file handle
*************************************************************************/
INT FileUnlock(INT handle)
{
    if (unlock(handle, 0, sizeof(INT)) == -1)
        return(-1);

    return(1);
}

/************************************************************************
FileOpen() - Opens a file and returns a file handle
*************************************************************************/
INT FileOpen(const CHAR *fname)
{
    INT fh;

    /* can't open file if there is no name */
    Assert(fname != NULL);

    if ((fh = handleopn(fname, O_BINARY|O_CREAT|SH_DENYNO|O_RDWR, S_IREAD|S_IWRITE)) == -1)
        message(M_SCR|M_ERR|M_KEY, "Unable to open %s due to: %s", fname, strerror(errno));
    return(fh);
}

/************************************************************************
FileClose() - Closes a file by it's handle
*************************************************************************/
VOID FileClose(INT handle)
{
    if (handle > 0)
        handlecls(handle);
}

/************************************************************************
FileSeek() - Seek file
*************************************************************************/
LONG FileSeek(INT handle, LONG offset, INT fromWhere)
{
    LONG retVal;

    if ((retVal = lseek(handle, offset, fromWhere)) == -1)
        message(M_SCR|M_ERR|M_KEY, "Unable to Seek file %d - hit any key", handle);

    return(retVal);
}


INT FileCreateTemp(CHAR *fileName)
{
    CHAR tempBuf[80];
    INT handle;

    /* open the temporary file */
    if ((handle = FileCreate(FileTempName(tempBuf))) != -1) {

        /*
        * guarantee access to the file
        */
        FileClose(handle);                /* This might not be needed.  */
        handle = FileOpen(tempBuf);       /* Lets test it for a bit.    */

        /*
        * if supplied a buffer, then fill it, otherwise they don't get to see filename
        */
        if (fileName != NULL)
            strcpy(fileName, tempBuf);
    }

    return(handle);
}


INT FileCreate(const CHAR *fname)
{
    INT fh;

    /* can't open file if there is no name */
    Assert(fname != NULL);

    if ((fh = handleopn(fname, O_BINARY|O_CREAT|O_TRUNC|SH_DENYNO|O_RDWR, S_IREAD|S_IWRITE)) == -1)
        message(M_SCR|M_ERR|M_KEY, "Unable to create %s due to: %s", fname, strerror(errno));

    return(fh);
}

/* debug functions */
FILE *fileopn(const CHAR *fname, const CHAR *mode)
{
    FILE *fp;

    fp = fopen(fname, mode);
//    message(M_SCR|M_KEY, "OPEN: name = %s, id = %d", fname, fp);
    return (fp);
}

VOID filecls(FILE *fp)
{
//    message(M_SCR|M_KEY, "CLOSE: %d", fp);
    fclose(fp);
}

INT handleopn(const CHAR *path, int access, UINT mode)
{
    INT handle;

    handle = open(path, access, mode);
//    message(M_SCR|M_KEY, "                     OPEN: file: %s, handle = %d", path, handle);
    return (handle);
}

VOID handlecls(INT handle)
{
//    message(M_SCR|M_KEY, "                     CLOSE: handle = %d", handle);
    close(handle);
}

