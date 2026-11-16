/**
*   VMISC.C     -
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include <assert.h>
#include <dos.h>
#include <bios.h>
#include <time.h>
#include "vmisc.h"
#include "vmouse.h"
#include "db_defs.h"

/***************************************************************************
StrTime - converts time to a string
	 returns pointer to string;
***************************************************************************/
CHAR *StrTime(CHAR *str, struct time *t)
{
    /*
    * can't write to null pointers
    */
    Assert(str != NULL);
    Assert(t != NULL);

    /* build a string containing the time */
    sprintf(str, "%02d:%02d:%02d", t->ti_hour, t->ti_min, t->ti_sec);
    return(str);
}


/**************************************************************************
* StrToTime - converts time in string format to struct time format
*           returns pointer to the time structure
**************************************************************************/
struct time *StrToTime(const CHAR *str, struct time *t)
{
    /*
    * can't write to null pointers
    */
    Assert(str != NULL);
    Assert(t != NULL);

    int hour = 0, min = 0, sec = 0;

    sscanf(str, "%d:%d:%d", &hour, &min, &sec);   /* fields are single bytes */
    t->ti_hour = (unsigned char)hour;
    t->ti_min = (unsigned char)min;
    t->ti_sec = (unsigned char)sec;
    return(t);
}


/***************************************************************************
StrTimeStamp - Returns a pointer to current time as a string "02d:%02d:%02d"
***************************************************************************/
CHAR *StrTimeStamp(CHAR *str)
{
    struct time t;

    /* can't write to a NULL pointer */
    Assert(str != NULL);

    gettime(&t);
    return(StrTime(str, &t));
}


/**************************************************************************
TimeUnpack - Converts packed time into sec, min, hour
**************************************************************************/
VOID TimeUnpack(USHORT *sec, USHORT *min, USHORT *hour, USHORT num)
{
    /* can't write to null pointers */
    Assert((sec != NULL) && (min != NULL) && (hour != NULL));

    *sec = (num&0x1f)*2;
    *min = (num&0x7e0) >> 0x05;
    *hour = (num&0xf800) >> 0x0b;
}


/**************************************************************************
TimePack - Converts sec, min, hour into packed time
**************************************************************************/
USHORT TimePack(USHORT sec, USHORT min, USHORT hour)
{
    sec >>= 1;
    min <<= 0x05;
    hour <<= 0x0b;
    sec += min;
    sec += hour;

    return(sec);
}


/****************************************************************************
StrDate - converts a date structure to a string.
****************************************************************************/
CHAR *StrDate(CHAR *str, struct date *dt)
{
    /* can't write to or read from null pointers */
    Assert((str != NULL) && (dt != NULL));

    sprintf(str, "%02d/%02d/%02d", dt->da_mon, dt->da_day, dt->da_year%100);
    return(str);
}

/****************************************************************************
DateStr - converts a string to a date structure.
****************************************************************************/
struct date *DateStr(struct date *dt, char *str)
{
    char mon[3], day[3], year[5];
    int y;

    /* can't write to or read from null pointers */
    Assert((str != NULL) && (dt != NULL));

    strncpy(mon, str, 2); mon[2] = '\0';
    strncpy(day, &str[3], 2); day[2] = '\0';
    y = atoi(&str[6]);
    if(y >= 80 && y <= 99)
        sprintf(year, "19");
    else
        sprintf(year, "20");
    strncat(year, &str[6], 2); year[4] = '\0';
    dt->da_mon = atoi(mon);
    dt->da_day = atoi(day);
    dt->da_year = atoi(year);
    return(dt);
}

/****************************************************************************
DateAdd - adds days to the date - returns new date.
****************************************************************************/
struct date *DateAdd(struct date *d1, SHORT days)
{
    /* SHORT mon[14] = {31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31}; */
    SHORT y, mon[14];

    /* verify that our date is a valid pointer */
    Assert(d1 != NULL);

    mon[0] = 31; mon[1] = 31; mon[3] = 31; mon[4] = 30; mon[5] = 31; mon[6] = 30;
    mon[7] = 31; mon[8] = 31; mon[9] = 30; mon[10] = 31; mon[11] = 30; mon[12] = 31; mon[13] = 31;

