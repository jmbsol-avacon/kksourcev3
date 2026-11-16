/**
*   S_FILE.C    -
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
#include <stdlib.h>
#include <io.h>
#include <dos.h>
#include <share.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <conio.h>

#include "db_defs.h"
#include "vmisc.h"
#include "vfile.h"

FIELDPARAMS attribute;
extern BOOLEAN changeflg;


/***************************************************************************
is_field_entered - checks the validaty of the specified field.
***************************************************************************/
BOOLEAN is_field_entered(char *field, char *mesg) /* not used, see ifdef restricted */
{
    if(field[0] != '\0') /* valid field */
        return TRUE;
    message(M_SCR|M_KEY, "'%s' field must be entered. Record not saved - hit any key to continue", mesg);
    return FALSE;
}

/***************************************************************************
check_validaty - checks input screen's validaty.
***************************************************************************/
BOOLEAN check_validaty(DB_TYPE type, VOID *rec)  /* not used, see ifdef restricted */
{
    VDREC *vd;
    JBREC *jb;

    if(rec == NULL)
        return FALSE;

    if(type == VD)
    {
        vd = (VDREC *) rec;
        /* the following fields must be entered before save */
        if(!is_field_entered(vd->product, "Product")) return FALSE;
        if(!is_field_entered(vd->tel, "VD Tel")) return FALSE;
        if(!is_field_entered(vd->fax, "VD Fax")) return FALSE;
        if(!is_field_entered(vd->address, "Address")) return FALSE;
        if(!is_field_entered(vd->city, "City")) return FALSE;
        if(!is_field_entered(vd->state, "State")) return FALSE;
        if(!is_field_entered(vd->zip, "Zip")) return FALSE;
        if(!is_field_entered(vd->contact, "Contact Name")) return FALSE;
        if(!is_field_entered(vd->agent, "Agent Name")) return FALSE;
    }
    else if(type == JB)
    {
        jb = (JBREC *) rec;
        /* the following fields must be entered before save */
        if(!is_field_entered(jb->city, "City")) return FALSE;
        if(!is_field_entered(jb->state, "State")) return FALSE;
        if(!is_field_entered(jb->due, "Due Date")) return FALSE;
    }

    return TRUE;
}

/***************************************************************************
s_lock - locks a file for critical access.
****************************************************************************/
INT s_lock(DB_TYPE type)
{
    /* validate 'type' */
    Assert(type <= DB_END+1);

    return(FileLock(db[type].file));
}

/**************************************************************************
s_unlock - unlocks a file after critical access.
***************************************************************************/
INT s_unlock(DB_TYPE type)
{
    /* validate 'type' */
    Assert(type <= DB_END+1);

    return(FileUnlock(db[type].file));
}

/****************************************************************************
s_open - opens a db file if closed.
*****************************************************************************/
INT s_open(DB_TYPE type)
{
    CHAR fname[60];
    INT retVal = 0;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    retVal = db[type].file;   
    if(type != PD && type != RS && retVal > 0)    // need to close PD or RS file, because not unique
        return (retVal);

//    for(retVal = 0; retVal <= DB_END; retVal++) // make sure to close other open files
//        s_close(retVal);

    s_close(type);
    sprintf(fname, "%s%s.%s", db[type].path, db[type].fname, db[type].extension);

    /* validate file name and path fields */
    Assert(strlen(db[type].path) < 30);
    Assert(strlen(db[type].fname) < 9);
    Assert(strlen(db[type].extension) < 4);

    /*
    * try to open the file as an existing file (don't try to create it yet)
    */
    retVal = db[type].file = handleopn(fname, O_RDWR|O_BINARY|SH_DENYNO, S_IFREG);

    if (db[type].file == -1) {

        /*
        * if doesn't exist then create it
        */
        if (errno == ENOENT) {

            if ((retVal = db[type].file = FileOpen(fname)) != -1) {
                s_modnrecs(type, 0);

                /* To indicate a NEW file. */
                retVal = -2;
            }
        } else {
            message(M_SCR|M_ERR|M_KEY, "Open Error (%s): %s - hit any key", fname, strerror(errno));
            quit(0);
        }
    } else {
        if (FileSize(retVal) == 0)
            s_modnrecs(type, 0);
    }
    return(retVal);
}

