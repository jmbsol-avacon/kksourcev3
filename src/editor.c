/**
*   EDITOR.C    - Full Screen Text Editor
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
#include <dos.h>
#include <string.h>
#include <conio.h>
#include <process.h>
#include "db_defs.h"
#include "vmouse.h"
#include "vmisc.h"
#include "vfile.h"

#define BUF_SIZE 33000  //16334
#define LINE_LEN 79
#define MAX_LINES 23
#define KILL_BUF_SIZE 1*(LINE_LEN+1)

CHAR *txtBuf;
CHAR *curloc, *endloc, *bstart, *bend;
INT scrnx, scrny;
BOOLEAN insert;
CHAR killbuf[KILL_BUF_SIZE];

extern SYSREC sysvar;
extern FILE *defPrinter;
extern BOOLEAN mouseActive;
extern MOUSE_CLICK *points, mouseClicks[];

/*
* Local Function Prototypes
*/
VOID edit_clr_eol(INT, INT);
VOID printl(CHAR *);
VOID delete_char(VOID);
VOID search(CHAR *);
VOID kill_line(VOID);
VOID upline(VOID), downline(VOID), left(VOID), right(VOID);
VOID scrolldn(INT, INT), scrollup(INT, INT, INT, INT);
VOID display_scrn(INT, INT, CHAR *);
VOID pagedown(VOID), pageup(VOID);
VOID home(VOID), gotoend(VOID);
VOID endline(VOID);
VOID goxy(INT,INT);
VOID homeline(VOID);
INT  load(const CHAR *);
INT  loader(const CHAR *, CHAR *);
INT  printit(VOID);
INT  save_file(CHAR *,INT);
CHAR *strsub(CHAR *, CHAR *, CHAR *);
INT  strnchr(CHAR *, CHAR);
VOID c_home(VOID);
VOID c_end(VOID);
VOID center(VOID);
INT  nlength(VOID);
VOID insertchar(const CHAR,INT);
VOID clrxy(INT);
VOID ctrl_left(VOID);
VOID ctrl_left2(VOID);
VOID ctrl_right(VOID);
VOID enter(VOID);
VOID insertfile(const CHAR *);
INT  countem(VOID);


