/**
*   DB_RPT.C    -
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
#include <dos.h>
#include "db_defs.h"
#include "vmisc.h"
#include "vfile.h"
extern struct date today;
extern SYSREC sysvar;
extern PRNCODES prnCodes;
extern CHAR *month[13];
extern FILE *defPrinter;
extern MOUSE_CLICK *points;

VOID reportmenu(VOID)
{
    STATIC CHAR *prompt[] = {
        "REPORT MENU",
        "JOB FOLLOW-UP REPORT",
#ifndef REP_VER
        "SALES ANALYSIS REPORT",
        "SALES FORCAST REPORT",
        "PRODUCT SEARCH REPORT",
        "PROJECT MANAGEMENT SHEET",
        "REQUISITION REPORT",
#endif        
        "CUSTOMIZED REPORTS",
        "RETURN TO MAIN MENU",
        NULL
    };
    MOUSE_CLICK *lastP;

#ifdef OLD_REPORT_MENU
    STATIC MOUSE_CLICK rptMenu[] = {
#ifndef REP_VER        
/*        {232, 64, 416, 64, KEY_1}, {232, 80, 424, 80, KEY_2},
        {232, 96, 416, 96, KEY_3}, {232, 112, 400, 112, KEY_4},
        {232, 128, 408, 128, KEY_5}, {232, 144, 408, 144, KEY_6},
        {0, 0, 0, 0, 0} this piece of code worked before adding General Bid Report */
        {232, 64, 416, 64, KEY_1}, {232, 80, 432, 80, KEY_2},
        {232, 96, 424, 96, KEY_3}, {232, 112, 416, 112, KEY_4}, 
        {232, 128, 400, 128, KEY_5}, {232, 144, 408, 144, KEY_6}, 
        {232, 160, 408, 160, KEY_7}, {232, 176, 408, 176, KEY_8}, {0, 0, 0, 0, 0}
#else    
        {232, 64, 416, 64, KEY_1}, {232, 80, 424, 80, KEY_2},
        {232, 96, 416, 96, KEY_3}, {0, 0, 0, 0, 0}
#endif
#endif

    /* Changed to two column menu on 3/8/98 */
    STATIC MOUSE_CLICK rptMenu[] = {
#ifndef REP_VER        
        {72, 64, 256, 64, KEY_1}, {72, 80, 264, 80, KEY_2},
        {72, 96, 256, 96, KEY_3}, {72, 112, 272, 112, KEY_4}, 
        {392, 64, 608, 64, KEY_5}, {392, 80, 560, 80, KEY_6},
        {392, 96, 560, 96, KEY_7}, {392, 112, 568, 112, KEY_8},
        {0, 0, 0, 0, 0}
#else    
        {232, 64, 416, 64, KEY_1}, {232, 80, 424, 80, KEY_2},
        {232, 96, 416, 96, KEY_3}, {0, 0, 0, 0, 0}
#endif
    };

    INT choice, lastchoice;

    /* inits */
    lastchoice = 1;

    /* save last mouse click points */
    lastP = points;

    do {

        /* register this menu's mouse click points */
        points = rptMenu;

//        switch (choice = menu(prompt, 1, 28, 0, lastchoice, HLP_REPORT)) {
        switch (choice = menu(prompt, 2, 10, 50, lastchoice, HLP_REPORT)) {

            /*
            * Job follow-up report
            */
            case 1:
                lastchoice = choice;
                jobfollowup();
                break;

            /*
            * Project Management Sheet
            */
            case 5:
                lastchoice = choice;
                project_management_sheet();
                break;

            /*
            * Sales analysis report
            */
            case 2:
                lastchoice = choice;
#ifndef REP_VER                
                sales_analysis();
#else                
                printmenu();
#endif
                break;

            /*
            * Sales Forcast Report
            */
            case 3:
                lastchoice = choice;
#ifndef REP_VER                
                sales_forcast();
#else                
                choice = 8;
#endif
                break;

            /* 
            * Job requisition report
            */
            case 6:
                lastchoice = choice;
#ifndef REP_VER
                jb_requisition_report();
#else
                choice = 8;
#endif
                break;

            /*
            * Customized reports
            */
            case 7:
                lastchoice = choice;
                printmenu();
                break;

            /*
            * exit this menu
            */
            case KEY_F10:
                choice = 8;
            case 8:
                break;

            /*
            * Product Search Report
            */
            case 4:
                lastchoice = choice;
                ProductSearchReport();
                break;

            /*
            * invalid key hit
            */
            default:
                beep();
                break;
        }
        s_closeall();

    } while (choice != 8);

    /* restore last mouse click points */
    points = lastP;
}

