/**
*   DB_DEFS.C   -
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
#include "db_defs.h"

/****************************************************************************
DATA BASE DEFINITIONS
*****************************************************************************/

/***************************************************************************
  FIELD DEFINITION STRUCTURE
  {data type},{bytes},{offset in structure},{format},{header},{help position},
  {x coordinate},{y coordinate},{input field width},{flags}

  INFO SCREEN STRUCTURE:
  {x position},{y position},{# of records},{# of fields},{message struc ptr}

flag mask:  1 = Sort
            2 = Find enable
            4 = Increment last record on F2
            8 = Insert blank record in Spreadsheet on F2
           32 = Use Field in F5=PRINT
*****************************************************************************/


/****************************************************************************
* The Following Defs are Global to REP or DB or DEMO
****************************************************************************/

MOUSE_CLICK mouseClicks[] = {
    {0, 184, 48, 184, KEY_F1},
    {64, 184, 104, 184, KEY_F2},
    {120, 184, 160, 184, KEY_F3},
    {176, 184, 224, 184, KEY_F4},
    {240, 184, 296, 184, KEY_F5},
    {312, 184, 360, 184, KEY_F6},
    {376, 184, 432, 184, KEY_F7},
    {448, 184, 496, 184, KEY_F8},
    {512, 184, 560, 184, KEY_F9},
    {576, 184, 632, 184, KEY_F10},

/* for page up/down, home/end function in the blue area of the input screen */
    {8, 104, 32, 128, KEY_HOME},
    {8, 144, 32, 168, KEY_END},
    {600, 104, 624, 128, KEY_PGUP},
    {600, 144, 624, 168, KEY_PGDOWN},

/* for mouse click in input screen to simulate KEY_ENTER. (temporary) */
//    {0, 0, 632, 120, KEY_ENTER},

    {0, 0, 0, 0, 0}
};

/************************* User Defs ********************************/

FIELDPARAMS usfields[] = {
    SP,       4,  0, "%-3s", "USER|",     1, 1, 1, 4,10,
    CT_CHAR, 10, 10, "%-8s", "PASSWORD|", 6, 1, 1, 9, 0,
    CT_CHAR,  2, 20, "%-2s", "HL|",      16, 1, 1, 2, 0,
    CT_CHAR,  2, 22, "%-2s", "CD|",      19, 1, 1, 2, 0,
    CT_CHAR,  2, 24, "%-2s", "VD|",      22, 1, 1, 2, 0,
    CT_CHAR,  2, 26, "%-2s", "PD|",      25, 1, 1, 2, 0,
    CT_CHAR,  2, 28, "%-2s", "SP|",      28, 1, 1, 2, 0,
    CT_CHAR,  2, 30, "%-2s", "JB|",      31, 1, 1, 2, 0,
    CT_CHAR,  2, 32, "%-2s", "ES|",      34, 1, 1, 2, 0,
    CT_CHAR,  2, 34, "%-2s", "PL|",      37, 1, 1, 2, 0,
    CT_CHAR,  2, 36, "%-2s", "PO|",      40, 1, 1, 2, 0,
    CT_CHAR,  2, 38, "%-2s", "SY|",      43, 1, 1, 2, 0,
    CT_CHAR,  2, 40, "%-2s", "LI|",      46, 1, 1, 2, 0,
    CT_CHAR,  2, 42, "%-2s", "CL|",      49, 1, 1, 2, 0,
    CT_CHAR,  2, 44, "%-2s", "DS|",      52, 1, 1, 2, 0,
    CT_CHAR,  2, 46, "%-2s", "RS|",      55, 1, 1, 2, 0,
    CT_CHAR,  2, 48, "%-2s", "LG|",      58, 1, 1, 2, 0,
    CT_CHAR,  2, 50, "%-2s", "IN|",      61, 1, 1, 2, 0,
    CT_CHAR,  2, 52, "%-2s", "RP|",      64, 1, 1, 2, 0,
    CT_CHAR,  2, 54, "%-2s", "XX|",      67, 1, 1, 2, 0,
    CT_CHAR,  2, 56, "%-2s", "XX|",      70, 1, 1, 2, 0,
    CT_CHAR,  2, 58, "%-2s", "XX|",      73, 1, 1, 2, 0
};

FUNC_TABLE usfuncs[] = {
    0, gen_funcs,
    1, gen_funcs,
    2, gen_funcs,
    3, gen_funcs,
    4, gen_funcs,
    5, gen_funcs,
    6, gen_funcs,
    7, gen_funcs,
    8, gen_funcs,
    9, gen_funcs,
    10, gen_funcs,
    11, gen_funcs,
    12, gen_funcs,
    13, gen_funcs,
    14, gen_funcs,
    15, gen_funcs,
    16, gen_funcs,
    17, gen_funcs,
    18, gen_funcs,
    19, gen_funcs,
    20, gen_funcs,
    21, gen_funcs,
    KEY_F5, print_whole_file,
    -1, NULL
};

DB_MSG usmsg[] = {1, 24, "F1=HELP F2=ADD F3=DEL         F5=PRINT F6=FIND                          F10=EXIT",0,0,NULL };

INFO_SCREEN usscreen = { 0, 0, 0, 0, usmsg };

/******************** Vendor Database ************************************/
FIELDPARAMS vdfields[] = {
    VD,        9,   0, "%-8s",    "VD|Code",         1,  2,  3,  8, 3,
    CT_CHAR,  33,   9, "%-31s",   "VD|Name",        10,  2,  4, 31, 2,
	CT_CHAR,  33, 287, "%-32s",   "Home|http Addr",  0,  2,  5, 32, 0, /* added on 05/28/00 */
    CT_CHAR,  21,  42, "%-20s",   "Product|",       57,  2,  6, 20, 0,
    CT_CHAR,  33,  63, "%-32s",   "Contact|Name",    0,  2,  7, 32, 0,
	CT_CHAR,  49, 320, "%-48s",	"Contact|EMail Addr",0,  2,  8, 48, 0, /* added on 05/28/00 */
#ifndef REP_VER
    CT_CHAR,  33,  96, "%-32s",   "Agent|Name",      0,  2,  9, 32, 0,
    CT_CHAR,  15, 232, "%-14s",   "Agent Tel|",      0,  2, 10, 14, 0,
    CT_CHAR,  15, 247, "%-14s",   "Agent Fax|",      0, 28, 10, 14, 0,
	CT_CHAR,  49, 369, "%-48s",	"Agent|EMail Addr",  0,  2, 11, 48, 0, /* added on 05/28/00 */
#endif
    CT_CHAR,  41, 129, "%-40s",   "Address|",        0,  2, 12, 40, 0,
    CT_CHAR,  16, 170, "%-15s",   "City   |",        0,  2, 13, 15, 0,
    ST,        3, 186, "%-2s",    "State|",          0, 31, 13,  2, 0,
    CT_CHAR,  11, 191, "%-12s",   "Zip|",            0, 54, 13, 10, 0,
    CT_CHAR,  15, 202, "%-14s",   "VD Tel #|",      43, 54,  3, 14, 0,
    CT_CHAR,  15, 217, "%-14s",   "VD Fax #|",       0, 54,  4, 14, 0,
    CT_CHAR,   2, 189, "%-1s",    "Taxable|",        0, 54,  6,  1, 0,
    CT_FLOAT,  4, 262, "%-10.2f", "YTD|Shipped",     0, 54,  7, 10, 0,
    CT_FLOAT,  4, 270, "%-10.2f", "YTD|On Order",    0, 54,  9, 10, 0,
    CT_FLOAT,  4, 266, "%-10.2f", "Total|Sales",     0, 54, 10, 10, 0,
    CT_CHDATE, 9, 278, "%-8s",    "Last|Active",     0, 54, 12,  8, 0,
#ifdef REP_VER
    CT_FLOAT3, 4, 274, "%-6.3f",  "Commission|Rate", 0, 54, 10,  6, 0
#endif
};

