/**
*   OUTPUT.C    -
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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <conio.h>
#include "db_defs.h"
#include "vmouse.h"
#include "vmisc.h"

extern struct date today;
extern SYSREC sysvar;
extern BOOLEAN mouseActive;

/******************************************************************************
printfile - generic db file printing routine
    fl:     output file
    type:   ident
    total:  records to print
******************************************************************************/
INT printfile(FILE *fl, DB_TYPE type, INT total)
{
    struct text_info ti;
    FIELDPARAMS attr;
    CHAR tmp[80], fname[20], out[80];
    INT i, j;
    INT record, pagelength=50, conflg=NO, lastx, ccnt, test;
    VOID *rec;
    CHAR *temp;
    SHORT *iout;
    INT newx, cx, k;
    struct date *dout;
    LONG *lout;
    FLOAT *fout;
    BOOLEAN closeflg;

    /* Inits */
    closeflg = NO;

    /* check if records exist */
    if (!total) {
        message(M_SCR|M_KEY, "No records exist - press a key");

    } else {

        gettextinfo(&ti);

        /*
        * get print destination
        */
        if (fl == NULL) {
            fl = printdest(fname);
            closeflg = YES;
        } else {
            strcpy(fname, "NONE");
        }

        if (fl == NULL)
            return(-1);

        /*
        * if output is printer check status
        */
        if (!strcmp(fname, "PRN")) {
            if ((printercheck() == -1)) {
                filecls(fl);
                return(-1);
            }

        /*
        * if output is console then set up parameters
        */
        } else if (!strcmp(fname, "CON")) {
            pagelength = 14;
            conflg = YES;
            window(1, 1, 80, 24);
            clrscr();
            printtitle(fl, db[type].title);
            printheader(fl, type, fname);
            window(1, 10, 80, 24);
        }

        /* allocate space for record */
        rec = s_alloc(db[type].fsize);

        /*
        * print each record in file
        */
        for (record=0; record<total; record++) {
            /*check if new page*/
            if ((record % pagelength) == 0) {

                /*
                * if printer then form feed and print header
                */
                if (!conflg) {
                    if (record)
                        fprintf(fl, "%c", 12);
                    printtitle(fl, db[type].title);
                    printheader(fl, type, fname);

                /*
                * otherwise pause for user
                */
                } else if (record) {
                    fflush(fl);
                    if (message(M_SCR|M_KEY, "Press a key") == KEY_ESC) {
                        if (closeflg)
                            filecls(fl);

                        scrrestore(&ti);

                        return(-1);
                    }
                    clrscr();
                    conflg = 2;
                }
            }

            /*get next record*/
            s_read(type, rec, record);
            if (conflg == 2)
                conflg = YES;
            else if (!conflg||record)
                fprintf(fl, "\n\r");
            j = 1;

            lastx = -1;

            /* for each field in record */
            for (i=0; i < db[type].fields; i++) {
                test=1000;
                for (k=0; k < db[type].fields; k++) {
                    newx=db[type].f[k].x;
                    if (newx && newx>lastx && newx-lastx<test) {
                        test = newx - lastx;
                        cx = newx;
                        ccnt = k;
                    }
                }

                if ((lastx == cx) || (test == 1000))
                    attr.x = 0;
                else {
                    lastx = cx;
                    attr = db[type].f[ccnt];
                }

                if (attr.x) {

                    for (;j<attr.x;fprintf(fl, " "),j++)
                        ;
                    temp = (CHAR *)rec;
                    temp += attr.offset;
                    dout = (struct date *)temp; lout = (LONG *)temp; fout = (FLOAT *)temp; iout = (SHORT *)temp;
                    memcpy(out,temp,attr.bytes);
                    switch(attr.type) {

                        case CT_LONG:
                            sprintf(tmp, attr.format, (long)*lout);
                            break;

                        case CT_INT:
                            sprintf(tmp, attr.format, *iout);
                            break;

                        case CT_FLOAT:
                        case CT_FLOAT3:
                            sprintf(tmp, attr.format, *fout);
                            break;

                        case CT_DATE:
                            /*sprintf(tmp, "%d/%d/%d", dout->da_mon, dout->da_day, dout->da_year%100);*/
                            StrDate(tmp, dout);
                            break;

                        default:
                            sprintf(tmp, attr.format, out);
                            break;
                    }

                    fprintf(fl, "%s", tmp);
                    j += strlen(tmp);
                    if (esccheck(NULL) == YES) {
                        record = total;
                        break;
                    }
                }
            }
        }

        fprintf(fl, "\n\r");
        for (i=0; i<80; fputc('=',fl),i++)
            ;
        if (!conflg)
            fprintf(fl, "\n\r");
        fflush(fl);
        if (closeflg) {
            if (!conflg)
                fprintf(fl, "%c", 12);
            filecls(fl);
        }
        s_free(rec);
        if (conflg)
            message(M_SCR|M_KEY, "Press a key");

        scrrestore(&ti);
    }
    return(0);
}

