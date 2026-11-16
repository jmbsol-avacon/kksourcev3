/*
 * kk_internal.h - functions shared between the platform layer modules.
 * Not for use by application code.
 */
#ifndef KK_INTERNAL_H
#define KK_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Code page 437 glyphs (the character set of the DOS text screen). */
extern const unsigned short kk_cp437_unicode[256];
unsigned char kk_unicode_to_cp437(unsigned codepoint);

/* Console services needed by the C modules. */
void kk_console_beep(unsigned hz, unsigned ms);   /* blocks for 'ms'       */
void kk_console_suspend(void);                    /* before running a child */
void kk_console_resume(void);                     /* after the child exits  */

/* Printer spooling (kk_stdio.c). */
void kk_printer_flush_all(void);

#ifdef __cplusplus
}
#endif

#endif /* KK_INTERNAL_H */
