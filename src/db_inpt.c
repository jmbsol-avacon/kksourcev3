/**
*   DB_INPT.C   -
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
#include <string.h>
#include <stdio.h>
#include <io.h>
#include <conio.h>
#include "db_defs.h"
#include "vmisc.h"


extern struct date today;
extern CHAR lastjob[9];
extern MOUSE_CLICK *points;
extern FILE *defPrinter;
extern BOOLEAN changeflg;

extern ESREC *tempEsPtr; /* for Ctrl_F2 copy PL func. */

/* flag to tell only update the time when db_inpt() is called */
SHORT db_inpt_flag;

/***************************************************************************
show_product_list - pops up a product list window
***************************************************************************/
CHAR *show_product_list()
{
    FILE *fp;
    int i, j, blank, tmp_len, len = 0, start = 0, current = 0;
    CHAR *list[65];
    STATIC CHAR tmp[128], *tmp_list[11];
    int choose;
    BOOLEAN endflag = FALSE;

    fp = fileopn("AUTOMAT.PRD", "r");
    if(fp == NULL)
    {
        message(M_SCR|M_KEY, "AUTOMAT.PRD file doesn't exist, no help provided - any key to continue");
        return NULL;
    }
    
    while(fgets(tmp, 128, fp) != NULL && len < 64)
    {
        /* delete trailing spaces */
        while(tmp[strlen(tmp) - 1] == ' ' || tmp[strlen(tmp) - 1] == '\n')
            tmp[strlen(tmp) - 1] = '\0';
        list[len] = strdup(tmp);
        len++;
    }
    list[len] = NULL;
    filecls(fp);

    if(len <= 10) /* can use window_chooser directly */
    {
        choose = window_chooser(10, 5, list);
        if(choose == -1) 
        {
            memset(tmp, 0, 128);
            goto end;
        }
        strcpy(tmp, list[choose]);
        tmp[127] = '\0';
        tmp_len = strlen(tmp);
        for(i = 0; i < tmp_len; i++)
            if(tmp[i] == ' ')
                break;
        blank = i + 1;
        for(i = blank; i < tmp_len; i++)
            tmp[i - blank] = tmp[i];
        tmp[i - blank] = '\0';
        tmp[20] = '\0'; /* product field length = 21 in VDREC */
        goto end;
    }
    
    for(i = 0; i < 10; i++)
        tmp_list[i] = list[i];
    tmp_list[i] = NULL;
    while(!endflag)
    {
        choose = window_chooser_mod(10, 5, current, tmp_list);
        if(choose == -2)
        {
            if(start == 0) /* already at the top, wrap to bottom */
            {
                for(i = len - 10, j = 0; i < len; i++, j++)
                    tmp_list[j] = list[i];
                start = len - 10;
                current = 9;
            }
            else /* shift 1 (downword) */
            {
                start--;
                for(i = start, j = 0; i < start + 10; i++, j++)
                    tmp_list[j] = list[i];
                current = 0;
            }
        }
        else if(choose == -3)
        {
            if(start == len - 10) /* already at the bottom, wrap to top */
            {
                for(i = 0; i < 10; i++)
                    tmp_list[i] = list[i];
                start = 0;
                current = 0;
            }
            else /* shift 1 (upword) */
            {
                start++;
                for(i = start, j = 0; i < start + 10; i++, j++)
                    tmp_list[j] = list[i];
                current = 9;
            }
        }
        else /* set endflag */
            endflag = TRUE;
    }
    if(choose == -1)
    {
        memset(tmp, 0, 128);
        goto end;
    }
    strcpy(tmp, tmp_list[choose]);
    tmp[127] = '\0';
    tmp_len = strlen(tmp);
    for(i = 0; i < tmp_len; i++)
        if(tmp[i] == ' ')
            break;
    blank = i + 1;
    for(i = blank; i < tmp_len; i++)
        tmp[i - blank] = tmp[i];
    tmp[i - blank] = '\0';
    tmp[20] = '\0'; /* product field length = 21 in VDREC */

end:
    for(i = 0; i < len; i++)
        if(list[i]) 
            free(list[i]);

    return tmp;
}


