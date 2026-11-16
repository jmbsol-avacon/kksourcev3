/* db1.c file, an extension of db.c file, because db.c is too big to compile */


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

/* local prototypes */
INT print_or_fax(VOID);


extern struct date today;
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


/* Functions below are used to be in DB.C */
/**************************************************************************
eslist - Outputs a Quote Request
**************************************************************************/
INT eslist(ESREC *esin)
{
    BOOLEAN send = NO;

    JBREC jb;
    VDREC vd, vdin;
    PLREC pl;
    SPREC sp;
    INT line, vec, i, n, idx = -1, bg, ed;
    CHAR fname[40], *m="     ";
    CHAR tgh, outname[33], outph[16], email[49];
    FILE *printer;

    CHAR ftemp[9], etemp[4], ptemp[40];
    INT k, num_vd, cnt = 0, handle;

    s_open(JB);
    s_read(JB, &jb, s_getno(JB, 0, esin->jbcode));

    s_open(SP);
    s_read(SP, &sp, s_getno(SP, 0, esin->spcode));

    if (openplfile(esin) == -1) {
	message(M_SCR|M_KEY, "No records in product list - hit any key");
	return(-1);
    }
    do {
	if ((idx=vd_chooser(vdin.code))==-1)
	    return(-1);
    } while (!strcmp(vdin.code, "*"));

    s_read(VD, &vdin, s_getno(VD, 0, vdin.code));

#ifndef REP_VER
    if (message(M_SCR|M_KEY, "Send to Competitor? (y/N)") == 'Y') {
	send = YES;

// only list these VDs with the same product description as the selected one
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

	    while ((vec=helpinpt(VD, 0, "Enter VD code, or F1 for List: ", &vd))==-4)
		;
	}
	else
	{
	    while ((vec=helpinpt(VD, 0, "Enter VD code, or F1 for List: ", &vd))==-4)
		;

	    // restore VD file
	    s_close(VD);
	    s_delfile(VD);

	    strcpy(db[VD].fname, ftemp);
	    strcpy(db[VD].extension, etemp);
	    strcpy(db[VD].path, ptemp);
	}

	if (vec==-1)
	    return(vec);

//        s_read(VD, &vd, vec);     // no need to read again, and vec is offset also

	strcpy(vdin.contact, vd.contact);
    }
#endif
    do {
#ifndef REP_VER
	if ((tgh=message(M_SCR|M_KEY, "Send to Vendor or Rep? (V/R)")) == 'V') {
#else            
	if((tgh = 'V') == 'V') {
#endif
	    if (send) {
		strcpy(outname, vd.name);
		strcpy(outph, vd.fax);
		strcpy(email, vd.email);
	    } else {
		strcpy(outname, vdin.name);
		strcpy(outph, vdin.fax);
		strcpy(email, vdin.email);
	    }
	} else if (tgh=='R') {
	    if (send) {
		strcpy(outname, vd.agent);
		strcpy(outph, vd.afax);
		strcpy(email, vd.aemail);
	    } else {
		strcpy(outname, vdin.agent);
		strcpy(outph, vdin.afax);
		strcpy(email, vdin.aemail);
	    }
	}

    } while (tgh!='R' && tgh!='V');

    /*sprintf(fname, "%sQREQTEMP.%03d", tempDir, random_n());*/
    /* provide a temporary file name */
    FileTempName(fname);

    if ((printer = fileopn(fname, "wt")) == NULL) {
	message(M_SCR|M_ERR|M_KEY, "Unable to Open %s - hit any key", fname);
	return(-1);
    }
    fprintf(printer, "\n\r\n\r\n\r\n\r");
    fprintf(printer, "\n\r%s%s %d, %d", m, month[today.da_mon], today.da_day, today.da_year);
    fprintf(printer, "\n\r%s%s", m, outname);
    fprintf(printer, "\n\r%sFax: %s", m, outph);
    fprintf(printer, "\n\r%s%s", m, vdin.contact);
    if (send) {
	fprintf(printer, "\n\r\n\r%sPlease quote us your equivalent to the following products by", m);
	fprintf(printer, "\n\r%s%s :",m,vdin.name);
    } else {
	fprintf(printer, "\n\r\n\r%sPlease quote us on the following quantities of the items", m);
	fprintf(printer, "\n\r%sdescribed :",m);
    }
    fprintf(printer, "\n\r\n\r%s  Quantity       Code                  Description",m);
    fprintf(printer, "\n\r%s  -------- ------------------- --------------------------------", m);
    line = 10;

    if(idx == -1)
	bg = ed = 0;
    else
	get_pl_pos(idx, vdin.code, &bg, &ed);

    n = s_nrecs(PL);
    for (i=0; i<n; i++) {
	s_read(PL,&pl,i);
	if (!strcheck(vdin.code,pl.vdcode) && i >= bg && i <= ed) {
	    fprintf(printer, "\n\r%s %9ld %-20s%s",m,(long)pl.qty,pl.pdcode,pl.description);

	    if (++line>60) {
		line=1;
		fprintf(printer, "%c",12);
	    }
	}
    }
    fprintf(printer, "\n\r\n\r%sPlease include estimated shipping and unit prices in your quotation.",m);
    fprintf(printer, "\n\r\n\r%sJob: %s",m,jb.name);
    fprintf(printer, "\n\r%s%s, %s %s",m,jb.city,jb.state,jb.zip);
    fprintf(printer, "\n\r\n\r\n\r%sBest Regards,\n\r\n\r\n\r%s%s\n\r%s%s\n\r",m,m,sp.name,m,sp.title);
    filecls(printer);
    TextEditor(fname, outph, email);
    FileDelete(fname);
    return(0);
}