/******************************************************************************
printdest - user window to choose file destination: CON,PRN,FAX or filename
******************************************************************************/
FILE *printdest(CHAR *fname)
{
    INT choice;
    FILE *fout;
    CHAR *output[] = {
        "SCREEN",
        "PRINTER",
        "FILE",
        "FAX",
        NULL
    };

    strcpy(fname, "TEMP.PRN");
    fout = NULL;

    store();
    choice = window_chooser(32, 10, output);
    restore();

    switch (choice) {

        case -1:
            return(NULL);

        case 0:
            strcpy(fname, "CON");
            break;

        case 1:
            if (printercheck() != -1)
                strcpy(fname, "PRN");
            else {
                return(NULL);
            }
            break;

        case 2:
            store();
            window(1, 1, 80, 25);
            if (inpt(1, 25, "Enter filename: ", fname, 12, CT_CHAR, NO) != 1)
                return(NULL);
            message(M_SCR, NULL);
            restore();
            break;

        case 3:
            sprintf(fname, "FAX.%03d", random_n());
            break;

        /*
        * It is a bug if this is ever executed
        */
        default:
            Assert(choice == 0);
            break;

    }

    if ((fout=fileopn(fname, "wt")) == NULL)
        message(M_SCR|M_ERR|M_KEY, "Unable to open %s", fname);

    return(fout);
}

/******************************************************************************
printtitle - prints title to output.
******************************************************************************/
VOID printtitle(FILE *fl, CHAR *ttl)
{
    INT i, length;
    CHAR tempdate[10];

    fprintf(fl, "\n\r");
    length = strlen(sysvar.company);
    for (i=0; i<40-length/2-length%2; fputc(' ',fl),i++)
        ;
    fprintf(fl, "%s\n\r", sysvar.company);
    length = strlen(ttl);
    for (i=0; i<40-length/2-length%2; fputc(' ',fl),i++)
        ;
    fprintf(fl, "%s\n\r", ttl);
    StrDate(tempdate, &today);
    length = strlen(tempdate);
    for (i=0;i<40-length/2-length%2;fputc(' ',fl),i++)
        ;
    fprintf(fl, "%s\n\r\n\r", tempdate);
}

