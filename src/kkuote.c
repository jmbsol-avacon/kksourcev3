/**
*   KKUOTE.C    - Contains functions that are used by Distributor & Rep
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
#include <dir.h>
#include <dos.h>
#include <string.h>
#include <ctype.h>
#include "db_defs.h"
#include "vmisc.h"
#include "vfile.h"

#include <sys/stat.h>
#include <io.h>
#include <fcntl.h>

CHAR vdlast[9]={"\0"};
CHAR vdlist[MAXVENDORS][9];
INT lastporec;
BOOLEAN tagged[MAXVENDORS];
BOOLEAN tag_allowed[MAXVENDORS];
CHAR lastjob[9]={"00000001"};
ESREC *tempEsPtr;
BOOLEAN switchflg = TRUE;

extern struct date today;
extern USREC curuser;
extern SYSREC sysvar;
extern COMPINFOREC compInfo;
extern CHAR *tempDir;
extern MOUSE_CLICK *points;
extern FILE *defPrinter;
extern PRNCODES prnCodes;
extern BOOLEAN inPO;
extern BOOLEAN changeflg;

/* Globals for this file */
static TXREC *tx = (TXREC *) NULL; /* tax table info */
static CHAR save_vd_in[64]; /* used in xref_help for shift_f1 key */
static INT save_lvec = 0; /* used in xref_help for shift_f1 key */

/* local function prototypes */
VOID put_win(DB_TYPE, CHAR *, INT);
VOID po_print(POREC *, const CHAR *);
VOID po_fax(POREC *, const CHAR *);
VOID set_taxrate(ESREC *, JBREC *);

/**************************************************************************
free_tx () - frees the tx memory, to be called when exits
**************************************************************************/
VOID free_tx(VOID)
{
    if(tx != (TXREC *) NULL)
	free(tx);
}

/**************************************************************************
set_taxrate (ESREC*, JBREC*) - sets the tax rate for ES, need to check taxtable
**************************************************************************/
VOID set_taxrate(ESREC *es, JBREC *jb)
{
/* added the following new implementation:    
   try to read in the tax info from AUTOMAT.TX, if this file doesn't exist,
   fine. If exists, only read it once.
   Then, if the es->taxrate is zero, needs to check tx array to find a match. */
    
    static BOOLEAN flag = FALSE;
    static SHORT num_tx;
    static INT tx_handle;
    SHORT i;

    if(flag == FALSE)
    {
	flag = TRUE;
	tx_handle = handleopn("AUTOMAT.TX", O_BINARY|O_CREAT|O_RDWR, S_IREAD|S_IWRITE);
	if(tx_handle == -1) 
	    return;

	FileRead(tx_handle, &num_tx, sizeof(SHORT));

	tx = (TXREC *) malloc(sizeof(TXREC) * num_tx);
	memset(tx, 0, sizeof(TXREC) * num_tx);
	for(i = 0; i < num_tx; i++)
	    FileRead(tx_handle, &tx[i], sizeof(TXREC));

	handlecls(tx_handle);
    }

    /* stick in taxrate from tax table */
    if(tx != (TXREC *) NULL)
    {
	FLOAT temp_rate;
	SHORT found = 0;
    
	for(i = 0; i < num_tx; i++)
	{
	    if(!strcmpi(jb->state, tx[i].state))
	    {
		temp_rate = tx[i].rate;
		found = 1;
		break;
	    }
	}
	for(; i < num_tx; i++)
	{
	    if(!strcmpi(jb->state, tx[i].state))
	    {
		if(!strcmpi(jb->city, tx[i].city))
		{
		    found = 2;
		    break;
		}
	    }
	    else
		break;
	}
	if(found == 1)
	{
	    es->taxrate = temp_rate;
	}
	else if(found == 2)
	{
	    es->taxrate = tx[i].rate; /* may also need to apply temp_rate */
	}
    }
}


/**************************************************************************
plmessage (message hook) - calls esmessage for PL file
**************************************************************************/
INT plmessage(VOID *ptr)
{
    ptr = ptr;
    return(esmessage(tempEsPtr));
}

/**************************************************************************
esmessage (message hook) - displays message on message bar for ES and PL
    es:     current record
**************************************************************************/
INT esmessage(ESREC *es)
{
    if (es != NULL) {

	if (es->plvec)
	    message(M_SCR, "JOB: %-32s      Part List: %s.%c%c%1.1d",es->jobname,es->jbcode,es->client_code[0],es->client_code[1],es->plvec);
	else
	    message(M_SCR, "JOB: %-32s      No Part List",es->jobname);
    }

    return(0);
}

/**************************************************************************
jb_es (hot key hook) - calls Estimate screen from Job screen
    type:   ident
    c:  cursor struct
    jb: current record
**************************************************************************/
INT jb_es(DB_TYPE type, IN_POS *c, JBREC *jb)
{
    ESREC es;
	CDREC cd;
	INT vec;
	CHAR estname[25];
    
    s_vcheck(type, jb,c->ypos);

    s_open(ES);
    if ((vec = s_getno(ES, 0, jb->code))==-1) {
	s_clearrec(ES, &es);
	strcpy(es.jbcode, jb->code);
	strcpy(es.jobname, jb->name);
	strcpy(es.state, jb->state);
	StrDate(es.estdate, &today);
	es.status[0] = jb->status[0];
	es.status[1] = '\0';
	set_taxrate(&es, jb); 
	s_write(ES, &es, s_nrecs(ES), FALSE);
    }
    else
		s_read(ES, &es, vec);

    db_inpt(ES);

	c->read = YES;
	strcpy(estname, es.estname);
	/* When we finish ES, copy the estname/client-email to job */
	if((vec = s_getno(ES, 0, jb->code)) != -1)
	{
		s_read(ES, &es, vec);
		if(strcmp(estname, es.estname) != 0)	/* Client contact name changed */
		{
			strcpy(jb->agent, es.estname);
			vec = s_getno(CD, 0, es.client_code);
			if(vec != -1)
			{
				s_read(CD, &cd, vec);
				strcpy(jb->pmgremail, cd.email);
			}
			s_close(CD);
			changeflg = TRUE;
			c->read = NO;

			c->x = 4;
			//c->x = 5;
		}
	}

    if (stricmp(jb->code, lastjob))
	c->ypos = s_getno(type, 0, lastjob);
    if (c->ypos == -1)
	c->ypos = 0;
    c->flags = 4;
    c->set = YES; //c->read = YES;
    
    return(0);
}

/**************************************************************************
jb_spupdate - updates SP file after a W status.
    jb: current record
**************************************************************************/
INT jb_spupdate(JBREC *jb)
{
    INT vec;
    ESREC es;
    SPREC sp;

    if ((jb->esvec = s_getno(ES, 0, jb->code)) == -1) {
	message(M_SCR|M_KEY, "There is no Estimate for this job - hit any key");
	*jb->status = 'B';
	return(-1);
    }

    s_read(ES, &es, jb->esvec);

    if ((vec = s_getno(SP, 0, es.spcode))==-1)
	return(0);
    s_read(SP, &sp, vec);
    sp.com_due += es.com;

#ifndef REP
    sp.total_sales += es.bid;   /* Do not execute this line if Rep version */
    sp.ytd_sales += es.bid;
#endif
    s_write(SP, &sp, vec, FALSE);

    return(0);
}

/*****************************************************************************
sp_pay() - Subtracts Commission Paid from Commission Due on F4 in SP Screen
*****************************************************************************/
INT sp_pay(DB_TYPE type, IN_POS *c, SPREC *sp)
{
    FLOAT pay = 0;

    message(M_SCR, NULL);
    if (inpt(1, 25, "Enter ammount paid to this Sales Person: ", &pay, 9, CT_FLOAT, NO) != KEY_F10) {
	sp->com_due -= pay;
	sp->paid += pay;
	changeflg = YES;
	(db[type].set_func)(type, c, sp);
    }
    return(0);
}

/**************************************************************************
jb_delete - Delete all records related to Job file
    type:    ident
    c:   cursor struct
    jb: current record
**************************************************************************/
INT jb_delete(DB_TYPE type, IN_POS *c, JBREC *jb)
{
    INT vecnew=0, rec;
    ESREC tempes, es;
    jobcom comments;
    CHAR pth[60], fileName[60];
    struct find_t ff;
    POREC ps;

    switch (c->x) {

	case KEY_F3: 
	    c->x = 0;
	    if (c->ypos==s_nrecs(type) || s_nrecs(type)==0)
		break;
	    strcpy(es.jbcode, jb->code);
	    if (s_delrec(type, jb, c->ypos) == -1)
		break;

	    if (c->ypos==s_nrecs(type))
		c->ypos--;
	    s_read(type,jb,c->ypos);

	    /* Delete ALL records related to the JB that is deleted. */
	    
	    /* Delete any POs for this Job */
      
	    if (s_nrecs(PO)>0) {
		do {
		    s_read(PO,&ps,vecnew);
		    if (!stricmp(es.jbcode,ps.jbcode)) {
			message(M_SCR, "Deleting Purchase Orders. . .");
			s_write(PO, &ps, vecnew, YES);  /* Delete PO */
		    } else {
			vecnew++;
		    }
		} while (vecnew != s_nrecs(PO));
	    }
	    s_close(PO);

	    /* Delete the Job Comments for this Job */
	    s_close(type);
	    strcpy(db[type].extension, "JC");
	    db[type].fsize=sizeof(jobcom);
	    s_open(type);
	    if ((rec = s_getno(type, 0, es.jbcode)) != -1) {
		message(M_SCR, "Deleting Job Comments. . .");
		memset(&comments, 0, sizeof(jobcom));
		s_recwrite(type, &comments, rec);
	    }
	    s_close(type);
	    strcpy(db[type].extension, "JB");
	    db[type].fsize = sizeof(JBREC);
	    s_open(type);

	    /* Delete all ES records & all Part Lists for this Job */
	    message(M_SCR, "Deleting Quotation. . .");
	    while ((vecnew = s_getno(ES, 0, es.jbcode)) != -1) {
		s_read(ES, &es, vecnew);
		tempes = es;

		/*
		* Del each estimate
		*/
		if (s_write(ES, &tempes, vecnew, YES) == -1) {
		    message(M_SCR|M_ERR|M_KEY, "Cannot Delete %s from ES file", es.jbcode);
		    break;
		}

		/*
		* Delete Part List for this Estimate
		*/
		if (es.plvec) {
		    message(M_SCR, "Deleting Part List. . .");
		    plfile(&es);
		    s_delfile(PL);
		}
	    }

	    /* Delete all RS files for this job (del jbcode.*) */

	    sprintf(pth, "%s%s.*", db[RS].path, es.jbcode);

	    /*if (findfirst(pth, &ff, 0) != -1) {
		message(M_SCR, "Deleting Room Schedule. . .");
		do {
		    FileDelete(ff.ff_name);
		} while (findnext(&ff)!=-1);
	    }*/
	    if (FileFindFirst(pth, 0, &ff)) {
		message(M_SCR, "Deleting Room Schedule. . .");
		do {
		    sprintf(fileName, "%s%s", db[RS].path, ff.name);
		    FileDelete(fileName);
		} while (FileFindNext(&ff));
	    }

	    c->set = YES;

	    break;
    }
    return(0);
}

/****************************************************************************
allcomments - Input screen for Comments.
*****************************************************************************/
INT allcomments(DB_TYPE type, IN_POS *c, VOID *data)
{
    INT choice=1, lastchoice=1, rec, changed=changeflg, tempsize;
    CHAR *cptr, *comments, tempname[9], tempext[4];
    BOOLEAN endflg;
    BOOLEAN shift = FALSE;
    int index;

    /* inits */
    endflg = FALSE;

    s_close(type);
    strcpy(tempname, db[type].fname);
    strcpy(db[type].fname, "AUTOMAT");
    strcpy(tempext, db[type].extension);

    switch (type) {
	case JB:
	case ES:
	    strcpy(db[type].extension, "JC");
	    break;

	case VD:
	case CD:
	case SP:
	    db[type].extension[1] = 'C';
	    break;

	default:
	    break;
    }
    tempsize = db[type].fsize;
    db[type].fsize = sizeof(jobcom);
    s_open(type);

    comments = (CHAR *)s_alloc(sizeof(jobcom));

    if ((rec = s_getno(type, 0, (CHAR *)data)) == -1) {
	memset(comments, 0, sizeof(jobcom));
	cptr = ((CHAR *)comments+0);
	strcpy(cptr, (CHAR *)data);
    } else {
	s_read(type, comments, rec);
    }
    s_close(type);

    store();
    setwindow(2, 15, 79, 23, "                                   COMMENTS", "", "F1=HELP                                                               F10=EXIT", " DATE     COMMENT");
    for (choice=1; choice<=16; choice+=2) {
	Print("%9s  %s\n\r", ((CHAR *)comments+9+((choice+1)/2-1)*75),((CHAR *)comments+18+((choice+1)/2-1)*75));
    }
    cptr = ((CHAR *)comments+9);
    choice = 1;

    do {

	switch (isodd(choice)) {

	    case YES:
		lastchoice = choice;
		cptr = ((CHAR *)comments+9+((lastchoice+1)/2-1)*75);
		do {
		    choice = inpt(2, (lastchoice+1)/2, "", cptr, 8, CT_CHDATE, NO);
		    if (choice == KEY_F1)
			StrDate(cptr, &today);
		} while (choice == KEY_F1);
		break;

	    case NO:
		lastchoice = choice;
		cptr = ((CHAR *)comments+18+(lastchoice/2-1)*75);
		choice = inpt(12, lastchoice/2, "", cptr, 65, CT_CHAR, NO);

		if(lastchoice == 16 && strcmp((CHAR *)comments+18+7*75, "")) /* need to shift comment array and redisplay it */
		{
		    for(index = 1; index <= 14; index+=2)
		    {
			memcpy(comments+9+((index+1)/2-1)*75, comments+9+((index+3)/2-1)*75, 9);
			memcpy(comments+18+((index+1)/2-1)*75, comments+18+((index+3)/2-1)*75, 66);
		    }
		    memset(comments+9+((index+1)/2-1)*75, 0, 9);
		    memset(comments+18+((index+1)/2-1)*75, 0, 66);

		    setwindow(2, 15, 79, 23, "                                   COMMENTS", "", "F1=HELP                                                               F10=EXIT", " DATE     COMMENT");
		    for(index = 1; index <= 16; index+=2)
		    {
			Print("%9s  %s\n\r", ((CHAR *)comments+9+((index+1)/2-1)*75),((CHAR *)comments+18+((index+1)/2-1)*75));
		    }
		    lastchoice = 14;
		}
		break;
	}

	/*
	* Direction
	*/
	switch (choice) {

	    case KEY_DOWN:
		choice = lastchoice+2;
		break;

	    case KEY_RIGHT:
	    case 1:
		choice = lastchoice + 1;
		break;

	    case KEY_UP:
		choice = lastchoice - 2;
		break;

	    case KEY_LEFT:
	    case KEY_SHFT_TAB:
		choice = lastchoice - 1;
		break;

	    case KEY_PGUP:
	    case KEY_CTRL_PGUP:
	    case KEY_HOME:
		choice = 1;
		break;

	    case KEY_END:
	    case KEY_CTRL_PGDOWN:
	    case KEY_PGDOWN:
		choice = 16;
		break;

	    case KEY_F10:
		endflg = TRUE;
		if(lastchoice == 16 && strcmp((CHAR *)comments+18+7*75, ""))
		    shift = TRUE;
		choice = lastchoice;
		break;

	    default:
		choice = lastchoice;
		break;
	}

	if (choice>16) {
	    beep();
	    choice=16;
	}

	if (choice<1) {
	    choice=1;
	    beep();
	}

    } while (!endflg);
    restore();

    if(shift) /* need to shift comment array only */
    {
	for(index = 1; index <= 14; index+=2)
	{
	    memcpy(comments+9+((index+1)/2-1)*75, comments+9+((index+3)/2-1)*75, 9);
	    memcpy(comments+18+((index+1)/2-1)*75, comments+18+((index+3)/2-1)*75, 66);
	    
	}
    }

    s_open(type);
    if (rec==-1)
	rec=0;
    s_write(type, comments, rec, FALSE);
    s_free(comments);
    s_close(type);

    strcpy(db[type].fname, tempname);
    strcpy(db[type].extension, tempext);
    db[type].fsize = tempsize;
    s_open(type);
    changeflg = changed;
#ifndef REP
    if (type == ES) {
	StrDate((CHAR *)data+db[type].f[7].offset, &today);
	changeflg = TRUE;
	s_inset(type, c, data);
    }
#endif
    return(0);
}

/**************************************************************************
jb_stat (hot key hook) - allows user to change status to (W)on, (L)ost,
    type:   ident                                 (S)hipped or (A)warded
    c:      cursor struct
    jb:     current record
***************************************************************************/
INT jb_stat(DB_TYPE type, IN_POS *c, JBREC *jb)
{
    INT choice;
    CHAR *list[]={"Won", "Lost", "Fully Shipped", "Partially Shipped", "Awarded", "Bid", NULL};

    ESREC es;
    INT vec;

    ESREC tmpes1, tmpes2;

    s_open(ES);
    if ((vec = s_getno(ES, 0, jb->code)) == -1) {
	message(M_SCR|M_KEY, "No Estimate exists for this Job - hit any key");
	return(-1);
    }

    /*
    * check to see how many es record(s) are associated with this job.
    * if only one, set the status to W
    */
    s_read(ES, &tmpes1, vec-1);
    s_read(ES, &tmpes2, vec+1);
    if(!strcmp(tmpes1.jbcode, jb->code) || !strcmp(tmpes2.jbcode, jb->code))
    {
	if (jb->status[0] == 'B') 
	{
	    if (message(M_SCR|M_KEY, "Job must first be Awarded from the Quotation Screen.  Go there now? (y/N)") == 'Y') 
	    {
		c->x = KEY_F4;
	    }
	    return(0);
	}
    }
    
    if (jb->status[0] == 'S') {
	message(M_SCR|M_KEY, "Job has already been shipped - hit any key");
	return(0);
    }

    if ((choice = window_chooser(33, 9, list)) == -1)
	return(-1);

    switch (choice) {

	case 0:
	    jb->status[0] = 'W';
	    if (jb_spupdate(jb) == -1)
		return(0);
	    break;

	case 1:
	    jb->status[0] = 'L';
	    if (jb_spupdate(jb) == -1)
		return(0);
	    break;

	case 2:
	    jb->status[0] = 'S';
	    StrDate(jb->sent, &today);
	    break;

	case 3:
	    jb->status[0] = 'P';
	    break;

	case 4:
	    jb->status[0] = 'A';
	    break;
	
	case 5:
	    if(jb->status[0] != 'A')
	    {
		message(M_SCR|M_KEY, "Job must first be Awarded from the Quotation Screen.  - hit any key");
		return (0);
	    }
	    jb->status[0] = 'B';
	    break;
    }
    jb->status[1] = '\0';
    s_write(type, jb, c->ypos, NO);
    c->set = YES;
    changeflg = YES;
    
    /* Update the ES status record */
    s_read(ES, &es, vec);
    es.status[0] = jb->status[0];
    s_write(ES, &es, vec, FALSE);
    s_close(ES);

    return(0);
}

