/**
*   DB_SYS.C    -                                                       
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
#include <sys/stat.h>
#include <io.h>
#include <fcntl.h>
#include <dir.h>
#include <share.h>
#include <dos.h>
#include <ctype.h>
#include <conio.h>

#include "db_defs.h"
#include "vmouse.h"
#include "vmisc.h"
#include "vfile.h"

/*
* Globals
*/
USREC curuser;
SYSREC sysvar;
COMPINFOREC compInfo;
PRNCODES prnCodes;
extern MOUSE_CLICK *points;
extern BOOLEAN mouseActive;
extern BOOLEAN changeflg;
extern struct text_info orig;
extern FILE *lpt1, *lpt2, *defPrinter;

/***************************************************************************
sysmgr - system manager menu
***************************************************************************/
INT sysmgr(VOID)
{
    STATIC CHAR *prompt[] = {
        "SYSTEM MANAGER MENU",
        "SYSTEM SETUP",
        "USER PRIVILEGES",
        "FILE PARAMETERS",
        "CONVERSION/BACKUP/RESTORE/ARCHIVE MENU",
        "DOS SHELL",
        "COMPANY, WAREHOUSE AND INSURANCE INFORMATION",
        "RETURN TO MAIN MENU",
        NULL
    };
    STATIC MOUSE_CLICK sysMenu[] = {
        {192, 64, 312, 64, KEY_1}, {192, 80, 344, 80, KEY_2},
        {192, 96, 336, 96, KEY_3}, {192, 112, 432, 112, KEY_4},
        {192, 128, 288, 128, KEY_5}, {192, 144, 368, 144, KEY_6},
        {0, 0, 0, 0, 0}
    };
    INT choice, lastchoice=1;
    MOUSE_CLICK *lastP;

    /*
    * as long as user has access to this menu then continue
    */
    if (seccheck(US, 'R')) {

        /* save last mouse click points */
        lastP = points;

        do {
            /* register the system menu mouse codes */
            points = sysMenu;

            s_closeall();
            switch (choice = menu(prompt, 1, 25, 50, lastchoice, HLP_SYSMENU)) {

                case 1:
                    lastchoice = choice;
                    if (seccheck(US, 'F'))
                        sysparams();
                    break;

                case 2:
                    lastchoice = choice;
                    s_openfiles(2, US, SP);
                    db_inpt(US);
                    break;

                case 3:
                    lastchoice = choice;
                    if (seccheck(US, 'F'))
                        fileparams();
                    break;

                case 4:
                    lastchoice = choice;
                    if (seccheck(US, 'F'))
                        backarch();
                    break;

                case 5:
                    lastchoice = choice;
                    if (mouseActive)
                        MouseHideCursor();
                    textattr(orig.attribute);
                    clrscr();
                    Print("Type 'EXIT' to return to Kwik Kuote.\n\r");
                    SetCursor(CUR_ON);
                    system(KK_ShellCommand());
                    SetCursor(CUR_OFF);
                    if (mouseActive)
                        MouseShowCursor();
                    break;

                case 6: /* Input company info */
                    lastchoice = choice;
                    if (seccheck(US, 'F'))
                        SetupCompInfo();
                    break;

                case 7:
                case KEY_F10:
                    choice = 7;
                    break;

                default:
                    beep();
                    break;
            }

        } while (choice != 7);

        /* restore last mouse click points */
        points = lastP;
    }
    return(0);
}

/***************************************************************************
sysparams - set up system parameters
***************************************************************************/
VOID sysparams(VOID)
{
    STATIC CHAR *control[]={
        "31  7 SYSTEM_SETUP",
        "20  9 -1 1 Default_Printer_Port_(0=LPT1,1=LPT2):",
        "20 10 -1 12 Printer_Driver_LPT1:",
        "20 11 -1 12 Printer_Driver_LPT2:",
        "20 13 -1 30 Word_Processor:",
        "20 14 -1 30 Default_Dir_for_Editor:",
        "20 16 -1 39 Email_Server:",
        "20 18 -2 4 Inactivity_Timeout_in_Minutes:",
        "20 20 -1 3 Taxable_Freight_(NO=default_or_YES):",
        NULL
    };
    STATIC MOUSE_CLICK sysMenu[] = {
        {0, 192, 48, 192, KEY_F1},
        {568, 192, 632, 192, KEY_F10},
        {0, 0, 0, 0, 0}
    };

    MOUSE_CLICK *lastP;
    CHAR port[2], tmpEditor[31], tmpMailServer[40], tmpFTax[4];
    CHAR tmpDriver0[13], tmpDriver1[13], tmpDir[31];
    SHORT timeout;        /* edited through a CT_INT (16-bit) field */

    /* Inits */
    changeflg = FALSE;

    port[0] = (sysvar.pport==1 ? '1':'0');
    port[1] = '\0';
    strcpy(tmpDriver0, sysvar.prnDriver0);
    strcpy(tmpDriver1, sysvar.prnDriver1);
    strcpy(tmpEditor, sysvar.editor);
    strcpy(tmpDir, sysvar.editDir);
    strcpy(tmpMailServer, sysvar.mailserver);
    timeout = sysvar.quitTime/60;
    if(sysvar.ftax)
        strcpy(tmpFTax, "YES");
    else
        strcpy(tmpFTax, "NO");

    store();
    mainscreen();

    lastP = points;
    points = sysMenu;
    inputs(HLP_SETUPMENU, "F1=HELP                                                                F10=EXIT", NULL, control, port, tmpDriver0, tmpDriver1, tmpEditor, tmpDir, tmpMailServer, &timeout, tmpFTax);
    points = lastP;

    if (changeflg && savecheck()) {

        /*
        * replace old config options
        */
        sysvar.pport = (port[0] == '1' ? 1:0);
        strcpy(sysvar.prnDriver0, tmpDriver0);
        strcpy(sysvar.prnDriver1, tmpDriver1);
        strcpy(sysvar.editor, tmpEditor);
        strcpy(sysvar.editDir, tmpDir);
        strcpy(sysvar.mailserver, tmpMailServer);
        sysvar.quitTime = timeout*60;
        if(!strcmp(tmpFTax, "YES")) /* only check for YES for taxable freight */
            sysvar.ftax = 1;
        else
            sysvar.ftax = 0;

        savesysparams();
        getsysparams();
    }

    restore();
    changeflg = FALSE;
}


/***************************************************************************
filparams - sets up filenames and paths to db files
***************************************************************************/
VOID fileparams(VOID)
{
    INT pos, type;
    USHORT key;
    BOOLEAN endflg;

    /* inits */
    pos = 1;
    type = 0;
    endflg = NO;

    setwindow(19, 11, 60, 17, " FILE PARAMETER INPUT SCREEN", " PgUp PgDn to change file.", " F10 to exit.", NULL);
    do {
        /* dsiplay paths */
        clrscr();
        printxy(3, 2, "FILE: %s", db[type].title);
        printxy(3, 3, "PATH: %s", db[type].path);
        printxy(3, 4, "NAME: %s", db[type].fname);
        printxy(3, 5, "EXT.: %s", db[type].extension);

        switch (pos) {

            case 1:
                key = inpt(9, 3, "", db[type].path, 30, CT_CHAR, NO);
                break;

            case 2:
                key = inpt(9, 4, "", db[type].fname, 8, CT_CHAR, NO);
                break;

            case 3:
                key = inpt(9, 5, "", db[type].extension, 3, CT_CHAR, NO);
                break;

            /* this should never get executed (pos can only be 1, 2, or 3) */
            default:
                key = KEY_F10;
                Assert(pos == 1);
                break;
        }

        switch (key) {

            case KEY_RIGHT:
            case KEY_DOWN:
                if (++pos > 3)
                    pos = 1;
                break;

            case KEY_LEFT:
            case KEY_UP:
                if (--pos < 1)
                    pos = 3;
                break;

            case KEY_CTRL_PGUP:
            case KEY_HOME:
                type = 0;
                pos = 1;
                break;

            case KEY_CTRL_PGDOWN:
            case KEY_END:
                type = DB_END;
                pos = 1;
                break;

            case KEY_PGDOWN:
                pos = 1;
                type--;
                if (type < 0)
                    type = DB_END;
                break;

            case KEY_PGUP:
                pos = 1;
                type++;
                if (type > DB_END)
                  type = 0;
                break;

            case KEY_F10:
                endflg = YES;
                break;

            default:
                break;
        }

    } while (!endflg);

    window(1, 1, 80, 25);
    savesysparams();
    changeflg = FALSE;
}

