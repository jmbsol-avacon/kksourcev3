/*
 * kk_stdio.c - stdio with DOS device semantics.
 *
 * Under DOS the program printed by opening "PRN" / "LPT2" (or by writing to
 * stdprn) and paged report output to the screen by opening "CON".  Here:
 *
 *   CON          -> a pseudo stream whose output goes to the virtual screen
 *                   with conio semantics (it respects the current window);
 *   PRN, LPTn    -> a spool file per port.  A job is handed to the host
 *                   print command when the stream is closed, when the
 *                   program has been idle for a moment after printing, and
 *                   at exit.  The command comes from the KK_LPT1_CMD..
 *                   KK_LPT3_CMD environment variables (see docs/PORTING.md);
 *   NUL          -> the null device;
 *   anything else-> a regular file (with DOS path translation).
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

#ifdef _WIN32
#  include <process.h>
#  define KK_DEVNULL "NUL"
#  define KK_PATH_MAX 260
#else
#  include <unistd.h>
#  define KK_DEVNULL "/dev/null"
#  define KK_PATH_MAX 4096
#endif

#include "include/kk_platform.h"
#include "include/conio.h"
#include "include/bios.h"
#include "kk_internal.h"

#define KK_PRINTERS 3
#define KK_PRINT_IDLE_SECONDS 2

struct spool {
    FILE  *fp;
    char   path[KK_PATH_MAX];
    int    dirty;
    time_t lastWrite;
};

static struct spool g_spool[KK_PRINTERS];
static FILE *g_con = NULL;

/* ------------------------------------------------------------------ */
/* Device detection                                                    */
/* ------------------------------------------------------------------ */

enum device { DEV_NONE, DEV_CON, DEV_NUL, DEV_PRN };

static enum device device_of(const char *name, int *port)
{
    const char *base;
    char dev[8];
    size_t len, i;

    base = strrchr(name, '/');
    {
        const char *b2 = strrchr(name, '\\');
        if (b2 != NULL && (base == NULL || b2 > base))
            base = b2;
    }
    base = base ? base + 1 : name;
    len = strlen(base);
    if (len > 0 && base[len - 1] == ':')
        len--;
    if (len == 0 || len >= sizeof dev)
        return DEV_NONE;
    for (i = 0; i < len; i++)
        dev[i] = (char)toupper((unsigned char)base[i]);
    dev[len] = '\0';

    *port = 0;
    if (strcmp(dev, "CON") == 0)
        return DEV_CON;
    if (strcmp(dev, "NUL") == 0)
        return DEV_NUL;
    if (strcmp(dev, "PRN") == 0 || strcmp(dev, "LPT1") == 0)
        return DEV_PRN;
    if (strcmp(dev, "LPT2") == 0) {
        *port = 1;
        return DEV_PRN;
    }
    if (strcmp(dev, "LPT3") == 0) {
        *port = 2;
        return DEV_PRN;
    }
    return DEV_NONE;
}

/* ------------------------------------------------------------------ */
/* Printer spooling                                                    */
/* ------------------------------------------------------------------ */

static const char *temp_dir(void)
{
    const char *d = getenv("KK_SPOOL_DIR");
    if (d == NULL || *d == '\0') d = getenv("TMPDIR");
    if (d == NULL || *d == '\0') d = getenv("TEMP");
    if (d == NULL || *d == '\0') d = getenv("TMP");
#ifdef _WIN32
    if (d == NULL || *d == '\0') d = ".";
#else
    if (d == NULL || *d == '\0') d = "/tmp";
#endif
    return d;
}

FILE *kk_printer_stream(int port)
{
    struct spool *s;

    if (port < 0 || port >= KK_PRINTERS)
        port = 0;
    s = &g_spool[port];
    if (s->fp == NULL) {
        snprintf(s->path, sizeof s->path, "%s/kk_lpt%d_%ld.prn", temp_dir(), port + 1,
#ifdef _WIN32
                 (long)_getpid());
#else
                 (long)getpid());
#endif
        s->fp = fopen(s->path, "w+b");
        if (s->fp == NULL) {
            /* last resort: spool next to the data files */
            snprintf(s->path, sizeof s->path, "kk_lpt%d.spool", port + 1);
            s->fp = fopen(s->path, "w+b");
        }
        s->dirty = 0;
    }
    return s->fp;
}