/*****************************************************************************
                Job Follow-Up Report
*****************************************************************************/
INT jobfollowup(VOID)
{
    SPREC sp;
    STREC state;
    JBREC jb;
    ESREC es, lastes;
    CDREC client;
    CHAR sdate[10], edate[10], status[2], ftemp[9], etemp[4], ptemp[40];
    CHAR fname[60], *dataptr, *lastptr = NULL, todaysdate[9];
    CHAR *list[]={"By Sales Person", "By Client", "By Status", "By Date (General bid)", "Bid Status Req. Let.", "By Architect", NULL};
    INT i, m, k, key, pos, test, choice, handle, field, np, lines;
    INT old, vec;
    FLOAT total_sale=0, mark_up=0, ttsale=0, ttmarkup=0;
    BOOLEAN endFlg, endthis;

    /* Inits */
    endFlg = endthis = FALSE;
    state.code[0] = '\0';
    sp.code[0] = '\0';
    status[0] = '\0';
    client.code[0] = '\0';
    key = KEY_DOWN;
    k = pos = np = lines = 0;
    total_sale = mark_up = ttsale = ttmarkup = 0;

    if ((choice = window_chooser(33, 9, list)) == -1)
        return(choice);

    store();
    setwindow(25, 11, 55, 18, "        Job Follow-Up", "     Enter Match Criteria", "F1=HELP                F10=DONE", "");
    Print("\n\r Start Date: \n\r Ending Date: \n\r SP Code: \n\r State: \n\r Client Code: \n\r Status:");
    do {

        switch (key) {
            case KEY_SHFT_TAB:
            case KEY_UP:
                if (pos == 1)
                    pos = 7;
                pos--;
                break;

            case KEY_TAB2:
            case KEY_DOWN:
                if (pos == 6)
                    pos = 0;
                pos++;
                break;

            case KEY_CTRL_PGUP:
            case KEY_HOME:
                pos = 1;
                break;

            case KEY_CTRL_PGDOWN:
            case KEY_END:
                pos = 6;
                break;

            case KEY_F10:
                pos = 7;
                break;

            /* this never gets hit because inpt() will not return KEY_ESC */
            case KEY_ESC:
                pos = 7;
                endFlg = TRUE;
                break;

            default:
                if (pos == 6)
                    pos = 0;
                pos++;
                break;
        }

        switch (pos) {

            /*
            * input of "Start Date"
            */
            case 1:
                key = inpt(14, 2, "", sdate, 8, CT_CHDATE, NO);
                break;

            /*
            * input of "Ending Date"
            */
            case 2:
                do {
                    key = inpt(15, 3, "", edate, 8, CT_CHDATE, NO);
                    if (key == KEY_F1)
                        StrDate(edate, &today);
                } while (key == KEY_F1);
                break;

            /*
            * Pick Sales Person for Report
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
            * choose matching "State"
            */
            case 4:
                do {
                    s_open(ST);
                    key = inpt(9, 5, "", state.code, 2, ST, NO);
                    if (key == KEY_F1) {
                        if ((test=gethelp(ST, s_getno(ST, 0, state.code), 9, 2, 2-1, 14-1)) != -1) {
                            s_read(ST, &state, test);
                        }
                    }
                    s_close(ST);
                } while (key == KEY_F1);
                break;

            /*
            * Client Code to match
            */
            case 5:    
                do {
                    s_open(CD);
                    key = inpt(15, 6, "", client.code, 10, CD, NO);
                    if (key == KEY_F1) {
                        if ((test=gethelp(CD, s_getno(CD, 0, client.code), 9, 4, 2-1, 14-1)) != -1)
                            s_read(CD, &client, test);
                    }
                    s_close(CD);

                } while (key == KEY_F1);
                break;

            /*
            *  choose "Status" to match
            */
            case 6:
                key = inpt(10, 7, "", status, 1, CT_CHAR, NO);
                break;

            case 7:
                break;
        }
    } while (pos != 7);

    restore();

    if (!endFlg) {

        if (sp.code[0] == '\0')
            strcpy(sp.code, "*");
        if (state.code[0] == '\0')
            strcpy(state.code, "*");
        if (status[0] == '\0')
            strcpy(status, "*");
        else
            strupr(status);
        if (client.code[0] == '\0')
            strcpy(client.code, "*");

        if (choice == 3) /* general bid report */
        {
            general_bid_report(sdate, edate, sp.code, state.code, client.code, status);
            return (0);
        }

        if (choice == 5) /* By Architect */
        {
            jobfollowup_by_arch(sdate, edate, sp.code, state.code, client.code, status);
            return (0);
        }

        if (choice == 4) /* bid status request letter */
        {
            if (!strcmp(client.code, "*"))
            {
                message(M_SCR|M_KEY, "Client Code must be specified -- any key to continue");
                return (0);
            }
            bid_status_req_let(sdate, edate, sp.code, state.code, client.code, status);
            return (0);
        }

        s_close(ES);
        strcpy(ftemp, db[ES].fname);
        strcpy(etemp, db[ES].extension);
        strcpy(ptemp, db[ES].path);
        sprintf(fname, "%s%s.%s", ptemp, ftemp, etemp);

        s_tempfile(ES);

        s_open(ES);
        db[ES].f[0].sort = 0;

        /*
        * determine which field to sort by
        */
        switch (choice) {

            /*
            * Sort by Sales Person
            */
            case 0:
                field = 3;
                break;

            /*
            * Sort by Cleint
            */
            case 1:
                field = 2;
                break;

            /*
            * Sort by Status
            */
            case 2:
                field = 17;
                break;
        }
        old = db[ES].f[field].sort;
        db[ES].f[field].sort = 3;
        db[ES].fields += 2;
        s_modnrecs(ES, 0);

        handle = FileOpen(fname);
        FileRead(handle, &m, 2);

        if (SelectPrinter() && printercheck() != -1) {

            /* Turn on Condensed mode. */
            fprintf(defPrinter, "%s", prnCodes.cond);
            
            /*
            * Dummy do-loop
            */
            do {

                message(M_SCR, "Working. . .");
                StrDate(todaysdate, &today);

                for (i=0; i<m; i++) {
                    FileRead(handle, &es, sizeof(ESREC));
                    if (!strcheck(client.code, es.client_code) && !strcheck(sp.code,es.spcode) && strdatecmp(es.estdate,sdate)>=0 && strdatecmp(es.estdate, edate) <= 0 && !strcheck(state.code, es.state)) {
                        vec = s_getno(JB, 0, es.jbcode);
                        if (vec == -1)
                            continue;
                        s_read(JB, &jb, vec);
                        if (status[0] != '*') /* use the specified status */
                        {
                            if (strcheck(status, jb.status))
                                continue;
                        }
                        else /* only look for B or A status */
                        {
                            if (jb.status[0] != 'B' && jb.status[0] != 'A')
                                continue;
                        }
                        strcpy(es.status, jb.status);
                        s_write(ES, &es, k, NO);
                        k++;
                        if (esccheck(NULL)) {
                            endthis = YES;
                            break;
                        }
                    }
                }
                FileClose(handle);

                if (endthis)
                    break;

                if (k == 0) {
                    message(M_SCR|M_KEY, "No Matching records found - hit any key");
                    break;
                }
                m = NO;

                for (i=0; i<k; i++) {


                    if (esccheck(""))
                        break;

                    if (lines == 0) {
                        np++;
                        fprintf(defPrinter, "                                                          %s\n\r\n\r", sysvar.company);
                        fprintf(defPrinter, "                                                        Job Follow-Up Report %s\n\r\n\r", list[choice]);
                        fprintf(defPrinter, "%-8s                                                                                                                       P. %2d\n\r\n\r", todaysdate, np);
#ifndef REP_VER
                        fprintf(defPrinter, "Job Code Job Name                         Client   Estimator                Telephone       Total     Mark-Up  SP  Date     Status\n\r");
#else                        
                        fprintf(defPrinter, "Job Code Job Name                         Client   Estimator                Telephone        COGS   Commission SP  Date     Status\n\r"); 
#endif
                        fprintf(defPrinter, "_____________________________________________________________________________________________________________________________________\n\r");
                        lines += 9;
                    }
                    s_read(ES, &es, i);
                    dataptr = (CHAR *)&es;

                    if (i>0 && stricmp(dataptr+db[ES].f[field].offset, lastptr+db[ES].f[field].offset)) {
                        fprintf(defPrinter, "\n\r%-8s SubTotals:                                                                        %10.2f %8.2f\n\r\n\r", lastptr+db[ES].f[field].offset, total_sale, mark_up);
                        lines += 3;
                        ttsale += total_sale;  /* Add last Subtotal to Totals. */
                        ttmarkup += mark_up;
                        total_sale = 0;      /* Reset Subtotals when new CD,SP, or State */
                        mark_up = 0;
                        m = YES;
                    }
                    lines++;
#ifndef REP_VER
                    fprintf(defPrinter, "%-8s %-32s %-8s %-24s %-15s %9.2f %8.2f %-3s %8s   %c\n\r",es.jbcode,es.jobname,es.client_code,es.estname,es.esttel,es.bid,es.mkup,es.spcode,es.estdate,es.status[0]);
#else
                    fprintf(defPrinter, "%-8s %-32s %-8s %-24s %-15s %9.2f %8.2f %-3s %8s   %c\n\r",es.jbcode,es.jobname,es.client_code,es.estname,es.esttel,es.cogs,es.com,es.spcode,es.estdate,es.status[0]); 
#endif
                    /*
                    * Insert Page Break at 60 lines
                    */
                    if (lines >= 58) {
                        lines = 0;
                        fputc(12, defPrinter);
                    }

                    if (stricmp(dataptr+db[ES].f[0].offset, lastptr+db[ES].f[0].offset)) {
                        total_sale += es.bid;  /* Calculate Sub-Totals */
                        mark_up += es.mkup;
                    }
                    lastes = es;
                    lastptr = (CHAR *)&lastes;
                }

                ttsale += total_sale;  /* Add last Subtotal to totals. */
                ttmarkup += mark_up;
                fprintf(defPrinter, "\n\r%-8s SubTotals:                                                                         %9.2f %8.2f\n\r\n\r", lastptr+db[ES].f[field].offset, total_sale, mark_up);
                if (m)
                    fprintf(defPrinter, "\n\rTotals:                                                                                     %9.2f %8.2f\n\r", ttsale, ttmarkup);
                fputc(12, defPrinter);


            } while (0);

            /* Turn off Condensed Mode. */
            fprintf(defPrinter, "%s", prnCodes.reset);
        }

        s_close(ES);
        s_delfile(ES);       /* Delete temp ES file. */

        db[ES].f[0].sort = 3;            /* Put all old info back into db[ES] */
        db[ES].f[field].sort = old;
        db[ES].fields -= 2;
        strcpy(db[ES].fname, ftemp);
        strcpy(db[ES].extension, etemp);
        strcpy(db[ES].path, ptemp);
    }

    return(0);
}