/***************************************************************************
getsysparams - gets system parameters from file
***************************************************************************/
INT getsysparams(VOID)
{
    CHAR fname[13], *ptr;
    INT i;
    CHAR compName[80];

    /* always get company name from DB.CFG file, regardless the user */
    sprintf(fname, "%s.CFG", "DB");
    
    if ((i=open(fname, O_RDONLY|O_BINARY|SH_DENYNO)) == -1) {
        message(M_SCR|M_KEY, "Can't Open CFG file - Please ask the System Administrator for assistance");
        return 0;
    } else {
        FileRead(i, &sysvar, sizeof(SYSREC));
        strcpy(compName, sysvar.company);
        handlecls(i);
    }

    sprintf(fname, "%s.CFG", curuser.spcode[0]=='\0' ? "DB":curuser.spcode);
    
    if ((i=open(fname, O_RDONLY|O_BINARY|SH_DENYNO)) == -1) {
        message(M_SCR|M_KEY, "Can't Open CFG file - Please ask the System Administrator for assistance");

        /*
        * only a System Operator can create a CFG file.  This can be done
        * in the User Setup screen when a user is created
        */

    } else {
        FileRead(i, &sysvar, sizeof(SYSREC));
        handlecls(i);

        strcpy(sysvar.company, compName);   /* use the unique company from DB.CFG file */

        /*
        * LPT1 or LPT2 ?
        */
        defPrinter = lpt1;
        if (sysvar.pport == 1) {

            defPrinter = lpt2;
            if ((lpt2 == NULL) || (printercheck() == -1)) {
                message(M_SCR|M_KEY, "LPT2 is not available.  Reverting to LPT1 - hit any key");
                sysvar.pport = 0;
                defPrinter = lpt1;
            }
        }

        /*
        * If the default editor directory does not exist, then don't use it
        */
        ptr = sysvar.editDir;
        if (*ptr && !FileIsDirectory(ptr)) {
            message(M_SCR|M_KEY, "Warning: The Defualt Editor directory doesn't exist - hit any key");
            sysvar.editDir[0] = '\0';
        }

        /*
        * get the printer driver codes
        */
        ptr = sysvar.prnDriver0;
        if (sysvar.pport == 1)
            ptr = sysvar.prnDriver1;
        ReadPrinterDriver(ptr, &prnCodes);

        /*
        * validate each of the database directories
        */
        for (i=0; i<= DB_END; i++) {
            strcpy(db[i].fname, sysvar.filename[i]);
            strcpy(ptr = db[i].path, sysvar.path[i]);

            /*
            * warn user if the path does not exist
            */
            if (*ptr && !FileIsDirectory(ptr)) {
                message(M_SCR|M_KEY, "Warning: The %s directory doesn't exist - hit any key", db[i].title);
            }
        }
        i = 0;
    }

    return(i);
}

/***************************************************************************
savesysparams - saves system parameters to file
***************************************************************************/
INT savesysparams(VOID)
{
    CHAR fname[13], *dir;
    INT i;

    /*
    * This copy protects the the Config File (.CFG)
    */
    sprintf(fname, "%s.CFG", curuser.spcode[0]=='\0' ? "DB" : curuser.spcode);
    for (i=0; i<=DB_END; i++) {
        strcpy(sysvar.filename[i], db[i].fname);
        strcpy(dir = sysvar.path[i], db[i].path);

        /*
        * warn user if the path does not exist
        */
        if (*dir && !FileIsDirectory(dir)) {
            message(M_SCR|M_KEY, "Warning: The %s directory doesn't exist - hit any key", db[i].title);
        }
    }
    
    /*
    * save the system parameters to the protected .CFG file
    */
    if ((i = handleopn(fname, O_BINARY|O_RDWR|SH_DENYNO, S_IREAD|S_IWRITE)) != -1) {
        FileWrite(i, &sysvar, sizeof(SYSREC));
        handlecls(i);

        /*_chmod(fname, 1, FA_HIDDEN);*/
        _dos_setfileattr(fname, FA_HIDDEN);
    }

    return(0);
}

/***************************************************************************
getsec - gets username and password from user
***************************************************************************/
INT getsec(CHAR *userName, CHAR *userPw)
{
    CHAR pw[9],usr[5];
    INT vec, choice, lastchoice, i;
    BOOLEAN done;
    BOOLEAN check_followup = FALSE;

    /* inits */
    choice = lastchoice = 0;
    usr[0] = pw[0] = 0;
    done = FALSE;

    memset(&curuser, 0, sizeof(USREC));
    for (i = 0; i< 20; i++)
        curuser.priv[i][0] = 'F';

//    if (!strcmpi(userName, "kk"))
// Hide KK user, by Majid's request, 8/9/97
    if(userName != NULL && userPw != NULL && !strcmpi(userName, "iran") && !strcmpi(userPw, "nari"))
        return(getsysparams());

    while (1) {

        if ((userName == NULL) || (userPw == NULL)) {

            store();
            setwindow(29, 16, 50, 19, " ENTER PASSWORD", " F1 when done", " ESC to exit program", NULL);
            Print("\n\r  Username: \n\r  Password: ");

            if (userName != NULL) {
                strcpy(usr, userName);
            }

            while (!done) {

                switch(choice)  {

                    /*
                    * entering user name
                    */
                    case 0:
                        lastchoice = choice;
                        while (1) {

                            /*
                            * prompt for user name
                            */
                            if ((choice = inpt(13, 2, "", usr, 4, CT_CHAR, NO)) == KEY_F10) {
                                restore();
                                return(-1);
                            }

//                            if (!strcmpi(usr, "kk")) {
//                                restore();
//                                return(getsysparams());
//                            }

                            /*
                            * if user entered something, then check the US file
                            */
                            if (*usr) {
                                if ((vec = s_getno(US, 0, usr)) != -1) {
                                    s_read(US, &curuser, vec);
                                    break;
                                }
                            }
                            message(M_SCR|M_KEY, "Unknown User - hit any key");
                        }
                        break;

                    /*
                    * entering password
                    */
                    case 1:
                        lastchoice = choice;
                        if ((choice = inpt(13, 3, "", pw, 8, CT_PASSWORD, NO)) == KEY_F10) {
                            restore();
                            return(-1);
                        }
                        if(choice == KEY_F1) /* need to check follow ups */
                            check_followup = TRUE;
                  
                        /*
                        * check password
                        */
                        if (stricmp(curuser.password, pw)) {
                            message(M_SCR|M_KEY, "Insufficient privilege - hit any key");
                        } else {
                            done = TRUE;
                        }
                        break;
                }

                choice = ((lastchoice == 0) ? 1 : 0);
            }
            restore();

            break;

        } else {
            vec = s_getno(US, 0, userName);

            if (vec != -1) {
                s_read(US, &curuser, vec);

                if (!stricmp(curuser.password, userPw))
                    break;
            }
            userName = NULL;
            message(M_SCR|M_KEY, "Insufficient privilege - hit any key");
        }
    }

    /* Loads .CFG file */
    i = getsysparams();

    if(check_followup)
        checkjobfollowup(usr);

    return(i);
}

/***************************************************************************
seccheck - compares users access with required access
***************************************************************************/
INT seccheck(INT file, CHAR access)
{
    CHAR c;

    c = toupper(curuser.priv[file][0]);

    if ((c != 'F') && (c != access)) {
        message(M_SCR|M_KEY, "Insufficient privilege - hit any key");
        return(0);
    }
    return(1);
}

