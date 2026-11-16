/**
*   DB_DEFS.H   - Global include for Kwik Kuote
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
*   3.00a       09-06-93     BCW        Revision 3
*
**/
#ifndef DB_DEFS_H
#define DB_DEFS_H

#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include "stdinc.h"

/****     Define Version as needed      *****/
/*#define XDEMO*/
/*#define DEMO*/
/*#define REP*/

/********************** CONSTANT DECLARATIONS ******************************/

#define MAXVENDORS   40  /* Can only have a Max of 40 Vendors in any PL. (as of 5/25/98) */
#define MAXVENDORS_PER_MARKUPWINDOW 20 /* Can only hold 20 VD in the markup window */
#define MAXESTIMATES 15  /* Can only have a Max of 15 Estimates per Job. */

/*
* Help Module file extension IDs
*/
#define HLP_TEXTEDIT  40
#define HLP_PRINTMENU 41
#define HLP_MAINMENU  42
#define HLP_REPORT    43
#define HLP_INFOMENU  44
#define HLP_SYSMENU   45
#define HLP_FILEMENU  46
#define HLP_SETUPMENU 47
#define HLP_HELPEDIT  48
#define HLP_SETRATE   49
#define HLP_COMPINFO  50

#define HLP_REP_MAINMENU        60

#ifndef DEMO
  #define HLP_END HLP_SETRATE
#else  /* DEMO */
  #define HLP_SCREEN1 42
  #define HLP_SCREEN2 44
  #define HLP_SCREEN3 40
  #define HLP_SCREEN4 41
  #define HLP_SCREEN5 43
  #define HLP_SCREEN6 45
  #define HLP_END HLP_SCREEN6
#endif /* DEMO */

/*
* comparison definitions
*/
#define EQ    0
#define GE    1
#define GT    2
#define NE    3 
#define LE   -1
#define LT   -2
#define NONE -3

/*
* Compare type defs
*/
#define CT_CHAR     -1
#define CT_INT      -2
#define CT_LONG     -3
#define CT_FLOAT    -4
#define CT_DOUBLE   -5
#define CT_DATE     -6
#define CT_CHDATE   -7
#define CT_PASSWORD -8
#define CT_FLOAT3   -9

/*
* database identifiers
*/
typedef UBYTE DB_TYPE;
#define HL  0           /* Help Module */
#define CD  1           /* Client Database */
#define VD  2           /* Vendor */
#define PD  3           /* Product Lists */
#define SP  4           /* Sales Person*/
#define JB  5           /* Job */
#define ES  6           /* Estiamtes */
#define PL  7           /* Part List */
#define PO  8           /* Purchase Orders */
#define US  9           /* User Defs */
#define LI 10           /* Product Line Defs */
#define CL 11           /* Product Class Defs */
#define DS 12           /* Vendor Discount Schedule */
#define RS 13           /* Room Schedule */
#define ST 14           /* State Help */
#define IN 15           /* Job Installation */
#define DB_END IN
#define RP (DB_END+1)   /* Reports Module - not used in this version? */

/*
* Color codes for textattr()
*/
#define ON_BLACK            0
#define ON_BLUE            16
#define ON_GREEN           32
#define ON_CYAN            48
#define ON_RED             64
#define ON_MAGENTA         80
#define ON_BROWN           96
#define ON_LIGHTGRAY      112
#define ON_DARKGRAY       128
#define ON_LIGHTBLUE      144
#define ON_LIGHTGREEN     160
#define ON_LIGHTCYAN      176
#define ON_LIGHTRED       192
#define ON_LIGHTMAGENTA   208
#define ON_YELLOW         224
#define ON_WHITE          240

/*
* Video definitions
*/
#define REVERSE   WHITE|ON_CYAN
#define NORMAL    BLACK|ON_LIGHTGRAY

/*
* The following flags for message() can be combined with |
*/
#define M_SCR 0x01      /* Send message to screen (message bar) */
#define M_KEY 0x02      /* get and return keystroke */
#define M_ERR 0x04      /* send message to error log (AUTOMAT.ERR) */

/*
* Definations moved from db.c
*/
#define PURCHASE 0
#define RELEASE 1
#define SUBCONTRACT_AGREEMENT 2

