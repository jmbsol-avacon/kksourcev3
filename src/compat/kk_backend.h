/*
 * kk_backend.h - interface between the virtual console (kk_console.cpp) and
 * the terminal back ends (kk_term_posix.cpp, kk_term_win32.cpp).
 */
#ifndef KK_BACKEND_H
#define KK_BACKEND_H

#include <cstdint>
#include <deque>
#include <string>

namespace kk {

constexpr int COLS = 80;
constexpr int ROWS = 25;

/* One character cell, exactly as in the DOS text mode video buffer. */
struct Cell {
    uint8_t ch;
    uint8_t attr;
};

enum CursorShape { CURSOR_HIDDEN, CURSOR_NORMAL, CURSOR_BLOCK };

/* Mouse button state in INT 33h terms (positions in "pixels": cell * 8). */
struct MouseButton {
    bool down = false;
    int  pressX = 0, pressY = 0;
    int  releaseX = 0, releaseY = 0;
};

enum MouseIndex { MOUSE_LEFT = 0, MOUSE_RIGHT = 1, MOUSE_MIDDLE = 2 };

struct InputState {
    std::deque<uint16_t> keys;      /* BIOS key codes: scan << 8 | ascii   */
    int         mouseX = 0;
    int         mouseY = 0;
    MouseButton buttons[3];
    bool        insertOn = false;   /* emulated BIOS Insert toggle         */
    unsigned    modifiers = 0;      /* current shift/ctrl/alt, bioskey(2)  */

    void pushKey(uint16_t code);
    void mouseMove(int col, int row);
    void mousePress(int button, int col, int row);
    void mouseRelease(int button, int col, int row);
};

InputState &input();

class Backend {
public:
    virtual ~Backend() {}
    virtual bool init(std::string &error) = 0;
    virtual void shutdown() = 0;
    /* Leave/re-enter full screen mode around a child process. */
    virtual void suspend() = 0;
    virtual void resume() = 0;
    /* Draw the screen; cursor coordinates are absolute and 0-based. */
    virtual void render(const Cell *cells, int cursorX, int cursorY, CursorShape shape, bool full) = 0;
    /* Read pending input, waiting at most timeoutMs for something to arrive. */
    virtual void pump(int timeoutMs) = 0;
    virtual void beep(unsigned hz, unsigned ms) = 0;
    virtual bool hasMouse() const = 0;
    /* Caps/Num/Scroll/Insert bits for bioskey(2); 0 if unknown. */
    virtual unsigned lockState() = 0;
};

Backend *createBackend();

/* Key code helpers (kk_keys.cpp). */
uint16_t keyFromChar(unsigned char cp437);          /* typed character       */
uint16_t keyAltChar(unsigned char c);               /* Alt + letter/digit    */
uint16_t keyFunction(int n, bool shift, bool ctrl, bool alt);  /* F1..F12 */

enum ExtKey { EK_UP, EK_DOWN, EK_LEFT, EK_RIGHT, EK_HOME, EK_END, EK_PGUP, EK_PGDN, EK_INS, EK_DEL };
uint16_t keyExtended(ExtKey k, bool shift, bool ctrl, bool alt);

constexpr uint16_t KEY_CODE_ESC = 0x011b;
constexpr uint16_t KEY_CODE_ENTER = 0x1c0d;
constexpr uint16_t KEY_CODE_CTRL_ENTER = 0x1c0a;
constexpr uint16_t KEY_CODE_TAB = 0x0f09;
constexpr uint16_t KEY_CODE_SHIFT_TAB = 0x0f00;
constexpr uint16_t KEY_CODE_BACKSPACE = 0x0e08;
constexpr uint16_t KEY_CODE_CTRL_BACKSPACE = 0x0e7f;
constexpr uint16_t KEY_CODE_INS = 0x5200;

} // namespace kk

#endif /* KK_BACKEND_H */