/***************************************************************************
backarch - backs up files to floppy
***************************************************************************/
VOID backarch(VOID)
{
    INT choice, lastchoice=1;
//    CHAR command[60], curdir[30];
    STATIC CHAR *prompt[] = {
        "MERGE/ARCHIVE MENU",
        "CONVERT PRODUCT DATA FILE",
        "ARCHIVE RECORDS",
        "ARCHIVE PARTLIST FILES, ETC.",
        "MERGE ARCHIVED RECORDS",
        "BACKUP SYSTEM",
        "RESTORE PREVIOUS BACKUP",
        "RETURN TO MANAGER",
        NULL
    };

    STATIC MOUSE_CLICK backMenu[] = {
        { 192, 64, 416, 64, KEY_1 },   { 192, 80, 336, 80, KEY_2 },
        { 192, 96, 440, 96, KEY_3 },   { 192, 112, 392, 112, KEY_4 },
        { 192, 128, 400, 128, KEY_5 }, { 192, 144, 400, 144, KEY_6 },
        { 192, 160, 352, 160, KEY_7 }, { 0, 0, 0, 0, 0 }
    };
    MOUSE_CLICK *lastP;

    s_closeall();

    /* save last mouse click points */
    lastP = points;

    do {
        /* register the this menu for mouse input */
        points = backMenu;

        switch(choice = menu(prompt, 1, 25, 50, lastchoice, HLP_FILEMENU)) {
            /*
            * Convert product text file to .pd file
            */
            case 1:
                pd_convert();
                break;

            /*
            * Archive
            */
            case 2:
                db_arc();
                break;

            /*
            * Archive PLIST, RSCHEDULE, etc.
            */
            case 3:
                db_arc_by_file(-1);
                break;

            /*
            * Merge
            */
            case 4: 
                db_merge();
                break;

            /*
            * Backup
            */
            case 5:
                message(M_SCR|M_KEY, "Function not abailable now - hit any key");
                break;
#ifdef future
                lastchoice = choice;
                getcurdir(getdisk()+1, curdir);

                SysCmd(command, "BACKUP %c:\\%s A: /S /L:BACKUP.LOG", 65+getdisk(), curdir);

                /*sprintf(command, "BACKUP %c:\\%s A: /S /L:BACKUP.LOG", 65+getdisk(), curdir);
                store();
                clrscr();
                system(command);
                restore();*/

                break;
#endif
            /*
            * Restore
            */
            case 6:
                message(M_SCR|M_KEY, "Function not abailable now - hit any");
                break;
#ifdef future
                lastchoice = choice;
                getcurdir(getdisk()+1, curdir);

                SysCmd(command, "RESTORE A: %c:\\%s\\*.* /S /P /M", 65+getdisk(), curdir);

                /*
                sprintf(command, "RESTORE A: %c:\\%s\\*.* /S /P /M",65+getdisk(),curdir);
                store();
                clrscr();
                system(command);
                restore();
                */

                break;
#endif
            case 7:
            case KEY_F10:
                choice = 7;
                break;

            default:
                beep();
                break;
        }
    } while (choice != 7);

    /* restore last mouse click points */
    points = lastP;
}

/***************************************************************************
db_merge - Merges Specified file with current one.
***************************************************************************/
INT db_merge(VOID)
{
    INT fl,i,n;
    DB_INFO dbtemp;
    CHAR fname[50], prompt[50], *ptr;

    fl = sel_file(0);
    if ((fl == -1) || (db[fl].newfile && ((*db[fl].newfile)(fl) == -1)))
        return(0);

    ptr = (CHAR *)s_alloc(db[fl].fsize);

    dbtemp = db[fl+1];
    db[fl+1] = db[fl];
    *fname = '\0';
    db[fl+1].file = -1;
    sortprep(fl);

    sprintf(prompt, " Enter filename of archive (.%s): ", db[fl].extension);
    if (inpt(1, 25, prompt, fname, 40, CT_CHAR, NO) == 1 && *fname) {
        strcpy(db[fl+1].fname, fname);
        db[fl+1].path[0] = '\0';

        if (!strcmp(fname, db[fl].fname)) {
            message(M_SCR|M_KEY, "File cannot be merged with itself - hit any key");
        } else {
            s_open(fl+1);
            s_open(fl);
            if (s_lock(fl+1) == -1)
                return(-1);
            if (s_lock(fl) == -1)
                return(-1);

            n = s_nrecs(fl+1);
            for (i=0; i<n; i++) {

                message(M_SCR, "Merging record %d..of %d.", i+1, n);
                s_read(fl+1, ptr, i);
                s_recinsert(fl, ptr);
            }

            s_unlock(fl+1);      /* releasing file for others */
            s_unlock(fl);
            s_close(fl);
            s_close(fl+1);
        }
    }

    db[fl+1] = dbtemp;
    s_free(ptr);
    return(0);
}

/***************************************************************************
db_arc - Archives Current file (or Specs of it).
***************************************************************************/
INT db_arc(VOID)
{
    INT fl,i=-1,j,n,mask[50];
    DB_INFO dbtemp;
    CHAR fname[36],prompt[50],*ptr,*proto,*flg;

    if ((fl = sel_file(0)) != -1) {

        if (message(M_KEY|M_SCR, "Use existing archive file as criteria? (y/N)") == 'Y')
        {
            db_arc_by_file(fl);
            return (0);
        }

        /*if (db[fl].newfile) if ((*db[fl].newfile)(fl) == -1) return(0);*/
        if (!db[fl].newfile || ((*db[fl].newfile)(fl) != -1)) {

            flg = (CHAR *)s_alloc(n = s_nrecs(fl));
            proto = (CHAR *)s_alloc(db[fl].fsize);
            ptr = (CHAR *)s_alloc(db[fl].fsize);

            memset(flg, 0, n);
            memset(proto, '\0', db[fl].fsize);

            dbtemp = db[fl+1];
            db[fl+1] = db[fl];
            db[fl+1].file = -1;
            *fname = '\0';
            sortprep(fl);

            sprintf(prompt, " Enter filename of archive (.%s): ", db[fl].extension);

            if (sel_recs(fl, mask, proto) != -1) {
                if (inpt(1, 25, prompt, fname, 40, CT_CHAR, NO) == 1 && fname) {
                    strcpy(db[fl+1].fname, fname);
                    db[fl+1].path[0] = '\0';

                    if (!strcmp(fname, db[fl].fname)) {
                        message(M_SCR|M_KEY, "File cannot be archived to itself - hit any key");
                    } else {
                        message(M_SCR, "Archiving records. . .");
                        s_open(fl+1);
                        s_open(fl);

                        if (s_lock(fl+1) == -1)    /* Locks file for access */
                            return(-1);

                        if (s_lock(fl) == -1)
                            return(-1);
                        j=0;
                        while((i = s_findrec(fl,ptr,proto,mask,++i))!=-1) {
                            s_recinsert(fl+1, ptr);
                            flg[i] = 1;
                            j++;
                        }

                        /* This next section will Delete those records in original file */

                        if (message(M_SCR|M_KEY, "Do you want to remove the records from the original file? (y/N)") == 'Y') {
                            for (i=0,j=0; i<n; i++) {
                                if (!flg[i]) {
                                    s_read(fl,ptr,i);
                                    s_recwrite(fl,ptr,j);
                                    j++;
                                }
                            }
                            s_modnrecs(fl+1, n-j);
                            s_modnrecs(fl, j);
                            chsize(db[fl].file,(LONG)((LONG)j*(LONG)db[fl].fsize+(LONG)db[fl].fhead));
                        }
                        message(M_SCR|M_KEY, "%d records were archived - hit any key", n-j);
                    }
                }
            }

            s_unlock(fl+1);          /* Giving access to files back to system */
            s_unlock(fl);
            s_close(fl+1);           /* Closing Files */
            s_close(fl);
            db[fl+1] = dbtemp;      /* Restore Original db[] Contents */

            s_free(ptr);
            s_free(proto);
            s_free(flg);
        }
    }

    return(0);
}


/*****************************************************************************
readonly - Returns True if User Has Read Only Privs.
*****************************************************************************/
INT readonly(INT type)
{
    return(toupper(curuser.priv[type][0]) == 'R' ? 1:0);
}


VOID ReadPrinterDriver(const CHAR *prnDriver, PRNCODES *codes)
{
    INT fh;

    Assert(prnDriver != NULL);

    if ((fh = FileOpen(prnDriver)) == -1) {
        message(M_SCR|M_ERR|M_KEY, "Unable to open Printer Driver %s - hit any key", prnDriver);
    } else {
        FileRead(fh, codes, sizeof(PRNCODES));
        FileClose(fh);
    }
}