static int spool_index(FILE *fp)
{
    int i;
    if (fp == NULL)
        return -1;
    for (i = 0; i < KK_PRINTERS; i++)
        if (g_spool[i].fp == fp)
            return i;
    return -1;
}

static void log_error(const char *format, ...)
{
    FILE *log = fopen("AUTOMAT.ERR", "at");
    va_list ap;
    time_t now = time(NULL);

    if (log == NULL)
        return;
    fprintf(log, "\n%s", ctime(&now));
    va_start(ap, format);
    vfprintf(log, format, ap);
    va_end(ap);
    fputc('\n', log);
    fclose(log);
}

static int append_file(const char *from, const char *to)
{
    FILE *in = fopen(from, "rb"), *out;
    char buf[8192];
    size_t n;

    if (in == NULL)
        return -1;
    out = fopen(to, "ab");
    if (out == NULL) {
        fclose(in);
        return -1;
    }
    while ((n = fread(buf, 1, sizeof buf, in)) > 0)
        fwrite(buf, 1, n, out);
    fclose(in);
    return fclose(out) == 0 ? 0 : -1;
}

static void submit_job(int port)
{
    struct spool *s = &g_spool[port];
    char var[32], cmd[KK_PATH_MAX * 2 + 64], fallback[32];
    const char *user, *p;
    long size;
    int rc = 0;

    if (s->fp == NULL)
        return;
    fflush(s->fp);
    fseek(s->fp, 0L, SEEK_END);
    size = ftell(s->fp);
    s->dirty = 0;
    if (size <= 0)
        return;

    fclose(s->fp);
    s->fp = NULL;

    snprintf(var, sizeof var, "KK_LPT%d_CMD", port + 1);
    user = getenv(var);
    if (user != NULL && strncmp(user, "file:", 5) == 0) {
        rc = append_file(s->path, user + 5);
    } else {
        if (user == NULL || *user == '\0') {
#ifdef _WIN32
            user = "notepad /p \"%s\"";
#else
            user = "lp -s \"%s\"";
#endif
        }
        p = strstr(user, "%s");
        if (p != NULL) {
            snprintf(cmd, sizeof cmd, "%.*s%s%s", (int)(p - user), user, s->path, p + 2);
        } else {
            snprintf(cmd, sizeof cmd, "%s \"%s\"", user, s->path);
        }
        if (strchr(cmd, '>') == NULL) {
#ifdef _WIN32
            strncat(cmd, " >NUL 2>&1", sizeof cmd - strlen(cmd) - 1);
#else
            strncat(cmd, " >/dev/null 2>&1", sizeof cmd - strlen(cmd) - 1);
#endif
        }
        rc = system(cmd);
    }

    if (rc != 0) {
        /* Never lose a document: keep it next to the data files. */
        snprintf(fallback, sizeof fallback, "LPT%d.PRN", port + 1);
        append_file(s->path, fallback);
        log_error("Print command failed (%s); output appended to %s", var, fallback);
    }

    /* start a fresh spool file for the next job */
    s->fp = fopen(s->path, "w+b");
    s->dirty = 0;
}

void kk_printer_idle(void)
{
    time_t now = time(NULL);
    int i;
    for (i = 0; i < KK_PRINTERS; i++)
        if (g_spool[i].fp != NULL && g_spool[i].dirty && now - g_spool[i].lastWrite >= KK_PRINT_IDLE_SECONDS)
            submit_job(i);
}

void kk_printer_flush_all(void)
{
    int i;
    for (i = 0; i < KK_PRINTERS; i++) {
        if (g_spool[i].fp != NULL) {
            if (g_spool[i].dirty)
                submit_job(i);
            if (g_spool[i].fp != NULL)
                fclose(g_spool[i].fp);
            g_spool[i].fp = NULL;
            remove(g_spool[i].path);
        }
    }
}