/****************************************************************************
s_modnrecs - modifies the nrecs value for a db file.
*****************************************************************************/
VOID s_modnrecs(DB_TYPE type, USHORT n)
{
    /* validate 'type' */
    Assert(type <= DB_END+1);

    FileSeek(db[type].file, 0L, SEEK_SET);
    FileWrite(db[type].file, &n, sizeof(USHORT));
    db[type].n = n;
}

/****************************************************************************
s_read - reads a record from a db file.
    type:
    ptr:    destination address for record
    rec:    record number
*****************************************************************************/
VOID s_read(DB_TYPE type, VOID *ptr, USHORT rec)
{
    LONG fptr;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    if (db[type].file <= 0)
        s_open(type);

    if (rec >= s_nrecs(type)) {
        s_clearrec(type, ptr);

    } else {
        fptr = rec;
        fptr *= db[type].fsize;
        fptr += db[type].fhead;
        FileSeek(db[type].file, fptr, SEEK_SET);

        if (FileRead(db[type].file, ptr, db[type].fsize) == -1)
            s_clearrec(type, ptr);
    }
}

/****************************************************************************
s_nrecs - returns the number of records for a db file
*****************************************************************************/
USHORT s_nrecs(DB_TYPE type)
{
    USHORT nrecs;

    /* verify that 'type' is a valid DB_TYPE (0 to DB_END+1) */
    Assert(type <= DB_END+1);

    if (db[type].file < 0)
        s_open(type);

    FileSeek(db[type].file, 0, SEEK_SET);

    if (FileRead(db[type].file, &nrecs, sizeof(USHORT)) == -1) {
        nrecs = 0;
    }

    /*
    * see if filesize has been modified by another user
    */
    if (nrecs != db[type].n) {
        s_open(type);
        db[type].n = nrecs;
    }
    return(nrecs);
}

/****************************************************************************
s_close - closes a db file if open.
*****************************************************************************/
INT s_close(DB_TYPE type)
{
    /* validate 'type' */
    Assert(type <= DB_END+1);

    if (db[type].file != -1) {

        /* Make sure file is unlocked. */
        s_unlock(type);

        /* Flush Buffers. */
        FileFlush(db[type].file);

        FileClose(db[type].file);

        db[type].file = -1;
    }
    return(0);
}

/****************************************************************************
s_write - writes or deletes a record.  s_write will replace the record if it
already exists, or create a record in its sorted position if it doesn't exist.
If delflg is non-zero the record is found and deleted.
    type:
    ptr:    record to write/delete
    rec:    record number when retrieved
    delflg: TRUE if want to delete this record
*****************************************************************************/
INT s_write(DB_TYPE type, VOID *ptr, USHORT rec, BOOLEAN delflg)
{
    CHAR *buf;
    INT fld, n, vec;
    BOOLEAN cmpflg;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    /* inits */
    cmpflg = FALSE;

    if (db[type].file < 0)
        s_open(type);

    buf = (CHAR *)s_alloc(db[type].fsize);

    attribute = db[type].f[0];

    for (fld = 0;  !(attribute.sort&0x01) && fld < db[type].fields; )
        attribute = db[type].f[++fld];

    if (fld == db[type].fields)
        fld = -1;

    /*
    * Take control of file
    */
    if (s_lock(type) == -1)
        return(-1);

    /* check if records match */
    s_read(type, buf, rec);

    if (fld != -1) if (gencmp(buf+attribute.offset,ptr) || rec == s_nrecs(type))
        cmpflg = TRUE;

    if (cmpflg) {
        /*
        * no match - search for record
        */
        if ((vec = s_getno(type, fld, ptr)) == -1) {

            /*not found - insert new record*/
            vec = s_recinsert(type, ptr);
        } else {

            /*
            * found -- replace
            */
            if (!delflg)
                s_recwrite(type, ptr, vec);
            else
                s_remove_rec(type, ptr, vec);
        }
    } else {

        /*match -- replace*/
        vec = rec;

        if (!delflg) {
            n = s_nrecs(type);
            if (rec == n)
                s_modnrecs(type, ++n);
            s_recwrite(type, ptr, rec);
        } else
            s_remove_rec(type, ptr, rec);
    }

    s_unlock(type);
    s_free(buf);

    /* Close file to flush out for other users */
    s_close(type);
    return(0);
}

