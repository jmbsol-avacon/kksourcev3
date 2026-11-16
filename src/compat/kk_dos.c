/*
 * kk_dos.c - DOS and Borland run-time services on a modern host:
 * date/time, speaker, directory search, disk space, drives, region
 * locking, the <io.h> extras, string helpers, path translation and the
 * low level file calls used by the application.
 */
#if !defined(_WIN32) && !defined(_DEFAULT_SOURCE)
#define _DEFAULT_SOURCE 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <io.h>
#  include <direct.h>
#  include <sys/locking.h>
#  define KK_PATH_MAX MAX_PATH
#else
#  include <unistd.h>
#  include <dirent.h>
#  include <glob.h>
#  include <strings.h>
#  include <sys/statvfs.h>
#  include <sys/time.h>
#  include <limits.h>
#  ifndef PATH_MAX
#    define PATH_MAX 4096
#  endif
#  define KK_PATH_MAX PATH_MAX
#endif

#include "include/kk_platform.h"
#include "include/dos.h"
#include "include/dir.h"
#include "kk_internal.h"

/* The Microsoft C run-time has the S_IF* mode bits but not the POSIX test macros. */
#ifndef S_ISDIR
#  define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif

/* ------------------------------------------------------------------ */
/* Date and time                                                       */
/* ------------------------------------------------------------------ */

void getdate(struct date *d)
{
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    d->da_year = lt->tm_year + 1900;
    d->da_mon = (char)(lt->tm_mon + 1);
    d->da_day = (char)lt->tm_mday;
}

void gettime(struct time *t)
{
#ifdef _WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);
    t->ti_hour = (unsigned char)st.wHour;
    t->ti_min = (unsigned char)st.wMinute;
    t->ti_sec = (unsigned char)st.wSecond;
    t->ti_hund = (unsigned char)(st.wMilliseconds / 10);
#else
    struct timeval tv;
    struct tm *lt;
    gettimeofday(&tv, NULL);
    lt = localtime(&tv.tv_sec);
    t->ti_hour = (unsigned char)lt->tm_hour;
    t->ti_min = (unsigned char)lt->tm_min;
    t->ti_sec = (unsigned char)lt->tm_sec;
    t->ti_hund = (unsigned char)(tv.tv_usec / 10000);
#endif
}

void setdate(struct date *d) { (void)d; }   /* the system clock is not ours to set */
void settime(struct time *t) { (void)t; }

/* ------------------------------------------------------------------ */
/* Timing and speaker                                                  */
/* ------------------------------------------------------------------ */

static unsigned g_tone = 0;