/**************************************************************************
jb_letters (hot key hook) - prints PO Form, Letter, Envelope, or Label
    type:   ident
    c:      cursor struct
    jb:     current record
***************************************************************************/
INT jb_letters(DB_TYPE type, IN_POS *c, JBREC *jb)
{
    SPREC sp;
    CDREC client;
    ESREC es;
    CHAR fname[60], tdate[9], now[9], name[60];
    CHAR *list[]={"Letter", "Label", "Envelope", "Address", "Fax Transmittal", "Letter of Transmittal", "Other Letters", NULL};
    INT choice, i, len;
    FILE *fp;
    BOOLEAN endflg;

    s_vcheck(type, jb, c->ypos);   /* Prompt to save changes */

    if ((choice = window_chooser(33, 9, list)) == -1)
	return(choice);

    s_read(ES, &es, s_getno(ES, 0, jb->code));
    s_read(CD, &client, s_getno(CD, 0, es.client_code));

    if (choice == 5) /* letter of transmittal */
    {
	letter_of_transmittal(jb, &es, &client);
	return (0);
    }
    if (choice == 6) /* other letters */
    {
	other_letters(jb, &es, &client);
	return (0);
    }

    s_read(SP, &sp, s_getno(SP, 0, curuser.spcode));
    StrDate(tdate, &today);

    if ((fp = fileopn(FileTempName(fname), "wb")) == NULL) {
	message(M_SCR|M_ERR|M_KEY, "Unable to create temporary file: %s - hit any key", fname);

    } else {

	endflg = FALSE;
	switch (choice) {

	    case 0:

		strcpy(name, jb->pmgr);
		len = strlen(name);
		for(i = 0; i < len; i++)
		{
		    if(name[i] == ' ')
		    {
			name[i] = '\0';
			break;
		    }
		}
		fprintf(fp, "\n\r\n\r\n\r\n\r");
		fprintf(fp, "\n\r        %s\n\r\n\r", tdate);
		fprintf(fp, "        %s\n\r", jb->pmgr);
		fprintf(fp, "        c/o %s\n\r", client.name);
		fprintf(fp, "        %s\n\r", jb->name);
		fprintf(fp, "        %s\n\r", jb->address);
		fprintf(fp, "        %s, %s %s\n\r", jb->city, jb->state, jb->zip);
		fprintf(fp, "        Phone: %s\n\r", jb->stel);
		fprintf(fp, "        Fax: %s\n\r", jb->sfax);
		fprintf(fp, "\n\r        RE: %s,\n\r", jb->code);
		fprintf(fp, "\n\r        Dear %s,\n\r\n\r", name);
		fprintf(fp, "\n\r\n\r        Sincerely,\n\r");
		fprintf(fp, "\n\r\n\r        %s", sp.name);
		fprintf(fp, "\n\r        %s", sp.title);
		break;

	    /*
	    * Label
	    */
	    case 1:
		if(message(M_KEY|M_SCR, "Print information to label sheet (n/Y) ?") != 'N')
		{
		    if(SelectPrinter() && printercheck() != -1)
		    {
			message(M_KEY|M_SCR, "Please insert the labeling sheet to the printer, and hit any key when ready");
			print_label_to_label_sheet(jb->code, jb->name, jb->address,
						   jb->city, jb->state, jb->zip, NULL);
		    }
		    filecls(fp);
		    FileDelete(fname);
		    return (0);
		}

		fprintf(fp, "\n\r  %s", jb->code);
		fprintf(fp, "\n\r  %s", jb->name);
		fprintf(fp, "\n\r  %s", jb->address);
		fprintf(fp, "\n\r  %s %s, %s\n\r", jb->city, jb->state, jb->zip);
		break;

	    /*
	    * Envelope
	    */
	    case 2:
		if(message(M_KEY|M_SCR, "Print to envelop (n/Y) ?") != 'N')
		{
		    if(SelectPrinter() && printercheck() != -1)
		    {
			message(M_KEY|M_SCR, "Please insert an envelop to the printer, and hit any key when ready");
			print_address_on_envelop(TRUE, client.name, jb->name, jb->address,
						 jb->city, jb->state, jb->zip, jb->pmgr);
		    }
		    filecls(fp);
		    FileDelete(fname);
		    return (0);
		}

		fprintf(fp, "\n\r                             %s", client.name);
		fprintf(fp, "\n\r                             c/o: %s", jb->name);
		fprintf(fp, "\n\r                             %s", jb->address);
		fprintf(fp, "\n\r                             %s %s, %s\n\r", jb->city, jb->state, jb->zip);
		fprintf(fp, "\n\r                             Attention: %s\n\r", jb->pmgr);
		break;

	    /*
	    * Address
	    */
	    case 3:

		fprintf(fp, "\n\r %s", client.name);
		fprintf(fp, "\n\r c/o: %s", jb->name);
		fprintf(fp, "\n\r %s", jb->address);
		fprintf(fp, "\n\r %s %s, %s\n\r", jb->city, jb->state, jb->zip);
		fprintf(fp, "\n\r Attention: %s\n\r", jb->pmgr);
		break;

	    /*
	    * Fax Transmittal
	    */
	    case 4:
		s_read(SP, &sp, s_getno(SP, 0, curuser.spcode));
		StrTimeStamp(now);
		fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r %-6s%-17s%-8s\n\r\n\r\n\r\n\r", "", tdate, now);
		fprintf(fp, "       %-40s%s\n\r\n\r", jb->pmgr, sp.name);
		fprintf(fp, "       %-35s%s\n\r\n\r", client.name, sysvar.company);
		fprintf(fp, "       %-35s%s\n\r\n\r", client.address, jb->name);
		fprintf(fp, "       %-35s%s\n\r\n\r", jb->pmgr, compInfo.fax);
		fprintf(fp, "       %-35s%s\n\r\n\r", jb->sfax, compInfo.tel);
		break;

	    /*
	    * Letter of Transmittal. Can't be here, ..
	    */
	    /* case 5: */

	    /*
	    * Other letters. Can't be here, because this is already handled before
	    */
	    /* case 6: */

	    default:
		endflg = TRUE;
		break;
	}

	filecls(fp);
	if (!endflg) {
	    TextEditor(fname, client.fax, client.email);
	}
	FileDelete(fname);
    }

    return(0);
}

/**************************************************************************
vd_val (validation hook) - checks if VD record is valid
    vd: record to check
**************************************************************************/
INT vd_val(VDREC *vd)
{
    if (vd->taxable[0] == '\0')
	vd->taxable[0] = 'Y';
    vd->taxable[1] = '\0';

    return(1);
}

/**************************************************************************
jb_val (validation hook) - checks if JB record is valid
    jb: record to check
**************************************************************************/
INT jb_val(JBREC *jb)
{
    if (jb->status[0] == '\0')
	jb->status[0] = 'B';
    jb->status[1] = '\0';

    return((*jb->status ? 1:0));
}

/**************************************************************************
jb_help (hot key hook) - displays estimate information for current JB
    type:   ident
    c:      cursor struct
    jb:     current record
**************************************************************************/
INT jb_help(DB_TYPE type, IN_POS *c, JBREC *jb)
{
    ESREC es;
    STREC state;
    INT i, n;
    BOOLEAN foundflg = NO;

    if (c->last > 0) {
//	if (c->last == 8) {
	if (strcmp(db[type].f[c->last].header, "State|") == 0) {
	    if ((i=gethelp(ST, s_getno(ST, 0, jb->state), db[ST].s->num, db[ST].s->fld, db[ST].s->x-1, db[ST].s->y-1)) != -1) {
		s_read(ST, &state, i);
		strcpy(jb->state, state.code);
	    }
	} else {
	    helpmessage(type);
	}
    } else if (*jb->status == 'B') {
	store();
	setwindow(10, 10, 70, 20, "CURRENT BIDS", " ", "Any key for more.", "CD Code  SP  Estimator                          Bid ($)");
	n = s_nrecs(ES);
	for (i=0; i<n; i++) {

	    s_read(ES, &es, i);
	    if (!stricmp(es.jbcode, jb->code)) {
		foundflg = YES;
#ifdef REP
		Print(" %-8s %-3s %-32s\n\r", es.client_code, es.spcode, es.estname);
#else
		Print(" %-8s %-3s %-32s %7.2f\n\r", es.client_code, es.spcode, es.estname, es.bid);
#endif
		if ((i+1)%11 == 0)
		    nextkey();
	    }
	    else if (foundflg) {
		break;
	    }
	}
	if (!foundflg)
	    Print(" No estimates for this job.");
	nextkey(); /* was getch(); */
	restore();
    } else {
	s_read(ES, &es, jb->esvec);
	c->flags = 4;
	if (stricmp(jb->code, es.jbcode)) {
	    foundflg = NO;
	    for (i=0; i<s_nrecs(ES); i++) {
		s_read(ES, &es, i);
		if (!stricmp(jb->code, es.jbcode)) {
		    foundflg = YES;
		    jb->esvec = i;
		    changeflg = YES;
		    break;
		}
	    }

	    if (!foundflg) {
		message(M_SCR|M_KEY, "Estimate does not exist - hit any key");
		*jb->status = 'B';
		return(-1);
	    }
	}
	window(2, 12, 79, 22);
	clrscr();
	Print(" ESTIMATE INFORMATION:");
	window(1, 12, 79, 22);
	s_inset(ES, c, &es);
	window(1, 1, 80, 25);
	c->top = 1;
    }
    return(0);
}

/**************************************************************************
newpl - gets job code and client code from user then opens chosen PL file
**************************************************************************/
INT newpl(DB_TYPE type)
{
    CDREC client;
    ESREC es;

    if (helpinpt(ES, 0, "Enter Job code or F1 for help: ", (CHAR *)&es) == -1)
	return(-1);

    switch (type) {

	case PL:
	    if (helpinpt(CD, 0, "Enter Client code or F1 for help: ",(CHAR *)&client)==-1)
		return(-1);
	    strcpy(es.client_code, client.code);
	    if (openplfile(&es) == -1)
		message(M_SCR|M_KEY, "Part list does not exist for this Estimate - hit any key");
	    break;

	case RS:
	    strcpy(db[RS].fname, es.jbcode);
	    strcpy(db[RS].extension, "001");
	    s_open(RS);
	    break;

	default:
	    break;
    }
    return(0);
}

/**************************************************************************
plmake - finds unique filename for PL related to the given es record.
    loc:    position of current es record.
    es:
**************************************************************************/
INT plmake(INT loc, ESREC *es)
{
    INT i,vec,cpflg=0;
    ESREC e,source;
    CHAR vec_table[10] = { 0,0,0,0,0,0,0,0,0,0 };

    es->plvec = 1;
    vec = s_getno(ES,0,es->jbcode);
    if (vec != -1) {
	/*
	* getno finds a matching record - but not necessarily the first so
	*  we have to look both forward and backward
	*/
	for (i=vec;; i++) {
	    if (loc == i)
		continue;
	    s_read(ES, &e, i);
	    if (stricmp(e.jbcode, es->jbcode))
		break;

	    if (!cpflg) {
		cpflg = 1;
		source = e;
	    }
	    if (*e.client_code == *es->client_code)
		vec_table[e.plvec] = 1;
	}
	for (i=vec-1;; i--) {
	    if (loc == i)
		continue;
	    s_read(ES, &e, i);
	    if (stricmp(e.jbcode, es->jbcode))
		break;

	    if (!cpflg) {
		cpflg = 1;
		source = e;
	    }
	    if (*e.client_code == *es->client_code)
		vec_table[e.plvec] = 1;
	}
	for (i=1; i<10; i++)
	    if (!vec_table[i])
		break;
	es->plvec = i;
	if (cpflg)
	    pl_copy(&source, es);
    }
    return(i);
}

/**************************************************************************
plfile - set current PL file to that pointed to by the es record
**************************************************************************/
VOID plfile(ESREC *es)
{
    s_close(PL);
    strcpy(db[PL].fname, es->jbcode);
    strncpy(db[PL].extension, es->client_code, 2);
    sprintf(db[PL].extension+2, "%1.1d", es->plvec);
}

/**************************************************************************
openplfile - opens PL file pointed to by given es record
**************************************************************************/
INT openplfile(ESREC *es)
{
    if (!es->plvec)
	return(-1);

    plfile(es);
    s_open(PL);
    if (!s_nrecs(PL))
	return(-1);

    return(0);
}

/**************************************************************************
pl_copy - copies the PL file for one ES record to another
**************************************************************************/
VOID pl_copy(ESREC *source, ESREC *dest)
{
    CHAR sfilename[80], dfilename[80];

    sprintf(sfilename, "%s%s.%c%c%1.1d", db[PL].path, source->jbcode, source->client_code[0], source->client_code[1], source->plvec);
    sprintf(dfilename, "%s%s.%c%c%1.1d", db[PL].path, dest->jbcode, dest->client_code[0], dest->client_code[1], dest->plvec);
    openplfile(source);
    openplfile(dest);
    s_close(PL);
    FileCopy(sfilename, dfilename);
}

/**************************************************************************
us_msg (message hook) - prints message bar message for US file
**************************************************************************/
INT us_msg(VOID)
{
    message(M_SCR, "Blank=No access  R=Read only  F=Full access");
    return(0);
}

/**************************************************************************
newfile (called by print menu) - allows user input to choose files
**************************************************************************/
INT newfile(DB_TYPE type)
{
    switch (type) {
	case RS:
	case PL:
	    return(newpl(type));

	case CL:
	case LI:
	case DS:
	case PD:
	    return(pd_new(type));

	default:
	    break;
    }

    return(-1);
}

/****************************************************************************
PDREC xref_help(CHAR *) - Super Help - Finds products from Class x Line
		       Returns the PD chosen.
*****************************************************************************/
PDREC xref_help(CHAR *vd_in, BOOLEAN shift_f1)
{
    INT lvec, file1=0, j=0, it, lv;
    CHAR pth[30], filename[53];
    static LIREC li;
    CLREC cl;
    PDREC pd;

    strcpy(db[LI].fname, vd_in);
    s_open(LI);
    strcpy(db[CL].fname, vd_in);
    s_open(CL);
    strcpy(db[PD].fname, vd_in);
    s_open(PD);

    if ((lv = (INT)(s_nrecs(LI)/2)) <= 0) {
	strcpy(pd.vendor, "NO");
	message(M_SCR|M_KEY, "No Product Line Descriptions Found - hit any key");
	return(pd);
    }

    store();

    /*
    * Allows User to go back & re-pick Line when ESC from Class.
    */
if(shift_f1 == FALSE || strcmp(save_vd_in, vd_in) != 0)
{
    do {
	lv = gethelp(LI, lv, db[LI].s->num, db[LI].s->fld, db[LI].s->x-1, db[LI].s->y-1);
	if (lv == -1)
	    break;
	s_read(LI, &li, lv);
      
	/*
	* Skip Class if Line starts with a 0
	*/
	if (li.line[0] == '0')
	    break;
    } while ((lvec = gethelp(CL, 0, db[CL].s->num, db[CL].s->fld, db[CL].s->x-1, db[CL].s->y-1)) == -1);
}
else
{
    lvec = gethelp(CL, save_lvec, db[CL].s->num, db[CL].s->fld, db[CL].s->x-1, db[CL].s->y-1);
}
    
    restore();

    s_close(LI);
    /* Aborts Command if ESC from both */
    if ((lvec == -1) || (lv == -1)) {
	strcpy(pd.vendor, "NO");
	s_close(CL);
	return(pd);
    }
    if (li.line[0] == '0')
	strcpy(cl.clas, "*");
    else
	s_read(CL, &cl, lvec);
    s_close(CL);

    /* save the current vd_in and lvec for next time use */
    memset(save_vd_in, 0, 64);
    strcpy(save_vd_in, vd_in);
    save_lvec = lvec;

    lv = s_nrecs(PD);

    /*    Make filename random as a function of time  */
    /*    So that no 2 users will open the same temp file */

    it = random_n();
    sprintf(filename, "%s$PDTEMP$.%03d", tempDir, it);

    file1 = FileOpen(filename);
    FileWrite(file1, &lv, sizeof(INT));
    lvec = 0;
    message(M_SCR, "Working . . .");
    while (lvec<lv) {
	s_read(PD, &pd, lvec);
	if (!strcheck(cl.clas, pd.clas) && !strcheck(li.line, pd.line)) {
	    FileWrite(file1, &pd, sizeof(PDREC));
	    ++j;
	}
	lvec++;
    }

    /* j = total number or records found */
    s_close(PD);

    FileSeek(file1, 0, SEEK_SET);
    FileWrite(file1, &j, sizeof(INT));
    FileClose(file1);
    if (!j) {
	message(M_SCR|M_KEY, "No Matching Products Found - hit any key");
	strcpy(pd.vendor, "NO");
	FileDelete(filename);
	return(pd);
    }
    strcpy(pth, db[PD].path);

    strcpy(db[PD].path, tempDir);
    strcpy(db[PD].fname, "$PDTEMP$");
    sprintf(db[PD].extension, "%03d", it);
    s_open(PD);
    lvec = gethelp(PD, (INT)(s_nrecs(PD)/2), db[PD].s->num, db[PD].s->fld, db[PD].s->x-1, db[PD].s->y-1);
    s_read(PD, &pd, lvec);
    s_close(PD);
    s_delfile(PD);
    strcpy(db[PD].fname, vd_in);
   
    strncpy(db[PD].extension, "PD", 3);
    strcpy(db[PD].path, pth);

    return(pd);
}

/**************************************************************************
pd_load (hot key hook) - sets return value to load new PD file
    type:   ident
    c:      cursor struct
    dataptr:current record
**************************************************************************/
INT pd_load(DB_TYPE type, IN_POS *c, CHAR *dataptr)
{
    type = type; dataptr = dataptr;
    c->rc = 1;
    c->x = KEY_F10;
    return(0);
}

/**************************************************************************
pd_mkup (hot key hook) - performs mkup function on PD record
    type:   ident
    c:      cursor struct
    pd:     current record
**************************************************************************/
INT pd_mkup(DB_TYPE type, IN_POS *c, PDREC *pd)
{
    FLOAT mkup = 0;

    type = type;
    if (inpt(1, 25, " Mark up Percent: %", &mkup, 8, CT_FLOAT3, NO) == 1) {
	pd->sales = pd->p1 + pd->p1 * (mkup/100 + pd->rate);
	c->set = YES;
    }
    return(0);
}

/*************************************************************************
vd_funcs (hot key hook) - processes all hot keys for CL,LI, & DS file
    ident:          type
    c:      cursor struct
    vd:     current record
**************************************************************************/
INT vd_funcs(DB_TYPE type, IN_POS *c, VDREC *vd)
{
    CHAR str[3] = {"CL"};
    MOUSE_CLICK *lastP;

    type = CL;

    if (c->x == KEY_F8) {
	type = LI;
	strcpy(str, "LI");
    } else if (c->x == KEY_F4) {
	type = DS;
	strcpy(str, "DS");
    }

    /* save last mouse click points */
    lastP = points;

    /* register mouse click points for this DB_TYPE */
    points = db[type].mousePoints;

    if (*vd->code) {
	strcpy(db[type].fname, vd->code);
	strncpy(db[type].extension, str, 3);
	wininput(type);
    }
    /* restore last mouse click points */
    points = lastP;

    return(0);
}

/*************************************************************************
wininput - Input Window for CL, LI, DS used for a 2 field input
**************************************************************************/
VOID wininput(DB_TYPE type)
{
    IN_POS c;
    INT i,n;
    CHAR temp2[40], ptr[50];
    BOOLEAN endflg;

    /* inits */
    endflg = NO;

    store();
    setwindow(15, 10, db[type].fsize+25, 19, "Generic Input Definitions", "", db[type].s->msg->msg, db[type].title);
    s_open(type);
    c.ypos = c.x = c.last = c.top = c.set = 0;
    c.y = 1;
    put_win(type, ptr, -1);

    do {
	c.flags = NO;
	n = s_nrecs(type);

	switch(c.x) {

	    case 0:
		c.last = 0;
		strcpy(temp2, ptr+db[type].f[0].offset);
		c.x = inpt(2, c.y, "", ptr+db[type].f[0].offset, db[type].f[0].width, db[type].f[0].type, NO);
		if (strcmp(temp2, ptr+db[type].f[0].offset))
		    c.flags = YES;
		break;

	    case 1:
		c.last = 1;
		strcpy(temp2, ptr+db[type].f[1].offset);
		c.x = inpt(5+db[type].f[0].width, c.y, "", ptr+db[type].f[1].offset,db[type].f[1].width, db[type].f[1].type, NO);
		if (strcmp(temp2, ptr+db[type].f[1].offset))
		    c.flags = YES;
		break;

	    /*
	    * Help
	    */
	    case KEY_F1:
		c.x = c.last;
		helpmessage(type);
		break;

	    /*
	    * Insert a Blank Record
	    */
	    case KEY_F2:
		c.x = c.last;
		if (c.ypos < n) {
		    for (i=n-1; i>=c.ypos; i--) {
			s_read(type, ptr, i);
			s_recwrite(type, ptr, i+1);
		    }
		    s_modnrecs(type, ++n);
		    s_clearrec(type, ptr);
		    s_recwrite(type, ptr, c.ypos);
		    c.set = YES;
		}
		break;

	    /*
	    * Delete a Record
	    */
	    case KEY_F3:
		c.x = c.last;
		if (c.ypos == n)
		    break;
		if (message(M_SCR|M_KEY, "Are you sure you want to Delete this record? (y/N)") == 'Y') {
		    s_write(type, ptr, c.ypos, TRUE);
		    if (c.ypos == 0) {
			c.x = KEY_HOME;
			break;
		    }
		    c.set = YES;
		}
		break;

	    /*
	    * Print
	    */
	    case KEY_F5:
		c.x = 0;
		print_whole_file(type, &c, ptr);
		break;

	    /*
	    * Exit
	    */
	    case KEY_F10:
		endflg = YES;
		break;

	    /*
	    * Go to first Record
	    */
	    case KEY_CTRL_PGUP:
	    case KEY_CTRL_HOME:
	    case KEY_HOME:
		c.x = c.last;
		c.y = 1;
		c.ypos = 0;
		c.set = YES;
		break;

	    /*
	    * Go to last Record
	    */
	    case KEY_CTRL_END:
	    case KEY_CTRL_PGDOWN:
	    case KEY_END:
		c.x=c.last;
		if (c.ypos!=n) {
		    c.y=9;
		    c.ypos=n;
		    c.set=YES;
		}
		break;

	    case KEY_RIGHT:
	    case KEY_TAB2:
	    case KEY_SHFT_TAB:
	    case KEY_LEFT:
		c.x=(c.last==0 ? 1:0);
		break;

	    /*
	    * Page Down 9 records
	    */
	    case KEY_PGDOWN:
		if (c.ypos>=n-18) {
		    c.x=KEY_END;
		    break;
		}
		c.ypos+=9;
		c.set=YES;
		c.x=c.last;
		break;

	    /*
	    * Page Up 9 records
	    */
	    case KEY_PGUP:
		if (c.ypos<18) {
		    c.x=KEY_HOME;
		    break;
		}
		c.ypos-=9;
		c.set=YES;
		c.x=c.last;
		break;

	    /*
	    * Move Down 1 record
	    */
	    case KEY_DOWN:
		c.x=c.last;
		if (c.ypos==n)
		    break;
		c.ypos++;
		if (c.y==9) {
		    c.set=YES;
		} else
		    c.y++;
		break;

	    /*
	    * Move Up 1 record
	    */
	    case KEY_UP:
		c.x=c.last;
		if (c.ypos==0)
		    break;
		c.ypos--;
		if (c.y==1) {
		    c.set=YES;
		} else
		    c.y--;
		break;

	    default:
		c.x=c.last;
		beep();
		break;
	}

	if (c.set) {
	    put_win(type,ptr,c.ypos-c.y);
	    c.set=NO;
	}

	if (c.flags) {
	    s_write(type, ptr, c.ypos, FALSE);
	    if (c.ypos == n) {
		put_win(type, ptr, c.ypos-(c.y-1));
		c.x = KEY_END;
		c.y=8;
	    } else if (c.last==0) {
		put_win(type,ptr,c.ypos-c.y);
	    }
	} else {
	    s_read(type,ptr,c.ypos);
	    printxy(2,c.y,db[type].f[0].format,ptr+db[type].f[0].offset);
	    printxy(5+db[type].f[0].width,c.y,db[type].f[1].format,ptr+db[type].f[1].offset);
	}
    } while (!endflg);
    s_close(type);
    restore();
}


VOID put_win(DB_TYPE type, CHAR *ptr, INT l)       /* Used for wininput() only */
{
    INT i;

    for (i=9; i>=1; i--) {
	s_read(type,ptr,i+l);
	printxy(2, i, db[type].f[0].format, ptr+db[type].f[0].offset);
	printxy(5+db[type].f[0].width, i, db[type].f[1].format, ptr+db[type].f[1].offset);
    }
}

