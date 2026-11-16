/**
*   STDINC.H    - Standard Header
*
*
*   Description - Standard include for personal programming macros
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
*   1.00a       01-20-93     BCW        Created this file
*
**/
#ifndef STDINC_H
#define STDINC_H

/*
* Shift Status Function Macros
*/
#include <bios.h>
#define IsRShiftDown()   (bioskey(2)&1)     /* True if Right Shift Key is down */
#define IsLShiftDown()   (bioskey(2)&2)     /* True if Left Shift Key is down */
#define IsCtrlDown()     (bioskey(2)&4)     /* True if Control Key is down */
#define IsAltDown()      (bioskey(2)&8)     /* True if Alt key is down */
#define IsScrollLockOn() (bioskey(2)&16)    /* True if Scroll Lock is ON */
#define IsNumLockOn()    (bioskey(2)&32)    /* True if Num Lock is ON */
#define IsCapsLockOn()   (bioskey(2)&64)    /* True if Caps Lock is ON */
#define IsInsertOn()     (bioskey(2)&128)   /* True if Insert is ON */

/*
* Math Function Macros
*/
#define isodd(number) ((number)&1)          /* Returns 1 if number is odd  */
#define iseven(number) !((number)&1)        /* Returns 1 if number is even */
#ifndef min
#define min(x,y) (x<y?x:y)                  /* returns the min of x and y */
#endif
#ifndef max
#define max(x,y) (x>y?x:y)                  /* returns the max of x and y */
#endif
#ifndef abs
#define abs(x) ((x)<0?-(x):(x))             /* returns the absolute value of x */
#endif

/*
* normal types
*/
#define STATIC static
#define CDECL  _cdecl
#define INLINE inline
#define ASM __asm
#ifndef FAR
#define FAR far
#endif
#define CHAR char
typedef unsigned char UCHAR;
#define BYTE char
typedef unsigned char UBYTE;
#define INT int
typedef unsigned int UINT;
/*
* Fixed to the Borland 16-bit sizes: SHORT/USHORT/LONG/ULONG/FLOAT occur in
* the record structures written to the data files (db_defs.h).  INT stays
* the native int of the host compiler (now 32 bits).
*/
#include <stdint.h>
#define WORD int16_t
#define SHORT int16_t
typedef uint16_t USHORT;
#define LONG int32_t
typedef uint32_t ULONG;
#define FLOAT float
#define DOUBLE double
#define VOID void
#define CONST const

/* abnormal types */
#define BOOLEAN UBYTE

#define YES   1
#define NO    0
#define TRUE  1
#define FALSE 0
#define ON    1
#define OFF   0


/*
* My assertion implementation writes the internal error to a log file.
*/
#ifdef DEBUG
    VOID ErrorLog(const CHAR *, const CHAR *, ...);
    #define Assert(f)       \
        if (f) ;            \
        else ErrorLog("ERRORS.LOG", "Internal Error: In file %s at line %u", __FILE__, __LINE__)
#else
    #define Assert(f)   ;
#endif

#define forever while(1)

/*
* Standard structures
*/
typedef struct {
    USHORT x;
    USHORT y;
} POINT;

/*
* error reporting codes
*/
#define ERROR_CODE       UBYTE
#define ERR_NONE             0
#define ERR_MEMORY           1
#define ERR_FILEOPEN         2
#define ERR_FILECLOSE        3
#define ERR_FILEREAD         4
#define ERR_FILEWRITE        5
#define ERR_FILESEEK         6
#define ERR_PATH             7
#define ERR_DISK             8
#define ERR_FILEFIND         9
#define ERR_FILETYPE        10
#define ERR_FILEDEL         11

/*
* Standard input key scan codes
*/
#define KEY_F1          0x3b00
#define KEY_F2          0x3c00
#define KEY_F3          0x3d00
#define KEY_F4          0x3e00
#define KEY_F5          0x3f00
#define KEY_F6          0x4000
#define KEY_F7          0x4100
#define KEY_F8          0x4200
#define KEY_F9          0x4300
#define KEY_F10         0x4400
#define KEY_F11         0x8500
#define KEY_F12         0x8600

