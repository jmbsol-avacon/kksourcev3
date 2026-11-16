/**
*   CHECK.C     -
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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include "db_defs.h"
#include "vmisc.h"

extern SYSREC sysvar;

/****************************************************************************
savecheck - New SAVECHECK Function. Returns 1 or 0
****************************************************************************/
BOOLEAN savecheck(VOID)
{
    CHAR response;

    response = message(M_SCR|M_KEY, "Do you want to save your changes? (Y/n)");
    if (response == 'N' || response == KEY_ESC1)
        return(NO);
    else
        return(YES);
}

/****************************************************************************
datecheck - checks if string is valid date format: mm/dd/yy.  The date
is returned as date structure.  If invalid then the returned year is 0.
****************************************************************************/
struct date datecheck(CHAR *indate)
{
    INT count,ptr;
    struct date dtemp;
    CHAR ctemp[5];

    dtemp.da_year=0;
    StrReplaceChar(indate, '-', '/');

    for (count=0,ptr=0; indate[ptr]!='/'&&count!=5; count++,ptr++)
        ctemp[count]=indate[ptr];

    if (count==5)
        return(dtemp);

    ctemp[count]=0;
    if ((dtemp.da_mon=atoi(ctemp))<=0||dtemp.da_mon>12)
        return(dtemp);

    for (count=0,ptr++; indate[ptr]!='/'&&count!=5; count++,ptr++)
        ctemp[count]=indate[ptr];

    if (count==5)
        return(dtemp);

    ctemp[count]=0;
    if ((dtemp.da_day=atoi(ctemp))<=0||dtemp.da_day>31)
        return(dtemp);

    for (count=0,ptr++; indate[ptr]!=0&&count!=5; count++,ptr++)
        ctemp[count]=indate[ptr];
    ctemp[count]=0;

//    if (count==5||atoi(ctemp)==0)
    if (count==5)
        return(dtemp);
    if ((dtemp.da_year=1900+atoi(ctemp))<=1900)
    {
        if(strncmp(ctemp, "00", 2) == 0 || strncmp(ctemp, "2000", 4) == 0)
            dtemp.da_year = 2000;
        else
            dtemp.da_year=0;
    }
    else if (dtemp.da_year<1980)
        dtemp.da_year+=100;

    return(dtemp);
}

/****************************************************************************
esccheck - checks if Escape key is pressed - if so then asks if user wants to
abort, else ignores key.
****************************************************************************/
INT esccheck(CHAR *mess)
{
    USHORT key;
    INT x, y;
    BOOLEAN esc;

    x = wherex();
    y = wherey();

    esc = NO;
    if (KeyPress(&key) && (key == KEY_ESC)) {
        if (message(M_SCR|M_KEY, "Are you sure you want to abort (y/N)?") == 'Y')
            esc = YES;
        if (mess != NULL)
            message(M_SCR, mess);
    }
    gotoxy(x, y);

    return(esc);
}

/****************************************************************************
strcheck - like stricmp except takes wildcard value of '*' and '?' on str1.
****************************************************************************/
INT strcheck(const CHAR *str1, CHAR *str2)
{
    INT count;
    CHAR c1, c2;

    if (str1[0] != 0) {
        for (count=0; count<=80; count++) {
            c1 = toupper(str1[count]);
            c2 = toupper(str2[count]);
            if ((c1 == '*') || (!c1 && !c2))
                break;

            if ((c1 != '?') && (c1 != '_') && (c1 != '|') && (c1 != c2))
                return((INT)c1 - c2);
        }
    }
    return(0);
}

/****************************************************************************
printercheck - checks if printer is ready.
****************************************************************************/
INT printercheck(VOID)
{
    INT test, err;

    do {
        err=0;
        test = biosprint(2, 0, sysvar.pport);
        if (test&0x01)
            err = 1;
        if (test&0x08)
            err = 2;
        if (!(test&0x10))
            err = 3;
        if (test&0x20)
            err = 4;

        switch(err) {

            /*
            * Printer is OK
            */
            case 0:
                return(0);

            case 1:
                test = message(M_SCR|M_KEY, "Printer time out - try again? (Y/n)");
                break;

            case 2:
                test = message(M_SCR|M_KEY, "Printer I/O error - try again? (Y/n)");
                break;

            case 3:
                test = message(M_SCR|M_KEY, "Printer not selected - try again? (Y/n)");
                break;

            case 4:
                test = message(M_SCR|M_KEY, "Printer out of paper - try again? (Y/n)");
                break;
        }
        if (test == KEY_ESC1)
            test = 'N';

    } while (test != 'N');

    return(-1);
}