/**************************************************************************
pl_funcs (hot key hook) - processes all hot keys for PL file
    type:   ident
    c:      cursor struct
    pl:     current record
**************************************************************************/
INT pl_funcs(DB_TYPE type, IN_POS *c, PLREC *pl)
{
    FIELDPARAMS attr, fp;
    VDREC vdin, vd;
    PLREC temppl;
    PDREC pd;
    CHAR check[20], vdcode[9], *list[]={"List Price", "Dealer Price", "Net Price", "Sales Price",NULL};
    FLOAT tp, tf, discount;
    INT i=0, n, start, vec, choice;
    BOOLEAN shift_f1_key = FALSE;
    INT idx = -1, bg, ed;

    CHAR fname[60], ftemp[9], etemp[4], ptemp[40];
    INT k, num_vd, cnt = 0, handle, ret;

    attr = db[type].f[c->x];

    switch (c->x) {

	/*
	* Vendor Scan to make sure Vendor Exists
	*/
	case 1:
	    /*
	    * Defualt Last VD
	    */
	    if (pl->vdcode[0]=='\0') {
		strcpy(pl->vdcode, vdlast);
	    }
	    c->last = c->x;
	    strcpy(check, pl->vdcode);
	    c->x = inpt(attr.x, c->y, "", pl->vdcode, attr.width, CT_CHAR, NO);
	    strupr(pl->vdcode);
	    if ((vec = s_getno(VD, 0, pl->vdcode)) == -1) {
		message(M_SCR|M_KEY, "Vendor is not defined - hit any key");
		strcpy(pl->vdcode, check);
	    }
	    if (strcmp(check, pl->vdcode)) {
		*pl->pdcode = '\0';
		pl->price = pl->f = 0;
	    }
	    in_printline(type, c->y, c->ypos, pl);
	    break;

	case 2:
	    c->last = c->x;
	    c->x = inpt(attr.x, c->y, "", ((CHAR *)pl + db[type].f[c->x].offset), attr.width-1, attr.type, NO);
	    
	    if (c->x == KEY_F1) {
		if (!pl->vdcode[0]) {
		    c->x = 2;
		} else {
		    c->x = c->last;
		    s_close(PD);
		    s_open(PD);
		    if ((vec = gethelp(PD, s_getno(PD, 0, pl->pdcode), db[PD].s->num, db[PD].s->fld, db[PD].s->x-1, db[PD].s->y-1)) == -1)
			break;
		    s_read(PD, &pd, vec);
		    strcpy(pl->pdcode, pd.code);
		    strcpy(pl->description, pd.description);
		    pl->description[23] = '\0';
		    pl->price = pd.p1;
		    pl->f = pd.rate;
		    pl->weight = pd.weight;

#ifdef REP
		    /* Get Commission Rate from VD if Rep Version */
		    s_read(VD, &vd, s_getno(VD, 0, pl->vdcode));
		    pl->markup = vd.com_rate;
#endif
		    in_printline(type, c->y, c->ypos, pl);
		    s_close(PD);
		}
	    }
	    break;

	/*
	* Switch Between Weight & Rate Shown
	*/
	case KEY_ALT_S:
	    c->x = 0;
	    fp = *(db[type].f+5);
	    *(db[type].f+5) = *(db[type].f+6);
	    *(db[type].f+6) = fp;
	    switchflg=(switchflg ? FALSE:TRUE);
	    c->set = YES;
	    c->flags = 4;
	    break;

	/*
	* Super Help = Class x Line
	*/
	case KEY_SHFT_F1:
	    shift_f1_key = TRUE;
	case KEY_ALT_F1:

	    c->x = c->last;
	    if (*pl->vdcode) {

		if (c->last != 2)
		    break;
		pd = xref_help(pl->vdcode, shift_f1_key);
		if (!strcmp(pd.vendor, "NO"))
		    break;
		strcpy(pl->pdcode, pd.code);
		strcpy(pl->description, pd.description);
		pl->description[23] = '\0';
		pl->price = pd.p1;
		pl->f = pd.rate;
		pl->weight = pd.weight;
		in_printline(type, c->y, c->ypos, pl);
	    }
	    break;

	/*
	* Delete entire vendor
	*/
	case KEY_ALT_F3:
	    if (vd_chooser(vdin.code) == -1)
		break;

	    if (message(M_SCR|M_KEY, "Are you sure you want to delete these vendors? (y/N)") == 'Y') {
		s_lock(type);

		i = 0;
		while (i < s_nrecs(type)) {

		    s_read(type, &temppl, i);

		    if (!strcheck(vdin.code, temppl.vdcode)) {
			s_remove_rec(type, &temppl, i);
		    } else {
			i++;
		    }
		}
		s_unlock(type);
		c->ypos = c->top = 0;
		c->flags = 4;
		c->read = c->set = YES;
	    }
	    break;

	/*
	* Discount a Vendor
	*/
	case KEY_F4:
	    strcpy(vdin.code, (pl->vdcode[0]=='\0' ? "*": pl->vdcode));
	    c->x = discount = 0;
	    if (c->ypos == s_nrecs(type)) {
		if ((idx = vd_chooser(vdin.code)) == -1)
		    break;
	    }
	    if (message(M_SCR|M_KEY, "Use Discount Schedule? (y/N)") != 'Y') {
		inpt(1, 25, " Discount: %", &discount, 8, CT_FLOAT, NO);
		message(M_SCR, NULL);
	    } else {
		if (!strcmp(vdin.code, "*")) {
		    message(M_SCR|M_KEY, "Cannot use Discount Schedule on multiple vendors - hit any key");
		    break;
		}
		discount = newdis(vdin.code);
	    }

	    if (discount != 0) {
		n = s_nrecs(type);
		start = c->ypos;
		s_lock(type);
		s_recwrite(type, pl, start);
		if (start == n)
		    start = 0;

		if(idx == -1)
		    bg = ed = c->ypos;
		else
		    get_pl_pos(idx, vdin.code, &bg, &ed);

		for (i=start; i < (c->ypos == n ? n:start+1); i++) {
		    s_read(type, &temppl, i);
		    if (!strcheck(vdin.code, temppl.vdcode) && i >= bg && i <= ed) {
			temppl.price *= ((FLOAT)1 - discount/100);
			s_recwrite(type, &temppl, i);
		    }
		}
		s_unlock(type);
		c->read = c->set = YES;
	    }
	    break;

	/*
	* Cross Reference
	*/
	case KEY_F7:

	    /* what record are we on */
	    start = c->ypos;

	    /*
	    * as long as there are records to cross reference
	    */
	    if ((n = s_nrecs(type)) != 0) {

		strcpy(vdcode, pl->vdcode);

		/*
		* xref entire PL ?
		*/
		if (start == n) {

		    /* choose from VDs in this PL */
		    if ((idx=vd_chooser(vdcode)) == -1)
			break;
		}

		// only list these VDs with the same product description as the selected one
		s_read(VD, &vdin, s_getno(VD, 0, vdcode));

		s_close(VD);
		strcpy(ftemp, db[VD].fname);
		strcpy(etemp, db[VD].extension);
		strcpy(ptemp, db[VD].path);
		sprintf(fname, "%s%s.%s", ptemp, ftemp, etemp);

		s_tempfile(VD);

		s_open(VD);

		s_modnrecs(VD, 0);

		handle = FileOpen(fname);
		FileRead(handle, &num_vd, 2);
	    
		for(k = 0; k < num_vd; k++)
		{
		    FileRead(handle, &vd, sizeof(VDREC));
		    if(strcmp(vd.product, vdin.product) || !strcmp(vd.code, vdin.code))
			continue;

		    s_write(VD, &vd, cnt++, NO);
		}
		FileClose(handle);

		if(cnt == 0)
		{
		    message(M_SCR|M_KEY, "No matching record found in VENDOR file, use whole VENDOR file - hit any key");

		    // restore VD file
		    s_close(VD);
		    s_delfile(VD);

		    strcpy(db[VD].fname, ftemp);
		    strcpy(db[VD].extension, etemp);
		    strcpy(db[VD].path, ptemp);

		    /* ask user which VD they want to xref to */
		    s_read(VD, &vdin, 0);
		    ret = helpinpt(VD, 0, "Enter VD to Cross Reference to, or F1 for List: ", (CHAR *)&vdin);
		}
		else
		{
		    /* ask user which VD they want to xref to */
		    s_read(VD, &vdin, 0);
		    ret = helpinpt(VD, 0, "Enter VD to Cross Reference to, or F1 for List: ", (CHAR *)&vdin);

		    // restore VD file
		    s_close(VD);
		    s_delfile(VD);

		    strcpy(db[VD].fname, ftemp);
		    strcpy(db[VD].extension, etemp);
		    strcpy(db[VD].path, ptemp);
		}

		if (ret != -1) {

		    /*
		    * just xref this one PL
		    */
		    if (start != n) {
			pl_xref(pl, vdin.code, NO);
			in_printline(type, c->y, c->ypos, pl);

		    /*
		    * xref the entire PL
		    */
		    } else {

			/* lock this PL file */
			s_lock(type);

			if(idx == -1)
			    bg = ed = start;
			else
			    get_pl_pos(idx, vdcode, &bg, &ed);

			/*
			* search each PL record for matching vendors
			*/
			for (i = 0; i < n; i++) {

			    /* read in next PL record */
			    s_read(type, &temppl, i);

			    /*
			    * if it matches the users selection, then xref it
			    */
			    if (!strcheck(vdcode, temppl.vdcode) && i >= bg && i <= ed) {

				/* cross reference this record */
				pl_xref(&temppl, vdin.code, YES);

				if (esccheck(NULL))
				    break;

				s_recwrite(type, &temppl, i);
			    }
			}

			/* unlock this PL file */
			s_unlock(type);

			/* tell db_inpt() to repaint the screen */
			c->read = c->set = YES;
		    }
		}
	    }
	    break;

	/*
	* Show Totals for Vendor(s) in PL
	*/
	case KEY_F8:

	    /*
	    * Only get totals if there are records to search
	    */
	    if (s_nrecs(type)) {

		tp = (FLOAT)c->ypos;
		newpltotals(&tp, &tf);
	    }
	    break;

	/*
	* Get Part Defs from PD file
	*/
	case KEY_F9:
	    if ((n=s_nrecs(type)) == 0)
		break;
	    start = c->ypos;
	    strcpy(vdcode, pl->vdcode);
	    if (start == n) {
		start = 0;
		if ((idx=vd_chooser(vdcode)) == -1)
		    break;
	    }
	    message(M_SCR, NULL);
	    if(idx == -1)
		bg = ed = c->ypos;
	    else
		get_pl_pos(idx, vdcode, &bg, &ed);
	    if ((choice=window_chooser(30, 9, list)) == -1)
		break;
	    for (i = start; i < (c->ypos == n ? n:start+1); i++) {
		s_read(type, &temppl, i);
		if (strcheck(vdcode, temppl.vdcode))
		    continue;
		if (i < bg || i > ed)
		    continue;
		if (pl_get(&temppl, choice) == -1)
		    message(M_SCR, NULL);
		s_recwrite(type, &temppl, i);
		if (esccheck(""))
		    break;
	    }
	    c->read = c->set = YES;
	    message(M_SCR, NULL);
	    break;
    }
    if (*pl->vdcode) {
	strcpy(vdlast, pl->vdcode);
    }
    strcpy(db[PD].fname, vdlast);
    return(0);
}

/**************************************************************************
pl_total - Calculates ALL totals of this PL file - for Price Check ONLY
**************************************************************************/
VOID pl_total(VOID)
{
    FLOAT p, f, m, t, w;
    INT choice, lastchoice;
    BOOLEAN endflg;

    /* inits */
    p = f = m = t = w = 0;
    choice = lastchoice = 0;
    endflg = FALSE;

    store();
    pltotals(&p, &f, &w, "*");
    setwindow(22, 9, 56, 16, "  Enter Markup & Tax Percents to", "  Calculate Totals for this List", "        Hit F10 When Done", "");
    printxy(2, 1, "Markup %               ");
    printxy(2, 2, "Tax %                  ");
    printxy(2, 3, "COGS:                  ");
    printxy(2, 4, "Total Markup:          ");
    printxy(2, 5, "Est. Tax:              ");
    printxy(2, 6, "Est. Shipping:         ");
    printxy(2, 7, "Bid Price:             ");

    while (!endflg) {

	switch(choice) {

	    /*
	    * Get user input for Markup rate
	    */
	    case 0:
		lastchoice = choice;
		choice = inpt(2, 1, "Markup % ", &m, 8, CT_FLOAT3, NO);
		break;

	    /*
	    * Get user input for the tax rate
	    */
	    case 1:
		lastchoice = choice;
		choice = inpt(2, 2, "Tax % ", &t, 8, CT_FLOAT3, NO);
		break;

	    /*
	    * finished
	    */
	    case KEY_F10:
		endflg = TRUE;
		break;
	}
	printxy(2, 3, "COGS:          %8.2f   ", p);
	printxy(2, 4, "Total Markup:  %8.2f   ", p*(m/100));
	printxy(2, 5, "Est. Tax:      %8.2f   ", (p+p*(m/100))*(t/100));
	printxy(2, 6, "Est. Shipping: %8.2f   ", f);
	printxy(2, 7, "Bid Price:     %8.2f   ", p+p*(m/100)+(p+p*(m/100))*(t/100)+f);

	if (choice != KEY_F10)  /* doesn't matter */
	    choice = ((lastchoice==0) ? 1 : 0);
    }
    restore();
}

