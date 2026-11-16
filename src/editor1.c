/**
*   EDITOR1.C    - Function only calls TextEditorEx() function, because
*                - when it returns, we need to check further whether
*                - we have to send the email. The email function used to
*                - resides in EDITOR.C file, but due to some memory limitation
*                - problem associated with 16-bit apps, I have to move it out.
*
*
*   Description - Created on 5/29/00, by Robert Bao
*
**/
#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <conio.h>
#include <process.h>
#include <sys/stat.h>
#include <io.h>
#include "db_defs.h"
#include "vmouse.h"
#include "vmisc.h"
#include "vfile.h"


extern SYSREC sysvar;
extern struct text_info orig;
extern BOOLEAN mouseActive;
extern BOOLEAN form_screen_flag;

/*
* Local Function Prototypes
*/
VOID Send_FaxOREmail(const CHAR *, const CHAR *, const CHAR *, INT iFunc);

/*****************************************************************************
Send_Email() - Sends e-mail by building a text file which has a special
	     header information and the body of the pass in file.
*****************************************************************************/
VOID Send_FaxOREmail(const CHAR *FileName, const CHAR *FaxNumber, const CHAR *ToAddr, INT iFunc)
{
	FILE *fin, *fout;
    CHAR SendToAddr[49], Subject[71], Phone[20], *ptr, ch, TempMailFileName[60], TempMailFileName1[9];
	CHAR Line[129];
	CHAR MailName[49], MailAddr[49];
	extern USREC curuser;
	SPREC logsp;
	INT i, len;
	static SHORT StartingFileNumber = 0;

    if((fin = fileopn(FileName, "rb")) == NULL)
	{
		message(M_SCR|M_ERR|M_KEY, "Can't open temporary file: %s, - hit any key", FileName);
		return;
	}

	/* Get the real starting file number */
	if(StartingFileNumber == 0) /* Need to generate the first good one */
	{
		while(1)
		{
			sprintf(TempMailFileName1, "%08d", ++StartingFileNumber);
			len = strlen(curuser.spcode);
			for(i = 0; i < len; i++)
				TempMailFileName1[i] = curuser.spcode[i];

			sprintf(TempMailFileName, ".\\FAX\\%s.dat", TempMailFileName1);
			if(access(TempMailFileName, 00) == 0)	/* File exists, try the next number */
				continue;

			break;
		}
	}
	else
	{
			sprintf(TempMailFileName1, "%08d", ++StartingFileNumber);
			len = strlen(curuser.spcode);
			for(i = 0; i < len; i++)
				TempMailFileName1[i] = curuser.spcode[i];

			sprintf(TempMailFileName, ".\\FAX\\%s.dat", TempMailFileName1);
	}
//message(M_SCR|M_KEY, "fname = %s", TempMailFileName);

	if((fout = fileopn(TempMailFileName, "wb")) == NULL)
	{
		message(M_SCR|M_ERR|M_KEY, "Unable to create file: %s - hit any key", TempMailFileName);
		return;
	}

	/* Search the file to find Re: sentence */
	Subject[0] = '\0';
	while(!feof(fin))
	{
		fgets(Line, 128, fin);
		if((ptr = strstr(Line, "Re: ")) != NULL)
		{
			strcpy(Subject, ptr);
			break;
		}
		else if((ptr = strstr(Line, "RE: ")) != NULL)
		{
			strcpy(Subject, ptr);
			break;
		}
	}
	fseek(fin, 0, SEEK_SET);
	/* Get rid of the trailing newline */
	if(strlen(Subject) > 0)
	{
		if(Subject[strlen(Subject) - 1] == '\n' || Subject[strlen(Subject) - 1] == '\r')
			Subject[strlen(Subject) - 1] = '\0';
		if(Subject[strlen(Subject) - 1] == '\n' || Subject[strlen(Subject) - 1] == '\r')
			Subject[strlen(Subject) - 1] = '\0';
	}

	window(1, 1, 80, 25);

	/* Ask SendTo address and subject, or FAX number */
	if(iFunc == 2)	/* Email function */
	{
		SendToAddr[0] = '\0';
		if(ToAddr != NULL)
			strcpy(SendToAddr, ToAddr);
		message(M_SCR, "SendTo address: ");
		if(inpt(17, 25, "", (VOID *)SendToAddr, 48, CT_CHAR, NO) == KEY_F10)
		{
			filecls(fin);
			filecls(fout);
//			FileDelete(TempFileName);
			return;
		}
		message(M_SCR, NULL);
		message(M_SCR, "Subject: ");
		if(inpt(10, 25, "", (VOID *)Subject, 69, CT_CHAR, NO) == KEY_F10)
		{
			filecls(fin);
			filecls(fout);
//			FileDelete(TempFileName);
			return;
		}
		message(M_SCR, NULL);

		/* Write all the information to the file */
		memset(&logsp, 0, sizeof(SPREC));
		s_read(SP, &logsp, s_getno(SP, 0, curuser.spcode));
		s_close(SP);
		ptr = strtok(logsp.email, "@");
		strcpy(MailName, ptr);
		ptr = strtok(NULL, "@");
		if(ptr)
			strcpy(MailAddr, ptr);
		fprintf(fout, "@F111 %s\\@%s@\n", MailName, MailAddr);
		fprintf(fout, "@F101 %s@\n@F106%s@\n", logsp.name, sysvar.company);

		ptr = strtok(SendToAddr, "@");
		strcpy(MailName, ptr);
		ptr = strtok(NULL, "@");
		if(ptr)
			strcpy(MailAddr, ptr);
		fprintf(fout, "@F212 %s\\@%s@\n", MailName, MailAddr);
		fprintf(fout, "@F307 %s@\n", Subject);
	}
	else if(iFunc == 1)	/* FAX function */
	{
		Phone[0] = '\0';
		if(FaxNumber != NULL)
			strcpy(Phone, FaxNumber);

		message(M_SCR, "FAX Number: ");
		if(inpt(19, 25, "", (VOID *)Phone, 19, CT_CHAR, NO) == KEY_F10)
		{
			filecls(fin);
			filecls(fout);
//			FileDelete(TempFileName);
			return;
		}
		message(M_SCR, NULL);
		fprintf(fout, "@F211 %s@\n", Phone);
	}

	/* Append fin contents to fout */
	ch = fgetc(fin);
	while(!feof(fin))
	{
		fputc(ch, fout);
		ch = fgetc(fin);
	} /* Skip the very last character */
	filecls(fin);
	filecls(fout);

/*
	switch (iRet)
	{
	case -1:
		message(M_SCR|M_KEY, "Message file is not specified, - hit any key");
		break;
	case -2:
		message(M_SCR|M_KEY, "Message file doesn't exist, or can't be opened, - hit any key");
		break;
	case -3:
		message(M_SCR|M_KEY, "Message file doesn't contain the correct special header info, - hit any key");
		break;
	case -4:
		message(M_SCR|M_KEY, "Message file doesn't contain the actual e-mail message, - hit any key");
		break;
	case -5:
		message(M_SCR|M_KEY, "winsock version error, - hit any key");
		break;
	case -6:
		message(M_SCR|M_KEY, "Can't connect to mail server: %s, - hit any key", sysvar.mailserver);
		break;
	case -7:
		message(M_SCR|M_KEY, "Mail server is not running on the specified address, - hit any key");
		break;
	case 0:		
		break;
	default:	
		message(M_SCR|M_KEY, "%s", sys_errlist[errno]);
	}
*/
//	FileDelete(TempFileName);
}


/*****************************************************************************
TextEditor() - Simply calls TextEditorEx() function, and do the possible EMail
*****************************************************************************/
VOID TextEditor(const CHAR *fnb, const CHAR *ph, const CHAR *email)
{
    INT iRet;

    iRet = TextEditorEx(fnb, ph);
    
    if(iRet)    /* Need to send email */
    {
        store();
        form_screen_flag = TRUE;    /* Turn off time update */
        if(mouseActive)
            MouseHideCursor();
        textattr(orig.attribute);
        clrscr();
        Print("When EMail function finishes, control will return back\n\r");
        Print("to the screen prior to the EDITOR screen.\n\r");
        SetCursor(CUR_ON);
        Send_FaxOREmail(fnb, ph, email, iRet);
        SetCursor(CUR_OFF);
        if(mouseActive)
            MouseShowCursor();
        form_screen_flag = FALSE;
        restore();

//        TextEditor(fnb, ph, email);
    }
}

