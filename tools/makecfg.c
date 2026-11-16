/**
*   MAKECFG.C   - Creates the default .CFG file into the specified directory
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
*   3.00a       09-15-93     BCW        Revision 3
*
**/
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "db_defs.h"

/*
   Takes one parameter: user ID as in MB for MB.CFG
*/
INT main(INT argc, CHAR *argv[])
{
    INT handle, i;
    SYSREC sysvar;
    CHAR target[60];

    memset(&sysvar, 0, sizeof(SYSREC));

    strcpy(sysvar.company, "Automation Solutions, Inc.");
    strcpy(sysvar.fax, "");
    strcpy(sysvar.phone, "");
    strcpy(sysvar.prnDriver0, "DEFAULT.DRV");
    strcpy(sysvar.prnDriver1, "DEFAULT.DRV");

    for (i=0; i<=DB_END; i++)
        strcpy(sysvar.filename[i], "AUTOMAT");

    strcpy(sysvar.filename[HL], "DBHELP");
    strcpy(sysvar.path[HL], ".\\HELP\\");
    strcpy(sysvar.path[PD], ".\\PRODUCTS\\");
    strcpy(sysvar.path[PL], ".\\PLIST\\");
    strcpy(sysvar.path[RS], ".\\RSCH\\");
    strcpy(sysvar.path[IN], ".\\INST\\");

    /* build target .CFG file */
    sprintf(target, "%s.CFG", (argc>1 ? argv[1]: "DB"));

    /*
    * open .CFG file
    */
    if ((handle = open(target, O_CREAT|O_BINARY|O_RDWR|O_DENYNONE, S_IREAD)) == -1) {
				perror("\nError creating .CFG file");
    } else {
        write(handle, &sysvar, sizeof(SYSREC));
        close(handle);
        _dos_setfileattr(target, FA_HIDDEN);
    }
    return(0);
}