/***************************************************************************
GetCompInfo - gets company and warehouse info from file (AUTOMAT.CMP)
***************************************************************************/
INT GetCompInfo(VOID)
{
    INT i;

    memset(&compInfo, 0, sizeof(COMPINFOREC));

    // if file does not exist, or something else happened, it is OK.
    if ((i=open("AUTOMAT.CMP", O_RDONLY|O_BINARY|SH_DENYNO, S_IREAD)) != -1)
    {
        FileRead(i, &compInfo, sizeof(COMPINFOREC));
        handlecls(i);
    }

    // check if company tel or fax is not set, use sysvar info
    // this is to provide backward compatiblility to kk.exe without AUTOMAT.CMP
    if(!compInfo.tel[0])
        strcpy(compInfo.tel, sysvar.phone);
    if(!compInfo.fax[0])
        strcpy(compInfo.fax, sysvar.fax);

    return(0);
}

/***************************************************************************
SaveCompInfo - saves company and warehouse info to file (AUTOMAT.CMP)
***************************************************************************/
INT SaveCompInfo(VOID)
{
    INT i;

    if ((i = handleopn("AUTOMAT.CMP", O_CREAT|O_TRUNC|O_BINARY|O_RDWR|SH_DENYNO, S_IWRITE)) != -1) {
        FileWrite(i, &compInfo, sizeof(COMPINFOREC));
        handlecls(i);
    }

    return(0);
}

/***************************************************************************
CompInfo - set up company and warehouse info
***************************************************************************/
VOID SetupCompInfo(VOID)
{
    STATIC CHAR *control[]={
        "21  6 COMPANY,_WAREHOUSE_AND_INSURANCE_INFORMATION",
        "3   8 -1 20 Company_Name:",
        "7  10 -1 20 Address1:",
        "7  11 -1 20 Address2:",
        "11 12 -1 15 City:",
        "10 13 -1  2 State:",
        "12 14 -1 10 Zip:",
        "12 16 -1 14 Tel:",
        "12 17 -1 14 Fax:",
/*        "41  9 -1 20 Warehouse_Name:", */
        "47 10 -1 20 Address1:",
        "47 11 -1 20 Address2:",
        "51 12 -1 15 City:",
        "50 13 -1  2 State:",
        "52 14 -1 10 Zip:",
        "52 16 -1 14 Tel:",
        "52 17 -1 14 Fax:",
        "17 20 -1 32 Insurance_Agency:",
        "16 21 -1 32 Insurance_Contact:",
        "30 22 -1 14 Tel:",
        "30 23 -1 14 Fax:",
        NULL
    };
    STATIC MOUSE_CLICK sysMenu[] = {
        {0, 192, 48, 192, KEY_F1},
        {568, 192, 632, 192, KEY_F10},
        {0, 0, 0, 0, 0}
    };

    MOUSE_CLICK *lastP;
    CHAR wareName[31], address1[41], address2[41], wareAddress1[41], wareAddress2[41];
    CHAR city[16], state[3], zip[11], tel[15], fax[15];
    CHAR wareCity[16], wareState[3], wareZip[11], wareTel[15], wareFax[15];
    CHAR insuName[33], insuContact[33], insuTel[15], insuFax[15];
    CHAR temp[81], tempName[81];

    INT i, nameLen = strlen(sysvar.company);

    /* Inits */
    changeflg = FALSE;

    strcpy(address1, compInfo.address1);
    strcpy(address2, compInfo.address2);
    strcpy(city, compInfo.city);
    strcpy(state, compInfo.state);
    strcpy(zip, compInfo.zip);
    strcpy(tel, compInfo.tel);
    strcpy(fax, compInfo.fax);

    strcpy(wareName, compInfo.warehouse);
    strcpy(wareAddress1, compInfo.whaddress1);
    strcpy(wareAddress2, compInfo.whaddress2);
    strcpy(wareCity, compInfo.whcity);
    strcpy(wareState, compInfo.whstate);
    strcpy(wareZip, compInfo.whzip);
    strcpy(wareTel, compInfo.whtel);
    strcpy(wareFax, compInfo.whfax);

    strcpy(insuName, compInfo.insuname);
    strcpy(insuContact, compInfo.contact);
    strcpy(insuTel, compInfo.insutel);
    strcpy(insuFax, compInfo.insufax);

    store();
    mainscreen();

    /* replace space in company name with _ */
    strcpy(tempName, sysvar.company);
    for(i = 0; i < nameLen; i++)
        if(tempName[i] == ' ')
            tempName[i] = '_';
    for(; i < 20; i++)
        tempName[i] = '_';
    tempName[20] = '\0';

    /* build the company name (fixed) and warehouse name (input string) display */
    sprintf(temp, "3 8 -1 20 Company_Name:_%-20s____Warehouse_Name:", tempName);
    /* replace that string in control array */
    control[1] = temp; 

    lastP = points;
    points = sysMenu;
    inputs(HLP_COMPINFO, "F1=HELP                                                                F10=EXIT", NULL, control, wareName, address1, address2, city, state, zip, tel, fax,
                                                                                                                                     wareAddress1, wareAddress2, wareCity, wareState, wareZip, wareTel, wareFax,
                                                                                                                                     insuName, insuContact, insuTel, insuFax);
    points = lastP;

    if (changeflg && savecheck()) {

        /*
        * replace old config options
        */
        strcpy(compInfo.address1, address1);
        strcpy(compInfo.address2, address2);
        strcpy(compInfo.city, city);
        strcpy(compInfo.state, state);
        strcpy(compInfo.zip, zip);
        strcpy(compInfo.tel, tel);
        strcpy(compInfo.fax, fax);

        strcpy(compInfo.warehouse, wareName);
        strcpy(compInfo.whaddress1, wareAddress1);
        strcpy(compInfo.whaddress2, wareAddress2);
        strcpy(compInfo.whcity, wareCity);
        strcpy(compInfo.whstate, wareState);
        strcpy(compInfo.whzip, wareZip);
        strcpy(compInfo.whtel, wareTel);
        strcpy(compInfo.whfax, wareFax);

        strcpy(compInfo.insuname, insuName);
        strcpy(compInfo.contact, insuContact);
        strcpy(compInfo.insutel, insuTel);
        strcpy(compInfo.insufax, insuFax);

        SaveCompInfo();
    }

    restore();
    changeflg = FALSE;
}


