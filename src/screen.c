/**
*   SCREEN.C    -  Screen I/O and Window routines
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
#include <time.h>
#include <conio.h>
#include "db_defs.h"
#include "vmouse.h"
#include "vmisc.h"

#ifndef REP_VER
STATIC CHAR *programTitle = "Kwik Kuote: Distributor Information System";
#else
STATIC CHAR *programTitle = "Kwik Kuote: Representative Quotation System";
#endif

CHAR *month[] = {
    "December",
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"
};

STATIC CHAR *svbuffer = NULL;
STATIC struct text_info *svti = NULL;
STATIC INT storecnt = 0;
extern struct text_info orig;

extern struct date today;
extern BOOLEAN mouseActive;
extern MOUSE_CLICK *points;

/* for display company name and time on the first line */
extern SYSREC sysvar;

/*************************************************************************
print_time - prints time on the up right cornor of the screen
*************************************************************************/
VOID print_time(CHAR *time)
{
    CHAR tt[32];      /* screendump() needs 2 bytes per cell */
    INT x, y;

    x = wherex(); y = wherey();
    strcpy(tt, time);
    screendump(64, 1, 72, 1, tt, YELLOW|ON_LIGHTGRAY);
    gotoxy(x, y);
}


/*************************************************************************
print_date - prints date on the up right cornor of the screen
*************************************************************************/
VOID print_date(CHAR *date)
{
    CHAR temp[32];    /* screendump() needs 2 bytes per cell */
    INT x, y;

    x = wherex(); y = wherey();
    strcpy(temp, date);
    screendump(73, 1, 80, 1, temp, YELLOW|ON_LIGHTGRAY);
    gotoxy(x, y);
}

/*************************************************************************
mainscreen - sets up screen partitions.
*************************************************************************/
VOID mainscreen(VOID)
{
    CHAR *str, strDate[4];
    time_t secs;
    INT i;

    clrscr();
    textattr(YELLOW|ON_LIGHTGRAY);
    Print("ษออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป");
    for (i=0; i<22; i++)
        Print("บ                                                                              บ");
    Print("ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ");
    printxy(1,5, "ฬออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออน");

    textattr(WHITE|ON_LIGHTGRAY);

    /* show this application's title  and copyright notice */
    printxy(40-strlen(programTitle)/2, 2, programTitle);
    printxy(18, 3, "(C) Copyright 1994 Automation Solutions, Inc.");

    time(&secs);
    strncpy(strDate, ctime(&secs), 3);
    strDate[3] = '\0';
    getdate(&today);
    str = (CHAR *)s_alloc(60);
    sprintf(str, "%s %s %d, %d", strDate, month[today.da_mon], today.da_day, today.da_year);
    printxy(40-strlen(str)/2, 4, str);
    message(M_SCR, NULL);

    /* fill last character on screen */
    *str = 32;
    *(str+1) = REVERSE;
    puttext(80, 25, 80, 25, str);
    s_free(str);
    textattr(NORMAL);
}

/*************************************************************************
screendump - dumps to screen from buffer.
    lf,tp,rt,bm:    coordinates of box for dump
    buf:            buffer containing screen data
*************************************************************************/
VOID screendump(INT lf, INT tp, INT rt, INT bm, CHAR *buf, INT attr)
{
    INT i, j, index;

    index = 2*(1+rt-lf);
    for (i=0; i<= bm-tp; i++) {
        for (j=rt-lf; j>=0; j--) {
            *(buf+(index*i+2*j)) = *(buf+(index*i+j));
            *(buf+(index*i+2*j+1)) = attr;
        }
    }
    /* turn off mouse cursor before any screen I/O */
    if (mouseActive)
        MouseHideCursor();

    puttext(lf, tp, rt, bm, buf);

    /* put mouse cursor back on */
    if (mouseActive)
        MouseShowCursor();
}