FUNC_TABLE vdfuncs[] = {
    KEY_ALT_F2, allcomments,
    KEY_F4, vd_funcs,
    KEY_F7, vd_funcs,
    KEY_F8, vd_funcs,
    KEY_F9, letters,
    -1,NULL
};

DB_MSG vdmsg[] = {
    25, 1, "       VENDOR INPUT SCREEN",
    2, 15, " VD Code  VD Name                          Telephone     Product",
    2, 16, "------------------------------------------------------------------------------",
    1, 24, "F1=HELP F2=ADD F3=DEL F4=DIS  F5=PRINT F6=FIND F7=CLASS F8=LINE F9=FORM F10=EXIT",
    0, 0, NULL
};

//INFO_SCREEN vdscreen = { 2,15,8,4,vdmsg };
INFO_SCREEN vdscreen = { 2,17,6,4,vdmsg };

/************************* Part List Database **************************/

FIELDPARAMS plfields[] = {
    CT_LONG,  4,  1, "%-6ld",    "QTY|",          1, 3, 3,  6, 8,
    VD,       9,  5, "%-8s",     "VENDOR|CODE",   8, 3, 4,  8, 2,
    PD,      21, 14, "%-21.21s", "PRODUCT|CODE", 18, 3, 5, 21, 2,
    CT_CHAR, 31, 35, "%-24s",    "DESCRIPTION|", 40, 3, 6, 24, 0,
    CT_FLOAT, 4, 66, "%-7.2f",   "PRICE|",       65, 3, 7,  8, 0,
    CT_FLOAT, 4, 74, "%-7.2f",   "UNIT|WEIGHT",  74, 3, 8,  7, 0,
    CT_FLOAT, 4, 70, "%-7.2f",   "UNIT|RATE",    74, 3, 8,  7, 0
};

FUNC_TABLE plfuncs[] = {
    0,gen_funcs,
    1,pl_funcs,  /* to validate Vendor Code */
    2,pl_funcs,  /* for F1 on Product Code */
    3,gen_funcs,
    4,gen_funcs,
    5,gen_funcs,
    KEY_ALT_S,pl_funcs,
    KEY_ALT_F1,pl_funcs,
    KEY_SHFT_F1,pl_funcs,
    KEY_ALT_F3,pl_funcs,
    KEY_F4,pl_funcs,
    KEY_F5,print_whole_file,
    KEY_F7,pl_funcs,
    KEY_F8,pl_funcs,
    KEY_F9,pl_funcs,
    -1,NULL
};

DB_MSG plmsg[] = {
    1,24,"F1=HELP F2=ADD F3=DEL F4=DSCT F5=PRINT F6=FIND F7=XREF F8=TOTAL F9=GET  F10=EXIT",
    0,0,NULL
};

INFO_SCREEN plscreen = { 0,0,0,0,plmsg };

/********************** Purchase Orders *****************************/

FIELDPARAMS pofields[] = {
    PO,        9,  0, "%-8s",   "PO|Number",        1,  3,  3,  8,  5,
    JB,        9, 89, "%-8s",   "JB|Code",         10,  3,  4,  8,  2,
    VD,        9, 67, "%-8s",   "VD|Code",          0,  3,  5,  8,  2,
    SP,        4, 98, "%-3s",   "SP|Code",         20,  3,  6,  3,  0,
    CT_CHAR,  33,102, "%-32s",  "Job|Name",        25,  3,  7, 32,  2,

    CT_CHDATE, 9,148, "%-8s",   "PO.|Date",        60,  3,  8,  8,  0,
    CT_CHDATE, 9,  9, "%-8s",   "Rels|Date",        0,  3,  9,  8,  0,
    CT_CHDATE, 9, 76, "%-8s",   "Ship.|Date",      70,  3, 10,  8,  0,

	CT_CHAR,  23,223, "%-22s",  "Ship Track|#",		0,  3, 11, 22,  0,	/* Added on 6/18/00 */

    CT_CHAR,   8, 36, "%-7s",   "Inv.|No",          0, 25,  3,  7,  2,
    CT_CHAR,   9, 58, "%-8s",   "CO JB|#",          0, 25,  4,  8,  0,
	CT_CHAR,   9,252, "%-8s",   "Material|PO",	    0, 25,  6,  8,  0,	/* Added on 6/18/00 */
    CT_CHDATE, 9,205, "%-8s",   "Sub|Date",         0, 25,  8,  8,  0,
    CT_CHDATE, 9,214, "%-8s",   "Aprvl|Date",       0, 25,  9,  8,  0,
    CT_CHDATE, 9,263, "%-8s",   "Invo.|Date",       0, 25, 10,  8,  0,

    CT_FLOAT,  4, 32, "%-9.2f", "PO Cost of|Goods", 0, 50,  3,  9,  0,
    CT_FLOAT, 10, 85, "%-9.2f", "Actual|COGS",      0, 50,  4,  9,  0,      
    CT_FLOAT,  4,157, "%-8.2f", "Est.|Ship",        0, 50,  5,  8,  0,
    CT_FLOAT,  4,161, "%-8.2f", "Actual|Ship",      0, 50,  6,  8,  0,
    CT_FLOAT,  4,165, "%-8.2f", "Tax|",             0, 50,  7,  8,  0,
    CT_FLOAT,  4,169, "%-8.2f", "Current|COGS",     0, 50,  8,  8,  0,
    CT_FLOAT,  4,173, "%-9.2f", "Profit|",          0, 50,  9,  9,  0,
    CT_FLOAT,  4,181, "%-9.2f", "Total|Bid",        0, 50, 10,  9,  0,
	CT_INT,    2,261, "%-5d",	"Ship|Company",	  262, 50, 11,  3,  0,	/* Added on 6/18/00 */

    CD,        9,185, "%-8s",   "Client|",          0,  0,  0,  0,  0,
    CT_CHAR,   3,135, "%-2s",   "State|",           0,  0,  0,  0,  0,
    CT_CHAR,   2,196, "%-1s",   "Status|",          0,  0,  0,  0,  0,
};