static void sleep_ms(unsigned ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

void delay(unsigned milliseconds)
{
    KK_ConsoleFlush();
    if (g_tone)
        kk_console_beep(g_tone, milliseconds);   /* sound()/delay()/nosound() idiom */
    else
        sleep_ms(milliseconds);
}

void sound(unsigned frequency) { g_tone = frequency; }
void nosound(void) { g_tone = 0; }

/* ------------------------------------------------------------------ */
/* Memory statistics                                                   */
/* ------------------------------------------------------------------ */

unsigned long coreleft(void) { return 0x7FFF0000UL; }
unsigned long farcoreleft(void) { return 0x7FFF0000UL; }
unsigned stackavail(void) { return 60000u; }

/* ------------------------------------------------------------------ */
/* Path translation                                                    */
/* ------------------------------------------------------------------ */

#ifndef _WIN32
/*
 * DOS file names were case-insensitive.  When a path does not exist as
 * written, look for a component that matches ignoring case.
 */
static void resolve_case(char *path, size_t size)
{
    char work[PATH_MAX];
    char *comp, *save;
    size_t len;

    if (access(path, F_OK) == 0 || strlen(path) >= sizeof work)
        return;

    strcpy(work, path);
    len = 0;
    path[0] = '\0';
    if (work[0] == '/') {
        strcpy(path, "/");
        len = 1;
    }
    for (comp = strtok_r(work, "/", &save); comp != NULL; comp = strtok_r(NULL, "/", &save)) {
        char candidate[PATH_MAX];
        struct stat st;

        snprintf(candidate, sizeof candidate, "%s%s", path, comp);
        if (lstat(candidate, &st) != 0) {
            const char *dirName = (len == 0) ? "." : path;
            DIR *dir = opendir(dirName);
            if (dir != NULL) {
                struct dirent *e;
                while ((e = readdir(dir)) != NULL) {
                    if (strcasecmp(e->d_name, comp) == 0) {
                        snprintf(candidate, sizeof candidate, "%s%s", path, e->d_name);
                        break;
                    }
                }
                closedir(dir);
            }
        }
        if (strlen(candidate) + 2 >= size)
            return;
        strcpy(path, candidate);
        len = strlen(path);
        if (save != NULL && *save != '\0') {
            strcat(path, "/");
            len++;
        }
    }
}
#endif

/*
 * DOS ignored blanks inside 8.3 file names: the program builds names from
 * space padded table entries ("AUTOMAT " + "." + "CD ") and relied on it.
 * Remove the blanks that precede a dot or end a path component; blanks in
 * the middle of a modern directory name are left alone.
 */
static void strip_dos_blanks(char *path)
{
    char *src, *dst, *compStart;

    for (src = dst = compStart = path; ; src++) {
        if (*src == ' ') {
            const char *p = src;
            while (*p == ' ')
                p++;
            if (*p == '.' || *p == '/' || *p == '\\' || *p == '\0') {
                src = (char *)p - 1;    /* drop the run of blanks */
                continue;
            }
        }
        *dst = *src;
        if (*src == '\0')
            break;
        if (*src == '/' || *src == '\\')
            compStart = dst + 1;
        dst++;
    }
    (void)compStart;
}

void kk_native_path(char *dst, size_t dstSize, const char *src)
{
    if (dstSize == 0)
        return;
#ifdef _WIN32
    snprintf(dst, dstSize, "%s", src);
    strip_dos_blanks(dst);
#else
    {
        size_t i, j = 0;

        /* drop a drive letter, turn back-slashes into slashes */
        if (isalpha((unsigned char)src[0]) && src[1] == ':')
            src += 2;
        for (i = 0; src[i] != '\0' && j + 1 < dstSize; i++)
            dst[j++] = (src[i] == '\\') ? '/' : src[i];
        dst[j] = '\0';
    }
    strip_dos_blanks(dst);
    resolve_case(dst, dstSize);
#endif
}

/* ------------------------------------------------------------------ */
/* Low level file calls                                                */
/* ------------------------------------------------------------------ */

volatile unsigned long kk_activity_counter = 0;

int kk_open(const char *path, int oflag, ...)
{
    char native[KK_PATH_MAX];
    int mode = 0;

    if (oflag & O_CREAT) {
        va_list ap;
        va_start(ap, oflag);
        mode = va_arg(ap, int);
        va_end(ap);
    }
    kk_native_path(native, sizeof native, path);
    kk_activity_counter++;
#ifdef _WIN32
    return _open(native, oflag, mode);
#else
    return open(native, oflag, mode);
#endif
}

int kk_read(int fd, void *buf, unsigned count)
{
    kk_activity_counter++;
#ifdef _WIN32
    return _read(fd, buf, count);
#else
    return (int)read(fd, buf, count);
#endif
}

int kk_write(int fd, const void *buf, unsigned count)
{
    kk_activity_counter++;
#ifdef _WIN32
    return _write(fd, buf, count);
#else
    return (int)write(fd, buf, count);
#endif
}

long kk_lseek(int fd, long offset, int whence)
{
#ifdef _WIN32
    return _lseek(fd, offset, whence);
#else
    return (long)lseek(fd, offset, whence);
#endif
}

int kk_close(int fd)
{
#ifdef _WIN32
    return _close(fd);
#else
    return close(fd);
#endif
}

int kk_access(const char *path, int mode)
{
    char native[KK_PATH_MAX];
    kk_native_path(native, sizeof native, path);
#ifdef _WIN32
    return _access(native, mode);
#else
    return access(native, mode);
#endif
}

int kk_remove(const char *path)
{
    char native[KK_PATH_MAX];
    kk_native_path(native, sizeof native, path);
    kk_activity_counter++;
    return remove(native);
}

int kk_rename(const char *from, const char *to)
{
    char nfrom[KK_PATH_MAX], nto[KK_PATH_MAX];
    kk_native_path(nfrom, sizeof nfrom, from);
    kk_native_path(nto, sizeof nto, to);
    kk_activity_counter++;
    return rename(nfrom, nto);
}

int kk_mkdir(const char *path)
{
    char native[KK_PATH_MAX];
    kk_native_path(native, sizeof native, path);
#ifdef _WIN32
    return _mkdir(native);
#else
    return mkdir(native, 0777);
#endif
}

int kk_rmdir(const char *path)
{
    char native[KK_PATH_MAX];
    kk_native_path(native, sizeof native, path);
#ifdef _WIN32
    return _rmdir(native);
#else
    return rmdir(native);
#endif
}

int kk_chdir(const char *path)
{
    char native[KK_PATH_MAX];
    kk_native_path(native, sizeof native, path);
#ifdef _WIN32
    return _chdir(native);
#else
    return chdir(native);
#endif
}

int kk_stat(const char *path, struct stat *st)
{
    char native[KK_PATH_MAX];
    kk_native_path(native, sizeof native, path);
    return stat(native, st);
}

#if defined(__GNUC__) && !defined(_WIN32)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
char *kk_tempnam(const char *dir, const char *prefix)
{
    char native[KK_PATH_MAX];
    if (dir == NULL)
        return tempnam(NULL, prefix);
    kk_native_path(native, sizeof native, dir);
#ifdef _WIN32
    return _tempnam(native, prefix);
#else
    return tempnam(native, prefix);
#endif
}
#if defined(__GNUC__) && !defined(_WIN32)
#pragma GCC diagnostic pop
#endif

/* ------------------------------------------------------------------ */
/* <io.h> extras                                                       */
/* ------------------------------------------------------------------ */

#ifndef _WIN32
int eof(int handle)
{
    struct stat st;
    off_t cur = lseek(handle, 0, SEEK_CUR);
    if (cur < 0 || fstat(handle, &st) != 0)
        return -1;
    return cur >= st.st_size ? 1 : 0;
}

long filelength(int handle)
{
    struct stat st;
    if (fstat(handle, &st) != 0)
        return -1L;
    return (long)st.st_size;
}

long tell(int handle)
{
    return (long)lseek(handle, 0, SEEK_CUR);
}

int _commit(int handle)
{
    return fsync(handle);
}

int chsize(int handle, long size)
{
    return ftruncate(handle, (off_t)size);
}

/*
 * _fullpath: absolute, lexically normalised path (the file need not exist,
 * which rules out realpath()).  Returns NULL only for a NULL input.
 */
char *_fullpath(char *absPath, const char *relPath, size_t maxLength)
{
    char native[PATH_MAX], joined[PATH_MAX * 2], out[PATH_MAX * 2];
    char *comp, *save;
    size_t len = 0;

    if (relPath == NULL)
        return NULL;
    if (absPath == NULL) {
        absPath = (char *)malloc(maxLength ? maxLength : PATH_MAX);
        if (absPath == NULL)
            return NULL;
        if (maxLength == 0)
            maxLength = PATH_MAX;
    }
    kk_native_path(native, sizeof native, relPath);
    if (native[0] == '/') {
        snprintf(joined, sizeof joined, "%s", native);
    } else {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof cwd) == NULL)
            strcpy(cwd, "/");
        snprintf(joined, sizeof joined, "%s/%s", cwd, native);
    }
    out[0] = '\0';
    for (comp = strtok_r(joined, "/", &save); comp != NULL; comp = strtok_r(NULL, "/", &save)) {
        if (strcmp(comp, ".") == 0 || comp[0] == '\0')
            continue;
        if (strcmp(comp, "..") == 0) {
            char *slash = strrchr(out, '/');
            if (slash != NULL)
                *slash = '\0';
            len = strlen(out);
            continue;
        }
        if (len + strlen(comp) + 2 >= sizeof out)
            break;
        out[len++] = '/';
        strcpy(out + len, comp);
        len += strlen(comp);
    }
    if (out[0] == '\0')
        strcpy(out, "/");
    if (strlen(out) >= maxLength) {
        /* too long for the caller's buffer: fall back to the input */
        strncpy(absPath, relPath, maxLength - 1);
        absPath[maxLength - 1] = '\0';
    } else {
        strcpy(absPath, out);
    }
    return absPath;
}

