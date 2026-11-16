
/**
*   JB_INST.C   - Contains functions for Job Installation Screen
*
*
*   Description -
*
*
*
*
*   (c) Copyright 1997 by Robert Bao for Automation Solutions, Inc.
*   All rights reserved.
*
*
*   Revision History:
*
*   Version     Date        Author      Comments
*   -------     --------    ------      --------------------------------
*   1.00a       07-01-97     ROB        Revision 0
*
**/
#include <stdlib.h>
#include <stdio.h>
#include <dir.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <share.h>
#include "db_defs.h"
#include "vmisc.h"
#include "vfile.h"

#include <sys/stat.h>
#include <io.h>
#include <fcntl.h>

extern struct date today;
extern FILE *defPrinter;
extern PRNCODES prnCodes;
/*
extern SYSREC sysvar;
extern COMPINFOREC compInfo;
extern CHAR *tempDir;
extern MOUSE_CLICK *points;
*/

/* Local globals */
#define CREATE_FROM_PO 0
#define RETRIVE_FROM_FILE 1
#define CREATE_FROM_PO_SCREEN_MSG \
    "F1=HELP F2=ADD F3=DEL F4=NAME F5=PRINT F6=FIND F7=PREV F8=NEXT F9=FORM  F10=EXIT"
#define RETRIVE_FROM_FILE_SCREEN_MSG \
    "F1=HELP F2=ADD F3=DEL F4=NAME F5=PRINT F6=FIND                 F9=FORM  F10=EXIT"

static INT InstFunc = CREATE_FROM_PO;
static BOOLEAN SwitchPOJBName = TRUE;
static CHAR JobInstPath[128];
static CHAR StartDate[9];
static CHAR VendorCode[9];
static CHAR SPCode[9];
static CHAR Status[2];

/* Local function prototype */
INT CreateJobInstallation(INT fileHandle);
INT RetriveJobInstallation(INT fileHandle, CHAR* orgFileName);
VOID SaveInstFile(CHAR* orgFileName, CHAR* sourceFile);
INT GenerateFromPO(CHAR* sDate, CHAR* vdCode, CHAR* spCode, CHAR* status, INT handle);
INT BatchPrintJobInstallation(CHAR* fileName, INT fileHandle);


/*
* JobInstallation - Main entry point for job installation screen
*/
VOID JobInstallation(VOID)
{
    CHAR *list[] = {"Create new installation", "Retrive installation from file", "Print installation", NULL};
    INT choice, i, tmpHandle;
    CHAR date[9], tmpFile[60], orgFileName[60];
    CHAR drive[3], ext[4], name[60], path[60];
    struct date startDate, tmpDate;

    DB_MSG *dbMsg;

    if((choice = window_chooser(15, 8, list)) == -1)
        return;

    /* Generate a temp file to hold the info, and work from that file */
    FileTempName(tmpFile);
    if((tmpHandle = FileOpen(tmpFile)) == -1)
    {
        message(M_SCR|M_KEY, "Unable to open temp file - hit any key to continue");
        return;
    }

    dbMsg = (db[IN].s)->msg;
    switch(choice)
    {
        case 0: /* Create new job installation */
            orgFileName[0] = '\0';
            InstFunc = CREATE_FROM_PO;
            dbMsg->msg = CREATE_FROM_PO_SCREEN_MSG;
            choice = CreateJobInstallation(tmpHandle);
            if(choice == 0)
                message(M_SCR|M_KEY, "No record found - hit any key to continue");
            if(choice < 0) /* User canceled */
            {
                FileClose(tmpHandle);
                FileDelete(tmpFile);
                return;
            }
            break;

        case 1: /* Retrive from existing file */
            InstFunc = RETRIVE_FROM_FILE;
            dbMsg->msg = RETRIVE_FROM_FILE_SCREEN_MSG;
            choice = RetriveJobInstallation(tmpHandle, orgFileName);
            if(choice < 0)
            {
                message(M_SCR|M_KEY, "Unable to open specified file, or file doesn't exist - hit any key");
                FileClose(tmpHandle);
                FileDelete(tmpFile);
                return;
            }
            if(choice == 0) /* user cancel */
            {
                FileClose(tmpHandle);
                FileDelete(tmpFile);
                return;
            }

            FileSeek(tmpHandle, (LONG)0, SEEK_SET);
            FileRead(tmpHandle, &i, sizeof(SHORT));
            if(i == 0)
            {
                message(M_SCR|M_KEY, "No record found in the specified file - hit any key to continue");
                FileClose(tmpHandle);
                FileDelete(tmpFile);
                return;
            }
            break;

        case 2: /* Print weeks of installation */
            BatchPrintJobInstallation(tmpFile, tmpHandle);
            FileClose(tmpHandle);
            FileDelete(tmpFile);

        default:
            /* Can't be */
            return;
    }

    FileClose(tmpHandle);

    /* save IN path */
    strcpy(JobInstPath, db[IN].path);

    _splitpath(_fullpath(tmpFile, tmpFile, 60), drive, path, name, ext);
    drive[2] = '\0';
    strcpy(db[IN].path, drive);
    strcat(db[IN].path, path);
    strcpy(db[IN].fname, name);
    strcpy(db[IN].extension, "$$$");
    s_open(IN);

    /* Prepare date and day of week */
    FileSeek(db[IN].file, (LONG)2, SEEK_SET);
    FileRead(db[IN].file, date, 8);
    date[8] = '\0';
    if(orgFileName[0] == '\0') /* create new */
    {
        orgFileName[0] = date[0];
        orgFileName[1] = date[1];
        orgFileName[2] = date[3];
        orgFileName[3] = date[4];
        orgFileName[4] = date[6];
        orgFileName[5] = date[7];
        orgFileName[6] = '\0';
        strcat(orgFileName, ".IN\0");
    }

    DateStr(&startDate, date);
    for(i = 0; i < 7; i++)
    {
        memcpy(&tmpDate, &startDate, sizeof(struct date));
        StrDate(date, DateAdd(&tmpDate, i));
        strcpy(db[IN].f[i+1].header, date);
        strcat(db[IN].f[i+1].header, "|   ");
        strcat(db[IN].f[i+1].header, DayOfWeek(date));
        /* also do it for job name */
        strcpy(db[IN].f[i+8].header, date);
        strcat(db[IN].f[i+8].header, "|   ");
        strcat(db[IN].f[i+8].header, DayOfWeek(date));
    }

    db_inpt(IN);

    /* restore IN path */
    strcpy(db[IN].path, JobInstPath);

    /* whether to save */
    SaveInstFile(orgFileName, tmpFile);

    FileDelete(tmpFile);
}