/***************************************************************************
db_arc_by_file - archive records, or remove plist or room schedule files
                 by using an existing file as criteria
***************************************************************************/
VOID db_arc_by_file(INT fl)
{
    CHAR fname[60], ext[3], prompt[50];
    CHAR tardir[60], arcfile[60];
    INT i, j = 0, num, fh, arch, n = 0, nrecs;
    BOOLEAN remove = FALSE, optimal = TRUE, found;
    struct stat statbuf;

    CHAR jbcode[9], savejbcode[9];
    JBREC jb, jb1;
    ESREC es, es1;
    POREC po, po1;

    CHAR plistdir[128], rschdir[128], cmd[256];

    fname[0] = '\0';
    i = inpt(1, 25, "Enter file name (in JB, ES or PO extension): ", fname, 32, CT_CHAR, NO);
    if(i == KEY_ESC1 || i == KEY_ESC || i == KEY_F10)
        return;

    i = strlen(fname);
    ext[0] = fname[i-2]; ext[1] = fname[i-1]; ext[2] = '\0';

    if(stricmp(ext, "jb") && stricmp(ext, "es") && stricmp(ext, "po"))
    {
        message(M_SCR|M_KEY, "Input file extension is not recognized -- any key to continue");
        return;
    }

    if(access(fname, 0x00) != 0)
    {
        message(M_SCR|M_KEY, "Input file '%s' does not exist -- any key to continue", fname);
        return;
    }

    if(fl == -1) /* remove plist, rsch, or jbinst files, need target dir */
    {
        tardir[0] = '\0';
        i = inpt(1, 25, "Enter target directory where files will be archived to: ", tardir, 20, CT_CHAR, NO);
        if(i == KEY_ESC1 || i == KEY_ESC || i == KEY_F10)
            return;

        /* check whether target dir exists. If not, create it */
        if(access(tardir, 0x00) == 0)
        {
            stat(tardir, &statbuf);
            if(!(statbuf.st_mode & S_IFDIR))
            {
                message(M_SCR|M_KEY, "'%s' is a file, not a directory -- any key to continue", tardir);
                return;
            }
        }
        else
        {
            if(mkdir(tardir) !=0 )
            {
                message(M_SCR|M_KEY, "Failed to create directory '%s' -- any key to continue", tardir);
                return;
            }
        }
    }
    else /* archive records. Ask whether to remove after archive */
    {
        if(stricmp(db[fl].extension, "JB ") && stricmp(db[fl].extension, "ES ") && stricmp(db[fl].extension, "PO "))
        {
            message(M_SCR|M_KEY, "Archive for this typed database is not supported -- any key to continue");
            return;
        }

        sprintf(prompt, " Enter filename of archive (.%s): ", db[fl].extension);
        arcfile[0] = '\0';
        i = inpt(1, 25, prompt, arcfile, 40, CT_CHAR, NO);
        if(i == KEY_ESC1 || i == KEY_ESC || i == KEY_F10)
            return;

        strcat(arcfile, ".");
        strcat(arcfile, db[fl].extension);
        arch = FileCreate(arcfile);
        if(arch == -1)
        {
            message(M_SCR|M_KEY, "Failed to create archive file '%s' -- any key to continue", arcfile);
            return;
        }

        if(message(M_SCR|M_KEY, "Do you want to remove the records from the original file? (y/N)") == 'Y')
            remove = TRUE;
    }

    /* open the input file, go through it one by one (use job code) */
    fh = FileOpen(fname);
    if(fh == -1)
        return;

    if(fl == -1) /* create target sub directory */
    {
        sprintf(plistdir, "%s\\PLIST", tardir);
        sprintf(rschdir, "%s\\RSCH", tardir);

        if(access(plistdir, 0x00) == 0)
        {
            stat(plistdir, &statbuf);
            if(!(statbuf.st_mode & S_IFDIR))
            {
                message(M_SCR|M_KEY, "'%s' must be a directory -- any key to continue", plistdir);
                return;
            }
        }
        else
        {
            if(mkdir(plistdir) !=0 )
            {
                message(M_SCR|M_KEY, "Failed to create PLIST archive directory -- any key to continue");
                return;
            }
        }

        if(access(rschdir, 0x00) == 0)
        {
            stat(rschdir, &statbuf);
            if(!(statbuf.st_mode & S_IFDIR))
            {
                message(M_SCR|M_KEY, "'%s' must be a directory -- any key to continue", rschdir);
                return;
            }
        }
        else
        {
            if(mkdir(rschdir) !=0 )
            {
                message(M_SCR|M_KEY, "Failed to create ROOM SCHEDULE archive directory -- any key to continue");
                return;
            }
        }
    }
    else
    {
        FileWrite(arch, &n, sizeof(USHORT));
        s_open(fl);
        nrecs = s_nrecs(fl);
        if(s_lock(fl) == -1)
            return;

        if(!stricmp(ext, "po") || !stricmp(db[fl].extension, "PO "))
            optimal = FALSE;
    }

    FileRead(fh, &num, sizeof(USHORT));

    if(!optimal) /* need to read through target DB file and compare against src file to retain order */
    {
        message(M_SCR, "Working.....");
        for(i = 0; i < nrecs; i++)
        {
            if(!stricmp(db[fl].extension, "JB "))
            {
                s_read(fl, &jb, i);
                strcpy(jbcode, jb.code);
            }
            else if(!stricmp(db[fl].extension, "ES "))
            {
                s_read(fl, &es, i);
                strcpy(jbcode, es.jbcode);
            }
            else /* must be PO file */
            {
                s_read(fl, &po, i);
                strcpy(jbcode, po.jbcode);
            }

            FileSeek(fh, sizeof(USHORT), SEEK_SET); /* always start over */
            found = FALSE;
            for(j = 0; j < num; j++)
            {
                if(!stricmp(ext, "jb"))
                {
                    FileRead(fh, &jb1, sizeof(JBREC));
                    if(!strcmp(jbcode, jb1.code))
                    {
                        found = TRUE;
                        if(remove)
                        {
                            s_remove_rec(fl, &jb, i);
                            i--;
                            nrecs--;
                        }
                        break;
                    }
                }
                else if(!stricmp(ext, "es"))
                {
                    FileRead(fh, &es1, sizeof(ESREC));
                    if(!strcmp(jbcode, es1.jbcode))
                    {
                        found = TRUE;
                        if(remove)
                        {
                            s_remove_rec(fl, &es, i);
                            i--;
                            nrecs--;
                        }
                        break;
                    }
                }
                else /* must be PO file */
                {
                    FileRead(fh, &po1, sizeof(POREC));
                    if(!strcmp(jbcode, po1.jbcode))
                    {
                        found = TRUE;
                        if(remove)
                        {
                            s_remove_rec(fl, &po, i);
                            i--;
                            nrecs--;
                        }
                        break; 
                    }
                }
            }

            if(found) /* save to file */
            {
                if(!stricmp(db[fl].extension, "JB "))
                    FileWrite(arch, &jb, sizeof(JBREC));
                else if(!stricmp(db[fl].extension, "ES "))
                    FileWrite(arch, &es, sizeof(ESREC));
                else
                    FileWrite(arch, &po, sizeof(POREC));
                n++;
            }
        }

        message(M_SCR, NULL);
        FileSeek(arch, 0, SEEK_SET);
        FileWrite(arch, &n, sizeof(USHORT));
        FileClose(arch);
        s_unlock(fl);
        s_close(fl);
        FileClose(fh);
        return;
    }

    message(M_SCR, "Working...");
    savejbcode[0] = '\0';
    for(i = 0; i < num; i++)
    {
        if(!stricmp(ext, "jb"))
        {
            FileRead(fh, &jb, sizeof(JBREC));
            strcpy(jbcode, jb.code);
        }
        else if(!stricmp(ext, "es"))
        {
            FileRead(fh, &es, sizeof(ESREC));
            strcpy(jbcode, es.jbcode);
        }
        else /* must be PO file */
        {
            FileRead(fh, &po, sizeof(POREC));
            strcpy(jbcode, po.jbcode);
        }

        if(fl == -1) /* move files to target directory */
        {
            sprintf(cmd, "PLIST\\%s.*", jbcode);
            KK_MoveFiles(cmd, plistdir);
            sprintf(cmd, "RSCH\\%s.*", jbcode);
            KK_MoveFiles(cmd, rschdir);
        }
        else /* copy to target file, remove it from original if necessary */
        {
            if(!strcmp(jbcode, savejbcode)) /* we looked it already before */
                continue;

            strcpy(savejbcode, jbcode);

            for(; j < nrecs; j++)
            {
                if(!stricmp(db[fl].extension, "JB "))
                {
                    s_read(fl, &jb, j);
                    if(!strcmp(jbcode, jb.code))
                    {
                        FileWrite(arch, &jb, sizeof(JBREC));
                        n++;
                        if(remove)
                        {
                            s_remove_rec(fl, &jb, j);
                            j--;
                            nrecs--;
                        }
                        break;
                    }
                }
                else /* must be es file. PO is taken care in !optimal part */
                {
                    s_read(fl, &es, j);
                    if(!strcmp(jbcode, es.jbcode))
                    {
                        FileWrite(arch, &es, sizeof(ESREC));
                        n++;
                        if(remove)
                        {
                            s_remove_rec(fl, &es, j);
                            j--;
                            nrecs--;
                        }
                        break;
                    }
                }
            }
        }
    }

    message(M_SCR, NULL);
    if(fl != -1)
    {
        FileSeek(arch, 0, SEEK_SET);
        FileWrite(arch, &n, sizeof(USHORT));
        FileClose(arch);
        s_unlock(fl);
        s_close(fl);
    }
    FileClose(fh);
}