void _splitpath(const char *path, char *drive, char *dir, char *fname, char *ext)
{
    const char *base, *dot, *slash;

    if (drive != NULL)
        drive[0] = '\0';
    slash = strrchr(path, '/');
    {
        const char *bs = strrchr(path, '\\');
        if (bs != NULL && (slash == NULL || bs > slash))
            slash = bs;
    }
    base = slash ? slash + 1 : path;
    if (dir != NULL) {
        size_t n = (size_t)(base - path);
        memcpy(dir, path, n);
        dir[n] = '\0';
    }
    dot = strrchr(base, '.');
    if (fname != NULL) {
        size_t n = dot ? (size_t)(dot - base) : strlen(base);
        memcpy(fname, base, n);
        fname[n] = '\0';
    }
    if (ext != NULL)
        strcpy(ext, dot ? dot : "");
}
#endif

int lock(int handle, long offset, long length)
{
#ifdef _WIN32
    long cur = _tell(handle);
    int r;
    if (cur < 0)
        return -1;
    if (_lseek(handle, offset, SEEK_SET) < 0)
        return -1;
    r = _locking(handle, _LK_NBLCK, length);
    _lseek(handle, cur, SEEK_SET);
    if (r != 0) {
        if (errno != EACCES)
            errno = EACCES;
        return -1;
    }
    return 0;
#else
    struct flock fl;
    memset(&fl, 0, sizeof fl);
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = offset;
    fl.l_len = length;
    if (fcntl(handle, F_SETLK, &fl) == -1) {
        if (errno == EAGAIN || errno == EACCES)
            errno = EACCES;
        return -1;
    }
    return 0;
#endif
}