/*****************************************************************************
in_funcs (hot key hook) - processes all hot keys for IN (Job installation) 
    ident:        type
        c:        cursor struct
      ptr:        current record
*****************************************************************************/
INT in_funcs(DB_TYPE type, IN_POS *c, INREC *in)
{
    FIELDPARAMS attr, fp;
    INT i, j, idx, vec, n, sourceHandle;
    CHAR tmpPoNum[9], curDate[9], fileName[128], sourceFile[128];
    struct date tmpDate;
    POREC po;
    INREC tmpIn;
    BOOLEAN found;
    static CHAR check[20];

    INT choice;
    CHAR *list[] = {"Print Installation Sheet", "Installation Request Form", NULL};

    attr = db[type].f[c->x];

    switch(c->x)
    {
        case KEY_F4:     /* Toggle PO code and job name */
            c->x = 0;
            for(i = 1; i < 8; i++)
            {
                fp = *(db[type].f+i);
                *(db[type].f+i) = *(db[type].f+7+i);
                *(db[type].f+7+i) = fp;
            }
            SwitchPOJBName = (SwitchPOJBName ? FALSE:TRUE);
            c->set = YES;
            c->flags = 4; /* this will refresh the window */

            break;

        case KEY_F7:
        case KEY_F8:
            if(InstFunc != CREATE_FROM_PO)
                return 0;

            fileName[0] = '\0';
            strncat(fileName, StartDate, 2);
            strncat(fileName, &StartDate[3], 2);
            strncat(fileName, &StartDate[6], 2);
            strcat(fileName, ".IN");
            sprintf(sourceFile, "%s%s.%s", db[type].path, db[type].fname, db[type].extension);
            SaveInstFile(fileName, sourceFile);

            /* Delete file contents */
            s_close(type);
            sourceHandle = open(sourceFile, O_BINARY|SH_DENYNO|O_RDWR|O_TRUNC, S_IREAD|S_IWRITE);
            if(sourceHandle == -1)
            {
                message(M_SCR|M_KEY, "Unable to create temp file - any key to continue");
                return -1;
            }
            db[type].file = sourceHandle;

            /* Update StartDate */
            DateStr(&tmpDate, StartDate);
            if(c->x == KEY_F7)
                StrDate(StartDate, DateAdd(&tmpDate, -7));
            else
                StrDate(StartDate, DateAdd(&tmpDate, 7));

            /* check whether the file for prev/next is already generated */
            sprintf(fileName, "%s", JobInstPath);
            strncat(fileName, StartDate, 2);
            strncat(fileName, &StartDate[3], 2);
            strncat(fileName, &StartDate[6], 2);
            strcat(fileName, ".IN");

            if(access(fileName, 00) == 0) /* exists */
            {
                sourceHandle = FileOpen(fileName);
                FileCopyH(sourceHandle, db[type].file);
                FileClose(sourceHandle);
            }
            else
                GenerateFromPO(StartDate, VendorCode, SPCode, Status, db[type].file);

            s_close(type);
            
            /* update date info */
            for(i = 0; i < 7; i++)
            {
                DateStr(&tmpDate, StartDate);
                StrDate(curDate, DateAdd(&tmpDate, i));
                strcpy(db[IN].f[i+1].header, curDate);
                strcat(db[IN].f[i+1].header, "|   ");
                strcat(db[IN].f[i+1].header, DayOfWeek(curDate));
                /* also do it for job name */
                strcpy(db[IN].f[i+8].header, curDate);
                strcat(db[IN].f[i+8].header, "|   ");
                strcat(db[IN].f[i+8].header, DayOfWeek(curDate));
            }

            /* update the current selected in */
            n = s_nrecs(type);
            for(i = 0; i < n; i++)
            {
                s_read(type, &tmpIn, i);
                if(i == c->ypos)
                {
                    *in = tmpIn;
                    break;
                }
            }
            if(i >= n)
            {
                s_read(type, &tmpIn, 0);
                *in = tmpIn;
                c->ypos = 0;
            } 

            c->set = YES;
            c->flags = 4;

            break;

        case KEY_F9:    /* Print functions */
            choice = window_chooser(26, 9, list);
            if(choice == -1)
                break;

            switch(choice)
            {
                case 0:
                    PrintInstallation(FALSE);
                    break;

                case 1:
                    inpt(1, 25, "Enter PO number: ", check, 8, CT_CHAR, NO);

                    /* convert PO number to be 0000xxxx format */
                    i = atoi(check);
                    sprintf(check, "%08d", i);
                    vec = s_getno(PO, 0, check);
                    if(vec == -1)
                    {
                        message(M_SCR|M_KEY, "PO number doesn't exist - hit any key to continue");
                        break;
                    }

                    s_read(PO, &po, vec);
                    InstallationRequestForm(&po);
                    break;

                default:
                    ;
            }

            break;

        case 0:         /* Make sure Vendor code exists */
            c->last = c->x;
            strcpy(check, in->inst_code);
            c->x = inpt(attr.x, c->y, "", in->inst_code, attr.width-1, CT_CHAR, NO);
            strupr(in->inst_code);
            if(s_getno(VD, 0, in->inst_code) == -1) {
                message(M_SCR|M_KEY, "Vendor is not defined - hit any key");
                strcpy(in->inst_code, check);
                if(check[0] == '\0' || !strcmp(check, "") || !strcmp(check, " "))
                    c->x = c->last;
            }
            in_printline(type, c->y, c->ypos, in);

            break;

        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            if(!SwitchPOJBName)
            {
                skipfield(type, c, in);
                break;
            }

            /* code to make sure PO number's existance and update shipdate, etc. */
            idx = c->x - 1;
            c->last = c->x;
            vec = -1;
            strcpy(check, in->po[idx]);
            c->x = inpt(attr.x, c->y, "", in->po[idx], attr.width-1, CT_CHAR, NO);

            /* convert PO number to be 0000xxxx format */
            i = atoi(in->po[idx]);
            sprintf(tmpPoNum, "%08d", i);
            if(in->po[idx][0] != '\0' && (vec = s_getno(PO, 0, tmpPoNum)) == -1)
            {
                if(!strcmp(in->po[idx], "0")) /* special, means deleting the existing PO */
                {
                    in->po[idx][0] = '\0';
                    in->name[idx][0] = '\0';
                    in->jb[idx][0] = '\0';

                    /* add a question to ask a date to move this PO to. 9/12/97 */
                    if(message(M_SCR|M_KEY, "Do you want to move this PO's shipdate to another day? (N/y)") == 'Y')
                    {
                        char period[9];
                        int key, diff;
                        struct date stdt, dstdt;

ag:                     memset(period, 0, 9);
                        key = inpt(1, 25, " Enter new shipdate: ", period, 8, CT_CHDATE, NO); 
                        if(key != KEY_ESC1 && key != KEY_ESC && key != KEY_F10)
                        {
                            period[8] = '\0';
                            if(period[0])
                            {
                                struct date testdate;
                                
                                testdate = datecheck(period);
                                if(testdate.da_year == 0)
                                {
                                    message(M_SCR|M_KEY, "Date must be in format mm/dd/yy - hit any key to enter again");
                                    goto ag;
                                }
                            }
                            else
                            {
                                message(M_SCR|M_KEY, "Date must be in format mm/dd/yy - hit any key to enter again");
                                goto ag;
                            }
                            vec = s_getno(PO, 0, check);
                            s_read(PO, &po, vec);
                            strcpy(po.shipdate, period);
                            s_write(PO, &po, vec, NO);

                            /* re-display the changed shipdate PO to current screen if needed */
                            DateStr(&stdt, StartDate);
                            DateStr(&dstdt, period);
                            diff = DiffDate(&stdt, &dstdt);
                            if(diff >= 0 && diff < 7)
                                strcpy(in->po[diff], check);
                        }
                    }
                }
                else
                {
                    message(M_SCR|M_KEY, "PO does not exist - hit any key");
                    strcpy(in->po[idx], check);
                }
            }
            else if(vec != -1)
            {
                s_read(PO, &po, vec);
                strcpy(in->po[idx], po.po_number);
                strcpy(in->name[idx], po.jbname);
                strcpy(in->jb[idx], po.jbcode);

                /* Check date for possible shipdate updating */
                n = s_nrecs(IN);
                found = FALSE;
                for(i = 0; i < n; i++)
                {
                    s_read(IN, &tmpIn, i);                    
                    for(j = 0; j <= idx; j++)
                    {
                        if(!strcmp(tmpIn.po[j], po.po_number)) /* no need to update */
                        {
                            found = TRUE;
                            break;
                        }
                    }
                    if(found)
                        break;
                }
                if(!found) /* potential for updating */
                {
                    FileSeek(db[IN].file, 2, SEEK_SET);
                    FileRead(db[IN].file, curDate, 8);
                    curDate[8] = '\0';

                    DateStr(&tmpDate, curDate);
                    StrDate(curDate, DateAdd(&tmpDate, idx));

                    if(strdatecmp(curDate, po.shipdate) != 0)
                    {
                        char msg[80];
                        INT ans;

                        sprintf(msg, "This PO's shipdate is changed from %s to %s, update it? (y/N)", po.shipdate, curDate);
                        ans = message(M_SCR|M_KEY, msg);
                        if(ans == 'Y' || ans == 'y')
                        {
                            strcpy(po.shipdate, curDate);
                            s_write(PO, &po, vec, NO);
                        }
                    }
                }
            }
            in_printline(type, c->y, c->ypos, in);

            strcpy(check, in->po[idx]); /* make a copy for future use */
            break;

        default:
            ;
    }

    return 0;
}