/**************************************************************************
esbid - outputs a bid letter (F9 on ES input)
**************************************************************************/
INT esbid(ESREC *esin)
{
    ESREC es;
    JBREC jb;
    CDREC client;
    CHAR vdcode[9];
    SPREC sp;
    CHAR plist, ups, matl_type[50], fname[60], *m="     ", name[60];
    FILE *printer;
    INT get, i, len, idx;
    BOOLEAN inst;

    strcpy(matl_type, "building accessories");
    memcpy(&es, esin, sizeof(ESREC));

    s_open(JB);
    s_read(JB, &jb, s_getno(JB, 0, es.jbcode));

    s_open(CD);
    s_read(CD, &client, s_getno(CD, 0, es.client_code));

    s_open(SP);
    s_read(SP, &sp, s_getno(SP, 0, es.spcode));

    if (openplfile(&es) == -1) {
	message(M_SCR|M_KEY, "No records in product list - hit any key");
	return(-1);
    }
    strcpy(vdcode, "*");
    set_rate(&es, vdcode, TRUE); /* this sets vdlist[] global */
#ifndef REP_VER
    if ((get = message(M_SCR | M_KEY, "F1 for list of Vendors, or ENTER for All:")) == KEY_F1) 
	idx = set_rate(&es, vdcode, FALSE);
    else if (get != KEY_CR)
	return(-1);
#else
    if((idx = vd_chooser(vdcode)) == -1)
	return(-1);
#endif
    re_calc(&es, vdcode, TRUE, idx);

    /* provide a temporary file name */
    FileTempName(fname);

    if ((printer = fileopn(fname, "wt")) == NULL) {
	message(M_SCR|M_ERR|M_KEY, "Unable to open %s - hit any key", fname);
	return(-1);
    }
    inpt(1, 25, " What kind of product: ", matl_type, 39, CT_CHAR, NO);

    if ((plist = message(M_SCR|M_KEY, "Include part list in bid letter?  (Y/n)")) != 'N')
	ups = message(M_SCR|M_KEY, "Include Unit Price in bid letter? (Y/n)");

    inst = FALSE;
#ifndef REP_VER
    if (message(M_SCR|M_KEY, "Furnish and Install? (y/N)") == 'Y')
#else        
    if (message(M_SCR|M_KEY, "Full Freight allowed? (y/N)") == 'Y')
#endif
	inst = TRUE;
    
    strcpy(name, es.estname);
    len = strlen(name);
    for(i = 0; i < len; i++)
    {
	if(name[i] == ' ')
	{
	    name[i] = '\0';
	    break;
	}
    }

    fprintf(printer, "\n\r\n\r\n\r\n\r");
    fprintf(printer, "\n\r%s%s %d, %d", m, month[today.da_mon], today.da_day, today.da_year);
    fprintf(printer, "\n\r\n\r%s%s", m, es.estname);
    fprintf(printer, "\n\r%s%s", m, client.name);
    fprintf(printer, "\n\r%s%s", m, client.address);
    fprintf(printer, "\n\r%s%s, %s %s", m, client.city, client.state, client.zip);
    fprintf(printer, "\n\r%sPhone: %s", m, client.tel);
    fprintf(printer, "\n\r%sFax  : %s", m, client.fax);
    fprintf(printer, "\n\r\n\r%sRe: %s (Quote #: %s)", m, jb.name, jb.code);
    fprintf(printer, "\n\r%s    %s, %s", m, jb.city, jb.state);
    fprintf(printer, "\n\r%sAddenda: %s", m, *(jb.addendum) ? jb.addendum : "None");
    fprintf(printer, "\n\r\n\r%sDear %s:", m, name);
#ifndef REP_VER    
    fprintf(printer, "\n\r\n\r%sWe propose to furnish%s, FOB, Factory, FFA, Single Shipment, the", m, inst ? " and install" : "");
#else    
    fprintf(printer, "\n\r\n\r%sWe propose to furnish, FOB, %s, Single Shipment, the", m, inst ? "Job Site, FFA" : "Factory");
#endif
    fprintf(printer, "\n\r%s",m);
    fprintf(printer, (plist == 'N' ?    "%s for the following sections:" : "following %s :"), matl_type);

    if (part_list_print(printer, plist, ups, &es, vdcode, inst, idx) == -1) {
	filecls(printer);
	FileDelete(fname);
	return(-1);
    }

    fprintf(printer, "\n\r\n\r%sIf you have any questions, please do not hesitate to call us.", m);
    fprintf(printer, "\n\r\n\r\n\r%sSincerely,", m);
    fprintf(printer, "\n\r\n\r\n\r\n\r\n\r%s%s", m, sp.name);
    fprintf(printer, "\n\r%s%s", m, sp.title);
    fprintf(printer, "\n\r\n\r\n\r%sValid: 60 days\n\r", m);
    filecls(printer);

    /* Just edit file then print/fax/save from there */
    TextEditor(fname, client.fax, client.email);
    FileDelete(fname);
    return(0);
}


/**************************************************************************
esletters - Outputs a letter to TextEditor (F8 on ES input)
**************************************************************************/
INT esletters(DB_TYPE type, IN_POS *c, ESREC *es)
{                                                         
#ifndef REP_VER
    CHAR *list[]={"Quote Request", "Quotation Validity/Revision Request", "Bid Letter",
		  "Request for Change Order", "General Bidding", "Award", "Acknowledgement", "Other", NULL};
#else
    CHAR *list[]={"Quote Request", "Bid Letter", "Other", NULL};
#endif
    
    INT choice;

    if (s_getno(JB, 0, es->jbcode) == -1) {
	message(M_SCR|M_KEY, "Job is not defined - hit any key");
	return(-1);
    }

    if (s_getno(CD, 0, es->client_code) == -1) {
	message(M_SCR|M_KEY, "Client is not defined - hit any key");
	return(-1);
    }

    if (s_getno(SP, 0, es->spcode) == -1) {
	message(M_SCR | M_KEY, "Sales person is not defined - hit any key");
	return(-1);
    }

    if ((choice = window_chooser(30, 9, list)) == -1)
	return(-1);

    switch (choice) {

	case 0:
	    eslist(es);
	    break;

	case 1:
	    esvalidity(es);
	    break;

	case 2:
	    esbid(es);
	    break;
#ifndef REP_VER /* REP_VER is broken, need re-visit if need to build REP version 6/7/98 */
	case 3:
	    eschangeorder(es);
	    break;

	case 4:
	    es_general_bid(type, c, es);
	    break;

	case 5:
	    esaward(es);
	    break;

	case 6:
	    esack(es);
	    break;

	case 7:
	    letters(type, c, (CHAR *)es);
	    break;
#else
	case 2:
	    letters(type, c, (CHAR *)es);
	    break;
#endif
    }

    return(0);
}