/*****************************************************************************
TextEditorEx() - Built in Word Processor - Wordstar Compatable Commands
	     - Can only edit files up to BUF_SIZE
*****************************************************************************/
INT TextEditorEx(const CHAR *fnb, const CHAR *ph)
{
    CHAR tempBuf[80], fname[80], phone[20], spec[80];
    INT choice, i, j, iEmailFunc = 0;
    MOUSE_CLICK *lastP;
    FILE *tmpPrinter;
    USHORT key;
    BOOLEAN changed, endflg;

    extern BOOLEAN form_screen_flag;

    /*
    * turn off time update
    */
    form_screen_flag = TRUE;

    /*
    * allocate some buffers
    */
    txtBuf = (CHAR *)s_alloc(BUF_SIZE);

    /* inits */
    endflg = changed = NO;
    insert = YES;

    fname[0] = '\0';
    if (fnb != NULL)
	strcpy(fname, fnb);

    /*
    * copy phone number to local buffer
    */
    phone[0] = '\0';
    if (ph != NULL)
	strcpy(phone, ph);

    scrnx = scrny = 0;
    memset(txtBuf, '\0', BUF_SIZE);

    if (!load(fname))
	curloc = endloc = txtBuf;

    fname[0] = '\0';

    store();
    textattr(NORMAL);
    printxy(1, 24, "F1=HELP F2=SAV F3=DEL F4=FAX  F5=PRINT F6=FIND F7=EMAIL         F9=LOAD F10=EXIT");
    textattr(CYAN|ON_BLACK);
    window(1, 1, 80, MAX_LINES);

    display_scrn(scrnx, scrny, curloc);
    goxy(scrnx, scrny);

    /* save last mouse clicks */
    lastP = points;

    do {
	/* register the mouse click points */
	points = mouseClicks;

	SetCursor(insert ? CUR_FUL : CUR_ON);
	message(M_SCR, NULL);

	/*
	* get a keystroke and take appropriate action
	*/
	switch (choice = nextkey()) {
#ifndef DEMO
	    case KEY_CTRL_S:        /* move left 1 char */
	    case KEY_LEFT:
		left();
		break;

	    case KEY_CTRL_D:        /* move right 1 char */
	    case KEY_RIGHT:
		right();
		break;

	    case KEY_CTRL_E:        /* move up 1 line */
	    case KEY_UP:
		upline();
		break;

	    case KEY_CTRL_X:
	    case KEY_DOWN:          /* move down 1 line */
		downline();
		break;

	    case KEY_CTRL_LEFT:     /* move left 1 word */
	    case KEY_CTRL_A:
		ctrl_left();
		break;

	    case KEY_CTRL_F:        /* move right 1 word */
	    case KEY_CTRL_RIGHT:
		ctrl_right();
		break;

	    case KEY_CTRL_PGUP:     /* Go to Beggining of document */
		home();
		break;

	    case KEY_CTRL_HOME:     /* Go to top of page */
		c_home();
		break;

	    case KEY_CTRL_PGDOWN:   /* Go to end of document */
		gotoend();
		break;

	    case KEY_CTRL_END:      /* Go to end of page */
		c_end();
		break;

	    case KEY_CTRL_R:        /* Page UP */
	    case KEY_PGUP:
		pageup();
		break;

	    case KEY_CTRL_C:        /* Page Down */
	    case KEY_PGDOWN:
		pagedown();
		break;

	    /*
	    * Go to Column 1 of current line
	    */
	    case KEY_HOME:
		homeline();
		break;

	    /*
	    * Go to end of line
	    */
	    case KEY_END:
		endline();
		break;
#endif
	    /*
	    * exit the editor
	    */
	    case KEY_ALT_X:
	    case KEY_ESC:
	    case KEY_F10:
		endflg = TRUE;
		break;

	    /*
	    * Help window
	    */
	    case KEY_CTRL_J:
	    case KEY_F1:
		helpmessage(HLP_TEXTEDIT);
		break;
#ifndef DEMO
	    /*
	    * Save current document
	    */
	    case KEY_F2:
		store();
		if (save_file(fname, 0) != -1)
		    changed = NO;
		restore();
		break;

	    /*
	    * Delete current line
	    */
	    case KEY_CTRL_Y:
	    case KEY_F3:
		homeline();
		kill_line();
		changed = YES;
		break;

	    /*
	    * FAX current document
	    */
	    case KEY_F4:

    		save_file(fnb, 1);  /* Save the buffer first */
            endflg = TRUE;      /* Let's get out of here, so EMail function will work */
            changed = FALSE;    /* Since we saved above already, we don't need to do again */
            iEmailFunc = 1;     /* This tells the caller function TextEditor() to send it as FAX */
			break;

		/* Code is not used anymore */
//                FileTempName(tempBuf);

//                save_file(tempBuf, 1);

//                window(1, 1, 80, 25);

		/*
		* Check to see whether fax command is valid 
		*/ 
//                if(sysvar.mailserver == NULL || !strcmp(sysvar.mailserver, ""))
//                {
//                    message(M_SCR|M_KEY, "FAX command is not defined in System Setup. Hit any key to continue."); 
//                    break;
//                }

//                message(M_SCR, "Enter FAX Number: ");
//                if (inpt(19, 25, "", (VOID *)phone, 19, CT_CHAR, NO) != KEY_F10) {
//                    message(M_SCR, NULL);
//                    FaxSend(tempBuf, phone);

		    /* delete that temporary file */
//                    FileDelete(tempBuf);
//                }
//                break;

	    case KEY_CTRL_P:         /* Print Working Copy */
	    case KEY_F5:
		tmpPrinter = defPrinter;
        if (SelectPrinter())
            printit();
		defPrinter = tmpPrinter;
        break;

	    case KEY_F6:             /* Search and replace */
		search(txtBuf);
		break;

	    case KEY_F7:		    /* E-mail function */
    		save_file(fnb, 1);  /* Save the buffer first */
            endflg = TRUE;      /* Let's get out of here, so EMail function will work */
            changed = FALSE;    /* Since we saved above already, we don't need to do again */
            iEmailFunc = 2;     /* This tells the caller function TextEditor() to send the email */
			break;

	    case KEY_F8:
		break;

	    /*
	    * Load a Text File
	    */
	    case KEY_F9:

		*tempBuf = '\0';
		if (changed && savecheck()) {
		    if (save_file(fname, 0) == -1)
			break;
		}

		window(1, 1, 80, 25);
		message(M_SCR, "Load File or F1 for list: ");
		key = inpt(27, 25, "", tempBuf, 50, CT_CHAR, NO);
		window(1, 1, 80, MAX_LINES);
		message(M_SCR, NULL);
		if (key != KEY_F10) {

		    if (key == KEY_F1) {

			sprintf(spec, "%s*.*", sysvar.editDir);
			if (loader(spec, tempBuf) == -1) {
			    break;
			}
		    }

		    strcpy(spec, tempBuf);
		    if (strlen(tempBuf) < 13) {
			sprintf(spec, "%s%s", sysvar.editDir, tempBuf);
		    }

		    if (load(spec)) {
			strcpy(fname, spec);
			scrnx = scrny = 0;
			clrscr();
			display_scrn(scrnx, scrny, curloc);
		    }
		    changed = NO;
		}
		break;

	    case KEY_CTRL_L:
		search(curloc);
		break;

	    /*
	    * Center current line
	    */
	    case KEY_CTRL_O:
		center();
		changed = YES;
		break;

	    /*
	    * Block & Save Menu
	    */
	    case KEY_CTRL_K:

		switch(message(M_SCR|M_KEY, "^K")) {

		    /*
		    * Mark Block start location
		    */
		    case 'B':
			bstart=curloc;
			break;

		    /*
		    * Save & exit
		    */
		    case 'D':
			endflg = TRUE;
			save_file(fname,1);
			changed = NO;
			break;

		    /*
		    * Run a DOS command
		    */
		    case 'F':

			*tempBuf = '\0';
			window(1, 1, 80, 25);
			message(M_SCR, "C>");
			if (inpt(3, 25, "", tempBuf, 40, CT_CHAR, NO) != KEY_F10) {
			    store();
			    clrscr();
			    system(tempBuf);
			    message(M_SCR|M_KEY, "Hit any key");
			    restore();
			}
			message(M_SCR, NULL);
			window(1, 1, 80, MAX_LINES);
			break;

		    /*
		    * Mark Block End
		    */
		    case 'K':
			bend = curloc;
			break;

		    /*
		    * Print working copy
		    */
		    case 'P':
			printit();
			break;

		    /*
		    * Quit - No Save
		    */
		    case 'Q':
			endflg = TRUE;
			changed = NO;
			break;

		    /*
		    * Read Block from a file
		    */
		    case 'R':
			*tempBuf = '\0';
			window(1, 1, 80, 25);
			message(M_SCR, "Enter Filename to Insert:");
			if (inpt(27, 25, "", tempBuf, 40, CT_CHAR, NO) == KEY_F10)
			    insertfile(tempBuf);
			changed = YES;
			break;

		    /*
		    * Save & Continue
		    */
		    case 'S':
			if (save_file(fname, 1) != -1)
			    changed = NO;
			break;

		    /*
		    * Write Block to a file
		    */
		    case 'W':
			break;

		    /*
		    * Save & Exit to DOS
		    */
		    case 'X':
			save_file(fname, 1);
			restore();

		    default:
			break;
		}
		break;
#endif
	    /*
	    * MISC Menu
	    */
	    case KEY_CTRL_Q:

		switch (message(M_SCR|M_KEY, "^Q")) {

		    /*
		    * Deletes all chars to front of line
		    */
		    case KEY_DEL:
		    {
			INT xtx = scrnx;

			homeline();
			while (xtx > 0) {
			    delete_char();
			    xtx--;
			}
		    } break;

		    /*
		    * Counts # of chars to cursor
		    */
		    case '?':
			message(M_SCR|M_KEY, "%d characters to this point. - hit any key", countem());
			break;

		    /*
		    * Search & replace
		    */
		    case 'A':
			search(txtBuf);
			break;

		    /*
		    * moves cursor to EOF
		    */
		    case 'C':
			while (*curloc != '\0')
			    right();
			break;

		    /*
		    * moves cursor to end of line
		    */
		    case 'D':
			endline();
			break;

		    /*
		    * Go to top of screen
		    */
		    case 'E':
			c_home();
			break;

		    /*
		    * Search & Replace
		    */
		    case 'F':
			search(txtBuf);
			break;

		    /*
		    * Go to beggining of document
		    */
		    case 'R':
			home();
			break;

		    /*
		    * Go to column 1 of current line
		    */
		    case 'S':
			homeline();
			break;

		    case 'W':
			message(M_SCR, "Hit any key");
			c_home();

			while (!KeyPress(NULL)) {
			    upline();
			    delay(500);
			}
			break;

		    /*
		    * moves cursor to last char on screen
		    */
		    case 'X':
			c_end();
			break;

		    /*
		    * Delete till End of Line
		    */
		    case 'Y':
			kill_line();
			changed=YES;
			break;

		    case 'Z':
			message(M_SCR, "Hit any key");
			c_end();

			while (!KeyPress(NULL)) {
			    downline();
			    delay(500);
			}
			break;

		    default:
			break;
		}
		break;

	    /*
	    * Scroll up 1 line
	    */
	    case KEY_CTRL_W:
		c_home();
		upline();
		break;

	    /*
	    * Scrolls down 1 line
	    */
	    case KEY_CTRL_Z:
		homeline();
		j = 23 - scrny;
		for (i=1; i<=j; i++)
		    downline();
		/*c_end();
		downline();*/
		break;

	    /*
	    * Toggle INSERT/OVERWRITE mode
	    */
	    case KEY_CTRL_V:
	    case KEY_INS:
		insert = (insert ? NO:YES);
		SetCursor(insert ? CUR_FUL : CUR_ON);
		break;

	    /*
	    * Delete 1 char on the right of cursor
	    */
	    case KEY_CTRL_G:
	    case KEY_DEL:
		if (curloc < endloc) {
		    CHAR *here;
		    INT x=scrnx, y=scrny;

		    here = curloc;
		    delete_char();
/* Don't really need this code. This causes the refresh if line is too long.
		    if (nlength() >= LINE_LEN) {
			homeline();

			for (i=1; i<=LINE_LEN; i++)
			    right();

			enter();
			left();
		    }
		    display_scrn(scrnx, scrny, curloc);
*/
		    curloc = here;
		    scrnx = x;
		    scrny = y;
		    changed = YES;
		}
		break;

	    /*
	    * Delete 1 word to the right
	    */
	    case KEY_CTRL_T:
		ctrl_right();
		ctrl_left();
		while (*(curloc+0) != ' ') {
		    delete_char();
		}

		delete_char();
		changed = YES;
		break;

	    /*
	    * Delete 1 word to the left
	    */
	    case KEY_CTRL_BS:
		if (curloc != txtBuf) {

		    while (*(curloc-1) == ' ')
			left();
		    while ((*(curloc-1) != ' ') && (*(curloc-1) != '\r') && (curloc != txtBuf)) {
			left();
			delete_char();
		    }
		    changed = YES;
		}
		break;

	    /*
	    * Delete 1 char on the left of the cursor
	    */
	    case KEY_CTRL_H:
	    case KEY_BACKSPACE:
		if (curloc != txtBuf) {
		    changed = YES;
		    left();
		    delete_char();
		}
		break;

	    /*
	    * TAB over 8 spaces
	    */
	    case KEY_CTRL_I:
	    case KEY_TAB:
		if (insert) {
		    if (endloc == txtBuf+BUF_SIZE-9)
			break;
		    memmove(curloc+8, curloc, endloc-curloc+8);
		    /* *(curloc+0) = *(curloc+1) = *(curloc+2) = *(curloc+3) = *(curloc+4) = *(curloc+5) = *(curloc+6) = *(curloc+7) = ' '; */
		    memset(curloc, ' ', 8);
		    /*puts("        ");*/
		    cprintf("        ");
		    scrnx += 8;
		    goxy(scrnx, scrny);
		    printl(curloc+8);
		    curloc += 8;
		    endloc += 8;
		    changed = YES;
		} else {

		    for (i=0; i<=7; i++)
			right();
		}
		break;

	    /*
	    * Insert a New Line
	    */
	    case KEY_CTRL_N:
	    case KEY_CTRL_M:
	    case KEY_ENTER:
		if (insert) {
		    enter();
		    changed = YES;
		} else {
		    downline();
		    homeline();
		}
		break;

	    /*
	    * Must check for these so no erroneous chars are inserted
	    */
	    case KEY_ALT_A:
	    case KEY_ALT_B:
	    case KEY_ALT_C:
	    case KEY_ALT_D:
	    case KEY_ALT_E:
	    case KEY_ALT_F:
	    case KEY_ALT_G:
	    case KEY_ALT_H:
	    case KEY_ALT_I:
	    case KEY_ALT_J:
	    case KEY_ALT_K:
	    case KEY_ALT_L:
	    case KEY_ALT_M:
	    case KEY_ALT_N:
	    case KEY_ALT_O:
	    case KEY_ALT_P:
	    case KEY_ALT_Q:
	    case KEY_ALT_R:
	    case KEY_ALT_S:
	    case KEY_ALT_T:
	    case KEY_ALT_U:
	    case KEY_ALT_V:
	    case KEY_ALT_W:
	    case KEY_ALT_Y:
	    case KEY_ALT_Z:
	    case KEY_CTRL_U:
	    case KEY_CTRL_B:
	    case KEY_ALT_F1:
	    case KEY_ALT_F2:
	    case KEY_ALT_F3:
	    case KEY_ALT_F4:
	    case KEY_ALT_F5:
	    case KEY_ALT_F6:
	    case KEY_ALT_F7:
	    case KEY_ALT_F8:
	    case KEY_ALT_F9:
	    case KEY_ALT_F10:
	    case KEY_SHFT_F1:
	    case KEY_SHFT_F2:
	    case KEY_SHFT_F3:
	    case KEY_SHFT_F4:
	    case KEY_SHFT_F5:
	    case KEY_SHFT_F6:
	    case KEY_SHFT_F7:
	    case KEY_SHFT_F8:
	    case KEY_SHFT_F9:
	    case KEY_SHFT_F10:
	    case KEY_CTRL_F2:
	    case KEY_CTRL_F3:
	    case KEY_CTRL_F4:
	    case KEY_CTRL_F5:
	    case KEY_CTRL_F6:
	    case KEY_CTRL_F7:
	    case KEY_CTRL_F8:
	    case KEY_CTRL_F9:
	    case KEY_CTRL_F10:
	    case KEY_CTRL_F11:
	    case KEY_CTRL_F12:
		beep();
		break;

	    /*
	    * Insert a Form Feed
	    */
	    case KEY_CTRL_ENTER:
		choice = 0x0c;

	    default:
		insertchar(choice, insert);
		changed = YES;
		break;
	}

	goxy(scrnx, scrny);
    } while (!endflg);

    if (changed && savecheck()) {
	while (save_file(fname, 0) == -2)
	    ;
    }

    /* restore last mouse clicks */
    points = lastP;

    restore();

    /* free the temporary buffers */
    s_free(txtBuf);

    /*
    * turn on time display
    */
    form_screen_flag = FALSE;

    return iEmailFunc;
}