/*****************************************************************************
 CreateJobInstallation - Generate job inst. info from PO file, and save
                         the info to the specified file
*****************************************************************************/
INT CreateJobInstallation(INT handle)
{
    INT pos = 0, key = KEY_DOWN, test;
    VDREC vd;
    SPREC sp;

    StartDate[0] = '\0';
    strcpy(vd.code, "IN*");
    sp.code[0] = '\0';
    strcpy(Status, "R");

    store();
    setwindow(25, 11, 55, 16, "       Job Installation", "     Enter Match Criteria", "F1=HELP                F10=DONE", "");
    Print("\n\r Start Date: \n\r VD Code: \n\r SP Code: \n\r Status:");
    do {
        switch (key) {
            case KEY_SHFT_TAB:
            case KEY_UP:
                if (pos == 1)
                    pos = 5;
                pos--;
                break;

            case KEY_TAB2:
            case KEY_DOWN:
                if (pos == 4)
                    pos = 0;
                pos++;
                break;

            case KEY_CTRL_PGUP:
            case KEY_HOME:
                pos = 1;
                break;

            case KEY_CTRL_PGDOWN:
            case KEY_END:
                pos = 4;
                break;

            case KEY_F10:
                pos = 5;
                break;

            /* this never gets hit because inpt() will not return KEY_ESC */
            case KEY_ESC:
                return -1;

            default:
                if (pos == 4)
                    pos = 0;
                pos++;
                break;
        }

        switch (pos) {

            /*
            * input of "Start Date"
            */
            case 1:
                do {
                    key = inpt(14, 2, "", StartDate, 8, CT_CHDATE, NO);
                    if(key == KEY_F1)
                        StrDate(StartDate, &today);
                } while (key == KEY_F1);
                break;

            /*
            * Pick Vendor code
            */
            case 2:
                do {
                    s_open(VD);
                    key = inpt(11, 3, "", vd.code, 8, VD, NO);
                    if (key == KEY_F1) {
                        if ((test = gethelp(VD, s_getno(VD, 0, vd.code), 9, 4, 2-1, 14-1)) != -1)
                            s_read(VD, &vd, test);
                    }
                    s_close(VD);
                } while (key == KEY_F1);
                break;


            /*
            * Pick SP code
            */
            case 3:
                do {
                    s_open(SP);
                    key = inpt(11, 4, "", sp.code, 3, SP, NO);
                    if (key == KEY_F1) {
                        if ((test = gethelp(SP, s_getno(SP, 0, sp.code), 9, 4, 2-1, 14-1)) != -1)
                            s_read(SP, &sp, test);
                    }
                    s_close(SP);

                } while (key == KEY_F1);
                break;

            /*
            *  choose "Status" to match
            */
            case 4:
                key = inpt(10, 5, "", Status, 1, CT_CHAR, NO);
                break;

            case 5:
                break;

            default:
                break;
        }
    } while (pos != 5);

    restore();

    strcpy(VendorCode, vd.code);
    strcpy(SPCode, sp.code);
    if(Status[0] == '\0' || Status[0] == ' ' || Status[0] == '*')
        Status[0] = '*';
    else
        Status[0] = toupper(Status[0]);
    Status[1] = '\0';

    return GenerateFromPO(StartDate, VendorCode, SPCode, Status, handle);
}