#define KEY_SHFT_F1     0x5400
#define KEY_SHFT_F2     0x5500
#define KEY_SHFT_F3     0x5600
#define KEY_SHFT_F4     0x5700
#define KEY_SHFT_F5     0x5800
#define KEY_SHFT_F6     0x5900
#define KEY_SHFT_F7     0x5a00
#define KEY_SHFT_F8     0x5b00
#define KEY_SHFT_F9     0x5c00
#define KEY_SHFT_F10    0x5d00
#define KEY_SHFT_F11    0x8700
#define KEY_SHFT_F12    0x8800

#define KEY_CTRL_F1     0x5e00
#define KEY_CTRL_F2     0x5f00
#define KEY_CTRL_F3     0x6000
#define KEY_CTRL_F4     0x6100
#define KEY_CTRL_F5     0x6200
#define KEY_CTRL_F6     0x6300
#define KEY_CTRL_F7     0x6400
#define KEY_CTRL_F8     0x6500
#define KEY_CTRL_F9     0x6600
#define KEY_CTRL_F10    0x6700
#define KEY_CTRL_F11    0x8b00
#define KEY_CTRL_F12    0x8c00

#define KEY_ALT_F1      0x6800
#define KEY_ALT_F2      0x6900
#define KEY_ALT_F3      0x6a00
#define KEY_ALT_F4      0x6b00
#define KEY_ALT_F5      0x6c00
#define KEY_ALT_F6      0x6d00
#define KEY_ALT_F7      0x6e00
#define KEY_ALT_F8      0x6f00
#define KEY_ALT_F9      0x7000
#define KEY_ALT_F10     0x7100
#define KEY_ALT_F11     0x8900
#define KEY_ALT_F12     0x8a00

#define KEY_ALT_A       0x1e00
#define KEY_ALT_B       0x3000
#define KEY_ALT_C       0x2e00
#define KEY_ALT_D       0x2000
#define KEY_ALT_E       0x1200
#define KEY_ALT_F       0x2100
#define KEY_ALT_G       0x2200
#define KEY_ALT_H       0x2300
#define KEY_ALT_I       0x1700
#define KEY_ALT_J       0x2400
#define KEY_ALT_K       0x2500
#define KEY_ALT_L       0x2600
#define KEY_ALT_M       0x3200
#define KEY_ALT_N       0x3100
#define KEY_ALT_O       0x1800
#define KEY_ALT_P       0x1900
#define KEY_ALT_Q       0x1000
#define KEY_ALT_R       0x1300
#define KEY_ALT_S       0x1f00
#define KEY_ALT_T       0x1400
#define KEY_ALT_U       0x1600
#define KEY_ALT_V       0x2f00
#define KEY_ALT_W       0x1100
#define KEY_ALT_X       0x2d00
#define KEY_ALT_Y       0x1500
#define KEY_ALT_Z       0x2c00

#define KEY_ALT_1       0x7800
#define KEY_ALT_2       0x7900
#define KEY_ALT_3       0x7a00
#define KEY_ALT_4       0x7b00
#define KEY_ALT_5       0x7c00
#define KEY_ALT_6       0x7d00
#define KEY_ALT_7       0x7e00
#define KEY_ALT_8       0x7f00
#define KEY_ALT_9       0x8000
#define KEY_ALT_0       0x8100