INT save_file(CHAR *fname, INT flag)
{
    CHAR fileName[80];
    FILE *fl;
    CHAR *p;

    /*if (!stricmp(fname, "NONAME.TXT"))*/
    if (fname[0] == '\0')
	flag = 0;

    if (!flag) {
	window(1, 1, 80, 25);

	do {
	    message(M_SCR, "Enter Filename to Save:");
	    if (inpt(25, 25, "", fname, 40, CT_CHAR, NO) == KEY_F10) {
		return(-1);
	    }
	} while (fname[0] == '\0');
    }

    /*
    * save all files to users directory (unless they also specified a path)
    */
    strcpy(fileName, fname);
    if ((p = strchr(fname, '\\')) == NULL) {
	sprintf(fileName, "%s%s", sysvar.editDir, fname);
    }

    if ((fl = fileopn(fileName, "wb")) == NULL) {
	message(M_SCR | M_KEY, "Unable to open %s - hit any key", fileName);
	return(-2);
    }
    p = txtBuf;
    while (p != endloc) {
	if (*p != '\r') {
	    putc(*p, fl);
	} else {
	    putc('\r', fl);
	    putc('\n', fl);
	}
	p++;
    }
    filecls(fl);
    return(0);
}


INT printit(VOID)   /* Sends current file to Printer */
{
    FILE *fl;
    CHAR *p;

    if (printercheck() == -1)
	return(-1);

    fl = defPrinter;
    p = txtBuf;

    while (p != endloc) {
	if (*p != '\r')
	    putc(*p, fl);
	else {
	    putc('\r', fl);
	    putc('\n', fl);
	}
	p++;
    }

    /* Adds Page Break at end of document */
    putc(12, fl);
    fflush(fl);

    return(0);
}

