/**
*   DB_FAX.C    -
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
#include "vfile.h"

extern SYSREC sysvar;

/***************************************************************************
FaxSend() - Send a fax
     fname: filename of Textfile to send.
     phone: fax number to send the fax to.
***************************************************************************/
INT FaxSend(const CHAR *fname, const CHAR *phone)
{
    CHAR *tmpName, *tmpFax, *p;

    /* validate the input params */
    Assert(fname != NULL);
    Assert(phone != NULL);

    tmpName = s_alloc(80);
    tmpFax = s_alloc(80);

    /*
    * if there is a cover sheet then fax that too
    */
    FileTempName(tmpName);

    if (FileExists("KKCOVER.FAX"))
        FileCopy("KKCOVER.FAX", tmpName);

    FileAppend(fname, tmpName);

    /* prepare the fax command for execution */
    strcpy(tmpFax, sysvar.mailserver);

    /*
    * get the filename
    */
    if ((p = strstr(tmpFax, "&f")) != NULL) {
        strcpy(p, tmpName);
        strcpy(p+strlen(tmpName), (CHAR *)sysvar.mailserver + (p - tmpFax) + 2);
    }

    /*
    * get the phone number
    */
    if ((p = strstr(tmpFax, "&p")) != NULL) {
        strcpy(p, phone);
        strcpy(p+strlen(phone), (CHAR *)sysvar.mailserver + (p - tmpFax) + 2);
    }

    /* run the FAX command */
    SysCmd(tmpFax);

    /*
    * remove the temporary file - this may not be allowed
    */
    FileDelete(tmpName);

    s_free(tmpFax);
    s_free(tmpName);

    return(0);
}
