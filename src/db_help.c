/**
*   DB_HELP.C   -
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
#include <string.h>
#include <dos.h>
#include <stdio.h>
#include <conio.h>
#include "db_defs.h"
#include "vmisc.h"

STATIC BOOLEAN inHelp;
STATIC BOOLEAN inHelpEdit;
extern MOUSE_CLICK *points;
extern BOOLEAN changeflg;

STATIC CHAR *helpMessage[] = {

            "Text Editor",  /* 40 */
            "Print Menu",
            "Main Menu",
            "Report Menu",
            "Info Menu",
            "System Menu",
            "File Menu",
            "System Setup",
            "Help Editor",
            "Markup Window",
#ifdef DEMO
            "Quotation",  /* 50 */
            "Part List",
            "Quotation",
            "Main Menu",
            "Main Menu",
            "Main Menu",  /* 55 */
            "Main Menu",
            "---------",
            "---------",
            "---------",
#endif
            "Rep Main Menu", /* 60 */
            };

/***************************************************************************
helpinpt - queries user for a value -- if valid then returns it.  If F1 then
a help window chooses the record.
    type:   ident
    rec:    record number (used for window)
    prompt: prompt for query
    dat:    return record
***************************************************************************/
INT helpinpt(DB_TYPE type, INT rec, CHAR *prompt, VOID *dat)
{
    CHAR temp2[30];
    INT inkey, test;

    message(M_SCR, NULL);
    *temp2 = '\0';
    do {
        inkey = inpt(1, 25, prompt, temp2, 30, CT_CHAR, NO);
        message(M_SCR, NULL);
        if (inkey == KEY_F10) {
            *temp2 = 0;
            return(-1);
        }
        if ((*temp2 == 0) && (inkey != KEY_F1)) {
            return(-4);
        }

        if (inkey == 1) {
            if (*temp2 == 0)
                return(-1);
            if ((test = s_getno(type, 0, temp2)) == -1)
                message(M_SCR|M_KEY, "Error: not defined - hit any key");
            else {
                if (dat != NULL)
                    s_read(type, dat, test);
                return(test);
            }
        }
    } while (inkey != KEY_F1);

    test = gethelp(type, rec, db[type].s->num, db[type].s->fld, db[type].s->x-1, db[type].s->y-1);

    if (test !=-1) {

        if (dat != NULL)
            s_read(type, dat, test);
    }

    return(test);
}

/***************************************************************************
rechelp - calls help window, returns record in dat.
    type:   ident
    rec:    record number
    dat:    pointer to record space
***************************************************************************/
INT rechelp(DB_TYPE type, INT rec, CHAR *dat)
{
    INT test;

    if ((test = gethelp(type, rec, db[type].s->num, db[type].s->fld, db[type].s->x, db[type].s->y)) != -1) {
        if (dat != NULL)
            s_read(type, dat, test);
    }

    return(test);
}

/***************************************************************************
gethelp - creates a scrollable window - allows user to choose a record.
    type:   ident
    rec:    record number
    num:    number of records in helpwindow
    flds:   number of fields in helpwindow
    x,y:    upper left corner of helpwindow
***************************************************************************/
INT gethelp(DB_TYPE type, INT rec, INT num, INT flds, INT x, INT y)
{
    CHAR htitle[81], buf[4];
    INT endflg=NO, inkey, lastrec;

    /* can't get help on an invalid DB_TYPE */
    Assert(type <= DB_END);

    if (type > DB_END) {
        message(M_SCR|M_ERR|M_KEY, "%u is not a valid DB_TYPE - hit any key", type);
        return(-1);
    }

    if (!s_nrecs(type)) {
        message(M_SCR|M_KEY, "No records in %s file - hit any key", db[type].title);
        return(-1);
    }
    if (rec < 0)
        rec = 0;

    store();
    sprintf(htitle, "HELP SCREEN: %s", db[type].title); /*M1*/

    setwindow(x+1, y, 79, y+num+1, htitle, "Home End PgUp PgDn \x18 \x19", "F10 to escape", NULL);
    textattr(YELLOW|ON_BLUE);
    clrscr();

    do {
        helpset(type, rec, num, flds, 2, 2);

        switch (inkey = nextkey()) {

            case KEY_CTRL_PGUP:
            case KEY_HOME:
                rec = 0;
                break;

/*          case KEY_SPACE:
                tagged[rec] = (tagged[rec] ? NO:YES);
                inkey = KEY_DOWN;    */

            case KEY_CTRL_PGDOWN:
            case KEY_END:
                rec = s_nrecs(type) - 1;
                /* fall thru */

            case KEY_DOWN:
                if (rec < s_nrecs(type)-1)
                    rec++;
                break;

            case KEY_SHFT_TAB:
            case KEY_UP:
                if (rec)
                    rec--;
                break;

            case KEY_PGDOWN:
                rec += num;
                if (rec >= s_nrecs(type))
                    rec = s_nrecs(type)-1;
                break;

            case KEY_PGUP:
                rec -= num;
                if (rec < 0)
                    rec = 0;
                break;

            case KEY_ENTER:
                endflg = YES;
                break;

            case KEY_ESC:
            case KEY_F10:
                endflg = YES;
                rec = -1;
                break;

            default:
                buf[0] = inkey;
                buf[1] = '*';
                buf[2] = '\0';
                lastrec = rec;
                if ((rec = getno_mess(type, 0, buf)) == -1)
                    rec = lastrec;
                message(M_SCR, NULL);
                break;
        }
    } while (!endflg);
    
    restore();
    changeflg = YES;
    return(rec);
}