FUNC_TABLE pofuncs[] = {
#ifndef REP
    4,		skipfield,	/* Job Name */
    15,		skipfield,	/* PO COGS */
    16,		skipfield,	/* Actual COGS */
    17,		skipfield,	/* Est. Ship */
    KEY_F1, po_funcs,
    KEY_F4, po_funcs,
    KEY_F7, po_funcs,
    KEY_F9, po_funcs,
#endif
    -1,NULL
};

DB_MSG pomsg[] = {
    26,1,"JOB ACCOUNTING INPUT SCREEN",
    2,13," PO #     JB Code  SP  JB Name                          PO Date  Ship Date",
    2,14,"------------------------------------------------------------------------------",
    1,24,"F1=HELP F2=ADD F3=DEL F4=CALC F5=PRINT F6=FIND F7=SHIP          F9=FORM F10=EXIT",
    0,0,NULL
};

INFO_SCREEN poscreen = { 2,15,8,6,pomsg };

/********************** Product Database ********************************/

FIELDPARAMS pdfields[] = {
    PD,      21,   0, "%-20s",  "PD|Code",         1,  2,  4, 20, 3,
    CT_CHAR, 11,  21, "%-10s",  "PD|Vendor",      53,  2,  5, 10, 0,
    CL,       5,  63, "%-4s",   "Class|",          0,  2,  7,  4, 0,
    LI,       3,  68, "%-2s",   "Prod.|Line",      0,  2,  8,  2, 0,
    CT_FLOAT, 4,  71, "%-6.2f", "Weight|(LBS)",    0,  2,  9,  6, 0,
    CT_FLOAT, 4,  75, "%-6.2f", "Size|(CuFt)",     0,  2, 10,  6, 0,
    CT_CHAR, 31,  32, "%-30s",  "Description|",   22, 33,  4, 30, 2,
    CT_CHAR, 21, 108, "%-20s",  "Cross|Reference", 0, 33,  5, 20, 2,
    CT_FLOAT, 4,  79, "%-8.2f", "List|Price",     64, 33,  7,  8, 0,
    CT_FLOAT, 4,  83, "%-8.2f", "Dealer|Price",    0, 33,  8,  8, 0,
    CT_FLOAT, 4,  87, "%-8.2f", "Net|Price",       0, 33,  9,  8, 0,
    CT_FLOAT, 4,  91, "%-8.2f", "Sales|Price",     0, 33, 10,  8, 0,
    CT_CHAR,  3,  95, "%-2s",   "Ship.|Meth.",     0, 60,  7,  2, 0,
    CT_CHAR,  3,  98, "%-2s",   "Ship.|Code",     71, 60,  8,  2, 0,
    CT_CHAR,  3, 101, "%-2s",   "Ship.|Note",      0, 60,  9,  2, 0,
    CT_FLOAT, 4, 104, "%-6.2f", "Ship.|Rate",      0, 60, 10,  6, 0
};

FUNC_TABLE pdfuncs[] = {
    KEY_F7, pd_xref,
    KEY_F8, pd_mkup,
    KEY_F9, pd_load,
    -1,NULL
};

DB_MSG pdmsg[] = {
    25,1,"      PRODUCT INPUT SCREEN",
    2,12," PD Code              Description                    Vendor     List $ Shp.",
    2,13,"------------------------------------------------------------------------------",
    1,24,"F1=HELP F2=ADD F3=DEL         F5=PRINT F6=FIND F7=XREF  F8=MARK F9=LOAD F10=EXIT",
    0,0,NULL
};

INFO_SCREEN pdscreen = { 2,14,9,5,pdmsg };

/********************** Product Line Defs ********************************/

FIELDPARAMS lifields[] = {
    CT_CHAR,  3,  0, "%-2s",   "Prod.|Line",      1,  3,  3,  2, 11,
    CT_CHAR, 40,  3, "%-39s",  "Description|",    7,  3,  4, 39, 0
};

MOUSE_CLICK liPoints[] = {
    {112, 152, 168, 152, KEY_F1},
    {208, 152, 256, 152, KEY_F2},
    {296, 152, 344, 152, KEY_F3},
    {384, 152, 440, 152, KEY_F5},
    {480, 152, 536, 152, KEY_F10},
    {0, 0, 0, 0, 0}
};

DB_MSG limsg[] = { 1,24,"F1=HELP     F2=ADD     F3=DEL     F5=PRINT    F10=EXIT",0,0,NULL };

INFO_SCREEN liscreen = { 2,14,8,2,limsg };


/********************** Product Class Defs ********************************/

FIELDPARAMS clfields[] = {
    CT_CHAR,  5,  0, "%-4s",   "Class|",           1,  3,  3,  4, 11,
    CT_CHAR, 40,  5, "%-39s",  "Description|",     7,  3,  4, 39, 0
};

MOUSE_CLICK clPoints[] = {
    {112, 152, 168, 152, KEY_F1},
    {208, 152, 256, 152, KEY_F2},
    {296, 152, 344, 152, KEY_F3},
    {392, 152, 448, 152, KEY_F5},
    {496, 152, 552, 152, KEY_F10},
    {0, 0, 0, 0, 0}
};

FUNC_TABLE clfuncs[] = {0,gen_funcs,1,gen_funcs,-1,NULL };

DB_MSG clmsg[] = { 1,24,"F1=HELP     F2=ADD     F3=DEL      F5=PRINT     F10=EXIT",0,0,NULL };

INFO_SCREEN clscreen = { 34,14,8,2,clmsg };


/********************** Vendor Discount Defs ********************************/

FIELDPARAMS ddfields[] = {
    CT_CHAR,  17,  0, "%-16s", "Range|",         1,  3,  3, 16, 10,
    CT_CHAR,  8,  17, "%-7s",  "Discount|",     17,  3,  4,  7, 0
};

MOUSE_CLICK ddPoints[] = {
    {112, 152, 160, 152, KEY_F1},
    {184, 152, 224, 152, KEY_F2},
    {248, 152, 288, 152, KEY_F3},
    {312, 152, 368, 152, KEY_F10},
    {0, 0, 0, 0, 0}
};

FUNC_TABLE ddfuncs[] = {0,gen_funcs,1,gen_funcs,-1,NULL };

DB_MSG ddmsg[] = { 1,24,"F1=HELP  F2=ADD  F3=DEL  F10=EXIT",0,0,NULL };

INFO_SCREEN ddscreen = { 2,14,8,2,ddmsg };

/********************** Help Module Defs ********************************/