int unlock(int handle, long offset, long length)
{
#ifdef _WIN32
    long cur = _tell(handle);
    int r;
    if (cur < 0)
        return -1;
    if (_lseek(handle, offset, SEEK_SET) < 0)
        return -1;
    r = _locking(handle, _LK_UNLCK, length);
    _lseek(handle, cur, SEEK_SET);
    return r == 0 ? 0 : -1;
#else
    struct flock fl;
    memset(&fl, 0, sizeof fl);
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = offset;
    fl.l_len = length;
    return fcntl(handle, F_SETLK, &fl) == -1 ? -1 : 0;
#endif
}

/* ------------------------------------------------------------------ */
/* String helpers missing from POSIX                                   */
/* ------------------------------------------------------------------ */

#ifndef _WIN32
char *strupr(char *s)
{
    char *p;
    for (p = s; *p; p++)
        *p = (char)toupper((unsigned char)*p);
    return s;
}

char *strlwr(char *s)
{
    char *p;
    for (p = s; *p; p++)
        *p = (char)tolower((unsigned char)*p);
    return s;
}

int stricmp(const char *a, const char *b) { return strcasecmp(a, b); }
int strnicmp(const char *a, const char *b, size_t n) { return strncasecmp(a, b, n); }
int strcmpi(const char *a, const char *b) { return strcasecmp(a, b); }

static char *radix_convert(unsigned long value, int negative, char *str, int radix)
{
    char tmp[66];
    int i = 0, j = 0;

    if (radix < 2 || radix > 36) {
        str[0] = '\0';
        return str;
    }
    do {
        int digit = (int)(value % (unsigned long)radix);
        tmp[i++] = (char)(digit < 10 ? '0' + digit : 'a' + digit - 10);
        value /= (unsigned long)radix;
    } while (value != 0);
    if (negative)
        str[j++] = '-';
    while (i > 0)
        str[j++] = tmp[--i];
    str[j] = '\0';
    return str;
}

char *itoa(int value, char *str, int radix)
{
    if (radix == 10 && value < 0)
        return radix_convert((unsigned long)(-(long)value), 1, str, radix);
    return radix_convert((unsigned long)(unsigned int)value, 0, str, radix);
}

char *ltoa(long value, char *str, int radix)
{
    if (radix == 10 && value < 0)
        return radix_convert((unsigned long)(-(value + 1)) + 1UL, 1, str, radix);
    return radix_convert((unsigned long)value, 0, str, radix);
}

