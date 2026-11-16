/*
 * conio.h - Borland C++ 3.1 console I/O, implemented on a virtual 80x25
 * text screen (see kk_console.cpp).
 *
 * Semantics follow the Borland library exactly where the application depends
 * on them: window() relative cursor, '\n' moves down without returning the
 * carriage, output wraps at the right edge of the window and scrolls the
 * window, gettext()/puttext() use absolute coordinates and char/attribute
 * pairs, extended keys come back from getch() as 0 followed by the scan code.
 */
#ifndef KK_CONIO_H
#define KK_CONIO_H

#ifdef __cplusplus
extern "C" {
#endif

enum COLORS {
    BLACK, BLUE, GREEN, CYAN, RED, MAGENTA, BROWN, LIGHTGRAY,
    DARKGRAY, LIGHTBLUE, LIGHTGREEN, LIGHTCYAN, LIGHTRED, LIGHTMAGENTA, YELLOW, WHITE
};
#define BLINK 128

enum text_modes { LASTMODE = -1, BW40 = 0, C40, BW80, C80, MONO = 7, C4350 = 64 };

#define _NOCURSOR     0
#define _SOLIDCURSOR  1
#define _NORMALCURSOR 2

struct text_info {
    unsigned char winleft;
    unsigned char wintop;
    unsigned char winright;
    unsigned char winbottom;
    unsigned char attribute;
    unsigned char normattr;
    unsigned char currmode;
    unsigned char screenheight;
    unsigned char screenwidth;
    unsigned char curx;
    unsigned char cury;
};

extern int directvideo;
extern int _wscroll;

void clrscr(void);
void clreol(void);
void delline(void);
void insline(void);
void gotoxy(int x, int y);
int  wherex(void);
int  wherey(void);
void window(int left, int top, int right, int bottom);
void textattr(int attr);
void textcolor(int color);
void textbackground(int color);
void highvideo(void);
void lowvideo(void);
void normvideo(void);
void textmode(int mode);
void gettextinfo(struct text_info *info);
/* GCC knows a built-in gettext() (libintl); this is Borland's, not that one. */
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
#endif
int  gettext(int left, int top, int right, int bottom, void *dest);
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic pop
#endif
int  puttext(int left, int top, int right, int bottom, void *source);
int  movetext(int left, int top, int right, int bottom, int destleft, int desttop);
int  cprintf(const char *format, ...);
int  cputs(const char *s);
int  putch(int c);
int  getch(void);
int  getche(void);
int  kbhit(void);
int  ungetch(int c);
void _setcursortype(int type);

#ifdef __cplusplus
}
#endif

#endif /* KK_CONIO_H */