/******************************************************************************
printheader - creates and prints header from db structure.
******************************************************************************/
VOID printheader(FILE *fl, DB_TYPE type, CHAR *fname)
{
    CHAR *dbl;
    FIELDPARAMS attr;
    INT i, j=1, lastx, test, k, newx, cx, ccnt;
    BOOLEAN conflg;

    conflg = NO;
    if (!strcmp(fname, "CON"))
        conflg = YES;

    dbl = s_alloc(82);

    /*setstr(dbl, '=', 81);*/
    memset(dbl, '=', 81);
    *(dbl+81) = '\0';

    fprintf(fl, "%s", dbl);
    if (!conflg)
        fprintf(fl, "\n\r");
    fflush(fl);
    lastx = -1;
    for (i=0; i<db[type].fields; i++) {

        test = 1000;
        for (k=0; k < db[type].fields; k++) {

            newx = db[type].f[k].x;
            if (newx && newx > lastx && newx-lastx < test) {

                test = newx-lastx;
                cx = newx;
                ccnt = k;
            }
        }

        if ((lastx == cx) || (test == 1000)) {
            attr.x = 0;
        } else {
            lastx = cx;
            attr = db[type].f[ccnt];
        }

        if (attr.x) {
            for (; j<attr.x; fputc(' ',fl),j++)
                ;
            j += fheadprintf(fl, KEY_LEFT, attr.header);
            fflush(fl);
        }
    }

    if (!conflg || j<=80)
        fprintf(fl, "\n\r");
    j = 1;
    lastx = -1;
    for (i=0; i<db[type].fields; i++) {

        test = 1000;
        for (k=0; k<db[type].fields; k++) {
            newx = db[type].f[k].x;

            if (newx && newx > lastx && newx-lastx < test) {
                test = newx-lastx;
                cx = newx;
                ccnt = k;
            }
        }

        if ((lastx == cx) || (test == 1000))
            attr.x = 0;
        else {
            lastx = cx;
            attr = db[type].f[ccnt];
        }

        if (attr.x) {
            for (; j<attr.x; fputc(' ',fl),j++)
                ;
            j += fheadprintf(fl, KEY_RIGHT, attr.header);
            fflush(fl);
        }
    }

    if (!conflg || j<80)
        fprintf(fl, "\n\r");
    fprintf(fl, "%s", dbl);
    s_free(dbl);
    fflush(fl);
}

/******************************************************************************
fheadprintf - prints left or right part of header string.
******************************************************************************/
INT fheadprintf(FILE *fl, INT lr, CHAR *head)
{
    INT rc;
    CHAR *string, *c;

    string = strdup(head);
    c = strchr(string, '|');

    if (lr == KEY_LEFT) {
        *c = '\0';
        c = string;
    } else {
        c++;
    }

    fprintf(fl, "%s", c);
    rc = strlen(c);
    s_free(string);

    return(rc);
}

/******************************************************************************
fmt_to_string - turns a numeric printf format (e.g. "%-6ld") into the string
format of the same width ("%-6s") so that blanks can be printed instead of a
zero.  Borland's printf ignored the 'l' in "%ls"; modern libraries treat it
as a wide string, so the length modifier has to go as well.
******************************************************************************/
STATIC VOID fmt_to_string(CHAR *format, CHAR conv)
{
    CHAR *src, *dst;

    for (src = dst = format; *src != '\0'; src++) {
        if ((*src == 'l' || *src == 'h') && (src[1] == conv || src[1] == 'l' || src[1] == 'h'))
            continue;
        *dst++ = (*src == conv) ? 's' : *src;
    }
    *dst = '\0';
}

/******************************************************************************
bcprintf - floating point: prints blanks if 0 else prints number.
******************************************************************************/
VOID bcprintf(CHAR *fmt, FLOAT number)
{
    CHAR format[50];

    strcpy(format,fmt);
    if (number < 0.006) {
        fmt_to_string(format, 'f');
        Print(format, " ");
    } else {
        Print(format, number);
    }
}

/******************************************************************************
biprintf - integer: prints blanks if number = 0 else prints number to file.
******************************************************************************/
VOID biprintf(FILE *output, CHAR *fmt, INT number)
{
    CHAR format[50];

    strcpy(format,fmt);
    if (number<=0) {
        fmt_to_string(format, 'd');
        fprintf(output, format, " ");
    } else {
        fprintf(output,format,number);
    }
}

/******************************************************************************
ciprintf - same as biprintf except to console.
******************************************************************************/
VOID ciprintf(CHAR *fmt, INT number)
{
    CHAR format[50];

    strcpy(format, fmt);
    if (!number) {
        fmt_to_string(format, 'd');
        Print(format, " ");
    } else {
        Print(format, number);
    }
}