/****************************************************************************
s_recinsert - puts new record in proper order in db file.
    type:
    ptr:    record to insert
*****************************************************************************/
INT s_recinsert(DB_TYPE type, VOID *ptr)
{
    USHORT nrecs, i;
    CHAR *temp2;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    temp2 = (CHAR *)s_alloc(db[type].fsize);
    nrecs = s_nrecs(type);

    for (i = nrecs; i > 0 ;i--) {
        s_read(type, temp2, i-1);

        /*
        * check if correct position found
        */
        if (sortcmp(temp2, ptr) < 0) {
            s_recwrite(type, ptr, i);
            break;
        } else {
            s_recwrite(type, temp2, i);
        }
    }

    /*
    * first record in file
    */
    if (!i) 
        s_recwrite(type, ptr, i);

    nrecs++;
    s_modnrecs(type, nrecs);
    s_free(temp2);

    return(i);
}

/*****************************************************************************
s_recwrite - writes a record at given position
    type:
    ptr - record to write
    rec - position to write it
******************************************************************************/
VOID s_recwrite(DB_TYPE type, VOID *ptr, USHORT rec)
{
    LONG fptr;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    fptr = rec;
    fptr *= db[type].fsize;
    fptr += db[type].fhead;

    /* seek to correct position to write record */
    FileSeek(db[type].file, fptr, SEEK_SET);

    /* write record at that point */
    FileWrite(db[type].file, ptr, db[type].fsize);
}

/****************************************************************************
s_delrec - queries user -- then deletes record
    type:   db type
    ptr:    record to delete
    record: record number when retrieved
*****************************************************************************/
INT s_delrec(DB_TYPE type, VOID *ptr, USHORT record)
{
    INT return_code = -1;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    if (!readonly(type)) {
        if (message(M_SCR|M_KEY, "Are you sure you want to delete this record? (y/N)") == 'Y') {
            message(M_SCR, "Deleting. . .");

            /* Delete the Record */
            return_code = s_write(type, ptr, record, YES);
            message(M_SCR, NULL);
        }
    }
    return(return_code);
}

/****************************************************************************
s_remove_rec - removes record and relocates remaining records
    type:
    ptr:
    record: record number to delete
*****************************************************************************/
VOID s_remove_rec(DB_TYPE type, VOID *ptr, USHORT record)
{
    USHORT i, n;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    n = s_nrecs(type)-1;
    for (i=record; i<n; i++) {
        s_read(type, ptr, i+1);
        s_recwrite(type, ptr, i);
    }

    s_modnrecs(type, n);
    if (chsize(db[type].file, (LONG)(((LONG)n*(LONG)db[type].fsize)+(LONG)db[type].fhead)) == -1)
        message(M_SCR|M_ERR|M_KEY, "Error Changing File Size! %s", strerror(errno));
}

/****************************************************************************
s_inset - prints record data at positions specified in db structure.  If data
== NULL then the record is read from the db file at the position c.ypos, else
the given record is used.
    type:   db type
    c:  cursor structure
    data:   pointer to a record
*****************************************************************************/
INT s_inset(DB_TYPE type, IN_POS *c, VOID *data)
{
    FIELDPARAMS attr;
    struct date *dout;
    CHAR *temp, out[80], tempdate[9];
    LONG *lout;
    FLOAT *fout;
    INT count, record;
    SHORT *iout;
    USHORT n;
    VOID *rec;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    record = c->ypos;
    n = s_nrecs(type);
    if (data == NULL) {
        rec = s_alloc(db[type].fsize);
        if (record >= n)
            s_clearrec(type, rec);
        else
            s_read(type, rec, record);
    } else {
        rec = data;
    }

    for (count=0; count<db[type].fields; count++) {
        attr = db[type].f[count];
        if (attr.header[strlen(attr.header)-1] == '|')
            attr.header[strlen(attr.header)-1] = 0;
        StrReplaceChar(attr.header, '|', ' ');
        gotoxy(attr.dx, attr.dy);
        textattr(NORMAL);
        Print("%s: ", attr.header);
        temp = (CHAR *)rec;
        temp += attr.offset;
        dout = (struct date *)temp; lout = (LONG *)temp; fout = (FLOAT *)temp; iout = (SHORT *)temp;
        memcpy(out, temp, attr.bytes);
        textattr(BLUE|ON_LIGHTGRAY);

        switch(attr.type) {

            case CT_LONG:
                if (record < n)
                    Print(attr.format, (long)*lout);
                else
                    ciprintf(attr.format, *iout);
                break;

            case CT_INT:
                if (record < n)
                    Print(attr.format, *iout);
                else
                    ciprintf(attr.format, *iout);
                break;

            case CT_FLOAT: case CT_FLOAT3:
                if (record < n)
                    Print(attr.format, *fout);
                else
                    bcprintf(attr.format, *fout);
                break;

            case CT_DATE:
                if (record < n) {
                    sprintf(tempdate, "%02d/%02d/%02d", dout->da_mon, dout->da_day,dout->da_year%100);
                    Print("%-8s", tempdate);
                } else {
                    Print("        ");
                }
                break;

            default:
                Print(attr.format,out);
                break;
        }
        Print(" ");
    }

    if (data == NULL)
        s_free(rec);

    return(0);
}

