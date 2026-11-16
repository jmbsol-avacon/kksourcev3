/**
*   DB_PRINT.C  -
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
#include <alloc.h>
#include <string.h>
#include <stdio.h>
#include <dos.h>
#include <bios.h>
#include <conio.h>
#include "db_defs.h"
#include "vmisc.h"
#include "vfile.h"

extern SYSREC sysvar;
extern MOUSE_CLICK *points;
extern FILE *lpt1, *lpt2, *defPrinter;

/*****************************************************************************
printmenu - generic db file printing routine.
*****************************************************************************/
VOID printmenu(VOID)
{
    DB_INFO tempdb;
    CHAR *fprompt[50], help[20];
    LONG fsize;
    INT choice, lastchoice=1, fl=0, direct=KEY_UP, i;
    CHAR *prompt[]={
        "CUSTOMIZED REPORTS MENU",
        "",
        "",
        "SORT FILE",
        "PRINT FILE",
        "RETURN TO MAIN MENU",
        NULL
    };

    STATIC MOUSE_CLICK custMenu[] = {
        {152, 64, 376, 64, KEY_1},
        {152, 80, 432, 80, KEY_2},
        {152, 96, 249, 96, KEY_3},
        {152, 112, 256, 112, KEY_4},
        {152, 128, 328, 128, KEY_5},
        {0, 0, 0, 0, 0}
    };

    MOUSE_CLICK *lastP;
    INT test,choice2,fld,found,mask[50];
    VOID *ptr=NULL,*newPtr=NULL;
    CHAR *sortdir[]= { "ASCENDING", "DESCENDING", NULL };
    INT fd=-1;
    CHAR filename[80];

    prompt[1] = (CHAR *)s_alloc(80);
    prompt[2] = (CHAR *)s_alloc(80);

    sprintf(help, "Use %c%c and Enter", 27, 26);
    sprintf(prompt[1], "LOAD FILE (CURRENT: NONE)");
    sprintf(prompt[2], "SELECT RECORDS (0 RECORDS TOTAL)");

    /*s_openfiles(0);*/
    s_closeall();

    /* save last mouse click points */
    lastP = points;

    memset(&tempdb, 0, sizeof(DB_INFO));

    do {
        /* register this menu's mouse points */
        points = custMenu;

        switch (choice = menu(prompt, 1, 20, 0, lastchoice, HLP_PRINTMENU)) {

            case 1:
                lastchoice=choice;
                if (fd != -1) {
                    s_close(fl);
                    db[fl] = tempdb;
                    s_close(fl);
                    FileDelete(filename);   /* was remove(filename);*/
                    fd = -1;
                }

                if ((fl = sel_file(0)) != -1)     /* there was a ; here */
                {
                    if (seccheck(fl,'R')) {
                        INT n;
                        if (db[fl].newfile) {
                            if ((*db[fl].newfile)(fl) == -1)
                                break;
                        }
                        if ((fd = s_tempcopy(fl, filename)) == -1)
                            break;
                        tempdb = db[fl];
                        n = s_nrecs(fl);
                        db[fl].file = fd;
                        strcpy(db[fl].fname, filename);
                        db[fl].extension[0] = '\0';
                        db[fl].path[0] = '\0';

                        s_modnrecs(fl, n);
                        sprintf(prompt[1], "LOAD FILE (CURRENT: %s)", db[fl].title);
                        sprintf(prompt[2], "SELECT RECORDS (%d RECORDS TOTAL)",s_nrecs(fl));
                        ptr=(ptr ? s_realloc(ptr,db[fl].fsize) : s_alloc(db[fl].fsize));
                        if (ptr == NULL)
                            break;
                        newPtr=(newPtr ? s_realloc(newPtr,db[fl].fsize) :s_alloc(db[fl].fsize));
                        if (newPtr == NULL)
                            break;
                        s_clearrec(fl, ptr);
                        s_clearrec(fl, newPtr);
                    } /*if seccheck*/
                } /*if sel_file*/
                break;

            case 2:
                lastchoice=choice;
                if (fd == -1 || s_nrecs(fl)==0)
                    break;
                sel_recs(fl, mask, (CHAR *)ptr);
                i=-1;
                found=0;
                while((i=s_findrec(fl,newPtr,ptr,mask,++i))!=-1)
                    s_recwrite(fl,newPtr,found++);
                s_modnrecs(fl,found);
                for (i=0;i< db[fl].fields;i++)
                    mask[i]+=3;
                sprintf(prompt[2], "SELECT RECORDS (%d RECORDS TOTAL)", s_nrecs(fl));
                break;

            case 3:
                lastchoice=choice;
                if (fd == -1 || s_nrecs(fl)==0)
                    break;
                fsize = FileSize(db[fl].file);
                if (fsize>65400L || fsize == -1L || fsize>coreleft()) {
                    message(M_SCR|M_KEY, "File is too big to fit into memory - hit any key");
                    break;
                }
                setwindow(29,16,54,19, "SORT FILE",help, "F10 when done",NULL);
                test=choice2=fld=direct=0;
                printxy(2,3, "NONE");
                do {
                    switch(choice2) {
                        case 0:
                            test = lineinput(2, 2, sortdir, direct, 20);
                            if (test == -1)
                                break;
                            direct = test;
                            choice2 = 1;
                            break;

                        case 1:

                            for (i=1; i<=db[fl].fields; i++) {
                                fprompt[i] = strdup(db[fl].f[i-1].header);
                                StrReplaceChar(fprompt[i], '|', ' ');
                            }
                            fprompt[0] = "NONE";
                            fprompt[i] = NULL;

                            test = lineinput(2, 3, fprompt, fld, 20);
                            if (test != -1) {
                                fld = test;
                                choice2 = 0;
                            }
                            for (i=1; i<=db[fl].fields; i++)
                                s_free(fprompt[i]);
                            break;

                        default:
                            if (test != -1)
                                choice2 = (choice2 ? 0:1);
                            break;
                    }
                } while (test != -1);

                if (fld)    {
                    sortfile(fl, ((direct) ? KEY_DOWN:KEY_UP),fld-1);
                }
                break;

            case 4:
                lastchoice = choice;
                if ((fd != -1) && (s_nrecs(fl) != 0))
                    printfile(NULL, fl, s_nrecs(fl));
                break;

            case KEY_F10:
                choice = 5;
                break;
        }
        window(1, 1, 80, 25);
    } while (choice != 5);

    /* restore mouse click points */
    points = lastP;

    if (ptr) {
        s_free(ptr);
        s_free(newPtr);
    }

    if (fd != -1) {
        s_close(fl);
        FileDelete(filename);
        db[fl] = tempdb;
        s_close(fl);
    }
    s_free(prompt[1]);
    s_free(prompt[2]);
}


