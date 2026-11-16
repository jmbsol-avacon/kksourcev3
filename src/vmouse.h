/**
*   VMOUSE.H    - Header file for Mouse Library routines
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
*   1.00a       07-08-93     BCW        Initial design
*
**/
#ifndef VMOUSE_H
#define VMOUSE_H

#include "stdinc.h"

#define MOUSE_LBUTTON 0
#define MOUSE_RBUTTON 1
#define MOUSE_MBUTTON 2

#ifdef __cplusplus
extern "C" {
#endif

BOOLEAN  MouseInit(VOID);
VOID     MouseShowCursor(VOID);
VOID     MouseHideCursor(VOID);
SHORT    MouseGetStatus(USHORT *, USHORT *);
VOID     MousePutCursor(USHORT, USHORT);
BOOLEAN  MouseIsButtonDown(USHORT, USHORT *, USHORT *);
BOOLEAN  MouseIsButtonUp(USHORT, USHORT *, USHORT *);
BOOLEAN  MouseGetClick(USHORT, USHORT *, USHORT *);
VOID     MouseSetCursorXlimit(SHORT, SHORT);
VOID     MouseSetCursorYlimit(SHORT, SHORT);
VOID     MouseDefineGCursor(SHORT, SHORT, VOID *);
VOID     MouseLPE(BOOLEAN);
VOID     MouseGetVersion(SHORT *, SHORT *, SHORT *);
BOOLEAN  MouseIsDriverInstalled(VOID);

#ifdef __cplusplus
}
#endif

#endif /* VMOUSE_H */