/****************************************************************************
s_getno - finds the record number of the record matching d1.  If the field
specified in fld is a sorted field in the db structure, then a binary search
is done, else linear search.
    type:   db type
    fld:    field number to match
    d1: data to match
*****************************************************************************/
INT s_getno(DB_TYPE type, INT fld, VOID *d1)
{
    USHORT count, n;
    VOID *d2;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    n = s_nrecs(type);
    attribute = db[type].f[fld];
    if (attribute.sort & 0x01)
        return(s_binsearch(type, d1));
    attribute.x = 1;

    d2 = s_alloc(db[type].fsize);
    for (count=0; count<n; count++) {

        s_read(type, d2, count);

        if (!gencmp(d1, d2)) {
            s_free(d2);
            return(count);
        }
    }
    s_free(d2);
    message(M_SCR, NULL);

    return(-1);
}

/*****************************************************************************
s_binsearch - performs a binary search on the sorted field using the data given
by d1.
    type:   db type
    d1: data to match.
******************************************************************************/
INT s_binsearch(DB_TYPE type, VOID *d1)
{
    INT vec=1, count=1, next, test, n;
    VOID *d2;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    d2 = s_alloc(db[type].fsize);
    attribute.x = 1;

    n = s_nrecs(type);
    while(vec < n)
        vec = power(2, count++);

    next = vec/=2;
    while(next) {
        if (vec<n) {
            s_read(type, d2, vec);
            if ((test = gencmp(d1, d2))==0) {
                s_free(d2);
                return(vec);
            }
        } else {
            test=-1;
        }
        next/=2;
        vec+=((test>0) ? next:-next);
    }
    s_read(type, d2, 0);
    test = gencmp(d1, d2);
    s_free(d2);

    return((test) ? -1:0);
}