/*
* Shipping name <=> index map used in PO screen
char *ShippingCompanyIndex[] = {
	"1 FedEx",
	"2 UPS",
	"3 DHL",
	"4 EMS",
	"5 US Post",
	NULL
};
*/

/*****************************************************************************
DATA BASE STRUCTURE DEFINITIONS
*****************************************************************************/

/*
* structure for points where mouse can be used to select keys
* used by ConvertMouseClick() in INPT.C
*/
typedef struct {
    USHORT xStart;
    USHORT yStart;
    USHORT xEnd;
    USHORT yEnd;
    USHORT key;

} MOUSE_CLICK;

/*
* input position structure
*/
typedef struct {
    SHORT x, last, top, ypos, y, set, read, rc, flags;
} IN_POS;

/*
* field definitions structure
*/
typedef struct {
    SHORT type, bytes, offset;
    CHAR format[9], header[25];
    SHORT x, dx, dy, width, sort;
} FIELDPARAMS;

/*
* user function structure
*/
typedef struct {
    SHORT scan;         /* key that triggers function */
    INT (*func)(DB_TYPE, IN_POS *, VOID *);      /* function */
} FUNC_TABLE;

/*
* message structure
*/
typedef struct {
    SHORT x,y;          /* coordinates of message */
    CHAR *msg;          /* message */
} DB_MSG;

/*
* info screen parameters
*/
typedef struct {
    SHORT x;        /* x coordinate for upper left */
    SHORT y;        /* y coordinate for upper left */
    SHORT num;      /* number of records to display */
    SHORT fld;      /* number of fields to display */
    DB_MSG *msg;    /* screen messages */
} INFO_SCREEN;


/*
* database info structure
*/
typedef struct {
    INT file;                   /* file handle */
    CHAR fname[9];              /* filename */
    CHAR path[30];              /* path to file */
    CHAR extension[4];          /* file extension */
    INT perm;                   /* permissions */
    USHORT n;                   /* number of records in file (64k max)*/
    SHORT fields;               /* fields in data base */
    LONG fhead;                 /* length of file header (bytes) */
    SHORT fsize;                /* length of a record (bytes) */
    CHAR title[20];             /* title of data base */
    MOUSE_CLICK *mousePoints;   /* list of click points for mouse */
    FIELDPARAMS *f;             /* field paramter structure */
    FUNC_TABLE *t;              /* user functions */
    INFO_SCREEN *s;             /* info screen paramters */
    INT (*msg)(CHAR *);         /* message bar handler */
    INT (*init_func)(DB_TYPE, IN_POS *, VOID *);   /* initialization function */
    INT (*scr_func)(DB_TYPE);       /* screen initialization function */
    INT (*set_func)(DB_TYPE, IN_POS *, VOID *);    /* data printing function */
    INT (*help_func)(DB_TYPE, INT, INT, INT, INT, INT);   /* help screen function */
    IN_POS (*dir_func)(DB_TYPE, IN_POS *, VOID *); /* cursor movement function */
    INT (*val_func)(VOID *);    /* record validation function */
    INT (*newfile)(DB_TYPE);        /* user interraction for new filename (print menu) */
} DB_INFO;

/************************* GLOBAL VARIABLES ********************************/

extern DB_INFO db[];

/************************ DB structures ************************************/

#define WEB_CHANGED		0x8000	/* Higher half is for web use, lower half is for SP marking the job */

#define SP_CLEARMARK	0xFF00	/* This will clear the lower half bits */
#define	ACCEPT			0x0001
#define	DECLINE_DUP		0x0002
#define DECLINE_OTHER	0x0004


/*
* Record layouts are byte packed, as Borland C++ laid them out: existing
* data files depend on it.
*/
#pragma pack(push, 1)

/*
* Estimates Structure
*/
typedef struct {       
    CHAR jbcode[9], client_code[9], spcode[4], estname[25], future[4];
    FLOAT taxable;
    CHAR esttel[15], junk;
    FLOAT mkuprate, taxrate;
    CHAR estdate[9];
    FLOAT cogs, mkup, com, esttax, estship, subtotal, bid, profit;
    SHORT plvec, award;
    CHAR jobname[33], state[3], status[2];
    FLOAT weight, discount;
} ESREC;