FIELDPARAMS hlfields[] = {
    CT_CHAR,  48,  0, "%-47s",  "Help Text|",      1, 3, 3, 47, 8,
    CT_CHAR,  0,  48, "%s",     " ",            0, 0, 0, 0,  0
};

FUNC_TABLE hlfuncs[] = {-1,NULL};

INFO_SCREEN hlscreen = { 2,14,8,2,pdmsg };

/********************** State Module Defs ********************************/

FIELDPARAMS stfields[] = {
    CT_CHAR,  3,  0, "%2s",    "Code|",      1, 3, 3, 2,  0,
    CT_CHAR, 22,  3, "%-21s",  "State|",     4, 3, 4, 21, 3
};

INFO_SCREEN stscreen = { 2,14,9,2,pdmsg };

/************************* Room Schedule *******************************/

FIELDPARAMS rsfields[] = {
    VD,      9,  0, "%-9s",     "VENDOR|",        1, 1, 1,  9,10,
    PD,     21,  9, "%-21.21s", "PRODUCT|",      11, 1, 1, 21, 2,
    CT_INT,  2, 61, "%-5d",     "",              34, 1, 1,  5, 0,
    CT_INT,  2, 63, "%-5d",     "",              40, 1, 1,  5, 0,
    CT_INT,  2, 65, "%-5d",     "",              46, 1, 1,  5, 0,
    CT_INT,  2, 67, "%-5d",     "",              52, 1, 1,  5, 0,
    CT_INT,  2, 69, "%-5d",     "",              58, 1, 1,  5, 0,
    CT_INT,  2, 71, "%-5d",     "",              64, 1, 1,  5, 0,
    CT_INT,  2, 73, "%-5d",     "",              70, 1, 1,  5, 0,
    CT_INT,  2, 75, "%-5d",     "TOTAL|",        76, 1, 1,  5, 0,
    CT_CHAR,21, 30, "%-21.21s", "DESCRIPTION|",  11, 1, 1, 21, 2
};

FUNC_TABLE rsfuncs[] = {
#ifndef REP
    0, rs_funcs,
    1, rs_funcs,
    2, rs_funcs,
    3, rs_funcs,
    4, rs_funcs,
    5, rs_funcs,
    6, rs_funcs,
    7, rs_funcs,
    8, rs_funcs,
    9, skipfield,
    KEY_F4, rs_funcs,
    KEY_F5, print_whole_file,
    KEY_F7, rs_funcs,
    KEY_F8, rs_funcs,
    KEY_F9, rs_funcs,
    KEY_ALT_F1, rs_funcs,
    KEY_ALT_F2, rs_funcs,
    KEY_CTRL_F2, rs_funcs,
    KEY_CTRL_LEFT, rs_funcs,
    KEY_CTRL_RIGHT, rs_funcs,
#endif
    -1,NULL
};

DB_MSG rsmsg[] = {
    1,24,"F1=HELP F2=ADD F3=DEL F4=PDDS F5=PRINT F6=FIND F7=ROOMS F8=PL  F9=FORM  F10=EXIT",
    0,0,NULL
};

INFO_SCREEN rsscreen = { 0,0,0,0,rsmsg };

/****************** Client Database *****************************/

FIELDPARAMS clientfields[] = {
    CD,        9,   0, "%-8s",    "Client|Code",   1,  3,  3,  8, 3,
    CT_CHAR,  33,   9, "%-32s",   "Client|Name",  12,  3,  4, 32, 2,
    CT_CHAR,  33,  42, "%-32s",   "Contact|1",     0,  3,  5, 32, 0,
    CT_CHAR,  33,  75, "%-32s",   "Contact|2",     0,  3,  6, 32, 0,
    CT_CHAR,  15, 179, "%-14s",   "Phone #|",     52,  3,  7, 14, 0,
    CT_CHAR,  15, 194, "%-14s",   "Fax #|",       67, 32,  7, 14, 0,
    CT_CHAR,  15, 283, "%-14s",   "Cell #|",       0, 54,  7, 14, 0, /* added on 06/16/00 */
	CT_CHAR,  49, 234, "%-48s",   "EMail|Addr",    0,  3,  8, 38, 0, /* added on 05/28/00 */
	CT_CHAR,  11, 298, "%-10s",   "Password",      0, 54,  8, 10, 0, /* added on 06/16/00 */
    CT_CHAR,  41, 108, "%-40s",   "Address|",      0,  3,  9, 40, 0,
    CT_CHAR,  16, 149, "%-15s",   "City   |",      0,  3, 10, 15, 0,
    ST,        3, 165, "%-2s",    "State|",        0, 32, 10,  2, 0,
    CT_CHAR,  11, 168, "%-12s",   "Zip|",          0, 54, 10, 10, 0,
	CT_CHAR,  33, 309, "%-32s",   "Home|http Addr",0,  3, 11, 32, 0, /* added on 07/08/00 */
    CT_FLOAT,  4, 209, "%-10.2f", "YTD|Shipped",   0, 54,  3, 10, 0,
    CT_FLOAT,  4, 217, "%-10.2f", "YTD|On Order",  0, 54,  4, 10, 0,
    CT_FLOAT,  4, 213, "%-10.2f", "Total|Sales",   0, 54,  5, 10, 0,
    CT_CHDATE, 9, 225, "%-8s",    "Last|Purchase", 0, 54,  6,  8, 0
};

FUNC_TABLE clientfuncs[] = {
    KEY_ALT_F2,allcomments,
    KEY_F9,letters,
    -1,NULL
};

DB_MSG clientmsg[] = {
    25,1,"CLIENT DATABASE INPUT SCREEN",
    2,13," Code     Client Name                      Telephone     FAX",
    2,14,"------------------------------------------------------------------------------",
    1,24,"F1=HELP F2=ADD F3=DEL         F5=PRINT F6=FIND                  F9=FORM F10=EXIT",
    0,0,NULL
};

INFO_SCREEN clientscreen = { 2,15,8,4,clientmsg };

/******************** Sales Person Database ********************************/