/*************************************************************************
inscreen - screen setup routine for column list type input screens
*************************************************************************/
INT inscreen(DB_TYPE type)
{
    FIELDPARAMS *attr;
    CHAR *scrbuf;
    INT i, j;
    struct time t;
    CHAR tt[9], date[9];
    
    /* allocate a buffer to hold the input screen */
    scrbuf = (CHAR *)s_alloc(25*160);

    memset(scrbuf, ' ', 25*160);
    memset(scrbuf+1*160, 205, 80);
    memset(scrbuf+22*160, 205, 80);
    memset(scrbuf+4*160, 205, 80);

    for (i=5;i<=21;i++) {
        memset(scrbuf+i*160, (i%2 ? ' ':196), 80);
    }

    for (i=0; i< db[type].fields;i++) {
        attr = db[type].f+i;
        sheadprintf(scrbuf+2*160, attr->x, KEY_LEFT, attr->header);
        sheadprintf(scrbuf+3*160, attr->x, KEY_RIGHT, attr->header);
        if (i > 0) {
            *(scrbuf+1*160+attr->x-2) = 209;
            *(scrbuf+4*160+attr->x-2) = 216;
            *(scrbuf+22*160+attr->x-2) = 207;
            *(scrbuf+2*160+attr->x-2) = *(scrbuf+3*160+attr->x-2) = 179;
            for (j = 5; j<=21; j++) {
                *(scrbuf+j*160+attr->x-2) = (j%2 ? 179 : 197);
            }
        }
    }

    strcpy(scrbuf+(80 - strlen(db[type].title))/2, db[type].title);
    strcpy(scrbuf+23*160, db[type].s->msg[0].msg);
    memset(scrbuf+24*160, 219, 80);

    /* print buffer to screen */
    screendump(1, 1, 80, 24, scrbuf, NORMAL);

    /* print empty message bar */
    message(M_SCR, NULL);

    /* fill last character on screen */
    *scrbuf = 32;
    *(scrbuf+1) = REVERSE;
    puttext(80, 25, 80, 25, scrbuf);

    /* display company and time on the up left and up right of the screen */
    textattr(YELLOW|ON_LIGHTGRAY);
    printxy(1, 1, "%.23s", sysvar.company);
    gettime(&t);

    /* don't print date & time if Room Schedule screen */
    if(type != RS)
        print_time(StrTime(tt, &t));
    StrDate(date, &today);
    if(type != RS)
        print_date(date);

    textattr(BLUE|ON_LIGHTGRAY);

    /* free the screen buffer */
    s_free(scrbuf);

    return(0);
}


/*************************************************************************
inpt_screen - draws standard input screen.
*************************************************************************/
INT inpt_screen(DB_TYPE type)
{
    DB_MSG *msgs;
    CHAR *scrbuf, *dblborder;
    INT i;
    struct time t;
    CHAR tt[9], date[9];

    scrbuf = (CHAR *)s_alloc(25*160);
    dblborder = (CHAR *)s_alloc(80);

    memset(scrbuf, ' ', 25*160);

    memset(dblborder, 205, 78);
    *(dblborder+78) = '\0';

    memset(scrbuf+1*160, 205, 80);

    msgs = db[type].s->msg;

    for (i = 0; msgs[i].msg != NULL; i++) {
        strcpy(scrbuf+(msgs[i].y-1)*160+msgs[i].x-1, msgs[i].msg);
    }
    sprintf(scrbuf+(db[type].s->y-4)*160, "%c%s%c", 201, dblborder, 187);
    for (i=db[type].s->y-3; i<=21; i++) {
        *(scrbuf+i*160) = *(scrbuf+i*160+79) = 186;
    }
    sprintf(scrbuf+22*160, "%c%s%c", 200, dblborder, 188);

    memset(scrbuf+24*160, 219, 80);

    screendump(1, 1, 80, 25, scrbuf, NORMAL);

    message(M_SCR, NULL);

    /* fill last character on screen */
    *scrbuf = 32;
    *(scrbuf+1) = REVERSE;
    puttext(80, 25, 80, 25, scrbuf); 
    
    /* display company name and time on the up left and up right cornor of the screen */
    textattr(YELLOW|ON_LIGHTGRAY);
    printxy(1, 1, "%.23s", sysvar.company);    
    gettime(&t);
    print_time(StrTime(tt, &t));    
    StrDate(date, &today);
    print_date(date);
    
    textattr(GREEN|ON_LIGHTGRAY);

    s_free(dblborder);
    s_free(scrbuf);

    return(0);
}

