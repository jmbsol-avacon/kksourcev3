/*
 * kk_console.cpp - the virtual 80x25 text screen and the Borland conio API.
 *
 * The original program wrote straight into the CGA/VGA text buffer through
 * the Borland conio library.  Here the same operations update an in-memory
 * copy of that buffer (character + attribute per cell, exactly the layout
 * gettext()/puttext() expect), and the terminal back end paints the cells
 * that changed whenever the program waits for input.
 */
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "kk_backend.h"
#include "kk_internal.h"
#include "include/kk_platform.h"
#include "include/conio.h"
#include "include/bios.h"

using namespace kk;

/* Borland globals the application may reference. */
int directvideo = 1;
int _wscroll = 1;

namespace {

Cell        g_screen[ROWS * COLS];
int         g_wl = 1, g_wt = 1, g_wr = COLS, g_wb = ROWS;  /* window, 1-based inclusive */
int         g_cx = 1, g_cy = 1;                            /* cursor, window relative    */
uint8_t     g_attr = LIGHTGRAY;
const uint8_t g_normattr = LIGHTGRAY;
CursorShape g_cursor = CURSOR_NORMAL;
bool        g_dirty = true;
bool        g_full = true;
int         g_lastCurX = -1, g_lastCurY = -1;
CursorShape g_lastCursor = CURSOR_HIDDEN;
bool        g_inited = false;
bool        g_shutdown = false;
bool        g_suspended = false;
Backend    *g_backend = nullptr;
int         g_pendingScan = -1;
InputState  g_input;

void ensureInit()
{
    if (g_inited)
        return;
    g_inited = true;
    for (Cell &c : g_screen) {
        c.ch = ' ';
        c.attr = g_normattr;
    }
    std::fflush(stdout);
    g_backend = createBackend();
    std::string err;
    if (!g_backend->init(err)) {
        std::fprintf(stderr, "Kwik Kuote: cannot initialise the console: %s\n", err.c_str());
        std::exit(1);
    }
    std::atexit(KK_PlatformShutdown);
    g_dirty = g_full = true;
}

inline int winWidth()  { return g_wr - g_wl + 1; }
inline int winHeight() { return g_wb - g_wt + 1; }
inline Cell &cellAt(int col0, int row0) { return g_screen[row0 * COLS + col0]; }

void fillRow(int row0, int colFrom0, int colTo0, uint8_t attr)
{
    for (int c = colFrom0; c <= colTo0; c++) {
        cellAt(c, row0).ch = ' ';
        cellAt(c, row0).attr = attr;
    }
}

/* Scroll an absolute, 0-based, inclusive region. lines == 0 clears it. */
void scrollRegion(bool up, int lines, int top, int left, int bottom, int right, uint8_t attr)
{
    int height = bottom - top + 1;
    if (height <= 0 || right < left)
        return;
    if (lines <= 0 || lines >= height) {
        for (int r = top; r <= bottom; r++)
            fillRow(r, left, right, attr);
    } else if (up) {
        for (int r = top; r <= bottom - lines; r++)
            std::memcpy(&cellAt(left, r), &cellAt(left, r + lines), (right - left + 1) * sizeof(Cell));
        for (int r = bottom - lines + 1; r <= bottom; r++)
            fillRow(r, left, right, attr);
    } else {
        for (int r = bottom; r >= top + lines; r--)
            std::memcpy(&cellAt(left, r), &cellAt(left, r - lines), (right - left + 1) * sizeof(Cell));
        for (int r = top; r < top + lines; r++)
            fillRow(r, left, right, attr);
    }
    g_dirty = true;
}

void putRaw(unsigned char c)
{
    switch (c) {
    case '\r':
        g_cx = 1;
        return;
    case '\n':
        g_cy++;
        break;
    case '\b':
        if (g_cx > 1)
            g_cx--;
        return;
    case 7:
        kk_console_beep(1000, 100);
        return;
    default: {
        Cell &cell = cellAt(g_wl - 1 + g_cx - 1, g_wt - 1 + g_cy - 1);
        cell.ch = c;
        cell.attr = g_attr;
        g_dirty = true;
        g_cx++;
        if (g_cx > winWidth()) {
            g_cx = 1;
            g_cy++;
        }
        break;
    }
    }
    if (g_cy > winHeight()) {
        if (_wscroll)
            scrollRegion(true, 1, g_wt - 1, g_wl - 1, g_wb - 1, g_wr - 1, g_attr);
        g_cy = winHeight();
    }
}

void flushIfNeeded()
{
    if (!g_inited || g_suspended)
        return;
    int curX = g_wl - 1 + g_cx - 1;
    int curY = g_wt - 1 + g_cy - 1;
    if (!g_dirty && !g_full && curX == g_lastCurX && curY == g_lastCurY && g_cursor == g_lastCursor)
        return;
    g_backend->render(g_screen, curX, curY, g_cursor, g_full);
    g_dirty = g_full = false;
    g_lastCurX = curX;
    g_lastCurY = curY;
    g_lastCursor = g_cursor;
}

/*
 * Wait for keyboard input.  A poll with timeout 0 is used while the program
 * is busy (printing, searching) and peeks at the keyboard to allow an
 * abort; a short wait is used when the program is merely idling in its
 * input loop, which keeps the CPU load negligible.
 */
bool pollKeys(int timeoutMs)
{
    ensureInit();
    flushIfNeeded();
    g_backend->pump(timeoutMs);
    kk_printer_idle();
    return !g_input.keys.empty();
}

} // namespace