    y = d1->da_year;
    if((y%4 == 0 && y%100 != 0) || y%400 == 0)
        mon[2] = 29;
    else
        mon[2] = 28;

    days += d1->da_day;
    while (days < 0)
    {
        days += mon[--d1->da_mon];
        if (d1->da_mon < 1) {
            d1->da_year--;
            y = d1->da_year;
            if((y%4 == 0 && y%100 != 0) || y%400 == 0)
                mon[2] = 29;
            else
                mon[2] = 28;
            d1->da_mon = 12;
            d1->da_mon = 12;
        }
    }

    while (days > mon[d1->da_mon])
    {
        days -= mon[d1->da_mon++];

        if (d1->da_mon > 12) {
            d1->da_year++;
            y = d1->da_year;
            if((y%4 == 0 && y%100 != 0) || y%400 == 0)
                mon[2] = 29;
            else
                mon[2] = 28;
            d1->da_mon = 12;
            d1->da_mon = 1;
        }
    }
    d1->da_day = days;

    /* verify that the day and month are valid numbers */
    Assert((d1->da_day >= 1) && (d1->da_day <= 31) && (d1->da_mon >= 1) && (d1->da_mon <= 12));

    return(d1);
}

/****************************************************************************
DiffDate - gets the days between the specified two dates. Assume d2 holds a newer date
****************************************************************************/
int DiffDate(struct date *d1, struct date *d2)
{
    int y, m, d, diff, i, minusflag = 0;
    struct date tmp1, tmp2, *AddDate();

    memset(&tmp1, 0, sizeof(struct date));
    memset(&tmp2, 0, sizeof(struct date));
    y = d2->da_year - d1->da_year;
    m = d2->da_mon - d1->da_mon;
    d = d2->da_day - d1->da_day;
    if(y > 0)
    {
	memcpy(&tmp1, d1, sizeof(struct date));
	memcpy(&tmp2, d2, sizeof(struct date));
	diff = (y - 1) * 365;
    }
    else if(y < 0)
    {
	memcpy(&tmp1, d2, sizeof(struct date));
	memcpy(&tmp2, d1, sizeof(struct date));
	diff = (-y - 1) * 365;
	minusflag = 1;
    }
    else if(y == 0 && m > 0)
    {
	memcpy(&tmp1, d1, sizeof(struct date));
	memcpy(&tmp2, d2, sizeof(struct date));
	diff = (m - 1) * 28;
    }
    else if(y == 0 && m < 0)
    {
	memcpy(&tmp1, d2, sizeof(struct date));
	memcpy(&tmp2, d1, sizeof(struct date));
	diff = (-m - 1) * 28;
	minusflag = 1;
    }
    else /* y == 0 && m == 0 */
	return d;

    DateAdd(&tmp1, diff);
    for(i = 1; ; i++)
    {
	DateAdd(&tmp1, 1);
	if(memcmp(&tmp1, &tmp2, sizeof(struct date)) == 0)
	    break;
    }
    diff += i;
    if(minusflag)
	return (-diff);
    return (diff);
}

