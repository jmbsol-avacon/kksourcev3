/*
 * dos.h - the parts of Borland's <dos.h> that Kwik Kuote uses, implemented
 * on the host operating system (see kk_dos.c).
 *
 * int86()/REGS and inline assembly are deliberately NOT provided: every
 * interrupt call in the original sources has been replaced by an explicit
 * platform call, and a leftover use must fail to compile.
 */
#ifndef KK_DOS_H
#define KK_DOS_H

#include <stdlib.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Borland layouts. struct date was 4 bytes with a 16-bit int; nothing on
 * disk uses either structure, so the modern int is fine.                  */
struct date {
    int  da_year;
    char da_day;
    char da_mon;
};

struct time {
    unsigned char ti_min;
    unsigned char ti_hour;
    unsigned char ti_hund;
    unsigned char ti_sec;
};

void getdate(struct date *d);
void gettime(struct time *t);
void setdate(struct date *d);
void settime(struct time *t);

/* Timing and the PC speaker.  sound()+delay()+nosound() become a beep.    */
void delay(unsigned milliseconds);
void sound(unsigned frequency);
void nosound(void);

/* DOS version: the program insists on DOS 3.1 or newer.                   */
#define _osmajor 7
#define _osminor 10

/* File attributes. */
#define FA_RDONLY 0x01
#define FA_HIDDEN 0x02
#define FA_SYSTEM 0x04
#define FA_LABEL  0x08
#define FA_DIREC  0x10
#define FA_ARCH   0x20

/* Directory search (MS style _dos_findfirst/_dos_findnext). */
struct find_t {
    void    *kk_handle;      /* private iteration state                    */
    unsigned attrib;
    unsigned wr_time;
    unsigned wr_date;
    long     size;
    char     name[13];
};

unsigned _dos_findfirst(const char *path, unsigned attrib, struct find_t *result);
unsigned _dos_findnext(struct find_t *result);
unsigned _dos_setfileattr(const char *path, unsigned attrib);
unsigned _dos_getfileattr(const char *path, unsigned *attrib);

/* Free disk space.  Values are scaled so that
 *   avail_clusters * sectors_per_cluster * bytes_per_sector
 * fits in a 32-bit signed long, which is how the program uses them.       */
#undef diskfree_t   /* MinGW's <direct.h> renames the Microsoft one */
struct diskfree_t {
    unsigned total_clusters;
    unsigned avail_clusters;
    unsigned sectors_per_cluster;
    unsigned bytes_per_sector;
};
unsigned _dos_getdiskfree(unsigned drive, struct diskfree_t *info);

/* Borland random() macro and friends. */
#define random(num)  ((int)(rand() % (num)))
#define randomize()  srand((unsigned)time(NULL))

/* Memory information (unbounded on a modern host). */
unsigned long coreleft(void);
unsigned long farcoreleft(void);
unsigned      stackavail(void);

#ifdef __cplusplus
}
#endif

#endif /* KK_DOS_H */