/*
* Job List Structure
*/
typedef struct {       
    CHAR code[9], name[33], contact[33], agent[33], arch[33], pmgr[33], address[41];
    CHAR city[16], state[3], addendum[4], status[2], stel[15], cur_retn, sfax[15], pre_retn, artel[15], junk3;
    CHAR due[9], sent[9], zip[11];
    SHORT esvec, plvec;
    CHAR ch_od_code[8], po[8];
	/* The following fields are added on 6/13/00 */
	CHAR pmgrcell[15], pmgremail[49], supercell[15], superemail[49];
	CHAR req_date[9], req_number[4], substantial_date[9];
	SHORT marked;
} JBREC;

/*
* Temp Job List Structure before prove by sales person of ASI, added on 07/25/00
*/
typedef struct {
	JBREC jb;
	CHAR client_code[9], sales_code[4];
	CHAR dup_code[9], comment[360];
} JBREC_TMP;

/*
* Client Database Structure
*/
typedef struct {         
    CHAR code[9], name[33], contact1[33], contact2[33], address[41], city[16];
    CHAR state[3], zip[11], tel[15], fax[15];
    FLOAT ytd_sales, total_sales, ytd_on_order, future4;
    CHAR last_purchase_date[9];
    CHAR email[49]; /* added on 5/6/00 */
	CHAR cell[15], pswd[11];	/* Added on 6/13/00 */
	CHAR homesite[33];			/* Client's web home address, added on 7/8/00 */
	SHORT marked;				/* If record is changed by either the web user or the in-house user, added on 7/8/00 */
} CDREC;

/*
* Vendor List Structure
*/
typedef struct {         
    CHAR code[9], name[33], product[21], contact[33], agent[33], address[41];
    CHAR city[16], state[3];
    CHAR taxable[2];
    CHAR zip[11], tel[15], fax[15], atel[15], afax[15];
    FLOAT ytd_sales, total_sales, ytd_on_order, com_rate;
    CHAR last_active_date[9];
    /* The followings are added on 5/6/00 */
    CHAR homesite[33]; /* Vender's web home address */
    CHAR email[49], aemail[49];
} VDREC;

/*
* Product Defs Structure
*/
typedef struct {         
    CHAR code[21], vendor[11], description[31], clas[5], line[3];
    FLOAT weight, size, p1, p2, p3, sales;
    CHAR sh1[3], sh2[3], sh3[3];
    FLOAT rate;
    CHAR ref[21];
} PDREC;

/*
* Sales Person Structure
*/
typedef struct {         
    CHAR code[4], name[33], ssn[12];
    FLOAT total_sales, com_due, rate, paid;
    CHAR address[41], city[16], state[3], zip[11], tel[15];
    FLOAT ytd_sales, last_year_sales, future3, future4;
    CHAR future5[9];
    CHAR title[31]; /* this is added on 11/28/96 */
    CHAR email[49]; /* added on 5/6/00 */
} SPREC;

/*
* Part List Structure
*/
typedef struct {         
    CHAR flg;
    LONG qty;
    CHAR vdcode[9], pdcode[21], description[31];
    FLOAT price, f, weight, markup;
} PLREC;

/*
* Purchase Order Structure
*/
/*
* On 6/13/00:
* Split pre_retainage[10] to pre_retanige[4], pre_ret2[3] and pre_ret3[3],
* Split junk[40] to shipcorp[23], shipcorpindex (short, or 2 char), pre_ret4[3], pre_ret5[3]
* and material_po[9]
*/
typedef struct {         
    CHAR po_number[9], release_date[9], vd_inv_number[14];
    FLOAT cogs;
    CHAR inv_number[8], billno[14], jbcode2[9], vdcode[9], shipdate[9];
    FLOAT actcogs;
    CHAR jbcode[9];
/*    CHAR spcode[4], jbname[33], state[3], pre_retanige[10], podate[9]; */
    CHAR spcode[4], jbname[33], state[3], pre_retanige[4], pre_ret2[3], pre_ret3[3], podate[9];
    FLOAT estship, shipping, tax, other, profit, commission, bid;
/*    CHAR client_code[9], junk[2], status[2], cur_retanige[3]; */
    CHAR client_code[9];
    SHORT plidx; /* saves the index of the vendor in PL list */
    CHAR status[2], cur_retanige[3];
    FLOAT mkup;
    /* Added two new date fields - 9/15/99 */
    CHAR sub_date[9], approval_date[9];
    /* Since we have to do conversion anyway, lets just add another junk field */
/*    CHAR junk[40]; */
    CHAR shipcorp[23], pre_ret4[3], pre_ret5[3], material_po[9];
	SHORT shipcompindex;
	/* Added on 7/8/00 */
	CHAR inv_date[9];
	CHAR junk[20];
} POREC;