/*****************************************************************************
                General Bid Report
*****************************************************************************/
INT general_bid_report(CHAR *sdate, CHAR *edate, CHAR *spcode, 
                       CHAR *state, CHAR *clientcode, CHAR *status)
{
    JBREC jb;
    ESREC es;
    CHAR ftemp[9], etemp[4], ptemp[40];
    CHAR fname[60];
    INT i, m, k, vec, handle, field, np, lines, old;
        
    k = np = lines = 0;
    s_close(JB);
    strcpy(ftemp, db[JB].fname);
    strcpy(etemp, db[JB].extension);
    strcpy(ptemp, db[JB].path);
    sprintf(fname, "%s%s.%s", ptemp, ftemp, etemp);

    s_tempfile(JB);

    s_open(JB);
    db[JB].f[0].sort = 0;

    field = 13; /* sort by due date */
    old = db[JB].f[field].sort;
    db[JB].f[field].sort = 3;
    db[JB].fields += 2;
    s_modnrecs(JB, 0);

    handle = FileOpen(fname);
    FileRead(handle, &m, 2);

    if (SelectPrinter() && printercheck() != -1) {

        /* Turn on Condensed mode. */
        fprintf(defPrinter, "%s", prnCodes.cond);
        
        /*
        * Dummy do-loop
        */
        do {
                message(M_SCR, "Working. . .");
                
                for (i=0; i<m; i++) 
                {
                    FileRead(handle, &jb, sizeof(JBREC));
                    if (status[0] != '*') /* use the specified status */
                    {
                        if (strcheck(status, jb.status))
                            continue;
                    }
                    else /* only look for B or A status */
                    {
                        if (jb.status[0] != 'B' && jb.status[0] != 'A')
                            continue;
                    }

                    vec = s_getno(ES, 0, jb.code);
                    if (vec == -1)
                        continue;
                    
                    memset(&es, 0, sizeof(ESREC));
                    s_read(ES, &es, vec);
                    
                    if (!strcheck(clientcode, es.client_code) && !strcheck(spcode,es.spcode) && 
                         strdatecmp(jb.due,sdate)>=0 && strdatecmp(jb.due, edate) <= 0 && 
                        !strcheck(state, jb.state)) {
                        
                        strcpy(jb.sent, es.client_code);
                        strcpy(jb.addendum, es.spcode);
                        s_write(JB, &jb, k, NO);
                        k++;
                    }
                }
                
                FileClose(handle);
                s_close(ES);

                if (k == 0) {
                    message(M_SCR|M_KEY, "No Matching records found - hit any key");
                    break;
                }

                for (i=0; i<k; i++) 
                {
                    if (lines == 0) {
                        np++;
                        fprintf(defPrinter, "                                                          %s\n\r\n\r\n\r", sysvar.company);
                        fprintf(defPrinter, "                                                           Bid Request Report\n\r\n\r\n\r");                        
                        fprintf(defPrinter, "Job Code Job Name                         Client   General Bid List                 SP   Due Date   Status   Comment\n\r");
                        fprintf(defPrinter, "_____________________________________________________________________________________________________________________________________\n\r");
                        lines += 9;
                    }
                    s_read(JB, &jb, i);
                    
                    lines++;
                    
                    fprintf(defPrinter, "%-8s %-32s %-8s %-32s %-4s %-8s    %c\n\r",jb.code,jb.name,jb.sent,jb.contact,jb.addendum,jb.due, jb.status[0]);
                    
                    /*
                    * Insert Page Break at 60 lines
                    */
                    if (lines >= 58) {
                        lines = 0;
                        fputc(12, defPrinter);
                    }
                }
        } while (0);

        /* Turn off Condensed Mode. */
        fprintf(defPrinter, "%s", prnCodes.reset);
    }
    else
        FileClose(handle);

    s_close(JB);
    s_delfile(JB);       /* Delete temp JB file. */

    db[JB].f[0].sort = 3;            /* Put all old info back into db[JB] */
    db[JB].f[field].sort = old;
    db[JB].fields -= 2;
    strcpy(db[JB].fname, ftemp);
    strcpy(db[JB].extension, etemp);
    strcpy(db[JB].path, ptemp);

    return(0);
}