#define KEY_CTRL_A      0x1e01
#define KEY_CTRL_B      0x3002
#define KEY_CTRL_C      0x2e03
#define KEY_CTRL_D      0x2004
#define KEY_CTRL_E      0x1205
#define KEY_CTRL_F      0x2106
#define KEY_CTRL_G      0x2207
#define KEY_CTRL_H      0x2308
#define KEY_CTRL_I      0x1709
#define KEY_CTRL_J      0x240a
#define KEY_CTRL_K      0x250b
#define KEY_CTRL_L      0x260c
#define KEY_CTRL_M      0x320d
#define KEY_CTRL_N      0x310e
#define KEY_CTRL_O      0x180f
#define KEY_CTRL_P      0x1910
#define KEY_CTRL_Q      0x1011
#define KEY_CTRL_R      0x1312
#define KEY_CTRL_S      0x1f13
#define KEY_CTRL_T      0x1414
#define KEY_CTRL_U      0x1615
#define KEY_CTRL_V      0x2f16
#define KEY_CTRL_W      0x1117
#define KEY_CTRL_X      0x2d18
#define KEY_CTRL_Y      0x1519
#define KEY_CTRL_Z      0x2c1a

#define KEY_ENTER       0x1c0d
#define KEY_CTRL_ENTER  0x1c0a
#define KEY_BACKSPACE   0x0e08
#define KEY_CTRL_BS     0x0e7f
#define KEY_LEFT        0x4b00
#define KEY_RIGHT       0x4d00
#define KEY_UP          0x4800
#define KEY_DOWN        0x5000
#define KEY_CR          13           /* ASCII Value of ENTER key. */
#define KEY_FF          12
#define KEY_ESC1        27           /* ASCII Value for Escape */
#define KEY_ESC         0x011b       /* Scan code for Escape   */
#define KEY_SPACE       0x3920
#define KEY_PLUS        0x4e00
#define KEY_MINUS       0x4a00
#define KEY_DEL         0x5300
#define KEY_INS         0x5200
#define KEY_HOME        0x4700
#define KEY_END         0x4f00
#define KEY_PGUP        0x4900
#define KEY_PGDOWN      0x5100
#define KEY_TAB2        9            /* ASCII value of TAB */
#define KEY_TAB         0x0f09       /* Scan code of TAB */
#define KEY_SHFT_TAB    0x0f00
#define KEY_CTRL_HOME   0x7700
#define KEY_CTRL_END    0x7500
#define KEY_CTRL_PGUP   0x8400
#define KEY_CTRL_PGDOWN 0x7600
#define KEY_CTRL_LEFT   0x7300
#define KEY_CTRL_RIGHT  0x7400

#define KEY_0           0x0b30
#define KEY_1           0x0231
#define KEY_2           0x0332
#define KEY_3           0x0433
#define KEY_4           0x0534
#define KEY_5           0x0635
#define KEY_6           0x0736
#define KEY_7           0x0837
#define KEY_8           0x0938
#define KEY_9           0x0a39

/*
* Pseudo key hits for Mouse input
*/
#define KEY_MOUSE_0     0xf000
#define KEY_MOUSE_1     0xf001
#define KEY_MOUSE_2     0xf002
#define KEY_MOUSE_3     0xf003
#define KEY_MOUSE_4     0xf004
#define KEY_MOUSE_5     0xf005
#define KEY_MOUSE_6     0xf006
#define KEY_MOUSE_7     0xf007
#define KEY_MOUSE_8     0xf008
#define KEY_MOUSE_9     0xf009
#define KEY_MOUSE_A     0xf00a
#define KEY_MOUSE_B     0xf00b
#define KEY_MOUSE_C     0xf00c
#define KEY_MOUSE_D     0xf00d
#define KEY_MOUSE_E     0xf00e
#define KEY_MOUSE_F     0xf00f
#define KEY_MOUSE_G     0xf010
#define KEY_MOUSE_H     0xf011
#define KEY_MOUSE_I     0xf012
#define KEY_MOUSE_J     0xf013
#define KEY_MOUSE_K     0xf014
#define KEY_MOUSE_L     0xf015
#define KEY_MOUSE_M     0xf016
#define KEY_MOUSE_N     0xf017

#endif  /* STDINC_H */
