/**
*   DIS_RPTS.C  - Reports used in Distributor version.
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
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include "db_defs.h"
#include "vmisc.h"
#include "vfile.h"

extern struct date today;
extern SYSREC sysvar;
extern PRNCODES prnCodes;
extern CHAR *month[13];
extern CHAR *tempDir;
extern FILE *defPrinter;

typedef struct {
    CHAR name[9];
    CHAR total[11];
    FLOAT ttotal;
    INT num;
} monthrec;

/* local function protocol */
INT prepare_daily_report(FILE *printer, CHAR* sdate, CHAR* status, CHAR* sp_date,
    SPREC sp, CDREC client, VDREC vendor, INT choice, CHAR* title, INT lines,
    BOOLEAN sch_flag, BOOLEAN screen);

/*****************************************************************************
		Project Management Sheet
*****************************************************************************/
INT project_management_sheet(VOID)
{
    POREC po;
    SPREC sp;
    CHAR sdate[10], edate[10], status[2], ftemp[9], etemp[4], ptemp[40], poshipdate[2];
    CHAR fname[60], todaysdate[9];
    INT i, j, k, key, pos, test, handle, np, lines;
    BOOLEAN endFlg, endthis;
    CHAR jbcode[9];
    CHAR *comment, *cmt = "* Not Released *";
    CHAR *cmt1 = "* Not Submitted *", *cmt2 = "* Not Approved *";
    struct date tmp_d;
    CHAR tmp_date[9];

    /* Inits */
    endFlg = endthis = FALSE;
    sp.code[0] = '\0';
    status[0] = '\0';
    key = KEY_DOWN;
    k = pos = np = lines = 0;
    jbcode[0] = '\0';
    poshipdate[0] = '\0';

    store();
    setwindow(25, 11, 55, 17, "   Project Management Sheet", "     Enter Match Criteria", "F1=HELP                F10=DONE", "");
    Print("\n\r Start Date: \n\r Ending Date: \n\r Ship or Po or suB Date: \n\r SP Code: \n\r Status:");
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
	    *  choose Ship Date or PO Date (Default is Ship Date)
	    */
	    case 3:
		key = inpt(26, 4, "", poshipdate, 1, CT_CHAR, NO);
		break;

	    /*
	    * Pick Sales Person for Report
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
		key = inpt(10, 6, "", status, 1, CT_CHAR, NO);
		break;

	    case 6:
		break;
	}
    } while (pos != 6);

    restore();

    if (!endFlg) {

	if (sp.code[0] == '\0')
	    strcpy(sp.code, "*");
	if (status[0] == '\0')
	    strcpy(status, "*");
	else
	    strupr(status);

	strupr(poshipdate);

	s_close(PO);
	strcpy(ftemp, db[PO].fname);
	strcpy(etemp, db[PO].extension);
	strcpy(ptemp, db[PO].path);
	sprintf(fname, "%s%s.%s", ptemp, ftemp, etemp);

	s_tempfile(PO);

	s_open(PO);
	db[PO].f[0].sort = 0;

	/* code to fix potential search problem 5/11/96 
	old = db[PO].f[1].sort; */
	/* db[PO].f[1].sort = 3; this causes missing records from the temp PO file */
	db[PO].fields += 3;
	s_modnrecs(PO, 0);
	/* end */
	
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
		    if (status[0] == '*') /* check for 'R' or 'O' */
		    {
			strupr(po.status);
			if (po.status[0] != 'R' && po.status[0] != 'O')
			    continue;
		    }
		    else if (strcheck(status, po.status))
			continue;
		    
		    if(poshipdate[0] == 'P') /* If we are required to check PO Date */
		    {
			if(strdatecmp(po.podate,sdate)<0 || strdatecmp(po.podate, edate)>0) 
			    continue;
		    }
		    else if(poshipdate[0] == 'B') /* If we are required to check Sub Date */
		    {
			if(strdatecmp(po.sub_date,sdate)<0 || strdatecmp(po.sub_date, edate)>0) 
			    continue;
		    }
		    else /* We must be required to check Ship Date */
		    {
			if(strdatecmp(po.shipdate,sdate)<0 || strdatecmp(po.shipdate, edate)>0) 
			    continue;
		    }

		    if (!strcheck(sp.code,po.spcode)) {

			if (esccheck(NULL)) {
			    endthis = YES;
			    break;
			}
			s_write(PO, &po, k, NO);
			k++;
		    }
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
			fprintf(defPrinter, "                                                        Project Management Sheet\n\r\n\r");
			fprintf(defPrinter, "%-8s                                                                                                                       P. %2d\n\r\n\r", todaysdate, np);
			fprintf(defPrinter, "Job Code Job Name                         VD No.   PO No.   PO Date   Sub Date  Apv Date  Rel Date  Ship Date  Sts  Comment                               \n\r");
			fprintf(defPrinter, "_____________________________________________________________________________________________________________________________________\n\r");
			lines += 9;
		    }
		    s_read(PO, &po, i);

		    comment = ""; 
		    if(po.status[0] == 'O' || po.status[0] == 'o')
		    {
			/* Is Sub Date set? */
			if(!strcmp(po.sub_date, "") || !strncmp(po.sub_date, "        ", 8))
			{
			    comment = cmt1;
			}
			/* Is Approval Date set? */
			else if(!strcmp(po.approval_date, "") || !strncmp(po.approval_date, "        ", 8))
			{
			    comment = cmt2;
			}
			else /* Both dates are set */
			{
			    memset(&tmp_d, 0, sizeof(struct date));
			    memcpy(&tmp_d, &today, sizeof(struct date));
			    DateAdd(&tmp_d, 35); 
			    StrDate(tmp_date, &tmp_d);
			    if(strdatecmp(po.shipdate,tmp_date)<0)
				comment = cmt; 
			}
		    }

		    lines++;
		    if(!strcmp(jbcode, po.jbcode)) /* same job code as the previous line */
		    {
			fprintf(defPrinter, "%-8s %-32s %-8s %-8s %-8s  %-8s  %-8s  %-8s  %-8s   %-2s   %s\n\r"," ",po.jbname,po.vdcode,po.po_number,po.podate,po.sub_date,po.approval_date,po.release_date,po.shipdate,po.status,comment);
		    }
		    else
		    {
			/* new group of jobs, a blank line for each group of jobs */
			if(lines != 10)  
			{    
			    fprintf(defPrinter, "\n\r"); /* but not at the beginning of a page */
			    lines++;
			}
			fprintf(defPrinter, "%-8s %-32s %-8s %-8s %-8s  %-8s  %-8s  %-8s  %-8s   %-2s   %s\n\r",po.jbcode,po.jbname,po.vdcode,po.po_number,po.podate,po.sub_date,po.approval_date,po.release_date,po.shipdate,po.status,comment);
			strcpy(jbcode, po.jbcode);
		    }
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

	/* code to fix the search problem 5/11/96 */
	db[PO].f[0].sort = 1;
	/* db[PO].f[1].sort = old; this causes missing record from the temp PO file */
	db[PO].fields -= 3;
	/* end */
	strcpy(db[PO].fname, ftemp);
	strcpy(db[PO].extension, etemp);
	strcpy(db[PO].path, ptemp);
    }

    return (0);
}