/*****************************************************************************
                Bit Status Request Letter
*****************************************************************************/
INT bid_status_req_let(CHAR *sdate, CHAR *edate, CHAR *spcode,
                       CHAR *state, CHAR *clientcode, CHAR *status)
{
    JBREC jb;
    ESREC es;
    CDREC cd;
    SPREC sp;
    CHAR ftemp[9], etemp[4], ptemp[40];
    CHAR fname[60];
    INT i, m, k, vec, handle, len;
    CHAR textfname[60], name[60], tdate[9];
    FILE *fp;
    extern USREC curuser;

    k = 0;
    s_close(JB);
    strcpy(ftemp, db[JB].fname);
    strcpy(etemp, db[JB].extension);
    strcpy(ptemp, db[JB].path);
    sprintf(fname, "%s%s.%s", ptemp, ftemp, etemp);

    s_tempfile(JB);
    s_open(JB);
    db[JB].f[0].sort = 0;

    handle = FileOpen(fname);
    FileRead(handle, &m, 2);
       
        /*
        * Dummy do-loop
        */
        do {
                message(M_SCR, "Working. . .");
                
                for (i=0; i<m; i++) 
                {
                    FileRead(handle, &jb, sizeof(JBREC));
                    if (status[0] != '*') /* use the specified status */
                    {
                        if (strcheck(status, jb.status))
                            continue;
                    }
                    else /* only look for B or A status */
                    {
                        if (jb.status[0] != 'B' && jb.status[0] != 'A')
                            continue;
                    }

                    vec = s_getno(ES, 0, jb.code);
                    if (vec == -1)
                        continue;
                    
                    memset(&es, 0, sizeof(ESREC));
                    s_read(ES, &es, vec);

                    if (!strcheck(clientcode, es.client_code) && !strcheck(spcode,es.spcode) &&
                         strdatecmp(jb.due,sdate)>=0 && strdatecmp(jb.due, edate) <= 0 && 
                        !strcheck(state, jb.state))
                    {
                        s_write(JB, &jb, k, NO);
                        k++;
                    }
                }
                
                s_close(ES);

                if (k == 0) {
                    message(M_SCR|M_KEY, "No Matching records found - hit any key");
                    break;
                }

                vec = s_getno(CD, 0, clientcode);
                if (vec == -1)
                    break;
                    
                memset(&cd, 0, sizeof(CDREC));
                s_read(CD, &cd, vec);
                s_close(CD);

                FileTempName(textfname);

                if ((fp = fileopn(textfname, "wt")) == NULL)
                {
                    message(M_SCR|M_ERR|M_KEY, "Unable to open %s - hit any key", textfname);
                    break ;
                }

                StrDate(tdate, &today);
                fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r");
                fprintf(fp, "        Date: %s\n\r\n\r\n\r", tdate);
                fprintf(fp, "        Estimate Department\n\r");
                fprintf(fp, "        %s\n\r", cd.name);
                fprintf(fp, "        %s\n\r", cd.address);
                fprintf(fp, "        %s, %s %s\n\r\n\r", cd.city, cd.state, cd.zip);
                fprintf(fp, "        Tel: %s\n\r", cd.tel);
                fprintf(fp, "        Fax: %s\n\r\n\r", cd.fax);
                fprintf(fp, "        Re: Project Status\n\r\n\r\n\r");

                strcpy(name, cd.contact1);
                len = strlen(name);
                for(i = 0; i < len; i++)
                {
                    if(name[i] == ' ')
                    {
                        name[i] = '\0';
                        break;
                    }
                }

                fprintf(fp, "        Dear %s:\n\r\n\r", name);
                fprintf(fp, "            We have bid your company on the following jobs in the last\n\r");
                fprintf(fp, "        few weeks/month.  Could you please advise if any of these were\n\r");
                fprintf(fp, "        awarded to your firm on the status.\n\r\n\r\n\r");

                fprintf(fp, "        Job Code Job Name                  Client Contact       Bid Date\n\r");
                fprintf(fp, "        ________________________________________________________________\n\r");

                for (i=0; i<k; i++) 
                {
                    s_read(JB, &jb, i);
                    vec = s_getno(ES, 0, jb.code);
                    s_read(ES, &es, vec);
                    fprintf(fp, "        %-8s %-25.25s %-20.20s %-8s\n\r",jb.code,jb.name,es.estname,jb.due);
                }
                
                fprintf(fp, "\n\r\n\r            Your help is greatly appreciated in updating our files.\n\r\n\r");
                fprintf(fp, "        Thank you.\n\r\n\r\n\r");

                vec = s_getno(SP, 0, curuser.spcode);
                if (vec == -1)
                    break;
                    
                memset(&sp, 0, sizeof(SPREC));
                s_read(SP, &sp, vec);
                s_close(SP);

                fprintf(fp, "        %s\n\r", sp.name);
                fprintf(fp, "        %s\n\r\n\r", sp.title);
        } while (0);

    FileClose(handle);
    
    s_close(JB);
    s_delfile(JB);       /* Delete temp JB file. */

    strcpy(db[JB].fname, ftemp);
    strcpy(db[JB].extension, etemp);
    strcpy(db[JB].path, ptemp);

    if (k != 0)
    {
        filecls(fp);
        TextEditor(textfname, cd.fax, cd.email);
        FileDelete(textfname);
    }

    return(0);
}