FIELDPARAMS spfields[] = {
    SP,       4,  0, "%-3s",    "SP|Code",         1,  3,  3,  3, 3,
    CT_CHAR, 33,  4, "%-32s",   "SP|Name",         6,  3,  4, 32, 2,
    CT_CHAR, 12, 37, "%-11s",   "SSN#|",          40,  3,  5, 11, 0,
    CT_CHAR, 31,176, "%-30s",   "Title|",          0,  3,  6, 30, 0, /* added on 11/28/96 */
    CT_CHAR, 15,136, "%-14s",   "Telephone|",     60,  3,  7, 14, 0,
	CT_CHAR, 49,207, "%-48s",	"EMail|Addr",	   0,  3,  8, 48, 0, /* added on 05/21/00 */
    CT_CHAR, 41, 65, "%-38s",   "Address|",        0,  3,  9, 38, 0,
    CT_CHAR, 16,106, "%-15s",   "City   |",        0,  3, 10, 15, 0,
    ST,       3,122, "%-2s",    "State|",          0, 32, 10,  2, 0,
    CT_CHAR, 11,125, "%-10s",   "Zip|",            0, 52, 10, 10, 0,
    CT_FLOAT, 4,151, "%-10.2f", "YTD|Sales",       0, 52,  3, 10, 0,
    CT_FLOAT, 4,155, "%-10.2f", "Last|Year",       0, 52,  4, 10, 0,
    CT_FLOAT, 4, 49, "%-11.2f", "Total|Sales",     0, 52,  5, 11, 0,
    CT_FLOAT3,4, 57, "%-6.3f",  "Commision|Rate",  0, 52,  6,  6, 0,
    CT_FLOAT, 4, 53, "%-10.2f", "Commision|Due",   0, 52,  7, 10, 0,
    CT_FLOAT, 4, 61, "%-10.2f", "Commision|Paid",  0, 52,  9, 10, 0
};

FUNC_TABLE spfuncs[] = {
    KEY_ALT_F2,allcomments,
    KEY_F4,sp_pay,
    KEY_F9,letters,
    -1,NULL
};

DB_MSG spmsg[] = {
    25,1,"  SALES PERSON INPUT SCREEN",
    2,12," Code   Name                          SSN #       Telephone",
    2,13,"------------------------------------------------------------------------------",
    1,24,"F1=HELP F2=ADD F3=DEL F4=PAY  F5=PRINT F6=FIND                  F9=FORM F10=EXIT",
    0,0,NULL
};

INFO_SCREEN spscreen = { 2,14,9,4,spmsg };

/********************** Job Database *********************************/
#ifndef REP_VER
FIELDPARAMS jbfields[] = {
    JB,        9,   0, "%-8s",  "Job|Code",     1,  3,  3,  8, 5,
    CT_CHAR,  33,   9, "%-32s", "Job|Name",    10,  3,  4, 32, 2,
    CT_CHAR,  33,  42, "%-32s", "Comments|",    0,  3,  5, 32, 0,
    CT_CHAR,  33, 108, "%-32s", "Architect|",   0,  3,  6, 32, 0,
	CT_CHAR,  33,  75, "%-32s", "Project|Mgr.", 0,  3,  7, 32, 0,
	CT_CHAR,  49, 352, "%-48s", "Mgr.|Mail",    0,  3,  8, 38, 0,	/* Added on 6/18/00 */
    CT_CHAR,  33, 141, "%-32s", "Job|Super",    0,  3,  9, 32, 0,
	CT_CHAR,  49, 416, "%-48s", "Super|Mail",   0,  3, 10, 38, 0,	/* Added on 6/18/00 */
	CT_CHDATE, 9, 465, "%-8s",  "Req.|Date",    0,  3, 11,  8, 0,	/* Added on 7/8/00 */
	CT_CHAR,   4, 474, "%-3s",  "Req.|Number",  0, 30, 11,  3, 0,	/* Added on 7/8/00 */
    CT_CHAR,  41, 174, "%-40s", "Address|",     0,  3, 12, 40, 0,
    CT_CHAR,  16, 215, "%-15s", "City   |",    43,  3, 13, 15, 0,
    ST,        3, 231, "%-2s",  "State|",       0, 30, 13,  2, 0,
    CT_CHAR,  11, 306, "%-10s", "Zip|",         0, 54, 13, 10, 0,
/*    CT_CHAR,   8, 321, "%-7s",  "Addendum|",    0, 30,  3,  7, 0, */
    CT_CHAR,   4, 234, "%-3s",  "Addendum|",    0, 30,  3,  3, 0,
    CT_CHAR,   8, 329, "%-7s",  "Client|PO #",  0, 54,  3,  7, 0,
    CT_CHAR,   2, 238, "%-1s",  "Status|",     59, 54,  4,  1, 0,
    CT_CHDATE, 9, 288, "%-8s",  "Due|Date",    68, 54,  5,  8, 0,
    CT_CHAR,  15, 272, "%-14s", "Arch.|Tel",    0, 54,  6, 14, 0,
	CT_CHDATE, 9, 478, "%-8s",  "Substantial|Date",  0, 54,  7,  8, 0,	/* Added on 6/18/00 */
	CT_CHAR,  15, 337, "%-14s", "Mgr.|Cell",    0, 54,  8, 14, 0,	/* Added on 6/18/00 */
    CT_CHDATE, 9, 297, "%-8s",  "Ship|Date",    0, 54,  9,  8, 0,
	CT_CHAR,  15, 401, "%-14s", "Super|Cell",   0, 54, 10, 14, 0,	/* Added on 6/18/00 */
    CT_CHAR,  15, 240, "%-14s", "Site|Tel.",    0, 54, 11, 14, 0,
    CT_CHAR,  15, 256, "%-14s", "Site|Fax.",    0, 54, 12, 14, 0,
};

FUNC_TABLE jbfuncs[] = {
    8, skipfield,	/* Requisition Date */
    9, skipfield,	/* Requisition Number */
    16, skipfield,	/* Status */
    KEY_F1, jb_help,
    KEY_ALT_F2, allcomments,
    KEY_F3, jb_delete,
    KEY_F4, jb_es,
    KEY_F7, jb_stat,
    KEY_F8, jb_po,
    KEY_F9, jb_letters,
    -1,NULL
};

DB_MSG jbmsg[] = {
    25,1,"     INFORMATION INPUT SCREEN",
    2,15," Code     Name                             Location        ST Bid Due",
    2,16,"------------------------------------------------------------------------------",
    1,24,"F1=HELP F2=ADD F3=DEL F4=QUOT F5=PRINT F6=FIND F7=STATS F8=PO   F9=FORM F10=EXIT",
    0,0,NULL
};

INFO_SCREEN jbscreen = { 2,17,6,5,jbmsg };
#else
FIELDPARAMS jbfields[] = {
    JB,        9,   0, "%-8s",  "Job|Code",     1,  3,  3,  8, 5,
    CT_CHAR,  33,   9, "%-32s", "Job|Name",    10,  3,  4, 32, 2,
    CT_CHAR,  33,  42, "%-32s", "Comments|",    0,  3,  5, 32, 0,
    CT_CHAR,  33, 108, "%-32s", "Architect|",   0,  3,  6, 32, 0,
    CT_CHAR,  33,  75, "%-32s", "Project|Mgr.", 0,  3,  7, 32, 0,
    CT_CHAR,  33, 141, "%-32s", "Job|Super",    0,  3,  8, 32, 0,
    CT_CHAR,  41, 174, "%-40s", "Address|",     0,  3,  9, 40, 0,
    CT_CHAR,  16, 215, "%-15s", "City   |",    43,  3, 10, 15, 0,
    ST,        3, 231, "%-2s",  "State|",       0, 30, 10,  2, 0,
    CT_CHAR,  11, 306, "%-10s", "Zip|",         0, 54, 10, 10, 0,
    CT_CHAR,   8, 321, "%-7s",  "Addendum|",    0, 30,  3,  7, 0,
    CT_CHAR,   8, 329, "%-7s",  "Client|PO #",  0, 54,  3,  7, 0,
    CT_CHAR,   2, 238, "%-1s",  "Status|",     59, 54,  4,  1, 0,
    CT_CHDATE, 9, 288, "%-8s",  "Due|Date",    68, 54,  5,  8, 0,
    CT_CHAR,  15, 272, "%-14s", "Arch.|Tel",    0, 54,  6, 14, 0,
    CT_CHDATE, 9, 297, "%-8s",  "Ship|Date",    0, 54,  7,  8, 0,
    CT_CHAR,  15, 240, "%-14s", "Site|Tel.",    0, 54,  8, 14, 0,
    CT_CHAR,  15, 256, "%-14s", "Site|Fax.",    0, 54,  9, 14, 0,

};