/*
* User Defs Structure
*/
typedef struct {      
    CHAR spcode[4], master_access[2], junk[4], password[10];
    CHAR priv[20][2];
} USREC;

/*
* Product Line Structure
*/
typedef struct {      
    CHAR line[3];
    CHAR desc[40];
} LIREC;

/*
* Product Class Structure
*/
typedef struct {      
    CHAR clas[5];
    CHAR desc[40];
} CLREC;

/*
* Vendor Discount Structure
*/
typedef struct {           
    CHAR range[17];
    CHAR discount[8];
} DDREC;


/*
* Help Module Structure
*/
typedef struct {
    CHAR text[48];
} HLREC;

/*
* Tax information for States and certain cities
*/
typedef struct {
    CHAR state[3], city[21];
    FLOAT rate;
} TXREC;


/*
* Room Schedule Structure
*/
typedef struct {            
    CHAR vdcode[9];
    CHAR pdcode[21];
    CHAR pddesc[31];
    SHORT qt[7], total, prevt;
    CHAR hws[13];
} RSREC;

typedef struct {
    CHAR vdcode[9];
    CHAR pdcode[21];
    CHAR pddesc[31];
    LONG total;
} temp_rsrec;

/*
* Job Comments Structure
*/
typedef struct {      
    CHAR jobcode[9];
    CHAR date1[9];
    CHAR comment1[66];
    CHAR date2[9];
    CHAR comment2[66];
    CHAR date3[9];
    CHAR comment3[66];
    CHAR date4[9];
    CHAR comment4[66];
    CHAR date5[9];
    CHAR comment5[66];
    CHAR date6[9];
    CHAR comment6[66];
    CHAR date7[9];
    CHAR comment7[66];
    CHAR date8[9];
    CHAR comment8[66];
} jobcom;

/*
* System Paramater Structure
*/
typedef struct {       
    CHAR  company[80];
    CHAR  filename[DB_END+1][9];
    CHAR  path[DB_END+1][30];
    SHORT pport;
    SHORT ftax;                     /* freight is taxable. added on 4/1/97 */
    CHAR  junk[6];
    LONG  quitTime;
    CHAR  editor[31];
    CHAR  fax[15];
    CHAR  phone[15];
    CHAR  mailserver[40];       /* Used to be: faxcmd[40]; changed on 5/6/00 */
    CHAR  prnDriver0[13];
    CHAR  editDir[31];
    CHAR  prnDriver1[13];
} SYSREC;

/*
* Company Information Structure
* This info will be kept in AUTOMAT.CMP file
*/
typedef struct {
    CHAR address1[41];  /* Company name is stored in DB.CFG file, which is not changable */
    CHAR address2[41];
    CHAR city[16];
    CHAR state[3];
    CHAR zip[11];
    CHAR tel[15];
    CHAR fax[15];
    CHAR warehouse[33]; /* Company warehouse name, address, etc. */
    CHAR whaddress1[41];  
    CHAR whaddress2[41];
    CHAR whcity[16];
    CHAR whstate[3];
    CHAR whzip[11];
    CHAR whtel[15];
    CHAR whfax[15];
    CHAR contact[33];   /* Insurance info */
    CHAR insuname[33];
    CHAR insutel[15];
    CHAR insufax[15];
} COMPINFOREC;

typedef struct {
    CHAR code[3], name[22];
} STREC;

