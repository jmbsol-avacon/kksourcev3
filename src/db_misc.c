/**
*   DB_MISC.C   -
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
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <dir.h>
#include <dos.h>
#include <conio.h>
#include <errno.h>
#include "db_defs.h"
#include "vmouse.h"
#include "vmisc.h"
#include "vfile.h"

/*
* Globals
*/
extern struct date today;
extern SYSREC sysvar;
extern CHAR *tempDir;
extern BOOLEAN mouseActive;

FILE *lpt1, *lpt2, *defPrinter;
struct text_info orig;

/****************************************************************************
strdatecmp - compares to character dates.
****************************************************************************/
INT strdatecmp(CHAR *date1, CHAR *date2)
{
    struct date d1, d2;

    d1 = datecheck(date1);
    d2 = datecheck(date2);
    if (!d1.da_year || !d2.da_year)
        return(-99);

    return(datecmp(&d1, &d2));
}

/****************************************************************************
datecmp - compares two date structures.
****************************************************************************/
INT datecmp(struct date *d1, struct date *d2)
{
    if (d1->da_year+20 > d2->da_year+20)
        return(1);

    if (d1->da_year+20 < d2->da_year+20)
        return(-1);

    if (d1->da_mon > d2->da_mon)
        return(1);

    if (d1->da_mon < d2->da_mon)
        return(-1);

    return(d1->da_day - d2->da_day);

    /*if (d1->da_day > d2->da_day)
        return(1);
    if (d1->da_day < d2->da_day)
        return(-1);
    return(0);*/
}

/****************************************************************************
longcmp - compares two long numbers.
****************************************************************************/
LONG longcmp(LONG *l1, LONG *l2)
{
    return(*l1 - *l2);
}

/****************************************************************************
intcmp - compares two integers.
****************************************************************************/
INT intcmp(SHORT *i1, SHORT *i2)
{
    return(*i1 - *i2);
}

/****************************************************************************
floatcmp - compares two floating point numbers.
****************************************************************************/
INT floatcmp(FLOAT *f1, FLOAT *f2)
{
    if (*f1 == *f2)
        return(0);

    if (*f1 > *f2)
        return(1);

    return(-1);
}


/*****************************************************************************
s_alloc, s_realloc - calls malloc/realloc exits if error.
*****************************************************************************/
VOID *s_alloc(UINT size)
{
    VOID *ptr;

    /*
    * allocate the buffer
    */
    if ((ptr = MemAlloc(size)) == NULL) {
        message(M_SCR|M_ERR|M_KEY, "Memory Allocation Error (%u requested) - hit any key", size);
        quit(0);
    }

    return(ptr);
}

VOID *s_realloc(VOID *ptr, UINT size)
{
    if ((ptr = MemRealloc(ptr, size)) == NULL) {
        message(M_SCR|M_ERR|M_KEY, "Memory Re-Allocation Error (%u requested) - hit any key", size);
        quit(0);
    }
    return(ptr);
}

VOID s_free(VOID *ptr)
{
    MemFree(ptr);
}


/*****************************************************************************
beep() - Sends a beep thru the PC speaker - For Invalid Commands
*****************************************************************************/
VOID beep(VOID)
{
    Sound(150);
}


/*****************************************************************************
ErrorChecking() - Checks Many Various Things to Ensure KK runs Correctly
*****************************************************************************/
VOID ErrorChecking(VOID)
{
    /*
    * Test DOS Version # - Should only run with DOS v3.1 or higher.
    * DOS v3.2 NOT recommended.
    */
    if (_osmajor < 3) {
        printf("\nError: Kwik Kuote will not run under Dos v%d.%d.  Dos 3.1 or higher is required.\n", _osmajor, _osminor);
        exit(-1);
    }

    /*
    * Test to make sure Kwik Kuote is Executed from the correct Directory
    */
    if (!FileExists("DB.CFG")) {
        printf("\nError: You must run Kwik Kuote from Kwik Kuote's Main Directory. (i.e. C:\\KK>)\n");
        exit(-1);
    }

    /*
    * Test if SHARE.EXE is installed
    */
    if (!IsShareInstalled()) {
        printf("\nSHARE Must be Installed - Type SHARE at the DOS Prompt.\n");
        exit(-1);
    }
}


INT Control_C(VOID)
{
    USHORT key;

    if (KeyPress(&key) && (key == KEY_CTRL_C))
        return(YES);

    return(NO);
}


