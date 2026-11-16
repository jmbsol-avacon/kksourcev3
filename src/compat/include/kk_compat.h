/*
 * kk_compat.h - force-included into every legacy translation unit.
 *
 * This header makes the 1994 Borland C++ 3.1 sources compile unchanged
 * under a modern 64-bit C compiler (GCC, Clang, MSVC):
 *
 *   1. it pulls in every standard header the legacy code uses, so that the
 *      macros defined below never rewrite a system declaration;
 *   2. it neutralises the Borland keywords (far, near, huge, _cdecl);
 *   3. it supplies Borland/DOS constants that have no modern equivalent;
 *   4. it routes stdio and low level file calls through the platform layer
 *      (DOS device names, path translation, printer spooling).
 *
 * The platform layer itself (the sources in src/compat) must NOT include this
 * header: it calls the real C library functions.
 */
#ifndef KK_COMPAT_H
#define KK_COMPAT_H

#if defined(_MSC_VER)
#  ifndef _CRT_SECURE_NO_WARNINGS
#    define _CRT_SECURE_NO_WARNINGS 1
#  endif
#  ifndef _CRT_NONSTDC_NO_WARNINGS
#    define _CRT_NONSTDC_NO_WARNINGS 1
#  endif
#  ifndef _CRT_NONSTDC_NO_DEPRECATE
#    define _CRT_NONSTDC_NO_DEPRECATE 1
#  endif
#  pragma warning(disable : 4996) /* "deprecated" POSIX names such as open() */
#endif

/* 1. Standard headers the legacy code relies on (must precede the macros). */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(_WIN32)
#  include <io.h>
#  include <direct.h>
#  include <share.h>
#  include <process.h>
#else
#  include <unistd.h>
#endif

#include "kk_platform.h"

/* 2. Borland keywords and calling conventions. */
#define far
#define near
#define huge
#ifndef _cdecl
#  define _cdecl
#endif
#ifndef cdecl
#  define cdecl
#endif

/* 3. Constants from the Borland run-time. */
#ifndef CLK_TCK
#  define CLK_TCK CLOCKS_PER_SEC
#endif
#ifndef O_BINARY
#  define O_BINARY 0
#endif
#ifndef O_TEXT
#  define O_TEXT 0
#endif
/*
 * DOS file sharing modes.  The legacy code ORs them into open() flags, which
 * Borland accepted.  Modern C libraries open files in "deny none" mode by
 * default and (on Windows) reuse the same bit values for other purposes
 * (_O_TEMPORARY!), so the sharing bits are forced to zero everywhere.
 */
#undef SH_COMPAT
#undef SH_DENYRW
#undef SH_DENYWR
#undef SH_DENYRD
#undef SH_DENYNO
#undef SH_DENYNONE
#define SH_COMPAT   0
#define SH_DENYRW   0
#define SH_DENYWR   0
#define SH_DENYRD   0
#define SH_DENYNO   0
#define SH_DENYNONE 0
#define O_DENYALL   0
#define O_DENYWRITE 0
#define O_DENYREAD  0
#define O_DENYNONE  0
#ifndef S_IREAD
#  define S_IREAD  S_IRUSR
#endif
#ifndef S_IWRITE
#  define S_IWRITE S_IWUSR
#endif
#ifndef MAXPATH
#  define MAXPATH 260
#endif
#ifndef MAXDRIVE
#  define MAXDRIVE 3
#endif
#ifndef MAXDIR
#  define MAXDIR 256
#endif
#ifndef MAXFILE
#  define MAXFILE 9
#endif
#ifndef MAXEXT
#  define MAXEXT 5
#endif

/* Borland's printer stream. */
#define stdprn (kk_printer_stream(0))

/* Windows C run-times spell the non-standard string functions with an
 * underscore; the old names exist too but are flagged as deprecated.      */
#if defined(_MSC_VER)
#  define stricmp  _stricmp
#  define strnicmp _strnicmp
#  define strcmpi  _stricmp
#  define strupr   _strupr
#  define strlwr   _strlwr
#  define itoa     _itoa
#  define ltoa     _ltoa
#  define ultoa    _ultoa
#  define strdup   _strdup
#endif

/* 4. Route stdio and file system calls through the platform layer. */
#undef fopen
#undef fclose
#undef fflush
#undef fprintf
#undef vfprintf
#undef fputc
#undef putc
#undef fputs
#undef fwrite
#define fopen(p, m)         kk_fopen((p), (m))
#define fclose(f)           kk_fclose(f)
#define fflush(f)           kk_fflush(f)
#define fprintf             kk_fprintf
#define vfprintf(f, fm, ap) kk_vfprintf((f), (fm), (ap))
#define fputc(c, f)         kk_fputc((c), (f))
#define putc(c, f)          kk_fputc((c), (f))
#define fputs(s, f)         kk_fputs((s), (f))
#define fwrite(p, s, n, f)  kk_fwrite((p), (s), (n), (f))

#undef open
#undef read
#undef write
#undef lseek
#undef close
#undef access
#undef remove
#undef unlink
#undef rename
#undef mkdir
#undef rmdir
#undef chdir
#undef stat
#undef tempnam
#undef system
#define open(...)           kk_open(__VA_ARGS__)
#define read(f, b, n)       kk_read((f), (b), (n))
#define write(f, b, n)      kk_write((f), (b), (n))
#define lseek(f, o, w)      kk_lseek((f), (o), (w))
#define close(f)            kk_close(f)
#define access(p, m)        kk_access((p), (m))
#define remove(p)           kk_remove(p)
#define unlink(p)           kk_remove(p)
#define rename(a, b)        kk_rename((a), (b))
#define mkdir(p)            kk_mkdir(p)
#define rmdir(p)            kk_rmdir(p)
#define chdir(p)            kk_chdir(p)
#define stat(p, b)          kk_stat((p), (b))
#define tempnam(d, p)       kk_tempnam((d), (p))
#define system(c)           kk_system(c)

#endif /* KK_COMPAT_H */