/*****************************************************************************
                Product Search Report
*****************************************************************************/
INT ProductSearchReport(VOID)
{
    POREC po;
    VDREC vd;
    PDREC pd;
    PLREC pl;
    ESREC es;
    CHAR sdate[10], edate[10], status[2], ftemp[9], etemp[4], ptemp[40];
    CHAR fname[60], todaysdate[9];
    INT i, j, k, l, key, pos, test, handle, np, lines, bg, ed, numPL;
    BOOLEAN endFlg, endthis;
    CHAR jbcode[9];

    /* Inits */
    endFlg = endthis = FALSE;
    vd.code[0] = '\0';
    pd.code[0] = '\0';
    status[0] = '\0';
    key = KEY_DOWN;
    k = pos = np = lines = 0;
    jbcode[0] = '\0';

    store();
    setwindow(22, 11, 58, 17, "        Product Search Report", "        Enter Match Criteria", "F1=HELP                      F10=DONE", "");
    Print("\n\r Start Date: \n\r Ending Date: \n\r Vendor Code: \n\r Product Code: \n\r Status:");
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
                pos = 6;
                endFlg = TRUE;
                break;

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
                key = inpt(14, 2, "", sdate, 8, CT_CHDATE, NO);
                break;

            /*
            * input of "Ending Date"
            */
            case 2:
                do {
                    key = inpt(15, 3, "", edate, 8, CT_CHDATE, NO);
                    if (key == KEY_F1)
                        StrDate(edate, &today);
                } while (key == KEY_F1);
                break;

            /*
            * Vendor Code:
            */
            case 3:            
                do {
                    s_open(VD);
                    key = inpt(15, 4, "", vd.code, 8, VD, NO);
                    if (key == KEY_F1) {
                        if ((test = gethelp(VD, s_getno(VD, 0, vd.code), 9, 4, 2-1, 14-1)) != -1)
                            s_read(VD, &vd, test);
                    }
                    s_close(VD);

                } while (key == KEY_F1);
                break;

            /*
            * Product Code:
            */
            case 4: {
                CHAR tmpFName[9];

                if(vd.code[0] != '\0')
                {
                    strcpy(tmpFName, db[PD].fname);
                    strcpy(db[PD].fname, vd.code);
                }

                do {
                    s_open(PD);
                    key = inpt(16, 5, "", pd.code, 20, PD, NO);
                    if (key == KEY_F1) {
                        if ((test = gethelp(PD, s_getno(PD, 0, pd.code), 9, 4, 2-1, 14-1)) != -1)
                            s_read(PD, &pd, test);
                    }
                    s_close(PD);

                } while (key == KEY_F1);

                if(vd.code[0] != '\0')
                    strcpy(db[PD].fname, tmpFName);

                break;
            }

            /*
            *  choose "Status" to match
            */
            case 5:
                key = inpt(10, 6, "", status, 1, CT_CHAR, NO);
                break;

            case 6:
                break;
        }
    } while (pos != 6);

    restore();

    if (!endFlg) {

        if (vd.code[0] == '\0')
            strcpy(vd.code, "*");
        if (pd.code[0] == '\0')
            strcpy(pd.code, "*");
        if (status[0] == '\0')
            strcpy(status, "*");
        else
            strupr(status);

        s_close(PO);
        strcpy(ftemp, db[PO].fname);
        strcpy(etemp, db[PO].extension);
        strcpy(ptemp, db[PO].path);
        sprintf(fname, "%s%s.%s", ptemp, ftemp, etemp);

        s_tempfile(PO);

        s_open(PO);
        
        handle = FileOpen(fname);
        FileRead(handle, &j, 2);

        if (SelectPrinter() && printercheck() != -1) {

            /* Turn on Condensed mode. */
            fprintf(defPrinter, "%s", prnCodes.cond);

            /*
            * Dummy do-loop
            */
            do {
                message(M_SCR, "Working. . .");
                StrDate(todaysdate, &today);

                for (i=0; i<j; i++) {
                    FileRead(handle, &po, sizeof(POREC));
                    if (status[0] == '*') /* check for 'O' or 'R' */
                    {
                        strupr(po.status);
                        if (po.status[0] != 'O' && po.status[0] != 'R')
                            continue;
                    }
                    else if (strcheck(status, po.status))
                        continue;

                    if(strdatecmp(po.podate,sdate)<0 || strdatecmp(po.podate, edate)>0) 
                        continue;

                    if (strcheck(vd.code,po.vdcode))
                        continue;

                    /* Finally, check for product code */
                    pos = s_getno(ES, 0, po.jbcode);
                    if(pos == -1) /* no ES associated, wrong. Skip */
                        continue;

                    s_read(ES, &es, pos);
                    if(openplfile(&es) == -1) /* no part list associated, wrong. Skip */
                        continue;

                    numPL = s_nrecs(PL);

                    if(po.plidx <= 0) /* old styly PO */
                    {
                        bg = 0;
                        ed = numPL - 1;
                    }
                    else
                    {
                        set_rate(&es, po.vdcode, TRUE); /* this sets vdlist[] global */
                        get_pl_pos(po.plidx, po.vdcode, &bg, &ed);
                    }

                    for(l = 0; l < numPL; l++)
                    {
                        if(l < bg || l > ed)
                            continue;

                        s_read(PL, &pl, l);
                        if(!strcheck(pl.vdcode, po.vdcode))
                            break;
                    }

                    if(l >= numPL) /* unable to find a product match */
                        continue;

                    if(pd.code[0] != '*' && strcheck(pl.pdcode, pd.code)) /* prod. code doesn't match */
                        continue;

                    strcpy(po.vd_inv_number, pl.pdcode);
                    po.plidx = pl.qty;

                    if (esccheck(NULL)) {
                        endthis = YES;
                        break;
                    }
                    s_write(PO, &po, k, NO);
                    k++;
                }

                if (endthis)
                    break;

                if (k == 0) {
                    message(M_SCR|M_KEY, "No Matching records found - hit any key");
                    break;
                }
                
                for (i=0; i<k; i++) {

                    if (esccheck(""))
                        break;

                    if (lines == 0) {
                        np++;
                        fprintf(defPrinter, "                                                        %s\n\r\n\r", sysvar.company);
                        fprintf(defPrinter, "                                                          Product Search Report\n\r\n\r");
                        fprintf(defPrinter, "%-8s                                                                                                                       P. %2d\n\r\n\r", todaysdate, np);
                        fprintf(defPrinter, "Job Code   Job Name                         VD Code   PO Number   Product               Quantity    Ship Date    Status    SP Code\n\r");
                        fprintf(defPrinter, "_____________________________________________________________________________________________________________________________________\n\r");
                        lines += 9;
                    }
                    s_read(PO, &po, i);


                    lines++;
                    fprintf(defPrinter, "%-8s   %-32s %-8s  %-8s    %-14.14s            %4d     %-8s      %-2s      %-8s\n\r",po.jbcode,po.jbname,po.vdcode,po.po_number,po.vd_inv_number,po.plidx,po.shipdate,po.status,po.spcode);
                    /*
                    * Insert Page Break at 60 lines
                    */
                    if (lines >= 58) {
                        lines = 0;
                        fputc(12, defPrinter);
                    }
                }
            } while (0);
                        
            if(k != 0)
                fputc(12, defPrinter);

            /* Turn off Condensed Mode. */
            fprintf(defPrinter, "%s", prnCodes.reset);
        }
        
        FileClose(handle);
        s_close(PO);
        s_delfile(PO);       /* Delete temp PO file. */

        strcpy(db[PO].fname, ftemp);
        strcpy(db[PO].extension, etemp);
        strcpy(db[PO].path, ptemp);
    }

    return (0);

}

