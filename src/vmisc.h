/**
*   VMISC.H     -
*
*
*   Description -
*
*
*
*
*   (c) Copyright 1994 by Brian Weed for Vanguard Information Systems
*   All rights reserved.
*
*
*   Revision History:
*
*   Version     Date        Author      Comments
*   -------     --------    ------      --------------------------------
*   1.00a       01-07-94     BCW
*
**/
#ifndef VMISC_H
#define VMISC_H

#include <dos.h>
#include "stdinc.h"           

/*
* cursor sizes for SetCursor()
*/
#define CUR_TOP     0x0001
#define CUR_FUL     0x0004
#define CUR_ON      0x008e
#define CUR_OFF     0x0090


/*
* Don't mangle these function names
*/
#ifdef __cplusplus
extern "C" {
#endif

/*
* prototypes
*/
CHAR        *StrTime(CHAR *, struct time *);
struct time *StrToTime(const CHAR *, struct time *);
CHAR        *StrTimeStamp(CHAR *);
USHORT       TimePack(USHORT, USHORT, USHORT);
VOID         TimeUnpack(USHORT *, USHORT *, USHORT *, USHORT);
CHAR        *StrDate(CHAR *, struct date *);
struct date *DateStr(struct date *, CHAR *);
struct date *DateAdd(struct date *, SHORT);
INT          DiffDate(struct date *, struct date *);
CHAR        *DayOfWeek(CHAR *);
BOOLEAN      SameMonth(CHAR *, CHAR *);
USHORT       DatePack(USHORT, USHORT, USHORT);
VOID         DateUnpack(USHORT *, USHORT *, USHORT *, USHORT);
VOID         SetCursor(USHORT);
VOID         StrReplaceChar(CHAR *, CHAR, CHAR);
VOID         Sound(USHORT);
VOID         SoundPlay(USHORT *, USHORT);
BOOLEAN      IsShareInstalled(VOID);
BOOLEAN      KeyPress(USHORT *);
VOID         ErrorLog(const CHAR *, const CHAR *, ...);
VOID        *MemAllocEx(UINT, CHAR *, INT);
VOID        *MemCallocEx(UINT, CHAR *, INT);
VOID        *MemReallocEx(VOID *, UINT, CHAR *, INT);
VOID         MemFreeEx(VOID *, CHAR *, INT);
LONG         DiskGetFreeSpace(INT);
INT          GeneratePassword(VOID);

#define MemAlloc(x) MemAllocEx(x,__FILE__,__LINE__)
#define MemCalloc(x) MemCallocEx(x,__FILE__,__LINE__)
#define MemRealloc(p,x) MemReallocEx(p,x,__FILE__,__LINE__)
#define MemFree(p) MemFreeEx(p,__FILE__,__LINE__)

#ifdef __cplusplus
}
#endif

#endif  /* VMISC_H */