/******************************************************************************
printxy - prints string to given coordinates.
******************************************************************************/
VOID printxy(INT x, INT y, const CHAR *format, ...)
{
    va_list argptr;
    CHAR out[120];

    /*
    * build the output text from the string format
    */
    va_start(argptr, format);
    vsprintf(out, format, argptr);
    va_end(argptr);

    /* check out length */
    if(strlen(out) >= 120)
        out[119] = '\0';

    /* output the text */
    gotoxy(x, y);

    /*
    * turn off mouse cursor when printing to screen
    */
    if (mouseActive)
        MouseHideCursor();

    cprintf("%s", out);

    /*
    * turn mouse cursor back on
    */
    if (mouseActive)
        MouseShowCursor();
}

/******************************************************************************
goput - puts a character at given coordinates.
******************************************************************************/
VOID goput(INT x, INT y, CHAR ch)
{
    gotoxy(x, y);
    putch(ch);
}

/****************************************************************************
fldprint - prints specified field of given record.
    type:   ident
    dat:    pointer to record
    fld:    field number
****************************************************************************/
VOID fldprint(DB_TYPE type, CHAR *dat, INT fld)
{
    CHAR *ptr, tempStr[9];

    ptr = dat + db[type].f[fld].offset;

    switch (db[type].f[fld].type) {

        case CT_FLOAT3:
        case CT_FLOAT:
            bcprintf(db[type].f[fld].format,*(FLOAT *)ptr);
            break;

        case CT_INT:
            ciprintf(db[type].f[fld].format,*(SHORT *)ptr);
            break;

        case CT_LONG:
            Print(db[type].f[fld].format,(long)*(LONG *)ptr);
            break;

        case CT_DATE:
            StrDate(tempStr, (struct date *)ptr);
            ptr = tempStr;

        default:
            Print(db[type].f[fld].format, ptr);
            break;
    }
    Print(" ");
}


/*****************************************************************
INT message(UBYTE, const CHAR *, ...) - Displays message at (1,25)
*****************************************************************/
INT message(UBYTE flag, const CHAR *format, ...)
{
    CHAR out[160];
    time_t t;
    struct text_info ti;
    va_list argptr;
    INT ret, inkey;

    /* inits */
    out[0] = '\0';

    /* store screen info */
    gettextinfo(&ti);

    /* setup attribs for the message bar */
    window(1, 1, 80, 25);
    gotoxy(1, 25);

    /*
    * don't parse the variable input if there aren't any
    */
    if (format != NULL) {

        va_start(argptr, format);
        vsprintf(out, format, argptr);
        va_end(argptr);
    }

    /* make sure we never blow the stack */
    Assert(strlen(out) < 140);

    /* set color of text for message bar */
    textattr((flag & M_ERR) ? RED|ON_CYAN : BLUE|ON_CYAN);

    /*
    * output the message with date and time to an error log file
    */
    if (flag & M_ERR) {

        time(&t);
        ErrorLog("AUTOMAT.ERR", "\n%s%s\n", ctime(&t), out);
    }

    /*
    * write message to message bar
    */
    if (flag & M_SCR) {

        out[79] = '\0';
        Print("%-79s", out);
    }

    ret = inkey = 0;

    /*
    * get keystroke from user
    */
    if (flag & M_KEY) {

        ret = toupper((inkey=nextkey())&0xff);

        /* clear the message bar after getting the key */
        gotoxy(1, 25);
        clreol();
    }

    /* restore the screen info */
    scrrestore(&ti);

    /* return the key that the user pressed */
    return((ret) ? ret:inkey);
}

VOID Print(const CHAR *format, ...)
{
    va_list argptr;
    CHAR buf[140];

    /* make sure format is not NULL */
    Assert(format != NULL);

    /*
    * build the output string
    */
    va_start(argptr, format);
    vsprintf(buf, format, argptr);
    va_end(argptr);

    /* make sure we never blow the stack */
    Assert(strlen(buf) < 140);

    /*
    * turn off mouse cursor when printing to screen
    */
    if (mouseActive)
        MouseHideCursor();

    /* print the output string */
    cprintf(buf);

    /*
    * turn mouse cursor back on
    */
    if (mouseActive)
        MouseShowCursor();
}