INT es_general_bid(DB_TYPE type, IN_POS *c, ESREC *es)
{
    CHAR vdcode[9], matl_type[50], *m = "     ";
    CHAR tdate[9], now[9], bidlist[60], *fileNames, *faxNumbers;
    ESREC esTemp;
    JBREC jb;
    SPREC sp;
    VDREC vd;
    CDREC client;
    FILE *outFile, *fp;
    INT i, n, rec;
    INT j, n_clients, idx;
    UINT key;
    BOOLEAN *clients, print, inst, stufftosend;
    CHAR plist, ups;

    type = type;
    c = c;

    s_open(JB);
    s_read(JB, &jb, s_getno(JB, 0, es->jbcode));

    if (openplfile(es) == -1) {
	message(M_SCR|M_KEY, "No records in product list - hit any key");
    } else {

	/* make a local copy of this ES record */
	memcpy(&esTemp, es, sizeof(ESREC));

	clients = (BOOLEAN *)s_alloc(sizeof(BOOLEAN) * s_nrecs(CD));

	/*
	* pick clients to send general bid to
	*/
	s_open(CD);
	if (cd_chooser(clients) != -1) {

	    /*
	    * count how many clients are being used
	    */
	    n_clients = 0;
	    n = s_nrecs(CD);
	    for (i = 0; i < n; i++) {
		if (clients[i]) {
		    n_clients++;
		}
	    }
	    Assert(n_clients != 0);

	    if (n_clients > 400) {

		message(M_SCR, "You have chosen too many clients - hit any key");

	    } else {

		/*
		* Print or Fax these bids?  (-1 = ESC, 0 = print, 1 = fax)
		*/
		print = TRUE;
		if ((n = print_or_fax()) != -1) {
		    if (n == 1) {
			print = FALSE;
		    }

		    fileNames = (CHAR *)s_alloc(80 * n_clients);
		    faxNumbers = (CHAR *)s_alloc(15 * n_clients);

		    /* provide a temporary file name */
		    FileTempName(bidlist);

		    stufftosend = FALSE;
		    while (1) {

			/*
			* pick vendors - with markup
			*/
			strcpy(vdcode, "*");
			idx = set_rate(&esTemp, vdcode, FALSE);

			re_calc(&esTemp, vdcode, TRUE, idx);

			strcpy(matl_type, "building accessories");
			message(M_SCR, "What kind of product: ");
			inpt(23, 25, "", matl_type, 39, CT_CHAR, NO);

			if ((plist = message(M_SCR|M_KEY, "Include part list in bid letter?  (Y/n)")) != 'N') {
			    ups = message(M_SCR|M_KEY, "Include Unit Price in bid letter? (Y/n)");
			}

			inst = FALSE;
			if (message(M_SCR|M_KEY, "Furnish and Install? (y/N)") == 'Y')
			    inst = TRUE;

			message(M_SCR, "Working...");

			if (!stufftosend) {
			    if ((fp = fileopn(bidlist, "wt")) != NULL) {

				StrDate(tdate, &today);
				StrTimeStamp(now);
				s_open(SP);
				s_read(SP, &sp, s_getno(SP, 0, curuser.spcode));
				fprintf(fp, "\n\n\n\n\n\n\n\n\n\t%-8s\t\t\t\t\t%-8s\n", tdate, now);
				fprintf(fp, "\n\t%s has sent bids to the following Clients", sp.name);
				fprintf(fp, "\n\tRE: Job #%s: %s, for these Vendors:", jb.code, jb.name);
				s_open(VD);
				for (i=0; i<MAXVENDORS; i++) {
				    if (tagged[i]) {
					if ((rec = getno_mess(VD, 0, vdlist[i])) != -1) {
					    s_read(VD, &vd, rec);
					    fprintf(fp, "\n\t%s", vd.name);
					}
				    }
				}
				s_close(VD);
				fprintf(fp, "\n\n\tClient   Name                             Phone           Fax\n");
			    }
			}

			/*
			* for each of the clients picked, create the bid, and send it
			*/
			j = 0;
			n = s_nrecs(CD);
			for (i = 0; i < n; i++) {

			    if (clients[i]) {
				s_read(CD, &client, i);

				if (!stufftosend) {
				    /* add to the bid list */
				    fprintf(fp, "\n\t%-8s %-32s %-15s %s", client.code, client.name, client.tel, client.fax);

				    FileTempName(fileNames + 80 * j);
				    strcpy(faxNumbers + 15 * j, client.fax);
				}

				if ((outFile = fileopn(fileNames + 80 * j, "at")) == NULL) {
				    message(M_SCR|M_ERR|M_KEY, "Unable to open %s - hit any key", fileNames + 80 * j);

				} else {

				    /* print sysvar.company name 1st? */

				    fprintf(outFile, "\n\n\n\n");
				    fprintf(outFile, "\n%s%s %d, %d", m, month[today.da_mon], today.da_day, today.da_year);
				    fprintf(outFile, "\n\n%s%s", m, esTemp.estname);
				    fprintf(outFile, "\n%s%s", m, client.name);
				    fprintf(outFile, "\n%s%s", m, client.address);
				    fprintf(outFile, "\n%s%s, %s %s", m, client.city, client.state, client.zip);
				    fprintf(outFile, "\n%sPhone: %s", m, client.tel);
				    fprintf(outFile, "\n%sFax  : %s", m, client.fax);
				    fprintf(outFile, "\n\n%sRe: %s (Quote #: %s)", m, jb.name, jb.code);
				    fprintf(outFile, "\n\r%s    %s, %s", m, jb.city, jb.state);
				    fprintf(outFile, "\n%sAddenda: %s", m, *(jb.addendum) ? jb.addendum : "None");
				    fprintf(outFile, "\n\n%sAttn: Estimating Department", m);
				    fprintf(outFile, "\n\n%sWe propose to furnish%s, FOB, Factory, FFA, Single Shipment, the", m, inst ? " and install" : "");
				    fprintf(outFile, "\n%s", m);
				    fprintf(outFile, (plist == 'N' ? "%s for the following sections:" : "following %s :"), matl_type);

				    part_list_print(outFile, plist, ups, &esTemp, vdcode, FALSE, idx);

				    s_read(SP, &sp, s_getno(SP, 0, es->spcode));

				    fprintf(outFile, "\n\n%sIf you have any questions, please do not hesitate to call us.", m);
				    fprintf(outFile, "\n\n\n%sSincerely,", m);
				    fprintf(outFile, "\n\n\n\n\n%s%s", m, sp.name);
				    fprintf(outFile, "\n%s%s", m, sp.title);
				    fprintf(outFile, "\n\n\n%sValid: 60 days\n", m);
				    fputc(12, outFile);
				    filecls(outFile);
				}
				j++;
			    }
			}
			if (!stufftosend)
			    filecls(fp);

			stufftosend = TRUE;

			key = message(M_SCR | M_KEY, "Send more bids to these clients? (Y/n)");
			if (key == 'N' || key == KEY_ESC)
			    break;
		    }

		    if (stufftosend) {

			key = message(M_SCR|M_KEY, "Would you like to print a list of the clients you sent bids to? (Y/n)");
			if (key != 'N' && key != KEY_ESC) {

			    FilePrint(bidlist, defPrinter);
			}
			FileDelete(bidlist);

			/*
			* print or fax those bids
			*/
			for (i = 0; i < n_clients; i++) {
			    if (print) {
				FilePrint(fileNames + 80 * i, defPrinter);
			    } else {
				FaxSend(fileNames + 80 * i, faxNumbers + 15 * i);
			    }
			    FileDelete(fileNames + 80 * i);
			}
		    }
		    s_free(faxNumbers);
		    s_free(fileNames);
		}
	    }
	}
	
	s_close(SP);
	s_close(CD);
	s_free(clients);

    }
    return(0);
}

INT print_or_fax()
{
    CHAR *list[] = {"Print", "Fax", NULL};

    return(window_chooser(33, 9, list));
}