char *ultoa(unsigned long value, char *str, int radix)
{
    return radix_convert(value, 0, str, radix);
}
#endif

/* ------------------------------------------------------------------ */
/* Drives and directories                                              */
/* ------------------------------------------------------------------ */

int getdisk(void)
{
#ifdef _WIN32
    return _getdrive() - 1;
#else
    return 2;   /* C: */
#endif
}

int setdisk(int drive)
{
#ifdef _WIN32
    _chdrive(drive + 1);
#else
    (void)drive;
#endif
    return 26;
}

char *getcurdir(int drive, char *directory)
{
    char cwd[KK_PATH_MAX];
    const char *p = cwd;

    (void)drive;
#ifdef _WIN32
    if (_getcwd(cwd, sizeof cwd) == NULL)
        return NULL;
    if (isalpha((unsigned char)p[0]) && p[1] == ':')
        p += 2;
#else
    if (getcwd(cwd, sizeof cwd) == NULL)
        return NULL;
#endif
    if (*p == '/' || *p == '\\')
        p++;
    strncpy(directory, p, 65);
    directory[65] = '\0';
    return directory;
}

unsigned _dos_getdiskfree(unsigned drive, struct diskfree_t *info)
{
    unsigned long long freeBytes = 0, totalBytes = 0;
    const unsigned long long cluster = 4096;
    const unsigned long long maxClusters = 0x7FFFFFFFULL / cluster;

    (void)drive;
#ifdef _WIN32
    {
        ULARGE_INTEGER avail, total, freeTotal;
        if (!GetDiskFreeSpaceExA(NULL, &avail, &total, &freeTotal))
            return 1;
        freeBytes = avail.QuadPart;
        totalBytes = total.QuadPart;
    }
#else
    {
        struct statvfs vfs;
        if (statvfs(".", &vfs) != 0)
            return 1;
        freeBytes = (unsigned long long)vfs.f_bavail * vfs.f_frsize;
        totalBytes = (unsigned long long)vfs.f_blocks * vfs.f_frsize;
    }
#endif
    info->bytes_per_sector = 512;
    info->sectors_per_cluster = (unsigned)(cluster / 512);
    freeBytes /= cluster;
    totalBytes /= cluster;
    info->avail_clusters = (unsigned)(freeBytes > maxClusters ? maxClusters : freeBytes);
    info->total_clusters = (unsigned)(totalBytes > maxClusters ? maxClusters : totalBytes);
    return 0;
}

unsigned _dos_setfileattr(const char *path, unsigned attrib)
{
    char native[KK_PATH_MAX];
    kk_native_path(native, sizeof native, path);
#ifdef _WIN32
    {
        DWORD a = FILE_ATTRIBUTE_NORMAL;
        if (attrib & FA_RDONLY) a |= FILE_ATTRIBUTE_READONLY;
        if (attrib & FA_HIDDEN) a |= FILE_ATTRIBUTE_HIDDEN;
        if (attrib & FA_SYSTEM) a |= FILE_ATTRIBUTE_SYSTEM;
        if (attrib & FA_ARCH)   a |= FILE_ATTRIBUTE_ARCHIVE;
        if (a != FILE_ATTRIBUTE_NORMAL)
            a &= ~(DWORD)FILE_ATTRIBUTE_NORMAL;
        return SetFileAttributesA(native, a) ? 0 : 1;
    }
#else
    (void)attrib;
    return access(native, F_OK) == 0 ? 0 : 1;
#endif
}