/*************************************************************************
sheadprintf - takes string of form lstring|rstring and prints either the
left or right string into another string at coordinate x.
*************************************************************************/
INT sheadprintf(CHAR *st1, INT x, INT lr, CHAR *head)
{
    INT i = 0;

    x--;
    if (lr == KEY_LEFT) {
        for (i=0; head[i]!='|'; i++,x++) {
            st1[x] = head[i];
        }
    } else {
        while (head[i++] != '|')
            ;
        while (head[i++] != 0) {
            st1[x] = head[i-1];
            x++;
        }
    }
    return(i);
}

/*************************************************************************
insetup - prints screen data for column type screen.
    type:   db type
    c:  cursor structure
    data:   not used
*************************************************************************/
INT insetup(DB_TYPE type, IN_POS *c, VOID *data)
{
    VOID *rec;
    INT i, record;

    data = data;

    record = c->top;

    rec = s_alloc(db[type].fsize);

    for (i=6; i<=22; i+=2,record++) {
        if (record >= s_nrecs(type)) {
            s_clearrec(type, rec);
        } else {
            s_read(type, rec, record);
        }
        in_printline(type, i, record, rec);
    }
    s_free(rec);

    return(0);
}

/*************************************************************************
in_printline - prints given record to column type screen at y.
*************************************************************************/
VOID in_printline(DB_TYPE type, INT y, INT record, VOID *rec)
{
    struct date *dout;
    FIELDPARAMS *attr;
    CHAR *temp2, tempStr[10];
    INT i, n;

    n = s_nrecs(type);
    attr = db[type].f;
    for (i=0; i < db[type].fields; i++) {
        gotoxy(attr->x, y);
        temp2 = (CHAR *)rec;
        temp2 += attr->offset;

        switch(attr->type) {

            case CT_LONG:
                if (record < n)
                    Print(attr->format, (long)*(LONG *)temp2);
                else {
                    if (*(LONG *)temp2 != 0)
                        Print(attr->format, (long)*(LONG *)temp2);
                    else {
                        strcpy(tempStr, attr->format);
                        StrReplaceChar(tempStr, 'l', '\0');
                        strcat(tempStr, "s");
                        Print(tempStr, " ");
                    }
                }
                break;

            case CT_INT:
                if (record < n)
                    Print(attr->format, *(SHORT *)temp2);
                else
                    ciprintf(attr->format, *(SHORT *)temp2);
                break;

            case CT_FLOAT:
            case CT_FLOAT3:
                if (record < n)
                    Print(attr->format, *(FLOAT *)temp2);
                else
                    bcprintf(attr->format, *(FLOAT *)temp2);
                break;

            case CT_DATE:
                dout = (struct date *)temp2;
                if (record < n) {
                    StrDate(tempStr, dout);
                    Print("%-8s", tempStr);
                } else {
                    Print("        ");
                }
                break;

            default:
//                Print(attr->format, temp2);   commented on 4/7/97 for %p in PL screen bug
                /*
                 * turn off mouse cursor when printing to screen
                 */
                if (mouseActive)
                    MouseHideCursor();

                cprintf(attr->format, temp2);
                /*
                 * turn mouse cursor back on
                 */
                if (mouseActive)
                    MouseShowCursor();

                break;
        }
        attr++;
    }
}