typedef struct PO_ALT {
    POREC po;
    struct PO_ALT *next;
} PO_ALT;

typedef struct RS_ALT {
    RSREC rs;
    INT idx;
    struct RS_ALT *next;
} RS_ALT;

typedef struct VD_ALT {
    VDREC vd;
    INT idx;
    struct VD_ALT *next;
} VD_ALT;

/*
* Printer escape codes (loaded via ReadPrinterDriver)
*/
typedef struct {
    CHAR model[40];
    CHAR cond[40];
    CHAR land[40];
    CHAR reset[40];
} PRNCODES;

/*
* Job Installation structure
*/
typedef struct {
    CHAR inst_code[9];  /* installer code, same as vendor code */
    CHAR po[7][9];      /* 7 days of PO code */
    CHAR name[7][33];   /* 7 days of Job name, toggle between po and name */
    CHAR jb[7][9];      /* not displayed on job installation screen */
} INREC;

#pragma pack(pop)

/****************** Function Prototypes by Module ***************************/

/* in DB.C  main() */
INT          jb_po(DB_TYPE, IN_POS *, JBREC *);
INT          es_room_schedule(DB_TYPE, IN_POS *, ESREC *);
INT          eslist(ESREC *);
INT          esbid(ESREC *);
INT          esaward(ESREC *);
INT          esack(ESREC *);
INT          esletters(DB_TYPE, IN_POS *, ESREC *);
INT          es_general_bid(DB_TYPE, IN_POS *, ESREC *);
INT          cd_chooser(BOOLEAN *);
INT          do_invoice(POREC *, BOOLEAN);
INT          po_createpo(POREC *, const CHAR *, BOOLEAN, const CHAR *);
INT          jb_po_form(DB_TYPE, IN_POS *, JBREC *);
INT          jb_installation(IN_POS *, DB_TYPE, VOID *);
INT          jb_requisition(JBREC *, BOOLEAN);
INT          po_funcs(DB_TYPE, IN_POS *, POREC *);
INT          po_form(POREC *);
INT          po_message(POREC *);
INT          rs_funcs(DB_TYPE, IN_POS *, RSREC *);
INT          po_release_fabrication(IN_POS *, POREC *, INT);
INT          po_createpo_for_release_fabrication(IN_POS *, POREC *, const CHAR *, INT);
INT          jb_proj_mang_form(DB_TYPE, IN_POS *, JBREC *, BOOLEAN);
VOID         print_room_schedule(const CHAR*);
VOID         populate_rs(INT, VOID *, BOOLEAN);
INT          part_list_print(FILE *, INT, INT, ESREC *, CHAR *, BOOLEAN, INT);


/* In DB1.C */
INT          esvalidity(ESREC *);
INT          eschangeorder(ESREC *);


/* In CHECK.C */
BOOLEAN      savecheck(VOID);
struct date  datecheck(CHAR *);
INT          esccheck(CHAR *);
INT          strcheck(const CHAR *, CHAR *);
INT          printercheck(VOID);

/* In DB_HELP.C */
INT          helpinpt(DB_TYPE, INT,CHAR *,VOID *);
INT          rechelp(DB_TYPE, INT,CHAR *);
INT          gethelp(DB_TYPE, INT, INT, INT, INT, INT);
VOID         helpset(DB_TYPE, INT, INT, INT, INT, INT);
INT          helpmessage(INT);
VOID         put_help(INT);
INT          HelpEditText(INT);
VOID         put_win2(CHAR *, INT);

/* In DB_INPT.C */
CHAR         *show_product_list(VOID);
INT          db_inpt(DB_TYPE);
VOID         mkprompt(DB_TYPE, INT, CHAR *);
INT          s_findinpt(DB_TYPE, CHAR *);
INT          skipfield(DB_TYPE, IN_POS *, CHAR *);
VOID         print_single_record(DB_TYPE);
INT          print_whole_file(DB_TYPE, IN_POS *, VOID *);
INT          gen_funcs(DB_TYPE, IN_POS *, CHAR *);

