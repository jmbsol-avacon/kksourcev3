/**
*   KK_MORE.C   - Additional Functions for Kwik Kuote
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
#include <ctype.h>
#include "db_defs.h"
#include "vmisc.h"
#include "vfile.h"

extern struct date today;
extern FIELDPARAMS attribute;
extern CHAR vdlist[MAXVENDORS][9];
extern BOOLEAN tagged[MAXVENDORS];
extern INT switchflg;
extern ESREC *tempEsPtr;
extern BOOLEAN changeflg;
extern MOUSE_CLICK *points;

extern PRNCODES prnCodes;
extern FILE *defPrinter;

extern SYSREC sysvar;   /* for taxable freight. added on 4/1/97 */

/**************************************************************************
set_rate - Set Markup/Commission Rate for Each Vendor in Part List.
**************************************************************************/
INT set_rate(ESREC *es, CHAR *ptr, BOOLEAN only)
{
    STATIC MOUSE_CLICK pSetRate[MAXVENDORS + 7];
    MOUSE_CLICK *lastP;
    PLREC pl, pre_pl;
    FLOAT rate[MAXVENDORS];
    CHAR list[MAXVENDORS][11];
    INT choice=0, lastchoice=0, n, i, j=0, k, jj;
    BOOLEAN endflg, nextpage = FALSE;

    /* inits */
    endflg = FALSE;

    plfile(es);

    for (i=0; i<MAXVENDORS; i++) {
	tagged[i] = NO;
	list[i][0] = '\0';
	vdlist[i][0] = '\0';
	rate[i] = 0;
    }

    n = s_nrecs(PL);
    if(n > 0)
    {
        s_read(PL, &pre_pl, 0);
        strcpy(list[j], pre_pl.vdcode);
        strcpy(vdlist[j], pre_pl.vdcode);
        rate[j] = pre_pl.markup;
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
        strcpy(list[j], pl.vdcode);
        strcpy(vdlist[j], pl.vdcode);
        rate[j] = pl.markup;
	    j++;

        memcpy(&pre_pl, &pl, sizeof(PLREC));
    }

    if(only) /* only calling set_rate to set vdlist[] global */
        return 0;

    /*
    * If no vendors, then close file & exit
    */
    if (!j) {
	s_close(PL);
	return(-1);
    }
    store();
    endflg = FALSE;
    i = 0;

    for (i=0; i<j; i++) {
	sprintf(list[i], "%s: ", vdlist[i]);
    }

    jj = j;
    if(j > MAXVENDORS_PER_MARKUPWINDOW) /* can only hold 20 vendors in the markup window */
        j = MAXVENDORS_PER_MARKUPWINDOW;

#ifndef REP
/*    setwindow(25, 7, 55, j+7, " Enter Markup for Each Vendor", "", "F1=HELP     F7=TAG     F10=DONE", ""); */
    setwindow(25, 3, 55, j+3, " Enter Markup for Each Vendor", "PgUp                       PgDn", "F1=HELP     F7=TAG     F10=DONE", NULL);
#else
    setwindow(25, 7, 55, j+7, "   Enter Vendor's Commission",  "", "F1=HELP     F7=TAG     F10=DONE", "");
#endif

    /* save last mouse click points */
    lastP = points;

    /*
    * need to implement the mouse here
    */
    for (i=0; i<j; i++) {
        printxy(3, i+1, "%s: %.5f\n", vdlist[i], rate[i]);
	tagged[i] = NO;
        FillMouseClickPoint(&pSetRate[i], 192, 16+(i*8), 320, 16+(i*8), KEY_MOUSE_0 + i);
    }
    FillMouseClickPoint(&pSetRate[j], 192, 32 + ((j-1)*8), 240, 32 + ((j-1)*8), KEY_F1);
    FillMouseClickPoint(&pSetRate[j+1], 288, 32 + ((j-1)*8), 336, 32 + ((j-1)*8), KEY_F7);
    FillMouseClickPoint(&pSetRate[j+2], 376, 32 + ((j-1)*8), 432, 32 + ((j-1)*8), KEY_F10);
    FillMouseClickPoint(&pSetRate[j+3], 192, 8, 224, 8, KEY_PGUP);
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
        choice = inpt(3, choice+1, list[choice], &rate[choice], 8, CT_FLOAT3, NO);
                else
        choice = inpt(3, choice+1, list[choice+MAXVENDORS_PER_MARKUPWINDOW], &rate[choice+MAXVENDORS_PER_MARKUPWINDOW], 8, CT_FLOAT3, NO);

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
                if(!nextpage)
                {
                    tagged[lastchoice] = (tagged[lastchoice] ? NO : YES);
        
                    if (tagged[lastchoice]) {
                        printxy(1, lastchoice+1, "\x10");
                    } else {
                        printxy(1, lastchoice+1, " ");
                    }
                }
                else
                {
                    tagged[lastchoice+MAXVENDORS_PER_MARKUPWINDOW] = (tagged[lastchoice+MAXVENDORS_PER_MARKUPWINDOW] ? NO : YES);
        
                    if (tagged[lastchoice+MAXVENDORS_PER_MARKUPWINDOW]) {
                        printxy(1, lastchoice+1, "\x10");
                    } else {
                        printxy(1, lastchoice+1, " ");
                    }
                }
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
                    if(tagged[i])
                        printxy(1, i+1, "\x10");
                    else
                        printxy(1, i+1, " ");
                    printxy(3, i+1, "%s: %.5f\n", vdlist[i], rate[i]);
                    FillMouseClickPoint(&pSetRate[i], 192, 16+(i*8), 320, 16+(i*8), KEY_MOUSE_0 + i);
                }
                FillMouseClickPoint(&pSetRate[j], 192, 32 + ((j-1)*8), 240, 32 + ((j-1)*8), KEY_F1);
                FillMouseClickPoint(&pSetRate[j+1], 288, 32 + ((j-1)*8), 336, 32 + ((j-1)*8), KEY_F7);
                FillMouseClickPoint(&pSetRate[j+2], 376, 32 + ((j-1)*8), 432, 32 + ((j-1)*8), KEY_F10);
                FillMouseClickPoint(&pSetRate[j+3], 192, 8, 224, 8, KEY_PGUP);
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
                    if(tagged[i])
                        printxy(1, i+1-j, "\x10");
                    else
                        printxy(1, i+1-j, " ");
                    printxy(3, i+1-j, "%s: %.5f\n", vdlist[i], rate[i]);
                    FillMouseClickPoint(&pSetRate[i-j], 192, 16+((i-j)*8), 320, 16+((i-j)*8), KEY_MOUSE_0 + i-j);
                }
                FillMouseClickPoint(&pSetRate[i-j], 192, 32 + ((j-1)*8), 240, 32 + ((j-1)*8), KEY_F1);
                FillMouseClickPoint(&pSetRate[i-j+1], 288, 32 + ((j-1)*8), 336, 32 + ((j-1)*8), KEY_F7);
                FillMouseClickPoint(&pSetRate[i-j+2], 376, 32 + ((j-1)*8), 432, 32 + ((j-1)*8), KEY_F10);
                FillMouseClickPoint(&pSetRate[i-j+3], 192, 8, 224, 8, KEY_PGUP);
                FillMouseClickPoint(&pSetRate[i-j+4], 400, 8, 432, 8, KEY_PGDOWN);
                FillMouseClickPoint(&pSetRate[i-j+5], 0, 0, 0, 0, 0);
                j = jj - MAXVENDORS_PER_MARKUPWINDOW;
                break;

	    case KEY_F7:
                if(!nextpage)
                {
                    tagged[lastchoice] = (tagged[lastchoice] ? NO : YES);

                    if (tagged[lastchoice]) {
                        printxy(1, lastchoice+1, "\x10");
                    } else {
                        printxy(1, lastchoice+1, " ");
                    }
                }
                else
                {
                    tagged[lastchoice+MAXVENDORS_PER_MARKUPWINDOW] = (tagged[lastchoice+MAXVENDORS_PER_MARKUPWINDOW] ? NO : YES);

                    if (tagged[lastchoice+MAXVENDORS_PER_MARKUPWINDOW]) {
                        printxy(1, lastchoice+1, "\x10");
                    } else {
                        printxy(1, lastchoice+1, " ");
                    }
                }
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

    /* put the new rate back to PL file */
    i = 0;
    for(k = 0; k < j; k++)
    {
        endflg = FALSE;
        while(!endflg) /* keeping reading, until find the right group */
        {
            if(i < n)
                s_read(PL, &pl, i++);
            else
                break;
            if(!strcheck(vdlist[k], pl.vdcode))
                endflg = TRUE;
        }

        if(!endflg) /* didn't find the group, wrong */
            break;

        endflg = FALSE;
        while(!endflg)
        {
            if(pl.markup != rate[k])
            {
                pl.markup = rate[k];
                s_write(PL, &pl, i-1, FALSE);
            }

            if(i < n)
                s_read(PL, &pl, i++);
            else
                break;
            if(strcheck(vdlist[k], pl.vdcode))
                endflg = TRUE;
        }
        i--;
    }
    s_close(PL);
    strcpy(ptr, vdlist[lastchoice]);

    endflg = FALSE;
    for (i=0; i<MAXVENDORS && !endflg; i++) {
	if (tagged[i]) {
	    endflg = TRUE;
	    strcpy(ptr, "!");
	}
    }

    return (lastchoice); //return(0);
}