FUNC_TABLE jbfuncs[] = {
    12, skipfield,
    KEY_F1, jb_help,
    KEY_ALT_F2, allcomments,
    KEY_F3, jb_delete,
    KEY_F4, jb_es,
    KEY_F7, jb_stat,
/*    KEY_F8, jb_po, */
    KEY_F9, jb_letters,
    -1,NULL
};

DB_MSG jbmsg[] = {
    25,1,"     INFORMATION INPUT SCREEN",
    2,12," Code     Name                             Location        ST Bid Due",
    2,13,"------------------------------------------------------------------------------",
    1,24,"F1=HELP F2=ADD F3=DEL F4=QUOT F5=PRINT F6=FIND F7=STATS         F9=FORM F10=EXIT",
    0,0,NULL
};
INFO_SCREEN jbscreen = { 2,14,9,5,jbmsg };
#endif

#ifndef REP_VER

/********************** Estimate Database ******************************/

FIELDPARAMS esfields[] = {
    JB,        9,   0, "%-8s",   "Job|Code",       1,  3,  3,  8, 3,
    CT_CHAR,  33, 124, "%-32s",  "Job|Name",      30,  3,  4, 32, 2,
    CD,        9,   9, "%-8s",   "Client|Code",   10,  3,  5,  8, 2,
    SP,        4,  18, "%-3s",   "SP|Code",       21,  3,  6,  3, 0,
    CT_CHAR,  25,  22, "%-24s",  "Client|Contact", 0,  3,  7, 24, 0,
    CT_CHAR,  15,  55, "%-14s",  "Client|Tel.",    0,  3,  8, 14, 0,
    CT_FLOAT3, 4,  75, "%-8.5f", "Tax|Rate",       0,  3,  9,  8, 0,
    CT_CHDATE, 9,  79, "%-8s",   "Est|Date",      72,  3, 10,  8, 0,
    CT_FLOAT3, 4,  71, "%-8.3f", "Markup|Rate",    0, 27, 10,  8, 0,
    CT_FLOAT,  4,  88, "%-9.2f", "COGS|",          0, 54,  3,  9, 0,
    CT_FLOAT,  4,  92, "%-9.2f", "Markup|",        0, 54,  4,  9, 0,
    CT_FLOAT,  4,  96, "%-9.2f", "Commission|",    0, 54,  5,  9, 0,
    CT_FLOAT,  4, 108, "%-9.2f", "SubTotal|",      0, 54,  6,  9, 0,
    CT_FLOAT,  4, 100, "%-8.2f", "Est|Tax",        0, 54,  7,  8, 0,
    CT_FLOAT,  4, 104, "%-8.2f", "Est|Ship.",      0, 54,  8,  8, 0,
    CT_FLOAT,  4, 112, "%-9.2f", "Bid|Price",      0, 54,  9,  9, 0,
    CT_FLOAT,  4, 116, "%-9.2f", "Profit|",        0, 54, 10,  9, 0,
    CT_CHAR,   2, 160, "%-1s",   "Status|",       64, 30,  3,  1, 0
};

FUNC_TABLE esfuncs[] = {
    1, es_funcs,                    /* F1 Gets Job Name from JB file.   */
    2, es_funcs,                    /* Gives CD help.                   */
    14, skipfield,                  /* Skips Shipping Field.            */
    8, skipfield,                   /* Skip Average Markup Rate Field.  */
    17, skipfield,                  /* Skip Status Field. */
    KEY_F2, es_funcs,               /* Copy this Estimate.              */
    KEY_F3, es_funcs,               /* Delete current record & it's PL. */
    KEY_F4, es_funcs,               /* Re-Calc prices.                  */
    KEY_F7, es_funcs,               /* Award job to this Client.        */
    KEY_F8, es_funcs,               /* Goto Part List.                  */
    KEY_F9, esletters,              /* New ES letters.                  */
    KEY_ALT_F2, allcomments,        /* Job Comments.                    */
    KEY_ALT_F8, es_room_schedule,   /* Room Schedule.                   */
    -1,NULL
};

DB_MSG esmsg[] = {
    29,1,"QUOTATION INPUT SCREEN",
    2,12," Job      Client   SP  Job Name                        ST Follow-Up Date",
    2,13,"------------------------------------------------------------------------------",
    1,24,"F1=HELP F2=ADD F3=DEL F4=CALC F5=PRINT F6=FIND F7=AWARD F8=PL   F9=FORM F10=EXIT",
    0,0,NULL
};

INFO_SCREEN esscreen = { 2,14,9,6,esmsg };

/************************* Job Installation *******************************/

FIELDPARAMS infields[] = {
    VD,      9,  0, "%-9s",     "INSTALLER|",     1, 1, 1,  9,10,
    PO,      9,  9, "%-9s",     "",              12, 1, 1,  9, 0,
    PO,      9, 18, "%-9s",     "",              22, 1, 1,  9, 0,
    PO,      9, 27, "%-9s",     "",              32, 1, 1,  9, 0,
    PO,      9, 36, "%-9s",     "",              42, 1, 1,  9, 0,
    PO,      9, 45, "%-9s",     "",              52, 1, 1,  9, 0,
    PO,      9, 54, "%-9s",     "",              62, 1, 1,  9, 0,
    PO,      9, 63, "%-9s",     "",              72, 1, 1,  9, 0,
    CT_CHAR, 9, 72, "%-9.9s",   "",              12, 1, 1,  9, 2,
    CT_CHAR, 9,105, "%-9.9s",   "",              22, 1, 1,  9, 2,
    CT_CHAR, 9,138, "%-9.9s",   "",              32, 1, 1,  9, 2,
    CT_CHAR, 9,171, "%-9.9s",   "",              42, 1, 1,  9, 2,
    CT_CHAR, 9,204, "%-9.9s",   "",              52, 1, 1,  9, 2,
    CT_CHAR, 9,237, "%-9.9s",   "",              62, 1, 1,  9, 2,
    CT_CHAR, 9,270, "%-9.9s",   "",              72, 1, 1,  9, 2 
};

