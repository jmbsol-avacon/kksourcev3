/**
*   INPUT.C     -
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
#include <malloc.h>
#include <bios.h>
#include <dos.h>
#include <stdarg.h>
#include <ctype.h>
#include <conio.h>
#include <time.h>

#include "db_defs.h"
#include "vmouse.h"
#include "vmisc.h"

extern MOUSE_CLICK *points;
extern BOOLEAN mouseActive;
extern SYSREC sysvar;

BOOLEAN changeflg, Fflag;

static CHAR save_time[9], save_date[9], cur_time[9], cur_date[9];
extern BOOLEAN form_screen_flag;

/**************************************************************************
nextkey - returns the next user input. Or reads DEMO.DAT to run Demo
**************************************************************************/
USHORT nextkey(VOID)
{
#ifndef DEMO
    POINT point;
    USHORT key;
    clock_t timer;
    struct time t;
    extern SHORT db_inpt_flag;
    extern struct date today;
    
    /*
    * init save_time
    */
    gettime(&t);
    StrTime(save_time, &t);

    /*
    * wait till we get a keystroke
    */
    timer = clock();
    while (!KeyPress(&key)) {
        
        /*
        * check for 1 second click, and update the time on the up right cornor of the screen
        */
        gettime(&t);
        StrTime(cur_time, &t);
        if(!form_screen_flag && db_inpt_flag && strcmp(save_time, cur_time) != 0)
        {
            if(strcmp(cur_time, "12:00:00") == 0 || strcmp(cur_time, "00:00:00") == 0)
            {
                getdate(&today);
                StrDate(cur_date, &today);
                if(strcmp(cur_date, save_date) != 0)
                {
                    print_date(cur_date);
                }
            }

            strcpy(save_time, cur_time);
            print_time(cur_time);
        }

        if (sysvar.quitTime) {
            if (((clock() - timer)/CLK_TCK) > sysvar.quitTime)
                quit(0);
        }

        if (mouseActive) {

#if 0
            MouseGetStatus(&point.x, &point.y);
            message(M_SCR, "(%d, %d)", point.x, point.y);
#endif

            /*
            * get mouse button press and return key based on MOUSE_CLICK
            */
            if (MouseGetClick(MOUSE_LBUTTON, &point.x, &point.y)) {
                key = ConvertMouseClick(&point);
                break;
            }

            if (MouseGetClick(MOUSE_RBUTTON, NULL, NULL)) {
                key = KEY_ESC;
                break;
            }
        }
    }

    /* verify that the stack and heap never get below 1k each */
    Assert((stackavail() > 1000) && (coreleft() > 1000L));

#ifdef DEBUG
    switch (key) {

        case KEY_ALT_F9:
            message(M_SCR, "Core: %ld, Stack: %d", coreleft(), stackavail());
            break;

        case KEY_ALT_F10:
            quit(0);

        default:
            break;
    }
#endif

#else  /* ifndef DEMO */

  USHORT key;
  static int cntl = 0;
  static FILE *inp;
  int ch;
  long start,seconds;

  while (bioskey(1)!=0) if ((ch=getch())==27 || ch==32) {
    message(M_SCR, " Press ESC to Quit, or SPACE BAR to continue.");
    if (getch()==27) {
      filecls(inp);
      quit(0); }
    message(M_SCR, " "); }

  if (!cntl)
   {
    char *path={"demo.dat"};
    struct ffblk ff;
    if (findfirst(path,&ff,0)==-1)
     {
      message(M_SCR, "File: DEMO.DAT must be in the current directory!");
      bioskey(0);
      quit(0);   /* For Copy Protection in DEMO Only */
      cntl=-1;
     }
    else { inp=fileopn("demo.dat","rt"); cntl=1; }
   }
  switch (cntl)
   {
    case -1: return(bioskey(0));
    case 1:
      if ((ch=getc(inp))==EOF)
       {
        rewind(inp);
        ch=getc(inp);
        ch=getc(inp);  
       }
      else if (ch=='|') return(KEY_ENTER); /*13*/
      else if (ch==13||ch==10) ch=getc(inp);
      else if (ch=='~') {
        time(&start);
        seconds=start;
        while(seconds-start<2) time(&seconds);
        return(nextkey()); }
      if (ch!='`') return(ch);
      else
       {
        ch=getc(inp);
        switch(toupper(ch))
         {
          case '1': return(KEY_F1);
          case '2': return(KEY_F2);
          case '3': return(KEY_F3);
          case '4': return(KEY_F4);
          case '5': return(KEY_F5);
          case '6': return(KEY_F6);
          case '7': return(KEY_F7);
          case '8': return(KEY_F8);
          case '9': return(KEY_F9);
          case 'X': return(KEY_F10);
          case 'E': return(KEY_END);
          case 'U': return(KEY_UP);
          case 'D': return(KEY_DOWN);
          case 'R': return(KEY_RIGHT);
          case 'L': return(KEY_LEFT);
          case 'H': return(KEY_HOME);
          case 'P': return(KEY_PGUP);
          case 'N': return(KEY_PGDOWN);
          case 'I': return(KEY_INS);
          case 'T': return(KEY_DEL);
          case 'C': return(27);
          case 'B': return(8);
          case 'A': return(KEY_CTRL_Q);
          case ';':
            helpmessage(HLP_SCREEN1);
            return(nextkey());
          case '%':
            helpmessage(52);
            return(KEY_F9);
          case '#':
            message(M_SCR, " Press F9 to continue.");
            while (bioskey(0)!=KEY_F9);
            return(KEY_F10);
          case '&':
            helpmessage(51);
            return(KEY_F7);
          case '@':
            message(M_SCR, " Press F7 to continue.");
            while (bioskey(0)!=KEY_F7);
            return(KEY_F10);
          case '*':
            helpmessage(50);
            return(KEY_F8);
          case '!':
            message(M_SCR, " Press F8 to continue.");
            while (bioskey(0)!=KEY_F8);
            return(KEY_F10);
          case '[':
            helpmessage(56);
            return(KEY_F4);
          case ']':
            message(M_SCR, " Press F4 to continue.");
            while (bioskey(0)!=KEY_F4);
            return(KEY_F10);
          case '(':
            helpmessage(53);
            return(KEY_F10);
          case '+':
            return(KEY_ALT_F1);
          case ')':
            message(M_SCR, " Press ALT_F2 to continue.");
            while (bioskey(0)!=KEY_ALT_F2);
            return(KEY_F10);
          case '{':
            return(KEY_ALT_F2);
          case '}':
            message(M_SCR, " Press ALT_F1 to continue.");
            while (bioskey(0)!=KEY_ALT_F1);
            return(KEY_F10);
          case ':':
            return(KEY_ALT_F1);
          case '?':
            message(M_SCR, " Press F10 to continue.");
            while (bioskey(0)!=KEY_F10);
/*
            message(M_SCR, " ");
            return(KEY_F10);
*/
            return(nextkey()); 

          case '~': return(13);
          default:
           message(M_SCR, "Unkown Character in DEMO.DAT.");
           return(nextkey());
         }
       }
   }
#endif /* ifndef DEMO */

    return(key);
}