/**************** Prints a line till CR ******************************/
VOID printl(CHAR *p)
{
    register INT i;

    i = scrnx;
    while ((*p != '\r') && *p && (i < LINE_LEN))  {
	Print("%c", *p);
	p++;
	i++;
    }
}


/***********************************************************************
kill_line() - Deletes a line
************************************************************************/
VOID kill_line(VOID)
{
    CHAR *p, *killbufptr;
    register INT i;

    if (*curloc == '\r') {
	delete_char();
    } else {
	edit_clr_eol(scrnx, scrny);
	p = curloc;
	i = 0;
	killbufptr = killbuf;
	while ((*p != '\r') && (p < endloc)) {
	    i++;
	    *killbufptr = *p;
	    p++;
	    if (killbufptr < killbuf+KILL_BUF_SIZE-2)
		killbufptr++;
	}
	*killbufptr='\0';
	memmove(curloc, curloc+i, endloc - curloc);
	endloc -= i;
	if (*(curloc+0) == '\r') {
	    delete_char();
	}
    }
}

VOID delete_char(VOID)
{
    if (curloc != '\0') {
	goxy(scrnx, scrny);
	if (*curloc == '\r') {
	    scrollup(0, scrny+1, LINE_LEN, MAX_LINES-1);
	    memmove(curloc, curloc+1, endloc-curloc);
	    endloc--;
	    display_scrn(scrnx, scrny, curloc);
	} else {
	    memmove(curloc, curloc+1, endloc - curloc);
	    endloc--;
	    printl(curloc);
	    cprintf(" ");
	}
    }
}

