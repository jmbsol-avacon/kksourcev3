/**
*   DB.C        - Main Program Module for Kwik Kuote Distributor
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
#include <ctype.h>
#include <conio.h>
#include <math.h>
#include <io.h>

#ifdef DEBUG
#include <malloc.h>
#endif

#include "db_defs.h"
#include "vmouse.h"
#include "vmisc.h"
#include "vfile.h"

#ifndef REP_VER
#define VERSION "\nKwik Kuote Distributor v3.0 (C)Copyright 1994 Automation Solutions, Inc.\n"
#else
#define VERSION "\nKwik Kuote Representative v3.0 (C)Copyright 1994 Automation Solutions, Inc.\n" 
#endif

/*
* Set the stack size to 64k
*/
extern unsigned _stklen = 65535;


/*
* Global Variables
*/
CHAR rooms1[7][5];
CHAR rooms2[7][5];
struct date today;
CHAR *tempDir;
BOOLEAN mouseActive;
MOUSE_CLICK *points;
BOOLEAN inPO;
static BOOLEAN switchPDDSFlg = TRUE, needSave;
static CHAR tempPath[60], tempFileName[60], savePath[30], saveFileName[9];
extern CHAR vdlast[9];
extern FILE *defPrinter;
extern PRNCODES prnCodes;
extern CHAR *month[];
extern ESREC *tempEsPtr;
extern BOOLEAN tagged[MAXVENDORS];
extern CHAR vdlist[MAXVENDORS][9];
extern SYSREC sysvar;
extern COMPINFOREC compInfo;
extern INT lastporec;
extern BOOLEAN changeflg;
extern USREC curuser;

/* flags for time updating */
extern SHORT db_inpt_flag;
BOOLEAN form_screen_flag;

/**
*
* MAIN ROUTINE
*
*
*
*
*
**/
INT main(INT argc, CHAR *argv[])
{
    STATIC CHAR *prompt[] = {
	"MAIN MENU",
	"INFORMATION MENU",
	"QUOTATIONS",
#ifndef REP_VER
	"ACCOUNTING",
#endif
	"PRICE CHECK",
	"JOB INSTALLATION", /* rep version needs re-work as of 6/30/97 */
	"REPORTS",
	"WORD PROCESSOR",
	"SYSTEM MANAGER",
	"EXIT THE SYSTEM",
	NULL
    };
    INT choice, lastchoice;
    CHAR *userName, *userPw;
    CHAR pltemp[30];

#ifdef DEBUG
    ULONG cr;
#endif

#ifndef REP_VER
    STATIC MOUSE_CLICK pTemp[] = {
	{ 72,  64, 224,  64, KEY_1},  { 72,  80, 176,  80, KEY_2},
	{ 72,  96, 176,  96, KEY_3},  { 72, 112, 184, 112, KEY_4},
	{392,  64, 552,  64, KEY_5},  {392,  80, 472,  80, KEY_6},
	{392,  96, 528,  96, KEY_7},  {392, 112, 528, 112, KEY_8},
	{232, 128, 376, 128, KEY_9},  {0, 0, 0, 0, 0}
    };
#else
    STATIC MOUSE_CLICK pTemp[] = { 
	{ 72, 64, 224, 64, KEY_1},  { 72,  80, 176,  80, KEY_2},
	{ 72, 96, 184, 96, KEY_3},
	{392, 64, 472, 64, KEY_4},  {392,  80, 528,  80, KEY_5},
	{392, 96, 528, 96, KEY_6},  {232, 112, 376, 112, KEY_7},  
	{0, 0, 0, 0, 0}
    };
#endif

    /* show version info */
    printf(VERSION);

    /*
    * Dos 5.0 Help Compatability
    */
    if ((argc >= 2) && (!strcmp(argv[1], "/?"))) {
#ifndef REP_VER
	printf("\nUsage:  KK [username] [password]");
#else
	printf("\nUsage:  REP [username] [password]");
#endif
    } else {

	/* Miscelaneous error checking */
	ErrorChecking();

	/* initializations */
	Inits();
	lastchoice = 1;

	/* Shows KWIK KUOTE Intro Screen */
	IntroScreen();

#ifdef DEMO
	FileCopy("AUTOMAT.PL", ".\\PLIST\\00000001.AB1");  /* For Demo Only */
#endif

	/* show the main input screen (main menu) */
	mainscreen();

	userName = userPw = NULL;
	if (argc > 1) {
	    userName = argv[1];
	    if (argc > 2)
		userPw = argv[2];
	}

#ifdef DEBUG
	/* keep track of available memory */
	cr = coreleft();
#endif

	/*
	* Get UserName & Password
	*/
	if (getsec(userName, userPw) == -1)
	    quit(0);

	/* Loads Company and Warehouse info */
	GetCompInfo();
	
	/*
	* Init time update flag
	*/
	db_inpt_flag = 0;
	form_screen_flag = FALSE;

	do {

	    /*
	    * while user doesn't quit
	    */
	    do {

		/* register the main menu mouse codes */
		points = pTemp;
		
		switch (choice = menu(prompt, 2, 10, 50, lastchoice, HLP_MAINMENU)) {
		    /*
		    * Information Menu
		    */
		    case 1:
			lastchoice = choice;
			s_openfiles(3, CD, VD, SP);
                        infomenu();
			break;

		    /*
		    * Job Control
		    */
		    case 2:
			lastchoice = choice;
			s_openfiles(3, ES, JB, PO);
			db_inpt(JB);
			break;
#ifndef REP_VER
		    /*
		    * Job Accounting
		    */
		    case 3:
			lastchoice = choice;
			s_openfiles(3, ES, JB, PO);
			lastporec = s_nrecs(PO) - 1;
			db_inpt(PO);
			break;

		    /*
		    * Price Check
		    */
		    case 4:
			lastchoice = choice;

			/* save PL path */
			strcpy(pltemp, db[PL].path);

			s_tempfile(PL);

			tempEsPtr = NULL;
			db_inpt(PL);
			s_delfile(PL);

			/* restore PL path */
			strcpy(db[PL].path, pltemp);
			break;

		    /*
		    * Job Installation
		    */
		    case 5:
			lastchoice = choice;
			if (seccheck(IN, 'F'))
			    JobInstallation();
			break;

		    /*
		    * Reports
		    */
		    case 6:
			lastchoice = choice;
			if (seccheck(RP, 'F'))
			    reportmenu();
			break;

		    /*
		    * Text Editor
		    */
		    case 7:
			lastchoice = choice;

			/*
			* If there is no external editor set up then use the internal editor
			*/
			if (!*sysvar.editor) {
			    TextEditor(NULL, NULL, NULL);
			} else {
			    SysCmd(sysvar.editor);
			}
			break;

		    /*
		    * System Manager
		    */
		    case 8:
			lastchoice = choice;
			sysmgr();
			break;

		    case 9:
		    case KEY_F10:
			choice = 9;
			break;
#else
		    /*
		    * Price Check
		    */
		    case 3:
			lastchoice = choice;

			/* save PL path */
			strcpy(pltemp, db[PL].path);

			s_tempfile(PL);

			tempEsPtr = NULL;
			db_inpt(PL);
			s_delfile(PL);

			/* restore PL path */
			strcpy(db[PL].path, pltemp);
			break;

		    /*
		    * Reports
		    */
		    case 4:
			lastchoice = choice;
			if (seccheck(RP, 'F'))
			    reportmenu();
			break;

		    /*
		    * Text Editor
		    */
		    case 5:
			lastchoice = choice;

			/*
			* If there is no external editor set up then use the internal editor
			*/
			if (!*sysvar.editor) {
			    TextEditor(NULL, NULL, NULL);
			} else {
			    SysCmd(sysvar.editor);
			}
			break;

		    /*
		    * System Manager
		    */
		    case 6:
			lastchoice = choice;
			sysmgr();
			break;

		    case 7:
		    case KEY_F10:
			choice = 8;
			break;

#endif                    
		    default:
			beep();
			break;
		}
		s_closeall();

	    } while (choice != 9);

	    points = NULL;

	} while (getsec(NULL, NULL) != -1);

	/* Is there a memory leak? */
	Assert(cr == coreleft());

	quit(0);
    }
    return(0);
}

/*************** Distributor Specific Functions **************************/

/****************************************************************************
es_room_schedule() - Runs Room Schedule from ES Screen
    ident:  db[] type
    c:  pointer to cursor structure
    es: Pointer to current estimate record
****************************************************************************/
INT es_room_schedule(DB_TYPE type, IN_POS *c, ESREC *es)
{
    INT i, anew;
    CHAR drive[3], ext[4], name[60], path[60];

    s_vcheck(type, es, c->ypos);
    strcpy(db[RS].fname, es->jbcode);
    strcpy(db[RS].extension, "001");
    anew = s_open(RS);
    plfile(es);
    FileSeek(db[RS].file, 2, SEEK_SET);
    FileRead(db[RS].file, &rooms1, 35);
    FileRead(db[RS].file, &rooms2, 35);

    for (i=0; i<=6; i++) {
	StrReplaceChar(db[RS].f[i+2].header, db[RS].f[i+2].header[5], '\0');
	if (anew == -2)
	{
	    memset(&rooms1, 0, 35);
	    memset(&rooms2, 0, 35);
	}
	strcpy(db[RS].f[i+2].header, rooms1[i]);
	strcat(db[RS].f[i+2].header, "|");
	strcat(db[RS].f[i+2].header, rooms2[i]);
    }

    if (anew == -2) {
	FileSeek(db[RS].file, 2, SEEK_SET);
	FileWrite(db[RS].file, &rooms1, 35);
	FileWrite(db[RS].file, &rooms2, 35);
    }

    /*
    * save the path and name of the current RS file
    */
    s_close(RS);
    strcpy(savePath, db[RS].path);
    strcpy(saveFileName, db[RS].fname);

    /*
    * make temporary copy of RS files, and work from that temp copies
    */
    _splitpath(_fullpath(tempFileName, FileTempName(tempFileName), 60), drive, path, name, ext);
    drive[2] = '\0';
    strcpy(tempPath, drive);
    strcat(tempPath, path);
    strcpy(tempFileName, name);

    i = 1; 
    sprintf(name, "%s%s.%03d", savePath, saveFileName, i);
    while(access(name, 0x00) == 0)
    {
	sprintf(path, "%s%s.%03d", tempPath, tempFileName, i);
	FileCopy(name, path);
	i++;
	sprintf(name, "%s%s.%03d", savePath, saveFileName, i);
    }

    strcpy(db[RS].path, tempPath);
    strcpy(db[RS].fname, tempFileName);

    tempEsPtr = es;
    store();

    needSave = TRUE;

    /* don't print time & date in RS screen */
    form_screen_flag = TRUE;
    db_inpt(RS);
    form_screen_flag = FALSE;

    /*
    * if user wants to save changes, then copy new stuff over the
    * original RS files
    */
    s_close(RS);
    if (needSave && savecheck()) {
	i = 1; 
	sprintf(name, "%s%s.%03d", tempPath, tempFileName, i);
	while(access(name, 0x00) == 0)
	{
	    sprintf(path, "%s%s.%03d", savePath, saveFileName, i);
	    FileCopy(name, path);
	    i++;
	    sprintf(name, "%s%s.%03d", tempPath, tempFileName, i);
	}
    }

    /* delete the temp file */
    i = 1; 
    sprintf(name, "%s%s.%03d", tempPath, tempFileName, i);
    while(access(name, 0x00) == 0)
    {
	FileDelete(name);
	i++;
	sprintf(name, "%s%s.%03d", tempPath, tempFileName, i);
    }
    restore();

    /*
    * restore the path, name and extension of the current RS file
    */
    strcpy(db[RS].path, savePath);
    strcpy(db[RS].fname, saveFileName);
    strcpy(db[RS].extension, "001");

//    s_close(RS);
    return(0);
}


INT cd_chooser(BOOLEAN *list)
{
    STATIC MOUSE_CLICK cdKeys[] = {
	{256, 184, 344, 184, KEY_F6}, {368, 184, 408, 184, KEY_F7},
	{432, 184, 504, 184, KEY_F8}, {568, 184, 624, 184, KEY_F10},
	{0, 0, 0, 0, 0}
    };

    MOUSE_CLICK *lastP;
    CDREC cd, cdTemp;
    INT i, rec, currec, lastrec;
    USHORT n, key;
    BOOLEAN endflg;

    /* can't write to a null pointer */
    Assert(list != NULL);

    if ((n = s_nrecs(CD)) == 0) {
	message(M_SCR | M_KEY, "No records to choose from - hit any key");
	return(-1);
    }

    memset(list, FALSE, n*sizeof(BOOLEAN));

    store();
    textattr(REVERSE);
    setwindow(2, 14, 79, 23, "Choose Clients", "Home End PgUp PgDn \x18 \x19", "                               F6=UNTAG_ALL  F7=TAG  F8=TAG_ALL       F10=DONE", NULL);

    /* save last mouse click points */
    lastP = points;

    /* assign new mouse click points for this window */
    points = cdKeys;

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
		    s_read(CD, &cd, i);
		    Print("\n\r%c %-8s  %-32s  %-14s  %-14s", (*(list+i) ? 0x10: ' '), cd.code, cd.name, cd.tel, cd.fax);
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
	    case KEY_CTRL_HOME:
	    case KEY_HOME:
		rec = 0;
		break;

	    /*
	    * Un-Tag all
	    */
	    case KEY_F6:

		memset(list, FALSE, (n*sizeof(BOOLEAN)));
		break;

	    /*
	    * tag current record
	    */
	    case KEY_F7:
	    case KEY_SPACE:

		Assert(rec >= 0);

		*(list+rec) = (*(list+rec) ? NO : YES);
		if (rec < (n - 1))
		    rec++;

		break;

	    /*
	    * Tag all
	    */
	    case KEY_F8:

		memset(list, TRUE, (n*sizeof(BOOLEAN)));
		break;

	    /*
	    * move to last record
	    */
	    case KEY_CTRL_PGDOWN:
	    case KEY_CTRL_END:
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
	    * user is done tagging Cleints
	    */
	    case KEY_F10:
		endflg = YES;
		break;

	    /*
	    * user has chosen a single Client
	    */
	    case KEY_ENTER:
		endflg = YES;
		break;

	    default:
		if (toupper(key) >= 'A' && toupper(key) <= 'Z') {

		    memset(&cd, 0, sizeof(CDREC));

		    cd.code[0] = (char)key;
		    cd.code[1] = '*';
		    cd.code[2] = '\0';
		    if ((i = s_findrec(CD, &cdTemp, &cd, NULL, 0)) != -1) {
			rec = i;
			break;
		    }
		}

		beep();
		break;
	}
	message(M_SCR, NULL);
    } while (!endflg);

    /* restore last mouse click points */
    points = lastP;

    restore();

    /*
    * continue if user did not hit ESCAPE
    */
    if (key == KEY_ESC) {
	return(-1);
    }

    /*
    * if user hit ENTER then they only picked one PO, so tag that one
    * and un-tag the rest
    */
    if (key == KEY_ENTER) {
	memset(list, FALSE, n*sizeof(BOOLEAN));
	*(list+rec) = TRUE;
    }

    return(0);
}