/****************************************************************************
s_vcheck - calls the verification function, if the verification function
returns 0, verification fails.  If 1 then the record is written with query,
else it is written without query.
*****************************************************************************/
INT s_vcheck(DB_TYPE type, VOID *rec, USHORT num)
{
    INT check, user;
    PLREC *pl; /* for checking PL not exceeding max vendor */
    INT plnum, num_pl, i, j;
    BOOLEAN plsamevd;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    check = (*db[type].val_func)(rec);

#ifdef restricted /* take away the code to check JB or VD fields before save */   
    if (create_delete_flag == TRUE)
    {
        /* need to delete the record without query */
        if (!check_validaty(type, rec))
        {
                if(message(M_SCR|M_KEY, "Do you want to re-enter the data? (n/Y)") != 'N')
                        return(2);
                /* if the record is already saved before, delete it now */
                s_write(type, rec, num, TRUE);
                create_delete_flag = FALSE;
                return(YES);
        }
    }    
#endif    

    if (!changeflg)
        return(check);
    changeflg = NO;

    if (readonly(type))
        return(check);

    if (check == 2) {
        user = 'Y';
        check = 1;

    } else {
        /* For Room Schedule, always save change. Added on 4/20/97 */
        if (type == RS || type == IN)
            user = 'Y';
        else
            user = message(M_SCR|M_KEY, "Save changes to this record? (y/N)");
    }

    if (user != 'Y')
        return(check);

    if (check) {
        if (check==1) {
#ifdef restricted
            /* code to check input screen's validity. Save only if valid */
            if (!check_validaty(type, rec))
            {
                if(message(M_SCR|M_KEY, "Do you want to re-enter the data? (n/Y)") != 'N')
                        return(2);
                
                /* if the record is already saved before, delete it now */
                s_write(type, rec, num, TRUE);
                return(YES);
            }
#endif
            if(type == PL && (plnum = s_nrecs(type)) != 0)
            {
                pl = (PLREC *) s_alloc((plnum+1) * sizeof(PLREC));
                for(i = 0; i < plnum; i++)
                    s_read(PL, &pl[i], i);
                memcpy(&pl[plnum], rec, sizeof(PLREC));
                num_pl = 0;
                for(i = 0; i < plnum; i++)
                {
                    plsamevd = NO;
                    for(j = i+1; j < plnum+1; j++)
                    {
                        if(!strcmp(pl[i].vdcode, pl[j].vdcode))
                        {
                             plsamevd = YES;
                             break;
                        }
                    }
                    if(plsamevd == NO)
                        num_pl++;
                }
                num_pl++; /* count last one */
                s_free(pl);
                if(num_pl > MAXVENDORS)
                {
                    message(M_SCR|M_KEY, "Number of PL exceeds %d -- any key to continue", MAXVENDORS);
                    return (YES);
                }
            }

            if (type == PO) /* look for Actual COGS type over. Must add them together */
            {
                POREC *po = (POREC *) rec;
                INT jbcode, i, len;

                if(po->other != 0)
                {
                    po->actcogs += po->other;
                    po->other = 0;
                }
                
                if(po->jbcode2[0] != '\0')  /* take care of CO jobcode format */
                {
                    po->jbcode2[8] = '\0';
                    jbcode = atoi(po->jbcode2);
                    if(jbcode == 0)
                    {
                        message(M_SCR|M_KEY, "'%s', invalid Change Order Job Code - hit any key", po->jbcode2);
                        po->jbcode2[0] = '\0';
                    }
                    else if(strlen(po->jbcode2) < 8) /* padded with leading zero */
                    {
                        len = strlen(po->jbcode2);
                        for(i = len - 1; i >= 0; i--)
                            po->jbcode2[8 + i - len] = po->jbcode2[i];
                        for(i = 0; i < 8 - len; i++)
                            po->jbcode2[i] = '0';
                    }
                }
            }
            if (s_write(type, rec, num, FALSE) == -1)
                return(NO);
        }
        return(YES);
    } else
        return(NO);
}

/*****************************************************************************
s_directions - adjusts the cursor structure for next position.
    type:   ident
    rec:    current data record
    c:      cursor structure
******************************************************************************/
IN_POS s_directions(DB_TYPE type, IN_POS *c, VOID *rec)
{
    USHORT n;
    INT number;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    n = s_nrecs(type);
    number = db[type].fsize;

    switch(c->x) {

        case 0:
        case 1:
            c->x += c->last;
            if (c->x >= number)
                c->x = c->last = 0;
            break;

        case KEY_RIGHT:
        case KEY_DOWN:
            if (dirnext(type, c, c->x) == 0)
                break;
            c->x = c->last + 1;
            if (c->x>=number)
                c->x = 0;
            c->last = 0;
            break;

        case KEY_SHFT_TAB:
        case KEY_LEFT:
        case KEY_UP:
            if (dirnext(type, c, c->x) == 0)
                break;
            c->x=c->last-1;
            if (c->x<0)
                c->x=number-1;
            break;

        case KEY_PGUP:
            s_vcheck(type,rec,c->top+c->ypos);
            c->x=c->last;
            if (c->ypos>0) {
                c->ypos--;
                c->set=c->read=YES;
            } else {
                beep();
            }
            break;

        case KEY_PGDOWN:
            s_vcheck(type,rec,c->top+c->ypos);
            c->x=c->last;
            if (c->ypos<n-1) {
                c->ypos++;
                c->set=c->read=YES;
            } else {
                beep();
            }
            break;

        case KEY_CTRL_PGUP:
        case KEY_CTRL_HOME:
        case KEY_HOME:
            s_vcheck(type,rec,c->top+c->ypos);
            c->x=0;
            if (c->ypos==0) {
                beep();
            } else {
                c->ypos=0;
                c->set=c->read=YES;
            }
            break;

        case KEY_CTRL_PGDOWN:
        case KEY_CTRL_END:
        case KEY_END:
            s_vcheck(type, rec, c->top + c->ypos);
            c->x=0;
            if (c->ypos == n-1) {
                beep();
            } else {
                c->ypos = (n ? n-1:0);
                c->set = c->read = YES;
            }
            break;
    }

    return(*c);
}