VOID left(VOID)
{
    if (curloc != txtBuf) {
	scrnx--;
	if (scrnx < 0) {
	    scrnx = 0;
	    upline();

	    while (*curloc != '\r') {
		curloc++;
		scrnx++;
	    }
	} else {
	    curloc--;
	}
    }
}

VOID right(VOID)
{
    if (curloc+1 <= endloc) {
	scrnx++;
	if (scrnx>LINE_LEN || *curloc=='\r') {
	    scrnx = 0;
	    scrny++;

	    if (scrny == MAX_LINES) {
		scrny = MAX_LINES - 1;
		downline();
		curloc--;
		while (*curloc != '\r')
		    curloc--;
		curloc++;
		scrnx = 0;
	    } else {
		curloc++;
	    }
	} else {
	    curloc++;
	}
    }
}

VOID search(CHAR *place)        /* Search And/or Replace */
{
    STATIC CHAR str1[33], str2[33];
    register CHAR *p;
    INT len, i;

    i = 0;
    if (place == txtBuf) {
	window(1, 1, 80, 25);
	message(M_SCR, "String to Find:");
	if (inpt(17, 25, "", str1, 32, CT_CHAR, NO) == KEY_F10) {
	    return;
	}
	message(M_SCR, "Replacement String or ENTER for None:");

	if (inpt(39, 25, "", str2, 32, CT_CHAR, NO) == KEY_F10) {
	    return;
	}
    }

    if (str2[0] != '\0') {
	len = message(M_SCR|M_KEY, "Automatically replace all occurances? (Y/n)");
	if ((len == KEY_F10) || (len == KEY_ESC))
	    return;
	if (len!='N') {
	    while ((p=strsub(place, str1, str2)) != NULL)
		i++;
	    right();
	    home();
	    message(M_SCR|M_KEY, "%d Replaced - hit any key", i);
	/*
	* Search & Replace one at a time
	*/
	} else {
	    message(M_SCR|M_KEY, "Not available");
	}
     /* Just Search, don't replace */
    } else {

	p = strstr(place, str1);
	if (p != NULL) {
	    curloc = p;
	    while (*(curloc+0)!='\r')
		curloc--;

	    curloc++;
	    display_scrn(0, 0, curloc);
	}
    }
}

VOID upline(VOID)
{
    register INT i;
    CHAR *p;

    if (curloc != txtBuf) {

	p=curloc;
	if (*p=='\r')
	    p--;
	for (;*p!='\r' && p>txtBuf;p--)
	    ;

	if (*p!='\r')
	    return;
	curloc = p;
	curloc--;
	i = scrnx;
	while (*(curloc+0)!='\r' && curloc>txtBuf)
	    curloc--;
	scrny--;
	scrnx=0;
	p=curloc;
	curloc++;
	if (scrny<0) {
	    scrolldn(0, 0);
	    scrny = 0;
	    goxy(0, 0);
	    printl(curloc);
	}
	while (i && *curloc!='\r') {
	    curloc++;
	    scrnx++;
	    i--;
	}
	if (p==txtBuf && *(p+0)!='\r')
	    curloc--;
    }
}

VOID downline(VOID)
{
    register INT i;
    CHAR *p;

    i = scrnx;
    p = curloc;
    while (*p!='\r' && p<endloc)
	p++;

    if (p != endloc) {

	p++;
	curloc = p;
	scrny++;
	scrnx = 0;
	if (scrny == MAX_LINES) {
	    scrny = MAX_LINES-1;
	    scrollup(0, 0, LINE_LEN, MAX_LINES-1);
	    goxy(scrnx, scrny);
	    printl(curloc);
	}

	while (i && *curloc!='\r' && curloc<endloc) {
	    curloc++;
	    scrnx++;
	    i--;
	}
    }
}

/*****************************************************************************
VOID display_scrn(INT,INT,CHAR *) - Main Screen Displayer
*****************************************************************************/
VOID display_scrn(INT x, INT y, CHAR *p)
{
    INT j=0, i=x;

    if (mouseActive)
	MouseHideCursor();

    SetCursor(CUR_OFF);
    gotoxy(x+1, y+1);
    if (y < MAX_LINES - 1) clrxy(y); /* For bug fix: del key wipes out the window */
    goxy(x, y);

    while (y<MAX_LINES && p[j]!='\0') {
	if (p[j] == '\r') {
	    i = 0;
	    if ((y != MAX_LINES-1) && (i < LINE_LEN)) {
		Print("\n");

		Print("\r");
	    }
	    y++;
	} else {
	    if (i >= LINE_LEN) {
		j++;
		continue;
	    }
	    Print("%c", p[j]);
	    i++;
	}
	j++;
    }
    SetCursor(insert ? CUR_FUL : CUR_ON);
    if (mouseActive)
	MouseShowCursor();
}


/*****************************************************************************
VOID clrxy(INT) - Clears current window from x,y down
*****************************************************************************/
VOID clrxy(INT y)
{
    clreol();
    window(1, y+2, LINE_LEN+1, MAX_LINES);
    clrscr();
    window(1, 1, 80, 23);
}