/***************************************************************************
db_inpt - user input screen for db files
***************************************************************************/
INT db_inpt(DB_TYPE type)
{
    STATIC BOOLEAN startover;
    IN_POS c;
    FIELDPARAMS attr;
    CHAR *dataptr, *search, *tempptr, *bob, check[80], prompt[80], prompt2[40];
    CHAR *data, *newdata, holdtemp[9];
    INT i, count, vec, data_type;
    BOOLEAN endflg, f1Flg, funcFlg, tempChange;
    MOUSE_CLICK *lastP;
    /* new variables for copying JB and PL */
    INT ans, intcode, maxplno = 0, posi;
    BOOLEAN copytype = NO, found_es, copyjob = FALSE;
    CHAR *saveptr, savecode[9];
    CHAR jbcode[9], clcode[9], old_plfname[21], new_plfname[21];
    JBREC *jb;
    ESREC es;

    /* verify that 'type' is a valid DB_TYPE */
    Assert(type <= DB_END);

    /* inits */
    memset(&c, 0, sizeof(IN_POS));

    /*make sure user has access to data*/
    if (seccheck(type, 'R')) {

        /*
        * allocate space for data
        */
        search  = (CHAR *)s_alloc(db[type].fsize);
        tempptr = (CHAR *)s_alloc(db[type].fsize);
        dataptr = (CHAR *)s_alloc(db[type].fsize);
        saveptr = (CHAR *)s_alloc(db[type].fsize);

        /*initialize search record*/
        s_clearrec(type, search);
        f1Flg = endflg = FALSE;

        /*
        * execute user initialization if defined
        */
        if (db[type].init_func) {
            if ((*db[type].init_func)(type, &c, dataptr) == -1) {
                c.rc = -1;
                endflg = TRUE;
            }
        }

        if (!endflg) {

            /*
            * can't edit the file if we can't open it
            */
            if (s_open(type) != -1) {

                /*
                * initialize the input screen
                */
                (*db[type].scr_func)(type);
                changeflg = NO;
                (*db[type].set_func)(type, &c, NULL);

                if (db[type].help_func != NULL)
                    (*db[type].help_func)(type, c.ypos, db[type].s->num,db[type].s->fld, db[type].s->x,db[type].s->y);
                
                s_read(type, dataptr, c.ypos);
                attr = db[type].f[0];
                mkprompt(type, c.x, prompt);
                (*db[type].msg)(dataptr);

                /* save last mouse click points */
                lastP = points;
                
                db_inpt_flag++;
                
                /*
                * input loop
                */
                do {
                    /* register mouse click points for this DB_TYPE */
                    points = db[type].mousePoints;

                    /*
                    * check if user function for this c.x
                    */
                    if (db[type].t) {
                        funcFlg = FALSE;						
                        for (i=0; (db[type].t[i].scan >= 0) && (db[type].t[i].scan != c.x); i++)
                            ;
                        if (db[type].t[i].scan == c.x)
                            funcFlg = TRUE;
                    }

                    /*
                    * if so execute it
                    */
                    if (funcFlg) {
                        INT temp2;
                        temp2 = c.x;
                        (*db[type].t[i].func)(type, &c, dataptr);
                        if ((c.x == temp2) && (c.x != 1))
                            c.x = 0;

                    /*
                    * execute mouse click and jump to the right field
                    */
                    } else if(c.x >= (SHORT)KEY_MOUSE_0 && c.x <= (SHORT)KEY_MOUSE_N) {
                        c.x = c.x - KEY_MOUSE_0;
                        c.last = 0;

                    /*
                    * otherwise execute default action
                    */
                    } else {

                        switch(c.x) {

                            /*
                            * field 0 will usually be handled differently from the others
                            */
                            case 0:
                                c.last = c.x;

                                /*
                                * check if field modified by F1 (help)
                                */
                                if (f1Flg) {
                                    f1Flg = FALSE;
                                /*
                                * otherwise get input from user
                                */
                                } else {

                                    /*save previous value of field 0 */
                                    strcpy(check, dataptr+attr.offset);

                                    /*user input of new value*/
                                    tempChange = changeflg;
                                    textattr(BLUE|ON_LIGHTGRAY);
                                    c.x = inpt(attr.dx+strlen(prompt), attr.dy, "", dataptr+attr.offset, attr.width, attr.type, NO);

                                    if (c.x == KEY_F1) {
                                        f1Flg = TRUE;
                                    }
                                }

                                /*
                                * check if it has been changed
                                */
                                if (strcmp(check, dataptr+attr.offset)) {

                                    /*
                                    * if so, check if the record already exists
                                    */
                                    if ((strchr(dataptr+attr.offset,'*') == NULL) && (strchr(dataptr+attr.offset,'?') == NULL))
                                        if ((db[type].f[0].type == ES) || (db[type].f[0].type == JB) || (db[type].f[0].type == PO))
                                            sprintf(dataptr+attr.offset, "%08ld", atol(dataptr+attr.offset));

                                    if ((vec = s_getno(type, 0, dataptr+attr.offset)) != -1) {
                                        if (tempChange) {
                                            if (savecheck()) {

                                                /* Save Previous Record */
                                                s_read(type, tempptr, c.ypos);
                                                memcpy(tempptr, dataptr, db[type].fsize);
                                                strcpy(tempptr+attr.offset, check);   /* BUG */
                                                s_lock(type);
                                                s_recwrite(type, tempptr, c.ypos);
                                                s_unlock(type);
                                            }
                                            tempChange = FALSE;  /* BUG */
                                        }

                                        /*if so, then go to the record*/
                                        s_read(type, dataptr, vec);
                                        c.ypos = vec;
                                        (*db[type].set_func)(type, &c, NULL);

                                    } else {
                                        CHAR tvd[12];

                                        /*
                                        * check if record exists in related file
                                        */
                                        if (attr.type > 0) {
                                            if (s_getno(attr.type, 0, dataptr+attr.offset) == -1) {
                                                message(M_SCR|M_KEY, "%s record does not exist - hit any key", db[attr.type].extension);
                                                strcpy(dataptr+attr.offset, check);
                                                c.x = 0;
                                                break;
                                            }
                                        }

                                        /*otherwise go to the end & create new record*/
                                        strcpy(check, dataptr+attr.offset);

                                        /*
                                        * If PD input, then Save record so that
                                        */
                                        if (type == PD) {

                                            /* the VD Code stays for new product.    */
                                            strcpy(tvd, dataptr+21);
                                        }
                                        s_clearrec(type, dataptr);
                                        strcpy(dataptr+attr.offset, check);
                                        if (type == PD) {
                                            strcpy(dataptr+21, tvd);
                                        }
                                        c.ypos = s_nrecs(type);
                                        (*db[type].set_func)(type, &c, NULL);
                                        printxy(attr.dx+strlen(prompt), attr.dy, "%s", dataptr+attr.offset);
                                    }

                                    /*
                                    * set up screen
                                    */
                                    if (db[type].help_func != NULL)
                                        (*db[type].help_func)(type, c.ypos, db[type].s->num,db[type].s->fld, db[type].s->x, db[type].s->y);
                                    changeflg = NO;
                                }
                                break;

                            /*
                            * help
                            */
                            case KEY_F1: {
                                c.x = 0;

                                data_type = db[type].f[c.last].type;
                                if (data_type < 0) {
                                    if(type == VD && strcmp(db[type].f[c.last].header, "Product|") == 0) /* bring up a product list window */ 
                                    {    
                                        CHAR *tmp;
                                        tmp = show_product_list();
                                        if(tmp) 
                                        {
                                            data = dataptr + attr.offset;
                                            strcpy(data, tmp);
                                            changeflg = YES;
                                        }
                                    }
									else if(type == CD && strcmp(db[type].f[c.last].header, "Password") == 0) /* set pswd for Client */
									{
										data = dataptr + attr.offset;
										for(i = 0; i < 6; i++)
											data[i] = (CHAR) GeneratePassword();
										data[i] = '\0';
										changeflg = YES;
									}
                                    else 
                                        helpmessage(type);
                                    break;
                                }
                                data = dataptr + attr.offset;  
                                vec = gethelp(data_type, s_getno(data_type, 0, data), db[data_type].s->num, db[data_type].s->fld, db[data_type].s->x-1, db[data_type].s->y-1);
                                if (vec != -1) {
                                    newdata = (CHAR *)s_alloc(db[data_type].fsize);
                                    s_read(data_type, newdata, vec);
                                    strcpy(data, newdata);
                                    s_free(newdata);
                                    changeflg = YES;

                                    if(type == RS)
                                        populate_rs(c.ypos, dataptr, TRUE);
                                }
                                break;
                            }

                            /*
                            * copy a job to a new record
                            */
                            case KEY_CTRL_F2:
                                if(type == JB) /* only in JB screen */
                                {
                                    ans = message(M_SCR|M_KEY, "Copy this job to a new job (N/y) ?");
                                    if(ans != 'Y' && ans != 'y')
                                    {
                                        c.x = c.last;
                                        c.last = 0;
                                        break;
                                    }
                                    else
                                    {
                                        copytype = YES;
                                        s_read(type, saveptr, c.ypos);
                                        jb = (JBREC *) saveptr;
/* always set status of copied JB to 'B' 
                                        if(jb->status[0] != 'B' && jb->status[0] != 'b')
                                        {
                                            if(jb->status[0] != 'A' && jb->status[0] != 'a')
                                                jb->status[0] = 'W';
                                        }
*/
                                        jb->status[0] = 'B';
                                        jb->cur_retn = 0; jb->pre_retn = 0;
                                        /* Don't copy due date and shipdate, 7/11/97 */
                                        jb->sent[0] = '\0'; jb->due[0] = '\0';
                                        strncpy(jb->ch_od_code, jb->code, 8); /* save change order job code */
                                        /* leave change order job code blank. This is as of 1/2/97 */
                                        jb->ch_od_code[0] = jb->ch_od_code[1] = jb->ch_od_code[2] = jb->ch_od_code[3] =
                                        jb->ch_od_code[4] = jb->ch_od_code[5] = jb->ch_od_code[6] = jb->ch_od_code[7] = '\0';
                                    }
                                }
                                else if(type == PL)
                                {
                                    ans = message(M_SCR|M_KEY, "Copy this part list to another job (N/y) ?");
                                    if(ans != 'Y' && ans != 'y')
                                    {
                                        c.x = c.last;
                                        c.last = 0;
                                        break;
                                    }
                                    else
                                    {
                                        jbcode[0] = '\0';
                                        clcode[0] = '\0';
                                        if (inpt(1, 25, "Job code: ", jbcode, 9, CT_CHAR, NO) >= KEY_F1)
                                        {
                                            c.x = c.last;
                                            c.last = 0;
                                            break;
                                        }
                                        message(M_SCR, "");
                                        if (inpt(1, 25, "Client code: ", clcode, 9, CT_CHAR, NO) >= KEY_F1)
                                        {
                                            c.x = c.last;
                                            c.last = 0;
                                            break;
                                        }

                                        found_es = NO;
                                        ans = s_nrecs(ES);
                                        for(i = 0; i < ans; i++)
                                        {
                                            s_read(ES, &es, i);
                                            intcode = atoi(jbcode);
                                            if(intcode != 0 && intcode == atoi(es.jbcode) && !stricmp(es.client_code, clcode))
                                            {
                                                found_es = YES;
                                                if(maxplno < es.plvec)
                                                    maxplno = es.plvec;
                                                posi = i;
                                            }
                                        }
                                        if(found_es == NO)
                                        {
                                            message(M_SCR|M_KEY, "No ES found with the specified job and client code -- any key to continue");
                                            c.x = c.last;
                                            c.last = 0;
                                            break;
                                        }

                                        s_read(ES, &es, posi); /* get the (last) one found before */
                                        if(es.plvec != 0)
                                        {
                                            ans = message(M_SCR|M_KEY, "Part List exists for the specified ES, overwrite it (N/y) ?");
                                            if(ans != 'Y' && ans != 'y')
                                            {
                                                c.x = c.last;
                                                c.last = 0;
                                                break;
                                            }
                                        }
                                        else
                                        {
                                            es.plvec = maxplno + 1;
                                            if(es.plvec > 9)
                                                es.plvec = 1;
                                        }
                                        s_write(ES, &es, ans, NO);
                                        sprintf(old_plfname, ".\\PLIST\\%s.%c%c%1.1d", tempEsPtr->jbcode,tempEsPtr->client_code[0],tempEsPtr->client_code[1],tempEsPtr->plvec);
                                        sprintf(new_plfname, ".\\PLIST\\%s.%c%c%1.1d", es.jbcode,clcode[0],clcode[1],es.plvec);
                                        FileCopy(old_plfname, new_plfname);
                                        c.x = c.last;
                                        c.last = 0;
                                        break;
                                    }
                                }
                                else
                                {
                                    c.x = c.last;
                                    c.last = 0;
                                    break;
                                }

                            /*
                            * Create a new record
                            */
                            case KEY_F2:
                                c.x = 0;
#ifdef restricted               if(s_vcheck(type, dataptr, c.ypos) == 2)
                                {
                                    changeflg = TRUE;
                                    if (type == JB || type == VD)
                                        create_delete_flag = TRUE;

                                    break;
                                }
#endif                          
                                s_vcheck(type, dataptr, c.ypos);
                                s_clearrec(type, tempptr);

#ifdef LTD_VER
#define LTD_NUM_JB      15
#define LTD_NUM_OTHER   5
                                if(type == JB && db[type].n >= LTD_NUM_JB)
                                {
                                    message(M_SCR|M_KEY, "You can not create more than %d jobs in this demo version.", LTD_NUM_JB);
                                    break;
                                }
                                else if(type != JB && db[type].n >= LTD_NUM_OTHER)
                                {    
                                    message(M_SCR|M_KEY, "You can not create more than %d such typed records in this demo version.", LTD_NUM_OTHER);
                                    break;
                                }
#endif

                                for (i=0; i<db[type].fields; i++) {
                                    /*
                                    * Spreadsheet Insert
                                    */
                                    if (db[type].f[i].sort&8) {
                                        c.x = i;
                                        if (c.ypos == s_nrecs(type))
                                            break;
                                        s_lock(type);
                                        for(i=s_nrecs(type); i>=c.ypos+1; i--) {
                                            s_read(type, tempptr, i-1);
                                            s_recwrite(type, tempptr, i);
                                        }
                                        s_modnrecs(type, s_nrecs(type)+1);
                                        s_clearrec(type, dataptr);
                                        s_recwrite(type, dataptr, c.ypos);
                                        s_unlock(type);
                                        insetup(type, &c, NULL);
                                        if(type == RS) /* populate new record to other page */
                                            populate_rs(c.ypos, dataptr, FALSE);
                                        break;
                                    }

                                    /*
                                    * Increment Last #
                                    */
                                    if (db[type].f[i].sort&4) {
                                        c.x = i;
                                        s_read(type, tempptr, s_nrecs(type)-1);
                                        strcpy(holdtemp, tempptr+db[type].f[c.x].offset);
                                        s_clearrec(type, tempptr);
                                        sprintf(tempptr+db[type].f[c.x].offset, "%08ld", atol(holdtemp)+1);
                                        break;
                                    }

                                    /*
                                    * Prompt for new Code
                                    */
                                    if (db[type].f[i].sort&1) {
                                        c.x = i;
                                        break;
                                    }
                                }

                                if (db[type].f[c.x].sort&8) {
                                    c.x = 0;
                                    break;
                                }
                                mkprompt(type, c.x, prompt2);
                                sprintf(prompt, "Enter new record's %s", prompt2);

                                message(M_SCR, NULL);
                                if (inpt(1, 25, prompt, tempptr+db[type].f[c.x].offset, db[type].f[c.x].width, db[type].f[c.x].type, NO) >= KEY_F1) {
                                    message(M_SCR, NULL);
                                    break;
                                }
                                copyjob = FALSE;
                                /* code to copy JB or PL */
                                if(copytype == YES)
                                {
                                    strcpy(savecode, tempptr+db[type].f[c.x].offset);
                                    memcpy(tempptr, saveptr, db[type].fsize);
                                    strcpy(tempptr+db[type].f[c.x].offset, savecode);
                                    copytype = NO;

                                    if(type == JB)
                                        copyjob = TRUE;
                                }
                                message(M_SCR, NULL);
                                if (!tempptr+db[type].f[c.x].offset)
                                    break;

                                if (db[type].f[c.x].type==PO || db[type].f[c.x].type==JB)
                                    sprintf(tempptr+db[type].f[c.x].offset, "%08ld", atol(tempptr+db[type].f[c.x].offset));

                                if ((c.ypos=s_getno(type, c.x, tempptr+db[type].f[c.x].offset)) != -1) {
                                    message(M_SCR|M_KEY, "Record already exists - hit any key");
                                    changeflg = NO;
                                } else {
                                    message(M_SCR, "Adding New Record. . .");

                                    /* If it is CTRL_F2 in creating JB, then need to create ES and PL/RS as well */
                                    if(copyjob)
                                    {
                                        INT ext = 1;
                                        ESREC es;
                                        CHAR oldname[60], filename[60];

                                        s_read(ES, &es, s_getno(ES, 0, jb->code));   /* get the old corresponding es */
                                        plfile(&es);                                 /* get the PL associated with the old es */
                                        sprintf(es.jbcode, "%08ld", atol(savecode)); /* copy the new job code */
                                        StrDate(es.estdate, &today); /* today's date */
                                        es.status[0] = 'B';          /* set status to B */
                                        s_write(ES, &es, s_nrecs(ES), NO);

                                        s_close(PL); 
                                        sprintf(oldname, "%s%s.%s", db[PL].path, db[PL].fname, db[PL].extension);
                                        sprintf(filename, "%s%s.%s", db[PL].path, es.jbcode, db[PL].extension);
                                        FileCopy(oldname, filename);

                                        sprintf(oldname, "%s%s.%03d", db[RS].path, jb->code, ext);
                                        while(access(oldname, 0x00) == 0)
                                        {
                                            sprintf(filename, "%s%s.%03d", db[RS].path, es.jbcode, ext);
                                            FileCopy(oldname, filename);
                                            ext++;
                                            sprintf(oldname, "%s%s.%03d", db[RS].path, jb->code, ext);
                                        }
                                    }

                                    if (type == PO) {

                                        /* copy the Job Code, Job Name, and SP Code to the new record */
                                        memcpy(tempptr+89, dataptr+89, 46);

                                        /* copy state and client as well */
                                        strcpy(((POREC *)tempptr)->client_code, ((POREC *)dataptr)->client_code);
                                        strcpy(((POREC *)tempptr)->state, ((POREC *)dataptr)->state);

                                        /* set the PO date to today */
                                        StrDate(tempptr+148, &today);

                                        /* also copy the job's change order code to jbcode2 */
                                        strcpy(((POREC *)tempptr)->jbcode2, ((POREC *)dataptr)->jbcode2);

                                        /* copy the ship date as well, 7/11/97 */
                                        strcpy(((POREC *)tempptr)->shipdate, ((POREC *)dataptr)->shipdate);
                                    }

                                    s_write(type, tempptr, s_nrecs(type), NO); 
                                    c.ypos = s_getno(type, c.x, tempptr+db[type].f[c.x].offset);
                                    c.x = 0;
                                    c.last = 1;
                                    changeflg = YES;
#ifdef restricted                   if (type == JB || type == VD)
                                        create_delete_flag = TRUE;
#endif
                                    /*
                                    * Create new CFG File for new SP
                                    */
                                    if (type == SP)
                                        sp_new(type, &c, (SPREC *)tempptr);
                                }

                                message(M_SCR, NULL);
                                c.set = c.read = YES;
                                break;

                            /*
                            * delete current record
                            */
                            case KEY_F3:

                                c.x = 0;
                                if ((c.ypos < s_nrecs(type)) && (s_nrecs(type) > 0)) {

                                    if (s_delrec(type, dataptr, c.ypos) != -1) {

                                        changeflg = NO;
                                        if(type == RS) /* also delete record in other page */
                                            populate_rs(c.ypos, NULL, FALSE);
                                        if (c.ypos==s_nrecs(type) && c.ypos!=0)
                                            c.ypos--;

                                        c.read = c.set = YES;
                                    }
                                }
                                break;

                            /*
                            * print
                            */
                            case KEY_F5:
                                c.x = 0;
                                print_single_record(type);
                                break;

                            /*
                            * find
                            */
                            case KEY_F6:
                                c.top = c.x = 0;

                                if (s_nrecs(type) == 0)
                                    break;

#ifdef restricted               if(s_vcheck(type, dataptr, c.ypos) == 2)
                                {
                                    changeflg = TRUE;
                                    break;
                                }
#endif                          
                                s_vcheck(type, dataptr, c.ypos);
                                message(M_SCR, NULL);

                                /*create search record*/
                                if (s_findinpt(type, search) == -1)
                                    break;

                                /*
                                * If Record is JB,ES,or PO, then pad w/0s
                                */
                                for (i=0;i<db[type].fields;i++) {
                                    if (db[type].f[i].sort&2 && (db[type].f[i].type==JB || db[type].f[i].type==ES || db[type].f[i].type==PO)) {
                                        bob = (search+db[type].f[i].offset);

                                        if (*bob)
                                            sprintf(bob, "%08ld", atol(bob));
                                    }
                                }

                                /*look for a match*/

                                if (changeflg)
                                    startover = YES;

                                if ((count = s_findrec(type, tempptr, search, NULL, (startover ? 0 : c.ypos+1))) != -1) {
                                    startover = NO;

                                    /*if match then goto record*/
                                    c.ypos = count;
                                    c.set = YES;
                                    memcpy(dataptr, tempptr, db[type].fsize);
                                    message(M_SCR, NULL);
                                } else {
                                    message(M_SCR|M_KEY, "No matches - hit any key");
                                    startover = YES;
                                }
                                (*db[type].msg)(dataptr);
                                changeflg = FALSE;
                                break;

                            /*
                            * exit
                            */
                            case KEY_F10: 
#ifdef restricted               if(s_vcheck(type, dataptr, c.ypos) == 2)
                                {
                                    changeflg = TRUE;
                                    c.x = c.last; c.last = 0;
                                    break;
                                }
#endif                          
                                s_vcheck(type, dataptr, c.ypos);
                                endflg = YES;
                                break;

                            /*
                            * input for curent field
                            */
                            default: 
                                /*
                                * if valid field
                                */
                                if (c.x < db[type].fields) {
                                    /*get user input for this field*/
                                    c.last = c.x;
                                    textattr(BLUE|ON_LIGHTGRAY);
                                    c.x = inpt(attr.dx+strlen(prompt), attr.dy, "", dataptr+attr.offset, attr.width, attr.type, NO);
                                /*
                                * otherwise go to field 0
                                */
                                } else {
                                    c.x = c.last;
                                    c.last = 0;
                                }
                                break;
                        }
                    }

#if 0
                    if (type == PO) {
                        JBREC jb;
                        INT dbugRec;

                        s_open(JB);
                        if ((dbugRec = s_getno(JB, 0, ((POREC *)dataptr)->jbcode)) != -1) {
                            s_read(JB, &jb, dbugRec);
                            if (jb.status[0] == 'W') {
                                jb.status[0] = 'O';
                                s_write(JB, &jb, dbugRec, NO);
                            }
                        }
                        s_close(JB);
                    }
#endif

                    /*move according to user's input*/
                    c = (*db[type].dir_func)(type, &c, dataptr);
#ifdef XDEMO
                    if ((type != PD) && (type != PL)) {
                        if ((type == VD) || (type == CD)) {
                            if (s_nrecs(type) >= 5) {
                                s_delete_rec(type, s_nrecs(type)-1);
                                if (c.ypos > 0)
                                    c.ypos--;
                            }
                        } else {
                            if (s_nrecs(type) >= 10) {
                                s_delete_rec(type, s_nrecs(type)-1);
                                if (c.ypos > 0)
                                    c.ypos--;
                            }
                        }
                    }
#endif

                    switch (type) {

                        case JB:
                        case ES:
                            strcpy(lastjob, dataptr);
                            break;

                        case PO:
                            strcpy(lastjob, dataptr+db[type].f[1].offset);
                            break;

                        default:
                            break;
                    }

                    /*
                    * read record if necessary
                    */
                    if (c.read) {
                        s_read(type, dataptr, c.ypos);
                        changeflg = NO;
                        c.read = NO;
                    }

                    /*
                    * update screen if necessary
                    */
                    if (c.set) {

                        /*
                        * Reset the Screen
                        */
                        if (c.flags & 0x04) {
                            (*db[type].scr_func)(type);
                            c.flags = 0;
                        }
                        (*db[type].set_func)(type, &c, NULL);

                        /*
                        * s_helpscreen(type, c.ypos);
                        */
                        if (db[type].help_func != NULL)
                            (*db[type].help_func)(type, c.ypos, db[type].s->num, db[type].s->fld, db[type].s->x, db[type].s->y);
                        c.set = NO;
                    }

                    /*
                    * get attribute information for current field
                    */
                    if (c.x<=db[type].fields) {
                        attr=db[type].f[c.x];
                        mkprompt(type,c.x,prompt);
                    }

                    (*db[type].msg)(dataptr);

                } while (!endflg); /*end of input do loop*/

                db_inpt_flag--;

                /* restore last mouse click points */
                points = lastP;

                s_close(type);
            }
        }

        /*
        * Give Allocated Space back to DOS
        */
        s_free(dataptr);
        s_free(tempptr);
        s_free(search);
    }

    return(c.rc);
}