/* ------------------------------------------------------------------ */
/* Input state shared with the back ends                               */
/* ------------------------------------------------------------------ */

namespace kk {

InputState &input() { return g_input; }

void InputState::pushKey(uint16_t code)
{
    if (code == 0)
        return;
    if (code == KEY_CODE_INS)
        insertOn = !insertOn;
    keys.push_back(code);
}

void InputState::mouseMove(int col, int row)
{
    mouseX = col * 8;
    mouseY = row * 8;
}

void InputState::mousePress(int button, int col, int row)
{
    if (button < 0 || button > 2)
        return;
    mouseMove(col, row);
    buttons[button].down = true;
    buttons[button].pressX = mouseX;
    buttons[button].pressY = mouseY;
}

void InputState::mouseRelease(int button, int col, int row)
{
    if (button < 0 || button > 2)
        return;
    mouseMove(col, row);
    buttons[button].down = false;
    buttons[button].releaseX = mouseX;
    buttons[button].releaseY = mouseY;
}

} // namespace kk

/* ------------------------------------------------------------------ */
/* Platform life cycle and console extras                              */
/* ------------------------------------------------------------------ */

extern "C" {

void KK_PlatformInit(void)
{
    ensureInit();
}

void KK_PlatformShutdown(void)
{
    if (g_shutdown)
        return;
    g_shutdown = true;
    kk_printer_flush_all();
    if (g_inited && g_backend) {
        if (!g_suspended) {
            g_full = true;
            flushIfNeeded();
            g_backend->shutdown();
        }
        delete g_backend;
        g_backend = nullptr;
    }
}

void KK_ConsoleFlush(void)
{
    if (g_inited)
        flushIfNeeded();
}

void kk_console_suspend(void)
{
    if (!g_inited || g_suspended)
        return;
    flushIfNeeded();
    g_backend->suspend();
    g_suspended = true;
}

void kk_console_resume(void)
{
    if (!g_inited || !g_suspended)
        return;
    g_backend->resume();
    g_suspended = false;
    g_full = g_dirty = true;
    flushIfNeeded();
}

void kk_console_beep(unsigned hz, unsigned ms)
{
    ensureInit();
    flushIfNeeded();
    g_backend->beep(hz, ms);
}

void KK_SetCursor(unsigned cursorCode)
{
    ensureInit();
    switch (cursorCode) {
    case 0x0090:                       /* CUR_OFF */
        g_cursor = CURSOR_HIDDEN;
        break;
    case 0x0004:                       /* CUR_FUL: insert mode block cursor */
        g_cursor = CURSOR_BLOCK;
        break;
    default:                           /* CUR_ON, CUR_TOP */
        g_cursor = CURSOR_NORMAL;
        break;
    }
}

void KK_BiosScroll(int up, int lines, int top, int left, int bottom, int right, int attr)
{
    ensureInit();
    if (top < 0) top = 0;
    if (left < 0) left = 0;
    if (bottom > ROWS - 1) bottom = ROWS - 1;
    if (right > COLS - 1) right = COLS - 1;
    scrollRegion(up != 0, lines, top, left, bottom, right, (uint8_t)attr);
}

void KK_BiosGotoXY(int x0, int y0)
{
    ensureInit();
    int cx = x0 + 1 - (g_wl - 1);
    int cy = y0 + 1 - (g_wt - 1);
    if (cx < 1) cx = 1;
    if (cy < 1) cy = 1;
    if (cx > winWidth()) cx = winWidth();
    if (cy > winHeight()) cy = winHeight();
    g_cx = cx;
    g_cy = cy;
}

/* ------------------------------------------------------------------ */
/* conio                                                               */
/* ------------------------------------------------------------------ */

void clrscr(void)
{
    ensureInit();
    for (int r = g_wt - 1; r <= g_wb - 1; r++)
        fillRow(r, g_wl - 1, g_wr - 1, g_attr);
    g_cx = g_cy = 1;
    g_dirty = true;
    kk_activity_counter++;
}

void clreol(void)
{
    ensureInit();
    fillRow(g_wt - 1 + g_cy - 1, g_wl - 1 + g_cx - 1, g_wr - 1, g_attr);
    g_dirty = true;
    kk_activity_counter++;
}

void delline(void)
{
    ensureInit();
    scrollRegion(true, 1, g_wt - 1 + g_cy - 1, g_wl - 1, g_wb - 1, g_wr - 1, g_attr);
}

void insline(void)
{
    ensureInit();
    scrollRegion(false, 1, g_wt - 1 + g_cy - 1, g_wl - 1, g_wb - 1, g_wr - 1, g_attr);
}

void gotoxy(int x, int y)
{
    ensureInit();
    if (x < 1 || y < 1 || x > winWidth() || y > winHeight())
        return;                        /* Borland ignores invalid coordinates */
    g_cx = x;
    g_cy = y;
}

int wherex(void) { ensureInit(); return g_cx; }
int wherey(void) { ensureInit(); return g_cy; }

void window(int left, int top, int right, int bottom)
{
    ensureInit();
    if (left < 1 || top < 1 || right > COLS || bottom > ROWS || left > right || top > bottom)
        return;
    g_wl = left;
    g_wt = top;
    g_wr = right;
    g_wb = bottom;
    g_cx = g_cy = 1;
}

void textattr(int attr)        { ensureInit(); g_attr = (uint8_t)attr; }
void textcolor(int color)      { ensureInit(); g_attr = (uint8_t)((g_attr & 0xf0) | (color & 0x0f)); }
void textbackground(int color) { ensureInit(); g_attr = (uint8_t)((g_attr & 0x8f) | ((color & 0x07) << 4)); }
void highvideo(void)           { ensureInit(); g_attr |= 0x08; }
void lowvideo(void)            { ensureInit(); g_attr &= (uint8_t)~0x08; }
void normvideo(void)           { ensureInit(); g_attr = g_normattr; }
void textmode(int mode)        { (void)mode; ensureInit(); window(1, 1, COLS, ROWS); clrscr(); }

void gettextinfo(struct text_info *info)
{
    ensureInit();
    info->winleft = (unsigned char)g_wl;
    info->wintop = (unsigned char)g_wt;
    info->winright = (unsigned char)g_wr;
    info->winbottom = (unsigned char)g_wb;
    info->attribute = g_attr;
    info->normattr = g_normattr;
    info->currmode = C80;
    info->screenheight = ROWS;
    info->screenwidth = COLS;
    info->curx = (unsigned char)g_cx;
    info->cury = (unsigned char)g_cy;
}

static bool validRect(int left, int top, int right, int bottom)
{
    return left >= 1 && top >= 1 && right <= COLS && bottom <= ROWS && left <= right && top <= bottom;
}

int gettext(int left, int top, int right, int bottom, void *dest)
{
    ensureInit();
    if (!validRect(left, top, right, bottom))
        return 0;
    unsigned char *d = static_cast<unsigned char *>(dest);
    for (int r = top - 1; r <= bottom - 1; r++)
        for (int c = left - 1; c <= right - 1; c++) {
            *d++ = cellAt(c, r).ch;
            *d++ = cellAt(c, r).attr;
        }
    return 1;
}

int puttext(int left, int top, int right, int bottom, void *source)
{
    ensureInit();
    if (!validRect(left, top, right, bottom))
        return 0;
    const unsigned char *s = static_cast<const unsigned char *>(source);
    for (int r = top - 1; r <= bottom - 1; r++)
        for (int c = left - 1; c <= right - 1; c++) {
            cellAt(c, r).ch = *s++;
            cellAt(c, r).attr = *s++;
        }
    g_dirty = true;
    kk_activity_counter++;
    return 1;
}

int movetext(int left, int top, int right, int bottom, int destleft, int desttop)
{
    ensureInit();
    int w = right - left + 1, h = bottom - top + 1;
    if (!validRect(left, top, right, bottom) || !validRect(destleft, desttop, destleft + w - 1, desttop + h - 1))
        return 0;
    std::string buf((size_t)w * h * 2, '\0');
    gettext(left, top, right, bottom, &buf[0]);
    puttext(destleft, desttop, destleft + w - 1, desttop + h - 1, &buf[0]);
    return 1;
}

int cputs(const char *s)
{
    ensureInit();
    int n = 0;
    for (const unsigned char *p = reinterpret_cast<const unsigned char *>(s); *p; p++, n++)
        putRaw(*p);
    kk_activity_counter++;
    return n;
}

int cprintf(const char *format, ...)
{
    char buf[4096];
    va_list ap;
    va_start(ap, format);
    int n = std::vsnprintf(buf, sizeof buf, format, ap);
    va_end(ap);
    if (n < 0)
        return n;
    cputs(buf);
    return n;
}

int putch(int c)
{
    ensureInit();
    putRaw((unsigned char)c);
    kk_activity_counter++;
    return c;
}

int bioskey(int cmd)
{
    ensureInit();
    switch (cmd) {
    case 0: {
        while (!pollKeys(50))
            ;
        uint16_t k = g_input.keys.front();
        g_input.keys.pop_front();
        return k;
    }
    case 1: {
        static unsigned long lastActivity = 0;
        static bool lastEmpty = false;
        bool idle = lastEmpty && kk_activity_counter == lastActivity;
        pollKeys(idle ? 10 : 0);
        lastActivity = kk_activity_counter;
        lastEmpty = g_input.keys.empty();
        return lastEmpty ? 0 : g_input.keys.front();
    }
    case 2:
        return (int)(g_backend->lockState() | g_input.modifiers | (g_input.insertOn ? 0x80 : 0));
    default:
        return 0;
    }
}

int getch(void)
{
    if (g_pendingScan >= 0) {
        int scan = g_pendingScan;
        g_pendingScan = -1;
        return scan;
    }
    int code = bioskey(0);
    int ascii = code & 0xff;
    if (ascii == 0) {
        g_pendingScan = (code >> 8) & 0xff;
        return 0;
    }
    return ascii;
}

int getche(void)
{
    int c = getch();
    if (c)
        putch(c);
    return c;
}

int kbhit(void)
{
    return g_pendingScan >= 0 || bioskey(1) != 0;
}

int ungetch(int c)
{
    ensureInit();
    g_input.keys.push_front(keyFromChar((unsigned char)c));
    return c;
}

void _setcursortype(int type)
{
    ensureInit();
    g_cursor = (type == _NOCURSOR) ? CURSOR_HIDDEN : (type == _SOLIDCURSOR) ? CURSOR_BLOCK : CURSOR_NORMAL;
}

/* ------------------------------------------------------------------ */
/* Mouse (INT 33h emulation, see vmouse.h)                             */
/* ------------------------------------------------------------------ */

unsigned char MouseInit(void)
{
    ensureInit();
    return g_backend->hasMouse() ? 1 : 0;
}

unsigned char MouseIsDriverInstalled(void)
{
    return MouseInit();
}

void MouseShowCursor(void) {}
void MouseHideCursor(void) {}
void MousePutCursor(unsigned short x, unsigned short y) { (void)x; (void)y; }
void MouseSetCursorXlimit(short xMax, short xMin) { (void)xMax; (void)xMin; }
void MouseSetCursorYlimit(short yMax, short yMin) { (void)yMax; (void)yMin; }
void MouseDefineGCursor(short x, short y, void *bitmap) { (void)x; (void)y; (void)bitmap; }
void MouseLPE(unsigned char on) { (void)on; }

void MouseGetVersion(short *version, short *irq, short *type)
{
    if (version) *version = 0x0820;
    if (irq) *irq = 0;
    if (type) *type = 0;
}

short MouseGetStatus(unsigned short *x, unsigned short *y)
{
    ensureInit();
    pollKeys(0);
    if (x) *x = (unsigned short)g_input.mouseX;
    if (y) *y = (unsigned short)g_input.mouseY;
    short mask = 0;
    if (g_input.buttons[MOUSE_LEFT].down) mask |= 1;
    if (g_input.buttons[MOUSE_RIGHT].down) mask |= 2;
    if (g_input.buttons[MOUSE_MIDDLE].down) mask |= 4;
    return mask;
}

unsigned char MouseIsButtonDown(unsigned short button, unsigned short *x, unsigned short *y)
{
    ensureInit();
    if (button > 2)
        return 0;
    pollKeys(0);
    const MouseButton &b = g_input.buttons[button];
    if (x) *x = (unsigned short)b.pressX;
    if (y) *y = (unsigned short)b.pressY;
    return b.down ? 1 : 0;
}

unsigned char MouseIsButtonUp(unsigned short button, unsigned short *x, unsigned short *y)
{
    ensureInit();
    if (button > 2)
        return 1;
    /* The caller spins on this until the button is released. */
    pollKeys(g_input.buttons[button].down ? 10 : 0);
    const MouseButton &b = g_input.buttons[button];
    if (x) *x = (unsigned short)b.releaseX;
    if (y) *y = (unsigned short)b.releaseY;
    return b.down ? 0 : 1;
}

unsigned char MouseGetClick(unsigned short button, unsigned short *x, unsigned short *y)
{
    if (MouseIsButtonDown(button, x, y)) {
        while (!MouseIsButtonUp(button, x, y))
            ;
        return 1;
    }
    return 0;
}

} /* extern "C" */