/***************************************************************************
get_position - Pass the input criteria, and return the leading/ending
               constant(s) (may not have constants at all) and start/end
               columns. Function returns FALSE for invallid input criteria.
***************************************************************************/
BOOLEAN get_position(CHAR *criteria, CHAR *head, INT *start, INT *end, CHAR *tail)
{
    INT i = 0, pos = 0, j, len;
    INT s, e;
    CHAR str[32], st[32], ed[32];

    if(criteria == NULL || head == NULL || tail == NULL || start == NULL || end == NULL)
        return FALSE;

    /* Init. */
    head[0] = tail[0] = st[0] = ed[0] = '\0';
    *start = 0; *end = 0;

    /* Strip all space(s) */
    while(criteria[pos])
    {
        if(criteria[pos] != ' ')
            str[i++] = criteria[pos];
        pos++;
    }
    str[i] = '\0';

    if(i == 0)
        return FALSE;

    i = 0;
    len = strlen(str);

    /* Look for possible leading constant */
    if(str[i] == '"')
    {
        i++;
        j = 0;
        while(i < len && str[i] != '"')
        {
            head[j++] = str[i++];
        }
        if(i >= len)
            return FALSE;

        head[j] = '\0';
        i++;

        if(!str[i])
            return TRUE;
    }


    /* Get the possible start position */
    if(str[i] != '"')
    {
        j = 0;
        while(i < len && str[i] != '-')
        {
            st[j++] = str[i++];
        }
        if(i >= len)
            return FALSE;

        st[j] = '\0';
        i++;

        if(!str[i])
            return FALSE;

        /* Get the end position */
        j = 0;
        while(i < len && str[i] != '"')
        {
            ed[j++] = str[i++];
        }

        ed[j] = '\0';

        s = atoi(st); e = atoi(ed);
        if(e < s) /* error */
            return FALSE;

        *start = s; *end = e;

        if(!str[i])
            return TRUE;
    }

    /* Look for possible ending constant */
    if(str[i] == '"')
    {
        i++;
        j = 0;
        while(i < len && str[i] != '"')
        {
            tail[j++] = str[i++];
        }
        if(i >= len)
            return FALSE;

        tail[j] = '\0';
        i++;

        if(!str[i])
            return TRUE;
    }

    return FALSE;
}