/**************************************************************************
re_calc - Calculates Estimate Info from Part List
    es:  Pointer to Estimate Record
    ptr: Pointer to String used as search spec of VD codes
    rd:  Boolean to indicate whether to do rounding
    idx: Index of VD picked. Only useful when one vdcode is passed in
**************************************************************************/
VOID re_calc(ESREC *es, CHAR *ptr, BOOLEAN rd, INT idx)
{
    INT n, i, j, k, num;
    SPREC sp;
    PLREC pl;
    VDREC vd;
    BOOLEAN tagd;
    FLOAT tmpBid, tmpMk, tmpTxb, markUp, qtyPrice;
        /* used for round up bid price, which will backword adjust markup rate,
            first edition on 5/4/97, second edition on 6/1/97 */
    LONG round;

    n = s_nrecs(PL);
    es->mkuprate = es->cogs = es->mkup = es->estship = es->weight = es->esttax = 0;
    es->taxable = 0;
    s_open(VD);

/* Changed on 6/1/97
    for (i=0;i<n;i++) {
        tagd = NO;
        s_read(PL, &pl, i);
        if (!strcmp(ptr, "!")) {
            // New improved Tagging method
            for (j=0; !tagd && j<MAXVENDORS; j++) {
                if (tagged[j] && (!stricmp(vdlist[j], pl.vdcode))) 
                    tagd = YES;
            }
        }
        else if (!strcheck(ptr,pl.vdcode)) {
            tagd = YES;
        }
        if (!tagd)
            continue;
*/
/* the following piece of code is replicated in db.c (part_list_print) as well */
    i = 0;
    for(j = 0; j < MAXVENDORS; j++) {
        // inits
        qtyPrice = 0;
        tmpTxb = 0;
        num = 1;

        tagd = NO;
        for(; !tagd && i < n; i++) {
            s_read(PL, &pl, i);
            if (!strcmp(ptr, "!")) {
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
            else if (!strcheck(ptr,pl.vdcode)) {
                if(!stricmp(ptr, pl.vdcode))
                {
                    if(stricmp(ptr, vdlist[j]))
                        break;
                    if(j == idx-1)
                        tagd = YES;
                    else /* skip all the PLs in the same group */
                    {
                        i++;
                        for(; i < n; i++)
                        {
                            s_read(PL, &pl, i);
                            if(stricmp(ptr, pl.vdcode))
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
        if(!tagd)
            continue;

        /* Read Vendor to check if it is Taxable */
        s_read(VD, &vd, s_getno(VD, 0, pl.vdcode));

        // Look ahead to figure out how many same vendors are in this group
        markUp = pl.markup;
        do {
            es->cogs     += pl.qty * pl.price;
            if(sysvar.ftax)
                es->estship  += pl.qty * pl.f * pl.weight * (1 + es->taxrate);
            else
                es->estship  += pl.qty * pl.f * pl.weight;
            qtyPrice += pl.qty * pl.price;
            es->weight   += pl.qty * pl.weight;
    
            if (toupper(vd.taxable[0]) == 'Y') 
                tmpTxb += (pl.qty * pl.price) + (pl.qty * pl.price * pl.markup/100);
            else
                tmpTxb = 0;

            if(i >= n)
                break;
            s_read(PL, &pl, i);
            if(stricmp(vdlist[j], pl.vdcode))   // not in the same group
                break;
            num++;
            i++;
        } while (1);

        /* round up bid price */
        tmpMk = qtyPrice * markUp / 100;
        if(rd && qtyPrice != 0)
        {
            tmpBid = es->cogs + (es->taxable + tmpTxb) * es->taxrate + es->mkup + tmpMk + es->estship;
            round = tmpBid;
            if(round != tmpBid) /* tmpBid is not yet rounded*/
                round = (tmpBid + 0.5);
again:      
            if(tmpTxb == 0) /* vd is not taxable */
                markUp = (FLOAT) (round - es->cogs - es->taxable * es->taxrate - es->mkup - es->estship) /
                    (FLOAT) (qtyPrice);
            else
                markUp = (FLOAT) (round - es->cogs - es->taxable * es->taxrate - es->mkup - es->estship - qtyPrice*es->taxrate) /
                    (FLOAT) (qtyPrice * (1 + es->taxrate));

            /* no negitave markup rate */
            if(markUp < 0)
            {
                round += 1;
                goto again;
            }

            markUp *= 100;

            // write markup rate back to file
            for(k = i - 1; num; k--, num--)
            {
                s_read(PL, &pl, k);
                pl.markup = markUp;
                s_write(PL, &pl, k, FALSE);
            }
        }
        else
        {
            s_read(PL, &pl, i-1);
            markUp = pl.markup;
        }

        es->mkup += qtyPrice * markUp/100;
        if (toupper(vd.taxable[0]) == 'Y') {
            es->taxable += (qtyPrice) + (qtyPrice * markUp/100);
        }
    }
    s_close(PL);

    if (!es->cogs)
      es->mkuprate=0;
    else
      es->mkuprate = es->mkup/es->cogs;
    s_open(SP);
    if ((i=s_getno(SP,0,es->spcode))!=-1)
      s_read(SP,&sp,i);
    else
      sp.rate = 0;
    s_close(SP);
#ifndef REP_VER
    es->com = es->mkup * sp.rate;
#else
    es->com = vd.com_rate * es->cogs * sp.rate;
#endif
    
    /* Only Applies the Tax to Products whose Vendors are Taxable. */
    es->esttax = es->taxable * es->taxrate;
#ifndef REP_VER
    es->bid = es->cogs + es->esttax + es->mkup + es->estship;
#else    
    es->bid = es->cogs + es->estship;
#endif
    es->subtotal = es->cogs + es->mkup;
    es->profit = es->mkup-es->com;
    return;
}

/****************************************************************************
es_funcs() - Processes the Keys that both Rep & Distributor can use.
*****************************************************************************/
INT es_funcs(DB_TYPE type, IN_POS *c, ESREC *es)
{
    FIELDPARAMS fp, attr;
    STATIC CHAR check[20];
    CHAR prompt[80], *data, prompt2[30], vdin[9];
    CHAR fname[60], tmpPath[30], tmpName[9], tmpExt[4];
    JBREC jb;
    CDREC client;
    STATIC INT F1flg=NO;
    ESREC tempes;
    SPREC sp;
    INT vec, datatype, choice, recnum, nem, i, fh, idx;

    attr = db[type].f[c->x];
    mkprompt(type, c->x, prompt);
    datatype = db[type].f[c->x].type;

    data = ((CHAR *)es + db[type].f[c->x].offset);

    if (es->plvec > 9)
	es->plvec = 1;

    /*
    * kludge to fix a colorization problem when F3 in ES
    */
    textattr(BLUE|ON_LIGHTGRAY);

    switch (c->x) {

	case 1:
	    c->last = c->x;
	    if (!*es->jobname) {
		s_read(JB, &jb, s_getno(JB, 0, es->jbcode));
		strcpy(es->jobname, jb.name);
		strcpy(es->state, jb.state);
	    }
	    c->x = inpt(attr.dx+strlen(prompt), attr.dy, "", data, attr.width, attr.type, NO);

	    break;

	case 2:
	    c->last = c->x;
	    if (!F1flg) {
		strcpy(check, data);
		c->x = inpt(attr.dx+strlen(prompt), attr.dy, "", data, attr.width, attr.type, NO);
		if (c->x == KEY_F1) {
		    if ((c->last == 2) && *check) {
			message(M_SCR|M_KEY, "Use F2 to create a new Estimate for a new Client - hit any key");
			c->x = 0;
			break;
		    }
		    F1flg = YES;
		    break;
		}
	    } else {
		c->x = 0;
		F1flg = NO;
	    }

	    if ((c->last == 2) && *check && stricmp(check,data)) {
		message(M_SCR|M_KEY, "Use F2 to create a new Estimate for a new Client - hit any key");
		strcpy(data, check);
		c->x = 0;
		break;
	    }

	    if (stricmp(check, data) && (vec = s_getno(datatype,0,data)) == -1) {
		c->x = 0;
		message(M_SCR|M_KEY, "%s is not defined - hit any key", db[datatype].extension);
		strcpy(data, check);
	    }

	    if ((c->last == 2) && stricmp(check,data)) {
		s_read(CD, &client, vec);
        strcpy(es->client_code, client.code);
        strcpy(es->esttel, client.tel);
		strcpy(es->estname, client.contact1);
        s_inset(type, c, es);
	    }
	    break;

	/*
	* Add
	*/
	case KEY_F2:
	    c->x = 0;
	    s_vcheck(type, es, c->ypos);
	    s_clearrec(type, &tempes);
	    mkprompt(type, c->x, prompt2);
	    strcpy(tempes.jbcode, es->jbcode);
	    sprintf(prompt, "Hit F1 or Enter new record's %s", prompt2);
	    message(M_SCR, NULL);
	    if ((choice = inpt(1, 25, prompt, tempes.jbcode, 8, JB, NO)) == KEY_F10) {
		message(M_SCR, NULL);
		break;
	    }
	    message(M_SCR, NULL);
	    if (choice == KEY_F1) {
		if ((recnum = s_getno(JB, 0, tempes.jbcode)) == -1)
		    recnum = s_nrecs(JB)-1;
		choice = gethelp(JB, recnum, db[JB].s->num, db[JB].s->fld, db[JB].s->x-1, db[JB].s->y-1);
		if (choice == -1)
		    break;
		s_read(JB, &jb, choice);
		strcpy(tempes.jbcode, jb.code);
	    } else {
		sprintf(tempes.jbcode, "%08ld", atol(tempes.jbcode));
		if ((vec = s_getno(JB, 0, tempes.jbcode)) == -1) {
		    message(M_SCR|M_KEY, "That Job does not exist - hit any key");
		    break;
		}
		s_read(JB, &jb, vec);
	    }
	    if (jb.status[0] != 'B') {
		message(M_SCR|M_KEY, "Job has already been awarded - hit any key");
		break;
	    }
	    vec = s_getno(type, 0, tempes.jbcode);
	    if (vec != -1) {
		if (vec >= 10)
		    vec -= 10;
		c->ypos = s_findrec(type, es, &tempes, NULL, vec);
		s_read(type, es, c->ypos);
		s_lock(type);
		s_recinsert(type, es);
		s_read(type, &tempes, c->ypos);
		if (memcmp(&tempes, es, sizeof(ESREC)))
		    c->ypos++;
		attribute = db[type].f[0];
		es->plvec = 0;
		*es->client_code = '\0';
		StrDate(es->estdate, &today);
		s_recwrite(type, es, c->ypos);
		s_unlock(type);
	    } else {
		strcpy(tempes.jobname, jb.name);
		strcpy(tempes.state, jb.state);
		StrDate(es->estdate, &today);
		s_lock(type);
		c->ypos = s_recinsert(type, &tempes);
		s_unlock(type);
	    }
	    changeflg = c->read = c->set = YES;  /*OPP*/
	    c->last = 2;
	    break;

	/*
	* Delete record
	*/
	case KEY_F3:
	    plfile(es);
	    memcpy(&tempes, es, sizeof(ESREC));

	    if (s_delrec(type, es, c->ypos) != -1) {
		if (tempes.plvec && *tempes.client_code) {
		    s_delfile(PL);
		}
		c->x = 0;
		changeflg = NO;
		if ((c->ypos == s_nrecs(type)) && (c->ypos != 0))
		    c->ypos--;
		c->read = c->set = YES;  /*OPP*/
	    }
	    break;

	/*
	* Re-calculate from PL
	*/
	case KEY_F4:

	    if (!es->plvec || !*es->client_code) {
		message(M_SCR|M_KEY, "Part List does not Exist - hit any key");
		break;
	    }

	    if ((vec = s_getno(JB, 0, es->jbcode)) == -1) {
		message(M_SCR|M_KEY, "Job is not defined - hit any key");
		break;
	    }
	    memcpy(&tempes, es, sizeof(ESREC));
	    if (message(M_SCR|M_KEY, "Get COGS and Estimated Shipping from Part List? (y/N)") == 'Y') {
		plfile(es);
		strcpy(vdin, "*");
        set_rate(es, vdin, TRUE); /* this sets vdlist[] global */
#ifndef REP_VER
		if (message(M_SCR|M_KEY, "F1 for list of Vendors, or ENTER for All:") == KEY_F1) {
            idx = set_rate(es, vdin, FALSE);
		}
#else
        if((idx = vd_chooser(vdin)) == -1)
		    break;
#endif
        re_calc(es,vdin, TRUE, idx);
	    } else {
		s_open(SP);
		vec = s_getno(SP, 0, es->spcode);
		if (vec != -1)
		    s_read(SP, &sp, vec);
		else {
		    sp.rate = 0;
		    message(M_SCR|M_KEY, "Sales Person is not defined - hit any key");
		}
		es->com = es->mkup * sp.rate;
		es->esttax = (es->cogs + es->mkup) * es->taxrate;
#ifndef REP_VER
		es->bid= es->cogs + es->esttax + es->mkup + es->estship;
#else               
		es->bid= es->cogs + es->estship;
/*
		if(strcmp(es->vdcode, ""))
		    s_read(VD, &vd, s_getno(VD, 0, es->vdcode));
		else
		    vd.com_rate = 0;
		es->com = es->cogs * sp.rate * vd.com_rate;
*/
#endif
		es->subtotal= es->cogs + es->mkup;
		es->profit=es->bid - (es->com+es->esttax+es->estship+es->cogs);
	    }
	    c->x = 0;
	    s_inset(type, c, es);
	    changeflg = YES;
	    break;

	/*
	* award job
	*/
	case KEY_F7:
	    if (!*es->client_code || (vec=s_getno(CD, 0, es->client_code)) == -1) {
		message(M_SCR|M_KEY, "Client is not defined - hit any key");
		break;
	    }
	    s_vcheck(type, es, c->ypos);
	    s_read(CD, &client, vec);
	    if (message(M_SCR|M_KEY, "Award job %s to %s? (y/N)", es->jbcode, client.name) != 'Y')
		break;
	    if ((vec=s_getno(JB, 0, es->jbcode)) == -1) {
		message(M_SCR|M_KEY, "Job is not defined - hit any key");
		break;
	    } else {
		s_read(JB, &jb, vec);
		if (*jb.status && jb.status[0]!='B') {
		    message(M_SCR|M_KEY, "Job has already been awarded - hit any key");
		    break;
		}
	    }
	    s_lock(type);
	    for (nem=i=(c->ypos>10 ? c->ypos-10:0); i<s_nrecs(type); i++) {
		s_read(type, &tempes, i);
		if (i==c->ypos || strcheck(tempes.jbcode, es->jbcode)) {
		    if (i == c->ypos) {
			c->ypos = nem;
			tempes.award = YES;
			tempes.status[0] = 'A';
			tempes.status[1] = '\0';
		    }
		    s_recwrite(type, &tempes, nem++);
		} else if (tempes.plvec) {
		    plfile(&tempes);
		    s_delfile(PL);
		}
	    }
	    s_modnrecs(type, nem);
	    s_unlock(type);
	    s_read(JB, &jb, vec);
	    jb.status[0] = 'A';
	    jb.status[1] = '\0';
	    jb.esvec = c->ypos;
	    s_recwrite(JB, &jb, vec);
	    c->x = 0;
	    s_read(type, es, c->ypos);

	    /* Change ES date to Today */
	    StrDate(es->estdate, &today);

	    s_write(type, es, c->ypos, FALSE);

	    c->set = YES;
	    break;

	/*
	* Part List
	*/
	case KEY_F8:
	    if (!*es->jbcode) {
		message(M_SCR|M_KEY, "Job is not defined - hit any key");
		break;
	    }
	    if (!*es->client_code) {
		message(M_SCR|M_KEY, "Client is not defined - hit any key");
		break;
	    } else {
		if (!switchflg) {
		    fp = *(db[PL].f+5);
		    *(db[PL].f+5) = *(db[PL].f+6);
		    *(db[PL].f+6) = fp;
		    switchflg = 1;
		}
	    }
	    if (!es->plvec) {
		s_close(PL);
		plmake(c->ypos, es);
		changeflg = YES;
	    }
	    /* s_vcheck(type, es, c->ypos); */

	    /* automatically save this record without asking */
	    s_write(type, es, c->ypos, FALSE);

	    /* find correct PL filename for this ES */
	    plfile(es);

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
	    if ((fh = FileOpen(fname)) != -1) {
		s_tempfile(PL);
		s_open(PL);
		FileCopyH(fh, db[PL].file);

		tempEsPtr = es;
		store();
		db_inpt(PL);
		s_open(PL);

		/*
		* if user wants to save changes, then copy new stuff over the
		* original PL file
		*/
		if (savecheck()) {
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

		memcpy(&tempes, es, sizeof(ESREC));
		vec = s_getno(ES, 0, tempes.jbcode);
		if (vec >= MAXESTIMATES)
		    vec -= MAXESTIMATES;
		c->ypos = s_findrec(ES, es, &tempes, NULL, vec);
		if (c->ypos < 0)
		    c->ypos = 0;
	    }

#ifdef REP
	    /*
	    * Automatically update ES totals in Rep Version
	    */
	    strcpy(vdin, "*");
        set_rate(es, vdin, TRUE); /* this sets vdlist[] global */
        re_calc(es, vdin, FALSE, -1);
	    s_lock(ES);
	    s_recwrite(ES, es, c->ypos);
	    s_unlock(ES);
#endif

	    c->x = 0;
	    c->set = YES;
	    break;
    }

    return(0);
}

/******************************************************************************
checkjobfollowup

Check the job followup comments in the jobcom structure for a specific user
******************************************************************************/
VOID checkjobfollowup(CHAR *user)
{
#ifdef CHECKJOBFOLLOW
    CHAR *jc;
    INT i, j, jc_file_handle;
    SHORT num_rec, find, rec_no, really_found = 0;
    CHAR *ch, *date, *com, save_date[9], *save_com = NULL;
    CHAR today_date[9];
    ESREC es;
    SPREC sp;

    StrDate(today_date, &today);
    today_date[8] = '\0';
    save_date[8] = '\0';

    jc_file_handle = FileOpen("AUTOMAT.JC");
    if(FileRead(jc_file_handle, &num_rec, sizeof(SHORT)) == -1)
	num_rec = 0;
    s_open(ES);
    /* get sales person's full name */
    s_open(SP);
    rec_no = s_getno(SP, 0, user);
    if(rec_no >= 0)
	s_read(SP, &sp, rec_no);
    else
	strcpy(sp.name, "");
    s_close(SP);

    jc = (CHAR *) s_alloc(sizeof(jobcom));

    for(i = 0; i < num_rec; i++)
    {
	memset(jc, 0, sizeof(jobcom));
	if(FileRead(jc_file_handle, jc, sizeof(jobcom)) == -1)
	    continue;
	find = FALSE;

	/* find the last comment */
	for(j = 9 + 75*7; j >= 9; j = j - 75)
	{
	    ch = (CHAR *) &jc[j];
	    com = (CHAR *) &ch[9];
	    ch[8] = '\0';
	    date = (CHAR *) ch;

	    if(strcmp(date, "") && !strcmp(com, "")) /* only date field must be specified */
	    {
		strcpy(save_date, date);
		date = &ch[6]; /* year */
        if(atoi(date)+20 < (today.da_year+20)%100)
		{
		    find = TRUE;
		    break;
		}
		ch[5] = '\0';
		date = &ch[3]; /* day */
		ch[2] = '\0'; /* month */
		if(atoi(ch) < today.da_mon)
		{
		    find = TRUE;
		    break;
		}
		if(atoi(date) <= today.da_day)
		{
		    find = TRUE;
		    break;
		}
	    }
	    else if(strcmp(date, "") && strcmp(com, "")) /* no followup date without a comment attached */
		break;
	    else
		jc[j-1] = '\0';
	}

	if(find)
	{
	    /* look up the ES file to find the salesman */
	    if(!strcmp(&jc[0], ""))
		continue;
	    rec_no = s_getno(ES, 0, &jc[0]);
	    if(rec_no < 0)
		continue;
	    s_read(ES, &es, rec_no);

	    if(!strcmpi(es.spcode, user) && strcmp(es.status, "S") && strcmp(es.status, "P"))
	    {
		/* get the previous comment */
		j = j - 75;
		if(j >= 9)
		{
		    ch = (CHAR *) &jc[j];
		    save_com = (CHAR *) &ch[9];
		}    
		    
		if(!really_found)
		{
		    really_found = 1;
		    /* Trun on the condense print mode */
		    fprintf(defPrinter, "%s", prnCodes.cond);

		    fprintf(defPrinter, "\n\tJob Follow-Up comment for sales Person %s\n", sp.name);
		    fprintf(defPrinter, "Job Code Job Name                         Date     Comments\n");
		    fprintf(defPrinter, "________________________________________________________\n");
		}
		fprintf(defPrinter, "%s %-32s %s %s\n", es.jbcode, es.jobname, save_date, save_com);
	    }
	}
    }

    free(jc);
    s_close(ES);
    FileClose(jc_file_handle);

    if(really_found)
    {
	/* Turn off the condense print mode */
	fprintf(defPrinter, "%s", prnCodes.reset);
    }
#endif
    user = user;
}