VOID Inits()
{
    MOUSE_CLICK *p, *newMouseClicks;
    INT i, j, oldSize, newSize, type;

    /* seed the random generator */
    srand((UINT)time(NULL));

    SetCursor(CUR_OFF);

    /* save original screen info */
    gettextinfo(&orig);

    /*
    * Get TEMP Environment Variable
    */
    tempDir = FileGetTempDir();

    /*
    * try to open LPT2:
    */
    defPrinter = lpt1 = stdprn;
    if ((lpt2 = fileopn("LPT2", "wb")) == NULL) {
        message(M_SCR|M_KEY, "LPT2 could not be opened - hit any key");
    }

    /* set default colors */
    textattr(NORMAL);

    /* get todays date */
    getdate(&today);

    /*
    * Initialize Mouse (if mouse driver is loaded)
    */
    if (MouseInit()) {
        mouseActive = TRUE;
        MouseShowCursor();
    }

#ifndef DEMO

    /*
    * Test Disk Free Space - Should not run with less than 100k
    */
    if  ((DiskGetFreeSpace(0)) <= 100000) {
        if (message(M_SCR|M_KEY, "WARNING: You are getting low on disk space - hit any key") == KEY_ESC1)
            quit(0);
    }
#endif

    /*
    * Modify the mouseClicks field for several input screen
    */
    oldSize = 14; // sizeof(*p) / sizeof(MOUSE_CLICK);
    for(type = CD; type <= PO; type++)
    {
        if(type == PL)
            continue;

        p = db[type].mousePoints;
        newSize = oldSize + db[type].fields; // sizeof(*(db[type].f)) / sizeof(FIELDPARAMS);
        newMouseClicks = (MOUSE_CLICK *) s_alloc(sizeof(MOUSE_CLICK) * (newSize+1));
    
        /* the first half are the old mouse click settings, which is pre-defined in db_defs.h */
        for(i = 0; i < oldSize; i++)
            newMouseClicks[i] = p[i];
    
        /* the second half are the new mouse click settings */
        for(; i < newSize; i++)
        {
            j = i - oldSize;
            FillMouseClickPoint(&newMouseClicks[i],
            (db[type].f[j].dx+strlen(db[type].f[j].header)) * 8, (db[type].f[j].dy - 1) * 8,
            (db[type].f[j].dx+strlen(db[type].f[j].header)+1 + db[type].f[j].width) * 8, (db[type].f[j].dy - 1) * 8,
            KEY_MOUSE_0 + j);
        }
        newMouseClicks[i].xStart = newMouseClicks[i].yStart =
        newMouseClicks[i].xEnd = newMouseClicks[i].yEnd = newMouseClicks[i].key = 0;
    
        db[type].mousePoints = newMouseClicks;
    }
}


/**
*  Perform cleanups and Exit
*
**/
VOID quit(INT code)
{
    INT type;

    /*
    * free the tax memory 
    */
    free_tx();

    /*
    * close LPT2 device if it was opened
    */
    if (lpt2 != NULL) {
        filecls(lpt2);
    }

    /*
    * free the TEMP directory global variable
    */
    s_free(tempDir);

    /*
    * turn off mouse cursor
    */
    if (mouseActive)
        MouseHideCursor();

    /*
    * return screen to original settings
    */
    window(1, 1, orig.screenwidth, orig.screenheight);
    textattr(orig.attribute);
    clrscr();
    SetCursor(CUR_ON);

    /* free newMouseClicks memory */
    for(type = CD; type <= PO; type++)
    {
        if(type == PL)
            continue;

        s_free(db[type].mousePoints);
    }

    /* exit with result code */
    exit(code);
}


INT SysCmd(const CHAR *format, ...)
{
    va_list argptr;
    CHAR *cmdStr;
    INT x;

    /* allocate a suitable buffer to hold the command string */
    cmdStr = (CHAR *)s_alloc(120);

    /*
    * build the output string
    */
    va_start(argptr, format);
    vsprintf(cmdStr, format, argptr);
    va_end(argptr);

    /*
    * turn off mouse cursor for system command
    */
    if (mouseActive)
        MouseHideCursor();

    /* execute the system command */
    store();
    clrscr();
    if ((x = system(cmdStr)) != 0) {
        message(M_SCR|M_ERR|M_KEY, "Could not execute '%s' - hit any key", cmdStr);
    }
    restore();

    /*
    * turn mouse cursor back on
    */
    if (mouseActive)
        MouseShowCursor();

    /* free that temp buffer */
    s_free(cmdStr);

    /* returns -1 if command could not be executed */
    return(x);
}


/*****************************************************************************
random_n(VOID) - Generates a Totally Random Number from 0 to 999
*****************************************************************************/
INT random_n(VOID)
{
    struct time t;
    gettime(&t);
    return(((INT)t.ti_hund)*10+random(10));
}