/**************************************************************************
esaward - Congrats letter to Client
**************************************************************************/
INT esaward(ESREC *es)
{
    JBREC jb;
    SPREC sp;
    CDREC client;
    CHAR plist, ups, vdcode[9];
    CHAR matl_type[37],fname[40],*m="     ", name[60];
    FILE *printer;
    INT get, i, len, idx;

    strcpy(matl_type, "building accessories");

    s_open(JB);
    s_read(JB, &jb, s_getno(JB, 0, es->jbcode));

    s_open(CD);
    s_read(CD, &client, s_getno(CD, 0, es->client_code));

    s_open(SP);
    s_read(SP, &sp, s_getno(SP, 0, es->spcode));

    if (openplfile(es) == -1) {
	message(M_SCR|M_KEY, "No records in product list - hit any key");
	return(-1);
    }
    strcpy(vdcode, "*");
    set_rate(es, vdcode, TRUE); /* this sets vdlist[] global */
    if ((get = message(M_SCR|M_KEY, "F1 for list of Vendors, or ENTER for All:")) == KEY_F1) {
	idx = set_rate(es, vdcode, FALSE);
    } else if (get != KEY_CR) {
	return(-1);
    }
    re_calc(es,vdcode, TRUE, idx);

    /*sprintf(fname, "%sBIDTEMP.%03d", tempDir, random_n());*/
    /* provide a temporary file name */
    FileTempName(fname);

    if ((printer=fileopn(fname, "wt")) == NULL) {
	message(M_SCR|M_ERR|M_KEY, "Unable to open %s - hit any key", fname);
	return(-1);
    }
    message(M_SCR, NULL);
    inpt(1, 25, " What kind of product: ", matl_type, 36, CT_CHAR, NO);
    message(M_SCR, NULL);

    strcpy(name, jb.agent);
    len = strlen(name);
    for(i = 0; i < len; i++)
    {
	if(name[i] == ' ')
	{
	    name[i] = '\0';
	    break;
	}
    }

    fprintf(printer, "\n\r\n\r\n\r\n\r");
    fprintf(printer, "\n\r%s%s %d, %d", m, month[today.da_mon], today.da_day, today.da_year);
    fprintf(printer, "\n\r\n\r%s%s", m, jb.agent);
    fprintf(printer, "\n\r%s%s", m, client.name);
    fprintf(printer, "\n\r%s%s", m, client.address);
    fprintf(printer, "\n\r%s%s, %s %s", m, client.city, client.state, client.zip);
    fprintf(printer, "\n\r%sPhone: %s", m, client.tel);
    fprintf(printer, "\n\r%sFax  : %s", m, client.fax);
    fprintf(printer, "\n\r\n\r%sRe: %s (Quote #: %s)", m, jb.name, jb.code);
    fprintf(printer, "\n\r%s    %s, %s", m, jb.city, jb.state);
    fprintf(printer, "\n\r%sAddenda: %s", m, *(jb.addendum) ? jb.addendum : "None");
    fprintf(printer, "\n\r\n\r%sDear %s:", m, name);
    fprintf(printer, "\n\r\n\r%sCongratulations on being the awarded contractor of the above", m);
    fprintf(printer, "\n\r%sreferenced project.  Enclosed please find another copy of our proposals", m);
    fprintf(printer, "\n\r%sfor %s, for your review and consideration.", m, matl_type);

    /* Part List here */
    if ((plist = message(M_SCR|M_KEY, "Include part list in bid letter?  (Y/n)"))!='N')
	ups = message(M_SCR|M_KEY, "Include Unit Price in bid letter? (Y/n)");

    if (part_list_print(printer, plist, ups, es, vdcode, FALSE, idx) == -1) {
	filecls(printer);
	FileDelete(fname);
	return(-1);
    }

    fprintf(printer, "\n\r\n\r%sWe will be looking forward to the possible opportunity of working with",m);
    fprintf(printer, "\n\r%syou on this project.  In the interim, if we can be of any further",m);
    fprintf(printer, "\n\r%sassistance, please do not hesitate to call us. Thank you.",m);
    fprintf(printer, "\n\r\n\r%sCordially,",m);
    fprintf(printer, "\n\r%sFor %s", m, sysvar.company);
    fprintf(printer, "\n\r\n\r\n\r%s%s", m, sp.name);
    fprintf(printer, "\n\r%s%s", m, sp.title);
    fprintf(printer, "\n\r\n\r\n\r%sValid: 60 days\n\r", m);
    filecls(printer);

    /* Just edit file then print/fax/save from there */
    TextEditor(fname, client.fax, client.email);
    FileDelete(fname);
    return(0);
}

/**************************************************************************
esack - Acknowledgement letter to Client
**************************************************************************/
INT esack(ESREC *es)
{
    JBREC jb;
    CDREC client;
    SPREC sp;
    CHAR plist,ups,vdcode[9];
    CHAR matl_type[37],fname[40],*m="     ", name[60];
    FILE *printer;
    INT get, i, len, idx;

    strcpy(matl_type, "building accessories");

    s_open(JB);
    s_read(JB, &jb, s_getno(JB, 0, es->jbcode));

    s_open(CD);
    s_read(CD, &client, s_getno(CD, 0, es->client_code));

    s_open(SP);
    s_read(SP, &sp, s_getno(SP,0,es->spcode));

    if (openplfile(es)==-1) {
	message(M_SCR|M_KEY, "No records in product list - hit any key");
	return(-1);
    }
    strcpy(vdcode, "*");
    set_rate(es, vdcode, TRUE); /* this sets vdlist[] global */
    if ((get = message(M_SCR|M_KEY, "F1 for list of Vendors, or ENTER for All:")) == KEY_F1) {
	idx = set_rate(es, vdcode, FALSE);
    } else if (get != KEY_CR)
	return(-1);

    re_calc(es, vdcode, TRUE, idx);

    /* provide a tempoarary file name */
    /*sprintf(fname, "%sBIDTEMP.%03d", tempDir, random_n());*/
    FileTempName(fname);

    if ((printer = fileopn(fname, "wt")) == NULL) {
	message(M_SCR|M_ERR|M_KEY, "Unable to open %s - hit any key", fname);
	return(-1);
    }

    inpt(1, 25, " What kind of product: ", matl_type, 36, CT_CHAR, NO);
    message(M_SCR, NULL);

    strcpy(name, jb.pmgr);
    len = strlen(name);
    for(i = 0; i < len; i++)
    {
	if(name[i] == ' ')
	{
	    name[i] = '\0';
	    break;
	}
    }

    fprintf(printer, "\n\r\n\r\n\r\n\r");
    fprintf(printer, "\n\r%s%s %d, %d",m,month[today.da_mon],today.da_day,today.da_year);
    fprintf(printer, "\n\r\n\r%s%s",m,jb.pmgr);
    fprintf(printer, "\n\r%s%s",m,client.name);
    fprintf(printer, "\n\r%s%s",m,client.address);
    fprintf(printer, "\n\r%s%s, %s %s",m,client.city,client.state,client.zip);
    fprintf(printer, "\n\r%sPhone: %s",m,client.tel);
    fprintf(printer, "\n\r%sFax  : %s",m,client.fax);
    fprintf(printer, "\n\r\n\r%sRe: %s (Quote #: %s)",m,jb.name,jb.code);
    fprintf(printer, "\n\r%s    %s, %s",m,jb.city,jb.state);
    fprintf(printer, "\n\r%s    Purchase Order Acknowledgement.",m);
    fprintf(printer, "\n\r\n\r%sDear %s:",m,name);
    fprintf(printer, "\n\r\n\r%sThank you for awarding us the %s for",m,matl_type);
    fprintf(printer, "\n\r%sthis project.  Architectural submittals will be forthcoming to your",m);
    fprintf(printer, "\n\r%sattention for the following:",m);

    /* Part List here */

    if ((plist = message(M_SCR|M_KEY, "Include part list in bid letter?  (Y/n)"))!='N')
	ups = message(M_SCR|M_KEY, "Include Unit Price in bid letter? (Y/n)");

    if (part_list_print(printer,plist,ups,es,vdcode,FALSE,idx)==-1) {
	filecls(printer);
	FileDelete(fname);
	return(-1);
    }

    fprintf(printer, "\n\r\n\r%sIn the interim, if we can be of any further assistance, please do", m);
    fprintf(printer, "\n\r%snot hesitate to call us. Thank you.", m);
    fprintf(printer, "\n\r\n\r%sCordially,", m);
    fprintf(printer, "\n\r%sFor %s", m, sysvar.company);
    fprintf(printer, "\n\r\n\r\n\r%s%s", m, sp.name);
    fprintf(printer, "\n\r%s%s", m, sp.title);
    fprintf(printer, "\n\r\n\r\n\r%scc: File %s\n\r", m, es->jbcode);
    filecls(printer);

    /* Just edit file then print/fax/save from there */
    TextEditor(fname, client.fax, client.email);
    FileDelete(fname);
    return(0);
}