VOID FillMouseClickPoint(MOUSE_CLICK *p, USHORT xStart, USHORT yStart, USHORT xEnd, USHORT yEnd, USHORT key)
{
    /* can't access a null pointer */
    Assert(p != NULL);

    p->xStart = xStart;
    p->yStart = yStart;
    p->xEnd = xEnd;
    p->yEnd = yEnd;
    p->key = key;
}


USHORT ConvertMouseClick(POINT *p)
{
    INT i;
    USHORT key;

    key = 0;
    i = 0;
    while ((points+i)->key != 0) {
        if ((p->x >= (points+i)->xStart) && (p->x <= (points+i)->xEnd) && (p->y >= (points+i)->yStart) && (p->y <= (points+i)->yEnd)) {
            key = (points+i)->key;
            break;
        }
        i++;
    }

    return(key);
}


/**************************************************************************
inpt - generic input routine.
    x,y:        coordinates of input field
    prompt:     prompt string
    result:     pointer to return value and initial value
    len:        width of input field
    ty:         input data type
    right:      right justify flag: NO:left YES:right
**************************************************************************/
USHORT inpt(int x, INT y, CHAR *prompt, VOID *result, INT len, INT ty, BOOLEAN right)
{
    STATIC CHAR temp2[80], spaces[80], pw[80], format[80];
    struct text_info ti;
    INT ptr, inend, inchar, z, term;
    USHORT out, inkey;
    BOOLEAN startflg, endflg, decflg;
    INT count;
    struct date *dt, tempdt;
#ifdef DEMO
    LONG seconds,start;
#endif

    /* inits */
    startflg = YES;
    decflg = NO;
    out = 1;

    /* get current cursor position and text settings */
    gettextinfo(&ti);

    /*
    * convert current data to string
    */
    switch(ty) {

        case CT_CHDATE:
            tempdt = datecheck((CHAR *)result);
            if (tempdt.da_year)
                StrDate(temp2, &tempdt);
            else
                temp2[0] = *(CHAR *)result = 0;
            break;

        case CT_INT:
            if (*(SHORT *)result)
                itoa(*(SHORT *)result, temp2, 10);
            else
                temp2[0] = '\0';
            break;

        case CT_LONG:
            if (*(LONG *)result)
                ltoa(*(LONG *)result, temp2, 10);
            else
                temp2[0] = '\0';
            break;

        case CT_FLOAT:
            if (*(FLOAT *)result!=0) {
                sprintf(temp2, "%.2f",*(FLOAT *)result);
                decflg=YES;
            } else {
                temp2[0] = '\0';
            }
            break;

        case CT_FLOAT3:
            if (*(FLOAT *)result!=0) {
                sprintf(temp2, "%.5f",*(FLOAT *)result);
                decflg=YES;
            } else {
                temp2[0] = '\0';
            }
            break;

        case CT_DATE:
            dt = (struct date *)result;
            if (dt->da_year)
                StrDate(temp2, dt);
            else
                temp2[0]=0;
            break;

        case CT_PASSWORD:
            temp2[0] = pw[0] = 0;
            break;

        default:
            strcpy(temp2, (CHAR *)result);
            break;
    }

    /*print prompt and current data*/

    memset(spaces, ' ', len+1);
    *(spaces+len+1) = '\0';

    printxy(x, y, "%s", prompt);
    textattr(REVERSE);
    printxy(x+strlen(prompt), y, "%s", spaces);

    /*initialize cursor and pointers*/
    z = x+strlen(prompt);
    inend = z+len;
    temp2[inend-z] = 0;
    term = z+strlen(temp2);
    ptr = term;
    printxy(z, y, "%s ", temp2);
    gotoxy(ptr, y);
    Fflag = endflg = NO;
    SetCursor(IsInsertOn() ? CUR_FUL : CUR_ON);

    do {
        /*read key*/
        inkey = nextkey();

        /* get ascii value of key */
        inchar = inkey & 0xff;

        /* ESC key is same as F10 */
        if (inkey == KEY_ESC) {
            inchar = 0;
            inkey = KEY_F10;
        }

        /*if CR then end of input*/
        if ((inkey == KEY_ENTER) || (inkey == KEY_TAB)) {
            endflg = YES;
        /*if ascii value is 0 then extended key*/
        } else if (!inchar || ((inkey >= KEY_MOUSE_0) && (inkey <= KEY_MOUSE_N))) {
            switch (inkey) {

                case KEY_DEL:
                    if (ptr != z) {
                        term--;
                        gotoxy(--ptr, y);
                        if ((ty == CT_FLOAT || ty == CT_FLOAT3) && (temp2[ptr-z] == '.'))
                            decflg = NO;
                        for (count=ptr; count<=term; temp2[count-z]=temp2[count-z+1],count++)
                            ;
                        printxy(z, y, "%s ", temp2);
                        gotoxy(ptr, y);
                        changeflg = YES;
                    } else {
                        beep();
                    }
                    break;

                case KEY_INS:
                    SetCursor(IsInsertOn() ? CUR_FUL : CUR_ON);
                    break;

                case KEY_RIGHT :
                    if (ptr-z < strlen(temp2)) {
                        gotoxy(++ptr, y);
                        break;
                    }

                default:
                    endflg = YES;
                    temp2[ptr-z] = 0;
                    Fflag = YES;
                    out = inkey;
                    break;
            }
        } else {

            switch (inchar) {

                case 8:
                    if (ptr != z)
                        gotoxy(--ptr, y);
                    break;
#ifdef DEMO
                case '~':
                    time(&start);
                    seconds = start;
                    while (seconds-start < 2)
                        time(&seconds);
                    break;
#endif

                case '.':
                    if ((ty==CT_FLOAT || ty==CT_FLOAT3)&&(!decflg)) {
                        decflg = YES;
                        goput(ptr, y, inchar);
                        if (IsInsertOn() || ptr==term)
                            term++;
                        if (IsInsertOn()) {
                            for (count=term; count>ptr; temp2[count-z]=temp2[count-z-1],count--)
                                ;
                        }
                        if (term > inend) {
                            term = inend;
                        }
                        temp2[term-z] = 0;
                        temp2[ptr-z] = inchar;
                        printxy(z, y, "%s ", temp2);
                        gotoxy(++ptr, y);
                        changeflg = YES;
                        break;
                    }

                case '-':
                    if (((ty==CT_FLOAT) || (ty==CT_FLOAT3) || (ty==CT_INT) || (ty==CT_LONG)) && (ptr==z)) {
                        if (IsInsertOn() || (ptr == term))
                            term++;
                        if (IsInsertOn()) {
                            for (count=term;count>ptr;temp2[count-z]=temp2[count-z-1],count--)
                                ;
                        }
                        if (term>inend) {
                            term = inend;
                        }
                        temp2[term-z] = 0;
                        temp2[ptr-z] = inchar;
                        printxy(z,y, "%s ", temp2);
                        gotoxy(++ptr, y);
                        changeflg = YES;
                        break;
                    }

                default :
                    if (startflg) {
                        temp2[0] = 0;
                        term = z;
                        printxy(x, y, "%s", prompt);
                        z = x+strlen(prompt);
                        printxy(z, y, "%s", spaces);
                        ptr = z;
                        gotoxy(ptr, y);
                        decflg = NO;
                        changeflg = YES;
                    }
                    if (ptr!=inend) {
                        if (((ty==CT_INT)||(ty==CT_LONG)||(ty==CT_FLOAT||ty==CT_FLOAT3)) && ((inchar<'0')||(inchar>'9')))
                            break;
                        if ((!IsInsertOn()) && (ty==CT_FLOAT||ty==CT_FLOAT3) && (temp2[ptr-z]=='.'))
                            decflg=NO;
                        if (IsInsertOn() || ptr==term)
                            term++;
                        if (IsInsertOn()) {
                            for (count=term; count>ptr; temp2[count-z]=temp2[count-z-1],count--)
                                ;
                        }
                        if (term > inend) {
                            term = inend;
                        }
                        temp2[term-z] = 0;
                        temp2[ptr-z] = ((ty==CT_PASSWORD) ? 15:inchar);
                        if (ty == CT_PASSWORD)
                            pw[ptr-z] = toupper(inchar);
                        printxy(z, y, "%s ", temp2);
                        gotoxy(++ptr, y);
                        changeflg = YES;
                        break;
                    } else {
                        beep();
                    }
            }
        }
        startflg = NO;
    } while (!endflg);

    /*convert string back to correct type*/
    switch (ty) {

        case CT_INT:
            *(SHORT *)result = (SHORT)atoi(temp2);
            break;

        case CT_LONG:
            *(LONG *)result = atol(temp2);
            break;

        case CT_FLOAT:
            *(FLOAT *)result = atof(temp2);
            sprintf(temp2, "%.2f", *(FLOAT *)result);
            temp2[inend-z] = 0;
            break;

        case CT_FLOAT3:
            *(FLOAT *)result = atof(temp2);
            sprintf(temp2, "%.5f", *(FLOAT *)result);
            temp2[inend-z] = 0;
            break;

        case CT_DATE:
            tempdt = datecheck(temp2);
            if (!tempdt.da_year) {
                if (dt->da_year)
                    StrDate(temp2, dt);
                else
                    temp2[0]=0;
            } else {
                *dt=tempdt;
            }
            break;

        case CT_CHDATE:
            tempdt = datecheck(temp2);
            if (!tempdt.da_year)
                temp2[0] = 0;
            else
                StrDate(temp2, &tempdt);
            strcpy((CHAR *)result, temp2);
            break;

        case CT_PASSWORD:
            pw[strlen(temp2)] = 0;
            strcpy((CHAR *)result, pw);
            break;

        /*
        * force Sales Person and State fields to upper case
        */
        case ST:
        case SP:
            strcpy((CHAR *)result, strupr(temp2));
            break;

        default:
            strcpy((CHAR *)result, temp2);
            break;
    }

    /*restore original text attributes*/
    textattr(ti.attribute);

    /*print new value*/
    printxy(x, y, "%s%s", prompt, spaces);

    if (right)
        sprintf(format, "%%%ds", len);
    else
        strcpy(format, "%s");
    printxy(z, y, format, temp2);


    Assert(strlen(temp2) < 80);
    Assert(strlen(spaces) < 80);
    Assert(strlen(pw) < 80);
    Assert(strlen(format) < 80);
                  
    /* turn off cursor */
    SetCursor(CUR_OFF);
    return(out);
}