/*****************************************************************************
sel_file - user input to select a db file.
*****************************************************************************/
INT sel_file(INT fl)
{
    INT direct=KEY_UP, choice;
    CHAR filein[50];
    BOOLEAN endflg;

    store();
    setwindow(29, 16, 54, 18, "LOAD FILE", "Use \x1b\x1a and Enter", "F10 to exit", NULL);
    endflg = NO;

    while (!endflg) {
        while (fl<0 || !db[fl].fields) {
            if (fl >= 0 && db[fl].fields == 0)
                fl = -1;
            fl += ((direct==KEY_UP) ? 1:-1);
            if (fl < 0) {
                for(fl=0; db[fl+1].fields; fl++)
                    ;
            }
        }
        sprintf(filein, "FILE: %s", db[fl].title);
        printxy(2, 2, "%-24s", filein);
        gotoxy(2+strlen(filein), 2);
        choice = nextkey();
        switch (choice) {

            case KEY_F10:
                fl = -1;
                endflg = YES;
                break;

            case KEY_RIGHT:
                fl++;
                direct = KEY_UP;
                break;

            case KEY_LEFT:
                fl--;
                direct = KEY_DOWN;
                break;

            default:
                choice &= 0xff;
                if (choice == KEY_ESC1) {
                    fl = -1;
                    endflg = YES;
                }

                if (choice == KEY_CR)
                    endflg = YES;
                break;

        } /*switch*/
    } /*while*/
    restore();
    return(fl);
}

/*****************************************************************************
sel_recs - creates prototype record and mask for searches (findrec)
    fl - type
    mask - array of integers of values: NONE,GT,GE,EQ,LE,LT,NE
    ptr - pointer to a record
*****************************************************************************/
INT sel_recs(INT fl, INT mask[], CHAR *ptr)
{
    CHAR *fprompt[50], sprompt[80];
    FIELDPARAMS attr;
    INT test, fld, choice=0;

    STATIC CHAR *maskprompt[]= {
        "NONE",
        " <  ",
        " <= ",
        " =  ",
        " >= ",
        " >  ",
        " <> ",
        NULL };

    fprompt[0] = strdup("NONE");
    for (fld=0; fld<db[fl].fields; fld++) {
        mask[fld] = NONE;
        fprompt[fld+1] = strdup(db[fl].f[fld].header);
        StrReplaceChar(fprompt[fld+1], '|', ' ');
    }
    fld = 0;

    choice = 0;
    store();
    setwindow(10, 16, 70, 18, "SELECT RECORDS", "Use \x1b\x1a and Enter", "F10 when done", NULL);
    Print("\n\r NONE %s: ", fprompt[1]);
    do {
        switch(choice) {

            case KEY_ESC1:
                return(-1);

            case KEY_UP:
                choice = ((fld<db[fl].fields-1) ? ++fld:0);
                break;

            case KEY_DOWN:
                choice = ((fld) ? --fld:db[fl].fields-1);
                break;

            default:
                if (choice >= db[fl].fields)
                    choice = 0;
                if (choice < 0)
                    choice = db[fl].fields - 1;
                fld = choice;
                attr = db[fl].f[fld];
                printxy(1, 2, " %s %-50s ", maskprompt[mask[fld]+3],fprompt[fld+1]);
                sprintf(sprompt, " %s: ", fprompt[fld+1]);
                choice = inpt(7, 2, sprompt, ((CHAR *)ptr+attr.offset), attr.width, attr.type, NO);
                if (choice != 1)
                    break;
                textattr(REVERSE);
                test = lineinput(2, 2, maskprompt, mask[fld]+3, 4);
                textattr(NORMAL);
                if (test == -1) {
                    choice = KEY_F10;
                    break;
                }
                mask[fld] = test-3;
                printxy(1, 2, " %s %s: ", maskprompt[mask[fld]+3],fprompt[fld+1]);
                choice = fld;
                break;
        }
    } while (choice != KEY_F10);

    for (fld=0; fld<=db[fl].fields; fld++)
        s_free(fprompt[fld]);

    restore();
    return(0);
}


BOOLEAN SelectPrinter(VOID)
{
    CHAR *list[] = {"LPT1", "LPT2", NULL};
    BOOLEAN ret = TRUE;

    /*
    * don't even ask if there is no LPT2
    */
    if (lpt2 != NULL) {

        switch (window_chooser(33, 11, list)) {

            case 0:
                defPrinter = lpt1;
                break;

            case 1:
                defPrinter = lpt2;
                break;

            /*
            * do nothing
            */
            default:
                ret = FALSE;
                break;
        }
    }
    return ret;
}