/*****************************************************************************
 RetriveJobInstallation - Retrive job inst. info from a file, and save
                          the info to the specified file
*****************************************************************************/
INT RetriveJobInstallation(INT handle, CHAR* orgFileName)
{
    CHAR destFile[128];
    INT key, destHandle;

    message(M_SCR, "Load File or F1 for list: ");
    orgFileName[0] = '\0';
    key = inpt(27, 25, "", orgFileName, 50, CT_CHAR, NO);
    message(M_SCR, NULL);
    if (key != KEY_F10)
    {
        if (key == KEY_F1)
        {
            sprintf(destFile, "%s*.*", db[IN].path);
            loader(destFile, orgFileName);
            sprintf(destFile, "%s%s", db[IN].path, orgFileName);
        }
        else
        {
            if(strstr(orgFileName, ":") || strstr(orgFileName, "\\"))
            /* user enters abosulate path, use it */
                strcpy(destFile, orgFileName);
            else
                sprintf(destFile, "%s%s", db[IN].path, orgFileName);
        }

        /* make sure file exists and can be open */
        if(access(destFile, 00) != 0) 
            return -1;
        if((destHandle = FileOpen(destFile)) == -1)
            return -1;

        FileCopyH(destHandle, handle);
        FileClose(destHandle);

        return 1;
    }

    return 0;
}