/*************************************************************************
setwindow - creates blank window on screen.
    lt,tp,rt,bm:    defines box for blank window
    m1:             top line message
    m2:             2nd line message
    m3:             bottom line message
    head:           title header (inside window)
*************************************************************************/
VOID setwindow(INT lt, INT tp, INT rt, INT bm, CHAR *m1, CHAR *m2, CHAR *m3, CHAR *head)
{
    INT i, width, length, index, hd;
    CHAR *scrBuf;

    bm++;
    width = rt-lt+3;
    index = 2*width;
    length = bm-tp+1;
    hd = ((head==NULL) ? 0 : 2);
    scrBuf = (CHAR *)s_alloc(length*index*2);

    memset(scrBuf, 219, index*2);
    memset(scrBuf+(length-1)*index, 219, width);

    for (i=2; i<hd+length+1; i++) {
        memset(scrBuf+i*index, ' ', width);
        *(scrBuf+i*index) = *(scrBuf+i*index+width-1) = 219;
    }
    if (head != NULL) {
        strcpy(scrBuf+2*index+2, head);
        memset(scrBuf+3*index+1, 196, width-2);
    }

    memset(scrBuf+(hd+length+1)*index, 219, width);
    screendump(lt-1, tp-hd-2, rt+1, bm, scrBuf, BROWN|ON_BLUE);
    s_free(scrBuf);

    window(lt-1, tp-hd-2, rt+1, bm+1);
    textattr(WHITE|ON_BROWN);
    printxy(2, 1, m1);
    printxy(2, 2, m2);
    printxy(2, 2+length+hd, m3);
    textattr(WHITE|ON_BLUE); /* NORMAL */
    window(lt, tp, rt, --bm);
}

/****************************************************************************
scrrestore - restores screen parameters given in ti.
****************************************************************************/
VOID scrrestore(struct text_info *ti)
{
    textattr(ti->attribute);
    window(ti->winleft, ti->wintop, ti->winright, ti->winbottom);
    gotoxy(ti->curx, ti->cury);
}


/****************************************************************************
store - pushes current screen buffer onto buffer stack.
****************************************************************************/
VOID store(VOID)
{
    struct text_info *ti;

    if (svbuffer == NULL) {
        storecnt = 1;
        svbuffer = (CHAR *)s_alloc(orig.screenwidth*orig.screenheight*2);
        svti = (struct text_info *)s_alloc(sizeof(struct text_info));
    } else {
        svbuffer = (CHAR *)s_realloc(svbuffer,++storecnt*orig.screenwidth*orig.screenheight*2);
        svti = (struct text_info *)s_realloc(svti,storecnt*sizeof(struct text_info));
    }

    /* turn off mouse cursor during screen I/O */
    if (mouseActive)
        MouseHideCursor();

    gettextinfo(ti = svti+storecnt-1);
    gettext(1, 1, ti->screenwidth, ti->screenheight, svbuffer+(storecnt-1)*orig.screenwidth*orig.screenheight*2);

    /* turn mouse cursor back on if the mouse is active */
    if (mouseActive)
        MouseShowCursor();
}

/****************************************************************************
restore - pops top of buffer stack to screen buffer.
****************************************************************************/
VOID restore(VOID)
{
    struct text_info *ti;
    extern SHORT db_inpt_flag;
    extern BOOLEAN form_screen_flag;
    CHAR tt[9], date[9];
    struct time t;

    ti = svti+(--storecnt);

    /*
    * turn off mouse cursor for all screen I/O
    */
    if (mouseActive)
        MouseHideCursor();

    /* paint the original screen back */
    puttext(1, 1, ti->screenwidth, ti->screenheight, svbuffer+storecnt*orig.screenwidth*orig.screenheight*2);

    /* restore the original screen attributes */
    scrrestore(ti);

    /* do not restore the original time and date */
    if(db_inpt_flag && !form_screen_flag)
    {
        gettime(&t);
        getdate(&today);
        StrDate(date, &today);
        print_time(StrTime(tt, &t));
        print_date(date);
    }

    /* turn mouse cursor back on if mouse is active */
    if (mouseActive)
        MouseShowCursor();

    /*
    * if this was the last restore, then free the screen buffers
    */
    if (storecnt == 0) {
        s_free(svbuffer);
        svbuffer = NULL;
        s_free(svti);
        svti = NULL;
    } else {
        svbuffer = (CHAR *)s_realloc(svbuffer, storecnt*orig.screenwidth*orig.screenheight*2);
        svti = (struct text_info *)s_realloc(svti, storecnt*sizeof(struct text_info));
    }
}