/**************************************************************************
inputs - generic full screen input routine.
    help:       message bar string
    ptr:        array of pointers to data
    control:    control structure array: x,y,data_type,width,prompt
    ...:        if ptr==NULL then data pointers are listed
**************************************************************************/
int inputs(int helptype, CHAR help[], VOID *ptr[], CHAR *control[], ...)
{
    struct date testdate;
    va_list argptr;
    STATIC CHAR prompt[20][80];
    VOID *params[20];
    CHAR *dataptr;
    INT choice=1, lastchoice, number, i, test, out=0;
    INT x[20], y[20], type[20], width[20];
    BOOLEAN endflg;

    message(M_SCR, help);

    for (number=0; control[number]!=NULL; number++) {
        if (!number)
            sscanf(control[0], "%d%d%s", &x[0], &y[0], prompt[0]);
        else
            sscanf(control[number], "%d%d%d%d%s", &x[number], &y[number], &type[number], &width[number], prompt[number]);

        StrReplaceChar(prompt[number], '_', ' ');
        strcat(prompt[number], " "); 
    }

    printxy(x[0], y[0], "%s", prompt[0]);
    if (ptr == NULL) {
        va_start(argptr,control);
        for (i=1; i<number; i++)
            if (type[i] != KEY_MINUS)
                params[i] = va_arg(argptr, VOID *);
        va_end(argptr); 
    }
    else {
        for (i=1; i<number; i++)
            if (type[i] != KEY_MINUS)
                params[i] = ptr[i];
    }


    for (i=1; i<number; i++) {

        textattr(NORMAL);
        printxy(x[i], y[i], "%s", prompt[i]);
        textattr(BLUE|ON_LIGHTGRAY);

        switch (type[i]) {

            case CT_INT:
                if (*(SHORT *)params[i])
                    Print("%d", *(SHORT *)params[i]);
                break;

            case CT_LONG:
                if (*(LONG *)params[i])
                    Print("%ld", (long)*(LONG *)params[i]);
                break;

            case CT_FLOAT:
                if (*(FLOAT *)params[i] != 0)
                    Print("%.2f", *(FLOAT *)params[i]);
                break;

            default:
                Print("%s", (CHAR *)params[i]);
                break; 
        } 
    }

    endflg = FALSE;

    do {
        switch (type[choice]) {

            case CT_INT:
            case CT_FLOAT:
            case CT_LONG:
                lastchoice = choice;
                choice = inpt(x[choice]+strlen(prompt[choice]), y[choice], "", params[choice], width[choice], type[choice], NO);
                break;

            case CT_CHDATE:
            case CT_DATE:
                lastchoice = choice;
                choice = inpt(x[choice]+strlen(prompt[choice]), y[choice], "", params[choice], 8, CT_CHAR, NO);
                if (Fflag)
                    break;

                if (*(CHAR *)params[lastchoice]) {
                    testdate = datecheck((CHAR *)params[lastchoice]);
                    if (!testdate.da_year) {
                        *(CHAR *)params[lastchoice] = 0;
                        message(M_SCR|M_KEY, "Date must be in format mm/dd/yy - hit any key");
                        message(M_SCR, help);
                        choice=0;
                    } 
                }
                break;

            default:
                if (type[choice] > 0) {
                    lastchoice = choice;
                    choice = inpt(x[choice]+strlen(prompt[choice]), y[choice], "", params[choice], 10, CT_CHAR, NO);
                    if (choice != 1)
                        break;
                    if (*(CHAR *)params[lastchoice] && s_getno(type[choice], 0, params[lastchoice]) == -1) {
                        message(M_SCR|M_KEY, "%s is not defined - hit any key", db[type[choice]].extension);
                        message(M_SCR, help);
                        *(CHAR *)params[lastchoice] = 0;
                        choice = 0; 
                    }
                    message(M_SCR, help);
                } else {
                    lastchoice = choice;
                    choice = inpt(x[choice]+strlen(prompt[choice]), y[choice], "", params[choice], width[choice], CT_CHAR, NO);
                }
                break;
        }

        switch (choice) {

            /*
            * Get help on field user is currently on
            */
            case KEY_F1:
                choice = lastchoice;
                if (type[lastchoice] > 0) {
                    dataptr = (CHAR *)s_alloc(db[type[lastchoice]].fsize);
                    test = rechelp(type[lastchoice], 0, dataptr);
                    if (test != -1)
                        strcpy((CHAR *)params[lastchoice], dataptr);
                    s_free(dataptr);
                } else {
                    helpmessage(helptype);
                    message(M_SCR, help);
                }
                break;

            /*
            * handle cursor up
            */
            case KEY_SHFT_TAB:
            case KEY_LEFT:
            case KEY_UP:
                choice = lastchoice;
                choice--;
                break;

            /*
            * handle cursor down
            */
            case KEY_RIGHT:
            case KEY_TAB:
            case KEY_ENTER:
            case KEY_DOWN:
                choice = lastchoice;
                choice++;
                break;


            case KEY_PGUP:
            case KEY_CTRL_PGUP:
            case KEY_HOME:
                choice = lastchoice = 1;
                break;

            case KEY_PGDOWN:
            case KEY_CTRL_PGDOWN:
            case KEY_END:
                choice = lastchoice = number - 1;
                break;

            /*
            * handle exit
            */
            case KEY_F10:
                endflg = TRUE;
                out = -1;
                break;

            default:
                choice = lastchoice;
                break;
        }
        if (choice < 1)
            choice = number - 1;
        else if (choice >= number)
            choice = 1;

    } while (!endflg);

    return(out);
}