/*****************************************************************************
VOID pagedown(VOID) - Moves Down 1 page
*****************************************************************************/
VOID pagedown(VOID)
{
    register INT i;

    if (strnchr(curloc,'\r')<23) {
	while (*(curloc+0)!='\0')
	    right();
    } else {
	clrscr();

	for (i=0;i<MAX_LINES && curloc<endloc;) {
	    if (*(curloc+0)=='\r')
		i++;
	    curloc++;
	}
	scrnx = scrny = 0;
	display_scrn(scrnx, scrny, curloc);
    }
}

/*****************************************************************************
pageup() - Moves UP 1 page
*****************************************************************************/
VOID pageup(VOID)
{
    register INT i;

    if (curloc != txtBuf) {
	clrscr();
	if ((*curloc == '\r') && (curloc > txtBuf))
	    curloc--;

	for (i=0; i<MAX_LINES+1 && curloc>txtBuf;) {
	    if (*curloc == '\r')
		i++;
	    curloc--;
	}
	if (i == MAX_LINES+1)
	    curloc += 2;
	scrnx = scrny = 0;
	display_scrn(0, 0, curloc);
    }
}

/****************************************************************************
home() - Sends cursor to Begining of File
****************************************************************************/
VOID home(VOID)
{
    if (curloc != txtBuf) {
	clrscr();
	curloc = txtBuf;
	scrnx = scrny = 0;
	display_scrn(scrnx, scrny, curloc);
    }
}

/*****************************************************************************
c_end() - Sends cursor to last char on screen
*****************************************************************************/
VOID c_end(VOID)
{
    INT j;

    c_home();
    for(j=1;j<=22 && *(curloc+0)!='\0';) {
	right();
	if (*(curloc+0)=='\r')
	    j++;
    }
}


VOID gotoend(VOID)
{
    while (*(curloc+0)!='\0')
	curloc++;
    while (*(curloc+0)!='\r')
	curloc--;
    curloc++;
    scrnx = scrny = 0;
    display_scrn(0, 0, curloc);
    endline();
}

/****************************************************************************
endline() - Send cursor to end of current line
****************************************************************************/
VOID endline(VOID)
{
    while (*(curloc+0)!='\r'&& *(curloc+0)!='\0') {
	curloc++;
	scrnx++;
    }
}

VOID edit_clr_eol(INT x, INT y)
{
    CHAR *p;

    p = curloc;
    goxy(x, y);
    for (;x<LINE_LEN && *p!='\r' && *p; x++,p++) {
	cprintf(" ");
    }
}

VOID scrolldn(INT x, INT y)
{
    if (mouseActive)
	MouseHideCursor();

    KK_BiosScroll(0, 1, y, x, MAX_LINES-1, LINE_LEN, 7);   /* INT 10h/07h scroll down */

    if (mouseActive)
	MouseShowCursor();
}

VOID scrollup(INT topx, INT topy, INT endx, INT endy)
{
    if (mouseActive)
	MouseHideCursor();

    KK_BiosScroll(1, 1, topy, topx, endy, endx, 7);   /* INT 10h/06h scroll up */

    if (mouseActive)
	MouseShowCursor();
}


INT load(const CHAR *fname)
{
    FILE *fp;
    CHAR ch;
    INT i=0;

    if ((fp=fileopn(fname, "rb")) == NULL)
	return(0);

    if (FileLength(fname) > BUF_SIZE) {
	message(M_KEY|M_SCR, "%s is to big and cannot be loaded - hit any key", fname);
	return(0);
    }

    while (!feof(fp)) {
	ch = getc(fp);
	if (ch == '\r')
	    continue;
	if (ch == KEY_TAB2) {

	    txtBuf[i]=txtBuf[i+1]=txtBuf[i+2]=txtBuf[i+3]=txtBuf[i+4]=txtBuf[i+5]=txtBuf[i+6]=txtBuf[i+7]=' ';
	    i += 8;
	    continue;
	}
	if (ch=='\n') {
	    txtBuf[i]='\r';
	} else {
	    txtBuf[i]=ch;
	}
	i++;
    }
    txtBuf[i]='\0';
    endloc = &txtBuf[i];
    filecls(fp);
    curloc = &txtBuf[0];
    return(1);
}

/****************************************************************************
VOID goxy(INT,INT) - Same as gotoxy() except origin is 0,0 instead of 1,1
****************************************************************************/
VOID goxy(INT x, INT y)
{
    KK_BiosGotoXY(x, y);   /* INT 10h/02h set cursor position */
}

/*****************************************************************************
VOID homeline(VOID) - Sends curloc to first byte of current line
*****************************************************************************/
VOID homeline(VOID)
{
    curloc -= scrnx;
    scrnx = 0;
}

/****************************************************************************
c_home() - Sends cursor to 0,0
****************************************************************************/
VOID c_home(VOID)
{
    while (scrny != 0)
	left();

    homeline();
}

/*****************************************************************************
strsub() - Replaces a substring of a string
*****************************************************************************/
CHAR *strsub(CHAR *s, CHAR *pat, CHAR *rep)
{
    CHAR *ptr, *tptr;
    INT plen, rlen;

    tptr = ptr = s;
    plen = strlen(pat);
    rlen = strlen(rep);

    /*
    * find a match?
    */
    if ((ptr = strstr(s, pat)) == NULL) {
       return NULL;
    }

    /*
    * Buy some workspace
    */
    if ((tptr = (CHAR *)s_alloc(strlen(ptr))) == NULL)
	return NULL;

    strcpy(tptr, ptr + plen);                   /* If there was space left */

    while (rlen--) {                            /* Copy replacement    */
       *ptr++ = *rep++;
    }

    strcpy(ptr, tptr);                          /* Add old stuff back  */
    s_free(tptr);
    return(ptr - plen);                         /* Return pointer to change */
}