/*****************************************************************************
 InstallationRequestForm - Call jb_installation() function to generate 
                           a on-screen form
*****************************************************************************/
VOID InstallationRequestForm(POREC *po)
{
    ESREC es;
    JBREC jb;
    CHAR sttoday[9];
    INT vec, ans = 'Y';

    StrDate(sttoday, &today);

    /* too bad, the following piece of code is duplicated for subcontract agreement */
    /* because it is used only for installation before, then sub-agree is added */
    /* if job is already released for fabrication, ask for whether to re-release it */
    if(po->status[0] == 'R')
    {
        ans = message(M_SCR|M_KEY, "Job is already released for fabrication. Re-release it (y/N) ?");
        if(ans != 'Y' && ans != 'y')
            return;
    }
    else /* only ask save it or not if not shipped */
    {
        if(po->status[0] == 'S')
            ans = 'N';
        else
            ans = message(M_SCR|M_KEY, "Change status to 'R' and save it (y/N) ?");
    }

    if (ans == 'Y')
    {
        vec = s_getno(PO, 0, po->po_number);
        po->status[0] = 'R';
        strcpy(po->release_date, sttoday);
        s_write(PO, po, vec, FALSE);
    }

    s_open(ES);
    if ((vec = s_getno(ES, 0, po->jbcode)) == -1) {
        message(M_SCR|M_KEY, "Estimate is not defined - hit any key");
        return;
    }
    s_read(ES, &es, vec);
    if (es.status[0] != 'R' && ans == 'Y')
    {
         es.status[0] = 'R';
         s_write(ES, &es, vec, FALSE);
    }
    s_close(ES);

    s_open(JB);
    if ((vec = s_getno(JB, 0, po->jbcode)) == -1) {
         message(M_SCR|M_KEY, "Job is not defined - hit any key");
         return;
    }
    s_read(JB, &jb, vec);
    if (jb.status[0] != 'R' && ans == 'Y')
    {
         jb.status[0] = 'R';
         s_write(JB, &jb, vec, FALSE);
    }
    s_close(JB);

    jb_installation(NULL, PO, po);

    return;
}