/****************************************************************************
jb_installation() - Shows Installation Form for all POs with IN in the vdcode
		    for current Job from Job screen
*****************************************************************************/
INT jb_installation(IN_POS *c, DB_TYPE type, VOID *ptr)
{
    JBREC *jb, jbrec;
    POREC po, *poptr, inptPo;
    ESREC es;
    CDREC client;
    CHAR tdate[9], fname[60], tmpFile[60];
    INT vec, i, n, tmpHandle, count, key;
    PO_ALT *tmpPo;
    FILE *fp;
    char poCode[10], tmpCode[9], prompt[80];   /* poCode[9] is written below */

    if (type == JB)
	jb = (JBREC *) ptr;
    else /* must be PO (for now) */
    {
	poptr = (POREC *) ptr;
/* This is commented out on 6/26/97 by request from MJ 
	if(strncmp(poptr->vdcode, "IN", 2))
	{
	    message(M_SCR|M_KEY, "This PO's VD code is not beginning with IN - hit any key");
	    return (0);
	}
*/
/* The following code is added on 9/23/97 by request from MJ 
	vec = s_getno(VD, 0, poptr->vdcode);
	if(vec == -1)
	{
	    message(M_SCR|M_KEY, "No vendor is defined - hit any key");
	    return (0);
	}
	s_read(VD, &vd, vec);
	if(strnicmp(vd.product, "INSTAL", 6))
	{
	    message(M_SCR|M_KEY, "This PO's vendor is not an installation vendor - hit any key");
	    return (0);
	}
then commented out again on 9/24/97 */
	vec = s_getno(JB, 0, poptr->jbcode);
	if(vec != -1)
	{
	    s_read(JB, &jbrec, vec);
	    jb = &jbrec;
	}
	else
	{
	    message(M_SCR|M_KEY, "No job is found - hit any key");
	    return (0);
	}

    }

    FileTempName(fname);

    if ((fp = fileopn(fname, "wt")) == NULL) {
	message(M_SCR|M_ERR|M_KEY, "Unable to open %s - hit any key", fname);
    } else {

	message(M_SCR, "Working. . .");

	/*
	* create the temp file and continue on success
	*/
	if ((tmpHandle = FileCreateTemp(tmpFile)) != -1) {

	    /* seek past 1st 2 bytes */
	    FileSeek(tmpHandle, sizeof(USHORT), SEEK_SET);

	    if (type == JB)
	    {
		/*
		* copy each found PO to a temp PO file
		*/
		count = 0;
		n = s_nrecs(PO);
		for (i = 0; i < n; i++) {
		    s_read(PO, &po, i);

		    /*
		    * write each match to the temp file
		    */
		    if (!stricmp(jb->code, po.jbcode) && 
			!strncmp(po.vdcode, "IN", 2)) {

			FileWrite(tmpHandle, &po, sizeof(POREC));
			count++;
		    }
		}
	    }
	    else /* alread a PO */
	    {
		count = 1;
		FileWrite(tmpHandle, poptr, sizeof(POREC));
	    }

	    /*
	    * if no records found then inform the user
	    */
	    if (count == 0) {
		message(M_SCR|M_KEY, "There are no POs beginning with IN for this Job - hit any key");
		FileClose(tmpHandle);
		FileDelete(tmpFile);  
		filecls(fp);
		return (0);
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
		if (type == PO)
		{
		    tmpPo = poptr;
		    tmpPo = s_alloc(sizeof(PO_ALT));

		    tmpPo->next = NULL;
		    memcpy(&tmpPo->po, poptr, sizeof(POREC));

		    /* also need to ask questions regarding enter PO number to be installed
		       and whether this is a regular installation sheet or a subcontract */
		    sprintf(prompt, "Enter PO # for list of items to be installed. <Return> for: %-9s", poptr->po_number);
		    memset(poCode, 0, 9);
		    key = inpt(1, 25, prompt, poCode, 9, CT_CHAR, NO); 
		    poCode[9] = '\0';
		    if(key == KEY_ESC1 || key == KEY_ESC || key == KEY_F10 || key == KEY_ENTER)
			poCode[0] = '\0';
		    else
		    {
			sprintf(tmpCode, "%s", "00000000");
			vec = strlen(poCode);
			for(i = 0; i < vec; i++)
			    tmpCode[8 - vec + i] = poCode[i]; /* tmpCode will have 000000xx, if poCode is xx */

			strcpy(poCode, tmpCode);
			vec = s_getno(PO, 0, poCode);
			if(vec == -1 && strcmp(poCode, "00000000"))
			    message(M_SCR|M_KEY, "PO # '%s' is not found. Default to none - hit any key", poCode);
			else if(vec != -1)
			    s_read(PO, &inptPo, vec);

			if(vec == -1)
			    poCode[0] = '\0';
		    }

		    key = message(M_SCR|M_KEY, "Installation Sheet or Subcontract Agreement (I/S) ?");
		    if(key == 'S' || key == 's')
		    {
			filecls(fp);
			FileDelete(fname); /* the following call will handle fname */
			po_release_fabrication(c, poptr, SUBCONTRACT_AGREEMENT);
			return (0);
		    }
		}

		if (type == PO || (tmpPo = po_chooser_one(tmpFile)) != NULL) {

		    StrDate(tdate, &today);

		    s_open(ES);
		    vec = s_getno(ES, 0, jb->code);
		    s_read(ES, &es, vec);
		    s_close(ES);

		    s_open(CD);
		    vec = s_getno(CD, 0, es.client_code);
		    s_read(CD, &client, vec);
		    s_close(CD);

		    fprintf(fp, "\n\r");
		    fprintf(fp, "DATE OF REQUEST: %-8s                     INSTALL PO#: %-8s\n\r", tdate, tmpPo->po.po_number);
		    fprintf(fp, "INSTALLATION BY: %-8s                     JOB NUMBER: %-8s:\n\r", tmpPo->po.vdcode, jb->code);
		    fprintf(fp, "_______________________________________________________________________________\n\r\n\r");
		    fprintf(fp, "JOB NAME: %-30s      GC NAME: %-25s\n\r", jb->name, client.name);
		    fprintf(fp, "ADDRESS: %-30s       PROJECT MANAGER: %-20s\n\r", jb->address, jb->agent);
		    fprintf(fp, "         %-16s %-2s %-10s       SITE CONTACT: %-20s\n\r", jb->city, jb->state, jb->zip, jb->pmgr);
		    fprintf(fp, "TEL #: %-30s         SITE TEL#: %-20s\n\r", client.tel, jb->stel);
		    fprintf(fp, "_______________________________________________________________________________\n\r\n\r");
		    
		    fprintf(fp, "INSTALLATION SCOPE:\n\r\n\r");

		    /* insert a list here if poCode is given */
		    if(vec != -1 && poCode[0] != '\0')
		    {
			ESREC es;
			CDREC client;
			VDREC vd;
			JBREC jb;
			SPREC sp;
			PLREC pl;
			INT vec;

			s_open(ES);
			if ((vec = s_getno(ES, 0, inptPo.jbcode)) == -1) {
			    message(M_SCR|M_KEY, "Estimate is not defined for the given PO - hit any key");
			    return(-1);
			}
			s_read(ES, &es, vec);
			s_close(ES);

			s_open(CD);
			if ((vec = s_getno(CD, 0, es.client_code)) == -1) {
			    message(M_SCR|M_KEY, "Client is not defined for the given PO - hit any key");
			    return(-1);
			}
			s_read(CD, &client, vec);
			s_close(CD);

			s_open(JB);
			if ((vec = s_getno(JB, 0, inptPo.jbcode)) == -1) {
			    message(M_SCR|M_KEY, "Job is not defined for the given PO - hit any key");
			    return(-1);
			}
			s_read(JB, &jb, vec);
			s_close(JB);

			s_open(SP);
			if ((vec = s_getno(SP, 0, inptPo.spcode)) == -1) {
			    message(M_SCR|M_KEY, "Sales Person is not defined for the given PO - hit any key");
			    return(-1);
			}
			s_read(SP, &sp, vec);
			s_close(SP);

			if (inptPo.vdcode[0]) {
			    if ((vec = s_getno(VD, 0, inptPo.vdcode)) == -1) {
				message(M_SCR|M_KEY, "Vendor is not defined for the given PO - hit any key");
				return(-1);
			    }
			    s_read(VD, &vd, vec);
			} 

			if (openplfile(&es) == -1) {
			    message(M_SCR|M_KEY, "There are no products in the Part List for the given PO - hit any key");
			    return(-1);
			}

			n = s_nrecs(PL);
			fprintf(fp, "  Qty     Product Code         Product Description\n\r");
			fprintf(fp, "_______________________________________________________________________________\n\r");
			for (i=0; i<n; i++)
			{
			    s_read(PL, &pl, i);
			    if (strcheck(vd.code, pl.vdcode))
				continue;

			    if ((vec = s_getno(VD, 0, pl.vdcode)) == -1) {
				message(M_SCR|M_KEY, "Undefined vendor in product list - hit any key");
				continue;
			    }
			    s_read(VD, &vd, vec);
			    if (pl.qty)
				fprintf(fp, "%5ld", (long)pl.qty);
			    else
				fprintf(fp, "     ");
			    fprintf(fp, "     %-21s%-31s\n\r", pl.pdcode, pl.description);
			}
		    }

		    fprintf(fp, "_______________________________________________________________________________\n\r\n\r");
		    fprintf(fp, "COMMENTS: \n\r\n\r");
/*                  fprintf(fp, "_____ TOILET PARTITIONS                              _____ FIRE EXTINGUISH/CABS\n\r");
		    fprintf(fp, "_____ TOILET ACCESSPROES                             _____ SIGNAGE\n\r");
		    fprintf(fp, "_____ LOCKERS                                        _____ BUMPER RAIL\n\r");
		    fprintf(fp, "_____ PROJECTION SCREENS                             _____ FLOOR MATS\n\r");
		    fprintf(fp, "_____ CHAULKBOARDS/TACKBOARDS                        _____ CURTAIN TRACK\n\r");
		    fprintf(fp, "_____ POSTAL SPECTALTIES                             _____ OPERABLE PARTITIONS\n\r");
		    fprintf(fp, "_____ UNIT KITCHENS                                  _____ FLAGPOLES\n\r");
		    fprintf(fp, "_____ CORNER GUARDS                                  _____ TELEPHONE ENCLOSURES\n\r");
		    fprintf(fp, "          _____OTHER_________________________________________________\n\r\n\r");        
		    fprintf(fp, "COMMENTS: _____________________________________________________________________\n\r");
		    fprintf(fp, "_______________________________________________________________________________\n\r");
		    fprintf(fp, "_______________________________________________________________________________\n\r");
		    fprintf(fp, "_______________________________________________________________________________\n\r");
		    fprintf(fp, "_______________________________________________________________________________\n\r\n\r"); */
		    fprintf(fp, "_______________________________________________________________________________\n\r\n\r");

		    fprintf(fp, "INSTALLATION REQUEST DATE: %-8s    SCHEDULE INSTALLATION DATE:_________\n\r", tmpPo->po.shipdate);
		    fprintf(fp, "_______________________________________________________________________________\n\r\n\r");
		    
		    fprintf(fp, "DATE: _______  ARRIVAL TIMES: _______  DEPART TIME: _______  MAN HOUR: ______\n\r");
		    fprintf(fp, "DATE: _______  ARRIVAL TIMES: _______  DEPART TIME: _______  MAN HOUR: ______\n\r");
		    fprintf(fp, "DATE: _______  ARRIVAL TIMES: _______  DEPART TIME: _______  MAN HOUR: ______\n\r");
		    fprintf(fp, "                                           RATE: _______  TOTAL HOURS: ______\n\r\n\r");
		    fprintf(fp, "FIELD COMMENTS: \n\r");
		    fprintf(fp, "_______________________________________________________________________________\n\r");
		    fprintf(fp, "_______________________________________________________________________________\n\r");
		    fprintf(fp, "_______________________________________________________________________________\n\r");
		    fprintf(fp, "_______________________________________________________________________________\n\r");
		    fprintf(fp, "_______________________________________________________________________________\n\r");
		    fprintf(fp, "_______________________________________________________________________________\n\r");
		    fprintf(fp, "_______________________________________________________________________________\n\r");
		    fprintf(fp, "_______________________________________________________________________________\n\r");
		    fprintf(fp, "_______________________________________________________________________________\n\r\n\r");
		    fprintf(fp, "PUNCH LIST: _____YES   _____NO                  COMPLETE: _____YES  _____NO\n\r\n\r");
		    
		}
	    }

	    FileDelete(tmpFile);
	}

	filecls(fp);
	if(tmpPo != NULL)
	{
	    TextEditor(fname, client.fax, client.email);
	    free(tmpPo);
	}
	FileDelete(fname);
    }

    return(0);
}