FUNC_TABLE infuncs[] = {
#ifndef REP
    0, in_funcs,
    1, in_funcs,
    2, in_funcs,
    3, in_funcs,
    4, in_funcs,
    5, in_funcs,
    6, in_funcs,
    7, in_funcs, 
    KEY_F4, in_funcs,
    KEY_F5, print_whole_file,
    KEY_F7, in_funcs,
    KEY_F8, in_funcs,
    KEY_F9, in_funcs, 
#endif
    -1,NULL
};

DB_MSG inmsg[] = {
    1,24,"F1=HELP F2=ADD F3=DEL F4=NAME F5=PRINT F6=FIND F7=PREV F8=NEXT F9=FORM  F10=EXIT",
    0,0,NULL
};

INFO_SCREEN jbinscreen = { 0,0,0,0,inmsg };


#else

/*****************************************************************************
                      REP version Structures
*****************************************************************************/

/********************** Quotation Database ******************************/

FIELDPARAMS esfields[] = {
/*  JB,        9,   0, "%-8s",   "Job|Code",       1,  3,  3,  8, 3,
    CT_CHAR,  33, 124, "%-32s",  "Job|Name",      30,  3,  4, 32, 2,
    CD,        9,   9, "%-8s",   "Client|Code",   10,  3,  5,  8, 2,
    SP,        4,  18, "%-3s",   "SP|Code",       21,  3,  6,  3, 0,
    CT_CHAR,  25,  22, "%-24s",  "Client|Contact", 0,  3,  7, 24, 0,
    CT_CHAR,  15,  55, "%-14s",  "Estr.|Tel.",     0,  3,  8, 14, 0,
    CT_CHDATE, 9,  79, "%-8s",   "Quote|Date",    64, 3,  10,  8, 0,
    CT_FLOAT,  4,  88, "%-9.2f", "COGS|",          0, 54,  3,  9, 0,
    CT_FLOAT,  4,  96, "%-9.2f", "SP|Commission|", 0, 54,  5,  9, 0,
    CT_FLOAT,  4, 162, "%-8.2f", "Total|Weight",   0, 54,  6,  8, 0,
    CT_FLOAT,  4, 104, "%-8.2f", "Est|Shipping",   0, 54,  8,  8, 0      */
    
    JB,        9,   0, "%-8s",   "Job|Code",       1,  3,  3,  8, 3,
    CT_CHAR,  33, 124, "%-32s",  "Job|Name",      30,  3,  4, 32, 2,
    CD,        9,   9, "%-8s",   "Client|Code",   10,  3,  5,  8, 2,
    SP,        4,  18, "%-3s",   "SP|Code",       21,  3,  6,  3, 0,
    CT_CHAR,  25,  22, "%-24s",  "Client|Contact", 0,  3,  7, 24, 0,
    CT_CHAR,  15,  55, "%-14s",  "Client|Tel.",    0,  3,  8, 14, 0,
/*  CT_FLOAT3, 4,  75, "%-8.3f", "Tax|Rate",       0,  3,  9,  8, 0, 
    CT_CHDATE, 9,  79, "%-8s",   "Est|Date",      64,  3, 10,  8, 0, */
    CT_CHDATE, 9,  79, "%-8s",   "Est|Date",      72, 54,  4,  8, 0,
/*  CT_FLOAT3, 4,  71, "%-8.3f", "Markup|Rate",    0, 27, 10,  8, 0, 
    CT_FLOAT,  4,  88, "%-9.2f", "COGS|",          0, 54,  3,  9, 0, */
    CT_FLOAT,  4,  88, "%-9.2f", "COGS|",          0, 54,  5,  9, 0,
/*  CT_FLOAT,  4,  92, "%-9.2f", "Markup|",        0, 54,  4,  9, 0, */ 
    CT_FLOAT,  4,  96, "%-9.2f", "Commission|",    0, 54,  6,  9, 0,
/*  CT_FLOAT,  4, 108, "%-9.2f", "SubTotal|",      0, 54,  6,  9, 0,
    CT_FLOAT,  4, 100, "%-8.2f", "Est|Tax",        0, 54,  7,  8, 0,  
    CT_FLOAT,  4, 104, "%-8.2f", "Est|Ship.",      0, 54,  8,  8, 0,
    CT_FLOAT,  4, 112, "%-9.2f", "Bid|Price",      0, 54,  9,  9, 0, */
    CT_FLOAT,  4, 104, "%-8.2f", "Est|Ship.",      0, 54,  7,  8, 0,
    CT_FLOAT,  4, 112, "%-9.2f", "Bid|Price",      0, 54,  8,  9, 0, 

/*  CT_FLOAT,  4, 116, "%-9.2f", "Profit|",        0, 54, 10,  9, 0, */ 
    CT_CHAR,   2, 160, "%-1s",   "Status|",       64, 30,  3,  1, 0

};

FUNC_TABLE esfuncs[] = {
/*  1,es_funcs,              F1 Gets Job Name from JB file.   
    2,es_funcs,              Gives Client help.               
    KEY_F2,es_funcs,         Copy this Estimate.              
    KEY_F3,es_funcs,         Delete current record & it's PL. 
    KEY_F4,es_funcs,         Re-Calc prices.                  
    KEY_F7,es_funcs,         Award job to this Client.        
    KEY_F8,es_funcs,         Goto Part List.                  
    KEY_F9,esletters,        New ES letters.                  
    KEY_ALT_F2,allcomments,  Job Comments.                    
    -1,NULL                                                    */

    1, es_funcs,                    /* F1 Gets Job Name from JB file.   */
    2, es_funcs,                    /* Gives CD help.                   */
    11, skipfield,                  /* Skip Status Field. */
    KEY_F2, es_funcs,               /* Copy this Estimate.              */
    KEY_F3, es_funcs,               /* Delete current record & it's PL. */
    KEY_F4, es_funcs,               /* Re-Calc prices.                  */
    KEY_F7, es_funcs,               /* Award job to this Client.        */
    KEY_F8, es_funcs,               /* Goto Part List.                  */
    KEY_F9, esletters,              /* New ES letters.                  */
    KEY_ALT_F2, allcomments,        /* Job Comments.                    */
    KEY_ALT_F8, es_room_schedule,   /* Room Schedule.                   */
    -1,NULL

};

DB_MSG esmsg[] = {
    29,1,"QUOTATION INPUT SCREEN",
    2,11," Job      Client   SP  Job Name                        ST Follow-Up Date",
    2,12,"------------------------------------------------------------------------------",
    1,24,"F1=HELP F2=ADD F3=DEL F4=CALC F5=PRINT F6=FIND F7=AWARD F8=PL   F9=FORM F10=EXIT",
    0,0,NULL
};