/*****************************************************************************
do_invoice() - Sends an Invoice or Purchase Order from PO to Text_Editor
*****************************************************************************/
INT do_invoice(POREC *po, BOOLEAN invflg)
{
    VDREC vd;
    CHAR fname[70], extra[70];
    INT vec;

    /* provide a temporary file name */
    FileTempName(fname);

    strcpy(extra, "Please submit 8 sets of submittals for approval ASAP.");
    if (invflg)
	extra[0] = '\0';
    else
	inpt(1, 25, "", extra, 69, CT_CHAR, NO);

    po_createpo(po, fname, invflg, extra);

    if ((vec=s_getno(VD, 0, po->vdcode)) != -1) {
	s_read(VD, &vd, vec);

	TextEditor(fname, vd.fax, vd.email);
    }
    FileDelete(fname);

    return(0);
}


INT po_createpo(POREC *po, const CHAR *fname, BOOLEAN invflg, const CHAR *extra)
{
    FILE *fp;
    CDREC client;
    VDREC vd, vdin;
    ESREC es;
    PLREC pl;
    JBREC jb;
    SPREC sp;
    struct date tempdate;
    CHAR tempstring[6][41], sttoday[9], dt1[9], dt2[9];
    FLOAT total_price=0, total_shipping=0;
    INT i, vec, p=1, line, ans, total_qty=0;
    USHORT n;
    BOOLEAN unit, packList = TRUE;
    INT shipTo = 0; /* ship to job site, default */
    INT bg, ed;

    s_open(ES);
    if ((vec = s_getno(ES, 0, po->jbcode)) == -1) {
	message(M_SCR|M_KEY, "Estimate is not defined - hit any key");
	return(-1);
    }
    s_read(ES, &es, vec);
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
    s_close(JB);

    s_open(SP);
    if ((vec = s_getno(SP, 0, po->spcode)) == -1) {
	message(M_SCR|M_KEY, "Sales Person is not defined - hit any key");
	return(-1);
    }
    s_read(SP, &sp, vec);
    s_close(SP);

    if (po->vdcode[0] || !invflg) {
	if ((vec = s_getno(VD, 0, po->vdcode)) == -1) {
	    message(M_SCR|M_KEY, "Vendor is not defined - hit any key");
	    return(-1);
	}
	s_read(VD, &vdin, vec);
	memcpy(&vd, &vdin, sizeof(VDREC));
    } else {
	vdin.code[0] = 0;
    }

    if (openplfile(&es) == -1) {
	message(M_SCR|M_KEY, "There are no products in the Part List - hit any key");
	return(-1);
    }

    if ((fp = fileopn(fname, "wb")) == NULL) {
	message(M_SCR|M_ERR|M_KEY, "Unable to Open %s - hit any key", fname);
	return(-1);
    }

    /*
    * skip 10 spaces
    */
    fprintf(fp, "\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r");

    if (invflg) {
	/* need to ask whether it is a packing slip */
	ans = message(M_SCR|M_KEY, "Invoice or Packing slip (i/P) ?");
	if(ans == 'i' || ans == 'I')
	    packList = FALSE;

	sprintf(tempstring[0], "%s, %s %s", client.city, client.state, client.zip);
	if(!packList)
	    fprintf(fp, "                                    INVOICE\n\r\n\r");
	else
	    fprintf(fp, "                                  PACKING SLIP\n\r\n\r");
	fprintf(fp, "   %-32s       Ship to:\n\r", client.name);
	fprintf(fp, "   %-32s       %s\n\r","Accounts Payable", jb.name);
	fprintf(fp, "   %-32s       c/o: %s\n\r", client.address, client.name);
	fprintf(fp, "   %-32s       %s\n\r", tempstring[0], jb.address);
	fprintf(fp, "   %-32s       %s, %s %s\n\r","", jb.city, jb.state, jb.zip);
	fprintf(fp, "   %-32s       Attn: %s\n\r","", jb.pmgr);
    } else {
	ans = message(M_SCR|M_KEY, "Ship to the job site or office or warehouse (J/o/w) ?");
	if(ans == 'o' || ans == 'O')
	    shipTo = 1; /* ship to office */
	else if(ans == 'w' || ans == 'W')
	    shipTo = 2; /* ship to warehose */

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

	fprintf(fp, "                                 PURCHASE ORDER\n\r\n\r");
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
    tempdate = datecheck(po->shipdate);
    if (!tempdate.da_year) {
	tempdate = today;
    }
    StrDate(sttoday, &today);
    StrDate(dt1, &tempdate);
    StrDate(dt2, DateAdd(&tempdate, 30));

    fprintf(fp, "\n\r    ");
    for(i=0; i<72; fprintf(fp, "_"),i++)
	;
    if (invflg)
    {
      if(!packList)
	fprintf(fp, "\n\r   |Inv:%-8s Date:%-8s  Our Ref:%-8s/%-8s  Your Ref:%-8s|", po->inv_number, po->shipdate, po->jbcode, po->po_number, jb.po);
      else
	fprintf(fp, "\n\r   |No: %-8s Date:%-8s  Our Ref:%-8s/%-8s  Your Ref:%-8s|", po->inv_number, po->shipdate, po->jbcode, po->po_number, jb.po);
    }
    else
	fprintf(fp, "\n\r   |PO: %-8s  Date: %-8s  Ship Date: %-8s  Ref: %-8s   P. %-2d|", po->po_number, po->podate, po->shipdate, jb.code, p);

    fprintf(fp, "\n\r   |");
    for(i=0; i<72; fprintf(fp, "_"),i++)
	;
    fprintf(fp, "|");

    if(!invflg || (invflg && !packList))
	fprintf(fp, "\n\r   |Product Code         Product Description             Qty");
    else
	fprintf(fp, "\n\r   |Product Code         Product Description        Qty Ordered");

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

    if(!invflg || !packList)
    {
	if (unit)
	    fprintf(fp, " $/Unit  Price  |");
	else
	    fprintf(fp, "                |");
    }
    else
	fprintf(fp, "  Qty Shipped|");

    fprintf(fp, "\n\r   |");
    for(i=0; i<72; fprintf(fp, "_"),i++)
	;
    fprintf(fp, "|");

    line = 14;

    if (!invflg)
	es.mkuprate = es.taxrate = 0;

    if(po->plidx <= 0) /* old style PO */
    {
	bg = 0;
	ed = n - 1;
    }
    else
    {
	set_rate(&es, vdin.code, TRUE); /* this sets vdlist[] global */
	get_pl_pos(po->plidx, vdin.code, &bg, &ed);
    }

    for (i=0; i<n; i++) {
	s_read(PL, &pl, i);
	if (strcheck(vdin.code, pl.vdcode))
	    continue;

	if(i < bg || i > ed)
	    continue;

	if ((vec = s_getno(VD, 0, pl.vdcode)) == -1) {
	    message(M_SCR|M_KEY, "Undefined vendor in product list - hit any key");
	    filecls(fp);
	    return(-1);
	}
	total_price += pl.price*(1+es.mkuprate)*pl.qty;
	if (invflg)
	{
	    if(!packList)
		total_shipping += pl.f*pl.weight*pl.qty;
	    else
		total_qty += pl.qty;
	}
	s_read(VD, &vd, vec);
	fprintf(fp, "\n\r   |%-21s%-29s", pl.pdcode, pl.description);
	if (pl.qty) {
	    if(!invflg || !packList)
	    {
		fprintf(fp, "%5ld", (long)pl.qty);
		if (unit)
		    fprintf(fp, "%8.2f %8.2f|", pl.price*(1+es.mkuprate), pl.price*(1+es.mkuprate)*pl.qty);
		else
		    fprintf(fp, "                 |");
	    }
	    else
	    {
		fprintf(fp, "%5ld        %5ld", (long)pl.qty, (long)pl.qty);
		fprintf(fp, "    |");
	    }
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
    if (invflg) {
      if(!packList)
      {
	fprintf(fp, "\n\r   %53.53s Subtotal: $%8.2f\n\r", "", total_price);
	fprintf(fp, "   %53.53s Shipping:  %8.2f\n\r", "", total_shipping);
	fprintf(fp, "   %53.53s      Tax:  %8.2f\n\r  ", "", total_price*es.taxrate);
	for(i=0; i<74; fprintf(fp, "_"),i++)
	    ;
      }
    }

    if(invflg && packList)
	fprintf(fp, "\n\r\n\r                                       No of Pieces:  %5d        %5d\n\r", total_qty, total_qty);
    else
	fprintf(fp, "\n\r   %53.53s    Total: $%8.2f\n\r", "", total_price*(1+es.taxrate)+total_shipping);
    if (!invflg) fprintf(fp, "\n\r   ************************************************************************");
    fprintf(fp, "\n\r   %s", extra);
    if (!invflg) fprintf(fp, "\n\r   ************************************************************************");
    if (!invflg)
    {
	fprintf(fp, "\n\r\n\r   *** SUBJECT TO REVIEW AND APPROVAL BY ARCHITECT ***");
	fprintf(fp, "\n\r\n\r   *** WILL CONFIRM THE REQUESTED SHIP DATE AFTER APPROVAL ***\n\r");
	fprintf(fp, "\n\r   Please contact %s if you have any questions", sp.name);
	fprintf(fp, "\n\r   regarding this PO.  Please refer to PO # and Reference # when invoicing.\n\r");
    }

    if(invflg && packList)
    {
	fprintf(fp, "\n\r\n\r\n\r   Received ____ pieces for %s in good condition.\n\r\n\r\n\r\n\r\n\r", client.name);
	fprintf(fp, "                                                         ____________\n\r");
	fprintf(fp, "                                                         %s\n\r\n\r\n\r", jb.pmgr);
	fprintf(fp, "   Damage Note: _________________________________________\n\r\n\r");
	fprintf(fp, "                _________________________________________\n\r\n\r");
    }

    filecls(fp);

    return(0);
}

/****************************************************************************
jb_po_form() - Shows PO Form for all POs for current Job from Job screen
*****************************************************************************/
INT jb_po_form(DB_TYPE type, IN_POS *c, JBREC *jb)
{
    POREC po;
    ESREC es;
    CDREC client;
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

		    StrDate(tdate, &today);

		    s_open(ES);
		    vec = s_getno(ES, 0, jb->code);
		    s_read(ES, &es, vec);
		    s_close(ES);

		    s_open(CD);
		    vec = s_getno(CD, 0, es.client_code);
		    s_read(CD, &client, vec);
		    s_close(CD);

		    n = (80-strlen(sysvar.company)) / 2;
		    fprintf(fp, "\n\r\n\r%*s\n\r", n+strlen(sysvar.company), sysvar.company);
		    fprintf(fp, " _____________________________________________________________________________ \n\r");
		    fprintf(fp, "|   Job No.: %-8s            Date: %-8s             SP: %-3s           |\n\r", jb->code, tdate, es.spcode);
		    fprintf(fp, "|_____________________________________________________________________________|\n\r");
		    fprintf(fp, "    Client: %-32s    Client's PO:%s\n\r", client.name, jb->po);
		    fprintf(fp, "    Contact: %-32s   Tel.: %s\n\r", es.estname, es.esttel);
		    fprintf(fp, "    Job Name: %-32s  Site Tel.: %s\n\r", jb->name, jb->stel);
		    fprintf(fp, "    Contact Jobsite: %-26s Due Date: %s\n\r", jb->pmgr, jb->sent);

		    /* not needed ? */
		    plfile(&es);

		    /* regular po table */
		    fprintf(fp, " _____________________________________________________________________________ \n\r");
		    fprintf(fp, "| PO No. | Inv. # | Vendor |  COGS  | Markup |Sub-Tot | Tax  | Ship |  Total  |\n\r");

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
			* goto next PO record
			*/
			tmpPo = tmpPo->next;
		    }

		    fprintf(fp, "|________|________|________|________|________|________|______|______|_________|\n\r");
		    fprintf(fp, "| Total  |        |        |%8.2f|%8.2f|%8.2f|%6.2f|%6.2f|%9.2f|\n\r", tcogs, tmkup, tsubt, ttax, tfreight, ttotal);
		    fprintf(fp, "|________|________|________|________|________|________|______|______|_________|\n\r");
		    fprintf(fp, "Comments:       Additional Expense: %8.2f     Profit: %8.2f\n\r\n\r", tother, tprofit);

		    if(hasChangeOrder)
		    {
		    /* change order po table */
		    fprintf(fp, "Change Order:\n\r");
		    fprintf(fp, " _____________________________________________________________________________ \n\r");
		    fprintf(fp, "| PO No. | Inv. # | Vendor |  COGS  | Markup |Sub-Tot | Tax  | Ship |  Total  |\n\r");

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
		    fprintf(fp, " __________________________________________________________________________ \n\r");
		    fprintf(fp, "| PO No. | VD Invoice #|  Date  |Sub-Total| Inv. No | Bill No.    |Shipping|\n\r");

		    tmpPo = poRec;
		    while (tmpPo != NULL) {
			if (!stricmp(tmpPo->po.jbcode, jb->code) || !stricmp(tmpPo->po.jbcode2, jb->code)) {
			    fprintf(fp, "|________|_____________|________|_________|_________|_____________|________|\n\r");
			    fprintf(fp, "|%-8s|%-13s|%-8s|%9.2f| %-8s|%-13s|%8.2f|\n\r", tmpPo->po.po_number, tmpPo->po.vd_inv_number, "", tmpPo->po.cogs, tmpPo->po.inv_number, tmpPo->po.billno, tmpPo->po.shipping);
			}
			poRec = tmpPo;
			tmpPo = tmpPo->next;
			s_free(poRec);
		    }

		    fprintf(fp, "|________|_____________|________|_________|_________|_____________|________|\n\r");
		}
	    }

	    FileDelete(tmpFile);
	}

	filecls(fp);
	TextEditor(fname, client.fax, client.email);
	FileDelete(fname);
    }

    return(0);
}