/*****************************************************************************
jb_requisition() - Generates the job requisition form
*****************************************************************************/
INT jb_requisition(JBREC *jb, BOOLEAN prnForm)
{
/* the first part of the code is to display the markup window */
/* the code is borrowed from INT set_rate(ESREC *es, CHAR *ptr) */
    STATIC MOUSE_CLICK pSetRate[MAXVENDORS + 7];
    MOUSE_CLICK *lastP;
    POREC po;
    SHORT rate[MAXVENDORS], old_rate[MAXVENDORS], use_rate[MAXVENDORS];
    CHAR list[MAXVENDORS][11], save_list[MAXVENDORS][11];
    SHORT index[MAXVENDORS], temp_index[MAXVENDORS];
    BOOLEAN chpo[MAXVENDORS];
    INT choice=0, lastchoice=0, n, i, j=0, k, jj, vec, co_index, ans, lineAdj;
    BOOLEAN endflg, nextpage = FALSE, retainflg = TRUE, retEntered = FALSE, shift = FALSE;
    FLOAT cur_retain = 0.1, pre_retain = 0.1, diff_retain;
    FLOAT tmpf = 0, tmpf1 = 0, tmpnegf = 0, tmpnegf1 = 0;
    CHAR tmpstr[10], tmpstr1[40], tmpstr2[40], firstPODate[9];
    struct date coPODate;

    FILE *fp;
    CHAR tdate[9], fname[60];
    ESREC es;
    CDREC client;
    VDREC vender;
    FLOAT t_bid = 0, t_invtd = 0, t_invwip = 0, t_totaltd = 0, t_balance = 0, t_ret = 0;
    FLOAT t_co_bid = 0, t_co_invtd = 0, t_co_invwip = 0, t_co_totaltd = 0, t_co_balance = 0, t_co_ret = 0;
    FLOAT bid, invtd, invwip, totaltd, comp, balance, ret;
    CHAR retainagestr[3], period[9], app_no[4], period_from[9];
    USHORT key;

    FLOAT numEight;

    if(prnForm && (!SelectPrinter() || printercheck() == -1))
	return -1;

    /* inits */
    endflg = FALSE;

    for (i=0; i<MAXVENDORS; i++) {
	tagged[i] = NO;
	list[i][0] = '\0';
	save_list[i][0] = '\0';
	index[i] = 0;
	temp_index[i] = 0; /* only used temporary */
	rate[i] = 0;
	old_rate[i] = 0;
	use_rate[i] = 0;
	chpo[i] = FALSE;
    }

    n = s_nrecs(PO);
    for (i=0; i<n && j<MAXVENDORS; i++) {
	s_read(PO, &po, i);
	if (!strcmp(po.jbcode, jb->code) || !strcmp(po.jbcode2, jb->code)) {
	    /* set the appropriate flag for change order */
	    if(!strcmp(po.jbcode2, jb->code))
		chpo[j] = TRUE;
/* the following else part is not needed, since we only rely on jbcode2 to
   identify a change order. Previously, we say if two POs have the same
   jb code and same VD code, the second PO is a change order. This is not true
   as of 12/29/97 from Majid */
/*
	    else
	    {
		// go through the existing save_list to check vendor code 
		for(k = 0; k < j; k++)
		{
		    if(!stricmp(save_list[k], po.vdcode)) // change order 
		    {
			chpo[j] = TRUE;
			break;
		    }
		}
	    }
*/ 
	    strcpy(save_list[j], po.vdcode); 
	    temp_index[j] = i;
	    old_rate[j] = atoi(po.cur_retanige);

	    j++;
	}
    }

    /* go through save_list twice to separate change order with regular PO */
    i = 0;
    for(k = 0; k < j; k++) /* regular PO */
    {
	if(chpo[k])
	    continue;

	strcpy(list[i], save_list[k]);
	index[i] = temp_index[k];
	rate[i] = old_rate[k];
	i++;
    }
    /* remember the starting index of change order PO */
    co_index = i;
    for(k = 0; k < j; k++) /* change order PO */
    {
	if(!chpo[k])
	    continue;

	strcpy(list[i], save_list[k]);
	index[i] = temp_index[k];
	rate[i] = old_rate[k];
	i++;
    }

    /* copy over to other array */
    for(k = 0; k < j; k++)
    {
	strcpy(save_list[k], list[k]);
	old_rate[k] = rate[k];
    }

    /*
    * If no vendors, then close file & exit
    */
    if (!j) {
	s_close(PO); 
	return(-1);
    }
    store();
    endflg = FALSE;
    i = 0;

    for (i=0; i<j; i++) {
	sprintf(list[i], "%s: ", save_list[i]);
    }

    /*
    * If it is to print requisition form directly, by-pass the markup window
    * This feature is added on 4/22/97
    */
    if(prnForm)
	goto bypass_markup;

    jj = j;
    if(j > MAXVENDORS_PER_MARKUPWINDOW) /* can only hold 20 vendors in the markup window */
	j = MAXVENDORS_PER_MARKUPWINDOW;

    setwindow(28, 3, 55, j+3, "\% Completion for Each Vendor", "PgUp                    PgDn", "F1=HELP             F10=DONE", NULL);

    /* save last mouse click points */
    lastP = points;

    /*
    * need to implement the mouse here
    */
    for (i=0; i<j; i++) {
	printxy(3, i+1, "%s: %-3d\n", save_list[i], rate[i]);
	tagged[i] = NO;
	FillMouseClickPoint(&pSetRate[i], 192, 16 + (i*8), 320, 16 + (i*8), KEY_MOUSE_0 + i);
    }
    FillMouseClickPoint(&pSetRate[j], 192, 32 + ((j-1)*8), 240, 32 + ((j-1)*8), KEY_F1);
    FillMouseClickPoint(&pSetRate[j+1], 288, 32 + ((j-1)*8), 336, 32 + ((j-1)*8), KEY_F7);
    FillMouseClickPoint(&pSetRate[j+2], 376, 32 + ((j-1)*8), 432, 32 + ((j-1)*8), KEY_F10);
    FillMouseClickPoint(&pSetRate[j+3], 208, 8, 240, 8, KEY_PGUP);
    FillMouseClickPoint(&pSetRate[j+4], 400, 8, 432, 8, KEY_PGDOWN);
    FillMouseClickPoint(&pSetRate[j+5], 0, 0, 0, 0, 0);
    points = pSetRate;

    while (!endflg) {

	switch (choice) {

	    case 0:
	    case 1:
	    case 2:
	    case 3:
	    case 4:
	    case 5:
	    case 6:
	    case 7:
	    case 8:
	    case 9:
	    case 10:
	    case 11:
	    case 12:
	    case 13:
	    case 14:
	    case 15:  /* expanded to hold 20 VD in this window */
	    case 16:
	    case 17:
	    case 18:
	    case 19:

		lastchoice = choice;
		if(!nextpage)
	choice = inpt(3, choice+1, list[choice], &rate[choice], 8, CT_INT, NO);
		else
	choice = inpt(3, choice+1, list[choice+MAXVENDORS_PER_MARKUPWINDOW], &rate[choice+MAXVENDORS_PER_MARKUPWINDOW], 8, CT_INT, NO);

		/*
		* Handle ENTER
		*/
		if (choice == 1) {

		    if (lastchoice == j-1)
			choice = lastchoice = 0;
		    else
			choice = ++lastchoice;
		}
		break;

	    case KEY_MOUSE_0:
	    case KEY_MOUSE_1:
	    case KEY_MOUSE_2:
	    case KEY_MOUSE_3:
	    case KEY_MOUSE_4:
	    case KEY_MOUSE_5:
	    case KEY_MOUSE_6:
	    case KEY_MOUSE_7:
	    case KEY_MOUSE_8:
	    case KEY_MOUSE_9:
	    case KEY_MOUSE_A:
	    case KEY_MOUSE_B:
	    case KEY_MOUSE_C:
	    case KEY_MOUSE_D:
	    case KEY_MOUSE_E:
	    case KEY_MOUSE_F:
	    case KEY_MOUSE_G:
	    case KEY_MOUSE_H:
	    case KEY_MOUSE_I:
	    case KEY_MOUSE_J:

		lastchoice = choice = choice - KEY_MOUSE_0;
		break;

	    case KEY_SHFT_TAB:
	    case KEY_LEFT:
	    case KEY_UP:
		if (lastchoice == 0)
		    choice = j-1;
		else
		    choice = lastchoice - 1;

		lastchoice = choice;
		break;

	    case KEY_F1:
		choice = lastchoice;
		helpmessage(HLP_SETRATE);
		break;

	    case KEY_F10:
		endflg = TRUE;
		break;
	    case KEY_PGUP:
		choice = lastchoice;
		if(!nextpage)
		    break;
		choice = lastchoice = 0;
		nextpage = FALSE;

		for (i=0; i<MAXVENDORS_PER_MARKUPWINDOW; i++) {
		    printxy(1, i+1, "                             ");
		}
		j = MAXVENDORS_PER_MARKUPWINDOW;
		for (i=0; i<j; i++) {
		    printxy(3, i+1, "%s: %-3d\n", save_list[i], rate[i]);
		    FillMouseClickPoint(&pSetRate[i], 192, 16+(i*8), 320, 16+(i*8), KEY_MOUSE_0 + i);
		}
		FillMouseClickPoint(&pSetRate[j], 192, 32 + ((j-1)*8), 240, 32 + ((j-1)*8), KEY_F1);
		FillMouseClickPoint(&pSetRate[j+1], 288, 32 + ((j-1)*8), 336, 32 + ((j-1)*8), KEY_F7);
		FillMouseClickPoint(&pSetRate[j+2], 376, 32 + ((j-1)*8), 432, 32 + ((j-1)*8), KEY_F10);
		FillMouseClickPoint(&pSetRate[j+3], 208, 8, 240, 8, KEY_PGUP);
		FillMouseClickPoint(&pSetRate[j+4], 400, 8, 432, 8, KEY_PGDOWN);
		FillMouseClickPoint(&pSetRate[j+5], 0, 0, 0, 0, 0);
		break;

	    case KEY_PGDOWN:
		choice = lastchoice;
		if(nextpage || jj <= MAXVENDORS_PER_MARKUPWINDOW)
		    break;
		choice = lastchoice = 0;
		nextpage = TRUE;

		for (i=0; i<MAXVENDORS_PER_MARKUPWINDOW; i++) {
		    printxy(1, i+1, "                             ");
		}
		for (i=j; i<jj; i++) {
		    printxy(3, i+1-j, "%s: %-3d\n", save_list[i], rate[i]);
		    FillMouseClickPoint(&pSetRate[i-j], 192, 16+((i-j)*8), 320, 16+((i-j)*8), KEY_MOUSE_0 + i-j);
		}
		FillMouseClickPoint(&pSetRate[i-j], 192, 32 + ((j-1)*8), 240, 32 + ((j-1)*8), KEY_F1);
		FillMouseClickPoint(&pSetRate[i-j+1], 288, 32 + ((j-1)*8), 336, 32 + ((j-1)*8), KEY_F7);
		FillMouseClickPoint(&pSetRate[i-j+2], 376, 32 + ((j-1)*8), 432, 32 + ((j-1)*8), KEY_F10);
		FillMouseClickPoint(&pSetRate[i-j+3], 208, 8, 240, 8, KEY_PGUP);
		FillMouseClickPoint(&pSetRate[i-j+4], 400, 8, 432, 8, KEY_PGDOWN);
		FillMouseClickPoint(&pSetRate[i-j+5], 0, 0, 0, 0, 0);
		j = jj - MAXVENDORS_PER_MARKUPWINDOW;
		break;

/*
	    case KEY_F7:
		tagged[lastchoice] = (tagged[lastchoice] ? NO : YES);

		if (tagged[lastchoice]) {
		    printxy(1, lastchoice+1, "\x10");
		} else {
		    printxy(1, lastchoice+1, " ");
		}
*/
		/* fall thru */

	    default:
		if (lastchoice == j-1)
		    choice = lastchoice = 0;
		else
		    choice = ++lastchoice;
		break;
	}
    }
    j = jj;

    /* restore last mouse click points */
    points = lastP;

    restore();

bypass_markup:
    endflg = FALSE; /* endflag is borrowed as a flag to indicate rate "shift" */

    /* check the validaty of rate */
    for(i = 0; i < j; i++)
    {
	s_read(PO, &po, index[i]);

	// check for all 100% retainage
//        if(atoi(po.cur_retanige) != 100)
//            retainflg = FALSE;

	if(rate[i] > 100)
	{
	    message(M_SCR|M_KEY, "The completion percentage can't be over 100% - hit any key");
	    s_close(PO);
	    return (-1);
	}
	if(old_rate[i] < rate[i])
	{
	    endflg = TRUE;
	}
	else if(old_rate[i] > rate[i])
	{
	    if(atoi(po.pre_retanige) > rate[i])
	    {
		message(M_SCR|M_KEY, "The completion percentage can't be less than the previous rate - hit any key");
		s_close(PO);
		return (-1);
	    }
	}
    }

    /* if all rates reach 100, need to ask retainage rate */
    for(i = 0; i < j; i++)
    {
	if(rate[i] != 100)
	{
	    retainflg = FALSE;
	    break;
	}
    }

//    if(!endflg) /* if all rates reach 100 already before, shift */
//    {
//        endflg = TRUE;
//        for(i = 0; i < j; i++)
//        {
//            if(rate[i] != 100)
//            {
//                endflg = FALSE;
//                break;
//            }
//        }
//    }

    if(retainflg) /* need to ask retainage value */
    {
	if(jb->cur_retn == 0) /* first time to ask */
	{
repeat1:
	    strcpy(retainagestr, "10");
	    key = inpt(1, 25, " Enter retainage (%), 0 or 5 or 10: (10) ", retainagestr, 2, CT_CHAR, NO);
	    if(key == KEY_ESC1 || key == KEY_ESC || key == KEY_F10)
	    {
		s_close(PO);
		return (-1);
	    }

	    if(strcmp(retainagestr, "0") && strcmp(retainagestr, "5") && strcmp(retainagestr, "10"))
		goto repeat1;

//            cur_retain = pre_retain = 0.1;    changed to the next one on 12/08/96
	    pre_retain = 0.1; cur_retain = atoi(retainagestr)/100.0;

	    if(pre_retain != cur_retain)
		retEntered = TRUE;
	}
	else if(jb->cur_retn == '0') /* already reaches 10%, no more anything */
	{
	    strcpy(retainagestr, "0");
	    cur_retain = 0;
	    if(jb->pre_retn == '5') /* coming from 5% */
// add a question to claim the full retainage here, and set retEntered, retainagestr, etc.
		pre_retain = 0.05;
	    else if(jb->pre_retn == '0') /* coming from 0% */
// add a question to claim the full retainage here, and set retEntered, retainagestr, etc.
		pre_retain = 0.1;
	    else /* coming from 10%, stay the same */
		pre_retain = 0;
	}
	else if(jb->cur_retn == '5') /* only have the next 5% left */
	{
repeat2:
	    strcpy(retainagestr, "5");
	    key = inpt(1, 25, " Enter retainage (%), 0 or 5: (5) ", retainagestr, 1, CT_CHAR, NO);
	    if(key == KEY_ESC1 || key == KEY_ESC || key == KEY_F10)
	    {
		s_close(PO);
		return (-1);
	    }

	    if(strcmp(retainagestr, "0") && strcmp(retainagestr, "5"))
		goto repeat2;

//            cur_retain = pre_retain = 0.05;   changed to the next one on 12/08/96
	    pre_retain = 0.05; cur_retain = atoi(retainagestr)/100.0;

	    if(pre_retain != cur_retain)
		retEntered = TRUE;

	    if(!strcmp(retainagestr, "5"))
		jb->pre_retn = '5';
	} 

	message(M_SCR, NULL);
	/* need to ask question to save the retainage */
	if(retEntered)
	{
	    endflg = TRUE; // if retainage percentage changed, also shift
	    ans = message(M_SCR|M_KEY, "Update the retainage percentage? (y/N)");
	}
	else
	    ans = 'n';
	if(ans == 'Y' || ans == 'y')
	{
	    if(!strcmp(retainagestr, "0")) /* enter 0% retainage, means take all */
	    {
		jb->pre_retn = jb->cur_retn;
		jb->cur_retn = '0';
	    }
	    else if(!strcmp(retainagestr, "5")) /* enter 5% retainage */
	    {
		jb->pre_retn = jb->cur_retn;
		jb->cur_retn = '5';
	    }
	    else /* must be entering 10% retainage */
		;

	    vec = s_getno(JB, 0, jb->code);
	    s_write(JB, jb, vec, FALSE);

	    shift = TRUE; // this tells to save the shifted rate later
	}

	if(pre_retain == cur_retain)
	    diff_retain = pre_retain;
	else
	    diff_retain = pre_retain - cur_retain;
    }

    for(i = 0; i < j; i++)
    {
	s_read(PO, &po, index[i]);

	if(old_rate[i] != rate[i]) /* at least one rate changed */
	    tagged[i] = YES;
	
	use_rate[i] = rate[i];
	
	if(endflg) /* if rate keeps the same or higher, shift */
	{
	    if(old_rate[i] > rate[i])
		old_rate[i] = atoi(po.pre_retanige);
	}
	else /* no need to shift rate */
	{
	    old_rate[i] = atoi(po.pre_retanige);
	}
    }
    
    message(M_SCR, NULL);
    memset(app_no, 0, 4);
    key = inpt(1, 25, " Enter Application Number: ", app_no, 3, CT_CHAR, NO); 
    app_no[3] = '\0';
    if(key == KEY_ESC1 || key == KEY_ESC || key == KEY_F10)
    {
	s_close(PO);
	return (-1);
    }

ag: memset(period, 0, 9);
    key = inpt(1, 25, " Enter Ending Period: ", period, 8, CT_CHDATE, NO); 
    if(key == KEY_ESC1 || key == KEY_ESC || key == KEY_F10)
    {
	s_close(PO);
	return (-1);
    }

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
    /* generate period from string according to period date */
    for(i = 0; period[i] != '/'; i++)
	period_from[i] = period[i];
    period_from[i] = period[i];   /* get the month part up to the first / */
    i++;
    period_from[i] = '1'; /* set to the first day of the month */
    period_from[i + 1] = '/';
    n =  i + 2; 

    for(; period[i] != '/'; i++); /* skip the day part */
    i++; /* now, i points to the year part */

    for(; period[i]; i++)
	period_from[n++] = period[i];
    period_from[n] = '\0';

    StrDate(tdate, &today);

    s_open(ES);
    vec = s_getno(ES, 0, jb->code);
    s_read(ES, &es, vec);
    s_close(ES);

    s_open(CD);
    vec = s_getno(CD, 0, es.client_code);
    s_read(CD, &client, vec);
    s_close(CD);

    s_open(VD);

    /* calculate once to get the total first */
    for(i=0; i<co_index; i++)
    {
	s_read(PO, &po, index[i]);
	if(i == 0)
	    strcpy(firstPODate, po.podate);

	bid = po.bid;
	if(bid == 0) /* prevent divided by zero */
	    bid = 0.01;
	invtd = bid * old_rate[i] / 100; invwip = bid * (use_rate[i] - old_rate[i]) / 100; totaltd = invtd + invwip; comp = totaltd/bid * 100; balance = bid - totaltd; ret = totaltd * cur_retain;
	t_bid += bid; t_invtd += invtd; t_invwip += invwip; t_totaltd += totaltd; t_balance += balance; t_ret += ret;
    }
    
    /* for change order PO */
    if(co_index == j) 
	goto skip;

    for(i=co_index; i<j; i++)
    {
	s_read(PO, &po, index[i]);

	bid = po.bid;
	if(bid == 0) /* prevent divided by zero */
	    bid = 0.01;
	invtd = bid * old_rate[i] / 100; invwip = bid * (use_rate[i] - old_rate[i]) / 100; totaltd = invtd + invwip; comp = totaltd/bid * 100; balance = bid - totaltd; ret = totaltd * cur_retain;
	t_co_bid += bid; t_co_invtd += invtd; t_co_invwip += invwip; t_co_totaltd += totaltd; t_co_balance += balance; t_co_ret += ret;       

	DateStr(&coPODate, po.podate);
	if(DiffDate(&coPODate, &today) <= 30 && coPODate.da_mon == today.da_mon)
	/* the podate and today are in the same month */
	    tmpf1 += bid;
	else
	    tmpf += bid;
    }

skip:
    t_invtd += t_co_invtd; t_invwip += t_co_invwip; t_totaltd += t_co_totaltd; t_balance += t_co_balance; t_ret += t_co_ret;

    /* calculate the correct "Current Payment Due" */
    if(retainflg && t_invwip == 0)
	numEight = t_totaltd * diff_retain;
    else
//        numEight = t_totaltd - t_ret - t_invtd * (1 - pre_retain);
    {
	numEight = t_invwip * (1 - cur_retain);
	/* also consider the previous work has been done */
	numEight += t_invtd * (pre_retain - cur_retain);
    }

    n = (80-strlen(sysvar.company)) / 2;
    itoa(100*cur_retain, tmpstr, 10);
    strcat(tmpstr, "%");

    /* code to print the requisition form. */
    FileTempName(fname);

    if(prnForm)
    {
      fp = defPrinter;
      /* turn on condensed and landscape mode */
      fprintf(fp, "%s", prnCodes.cond);
      fprintf(fp, "%s", prnCodes.land);
    }
    else
    {
      if((fp = fileopn(fname, "wt")) == NULL) 
      {
	s_close(PO);
	message(M_SCR|M_ERR|M_KEY, "Unable to open %s - hit any key", fname);
	return (-1);
      } 
    }


    if(prnForm)
    {
/*
retry:  strcpy(tmpstr, "0");
	key = inpt(1, 25, " Enter line adjustment (-1, 0, 1): ", tmpstr, 2, CT_CHAR, NO); 
	if(key == KEY_ESC1 || key == KEY_ESC || key == KEY_F10)
	{
	    s_close(PO);
	    return (-1);
	}
	lineAdj = atoi(tmpstr);
	if(lineAdj < -1 || lineAdj > 1)
	    goto retry;

	lineAdj++;
	for(i = 0; i < lineAdj; i++)
	    fprintf(fp, "\n\r");
*/
	/* on 4/22/97, added 4 spaces in each line by request from Majid */
	/* on 5/12/97, added 2 spaces in each line for 702, and add one line for 702 */
	fprintf(fp, "\n\r");

	fprintf(fp, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t  2\n\r");
	fprintf(fp, "\t\t  %-58.30s%-65.30s%-8.8s\n\r", client.name, jb->name, app_no);
	fprintf(fp, "\t\t  %-58.30s%-58.30s%-8.8s\n\r", client.address, jb->address, period);
	sprintf(tmpstr1, "%s, %s %s", client.city, client.state, client.zip);
	sprintf(tmpstr2, "%s, %s %s", jb->city, jb->state, jb->zip);
	fprintf(fp, "\t\t  %-58.30s%-61.30s%-8.8s\n\r", tmpstr1, tmpstr2, jb->po);
	fprintf(fp, "\n\r");
	fprintf(fp, "\t\t\t  %-58.30s%-56.30s%-8.8s\n\r", sysvar.company, jb->arch, firstPODate);
	fprintf(fp, "\t\t\t  %s, %s\n\r", compInfo.address1, compInfo.address2);
	fprintf(fp, "\t\t\t  %s, %s %s\n\r", compInfo.city, compInfo.state, compInfo.zip);
	fprintf(fp, "\t\t  %s\n\r", " "); /* not "Contract for" for now */

	fprintf(fp, "\n\r\n\r\n\r\n\r");
	fprintf(fp, "\t\t\t\t\t\t\t   %8.2f\n\r", t_bid);
	fprintf(fp, "\t\t\t\t\t\t\t\t\t\t\t\t\t   %-20.20s\n\r", sysvar.company);
	fprintf(fp, "\t\t\t\t\t\t\t   %8.2f\n\r", t_co_bid);
	fprintf(fp, "\t\t\t\t\t\t\t   %8.2f\n\r", t_bid + t_co_bid);
	fprintf(fp, "\n\r");
	fprintf(fp, "\t\t\t\t\t\t\t   %8.2f\n\r", t_totaltd);
	fprintf(fp, "\n\r\n\r");

	fprintf(fp, "\t      %-2.2s\t\t\t\t\t   %8.2f\n\r", tmpstr, t_ret);
	fprintf(fp, "\n\r");
	fprintf(fp, "\t      %-2.2s\t\t\t\t\t   %8.2f\n\r", tmpstr, 0.00); 
	fprintf(fp, "\n\r");
	fprintf(fp, "\t\t\t\t\t\t\t   %8.2f\n\r", t_ret);
	fprintf(fp, "\n\r");
	fprintf(fp, "\t\t\t\t\t\t\t   %8.2f\n\r", t_totaltd - t_ret);
	fprintf(fp, "\n\r");
	fprintf(fp, "\n\r");
	fprintf(fp, "\t\t\t\t\t\t\t   %8.2f\n\r", t_invtd * (1 - pre_retain));
	fprintf(fp, "\n\r");
	fprintf(fp, "\t\t\t\t\t\t\t   %8.2f\n\r", numEight);
	fprintf(fp, "\t\t\t\t\t\t\t   %8.2f\n\r", t_bid + t_co_bid - t_totaltd + t_ret);
	fprintf(fp, "\n\r");

	if(co_index != j) /* has change order */
	{
	    if(tmpf < 0)
	    {
		tmpnegf = -tmpf;
		tmpf = 0;
	    }
	    if(tmpf1 < 0)
	    {
		tmpnegf1 = -tmpf1;
		tmpf1 = 0;
	    }

	    fprintf(fp, "\n\r\n\r");
	    if(tmpf != 0)
		fprintf(fp, "\t\t\t\t\t   %8.2f\n\r", tmpf);
	    else if( tmpnegf != 0)
		fprintf(fp, "\t\t\t\t\t\t\t\t  %8.2f\n\r", tmpf);
	    else
		fprintf(fp, "\n\r");
	    fprintf(fp, "\n\r");
	    if(tmpf1 != 0)
		fprintf(fp, "\t\t\t\t\t   %8.2f\n\r", tmpf1);
	    else if(tmpnegf1 != 0)
		fprintf(fp, "\t\t\t\t\t\t\t\t  %8.2f\n\r", tmpnegf1);
	    else
		fprintf(fp, "\n\r");
	    if(tmpf + tmpf1 != 0)
		fprintf(fp, "\t\t\t\t\t   %8.2f\n\r", tmpf + tmpf1);
	    else if(tmpnegf + tmpnegf1 != 0)
		fprintf(fp, "\t\t\t\t\t\t\t\t  %8.2f\n\r", tmpnegf + tmpnegf1);
	    else
		fprintf(fp, "\n\r");
/* NET CHANGES by Change Order is left blank
	    if(t_co_invtd + t_co_invwip != 0)
		fprintf(fp, "\t\t\t\t\t   %8.2f\n\r", t_co_invtd + t_co_invwip);
*/
	}
	fputc(12, fp); /* page change */
	fprintf(fp, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t2    2\n\r");
	fprintf(fp, "\n\r");

	fprintf(fp, "\t\t%-66.30s%-61.30s%-8.8s\n\r", " ", jb->name, app_no);
	fprintf(fp, "\t\t%-66.30s%-61.30s%-8.8s\n\r", " ", jb->address, tdate);
	sprintf(tmpstr2, "%s, %s %s", jb->city, jb->state, jb->zip);
	fprintf(fp, "\t\t%-66.30s%-61.30s%-8.8s\n\r", " ", tmpstr2, period);
/*
	fprintf(fp, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t     %s\n\r", jb->name, app_no);
	fprintf(fp, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t     %s\n\r", jb->address, tdate);
	fprintf(fp, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t     %s\n\r", tmpstr2, period);
*/
	fprintf(fp, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t     %s\n\r", " ");
	fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r\n\r");
    }
    else
    {
      fprintf(fp, "\n\r%*s\n\r", n+strlen(sysvar.company), sysvar.company);
      fprintf(fp, "                            Payment Requisition Form                           \n\r");
      fprintf(fp, " ____________________________________    _____________________________________ \n\r");
      fprintf(fp, "| Client: %-26.26s |  |            CONTRACT STATUS          |\n\r", client.name);
      fprintf(fp, "| Client PO: %-23s |  | 1: Original Contract    : $%8.2f |\n\r", jb->po, t_bid);
      fprintf(fp, "| Contact: %-25.25s |  | 2: Net Change Order     : $%8.2f |\n\r", es.estname, t_co_bid);
      fprintf(fp, "| Job Name: %-24.24s |  | 3: Contract Sum To Date : $%8.2f |\n\r", jb->name, t_bid + t_co_bid);
      fprintf(fp, "| Job No.: %-25s |  | 4: Total Completed      : $%8.2f |\n\r", jb->code, t_totaltd);
      fprintf(fp, "| Application No.: %-17s |  | 5: Retainage            : $%8.2f |\n\r", app_no, t_ret);
      fprintf(fp, "| Date of Req.: %-20s |  | 6: Total Earned Less Ret: $%8.2f |\n\r", tdate, t_totaltd - t_ret);
      fprintf(fp, "| Period From: %-21s |  | 7: Previous Application : $%8.2f |\n\r", period_from, t_invtd * (1 - pre_retain));
      fprintf(fp, "| Period To: %-23s |  | 8: Current Payment Due  : $%8.2f |\n\r", period, numEight);
      fprintf(fp, "| Retainage: %-23s |  | 9: Balance To Complete  : $%8.2f |\n\r", tmpstr, t_bid + t_co_bid - t_totaltd + t_ret); 
      fprintf(fp, "|____________________________________|  |_____________________________________|\n\r");

      fprintf(fp, "\n\r");
      fprintf(fp, "                  Itemized breakdown of work and change orders                  \n\r");
      fprintf(fp, " _____________________________________________________________________________ \n\r");
      fprintf(fp, "|   PRODUCT    |S. VALUE|PREV APP|WORK INP|TOTAL CM| % COMP | BALANCE| RETAGE |\n\r");
    }

    /* go through the loop second time to print the table */
    t_bid = 0; t_invtd = 0; t_invwip = 0; t_totaltd = 0; t_balance = 0; t_ret = 0;
    t_co_bid = 0; t_co_invtd = 0; t_co_invwip = 0; t_co_totaltd = 0; t_co_balance = 0; t_co_ret = 0;
    for(i=0; i<co_index; i++)
    {
	s_read(PO, &po, index[i]);

	bid = po.bid;
	if(bid == 0)
	    bid = 0.01;
	invtd = bid * old_rate[i] / 100; invwip = bid * (use_rate[i] - old_rate[i]) / 100; totaltd = invtd + invwip; comp = totaltd/bid * 100; balance = bid - totaltd; ret = totaltd * cur_retain;
	t_bid += bid; t_invtd += invtd; t_invwip += invwip; t_totaltd += totaltd; t_balance += balance; t_ret += ret;
	
	vec = s_getno(VD, 0, po.vdcode);
	s_read(VD, &vender, vec);

	itoa(i+1, tmpstr1, 10);

	if(!prnForm)
	{
	    fprintf(fp, "|______________|________|________|________|________|________|________|________|\n\r");
	    fprintf(fp, "|%-14.14s|%8.2f|%8.2f|%8.2f|%8.2f|%8.2f|%8.2f|%8.2f|\n\r", vender.product, bid, invtd, invwip, totaltd, comp, balance, ret);
	}
	else
	{
	    fprintf(fp, "\t %-12.2s%-32.30s%8.2f\t     %8.2f\t     %8.2f\t     %8.2f\t     %8.2f\t      %5.2f      %8.2f\t%8.2f\n\r",
		tmpstr1, vender.product, bid, invtd, invwip, 0.00, totaltd, comp, balance, ret);                
	}
    }

    if(!prnForm)
    {
	fprintf(fp, "|______________|________|________|________|________|________|________|________|\n\r");
	fprintf(fp, "|    Total     |%8.2f|%8.2f|%8.2f|%8.2f|        |%8.2f|%8.2f|\n\r", t_bid, t_invtd, t_invwip, t_totaltd, t_balance, t_ret);
	fprintf(fp, "|______________|________|________|________|________|________|________|________|\n\r");
    }
    else if(co_index == j) /* no change order */
    {
	/* advance the print header to the bottom. There are about 26 lines from the begging of the table */
	for(lineAdj = i; lineAdj < 26; lineAdj++)
	    fprintf(fp, "\n\r");
	/* then, print the total */
	fprintf(fp, "\t %-12.2s%-32.30s%8.2f\t     %8.2f\t     %8.2f\t     %8.2f\t     %8.2f\t      %5.2f      %8.2f\t%8.2f\n\r",
	    " ", " ", t_bid, t_invtd, t_invwip, 0.00, t_totaltd, t_totaltd/t_bid * 100, t_balance, t_ret);
    }

    /* for change order PO */
    if(co_index == j) 
	goto skip_second;

    if(!prnForm)
    {
      fprintf(fp, "\nChange Order:\n\r");
      fprintf(fp, " _____________________________________________________________________________ \n\r");
      fprintf(fp, "|   PRODUCT    |S. VALUE|PREV APP|WORK INP|TOTAL CM| % COMP | BALANCE| RETAGE |\n\r");
    }
    else
	fprintf(fp, "\n\r"); /* put one blank line for change order */

    for(i=co_index; i<j; i++)
    {
	s_read(PO, &po, index[i]);

	bid = po.bid;
	if(bid == 0)
	    bid = 0.01;
	invtd = bid * old_rate[i] / 100; invwip = bid * (use_rate[i] - old_rate[i]) / 100; totaltd = invtd + invwip; comp = totaltd/bid * 100; balance = bid - totaltd; ret = totaltd * cur_retain;
	t_co_bid += bid; t_co_invtd += invtd; t_co_invwip += invwip; t_co_totaltd += totaltd; t_co_balance += balance; t_co_ret += ret;       
	
	vec = s_getno(VD, 0, po.vdcode);
	s_read(VD, &vender, vec);

	itoa(i+1, tmpstr1, 10);

	if(!prnForm)
	{
	  fprintf(fp, "|______________|________|________|________|________|________|________|________|\n\r");
	  fprintf(fp, "|%-14.14s|%8.2f|%8.2f|%8.2f|%8.2f|%8.2f|%8.2f|%8.2f|     |\n\r", vender.product, bid, invtd, invwip, totaltd, comp, balance, ret);
	}
	else
	{
	    fprintf(fp, "\t %-12.2s%-32.30s%8.2f\t     %8.2f\t     %8.2f\t     %8.2f\t     %8.2f\t      %5.2f      %8.2f\t%8.2f\n\r",
		tmpstr1, vender.product, bid, invtd, invwip, 0.00, totaltd, comp, balance, ret);                
	}
    }

    if(!prnForm)
    {
      fprintf(fp, "|______________|________|________|________|________|________|________|________|\n\r");
      fprintf(fp, "|    Total     |%8.2f|%8.2f|%8.2f|%8.2f|        |%8.2f|%8.2f|\n\r", t_co_bid, t_co_invtd, t_co_invwip, t_co_totaltd, t_co_balance, t_co_ret);
      fprintf(fp, "|______________|________|________|________|________|________|________|________|\n\r");
    }
    else
    {
	/* advance the print header to the bottom. There are about 26 lines from the begging of the table */
	for(lineAdj = i; lineAdj < 25; lineAdj++) /* minus one blank line for starting of the change order */
	    fprintf(fp, "\n\r");
	/* then, print the total */
	fprintf(fp, "\t %-12.2s%-32.30s%8.2f\t     %8.2f\t     %8.2f\t     %8.2f\t     %8.2f\t      %5.2f      %8.2f\t%8.2f\n\r",
	    " ", " ", t_bid+t_co_bid, t_invtd+t_co_invtd, t_invwip+t_co_invwip, 0.00, t_totaltd+t_co_totaltd, (t_totaltd+t_co_totaltd)/(t_bid+t_co_bid) * 100, t_balance+t_co_balance, t_ret+t_co_ret);
    }

skip_second:
    if(!prnForm)
    {
      fprintf(fp, "\n\r");

      fprintf(fp, " _____________________________________________________________________________ \n\r");
      fprintf(fp, "| The undersigned, hereby certifies that, except as listed below, he has paid |\n\r");
      fprintf(fp, "| in full or has otherwise satisfied all obligations for all materials and    |\n\r");
      fprintf(fp, "| equipment furnished, for all work, labor, and services performed, and for   |\n\r");
      fprintf(fp, "| all known indebtedness and claims against the Subcontractor for damages     |\n\r");
      fprintf(fp, "| arising in any matter in connection with the performance of the Contract    |\n\r");
      fprintf(fp, "| referenced above for which the Contractor, Owner or his property might in   |\n\r");
      fprintf(fp, "| any way be held responsible.                                                |\n\r");
      fprintf(fp, "|                                                                             |\n\r");
      fprintf(fp, "| By________________________________  Title__________________________________ |\n\r");
      fprintf(fp, "|-----------------------------------------------------------------------------|\n\r");
      fprintf(fp, "| Subscribed and sworn to before me this __________ day __________ 19____     |\n\r");
      fprintf(fp, "|                                                                             |\n\r");
      fprintf(fp, "| NOTARY PUBLIC _____________________________________________________________ |\n\r");
      fprintf(fp, "|                                                                             |\n\r");
      fprintf(fp, "| My Commission Expires: ____________________________________________________ |\n\r");
      fprintf(fp, "|_____________________________________________________________________________|\n\r");
    }

    for(i = 0; i < j; i++)
	if(tagged[i] == YES)
	    break;

    if(i < j || shift)
    {
	/* write the rate back to PO file */
	if(shift) // shift if retainage changed and saved before
	    ans = 'Y';
	else
	    ans = message(M_SCR|M_KEY, "Save the completion percentage entered? (y/N)");
	if(ans == 'Y' || ans == 'y')
	for(i = 0; i < j; i++) 
	{
	    s_read(PO, &po, index[i]);

	    if(endflg && atoi(po.cur_retanige) <= rate[i]) /* apply shift */
	    {
		strncpy(po.pre_retanige, po.cur_retanige, 3);
		po.pre_retanige[3] = '\0';
		sprintf(po.cur_retanige, "%-3.3d", rate[i]);
		s_write(PO, &po, index[i], FALSE);
	    }
	    else if(atoi(po.cur_retanige) > rate[i])
	    {
		sprintf(po.cur_retanige, "%-3.3d", rate[i]);
		s_write(PO, &po, index[i], FALSE);
	    }
	}
    }
    
    s_close(VD);
    s_close(PO);

    if(prnForm)
    {
	/* Turn off specifial mood */
	fprintf(fp, "%s", prnCodes.reset);
    }
    else
    {
	filecls(fp);
	TextEditor(fname, client.fax, client.email);
	FileDelete(fname);
    }

    return(0);
}


/*****************************************************************************
po_funcs() - Controls Input for Purchase Order Screen
*****************************************************************************/
INT po_funcs(DB_TYPE type, IN_POS *c, POREC *po)
{
    VDREC vd, tmpVD;
    JBREC jb;
    ESREC es;
    CDREC client, tmpCD;
    POREC temppo;
    CHAR *list[]={"Purchase Order for Approval", "Purchase Order for Shipment", "Release for Fabrication", "Installation Request Form", "Accounting Form", "Invoice/Packing list", NULL};
    CHAR *statlist[]={"Fully Shipped", "Fully Paid", NULL};
    LONG top=0L;
    INT i, choice, vec, ncd, idx, choose, len;
    CHAR *list1[10];

/*
* Shipping name <=> index map
*/
char *ShippingCompanyIndex[] = {
	"1 FedEx",
	"2 UPS",
	"3 DHL",
	"4 EMS",
	"5 US Post",
	"6 RoadWay",
	NULL
};

    switch (c->x) {

	case KEY_F1:
	    switch (c->last) {

		/*
		* Pick Job for this New PO
		*/
		case 1:  
		    if (!*po->jbcode) {
			s_read(JB, &jb, gethelp(JB, s_nrecs(JB)-1, db[JB].s->num, db[JB].s->fld, db[JB].s->x-1, db[JB].s->y-1));
			if ((jb.status[0] != 'W') && (jb.status[0] != 'S') && (jb.status[0] != 'P') && jb.status[0] != 'O')
			    break;
			s_read(ES, &es, s_getno(ES, 0, jb.code));
			strcpy(po->client_code, es.client_code);
			strcpy(po->jbcode, jb.code);
			strcpy(po->jbname, jb.name);
			strcpy(po->state, jb.state);
			StrDate(po->podate, &today);
			strcpy(po->spcode, es.spcode);
			po->status[0] = 'O';
			po->status[1] = '\0';
			changeflg=TRUE;
			(*db[type].set_func)(type, c, po);
		    } else {
			helpmessage(type);
		    }
		    break;

		/*
		* Pick Vendor for this New PO
		*/
		case 2:   
		    if (!*po->vdcode) {
			if ((vec=s_getno(ES, 0, po->jbcode)) == -1) {
			    message(M_SCR|M_KEY, "Estimate does not exist - hit any key");
			    break;
			}
			s_read(ES, &es, vec);
			plfile(&es);
			inPO = YES;
			if ((idx = vd_chooser(po->vdcode)) == -1) {
			    inPO = NO;
			    break;
			}
			inPO = NO;

			/*
			* Recalculate totals from Estimate
			*/
			re_calc(&es, po->vdcode, FALSE, idx);
			po->plidx = (SHORT) idx;
			po->cogs = es.cogs;
			po->commission = es.com - (po->status[0] == 'S' ? po->actcogs-po->cogs:0);
			po->tax = es.esttax;
			po->estship = es.estship;
			po->bid = es.bid;
			po->mkup = es.mkup;
			/* po->profit = po->mkup-po->other+(po->shipping==0 ? 0:po->estship-po->shipping); */
			po->profit = po->mkup-(po->status[0] == 'S' ? po->actcogs-po->cogs:0)+(po->shipping==0 ? 0:po->estship-po->shipping);     
			/* commission should be based on profit */
			{
			    INT i;
			    SPREC sp;

			    s_open(SP);
			    if((i = s_getno(SP, 0, po->spcode)) != -1)
				s_read(SP, &sp, i);
			    else
				sp.rate = 0;
			    s_close(SP);
			    po->commission = po->profit * sp.rate;
			}

			/*
			* update YTD On Order for Client and Vendor
			*/
			s_read(CD, &tmpCD, ncd=s_getno(CD, 0, po->client_code));
			tmpCD.ytd_on_order += po->bid;
			s_recwrite(CD, &tmpCD, ncd);

			s_read(VD, &tmpVD, ncd=s_getno(VD, 0, po->vdcode));
			tmpVD.ytd_on_order += po->cogs;
			s_recwrite(VD, &tmpVD, ncd);

			/* Set PO Date to today */
			/* if podate is already set, don't set it again */
			if(po->podate[0] == '\0') StrDate(po->podate, &today); 
			po->status[0] = 'O';
			po->status[1] = '\0';          /* Set Status to On Order */
			changeflg = TRUE;
			(*db[type].set_func)(type, c, po);
		    } else {
			helpmessage(type);
		    }
		    break;

		/*
		* Get new Invoice #
		*/
		case 8:
		    if (!*po->inv_number) {
			for (i=0; i<s_nrecs(PO); i++) {
			    s_read(PO, &temppo, i);
			    top = (top<atol(temppo.inv_number) ? atol(temppo.inv_number):top);
			}
			sprintf(po->inv_number, "%08ld", (long)++top);
			changeflg = TRUE;
		    }
		    break;

		default:
			if(strcmp(db[type].f[c->last].header, "Ship|Company") == 0)
			{
				/* Build the list window */
				for(i = 0; ShippingCompanyIndex[i]; i++)
					list1[i] = strdup(ShippingCompanyIndex[i]);
				list1[i] = NULL;
				len = i;

				choose = window_chooser(50, 12, list1);
				if(choose >= 0) 
				{
					/* Get the digit part */
					sscanf(list1[choose], "%hd", &(po->shipcompindex));
					changeflg = TRUE;
				}

				/* Free the memory */
				for(i = 0; i < len; i++)
					if(list1[i]) 
						free(list1[i]);
			}
			else
				helpmessage(type);
		    break;
	    } break;

	/*
	* ReCalculate Totals
	*/
	case KEY_F4:        
	    c->x = c->last;

	    if(po->other != 0)
	    {    
		po->actcogs += po->other;
		po->other = 0;
	    }
	    /*
	    * Sets Status to O if no Ship Date, otherwise S
	    */
	    if (*po->vdcode) {
		if ((choice = message(M_SCR|M_KEY, "Get info from Part List? (Y/n)")) != 'N') {
		    if ((choice == KEY_F10) || (choice == KEY_ESC1))
			break;
		    s_open(ES);
		    if ((i=s_getno(ES, 0, po->jbcode)) == -1) {
			message(M_SCR|M_KEY, "Estimate does not exist - hit any key");
			break;
		    }
		    s_read(ES, &es, i);
		    plfile(&es);
		    set_rate(&es, po->vdcode, TRUE); /* this sets vdlist[] global */
		    re_calc(&es, po->vdcode, FALSE, po->plidx); //re_calc(&es, po->vdcode, FALSE);
		    po->cogs = es.cogs;
		    po->commission = es.com - (po->status[0] == 'S' ? po->actcogs-po->cogs:0);
		    po->tax = es.esttax;
		    po->estship = es.estship;
		    po->bid = es.bid;
		    po->mkup = es.mkup;
		} else {
		    po->bid = po->cogs + po->estship + po->mkup + po->tax;
		}
		/* po->profit = po->mkup - po->other + (po->shipping == 0 ? 0 : po->estship - po->shipping); */
		po->profit = po->mkup - (po->status[0] == 'S' ? po->actcogs - po->cogs:0) + (po->shipping == 0 ? 0 : po->estship - po->shipping); 
		/* commission should be based on profit */
		{
		    INT i;
		    SPREC sp;

		    s_open(SP);
		    if((i = s_getno(SP, 0, po->spcode)) != -1)
			s_read(SP, &sp, i);
		    else
			sp.rate = 0;
		    s_close(SP);
		    po->commission = po->profit * sp.rate;
		}
		
		changeflg = TRUE;
		(*db[type].set_func)(type, c, po);
	    }
	    break;

	/*
	* Set Status to Shipped
	*/
	case KEY_F7:
	    if (po->status[0] == 'S') {
		message(M_SCR|M_KEY, "Order has already been put into Shipped Status - hit any key");
		break;
	    }

	    if ((choice = window_chooser(30, 9, statlist)) == -1)
		break;

	    i = -2;
	    if (choice == 0) {
		if (message(M_SCR | M_KEY, "Changes will be perminant.  Are you sure (y/N)") == 'Y') {
		    if (po->shipdate[0] == '\0')
			StrDate(po->shipdate, &today);

		    /*
		    * Update the current cogs field 
		    */
		    if(po->other != 0)
		    {    
			po->actcogs += po->other;
			po->other = 0;
		    }

		    /*
		    * Update Client Records
		    */
		    s_open(CD);
		    vec = s_getno(CD, 0, po->client_code);
		    s_read(CD, &client, vec);
		    client.ytd_sales += po->bid;
		    client.total_sales += po->bid;
		    client.ytd_on_order -= po->bid;
		    StrDate(client.last_purchase_date, &today);
		    s_write(CD, &client, vec, NO);
		    s_close(CD);

		    /*
		    * Update Vendor Records
		    */
		    s_open(VD);
		    vec = s_getno(VD, 0, po->vdcode);
		    s_read(VD, &vd, vec);
		    vd.ytd_sales += po->cogs;
		    vd.total_sales += po->cogs;
		    vd.ytd_on_order -= po->cogs;
		    StrDate(vd.last_active_date, &today);
		    s_write(VD, &vd, vec, NO);
		    s_close(VD);

		    po->status[0] = 'S';
		    po->status[1] = '\0';
		    /* update the profit field */
		    po->profit = po->mkup - (po->actcogs - po->cogs) + (po->shipping == 0 ? 0 : po->estship - po->shipping); 
		    s_write(type, po, c->ypos, NO);
		    for (i = 0; i < s_nrecs(PO); i++) {
			s_read(PO, &temppo, i);
			if (!strcmp(po->po_number, temppo.po_number))
			    if (temppo.status[0] != 'S')
				break;
		    }
		} else {
		    po->status[0] = 'P';
		    po->status[1] = '\0';
		}
		s_read(JB, &jb, (vec = s_getno(JB, 0, po->jbcode)));
		if (i == s_nrecs(PO)) {
		    jb.status[0] = 'S';
		} else {
		    jb.status[0] = 'P';
		}

		/* update the Job record ship date */
		/* StrDate(jb.sent, &today); */
		strcpy(jb.sent, po->shipdate);

		s_write(JB, &jb, vec, NO);

		/* update the ES status record */
		s_open(ES);
		s_read(ES, &es, (vec = s_getno(ES, 0, jb.code)));
		es.status[0] = jb.status[0];
		/* also update the ES ship date */
		strcpy(es.estdate, po->shipdate);
		s_write(ES, &es, vec, FALSE);
		s_close(ES);

		changeflg = FALSE;
	    } break;

	/*
	* Pick from window for PO, Invoice, or Form
	*/
	case KEY_F9:  
	    c->x = c->last;
	    s_vcheck(type, po, c->ypos);
	    if ((choice = window_chooser(30, 9, list)) == -1)
		break;

	    switch (choice) {
		/*
		* PO for approval
		*/
		case 0:
		    do_invoice(po, (choice == 0 ? FALSE : TRUE));
		    break;

		/*
		* PO for shipment
		*/
		case 1:
		    if(po->status[0] == 'S' || po->status[0] == 's')
			message(M_SCR|M_KEY, "Job is already shipped -- hit any key");
		    else
			po_release_fabrication(c, po, PURCHASE);
		    break;

		/*
		* Inoice
		*/
		case 5:
		    do_invoice(po, (choice == 0 ? FALSE : TRUE));
		    break;

		/*
		* Account Form
		*/
		case 4:
		    po_form(po);
		    break;

		/*
		* Release for Fabrication
		*/
		case 2:
		    if(po->status[0] == 'S' || po->status[0] == 's')
			message(M_SCR|M_KEY, "Job is already shipped -- hit any key");
		    else
			po_release_fabrication(c, po, RELEASE);
		    break;

		/*
		* Installation Request Form
		*/
		case 3:
		{
		    /* code to do release a job first. put the code here is a little odd */
		    ESREC es;
		    JBREC jb;
		    CHAR sttoday[9];
		    INT vec, ans = 'Y';

		    StrDate(sttoday, &today);

		    /* too bad, the following piece of code is duplicated if for subcontract agreement */
		    /* because it is used only for installation before, then sub-agree is added */
		    /* if job is already released for fabrication, ask for whether to re-release it */
		    if(po->status[0] == 'R')
		    {
			ans = message(M_SCR|M_KEY, "Job is already released for fabrication. Re-release it (y/N) ?");
			if(ans != 'Y' && ans != 'y')
			    return (0);
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
			return(0);
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
			return(0);
		    }
		    s_read(JB, &jb, vec);
		    if (jb.status[0] != 'R' && ans == 'Y')
		    {
			jb.status[0] = 'R';
			s_write(JB, &jb, vec, FALSE);
		    }
		    s_close(JB);

		    jb_installation(c, PO, po);
		    break;
	       }
	    }
	    break;
    }
    return(0);
}

/*****************************************************************************
po_form() - Shows PO Form for One PO only
*****************************************************************************/
INT po_form(POREC *po)
{
    INT vec;
    ESREC es;
    JBREC jb;
    CDREC client;
    CHAR tdate[9], fname[50];
    FILE *fp;

    s_open(JB);
    vec = s_getno(JB, 0, po->jbcode);
    if (vec==-1) {
	message(M_SCR|M_KEY, "Job record does not exist - hit any key");
	return(-1);
    }
    s_read(JB, &jb, vec);
    s_close(JB);

    s_open(ES);
    vec = s_getno(ES, 0, po->jbcode);
    if (vec == -1) {
	message(M_SCR|M_KEY, "Estimate does not exist - hit any key");
	return(-1);
    }
    s_read(ES, &es, vec);
    s_close(ES);

    s_open(CD);
    vec = s_getno(CD, 0, es.client_code);
    if (vec == -1) {
	message(M_SCR|M_KEY, "Client does not exist - hit any key");
	return(-1);
    }
    s_read(CD, &client, vec);
    s_close(CD);

    FileTempName(fname);

    if ((fp = fileopn(fname, "wt")) == NULL) {
	message(M_SCR|M_ERR|M_KEY, "Unable to open %s - hit any key", fname);
	return(-1);
    }

    StrDate(tdate, &today);
    fprintf(fp, "\n\r\n\r%*s\n\r",(80-strlen(sysvar.company))/2+strlen(sysvar.company),sysvar.company);
    fprintf(fp, " _____________________________________________________________________________\n\r");
    fprintf(fp, "|   Job No.: %-8s            Date: %-8s             SP: %-3s           |\n\r",po->jbcode,tdate,es.spcode);
    fprintf(fp, "|_____________________________________________________________________________|\n\r");
    fprintf(fp, "    Client: %-32s    Client's PO:%s\n\r",client.name,jb.po);
    fprintf(fp, "    Contact: %-32s   Tel.: %s\n\r",es.estname,es.esttel);
    fprintf(fp, "    Job Name: %-32s  Site Tel.: %s\n\r",jb.name,jb.stel);
    fprintf(fp, "    Contact Jobsite: %-26s Due Date: %s\n\r",jb.pmgr,po->shipdate);
    fprintf(fp, " _____________________________________________________________________________ \n\r");
    fprintf(fp, "| PO No. |  Inv.  | Vendor |  COGS  | Markup |Sub-Tot | Tax  | Ship |  Total  |\n\r");
    fprintf(fp, "|________|________|________|________|________|________|______|______|_________|\n\r");
    fprintf(fp, "|%-8s|%-8s|%-8s|%8.2f|%8.2f|%8.2f|%6.2f|%6.2f|%9.2f|\n\r",po->po_number,po->inv_number,po->vdcode,po->cogs,po->mkup,po->cogs+po->mkup,po->tax,po->estship,po->bid);
    fprintf(fp, "|________|________|________|________|________|________|______|______|_________|\n\r");
    fprintf(fp, "Comments:       Additional Expense: %8.2f     Profit: %8.2f\n\r\n\r\n\r\n\r\n\r",(po->status[0] == 'S' ? po->actcogs-po->cogs:0),po->profit);
    fprintf(fp, "  ___________________________________________________________________________ \n\r");
    fprintf(fp, " | PO No. | VD Invoice # |  Date  |Sub-Total| Inv. No | Bill Number |Shipping|\n\r");
    fprintf(fp, " |________|______________|________|_________|_________|_____________|________|\n\r");
    fprintf(fp, " |%-8s|%-13s |%-8s|%9.2f| %-8s|%-13s|%8.2f|\n\r",po->po_number,po->vd_inv_number,po->podate,po->cogs,po->inv_number,po->billno,po->shipping);
    fprintf(fp, " |________|______________|________|_________|_________|_____________|________|\n\r");
    fprintf(fp, " |        |              |        |         |         |             |        |\n\r");
    fprintf(fp, " |________|______________|________|_________|_________|_____________|________|\n\r");
    fprintf(fp, " |        |              |        |         |         |             |        |\n\r");
    fprintf(fp, " |________|______________|________|_________|_________|_____________|________|\n\r");
    fprintf(fp, " | Total  |              |        |         |         |             |        |\n\r");
    fprintf(fp, " |________|______________|________|_________|_________|_____________|________|\n\r");
    filecls(fp);

    TextEditor(fname, client.fax, client.email);
    FileDelete(fname);

    return(0);
}


/*****************************************************************************
po_release_fabrication() - Shows release fabrication form for One PO only
*****************************************************************************/
INT po_release_fabrication(IN_POS *c, POREC *po, INT flag)
{
    VDREC vd;
    CHAR fname[70];
    INT vec, ans;

    /* if job is already released for fabrication, ask for whether to re-release it */
    if(flag != SUBCONTRACT_AGREEMENT && po->status[0] == 'R')
    {
	ans = message(M_SCR|M_KEY, "Job is already released for fabrication. Re-release it (y/N) ?");
	if(ans != 'Y' && ans != 'y')
	    return (0);
    }

    /* provide a temporary file name */
    FileTempName(fname);

    po_createpo_for_release_fabrication(c, po, fname, flag);

    if ((vec=s_getno(VD, 0, po->vdcode)) != -1) {
	s_read(VD, &vd, vec);

	TextEditor(fname, vd.fax, vd.email);

	s_close(VD);
    }
    FileDelete(fname);

    return(0);
}

INT po_createpo_for_release_fabrication(IN_POS *c, POREC *po, const CHAR *fname, INT flag)
{
    FILE *fp;
    CDREC client;
    VDREC vd;
    ESREC es;
    PLREC pl;
    JBREC jb;
    SPREC sp;
    struct date tempdate;
    CHAR tempstring[6][41], sttoday[9], dt1[9], dt2[9], change_status = 'Y', name[60], vdcode[9], vdincode[9];
    FLOAT total_price=0, total_shipping=0;
    INT i, vec, p=1, line, ans;
    USHORT n;
    BOOLEAN unit, saveflg = inPO;
    INT shipTo = 0; /* ship to job site, default */
    INT bg, ed;

    StrDate(sttoday, &today);

    if (po->status[0] != 'R' && flag != SUBCONTRACT_AGREEMENT) /* no need to ask question. We asked already before */
	change_status = message(M_SCR|M_KEY, "Change status to 'R' and save it (y/N) ?");

    if (change_status == 'Y')
    {
	vec = s_getno(PO, 0, po->po_number);
	po->status[0] = 'R';
	strcpy(po->release_date, sttoday);
	s_write(PO, po, vec, FALSE);
    }

    s_open(ES);
    if ((vec = s_getno(ES, 0, po->jbcode)) == -1) {
	message(M_SCR|M_KEY, "Estimate is not defined - hit any key");
	return(-1);
    }
    s_read(ES, &es, vec);
    if (es.status[0] != 'R' && change_status == 'Y')
    {
	es.status[0] = 'R';
	s_write(ES, &es, vec, FALSE);
    }
    s_close(ES);

    s_open(CD);
    if ((vec = s_getno(CD, 0, es.client_code)) == -1) {
	message(M_SCR|M_KEY, "Client is not defined - hit any key");
	return(-1);
    }
    s_read(CD, &client, vec);
    s_close(CD);

    s_open(JB);
    if ((vec = s_getno(JB, 0, po->jbcode)) == -1) {
      message(M_SCR|M_KEY, "Job is not defined - hit any key");
      return(-1);
    }
    s_read(JB, &jb, vec);
    if (jb.status[0] != 'R' && change_status == 'Y')
    {
	jb.status[0] = 'R';
	s_write(JB, &jb, vec, FALSE);
    }
    s_close(JB);

    s_open(SP);
    if ((vec = s_getno(SP, 0, po->spcode)) == -1) {
	message(M_SCR|M_KEY, "Sales Person is not defined - hit any key");
	return(-1);
    }
    s_read(SP, &sp, vec);
    s_close(SP);

    if (po->vdcode[0]) {
	if ((vec = s_getno(VD, 0, po->vdcode)) == -1) {
	    message(M_SCR|M_KEY, "Vendor is not defined - hit any key");
	    return(-1);
	}
	s_read(VD, &vd, vec);
	strcpy(vdincode, vd.code);
    } else {
	vdincode[0] = 0;
    }

    if (openplfile(&es) == -1) {
	message(M_SCR|M_KEY, "There are no products in the Part List - hit any key");
	return(-1);
    }

    if ((fp = fileopn(fname, "wb")) == NULL) {
	message(M_SCR|M_ERR|M_KEY, "Unable to Open %s - hit any key", fname);
	return(-1);
    }

    /* if subcontract, need to show vd pick up list */
    /* open PL file associated to this ES record */
    if(flag == SUBCONTRACT_AGREEMENT)
    {
	vdcode[0] = '\0';
	inPO = TRUE;
	if (vd_chooser_install(vdcode) == -1) 
	{
	    inPO = saveflg;
	    return (-1);
	}

	/* added on 1/28/98, change IN vendor to the subcontract just selected */
	if(!strnicmp(po->vdcode, "IN", 2))
	{
	    if(message(M_SCR|M_KEY, "Do you want to change the Vendor name to the subcontrator (y/N) ?") == 'Y')
	    {
		set_rate(&es, vdcode, TRUE);
		get_pl_pos(po->plidx, po->vdcode, &bg, &ed);

		strcpy(po->vdcode, vdcode);
		vec = s_getno(PO, 0, po->po_number);
		s_write(PO, po, vec, FALSE);

		/* update the screen */
		if(c != NULL)
		    printxy(5+strlen(db[PO].f[2].header), db[PO].f[2].dy, po->vdcode);

		/* also change PL */
//                vec = s_getno(VD, 0, vdcode);
//                s_read(VD, &vd, vec);
		for(i = bg; i <= ed; i++)
		{
		    s_read(PL, &pl, i);
		    strcpy(pl.vdcode, vdcode);
//                    strcpy(pl.pdcode, vd.product);
//                    strcpy(pl.description, vd.product);
		    s_write(PL, &pl, i, FALSE);
		}
	    }
	}
	inPO = saveflg;
    }
    else
    {
	ans = message(M_SCR|M_KEY, "Ship to the job site or office or warehouse (J/o/w) ?");
	if(ans == 'o' || ans == 'O')
	    shipTo = 1; /* ship to office */
	else if(ans == 'w' || ans == 'W')
	    shipTo = 2; /* ship to warehouse */

	ans = message(M_SCR|M_KEY, "Send to the Vendor or the Representative (V/r) ?");
	if(ans == 'r' || ans == 'R') /* to rep */
	{
	    sprintf(tempstring[0], "%s", vd.agent);
	    sprintf(tempstring[1], "c/o %s", vd.name);
	    sprintf(tempstring[2], "%s", vd.address);
	    sprintf(tempstring[3], "%s, %s %s", vd.city, vd.state, vd.zip);
	    sprintf(tempstring[4], "Tel: %s", vd.atel);
	    sprintf(tempstring[5], "Fax: %s", vd.afax);
	}
	else /* to the Vendor, also is the default */
	{
	    sprintf(tempstring[0], "%s", vd.name);
	    sprintf(tempstring[1], "%s", vd.address);
	    sprintf(tempstring[2], "%s, %s %s", vd.city, vd.state, vd.zip);
	    sprintf(tempstring[3], "Tel: %s", vd.tel);
	    sprintf(tempstring[4], "Fax: %s", vd.fax);
	    sprintf(tempstring[5], "%s", "");
	}

	ans = message(M_SCR|M_KEY, "Need delivery notification (N/y) ?");
    }

    /*
    * skip 10 spaces
    */
    fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r");

    if(flag == RELEASE)
	fprintf(fp, "                       R E L E A S E  T O  F A B R I C A T I O N\n\r\n\r\n\r");
    else if(flag == PURCHASE)
	fprintf(fp, "                           P U R C H A S E  O R D E R\n\r\n\r\n\r");
    else /* subcontract */
	fprintf(fp, "                     S U B C O N T R A C T  A G R E E M E N T\n\r\n\r\n\r");

    if(flag != SUBCONTRACT_AGREEMENT)
    {
	fprintf(fp, "   %-32s       Ship to:\n\r", tempstring[0]);
	if(shipTo == 2)
	    fprintf(fp, "   %-32s       %s\n\r", tempstring[1], compInfo.warehouse);
	else
	    fprintf(fp, "   %-32s       %s\n\r", tempstring[1], client.name);

	if(shipTo == 0)
	    fprintf(fp, "   %-32s       c/o: %s\n\r", tempstring[2], jb.name);
	else if(shipTo == 1)
	    fprintf(fp, "   %-32s       c/o: %s\n\r", tempstring[2], jb.name);
	else
	    fprintf(fp, "   %-32s       TAG: %s\n\r", tempstring[2], jb.name);

	if(shipTo == 0)
	    fprintf(fp, "   %-32s       %s\n\r", tempstring[3], jb.address);
	else if(shipTo == 1)
	    fprintf(fp, "   %-32s       %s\n\r", tempstring[3], client.address);
	else
	    fprintf(fp, "   %-32s       %s\n\r", tempstring[3], compInfo.whaddress1);

	if(shipTo == 0)
	    fprintf(fp, "   %-32s       %s, %s %s\n\r",tempstring[4], jb.city, jb.state, jb.zip);
	else if(shipTo == 1)
	    fprintf(fp, "   %-32s       %s, %s %s\n\r",tempstring[4], client.city, client.state, client.zip);
	else
	    fprintf(fp, "   %-32s       %s, %s %s\n\r",tempstring[4], compInfo.whcity, compInfo.whstate, compInfo.whzip);

	if(shipTo == 0)
	    fprintf(fp, "   %-32s       Attn: %s\n\r", tempstring[5], jb.pmgr);
	else if(shipTo == 1)
	    fprintf(fp, "   %-32s       Attn: %s\n\r", tempstring[5], jb.agent);
	else
	    fprintf(fp, "   %-32s       Attn: %s\n\r", tempstring[5], sp.name);

	if(shipTo == 0)
	    fprintf(fp, "   %-32s       Site Telephone: %s\n\r", "", jb.stel);
	else if(shipTo == 1)
	    fprintf(fp, "   %-32s       Contact Telephone: %s\n\r", "", client.tel);
	else
	    fprintf(fp, "   %-32s       Contact Telephone: %s\n\r", "", compInfo.whtel);
    }
    else
    {
	s_read(VD, &vd, s_getno(VD, 0, vdcode));
	s_close(VD);

	sprintf(tempstring[0], "%s, %s %s", vd.city, vd.state, vd.zip);
	fprintf(fp, "   %-32s       Ship to:\n\r", vd.name);
	fprintf(fp, "   %-32s       %s\n\r", vd.address, client.name);
	fprintf(fp, "   %-32s       c/o: %s\n\r", tempstring[0], jb.name);
	fprintf(fp, "   %-32s       %s\n\r", vd.fax, jb.address);
	fprintf(fp, "   %-32s       %s, %s %s\n\r","", jb.city, jb.state, jb.zip);
	fprintf(fp, "   %-32s       Attn: %s\n\r", "", jb.pmgr);
	fprintf(fp, "   %-32s       Site Telephone: %s\n\r", "", jb.stel);
    }
    
    tempdate = datecheck(po->shipdate);
    if (!tempdate.da_year) {
	tempdate = today;
    }

    StrDate(dt1, &tempdate);
    StrDate(dt2, DateAdd(&tempdate, 30));

    strcpy(name, jb.pmgr);
    vec = strlen(name);
    for(i = 0; i < vec; i++)
    {
	if(name[i] == ' ')
	{
	    name[i] = '\0';
	    break;
	}
    }

    if(flag != SUBCONTRACT_AGREEMENT)
	fprintf(fp, "\n\r    Attn: Customer service\n\r");
    else
	fprintf(fp, "\n\r    Attn: %s\n\r", vd.contact);
 
    fprintf(fp, "\n\r    ");
    for(i=0; i<72; fprintf(fp, "_"),i++)
	;

    /* use release date for all occurance, 7/15/97 */
//    if(flag != PURCHASE)
//        fprintf(fp, "\n\r   |PO: %-8s  Date: %-8s  Ship Date: %-8s  Ref: %-8s   P. %-2d|", po->po_number, po->podate, po->shipdate, jb.code, p);
//    else
//        fprintf(fp, "\n\r   |PO: %-8s  Date: %-8s  Ship Date: %-8s  Ref: %-8s   P. %-2d|", po->po_number, po->release_date, po->shipdate, jb.code, p);
    if(po->release_date[0])
	fprintf(fp, "\n\r   |PO: %-8s  Date: %-8s  Ship Date: %-8s  Ref: %-8s   P. %-2d|", po->po_number, po->release_date, po->shipdate, jb.code, p);
    else
	fprintf(fp, "\n\r   |PO: %-8s  Date: %-8s  Ship Date: %-8s  Ref: %-8s   P. %-2d|", po->po_number, sttoday, po->shipdate, jb.code, p);

    fprintf(fp, "\n\r   |");
    for(i=0; i<72; fprintf(fp, "_"),i++)
	;
    fprintf(fp, "|");

    fprintf(fp, "\n\r   |Product Code         Product Description             Qty");

    /*
    * automatically detect the need for NOT displaying Unit prices
    */
    unit = TRUE;
    n = s_nrecs(PL);
    if (n > 1) {
	s_read(PL, &pl, 1);
	if (pl.price == 0.0)
	    unit = FALSE;
    }

    if (unit)
	fprintf(fp, " $/Unit    Price|");
    else
	fprintf(fp, "                |");

    fprintf(fp, "\n\r   |");
    for(i=0; i<72; fprintf(fp, "_"),i++)
	;
    fprintf(fp, "|");

    line = 14;

    es.mkuprate = es.taxrate = 0;

    if(po->plidx <= 0) /* old style PO */
    {
	bg = 0;
	ed = n - 1;
    }
    else
    {
	set_rate(&es, vdincode, TRUE); /* this sets vdlist[] global */
	get_pl_pos(po->plidx, vdincode, &bg, &ed);
    }

    for (i=0; i<n; i++) {
	s_read(PL, &pl, i);
	if (strcheck(vdincode, pl.vdcode))
	    continue;

	if(i < bg || i > ed)
	    continue;

	if ((vec = s_getno(VD, 0, pl.vdcode)) == -1) {
	    message(M_SCR|M_KEY, "Undefined vendor in product list - hit any key");
	    filecls(fp);
	    return(-1);
	}
	total_price += pl.price*(1+es.mkuprate)*pl.qty;
	s_read(VD, &vd, vec);
	fprintf(fp, "\n\r   |%-21s%-29s", pl.pdcode, pl.description);
	if (pl.qty) {
	    fprintf(fp, "%5ld", (long)pl.qty);
	    if (unit)
		fprintf(fp, "%8.2f %8.2f|", pl.price*(1+es.mkuprate), pl.price*(1+es.mkuprate)*pl.qty);
	    else
		fprintf(fp, "                 |");
	} else {
	    fprintf(fp, "                      |");
	}

	if (++line > 60) {
	    line=1;
	    fprintf(fp, "\f");
	}
    }

    if (++line > 60) {
	line=1;
	fputc('\f',fp);
    }
    fprintf(fp, "\n\r   |");
    for(i=0; i<72; fprintf(fp, "_"),i++)
	;
    fprintf(fp, "|");
    fprintf(fp, "\n\r   %53.53s    Total: $%8.2f\n\r", "", total_price*(1+es.taxrate)+total_shipping);
    
    DateStr(&tempdate, po->shipdate);

    if(flag != SUBCONTRACT_AGREEMENT)
    {
	fprintf(fp, "\n\r   Material is required on site by %s.", StrDate(dt1, DateAdd(&tempdate, 5)));
	fprintf(fp, "\n\r   Please confirm in writing that you can meet this delivery date.");
    }
    else
    {
	fprintf(fp, "\n\r   Material needs to be installed by %s.", StrDate(dt1, DateAdd(&tempdate, 5)));
	fprintf(fp, "\n\r   Please confirm in writing that you can meet this installation date.");
	fprintf(fp, "\n\r\n\r   Subcontractor should carry Liability and Workman Compansation");
	fprintf(fp, "\n\r   Insurance and comply with all State and Federal Labor Laws.");
    }

    if(flag != SUBCONTRACT_AGREEMENT)
    {
	fprintf(fp, "\n\r\n\r   Upon shipment please advise our office of your designated");   
	fprintf(fp, "\n\r   freight Co., Pro number and their local terminal phone number.");
    }

    if(ans == 'Y' || ans == 'y')
    {
	fprintf(fp, "\n\r\n\r   Please have freight company notify this office and job site");
	fprintf(fp, "\n\r   24 hours prior to scheduled delivery date.");
    }

    fprintf(fp, "\n\r\n\r   Please contact %s at 617-542-4445 if you have", sp.name);
    fprintf(fp, "\n\r   any questions regarding this PO.  Please refer to PO # and");
    fprintf(fp, "\n\r   Reference # when invoicing.\n\r");
    fprintf(fp, "\n\r\n\r   Thank you.");

    if(flag == RELEASE)
	fprintf(fp, "\n\r\n\r\n\r   PS: Please do not duplicate this order.");
    
    filecls(fp);

    return(0);
}

/*****************************************************************************
rs_funcs (hot key hook) - processes all hot keys for RS (Room Schedule) file
    ident:          type
    c:      cursor struct
    ptr:        current record
*****************************************************************************/
INT rs_funcs(DB_TYPE type, IN_POS *c, RSREC *rs)
{
    FIELDPARAMS attr, fp;
    PDREC pd;
    RSREC rs1;
    temp_rsrec rst[40]; // Changed size from 30 to 40, 8/9/97
    CHAR check[21];
    INT anew=0,i,j,n,cur=1,cy,cyp,roomRow=1,fh;
    BOOLEAN tempchangeflag;
    CHAR fname[60], tmpPath[30], tmpName[9], tmpExt[4];

    SHORT sheetNo, totalSheet = 0;

    CHAR *list[] = {"Print to sheet", NULL};
    INT choice;

    // Figure out total number of pages
    do {
	sprintf(fname, "%s%s.%03d", db[type].path, db[type].fname, cur++);
	if(access(fname, 0x00) != 0)
	    break;
	totalSheet++;
    } while(1);

    attr = db[type].f[c->x];
    gotoxy(66, 1);
    sscanf(db[type].extension, "%hd", &sheetNo);
    Print("Sheet: %2d of %d", sheetNo, totalSheet);
    cur = atoi(db[type].extension);

    if (cur==1)
	rs->prevt=0;

    if (rs->vdcode[0]!='\0') {
	strcpy(db[PD].fname, rs->vdcode);
    }

    switch(c->x) {

	/*
	* Super Help = Class x Line
	*/
	case KEY_ALT_F1: {

	    c->x=c->last;
	    if (c->last!=1)
		break;
	    pd=xref_help(rs->vdcode, FALSE);
	    if (!strcmp(pd.vendor, "NO"))
		break;
	    strcpy(rs->pdcode,pd.code);
	    strcpy(rs->pddesc,pd.description);
	    in_printline(type,c->y,c->ypos,rs);
	}
	break;

	/*
	* Multiply column
	*/
	case KEY_ALT_F2:
	{
	    INT key;
	    CHAR col[2], factor[16];

aa:         col[0] = '1'; col[1] = '\0';
	    message(M_SCR, NULL);
	    key = inpt(1, 25, "Enter column number (1 - 7) for multiplication: ", col, 1, CT_CHAR, NO);
	    if(key == KEY_ESC1 || key == KEY_ESC || key == KEY_F10 || key == KEY_ENTER)
		return 0;
	    if(col[0] < '1' || col[0] > '7')
		goto aa;

bb:         factor[0] = '\0';
	    message(M_SCR, NULL);
	    key = inpt(1, 25, "Enter factor to multiply: ", factor, 10, CT_CHAR, NO);
	    if(key == KEY_ESC1 || key == KEY_ESC || key == KEY_F10 || key == KEY_ENTER)
		return 0;
	    if((key = atoi(factor)) < 0)
		goto bb;

	    n = s_nrecs(type);
	    for (i=0; i<n; i++) {
		s_read(type,&rs1,i);
		rs1.total -= rs1.qt[col[0] - '0' - 1];
		rs1.qt[col[0] - '0' - 1] *= key;
		rs1.total += rs1.qt[col[0] - '0' - 1];
		s_recwrite(type, &rs1, i);
		if(i == c->ypos)
		    *rs = rs1;
	    }
	    insetup(type, c, NULL);
	}
	break;

	/*
	* Copy column
	*/
	case KEY_CTRL_F2:
	{
	    INT key;
	    CHAR fromCol[2], toCol[2], fromSht[3], toSht[3];
	    CHAR fromFile[60], toFile[60];
	    SHORT *qty;

cc:         sprintf(fromSht, "%d", cur);
	    message(M_SCR, NULL);
	    key = inpt(1, 25, "Enter source sheet number to copy from: ", fromSht, 2, CT_CHAR, NO);
	    if(key == KEY_ESC1 || key == KEY_ESC || key == KEY_F10 || key == KEY_ENTER)
		return 0;
	    sprintf(fromFile, "%s%s.%03d", db[type].path, db[type].fname, atoi(fromSht));
	    if(access(fromFile, 0x00) != 0)
	    {
		message(M_SCR|M_KEY, "Specified sheet doesn't exist. Hit any key to continue");
		goto cc;
	    }

dd:         fromCol[0] = '1'; fromCol[1] = '\0';
	    message(M_SCR, NULL);
	    key = inpt(1, 25, "Enter source column number (1 - 7) to copy from: ", fromCol, 1, CT_CHAR, NO);
	    if(key == KEY_ESC1 || key == KEY_ESC || key == KEY_F10 || key == KEY_ENTER)
		return 0;
	    if(fromCol[0] < '1' || fromCol[0] > '7')
		goto dd;

ee:         sprintf(toSht, "%d", cur);
	    message(M_SCR, NULL);
	    key = inpt(1, 25, "Enter destination sheet number to copy to: ", toSht, 2, CT_CHAR, NO);
	    if(key == KEY_ESC1 || key == KEY_ESC || key == KEY_F10 || key == KEY_ENTER)
		return 0;
	    sprintf(toFile, "%s%s.%03d", db[type].path, db[type].fname, atoi(toSht));
	    if(access(toFile, 0x00) != 0)
	    {
		message(M_SCR|M_KEY, "Specified sheet doesn't exist. Hit any key to continue");
		goto ee;
	    }

ff:         toCol[0] = '\0';
	    message(M_SCR, NULL);
	    key = inpt(1, 25, "Enter destination column number (1 - 7) to copy to: ", toCol, 1, CT_CHAR, NO);
	    if(key == KEY_ESC1 || key == KEY_ESC || key == KEY_F10 || key == KEY_ENTER)
		return 0;
	    if(toCol[0] < '1' || toCol[0] > '7')
		goto ff;

	    n = s_nrecs(type);
	    qty = (SHORT*) malloc(n * sizeof(SHORT));
	    if(qty == NULL)
		return 0;
	    
	    /* open source RS file to read qty */
	    s_close(type);
	    sprintf(db[type].extension, "%03d", atoi(fromSht));
	    s_open(type);
	    for (i=0; i<n; i++) {
		s_read(type, &rs1, i);
		qty[i] = rs1.qt[fromCol[0] - '0' - 1];
	    }

	    /* open destination RS file to copy qty */
	    s_close(type);
	    sprintf(db[type].extension, "%03d", atoi(toSht));
	    s_open(type);
	    for (i=0; i<n; i++) {
		s_read(type, &rs1, i);
		rs1.total -= rs1.qt[toCol[0] - '0' - 1];
		rs1.qt[toCol[0] - '0' - 1] = qty[i];
		rs1.total += rs1.qt[toCol[0] - '0' - 1];
		s_recwrite(type, &rs1, i);
		if(i == c->ypos && atoi(toSht) == cur)
		    *rs = rs1;
	    }

	    /* if the destination page is the current sheet, re-draw the sheet */
	    if(atoi(toSht) == cur)
		insetup(type, c, NULL);

	    /* put the original RS file back */
	    s_close(type);
	    sprintf(db[type].extension, "%03d", cur);
	    free(qty);
	}
	break;

	/*
	* Form function for RS (e.g., landscaping printing, etc.)
	*/
	case KEY_F9:
	    if ((choice = window_chooser(33, 10, list)) == -1)
		return(0);
	    switch(choice) {
		/* landscape print */
		case 0:
		    if(SelectPrinter())
			print_room_schedule(saveFileName);
		break;

		/* to be implemented */

		default:
		break;
	    }
	break;

	/*
	* Switch between Product and PD Description
	*/
	case KEY_F4:
	    c->x = 0;
	    fp = *(db[type].f+1);
	    *(db[type].f+1) = *(db[type].f+10);
	    *(db[type].f+10) = fp;
	    switchPDDSFlg=(switchPDDSFlg ? FALSE:TRUE);
	    c->set = YES; 
	    c->flags = 4; /* this will refresh the window */

	break;

	/*
	* Enter Room Numbers to be shown at header
	*/
	case KEY_F7:
	    tempchangeflag = changeflg;

	    i=0;
	    while (c->x!=KEY_F10) {
		if(roomRow == 1) /* enter first line of room number */
		    c->x = inpt(34+(i*6), 3, "", rooms1[i], 4, CT_CHAR, NO);
		else /* enter second line of room number */
		    c->x = inpt(34+(i*6), 4, "", rooms2[i], 4, CT_CHAR, NO);
		StrReplaceChar(db[type].f[i+2].header,db[type].f[i+2].header[5],'\0');
		strcpy(db[type].f[i+2].header,rooms1[i]);
		strcat(db[type].f[i+2].header,"|");
		strcat(db[type].f[i+2].header,rooms2[i]);

		switch (c->x) {

		    case KEY_RIGHT:
		    case 1:
			i++;
			break;

		    case KEY_LEFT:
		    case KEY_SHFT_TAB:
			if (i==0)
			    break;
			i--;
			break;

		    case KEY_UP:
			if(roomRow == 1)
			    c->x=KEY_F10;
			else
			    roomRow = 1;
			break;

		    case KEY_DOWN:
			if(roomRow == 2)
			    c->x=KEY_F10;
			else
			    roomRow = 2;

		    default:
			break;
		}
		if (i==7)
		    c->x=KEY_F10;
	    }
	    c->x = c->last;
	    changeflg = tempchangeflag;
	    FileSeek(db[type].file, (LONG)(2), SEEK_SET);
	    FileWrite(db[type].file, &rooms1, 35);
	    FileWrite(db[type].file, &rooms2, 35);
	    c->set = YES;  /*OPP*/
	    break;

	case KEY_CTRL_LEFT:   /* Go Back one Sheet */
	    c->x=c->last;
	    if (atoi(db[type].extension) == 1)
		break;
	    s_vcheck(type, rs, c->ypos);
	    cur=atoi(db[type].extension);
	    sprintf(db[type].extension, "%03d", --cur);
	    s_open(type);
	    FileSeek(db[type].file, 2, SEEK_SET);
	    FileRead(db[type].file, &rooms1, 35);
	    FileRead(db[type].file, &rooms2, 35);
	    n = s_nrecs(type);
	    for (i=0; i<n; i++) {
		s_read(type, &rs1, i);
		if (i == c->ypos) {
		    *rs = rs1;
		    break;
		}
	    }
	    s_close(type);
	    for (i=0; i<=6; i++) {
		StrReplaceChar(db[type].f[i+2].header,db[type].f[i+2].header[5],'\0');
		strcpy(db[type].f[i+2].header,rooms1[i]);
		strcat(db[type].f[i+2].header,"|");
		strcat(db[type].f[i+2].header,rooms2[i]);
	    }
	    c->set = YES;
	    c->flags = 4;
	    break;

	case KEY_F8:
	case KEY_CTRL_RIGHT:  /* Go to next sheet, creating it if need be */
	{
	    CHAR fpath[60];

	    s_vcheck(type, rs, c->ypos);
	    sprintf(fpath, "%s%s.%03d", db[type].path, db[type].fname, cur+1);
	    if ((c->x == KEY_F8) && !FileExists(fpath))
		;
	    else
		do {
		    n = s_nrecs(type);
		    for (i=0; i<n; i++) {
			s_read(type,&rs1,i);
			strcpy(rst[i].vdcode, rs1.vdcode);
			strcpy(rst[i].pdcode, rs1.pdcode);
			strcpy(rst[i].pddesc, rs1.pddesc);
			rst[i].total = rs1.total;
		    }
		    s_close(type);
		    cur = atoi(db[type].extension);
		    sprintf(db[type].extension, "%03d", ++cur);
		    anew = s_open(type);
		    for (i=0; i<n; i++) {
			s_read(type,&rs1,i);
			if (i == c->ypos)
			    *rs=rs1;
			strcpy(rs1.vdcode, rst[i].vdcode);
			strcpy(rs1.pdcode, rst[i].pdcode);
			strcpy(rs1.pddesc, rst[i].pddesc);
			rs1.prevt = rst[i].total;
			rs1.total = rs1.qt[0]+rs1.qt[1]+rs1.qt[2]+rs1.qt[3]+rs1.qt[4]+rs1.qt[5]+rs1.qt[6]+rs1.prevt;
			s_recwrite(type, &rs1, i);
		    }
		    s_modnrecs(type,n);
		    FileSeek(db[type].file,(LONG)(2),SEEK_SET);
		    FileRead(db[type].file,&rooms1,35);
		    FileRead(db[type].file,&rooms2,35);
		    for (i=0; i<=6; i++) {
			StrReplaceChar(db[type].f[i+2].header,db[type].f[i+2].header[5],'\0');
			if (anew == -2)
			{
			    memset(&rooms1, 0, 35);
			    memset(&rooms2, 0, 35);
			}
			strcpy(db[type].f[i+2].header,rooms1[i]);
			strcat(db[type].f[i+2].header,"|");
			strcat(db[type].f[i+2].header,rooms2[i]);
		    }
		    if (anew == -2) {
			FileSeek(db[type].file,(LONG)(2),SEEK_SET);
			FileWrite(db[type].file,&rooms1,35);
			FileWrite(db[type].file,&rooms2,35);
		    }
		    sprintf(fpath, "%s%s.%03d", db[type].path, db[type].fname, cur+1);
		} while ((c->x == KEY_F8) && FileExists(fpath));

		/*
		* Create Part List from this Room Schedule, then go to PL
		*/
		if (c->x == KEY_F8) {
		    PLREC *pl, tmpPl;
		    INT numPl, numRs, numCur, savePos;
		    CHAR path[60], name[60];

		    ESREC es;
		    INT vec;

/* No question is asked, because it will not overwrite PL. 5/12/97
		    if (message(M_SCR|M_KEY, "This will Destroy the Old Part List! Continue? (y/N)") != 'Y') {
			c->x = c->last;
			c->set = YES;
			c->flags = 4;
			break;
		    }
*/
		    /*
		    * Record the PL file to es record
		    */
		    vec = s_getno(ES, 0, db[PL].fname);
		    if(vec == -1)
			break;  // impossible

		    s_read(ES, &es, vec);
		    if(es.plvec == 0) // no Part List, create one
		    {
			plmake(vec, &es);
			s_write(ES, &es, vec, FALSE);
		    }
		    plfile(&es);

		    s_open(PL);
		    numRs = s_nrecs(type);
		    numPl = s_nrecs(PL);
		    pl = (PLREC*) malloc(sizeof(PLREC) * (numRs + numPl));
		    if(pl == NULL)
			break;
		    s_close(PL);

		    /*
		    * save the path, name and extension of the current PL file
		    */
		    strcpy(tmpPath, db[PL].path);
		    strcpy(tmpName, db[PL].fname);
		    strcpy(tmpExt, db[PL].extension);

		    /*
		    * make temporary copy of PL file, and work from that temp copy
		    */
		    sprintf(fname, "%s%s.%s", tmpPath, tmpName, tmpExt);
		    if ((fh = FileOpen(fname)) == -1)
			break;

		    s_tempfile(PL);
		    s_open(PL);
		    FileCopyH(fh, db[PL].file);

		    memset(pl, 0, sizeof(PLREC) * (numRs + numPl));
		    for(j = 0; j < numPl; j++)
			s_read(PL, &pl[j], j);
		    numCur = numPl;

		    for (i=0; i<numRs; i++) {
			s_read(type,&rs1,i);
			savePos = numPl + numRs;
			for(j = 0; j < numCur; j++)
			{
			    if(!strcmp(pl[j].vdcode, rs1.vdcode) && !strcmp(pl[j].pdcode, rs1.pdcode))
			    /* found a match, only update its quantity field */
			    {
				pl[j].qty = rs1.total;
				strcpy(pl[j].description, rs1.pddesc);
				break;
			    }
			    else if(!strcmp(pl[j].vdcode, rs1.vdcode))
				savePos = j; /* will be inserted to this position */
			}
			if(j >= numCur) /* no matching one is found, write to the saved position */
			{
			    memset(&tmpPl, 0, sizeof(PLREC));
			    strcpy(tmpPl.vdcode, rs1.vdcode);
			    strcpy(tmpPl.pdcode, rs1.pdcode);
			    strcpy(tmpPl.description, rs1.pddesc);
			    tmpPl.qty = rs1.total;
			    if (pl_get(&tmpPl, 0) == -1) {
//                                tmpPl.description[0] = '\0';
				tmpPl.f = tmpPl.price = tmpPl.weight = 0;
			    }

			    if(savePos < numRs + numPl) /* shifting */
			    {
				for(j = numCur - 1; j >= savePos+1; j--)
				    memcpy(&pl[j+1], &pl[j], sizeof(PLREC));
				savePos += 1;
			    }
			    else
				savePos = numCur;
			    memcpy(&pl[savePos], &tmpPl, sizeof(PLREC));
			    numCur++;
			}
		    }
		    /* write PL back to file */
		    for(j = 0; j < numCur; j++)
			s_recwrite(PL,&pl[j],j);
		    s_modnrecs(PL, j);
		    free(pl);

		    /* We will exit RS screen. Ask whether to save */
		    s_close(type);
		    needSave = FALSE;
		    if (savecheck()) {
			j = 1; 
			sprintf(name, "%s%s.%03d", tempPath, tempFileName, j);
			while(access(name, 0x00) == 0)
			{
			    sprintf(path, "%s%s.%03d", savePath, saveFileName, j);
			    FileCopy(name, path);
			    j++;
			    sprintf(name, "%s%s.%03d", tempPath, tempFileName, j);
			}
		    }

		    store();
		    db_inpt(PL);             /* Go to Part List */

		    /*
		    * if user wants to save changes, then copy new stuff over the
		    * original PL file
		    */
		    if (savecheck()) {
			s_open(PL);
			FileCopyH(db[PL].file, fh);
		    }

		    /* delete the temp file */
		    s_delfile(PL);
		    FileClose(fh);
		    restore();

		    /*
		    * restore the path, name and extension of the current PL file
		    */
		    strcpy(db[PL].path, tmpPath);
		    strcpy(db[PL].fname, tmpName);
		    strcpy(db[PL].extension, tmpExt);

		    /* exit directly to Quotation screen */
		    c->x = KEY_F10;
		    break;
		}
		c->x = c->last;
		c->set = YES;
		c->flags = 4;  /*OPP*/
	} break;

	default:    /* Standard Input for other Fields */
	    /*strcpy(check,PTR(rs,type,c->x));*/
	    strcpy(check, ((CHAR *)rs + db[type].f[c->x].offset));
	    c->last = c->x;
	    cy = c->y;
	    cyp = c->ypos;
	    if (rs->vdcode[0]=='\0')
		strcpy(rs->vdcode,vdlast);

	    /*c->x = inpt(attr.x,c->y,"",PTR(rs,type,c->x),attr.width-1,attr.type,NO);*/
	    c->x = inpt(attr.x,c->y,"", ((CHAR *)rs + db[type].f[c->x].offset),attr.width-1,attr.type,NO);
	    if(c->last == 1 && switchPDDSFlg) /* need to copy PD Desc to rs.pddesc */
	    {
		if(strlen(rs->pddesc) == 0 || !strcmp(rs->pddesc, ""))
		{
		    if(rs->pdcode[0] != '\0')
		    {
			s_read(PD, &pd, s_getno(PD, 0, rs->pdcode));
			strcpy(rs->pddesc, pd.description);
		    }
		}
	    }
	    rs->total=rs->qt[0]+rs->qt[1]+rs->qt[2]+rs->qt[3]+rs->qt[4]+rs->qt[5]+rs->qt[6]+rs->prevt;
	    strupr(rs->vdcode);
	    if (*rs->vdcode && (c->last == 0)) {
		if (s_getno(VD,0,rs->vdcode)==-1) {
		    message(M_SCR|M_KEY, "Vendor is not defined - hit any key");
		    strcpy(rs->vdcode, check);
		    gotoxy(attr.x, c->y);
		    Print(attr.format, rs->vdcode);
		}
	    }
// The following section was commented on 8/9/97, because modify VD code usually means
// that the PD or Desc should be modified, which is coverd later
//            if(c->last == 0 && strcmp(check, rs->vdcode)) /* modified vdcode */
//            {
//                if(!strcmp(check, ""))
//                    populate_rs(c->ypos, rs, FALSE);
//                else /* vdcode changed */
//                    populate_rs(c->ypos, rs, TRUE);
//            }
	    else if(c->last == 1 && strcmp(check, rs->pddesc) && !switchPDDSFlg)
		populate_rs(c->ypos, rs, TRUE);
	    else if(c->last == 1 && strcmp(check, rs->pdcode) && switchPDDSFlg)
		populate_rs(c->ypos, rs, TRUE); 

	    if (rs->vdcode[0] != '\0') {
		strcpy(vdlast,rs->vdcode);
		s_close(PD);
		strcpy(db[PD].fname, rs->vdcode);
	    }
	    in_printline(type, cy, cyp, rs);
	    break;
    }
    return(0);
}

/*****************************************************************************
populate_rs - Adds or deletes the record to/from other pages.
*****************************************************************************/
VOID populate_rs(INT pos, VOID *data, BOOLEAN change)
{
    RSREC *rs = (RSREC*) data, temp;
    int cur, i = 0, j;
    CHAR saveExt[4], fName[128];

    s_close(RS);
    cur = atoi(db[RS].extension);
    strcpy(saveExt, db[RS].extension);

    do {
	if(++i == cur)
	    continue;
	sprintf(fName, "%s%s.%03d", db[RS].path, db[RS].fname, i);
	if(access(fName, 0x00) != 0)
	    break;

	/* update other pages */
	sprintf(db[RS].extension, "%03d", i);
	if(change == FALSE)
	{
	    if(rs == NULL) /* delete */
		s_remove_rec(RS, &temp, pos);
	    else
	    {
		s_lock(RS);
		for(j=s_nrecs(RS); j>=pos+1; j--)
		{
		    s_read(RS, &temp, j-1);
		    s_recwrite(RS, &temp, j);
		}
		s_modnrecs(RS, s_nrecs(RS)+1);
		s_recwrite(RS, rs, pos);
		s_unlock(RS);
	    }
	}
	else /* change record, only changes the vdcode, pdcode and pddesc field */
	{
	    if(pos < s_nrecs(RS))
	    {
		s_read(RS, &temp, pos);
		strcpy(temp.vdcode, rs->vdcode);
		strcpy(temp.pdcode, rs->pdcode);
		strcpy(temp.pddesc, rs->pddesc);
		s_write(RS, &temp, pos, FALSE);
	    }
	    else /* this equals adding a new record */
	    {
		s_lock(RS);
		s_modnrecs(RS, s_nrecs(RS)+1);
		s_recwrite(RS, rs, pos);
		s_unlock(RS);
	    }
	}
	s_close(RS);
    } while(1);

    /* restore the current RS file */
    strcpy(db[RS].extension, saveExt);
}

/*****************************************************************************
part_list_print - Prints Part List for Bid, Award, & Acknowledgement.
*****************************************************************************/
INT part_list_print(FILE *printer, INT plist, INT ups, ESREC *es, CHAR *vdcode, BOOLEAN flag, INT idx)
{
    PLREC pl;
    VDREC vd;
    CHAR m[6]={"     "};
    INT line;
    INT n,tagd=NO,i,j,vec,k;
    ESREC estemp=*es;
    FLOAT unitPrice, totalMerch;
    INT tx, fg; /* added on 4/15/97 for unit price including tax and freight */

    flag = flag;

    if(plist == 'N')
	ups = 'N';
    tx = fg = 'N';
    if(ups != 'N')
    {
	tx = message(M_SCR|M_KEY, "Include tax in unit price? (Y/n)");
	fg = message(M_SCR|M_KEY, "Include freight in unit price? (Y/n)");
	if(tx != 'N') tx = 'Y';
	if(fg != 'N') fg = 'Y';
    }

    line = 0;
    s_open(VD);
    s_open(PL);
    if (plist != 'N') {
	if (ups!='N') {
	    fprintf(printer, "\n\n%sQty   Model No.                  Description          Unit $  Brand Nm.", m);
	    fprintf(printer, "\n%s----- ------------------- -------------------------- -------- -----------", m);
	} else {
	    fprintf(printer, "\n\n%sQty   Model No.                  Description              Brand Nm.", m);
	    fprintf(printer, "\n%s----- ------------------- ------------------------------- --------------", m);
	}
	line = 16;
    }
    es->mkup = es->estship = es->cogs = es->taxable = 0;
    n = s_nrecs(PL);

/* the following piece of code is replicated in kk_more.c (re_calc) as well */
    if(!stricmp(vdcode, "*"))
    {
	vdcode[0] = '!';
	for(j = 0; j < MAXVENDORS; j++)
	    if(vdlist[j][0] != '\0')
		tagged[j] = TRUE;
    }
    i = 0;
    for(j = 0; j < MAXVENDORS; j++) {
	// inits
	tagd = NO;
	for(; !tagd && i < n; i++) {
	    s_read(PL, &pl, i);
	    if (!strcmp(vdcode, "!")) {
		if(!stricmp(vdlist[j], pl.vdcode)) {
		    if(tagged[j])
			tagd = YES;
		    else /* skip all the PLs in the same group */
		    {
			i++;
			for(; i < n; i++)
			{
			    s_read(PL, &pl, i);
			    if(stricmp(vdlist[j], pl.vdcode))
				break;
			}
			break;
		    }
		}
	    }
	    else if (!strcheck(vdcode,pl.vdcode)) {
		if(!stricmp(vdcode, pl.vdcode))
		{
		    if(stricmp(vdcode, vdlist[j]))
			break;
		    if(j == idx-1)
			tagd = YES;
		    else /* skip all the PLs in the same group */
		    {
			i++;
			for(; i < n; i++)
			{
			    s_read(PL, &pl, i);
			    if(stricmp(vdcode, pl.vdcode))
				break;
			}
			break;
		    }
		}
		else
		{
		    if(!stricmp(vdlist[j], pl.vdcode))
			tagd = YES;
		}
	    }
	}

/*    for (p=0; p<n; p++) {
	tagd = NO;
	s_read(PL, &pl, p);
	if (!strcmp(vdcode, "!")) {
	    for (i=pos; !tagd&&i<MAXVENDORS; i++) {
		if (tagged[i] && (!stricmp(vdlist[i], pl.vdcode)))
		    tagd = YES;
		else if (stricmp(vdlist[i], pl.vdcode))
		    pos = i+1;
		else if (!stricmp(vdlist[i], pl.vdcode))
		{
		    pos = i;
		    break;
		}
	    }
	} else if (!strcheck(vdcode, pl.vdcode)) {
	    if(!stricmp(vdcode, pl.vdcode))
	    {
		for (i=pos; !tagd&&i<MAXVENDORS; i++) {
		    if (tagged[i] && (!stricmp(vdlist[i], pl.vdcode)))
			tagd = YES;
		    else if (stricmp(vdlist[i], pl.vdcode))
			pos = i+1;
		    else if (!stricmp(vdlist[i], pl.vdcode))
			break;
		}
	    }
	    else
		tagd = YES;
	}
*/
	if (!tagd)
	    continue;

      do {
	if ((vec = s_getno(VD, 0, pl.vdcode)) == -1) {
	    message(M_SCR|M_KEY, "Undefined vendor in product list - hit any key");
	    return(-1);
	}
	s_read(VD,&vd,vec);
	es->cogs += pl.price * pl.qty;

	/* Charge Tax if Taxable */
	es->taxable += (toupper(vd.taxable[0])=='Y' ? pl.price * pl.qty + pl.price * pl.qty * pl.markup/100:0);
	es->mkup += pl.price * pl.qty * pl.markup/100;
	if(sysvar.ftax)
	    es->estship += pl.f * pl.qty * pl.weight * (1 + es->taxrate);
	else
	    es->estship += pl.f * pl.qty * pl.weight;
	if (plist != 'N') {
	    unitPrice = pl.price*pl.markup/100+pl.price;
	    if(tx != 'N' && toupper(vd.taxable[0])=='Y')
	      unitPrice = unitPrice * (1 + es->taxrate);
	    if(fg != 'N')
	    {
	      if(sysvar.ftax)
		unitPrice = unitPrice + pl.f*pl.weight*(1+es->taxrate);
	      else
		unitPrice = unitPrice + pl.f*pl.weight;
	    }
	    if (ups!='N') {
	      if(unitPrice != 0) {
		if (!pl.qty)
		    fprintf(printer, "\n%s      %-21s%-24s %8.2f  %-11.11s",m,pl.pdcode,pl.description,unitPrice,vd.name);
		else
		    fprintf(printer, "\n%s%-5ld %-21s%-24s %8.2f  %-11.11s",m,(long)pl.qty,pl.pdcode,pl.description,unitPrice,vd.name);
	      }
	    } else {
		if (!pl.qty)
		    fprintf(printer, "\n%s      %-21s%-30s %-14.14s",m,pl.pdcode,pl.description,vd.name);
		else
		    fprintf(printer, "\n%s%-5ld %-21s%-30s %-14.14s",m,(long)pl.qty,pl.pdcode,pl.description,vd.name);
	    }

	    if (++line > 60) {
		line = 1;
		/*fprintf(printer, "%c", 12);*/
		fputc(12, printer);
	    }
	}
	if(i >= n)
	    break;
	s_read(PL, &pl, i);
	if(stricmp(vdlist[j], pl.vdcode))   /* not in the same group */
	    break;
	i++;
      }
      while (1);
    }
    if (line > 60) {
	line = 1;
	/*fprintf(printer, "%c", 12);*/
	fputc(12, printer);
    }
    es->esttax = es->taxable*es->taxrate;  /* Only Tax that which is taxable. */
#ifndef REP_VER
    es->bid = es->cogs+es->esttax+es->mkup+es->estship;
#else
    es->bid = es->cogs+es->estship;
#endif
    es->profit = es->bid - (es->com+es->esttax+es->estship+es->cogs);

    /*
    * If not print Part List then show tagged Vendors by Category
    */
    if (plist == 'N') {
	/*
	* If user picked all Vendors in PL then tag all.
	*/
	if (!strcmp(vdcode, "*")) { /* this is never TRUE since 1/7/98 */
	    for (i=0; i<MAXVENDORS; i++) {
		tagged[i] = NO;
		vdlist[i][0] = '\0';
	    }
	    n = s_nrecs(PL);
	    for (i=0; i<n; i++) {
		s_read(PL, &pl, i);
		for (k=0; k<MAXVENDORS; k++) {
		    if (!stricmp(vdlist[k], pl.vdcode))
			break;
		    if (vdlist[k][0] == '\0') {
			strcpy(vdlist[k], pl.vdcode);
			tagged[k] = YES;
			break;
		    }
		    /*else {
			continue;
		    }*/
		}
	    }
	}

	/*
	* Print Tagged Vendors by Product Type.
	*/
	for (i=0; i<MAXVENDORS; i++) {
	    if (tagged[i] || !strcmp(vdlist[i], "*")) {

		/* Calc totals for each Vendor */
		re_calc(&estemp, vdlist[i], FALSE, i+1);

		/* Read Vendor info */
		s_read(VD, &vd, s_getno(VD, 0, vdlist[i]));

		fprintf(printer, "\n\n%s%-21s by %-32s for $%9.2f",m,vd.product,vd.name,estemp.bid);
	    }
	}
    }

    if (plist=='N') {
/* commented out on 4/22/97, and changed to only print Total line 
	fprintf(printer, "\n\n%sFor the sum of :                          Total Merchandise : $%9.2f", m, es->cogs+es->mkup);
	fprintf(printer, "\n%s                                          Total Shipping    : $%9.2f", m, es->estship);
#ifndef REP_VER        
	fprintf(printer, "\n%s                                          Sales TAX         : $%9.2f", m, es->esttax);
#endif        
	fprintf(printer, "\n\n%s                                          Total             : $%9.2f", m, es->bid);
*/
	fprintf(printer, "\n\n\n%s                                  For the total lump sum of : $%9.2f", m, es->bid);
    } else {
      if(ups == 'Y' && tx == 'Y' && fg == 'Y')
      {
	fprintf(printer, "\n\n%sFor the total lump sum of : %8.2f",m, es->bid);
      }
      else
      {
	fprintf(printer, "\n\n%sFor the sum of :",m);
	totalMerch = es->cogs+es->mkup;
	if(tx == 'Y') totalMerch += es->esttax;
	if(fg == 'Y') totalMerch += es->estship;
	fprintf(printer, "\n\n%s     Total Merchandise : $%8.2f", m, totalMerch);
#ifndef REP_VER        
	if(fg == 'N') fprintf(printer, "\n%s     Total Shipping    :  %8.2f", m, es->estship);
#else
	if(!flag) fprintf(printer, "\n%s     Estimated Shipping:  %8.2f", m, es->estship);
#endif
#ifndef REP_VER
	if(tx == 'N') fprintf(printer, "\n%s     Sales TAX         :  %8.2f", m, es->esttax);
#endif
	fprintf(printer, "\n\n%s     Total             : $%8.2f", m, es->bid);
      }
    }

    line += 9;
    if (line > 60) {
	line = 1;
	fputc(12, printer);
    }
    s_close(VD);
    return(0);
}


/****************************************************************************
jb_proj_mang_form() - Shows Project Management Form for all POs for current
			Job from Job screen
Added a new parameter to indicate that whether this function is the true
project management form (as it was before) or update the approval date - 9/19/99
*****************************************************************************/
INT jb_proj_mang_form(DB_TYPE type, IN_POS *c, JBREC *jb, BOOLEAN bForm)
{
    POREC po;
    ESREC es;
    CDREC client;
    VDREC vd;
    CHAR tdate[9], fname[60], tmpFile[60];
    FLOAT subt, tcogs, ttax, ttotal, tsubt, tfreight, tmkup;
    FLOAT tother, tprofit;
    FLOAT csubt, tccogs, tctax, tctotal, tcsubt, tcfreight, tcmkup;
    FLOAT tcother, tcprofit;
    INT vec, i, n, tmpHandle, count;
    PO_ALT *poRec, *tmpPo;
    FILE *fp;
    BOOLEAN hasChangeOrder = FALSE;

    type = type;
    c = c;

    /* Inits */
    tcogs = ttax = ttotal = tsubt = tfreight = tmkup = tother = tprofit = 0;
    tccogs = tctax = tctotal = tcsubt = tcfreight = tcmkup = tcother = tcprofit = 0;

    FileTempName(fname);

    if ((fp = fileopn(fname, "wt")) == NULL) {
	message(M_SCR|M_ERR|M_KEY, "Unable to open %s - hit any key", fname);
    } else {

	message(M_SCR, "Working. . .");

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
	    n = s_nrecs(PO);
	    for (i = 0; i < n; i++) {
		s_read(PO, &po, i);

		/*
		* write each match to the temp file  (including Change Order)
		*/
		if (!stricmp(jb->code, po.jbcode) || !stricmp(jb->code, po.jbcode2)) {

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

		  if(bForm)
		  {
		    StrDate(tdate, &today);

		    s_open(ES);
		    vec = s_getno(ES, 0, jb->code);
		    s_read(ES, &es, vec);
		    s_close(ES);

		    s_open(CD);
		    vec = s_getno(CD, 0, es.client_code);
		    s_read(CD, &client, vec);
		    s_close(CD);

		    /* not needed ? */
		    plfile(&es);

		    n = (80-strlen(sysvar.company)) / 2;
		    fprintf(fp, "\n\r\n\r%*s\n\r\n\r", n+strlen(sysvar.company), sysvar.company);
		    fprintf(fp, "               P r o j e c t   M a n a g e m e n t   F o r m\n\r\n\r");
		    fprintf(fp, " _____________________________________________________________________________ \n\r");
		    fprintf(fp, "|   Job No.: %-8s            Date: %-8s             SP: %-3s           |\n\r", jb->code, tdate, es.spcode);
		    fprintf(fp, "|_____________________________________________________________________________|\n\r");
		    fprintf(fp, "    Client: %-32s    Client's PO:%s\n\r", client.name, jb->po);
		    fprintf(fp, "    Contact: %-32s   Tel.: %s\n\r", es.estname, es.esttel);
		    fprintf(fp, "    Job Name: %-32s  Site Tel.: %s\n\r", jb->name, jb->stel);
		    fprintf(fp, "    Contact Jobsite: %-26s Due Date: %s\n\r", jb->pmgr, jb->sent);

		    /* table for regular PO */
		    fprintf(fp, " _____________________________________________________________________________ \n\r");
		    fprintf(fp, "| PO No. | Order# | Vendor |  COGS  | Markup |Sub-Tot | Tax  | Ship |  Total  |\n\r");

		    tmpPo = poRec;
		    while (tmpPo != NULL) {
			if (!stricmp(tmpPo->po.jbcode2, jb->code)) 
			    hasChangeOrder = TRUE;

			if (stricmp(tmpPo->po.jbcode2, jb->code)) {
			    subt = tmpPo->po.cogs + tmpPo->po.mkup;
			    fprintf(fp, "|________|________|________|________|________|________|______|______|_________|\n\r");
			    fprintf(fp, "|%-8s|%-8s|%-8s|%8.2f|%8.2f|%8.2f|%6.2f|%6.2f|%9.2f|\n\r", tmpPo->po.po_number, tmpPo->po.inv_number, tmpPo->po.vdcode, tmpPo->po.cogs, tmpPo->po.mkup, subt, tmpPo->po.tax, tmpPo->po.estship, tmpPo->po.bid);
			    tcogs += tmpPo->po.cogs;
			    tmkup += tmpPo->po.mkup;
			    tsubt += subt;
			    ttax += tmpPo->po.tax;
			    tfreight += tmpPo->po.estship;
			    ttotal += tmpPo->po.bid;
			    if(tmpPo->po.status[0] == 'S' || tmpPo->po.status[0] == 's')
				tother += (tmpPo->po.status[0] == 'S' ? tmpPo->po.actcogs - tmpPo->po.cogs:0);
			    tprofit += tmpPo->po.profit;
			}

			/*
			* goto next PO record, and delete the current one
			*/
			tmpPo = tmpPo->next;
		    }

		    fprintf(fp, "|________|________|________|________|________|________|______|______|_________|\n\r");
		    fprintf(fp, "| Total  |        |        |%8.2f|%8.2f|%8.2f|%6.2f|%6.2f|%9.2f|\n\r", tcogs, tmkup, tsubt, ttax, tfreight, ttotal);
		    fprintf(fp, "|________|________|________|________|________|________|______|______|_________|\n\r");
		    fprintf(fp, "Comments:       Additional Expense: %8.2f     Profit: %8.2f\n\r\n\r", tother, tprofit);

		    if(hasChangeOrder)
		    {
		    /* table for change order PO */
		    fprintf(fp, "Change Order:\n\r");
		    fprintf(fp, " _____________________________________________________________________________ \n\r");
		    fprintf(fp, "| PO No. | Order# | Vendor |  COGS  | Markup |Sub-Tot | Tax  | Ship |  Total  |\n\r");

		    tmpPo = poRec;
		    while (tmpPo != NULL) {

			if (!stricmp(tmpPo->po.jbcode2, jb->code)) {
			    csubt = tmpPo->po.cogs + tmpPo->po.mkup;
			    fprintf(fp, "|________|________|________|________|________|________|______|______|_________|\n\r");
			    fprintf(fp, "|%-8s|%-8s|%-8s|%8.2f|%8.2f|%8.2f|%6.2f|%6.2f|%9.2f|\n\r", tmpPo->po.po_number, tmpPo->po.inv_number, tmpPo->po.vdcode, tmpPo->po.cogs, tmpPo->po.mkup, csubt, tmpPo->po.tax, tmpPo->po.estship, tmpPo->po.bid);
			    tccogs += tmpPo->po.cogs;
			    tcmkup += tmpPo->po.mkup;
			    tcsubt += csubt;
			    tctax += tmpPo->po.tax;
			    tcfreight += tmpPo->po.estship;
			    tctotal += tmpPo->po.bid;
			    if(tmpPo->po.status[0] == 'S' || tmpPo->po.status[0] == 's')
				tcother += (tmpPo->po.status[0] == 'S' ? tmpPo->po.actcogs - tmpPo->po.cogs:0);
			    tcprofit += tmpPo->po.profit;
			}

			/*
			* goto next PO record, and delete the current one
			*/
			tmpPo = tmpPo->next;
		    }

		    fprintf(fp, "|________|________|________|________|________|________|______|______|_________|\n\r");
		    fprintf(fp, "| Total  |        |        |%8.2f|%8.2f|%8.2f|%6.2f|%6.2f|%9.2f|\n\r", tccogs, tcmkup, tcsubt, tctax, tcfreight, tctotal);
		    fprintf(fp, "|________|________|________|________|________|________|______|______|_________|\n\r");
		    fprintf(fp, "Comments:       Additional Expense: %8.2f     Profit: %8.2f\n\r\n\r", tcother, tcprofit);
		    }

		    /* table for other information */
		    fprintf(fp, " _____________________________________________________________________________ \n\r");
		    fprintf(fp, "| PO No. | PO Date|Sub Date|Apr Date|R.F.Date|S.S.Date|        Comment        |\n\r");

		    tmpPo = poRec;
		    while (tmpPo != NULL) {
			if (!stricmp(tmpPo->po.jbcode, jb->code) || !strcmp(tmpPo->po.jbcode2, jb->code)) {
			  // make sure the corresponding VD doesn't have INSTAL in product description field
			  vec = s_getno(VD, 0, tmpPo->po.vdcode);
			  if(vec != -1)
			  {
			    s_read(VD, &vd, vec);
			    if(strnicmp(vd.product, "INSTAL", 6))
			    {
			    fprintf(fp, "|________|________|________|________|________|________|_______________________|\n\r");
			    fprintf(fp, "|%-8s|%-8s|%-8s|%-8s|%-8s|%-8s|%-23s|\n\r", tmpPo->po.po_number, tmpPo->po.podate, tmpPo->po.sub_date, tmpPo->po.approval_date, tmpPo->po.release_date, tmpPo->po.shipdate, "");
			    }
			  }
			}
			poRec = tmpPo;
			tmpPo = tmpPo->next;
			s_free(poRec);
		    }

		    fprintf(fp, "|________|________|________|________|________|________|_______________________|\n\r");
//                    fprintf(fp, "Sub Date:      Submittal Date\n\r");
//                    fprintf(fp, "Apr Date:      Approval Date\n\r");
//                    fprintf(fp, "R.F.Date:      Release for Fabrication Date\n\r");
//                    fprintf(fp, "S.S.Date:      Scheduled Ship Date\n\r");
		  }
		  else /* Update the submital approval date for the selected PO, 9/15/99 */
		  {
		    tmpPo = poRec;
		    while (tmpPo != NULL)
		    {
			/* Only update if teh Sub date is not empty */
			if(strcmp(tmpPo->po.sub_date, "") && strncmp(tmpPo->po.sub_date, "        ", 8))
			{
			    StrDate(tmpPo->po.approval_date, &today);
			    vec = s_getno(PO, 0, tmpPo->po.po_number);
			    s_write(PO, &(tmpPo->po), vec, FALSE);
			}
			poRec = tmpPo;
			tmpPo = tmpPo->next;
			s_free(poRec);
		    }
		  }
		}
	    }

	    FileDelete(tmpFile);
	}

	filecls(fp);
	if(bForm)
	    TextEditor(fname, client.fax, client.email);
	FileDelete(fname);
    }

    return(0);
}


/*****************************************************************************
print_room_schedule() - prints room schedule in landscape mode
*****************************************************************************/
VOID print_room_schedule(const CHAR* jbcode)
{
    CHAR tdate[9], saveExt[4], fname[60];
    INT i, j, k, l, cur = 1, n = 0, row, col, numPage = 0, get;
    RS_ALT *rsList, *p, *savePtr;
    FILE *fp = defPrinter;
    JBREC jb;
    INT f1, f2, f3;
    INT subTotal;
    RSREC rs1, rs2, rs3;
    CHAR rm1[7][5], rm2[7][5], rma[7][5], rmb[7][5], rmx[7][5], rmy[7][5];

    strcpy(saveExt, db[RS].extension);
    do {
	sprintf(fname, "%s%s.%03d", db[RS].path, db[RS].fname, cur++);
	if(access(fname, 0x00) != 0)
	    break;
	numPage++;
    } while(1);

    sprintf(fname, "%s%s.%s", db[RS].path, db[RS].fname, saveExt);
    if ((get = message(M_SCR|M_KEY, "F1 for list of Vendors, or ENTER for All:")) == KEY_F1) 
	rsList = rs_chooser(fname, FALSE);
    else if (get != KEY_CR) 
	return;
    else
	rsList = rs_chooser(fname, TRUE);

    if((p = rsList) == NULL)
	return;

    while(p)
    {
	n++;
	p = p->next;
    }
    row = (n-1) / 15 + 1;
    col = (numPage-1) / 3 + 1; // 1 print sheet contains 3 RS pages and 15 venders

    if(!SelectPrinter() || printercheck() == -1)
    {
	p = rsList;
	while(p)
	{
	    rsList = rsList->next;
	    free(p);
	    p = rsList;
	}
	return;
    }

    fprintf(fp, "%s", prnCodes.cond);
    fprintf(fp, "%s", prnCodes.land);

    StrDate(tdate, &today);
    s_read(JB, &jb, s_getno(JB, 0, jbcode));

    savePtr = rsList;
    for(i = 1; i <= row; i++)
    {
	for(j = 1; j <= col; j++)
	{
	    // get room names
	    memset(&rm1, 0, 35);
	    memset(&rm2, 0, 35);
	    memset(&rma, 0, 35);
	    memset(&rmb, 0, 35);
	    memset(&rmx, 0, 35);
	    memset(&rmy, 0, 35);
	    for(k = 1; k <= 3; k++)
	    {
		sprintf(fname, "%s%s.%03d", db[RS].path, db[RS].fname, (j-1)*3 + k);
		if(access(fname, 0x00) != 0)
		    break;
		if(k == 1)
		{
		    f1 = FileOpen(fname);
		    FileSeek(f1, 2, SEEK_SET);
		    FileRead(f1, &rm1, 35);
		    FileRead(f1, &rm2, 35);
		    f2 = f3 = -1;
		}
		else if(k == 2)
		{
		    f2 = FileOpen(fname);
		    FileSeek(f2, 2, SEEK_SET);
		    FileRead(f2, &rma, 35);
		    FileRead(f2, &rmb, 35);
		    f3 = -1;
		}
		else
		{
		    f3 = FileOpen(fname);
		    FileSeek(f3, 2, SEEK_SET);
		    FileRead(f3, &rmx, 35);
		    FileRead(f3, &rmy, 35);
		}
	    }

	    // print header
	    if(i != 1 || j != 1) // change page
		fputc(12, fp);
	    fprintf(fp, "\n\r");
	    fprintf(fp, "Date: %s\t\t\t\t\t\t\t\tRoom Schedule\t\t\t\t\t\t\t\tPage %d,%d of %dx%d\n\r", tdate, i, j, row, col);
	    fprintf(fp, "\n\r");
	    fprintf(fp, "\t\t\t\t\t\t\t\t\t%s\n\r", jb.name);
	    fprintf(fp, "\t\t\t\t\t\t\t\t\t%s\n\r", jb.address);
	    fprintf(fp, "\t\t\t\t\t\t\t\t\t%s, %s %s\n\r", jb.city, jb.state, jb.zip);
	    fprintf(fp, "\n\r");

	    if(j == col) // print total column
	    {
		fprintf(fp, "----------------------------------------------------------------------------------------------------------------------------------------------------------------------\n\r");
		fprintf(fp, "|                    |                          | Room Names                                                                                             |     |     |\n\r");
		fprintf(fp, "|                    |                          |--------------------------------------------------------------------------------------------------------|     |     |\n\r");
		fprintf(fp, "|                    |                          |%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s| Sub |     |\n\r",
									      rm1[0], rm1[1], rm1[2], rm1[3], rm1[4], rm1[5], rm1[6], rma[0], rma[1], rma[2], rma[3], rma[4], rma[5], rma[6],
									      rmx[0], rmx[1], rmx[2], rmx[3], rmx[4], rmx[5], rmx[6]);
		fprintf(fp, "|      Product       |       Description        |%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|Total|Total|\n\r",
									      rm2[0], rm2[1], rm2[2], rm2[3], rm2[4], rm2[5], rm2[6], rmb[0], rmb[1], rmb[2], rmb[3], rmb[4], rmb[5], rmb[6],
									      rmy[0], rmy[1], rmy[2], rmy[3], rmy[4], rmy[5], rmy[6]);
		fprintf(fp, "----------------------------------------------------------------------------------------------------------------------------------------------------------------------\n\r");
	    }
	    else // no total column
	    {
		fprintf(fp, "----------------------------------------------------------------------------------------------------------------------------------------------------------------\n\r");
		fprintf(fp, "|                    |                          | Room Names                                                                                             |     |\n\r");
		fprintf(fp, "|                    |                          |--------------------------------------------------------------------------------------------------------|     |\n\r");
		fprintf(fp, "|                    |                          |%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s| Sub |\n\r",
									      rm1[0], rm1[1], rm1[2], rm1[3], rm1[4], rm1[5], rm1[6], rma[0], rma[1], rma[2], rma[3], rma[4], rma[5], rma[6],
									      rmx[0], rmx[1], rmx[2], rmx[3], rmx[4], rmx[5], rmx[6]);
		fprintf(fp, "|      Product       |       Description        |%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|Total|\n\r",
									      rm2[0], rm2[1], rm2[2], rm2[3], rm2[4], rm2[5], rm2[6], rmb[0], rmb[1], rmb[2], rmb[3], rmb[4], rmb[5], rmb[6],
									      rmy[0], rmy[1], rmy[2], rmy[3], rmy[4], rmy[5], rmy[6]);
	    fprintf(fp, "----------------------------------------------------------------------------------------------------------------------------------------------------------------\n\r");
	    }

	    p = savePtr;
	    n = 0;
	    while(p)
	    {
		// process;
		memset(&rs1, 0, sizeof(RSREC));
		memset(&rs2, 0, sizeof(RSREC));
		memset(&rs3, 0, sizeof(RSREC));
		for(k = 1; k <= 3; k++)
		{
		    if(k == 1 && f1 != -1)
		    {
			FileSeek(f1, ((LONG)p->idx*sizeof(RSREC))+sizeof(USHORT)+70, SEEK_SET);
			FileRead(f1, &rs1, sizeof(RSREC));
		    }
		    else if(k == 2 && f2 != -1)
		    {
			FileSeek(f2, ((LONG)p->idx*sizeof(RSREC))+sizeof(USHORT)+70, SEEK_SET);
			FileRead(f2, &rs2, sizeof(RSREC));
		    }
		    else if(f3 != -1)
		    {
			FileSeek(f3, ((LONG)p->idx*sizeof(RSREC))+sizeof(USHORT)+70, SEEK_SET);
			FileRead(f3, &rs3, sizeof(RSREC));
		    }
		}

		subTotal = 0;
		for(l = 0; l < 7; l++)
		    subTotal += rs1.qt[l] + rs2.qt[l] + rs3.qt[l];

		// do not print if there is no page defined
		if(f1 != -1 && f2 != -1 && f3 != -1)
		{
		    if(j == col) // print total column
			fprintf(fp, "|%-20.20s|%-26.26s|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%5d|%5d|\n\r",
			    p->rs.pdcode, p->rs.pddesc, rs1.qt[0], rs1.qt[1], rs1.qt[2], rs1.qt[3], rs1.qt[4], rs1.qt[5], rs1.qt[6],
							rs2.qt[0], rs2.qt[1], rs2.qt[2], rs2.qt[3], rs2.qt[4], rs2.qt[5], rs2.qt[6],
							rs3.qt[0], rs3.qt[1], rs3.qt[2], rs3.qt[3], rs3.qt[4], rs3.qt[5], rs3.qt[6],
							subTotal, rs3.total);
		    else
			fprintf(fp, "|%-20.20s|%-26.26s|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%5d|\n\r",
			    p->rs.pdcode, p->rs.pddesc, rs1.qt[0], rs1.qt[1], rs1.qt[2], rs1.qt[3], rs1.qt[4], rs1.qt[5], rs1.qt[6],
							rs2.qt[0], rs2.qt[1], rs2.qt[2], rs2.qt[3], rs2.qt[4], rs2.qt[5], rs2.qt[6],
							rs3.qt[0], rs3.qt[1], rs3.qt[2], rs3.qt[3], rs3.qt[4], rs3.qt[5], rs3.qt[6],
							subTotal);
		}
		else if(f1 != -1 && f2 != -1)
		{
		    if(j == col) // print total column
			fprintf(fp, "|%-20.20s|%-26.26s|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%5d|%5d|\n\r",
			    p->rs.pdcode, p->rs.pddesc, rs1.qt[0], rs1.qt[1], rs1.qt[2], rs1.qt[3], rs1.qt[4], rs1.qt[5], rs1.qt[6],
							rs2.qt[0], rs2.qt[1], rs2.qt[2], rs2.qt[3], rs2.qt[4], rs2.qt[5], rs2.qt[6],
							" ", " ", " ", " ", " ", " ", " ",
							subTotal, rs2.total);
		    else
			fprintf(fp, "|%-20.20s|%-26.26s|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%5d|\n\r",
			    p->rs.pdcode, p->rs.pddesc, rs1.qt[0], rs1.qt[1], rs1.qt[2], rs1.qt[3], rs1.qt[4], rs1.qt[5], rs1.qt[6],
							rs2.qt[0], rs2.qt[1], rs2.qt[2], rs2.qt[3], rs2.qt[4], rs2.qt[5], rs2.qt[6],
							" ", " ", " ", " ", " ", " ", " ",
							subTotal);
		}
		else if(f1 != -1) // must be
		{
		    if(j == col) // print total column
			fprintf(fp, "|%-20.20s|%-26.26s|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%5d|%5d|\n\r",
			    p->rs.pdcode, p->rs.pddesc, rs1.qt[0], rs1.qt[1], rs1.qt[2], rs1.qt[3], rs1.qt[4], rs1.qt[5], rs1.qt[6],
							" ", " ", " ", " ", " ", " ", " ",
							" ", " ", " ", " ", " ", " ", " ",
							subTotal, rs1.total);
		    else
			fprintf(fp, "|%-20.20s|%-26.26s|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%4s|%5d|\n\r",
			    p->rs.pdcode, p->rs.pddesc, rs1.qt[0], rs1.qt[1], rs1.qt[2], rs1.qt[3], rs1.qt[4], rs1.qt[5], rs1.qt[6],
							" ", " ", " ", " ", " ", " ", " ",
							" ", " ", " ", " ", " ", " ", " ",
							subTotal);
		}

		if(j == col) // print total column
		    fprintf(fp, "----------------------------------------------------------------------------------------------------------------------------------------------------------------------\n\r");
		else
		    fprintf(fp, "----------------------------------------------------------------------------------------------------------------------------------------------------------------\n\r");

		n++;
		p = p->next;
		if(n == 15)
		    break;
	    }
	    if(f1 != -1)
		FileClose(f1);
	    if(f2 != -1)
		FileClose(f2);
	    if(f3 != -1)
		FileClose(f3);
	} // for(j
	savePtr = p;
    } // for(i

    /* Turn off specifial mood */
    fprintf(fp, "%s", prnCodes.reset);

    p = rsList;
    while(p)
    {
	rsList = rsList->next;
	free(p);
	p = rsList;
    }
}