/***************************************************************************
mkprompt - makes a prompt string from the header in the db structure.
    type:
    fld:    field for prompt
    ptr:    destination for prompt string
***************************************************************************/
VOID mkprompt(DB_TYPE type, INT fld, CHAR *ptr)
{
    FIELDPARAMS attr;

    attr=db[type].f[fld];
    if (attr.header[strlen(attr.header)-1]=='|')
          attr.header[strlen(attr.header)-1]=0;
    sprintf(ptr, "%s: ",attr.header);
    StrReplaceChar(ptr,'|',' ');
}

/***************************************************************************
s_findinpt - creates search prototype from user input for each field with
the find flag set in the db structure.
    type:
    search: field for prompt
***************************************************************************/
INT s_findinpt(DB_TYPE type, CHAR *search)
{
    INT i;
    CHAR prompt[80];

    textattr(REVERSE);
    for (i=0; i < db[type].fields; i++) {
        if (db[type].f[i].sort & 0x02) {
            mkprompt(type, i, prompt);
            if (inpt(1, 25, prompt, search+db[type].f[i].offset, db[type].f[i].width,db[type].f[i].type, NO) != 1) {
                textattr(NORMAL);
                return(-1);
            }
        }
    }
    textattr(NORMAL);
    return(0);
}

/****************************************************************************
skipfield - Skips field c->x for any type
****************************************************************************/
INT skipfield(DB_TYPE type, IN_POS *c, CHAR *data)
{
    type = type; data = data;

    c->last = (c->last==c->x-1 ? c->x : c->x-2);
    c->x = 1;
    return(0);
}