/*****************************************************************************
		Sales Analysis Report
*****************************************************************************/
INT sales_analysis(VOID)
{
    CHAR sdate[10],edate[10],sts[2],sp_date[2],ftemp[9],etemp[4],ptemp[40],fname[60],fname1[60],*dataptr,*lastptr=NULL,todaysdate[9],tmp_date[10];
    CHAR *list[]={"By Sales Person", "By Client", "By Vendor", "By State", "By Status", NULL};
    INT i,m,k=0,l=0,key=KEY_DOWN,pos=0,test,choice,handle,field,np=0,lines=0,old,endthis=NO;
    FLOAT total_sale=0,tax=0,mark_up=0,ttsale=0,ttax=0,ttmarkup=0,subcom=0,tcom=0,tcogs=0,subcogs=0,subactcogs=0,tactcogs=0;
    SPREC sp;
    STREC state;
    POREC po, lastpo;
    VDREC vendor;
    CDREC client;
    FILE *printer;   /* Stream of Output File/Printer/Screen/Fax */
    BOOLEAN taxit, taxable;
    INT ans, ans_actcogs, ans_screen;
    FLOAT other_exp=0, tt_other_exp=0;
    CHAR print_date[9];
    INT sum_rpt, num_line_page;
    FLOAT min_cogs = 0, min_bid = 0, min_profit = 0, min_com = 0, min_other = 0, min_tax = 0, min_actcogs = 0;
    POREC savepo;

    CHAR esftemp[9], esetemp[4], esptemp[40], jbcd_name[60];
    INT sorted, sales, esold, esold0, esfield, jbcd_handle, tNUM = 0, vec;
    SHORT numJB = 0, numPO = 0;
    ESREC es;
    CHAR rpt_type[2], amt_limit[16];
    FLOAT limit;
    FLOAT oth_cogs = 0, oth_bid = 0, oth_profit = 0, oth_com = 0, oth_mkup = 0, oth_tax = 0, oth_actcogs = 0;
    INT oth_jb = 0, oth_po = 0;
    CHAR title_date[32], title_by[32];

    /* inits */
    state.code[0]='\0';
    sp.code[0]='\0';
    sts[0]='\0';
    sp_date[0]='\0';
    client.code[0]='\0';
    vendor.code[0]='\0';
    taxit = taxable = NO;

    rpt_type[0] = '\0';
    amt_limit[0] = '\0';

    /*
    * choose sort field
    */
    if ((choice = window_chooser(34, 11, list)) == -1)
	return(choice);

    store();
    setwindow(25, 10, 55, 21, "        Sales Report", "     Enter Match Criteria", "F1=HELP                F10=DONE", "");
    Print("\n\r Start Date: \n\r Ending Date: \n\r SP Code: \n\r State: \n\r Client Code: \n\r Status: \n\r Ship Date or PO Date: ");
    Print("\n\r Vendor Code: \n\r Report Type (S/D/T): \n\r Sales Range: ");

retry:
    do {
	switch (key) {

	    case KEY_SHFT_TAB:
	    case KEY_UP:
		if (pos == 1)
		    pos = 11;
		pos--;
		break;

	    case KEY_TAB2:
	    case KEY_DOWN:
		if (pos == 10)
		    pos = 0;
		pos++;
		break;

	    case KEY_CTRL_PGUP:
	    case KEY_HOME:
		pos = 1;
		break;

	    case KEY_CTRL_PGDOWN:
	    case KEY_END:
		pos = 10;
		break;

	    case KEY_F10:
		pos = 11;
		break;

	    default:
		if (pos == 10)
		    pos = 0;
		pos++;
		break;
	}

	switch (pos) {

	    /*
	    * Starting date entry
	    */
	    case 1:
		key = inpt(14, 2, "", sdate, 8, CT_CHDATE, NO);
		break;

	    /*
	    * Ending date entry
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
	    * input State field
	    */
	    case 4:
		do {
		    s_open(ST);
		    key = inpt(9, 5, "", state.code, 2, ST, 0);
		    if (key == KEY_F1)
			if ((test = gethelp(ST, s_getno(ST, 0, state.code), 9, 2, 2-1, 14-1)) != -1)
			    s_read(ST, &state, test);
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
		    if (key == KEY_F1)
			if ((test=gethelp(CD, s_getno(CD, 0, client.code), 9, 4, 2-1, 14-1)) != -1)
			    s_read(CD, &client, test);
		    s_close(CD);
		} while (key == KEY_F1);
		break;

	    /*
	    * Status to match
	    */
	    case 6:
		key = inpt(10, 7, "", sts, 1, CT_CHAR, NO);
		break;

	    /*
	    * Using Ship Date or PO Date
	    */
	    case 7:
		key = inpt(24, 8, "", sp_date, 1, CT_CHAR, NO);
		break;
	    /*
	    * Vendor to match
	    */
	    case 8:
		do {
		    s_open(VD);
		    key = inpt(15, 9, "", vendor.code, 8, VD, 0);
		    if (key == KEY_F1)
			if ((test = gethelp(VD, s_getno(VD, 0, vendor.code), 9, 4, 2-1, 14-1)) != -1)
			    s_read(VD, &vendor, test);
		    s_close(VD);
		} while (key == KEY_F1);
		break;

	    /*
	    * Report type
	    */
	    case 9:
		key = inpt(23, 10, "", rpt_type, 1, CT_CHAR, NO);
		break;

	    /*
	    * Sales range
	    */
	    case 10:
		key = inpt(15, 11, "", amt_limit, 10, CT_CHAR, NO);
		break;

	    default:
		break;
	}
    } while (pos != 11);

    /* summary report? */
    if(rpt_type[0] == 'S' || rpt_type[0] == 's')
	sum_rpt = 'S';
    else if(rpt_type[0] == 'D' || rpt_type[0] == 'd')
	sum_rpt = 'D';
    else if(rpt_type[0] == 'T' || rpt_type[0] == 't')
	sum_rpt = 'T';
    else
    {
	message(M_SCR|M_KEY, "Report type must be Summary (S) or Detailed (D) or Total (T)");
	key = KEY_DOWN;
	pos = 8;
	goto retry;
    }

    /* generate start->end date title line and by title */
    sprintf(title_date, "From %s to %s", sdate[0]?sdate:" ", edate[0]?edate:" ");
    title_by[0] = '\0';
    if(sp_date[0])
    {
	if(sp_date[0] == 'S' || sp_date[0] == 's')
	    strcat(title_by, "By Ship Date ");
	else if(sp_date[0] == 'O' || sp_date[0] == 'o')
	    strcat(title_by, "By PO Date ");
    }
    if(sp.code[0])
    {
	strcat(title_by, "For ");
	strcat(title_by, sp.code);
    }

    /* generate criteria message. deleted on 3/9/98
    strcpy(cri, "Start: ");
    if(sdate[0])
	strcat(cri, sdate);
    strcat(cri, ", Ending: ");
    if(edate[0])
	strcat(cri, edate);
    strcat(cri, ", SP: ");
    if(sp.code[0])
	strcat(cri, sp.code);
    strcat(cri, ", State: ");
    if(state.code[0])
	strcat(cri, state.code);
    strcat(cri, ", Client: ");
    if(client.code[0])
	strcat(cri, client.code);
    strcat(cri, ", Status: ");
    if(sts[0])
	strcat(cri, sts);
    strcat(cri, ", VD: ");
    if(vendor.code[0])
	strcat(cri, vendor.code);
    strcat(cri, ", Sales Range: ");
    if(amt_limit[0])
	strcat(cri, amt_limit);
    if(sp_date[0])
    {
	strcat(cri, ", By ");
	if(sp_date[0] == 'S' || sp_date[0] == 's')
	    strcat(cri, "Ship Date");
	else if(sp_date[0] == 'O' || sp_date[0] == 'o')
	    strcat(cri, "PO Date");
    } */

    if (sp.code[0] == '\0')
	strcpy(sp.code, "*");
    if (state.code[0] == '\0')
	strcpy(state.code, "*");
    if (sts[0] == '\0')
	strcpy(sts, "*");
    else
	strupr(sts);
    if (client.code[0] == '\0')
	strcpy(client.code, "*");
    if (vendor.code[0] == '\0')
	strcpy(vendor.code, "*");
    if (amt_limit[0] == '\0')
	limit = 0;
    else
	limit = atof(amt_limit);
    
    restore();

    if (message(M_SCR|M_KEY, "Print a Tax Report? (y/N)") == 'Y') {
	taxit = YES;
	if (message(M_SCR|M_KEY, "Taxable? (Y/n)") != 'N')
	    taxable = YES;
    }
    /* whether this is a commission report */
    ans = message(M_SCR|M_KEY, "Is this a commission report? (n/Y)");
    if(ans == 'N') /* Not a commission report, need to ask whether actual COGS report 5/9/00 */
	ans_actcogs = message(M_SCR|M_KEY, "Is this an actual COGS report? (y/N)");

    /* if total report, is it sorted? based on what? */
    if(sum_rpt == 'T')
    {
	sorted = message(M_SCR|M_KEY, "Sorted Total report? (Y/n)");
	sales = ' ';
	if(sorted != 'N') /* sorted report */
	{
repeat:     sales = message(M_SCR|M_KEY, "Sorting is based on Sales or Profit? (S/P)");
	    if(sales != 'S' && sales != 'P')
		goto repeat;
	}

	/* create a temp file to hold the information */
	s_close(ES);
	strcpy(esftemp, db[ES].fname);
	strcpy(esetemp, db[ES].extension);
	strcpy(esptemp, db[ES].path);
    
	s_tempfile(ES);
    
	s_open(ES);
	esold0 = db[ES].f[0].sort;
	db[ES].f[0].sort = 0;

	if(sales == 'S')
	    esfield = 15;
	else if(sales == 'P')
	    esfield = 16;
    
	esold = db[ES].f[esfield].sort;
	db[ES].f[esfield].sort = 3;
	db[ES].fields += 2; /* was +=2 before status */
	s_modnrecs(ES, 0);
    }

    s_close(PO);
    strcpy(ftemp, db[PO].fname);
    strcpy(etemp, db[PO].extension);
    strcpy(ptemp, db[PO].path);
    sprintf(fname, "%s%s.%s", ptemp, ftemp, etemp);

    s_tempfile(PO);

    s_open(PO);
    db[PO].f[0].sort = 0;

    switch (choice) {

	/*
	* sort by Sales Person
	*/
	case 0:    
	    field = 3;
	    break;

	/*
	* sort by Client
	*/
	case 1:    
	    field = 20;
	    break;

	/*
	* sort by Vendor
	*/
	case 2:    
	    field = 2;
	    break;

	/*
	* sort by State
	*/
	case 3:
	    field = 21;
	    break;

	/*
	* sort by Status
	*/
	case 4:
	    field = 22;
	    break;
    }

    old = db[PO].f[field].sort;
    db[PO].f[field].sort = 3;
    db[PO].fields += 3; /* was +=2 before status */
    s_modnrecs(PO, 0);

    handle = FileOpen(fname);
    FileRead(handle, &m, 2);

	/* Ask whether to write to the printer directly or to the screen */
    ans_screen = message(M_SCR|M_KEY, "Send report to printer or display on screen? (P/s)");
	if(ans_screen != 'S' && ans_screen != 's')
	{
		num_line_page = 58;
		ans_screen = 0;
	}
	else
	{
		num_line_page = 999999; /* Kludge: this will cheat the system to not put a "new page" to the file */
		ans_screen = 1;
		/* create the temp file */
		if ((printer = fileopn(FileTempName(fname1), "wb")) == NULL)
		{
			message(M_SCR|M_ERR|M_KEY, "Unable to create temporary file: %s - hit any key", fname1);
			goto cleanup;
		}
	}

    if ( ans_screen || (SelectPrinter() && printercheck() != -1) ) {

	/* Can ONLY send report to printer in condensed mode */
		if(!ans_screen)
		{
		printer = defPrinter;
//printer = fopen("c:\\temp\\temp.txt", "wt");
			/* Turn on Condensed mode. */
			fprintf(defPrinter, "%s", prnCodes.cond);
		}

	message(M_SCR, "Working. . .");
	StrDate(todaysdate, &today);
	
	do {
	    for (i=0; i<m; i++) {
		FileRead(handle, &po, sizeof(POREC));
		
		if(sp_date[0] == 'S' || sp_date[0] == 's')
		{
		    strcpy(tmp_date, po.shipdate);
		    strcpy(print_date, po.shipdate);
		}
		else if(sp_date[0] == 'O' || sp_date[0] == 'o')
		{    
		    strcpy(tmp_date, po.podate);
		    strcpy(print_date, po.podate);
		}
		else
		{
		    strcpy(tmp_date, (po.status[0]=='S' ? po.shipdate:po.podate));
		    if(sts[0] == 'S' || sts[0] == 's')
			strcpy(print_date, po.shipdate);
		    else 
			strcpy(print_date, po.podate);
		}

		if (!strcheck(client.code, po.client_code) && !strcheck(sp.code,po.spcode) && strdatecmp(tmp_date,sdate)>=0 && strdatecmp(tmp_date,edate)<=0 && !strcheck(state.code,po.state) && !strcheck(sts, po.status) && !strcheck(vendor.code, po.vdcode)) {
		    if (esccheck(NULL)) {                                                                                                 
			endthis = YES;
			break;
		    }

		    if (taxit) {
			if (taxable && (po.tax==0 /* || VD is not taxable */))
			    continue;
			if (!taxable && po.tax!=0)
			    continue;
			po.bid = po.cogs+po.mkup;
		    }
		    strupr(po.state);
		    strcpy(po.release_date, print_date); /* release date is not used in this report */
		    s_write(PO, &po, k, NO);
		    k++;
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

	    if(sum_rpt == 'T' && choice != 2) /* need to figure out # of jobs */
	    {
		if((jbcd_handle = FileCreateTemp(jbcd_name)) == -1)
		{
		    message(M_SCR|M_ERR|M_KEY, "Unable to Open %s - hit any key", jbcd_name);
		    break;
		}
		FileWrite(jbcd_handle, &numJB, 2);
	    }

	    if(sum_rpt == 'D' || sum_rpt == 'T')
	    {
	      /* The following for loop handles both Detailed and summary cases previously,
	       * I changed on 12/20/97 to handle these two cases separately.
	       */
	      /* Added code to handle Total report on 2/8/98 */
	      for (i=0; i<k; i++)
	      {
		if (esccheck(NULL))
		    break;
		
		if (lines == 0 && sum_rpt == 'D') {
		    np++;
		    fprintf(printer, "                                                          %s\n\r\n\r", sysvar.company);
		    if(title_date[0] != '\0')
		    {
			fprintf(printer, "                                                          %s\n\r", title_date);
			lines++;
		    }
		    fprintf(printer, "                                          Sales Analysis Report %s (%s) %s\n\r\n\r", list[choice], sum_rpt=='D'? "Detailed":"Summary", title_by[0]?title_by:" ");
		    fprintf(printer, "%-8s                                                                                                                        P. %2d\n\r\n\r", todaysdate, np);
		    if(ans != 'N') 
			fprintf(printer, "Job Code Job Name                  PO #     Vendor       COGS Client   Markup     Sales   Profit Commission Tax State SP Date  Status\n\r");
		    else
		    {
		      if(ans_actcogs == 'Y') /* actual COGS report */
			fprintf(printer, "Job Code Job Name                  PO #     Vendor       COGS Client   Markup     Sales   Profit Act-COGS Inv-# State SP Date  Status\n\r");
		      else
			fprintf(printer, "Job Code Job Name                  PO #     Vendor       COGS Client   Markup     Sales   Profit Other-Exp. Tax State SP Date  Status\n\r");
		    }
		    fprintf(printer, "_____________________________________________________________________________________________________________________________________\n\r");
		    lines += 9;
		}
		s_read(PO, &po, i);
		numPO++;

		dataptr = (CHAR *)&po;

		/* don't include the last one, which belongs to the next group */
		if(i>0 && stricmp(dataptr+db[PO].f[field].offset, lastptr+db[PO].f[field].offset))
		    numPO--;

		/* figure out the job # based on the temp file holding jb code of PO */
		if(sum_rpt == 'T' && choice != 2)
		{
		    /* don't include the last one, which belongs to the next group */
		    if(i>0 && stricmp(dataptr+db[PO].f[field].offset, lastptr+db[PO].f[field].offset))
			;
		    else
		    {
			FileSeek(jbcd_handle, 0, SEEK_SET);
			FileRead(jbcd_handle, &numJB, 2);
    
			for(vec = 0; vec < numJB; vec++)
			{
			    FileRead(jbcd_handle, savepo.jbcode, 8);
			    savepo.jbcode[8] = '\0';
			    if(!strcmp(savepo.jbcode, po.jbcode)) /* found the same jb code */
				break;
			}
			if(vec >= numJB) /* could not find the same jb code, increase count */
			{
			    FileWrite(jbcd_handle, po.jbcode, 8);
			    numJB++;
			    FileSeek(jbcd_handle, 0, SEEK_SET);
			    FileWrite(jbcd_handle, &numJB, 2);
			}
		    }
		}

		if (i>0 && stricmp(dataptr+db[PO].f[field].offset, lastptr+db[PO].f[field].offset)) {
		    if(sum_rpt=='D') 
		    {
		      if(ans != 'N')
			fprintf(printer, "\n\r%-8s SubTotals:                                %10.2f          %6.2f%10.2f %8.2f %7.2f %8.2f\n\r\n\r", lastptr+db[PO].f[field].offset, subcogs, subcogs==0?0:mark_up/subcogs, total_sale, mark_up, subcom, tax);
		      else
		      {
			if(ans_actcogs == 'Y')
			    fprintf(printer, "\n\r%-8s SubTotals:                                %10.2f          %6.2f%10.2f %8.2f %7.2f\n\r\n\r", lastptr+db[PO].f[field].offset, subcogs, subcogs==0?0:mark_up/subcogs, total_sale, mark_up, subactcogs);
			else
			    fprintf(printer, "\n\r%-8s SubTotals:                                %10.2f          %6.2f%10.2f %8.2f %7.2f %8.2f\n\r\n\r", lastptr+db[PO].f[field].offset, subcogs, subcogs==0?0:mark_up/subcogs, total_sale, mark_up, other_exp, tax);
		      }
		    }
		    lines += 3;
		    tcogs += subcogs;
		    ttsale += total_sale;
		    ttmarkup += mark_up;
		    tt_other_exp += other_exp;
		    tcom += subcom;
		    ttax += tax;
		    tactcogs += subactcogs;

		    if(sum_rpt == 'T')
		    {
			switch (choice) {
		    
			    case 0: /* Sales Person */
				vec = s_getno(SP, 0, lastpo.spcode);
				s_read(SP, &sp, vec);
				strcpy(es.jobname, sp.name);
				break;
		    
			    case 1: /* Client */   
				vec = s_getno(CD, 0, lastpo.client_code);
				s_read(CD, &client, vec);
				strcpy(es.jobname, client.name);
				break;
		    
			    case 2: /* Vendor */   
				vec = s_getno(VD, 0, lastpo.vdcode);
				s_read(VD, &vendor, vec);
				strcpy(es.jobname, vendor.name);
				break;
		    
			    case 3: /* State */
				vec = s_getno(ST, 0, lastpo.state);
				s_read(ST, &state, vec);
				strcpy(es.jobname, state.name);
				break;
		    
			    case 4: /* Status */
				strcpy(es.jobname, "                                ");
				break;
			}

			strcpy(es.jbcode, lastptr+db[PO].f[field].offset);
			es.cogs = subcogs;
			es.bid = total_sale;
			es.profit = mark_up;
			es.mkup = other_exp;
			es.com = subcom;
			es.esttax = tax;
			es.discount = subactcogs;
			es.plvec = numJB;
			es.award = numPO;
			s_write(ES, &es, l, NO);
			l++;

			/* clean the temp jbcd file */
			if(choice != 2)
			{
			    tNUM += numJB;
			    numJB = 1;
			    FileSeek(jbcd_handle, 0, SEEK_SET);
			    FileWrite(jbcd_handle, &numJB, 2);

			    FileWrite(jbcd_handle, po.jbcode, 8);
			}
			else
			{
			    tNUM += numPO;
			    numPO = 1;
			}
		    }

		    subcogs = 0;
		    total_sale = 0;
		    mark_up = 0;
		    other_exp = 0;
		    subcom = 0;
		    tax = 0;
		    subactcogs = 0;
		    m = YES;
		}

		lines++;
    
		if(sum_rpt == 'D')
		{
		  if(ans != 'N')
		    fprintf(printer, "%-8s %-25.25s %-8s %-8s %8.2f %-8s %6.2f %9.2f %8.2f %7.2f %8.2f %-2s %3s %8s %c\n\r", po.jbcode, po.jbname, po.po_number, po.vdcode, po.cogs, po.client_code, 
			po.cogs == 0 ? 0:po.profit/po.cogs, po.bid, po.profit, po.commission, 
			po.tax, po.state, po.spcode, po.release_date, po.status[0]);
		  else
		  {
		    if(ans_actcogs == 'Y')
		      fprintf(printer, "%-8s %-25.25s %-8s %-8s %8.2f %-8s %6.2f %9.2f %8.2f %7.2f %8s %-2s %3s %8s %c\n\r", po.jbcode, po.jbname, po.po_number, po.vdcode, po.cogs, po.client_code, 
			po.cogs == 0 ? 0:po.profit/po.cogs, po.bid, po.profit, po.actcogs, 
			po.inv_number, po.state, po.spcode, po.release_date, po.status[0]);
		    else
		      fprintf(printer, "%-8s %-25.25s %-8s %-8s %8.2f %-8s %6.2f %9.2f %8.2f %7.2f %8.2f %-2s %3s %8s %c\n\r", po.jbcode, po.jbname, po.po_number, po.vdcode, po.cogs, po.client_code, 
			po.cogs == 0 ? 0:po.profit/po.cogs, po.bid, po.profit, (po.status[0] == 'S' ? po.actcogs-po.cogs:0), 
			po.tax, po.state, po.spcode, po.release_date, po.status[0]);
		  }
		}

		min_cogs += po.cogs;
		min_bid += po.bid;
		min_profit += po.profit;
		min_com += po.commission;
		min_other += (po.status[0] == 'S' ? po.actcogs-po.cogs:0); /* other exp */
		min_tax += po.tax;
		min_actcogs += po.actcogs;

		subcogs += po.cogs;
		total_sale += po.bid;  /* Calculate Sub-Totals */
		mark_up += po.profit; /*profit;*/
		other_exp += (po.status[0] == 'S' ? po.actcogs-po.cogs:0); /* other exp */
		subcom += po.commission;
		tax += po.tax;
		subactcogs += po.actcogs;
		lastpo = po;
		lastptr = (CHAR *)&lastpo;

		/* Insert Page Break at 60 lines */
		if (lines >= num_line_page) {
		    lines = 0;
		    if(sum_rpt == 'D') fputc(12, printer);
		}
	      } /* end of for loop */
	    }
	    else 
	    /* Code to handle summary report, added on 12/20/97
	     */
	    {
	      s_read(PO, &po, 0);
	      lastpo = po;
	      lastptr = (CHAR *)&lastpo;

	      while(s_nrecs(PO) > 0)
	      {
		if (esccheck(NULL))
		    break;

		s_read(PO, &savepo, 0);

		for(i = 0; i < s_nrecs(PO); i++)
		{
		    s_read(PO, &po, i);

		    if(strcmp(savepo.jbcode, po.jbcode))
			continue;

		    if (lines == 0) {
			np++;
			fprintf(printer, "                                                          %s\n\r\n\r", sysvar.company);
			if(title_date[0] != '\0')
			{
			    fprintf(printer, "                                                          %s\n\r", title_date);
			    lines++;
			}
			fprintf(printer, "                                          Sales Analysis Report %s (%s) %s\n\r\n\r", list[choice], sum_rpt=='D'? "Detailed":"Summary", title_by[0]?title_by:" ");
			fprintf(printer, "%-8s                                                                                                                        P. %2d\n\r\n\r", todaysdate, np);
			if(ans != 'N') 
			    fprintf(printer, "Job Code Job Name                      COGS Client                     Markup     Sales   Profit Commission Tax State SP Date  Status\n\r");
			else
			{
			  if(ans_actcogs == 'Y') /* actual COGS */
			    fprintf(printer, "Job Code Job Name                      COGS Client                     Markup     Sales   Profit Act-COGS Inv-# State SP Date  Status\n\r");
			  else
			    fprintf(printer, "Job Code Job Name                      COGS Client                     Markup     Sales   Profit Other-Exp. Tax State SP Date  Status\n\r");
			}
			fprintf(printer, "_____________________________________________________________________________________________________________________________________\n\r");
			lines += 9;
		    }

		    dataptr = (CHAR *)&po;
		    if (stricmp(dataptr+db[PO].f[field].offset, lastptr+db[PO].f[field].offset)) {
			if(ans != 'N')
			    fprintf(printer, "\n\r%-8s SubTotals:              %10.2f                            %6.2f%10.2f %8.2f %7.2f %8.2f\n\r\n\r", lastptr+db[PO].f[field].offset, subcogs, subcogs==0?0:mark_up/subcogs, total_sale, mark_up, subcom, tax);
			else
			{
			  if(ans_actcogs == 'Y')
			    fprintf(printer, "\n\r%-8s SubTotals:              %10.2f                            %6.2f%10.2f %8.2f %7.2f\n\r\n\r", lastptr+db[PO].f[field].offset, subcogs, subcogs==0?0:mark_up/subcogs, total_sale, mark_up, subactcogs);
			  else
			    fprintf(printer, "\n\r%-8s SubTotals:              %10.2f                            %6.2f%10.2f %8.2f %7.2f %8.2f\n\r\n\r", lastptr+db[PO].f[field].offset, subcogs, subcogs==0?0:mark_up/subcogs, total_sale, mark_up, other_exp, tax);
			}
			lines += 3;
			tcogs += subcogs;
			ttsale += total_sale;
			ttmarkup += mark_up;
			tt_other_exp += other_exp;
			tcom += subcom;
			ttax += tax;
			tactcogs += subactcogs;
			subcogs = 0;
			total_sale = 0;
			mark_up = 0;
			other_exp = 0;
			subcom = 0;
			tax = 0;
			subactcogs = 0;
			m = YES;
		    }

		    min_cogs += po.cogs;
		    min_bid += po.bid;
		    min_profit += po.profit;
		    min_com += po.commission;
		    min_other += (po.status[0] == 'S' ? po.actcogs-po.cogs:0); /* other exp */
		    min_tax += po.tax;
		    min_actcogs += po.actcogs;
    
		    subcogs += po.cogs;
		    total_sale += po.bid;  /* Calculate Sub-Totals */
		    mark_up += po.profit; /*profit;*/
		    other_exp += (po.status[0] == 'S' ? po.actcogs-po.cogs:0); /* other exp */
		    subcom += po.commission;
		    tax += po.tax;
		    subactcogs += po.actcogs;
		    lastpo = po;
		    lastptr = (CHAR *)&lastpo;

		    s_write(PO, &po, i, TRUE); /* delete it, so it won't be picked next time */
		    i--; /* do not advance i, because we just deleted one record */
		} /* end of for loop */
    
		/* It's time to print one summarized PO record */
		lines++;
    
		s_read(CD, &client, s_getno(CD, 0, savepo.client_code));
		if(ans != 'N')
		  fprintf(printer, "%-8s %-25.25s %8.2f %-26.26s %6.2f %9.2f %8.2f %7.2f %8.2f %-2s %3s %8s %c\n\r", savepo.jbcode, savepo.jbname, min_cogs, client.name, 
		    min_cogs == 0 ? 0:min_profit/min_cogs, min_bid, min_profit, min_com, 
		    min_tax, savepo.state, savepo.spcode, savepo.release_date, savepo.status[0]);
		else
		{
		  if(ans_actcogs == 'Y')
		    fprintf(printer, "%-8s %-25.25s %8.2f %-26.26s %6.2f %9.2f %8.2f %7.2f %8s %-2s %3s %8s %c\n\r", savepo.jbcode, savepo.jbname, min_cogs, client.name, 
			min_cogs == 0 ? 0:min_profit/min_cogs, min_bid, min_profit, min_actcogs, 
			"        ", savepo.state, savepo.spcode, savepo.release_date, savepo.status[0]);
		  else
		    fprintf(printer, "%-8s %-25.25s %8.2f %-26.26s %6.2f %9.2f %8.2f %7.2f %8.2f %-2s %3s %8s %c\n\r", savepo.jbcode, savepo.jbname, min_cogs, client.name, 
			min_cogs == 0 ? 0:min_profit/min_cogs, min_bid, min_profit, (savepo.status[0] == 'S' ? min_other:0), 
			min_tax, savepo.state, savepo.spcode, savepo.release_date, savepo.status[0]);
		}

		min_cogs = min_bid = min_profit = min_com = min_other = min_tax = min_actcogs = 0;

		/* Insert Page Break at 60 lines */
		if (lines >= num_line_page) {
		    lines = 0;
		    fputc(12, printer);
		}
	      } /* end of while loop */
	    }

	    tcogs += subcogs;
	    ttsale += total_sale;
	    ttmarkup += mark_up;
	    tt_other_exp += other_exp;
	    tcom += subcom;
	    ttax += tax;
	    tactcogs += subactcogs;
	    if(sum_rpt == 'D')
	    {
	      if(ans != 'N')
	      {
		    fprintf(printer, "\n\r%-8s SubTotals:                                %10.2f          %6.2f%10.2f %8.2f %7.2f %8.2f\n\r\n\r", lastptr+db[PO].f[field].offset, subcogs, subcogs==0?0:mark_up/subcogs, total_sale, mark_up, subcom, tax);
		    if (m)
			fprintf(printer, "\n\rTotals:                                            %10.2f          %6.2f%10.2f %8.2f %7.2f %8.2f\n\r", tcogs, tcogs==0?0:ttmarkup/tcogs, ttsale, ttmarkup, tcom, ttax);
	      }
	      else
	      {
		if(ans_actcogs == 'Y')
		{
		    fprintf(printer, "\n\r%-8s SubTotals:                                %10.2f          %6.2f%10.2f %8.2f %7.2f\n\r\n\r", lastptr+db[PO].f[field].offset, subcogs, subcogs==0?0:mark_up/subcogs, total_sale, mark_up, subactcogs);
		    if (m)
			fprintf(printer, "\n\rTotals:                                            %10.2f          %6.2f%10.2f %8.2f %7.2f\n\r", tcogs, tcogs==0?0:ttmarkup/tcogs, ttsale, ttmarkup, tactcogs);
		}
		else
		{
		    fprintf(printer, "\n\r%-8s SubTotals:                                %10.2f          %6.2f%10.2f %8.2f %7.2f %8.2f\n\r\n\r", lastptr+db[PO].f[field].offset, subcogs, subcogs==0?0:mark_up/subcogs, total_sale, mark_up, other_exp, tax);
		    if (m)
			fprintf(printer, "\n\rTotals:                                            %10.2f          %6.2f%10.2f %8.2f %7.2f %8.2f\n\r", tcogs, tcogs==0?0:ttmarkup/tcogs, ttsale, ttmarkup, tt_other_exp, ttax);
		}
	      }
	    }
	    else if(sum_rpt == 'S')
	    {
		if(ans != 'N')
		{
		    fprintf(printer, "\n\r%-8s SubTotals:              %10.2f                            %6.2f%10.2f %8.2f %7.2f %8.2f\n\r\n\r", lastptr+db[PO].f[field].offset, subcogs, subcogs==0?0:mark_up/subcogs, total_sale, mark_up, subcom, tax);
		    if (m)
			fprintf(printer, "\n\rTotals:                          %10.2f                            %6.2f%10.2f %8.2f %7.2f %8.2f\n\r", tcogs, tcogs==0?0:ttmarkup/tcogs, ttsale, ttmarkup, tcom, ttax);
		}
		else
		{
		  if(ans_actcogs == 'Y')
		  {
		    fprintf(printer, "\n\r%-8s SubTotals:              %10.2f                            %6.2f%10.2f %8.2f %7.2f\n\r\n\r", lastptr+db[PO].f[field].offset, subcogs, subcogs==0?0:mark_up/subcogs, total_sale, mark_up, subactcogs);
		    if (m)
			fprintf(printer, "\n\rTotals:                          %10.2f                            %6.2f%10.2f %8.2f %7.2f\n\r", tcogs, tcogs==0?0:ttmarkup/tcogs, ttsale, ttmarkup, tactcogs);
		  }
		  else
		  {
		    fprintf(printer, "\n\r%-8s SubTotals:              %10.2f                            %6.2f%10.2f %8.2f %7.2f %8.2f\n\r\n\r", lastptr+db[PO].f[field].offset, subcogs, subcogs==0?0:mark_up/subcogs, total_sale, mark_up, other_exp, tax);
		    if (m)
			fprintf(printer, "\n\rTotals:                          %10.2f                            %6.2f%10.2f %8.2f %7.2f %8.2f\n\r", tcogs, tcogs==0?0:ttmarkup/tcogs, ttsale, ttmarkup, tt_other_exp, ttax);
		  }
		}
	    }
	    else /* Print Total report */
	    {
		switch (choice) {
		   
		    case 0: /* Sales Person */
			vec = s_getno(SP, 0, lastpo.spcode);
			s_read(SP, &sp, vec);
			strcpy(es.jobname, sp.name);
			break;
		    
		    case 1: /* Client */   
			vec = s_getno(CD, 0, lastpo.client_code);
			s_read(CD, &client, vec);
			strcpy(es.jobname, client.name);
			break;
		    
		    case 2: /* Vendor */   
			vec = s_getno(VD, 0, lastpo.vdcode);
			s_read(VD, &vendor, vec);
			strcpy(es.jobname, vendor.name);
			break;
		    
		    case 3: /* State */
			vec = s_getno(ST, 0, lastpo.state);
			s_read(ST, &state, vec);
			strcpy(es.jobname, state.name);
			break;
		    
		    case 4: /* Status */
			strcpy(es.jobname, "                                ");
			break;
		}

		strcpy(es.jbcode, lastptr+db[PO].f[field].offset);
		es.cogs = subcogs;
		es.bid = total_sale;
		es.profit = mark_up;
		es.mkup = other_exp;
		es.com = subcom;
		es.esttax = tax;
		es.discount = subactcogs;
		es.plvec = numJB;
		es.award = numPO;
		s_write(ES, &es, l, NO);
		l++;

		if(choice != 2)
		    tNUM += numJB;
		else
		    tNUM += numPO;

		lines = 0;

	      for (i=0; i<l; i++)
	      {
		if (esccheck(NULL))
		    break;
		
		if (lines == 0) {
		    np++;
		    fprintf(printer, "                                                          %s\n\r\n\r", sysvar.company);
		    if(title_date[0] != '\0')
		    {
			fprintf(printer, "                                                          %s\n\r", title_date);
			lines++;
		    }
		    fprintf(printer, "                                          Sales Analysis Report %s (%s) %s\n\r\n\r", list[choice], "Total", title_by[0]?title_by:" ");
		    fprintf(printer, "%-8s                                                                                                                        P. %2d\n\r\n\r", todaysdate, np);
		    if(ans != 'N') 
			fprintf(printer, "                                                COGS  Markup        Sales      Profit   Commission         Tax");
		    else
		    {
		      if(ans_actcogs == 'Y')
			fprintf(printer, "                                                COGS  Markup        Sales      Profit   Actual COGS           ");
		      else
			fprintf(printer, "                                                COGS  Markup        Sales      Profit   Other-Exp.         Tax");
		    }
		    if(choice != 2)
			fprintf(printer, "   # JB");
		    else
			fprintf(printer, "   # PO");
		    if(sorted != 'N')
			fprintf(printer, "   Rank\n\r");
		    else
			fprintf(printer, "\n\r");
		    fprintf(printer, "_____________________________________________________________________________________________________________________________________\n\r");
		    lines += 9;
		}

		/* print sorted report in reverse order */
		if(sorted != 'N') 
		    s_read(ES, &es, l - 1 - i);
		else
		    s_read(ES, &es, i);

		/* filter out those below limit */
		if(es.bid < limit)
		{
		    oth_cogs += es.cogs;
		    oth_bid += es.bid;
		    oth_profit += es.profit;
		    oth_com += es.com;
		    oth_mkup += es.mkup;
		    oth_tax += es.esttax;
		    oth_actcogs += es.discount;
		    oth_jb += es.plvec;
		    oth_po += es.award;
		}
		else
		{
		    if(sorted != 'N')
		    {
			if(ans != 'N')
			{
			    fprintf(printer, "\n\r%-8s %-32.32s %10.2f  %6.2f   %10.2f  %10.2f   %10.2f    %8.2f   %4d   %4d", es.jbcode, es.jobname, es.cogs, es.cogs==0?0:es.profit/es.cogs, es.bid, es.profit, es.com, es.esttax, (choice!=2 ? es.plvec:es.award), i+1);
			}
			else
			{
			  if(ans_actcogs == 'Y')
			    fprintf(printer, "\n\r%-8s %-32.32s %10.2f  %6.2f   %10.2f  %10.2f   %10.2f    %8s   %4d   %4d", es.jbcode, es.jobname, es.cogs, es.cogs==0?0:es.profit/es.cogs, es.bid, es.profit, es.discount, "        ", (choice!=2 ? es.plvec:es.award), i+1);
			  else
			    fprintf(printer, "\n\r%-8s %-32.32s %10.2f  %6.2f   %10.2f  %10.2f   %10.2f    %8.2f   %4d   %4d", es.jbcode, es.jobname, es.cogs, es.cogs==0?0:es.profit/es.cogs, es.bid, es.profit, es.mkup, es.esttax, (choice!=2 ? es.plvec:es.award), i+1);
			}
		    }
		    else
		    {
			if(ans != 'N')
			{
			    fprintf(printer, "\n\r%-8s %-32.32s %10.2f  %6.2f   %10.2f  %10.2f   %10.2f    %8.2f   %4d", es.jbcode, es.jobname, es.cogs, es.cogs==0?0:es.profit/es.cogs, es.bid, es.profit, es.com, es.esttax, (choice!=2 ? es.plvec:es.award));
			}
			else
			{
			  if(ans_actcogs == 'Y')
			    fprintf(printer, "\n\r%-8s %-32.32s %10.2f  %6.2f   %10.2f  %10.2f   %10.2f    %8s   %4d", es.jbcode, es.jobname, es.cogs, es.cogs==0?0:es.profit/es.cogs, es.bid, es.profit, es.discount, "        ", (choice!=2 ? es.plvec:es.award));
			  else
			    fprintf(printer, "\n\r%-8s %-32.32s %10.2f  %6.2f   %10.2f  %10.2f   %10.2f    %8.2f   %4d", es.jbcode, es.jobname, es.cogs, es.cogs==0?0:es.profit/es.cogs, es.bid, es.profit, es.mkup, es.esttax, (choice!=2 ? es.plvec:es.award));
			}
		    }

		    lines++;
		}

		/* Insert Page Break at 60 lines */
		if (lines >= num_line_page - 1) {
		    lines = 0;
		    fputc(12, printer);
		}
	      } /* end of for loop */

	      /* print other sales below limit */
	      if(oth_cogs != 0 || oth_bid != 0 || oth_profit != 0)
	      {
		if(ans != 'N')
		    fprintf(printer, "\n\r%-8s %-32.32s %10.2f  %6.2f   %10.2f  %10.2f   %10.2f    %8.2f   %4d", "********", "******** ALL OTHERS ********", oth_cogs, oth_cogs==0?0:oth_profit/oth_cogs, oth_bid, oth_profit, oth_com, oth_tax, (choice!=2 ? oth_jb:oth_po));
		else
		{
		  if(ans_actcogs == 'Y')
		    fprintf(printer, "\n\r%-8s %-32.32s %10.2f  %6.2f   %10.2f  %10.2f   %10.2f    %8s   %4d", "********", "******** ALL OTHERS ********", oth_cogs, oth_cogs==0?0:oth_profit/oth_cogs, oth_bid, oth_profit, oth_actcogs, "        ", (choice!=2 ? oth_jb:oth_po));
		  else
		    fprintf(printer, "\n\r%-8s %-32.32s %10.2f  %6.2f   %10.2f  %10.2f   %10.2f    %8.2f   %4d", "********", "******** ALL OTHERS ********", oth_cogs, oth_cogs==0?0:oth_profit/oth_cogs, oth_bid, oth_profit, oth_mkup, oth_tax, (choice!=2 ? oth_jb:oth_po));
		}
	      }

	      if(ans != 'N')
		fprintf(printer, "\n\rTotals:                                   %10.2f  %6.2f   %10.2f  %10.2f   %10.2f    %8.2f   %4d\n\r", tcogs, tcogs==0?0:ttmarkup/tcogs, ttsale, ttmarkup, tcom, ttax, tNUM);
	      else
	      {
		if(ans_actcogs == 'Y')
		    fprintf(printer, "\n\rTotals:                                   %10.2f  %6.2f   %10.2f  %10.2f   %10.2f    %8s   %4d\n\r", tcogs, tcogs==0?0:ttmarkup/tcogs, ttsale, ttmarkup, tactcogs, "        ", tNUM);
		else
		    fprintf(printer, "\n\rTotals:                                   %10.2f  %6.2f   %10.2f  %10.2f   %10.2f    %8.2f   %4d\n\r", tcogs, tcogs==0?0:ttmarkup/tcogs, ttsale, ttmarkup, tt_other_exp, ttax, tNUM);
	      }
	    }

	    if(!ans_screen)
				fputc(12, printer);

	    if(sum_rpt == 'T' && choice != 2) /* need to delete temp file */
	    {
		FileClose(jbcd_handle);
		FileDelete(jbcd_name);
	    }
	} while (0);
//fclose(printer);
		if(!ans_screen)
		{
			/* Turn off Condensed Mode. */
			fprintf(defPrinter, "%s", prnCodes.reset);
		}
		else
		{
	    filecls(printer);
	    TextEditor(fname1, "", "");
	    FileDelete(fname1);
		}
    }

cleanup:
    s_close(PO);
    s_delfile(PO);       /* Delete temp PO file. */

    db[PO].f[0].sort = 1;            /* Put all old info back into db[PO] */
    db[PO].f[field].sort = old;
    db[PO].fields -= 3;             /* was -=2 before sort on status */
    strcpy(db[PO].fname, ftemp);
    strcpy(db[PO].extension, etemp);
    strcpy(db[PO].path, ptemp);

    if(sum_rpt == 'T')
    {
	s_close(ES);
	s_delfile(ES);
    
	db[ES].f[0].sort = esold0; 
	db[ES].f[esfield].sort = esold;
	db[ES].fields -= 2; 
	strcpy(db[ES].fname, esftemp);
	strcpy(db[ES].extension, esetemp);
	strcpy(db[ES].path, esptemp);
    }

    return(0);
}


/*****************************************************************************
		Sales Forcast Report
*****************************************************************************/
INT sales_forcast(VOID)
{
    CHAR sdate[9], todaysdate[9], status[2], sp_date[2], mon_day[2], edate[9], tmp_date[10];
    CHAR *list[]={"For COGS", "For Sales", "For Profit", "For Schedule", NULL};
    INT i,k=0,key=KEY_DOWN,pos=0,test,choice,np=0,lines=0,j,tmon,tday,tyear,ans;
    FLOAT total=0,price=0;
    SPREC sp;
    POREC po;
    VDREC vendor;
    CDREC client;
    monthrec rmonth[12];
    struct date tmp_d;
    FILE *printer;   /* Stream of Output File/Printer/Screen/Fax */
    BOOLEAN sch_flag = FALSE, screen = FALSE;

    SHORT max[12], size;
    CHAR **mm[12];
    CHAR space[] = "          ", fname[60];

    INT sum_rpt, old, m, handle, field;
    CHAR ftemp[9], etemp[4], ptemp[40];
    POREC savepo;
    BOOLEAN print_sum;
    FLOAT prev_price;

    if ((choice=window_chooser(33, 13, list)) == -1)
	return(choice);

    status[0] = '\0';
    mon_day[0] = '\0';
    sdate[0] = '\0';
    sp_date[0] = '\0';

    sp.code[0] = '\0';
    client.code[0] = '\0';
    vendor.code[0] = '\0';

    store();
    setwindow(25, 11, 55, 18, "     Sales Forcast Report", "     Enter Match Criteria", "F1=HELP                F10=DONE", "");
    Print(" Start Date: \n\r SP Code: \n\r Client Code: \n\r Vendor Code: \n\r Status: \n\r Ship Date or PO Date: \n\r Monthly/Daily: ");

    do {
	switch (key) {

	    case KEY_SHFT_TAB:
	    case KEY_UP:
		if (pos == 1)
		    pos = 8;
		pos--;
		break;

	    case KEY_TAB2:
	    case KEY_DOWN:
		if (pos == 7)
		    pos = 0;
		pos++;
		break;

	    case KEY_CTRL_PGUP:
	    case KEY_HOME:
		pos = 1;
		break;

	    case KEY_CTRL_PGDOWN:
	    case KEY_END:
		pos = 7;
		break;

	    case KEY_F10:
		pos = 8;
		break;

	    default:
		if (pos == 7)
		    pos = 0;
		pos++;
		break;
	}

	switch (pos) {

	    /*
	    * Date to start report from
	    */
	    case 1:
		do {
		    key = inpt(14, pos, "", sdate, 8, CT_CHDATE, NO);
		    if (key == KEY_F1)
			StrDate(sdate, &today);
		} while (key == KEY_F1);
		break;

	    /*
	    * Pick Sales Person for Report
	    */
	    case 2:
		do {
		    s_open(SP);
		    key = inpt(11, pos, "", sp.code, 3, SP, NO);
		    if (key == KEY_F1) {
			if ((test=gethelp(SP, s_getno(SP, 0, sp.code), 9, 4, 2-1, 14-1)) != -1)
			    s_read(SP, &sp, test);
		    }
		    s_close(SP);
		} while (key == KEY_F1);
		break;

	    /*
	    * Client Code to match
	    */
	    case 3:
		do {
		    s_open(CD);
		    key = inpt(15, pos, "", client.code, 10, CD, 0);
		    if (key == KEY_F1) {
			if ((test = gethelp(CD, s_getno(CD, 0, client.code), 9, 4, 2-1, 14-1)) != -1)
			    s_read(CD, &client, test);
		    }
		    s_close(CD);
		} while (key == KEY_F1);
		break;

	    /*
	    * Vendor Code to match
	    */
	    case 4:
		do {
		    s_open(VD);
		    key = inpt(15, pos, "", vendor.code, 8, VD, 0);
		    if (key == KEY_F1) {
			if ((test=gethelp(VD, s_getno(VD, 0, vendor.code), 9, 4, 2-1, 14-1)) != -1)
			    s_read(VD, &vendor, test);
		    }
		    s_close(VD);
		} while (key == KEY_F1);
		break;

	    case 5:
		key = inpt(10, pos, "", status, 1, CT_CHAR, 0);
		break;

	    /*
	    * Using Ship Date or PO Date
	    */
	    case 6:
		key = inpt(24, pos, "", sp_date, 1, CT_CHAR, NO);
		break;

	    case 7: /* Monthly or Daily */
		key = inpt(17, pos, "", mon_day, 1, CT_CHAR, 0);
		break;

	    /*
	    * Done
	    */
	    case 8:
		break;
	}
    } while (pos != 8);
    restore();

    StrDate(todaysdate, &today);
    if (sdate[0] == '\0')
	strcpy(sdate, todaysdate);
    if (sp.code[0] == '\0')
	strcpy(sp.code, "*");
    if (client.code[0] == '\0')
	strcpy(client.code, "*");
    if (vendor.code[0] == '\0')
	strcpy(vendor.code, "*");
    if (status[0] == '\0')
	strcpy(status, "*");
    else
	strupr(status);
    if (mon_day[0] == '\0')
	strcpy(mon_day, "M");

    /* two branches: one to print monthly report (unchanged),
       the other one is to print daily report */
    if(mon_day[0] != 'M' && mon_day[0] != 'm')
	goto daily_report;

    if (choice == 3)
    {
	message(M_SCR|M_KEY, "'For Schedule' is only for Daily report - hit any key"); 
	return (0);
    }

    pos = message(M_SCR|M_KEY, "Report in compacted format (N/y)?");
    if (pos == 'Y')
	goto compact_report;

    /* summary report? */
retry:
    sum_rpt = message(M_SCR|M_KEY, "Summary report or Detailed report? (S/D)");
    if(sum_rpt != 'S' && sum_rpt != 'D')
	goto retry;

    if (SelectPrinter() && printercheck() != -1) {
	printer = defPrinter;

	/* turn on Condensed Mode and Landscape */
	fprintf(defPrinter, "%s", prnCodes.cond);
	fprintf(defPrinter, "%s", prnCodes.land);

	message(M_SCR, "Working. . .");

	s_close(PO);
	strcpy(ftemp, db[PO].fname);
	strcpy(etemp, db[PO].extension);
	strcpy(ptemp, db[PO].path);
	sprintf(fname, "%s%s.%s", ptemp, ftemp, etemp);

	s_tempfile(PO);

	s_open(PO);
	db[PO].f[0].sort = 0;

	/* which field to sort */
	if(sp_date[0] == 'S' || sp_date[0] == 's')
	    field = 5; /* sort by shipdate */
	else
	    field = 6; /* sort by podate */

//        field = 1; /* sorting by job code never works */
	old = db[PO].f[field].sort;
	db[PO].f[field].sort = 3;
	db[PO].fields += 3;
	s_modnrecs(PO, 0);

	handle = FileOpen(fname);
	FileRead(handle, &m, 2);

	DateStr(&tmp_d, sdate);
	DateAdd(&tmp_d, 365); /* 1 year period */
	StrDate(edate, &tmp_d);
	edate[8] = '\0';

	sscanf(sdate, "%02d/%02d/%02d", &tmon, &tday, &tyear);
	for (i=0; i<=11; i++) {
	    rmonth[i].ttotal = 0;
	    rmonth[i].total[0] = '\0';
	    sprintf(rmonth[i].name, "%c%c%c %02d", month[tmon][0], month[tmon][1], month[tmon][2], tyear);
	    rmonth[i].num = tmon;
	    tmon++;
	    if (tmon == 13) {
		++tyear;
		tmon = 1;
	    }
	}
	
	for (i=0;i<m;i++) {

	    if (esccheck(NULL))
		break;

	    FileRead(handle, &po, sizeof(POREC));
	    if(sp_date[0] == 'S' || sp_date[0] == 's')
		strcpy(tmp_date, po.shipdate);
	    else if(sp_date[0] == 'O' || sp_date[0] == 'o')
		strcpy(tmp_date, po.podate);
	    else
		strcpy(tmp_date, (po.shipdate[0]!='\0' ? po.shipdate:po.podate));

	    if (!strcheck(client.code, po.client_code) && !strcheck(sp.code, po.spcode) && 
		strdatecmp(tmp_date, edate)<0 && strdatecmp(tmp_date, sdate)>=0 && !strcheck(vendor.code, po.vdcode) && !strcheck(status, po.status)) {
		s_write(PO, &po, k, NO);
		k++;
	    }
	}
	FileClose(handle);
	
	for (j=0; j<=11; j++) 
	    rmonth[j].total[0] = '\0';

	if(sum_rpt == 'D')
	{
	    if(k > 0)
		s_read(PO, &savepo, 0);

	    /* The following for loop handles both Detailed and summary cases previously,
	     * however, it was found late that sorting PO by job code never works,
	     * so I have to handle summary case seperately, but this for loop is
	     * not modified accordingly, since we may fix the sorting problem and
	     * reuse this code later. Modified on 12/14/97
	     */
	    for(i = 0; i < k; i++)
	    {
		memcpy(&po, &savepo, sizeof(POREC));
		if(i < k - 1)
		    s_read(PO, &savepo, i+1);
		if(i >= k - 1 || strcmp(po.jbcode, savepo.jbcode))
		    print_sum = TRUE;
		else
			print_sum = FALSE;

		if(sp_date[0] == 'S' || sp_date[0] == 's')
		    strcpy(tmp_date, po.shipdate);
		else if(sp_date[0] == 'O' || sp_date[0] == 'o')
		    strcpy(tmp_date, po.podate);
		else
		    strcpy(tmp_date, (po.shipdate[0]!='\0' ? po.shipdate:po.podate));

		if (lines == 0) {
		    np++;
		    fprintf(printer, "%-8s                                     %s     Sales Forcast Report %s (%s)                                           P. %2d\n\r\n\r",todaysdate,sysvar.company,list[choice],sum_rpt=='D'?"Detailed":"Summary",np);
		    fprintf(printer, "Job Code|Job Name                  |  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s\n\r",\
			rmonth[0].name, rmonth[1].name, rmonth[2].name, rmonth[3].name, rmonth[4].name, rmonth[5].name, rmonth[6].name, rmonth[7].name, rmonth[8].name, rmonth[9].name, rmonth[10].name, rmonth[11].name);
		    fprintf(printer, "________|__________________________|__________|__________|__________|__________|__________|__________|__________|__________|__________|__________|__________|__________\n\r");

		    lines += 5;
		}

		switch (choice) {
		    case 0:
			price += po.cogs;
			break;

		    case 1:
			price += po.bid;
			break;

		    case 2:
			price += po.profit;
			break;
		}
		sscanf(tmp_date, "%02d/%02d/%02d", &tmon, &tday, &tyear);
		for (j=0; j<=11; j++) {
		    if (rmonth[j].num == tmon) {
			if(sum_rpt == 'D')
			    sprintf(rmonth[j].total, "%10.2f", price);
			else
			{
			    prev_price = atof(rmonth[j].total);
			    sprintf(rmonth[j].total, "%10.2f", price+prev_price);
			}
			rmonth[j].ttotal += price;
		    }
		}
		total += price;
		price = 0;
		if(sum_rpt == 'D' || (sum_rpt != 'D' && print_sum))
		{
		    fprintf(printer, "%-8s|%-26.26s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s\n\r",
				     po.jbcode,po.jbname,rmonth[0].total,rmonth[1].total,rmonth[2].total,rmonth[3].total,rmonth[4].total,rmonth[5].total,
				     rmonth[6].total,rmonth[7].total,rmonth[8].total,rmonth[9].total,rmonth[10].total,rmonth[11].total);
		    lines++;

		    for (j=0; j<=11; j++) 
			rmonth[j].total[0] = '\0';
		}

		/*
		* Insert Page Break at 44 lines
		*/
		if (lines > 44) {
		    lines = 0;
		    fputc(12, printer);
		}
	    } /* end of for */
	}
	else
	/* code to handle summary report, added on 12/14/97
	 * if we can fix sorting PO by job code problem, this piece of code
	 * can be removed, and the same function is handled in the above
	 * for loop (also change sorting field to 1
	 */
	{
	    while(s_nrecs(PO) > 0)
	    {
		s_read(PO, &savepo, 0);
		for(i = 0; i < s_nrecs(PO); i++)
		{
		    s_read(PO, &po, i);
		    if(strcmp(savepo.jbcode, po.jbcode))
			continue;

		    if(sp_date[0] == 'S' || sp_date[0] == 's')
			strcpy(tmp_date, po.shipdate);
		    else if(sp_date[0] == 'O' || sp_date[0] == 'o')
			strcpy(tmp_date, po.podate);
		    else
			strcpy(tmp_date, (po.shipdate[0]!='\0' ? po.shipdate:po.podate));
    
		    switch (choice) {
			case 0:
			    price += po.cogs;
			    break;
    
			case 1:
			    price += po.bid;
			    break;
    
			case 2:
			    price += po.profit;
			    break;
		    }
		    sscanf(tmp_date, "%02d/%02d/%02d", &tmon, &tday, &tyear);
		    for (j=0; j<=11; j++) {
			if (rmonth[j].num == tmon) {
			    prev_price = atof(rmonth[j].total);
			    sprintf(rmonth[j].total, "%10.2f", price+prev_price);
			    rmonth[j].ttotal += price;
			}
		    }
		    total += price;
		    price = 0;
		    s_write(PO, &po, i, TRUE); /* delete it, so it won't be picked next time */
		    i--; /* do not advance i, because we just deleted one record */
		} /* end of for */

		/* ready to print one line of info */
		if (lines == 0) {
		    np++;
		    fprintf(printer, "%-8s                                     %s     Sales Forcast Report %s (%s)                                           P. %2d\n\r\n\r",todaysdate,sysvar.company,list[choice],sum_rpt=='D'?"Detailed":"Summary",np);
		    fprintf(printer, "Job Code|Job Name                  |  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s\n\r",\
			rmonth[0].name, rmonth[1].name, rmonth[2].name, rmonth[3].name, rmonth[4].name, rmonth[5].name, rmonth[6].name, rmonth[7].name, rmonth[8].name, rmonth[9].name, rmonth[10].name, rmonth[11].name);
		    fprintf(printer, "________|__________________________|__________|__________|__________|__________|__________|__________|__________|__________|__________|__________|__________|__________\n\r");
    
		    lines += 5;
		}

		fprintf(printer, "%-8s|%-26.26s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s\n\r",
				 savepo.jbcode,savepo.jbname,rmonth[0].total,rmonth[1].total,rmonth[2].total,rmonth[3].total,rmonth[4].total,rmonth[5].total,
				 rmonth[6].total,rmonth[7].total,rmonth[8].total,rmonth[9].total,rmonth[10].total,rmonth[11].total);
		lines++;
  
		for (j=0; j<=11; j++) 
		    rmonth[j].total[0] = '\0';
   
		/*
		* Insert Page Break at 44 lines
		*/
		if (lines > 44) {
		    lines = 0;
		    fputc(12, printer);
		}
	    } /* end of while() loop */
	}

	if (k == 0)
	    message(M_SCR|M_KEY, "No Matching records found - hit any key");
	else {
	    fprintf(printer, "________|__________________________|__________|__________|__________|__________|__________|__________|__________|__________|__________|__________|__________|__________\n\r");
	    fprintf(printer, "Totals                             |%10.2f|%10.2f|%10.2f|%10.2f|%10.2f|%10.2f|%10.2f|%10.2f|%10.2f|%10.2f|%10.2f|%10.2f\n\r", rmonth[0].ttotal, rmonth[1].ttotal, rmonth[2].ttotal, rmonth[3].ttotal, rmonth[4].ttotal, rmonth[5].ttotal,
			     rmonth[6].ttotal, rmonth[7].ttotal, rmonth[8].ttotal, rmonth[9].ttotal, rmonth[10].ttotal, rmonth[11].ttotal);
	    fputc(12, printer);
	}

	/* Turn off Condensed Mode. */
	fprintf(defPrinter, "%s", prnCodes.reset);

	s_close(PO);

	s_delfile(PO);       /* Delete temp PO file. */

	db[PO].f[0].sort = 1;            /* Put all old info back into db[PO] */
	db[PO].f[field].sort = old;
	db[PO].fields -= 3;
	strcpy(db[PO].fname, ftemp);
	strcpy(db[PO].extension, etemp);
	strcpy(db[PO].path, ptemp);
    }

    return(0);

daily_report:

    if(choice == 3)
    {
	sch_flag = TRUE;
	/* ask where to print the report */
	ans = message(M_SCR|M_KEY, "Print report to Screen for editting or to Printer (s/P) ?");
	if(ans == 's' || ans == 'S')
	    screen = TRUE;
    }

    message(M_SCR, "Working. . .");

    if(screen) /* print to screen */
    {
	if((printer = fileopn(FileTempName(fname), "wb")) == NULL)
	{
	    message(M_SCR|M_ERR|M_KEY, "Unable to create temporary file: %s - hit any key", fname);
	    return (0);
	}
    }
    else
    {
	if(!SelectPrinter() || printercheck() == -1)
	    return (0);

	printer = defPrinter;

	/* turn on Condensed Mode */
	fprintf(defPrinter, "%s", prnCodes.cond); 
    }


    /* print the first week */
    k = prepare_daily_report(printer, sdate, status, sp_date,
			     sp, client, vendor, choice, list[choice], 0,
			     sch_flag, screen);

    if (sch_flag) /* for schedule, need to print second week */
    {
	DateStr(&tmp_d, sdate);
	DateAdd(&tmp_d, 7); /* 7 days apart for second week */
	StrDate(sdate, &tmp_d);
	sdate[8] = '\0';
	    
	i = prepare_daily_report(printer, sdate, status, sp_date,
				 sp, client, vendor, choice, list[choice], k,
				 sch_flag, screen);

	if(k == 0 && i == 0)
	    message(M_SCR|M_KEY, "No Matching records found - hit any key");
    }

    if(screen)
    {
	filecls(printer);
	TextEditor(fname, NULL, NULL);
	FileDelete(fname);
    }
    else
    {
	/* Turn off Condensed Mode. */
	fprintf(defPrinter, "%s", prnCodes.reset); 
    }


    return(0);

compact_report:
    for(i = 0; i < 12; i++) max[i] = 0;

    if (SelectPrinter() && printercheck() != -1) {
	printer = defPrinter;

	/* turn on Condensed Mode */
	fprintf(defPrinter, "%s", prnCodes.cond);

	message(M_SCR, "Working. . .");
	DateStr(&tmp_d, sdate);
	DateAdd(&tmp_d, 365); /* one year report */
	StrDate(edate, &tmp_d);
	edate[8] = '\0';

	sscanf(sdate, "%02d/%02d/%02d", &tmon, &tday, &tyear);

	for (i=0; i<12; i++) {
	    rmonth[i].ttotal = 0;
	    rmonth[i].total[0] = '\0';
	    sprintf(rmonth[i].name, "%c%c%c %02d", month[tmon][0], month[tmon][1], month[tmon][2], tyear);
	    rmonth[i].num = tmon;
	    tmon++;
	    if (tmon == 13) {
		++tyear;
		tmon = 1;
	    }
	}
	
	for (i=0;i<s_nrecs(PO);i++) {

	    if (esccheck(NULL))
		break;

	    s_read(PO,&po,i);
	    if(sp_date[0] == 'S' || sp_date[0] == 's')
		strcpy(tmp_date, po.shipdate);
	    else if(sp_date[0] == 'O' || sp_date[0] == 'o')
		strcpy(tmp_date, po.podate);
	    else
		strcpy(tmp_date, (po.shipdate[0]!='\0' ? po.shipdate:po.podate));

	    if (!strcheck(client.code, po.client_code) && !strcheck(sp.code, po.spcode) && 
		strdatecmp(tmp_date, edate)<0 && strdatecmp(tmp_date, sdate)>=0 && !strcheck(vendor.code, po.vdcode) && !strcheck(status, po.status)) {

		switch (choice) {
		    case 0:
			if(po.status[0] == 'O' || po.status[0] == 'o')
			    price += po.cogs;
			else
			    price += po.actcogs;
			break;

		    case 1:
			price += po.bid;
			break;

		    case 2:
			price += po.profit;
			break;
		}
		sscanf(tmp_date, "%02d/%02d/%02d", &tmon, &tday, &tyear);
		for (j=0; j<12; j++) {
		    if (rmonth[j].num == tmon && price != 0) 
			max[j] += 1;
		}
		price = 0;
		k++;
	    }
	}

	if (k == 0)
	{
	    message(M_SCR|M_KEY, "No Matching records found - hit any key");
	    /* Turn off Condensed Mode. */
	    fprintf(defPrinter, "%s", prnCodes.reset);
	    s_close(PO);
	    return (-1);
	}
	       
	/* allocate memory */
	size = max[0];
	for(i = 1; i < 12; i++)
	    if(size < max[i]) size = max[i];
	
	for(i = 0; i < 12; i++)
	{
	    mm[i] = (char **) malloc(size * sizeof(char *));
	    if(!mm[i])
	    {
		message(M_SCR|M_KEY, "No enough memory - hit any key");
		/* Turn off Condensed Mode. */
		fprintf(defPrinter, "%s", prnCodes.reset);
	    
		s_close(PO);
		return (-1);
	    }    
	    memset(mm[i], 0, size * sizeof(char *));
	    for(j = 0; j < max[i]; j++)
	    {
		mm[i][j] = (char *) malloc(11);
		if(!mm[i][j])
		{
		    message(M_SCR|M_KEY, "No enough memory - hit any key");
		    /* Turn off Condensed Mode. */
		    fprintf(defPrinter, "%s", prnCodes.reset);
	    
		    s_close(PO);
		    return (-1);
		}    
	    }
	}
	
	for(i = 0; i < 12; i++) max[i] = 0;

	for (i=0;i<s_nrecs(PO);i++) {

	    if (esccheck(NULL))
		break;

	    s_read(PO,&po,i);
	    if(sp_date[0] == 'S' || sp_date[0] == 's')
		strcpy(tmp_date, po.shipdate);
	    else if(sp_date[0] == 'O' || sp_date[0] == 'o')
		strcpy(tmp_date, po.podate);
	    else
		strcpy(tmp_date, (po.shipdate[0]!='\0' ? po.shipdate:po.podate));

	    if (!strcheck(client.code, po.client_code) && !strcheck(sp.code, po.spcode) && 
		strdatecmp(tmp_date, edate)<0 && strdatecmp(tmp_date, sdate)>=0 && !strcheck(vendor.code, po.vdcode) && !strcheck(status, po.status)) {
		if (lines == 0) {
		    np++;
		    fprintf(printer, "                                                          %s\n\r\n\r",sysvar.company);
		    fprintf(printer, "                                                        Sales Forcast Report %s\n\r\n\r",list[choice]);
		    fprintf(printer, "%-8s                                                                                                                      P. %2d\n\r\n\r",todaysdate,np);
		    fprintf(printer, "|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|  %-8s|\n\r",\
			rmonth[0].name, rmonth[1].name, rmonth[2].name, rmonth[3].name, rmonth[4].name, rmonth[5].name, rmonth[6].name, rmonth[7].name, rmonth[8].name, rmonth[9].name, rmonth[10].name, rmonth[11].name);
		    fprintf(printer, "|__________|__________|__________|__________|__________|__________|__________|__________|__________|__________|__________|__________|\n\r");

		    lines += 9;
		}

		switch (choice) {
		    case 0:
			if(po.status[0] == 'O' || po.status[0] == 'o')
			    price += po.cogs;
			else
			    price += po.actcogs;
			break;

		    case 1:
			price += po.bid;
			break;

		    case 2:
			price += po.profit;
			break;
		}
		sscanf(tmp_date, "%02d/%02d/%02d", &tmon, &tday, &tyear);
		for (j=0; j<12; j++) {
		    rmonth[j].total[0] = '\0';

		    if (rmonth[j].num == tmon && price != 0) {
			sprintf(mm[j][max[j]], "%10.2f", price);
			max[j] += 1;
			rmonth[j].ttotal += price;
		    }
		}
		total += price;
		price = 0;
	    }
	}

	for(i = 0; i < size; i++)
	    for(j = 0; j < 12; j++)
		if(!mm[j][i])
		    mm[j][i] = space;

	for(i = 0; i < size; i++)
	{    
	    fprintf(printer, "|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|\n\r",mm[0][i],mm[1][i],mm[2][i],mm[3][i],mm[4][i],mm[5][i],mm[6][i],mm[7][i],mm[8][i],mm[9][i],mm[10][i],mm[11][i]);
	    lines++;
	    /*
	    * Insert Page Break at 60 lines
	    */
	    if (lines >= 59) {
		lines = 0;
		fputc(12, printer);
	    }
	}
	fprintf(printer, "|__________|__________|__________|__________|__________|__________|__________|__________|__________|__________|__________|__________|\n\r");
	fprintf(printer, "|%10.2f|%10.2f|%10.2f|%10.2f|%10.2f|%10.2f|%10.2f|%10.2f|%10.2f|%10.2f|%10.2f|%10.2f|\n\r", rmonth[0].ttotal, rmonth[1].ttotal, rmonth[2].ttotal, rmonth[3].ttotal, rmonth[4].ttotal, 
	    rmonth[5].ttotal, rmonth[6].ttotal, rmonth[7].ttotal, rmonth[8].ttotal, rmonth[9].ttotal, rmonth[10].ttotal, rmonth[11].ttotal);
	fputc(12, printer);
	
	/* Turn off Condensed Mode. */
	fprintf(defPrinter, "%s", prnCodes.reset);

	s_close(PO);

	/* free the memory */
	for(i = 0; i < 12; i++)
	{
	    for(j = 0; j < max[i]; j++)
		if(mm[i][j] && strcmp(mm[i][j], space))
		    free(mm[i][j]);
	
	    free(mm[i]);
	}
    }

    return(0);
}


/*****************************************************************************
		Job Requisition Report
*****************************************************************************/
INT jb_requisition_report(VOID)
{
    CHAR sdate[10],edate[10],sts[2],sp_date[2],ftemp[9],etemp[4],ptemp[40],fname[60],*dataptr,*lastptr=NULL,todaysdate[9],tmp_date[10];
    CHAR *list[]={"By Sales Person", "By Client", "By Vendor", "By State", "By Status", "By % Complete", NULL};
    INT i,m,k=0,key=KEY_DOWN,pos=0,test,choice,handle,field,np=0,lines=0,old,endthis=NO;
    FLOAT total_sale=0,tax=0,mark_up=0,ttsale=0,ttax=0,ttmarkup=0,subcom=0,tcom=0,tcogs=0,subcogs=0;
    SPREC sp;
    STREC state;
    POREC po, lastpo;
    VDREC vendor;
    CDREC client;
    FILE *printer;   /* Stream of Output File/Printer/Screen/Fax */
    INT ans;
    FLOAT other_exp=0, tt_other_exp=0;
    BOOLEAN otherExp = FALSE;

    INT per, pre_per;
    FLOAT comp, pre_comp, left, sub_comp=0, sub_pre_comp=0, sub_left=0, t_comp=0, t_pre_comp=0, t_left=0;

    /* inits */
    state.code[0]='\0';
    sp.code[0]='\0';
    sts[0]='\0';
    sp_date[0]='\0';
    client.code[0]='\0';
    vendor.code[0]='\0';

    /*
    * choose sort field
    */
    if ((choice = window_chooser(33, 11, list)) == -1)
	return(choice);

    store();
    setwindow(25, 11, 55, 20, "      Requisition Report", "     Enter Match Criteria", "F1=HELP                F10=DONE", "");
    Print("\n\r Start Date: \n\r Ending Date: \n\r SP Code: \n\r State: \n\r Client Code: \n\r Status: \n\r Ship Date or PO Date: \n\r Vendor Code:");

    do {
	switch (key) {

	    case KEY_SHFT_TAB:
	    case KEY_UP:
		if (pos == 1)
		    pos = 9;
		pos--;
		break;

	    case KEY_TAB2:
	    case KEY_DOWN:
		if (pos == 8)
		    pos = 0;
		pos++;
		break;

	    case KEY_CTRL_PGUP:
	    case KEY_HOME:
		pos = 1;
		break;

	    case KEY_CTRL_PGDOWN:
	    case KEY_END:
		pos = 8;
		break;

	    case KEY_F10:
		pos = 9;
		break;

	    default:
		if (pos == 8)
		    pos = 0;
		pos++;
		break;
	}

	switch (pos) {

	    /*
	    * Starting date entry
	    */
	    case 1:
		key = inpt(14, 2, "", sdate, 8, CT_CHDATE, NO);
		break;

	    /*
	    * Ending date entry
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
	    * input State field
	    */
	    case 4:
		do {
		    s_open(ST);
		    key = inpt(9, 5, "", state.code, 2, ST, 0);
		    if (key == KEY_F1)
			if ((test = gethelp(ST, s_getno(ST, 0, state.code), 9, 2, 2-1, 14-1)) != -1)
			    s_read(ST, &state, test);
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
		    if (key == KEY_F1)
			if ((test=gethelp(CD, s_getno(CD, 0, client.code), 9, 4, 2-1, 14-1)) != -1)
			    s_read(CD, &client, test);
		    s_close(CD);
		} while (key == KEY_F1);
		break;

	    /*
	    * Status to match
	    */
	    case 6:
		key = inpt(10, 7, "", sts, 1, CT_CHAR, NO);
		break;

	    /*
	    * Using Ship Date or PO Date
	    */
	    case 7:
		key = inpt(24, 8, "", sp_date, 1, CT_CHAR, NO);
		break;
	    /*
	    * Vendor to match
	    */
	    case 8:
		do {
		    s_open(VD);
		    key = inpt(15, 9, "", vendor.code, 8, VD, 0);
		    if (key == KEY_F1)
			if ((test = gethelp(VD, s_getno(VD, 0, vendor.code), 9, 4, 2-1, 14-1)) != -1)
			    s_read(VD, &vendor, test);
		    s_close(VD);
		} while (key == KEY_F1);
		break;

	    case 9:
		break;
	}
    } while (pos != 9);

    if (sp.code[0] == '\0')
	strcpy(sp.code, "*");
    if (state.code[0] == '\0')
	strcpy(state.code, "*");
    if (sts[0] == '\0')
	strcpy(sts, "*");
    else
	strupr(sts);
    if (client.code[0] == '\0')
	strcpy(client.code, "*");
    if (vendor.code[0] == '\0')
	strcpy(vendor.code, "*");
    restore();

    s_close(PO);
    strcpy(ftemp, db[PO].fname);
    strcpy(etemp, db[PO].extension);
    strcpy(ptemp, db[PO].path);
    sprintf(fname, "%s%s.%s", ptemp, ftemp, etemp);

    s_tempfile(PO);

    s_open(PO);
    db[PO].f[0].sort = 0;

    switch (choice) {

	/*
	* sort by Sales Person
	*/
	case 0:    
	    field = 3;
	    break;

	/*
	* sort by Client
	*/
	case 1:    
	    field = 20;
	    break;

	/*
	* sort by Vendor
	*/
	case 2:    
	    field = 2;
	    break;

	/*
	* sort by State
	*/
	case 3:
	    field = 21;
	    break;

	/*
	* sort by Status
	*/
	case 4:
	    field = 22;
	    break;

	/*
	* soft by JB code for % complete report
	*/
	case 5:
	    field = 1;
	    break;
    }

    old = db[PO].f[field].sort;
    db[PO].f[field].sort = 3;
    db[PO].fields += 3; /* was +=2 before status */
    s_modnrecs(PO, 0);
/* moved to inside for (l=0... loop
    handle = FileOpen(fname);
    FileRead(handle, &m, 2);
*/
    if (!SelectPrinter() || printercheck() == -1) goto cleanup;

    /* Can ONLY send report to printer in condensed mode */
    printer = defPrinter;
//printer = fopen("c:\\temp\\temp.txt", "wt");
    /* Turn on Condensed mode. */
    fprintf(defPrinter, "%s", prnCodes.cond);

    message(M_SCR, "Working. . .");
    StrDate(todaysdate, &today);

    /* The following code is changed on 4/5/00. It used to be inside another for loop under JB.
       However, for this typed report, there is no need to look at JB again, because all the PO
       have to be searched anyway. */        
    handle = FileOpen(fname);
    FileRead(handle, &m, 2);

    for (i=0; i<m; i++) {
	    message(M_SCR, "Working. . . %d\%", (INT)((float)((float)i/m) * 100));

	    if (esccheck(NULL)) {                                                                                                 
		endthis = YES;
		break;
	    }

	    FileRead(handle, &po, sizeof(POREC));
	    
	    if(sp_date[0] == 'S' || sp_date[0] == 's')
		strcpy(tmp_date, po.shipdate);
	    else if(sp_date[0] == 'O' || sp_date[0] == 'o')
		strcpy(tmp_date, po.podate);
	    else
		strcpy(tmp_date, (po.status[0]=='S' ? po.shipdate:po.podate));

	    if (!strcheck(client.code, po.client_code) && !strcheck(sp.code,po.spcode) && strdatecmp(tmp_date,sdate)>=0 && strdatecmp(tmp_date,edate)<=0 && !strcheck(state.code,po.state) && !strcheck(sts, po.status) && !strcheck(vendor.code, po.vdcode)) {
		if (esccheck(NULL)) {                                                                                                 
		    endthis = YES;
		    break;
		}

		strupr(po.state);
		s_write(PO, &po, k, NO);
		k++;

	    }
    } /* for(i=0... */
    FileClose(handle);

    do {
	    if (endthis)
		break;

	    m = NO;

	    if (k == 0) {
		message(M_SCR|M_KEY, "No Matching records found - hit any key");
		break;
	    }

	    /* Handle % complete requisition report differently */
	    if(choice == 5)
	    {
	      for (i=0; i<k; i++)
	      {
		if (esccheck(NULL))
		    break;
		
		if (lines == 0) {
		    fprintf(printer, "                                                          %s\n\r\n\r", sysvar.company);
		    fprintf(printer, "                                                      Requisition Report %s\n\r\n\r", list[choice]);
		    fprintf(printer, "                                                   Period covered    %s -- %s\n\r\n\r", sdate, edate); 
		    fprintf(printer, "%-8s                                                                                                                        P. %2d\n\r\n\r", todaysdate, np);
		    fprintf(printer, "Job Name                                Client     Vendor     % Completion        $ Completed      $ Completed (Prev.)        $ Left\n\r");
		    fprintf(printer, "_____________________________________________________________________________________________________________________________________\n\r");
		    lines += 11;
		}
		s_read(PO, &po, i);

		dataptr = (CHAR *)&po;
		if (i>0 && stricmp(dataptr+db[PO].f[field].offset, lastptr+db[PO].f[field].offset)) {
		    fprintf(printer, "\n\r%-8s   SubTotals:                                                              %10.2f       %10.2f            %10.2f\n\r\n\r", lastptr+db[PO].f[field].offset, sub_comp, sub_pre_comp, sub_left);
		    lines += 3;
		    t_comp += sub_comp;
		    t_pre_comp += sub_pre_comp;
		    t_left += sub_left;
		    sub_comp = 0;
		    sub_pre_comp = 0;
		    sub_left = 0;
		    m = YES;
		}
		lines++;

		per = atoi(po.cur_retanige);
		pre_per = atoi(po.pre_retanige);
		comp = po.bid * per / 100;
		pre_comp = po.bid * pre_per / 100;
		left = po.bid - comp;

		fprintf(printer, "%-32s        %-8s   %-8s        %-3d%%             %9.2f        %9.2f             %9.2f\n\r", po.jbname, po.client_code, po.vdcode, per, comp, pre_comp, left);

		sub_comp += comp;
		sub_pre_comp += pre_comp;
		sub_left += left;

		lastpo = po;
		lastptr = (CHAR *)&lastpo;

		/* Insert Page Break at 60 lines */
		if (lines >= 58) {
		    lines = 0;
		    fputc(12, printer);
		}
	      }
	      
	      t_comp += sub_comp;
	      t_pre_comp += sub_pre_comp;
	      t_left += sub_left;
	      fprintf(printer, "\n\r%-8s   SubTotals:                                                              %10.2f       %10.2f            %10.2f\n\r\n\r", lastptr+db[PO].f[field].offset, sub_comp, sub_pre_comp, sub_left);
	      if (m)
		fprintf(printer, "\n\rTotals:                                                                            %10.2f       %10.2f            %10.2f\n\r", t_comp, t_pre_comp, t_left);
	      fputc(12, printer);

	      break;  /* this will jump out of the do loop */
	    }

	    ans = message(M_SCR|M_KEY, "Is it an other-expense report or commision report (o/C) ?");
	    if(ans == 'O' || ans == 'o')
		otherExp = TRUE;

	    for (i=0; i<k; i++) {

		if (esccheck(NULL))
		    break;
		
		if (lines == 0) {
		    np++;
		    fprintf(printer, "                                                          %s\n\r\n\r", sysvar.company);
		    fprintf(printer, "                                                      Requisition Report %s\n\r\n\r", list[choice]);
		    fprintf(printer, "%-8s                                                                                                                        P. %2d\n\r\n\r", todaysdate, np);
		    if(!otherExp) 
			fprintf(printer, "Job Code Job Name                         PO #     Vendor     COGS   Client     Sales    Profit Commission  Tax State SP Date  Status\n\r");
		    else
			fprintf(printer, "Job Code Job Name                         PO #     Vendor     COGS   Client     Sales    Profit Other-Exp.  Tax State SP Date  Status\n\r");
		    fprintf(printer, "_____________________________________________________________________________________________________________________________________\n\r");
		    lines += 9;
		}
		s_read(PO, &po, i);
		dataptr = (CHAR *)&po;
		if (i>0 && stricmp(dataptr+db[PO].f[field].offset, lastptr+db[PO].f[field].offset)) {
		    fprintf(printer, "\n\r%-8s SubTotals:                                       %10.2f         %10.2f %8.2f %7.2f %8.2f\n\r\n\r", lastptr+db[PO].f[field].offset, subcogs, total_sale, mark_up, (!otherExp ? subcom:other_exp), tax);
		    lines += 3;
		    tcogs += subcogs;
		    ttsale += total_sale;
		    ttmarkup += mark_up;
		    tt_other_exp += other_exp;
		    tcom += subcom;
		    ttax += tax;
		    subcogs = 0;
		    total_sale = 0;
		    mark_up = 0;
		    other_exp = 0;
		    subcom = 0;
		    tax = 0;
		    m = YES;
		}
		lines++;

		fprintf(printer, "%-8s %-32s %-8s %-8s %8.2f %-8s %9.2f %8.2f %7.2f %8.2f %-2s %-3s %8s %c\n\r", po.jbcode, po.jbname, po.po_number, po.vdcode, po.cogs, po.client_code, 
		    po.bid, po.profit, (!otherExp ? po.commission:(po.status[0] == 'S' ? po.actcogs-po.cogs:0)), 
		    po.tax, po.state, po.spcode, (po.status[0]=='S' ? po.shipdate : po.podate), po.status[0]);
		subcogs += po.cogs;
		total_sale += po.bid;  /* Calculate Sub-Totals */
		mark_up += po.profit; /*profit;*/
		other_exp += (po.status[0] == 'S' ? po.actcogs-po.cogs:0); /* other exp */
		subcom += po.commission;
		tax += po.tax;
		lastpo = po;
		lastptr = (CHAR *)&lastpo;

		/* Insert Page Break at 60 lines */
		if (lines >= 58) {
		    lines = 0;
		    fputc(12, printer);
		}
	    }
	    tcogs += subcogs;
	    ttsale += total_sale;
	    ttmarkup += mark_up;
	    tt_other_exp += other_exp;
	    tcom += subcom;
	    ttax += tax;
	    fprintf(printer, "\n\r%-8s SubTotals:                                       %10.2f         %10.2f %8.2f %7.2f %8.2f\n\r\n\r", lastptr+db[PO].f[field].offset, subcogs, total_sale, mark_up, (!otherExp ? subcom:other_exp), tax);
	    if (m)
		fprintf(printer, "\n\rTotals:                                                   %10.2f         %10.2f %8.2f %7.2f %8.2f\n\r", tcogs, ttsale, ttmarkup, (!otherExp ? tcom:tt_other_exp), ttax);
	    fputc(12, printer);

    } while (0);

    /* Turn off Condensed Mode. */
    fprintf(defPrinter, "%s", prnCodes.reset);
//fclose(printer);
    s_close(JB);

cleanup:    
    s_close(PO);
    s_delfile(PO);       /* Delete temp PO file. */

    db[PO].f[0].sort = 1;            /* Put all old info back into db[PO] */
    db[PO].f[field].sort = old;
    db[PO].fields -= 3;             /* was -=2 before sort on status */
    strcpy(db[PO].fname, ftemp);
    strcpy(db[PO].extension, etemp);
    strcpy(db[PO].path, ptemp);

    return(0);
}

/*****************************************************************************
 This is a very nasty function. The reason it is built as a separate function 
 is that it is re-used to print a two week schedule in one page.
 It returns 0 to indicate there is no match, or the line number (for next call).
******************************************************************************/
INT prepare_daily_report(FILE *printer, CHAR* sdate, CHAR* status, CHAR* sp_date,
    SPREC sp, CDREC client, VDREC vendor, INT choice, CHAR* title, INT lines,
    BOOLEAN sch_flag, BOOLEAN screen)
{
    CHAR todaysdate[9], edate[9], tmp_date[10];
    INT i,k=0,np=0,j,tmon,tday,tyear;
    FLOAT total=0,price=0;
    POREC po;
    monthrec rmonth[12];
    CHAR *rweek[7];
    struct date tmp_d;
    CHAR wk_plot[7][11];
    CHAR wk_plot_screen[7][11];
    SHORT wk_total[7];
    BOOLEAN notPrinted = TRUE;
	
	DateStr(&tmp_d, sdate);
	DateAdd(&tmp_d, 7); /* 7 days apart */
	StrDate(edate, &tmp_d);
	edate[8] = '\0';

	StrDate(todaysdate, &today);

	DateStr(&tmp_d, sdate);
	for (i=0; i<=6; i++) {
	    char tmp[9];

	    StrDate(tmp, &tmp_d); tmp[8] = '\0';
	    rweek[i] = DayOfWeek(tmp); 
	    sscanf(tmp, "%02d/%02d/%02d", &tmon, &tday, &tyear);
	    rmonth[i].ttotal = 0;
	    rmonth[i].total[0] = '\0';
	    sprintf(rmonth[i].name, "%c%c%c %02d, %02d", month[tmon][0], month[tmon][1], month[tmon][2], tday, tyear);
	    rmonth[i].num = tday;
	    DateAdd(&tmp_d, 1);
	    wk_plot[i][10] = '\0';
	    wk_plot_screen[i][10] = '\0';
	    wk_total[i] = 0;
	}
	
	for (i=0;i<s_nrecs(PO);i++) {

	    if (esccheck(NULL))
		break;

	    s_read(PO,&po,i);
	    if(sp_date[0] == 'S' || sp_date[0] == 's')
		strcpy(tmp_date, po.shipdate);
	    else if(sp_date[0] == 'O' || sp_date[0] == 'o')
		strcpy(tmp_date, po.podate);
	    else
		strcpy(tmp_date, (po.shipdate[0]!='\0' ? po.shipdate:po.podate));

	    if (!strcheck(client.code, po.client_code) && !strcheck(sp.code, po.spcode) && 
		strdatecmp(tmp_date, edate)<0 && strdatecmp(tmp_date, sdate)>=0 && !strcheck(vendor.code, po.vdcode) && !strcheck(status, po.status)) {
		if (lines == 0) {
		    np++;

		    if(!screen)
			fprintf(printer, "                                                          %s\n\r\n\r",sysvar.company);
		    else
			fprintf(printer, "                           %s\n\r\n\r",sysvar.company);

		    if(!sch_flag)
			fprintf(printer, "                                                          Sales Forcast Report %s\n\r\n\r",title);
		    else
		    {
			if(!screen)
			    fprintf(printer, "                                                          Installation Schedule\n\r\n\r");
			else
			    fprintf(printer, "                           Installation Schedule\n\r\n\r");
		    }

		    if(!screen)
			fprintf(printer, "%-8s                                                                                                                      P. %2d\n\r\n\r",todaysdate,np);
		    else
			fprintf(printer, " %-8.8s                                                                P. %2d\n\r\n\r",todaysdate,np);

		    lines += 6;
		}
		if (notPrinted) /* only need to print the table title once */
		{
		  if(!screen)
		  {
		    fprintf(printer, "\n\r\n\rPO NO   |Job Name                        |%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s\n\r",\
			rmonth[0].name, rmonth[1].name, rmonth[2].name, rmonth[3].name, rmonth[4].name, rmonth[5].name, rmonth[6].name);
		    fprintf(printer, "        |                                |   %-3s    |   %-3s    |   %-3s    |   %-3s    |   %-3s    |   %-3s    |   %-3s    \n\r",\
			rweek[0], rweek[1], rweek[2], rweek[3], rweek[4], rweek[5], rweek[6]);
		    fprintf(printer, "________|________________________________|__________|__________|__________|__________|__________|__________|__________\n\r");
		  }
		  else
		  {
		    fprintf(printer, "\n\r\n\r |%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|\n\r",\
			rmonth[0].name, rmonth[1].name, rmonth[2].name, rmonth[3].name, rmonth[4].name, rmonth[5].name, rmonth[6].name);
		    fprintf(printer, " |   %-3s    |   %-3s    |   %-3s    |   %-3s    |   %-3s    |   %-3s    |   %-3s    |\n\r",\
			rweek[0], rweek[1], rweek[2], rweek[3], rweek[4], rweek[5], rweek[6]);
		    fprintf(printer, " |__________|__________|__________|__________|__________|__________|__________|\n\r");
		  }

		  lines += 5;
		  notPrinted = FALSE;
		}

		switch (choice) {
		    case 0:
			price += po.cogs;
			break;

		    case 1:
			price += po.bid;
			break;

		    case 2:
			price += po.profit;
			break;
		    
		    case 3:
			price += po.cogs;
			break;
		}
		sscanf(tmp_date, "%02d/%02d/%02d", &tmon, &tday, &tyear);
		for (j=0; j<=6; j++) {
		    INT c, o;

		    rmonth[j].total[0] = '\0';
/*                    if (po.status[0] == 'S')
			sscanf(po.shipdate, "%02d/%02d/%02d", &tmon, &tday, &tyear);
		    else
			sscanf(po.podate, "%02d/%02d/%02d", &tmon, &tday, &tyear);
*/
		    if (rmonth[j].num == tday) {
			sprintf(rmonth[j].total, "%10.2f", price);
			rmonth[j].ttotal += price;
			if(!screen)
			    sprintf(wk_plot[j], "%s", "   ****   ");
			else
			{
			    sprintf(wk_plot[j], " %s ", po.po_number);
			    sprintf(wk_plot_screen[j], "%-10.10s", po.jbname);                            
			}
			wk_total[j] += 1;
			if(sch_flag) /* need to print **** on more than one day depending on $650/day rule */
			{
			    c = po.cogs / 650;
			    for(o = 1; o <= c && j + o <= 6; o++)
			    {
				if(!screen)
				    sprintf(wk_plot[j + o], "%s", "   ****   ");
				else
				{
				    sprintf(wk_plot[j + o], " %s ", po.po_number);
				    sprintf(wk_plot_screen[j + o], "%-10.10s", po.jbname);
				}

				wk_total[j + o] += 1;
			    }
			    if(c > 0)
				j = j + o - 1;
			}
		    }
		    else
		    {
			strncpy(wk_plot[j], "          ", 10);
			strncpy(wk_plot_screen[j], "          ", 10);
		    }
		}
		total += price;
		price = 0;

		if(!sch_flag)
		    fprintf(printer, "%-8s|%-32s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s\n\r",po.po_number,po.jbname,rmonth[0].total,rmonth[1].total,rmonth[2].total,rmonth[3].total,rmonth[4].total,rmonth[5].total,rmonth[6].total);
		else
		{
		    if(!screen)
			fprintf(printer, "%-8s|%-32s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s\n\r",po.po_number,po.jbname,wk_plot[0],wk_plot[1],wk_plot[2],wk_plot[3],wk_plot[4],wk_plot[5],wk_plot[6]);
		    else
		    {
			fprintf(printer, " |%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|\n\r",wk_plot[0],wk_plot[1],wk_plot[2],wk_plot[3],wk_plot[4],wk_plot[5],wk_plot[6]);
			fprintf(printer, " |%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|%-10s|\n\r",wk_plot_screen[0],wk_plot_screen[1],wk_plot_screen[2],wk_plot_screen[3],wk_plot_screen[4],wk_plot_screen[5],wk_plot_screen[6]);
			fprintf(printer, " |__________|__________|__________|__________|__________|__________|__________|\n\r");
		    }
		}

		lines++;
		k++;

		/*
		* Insert Page Break at 60 lines
		*/
		if (lines >= 59) {
		    lines = 0;
		    if (!screen) /* no need to page break for on screen editor */
			fputc(12, printer);
		}
	    }
	}

	if (!sch_flag && k == 0)
	    message(M_SCR|M_KEY, "No Matching records found - hit any key");
	else if(k > 0) {
	    if(!screen)
		fprintf(printer, "________|________________________________|__________|__________|__________|__________|__________|__________|__________\n\r");

	    if(!sch_flag)
	      fprintf(printer, "Totals                                   |%10.2f|%10.2f|%10.2f|%10.2f|%10.2f|%10.2f|%10.2f\n\r", rmonth[0].ttotal, rmonth[1].ttotal, rmonth[2].ttotal, rmonth[3].ttotal, rmonth[4].ttotal, 
		rmonth[5].ttotal, rmonth[6].ttotal);
	    else
	    {
	      if(!screen)
		fprintf(printer, "Totals                                   |%10i|%10i|%10i|%10i|%10i|%10i|%10i\n\r", wk_total[0], wk_total[1], wk_total[2], wk_total[3], wk_total[4], 
		  wk_total[5], wk_total[6]);
	      else
		fprintf(printer, " |%10i|%10i|%10i|%10i|%10i|%10i|%10i|\n\r", wk_total[0], wk_total[1], wk_total[2], wk_total[3], wk_total[4], 
		  wk_total[5], wk_total[6]);
	    }

//          fputc(12, printer);
	}
	s_close(PO);

	if(k == 0)
	    return (0);
	else
	    return (lines);
}