/***************************************************************************
strnchr() - Returns the Number of occurances of Char (c) in String (*p)
****************************************************************************/
INT strnchr(CHAR *p, CHAR c)
{
    INT i = 0;

    while (*p != '\0') {
	if (*p == c)
	    i++;
	p++;
    }

    return(i);
}

/*****************************************************************************
nlength() - Finds length of a line until '\r'
*****************************************************************************/
INT nlength(VOID)
{
    INT i, x, y;
    CHAR *p;

    /* Inits */
    i = 0;
    x = scrnx;
    y = scrny;
    p = curloc;

    homeline();
    while ((*curloc!='\r') && (*curloc!='\0')) {
	right();
	i++;
    }
    curloc = p;
    scrnx = x;
    scrny = y;

    return(i);
}

/*****************************************************************************
center() - Centers a line of text
*****************************************************************************/
VOID center(VOID)
{
    INT i,n;

    if (endloc != txtBuf+BUF_SIZE-2) {

	homeline();
	while (*curloc == ' ')
	    delete_char();

	/*n=(INT)floor((LINE_LEN+1-nlength())/2);*/
	n=(LINE_LEN+1-nlength())/2;
	homeline();

	goxy(scrnx, scrny);
	for (i=0; i<n; i++) {
	    insertchar(' ', YES);
	    goxy(scrnx, scrny);
	}
    }
}

/*****************************************************************************
insertchar() - Inserts or overwrites with choice into txtBuf
*****************************************************************************/
#if 1
VOID insertchar(const CHAR choice, INT insflag)
{
    if ((endloc < txtBuf+BUF_SIZE-2)) {

	if (insflag) {

	    if (nlength() >= LINE_LEN) {  
		enter();
		homeline();
		goxy(scrnx, scrny);
	    }

	    memmove(curloc + 1, curloc, endloc - curloc + 1);
	    *curloc = choice;
	    printl(curloc);

	    curloc++;
	    endloc++;
	    scrnx++;
	} else {
	    if (*curloc != '\r' && *curloc != '\0') {
		*curloc = choice;
		Print("%c", *curloc);
		curloc++;
		scrnx++;
	    }
	}
    } else {
	message(M_SCR|M_ERR|M_KEY, "Buffer is full, Can't add more text - hit any key");
    }
}
#else
VOID insertchar(const CHAR choice, INT insflag)
{
    CHAR *p, *pp;
    INT x, y, fix=FALSE;
    INT xx, yy;

    if ((endloc == txtBuf+BUF_SIZE-2) && insflag) {
	message(M_SCR|M_ERR|M_KEY, "Buffer is full, Can't add more text - hit any key");
    } else {
	if ((*curloc != '\0') || insflag) {

	    if ((nlength() >= LINE_LEN) && insflag) {
		p = curloc;
		x = scrnx;
		y = scrny;

		endline();
		ctrl_left2();
		if (p > curloc)
		    fix = TRUE;
		xx = scrnx;
		yy = scrny;
		pp = curloc;
		enter();

		/*if (fix) {
		    endline();
		    left();
		} else {*/
		    curloc = p;

		    scrnx = x;
		    scrny = y;
		    goxy(x, y);
		//}
	    }
	    if (scrnx == LINE_LEN) {
		right();
	    }

	    if (insflag)
		memmove(curloc + 1, curloc, endloc - curloc + 1);
	    *curloc = choice;

	    Print("%c", *curloc);

	    scrnx++;
	    goxy(scrnx, scrny);
	    if (insflag)
		printl(curloc+1);
	    curloc++;
	    if (insflag) {
		endloc++;
	    }

	    if (fix) {
		upline();
		downline();
		endline();
		display_scrn(xx, yy, pp);
	    }
	}
    }
}
#endif


/*****************************************************************************
VOID ctrl_left(VOID) - Move Left by 1 word
*****************************************************************************/
VOID ctrl_left(VOID)
{
    while (*curloc != ' ') {
	if (scrnx == 0) {
	    left();
	    break;
	}
	curloc--;
	scrnx--;
    }
    while ((*curloc == ' ') && (scrnx>=0)) {
	if (scrnx == 0) {
	    left();
	    break;
	}
	curloc--;
	scrnx--;
    }

    while ((*curloc != ' ') && (scrnx>=0)) {
	curloc--;
	scrnx--;
    }
    curloc++;
    scrnx++;
}

/*****************************************************************************
VOID ctrl_left2(VOID) - Move Left by 1 word
*****************************************************************************/
VOID ctrl_left2(VOID)
{
    while (*(curloc+0)!=' ' && scrnx>=0) {
	if (scrnx==0) {
	    left();
	    break;
	}
	curloc--;
	scrnx--;
    }
}

/*****************************************************************************
VOID ctrl_right(VOID) - Move Right by 1 word
*****************************************************************************/
VOID ctrl_right(VOID)
{
    while ((*curloc != ' ') && (scrnx <= LINE_LEN)) {
	if (*curloc == '\r') {
	    right();
	    break;
	}
	if (*curloc == '\0') {
	    curloc--;
	    scrnx--;
	    break;
	}
	curloc++;
	scrnx++;
    }

    while (*(curloc+0)==' ') {
	curloc++;
	scrnx++;
    }
}