/* In DB_MISC.C */
INT          strdatecmp(CHAR *, CHAR *);
INT          numdays(CHAR *, CHAR *);
LONG         longcmp(LONG *, LONG *);
INT          floatcmp(FLOAT *, FLOAT *);
INT          datecmp(struct date *, struct date *);
INT          intcmp(SHORT *, SHORT *);
VOID        *s_alloc(UINT);
VOID        *s_realloc(VOID *, UINT);
VOID         s_free(VOID *);
VOID         scrrestore(struct text_info *);
VOID         store(VOID);
VOID         restore(VOID);
VOID         quit(INT);
VOID         beep(VOID);
VOID         IntroScreen(VOID);
VOID         PrnPrint(UBYTE);
INT          handler(INT, INT, INT, INT);
VOID         ErrorChecking(VOID);
VOID         show_window(INT, INT, INT, INT, INT);
VOID         getborder(INT, INT *, INT *, INT *, INT *, INT *, INT *);
INT          window_chooser(INT, INT, CHAR *list[]);
INT          window_chooser_mod(INT, INT, INT, CHAR *list[]);
INT          Control_C(VOID);
VOID         Inits(VOID);
INT          SysCmd(const CHAR *, ...);
INT          random_n(VOID);

/* In DB_PRINT.C */
VOID         printmenu(VOID);
INT          sel_file(INT);
INT          sel_recs(INT, INT [], CHAR *);
BOOLEAN      SelectPrinter(VOID);


/* In FILE.C */
VOID         FileCopyH(INT, INT);
VOID         FileAppend(const CHAR *, const CHAR *);
VOID         FileCopy(const CHAR *, const CHAR *);
LONG         diskspace(INT);
INT          FileWrite(INT, VOID *, INT);
INT          FileRead(INT, VOID *, INT);
INT          FileLock(INT);
INT          FileUnlock(INT);
INT          FileOpen(const CHAR *);
VOID         FileClose(INT);
LONG         FileSeek(INT, LONG, INT);
INT          FileCreateTemp(CHAR *);
INT          FileCreate(const CHAR *);


/* In MENU.C */
INT          menu(CHAR **, INT, INT, INT, INT, INT);

/* In S_FILE.C */
INT          s_lock(DB_TYPE);
INT          s_unlock(DB_TYPE);
INT          s_open(DB_TYPE);
VOID         s_modnrecs(DB_TYPE, USHORT);
VOID         s_read(DB_TYPE, VOID *, USHORT);
IN_POS       s_directions(DB_TYPE, IN_POS *, VOID *);
USHORT       s_nrecs(DB_TYPE);
INT          s_getno(DB_TYPE, INT, VOID *);
INT          s_binsearch(DB_TYPE, VOID *);
INT          s_write(DB_TYPE, VOID *, USHORT, BOOLEAN);
INT          s_inhelpset(DB_TYPE, INT, INT, INT, INT, INT);
INT          s_findrec(DB_TYPE, VOID *, VOID *, INT *, INT);
INT          s_recinsert(DB_TYPE, VOID *);
VOID         s_recwrite(DB_TYPE, VOID *, USHORT);
INT          s_delrec(DB_TYPE, VOID *, USHORT);
VOID         s_remove_rec(DB_TYPE, VOID *, USHORT);
INT          s_inset(DB_TYPE, IN_POS *, VOID *);
INT          s_vcheck(DB_TYPE, VOID *, USHORT);
INT          dirnext(DB_TYPE, IN_POS *, INT);
IN_POS       directions(DB_TYPE, IN_POS *, VOID *);
VOID         s_clearrec(DB_TYPE, VOID *);
VOID         s_openfiles(INT, ...);
INT          s_close(DB_TYPE);
VOID         s_closeall(VOID);
INT          s_delfile(DB_TYPE);
INT          s_tempcopy(DB_TYPE, CHAR *);
VOID         s_tempfile(DB_TYPE);

/* In SCREEN.C */
VOID         print_time(CHAR *);
VOID         print_date(CHAR *);
INT          sheadprintf(CHAR *, INT, INT, CHAR *);
VOID         mainscreen(VOID);
VOID         screendump(INT, INT, INT, INT, CHAR [], INT);
INT          inscreen(DB_TYPE);
INT          inpt_screen(DB_TYPE);
INT          insetup(DB_TYPE, IN_POS *, VOID *);
VOID         in_printline(DB_TYPE, INT, INT, VOID *);
VOID         setwindow(INT, INT, INT, INT, CHAR [], CHAR [], CHAR [], CHAR []);