/*****************************************************************************
                JOb followup by architect
*****************************************************************************/
INT jobfollowup_by_arch(CHAR *sdate, CHAR *edate, CHAR *spcode, 
                        CHAR *state, CHAR *clientcode, CHAR *status)
{
    JBREC jb;
    ESREC es;
    CHAR ftemp[9], etemp[4], ptemp[40];
    CHAR fname[60];
    INT i, m, k, vec, handle, field, np, lines, old;
        
    k = np = lines = 0;
    s_close(JB);
    strcpy(ftemp, db[JB].fname);
    strcpy(etemp, db[JB].extension);
    strcpy(ptemp, db[JB].path);
    sprintf(fname, "%s%s.%s", ptemp, ftemp, etemp);

    s_tempfile(JB);

    s_open(JB);
    db[JB].f[0].sort = 0;

    field = 3; /* sort by architect */
    old = db[JB].f[field].sort;
    db[JB].f[field].sort = 3;
    db[JB].fields += 2;
    s_modnrecs(JB, 0);

    handle = FileOpen(fname);
    FileRead(handle, &m, 2);

    if (SelectPrinter() && printercheck() != -1) {

        /* Turn on Condensed mode. */
        fprintf(defPrinter, "%s", prnCodes.cond);
//defPrinter = fopen("c:\\temp\\temp.txt", "wt");

        /*
        * Dummy do-loop
        */
        do {
                message(M_SCR, "Working. . .");
                
                for (i=0; i<m; i++) 
                {
                    FileRead(handle, &jb, sizeof(JBREC));
                    if (status[0] != '*') /* use the specified status */
                    {
                        if (strcheck(status, jb.status))
                            continue;
                    }
                    else /* only look for B or A status */
                    {
                        if (jb.status[0] != 'B' && jb.status[0] != 'A')
                            continue;
                    }

                    vec = s_getno(ES, 0, jb.code);
                    if (vec == -1)
                        continue;
                    
                    memset(&es, 0, sizeof(ESREC));
                    s_read(ES, &es, vec);
                    
                    if (!strcheck(clientcode, es.client_code) && !strcheck(spcode, es.spcode) && 
                         strdatecmp(es.estdate, sdate) >= 0 && strdatecmp(es.estdate, edate) <= 0 && 
                        !strcheck(state, es.state)) {
                        
                        strcpy(jb.sent, es.client_code);
                        strcpy(jb.agent, es.estname);
                        strcpy(jb.sfax, es.esttel);
                        strcpy(jb.zip, es.estdate);
                        s_write(JB, &jb, k, NO);
                        k++;
                    }
                }
                
                FileClose(handle);
                s_close(ES);

                if (k == 0) {
                    message(M_SCR|M_KEY, "No Matching records found - hit any key");
                    break;
                }

                for (i=0; i<k; i++) 
                {
                    if (lines == 0) {
                        np++;
                        fprintf(defPrinter, "                                                          %s\n\r\n\r\n\r", sysvar.company);
                        fprintf(defPrinter, "                                                      Job Follow-Up Report By Architect\n\r\n\r\n\r");                        
                        fprintf(defPrinter, "Job Code Job Name                 Client   Client Contact           Client Tel.     Architect                Architect Tel.  Date\n\r");
                        fprintf(defPrinter, "_____________________________________________________________________________________________________________________________________\n\r");
                        lines += 9;
                    }
                    s_read(JB, &jb, i);
                    
                    lines++;
                    
                    fprintf(defPrinter, "%-8s %-24.24s %-8s %-24.24s %-15.15s %-24.24s %-15.15s %-8.8s\n\r",jb.code,jb.name,jb.sent,jb.agent,jb.sfax,jb.arch,jb.artel,jb.zip);
                    
                    /*
                    * Insert Page Break at 60 lines
                    */
                    if (lines >= 58) {
                        lines = 0;
                        fputc(12, defPrinter);
                    }
                }
        } while (0);
//fclose(defPrinter);

        /* Turn off Condensed Mode. */
        fprintf(defPrinter, "%s", prnCodes.reset);
    }
    else
        FileClose(handle);

    s_close(JB);
    s_delfile(JB);       /* Delete temp JB file. */

    db[JB].f[0].sort = 3;            /* Put all old info back into db[JB] */
    db[JB].f[field].sort = old;
    db[JB].fields -= 2;
    strcpy(db[JB].fname, ftemp);
    strcpy(db[JB].extension, etemp);
    strcpy(db[JB].path, ptemp);

    return(0);
}