/*****************************************************************************
s_inhelpset - updates the help screen.
    type:   ident
    rec:    current record number
    num:    number of records to print in help screen
    flds:   number of fields to print in help screen
    x:      left position for window
    y:      top position for window
******************************************************************************/
INT s_inhelpset(DB_TYPE type, INT rec, INT num, INT flds, INT x, INT y)
{
    INT count, count2, currec, newx, lastx=-1, fieldcount, test, ccnt, cx, n;
    CHAR *dat;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    dat = (CHAR *)s_alloc(db[type].fsize);
    currec = rec-num/2;
    n = s_nrecs(type);

    textattr(YELLOW|ON_BLUE);

    for (count=currec; count<currec+num; count++) {
        printxy(x, y, "%78s", "");
        gotoxy(x, y++);

        Print("%c", (count==rec ? 26:32));
        if ((count < n) && (count >= 0)) {
            s_read(type, dat, count);
        } else {
            /*setstr(dat, 0, db[type].fsize-1);*/
            memset(dat, 0, db[type].fsize);
        }
        lastx = -1;
        for (fieldcount=0; fieldcount<flds; fieldcount++) {
            test = 1000;
            for (count2=0; count2<db[type].fields; count2++) {
                newx = db[type].f[count2].x;
                if (newx && (newx > lastx) && ((newx-lastx) < test)) {
                    test = newx-lastx;
                    cx = newx;
                    ccnt = count2;
                }
            }
            lastx = cx;
            fldprint(type, dat, ccnt);
        }
    }
    textattr(NORMAL);
    s_free(dat);

    return(0);
}

/*****************************************************************************
s_findrec - finds a record matching multiple fields and criteria.
    type:   ident
    r1:     current search record/returned record
    r2:     prototype for match
    mask:   array for criteria: NONE,GE,GT,LT,LE,EQ,NE for each field
    startvec: first record to check -- s_findrec works up from startvec
******************************************************************************/
INT s_findrec(DB_TYPE type, VOID *r1, VOID *r2, INT *mask, INT startvec)
{
    INT count, count2, cmp, test, findflg;
    USHORT n = s_nrecs(type);
    FIELDPARAMS attr;
    VOID *p1, *p2;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    message(M_SCR, "Working . . .");

    /* linear search through file (starts at current record) */
    for (count=startvec; count<n; count++) {
        s_read(type, r1, count);
        findflg = YES;

        /* check each field of current record */
        for (count2=0; count2<db[type].fields; count2++) {
            attr = db[type].f[count2];
            p1 = (CHAR *)r1+attr.offset;
            p2 = (CHAR *)r2+attr.offset;

            if (mask==NULL)
                cmp=((attr.type >= CT_CHAR) ? EQ:NONE);
            else
                cmp = *(mask+count2);

            /*
            * compare field with search spec
            */
            switch (attr.type) {

                case CT_INT:
                    test = intcmp((SHORT *)p1,(SHORT *)p2);
                    break;

                case CT_FLOAT3:
                case CT_FLOAT:
                    test = floatcmp((FLOAT *)p1,(FLOAT *)p2);
                    break;

                case CT_DATE:
                    test = datecmp((struct date *)p1, (struct date *)p2);
                    break;

                case CT_CHDATE:
                    test = strdatecmp((CHAR *)p1, (CHAR *)p2);
                    break;

                case CT_LONG:
                    test = longcmp((LONG *)p1, (LONG *)p2);
                    break;

                default:
                    test = -strcheck((CHAR *)p2, (CHAR *)p1);
                    break;
            }

            /*
            * if this field should be compared
            */
            if (cmp!=NONE)  {
                /*
                * check if matches searchspec (>,<,== or !=)
                */
                if ((cmp == NE) && !test) {
                    findflg = NO;
                    break;
                } else if (cmp != NE && ((cmp == EQ && test) || (cmp > EQ && test<0)||(cmp<EQ&&test>0) || ((cmp==GT||cmp==LT)&&!test))) {
                    /* if not then no match */
                    findflg = NO;
                    break;
                }
            }
        }
        if (findflg)
            return(count);
    }
    return(-1);
}