/**************************************************************************
newpltotals - prints price and freight totals on the message bar (F8)
    p:  price
    f:  freight
**************************************************************************/
INT newpltotals(FLOAT *p, FLOAT *f)
{
    PLREC pl;
    VDREC vdin;
    double tp, tf, tw;
    INT i, n, j;
    INT idx = -1, bg, ed;

    j = *p;

#ifndef REP_VER
    if (tempEsPtr == NULL) {
	pl_total();
#else
    if (FALSE) {
	;
#endif

    } else {

	n = s_nrecs(PL);
	if (j != n) {
	    n = j + 1;

	} else {

	    j=0;

	    /*
	    * Chooses from VDs in PL
	    */
	    if ((idx = vd_chooser(vdin.code)) == -1)
		return(-1);
	}

	if(idx == -1)
	    bg = ed = j;
	else
	    get_pl_pos(idx, vdin.code, &bg, &ed);

	tp = tf = tw = 0;
	for (i=j; i < n; i++) {
	    s_read(PL, &pl, i);
	    if (j == (n-1))
		strcpy(vdin.code, pl.vdcode);
	    if (!strcheck(vdin.code, pl.vdcode) && i >= bg && i <= ed) {

		tp += pl.price * pl.qty;
		tf += pl.f * pl.weight * pl.qty;
		tw += pl.weight * pl.qty;
	    }
	}
	if (switchflg)
	    message(M_SCR|M_KEY, "Total price (%s): $%.2f  Total Weight: %.2f lbs  - hit any key", vdin.code, tp, tw);
	else
	    message(M_SCR|M_KEY, "Total price (%s): $%.2f  Total Shipping: %.2f    - hit any key", vdin.code, tp, tf);

	*p = tp;
	*f = tf;
    }

    return(0);
}


/**************************************************************************
pl_get - gets the PD record pointed to by the given PL record and fills the
    PL structure.
    pl: record to get
    altflg: 0 if F9, 1 for ALT_F9, 2 for SHFT_F9
**************************************************************************/
INT pl_get(PLREC *pl, INT altflg)
{
    INT vec;
    PDREC pd;

#ifdef REP
    VDREC vd;
#endif

    if (*pl->vdcode) {

	strcpy(db[PD].fname, pl->vdcode);
	s_open(PD);

	if (*pl->pdcode) {
	    if ((vec=getno_mess(PD, 0, pl->pdcode)) != -1) {
		changeflg = YES;
		s_read(PD, &pd, vec);
		if (altflg != 4) {
		    strcpy(pl->description, pd.description);
		    pl->description[23] = '\0';
		    strcpy(pl->pdcode, pd.code);
		    pl->f = pd.rate;

		    switch (altflg) {

			/*
			* List Price
			*/
			case 0:
			    pl->price=pd.p1;
			    break;

			/*
			* Dealer Price
			*/
			case 1:
			    pl->price = pd.p2;
			    break;

			/*
			* Net Price
			*/
			case 2:
			    pl->price=pd.p3;
			    break;

			/*
			* Sales Price
			*/
			case 3:
			    pl->price=pd.sales;
			    break;
		    }
		}
		pl->weight=pd.weight;

#ifdef REP
		/* Get Commission Rate from VD if Rep Version */
		s_read(VD,&vd,s_getno(VD,0,pl->vdcode));
		pl->markup=vd.com_rate;
#endif
	    } else {
		return(-1);
	    }
	}
    }

    return(0);
}

/**************************************************************************
newdis - Reads DS file to calculate discount for said vendor.
    vdc:  VD code of Vendor to be discounted
**************************************************************************/
FLOAT newdis(CHAR *vdc)
{
    DDREC dd;
    FLOAT p, from, to;
    INT i, n;

    strcpy(db[DS].fname, vdc);
    pltotals(&p, &from, &to, vdc);
    s_open(DS);
    if ((n=s_nrecs(DS)) == 0) {
	message(M_SCR|M_KEY, "No Discount Schedule for this Vendor - hit any key");
	return(0);
    }

    for (i=0; i<n; i++) {
	s_read(DS, &dd, i);
	sscanf(dd.range, "%f-%f", &from, &to);
	/*
	* Range is inclusive
	*/
	if ((p >= from) && (p <= to))
	    break;
    }

    if (i == n) {
	sscanf(dd.range, "%f-%f", &from, &to);
    }

    s_close(DS);
    message(M_SCR|M_KEY, "Range=%s, Discount=%%%7.4f - hit any key", dd.range, atof(dd.discount));

    return(atof(dd.discount));
}

/**************************************************************************
pl_val (validation hook) - checks if PL record is valid
    pl: record to check
**************************************************************************/
INT pl_val(PLREC *pl)
{
    if (! pl->flg && *pl->pdcode && *pl->vdcode)
	pl->flg = 2;
    else if (pl->flg && !*pl->pdcode || !*pl->vdcode)
	pl->flg = 0;

    return(pl->flg);
}

/**************************************************************************
pl_init (init hook) - sets initial y position on screen to 6
    type            db ident
    c:      cursor struct
    pl:     current record
**************************************************************************/
INT pl_init(DB_TYPE type, IN_POS *c, CHAR *data)
{
    type = type; data = data;
    c->y = 6;
    return(0);
}

/**************************************************************************
pd_xref (hot key hook) - calls xref for PD
    type:   ident
    c:      cursor struct
    pd:     current record
**************************************************************************/
INT pd_xref(DB_TYPE type, IN_POS *c, PDREC *pd)
{
    PLREC pl;
    VDREC vd, vdin;
    CHAR tmpBuf[9];
    BOOLEAN changes;

    CHAR fname[60], ftemp[9], etemp[4], ptemp[40];
    INT k, num_vd, cnt = 0, handle, ret;

    c = c;

    /* save changeflag state */
    changes = changeflg;

    pl.qty = 1;
    strcpy(pl.pdcode, pd->code);
    strcpy(pl.vdcode, db[type].fname);
    strcpy(tmpBuf, db[type].fname);

// only list these VDs with the same product description as the selected one
    s_read(VD, &vdin, s_getno(VD, 0, pl.vdcode));

    s_close(VD);
    strcpy(ftemp, db[VD].fname);
    strcpy(etemp, db[VD].extension);
    strcpy(ptemp, db[VD].path);
    sprintf(fname, "%s%s.%s", ptemp, ftemp, etemp);

    s_tempfile(VD);

    s_open(VD);

    s_modnrecs(VD, 0);

    handle = FileOpen(fname);
    FileRead(handle, &num_vd, 2);

    for(k = 0; k < num_vd; k++)
    {
	FileRead(handle, &vd, sizeof(VDREC));
	if(strcmp(vd.product, vdin.product) || !strcmp(vd.code, vdin.code))
	    continue;

	s_write(VD, &vd, cnt++, NO);
    }
    FileClose(handle);

    if(cnt == 0)
    {
	message(M_SCR|M_KEY, "No matching record found in VENDOR file, use whole VENDOR file - hit any key");

	// restore VD file
	s_close(VD);
	s_delfile(VD);

	strcpy(db[VD].fname, ftemp);
	strcpy(db[VD].extension, etemp);
	strcpy(db[VD].path, ptemp);
	ret = helpinpt(VD, 0, "Enter VD to Cross Reference to, or F1 for List: ", (CHAR *)&vd);
    }
    else
    {
	/*
	* choose VD to cross reference to
	*/
	ret = helpinpt(VD, 0, "Enter VD to Cross Reference to, or F1 for List: ", (CHAR *)&vd);

	// restore VD file
	s_close(VD);
	s_delfile(VD);

	strcpy(db[VD].fname, ftemp);
	strcpy(db[VD].extension, etemp);
	strcpy(db[VD].path, ptemp);
    }

    if (ret != -1) {
	pl_xref(&pl, vd.code, NO);
	s_close(type);
	strcpy(db[type].fname, tmpBuf);
	s_open(type);
    }

    /* restore changeflag state */
    changeflg = changes;

    return(0);
}

/**************************************************************************
pd_new (hot key hook) - Queries user for vendor and loads PD file
    type:   ident
**************************************************************************/
INT pd_new(DB_TYPE type)
{
    INT vec;
    VDREC vd;

    s_close(type);
    s_open(VD);

    if (!s_nrecs(VD)) {
	message(M_SCR|M_KEY, "No vendors are defined - hit any key");
	return(-1);
    }
    s_read(VD, &vd, 0);
    textattr(REVERSE);
    vec = helpinpt(VD, 0, "Enter VD code, or F1 for List: ", (CHAR *)&vd);
    if ((vec == -1) || (vec == -4))
	return(-1);
    strcpy(db[PD].fname, vd.code);
    strcpy(db[CL].fname, vd.code);
    strcpy(db[LI].fname, vd.code);
    strcpy(db[DS].fname, vd.code);
    s_open(type);
    return(0);
}

/**************************************************************************
pl_xref - performs cross-reference function
    pl         : PL record to cross-reference
    vdcode     : code of VD to search
    autoReplace: TRUE to automatically replace PL record
**************************************************************************/
INT pl_xref(PLREC *pl, CHAR *vdcode, BOOLEAN autoReplace)
{
    STATIC MOUSE_CLICK xrefP[]= {
	{152, 128, 216, 128, 'R'},
	{232, 128, 272, 128, 'N'},
	{0, 0, 0, 0, 0}
    };
    PDREC pd, oldpd;
    CHAR tempname[9];
    MOUSE_CLICK *lastP;
    INT i, vec;
    BOOLEAN change;

    strcpy(tempname, db[PD].fname);

    /* save previous changeflg state */
    change = changeflg;

    /*
    * open PD file for current pl item
    */
    strcpy(db[PD].fname, pl->vdcode);
    s_open(PD);

    /*
    * get pd record
    */
    if ((vec = s_getno(PD, 0, pl->pdcode)) != -1) {

	s_read(PD, &oldpd, vec);
	s_close(PD);

	/* This fixes the bug, where asi 002 product is always displayed
	   for xref with NULL ref field */
	if(oldpd.ref == NULL || !strcmp(oldpd.ref, ""))
	{
	    message(M_SCR|M_KEY, "No cross reference is available - hit any key");
	    return(0);
	}

	/* open xref PD file */
	strcpy(db[PD].fname, vdcode);
	s_open(PD);

	/*
	* search thru each PD record for a cross-reference
	*/
	for (i=0; i < s_nrecs(PD); i++) {
	    s_read(PD, &pd, i);

	    if (!stricmp(pd.ref, oldpd.ref)) {

		/*
		* Replace automatically?
		*/
		if (autoReplace) {
		    vec = 'R';

		/*
		* prompt user to replace, or goto next
		*/
		} else {
		    store();
		    lastP = points;
		    points = xrefP;
		    setwindow(20, 10, 60, 16, "Cross Reference", " ", "(R)eplace,(N)ext, or F10 to Exit", NULL);
		    Print("\n\r Vendor: %s", vdcode);
		    Print("\n\r Product: %s", pd.code);
		    Print("\n\r Desc.: %s", pd.description);
		    Print("\n\r Price: %.2f", pd.p1);
		    Print("\n\r Ship.rate: %.2f", pd.rate);
		    vec = toupper(nextkey());
		    points = lastP;
		    restore();

		}

		switch (vec) {

		    case 'N':
			break;

		    case 'R':
			strcpy(pl->vdcode, vdcode);
			strcpy(pl->pdcode, pd.code);
			strcpy(pl->description, pd.description);
			pl->description[23] = '\0';
			pl->price = pd.p1;
			pl->f = pd.rate;
			change = TRUE;

			/* fall thru */

		    default:
			changeflg = change;
			return(0);
		}
	    }
	}

	/*
	* inform user that no cross references were found for this record
	*/
	if (!autoReplace)
	    message(M_SCR|M_KEY, "No matches - hit any key");
    }

    return(0);
}

/**************************************************************************
pltotals - gets total price and total freight from PL file
    p:  Pointer to Price
    f:  Pointer to Frieght Rate
    w:      Pointer to Weight
    temp2:  Pointer to Vendor to match (* for All)
**************************************************************************/
FLOAT pltotals(FLOAT *p, FLOAT *f, FLOAT *w, const CHAR *temp2)
{
    FLOAT tp=0,tf=0,tw=0,mkup=0;
    INT i;
    PLREC pl;

    for (i=0; i < s_nrecs(PL); i++) {
	s_read(PL,&pl,i);

	if (!strcheck(temp2,pl.vdcode)) {
	  tp += pl.price * pl.qty;
	  tf += pl.f * pl.weight * pl.qty;
	  tw += pl.weight;
	  mkup=pl.markup;
	}
    }
    *p = tp;
    *f = tf;
    *w = tw;
    return(mkup);
}

/*****************************************************************************
INT vd_chooser(CHAR *) - Loads window of only the VDs in the current PL file
*****************************************************************************/
INT vd_chooser(CHAR *ptr)
{
    STATIC CHAR hold[13], filen[13], ptemp[60];
    struct text_info ti;
    VDREC vd;
    PLREC pl, pre_pl;
    INT i, j, k, fh, n, it, numvd;

    /* inits */
    j = 0;

    message(M_SCR, "Searching . . .");
    n = s_nrecs(PL);

    memset(vdlist, 0, MAXVENDORS * 9 * sizeof(CHAR));

    /* initially, no records are tagged */
    memset(tagged, NO, MAXVENDORS * sizeof(BOOLEAN));

    if(n > 0)
    {
	s_read(PL, &pre_pl, 0);
	strcpy(vdlist[j], pre_pl.vdcode);
	j++;
    }
    for(i = 1; i < n && j < MAXVENDORS; i++)
    {
	s_read(PL, &pl, i);
	if(!stricmp(pl.vdcode, pre_pl.vdcode)) /* in the same group, ignore */
	    continue;
	if(!strnicmp(pl.vdcode, "__", 2)) /* ignore separator */
	{
	     memcpy(&pre_pl, &pl, sizeof(PLREC));
	     continue;
	}

	/* push pl to the list, and save pl to pre_pl to start over */
	strcpy(vdlist[j], pl.vdcode);
	j++;

	memcpy(&pre_pl, &pl, sizeof(PLREC));
    }

    s_close(PL);

    /*
    * If no vendors, then close file & exit
    */
    if (!j) {
	message(M_SCR|M_KEY, "No Vendors in Part List - hit any key");
	return(-1);
    }

    /*
    * Only 1 vendor in PL
    */
    if (j == 1) {
	strcpy(ptr, vdlist[0]);
	j--;
    } else {

	/*
	* Pick Random number for a FileName and open it
	*/
	do {
	    sprintf(filen, "$VDTEMP$.%03d", it=random_n());
	    sprintf(ptemp, "%s%s", tempDir, filen);
	} while ((fh = FileOpen(ptemp)) == -1);

	FileWrite(fh, &j, sizeof(INT));
	numvd = j;
	s_open(VD);
	for (i=0; i<j; i++) {
	    if ((k = s_getno(VD, 0, vdlist[i])) == -1) {
		message(M_SCR|M_KEY, "%s is not defined in Vendor List - hit any key", vdlist[i]);
		message(M_SCR, "Searching . . .");
		numvd--; /* if vd doesn't exist, we should not count it */
	    } else {
		s_read(VD, &vd, k);
		FileWrite(fh, &vd, sizeof(VDREC));
	    }
	}
	if(j != numvd)
	{
	    FileSeek(fh, 0L, SEEK_SET);
	    FileWrite(fh, &numvd, sizeof(INT));
	}
	s_close(VD);
	FileClose(fh);
	strcpy(hold, db[VD].fname);
	strcpy(db[VD].fname, "$VDTEMP$");
	strcpy(ptemp, db[VD].path);
	strcpy(db[VD].path, tempDir);
	sprintf(db[VD].extension, "%03d", it);
	s_open(VD);
	/* sortfile(VD, KEY_UP, 0); no sorting */
	message(M_SCR, " ");
	if (!inPO) {
	    gettextinfo(&ti);
	    textattr(REVERSE);
	    j = helpinpt(VD, 0, "Enter VD code, F1 for List, or RETURN for All: ", &vd);
	    scrrestore(&ti);
	} else {
	    if ((j = gethelp(VD, 0, 9, 4, 1, 13)) >= 0)
		s_read(VD, &vd, j);
	}

	/* j=-1 when ESC is hit */

	s_close(VD);

	strcpy(ptr, vd.code);
	strcpy(db[VD].fname, hold);
	strcpy(db[VD].path, ptemp);
	strncpy(db[VD].extension, "VD", 3);
	sprintf(ptemp, "%s%s", tempDir, filen);

	/* Delete Temp File */
	FileDelete(ptemp);

	/*
	* If ESC was hit then return -1 so function will be cancelled
	*/
	if (j == -1) {
	    return(-1);
	/*
	* If ENTER was hit then use All vendors
	*/
	} else if (j == -4) {
	    strcpy(ptr, "*");
	    return(0);
	}
    }

    return (j+1); //return(0);
}

/*****************************************************************************
INT vd_chooser_install(CHAR *) - Loads window of only the VDs (with INSTALL)
	in the VD database. Otherwise, function is equivlent to the above one.
*****************************************************************************/
INT vd_chooser_install(CHAR *ptr)
{
    STATIC CHAR hold[13], filen[13], ptemp[60];
    struct text_info ti;
    VDREC vd;
    INT i, j, fh, n, it;

    /* inits */
    j = 0;

    message(M_SCR, "Searching . . .");
    n = s_nrecs(VD);

    memset(vdlist, 0, MAXVENDORS * 9 * sizeof(CHAR));

//    for (i=0; i<n && j<MAXVENDORS; i++) {
    for (i=0; i<n; i++) {
	s_read(VD, &vd, i);

	if (!strnicmp(vd.product, "INSTAL", 6)) {
//            strcpy(vdlist[j], vd.code);
	    if(j == 0)
		strcpy(vdlist[j], vd.code);
	    j++;
	}
    }
    s_close(VD);

    /*
    * If no vendors, then close file & exit
    */
    if (!j) {
	message(M_SCR|M_KEY, "No Vendors beginning with IN in VD list - hit any key");
	return(-1);
    }

    /*
    * Only 1 IN vendor in VD
    */
    if (j == 1) {
	strcpy(ptr, vdlist[0]);
    } else {

	/*
	* Pick Random number for a FileName and open it
	*/
	do {
	    sprintf(filen, "$VDTEMP$.%03d", it=random_n());
	    sprintf(ptemp, "%s%s", tempDir, filen);
	} while ((fh = FileOpen(ptemp)) == -1);

	FileWrite(fh, &j, sizeof(INT));
	s_open(VD);
	for (i=0; i<n; i++) {
	    s_read(VD, &vd, i);
	    if (!strnicmp(vd.product, "INSTAL", 6)) 
		FileWrite(fh, &vd, sizeof(VDREC));
	}
	s_close(VD);
	FileClose(fh);
	strcpy(hold, db[VD].fname);
	strcpy(db[VD].fname, "$VDTEMP$");
	strcpy(ptemp, db[VD].path);
	strcpy(db[VD].path, tempDir);
	sprintf(db[VD].extension, "%03d", it);
	s_open(VD);
	sortfile(VD, KEY_UP, 0);
	message(M_SCR, " ");
	if (!inPO) {
	    gettextinfo(&ti);
	    textattr(REVERSE);
	    j = helpinpt(VD, 0, "Enter VD code, F1 for List, or RETURN for All: ", &vd);
	    scrrestore(&ti);
	} else {
	    if ((j = gethelp(VD, 0, 9, 4, 1, 13)) >= 0)
		s_read(VD, &vd, j);
	}

	/* j=-1 when ESC is hit */

	s_close(VD);

	strcpy(ptr, vd.code);
	strcpy(db[VD].fname, hold);
	strcpy(db[VD].path, ptemp);
	strncpy(db[VD].extension, "VD", 3);
	sprintf(ptemp, "%s%s", tempDir, filen);

	/* Delete Temp File */
	FileDelete(ptemp);

	/*
	* If ESC was hit then return -1 so function will be cancelled
	*/
	if (j == -1) {
	    return(j);
	/*
	* If ENTER was hit then use All vendors
	*/
	} else if (j == -4) {
	    strcpy(ptr, "*");
	}
    }

    return(0);
}

/*****************************************************************************
letters - Choose output of record to Letter, Label, Envelope, or Transmittal
*****************************************************************************/
INT letters(DB_TYPE type, IN_POS *c, CHAR *dataptr)
{
    SPREC sp;
    CDREC client;
    CHAR *name, *company, *address, *city, *state, *zip, *tel, *fax, *email, *regarding, fname[60], tdate[9];
    CHAR now[9], *list[]={"Letter", "Label", "Envelope", "Transmittal", NULL};
    FILE *fp;
    INT choice;
    BOOLEAN endflg;

    /* Prompt to save changes. */
    s_vcheck(type, dataptr, c->ypos);

    if ((choice = window_chooser(33, 9, list)) == -1)
	return(choice);

    StrDate(tdate, &today);

    switch (type) {

	case CD:
	    name = ((CHAR *)dataptr + db[type].f[2].offset);
	    regarding = NULL;
	    company = ((CHAR *)dataptr + db[type].f[1].offset);
	    address = ((CHAR *)dataptr + db[type].f[9].offset);
	    city = ((CHAR *)dataptr + db[type].f[10].offset);
	    state = ((CHAR *)dataptr + db[type].f[11].offset);
	    zip = ((CHAR *)dataptr + db[type].f[12].offset);
	    tel = ((CHAR *)dataptr + db[type].f[4].offset);
	    fax = ((CHAR *)dataptr + db[type].f[5].offset);
	    email = ((CHAR *)dataptr + db[type].f[7].offset);
	    break;

	case VD:
	    name = ((CHAR *)dataptr + db[type].f[4].offset);
	    regarding = NULL;
	    company = ((CHAR *)dataptr + db[type].f[1].offset);
	    address = ((CHAR *)dataptr + db[type].f[10].offset);
	    city = ((CHAR *)dataptr + db[type].f[11].offset);
	    state = ((CHAR *)dataptr + db[type].f[12].offset);
	    zip = ((CHAR *)dataptr + db[type].f[13].offset);
	    tel = ((CHAR *)dataptr + db[type].f[14].offset);
	    fax = ((CHAR *)dataptr + db[type].f[15].offset);
	    email = ((CHAR *)dataptr + db[type].f[5].offset);
	    break;

	case SP:
	    name = ((CHAR *)dataptr + db[type].f[1].offset);
	    regarding = NULL;
	    company = NULL;
	    address = ((CHAR *)dataptr + db[type].f[6].offset);
	    city = ((CHAR *)dataptr + db[type].f[7].offset);
	    state = ((CHAR *)dataptr + db[type].f[8].offset);
	    zip = ((CHAR *)dataptr + db[type].f[9].offset);
	    tel = ((CHAR *)dataptr + db[type].f[4].offset);
	    fax = NULL;
	    email = NULL;
	    break;

	case ES:
	    s_read(CD,&client,s_getno(CD,0,((CHAR *)dataptr + db[type].f[2].offset)));
	    name = ((CHAR *)dataptr + db[type].f[4].offset);
	    regarding = ((CHAR *)dataptr + db[type].f[1].offset);
	    company = client.name;
	    address = client.address;
	    city = client.city;
	    state = client.state;
	    zip = client.zip;
	    tel = client.tel;
	    fax = client.fax;
	    email = client.email;
	    break;

	/*
	* there is a BUG if this is ever executed
	*/
	default:
	    message(M_SCR|M_ERR|M_KEY, "Internal Error in %s line %d - hit any key", __FILE__, __LINE__);
	    break;
    }

    /*
    * create temporary file
    */
    if ((fp = fileopn(FileTempName(fname), "wb")) == NULL) {
	message(M_SCR|M_ERR|M_KEY, "Unable to create temporary file: %s - hit any key", fname);

    } else {

	endflg = FALSE;
	switch (choice) {

	    /*
	    * Letter
	    */
	    case 0:

		s_read(SP, &sp, s_getno(SP, 0, curuser.spcode));

		fprintf(fp, "\n\r\n\r\n\r\n\r");
		fprintf(fp, "\n\r        %s\n\r\n\r",tdate);
		fprintf(fp, "        %s\n\r",name);
		if (company)
		    fprintf(fp, "        %s\n\r", company);
		fprintf(fp, "        %s\n\r", address);
		fprintf(fp, "        %s, %s %s\n\r", city, state, zip);
		fprintf(fp, "        Phone: %s\n\r", tel);
		if (fax)
		    fprintf(fp, "        Fax: %s\n\r", fax);
		if (regarding)
		    fprintf(fp, "\n\r        RE: %s,\n\r", regarding);
		fprintf(fp, "\n\r        Dear %s,\n\r\n\r", name);
		fprintf(fp, "\n\r\n\r        Sincerely,\n\r");
		fprintf(fp, "\n\r\n\r        %s", sp.name);
		fprintf(fp, "\n\r        %s", sp.title);
		break;

	    /*
	    * Label
	    */
	    case 1:
		if(message(M_KEY|M_SCR, "Print information to label sheet (n/Y) ?") != 'N')
		{
		    if(SelectPrinter() && printercheck() != -1)
		    {
			message(M_KEY|M_SCR, "Please insert the labeling sheet to the printer, and hit any key when ready");
			print_label_to_label_sheet(name, company, address, city, state, zip, regarding);
		    }
		    filecls(fp);
		    FileDelete(fname);
		    return (0);
		}

		fprintf(fp, "\n\r  %s", name);
		if (company)
		    fprintf(fp, "\n\r  %s", company);
		fprintf(fp, "\n\r  %s", address);
		fprintf(fp, "\n\r  %s %s, %s\n\r", city, state, zip);
		if (regarding)
		    fprintf(fp, "\n\r  RE: %s\n\r", regarding);
		break;

	    /*
	    * Envelope
	    */
	    case 2:
		if(message(M_KEY|M_SCR, "Print to envelop (n/Y) ?") != 'N')
		{
		    if(SelectPrinter() && printercheck() != -1)
		    {
			message(M_KEY|M_SCR, "Please insert an envelop to the printer, and hit any key when ready");
			print_address_on_envelop(FALSE, name, company, address, city, state, zip, regarding);
		    }
		    filecls(fp);
		    FileDelete(fname);
		    return (0);
		}

		fprintf(fp, "\n\r                             %s",name);
		if (company) {
		    fprintf(fp, "\n\r                             %s",company);
		}
		fprintf(fp, "\n\r                             %s",address);
		fprintf(fp, "\n\r                             %s %s, %s\n\r",city,state,zip);
		if (regarding)
		    fprintf(fp, "\n\r                             RE: %s\n\r",regarding);
		break;

	    /*
	    * Transmittal
	    */
	    case 3:

		s_read(SP, &sp, s_getno(SP, 0, curuser.spcode));
		StrTimeStamp(now);
		fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r%-6s%-17s%-8s\n\r\n\r\n\r\n\r", "", tdate, now);
		fprintf(fp, "       %-40s%s\n\r\n\r", name, sp.name);
		fprintf(fp, "       %-35s%s\n\r\n\r", (company ? company:""), sysvar.company);
		fprintf(fp, "       %-35s%s\n\r\n\r", address, (regarding ? regarding:""));
		fprintf(fp, "       %-35s%s\n\r\n\r", name, compInfo.fax);
		fprintf(fp, "       %-35s%s\n\r\n\r\n\r\n\r          ", (fax ? fax:""), compInfo.tel);
		break;

	    default:
		endflg = TRUE;
		break;
	}

	if (!endflg) {
	    filecls(fp);
	    TextEditor(fname, fax, email);
	    FileDelete(fname);
	}
    }

    return(0);
}

/*****************************************************************************
goto_last_job() - Sets Cursor to PO pointed to by the Last JOB used.
*****************************************************************************/
INT goto_last_job(DB_TYPE type, IN_POS *c, VOID *ptr)
{
    ptr = ptr;

    switch (type) {

	case PO:
	    if (lastporec) {
		c->ypos = lastporec;
		lastporec = 0;
	    } else {
		c->ypos = getno_mess(type, 1, lastjob);
	    }
	    break;

	case JB:
	case ES:
	    c->ypos = s_getno(type, 0, lastjob);
	    break;

	default:
	    c->ypos = 0;
	    break;
    }

    if ((c->ypos < 0) || (c->ypos >= s_nrecs(type)))
	c->ypos = 0;

    return(0);
}

/********************** Information Menu *************************/
VOID infomenu(VOID)
{
    STATIC CHAR *prompt[] = {
	"INFORMATION MENU",
	"CLIENT INFORMATION",
	"VENDOR INFORMATION",
	"PRODUCT INFORMATION",
	"SALES PERSONS INFORMATION",
	"RETURN TO MAIN MENU",
	NULL
    };
    STATIC MOUSE_CLICK infoMenu[] = {
	{152,  64, 320,  64, KEY_1 },
	{152,  80, 320,  80, KEY_2 },
	{152,  96, 328,  96, KEY_3 },
	{152, 112, 316, 112, KEY_4 },
	{152, 128, 328, 128, KEY_5 },
	{0, 0, 0, 0, 0}
    };
    INT choice, lastchoice=1;
    MOUSE_CLICK *lastP;

    /* save last mouse click points */
    lastP = points;

    do {

	/* register the info menu mouse codes */
	points = infoMenu;

	switch(choice = menu(prompt, 1, 20, 0, lastchoice, HLP_INFOMENU)) {

	    /*
	    * Client database
	    */
	    case 1:
		lastchoice = choice;
		db_inpt(CD);
		break;

	    /*
	    * Vendor database
	    */
	    case 2:
		lastchoice = choice;
		db_inpt(VD);
		break;

	    /*
	    * Product database
	    */
	    case 3:
		lastchoice = choice;
		while (db_inpt(PD) == 1)
		    ;
		break;

	    /*
	    * Sale Person database
	    */
	    case 4:
		lastchoice = choice;
		db_inpt(SP);
		break;

	    /*
	    * Exit this menu
	    */
	    case 5:
	    case KEY_F10:
		choice = 5;
		break;

	    /*
	    * invalid key hit
	    */
	    default:
		beep();
		break;
	}
       s_closeall();
    } while (choice != 5);

    /* restore last mouse click points */
    points = lastP;
}

/*****************************************************************************
INT po_message(POREC *) - Displays PO Status Info on Message Bar
*****************************************************************************/
INT po_message(POREC *po)
{
    CHAR outstring[120], chtoday[9];

    StrDate(chtoday, &today);

    switch (toupper(po->status[0])) {

	case 'S':
	    if (strdatecmp(po->shipdate, chtoday) >= 0)
		sprintf(outstring, "Will Ship on %s", po->shipdate);
	    else
		sprintf(outstring, "Shipped on %s", po->shipdate);
	    break;

	case 'O':
	    if (po->shipdate[0] == '\0')
		sprintf(outstring, "Placed On Order on %s", po->podate);
	    else
		sprintf(outstring, "Placed On Order on %s, and will Ship on %s",po->podate,po->shipdate);
	    break;

	case 'P':
	    if (strdatecmp(po->shipdate, chtoday) >= 0)
		sprintf(outstring, "Will Ship Partially on %s", po->shipdate);
	    else
		sprintf(outstring, "Partially Shipped on %s", po->shipdate);
	    break;

	case 'R':
	    if (strdatecmp(po->release_date, chtoday) >= 0)
		sprintf(outstring, "Will Release on %s", po->release_date);
	    else
		sprintf(outstring, "Released on %s", po->release_date);
	    break;

	default:
	    outstring[0] = '\0';
	    break;
    }

    /* make sure we don't blow the stack */
    Assert(strlen(outstring) < 120);

    message(M_SCR, "%s", outstring);

    return(0);
}

/************************************************************************
jb_po (hot key hook) - calls PO
    type:   ident
    c:      cursor struct
    jb:     current record
*************************************************************************/
INT jb_po(DB_TYPE type, IN_POS *c, JBREC *jb)
{
    CHAR *list[] = {
	"Create PO",
	"Create Change Order PO",
	"Goto PO",
	"Print PO",
	"Fax PO",
	"PO Form",
/*      "Installation",       deleted permenently on 11/23/96, function is duplicated in Account F8 */
	"Requisition (Screen)",
	"Requisition (Form)", /* added on 4/8/97 for printing to form */
	"Proj. Mang. Form",
	"Set Submital Approval Date",   /* added on 9/15/99 */
	NULL
    };
    POREC po, po1;
    ESREC es;
    CDREC tmpCD;
    VDREC tmpVD;
    PO_ALT *poRec, *tmpPo;
    CHAR tmpFile[80], extra[70], material_po_num[9];
    LONG code;
    INT i, j, choice, tmpHandle, count, rec, ncd, vdgrp = 1;
    SHORT esn;
    BOOLEAN endflg, chpo = TRUE, more_po = TRUE;
    ESREC tmpes1, tmpes2;

    VD_ALT *vdRec, *tmpVd;
    PLREC pl1, pl2;
    USHORT num_vd = 0;

    /* Inits */
    endflg = FALSE;

    /*
    * we cannot create a PO for a job that has no ESREC
    */
    s_open(ES);
    if ((esn = s_getno(ES, 0, jb->code)) == -1) {
	message(M_SCR|M_KEY, "No Estimate exists for this Job - hit any key");
	return(-1);
    }

    /*
    * check to see how many es record(s) are associated with this job.
    * if only one, set the status to W
    */
    s_read(ES, &tmpes1, esn-1);
    s_read(ES, &tmpes2, esn+1);
    if(strcmp(tmpes1.jbcode, jb->code) && strcmp(tmpes2.jbcode, jb->code))
    {
	if((jb->status[0] != 'S') && (jb->status[0] != 'P') && (jb->status[0] != 'O') && (jb->status[0] != 'R'))
	    jb->status[0] = 'W';
    }

    /*
    * cannot create a PO until job is Won, Shipped, Partially shipped, or On Order, or Released
    */
    if ((jb->status[0] != 'W') && (jb->status[0] != 'S') && (jb->status[0] != 'P') && (jb->status[0] != 'O') && (jb->status[0] != 'R') ) {
	message(M_SCR|M_KEY, "Job must be Won before you can create a PO - hit any key");
	return(-1);
    }

#ifdef restricted /* take away the code to check the job fields before goto PO */
    /* cannot create PO if the following fields are not entered */
    if(!is_field_entered(jb->city, "City")) return(-1);
    if(!is_field_entered(jb->state, "State")) return(-1);
    if(!is_field_entered(jb->due, "Due Date")) return(-1);

    if(!is_field_entered(jb->sent, "Ship Date")) return(-1);
    if(!is_field_entered(jb->address, "Address")) return(-1);
    if(!is_field_entered(jb->zip, "Zip")) return(-1);
    if(!is_field_entered(jb->stel, "Site Tel")) return(-1);
    if(!is_field_entered(jb->agent, "Project Mgr.")) return(-1); /* there is a bug in db_defs.h jbfield array */
    if(!is_field_entered(jb->pmgr, "Job Super")) return(-1); /* thus, these two do not look right */
#endif
    /*
    * Get users choice of "Create PO", "Goto PO", "Print PO", or "Fax PO".
    * Don't do anything if user cancels.
    */
    if ((choice = window_chooser(31, 10, list)) != -1) {

	switch (choice) {

	    /*
	    * Create PO
	    */
	    case 0:
		chpo = FALSE;

	    /*
	    * Create Change Order PO
	    */
	    case 1:

		/* init tag_allowed */
		memset(tag_allowed, TRUE, MAXVENDORS * sizeof(BOOLEAN));

		/* stay in JB after create POs */
		endflg = TRUE;

		/* reads this jobs ESREC */
		s_read(ES, &es, esn);

		/* set the PL file for this Job's ES record */
		plfile(&es);
		s_vcheck(type, jb, c->ypos);

		/*
		* if there is PL left has no Purchase Order associated, then choose from the Vendor list
		*/

		/* go through PL to figure how many vendor groups */
		count = s_nrecs(PL);
		if (count <= 0) {
		    message(M_SCR|M_KEY, "No Vendors in Part List - hit any key");
		    break;
		}
		s_read(PL, &pl1, 0);
		for(i = 0; i < count; i++)
		{
		    s_read(PL, &pl2, i);
		    if(!stricmp(pl1.vdcode, pl2.vdcode)) /* same group */
			continue;

		    memcpy(&pl1, &pl2, sizeof(PLREC));

		    /* skip the separator (special VD code) */
		    if(!strnicmp(pl1.vdcode, "__", 2))
			continue;

		    vdgrp++;
		}
		if(vdgrp > MAXVENDORS)
		    vdgrp = MAXVENDORS;

		count = s_nrecs(PO);
		for(i = 0; i < count; i++)
		{
		    s_read(PO, &po, i);
		    if(strcmp(po.jbcode, jb->code))
			continue;

		    tag_allowed[po.plidx - 1] = FALSE; /* not allowed to tag, since the PO exists */
		    vdgrp--;
		}

		if(vdgrp <= 0) /* No PL is left over, cannot create more PO */
		    more_po = FALSE;

/*                if ((jb->status[0] == 'O') || (lastporec = getno_mess(PO, 1, jb->code)) != -1) { */
		if (!more_po) {
		    i = message(M_SCR|M_KEY, "POs already exist for this Job - Go there now? (Y/n)");
		    if (i != 'N' && i != KEY_ESC)
			endflg = FALSE;
		} else {
		    count = s_nrecs(PL);
		    if (count <= 0) {
			message(M_SCR|M_KEY, "No Vendors in Part List - hit any key");
			break;
		    }

		    /*
		    * Generate a temporary file contains the right Vendors
		    */
		    if ((tmpHandle = FileCreateTemp(tmpFile)) == -1)
			break;

		    FileSeek(tmpHandle, sizeof(USHORT), SEEK_SET);

		    for(i = 0; i < count-1; i++)
		    {
			s_read(PL, &pl1, i);
			/* skip the separator (special VD code) */
			if(!strnicmp(pl1.vdcode, "__", 2))
			    continue;

			s_read(PL, &pl2, i+1);
			if(strcmp(pl1.vdcode, pl2.vdcode))  // vd code not same
			{
			    rec = s_getno(VD, 0, pl1.vdcode);
			    if(rec != -1)
			    {
				s_read(VD, &tmpVD, rec);
				FileWrite(tmpHandle, &tmpVD, sizeof(VDREC));
				num_vd ++;
			    }
			}
		    }
		    // write the last one
		    s_read(PL, &pl1, i);
		    rec = s_getno(VD, 0, pl1.vdcode);
		    if(rec != -1)
		    {
			s_read(VD, &tmpVD, rec);
			FileWrite(tmpHandle, &tmpVD, sizeof(VDREC));
			num_vd ++;
		    }
		    if(num_vd > MAXVENDORS)
			num_vd = MAXVENDORS;
		    FileSeek(tmpHandle, 0, SEEK_SET);
		    FileWrite(tmpHandle, &num_vd, sizeof(USHORT));
		    FileClose(tmpHandle);

		    /*
		    * choose from among the vendors in the Part List
		    */
		    if ((i = message(M_SCR|M_KEY, "F1 for list of Vendors, or ENTER for All:")) == KEY_F1)
			vdRec = vd_chooser_multi(tmpFile, FALSE);                                    
		    else if (i != KEY_CR)
		    {
			FileDelete(tmpFile);
			break;
		    }
		    else                                                                      
			vdRec = vd_chooser_multi(tmpFile, TRUE);                                     

		    if (vdRec) {

			/* read last record so we can increment PO numbers */
			s_read(PO, &po, s_nrecs(PO)-1);
			code = atol(po.po_number);

			/*
			* add a PO for each VD chosen from PL
			* FUTURE: create a PO for each tagged Vendor
			*/
			tmpVd = vdRec;
			while (tmpVd) {

			    message(M_SCR, "Creating PO# %08ld for this Job with %s...", (long)code, tmpVd->vd.code);

			    /* next PO has incremented po number */
			    code++;

			    /*
			    * fill PO with JB info
			    */
			    s_clearrec(PO, &po);
			    sprintf(po.po_number, "%08ld", (long)code);
			    strcpy(po.vdcode, tmpVd->vd.code);
			    strcpy(po.jbcode, jb->code);
			    strcpy(po.jbname, jb->name);
			    strcpy(po.state, jb->state);
			    strcpy(po.client_code, es.client_code);
			    strcpy(po.spcode, es.spcode);
			    if(chpo) /* By Majid's order on 3/6/00 */
				{
					StrDate(po.shipdate, &today);
					StrDate(po.inv_date, &today);	/* 7/14/00 */
				}
			    else
				{
					strcpy(po.shipdate, jb->sent);
					strcpy(po.inv_date, jb->sent);	/* 7/14/00 */
				}
			    StrDate(po.podate, &today); 

				/* Figure out the material PO # (7/14/00) */
				if(strncmp(po.vdcode, "IN", 2) == 0)
				{
					material_po_num[0] = '\0';

					count = s_nrecs(PO);
					for(j = 0; j < count; j++)
					{
						s_read(PO, &po1, j);
						if(strcmp(po1.jbcode, jb->code) == 0 && strncmp(po1.vdcode, "IN", 2) != 0)
							/* Found a possible previous material PO, but we always remember the lastone */
							strcpy(material_po_num, po1.po_number);
					}

					if(material_po_num[0])
						strcpy(po.material_po, material_po_num);
				}

			    /* save the change order info */
			    if(chpo)
				strcpy(po.jbcode2, jb->code);
			    else
				strncpy(po.jbcode2, jb->ch_od_code, 8);
			    po.jbcode2[8] = '\0';

			    /* update Job status to On Order */
			    jb->status[0] = po.status[0] = 'O';
			    jb->status[1] = po.status[1] = '\0';

			    /* if Job Proj. Mgr. field is empty, set it by es.estname */
			    if (jb->agent[0] == '\0')
				strcpy(jb->agent, es.estname);

			    if ((rec = s_getno(JB, 0, jb->code)) != -1) {
				s_write(JB, jb, rec, NO);
				c->set = TRUE;
			    
				/* Update the ES status record */
				s_open(ES);
				s_read(ES, &es, (rec = s_getno(ES, 0, jb->code)));
				es.status[0] = 'O';
				s_write(ES, &es, rec, FALSE);
				s_close(ES);
			    }

			    /*
			    * recalc $$$ for this new PO
			    */
			    set_rate(&es, po.vdcode, TRUE); /* this sets vdlist[] */
			    re_calc(&es, po.vdcode, FALSE, tmpVd->idx+1);
			    po.plidx = (SHORT) (tmpVd->idx+1);
			    po.cogs = es.cogs;
			    po.commission = es.com;
			    po.tax = es.esttax;
			    po.estship = es.estship;
			    po.bid = es.bid;
			    po.mkup = es.mkup;
			    /* po.profit = po.mkup-po.other+(po.shipping==0 ? 0:po.estship-po.shipping); */
			    po.profit = po.mkup-(po.status[0] == 'S' ? po.actcogs-po.cogs:0)+(po.shipping==0 ? 0:po.estship-po.shipping);
			    /* commission should be based on profit */
			    {
				INT i;
				SPREC sp;

				s_open(SP);
				if((i = s_getno(SP, 0, po.spcode)) != -1)
				    s_read(SP, &sp, i);
				else
				    sp.rate = 0;
				s_close(SP);
				po.commission = po.profit * sp.rate;
			    }

			    /*
			    * update YTD On Order for Client and Vendor
			    */
			    s_read(CD, &tmpCD, ncd=s_getno(CD, 0, po.client_code));
			    tmpCD.ytd_on_order += po.bid;
			    s_recwrite(CD, &tmpCD, ncd);

			    s_read(VD, &tmpVD, ncd=s_getno(VD, 0, po.vdcode));
			    tmpVD.ytd_on_order += po.cogs;
			    s_recwrite(VD, &tmpVD, ncd);

			    /* commit this new Purchase Order to the PO file */
			    s_write(PO, &po, s_nrecs(PO), FALSE);

			    tmpVd = tmpVd->next;
			}

			lastporec = s_nrecs(PO) - 1;

			tmpVd = vdRec;
			while(tmpVd)
			{
			    vdRec = vdRec->next;
			    free(tmpVd);
			    tmpVd = vdRec;
			}
		    }
		    FileDelete(tmpFile);
		}
		break;

	    /*
	    * Goto this PO
	    */
	    case 2:
		/*
		* we don't need any code here, because the way the PO is set up
		* right now is such that it automatically goes to the 1st PO it
		* finds with this job code.
		*/
		s_vcheck(type, jb, c->ypos);
		
		break;

	    /*
	    * PO Form
	    */
	    case 5:
		endflg = TRUE;
		jb_po_form(type, c, jb);
		break;

	    /*
	    * Installation      Gone now, as of 11/23/96
	    * Requisition
	    */
	    case 6:
		endflg = TRUE;
/*                jb_installation(type, jb);         */
		jb_requisition(jb, FALSE); /* print to screen */
		break;

	    /*
	    * Requisition (print to form), added on 4/8/97 
	    */
	    case 7:
		endflg = TRUE;
		jb_requisition(jb, TRUE); /* print to form */
		break;

	    /*
	    * Project Management Form
	    */
	    case 8:
		endflg = TRUE;
		jb_proj_mang_form(type, c, jb, TRUE);
		break;

	    /*
	    * Submital approval date
	    */
	    case 9:
		endflg = TRUE;
		jb_proj_mang_form(type, c, jb, FALSE);
		break;

	    /*
	    * Print PO, or Fax PO
	    */
	    default:

		endflg = TRUE;
		message(M_SCR, "Working. . .");

		/* search PO file for all POs with this job code */
		s_open(PO);

		/*
		* create the temp file and continue on success
		*/
		if ((tmpHandle = FileCreateTemp(tmpFile)) != -1) {

		    /* seek past 1st 2 bytes */
		    FileSeek(tmpHandle, sizeof(USHORT), SEEK_SET);

		    /*
		    * copy each found PO to a temp PO file
		    */
		    count = 0;
		    for (i = 0; i<s_nrecs(PO); i++) {
			s_read(PO, &po, i);

			/*
			* write each match to the temp file
			*/
			if (!stricmp(jb->code, po.jbcode)) {

			    FileWrite(tmpHandle, &po, sizeof(POREC));
			    count++;
			}
		    }

		    /*
		    * if no records found then inform the user
		    */
		    if (count == 0) {
			message(M_SCR|M_KEY, "There are no POs for this Job - hit any key");
			FileClose(tmpHandle);

		    } else {

			/* write # of entries as the header of the temp file */
			FileSeek(tmpHandle, 0, SEEK_SET);
			FileWrite(tmpHandle, &count, sizeof(USHORT));

			FileClose(tmpHandle);

			s_close(PO);
			message(M_SCR, NULL);

			/*
			* choose PO from that temp PO file
			*/
			if ((poRec = po_chooser(tmpFile)) != NULL) {

			    /*
			    * get user input for extra line
			    */
			    strcpy(extra, "Please submit 8 sets of submittals for approval ASAP.");
			    inpt(1, 25, "", extra, 69, CT_CHAR, NO);
			    message(M_SCR, NULL);

			    switch (choice) {

				/*
				* Print all tagged POs
				*/
				case 3:

				    while (poRec != NULL) {

					/* find this record in PO file, and fax it */
					po_print(&poRec->po, extra);

					/* goto next po record, and delete the current one */
					tmpPo = poRec;
					poRec = poRec->next;
					s_free(tmpPo);
				    }
				    break;

				/*
				* FAX all tagged POs
				*/
				case 4:

				    while (poRec != NULL) {

					/* find this record in PO file, and fax it */
					po_fax(&poRec->po, extra);

					/*
					* goto next PO record, and delete the current one
					*/
					tmpPo = poRec;
					poRec = poRec->next;
					s_free(tmpPo);
				    }
				    break;
			    }
			}
		    }

		    FileDelete(tmpFile);
		}
		break;
	}

	if (!endflg) {

	    db_inpt(PO);

	    c->flags = 4;
	    c->set = c->read = YES;
	}
    }

    return(0);
}


PO_ALT *po_chooser(const CHAR *fileName)
{
    STATIC MOUSE_CLICK poKeys[] = {
	{256, 184, 344, 184, KEY_F6}, {368, 184, 408, 184, KEY_F7},
	{432, 184, 504, 184, KEY_F8}, {568, 184, 624, 184, KEY_F10},
	{0, 0, 0, 0, 0}
    };

    MOUSE_CLICK *lastP;
    POREC po;
    PO_ALT *poList, *poPtr;
    INT fh, i, rec, currec, lastrec;
    USHORT n, key;
    BOOLEAN endflg, *po_tagged;

    /* inits */
    poList = NULL;

    if ((fh = FileOpen(fileName)) != -1) {

	FileRead(fh, &n, sizeof(USHORT));

	if (n == 0) {

	    message(M_SCR|M_KEY, "No records to choose from - hit any key");

	} else {

	    po_tagged = s_alloc(n*sizeof(BOOLEAN));
	    memset(po_tagged, FALSE, n*sizeof(BOOLEAN));

	    store();
	    textattr(REVERSE);
	    setwindow(2, 14, 79, 23, "Choose POs", "Home End PgUp PgDn \x18 \x19", "                               F6=UNTAG_ALL  F7=TAG  F8=TAG_ALL       F10=DONE", NULL);

	    /* save last mouse click points */
	    lastP = points;

	    /* assign new mouse click points for this window */
	    points = poKeys;

	    key = 0;
	    rec = 0;
	    lastrec = 1;
	    endflg = FALSE;
	    do {

		/*
		* don't refresh the screen if we don't need to
		*/
		if ((lastrec != rec) || (key == KEY_F6) || (key == KEY_F7) || (key == KEY_SPACE) || (key == KEY_F8) ) {

		    /*
		    * fill window with records
		    */
		    gotoxy(1, 1);

		    currec = rec - 5;
		    for (i=currec; i<currec+10; i++) {
			textattr((i == rec) ? REVERSE : YELLOW|ON_BLUE);
			if ((i >= 0) && (i < n)) {
			    FileSeek(fh, ((LONG)i*sizeof(POREC))+sizeof(USHORT), SEEK_SET);
			    FileRead(fh, &po, sizeof(POREC));
			    Print("\n\r%c %-8s  %-8s  %-8s  %-32s  %-8s", (*(po_tagged+i) ? 0x10: ' '), po.po_number, po.vdcode, po.jbcode, po.jbname, po.shipdate);
			} else {
			    Print("\n\r%-77s", " ");
			}
		    }
		}

		lastrec = rec;

		switch (key = nextkey()) {

		    /*
		    * move to the 1st record
		    */
		    case KEY_CTRL_PGUP:
		    case KEY_HOME:
			rec = 0;
			break;

		    /*
		    * Un-Tag all
		    */
		    case KEY_F6:

			memset(po_tagged, FALSE, (n*sizeof(BOOLEAN)));
			break;

		    /*
		    * tag current record
		    */
		    case KEY_F7:
		    case KEY_SPACE:

			Assert(rec >= 0);

			*(po_tagged+rec) = (*(po_tagged+rec) ? NO : YES);
			if (rec < (n - 1))
			    rec++;

			break;

		    /*
		    * Tag all
		    */
		    case KEY_F8:

			memset(po_tagged, TRUE, (n*sizeof(BOOLEAN)));
			break;

		    /*
		    * move to last record
		    */
		    case KEY_CTRL_PGDOWN:
		    case KEY_END:
			rec = n - 1;
			break;

		    /*
		    * move down 1 record
		    */
		    case KEY_DOWN:
			if (rec < (n - 1))
			    rec++;
			break;

		    /*
		    * move up one record
		    */
		    case KEY_SHFT_TAB:
		    case KEY_UP:
			if (rec)
			    rec--;
			break;

		    /*
		    * move down 1 page (10 records)
		    */
		    case KEY_PGDOWN:
			rec += 10;
			if (rec >= n)
			    rec = n - 1;
			break;

		    /*
		    * move up 1 page (10 records)
		    */
		    case KEY_PGUP:
			rec -= 10;
			if (rec < 0)
			    rec = 0;
			break;

		    /*
		    * user has quit
		    */
		    case KEY_ESC:
			endflg = YES;
			break;

		    /*
		    * user is done tagging POs
		    */
		    case KEY_F10:
			endflg = YES;
			break;

		    /*
		    * user has chosen a single PO
		    */
		    case KEY_ENTER:
			endflg = YES;
			break;

		    default:
			beep();
			break;
		}
	    } while (!endflg);

	    /* restore last mouse click points */
	    points = lastP;

	    restore();

	    /*
	    * continue if user did not hit ESCAPE
	    */
	    if (key != KEY_ESC) {

		/*
		* if user hit ENTER then they only picked one PO, so tag that one
		* and un-tag the rest
		*/
		if (key == KEY_ENTER) {
		    memset(po_tagged, FALSE, n*sizeof(BOOLEAN));
		    po_tagged[rec] = TRUE;
		}

		/*
		* create the PO_ALT linked list of tagged POs
		*/
		for (i=0; i<n; i++) {

		    /*
		    * if this entry was tagged, then add it to the linked list
		    */
		    if (po_tagged[i]) {
			FileSeek(fh, ((LONG)i*sizeof(POREC))+sizeof(USHORT), SEEK_SET);
			FileRead(fh, &po, sizeof(POREC));

			/*
			* if 1st node in list
			*/
			if (poList == NULL) {
			    poPtr = poList = s_alloc(sizeof(PO_ALT));

			} else {
			    poPtr->next = s_alloc(sizeof(PO_ALT));
			    poPtr = poPtr->next;
			}

			/*
			* add PO data to this new node
			*/
			poPtr->next = NULL;
			memcpy(&poPtr->po, &po, sizeof(POREC));
		    }
		}
	    }

	    s_free(po_tagged);
	}
    }
    FileClose(fh);

    return(poList);
}


/* this is the function to choose one and only one PO. It is almost the same
   as function po_chooser(), except this function only allows one selection.
*/
PO_ALT *po_chooser_one(const CHAR *fileName)
{
    STATIC MOUSE_CLICK poKeys[] = {
	{256, 184, 344, 184, KEY_F6}, {368, 184, 408, 184, KEY_F7},
	{432, 184, 504, 184, KEY_F8}, {568, 184, 624, 184, KEY_F10},
	{0, 0, 0, 0, 0}
    };

    MOUSE_CLICK *lastP;
    POREC po;
    PO_ALT *poList, *poPtr;
    INT fh, i, rec, currec, lastrec;
    USHORT n, key;
    BOOLEAN endflg;

    /* inits */
    poList = NULL;

    if ((fh = FileOpen(fileName)) != -1) {

	FileRead(fh, &n, sizeof(USHORT));

	if (n == 0) {

	    message(M_SCR|M_KEY, "No records to choose from - hit any key");

	} else {
	    store();
	    textattr(REVERSE);
	    setwindow(2, 14, 79, 23, "Choose PO", "Home End PgUp PgDn \x18 \x19", "                                                                    Enter=DONE", NULL);

	    /* save last mouse click points */
	    lastP = points;

	    /* assign new mouse click points for this window */
	    points = poKeys;

	    key = 0;
	    rec = 0;
	    lastrec = 1;
	    endflg = FALSE;
	    do {
		/*
		* don't refresh the screen if we don't need to
		*/
		if (lastrec != rec) {

		    /*
		    * fill window with records
		    */
		    gotoxy(1, 1);

		    currec = rec - 5;
		    for (i=currec; i<currec+10; i++) {
			textattr((i == rec) ? REVERSE : YELLOW|ON_BLUE);
			if ((i >= 0) && (i < n)) {
			    FileSeek(fh, ((LONG)i*sizeof(POREC))+sizeof(USHORT), SEEK_SET);
			    FileRead(fh, &po, sizeof(POREC));
			    Print("\n\r%c %-8s  %-8s  %-8s  %-32s  %-8s", ' ', po.po_number, po.vdcode, po.jbcode, po.jbname, po.shipdate);
			} else {
			    Print("\n\r%-77s", " ");
			}
		    }
		}

		lastrec = rec;

		switch (key = nextkey()) {

		    /*
		    * move to the 1st record
		    */
		    case KEY_CTRL_PGUP:
		    case KEY_HOME:
			rec = 0;
			break;
		    
		    /*
		    * move to last record
		    */
		    case KEY_CTRL_PGDOWN:
		    case KEY_END:
			rec = n - 1;
			break;

		    /*
		    * move down 1 record
		    */
		    case KEY_DOWN:
			if (rec < (n - 1))
			    rec++;
			break;

		    /*
		    * move up one record
		    */
		    case KEY_SHFT_TAB:
		    case KEY_UP:
			if (rec)
			    rec--;
			break;

		    /*
		    * move down 1 page (10 records)
		    */
		    case KEY_PGDOWN:
			rec += 10;
			if (rec >= n)
			    rec = n - 1;
			break;

		    /*
		    * move up 1 page (10 records)
		    */
		    case KEY_PGUP:
			rec -= 10;
			if (rec < 0)
			    rec = 0;
			break;

		    /*
		    * user has quit
		    */
		    case KEY_ESC:
			endflg = YES;
			break;
 
		    /*
		    * user is done tagging POs
		    */
		    case KEY_F10:
			endflg = YES;
			break;

		    /*
		    * user has chosen a single PO
		    */
		    case KEY_ENTER:
			endflg = YES;
			break;

		    default:
			beep();
			break;
		}
	    } while (!endflg);

	    /* restore last mouse click points */
	    points = lastP;

	    restore();

	    /*
	    * continue if user did not hit ESCAPE
	    */
	    if (key == KEY_ESC || key == KEY_F10) {
		FileClose(fh);
		return(poList);
	    }

	    /*
	    * if user hit ENTER then they only picked one PO, so tag that one
	    * and un-tag the rest
	    */
	    if (key == KEY_ENTER) {
		FileSeek(fh, ((LONG)rec*sizeof(POREC))+sizeof(USHORT), SEEK_SET);
		FileRead(fh, &po, sizeof(POREC));
		poPtr = poList = s_alloc(sizeof(PO_ALT));

		/*
		* add PO data to this new node
		*/
		poPtr->next = NULL;
		memcpy(&poPtr->po, &po, sizeof(POREC));
	    }
	}
    }
    FileClose(fh);

    return(poList);
}

VOID po_print(POREC *po, const CHAR *extra)
{
    CHAR fname[60];

    FileTempName(fname);
    po_createpo(po, fname, FALSE, extra);

    message(M_SCR, "Printing. . .");
    FilePrint(fname, defPrinter);
    FileDelete(fname);
    message(M_SCR, NULL);
}


VOID po_fax(POREC *po, const CHAR *extra)
{
    VDREC vd;
    CHAR fname[60];
    INT vec;

    FileTempName(fname);

    po_createpo(po, fname, FALSE, extra);

    /*
    * Fax this Purchase Order
    */
    if ((vec = s_getno(VD, 0, po->vdcode)) != -1) {
	s_read(VD, &vd, vec);

	FaxSend(fname, vd.fax);
    }

    FileDelete(fname);
}


/*****************************************************************************
VOID sp_new() - Creates a new .CFG file when a New Sales Person is Created
*****************************************************************************/
VOID sp_new(DB_TYPE type, IN_POS *c, SPREC *sp)
{
    CHAR cfgfile[13], newspfile[13];

    type = type; c = c;
    strcpy(cfgfile, "DB.CFG");
    sprintf(newspfile, "%s.CFG", sp->code);
    FileCopy(cfgfile, newspfile);
    _dos_setfileattr(newspfile, FA_HIDDEN);
}

/* msg_def - clears message bar */
INT msg_def(VOID *ptr)
{
    ptr = ptr;
    message(M_SCR, NULL);
    return(0);
}

/* val_def - valid if first byte of structure is non-zero. */
INT val_def(CHAR *ptr)
{
    return((*ptr ? 1:0));
}


/*****************************************************************************
other_letters - Choose output of record to:
		1. Request for Field Measurement Letter
		2. Receipt of Approval Acknowledgement (Question on Approval)
		3. Receipt of Approval Acknowledgement (Schedules ship date)
		4. Warrantee Letter
		5. etc.
*****************************************************************************/
INT other_letters(JBREC *jb, ESREC *es, CDREC *client)
{
    VDREC vd, tmpVD;
    SPREC sp, logsp;
    POREC po;
    PLREC pl1, pl2;
    CHAR fname[60], tdate[9], vdcode[9], name[60], faxNo[15], emailAddr[49], *m = "      ", tmpFile[60];
    CHAR *list[]={"Request for project information", "Purchase order/subcontract request",
		  "Request for Insurance Certificate", "Shopdrawing status request",
		  "Submital approval request", "Approval Acknowledgement (Ship date)",
		  "Approval Acknowledgement (Reply)", "Request for Field Measurement",
		  "Shipment status request", "Request for addition/missing material",
		  "Request for clarification", "Return good authorization (RGA) request", "Warrantee Letter", NULL};

    FILE *fp;
    INT choice, i, len, idx = -1, n, bg, ed, tmpHandle;
    BOOLEAN endflg, saveflg = inPO, multi = FALSE, more = TRUE;
    FLOAT total = 0;
    CHAR pTotal[20], desc[80];   /* "%-16.2f" needs 17 bytes */
    INT ans1, ans2, rec, count;
    VD_ALT *vdRec, *tmpVd;
    USHORT num_vd = 0;

    strcpy(name, jb->agent);
    len = strlen(name);
    for(i = 0; i < len; i++)
    {
	if(name[i] == ' ')
	{
	    name[i] = '\0';
	    break;
	}
    }

    if ((choice = window_chooser(33, 9, list)) == -1)
	return(choice);

    StrDate(tdate, &today);

    /* if print Request for Insu. Cert., and PO/SubCon, no need to read VD from list */
    if(choice != 1 && choice != 2)
    {
	/* open PL file associated to this ES record */
	vdcode[0] = '\0';
	openplfile(es);

	if(choice >= 6 && choice <= 11 && choice != 8)
	{
	    inPO = TRUE;
	    if ((idx = vd_chooser(vdcode)) == -1) 
	    {
		inPO = saveflg;
		return (-1);
	    }
	    inPO = saveflg;
	}
	else /* generate a temp file for vd_chooser_multi() */
	{
	    multi = TRUE;

	    if ((tmpHandle = FileCreateTemp(tmpFile)) == -1)
		return (-1);

	    count = s_nrecs(PL);
	    FileSeek(tmpHandle, sizeof(USHORT), SEEK_SET);

	    for(i = 0; i < count-1; i++)
	    {
		s_read(PL, &pl1, i);
		/* skip the separator (special VD code) */
		if(!strnicmp(pl1.vdcode, "__", 2))
		    continue;

		s_read(PL, &pl2, i+1);
		if(strcmp(pl1.vdcode, pl2.vdcode))  // vd code not same
		{
		    rec = s_getno(VD, 0, pl1.vdcode);
		    if(rec != -1)
		    {
			s_read(VD, &tmpVD, rec);
			FileWrite(tmpHandle, &tmpVD, sizeof(VDREC));
			num_vd ++;
		    }
		}
	    }
	    // write the last one
	    s_read(PL, &pl1, i);
	    rec = s_getno(VD, 0, pl1.vdcode);
	    if(rec != -1)
	    {
		s_read(VD, &tmpVD, rec);
		FileWrite(tmpHandle, &tmpVD, sizeof(VDREC));
		num_vd ++;
	    }
	    if(num_vd > MAXVENDORS)
		num_vd = MAXVENDORS;
	    FileSeek(tmpHandle, 0, SEEK_SET);
	    FileWrite(tmpHandle, &num_vd, sizeof(USHORT));
	    FileClose(tmpHandle);

	    memset(tag_allowed, TRUE, MAXVENDORS * sizeof(BOOLEAN));

	    vdRec = vd_chooser_multi(tmpFile, FALSE);                                    

	    if(!vdRec)
	    {
		FileDelete(tmpFile);
		return (-1);
	    }
	    tmpVd = vdRec;
	    strcpy(vdcode, tmpVd->vd.code);
	    idx = tmpVd->idx + 1;
	}
    }

  while(more)
  {
    /*
    * create temporary file
    */
    if ((fp = fileopn(FileTempName(fname), "wb")) == NULL) {
	message(M_SCR|M_ERR|M_KEY, "Unable to create temporary file: %s - hit any key", fname);

    } else {
	memset(&vd, 0, sizeof(VDREC));
	memset(&sp, 0, sizeof(SPREC));
	memset(&po, 0, sizeof(POREC));
	s_read(SP, &sp, s_getno(SP, 0, es->spcode));
	s_close(SP);

	s_read(VD, &vd, s_getno(VD, 0, vdcode));
	s_close(VD);

	endflg = FALSE;
	strcpy(faxNo, client->fax);
	strcpy(emailAddr, client->email);

	if(choice == 3 || choice == 8 || choice == 9 || choice == 11)
	{
	    len = s_nrecs(PO);
	    for(i = 0; i < len; i++)
	    {
		s_read(PO, &po, i);
		if(!strcmp(po.vdcode, vd.code) && !strcmp(po.client_code, client->code) && !strcmp(po.jbcode, jb->code))
		{
		    if(po.plidx == 0) /* old style PO, we found it */
			break;
		    if(po.plidx == idx) /* new style PO, we found it */
			break;
		}
	    }
	    if(i == len) memset(&po, 0, sizeof(POREC));

	}

	switch (choice) {

	    /*
	    * Request for project information
	    */
	    case 0:
		fprintf(fp, "\n\r\n\r\n\r\n\r        %s\n\r\n\r\n\r\n\r", tdate);
		fprintf(fp, "        %s\n\r", jb->agent); 
		fprintf(fp, "        %s\n\r", client->name);
		fprintf(fp, "        %s\n\r", client->address);
		fprintf(fp, "        %s, %s %s\n\r", client->city, client->state, client->zip);
		fprintf(fp, "        Tel: %s\n\r", client->tel);
		fprintf(fp, "        Fax: %s\n\r", client->fax);
		fprintf(fp, "\n\r        Re: %s (Job #: %s)\n\r", jb->name, jb->code);
		fprintf(fp, "            %s, %s %s\n\r", jb->city, jb->state, jb->zip);

		fprintf(fp, "\n\r        Dear %s:\n\r\n\r", name);
		fprintf(fp, "        For the above referenced project, thank you for awarding us the\n\r");

		strcpy(desc, "        ");
		while(tmpVd)
		{
		    if(strlen(desc) + strlen(tmpVd->vd.product) + 2 <= 70)
		    {
			strcat(desc, tmpVd->vd.product);
			strcat(desc, ", ");
		    }
		    else
		    {
			fprintf(fp, "%s\n\r", desc);
			strcpy(desc, "        ");
			strcat(desc, tmpVd->vd.product);
			strcat(desc, ", ");
		    }
		    tmpVd = tmpVd->next;
		}
		desc[strlen(desc)-2] = '.';
		fprintf(fp, "%s\n\r", desc);
		fprintf(fp, "        Architectural submittals will be forthcoming to your attention\n\r");
		fprintf(fp, "        shortly.\n\r\n\r");

		fprintf(fp, "        In order for us to be able to provide you with the best\n\r");
		fprintf(fp, "        possible service, we would appreciate if you could forward\n\r");
		fprintf(fp, "        us the following information with regard to the project:\n\r\n\r");
		
		fprintf(fp, "\n\r        Exact Job Address:             __\n\r");
		fprintf(fp, "\n\r        Super Intendent Name:          __\n\r");
		fprintf(fp, "\n\r        Super Intendent Pager Number:  __\n\r");
		fprintf(fp, "\n\r        Job Site Telephone:            __\n\r");
		fprintf(fp, "\n\r        Job Site Fax:                  __\n\r");
		fprintf(fp, "\n\r        Required Delivery Date:        __\n\r");
		fprintf(fp, "\n\r        Substantial Completion Date:   __\n\r\n\r");

		fprintf(fp, "\n\r        We look forward to working with you on this project. Please\n\r");
		fprintf(fp, "        do not hesitate to call me if I could be of any assistance.\n\r");

		fprintf(fp, "\n\r\n\r        Cordially,\n\r");
		fprintf(fp, "        For %s\n\r", sysvar.company);

		s_read(SP, &sp, s_getno(SP, 0, curuser.spcode));
		s_close(SP);
		fprintf(fp, "\n\r\n\r        %s", sp.name);
		fprintf(fp, "\n\r        %s", sp.title);
		fprintf(fp, "\n\r\n\r        cc:    File (%s)", jb->code);
		more = FALSE;
		break;

	    /*
	    * Purchase Order/Subcontract request
	    */
	    case 1:
		vd.fax[0] = '\0';
		len = s_nrecs(PO);
		for(i = 0; i < len; i++)
		{
		    s_read(PO, &po, i);
		    if(!strcmp(po.jbcode, jb->code) || !strcmp(po.jbcode2, jb->code))
			total += po.bid;
		}

		/* convert float total to string value */
		sprintf(pTotal, "%-16.2f", total);
		while(pTotal[strlen(pTotal) - 1] == ' ')
		    pTotal[strlen(pTotal) - 1] = '\0';

		fprintf(fp, "\n\r\n\r\n\r\n\r        %s\n\r\n\r\n\r\n\r", tdate);
		fprintf(fp, "        %s\n\r", jb->agent); 
		fprintf(fp, "        %s\n\r", client->name);
		fprintf(fp, "        %s\n\r", client->address);
		fprintf(fp, "        %s, %s %s\n\r", client->city, client->state, client->zip);
		fprintf(fp, "        Tel: %s\n\r", client->tel);
		fprintf(fp, "        Fax: %s\n\r", client->fax);
		fprintf(fp, "\n\r        Re: %s (Job #: %s)\n\r", jb->name, jb->code);
		fprintf(fp, "            %s, %s %s\n\r", jb->city, jb->state, jb->zip);

		fprintf(fp, "\n\r        Dear %s:\n\r\n\r", name);
		fprintf(fp, "        Thank you for awarding us the building specialties for\n\r");
		fprintf(fp, "        %s in the amount of $%s.  Please forward\n\r", jb->name, pTotal);
		fprintf(fp, "        a contract or purchase order as soon as possible.  We cannot\n\r");
		fprintf(fp, "        commence with any work until our order has been formally accepted.\n\r");
		
		if(es->taxrate == 0)
		{
		fprintf(fp, "\n\r        Please provide a sales tax exemption certificate for this\n\r");
		fprintf(fp, "        project.  It is not sufficient to provide the tax exempt number;\n\r");
		fprintf(fp, "        the law requires that we have a copy of the actual certificate.\n\r");
		}

		fprintf(fp, "\n\r        We look forward to working with %s on this\n\r", client->name);
		fprintf(fp, "        project.  If you have any questions, do not hesitate to call me.\n\r");

		fprintf(fp, "\n\r\n\r        Sincerely,\n\r");
		s_read(SP, &sp, s_getno(SP, 0, curuser.spcode));
		s_close(SP);
		fprintf(fp, "\n\r\n\r        %s", sp.name);
		fprintf(fp, "\n\r        %s", sp.title);
		more = FALSE;
		break;

	    /*
	    * Request for Insurance Certificate
	    */
	    case 2:
		vd.fax[0] = '\0';
		memset(&logsp, 0, sizeof(SPREC));
		s_read(SP, &logsp, s_getno(SP, 0, curuser.spcode));
		s_close(SP);
		fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r");
		fprintf(fp, "        TO:       %s\n\r", compInfo.contact);
		fprintf(fp, "                  %s\n\r\n\r\n\r\n\r", compInfo.insuname);
		fprintf(fp, "        TEL       %s\n\r", compInfo.insutel);
		fprintf(fp, "        FAX       %s\n\r\n\r\n\r\n\r", compInfo.insufax);

		fprintf(fp, "                        CERTIFICATE OF INSURANCE REQUEST\n\r\n\r\n\r\n\r");
		fprintf(fp, "            CERTIFICATE HOLDER: %s\n\r", client->name);
		fprintf(fp, "                                %s\n\r", client->address);
		fprintf(fp, "                                %s, %s %s\n\r\n\r", client->city, client->state, client->zip);
		fprintf(fp, "            PROJECT NAME: %s\n\r\n\r", jb->name);
		fprintf(fp, "            ADDITIONAL INSURED: %s\n\r\n\r\n\r", client->name);
		fprintf(fp, "            COMMENTS:   30 days cancellation\n\r");
		fprintf(fp, "                        PROJECT NAME MUST BE LISTED ON ALL CERTIFICATES\n\r\n\r");


		fprintf(fp, "        PLEASE FAX A COPY OF THE CERTIFICATE OF INSURANCE TO OUR OFFICE\n\r");
		fprintf(fp, "        AS SOON AS POSSIBLE AND MAIL THE ORIGINAL TO THE CERTIFICATE\n\r");
		fprintf(fp, "        HOLDER (UNLESS DIRECTED OTHERWISE).\n\r\n\r");

		fprintf(fp, "        REQUESTED BY: %s\n\r", logsp.name);
		fprintf(fp, "                      %s\n\r", logsp.title);

		strcpy(faxNo, compInfo.insufax);
		strcpy(emailAddr, "");
		more = FALSE;
		break;

	    /*
	    * Shopdrawing status request
	    */
	    case 3:
		fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r        %s\n\r\n\r", tdate);
		fprintf(fp, "        %s\n\r", vd.contact); 
		fprintf(fp, "        %s\n\r", vd.name);
		fprintf(fp, "        %s\n\r", vd.address);
		fprintf(fp, "        %s, %s %s\n\r", vd.city, vd.state, vd.zip);
		fprintf(fp, "        Tel: %s\n\r", vd.tel);
		fprintf(fp, "        Fax: %s\n\r", vd.fax);
		fprintf(fp, "\n\r        Re: %s (Job #: %s)\n\r", jb->name, jb->code);
		fprintf(fp, "        Our PO #: %s\n\r", po.po_number); 

		fprintf(fp, "\n\r        Attn: Customer Service Department\n\r\n\r");
		fprintf(fp, "        In regards to the above referenced project, we are yet to receive\n\r");
		fprintf(fp, "        shop drawings and/or color selection(s).\n\r");
		fprintf(fp, "\n\r        In an effort to meet our customer's target delivery dates and\n\r");
		fprintf(fp, "        project completion date, we need to receive this information\n\r");
		fprintf(fp, "        immediately.  Please expedite and forward this material to our\n\r");
		fprintf(fp, "        office for submittion process.  Thank you,\n\r");

		fprintf(fp, "\n\r\n\r        Sincerely,\n\r");
		fprintf(fp, "\n\r\n\r        %s", sp.name);
		fprintf(fp, "\n\r        %s", sp.title);

		strcpy(faxNo, vd.fax);
		strcpy(emailAddr, vd.email);
		more = FALSE;
		tmpVd = tmpVd->next;
		if(tmpVd)
		{
		    strcpy(vdcode, tmpVd->vd.code);
		    idx = tmpVd->idx + 1;
		    s_read(VD, &vd, s_getno(VD, 0, vdcode));
		    s_close(VD);
		    more = TRUE;
		}

		break;

	    /* 
	    * Submital approval request
	    */
	    case 4:
		fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r        %s\n\r\n\r", tdate);
		fprintf(fp, "        %s\n\r", jb->agent); 
		fprintf(fp, "        %s\n\r", client->name);
		fprintf(fp, "        %s\n\r", client->address);
		fprintf(fp, "        %s, %s %s\n\r", client->city, client->state, client->zip);
		fprintf(fp, "        Tel: %s\n\r", client->tel);
		fprintf(fp, "        Fax: %s\n\r", client->fax);
		fprintf(fp, "\n\r        Re: %s (Job #: %s)\n\r", jb->name, jb->code);
//                fprintf(fp, "            %s\n\r", vd.product); /* changed on 5/3/98
		fprintf(fp, "            %s\n\r", "Submittal approval status");

		fprintf(fp, "\n\r        Dear %s:\n\r\n\r", name);
		fprintf(fp, "        We have not yet received the approved shop drawing and/or\n\r");
		fprintf(fp, "        color selection(s) for the %s.\n\r", vd.product); 
		fprintf(fp, "        Fabrication can not be released prior to receipt of this\n\r");
		fprintf(fp, "        information.  Current fabrication lead time for the subject\n\r");
		fprintf(fp, "        above is 4 - 6 weeks.  In an effort to meet your target\n\r");
		fprintf(fp, "        delivery dates and project completion date, we need to\n\r");
		fprintf(fp, "        receive this information immediately.\n\r");

		fprintf(fp, "\n\r\n\r        Sincerely,\n\r");
		fprintf(fp, "\n\r\n\r        %s", sp.name);
		fprintf(fp, "\n\r        %s", sp.title);
		more = FALSE;
		tmpVd = tmpVd->next;
		if(tmpVd)
		{
		    strcpy(vdcode, tmpVd->vd.code);
		    idx = tmpVd->idx + 1;
		    more = TRUE;
		}

		break;

	    /*
	    * Approval acknowledgement (Ship date)
	    */
	    case 5:
try3:           ans1 = message(M_SCR|M_KEY, "Requesting or advising a ship date? (R/A)");
		if(ans1 != 'R' && ans1 != 'A')
		    goto try3;

		fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r        %s\n\r\n\r", tdate);
		fprintf(fp, "        %s\n\r", jb->agent); 
		fprintf(fp, "        %s\n\r", client->name);
		fprintf(fp, "        %s\n\r", client->address);
		fprintf(fp, "        %s, %s %s\n\r", client->city, client->state, client->zip);
		fprintf(fp, "        Tel: %s\n\r", client->tel);
		fprintf(fp, "        Fax: %s\n\r", client->fax);
		fprintf(fp, "\n\r        Re: %s (Job #: %s)\n\r", jb->name, jb->code);
		fprintf(fp, "\n\r        Subject: %s\n\r", vd.product);

		fprintf(fp, "\n\r        Dear %s:\n\r\n\r", name);
		fprintf(fp, "        Herein, we acknowledge receipt of approved submittal(s) and/or\n\r");
		if(ans1 == 'A')
		{
		    fprintf(fp, "        shopdrawing(s) for the %s.  Material has\n\r", vd.product);
		    fprintf(fp, "        been released for fabrication and scheduled for shipment\n\r");
		    fprintf(fp, "        on/about %s.  Please kindly confirm that this\n\r", jb->sent);
		    fprintf(fp, "        date meets your requirements.\n\r");
		}
		else
		{
		    fprintf(fp, "        shopdrawing(s) for the %s.  Please kindly\n\r", vd.product);
		    fprintf(fp, "        advise when do you require the material to be on your job\n\r");
		    fprintf(fp, "        site. Please note that the current manufacturing lead time\n\r");
		    fprintf(fp, "        for this material is 4 - 6 weeks. Please do not hesitate to\n\r");
		    fprintf(fp, "        call me if you have any questions.\n\r");
		}
		fprintf(fp, "\n\r\n\r        Sincerely,\n\r");
		fprintf(fp, "\n\r\n\r        %s", sp.name);
		fprintf(fp, "\n\r        %s", sp.title);
		more = FALSE;
		tmpVd = tmpVd->next;
		if(tmpVd)
		{
		    strcpy(vdcode, tmpVd->vd.code);
		    idx = tmpVd->idx + 1;
		    more = TRUE;
		}

		break;

	    /*
	    * Approval acknowledgement (Reply)
	    */
	    case 6:

		fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r        %s\n\r\n\r", tdate);
		fprintf(fp, "        %s\n\r", jb->agent); 
		fprintf(fp, "        %s\n\r", client->name);
		fprintf(fp, "        %s\n\r", client->address);
		fprintf(fp, "        %s, %s %s\n\r", client->city, client->state, client->zip);
		fprintf(fp, "        Tel: %s\n\r", client->tel);
		fprintf(fp, "        Fax: %s\n\r", client->fax);
		fprintf(fp, "\n\r        Re: %s (Job #: %s)\n\r", jb->name, jb->code);
		fprintf(fp, "\n\r        Subject: %s\n\r", vd.product);

		fprintf(fp, "\n\r        Dear %s:\n\r\n\r", name);
		fprintf(fp, "        We are in receipt of submittal(s) and/or shopdrawing(s) for the\n\r");
		fprintf(fp, "        above referenced project.  In reviewing these submittals, we have\n\r");
		fprintf(fp, "        noted need for further clarification on: %s.\n\r", vd.product);
		
		fprintf(fp, "\n\r\n\r        Sincerely,\n\r");
		fprintf(fp, "\n\r\n\r        %s", sp.name);
		fprintf(fp, "\n\r        %s", sp.title);
		more = FALSE;
		break;

	    /*
	    * Request for field measurement
	    */
	    case 7:

		fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r        %s\n\r\n\r", tdate);
		fprintf(fp, "        %s\n\r", jb->agent);
		fprintf(fp, "        %s\n\r", client->name);
		fprintf(fp, "        %s\n\r", client->address);
		fprintf(fp, "        %s, %s %s\n\r", client->city, client->state, client->zip);
		fprintf(fp, "        Tel: %s\n\r", client->tel);
		fprintf(fp, "        Fax: %s\n\r", client->fax);
		fprintf(fp, "\n\r        Re: %s (Job #: %s)\n\r", jb->name, jb->code);
		fprintf(fp, "\n\r        Subject: %s\n\r", vd.product);
		fprintf(fp, "                 Field Measrement\n\r");

		fprintf(fp, "\n\r        Dear %s:\n\r\n\r", name);
		fprintf(fp, "        Herein, we acknowledge receipt of approved submittal(s) and/or\n\r");
		fprintf(fp, "        shopdrawing(s) for the above referenced project.  At your earliest\n\r");
		fprintf(fp, "        convenience, please advise when we can verify site conditions\n\r");
		fprintf(fp, "        and do field measurements or furnish us with GUARANTEED field\n\r");
		fprintf(fp, "        measurements and/or dimensions.  Please furnish GUARANTEED dimensions\n\r");
		fprintf(fp, "        if site conditions do not allow field measurements immediately,\n\r");
		fprintf(fp, "        and your project target completion date prevents any further\n\r");
		fprintf(fp, "        delay to our 6 - 8 weeks LEAD TIME.\n\r");

		fprintf(fp, "\n\r        Material can NOT be SCHEDULED for PRODUCTION without this information.\n\r");
		fprintf(fp, "        Please respond in writting as soon as possible.\n\r");
		fprintf(fp, "        Thank you.\n\r");

		fprintf(fp, "\n\r\n\r        Sincerely,\n\r");
		fprintf(fp, "\n\r\n\r        %s", sp.name);
		fprintf(fp, "\n\r        %s", sp.title);
		more = FALSE;
		break;

	    /* 
	    * Shipment status request
	    */
	    case 8:
		fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r        %s\n\r\n\r", tdate);
		fprintf(fp, "        %s\n\r", vd.contact); 
		fprintf(fp, "        %s\n\r", vd.name);
		fprintf(fp, "        %s\n\r", vd.address);
		fprintf(fp, "        %s, %s %s\n\r", vd.city, vd.state, vd.zip);
		fprintf(fp, "        Tel: %s\n\r", vd.tel);
		fprintf(fp, "        Fax: %s\n\r", vd.fax);
		fprintf(fp, "\n\r        Re: %s (Job #: %s)\n\r", jb->name, jb->code);
		fprintf(fp, "        Our PO #: %s\n\r", po.po_number); 

		fprintf(fp, "\n\r        Attn: Customer Service Department\n\r\n\r");
		if(po.release_date[0])
		{
		    fprintf(fp, "        In regards to the above referenced purchase order released\n\r");
		    fprintf(fp, "        on %s, we had requested a ship date of %s,\n\r", po.release_date, po.shipdate);
		    fprintf(fp, "        would you please advise the status of the order and confirm\n\r");
		    fprintf(fp, "        that it is on schedule. Please also advise the name of the\n\r");
		    fprintf(fp, "        freight company and the Pro number as soon as it is shipped\n\r"); 
		    fprintf(fp, "        so we could track it down.  Thank you,\n\r");
		}
		else
		{
		    fprintf(fp, "        In regards to the above referenced purchase order, we had\n\r");
		    fprintf(fp, "        requested a ship date of %s, would you please advise\n\r", po.shipdate);
		    fprintf(fp, "        the status of the order and confirm that it is on schedule.\n\r");
		    fprintf(fp, "        Please also advise the name of the freight company and\n\r");
		    fprintf(fp, "        the Pro number as soon as it is shipped so we could track\n\r"); 
		    fprintf(fp, "        it down.  Thank you,\n\r");
		}
		fprintf(fp, "\n\r\n\r        Sincerely,\n\r");
		fprintf(fp, "\n\r\n\r        %s", sp.name);
		fprintf(fp, "\n\r        %s", sp.title);

		strcpy(faxNo, vd.fax);
		strcpy(emailAddr, vd.email);
		more = FALSE;
		tmpVd = tmpVd->next;
		if(tmpVd)
		{
		    strcpy(vdcode, tmpVd->vd.code);
		    idx = tmpVd->idx + 1;
		    s_read(VD, &vd, s_getno(VD, 0, vdcode));
		    s_close(VD);
		    more = TRUE;
		}

		break;

	    /*
	    * Request for addition/missing material
	    */
	    case 9:
try1:           ans1 = message(M_SCR|M_KEY, "Is it additional, missing or replacement? (A/M/R)");
		if(ans1 != 'A' && ans1 != 'M' && ans1 != 'R')
		    goto try1;
try2:           ans2 = message(M_SCR|M_KEY, "Ship to job site, our office or GC office? (J/O/G)");
		if(ans2 != 'J' && ans2 != 'O' && ans2 != 'G')
		    goto try2;
		
		fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r        %s\n\r\n\r", tdate);
		fprintf(fp, "        %s\n\r", vd.contact); 
		fprintf(fp, "        %s\n\r", vd.name);
		fprintf(fp, "        %s\n\r", vd.address);
		fprintf(fp, "        %s, %s %s\n\r", vd.city, vd.state, vd.zip);
		fprintf(fp, "        Tel: %s\n\r", vd.tel);
		fprintf(fp, "        Fax: %s\n\r", vd.fax);
		fprintf(fp, "\n\r        Re: %s (Job #: %s)\n\r", jb->name, jb->code);
		fprintf(fp, "        Our PO #: %s\n\r", po.po_number); 

		fprintf(fp, "\n\r        Attn: Customer Service Department\n\r\n\r");
		fprintf(fp, "        In regards to the above referenced project, please ship the\n\r");
		if(ans1 == 'A')
		{
		    fprintf(fp, "        following additional material, using the original purchase\n\r");
		    fprintf(fp, "        order number:\n\r\n\r");
		}
		else if(ans1 == 'M')
		{
		    fprintf(fp, "        following missing material which was not part of the original\n\r");
		    fprintf(fp, "        shipment we received. Please forward proof of delivery if it\n\r");
		    fprintf(fp, "        was already delivered.\n\r\n\r");
		}
		else
		{
		    fprintf(fp, "        following replacement for damaged material. Also please have\n\r");
		    fprintf(fp, "        the freight company fax us a proof of delivery and claim form.\n\r\n\r");
		}

		if(idx == -1)
		    bg = ed = 0;
		else
		    get_pl_pos(idx, vdcode, &bg, &ed);
	
		n = s_nrecs(PL);
	 
		fprintf(fp, "%s  Quantity       Code                  Description",m);
		fprintf(fp, "\n\r%s  -------- ------------------- --------------------------------", m);
	
		for (i=0; i<n; i++)
		{
		    s_read(PL,&pl1,i);
		    if (!strcheck(vdcode,pl1.vdcode) && i >= bg && i <= ed) {
			fprintf(fp, "\n\r%s %9ld %-20s%s",m,(long)pl1.qty,pl1.pdcode,pl1.description);
		    }
		}
		
		fprintf(fp, "\n\r\n\r        In an effort to complete this project in a timely manner, we\n\r");
		fprintf(fp, "        need to receive this material as soon as possible. Please ship\n\r");
		if(ans2 == 'J')
		{
		    fprintf(fp, "        the material to original job site address.\n\r");
		}
		else if(ans2 == 'O')
		{
		    fprintf(fp, "        the material to our office.\n\r");
		}
		else
		{
		    fprintf(fp, "        the material to the GC office below:\n\r\n\r");
		    fprintf(fp, "\t\t        %s\n\r", jb->agent); 
		    fprintf(fp, "\t\t        %s\n\r", client->name);
		    fprintf(fp, "\t\t        %s\n\r", client->address);
		    fprintf(fp, "\t\t        %s, %s %s\n\r", client->city, client->state, client->zip);
		    fprintf(fp, "\t\t        Tel: %s\n\r", client->tel);
		    fprintf(fp, "\t\t        Fax: %s\n\r", client->fax);
		}

		fprintf(fp, "\n\r\n\r        Sincerely,\n\r");
		fprintf(fp, "\n\r\n\r        %s", sp.name);
		fprintf(fp, "\n\r        %s", sp.title);

		strcpy(faxNo, vd.fax);
		strcpy(emailAddr, vd.email);
		more = FALSE;
		break;

	    /*
	    * Request for clarification
	    */
	    case 10:
		fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r        %s\n\r\n\r", tdate);
		fprintf(fp, "        %s\n\r", jb->agent); 
		fprintf(fp, "        %s\n\r", client->name);
		fprintf(fp, "        %s\n\r", client->address);
		fprintf(fp, "        %s, %s %s\n\r", client->city, client->state, client->zip);
		fprintf(fp, "        Tel: %s\n\r", client->tel);
		fprintf(fp, "        Fax: %s\n\r", client->fax);
		fprintf(fp, "\n\r        Re: %s (Job #: %s)\n\r", jb->name, jb->code);
		fprintf(fp, "\n\r        Subject: %s\n\r", vd.product);

		fprintf(fp, "\n\r        Dear %s:\n\r\n\r", name);
		fprintf(fp, "        We have received the specifications and drawings on the above\n\r");
		fprintf(fp, "        job and have the following questions which need to be clearly\n\r");
		fprintf(fp, "        answered before we can proceed with take off/pricing:\n\r");
		fprintf(fp, "\n\r");
		fprintf(fp, "\n\r");
		fprintf(fp, "\n\r");
		fprintf(fp, "        Your prompt attention to this matter is appreciated.\n\r");

		fprintf(fp, "\n\r\n\r        Sincerely,\n\r");
		fprintf(fp, "\n\r\n\r        %s", sp.name);
		fprintf(fp, "\n\r        %s", sp.title);
		more = FALSE;
		break;

	    /*
	    * Return good authorization (RGA) request
	    */
	    case 11:
try4:           ans1 = message(M_SCR|M_KEY, "Is it incorrect shipment or job site change? (I/C)");
		if(ans1 != 'I' && ans1 != 'C')
		    goto try4;
		
		fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r        %s\n\r\n\r", tdate);
		fprintf(fp, "        %s\n\r", vd.contact); 
		fprintf(fp, "        %s\n\r", vd.name);
		fprintf(fp, "        %s\n\r", vd.address);
		fprintf(fp, "        %s, %s %s\n\r", vd.city, vd.state, vd.zip);
		fprintf(fp, "        Tel: %s\n\r", vd.tel);
		fprintf(fp, "        Fax: %s\n\r", vd.fax);
		fprintf(fp, "\n\r        Re: %s (Job #: %s)\n\r", jb->name, jb->code);
		fprintf(fp, "        Our PO #: %s\n\r", po.po_number); 

		fprintf(fp, "\n\r        Attn: Customer Service Department\n\r\n\r");
		fprintf(fp, "        In regards to the above referenced project, we need to return the\n\r");
		if(ans1 == 'C')
		    fprintf(fp, "        following list of material due to job site change.\n\r\n\r");
		else
		    fprintf(fp, "        following list of material due to incorrect shipment by your factory.\n\r\n\r");

		if(idx == -1)
		    bg = ed = 0;
		else
		    get_pl_pos(idx, vdcode, &bg, &ed);
	
		n = s_nrecs(PL);
	 
		fprintf(fp, "%s  Quantity       Code                  Description",m);
		fprintf(fp, "\n\r%s  -------- ------------------- --------------------------------", m);
	
		for (i=0; i<n; i++)
		{
		    s_read(PL,&pl1,i);
		    if (!strcheck(vdcode,pl1.vdcode) && i >= bg && i <= ed) {
			fprintf(fp, "\n\r%s %9ld %-20s%s",m,(long)pl1.qty,pl1.pdcode,pl1.description);
		    }
		}
		
		fprintf(fp, "\n\r\n\r        Please forward an RGA to our office with lables as soon as\n\r");
		fprintf(fp, "        possible so the above listed material may be returned.\n\r");

		fprintf(fp, "\n\r\n\r        Sincerely,\n\r");
		fprintf(fp, "\n\r\n\r        %s", sp.name);
		fprintf(fp, "\n\r        %s", sp.title);

		strcpy(faxNo, vd.fax);
		strcpy(emailAddr, vd.email);
		more = FALSE;
		break;
	    /*
	    * Warrantee letter
	    */
	    case 12:
		
		fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r                                 STANDARD WARRANTY FORM\n\r\n\r\n\r");
		fprintf(fp, "\n\r        JOB NAME: %s\n\r\n\r", jb->name);
		fprintf(fp, "\n\r        SUBSTANTIAL COMPLETION DATE: %s\n\r\n\r", tdate);
		fprintf(fp, "\n\r        TO: %s\n\r\n\r", client->name);
		fprintf(fp, "\n\r        FROM: AUTOMATION SOLUTIONS, INC.\n\r\n\r");

		fprintf(fp, "\n\r\n\r        The undersigned guarantees to the owner that he will be responsible\n\r");
		fprintf(fp, "        for faulty or defective materials, equipment and workmanship in the\n\r");
		fprintf(fp, "        %s work and will remedy any defects due thereto\n\r", vd.product);
		fprintf(fp, "        and pay for all damage to other work resulting therefrom which shall\n\r");
		fprintf(fp, "        appear within a period of one year from the date of which substantial\n\r");
		fprintf(fp, "        completion of the entire work is certified by the Architect.\n\r");
		
		fprintf(fp, "\n\r        During this period, upon written notice from the Owner to do so,\n\r");
		fprintf(fp, "        the undersigned shall proceed with due diligence at the undersigned's\n\r");
		fprintf(fp, "        expense to replace properly any defective materials, equipment or\n\r");
		fprintf(fp, "        perform any labor necessary to correct any defect in the work.  In\n\r");
		fprintf(fp, "        case the undersigned fails, upon reasonable notice to remedy such\n\r");
		fprintf(fp, "        defects, the Owner may furnish such materials or labor as are\n\r");
		fprintf(fp, "        necessary to bring the work up to the standard called for, and\n\r");
		fprintf(fp, "        the undersigned agrees to reimburse the Owner fully and promptly.\n\r");

		fprintf(fp, "\n\r\n\r\n\r\n\r        Signed: _____________________________  Date: ___/___/___\n\r");
/* removed on 1/6/98
		fprintf(fp, "\n\r\n\r\n\r\n\r\n\r        Subscribed and notarized this _____day of ________ 19___\n\r");
		fprintf(fp, "\n\r        Notary Public ____________________________________________________________\n\r");
*/
		more = FALSE;
		tmpVd = tmpVd->next;
		if(tmpVd)
		{
		    strcpy(vdcode, tmpVd->vd.code);
		    idx = tmpVd->idx + 1;
		    more = TRUE;
		}

		break;
	    
	    default:
		endflg = TRUE;
		break;
	}

	if (!endflg) {
	    filecls(fp);
	    TextEditor(fname, faxNo, emailAddr);
	    FileDelete(fname);
	}
    }
  }

    if(multi)
    {
	tmpVd = vdRec;
	while(tmpVd)
	{
	    vdRec = vdRec->next;
	    free(tmpVd);
	    tmpVd = vdRec;
	}
	FileDelete(tmpFile);
    }

    return(0);
}


/*****************************************************************************
letter_of_transmittal - Prepare Letter of Transmittal
*****************************************************************************/
INT letter_of_transmittal(JBREC *jb, ESREC *es, CDREC *client)
{
    VDREC tmpVD;
    SPREC sp;
    PLREC pl1, pl2;
    POREC po;
    CHAR fname[60], tdate[9], tmpFile[60];
    VD_ALT *vdRec, *tmpVd;
    USHORT num_vd = 0;
    FILE *fp;
    INT i, n, rec, tmpHandle;

    if (openplfile(es) == -1)
    {
	return(-1);
    }

    if ((tmpHandle = FileCreateTemp(tmpFile)) == -1)
	return (-1);
    FileSeek(tmpHandle, sizeof(USHORT), SEEK_SET);

	n = s_nrecs(PL);

    for(i = 0; i < n-1; i++)
    {
	s_read(PL, &pl1, i);
	/* skip the separator (special VD code) */
	if(!strnicmp(pl1.vdcode, "__", 2))
	    continue;

	s_read(PL, &pl2, i+1);
	if(strcmp(pl1.vdcode, pl2.vdcode))  // vd code not same
	{
	    rec = s_getno(VD, 0, pl1.vdcode);
	    if(rec != -1)
	    {
		s_read(VD, &tmpVD, rec);
		FileWrite(tmpHandle, &tmpVD, sizeof(VDREC));
		num_vd ++;
	    }
	}
    }
    // write the last one
    s_read(PL, &pl1, i);
    rec = s_getno(VD, 0, pl1.vdcode);
    if(rec != -1)
    {
	s_read(VD, &tmpVD, rec);
	FileWrite(tmpHandle, &tmpVD, sizeof(VDREC));
	num_vd ++;
    }
    if(num_vd > MAXVENDORS)
	num_vd = MAXVENDORS;
    FileSeek(tmpHandle, 0, SEEK_SET);
    FileWrite(tmpHandle, &num_vd, sizeof(USHORT));
    FileClose(tmpHandle);

    memset(tag_allowed, TRUE, MAXVENDORS * sizeof(BOOLEAN));

    vdRec = vd_chooser_multi(tmpFile, FALSE);                                    

    if(!vdRec)
    {
	FileDelete(tmpFile);
	return (-1);
    }

    StrDate(tdate, &today);

    /* Ask question if user wants to update the sub_date of the associated po 9/15/99 */
    if(message(M_SCR|M_KEY, "Do you want to update the submital date (y/N) ?") == 'Y')
    {
	/* Go through each PO and check whether the PO is for the current job */
	n = s_nrecs(PO);
	for(i = 0; i < n; i++)
	{
	    s_read(PO, &po, i);
	    if(!stricmp(jb->code, po.jbcode) || !stricmp(jb->code, po.jbcode2))
	    {
		/* Go through each selected VD, find whether the PO is for */
		tmpVd = vdRec;
		while(tmpVd)
		{
		    if(!stricmp(tmpVd->vd.code, po.vdcode))
		    {
			StrDate(po.sub_date, &today);
			s_write(PO, &po, i, FALSE);
			break;
		    }
		    tmpVd = tmpVd->next;
		}
	    }
	}
    }
	
    /*
    * create temporary file
    */
    if ((fp = fileopn(FileTempName(fname), "wb")) == NULL) {
	message(M_SCR|M_ERR|M_KEY, "Unable to create temporary file: %s - hit any key", fname);

    } else {

	memset(&sp, 0, sizeof(SPREC));
	s_read(SP, &sp, s_getno(SP, 0, es->spcode));
	s_close(SP);
	
	fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r                   L e t t e r  o f  T r a n s m i t t a l\n\r\n\r\n\r\n\r");
	fprintf(fp, "        TO:   %-34s Re:   %s\n\r", client->name, jb->name);
	fprintf(fp, "              %-34s JOB#: %s\n\r", client->address, jb->code);
	fprintf(fp, "              %s, %s %s\n\r", client->city, client->state, client->zip);
	fprintf(fp, "        FAX:  %s\n\r", client->fax);
	fprintf(fp, "        ATTN: %-34s DATE: %s\n\r\n\r", jb->agent, tdate);

	fprintf(fp, "\n\r        We are sending you herewith the following:\n\r");
	fprintf(fp, "           - AS Built Drawing\n\r");
	fprintf(fp, "           - Shopdrawings\n\r");
	fprintf(fp, "           - Submittals\n\r");
	fprintf(fp, "           - Others\n\r\n\r");
	fprintf(fp, "        These are transmitted as checked below:\n\r");
	fprintf(fp, "             For Approval                        Resubmitting for Approval\n\r");
	fprintf(fp, "             For Your Record                     For Field Use\n\r");
	fprintf(fp, "             Approved as Noted                   Return for Correction\n\r");

	fprintf(fp, "\n\r        Please return to us as soon as possible    copies for:\n\r");
	fprintf(fp, "             Our Record\n\r");
	fprintf(fp, "             Release for Fabrication\n\r");
	fprintf(fp, "             Others\r");

	fprintf(fp, "\n\r\n\r        ___________________________________________________\n\r");
	fprintf(fp, "        | COPIES | DESCRIPTION                            |\n\r");
	fprintf(fp, "        ___________________________________________________\n\r");
	fprintf(fp, "        ---------------------------------------------------\n\r");

	tmpVd = vdRec;
	while(tmpVd)
	{
	    fprintf(fp, "        |        | %-9s %-29s|\n\r", tmpVd->vd.code, " ");
	    fprintf(fp, "        ---------------------------------------------------\n\r");
	    tmpVd = tmpVd->next;
	}

	fprintf(fp, "\n\r\n\r        From: %s", sp.name);
	fprintf(fp, "\n\r\n\r        cc: Jobfile\n\r");

	filecls(fp);
	TextEditor(fname, client->fax, client->email);
	FileDelete(fname);
    }

	/* Release more memory */
    tmpVd = vdRec;
    while(tmpVd)
    {
	vdRec = vdRec->next;
	free(tmpVd);
	tmpVd = vdRec;
    }
    FileDelete(tmpFile);

    return(0);
}

/*****************************************************************************
rs_chooser() - Display a scroll window with all RS records, with multiple
	       selection ability
*****************************************************************************/
RS_ALT *rs_chooser(const CHAR* fileName, BOOLEAN all)
{
    STATIC MOUSE_CLICK rsKeys[] = {
	{256, 184, 344, 184, KEY_F6}, {368, 184, 408, 184, KEY_F7},
	{432, 184, 504, 184, KEY_F8}, {568, 184, 624, 184, KEY_F10},
	{0, 0, 0, 0, 0}
    };

    MOUSE_CLICK *lastP;
    RSREC rs;
    RS_ALT *rsList, *rsPtr;
    INT fh, i, rec, currec, lastrec;
    USHORT n, key;
    BOOLEAN endflg, *rs_tagged;

    /* inits */
    rsList = NULL;

    if ((fh = FileOpen(fileName)) != -1) {

	FileRead(fh, &n, sizeof(USHORT));
	if (n == 0) {

	    message(M_SCR|M_KEY, "No records to choose from - hit any key");

	} else {

	    rs_tagged = s_alloc(n*sizeof(BOOLEAN));
	    memset(rs_tagged, FALSE, n*sizeof(BOOLEAN));

	    store();
	    textattr(REVERSE);
	    setwindow(2, 14, 79, 23, "Choose Products", "Home End PgUp PgDn \x18 \x19", "                               F6=UNTAG_ALL  F7=TAG  F8=TAG_ALL       F10=DONE", NULL);

	    /* save last mouse click points */
	    lastP = points;

	    /* assign new mouse click points for this window */
	    points = rsKeys;

	    key = 0;
	    rec = 0;
	    lastrec = 1;
	    endflg = FALSE;
	    if(all)
	    {
		endflg = YES;
		memset(rs_tagged, TRUE, n*sizeof(BOOLEAN));
	    }
	    while(!endflg) {

		/*
		* don't refresh the screen if we don't need to
		*/
		if ((lastrec != rec) || (key == KEY_F6) || (key == KEY_F7) || (key == KEY_SPACE) || (key == KEY_F8) ) {

		    /*
		    * fill window with records
		    */
		    gotoxy(1, 1);

		    currec = rec - 5;
		    for (i=currec; i<currec+10; i++) {
			textattr((i == rec) ? REVERSE : YELLOW|ON_BLUE);
			if ((i >= 0) && (i < n)) {
			    FileSeek(fh, ((LONG)i*sizeof(RSREC))+sizeof(USHORT)+70, SEEK_SET);
			    FileRead(fh, &rs, sizeof(RSREC));
			    Print("\n\r%c %-8s  %-20s  %-30s", (*(rs_tagged+i) ? 0x10: ' '), rs.vdcode, rs.pdcode, rs.pddesc);
			} else {
			    Print("\n\r%-77s", " ");
			}
		    }
		}

		lastrec = rec;

		switch (key = nextkey()) {

		    /*
		    * move to the 1st record
		    */
		    case KEY_CTRL_PGUP:
		    case KEY_HOME:
			rec = 0;
			break;

		    /*
		    * Un-Tag all
		    */
		    case KEY_F6:

			memset(rs_tagged, FALSE, (n*sizeof(BOOLEAN)));
			break;

		    /*
		    * tag current record
		    */
		    case KEY_F7:
		    case KEY_SPACE:

			Assert(rec >= 0);

			*(rs_tagged+rec) = (*(rs_tagged+rec) ? NO : YES);
			if (rec < (n - 1))
			    rec++;

			break;

		    /*
		    * Tag all
		    */
		    case KEY_F8:

			memset(rs_tagged, TRUE, (n*sizeof(BOOLEAN)));
			break;

		    /*
		    * move to last record
		    */
		    case KEY_CTRL_PGDOWN:
		    case KEY_END:
			rec = n - 1;
			break;

		    /*
		    * move down 1 record
		    */
		    case KEY_DOWN:
			if (rec < (n - 1))
			    rec++;
			break;

		    /*
		    * move up one record
		    */
		    case KEY_SHFT_TAB:
		    case KEY_UP:
			if (rec)
			    rec--;
			break;

		    /*
		    * move down 1 page (10 records)
		    */
		    case KEY_PGDOWN:
			rec += 10;
			if (rec >= n)
			    rec = n - 1;
			break;

		    /*
		    * move up 1 page (10 records)
		    */
		    case KEY_PGUP:
			rec -= 10;
			if (rec < 0)
			    rec = 0;
			break;

		    /*
		    * user has quit
		    */
		    case KEY_ESC:
			endflg = YES;
			break;

		    /*
		    * user is done tagging RSs
		    */
		    case KEY_F10:
			endflg = YES;
			break;

		    /*
		    * user has chosen a single RS
		    */
		    case KEY_ENTER:
			endflg = YES;
			break;

		    default:
			beep();
			break;
		}
	    }

	    /* restore last mouse click points */
	    points = lastP;

	    restore();

	    /*
	    * continue if user did not hit ESCAPE
	    */
	    if (key != KEY_ESC) {

		/*
		* if user hit ENTER then they only picked one RS, so tag that one
		* and un-tag the rest
		*/
		if (key == KEY_ENTER) {
		    memset(rs_tagged, FALSE, n*sizeof(BOOLEAN));
		    rs_tagged[rec] = TRUE;
		}

		/*
		* create the RS_ALT linked list of tagged RSs
		*/
		for (i=0; i<n; i++) {

		    /*
		    * if this entry was tagged, then add it to the linked list
		    */
		    if (rs_tagged[i]) {
			FileSeek(fh, ((LONG)i*sizeof(RSREC))+sizeof(USHORT)+70, SEEK_SET);
			FileRead(fh, &rs, sizeof(RSREC));

			/*
			* if 1st node in list
			*/
			if (rsList == NULL) {
			    rsPtr = rsList = s_alloc(sizeof(RS_ALT));

			} else {
			    rsPtr->next = s_alloc(sizeof(RS_ALT));
			    rsPtr = rsPtr->next;
			}

			/*
			* add RS data to this new node
			*/
			rsPtr->next = NULL;
			memcpy(&rsPtr->rs, &rs, sizeof(RSREC));
			rsPtr->idx = i;
		    }
		}
	    }

	    s_free(rs_tagged);
	}
    }
    FileClose(fh);

    return(rsList);
}


/*****************************************************************************
vd_chooser_multi() - Display a scroll window with all VD records, with multiple
		     selection ability
*****************************************************************************/
VD_ALT *vd_chooser_multi(const CHAR* fileName, BOOLEAN all)
{
    STATIC MOUSE_CLICK vdKeys[] = {
	{256, 184, 344, 184, KEY_F6}, {368, 184, 408, 184, KEY_F7},
	{432, 184, 504, 184, KEY_F8}, {568, 184, 624, 184, KEY_F10},
	{0, 0, 0, 0, 0}
    };

    MOUSE_CLICK *lastP;
    VDREC vd;
    VD_ALT *vdList, *vdPtr;
    INT fh, i, rec, currec, lastrec;
    USHORT n, key;
    BOOLEAN endflg, *vd_tagged;

    /* inits */
    vdList = NULL;

    if ((fh = FileOpen(fileName)) != -1) {

	FileRead(fh, &n, sizeof(USHORT));
	if (n == 0) {

	    message(M_SCR|M_KEY, "No records to choose from - hit any key");

	} else {

	    vd_tagged = s_alloc(n*sizeof(BOOLEAN));
	    memset(vd_tagged, FALSE, n*sizeof(BOOLEAN));

	    store();
	    textattr(REVERSE);
	    setwindow(2, 14, 79, 23, "Choose Vendors", "Home End PgUp PgDn \x18 \x19", "                               F6=UNTAG_ALL  F7=TAG  F8=TAG_ALL       F10=DONE", NULL);

	    /* save last mouse click points */
	    lastP = points;

	    /* assign new mouse click points for this window */
	    points = vdKeys;

	    key = 0;
	    rec = 0;
	    lastrec = 1;
	    endflg = FALSE;
	    if(all)
	    {
		endflg = YES;
		memset(vd_tagged, TRUE, n*sizeof(BOOLEAN));
	    }
	    while(!endflg) {

		/*
		* don't refresh the screen if we don't need to
		*/
		if ((lastrec != rec) || (key == KEY_F6) || (key == KEY_F7) || (key == KEY_SPACE) || (key == KEY_F8) ) {

		    /*
		    * fill window with records
		    */
		    gotoxy(1, 1);

		    currec = rec - 5;
		    for (i=currec; i<currec+10; i++) {
			textattr((i == rec) ? REVERSE : YELLOW|ON_BLUE);
			if ((i >= 0) && (i < n)) {
			    FileSeek(fh, ((LONG)i*sizeof(VDREC))+sizeof(USHORT), SEEK_SET);
			    FileRead(fh, &vd, sizeof(VDREC));
			    Print("\n\r%c %-8.8s %-30.30s %-14.14s %-20.20s", ((*(vd_tagged+i))&&(*(tag_allowed+i)) ? 0x10: ' '), vd.code, vd.name, vd.tel, vd.product);
			} else {
			    Print("\n\r%-77s", " ");
			}
		    }
		}

		lastrec = rec;

		switch (key = nextkey()) {

		    /*
		    * move to the 1st record
		    */
		    case KEY_CTRL_PGUP:
		    case KEY_HOME:
			rec = 0;
			break;

		    /*
		    * Un-Tag all
		    */
		    case KEY_F6:

			memset(vd_tagged, FALSE, (n*sizeof(BOOLEAN)));
			break;

		    /*
		    * tag current record
		    */
		    case KEY_F7:
		    case KEY_SPACE:

			Assert(rec >= 0);

			*(vd_tagged+rec) = (*(vd_tagged+rec) ? NO : YES);
			if (rec < (n - 1))
			    rec++;

			break;

		    /*
		    * Tag all
		    */
		    case KEY_F8:

			memset(vd_tagged, TRUE, (n*sizeof(BOOLEAN)));
			break;

		    /*
		    * move to last record
		    */
		    case KEY_CTRL_PGDOWN:
		    case KEY_END:
			rec = n - 1;
			break;

		    /*
		    * move down 1 record
		    */
		    case KEY_DOWN:
			if (rec < (n - 1))
			    rec++;
			break;

		    /*
		    * move up one record
		    */
		    case KEY_SHFT_TAB:
		    case KEY_UP:
			if (rec)
			    rec--;
			break;

		    /*
		    * move down 1 page (10 records)
		    */
		    case KEY_PGDOWN:
			rec += 10;
			if (rec >= n)
			    rec = n - 1;
			break;

		    /*
		    * move up 1 page (10 records)
		    */
		    case KEY_PGUP:
			rec -= 10;
			if (rec < 0)
			    rec = 0;
			break;

		    /*
		    * user has quit
		    */
		    case KEY_ESC:
			endflg = YES;
			break;

		    /*
		    * user is done tagging VDs
		    */
		    case KEY_F10:
			endflg = YES;
			break;

		    /*
		    * user has chosen a single VD
		    */
		    case KEY_ENTER:
			endflg = YES;
			break;

		    default:
			beep();
			break;
		}
	    }

	    /* restore last mouse click points */
	    points = lastP;

	    restore();

	    /*
	    * continue if user did not hit ESCAPE
	    */
	    if (key != KEY_ESC) {

		/*
		* if user hit ENTER then they only picked one VD, so tag that one
		* and un-tag the rest
		*/
		if (key == KEY_ENTER) {
		    memset(vd_tagged, FALSE, n*sizeof(BOOLEAN));
		    vd_tagged[rec] = TRUE;
		}

		/*
		* create the VD_ALT linked list of tagged VDs
		*/
		for (i=0; i<n; i++) {

		    /*
		    * if this entry was tagged, then add it to the linked list
		    */
		    if (vd_tagged[i] && tag_allowed[i]) {
			FileSeek(fh, ((LONG)i*sizeof(VDREC))+sizeof(USHORT), SEEK_SET);
			FileRead(fh, &vd, sizeof(VDREC));

			/*
			* if 1st node in list
			*/
			if (vdList == NULL) {
			    vdPtr = vdList = s_alloc(sizeof(VD_ALT));

			} else {
			    vdPtr->next = s_alloc(sizeof(VD_ALT));
			    vdPtr = vdPtr->next;
			}

			/*
			* add VD data to this new node
			*/
			vdPtr->next = NULL;
			memcpy(&vdPtr->vd, &vd, sizeof(VDREC));
			vdPtr->idx = i;
		    }
		}
	    }

	    s_free(vd_tagged);
	}
    }
    FileClose(fh);

    return(vdList);
}

/*****************************************************************************
print_label_to_label_sheet() - Prints label info on label sheet.
*****************************************************************************/
VOID print_label_to_label_sheet(CHAR* name, CHAR* comp, CHAR* addr,
				CHAR* city, CHAR* state, CHAR* zip, CHAR* re)
{
    FILE *fp = defPrinter;
    USHORT key, x, y, i;
    CHAR str[5], *token;

repeat:
    str[0] = '\0';
    key = inpt(1, 25, "Enter label coordinator. E.g: 1,1  1,2 .. 1,10  2,1 .. 2,10 3,1 .. 3,10 : ", str, 4, CT_CHAR, NO);
    if(key == KEY_ESC1 || key == KEY_ESC || key == KEY_F10)
	return;

    token = strtok(str, ", ");
    x = atoi(token);
    token = strtok(NULL, ", ");
    y = atoi(token);

    if(x < 1 || x > 3 || y < 1 || y > 10)
    {
	message(M_SCR|M_KEY, "Invalid coordinator - any key to continue");
	goto repeat;
    }

    /* Line adjustment */
    y = (y - 1) * 6;
    for(i = 0; i < y; i++)
	fprintf(fp, "\n\r");

    /* Which column to print */
    if(x == 2)
	goto col2;
    else if(x == 3)
	goto col3;
    /* else x == 1 */

    fprintf(fp, "%-25.25s\n\r", name);
    if (comp)
	fprintf(fp, "%-25.25s\n\r", comp);
    fprintf(fp, "%-25.25s\n\r", addr);
    fprintf(fp, "%s %s, %s\n\r\n\r", city, state, zip);
    if (re)
	fprintf(fp, "RE: %-22.22s\n\r", re);

    fprintf(fp, "%s", prnCodes.reset);
    return;

col2:
    fprintf(fp, "\t\t\t    %-25.25s\n\r", name);
    if (comp)
	fprintf(fp, "\t\t\t    %-25.25s\n\r", comp);
    fprintf(fp, "\t\t\t    %-25.25s\n\r", addr);
    fprintf(fp, "\t\t\t    %s %s, %s\n\r\n\r", city, state, zip);
    if (re)
	fprintf(fp, "\t\t\t    RE: %-22.22s\n\r", re);

    fprintf(fp, "%s", prnCodes.reset);
    return;

col3:
    fprintf(fp, "\t\t\t\t\t\t\t%-25.25s\n\r", name);
    if (comp)
	fprintf(fp, "\t\t\t\t\t\t\t%-25.25s\n\r", comp);
    fprintf(fp, "\t\t\t\t\t\t\t%-25.25s\n\r", addr);
    fprintf(fp, "\t\t\t\t\t\t\t%s %s, %s\n\r\n\r", city, state, zip);
    if (re)
	fprintf(fp, "\t\t\t\t\t\t\tRE: %-22.22s\n\r", re);

    fprintf(fp, "%s", prnCodes.reset);
    return;
}


/*****************************************************************************
print_address_on_envelop() - Prints address info on real envelop.
*****************************************************************************/
VOID print_address_on_envelop(BOOLEAN job, CHAR* name, CHAR* comp, CHAR* addr,
			      CHAR* city, CHAR* state, CHAR* zip, CHAR* atten)
{
    FILE *fp = defPrinter;

    fprintf(fp, "%s", prnCodes.land);

    /* line adjustment */
    fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r");

    fprintf(fp, "\n\r\t\t\t                             %s",name);
    if(job)
    {
	if (comp) 
	    fprintf(fp, "\n\r\t\t\t                             c/o: %s",comp);
    }
    else
    {
	if (comp) 
	    fprintf(fp, "\n\r\t\t\t                             %s",comp);
    }
    fprintf(fp, "\n\r\t\t\t                             %s",addr);
    fprintf(fp, "\n\r\t\t\t                             %s %s, %s\n\r",city,state,zip);
    if(job)
    {
	if (atten)
	    fprintf(fp, "\n\r\t\t\t                             Attention: %s\n\r", atten);
    }
    else
    {
	if (atten)
	    fprintf(fp, "\n\r\t\t\t                             RE: %s\n\r",atten);
    }

    fprintf(fp, "%s", prnCodes.reset);
}

/*****************************************************************************
get_pl_pos() - Get the starting and ending index of PL based on the vendor index
	       in the selection window
*****************************************************************************/
VOID get_pl_pos(INT idx, CHAR *vdcode, INT *start, INT *end)
{
    INT i, j = 0, n;
    PLREC pl;

    n = s_nrecs(PL);
    if(!strcmp(vdcode, "*")) /* all VDs are selected */
    {
	*start = 0;
	*end = n - 1;
	return;
    }

    for(i = 0; i < MAXVENDORS && j < n; i++)
    {
	s_read(PL, &pl, j);

	if(!strnicmp(pl.vdcode, "__", 2)) /* ignore separator */
	{
	    j++;
	    i--;
	    continue;
	}

	if(!stricmp(pl.vdcode, vdlist[i]))
	{
	    if(i == idx-1)
		break;
	    else /* keep reading until next group */
	    {
		for(j = j+1; j < n; j++)
		{
		    s_read(PL, &pl, j);
		    if(stricmp(pl.vdcode, vdlist[i]))
			break;
		}
	    }
	}
    }

    *start = j;
    /* keep reading the group to get the ending index */
    for(i = j + 1; i < n; i++)
    {
	s_read(PL, &pl, i);
	if(!strnicmp(pl.vdcode, "__", 2)) /* ignore separator */
	    break;
	if(stricmp(vdcode, pl.vdcode))
	    break;
    }
    *end = i - 1;
}