/**************************************************************************
esvalidity - Outputs a Quote validity/revision request
**************************************************************************/
INT esvalidity(ESREC *esin)
{
    BOOLEAN send = NO;

    JBREC jb;
    VDREC vd, vdin;
    PLREC pl;
    SPREC sp;
    INT line, vec, i, n, idx = -1, bg, ed;
    CHAR fname[40], *m="     ";
    CHAR tgh, outname[33], outph[16], email[49];
    FILE *printer;

    CHAR ftemp[9], etemp[4], ptemp[40];
    INT k, num_vd, cnt = 0, handle, ans;

    s_open(JB);
    s_read(JB, &jb, s_getno(JB, 0, esin->jbcode));

    s_open(SP);
    s_read(SP, &sp, s_getno(SP, 0, esin->spcode));

    if (openplfile(esin) == -1) {
        message(M_SCR|M_KEY, "No records in product list - hit any key");
        return(-1);
    }
    do {
        if ((idx=vd_chooser(vdin.code))==-1)
            return(-1);
    } while (!strcmp(vdin.code, "*"));

    s_read(VD, &vdin, s_getno(VD, 0, vdin.code));


re: ans = message(M_SCR|M_KEY, "Validity, Revision or Best pricing? (V/R/B)");
    if(ans != 'V' && ans != 'R' && ans != 'B')
        goto re;


#ifndef REP_VER
    if(send) {
//    if (message(M_SCR|M_KEY, "Send to Competitor? (y/N)") == 'Y') {
//        send = YES;

// only list these VDs with the same product description as the selected one
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

            while ((vec=helpinpt(VD, 0, "Enter VD code, or F1 for List: ", &vd))==-4)
                ;
        }
        else
        {
            while ((vec=helpinpt(VD, 0, "Enter VD code, or F1 for List: ", &vd))==-4)
                ;

            // restore VD file
            s_close(VD);
            s_delfile(VD);

            strcpy(db[VD].fname, ftemp);
            strcpy(db[VD].extension, etemp);
            strcpy(db[VD].path, ptemp);
        }

        if (vec==-1)
            return(vec);

//        s_read(VD, &vd, vec);     // no need to read again, and vec is offset also

        strcpy(vdin.contact, vd.contact);
    }