/*****************************************************************************
 PrintInstallation - Print Installation Schedule in Condensed (not rotate) mode
*****************************************************************************/
VOID PrintInstallation(BOOLEAN bSilence)
{
    INT i, j, n, pageNum;
    INREC in;
    FILE *fp = defPrinter;
    CHAR curDate[7][9];
    struct date tmpDate;

    CHAR other[7][20], spCode[7][4], tempStr[32];
    POREC po;
    ESREC es;
    PLREC pl;
    INT k, bg, ed, numPL, qty, vec;

    if((n = s_nrecs(IN)) <= 0)
    {
        if(!bSilence)
            message(M_SCR|M_KEY, "No records found - hit any key to continue");
        return;
    }

    if(!bSilence)
    {
        if(!SelectPrinter() || printercheck() == -1)
            return;
    }

    fprintf(fp, "%s", prnCodes.cond);
    fprintf(fp, "%s", prnCodes.land);

    /* init */
    pageNum = (n-1)/7 + 1;

    FileSeek(db[IN].file, 2, SEEK_SET);
    FileRead(db[IN].file, curDate[0], 8);
    curDate[0][8] = '\0';
    for(i = 1; i < 7; i++)
    {
        DateStr(&tmpDate, curDate[0]);
        StrDate(curDate[i], DateAdd(&tmpDate, i));
    }

    for(i = 0; i < n; i++)
    {
        if((i%7) == 0) /* need to print header, 7 venders per page */
        {
            if(i != 0) /* page break */
                fputc(12, fp);
            fprintf(fp, "                                                                   Job Installation Schedule                                                                Page %d of %d\n\r\n\r", i/7+1, pageNum);
            fprintf(fp, "                                                            Time period: %s(%s) - %s(%s)\n\r", curDate[0], DayOfWeek(curDate[0]), curDate[6], DayOfWeek(curDate[6]));
            fprintf(fp, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------\n\r");
            fprintf(fp, "| Installer |       %8.8s      |       %8.8s      |       %8.8s      |       %8.8s      |       %8.8s      |       %8.8s      |       %8.8s      |\n\r",
                curDate[0], curDate[1], curDate[2], curDate[3], curDate[4], curDate[5], curDate[6]);
            fprintf(fp, "|           |         %3.3s         |         %3.3s         |         %3.3s         |         %3.3s         |         %3.3s         |         %3.3s         |         %3.3s         |\n\r",
                DayOfWeek(curDate[0]), DayOfWeek(curDate[1]), DayOfWeek(curDate[2]), DayOfWeek(curDate[3]), DayOfWeek(curDate[4]), DayOfWeek(curDate[5]), DayOfWeek(curDate[6]));
            fprintf(fp, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------\n\r");
        }                                                                                                                                                                         

        s_read(IN, &in, i);

        /* code is added on 2/2/98 to print (quantity) and sp code */
        for(j = 0; j < 7; j++)
        {
            if(!in.po[j][0] || !in.jb[j][0])
            {
                other[j][0] = '\0';
                continue;
            }

            vec = s_getno(PO, 0, in.po[j]);
            if(vec == -1)
            {
                message(M_SCR|M_KEY, "Bad PO number found - hit any key to continue");
                continue;
            }
            s_read(PO, &po, vec);

            vec = s_getno(ES, 0, in.jb[j]);
            if(vec == -1)
            {
                message(M_SCR|M_KEY, "Bad Job number found - hit any key to continue");
                continue;
            }
            s_read(ES, &es, vec);

            plfile(&es);

            numPL = s_nrecs(PL);
            qty = 0;

            if(po.plidx <= 0) /* old style */
            {
                bg = 0;
                ed = numPL - 1;
            }
            else
            {
                set_rate(&es, po.vdcode, TRUE);
                get_pl_pos(po.plidx, po.vdcode, &bg, &ed);
            }

            for(k = 0; k < numPL; k++)
            {
                s_read(PL, &pl, k);
                if(!strcheck(po.vdcode, pl.vdcode) && k >= bg && k <= ed)
                {
                    qty += (INT) pl.qty;
                    sprintf(tempStr, "(%d) %s", qty, pl.pdcode);
                }
            }
//message(M_SCR|M_KEY, "bg=%d,ed=%d,qty=%d,code=%s", bg,ed,qty,pl.pdcode);
            strncpy(other[j], tempStr, 19);
            strcpy(spCode[j], es.spcode);
        }

        fprintf(fp, "| %-8.8s  |", in.inst_code);
        for(j = 0; j < 7; j++)
        {
            if(in.po[j][0])
                fprintf(fp, " PO #: %-8.8s      |", in.po[j]);
            else
                fprintf(fp, "                     |");
        }
        fprintf(fp, "\n\r");
        fprintf(fp, "|           |");
        for(j = 0; j < 7; j++)
        {
            if(in.jb[j][0])
                fprintf(fp, " JB #: %-8.8s  %-3.3s |", in.jb[j], spCode[j]);
            else
                fprintf(fp, "                     |");
        }
        fprintf(fp, "\n\r");
        fprintf(fp, "|           | %-19.19s | %-19.19s | %-19.19s | %-19.19s | %-19.19s | %-19.19s | %-19.19s |\n\r",
            in.name[0], in.name[1], in.name[2], in.name[3], in.name[4], in.name[5], in.name[6]);
        fprintf(fp, "|           |");
        for(j = 0; j < 7; j++)
        {
            if(other[j][0])
                fprintf(fp, " %-19.19s |", other[j]);
            else
                fprintf(fp, "                     |");
        }
        fprintf(fp, "\n\r");
        fprintf(fp, "-----------------------------------------------------------------------------------------------------------------------------------------------------------------------\n\r");
    }

    /* Turn off specifial mood */
    fprintf(fp, "%s", prnCodes.reset);
}

/*****************************************************************************
 SaveInstFile - Saves installation to a user specified file
*****************************************************************************/
VOID SaveInstFile(CHAR* orgFileName, CHAR* sourceFile)
{
    INT ans, tmpHandle;
    CHAR tmpName[60], destFile[128];

ag: if(savecheck())
    {
        strcpy(tmpName, orgFileName);
        inpt(1, 25, "Enter file name to save: ", tmpName, 50, CT_CHAR, NO);
        if(strstr(tmpName, ":") || strstr(tmpName, "\\"))
        /* user enters abosulate path, use it */
            strcpy(destFile, tmpName);
        else
            sprintf(destFile, "%s%s", JobInstPath, tmpName);

        if(access(destFile, 00) == 0) /* file exists */
        {
            ans = message(M_SCR|M_KEY, "File exists (%s), overwrite it? (y/N)", destFile);
            if(ans != 'Y')
                goto ag;
        }

        /* copy tmpFile over to orgFileName */
        if((tmpHandle = FileOpen(destFile)) != -1) /* make sure we can open file */
        {
            FileClose(tmpHandle);
            FileCopy(sourceFile, destFile);
        }
        else
            goto ag;
    }
}


/*****************************************************************************
 GenerateFromPO - Based on the search criteria given, generates the installation
                  info from PO and write to the file handle given.
                  Function returns the number of records found.
*****************************************************************************/
INT GenerateFromPO(CHAR* sdate, CHAR* vdcode, CHAR* spcode, CHAR* status, INT handle)
{
    INT i, j, n;
    SHORT count = 0;
    CHAR edate[9], tdate[9];
    POREC po;
    INREC in;
    BOOLEAN found;
    struct date tmpDate;

    message(M_SCR, "Working...");

    DateStr(&tmpDate, sdate);
    StrDate(edate, DateAdd(&tmpDate, 6));

    n = s_nrecs(PO);
    FileWrite(handle, &count, sizeof(SHORT)); /* size 2 */
    FileWrite(handle, sdate, 8); /* also save starting date */

    for(i = 0; i < n; i++)
    {
        s_read(PO, &po, i);
        found = FALSE;
        if(strdatecmp(po.shipdate,sdate)>=0 && strdatecmp(po.shipdate, edate)<=0) /* potential PO */
        {
            if(strncmp(vdcode, "IN", 2) == 0 && vdcode[2] == '*') /* case of IN* */
            {
                if(strncmp(po.vdcode, "IN", 2) == 0)
                    found = TRUE;
            }
            else if(!strcmp(vdcode, "*") || !strcmp(vdcode, "") || !strcmp(vdcode, " ")) /* any vendor */
                found = TRUE;
            else /* exact vendor */
            {
                if(strcmp(po.vdcode, vdcode) == 0)
                    found = TRUE;
            }
        }

        /* Look for SP as well */
        if(found && spcode[0] != '\0')
        {
            if(strcmp(spcode, "") && strcmp(spcode, " ") && strcmp(spcode, "*") && strcmp(spcode, po.spcode))
                found = FALSE;
        }

        /* Finally, look for PO status */
        if(found && status[0] != '*')
        {
            if(status[0] != toupper(po.status[0]))
                found = FALSE;
        }

        if(found) /* found a match */
        {
            /* figure out date index */
            for(j = 0; j < 7; j++)
            {
                DateStr(&tmpDate, sdate);
                StrDate(tdate, DateAdd(&tmpDate, j));
                if(strdatecmp(tdate, po.shipdate) == 0)
                    break;
            }
            if(j >= 7) /* impossible */
            {
                message(M_SCR|M_KEY, "Impossible: start date = %s, end date = %s, shipdate = %s", sdate, edate, po.shipdate);
                exit(-1);
            }

            found = FALSE;
            FileSeek(handle, (LONG)10, SEEK_SET);  /* put reader to the beginning */
            while(!eof(handle))
            {
                FileRead(handle, &in, sizeof(INREC));
                if(!strcmp(in.inst_code, po.vdcode) && in.po[j][0] == '\0')
                {
                    found = TRUE;
                    strcpy(in.po[j], po.po_number);
                    strcpy(in.jb[j], po.jbcode);
                    strcpy(in.name[j], po.jbname);

                    /* write it back */
                    FileSeek(handle, (LONG)(-1)*sizeof(INREC), SEEK_CUR);
                    FileWrite(handle, &in, sizeof(INREC));
                    break;
                }
            }
            if(!found) /* need to add a new record to the file */
            {
                memset(&in, 0, sizeof(INREC));
                strcpy(in.inst_code, po.vdcode);
                strcpy(in.po[j], po.po_number);
                strcpy(in.jb[j], po.jbcode);
                strcpy(in.name[j], po.jbname);

                FileWrite(handle, &in, sizeof(INREC));
                count++;
            }
        }
    }

    message(M_SCR, NULL);

    if(count > 0)
    {
        FileSeek(handle, (LONG)0, SEEK_SET);
        FileWrite(handle, &count, sizeof(SHORT));
    }

    return count;
}

/*****************************************************************************
 BatchPrintJobInstallation - Print weeks of job installation at once.      
*****************************************************************************/
INT BatchPrintJobInstallation(CHAR* fName, INT handle)
{
    INT pos = 0, key = KEY_DOWN, test, i, numOfWeeks;
    VDREC vd;
    SPREC sp;
    CHAR sDate[9], eDate[9];
    struct date sd, ed;
    CHAR drive[3], ext[4], name[60], path[60];


    sDate[0] = '\0';
    eDate[0] = '\0';
    strcpy(vd.code, "IN*");
    sp.code[0] = '\0';
    strcpy(Status, "R");

    store();
    setwindow(25, 11, 55, 17, "    Print Job Installation", "     Enter Match Criteria", "F1=HELP                F10=DONE", "");
    Print("\n\r Start Date: \n\r End Date: \n\r VD Code: \n\r SP Code: \n\r Status:");
    do {
        switch (key) {
            case KEY_SHFT_TAB:
            case KEY_UP:
                if (pos == 1)
                    pos = 6;
                pos--;
                break;

            case KEY_TAB2:
            case KEY_DOWN:
                if (pos == 5)
                    pos = 0;
                pos++;
                break;

            case KEY_CTRL_PGUP:
            case KEY_HOME:
                pos = 1;
                break;

            case KEY_CTRL_PGDOWN:
            case KEY_END:
                pos = 5;
                break;

            case KEY_F10:
                pos = 6;
                break;

            /* this never gets hit because inpt() will not return KEY_ESC */
            case KEY_ESC:
                return -1;

            default:
                if (pos == 5)
                    pos = 0;
                pos++;
                break;
        }

        switch (pos) {

            /*
            * input of "Start Date"
            */
            case 1:
                do {
                    key = inpt(14, 2, "", sDate, 8, CT_CHDATE, NO);
                    if(key == KEY_F1)
                        StrDate(sDate, &today);
                } while (key == KEY_F1);
                break;

            /*
            * input of "End Date"
            */
            case 2:
                do {
                    key = inpt(12, 3, "", eDate, 8, CT_CHDATE, NO);
                    if(key == KEY_F1)
                        StrDate(eDate, &today);
                } while (key == KEY_F1);
                break;

            /*
            * Pick Vendor code
            */
            case 3:
                do {
                    s_open(VD);
                    key = inpt(11, 4, "", vd.code, 8, VD, NO);
                    if (key == KEY_F1) {
                        if ((test = gethelp(VD, s_getno(VD, 0, vd.code), 9, 4, 2-1, 14-1)) != -1)
                            s_read(VD, &vd, test);
                    }
                    s_close(VD);
                } while (key == KEY_F1);
                break;


            /*
            * Pick SP code
            */
            case 4:
                do {
                    s_open(SP);
                    key = inpt(11, 5, "", sp.code, 3, SP, NO);
                    if (key == KEY_F1) {
                        if ((test = gethelp(SP, s_getno(SP, 0, sp.code), 9, 4, 2-1, 14-1)) != -1)
                            s_read(SP, &sp, test);
                    }
                    s_close(SP);

                } while (key == KEY_F1);
                break;

            /*
            *  choose "Status" to match
            */
            case 5:
                key = inpt(10, 6, "", Status, 1, CT_CHAR, NO);
                break;

            case 6:
                break;

            default:
                break;
        }
    } while (pos != 6);

    restore();

    strcpy(VendorCode, vd.code);
    strcpy(SPCode, sp.code);
    if(Status[0] == '\0' || Status[0] == ' ' || Status[0] == '*')
        Status[0] = '*';
    else
        Status[0] = toupper(Status[0]);
    Status[1] = '\0';

    if(strdatecmp(sDate, eDate) > 0)
    {
        message(M_SCR|M_KEY, "End Date must be more recent that Start Date - any key to continue");
        return -1;
    }

    /* Break the start and end date to be 7 days a chunk */
    DateStr(&sd, sDate);
    DateStr(&ed, eDate);
    numOfWeeks = (DiffDate(&sd, &ed)+1) / 7;
    if((DiffDate(&sd, &ed)+1) % 7 != 0)
        numOfWeeks++;

    if(!SelectPrinter() || printercheck() == -1)
        return -1;

    /* save IN path */
    strcpy(JobInstPath, db[IN].path);

    _splitpath(_fullpath(fName, fName, 60), drive, path, name, ext);
    drive[2] = '\0';
    strcpy(db[IN].path, drive);
    strcat(db[IN].path, path);
    strcpy(db[IN].fname, name);
    strcpy(db[IN].extension, "$$$");

    for(i = 0; i < numOfWeeks; i++)
    {
        StrDate(StartDate, &sd);
        DateAdd(&sd, 7);

        GenerateFromPO(StartDate, VendorCode, SPCode, Status, handle);
        FileClose(handle);

        s_open(IN);
        
        PrintInstallation(TRUE);
        s_close(IN);
        FileDelete(fName);

        if((handle = FileOpen(fName)) == -1)
        {
            message(M_SCR|M_KEY, "Unable to open temp file - hit any key to continue");
            strcpy(db[IN].path, JobInstPath);
            return -1;
        }
    }

    /* restore IN path */
    strcpy(db[IN].path, JobInstPath);

    return 0;
}