/***************************************************************************
helpset - updates help window after cursor movement.
    type:   ident
    rec:    record number
    num:    number of records in helpwindow
    flds:   number of fields in helpwindow
    x,y:    upper left corner of helpwindow
***************************************************************************/
VOID helpset(DB_TYPE type, INT rec, INT num, INT flds, INT x, INT y)
{
    INT i, j, currec, newx, lastx=-1, fieldi, test, ccnt, cx;
    CHAR *dat;

    dat = (CHAR *)s_alloc(db[type].fsize);

    currec = rec-num/2;
    for (i=currec; i<currec+num; i++) {
        gotoxy(x, y++);
        textattr((i==rec) ? REVERSE : YELLOW|ON_BLUE);
        if ((i < s_nrecs(type)) && (i >= 0)) {
            s_read(type, dat, i);
        } else {
            memset(dat, 0, db[type].fsize);
        }
        lastx = -1;
        for (fieldi=0; fieldi<flds; fieldi++) {
            test = 1000;
            for (j=0; j<db[type].fields;j++) {
                newx = db[type].f[j].x;
                if ((newx&&newx>lastx) && (newx-lastx<test)) {
                    test = newx-lastx;
                    cx = newx;
                    ccnt = j;
                }
            }
            lastx = cx;
            fldprint(type, dat, ccnt);
        }
    }
    textattr(NORMAL);
    s_free(dat);
}


/***************************************************************************
helpmessage - Context Sensative Help Module
***************************************************************************/
INT helpmessage(INT type)
{
    INT currec=0, n, inc, last=-2;
    CHAR *mes1;
    MOUSE_CLICK *lastP;

    STATIC MOUSE_CLICK pHelp[] = {
        {120, 176, 168, 176, KEY_F1},  {280, 176, 336, 176, KEY_F5},
        {448, 176, 504, 176, KEY_F10}, {0, 0, 0, 0, 0}
    };

    mes1 = (CHAR *)s_alloc(50);

    if ((type < 0) || (type > HLP_END))
        message(M_SCR|M_KEY, "No help available for this field - hit any key");
    else {
        sprintf(mes1, "               Help for %s", ((type <= DB_END) ? db[type].title : helpMessage[type-40]));

        store();
        SetCursor(CUR_OFF);
        setwindow(16, 5, 64, 22, mes1, "Home End PgUp PgDn \x18 \x19", "F1=HELP             F5=PRINT             F10=EXIT", "");
        sprintf(db[HL].extension, "%02.2d", type);
        s_open(HL);
        n = s_nrecs(HL);
        put_help(currec);

        /* save last mouse click points */
        lastP = points;

        do {
            /* register new mouse click points */
            points = pHelp;

            switch (inc = nextkey()) {

                /*
                * Help Editor
                */
                case KEY_CTRL_F1:
                    HelpEditText(type);
                    break;

                /*
                * Help on Help
                */
                case KEY_F1:
                    if (!inHelp) {
                        inHelp = TRUE;
                        s_close(HL);
                        helpmessage(HL);
                        sprintf(db[HL].extension, "%02.2d", type);
                        s_open(HL);
                        inHelp = FALSE;
                    }
                    break;

                /*
                * Print Help text
                */
                case KEY_F5:
                    print_whole_file(HL, NULL, NULL);
                    break;

                case KEY_CTRL_PGUP:
                case KEY_HOME:
                    currec = 0;
                    break;

                case KEY_CTRL_PGDOWN:
                case KEY_END:
                    currec = n - 16;
                    break;

                case KEY_PGDOWN:
                    if (currec>=n-(2*17)) {
                        currec = n - 16;
                    } else
                        currec += 17;
                    break;

                case KEY_PGUP:
                    if (currec<2*17) {
                        currec = 0;
                    } else {
                        currec -= 17;
                    }
                    break;

                case KEY_DOWN:
                    if (currec == n-16)
                        break;
                    currec++;
                    break;

                case KEY_UP:
                    if (currec == 0)
                        break;
                    currec--;
                    break;

                default:
                    break;
            }

            if (last != currec) {
                put_help(currec);
                last = currec;
            }
        } while ((inc != KEY_F10) && (inc != KEY_ESC));

        /* restore last mouse click locations */
        points = lastP;

        s_close(HL);
        restore();
    }
    s_free(mes1);
    return(0);
}