/***************************************************************************
pd_convert - Convert product text file to .pd file.
***************************************************************************/
INT pd_convert(VOID)
{
#define BIG_BUF  250
    INT key, pos, n = 0, len, i;
    CHAR prompt[80], SrcFile[21], DstFile[21], criteria[16][21];
    BOOLEAN endFlg = FALSE, leave;
    INT start, end;
    CHAR head[32], tail[32];
    PDREC pd;
    FILE *fin, *fout;
    CHAR line[BIG_BUF], *ptr, temp[32];

    memset(criteria, 0, 16*21);

    SrcFile[0] = '\0';
    strcpy(prompt, "Enter product text file path and name (e.g. A:\\YYY.TXT): ");
    key = inpt(1, 25, prompt, SrcFile, 20, CT_CHAR, NO);
    SrcFile[20] = '\0';
    if(key == KEY_ESC1 || key == KEY_ESC || key == KEY_F10 || key == KEY_ENTER)
        return 0;

    /* Does file exist ? */
    if(!FileExists(SrcFile))
    {
        message(M_SCR|M_KEY, "Product text file (%s) doesn't exist - hit any key", SrcFile);
        return 0;
    }

    DstFile[0] = '\0';
    strcpy(prompt, "Enter output .pd file path and name (e.g. C:\\KK\\YYY.PD): ");
    key = inpt(1, 25, prompt, DstFile, 20, CT_CHAR, NO);
    DstFile[20] = '\0';
    if(key == KEY_ESC1 || key == KEY_ESC || key == KEY_F10 || key == KEY_ENTER)
        return 0;

    message(M_SCR, NULL);

    store();

    setwindow(20, 5, 59, 22, "       Convert product data file", "  Enter starting/ending position, etc.", "F1=HELP                         F10=DONE", "");
    Print("\n\r PD Code: \n\r Description: \n\r PD Vendor: \n\r Cross Reference: \n\r Class: \n\r Prod. Line:");
    Print("\n\r Weight (LBS): \n\r Size (CuFt): \n\r List Price: \n\r Dealer Price: \n\r Net Price: \n\r Sales Price:");
    Print("\n\r Ship. Meth.: \n\r Ship. Code: \n\r Ship. Note: \n\r Ship. Rate:");

    key = KEY_DOWN;
    pos = 0;
    do {
        switch (key) {
            case KEY_SHFT_TAB:
            case KEY_UP:
                if (pos == 1)
                    pos = 17;
                pos--;
                break;

            case KEY_TAB2:
            case KEY_DOWN:
                if (pos == 16)
                    pos = 0;
                pos++;
                break;

            case KEY_CTRL_PGUP:
            case KEY_HOME:
                pos = 1;
                break;

            case KEY_CTRL_PGDOWN:
            case KEY_END:
                pos = 16;
                break;

            case KEY_F10:
                pos = 17;
                break;

            /* this never gets hit because inpt() will not return KEY_ESC */
            case KEY_ESC:
                pos = 17;
                endFlg = TRUE;
                break;

            default:
                if (pos == 16)
                    pos = 0;
                pos++;
                break;
        }

        switch (pos) {
            case 1: /* PD code */
                leave = FALSE;
                while(!leave)
                {
                    key = inpt(19, pos+1, "", criteria[pos-1], 20, CT_CHAR, NO);
                    if(!criteria[pos-1][0])
                        leave = TRUE;
                    else if(get_position(criteria[pos-1], head, &start, &end, tail) != TRUE)
                        message(M_SCR|M_KEY, "Input error. Correct format: \"Const\"StartPos-EndPos\"Const\" - hit any key");
                    else /* Input is OK */
                        leave = TRUE;
                }
                break;
            case 2:
                leave = FALSE;
                while(!leave)
                {
                    key = inpt(19, pos+1, "", criteria[pos-1], 20, CT_CHAR, NO);
                    if(!criteria[pos-1][0])
                        leave = TRUE;
                    else if(get_position(criteria[pos-1], head, &start, &end, tail) != TRUE)
                        message(M_SCR|M_KEY, "Input error. Correct format: \"Const\"StartPos-EndPos\"Const\" - hit any key");
                    else /* Input is OK */
                        leave = TRUE;
                }
                break;
            case 3:
                leave = FALSE;
                while(!leave)
                {
                    key = inpt(19, pos+1, "", criteria[pos-1], 20, CT_CHAR, NO);
                    if(!criteria[pos-1][0])
                        leave = TRUE;
                    else if(get_position(criteria[pos-1], head, &start, &end, tail) != TRUE)
                        message(M_SCR|M_KEY, "Input error. Correct format: \"Const\"StartPos-EndPos\"Const\" - hit any key");
                    else /* Input is OK */
                        leave = TRUE;
                }
                break;
            case 4:
                leave = FALSE;
                while(!leave)
                {
                    key = inpt(19, pos+1, "", criteria[pos-1], 20, CT_CHAR, NO);
                    if(!criteria[pos-1][0])
                        leave = TRUE;
                    else if(get_position(criteria[pos-1], head, &start, &end, tail) != TRUE)
                        message(M_SCR|M_KEY, "Input error. Correct format: \"Const\"StartPos-EndPos\"Const\" - hit any key");
                    else /* Input is OK */
                        leave = TRUE;
                }
                break;
            case 5:
                leave = FALSE;
                while(!leave)
                {
                    key = inpt(19, pos+1, "", criteria[pos-1], 20, CT_CHAR, NO);
                    if(!criteria[pos-1][0])
                        leave = TRUE;
                    else if(get_position(criteria[pos-1], head, &start, &end, tail) != TRUE)
                        message(M_SCR|M_KEY, "Input error. Correct format: \"Const\"StartPos-EndPos\"Const\" - hit any key");
                    else /* Input is OK */
                        leave = TRUE;
                }
                break;
            case 6:
                leave = FALSE;
                while(!leave)
                {
                    key = inpt(19, pos+1, "", criteria[pos-1], 20, CT_CHAR, NO);
                    if(!criteria[pos-1][0])
                        leave = TRUE;
                    else if(get_position(criteria[pos-1], head, &start, &end, tail) != TRUE)
                        message(M_SCR|M_KEY, "Input error. Correct format: \"Const\"StartPos-EndPos\"Const\" - hit any key");
                    else /* Input is OK */
                        leave = TRUE;
                }
                break;
            case 7:
                leave = FALSE;
                while(!leave)
                {
                    key = inpt(19, pos+1, "", criteria[pos-1], 20, CT_CHAR, NO);
                    if(!criteria[pos-1][0])
                        leave = TRUE;
                    else if(get_position(criteria[pos-1], head, &start, &end, tail) != TRUE)
                        message(M_SCR|M_KEY, "Input error. Correct format: \"Const\"StartPos-EndPos\"Const\" - hit any key");
                    else /* Input is OK */
                        leave = TRUE;
                }
                break;
            case 8:
                leave = FALSE;
                while(!leave)
                {
                    key = inpt(19, pos+1, "", criteria[pos-1], 20, CT_CHAR, NO);
                    if(!criteria[pos-1][0])
                        leave = TRUE;
                    else if(get_position(criteria[pos-1], head, &start, &end, tail) != TRUE)
                        message(M_SCR|M_KEY, "Input error. Correct format: \"Const\"StartPos-EndPos\"Const\" - hit any key");
                    else /* Input is OK */
                        leave = TRUE;
                }
                break;
            case 9:
                leave = FALSE;
                while(!leave)
                {
                    key = inpt(19, pos+1, "", criteria[pos-1], 20, CT_CHAR, NO);
                    if(!criteria[pos-1][0])
                        leave = TRUE;
                    else if(get_position(criteria[pos-1], head, &start, &end, tail) != TRUE)
                        message(M_SCR|M_KEY, "Input error. Correct format: \"Const\"StartPos-EndPos\"Const\" - hit any key");
                    else /* Input is OK */
                        leave = TRUE;
                }
                break;
            case 10:
                leave = FALSE;
                while(!leave)
                {
                    key = inpt(19, pos+1, "", criteria[pos-1], 20, CT_CHAR, NO);
                    if(!criteria[pos-1][0])
                        leave = TRUE;
                    else if(get_position(criteria[pos-1], head, &start, &end, tail) != TRUE)
                        message(M_SCR|M_KEY, "Input error. Correct format: \"Const\"StartPos-EndPos\"Const\" - hit any key");
                    else /* Input is OK */
                        leave = TRUE;
                }
                break;
            case 11:
                leave = FALSE;
                while(!leave)
                {
                    key = inpt(19, pos+1, "", criteria[pos-1], 20, CT_CHAR, NO);
                    if(!criteria[pos-1][0])
                        leave = TRUE;
                    else if(get_position(criteria[pos-1], head, &start, &end, tail) != TRUE)
                        message(M_SCR|M_KEY, "Input error. Correct format: \"Const\"StartPos-EndPos\"Const\" - hit any key");
                    else /* Input is OK */
                        leave = TRUE;
                }
                break;
            case 12:
                leave = FALSE;
                while(!leave)
                {
                    key = inpt(19, pos+1, "", criteria[pos-1], 20, CT_CHAR, NO);
                    if(!criteria[pos-1][0])
                        leave = TRUE;
                    else if(get_position(criteria[pos-1], head, &start, &end, tail) != TRUE)
                        message(M_SCR|M_KEY, "Input error. Correct format: \"Const\"StartPos-EndPos\"Const\" - hit any key");
                    else /* Input is OK */
                        leave = TRUE;
                }
                break;
            case 13:
                leave = FALSE;
                while(!leave)
                {
                    key = inpt(19, pos+1, "", criteria[pos-1], 20, CT_CHAR, NO);
                    if(!criteria[pos-1][0])
                        leave = TRUE;
                    else if(get_position(criteria[pos-1], head, &start, &end, tail) != TRUE)
                        message(M_SCR|M_KEY, "Input error. Correct format: \"Const\"StartPos-EndPos\"Const\" - hit any key");
                    else /* Input is OK */
                        leave = TRUE;
                }
                break;
            case 14:
                leave = FALSE;
                while(!leave)
                {
                    key = inpt(19, pos+1, "", criteria[pos-1], 20, CT_CHAR, NO);
                    if(!criteria[pos-1][0])
                        leave = TRUE;
                    else if(get_position(criteria[pos-1], head, &start, &end, tail) != TRUE)
                        message(M_SCR|M_KEY, "Input error. Correct format: \"Const\"StartPos-EndPos\"Const\" - hit any key");
                    else /* Input is OK */
                        leave = TRUE;
                }
                break;
            case 15:
                leave = FALSE;
                while(!leave)
                {
                    key = inpt(19, pos+1, "", criteria[pos-1], 20, CT_CHAR, NO);
                    if(!criteria[pos-1][0])
                        leave = TRUE;
                    else if(get_position(criteria[pos-1], head, &start, &end, tail) != TRUE)
                        message(M_SCR|M_KEY, "Input error. Correct format: \"Const\"StartPos-EndPos\"Const\" - hit any key");
                    else /* Input is OK */
                        leave = TRUE;
                }
                break;
            case 16:
                leave = FALSE;
                while(!leave)
                {
                    key = inpt(19, pos+1, "", criteria[pos-1], 20, CT_CHAR, NO);
                    if(!criteria[pos-1][0])
                        leave = TRUE;
                    else if(get_position(criteria[pos-1], head, &start, &end, tail) != TRUE)
                        message(M_SCR|M_KEY, "Input error. Correct format: \"Const\"StartPos-EndPos\"Const\" - hit any key");
                    else /* Input is OK */
                        leave = TRUE;
                }
                break;
        }

    } while (pos != 17);

    restore();

    if(endFlg) /* user wants to exit */
        return 0;

    /* Convert the file */
    fin = fopen(SrcFile, "r");
    fout = fopen(DstFile, "w+b");

    { USHORT nrecs = (USHORT)n; fwrite(&nrecs, sizeof(USHORT), 1, fout); }

    while(fgets(line, BIG_BUF, fin) > 0)
    {
        line[BIG_BUF-1] = '\0';
        memset(&pd, 0, sizeof(PDREC));

        /* Go through criteria one by one, and convert field one by one */
        if(criteria[0][0]) /* PD code */
        {
            get_position(criteria[0], head, &start, &end, tail);

            if(head[0])
                strcat(pd.code, head);

            if(start > 0 && start <= strlen(line))
            {
                len = end - start + 1;
                if(len > 20 - strlen(head))
                    len = 20 - strlen(head);
                ptr = line + start - 1;
                strncat(pd.code, ptr, len);
            }

            while(pd.code[strlen(pd.code)-1] == ' ') /* Strip trailing space */
                pd.code[strlen(pd.code)-1] = '\0';

            /* replace the space with hyphen in the code field */
            for(i = 0; pd.code[i] && i < 20; i++)
                if(pd.code[i] == ' ')
                    pd.code[i] = '-';

            if(tail[0])
                strncat(pd.code, tail, 20 - strlen(pd.code));
        }
        if(criteria[1][0]) /* Description */
        {
            get_position(criteria[1], head, &start, &end, tail);

            if(head[0])
                strcat(pd.description, head);

            if(start > 0 && start <= strlen(line))
            {
                len = end - start + 1;
                if(len > 30 - strlen(head))
                    len = 30 - strlen(head);
                ptr = line + start - 1;
                strncat(pd.description, ptr, len);
            }

            while(pd.description[strlen(pd.description)-1] == ' ') /* Strip trailing space */
                pd.description[strlen(pd.description)-1] = '\0';

            if(tail[0])
                strncat(pd.description, tail, 30 - strlen(pd.description));
        }
        if(criteria[2][0]) /* PD vendor */
        {
            get_position(criteria[2], head, &start, &end, tail);

            if(head[0])
                strcat(pd.vendor, head);

            if(start > 0 && start <= strlen(line))
            {
                len = end - start + 1;
                if(len > 10 - strlen(head))
                    len = 10 - strlen(head);
                ptr = line + start - 1;
                strncat(pd.vendor, ptr, len);
            }

            while(pd.vendor[strlen(pd.vendor)-1] == ' ') /* Strip trailing space */
                pd.vendor[strlen(pd.vendor)-1] = '\0';

            if(tail[0])
                strncat(pd.vendor, tail, 10 - strlen(pd.vendor));
        }
        if(criteria[3][0]) /* Cross ref. */
        {
            get_position(criteria[3], head, &start, &end, tail);

            if(head[0])
                strcat(pd.ref, head);

            if(start > 0 && start <= strlen(line))
            {
                len = end - start + 1;
                if(len > 20 - strlen(head))
                    len = 20 - strlen(head);
                ptr = line + start - 1;
                strncat(pd.ref, ptr, len);
            }

            while(pd.ref[strlen(pd.ref)-1] == ' ') /* Strip trailing space */
                pd.ref[strlen(pd.ref)-1] = '\0';

            /* replace the space with hyphen in the cross ref. field */
            for(i = 0; pd.ref[i] && i < 20; i++)
                if(pd.ref[i] == ' ')
                    pd.ref[i] = '-';

            if(tail[0])
                strncat(pd.ref, tail, 20 - strlen(pd.ref));
        }
        if(criteria[4][0]) /* Class */
        {
            get_position(criteria[4], head, &start, &end, tail);

            if(head[0])
                strcat(pd.clas, head);

            if(start > 0 && start <= strlen(line))
            {
                len = end - start + 1;
                if(len > 4 - strlen(head))
                    len = 4 - strlen(head);
                ptr = line + start - 1;
                strncat(pd.clas, ptr, len);
            }

            while(pd.clas[strlen(pd.clas)-1] == ' ') /* Strip trailing space */
                pd.clas[strlen(pd.clas)-1] = '\0';

            if(tail[0])
                strncat(pd.clas, tail, 4 - strlen(pd.clas));
        }
        if(criteria[5][0]) /* Prod. line */
        {
            get_position(criteria[5], head, &start, &end, tail);

            if(head[0])
                strcat(pd.line, head);

            if(start > 0 && start <= strlen(line))
            {
                len = end - start + 1;
                if(len > 2 - strlen(head))
                    len = 2 - strlen(head);
                ptr = line + start - 1;
                strncat(pd.line, ptr, len);
            }

            while(pd.line[strlen(pd.line)-1] == ' ') /* Strip trailing space */
                pd.line[strlen(pd.line)-1] = '\0';

            if(tail[0])
                strncat(pd.line, tail, 2 - strlen(pd.line));
        }
        if(criteria[6][0]) /* Weight */
        {
            get_position(criteria[6], head, &start, &end, tail);

            /* If heading constant is given, use it, otherwise
               assume reading from the file. */
            if(head[0])
            {
                pd.weight = (float) atof(head);
            }
            else if(start > 0 && start <= strlen(line))
            {
                len = end - start + 1;
                if(len > 20)
                    len = 20;
                ptr = line + start - 1;
                memset(temp, 0, 32);
                strncat(temp, ptr, len);
                pd.weight = (float) atof(temp);
            }
        }
        if(criteria[7][0]) /* Size */
        {
            get_position(criteria[7], head, &start, &end, tail);

            /* If heading constant is given, use it, otherwise
               assume reading from the file. */
            if(head[0])
            {
                pd.size = (float) atof(head);
            }
            else if(start > 0 && start <= strlen(line))
            {
                len = end - start + 1;
                if(len > 20)
                    len = 20;
                ptr = line + start - 1;
                memset(temp, 0, 32);
                strncat(temp, ptr, len);
                pd.size = (float) atof(temp);
            }
        }
        if(criteria[8][0]) /* List price */
        {
            get_position(criteria[8], head, &start, &end, tail);

            /* If heading constant is given, use it, otherwise
               assume reading from the file. */
            if(head[0])
            {
                pd.p1 = (float) atof(head);
            }
            else if(start > 0 && start <= strlen(line))
            {
                len = end - start + 1;
                if(len > 20)
                    len = 20;
                ptr = line + start - 1;
                memset(temp, 0, 32);
                strncat(temp, ptr, len);
                pd.p1 = (float) atof(temp);
            }
        }
        if(criteria[9][0]) /* Dealer price */
        {
            get_position(criteria[9], head, &start, &end, tail);

            /* If heading constant is given, use it, otherwise
               assume reading from the file. */
            if(head[0])
            {
                pd.p2 = (float) atof(head);
            }
            else if(start > 0 && start <= strlen(line))
            {
                len = end - start + 1;
                if(len > 20)
                    len = 20;
                ptr = line + start - 1;
                memset(temp, 0, 32);
                strncat(temp, ptr, len);
                pd.p2 = (float) atof(temp);
            }
        }
        if(criteria[10][0]) /* Net price */
        {
            get_position(criteria[10], head, &start, &end, tail);

            /* If heading constant is given, use it, otherwise
               assume reading from the file. */
            if(head[0])
            {
                pd.p3 = (float) atof(head);
            }
            else if(start > 0 && start <= strlen(line))
            {
                len = end - start + 1;
                if(len > 20)
                    len = 20;
                ptr = line + start - 1;
                memset(temp, 0, 32);
                strncat(temp, ptr, len);
                pd.p3 = (float) atof(temp);
            }
        }
        if(criteria[11][0]) /* Sales price */
        {
            get_position(criteria[11], head, &start, &end, tail);

            /* If heading constant is given, use it, otherwise
               assume reading from the file. */
            if(head[0])
            {
                pd.sales = (float) atof(head);
            }
            else if(start > 0 && start <= strlen(line))
            {
                len = end - start + 1;
                if(len > 20)
                    len = 20;
                ptr = line + start - 1;
                memset(temp, 0, 32);
                strncat(temp, ptr, len);
                pd.sales = (float) atof(temp);
            }
        }
        if(criteria[12][0]) /* Ship. method */
        {
            get_position(criteria[12], head, &start, &end, tail);

            if(head[0])
                strcat(pd.sh1, head);

            if(start > 0 && start <= strlen(line))
            {
                len = end - start + 1;
                if(len > 2 - strlen(head))
                    len = 2 - strlen(head);
                ptr = line + start - 1;
                strncat(pd.sh1, ptr, len);
            }

            while(pd.sh1[strlen(pd.sh1)-1] == ' ') /* Strip trailing space */
                pd.sh1[strlen(pd.sh1)-1] = '\0';

            if(tail[0])
                strncat(pd.sh1, tail, 2 - strlen(pd.sh1));
        }
        if(criteria[13][0]) /* Ship. code */
        {
            get_position(criteria[13], head, &start, &end, tail);

            if(head[0])
                strcat(pd.sh2, head);

            if(start > 0 && start <= strlen(line))
            {
                len = end - start + 1;
                if(len > 2 - strlen(head))
                    len = 2 - strlen(head);
                ptr = line + start - 1;
                strncat(pd.sh2, ptr, len);
            }

            while(pd.sh2[strlen(pd.sh2)-1] == ' ') /* Strip trailing space */
                pd.sh2[strlen(pd.sh2)-1] = '\0';

            if(tail[0])
                strncat(pd.sh2, tail, 2 - strlen(pd.sh2));
        }
        if(criteria[14][0]) /* Ship. note */
        {
            get_position(criteria[14], head, &start, &end, tail);

            if(head[0])
                strcat(pd.sh3, head);

            if(start > 0 && start <= strlen(line))
            {
                len = end - start + 1;
                if(len > 2 - strlen(head))
                    len = 2 - strlen(head);
                ptr = line + start - 1;
                strncat(pd.sh3, ptr, len);
            }

            while(pd.sh3[strlen(pd.sh3)-1] == ' ') /* Strip trailing space */
                pd.sh3[strlen(pd.sh3)-1] = '\0';

            if(tail[0])
                strncat(pd.sh3, tail, 2 - strlen(pd.sh3));
        }
        if(criteria[15][0]) /* Ship. rate */
        {
            get_position(criteria[15], head, &start, &end, tail);

            /* If heading constant is given, use it, otherwise
               assume reading from the file. */
            if(head[0])
            {
                pd.rate = (float) atof(head);
            }
            else if(start > 0 && start <= strlen(line))
            {
                len = end - start + 1;
                if(len > 20)
                    len = 20;
                ptr = line + start - 1;
                memset(temp, 0, 32);
                strncat(temp, ptr, len);
                pd.rate = (float) atof(temp);
            }
        }

        fwrite(&pd, sizeof(PDREC), 1, fout);
        n++;
    }

    fclose(fin);
    fseek(fout, 0L, SEEK_SET);
    { USHORT nrecs = (USHORT)n; fwrite(&nrecs, sizeof(USHORT), 1, fout); }
    fclose(fout);

    return 0;
}
