/*
 * kk_platform.h - C interface of the Kwik Kuote platform layer.
 *
 * The platform layer replaces everything the original Borland C++ 3.1 / MS-DOS
 * build got from the operating system, the BIOS and the Borland run-time:
 *
 *   - a virtual 80x25 colour text screen with the Borland conio semantics
 *     (window(), textattr(), gettext()/puttext(), cprintf() ...), rendered
 *     through a terminal back end (POSIX ANSI terminal or the Win32 console);
 *   - keyboard input delivered as BIOS INT 16h scan/ASCII codes (bioskey());
 *   - an INT 33h compatible mouse API (see vmouse.h);
 *   - DOS file semantics: back-slash paths, record locking, findfirst,
 *     the CON and PRN/LPTn devices (printer output is spooled and handed to
 *     the host print system);
 *   - the Borland date/time, string and memory helpers.
 *
 * Every legacy translation unit receives this header through kk_compat.h,
 * which the build system force-includes.  New code can include it directly.
 */
#ifndef KK_PLATFORM_H
#define KK_PLATFORM_H

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Life cycle                                                          */
/* ------------------------------------------------------------------ */

/* The console initialises itself on first use; call this to force it.  */
void KK_PlatformInit(void);

/* Restores the terminal and submits pending printer output.  Registered
 * with atexit() automatically, safe to call more than once.             */
void KK_PlatformShutdown(void);

/* ------------------------------------------------------------------ */
/* Console extras (the standard conio API lives in <conio.h>)          */
/* ------------------------------------------------------------------ */

/* Replacement for the INT 10h/AH=01h cursor shape call in VMISC.C.
 * Accepts the CUR_OFF / CUR_ON / CUR_FUL / CUR_TOP values from vmisc.h. */
void KK_SetCursor(unsigned cursorCode);

/* Replacement for INT 10h/AH=06h (up) and AH=07h (down) window scroll.
 * Coordinates are absolute, 0-based and inclusive; 'attr' fills the
 * vacated lines.  lines == 0 clears the whole region (BIOS semantics).   */
void KK_BiosScroll(int up, int lines, int top, int left, int bottom, int right, int attr);

/* Replacement for INT 10h/AH=02h: absolute, 0-based cursor positioning. */
void KK_BiosGotoXY(int x0, int y0);

/* Flush the virtual screen to the real terminal now.                    */
void KK_ConsoleFlush(void);

/* ------------------------------------------------------------------ */
/* Shell, files and devices                                            */
/* ------------------------------------------------------------------ */

/* Name of the interactive command interpreter (COMSPEC / SHELL).        */
const char *KK_ShellCommand(void);

/* Runs a host command with the terminal returned to its normal state
 * for the duration of the command (what system() must do here).         */
int kk_system(const char *command);

/* Moves every file matching a DOS wild-card pattern into destDir.
 * Returns the number of files moved, or -1 on error.                    */
int KK_MoveFiles(const char *pattern, const char *destDir);

/* Converts a DOS style path (back-slashes, optional drive letter) into
 * the host's notation.  On POSIX hosts an existing file is also located
 * case-insensitively, since DOS file names were case-insensitive.       */
void kk_native_path(char *dst, size_t dstSize, const char *src);

/* stdio replacements.  They understand the DOS device names CON, PRN,
 * LPT1..LPT3 and NUL, spool printer output, and translate paths.        */
FILE  *kk_fopen(const char *name, const char *mode);
int    kk_fclose(FILE *fp);
int    kk_fflush(FILE *fp);
int    kk_fprintf(FILE *fp, const char *format, ...);
int    kk_vfprintf(FILE *fp, const char *format, va_list ap);
int    kk_fputc(int c, FILE *fp);
int    kk_fputs(const char *s, FILE *fp);
size_t kk_fwrite(const void *ptr, size_t size, size_t n, FILE *fp);

/* Low level file replacements (path translation, DOS share flags).      */
int    kk_open(const char *path, int oflag, ...);
int    kk_read(int fd, void *buf, unsigned count);
int    kk_write(int fd, const void *buf, unsigned count);
long   kk_lseek(int fd, long offset, int whence);
int    kk_close(int fd);
int    kk_access(const char *path, int mode);
int    kk_remove(const char *path);
int    kk_rename(const char *from, const char *to);
int    kk_mkdir(const char *path);
int    kk_rmdir(const char *path);
int    kk_chdir(const char *path);
int    kk_stat(const char *path, struct stat *st);
char  *kk_tempnam(const char *dir, const char *prefix);

/* The spool stream for printer port 0 (LPT1/PRN), 1 (LPT2) or 2 (LPT3).
 * Output accumulates until the job is closed or the program goes idle,
 * then it is handed to the host print command (see docs/PORTING.md).   */
FILE  *kk_printer_stream(int port);

/* The stream that stands for the CON device: output goes to the screen. */
FILE  *kk_console_stream(void);

/* Submit finished print jobs; called from the input routines.           */
void   kk_printer_idle(void);

/* Borland style region locking: 0 on success, -1 (errno=EACCES) if the
 * region is locked by somebody else.                                   */
int    lock(int handle, long offset, long length);
int    unlock(int handle, long offset, long length);

#ifndef _WIN32
/* Borland/MS run-time functions that POSIX lacks.                       */
int    eof(int handle);
long   filelength(int handle);
long   tell(int handle);
int    _commit(int handle);
int    chsize(int handle, long size);
char  *_fullpath(char *absPath, const char *relPath, size_t maxLength);
void   _splitpath(const char *path, char *drive, char *dir, char *fname, char *ext);
char  *strupr(char *s);
char  *strlwr(char *s);
int    stricmp(const char *a, const char *b);
int    strnicmp(const char *a, const char *b, size_t n);
int    strcmpi(const char *a, const char *b);
char  *itoa(int value, char *str, int radix);
char  *ltoa(long value, char *str, int radix);
char  *ultoa(unsigned long value, char *str, int radix);
#endif

/* Bumped by every file and screen operation; the input routines use it
 * to tell an idle keyboard poll from a busy loop that peeks at the
 * keyboard while working (see kk_console.cpp).                          */
extern volatile unsigned long kk_activity_counter;

#ifdef __cplusplus
}
#endif

#endif /* KK_PLATFORM_H */