/******************** Kwik Kuote Intro Screen ****************************/
VOID IntroScreen(VOID)
{
    CHAR str[2];
    INT i;

    if (!KeyPress(NULL)) {

        if (mouseActive)
            MouseHideCursor();
        textattr(WHITE|ON_BLUE);

        clrscr();
        Print("ษออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป");
        Print("บ             ÛÛ    ÛÛ      ÛÛ        ÛÛ      ÛÛ      ÛÛ    ÛÛ                 บ");
        Print("บ             ÛÛ   ÛÛ       ÛÛ        ÛÛ      ÛÛ      ÛÛ   ÛÛ                  บ");
        Print("บ             ÛÛ  ÛÛ        ÛÛ        ÛÛ      ÛÛ      ÛÛ  ÛÛ                   บ");
        Print("บ             ÛÛÛÛÛ         ÛÛ        ÛÛ      ÛÛ      ÛÛÛÛÛ                    บ");
        Print("บ             ÛÛÛÛÛ         ÛÛ        ÛÛ      ÛÛ      ÛÛÛÛÛ                    บ");
        Print("บ             ÛÛ  ÛÛ        ÛÛ   ÛÛ   ÛÛ      ÛÛ      ÛÛ  ÛÛ                   บ");
        Print("บ             ÛÛ   ÛÛ       ÛÛ  ÛÛÛÛ  ÛÛ      ÛÛ      ÛÛ   ÛÛ                  บ");
        Print("บ             ÛÛ    ÛÛ      ÛÛ  ÛÛÛÛ  ÛÛ      ÛÛ      ÛÛ    ÛÛ                 บ");
        Print("บ             ÛÛ     ÛÛ     ÛÛ  Û  Û  ÛÛ      ÛÛ      ÛÛ     ÛÛ                บ");
        Print("บ             ÛÛ      ÛÛ    ÛÛÛÛÛ  ÛÛÛÛÛ      ÛÛ      ÛÛ      ÛÛ               บ");
        Print("บ                                                                              บ");
        Print("บ                                                                              บ");
        Print("บ           ÛÛ    ÛÛ     ÛÛ    ÛÛ     ÛÛÛÛÛÛ     ÛÛÛÛÛÛÛÛ    ÛÛÛÛÛÛ            บ");
        Print("บ           ÛÛ   ÛÛ      ÛÛ    ÛÛ    ÛÛ    ÛÛ       ÛÛ       ÛÛ                บ");
        Print("บ           ÛÛ  ÛÛ       ÛÛ    ÛÛ    ÛÛ    ÛÛ       ÛÛ       ÛÛ                บ");
        Print("บ           ÛÛÛÛÛ        ÛÛ    ÛÛ    ÛÛ    ÛÛ       ÛÛ       ÛÛ                บ");
        Print("บ           ÛÛÛÛÛ        ÛÛ    ÛÛ    ÛÛ    ÛÛ       ÛÛ       ÛÛÛÛÛ             บ");
        Print("บ           ÛÛ  ÛÛ       ÛÛ    ÛÛ    ÛÛ    ÛÛ       ÛÛ       ÛÛ                บ");
        Print("บ           ÛÛ   ÛÛ      ÛÛ    ÛÛ    ÛÛ    ÛÛ       ÛÛ       ÛÛ                บ");
        Print("บ           ÛÛ    ÛÛ     ÛÛ    ÛÛ    ÛÛ    ÛÛ       ÛÛ       ÛÛ                บ");
        Print("บ           ÛÛ     ÛÛ    ÛÛ    ÛÛ    ÛÛ    ÛÛ       ÛÛ       ÛÛ                บ");
        Print("บ           ÛÛ      ÛÛ    ÛÛÛÛÛÛ      ÛÛÛÛÛÛ        ÛÛ       ÛÛÛÛÛÛ            บ");
        Print("บ                                                                              บ");
        Print("ศออออออออออออออออ(C)Copyright 1994 - Automation Solutions, Inc.ออออออออออออออออ");

        /*
        * put last character on screen manualy so that it does not scroll
        */
        str[0] = 188;
        str[1] = WHITE|ON_BLUE;
        puttext(80, 25, 80, 25, str);

        for (i=0; i<=50; i++) {
            if (KeyPress(NULL) || (mouseActive && MouseGetClick(MOUSE_LBUTTON, NULL, NULL)))
                break;
            delay(100);
        }
        textattr(NORMAL);
        if (mouseActive)
            MouseShowCursor();
    }
}