/*****************************************************************************
s_clearrec - clears record to NULL
******************************************************************************/
VOID s_clearrec(DB_TYPE type, VOID *rec)
{
    /* validate 'type' */
    Assert(type <= DB_END+1);

    memset(rec, 0, db[type].fsize);
}

/***************************************************************************
directions - sets up the structure c for the next input for Spreadsheet.
    type:   db type
    rec:    pointer to record
    c:  pointer to cursor structure
***************************************************************************/
IN_POS directions(DB_TYPE type, IN_POS *c, VOID *rec)
{
    INT number;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    /* inits */
    number = db[type].fields;

    switch(c->x) {

        case 0:
        case 1:
            c->x += c->last;
            if (c->x < number)
                break;

            if (s_vcheck(type, rec, c->ypos)) {
                c->ypos++;

                if (c->ypos > s_nrecs(type))
                    c->ypos--;
                else {
                    if (c->ypos-c->top > 8) {
                        c->top++;
                        c->set = YES;
                    }
                    c->read = YES;
                }
            }
            c->x = 0;
            break;

        case KEY_RIGHT:
            if (c->last < number-1) {
                c->x = c->last+1;
                break;
            }
            c->last = 0;
            break;

        case KEY_DOWN:
            if (s_vcheck(type, rec, c->ypos)) {
                c->ypos++;
                if (c->ypos > s_nrecs(type))
                    c->ypos--;
                else {
                    if (c->ypos-c->top > 8) {
                        c->top++;
                        c->set = YES;
                    }
                    c->read=YES;
                }
            }
            c->x=c->last;
            break;

        case KEY_SHFT_TAB:
        case KEY_LEFT:
            if (c->last) {
                c->x = c->last-1;
                break;
            }
            c->last = number-1;
            break;

        /*
        * Sends Cursor to top when at top & going UP
        */
        case KEY_UP:  
            s_vcheck(type, rec, c->ypos);
            if (c->ypos == c->top) {
                if (c->ypos > 0)
                    c->ypos--;

                if (c->top > 0) {
                    c->top--;
                    c->set = YES;
                }
            } else if (c->ypos>0) {
                c->ypos--;
            }
            c->read = YES;
            c->x = c->last;
            break;

        case KEY_PGUP:
            s_vcheck(type, rec, c->ypos);
            c->x = c->last;
            c->top -= 8;
            c->ypos -= 8;
            if (c->top < 0)
                c->top = c->ypos = 0;
            c->set = c->read = YES;
            break;

        case KEY_PGDOWN:
            s_vcheck(type, rec, c->ypos);
            c->x = c->last;
            c->top += 8;
            c->ypos += 8;
            if (c->top+8 > s_nrecs(type)) {
                c->ypos = s_nrecs(type);
                c->top = c->ypos-8;
            }
            if (c->top < 0) {
                c->top = 0;
                c->ypos = s_nrecs(type);
            }
            c->set = c->read = YES;
            break;

        case KEY_CTRL_PGUP:
        case KEY_CTRL_HOME:
        case KEY_HOME:
            c->x = 0;
            s_vcheck(type, rec, c->ypos);
            c->top = 0;
            c->ypos = 0;
            c->set = c->read = YES;
            break;

        case KEY_CTRL_PGDOWN:
        case KEY_CTRL_END:
        case KEY_END:
            s_vcheck(type, rec, c->ypos);

            c->top = 0;
            if ((c->ypos = s_nrecs(type)) > 8)
                c->top = c->ypos - 8;

            c->set = c->read = YES;
            c->x = 0;
            break;
    }

    if (c->ypos - c->top > 8) {
        c->top = c->ypos - 8;
        c->set = YES;
    }

    c->y = 2*(c->ypos - c->top) + 6;
    return(*c);
}