#endif
    do {
#ifndef REP_VER
        if ((tgh=message(M_SCR|M_KEY, "Send to Vendor or Rep? (V/R)")) == 'V') {
#else            
        if((tgh = 'V') == 'V') {
#endif
            if (send) {
                strcpy(outname, vd.name);
                strcpy(outph, vd.fax);
                strcpy(email, vd.email);
            } else {
                strcpy(outname, vdin.name);
                strcpy(outph, vdin.fax);
                strcpy(email, vdin.email);
            }
        } else if (tgh=='R') {
            if (send) {
                strcpy(outname, vd.agent);
                strcpy(outph, vd.afax);
                strcpy(email, vd.aemail);
            } else {
                strcpy(outname, vdin.agent);
                strcpy(outph, vdin.afax);
                strcpy(email, vdin.aemail);
            }
        }

    } while (tgh!='R' && tgh!='V');

    /*sprintf(fname, "%sQREQTEMP.%03d", tempDir, random_n());*/
    /* provide a temporary file name */
    FileTempName(fname);

    if ((printer = fileopn(fname, "wt")) == NULL) {
        message(M_SCR|M_ERR|M_KEY, "Unable to Open %s - hit any key", fname);
        return(-1);
    }
    fprintf(printer, "\n\r\n\r\n\r\n\r");
    fprintf(printer, "\n\r%s%s %d, %d", m, month[today.da_mon], today.da_day, today.da_year);
    fprintf(printer, "\n\r%s%s", m, outname);
    fprintf(printer, "\n\r%sFax: %s", m, outph);
    fprintf(printer, "\n\r%s%s", m, vdin.contact);
/*
    if (send) {
        fprintf(printer, "\n\r\n\r%sPlease quote us your equivalent to the following products by", m);
        fprintf(printer, "\n\r%s%s :",m,vdin.name);
    } else {
        fprintf(printer, "\n\r\n\r%sPlease quote us on the following quantities of the items", m);
        fprintf(printer, "\n\r%sdescribed :",m);
    }
*/
    if(ans == 'V')
    {
        fprintf(printer, "\n\r\n\r%sWe have been asked by our client to send an updated", m);
        fprintf(printer, "\n\r%squotation for this project. Please confirm the validity",m);
        fprintf(printer, "\n\r%sof your quote based on the following quantities and fax",m);
        fprintf(printer, "\n\r%sus your best possible price.",m);
    }
    else if(ans == 'R')
    {
        fprintf(printer, "\n\r\n\r%sWe have been asked by our client to send a revised quotation", m);
        fprintf(printer, "\n\r%sbased on our revised drawing/specification for this project.",m);
        fprintf(printer, "\n\r%sKindly revise your quote based on the following quantities",m);
        fprintf(printer, "\n\r%sand fax us your best possible price.",m);
    }
    else /* ans == 'B' */
    {
        fprintf(printer, "\n\r\n\r%sWe have been awarded this project. We had to go down on our", m);
        fprintf(printer, "\n\r%spricing to get the job and would appreciate if you could",m);
        fprintf(printer, "\n\r%sgive us a better discount. Kindly review your quote and send",m);
        fprintf(printer, "\n\r%sus your best possible price based on the following quantities :",m);
    }

    fprintf(printer, "\n\r\n\r%s  Quantity       Code                  Description",m);
    fprintf(printer, "\n\r%s  -------- ------------------- --------------------------------", m);
    line = 10;

    if(idx == -1)
        bg = ed = 0;
    else
        get_pl_pos(idx, vdin.code, &bg, &ed);

    n = s_nrecs(PL);
    for (i=0; i<n; i++) {
        s_read(PL,&pl,i);
        if (!strcheck(vdin.code,pl.vdcode) && i >= bg && i <= ed) {
            fprintf(printer, "\n\r%s %9ld %-20s%s",m,(long)pl.qty,pl.pdcode,pl.description);

            if (++line>60) {
                line=1;
                fprintf(printer, "%c",12);
            }
        }
    }
    fprintf(printer, "\n\r\n\r%sPlease include estimated shipping and unit prices in your quotation.",m);
    fprintf(printer, "\n\r\n\r%sJob: %s",m,jb.name);
    fprintf(printer, "\n\r%s%s, %s %s",m,jb.city,jb.state,jb.zip);
    fprintf(printer, "\n\r\n\r\n\r%sBest Regards,\n\r\n\r\n\r%s%s\n\r%s%s\n\r",m,m,sp.name,m,sp.title);
    filecls(printer);
    TextEditor(fname, outph, email);
    FileDelete(fname);
    return(0);
}