/**************** New Text Mode Window Routines **************/
#define WN_CT_DOUBLE 0
#define WN_SINGLE 1

/*****************************************************************************
show_window() - Creates a text window.
    input:  left,top,right,bottom = standard window() coordinates.
        border = borderstyle (WN_CT_DOUBLE|WN_SINGLE).
*****************************************************************************/
VOID show_window(INT left, INT top, INT right, INT bottom, INT border)
{
    INT size, i, lt, rt, lb, rb, horz, vert;
    CHAR *buf;

    size = (right-left+1)*(bottom-top+1)*2;

    getborder(border, &lt, &rt, &lb, &rb, &horz, &vert);

    buf = (CHAR *)s_alloc(size);

    for (i=0;i<size;i++) {
        if (iseven(i))
            *(buf+i) = (CHAR)0;
        else
            *(buf+i) = (CHAR)YELLOW|ON_LIGHTGRAY;
    }

    for (i=1; i<right-left; i++) {
        *(buf+i*2) = horz;
        *(buf+(bottom-top)*(right-left+1)*2+i*2) = horz;
    }

    for (i=1; i<bottom-top; i++) {
        *(buf+i*2*(right-left+1)) = vert;
        *(buf+i*2*(right-left+1)+(right-left)*2) = vert;
    }

    *(buf+0) = lt;
    *(buf+size-2) = rb;
    *(buf+(bottom-top)*((right-left)*2+2)) = lb;
    *(buf+(right-left)*2) = rt;

    window(left, top, right, bottom);

    if (mouseActive)
        MouseHideCursor();

    puttext(left, top, right, bottom, buf);

    if (mouseActive)
        MouseShowCursor();

    window(left+1, top+1, right-1, bottom-1);
    s_free(buf);
}

/*****************************************************************************
getborder() - Sets Border Chars determined by border= WN_CT_DOUBLE or WN_SINGLE
*****************************************************************************/
VOID getborder(INT border, INT *upperleft, INT *upperright, INT *lowerleft, INT *lowerright, INT *horizontal, INT *vertical)
{
    switch (border) {

        case WN_CT_DOUBLE:
            *upperleft = 201;
            *vertical = 186;
            *lowerleft = 200;
            *lowerright = 188;
            *upperright = 187;
            *horizontal = 205;
            break;

        case WN_SINGLE:
        default:
            *upperleft = 218;
            *vertical = 179;
            *lowerleft = 192;
            *lowerright = 217;
            *upperright = 191;
            *horizontal = 196;
            break;
    }
}