/*****************************************************************************
print_whole_file() - Executes Print routines for F5 from ANY module.
*****************************************************************************/
INT print_whole_file(DB_TYPE type, IN_POS *c, VOID *ptr)
{
    c = c; ptr = ptr;

    if ((s_nrecs(type) != 0) && (printercheck() != -1)) {

        printfile(defPrinter, type, s_nrecs(type));
        fputc(12, defPrinter);
    }
    return(0);
}

/***************************************************************************
print_singlr_record() - Prints Current record on F5.
****************************************************************************/
VOID print_single_record(DB_TYPE type)
{
    INT i, j;
    CHAR *bufr;

    if ((s_nrecs(type) != 0) && (printercheck() != -1)) {

        bufr = s_alloc(4096);

        gettext(1, 3, 80, db[type].s->y-1, bufr);

        j = 5;
        for (i=2; j!=db[type].s->y-1; i+=2) {

            if (i%160 == 0) {
                fputc('\n', defPrinter);
                fputc('\r', defPrinter);
                j++;
            } else {
                fputc(bufr[i], defPrinter);
            }
        }

        s_free(bufr);

        fputc('\n', defPrinter);
        fputc('\r', defPrinter);
        fflush(defPrinter);
        fputc(12, defPrinter);
    }
}

/**************************************************************************
gen_funcs (hot key hook) - processes all hot keys for ANY Spreadsheet type
    type:       db[] type
    c:          pointer to cursor struct
    ptr:        pointer to current record
**************************************************************************/
INT gen_funcs(DB_TYPE type, IN_POS *c, CHAR *ptr)
{
    FIELDPARAMS attr;

    attr = db[type].f[c->x];

    c->last = c->x;
    c->x = inpt(attr.x, c->y, "", ((CHAR *)ptr + db[type].f[c->x].offset), attr.width-1, attr.type, NO);

    return(0);
}