/**************************************************************************
eschangeorder - outputs a Request for Change Order (F9 on ES input)
**************************************************************************/
INT eschangeorder(ESREC *esin)
{
    ESREC es;
    JBREC jb;
    CDREC client;
    CHAR vdcode[9];
    SPREC sp;
    CHAR plist, ups, matl_type[50], fname[60], *m="     ", name[60];
    FILE *printer;
    INT get, i, len, idx;
    BOOLEAN inst;

    strcpy(matl_type, "building accessories");
    memcpy(&es, esin, sizeof(ESREC));

    s_open(JB);
    s_read(JB, &jb, s_getno(JB, 0, es.jbcode));

    s_open(CD);
    s_read(CD, &client, s_getno(CD, 0, es.client_code));

    s_open(SP);
    s_read(SP, &sp, s_getno(SP, 0, es.spcode));

    if (openplfile(&es) == -1) {
        message(M_SCR|M_KEY, "No records in product list - hit any key");
        return(-1);
    }
    strcpy(vdcode, "*");
    set_rate(&es, vdcode, TRUE); /* this sets vdlist[] global */
#ifndef REP_VER
    if ((get = message(M_SCR | M_KEY, "F1 for list of Vendors, or ENTER for All:")) == KEY_F1) 
        idx = set_rate(&es, vdcode, FALSE);
    else if (get != KEY_CR)
        return(-1);
#else
    if((idx = vd_chooser(vdcode)) == -1)
        return(-1);
#endif
    re_calc(&es, vdcode, TRUE, idx);

    /* provide a temporary file name */
    FileTempName(fname);

    if ((printer = fileopn(fname, "wt")) == NULL) {
        message(M_SCR|M_ERR|M_KEY, "Unable to open %s - hit any key", fname);
        return(-1);
    }
    inpt(1, 25, " What kind of product: ", matl_type, 39, CT_CHAR, NO);

    if ((plist = message(M_SCR|M_KEY, "Include part list in bid letter?  (Y/n)")) != 'N')
        ups = message(M_SCR|M_KEY, "Include Unit Price in bid letter? (Y/n)");

    inst = FALSE;
#ifndef REP_VER
    if (message(M_SCR|M_KEY, "Furnish and Install? (y/N)") == 'Y')
#else        
    if (message(M_SCR|M_KEY, "Full Freight allowed? (y/N)") == 'Y')
#endif
        inst = TRUE;
    
    strcpy(name, es.estname);
    len = strlen(name);
    for(i = 0; i < len; i++)
    {
        if(name[i] == ' ')
        {
            name[i] = '\0';
            break;
        }
    }

    fprintf(printer, "\n\r\n\r\n\r\n\r");
    fprintf(printer, "\n\r%s%s %d, %d", m, month[today.da_mon], today.da_day, today.da_year);
    fprintf(printer, "\n\r\n\r%s%s", m, es.estname);
    fprintf(printer, "\n\r%s%s", m, client.name);
    fprintf(printer, "\n\r%s%s", m, client.address);
    fprintf(printer, "\n\r%s%s, %s %s", m, client.city, client.state, client.zip);
    fprintf(printer, "\n\r%sPhone: %s", m, client.tel);
    fprintf(printer, "\n\r%sFax  : %s", m, client.fax);
    fprintf(printer, "\n\r\n\r%sRe: %s (Quote #: %s)", m, jb.name, jb.code);
    fprintf(printer, "\n\r%s    %s, %s", m, jb.city, jb.state);
    fprintf(printer, "\n\r%s    %s", m, "Change Order Request");
//    fprintf(printer, "\n\r%sAddenda: %s", m, *(jb.addendum) ? jb.addendum : "None");
    fprintf(printer, "\n\r\n\r%sDear %s:", m, name);
#ifndef REP_VER    
    fprintf(printer, "\n\r\n\r%sPlease find our price to furnish%s, FOB, Factory, FFA, the", m, inst ? " and install" : "");
    fprintf(printer, "\n\r%sfollowing additional material :", m);
#else    
    fprintf(printer, "\n\r\n\r%sWe propose to furnish, FOB, %s, Single Shipment, the", m, inst ? "Job Site, FFA" : "Factory");
#endif
    fprintf(printer, "\n\r%s",m);
//    fprintf(printer, (plist == 'N' ?    "%s for the following sections:" : "following %s :"), matl_type);

    if (part_list_print(printer, plist, ups, &es, vdcode, inst, idx) == -1) {
        filecls(printer);
        FileDelete(fname);
        return(-1);
    }

    fprintf(printer, "\n\r\n\r%sWe would greatly appreciate if you could process a change order,", m);
    fprintf(printer, "\n\r%sand send it to our office if you want us to process this order.", m);
    fprintf(printer, "\n\r%sIf you have any questions, please do not hesitate to call us.", m);
    fprintf(printer, "\n\r\n\r\n\r%sSincerely,", m);
    fprintf(printer, "\n\r\n\r\n\r\n\r\n\r%s%s", m, sp.name);
    fprintf(printer, "\n\r%s%s", m, sp.title);
    fprintf(printer, "\n\r\n\r\n\r%sValid: 60 days\n\r", m);
    filecls(printer);

    /* Just edit file then print/fax/save from there */
    TextEditor(fname, client.fax, client.email);
    FileDelete(fname);
    return(0);
}