static void mark_written(int port)
{
    g_spool[port].dirty = 1;
    g_spool[port].lastWrite = time(NULL);
}

int biosprint(int cmd, int abyte, int port)
{
    if (cmd == 0) {
        FILE *fp = kk_printer_stream(port);
        if (fp != NULL) {
            fputc(abyte, fp);
            mark_written(port < 0 || port >= KK_PRINTERS ? 0 : port);
        }
    }
    return 0x90;   /* selected, acknowledged, no error */
}

/* ------------------------------------------------------------------ */
/* The CON pseudo stream                                               */
/* ------------------------------------------------------------------ */

FILE *kk_console_stream(void)
{
    if (g_con == NULL)
        g_con = fopen(KK_DEVNULL, "w");
    return g_con;
}

static int is_con(FILE *fp) { return fp != NULL && fp == g_con; }

/* ------------------------------------------------------------------ */
/* stdio replacements                                                  */
/* ------------------------------------------------------------------ */

FILE *kk_fopen(const char *name, const char *mode)
{
    char native[KK_PATH_MAX];
    int port = 0;
    FILE *fp;

    kk_activity_counter++;
    switch (device_of(name, &port)) {
    case DEV_CON:
        return kk_console_stream();
    case DEV_NUL:
        return fopen(KK_DEVNULL, mode);
    case DEV_PRN:
        return kk_printer_stream(port);
    default:
        break;
    }
    kk_native_path(native, sizeof native, name);
    fp = fopen(native, mode);
    return fp;
}

int kk_fclose(FILE *fp)
{
    int port;

    kk_activity_counter++;
    if (fp == NULL)
        return EOF;
    if (is_con(fp))
        return 0;
    port = spool_index(fp);
    if (port >= 0) {
        /* closing PRN ends the print job */
        if (g_spool[port].dirty)
            submit_job(port);
        return 0;
    }
    return fclose(fp);
}

int kk_fflush(FILE *fp)
{
    if (fp != NULL && is_con(fp)) {
        KK_ConsoleFlush();
        return 0;
    }
    return fflush(fp);
}

int kk_vfprintf(FILE *fp, const char *format, va_list ap)
{
    int port, n;

    kk_activity_counter++;
    if (is_con(fp)) {
        char buf[4096];
        n = vsnprintf(buf, sizeof buf, format, ap);
        if (n > 0)
            cputs(buf);
        return n;
    }
    n = vfprintf(fp, format, ap);
    port = spool_index(fp);
    if (port >= 0)
        mark_written(port);
    return n;
}

int kk_fprintf(FILE *fp, const char *format, ...)
{
    va_list ap;
    int n;
    va_start(ap, format);
    n = kk_vfprintf(fp, format, ap);
    va_end(ap);
    return n;
}

int kk_fputc(int c, FILE *fp)
{
    int port;

    kk_activity_counter++;
    if (is_con(fp)) {
        putch(c);
        return (unsigned char)c;
    }
    c = fputc(c, fp);
    port = spool_index(fp);
    if (port >= 0)
        mark_written(port);
    return c;
}

int kk_fputs(const char *s, FILE *fp)
{
    int port, r;

    kk_activity_counter++;
    if (is_con(fp)) {
        cputs(s);
        return 0;
    }
    r = fputs(s, fp);
    port = spool_index(fp);
    if (port >= 0)
        mark_written(port);
    return r;
}

size_t kk_fwrite(const void *ptr, size_t size, size_t n, FILE *fp)
{
    int port;
    size_t r;

    kk_activity_counter++;
    if (is_con(fp)) {
        const unsigned char *p = (const unsigned char *)ptr;
        size_t total = size * n, i;
        for (i = 0; i < total; i++)
            putch(p[i]);
        return n;
    }
    r = fwrite(ptr, size, n, fp);
    port = spool_index(fp);
    if (port >= 0)
        mark_written(port);
    return r;
}