/*****************************************************************************
window_chooser() - Makes Window from (startx,starty)
                to   (startx+length+1,starty+number_of_items)
           Lets User choose from CHAR *list[] of items.
       Returns item # or -1 if Escape.
*****************************************************************************/
INT window_chooser(INT startx, INT starty, CHAR *list[])
{
    STATIC MOUSE_CLICK p[10];
    INT i, end, pos=0, last=1, maxl=0;
    USHORT tmp;
    MOUSE_CLICK *lastP;
    BOOLEAN endFlg;

    /* inits */
    endFlg = FALSE;

    store();
    for (i=0; list[i]!=NULL; i++)
        maxl = (strlen(list[i])>maxl ? strlen(list[i]):maxl);

    /* calculate index of last choice */
    end = i - 1;

    /* display window on screen */
    show_window(startx, starty, startx+maxl+4, starty+end+2, WN_CT_DOUBLE);

    textattr(NORMAL);

    /*
    * display entries in window, and calculate mouse click points
    */
    for (i=0; list[i]!=NULL; i++) {
        gotoxy(1, i+1);
        Print(" %-*s ", maxl, list[i]);
        p[i].xStart = (startx)*8;
        p[i].yStart = p[i].yEnd = (starty + i)*8;
        p[i].xEnd = (startx + 2 + maxl)*8;
        p[i].key = KEY_MOUSE_0 + i;
    }

    /* nullify last record so ConvertMouseClick() knows when to stop */
    p[i].xStart = p[i].xEnd = p[i].yStart = p[i].yEnd = p[i].key = 0;

    /*
    * save last mouse click points and register the mouse click points
    * for this window
    */
    lastP = points;
    points = p;

    /*
    * continue receiving input until a menu item has been chosen
    */
    while (!endFlg) {

        /* display last menu item in normal mode */
        gotoxy(1, last+1);
        textattr(NORMAL);
        Print(" %-*s ", maxl, list[last]);

        /* handle wrap around */
        if (pos < 0)
            pos = end;
        else if (pos > end)
            pos = 0;

        last = pos;

        /* display current menu item in reverse mode */
        gotoxy(1, pos+1);
        textattr(REVERSE);
        Print(" %-*s ", maxl, list[pos]);
        textattr(NORMAL);

        /*
        * get and handle user input
        */
        switch (tmp = nextkey()) {

            case KEY_UP:
            case KEY_LEFT:
                pos--;
                break;

            case KEY_DOWN:
            case KEY_RIGHT:
                pos++;
                break;

            case KEY_HOME:
            case KEY_PGUP:
            case KEY_CTRL_PGUP:
            case KEY_CTRL_HOME:
                pos = 0;
                break;

            case KEY_END:
            case KEY_PGDOWN:
            case KEY_CTRL_PGDOWN:
            case KEY_CTRL_END:
                pos = end;
                break;

            case KEY_ENTER:
                endFlg = TRUE;
                break;

            case KEY_F10:
            case KEY_ESC:
                pos = -1;
                endFlg = TRUE;
                break;

            /*
            * handle mouse input
            */
            case KEY_MOUSE_0: /* 1st item */
            case KEY_MOUSE_1: /* 2nd item */
            case KEY_MOUSE_2:
            case KEY_MOUSE_3:
            case KEY_MOUSE_4:
            case KEY_MOUSE_5:
            case KEY_MOUSE_6:
            case KEY_MOUSE_7:
            case KEY_MOUSE_8:
            case KEY_MOUSE_9:
                pos = tmp - KEY_MOUSE_0;

                if ((pos >= 0) && (pos <= end)) {
                    endFlg = TRUE;
                    break;
                }
                pos = last;
                /* fall thru to default */

            default:
                beep();
                break;
        }
    }

    /* restore previous mouse click info */
    points = lastP;

    /* restore screen */
    restore();

    /* return chosen menu item */
    return(pos);
}