/***************************************************************************
dirnext - sets up the structure c for the next input. (called by s_directions)
    type:   db type
    rec:    pointer to record
    c:      pointer to cursor structure
***************************************************************************/
INT dirnext(DB_TYPE type, IN_POS *c, INT dir)
{
    FIELDPARAMS attr, l, f;
    INT count, temp2 = -1;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    f.dy = f.dx = -1;
    l = db[type].f[c->last];

    for (count=0; count<db[type].fields; count++) {

        attr = db[type].f[count];

        if (count != c->last) {

            switch (dir) {

                case KEY_RIGHT:
                    if ((attr.dy != l.dy) || (l.dx > attr.dx))
                        break;

                    if ((f.dx == -1) || (attr.dx < f.dx)) {
                        f = attr;
                        temp2 = count;
                    }
                    break;

                case KEY_SHFT_TAB:
                case KEY_LEFT:
                    if ((attr.dy != l.dy) || (l.dx < attr.dx))
                        break;

                    if ((f.dx == -1) || (-attr.dx < -f.dx))  {
                        f = attr;
                        temp2 = count;
                    }
                    break;

                case KEY_UP:
                    if ((attr.dx != l.dx) || (l.dy < attr.dy))
                        break;
                    if ((f.dy == -1) || ( -attr.dy < -f.dy))  {
                        f = attr;
                        temp2 = count;
                    }
                    break;

                case KEY_DOWN:
                    if ((attr.dx != l.dx) || (l.dy > attr.dy))
                        break;

                    if ((f.dy == -1) || (attr.dy < f.dy)) {
                        f = attr;
                        temp2 = count;
                    }
                    break;
            }
        }
    }
    if ((f.dx == -1) || (f.dy == -1))
        return(-1);

    c->x = temp2;

    return(0);
}

/***************************************************************************
s_openfiles - opens the files specified by types
    num: number of files to open
    ...: types for files to open must be given
***************************************************************************/
VOID s_openfiles(INT num, ...)
{
    va_list ap;
    INT i;

    va_start(ap, num);

    s_closeall();

    for (i=0; i<num; i++)
        s_open((DB_TYPE)va_arg(ap, int));   /* DB_TYPE is promoted to int: GCC traps otherwise */
}

/***************************************************************************
s_closeall - closes all open db files
***************************************************************************/
VOID s_closeall(VOID)
{
    INT i;

    for (i=0; db[i].f != NULL; i++)
        if (db[i].file != -1)
            s_close(db[i].file);
}

/***************************************************************************
s_delfile - removes the db file specified by type number
***************************************************************************/
INT s_delfile(DB_TYPE type)
{
    CHAR fileName[80];

    /* validate 'type' */
    Assert(type <= DB_END+1);

    sprintf(fileName, "%s%s.%s", db[type].path, db[type].fname, db[type].extension);

    /* don't blow the stack */
    Assert(strlen(fileName) < 80);

    /*
    * unlock, close, and delete the file
    */
    s_unlock(type);
    s_close(type);

    FileDelete(fileName);

    return(0);
}

/****************************************************************************
s_delete_rec() - Deletes rec from type.
****************************************************************************/
INT s_delete_rec(DB_TYPE type, USHORT rec)
{
    VOID *ptr;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    /* allocate a temporary buffer to hold this record */
    ptr = s_alloc(db[type].fsize);

    /* lock the file so we can delete a record */
    s_lock(type);

    /* delete the record */
    s_remove_rec(type, ptr, rec);

    s_unlock(type);
    s_free(ptr);

    return(0);
}

/*****************************************************************************
s_tempcopy - creates a temporary copy of the given db file - returns name.
    type:  DB_TYPE
    filename:   returned name of temporary file.
*****************************************************************************/
INT s_tempcopy(DB_TYPE type, CHAR *fileName)
{
    INT fd;

    /* validate 'type' */
    Assert(type <= DB_END+1);

    /* can't write to a null pointer */
    Assert(fileName != NULL);

    *fileName = '\0';

    FileClose(FileCreateTemp(fileName));

    fd = FileOpen(fileName);
    s_open(type);
    FileCopyH(db[type].file, fd);

    return(fd);
}


VOID s_tempfile(DB_TYPE type)
{
    CHAR fname[60], fname2[60], ext[5], *p;
    CHAR drive[3];
/* reserve the drive name */
/*
    _splitpath(_fullpath(fname, FileTempName(fname2), 60), NULL, db[type].path, db[type].fname, p = ext);
*/    
    _splitpath(_fullpath(fname, FileTempName(fname2), 60), drive, db[type].path, db[type].fname, p = ext);
    memset(fname, 0, 60);
    drive[2] = '\0';
    strcat(fname, drive);
    strcat(fname, db[type].path);
    strcpy(db[type].path, fname);
    strcpy(db[type].extension , p+1);
}