/*****************************************************************************
VOID enter(VOID) - Inserts a Carridge Return
*****************************************************************************/
VOID enter(VOID)
{
    if (endloc != txtBuf+BUF_SIZE-2) {
	clreol();
	memmove(curloc+1, curloc, endloc-curloc+1);
	*curloc = KEY_ENTER;
	curloc++;
	edit_clr_eol(scrnx, scrny);
	scrnx=0;
	scrny++;
	if (scrny == MAX_LINES) {
	    scrny = MAX_LINES-1;
	    scrollup(0, 0, LINE_LEN, scrny);
	} else {
	    scrolldn(scrnx, scrny);
	}
	goxy(scrnx, scrny);
	printl(curloc);
	endloc++;
    }
}

/*****************************************************************************
VOID readfile(VOID) - For Future - Insert a file
*****************************************************************************/
VOID insertfile(const CHAR *fname)
{
    FILE *fp;
    CHAR ch;
    INT i;

    if ((fp=fileopn(fname, "rb")) == NULL) {
	message(M_SCR|M_ERR|M_KEY, "Unable to open %s - hit any key", fname);
    } else {
	while (!feof(fp)) {

	    if ((ch=getc(fp)) != '\r') {

		if (ch==KEY_TAB2) {
		    for (i=0; i<=7; i++)
			insertchar(' ', YES);
		} else if (ch == '\n') {
		    insertchar('\r', YES);
		} else {
		    insertchar(ch, YES);
		}
	    }
	}
    filecls(fp);
    }
}

INT countem(VOID)    /* Counts Chars to cursor */
{
    CHAR *p;
    INT i=0;

    p = txtBuf;
    while (p != curloc) {
	p++;
	i++;
    }
    return(i);
}


INT loader(const CHAR *searchSpec, CHAR *outFileName)
{
    STATIC MOUSE_CLICK lKeys[] = {
	{400, 80, 400, 80, KEY_UP}, {400, 152, 400, 152, KEY_DOWN},
	{232, 80, 384, 80, KEY_MOUSE_0}, {232, 88, 384, 88, KEY_MOUSE_1},
	{232, 96, 384,  96, KEY_MOUSE_2}, {232, 104, 384, 104, KEY_MOUSE_3},
	{232, 112, 384, 112, KEY_MOUSE_4}, {232, 120, 384, 120, KEY_MOUSE_5},
	{232, 128, 384, 128, KEY_MOUSE_6}, {232, 136, 384, 136, KEY_MOUSE_7},
	{232, 144, 384, 144, KEY_MOUSE_8}, {232, 152, 384, 152, KEY_MOUSE_9},
	{336, 160, 392, 160, KEY_F10}, {0, 0, 0, 0, 0}
    };

    struct find_t nextFile;
    CHAR tmpFile[60], tmpName[13];
    MOUSE_CLICK *lastP;
    INT fh, i, rec, currec, lastrec, retVal;
    USHORT n, key;
    BOOLEAN endflg;

    Assert(searchSpec != NULL);
    Assert(outFileName != NULL);

    /* Inits */
    n = 0;
    retVal = -1;
    *outFileName = '\0';

    /*
    * Open temp file to store list of files
    */
    if ((fh = FileOpen(FileTempName(tmpFile))) != -1) {

	if (!FileFindFirst(searchSpec, 0, &nextFile)) {
	    message(M_SCR|M_KEY, "Your user directory is empty - hit any key");
	} else {

	    do {
		FileWrite(fh, nextFile.name, 13);
		n++;

	    } while (FileFindNext(&nextFile));

	    store();
	    textattr(REVERSE);
	    SetCursor(CUR_OFF);
	    setwindow(30, 11, 50, 20, "Choose File to Load", searchSpec, "             F10=EXIT", NULL);

	    /* save last mouse click points */
	    lastP = points;

	    /* assign new mouse click points for this window */
	    points = lKeys;

	    key = 0;
	    rec = (n > 5 ? 5: 0);
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
			    FileSeek(fh, (LONG)i*13, SEEK_SET);
			    FileRead(fh, &tmpName, 13);
			} else {
			    tmpName[0] = '\0';
			}
			Print("\n\r%-12s", tmpName);
		    }
		}

		lastrec = rec;

		switch (key = nextkey()) {

		    /*
		    * move to the 1st record
		    */
		    case KEY_CTRL_PGUP:
		    case KEY_HOME:
		    case KEY_CTRL_HOME:
			rec = 0;
			break;

		    /*
		    * move to last record
		    */
		    case KEY_CTRL_PGDOWN:
		    case KEY_END:
		    case KEY_CTRL_END:
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
		    case KEY_F10:
			endflg = YES;
			break;

		    /*
		    * user has chosen a single PO
		    */
		    case KEY_ENTER:
			endflg = YES;
			break;

		    /*
		    * choose file with mouse
		    */
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

			/* detrmine which file user selected */
			key = key & 0x0FFF;
			rec = rec + (key - 5);

			/*
			* is choice invalid?
			*/
			if ((rec < 0) || (rec >= n)) {
			    rec = lastrec;
			    beep();
			} else {
			    endflg = TRUE;
			    key = KEY_ENTER;
			}
			break;

		    default:
			beep();
			break;
		}
	    } while (!endflg);

	    retVal = -1;
	    *outFileName = '\0';
	    if (key == KEY_ENTER) {
		retVal = 0;
		FileSeek(fh, (LONG)rec*13, SEEK_SET);
		FileRead(fh, outFileName, 13);
	    }

	    /* restore last mouse click points */
	    points = lastP;

	    SetCursor(CUR_ON);
	    restore();
	}

	FileClose(fh);
	FileDelete(tmpFile);
    }

    return(retVal);
}