/* in OUTPUT.C */
INT          printfile(FILE *, DB_TYPE, INT);
FILE        *printdest(CHAR *);
VOID         printtitle(FILE *, CHAR *);
VOID         printheader(FILE *, DB_TYPE, CHAR *);
INT          fheadprintf(FILE *, INT, CHAR *);
VOID         bcprintf(CHAR *, FLOAT);
VOID         biprintf(FILE *, CHAR *, INT);
VOID         ciprintf(CHAR *, INT);
VOID         printxy(INT, INT, const CHAR *, ...);
VOID         goput(INT, INT, CHAR);
VOID         fldprint(DB_TYPE, CHAR *, INT);
INT          message(UBYTE, const CHAR *, ...);
VOID         Print(const CHAR *, ...);

/* In DB_SYS.C */
INT          sysmgr(VOID);
VOID         sysparams(VOID);
VOID         fileparams(VOID);
INT          getsysparams(VOID);
INT          savesysparams(VOID);
INT          getsec(CHAR *, CHAR *);
INT          seccheck(INT, CHAR);
VOID         backarch(VOID);
INT          db_merge(VOID);
INT          db_arc(VOID);
INT          readonly(INT);
VOID         ReadPrinterDriver(const CHAR *, PRNCODES *);
INT          GetCompInfo(VOID);
INT          SaveCompInfo(VOID);
VOID         SetupCompInfo(VOID);
VOID         db_arc_by_file(INT);
INT          pd_convert(VOID);

/* In DIS_RPT.C */
VOID         reportmenu(VOID);
INT          sales_report(VOID);
INT          sales_analysis(VOID);
INT          sales_forcast(VOID);
INT          jb_requisition_report(VOID);
INT          project_management_sheet(VOID);

/* In DB_RPT.C */
INT          jobfollowup(VOID);           
INT          general_bid_report(CHAR *, CHAR *, CHAR *, CHAR *, CHAR *, CHAR *);
INT          bid_status_req_let(CHAR *, CHAR *, CHAR *, CHAR *, CHAR *, CHAR *);
INT          ProductSearchReport(VOID);
INT          jobfollowup_by_arch(CHAR *, CHAR *, CHAR *, CHAR *, CHAR *, CHAR *);

/* in INPUT.C */
USHORT       nextkey(VOID);
VOID         FillMouseClickPoint(MOUSE_CLICK *, USHORT, USHORT, USHORT, USHORT, USHORT);
USHORT       ConvertMouseClick(POINT *);
USHORT       inpt(INT, INT, CHAR *, VOID *, INT, INT, BOOLEAN);
INT          inputs(INT, CHAR [], VOID *[], CHAR *[],...);
INT          lineinput(INT, INT, CHAR *[], INT, INT);