/*****************************************************************************
window_chooser_mod() - Modified version of window_chooser().
       Returns item # or -1 if Escape or -2 if move to above top or 
       -3 if move to below bottom.
*****************************************************************************/
INT window_chooser_mod(INT startx, INT starty, INT start, CHAR *list[])
{
    STATIC MOUSE_CLICK p[10];
    INT i, end, pos=start, last=1, maxl=0;
    USHORT tmp;
    MOUSE_CLICK *lastP;
    BOOLEAN endFlg;

    /* inits */
    endFlg = FALSE;

    store();
    for (i=0; list[i]!=NULL; i++)
        maxl = (strlen(list[i])>maxl ? strlen(list[i]):maxl);

    /* calculate index of last choice */
    end = i - 1;

    /* display window on screen */
    show_window(startx, starty, startx+maxl+4, starty+end+2, WN_CT_DOUBLE);

    textattr(NORMAL);

    /*
    * display entries in window, and calculate mouse click points
    */
    for (i=0; list[i]!=NULL; i++) {
        gotoxy(1, i+1);
        Print(" %-*s ", maxl, list[i]);
        p[i].xStart = (startx)*8;
        p[i].yStart = p[i].yEnd = (starty + i)*8;
        p[i].xEnd = (startx + 2 + maxl)*8;
        p[i].key = KEY_MOUSE_0 + i;
    }

    /* nullify last record so ConvertMouseClick() knows when to stop */
    p[i].xStart = p[i].xEnd = p[i].yStart = p[i].yEnd = p[i].key = 0;

    /*
    * save last mouse click points and register the mouse click points
    * for this window
    */
    lastP = points;
    points = p;

    /*
    * continue receiving input until a menu item has been chosen
    */
    while (!endFlg) {

        /* display last menu item in normal mode */
        gotoxy(1, last+1);
        textattr(NORMAL);
        Print(" %-*s ", maxl, list[last]);

        /* handle wrap around */
        if (pos < 0)
            pos = end;
        else if (pos > end)
            pos = 0;

        last = pos;

        /* display current menu item in reverse mode */
        gotoxy(1, pos+1);
        textattr(REVERSE);
        Print(" %-*s ", maxl, list[pos]);
        textattr(NORMAL);

        /*
        * get and handle user input
        */
        switch (tmp = nextkey()) {

            case KEY_UP:
            case KEY_LEFT:
                pos--;
                if(pos < 0)
                {
                    endFlg = TRUE;
                    pos = -2;
                }
                break;

            case KEY_DOWN:
            case KEY_RIGHT:
                pos++;
                if(pos > end)
                {
                    endFlg = TRUE;
                    pos = -3;
                }
                break;

            case KEY_HOME:
            case KEY_PGUP:
            case KEY_CTRL_PGUP:
            case KEY_CTRL_HOME:
                pos = 0;
                break;

            case KEY_END:
            case KEY_PGDOWN:
            case KEY_CTRL_PGDOWN:
            case KEY_CTRL_END:
                pos = end;
                break;

            case KEY_ENTER:
                endFlg = TRUE;
                break;

            case KEY_F10:
            case KEY_ESC:
                pos = -1;
                endFlg = TRUE;
                break;

            /*
            * handle mouse input
            */
            case KEY_MOUSE_0: /* 1st item */
            case KEY_MOUSE_1: /* 2nd item */
            case KEY_MOUSE_2:
            case KEY_MOUSE_3:
            case KEY_MOUSE_4:
            case KEY_MOUSE_5:
            case KEY_MOUSE_6:
            case KEY_MOUSE_7:
            case KEY_MOUSE_8:
            case KEY_MOUSE_9:
                pos = tmp - KEY_MOUSE_0;

                if ((pos >= 0) && (pos <= end)) {
                    endFlg = TRUE;
                    break;
                }
                pos = last;
                /* fall thru to default */

            default:
                beep();
                break;
        }
    }

    /* restore previous mouse click info */
    points = lastP;

    /* restore screen */
    restore();

    /* return chosen menu item */
    return(pos);
}