/****************************************************************************
DayOfWeek - Given a date, return 3 character of day of week
*****************************************************************************/
char *DayOfWeek(char *da)
{
	struct date today, temp;
	time_t secs;
	char str[4];
    static char *week[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	int i, dif;

	DateStr(&temp, da);
	time(&secs);
	strncpy(str, ctime(&secs), 3); str[3] = '\0';
	for(i = 0; i < 7; i++)
	    if(!strcmp(str, week[i]))
		break;

	getdate(&today);
	dif = DiffDate(&temp, &today);
	dif = dif % 7;
	if(dif >= 0)
	{
	    dif = i - dif;
	    if(dif < 0)
		dif = dif + 7;
	}
	else
	{
	    dif = -dif;
	    dif = i + dif;
	    if(dif >= 7)
		dif = dif - 7;
	}
	return (week[dif]);
}


/**************************************************************************
SameMonth - Compare two dates against their month and year
          - Assume both dates have xx/xx/xx format (must have leading 0
          - for Month of January, etc.)
**************************************************************************/
BOOLEAN SameMonth(CHAR *day1, CHAR *day2)
{
    if(!day1 || !day2)
        return FALSE;

    if(day1[0] == day2[0] && day1[1] == day2[1] &&
       day1[6] == day2[6] && day1[7] == day2[7])
       return TRUE;

    return FALSE;
}


/*****************************************************************************
StrReplaceChar - changes all occurances of character oldCh to newCh in string
*****************************************************************************/
VOID StrReplaceChar(CHAR *str, CHAR oldCh, CHAR newCh)
{
    /* can't write to null pointer */
    Assert(str != NULL);

    /*
    * replace oldCh with newCh until end of string
    */
    while (*str != '\0') {
	if (*str == oldCh)
	    *str = newCh;
	str++;
    }
}


/****************************************************************************
SetCursor - changes cursor - uses interrupt 10h funtion 1
	 size = CUR_TOP, CUR_OFF, CUR_ON, or CUR_FUL
****************************************************************************/
VOID SetCursor(USHORT curSize)
{
    KK_SetCursor(curSize);   /* INT 10h/01h set cursor shape */
}


/***************************************************************************
* Sound - Plays frequency 'note' for 22ms
***************************************************************************/
VOID Sound(USHORT note)
{
    sound(note);
    delay(22);
    nosound();
}


/***************************************************************************
* SoundPlay - Plays string of notes
***************************************************************************/
VOID SoundPlay(USHORT *notes, USHORT nNotes)
{
    /* can't read from null pointer */
    Assert(notes != NULL);

    /* play each note in the string of notes */
    while (nNotes--)
	Sound(*notes++);
}


/**
*
*  name      IsShareInstalled - Determines if SHARE.EXE is installed
*
*  synopsis  shareOn = IsShareInstalled()
*
*
*  purpose   To determine if SHARE.EXE is installed (requires DOS 3.x or higher)
*
*
*  returns   shareOn = TRUE if SHARE is installed, FALSE otherwise
*
**/
BOOLEAN IsShareInstalled(VOID)
{
    /* File sharing is a property of the host operating system now. */
    return(TRUE);
}


/**
*
*  name      KeyPress - Determines if a keyboard key was hit
*
*  synopsis  press = KeyPress(&key)
*            USHORT *key;                   pointer to key (if one was pressed)
*
*  purpose   To determine if a key was pressed, and if so, returns it
*
*
*  returns   press = TRUE if a key was pressed, otherwise FALSE
*
**/
BOOLEAN KeyPress(USHORT *key)
{
    USHORT tmp;

    /*
    * if a key was pressed 
    */
    if (bioskey(1)) {

	/* then remove it from keyboard buffer */
	tmp = bioskey(0);

	/*
	* don't update pointer if it points nowhere
	*/
	if (key != NULL)
	    *key = tmp;

	return(TRUE);
    }
    return(FALSE);
}


/**
*
*  name      ErrorLog - Appends formatted string to a log file
*
*  synopsis  ErrorLog(fileName, format, ...)
*            const CHAR *fileName;           name of log file
*            const CHAR *format;             string to send to log file
*
*  purpose   To provide a log file for send error messages
*
*
*  returns   Nothing
*
**/
VOID ErrorLog(const CHAR *logFile, const CHAR *format, ...)
{
    CHAR out[160];
    va_list argptr;
    FILE *fs;

    /*
    * don't parse the variable input if there aren't any
    */
    if (format != NULL) {

	/*
	* parse the variable arguement list
	*/
	va_start(argptr, format);
	vsprintf(out, format, argptr);
	va_end(argptr);

	/* make sure we don't blow the stack */
	assert(strlen(out) < 160);

	/*
	* output the message to an error log file
	*/
    if ((fs = fileopn(logFile, "at")) != NULL) {
	    fprintf(fs, "\n%s", out);
	filecls(fs);
	}
    }
}


/**
*
*  name      MemAlloc - Allocates a block of memory
*
*  synopsis  ptr = MemAlloc(size)
*            UINT size;                    size for new block of memory
*
*  purpose   To allocate a block of memory (and provide debugging info on allocations)
*
*
*  returns   ptr = pointer to new block of memory
*
**/
VOID *MemAllocEx(UINT size, CHAR *file, INT line)
{
    VOID *ptr;

    /* can't allocate 0 bytes */
    Assert(size != 0);

    file = file; line = line;

    /*
    * allocate the buffer
    */
    ptr = malloc(size);

    /*
    * running out of memory during development is probably a bug
    */
    Assert(ptr != NULL);

    #ifdef DEBUG
	#if DEBUG > 3
	    ErrorLog("DEBUG.LOG", "MemAlloc: (%08lx) in %s at %d", ptr, file, line);
	#endif
    #endif
      
    return(ptr);
}


/**
*
*  name      MemCalloc - Allocates and clears block of memory
*
*  synopsis  ptr = MemCalloc(size)
*            UINT size;                    size for new block of memory
*
*  purpose   To allocate and clear a block of memory (and provide debugging info on allocations)
*
*
*  returns   ptr = pointer to new block of memory
*
**/
VOID *MemCallocEx(UINT size, CHAR *file, INT line)
{
    VOID *ptr;

    /* can't allocate 0 bytes */
    Assert(size != 0);

    file = file; line = line;

    /*
    * allocate the buffer and clear it
    */
    ptr = calloc(size, 1);

    /* alert me if we run out of memory */
    Assert(ptr != NULL);

    #ifdef DEBUG
	#if DEBUG > 3
	    ErrorLog("DEBUG.LOG", "MemCalloc: (%08lx) in %s at %d", ptr, file, line);
	#endif
    #endif

    return(ptr);
}


/**
*
*  name      MemRealloc - Re-allocates a block of memory to a different size
*
*  synopsis  ptr = Realloc(ptr, size)
*            VOID *ptr;                    pointer to reallocate
*            UINT size;                    new size for pointer
*
*  purpose   To re-allocate a block of memory (and provide debugging info on allocations)
*
*
*  returns   ptr = pointer to new block of memory
*
**/
VOID *MemReallocEx(VOID *ptr, UINT size, CHAR *file, INT line)
{
    /* can't realloc to 0 bytes */
    Assert(size != 0);

    /* can't reallocate a NULL pointer */
    Assert(ptr != NULL);

    file = file; line = line;

    /* reallocate memory */
    ptr = realloc(ptr, size);

    /* ANSI says this is not possible */
    Assert(ptr != NULL);

    #ifdef DEBUG
	#if DEBUG > 3
	    ErrorLog("DEBUG.LOG", "MemRealloc: (%08lx) in %s at %d", ptr, file, line);
	#endif
    #endif

    return(ptr);
}


/**
*
*  name      MemFree - frees previously allocated memory
*
*  synopsis  MemFree(ptr)
*            VOID *ptr                    pointer to free
*
*  purpose   To free a pointer (and provide debugging info on allocations)
*
*
*  returns   Nothing
*
**/
VOID MemFreeEx(VOID *ptr, CHAR *file, INT line)
{
    /* why would we be free-ing a NULL pointer ? */
    Assert(ptr != NULL);

    file = file; line = line;

    #ifdef DEBUG
	#if DEBUG > 3
	    ErrorLog("DEBUG.LOG", " MemFree: (%08lx) in %s at %d", ptr, file, line);
	#endif
    #endif

    free(ptr);
}


/**
*
*  name      DiskGetFreeSpace - gets number of free bytes on specified drive
*
*  synopsis  space = DiskGetFreeSpace(drive)
*            INT drive;                   drive to get free space (0= default, 1=A:, etc...)
*
*  purpose   To get the number of free bytes on specified drive
*
*
*  returns   space = free drive space
*
**/
LONG DiskGetFreeSpace(INT drive)
{
    struct diskfree_t df;

    if (!_dos_getdiskfree(drive, &df))
	return((LONG)((LONG)df.avail_clusters*(LONG)df.bytes_per_sector*(LONG)df.sectors_per_cluster));

    return((LONG)-1);
}

/*
* Function to generate one bit of random characters (can be either a number or a lower char)
*/
INT GeneratePassword()
{
	 int aski;
	 static int base = 2;
	 struct  time tt;

	 gettime(&tt);
	 srand(base * tt.ti_hund);
	 base++;

	 aski = rand() % 35;
	 if (aski<=9)
		 return (aski+48);
	 else
		 return (aski+87);
}