VOID put_help(INT currec)
{
    INT i;
    HLREC hl;

    textattr(WHITE|ON_BLUE);
    for (i=currec; i<=currec+16; i++) {
        s_read(HL, &hl, i);
        printxy(2, i-currec+1, "%-47s", hl.text);
    }
}

/*************************************************************************
HelpEditText - Input Window for HL (Help)
    input: type = Which Help Module to Edit
**************************************************************************/
INT HelpEditText(INT type)
{
    CHAR temp2[50], ptr[50], old[4];
    IN_POS c;
    INT i, currec=0, n, cflag=0;
    BOOLEAN endflg, changes;
    MOUSE_CLICK *lastP;
    STATIC MOUSE_CLICK pHelpEdit[] = {
        {112, 152, 168, 152, KEY_F1},  {200, 152, 248, 152, KEY_F2},
        {280, 152, 328, 152, KEY_F3},  {360, 152, 416, 152, KEY_F5},
        {456, 152, 512, 152, KEY_F10}, {0, 0, 0, 0, 0}
    };

    /*
    * don't allow nested help edits
    */
    if (!inHelpEdit) {
        inHelpEdit = TRUE;

        /* Inits */
        endflg = NO;
        changes = changeflg;

        if (!seccheck(HL,'F') || (type == -1))
            return(-1);

        sprintf(db[HL].extension, "%02.2d", type);
        store();

        sprintf(ptr, "Input for %s Help", ((type <= DB_END) ? db[type].title : helpMessage[type-40]));

        setwindow(15, 10, 65, 19, "                Help Input Screen", ptr, "F1=HELP    F2=ADD    F3=DEL    F5=PRINT    F10=EXIT", db[HL].title);
        s_open(HL);
        put_win2(ptr, currec-1);
        c.x = c.last = c.top = 0;
        c.ypos = 1;

        /* save last mouse click points */
        lastP = points;

        do {

            /* register new mouse click points */
            points = pHelpEdit;

            cflag = 0;
            n = s_nrecs(HL);

            switch (c.x) {

                case 0:
                    c.last = 0;
                    strcpy(temp2, ptr+db[HL].f[0].offset);
                    c.x = inpt(2, c.ypos, "", ptr+db[HL].f[0].offset, db[HL].f[0].width+1, db[HL].f[0].type, NO);
                    if (strcmp(temp2, ptr+db[HL].f[0].offset))
                        cflag=1;
                    break;

                /*
                * Help on editing help
                */
                case KEY_F1:
                    c.x = c.last;
                    strcpy(old, db[HL].extension); /*M1*/
                    helpmessage(HLP_HELPEDIT);
                    strcpy(db[HL].extension, old);
                    break;

                /*
                * Insert a Blank Record
                */
                case KEY_F2:
                    c.x = c.last;
                    if (currec<n) {
                        for (i=n-1; i>=currec; i--) {
                            s_read(HL, ptr, i);
                            s_recwrite(HL, ptr, i+1);
                        }
                        s_modnrecs(HL, ++n);
                        s_clearrec(HL, ptr);
                        s_recwrite(HL, ptr, currec);
                        put_win2(ptr, currec-c.ypos);
                    }
                    break;

                /*
                * Delete a Record
                */
                case KEY_F3:
                    c.x = c.last;
                    if (currec != n) {
                        if (message(M_SCR|M_KEY, "Are you sure you want to Delete this record? (y/N)") == 'Y') {
                            s_write(HL, ptr, currec, TRUE);
                            if (currec == 0) {
                                c.x = KEY_HOME;
                                break;
                            }
                            put_win2(ptr, currec-c.ypos);
                        }
                    }
                    break;

                /*
                * Print Help text
                */
                case KEY_F5:
                    c.x = 0;
                    print_whole_file(HL, NULL, NULL);
                    break;

                case KEY_F10:
                    endflg=YES;
                    break;

                /*
                * Go to first Record
                */
                case KEY_CTRL_PGUP:
                case KEY_HOME:
                    c.x = c.last;
                    if (currec!=1) {
                        c.ypos = 1;
                        currec = 0;
                        put_win2(ptr, currec-c.ypos);
                    }
                    break;

                /*
                * Go to last Record
                */
                case KEY_CTRL_PGDOWN:
                case KEY_END:
                    c.x = c.last;
                    if (currec != n) {
                        c.ypos = 9;
                        currec = n;
                        put_win2(ptr, currec-c.ypos);
                    }
                    break;

                /*
                * Page Down 9 records
                */
                case KEY_PGDOWN:
                    if (currec >= n-18) {
                        c.x = KEY_END;
                        break;
                    }
                    currec += 9;
                    put_win2(ptr, currec-c.ypos);
                    c.x = c.last;
                    break;

                /*
                * Page Up 9 records
                */
                case KEY_PGUP:
                    if (currec < 18) {
                        c.x = KEY_HOME;
                        break;
                    }

                    currec -= 9;
                    put_win2(ptr, currec-c.ypos);
                    c.x = c.last;
                    break;

                /*
                * Move Down 1 record
                */
                case KEY_DOWN:
                case KEY_TAB2:
                case KEY_RIGHT:
                    c.x = c.last;
                    if (currec == n)
                        break;
                    if (c.ypos==9) {
                        for (i=7;i>=-1;i--) {
                            s_read(HL,ptr,currec-i);
                            printxy(2,c.ypos-i-1,db[HL].f[0].format,ptr+db[HL].f[0].offset);
                        }
                    } else
                        c.ypos++;
                    currec++;
                    break;

                /*
                * Move Up 1 record
                */
                case KEY_UP:
                case KEY_SHFT_TAB:
                case KEY_LEFT:
                    c.x=c.last;
                    if (currec==0)
                        break;
                    if (c.ypos==1) {
                        for (i=7;i>=-1;i--) {
                            s_read(HL,ptr,currec+i);
                            printxy(2,i+2,db[HL].f[0].format,ptr+db[HL].f[0].offset);
                        }
                    } else
                        c.ypos--;
                    currec--;
                    break;

                default:
                    c.last=c.x;
                    c.x=(c.x==0 ? 1:0);
                    break;
            }

            if (cflag) {
                if (message(M_SCR|M_KEY, "Do you want to save your changes? (y/N)") == 'Y') {
                    s_write(HL, ptr, currec, FALSE);
                    if (currec == n) {
                        put_win2(ptr, currec-(c.ypos-1));
                        c.x = KEY_END;
                        c.ypos = 8;
                    } else if (c.last == 0) {
                        put_win2(ptr, currec-c.ypos);
                    }
                } else {
                    s_read(HL, ptr, currec);
                    printxy(2, c.ypos, db[HL].f[0].format, ptr+db[HL].f[0].offset);
                }
            } else {
                s_read(HL, ptr, currec);
                printxy(2, c.ypos, db[HL].f[0].format, ptr+db[HL].f[0].offset);
            }
        } while (!endflg);

        /* restore mouse click points */
        points = lastP;

        s_close(HL);
        restore();
        changeflg = changes;
        inHelpEdit = FALSE;
    }
    return(0);
}

/*****************************************************************************
put_win2 - Used for winhelp() only
*****************************************************************************/
VOID put_win2(CHAR *ptr, INT l)
{
    INT i;

    for (i=9; i>=1; i--) {
        s_read(HL, ptr, i+l);
        printxy(2, i, db[HL].f[0].format, ptr+db[HL].f[0].offset);
    }
}