/**************************************************************************
lineinput - user input for choosing one string from an array of strings.
    x,y:    coordinates for scrolling window
    output: null string terminated array of strings
    start:  index of first string to be shown
    width:  width of scroll window
**************************************************************************/
int lineinput(int x, INT y, CHAR *output[], INT start, INT width)
{
    CHAR format[10];
    INT choice=0,out,max=0;
    BOOLEAN endflg;

    /* inits */
    endflg = NO;

    SetCursor(CUR_ON);
    out = start;
    while (output[max]!=NULL) max++;
    sprintf(format, "%%-%ds",width);
    while (!endflg) {
        printxy(x,y,format,output[out]);
        gotoxy(x+strlen(output[out]),y);

        switch (choice = nextkey()) {

            case KEY_F10:
                endflg=YES;
                out=-1;
                break;

            case KEY_RIGHT:
                out=(out+1)%max;
                break;

            case KEY_LEFT:
                if (out)
                    out=(out-1)%max;
                else
                    out=max-1;
                break;

            default:
                choice&=0xff;
                if (choice==KEY_ESC1) {
                    endflg=YES;
                    out=-1;
                    break;
                }
                if (choice==KEY_CR)
                    endflg=YES;
                break;
        }
    }
    SetCursor(CUR_OFF);
    return(out);
}
