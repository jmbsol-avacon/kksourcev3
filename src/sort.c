/**
*   SORT.C      -
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
#include "db_defs.h"

extern FIELDPARAMS attribute;

/*********************************************************************
sortcmp - generic comparison routine for sortfile.
*********************************************************************/
INT sortcmp(const VOID *c1, const VOID *c2)
{
    INT out;

    out = 0;
    if (attribute.offset != -1) {

        c1 = (const CHAR *)c1 + attribute.offset;
        c2 = (const CHAR *)c2 + attribute.offset;

        switch(attribute.type) {

            case CT_INT:
                out = intcmp((SHORT *)c1, (SHORT *)c2);
                break;

            case CT_FLOAT3:
            case CT_FLOAT:
                out = floatcmp((FLOAT *)c1, (FLOAT *)c2);
                break;

            case CT_DATE:
                out = datecmp((struct date *)c1, (struct date *)c2);
                break;

            case CT_CHDATE:
                out = strdatecmp((CHAR *)c1, (CHAR *)c2);
                break;

            case CT_LONG:
                out = longcmp((LONG *)c1, (LONG *)c2);
                break;

            default:
                out = stricmp((const CHAR *)c1, (const CHAR *)c2);
                break;
        }
    }
    return(out * attribute.x);
}

/*********************************************************************
sortprep - sets up attribute for sortcmp
*********************************************************************/
VOID sortprep(DB_TYPE type)
{
    INT fld;

    attribute = db[type].f[0];

    for (fld = 0; !(attribute.sort&0x01) && fld < db[type].fields; )
        attribute = db[type].f[++fld];
    if (fld == db[type].fields)
        attribute.offset = -1;
}

/*****************************************************************************
sortfile - sorts given file by the given fld.
*****************************************************************************/
INT sortfile(DB_TYPE type, INT updown, INT fld)
{
    INT i,n;
    VOID *ptr;

    s_lock(type);
    if ((n = s_nrecs(type)) != 0) {

        attribute=db[type].f[fld];
        attribute.x=(updown==KEY_DOWN ? -1:1);

        ptr = s_alloc(n*db[type].fsize);

        for (i=0;i<n;i++)
            s_read(type, ((CHAR *)ptr+i*db[type].fsize), i);

        message(M_SCR, "Sorting...");
        qsort(ptr, n, db[type].fsize, sortcmp);
        for (i=0;i<n;i++)
            s_recwrite(type, ((CHAR *)ptr+i*db[type].fsize), i);
        s_free(ptr);
        s_unlock(type);
    }

    return(0);
}

/*********************************************************************
gencmp - generic comparison routine.
*********************************************************************/
INT gencmp(VOID *c1, VOID *c2)
{
    INT x;

    x = 0;
    if (attribute.offset != -1) {

        c2 = (CHAR *)c2 + attribute.offset;
        switch(attribute.type) {
            case CT_INT:
                x = intcmp((SHORT *)c1, (SHORT *)c2);
                break;

            case CT_FLOAT3:
            case CT_FLOAT:
                x = floatcmp((FLOAT *)c1, (FLOAT *)c2);
                break;

            case CT_DATE:
                x = datecmp((struct date *)c1, (struct date *)c2);
                break;

            case CT_CHDATE:
                x = strdatecmp((CHAR *)c1, (CHAR *)c2);
                break;

            case CT_LONG:
                x = longcmp((LONG *)c1, (LONG *)c2);
                break;

            default:
                x = strcheck((CHAR *)c1, (CHAR *)c2);
                break;
        }
    }
    return(attribute.x * x);
}

/*********************************************************************
getno_mess - executes s_getno and prints message
*********************************************************************/
INT getno_mess(DB_TYPE type, INT fld, VOID *d1)
{
    message(M_SCR, "Working. . .");
    return(s_getno(type, fld, d1));
}

/*********************************************************************
power - calculates n1 to the power of n2.
*********************************************************************/
INT power(INT n1, INT n2)
{
    INT i,n3;

    for(i=1,n3=n1; i<n2; n1*=n3,i++)
        ;
    return(n1);
}