unsigned _dos_getfileattr(const char *path, unsigned *attrib)
{
    struct stat st;
    char native[KK_PATH_MAX];
    kk_native_path(native, sizeof native, path);
    if (stat(native, &st) != 0)
        return 1;
    *attrib = S_ISDIR(st.st_mode) ? FA_DIREC : FA_ARCH;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Directory search                                                    */
/* ------------------------------------------------------------------ */

static void pack_dos_time(time_t t, unsigned *wr_time, unsigned *wr_date)
{
    struct tm *lt = localtime(&t);
    *wr_time = (unsigned)((lt->tm_hour << 11) | (lt->tm_min << 5) | (lt->tm_sec / 2));
    *wr_date = (unsigned)(((lt->tm_year - 80) << 9) | ((lt->tm_mon + 1) << 5) | lt->tm_mday);
}

static void copy_short_name(char *dst, const char *name)
{
    snprintf(dst, 13, "%s", name);   /* find_t.name holds 8.3 names */
}

#ifdef _WIN32

struct find_state {
    HANDLE   handle;
    unsigned attrib;
};

static int win_accept(const WIN32_FIND_DATAA *fd, unsigned attrib)
{
    if ((fd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(attrib & FA_DIREC))
        return 0;
    if ((fd->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && !(attrib & FA_HIDDEN))
        return 0;
    if ((fd->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) && !(attrib & FA_SYSTEM))
        return 0;
    return 1;
}

static void win_fill(struct find_t *result, const WIN32_FIND_DATAA *fd)
{
    ULARGE_INTEGER t;
    unsigned long long unixTime;
    const char *name = fd->cFileName;

    if (strlen(name) > 12 && fd->cAlternateFileName[0] != '\0')
        name = fd->cAlternateFileName;
    copy_short_name(result->name, name);
    result->size = (long)fd->nFileSizeLow;
    result->attrib = 0;
    if (fd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) result->attrib |= FA_DIREC;
    if (fd->dwFileAttributes & FILE_ATTRIBUTE_READONLY)  result->attrib |= FA_RDONLY;
    if (fd->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)    result->attrib |= FA_HIDDEN;
    if (fd->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)   result->attrib |= FA_ARCH;
    t.LowPart = fd->ftLastWriteTime.dwLowDateTime;
    t.HighPart = fd->ftLastWriteTime.dwHighDateTime;
    unixTime = (t.QuadPart - 116444736000000000ULL) / 10000000ULL;
    pack_dos_time((time_t)unixTime, &result->wr_time, &result->wr_date);
}

unsigned _dos_findfirst(const char *path, unsigned attrib, struct find_t *result)
{
    WIN32_FIND_DATAA fd;
    HANDLE h;
    char native[KK_PATH_MAX];
    struct find_state *st;

    memset(result, 0, sizeof *result);
    kk_native_path(native, sizeof native, path);
    h = FindFirstFileA(native, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return 1;
    while (!win_accept(&fd, attrib)) {
        if (!FindNextFileA(h, &fd)) {
            FindClose(h);
            return 1;
        }
    }
    win_fill(result, &fd);
    st = (struct find_state *)malloc(sizeof *st);
    if (st == NULL) {
        FindClose(h);
        return 0;
    }
    st->handle = h;
    st->attrib = attrib;
    result->kk_handle = st;
    return 0;
}

unsigned _dos_findnext(struct find_t *result)
{
    struct find_state *st = (struct find_state *)result->kk_handle;
    WIN32_FIND_DATAA fd;

    if (st == NULL)
        return 1;
    for (;;) {
        if (!FindNextFileA(st->handle, &fd)) {
            FindClose(st->handle);
            free(st);
            result->kk_handle = NULL;
            return 1;
        }
        if (win_accept(&fd, st->attrib))
            break;
    }
    win_fill(result, &fd);
    return 0;
}

#else /* POSIX */

struct find_state {
    glob_t   g;
    size_t   next;
    unsigned attrib;
};

static int posix_fill(struct find_t *result, const char *path, unsigned attrib)
{
    struct stat st;
    const char *base;

    if (stat(path, &st) != 0)
        return 0;
    if (S_ISDIR(st.st_mode) && !(attrib & FA_DIREC))
        return 0;
    base = strrchr(path, '/');
    base = base ? base + 1 : path;
    copy_short_name(result->name, base);
    result->size = (long)(st.st_size > 0x7FFFFFFFL ? 0x7FFFFFFFL : st.st_size);
    result->attrib = S_ISDIR(st.st_mode) ? FA_DIREC : FA_ARCH;
    if (!(st.st_mode & S_IWUSR))
        result->attrib |= FA_RDONLY;
    pack_dos_time(st.st_mtime, &result->wr_time, &result->wr_date);
    return 1;
}

static int posix_advance(struct find_t *result)
{
    struct find_state *st = (struct find_state *)result->kk_handle;

    while (st->next < st->g.gl_pathc) {
        const char *path = st->g.gl_pathv[st->next++];
        if (posix_fill(result, path, st->attrib))
            return 0;
    }
    globfree(&st->g);
    free(st);
    result->kk_handle = NULL;
    return 1;
}

unsigned _dos_findfirst(const char *path, unsigned attrib, struct find_t *result)
{
    char native[KK_PATH_MAX], pattern[KK_PATH_MAX];
    struct find_state *st;
    size_t i, j;

    memset(result, 0, sizeof *result);
    kk_native_path(native, sizeof native, path);

    if (strpbrk(native, "*?") == NULL)
        return posix_fill(result, native, attrib) ? 0 : 1;

    /* DOS "*.*" means every name, including names without an extension. */
    for (i = 0, j = 0; native[i] != '\0' && j + 1 < sizeof pattern; i++) {
        if (native[i] == '*' && native[i + 1] == '.' && native[i + 2] == '*') {
            pattern[j++] = '*';
            i += 2;
        } else {
            pattern[j++] = native[i];
        }
    }
    pattern[j] = '\0';

    st = (struct find_state *)calloc(1, sizeof *st);
    if (st == NULL)
        return 1;
    st->attrib = attrib;
    if (glob(pattern, 0, NULL, &st->g) != 0) {
        globfree(&st->g);
        free(st);
        return 1;
    }
    result->kk_handle = st;
    return posix_advance(result) == 0 ? 0 : 1;
}

unsigned _dos_findnext(struct find_t *result)
{
    if (result->kk_handle == NULL)
        return 1;
    return posix_advance(result) == 0 ? 0 : 1;
}

#endif

/* ------------------------------------------------------------------ */
/* Shell and file moves                                                */
/* ------------------------------------------------------------------ */

const char *KK_ShellCommand(void)
{
#ifdef _WIN32
    const char *s = getenv("COMSPEC");
    return (s && *s) ? s : "cmd.exe";
#else
    const char *s = getenv("SHELL");
    return (s && *s) ? s : "/bin/sh";
#endif
}

int kk_system(const char *command)
{
    int r;
    kk_console_suspend();
    fflush(stdout);
    r = system(command);
    kk_console_resume();
    return r;
}

static int copy_file(const char *from, const char *to)
{
    FILE *in = fopen(from, "rb"), *out;
    char buf[16384];
    size_t n;

    if (in == NULL)
        return -1;
    out = fopen(to, "wb");
    if (out == NULL) {
        fclose(in);
        return -1;
    }
    while ((n = fread(buf, 1, sizeof buf, in)) > 0)
        fwrite(buf, 1, n, out);
    fclose(in);
    return fclose(out) == 0 ? 0 : -1;
}

int KK_MoveFiles(const char *pattern, const char *destDir)
{
    char native[KK_PATH_MAX], dir[KK_PATH_MAX], dest[KK_PATH_MAX];
    char from[KK_PATH_MAX], to[KK_PATH_MAX];
    struct find_t ft;
    char *slash;
    int moved = 0;

    kk_native_path(native, sizeof native, pattern);
    kk_native_path(dest, sizeof dest, destDir);
    strcpy(dir, native);
    slash = strrchr(dir, '/');
#ifdef _WIN32
    {
        char *bs = strrchr(dir, '\\');
        if (bs != NULL && (slash == NULL || bs > slash))
            slash = bs;
    }
#endif
    if (slash != NULL)
        slash[1] = '\0';
    else
        dir[0] = '\0';

    if (_dos_findfirst(native, 0, &ft) != 0)
        return 0;
    do {
        snprintf(from, sizeof from, "%s%s", dir, ft.name);
        snprintf(to, sizeof to, "%s%s%s", dest,
                 (dest[0] != '\0' && dest[strlen(dest) - 1] != '/' && dest[strlen(dest) - 1] != '\\') ? "/" : "",
                 ft.name);
        if (rename(from, to) != 0) {
            if (copy_file(from, to) != 0)
                return -1;
            remove(from);
        }
        moved++;
    } while (_dos_findnext(&ft) == 0);
    return moved;
}