/* In KKUOTE.C */
VOID         free_tx(VOID);
BOOLEAN      is_field_entered(CHAR *, CHAR *);
INT          esmessage(ESREC *);
INT          plmessage(VOID *);
INT          jb_es(DB_TYPE, IN_POS *, JBREC *);
INT          jb_spupdate(JBREC *);
INT          sp_pay(DB_TYPE, IN_POS *, SPREC *);
INT          jb_delete(DB_TYPE, IN_POS *, JBREC *);
INT          allcomments(DB_TYPE, IN_POS *, VOID *);
INT          jb_stat(DB_TYPE, IN_POS *, JBREC *);
INT          jb_letters(DB_TYPE, IN_POS *, JBREC *);
INT          vd_val(VDREC *);
INT          jb_val(JBREC *);
INT          jb_help(DB_TYPE, IN_POS *, JBREC *);
INT          pl_funcs(DB_TYPE, IN_POS *, PLREC *);
INT          pl_init(DB_TYPE, IN_POS *, CHAR *);
INT          pl_val(PLREC *);
INT          newpl(DB_TYPE);
INT          plmake(INT, ESREC *);
VOID         plfile(ESREC *);
INT          openplfile(ESREC *);
VOID         pl_copy(ESREC *, ESREC *);
INT          pd_xref(DB_TYPE, IN_POS *, PDREC *);
INT          us_msg(VOID);
INT          newfile(DB_TYPE);
PDREC        xref_help(CHAR *, BOOLEAN);
INT          pd_load(DB_TYPE, IN_POS *, CHAR *);
INT          pd_mkup(DB_TYPE, IN_POS *, PDREC *);
INT          pd_new(DB_TYPE);
INT          pl_xref(PLREC *, CHAR *, BOOLEAN);
FLOAT        newdis(CHAR *);
VOID         pl_total(VOID);
INT          vd_funcs(DB_TYPE, IN_POS *, VDREC *);
INT          newpltotals(FLOAT *, FLOAT *);
INT          pl_get(PLREC *, INT);
FLOAT        pltotals(FLOAT *, FLOAT *, FLOAT *, const CHAR *);
VOID         wininput(DB_TYPE);
INT          vd_chooser(CHAR *);
INT          vd_chooser_install(CHAR *);
INT          letters(DB_TYPE, IN_POS *, CHAR *);
INT          po_init(INT, IN_POS *, POREC *);
INT          goto_last_job(DB_TYPE, IN_POS *, VOID *);
VOID         infomenu(VOID);
VOID         sp_new(DB_TYPE, IN_POS *, SPREC *);
INT          val_def(CHAR *);
INT          msg_def(VOID *);
PO_ALT      *po_chooser(const CHAR *);
PO_ALT      *po_chooser_one(const CHAR *); 
INT          other_letters(JBREC *, ESREC *, CDREC *);
INT          letter_of_transmittal(JBREC *, ESREC *, CDREC *);
RS_ALT      *rs_chooser(const CHAR *, BOOLEAN);
VD_ALT      *vd_chooser_multi(const CHAR *, BOOLEAN);
VOID         print_label_to_label_sheet(CHAR*, CHAR*, CHAR*, CHAR*, CHAR*, CHAR*, CHAR*);
VOID         print_address_on_envelop(BOOLEAN, CHAR*, CHAR*, CHAR*, CHAR*, CHAR*, CHAR*, CHAR*);
VOID         get_pl_pos(INT, CHAR *, INT *, INT *);

/* in SORT.C */
INT          sortcmp(const VOID *, const VOID *);
VOID         sortprep(DB_TYPE);
INT          sortfile(DB_TYPE, INT, INT);
INT          gencmp(VOID *, VOID *);
INT          getno_mess(DB_TYPE, INT, VOID *);
INT          power(INT, INT);

/* in EDITOR.C */
INT          TextEditorEx(const CHAR *, const CHAR *);
INT          loader(const CHAR *, CHAR *);

/* in EDITOR1.C */
VOID         TextEditor(const CHAR *, const CHAR *, const CHAR *);

/* only defined for REP version */
#ifdef REP

/* in REP.C  main() */
INT          esbid(ESREC *);
INT          esletters(DB_TYPE, IN_POS *, ESREC *);

/* in REP_RPTS.C */
INT          sales_analysis(VOID);
INT          sales_forcast(VOID);

#endif

/* in DB_FAX.C */
INT          FaxSend(const CHAR *, const CHAR *);

/* in KK_MORE.C */
INT          set_rate(ESREC *, CHAR *, BOOLEAN);
VOID         re_calc(ESREC *, CHAR *, BOOLEAN, INT);
INT          es_funcs(DB_TYPE, IN_POS *, ESREC *);
VOID         checkjobfollowup(CHAR *);

/* in FILE.C , functions to provide open/close file debug */
FILE*        fileopn(const CHAR *, const CHAR *); /* same as fopen */
VOID         filecls(FILE *); /* same as fclose */
INT          handleopn(const CHAR *, INT, UINT); /* same as open */
VOID         handlecls(INT); /* same as close */

/* in JB_INST.C */
VOID         JobInstallation(VOID);
INT          in_funcs(DB_TYPE, IN_POS *, INREC *);
VOID         PrintInstallation(BOOLEAN);
VOID         InstallationRequestForm(POREC *);

#endif /* DB_DEFS_H */