INFO_SCREEN esscreen = { 2,13,10,6,esmsg };

#endif

/**************************************************************************
DATA BASE STRUCTURE:
{file handle},{filename},{extension},{protection},{n records},{# fields},
    {file header length},{record size},{name},{field struct},
    {func struct},{screen struct},{msg function},{init function},
    {screen init func},{screen setup func},{help window func},
    {cursor movement func},{validation function},{newfile function}
*/

DB_INFO db[] = {

    /*Help Module Definitions*/         /*48*/
    -1, "DBHELP  ", ".\\HELP\\","00 ",0x0180,0,1,2,sizeof(HLREC),"HELP MODULE",
        mouseClicks, hlfields, hlfuncs, &hlscreen, msg_def, pl_init,
        inscreen,insetup,NULL,directions,val_def,NULL,

    /*Client Database*/                 /*219*/
    -1,"AUTOMAT ","","CD ",0x0180,0,18,2,sizeof(CDREC),"CLIENT",
        mouseClicks, clientfields, clientfuncs, &clientscreen, msg_def, NULL,
        inpt_screen,s_inset,s_inhelpset,s_directions,val_def,NULL,

    /*Vendor*/                            /*266*/
#ifndef REP_VER
    -1,"AUTOMAT ","","VD ",0x0180,0,21,2,sizeof(VDREC),"VENDOR",
        mouseClicks, vdfields, vdfuncs, &vdscreen, msg_def, NULL,
        inpt_screen,s_inset,s_inhelpset,s_directions,vd_val,NULL,
#else
    -1,"AUTOMAT ","","VD ",0x0180,0,16,2,sizeof(VDREC),"VENDOR",
        mouseClicks, vdfields, vdfuncs, &vdscreen, msg_def, NULL,
        inpt_screen,s_inset,s_inhelpset,s_directions,vd_val,NULL,
#endif

    /* Product */                           /*129*/
    -1,"AUTOMAT ",".\\PRODUCTS\\","PD ",0x0180,0,16,2,sizeof(PDREC),"PRODUCT",
        mouseClicks, pdfields, pdfuncs, &pdscreen, msg_def, pd_new,
        inpt_screen,s_inset,s_inhelpset,s_directions,val_def,newfile,

    /* Sales Person */                      /*160*/
    -1,"AUTOMAT ","","SP ",0x0180,0,16,2,sizeof(SPREC),"SALES PERSON",
        mouseClicks, spfields, spfuncs, &spscreen, msg_def, NULL,
        inpt_screen, s_inset, s_inhelpset, s_directions, val_def, NULL,

    /* Job */                               /*337*/
    -1,"AUTOMAT ","","JB ",0x0180,0,25,2,sizeof(JBREC),"JOB",
        mouseClicks, jbfields, jbfuncs, &jbscreen, msg_def,goto_last_job,
        inpt_screen,s_inset,s_inhelpset,s_directions,jb_val,NULL,

    /* Estimate */                          /*124*/
#ifndef REP_VER
    -1,"AUTOMAT ","","ES ",0x0180,0,18,2,sizeof(ESREC),"ESTIMATE",
        mouseClicks, esfields, esfuncs, &esscreen, esmessage, goto_last_job,
        inpt_screen,s_inset,s_inhelpset,s_directions,val_def,NULL,
#else
    -1,"AUTOMAT ","","ES ",0x0180,0,12,2,sizeof(ESREC),"ESTIMATE",
        mouseClicks, esfields, esfuncs, &esscreen, esmessage, goto_last_job,
        inpt_screen,s_inset,s_inhelpset,s_directions,val_def,NULL,
#endif

    /* Part list*/                        /*82*/
    -1,"AUTOMAT ",".\\PLIST\\","PL ",0x0180,0,6,2,sizeof(PLREC),"PRODUCT LIST",
        mouseClicks, plfields, plfuncs, &plscreen, plmessage, pl_init,
        inscreen, insetup, NULL, directions, pl_val, newfile,

    /* Purchase Orders */                 /*203*/
    -1,"AUTOMAT ","","PO ",0x0180,0,24,2,sizeof(POREC),"PURCHASE ORDER",
        mouseClicks, pofields, pofuncs, &poscreen, po_message, goto_last_job,
        inpt_screen,s_inset,s_inhelpset,s_directions,val_def,NULL,

    /* user definitions*/                 /*60*/
    -1,"AUTOMAT ","","US ",0x0180,0,22,2,sizeof(USREC),"USER DEFINITIONS",
        mouseClicks, usfields, usfuncs, &usscreen, us_msg, pl_init,
        inscreen,insetup,NULL,directions,val_def,NULL,

    /* Product Line definitions */         /*43*/
    -1,"AUTOMAT ","","LI ",0x0180,0,2,2,sizeof(LIREC),"LINE DESCRIPTION",
        liPoints, lifields, clfuncs, &liscreen, msg_def, pl_init,
        inscreen,insetup,NULL,directions,val_def,newfile,

    /* Product Class definitions */        /*45*/
    -1,"AUTOMAT ","","CL ",0x0180,0,2,2,sizeof(CLREC),"CLASS DESCRIPTION",
        clPoints, clfields, clfuncs, &clscreen, msg_def, pl_init,
        inscreen,insetup,NULL,directions,val_def,newfile,

    /* Vendor Discount definitions */     /*13*/
    -1,"AUTOMAT ","","DS ",0x0180,0,2,2,sizeof(DDREC),"QUANTITY DISCOUNT",
        ddPoints, ddfields, ddfuncs, &ddscreen, msg_def, pl_init,
        inscreen, insetup, NULL, directions, val_def, newfile,

    /* Room Schedule */                   /*??*/
    -1,"AUTOMAT ",".\\RSCH\\","RS ",0x0180,0,10,72,sizeof(RSREC),"ROOM SCHEDULE",
        mouseClicks, rsfields, rsfuncs, &rsscreen, plmessage, pl_init,
        inscreen, insetup, NULL, directions, val_def, newfile,

    /* State Definitions */              /* 25 */
    -1,"AUTOMAT ","","ST ",0x0180,0,2,2,sizeof(STREC),"STATE MODULE",
        mouseClicks, stfields, hlfuncs, &stscreen, msg_def, NULL,
        inpt_screen, s_inset, s_inhelpset, s_directions, val_def, NULL,

    /* Job Installation */               /*366*/
    -1,"AUTOMAT ",".\\INST\\","IN ",0x0180,0,8,10,sizeof(INREC),"JOB INSTALLATION",
        mouseClicks, infields, infuncs, &jbinscreen, msg_def, pl_init,
        inscreen, insetup, NULL, directions, val_def, NULL,

    /* Final NULL record */
    -1, "", "", "", 0,0,0,0,0, "",
        NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL
};
