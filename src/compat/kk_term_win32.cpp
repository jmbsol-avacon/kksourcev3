/*
 * kk_term_win32.cpp - terminal back end for the Windows console
 * (conhost and Windows Terminal).
 *
 * The program gets its own 80x25 screen buffer; cells are written with
 * WriteConsoleOutputW using the CP437 glyph table, and input comes from
 * ReadConsoleInputW, whose scan codes map directly onto the BIOS codes.
 */
#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "kk_backend.h"
#include "kk_internal.h"

namespace kk {
namespace {

BOOL WINAPI ctrlHandler(DWORD type)
{
    /* Ctrl+C arrives as a key (processed input is off); swallow Ctrl+Break. */
    return (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT) ? TRUE : FALSE;
}

class Win32Backend : public Backend {
public:
    bool init(std::string &error) override
    {
        hIn_ = GetStdHandle(STD_INPUT_HANDLE);
        hOrigOut_ = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hIn_ == INVALID_HANDLE_VALUE || hOrigOut_ == INVALID_HANDLE_VALUE) {
            error = "no console attached";
            return false;
        }
        if (!GetConsoleMode(hIn_, &origInMode_)) {
            error = "standard input is not a console";
            return false;
        }
        SetConsoleCtrlHandler(ctrlHandler, TRUE);
        enter();
        return true;
    }

    void shutdown() override { leave(); }
    void suspend() override { leave(); }
    void resume() override { enter(); }

    void render(const Cell *cells, int cursorX, int cursorY, CursorShape shape, bool full) override
    {
        (void)full;
        for (int i = 0; i < ROWS * COLS; i++) {
            buf_[i].Char.UnicodeChar = kk_cp437_unicode[cells[i].ch];
            buf_[i].Attributes = cells[i].attr;   /* DOS attribute bits == console attribute bits */
        }
        COORD size = { (SHORT)COLS, (SHORT)ROWS };
        COORD origin = { 0, 0 };
        SMALL_RECT rect = { 0, 0, (SHORT)(COLS - 1), (SHORT)(ROWS - 1) };
        WriteConsoleOutputW(hOut_, buf_, size, origin, &rect);

        COORD pos = { (SHORT)cursorX, (SHORT)cursorY };
        SetConsoleCursorPosition(hOut_, pos);
        CONSOLE_CURSOR_INFO ci;
        ci.bVisible = (shape != CURSOR_HIDDEN);
        ci.dwSize = (shape == CURSOR_BLOCK) ? 100 : 25;
        SetConsoleCursorInfo(hOut_, &ci);
    }

    void pump(int timeoutMs) override
    {
        if (WaitForSingleObject(hIn_, (DWORD)timeoutMs) != WAIT_OBJECT_0)
            return;
        DWORD count = 0;
        while (GetNumberOfConsoleInputEvents(hIn_, &count) && count > 0) {
            INPUT_RECORD rec;
            DWORD got = 0;
            if (!ReadConsoleInputW(hIn_, &rec, 1, &got) || got == 0)
                break;
            switch (rec.EventType) {
            case KEY_EVENT:
                handleKey(rec.Event.KeyEvent);
                break;
            case MOUSE_EVENT:
                handleMouse(rec.Event.MouseEvent);
                break;
            default:
                break;
            }
        }
    }

    void beep(unsigned hz, unsigned ms) override
    {
        if (hz < 37) hz = 37;
        if (hz > 32767) hz = 32767;
        Beep(hz, ms ? ms : 50);
    }

    bool hasMouse() const override { return true; }

    unsigned lockState() override
    {
        unsigned s = 0;
        if (GetKeyState(VK_INSERT) & 1)  s |= 0x80;
        if (GetKeyState(VK_CAPITAL) & 1) s |= 0x40;
        if (GetKeyState(VK_NUMLOCK) & 1) s |= 0x20;
        if (GetKeyState(VK_SCROLL) & 1)  s |= 0x10;
        return s;
    }

private:
    HANDLE    hIn_ = INVALID_HANDLE_VALUE;
    HANDLE    hOrigOut_ = INVALID_HANDLE_VALUE;
    HANDLE    hOut_ = INVALID_HANDLE_VALUE;
    DWORD     origInMode_ = 0;
    DWORD     lastButtons_ = 0;
    bool      active_ = false;
    CHAR_INFO buf_[ROWS * COLS];

    void enter()
    {
        if (active_)
            return;
        SetConsoleMode(hIn_, ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT);
        hOut_ = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, nullptr, CONSOLE_TEXTMODE_BUFFER, nullptr);
        if (hOut_ == INVALID_HANDLE_VALUE)
            hOut_ = hOrigOut_;
        COORD size = { (SHORT)COLS, (SHORT)ROWS };
        SMALL_RECT win = { 0, 0, (SHORT)(COLS - 1), (SHORT)(ROWS - 1) };
        /* Shrink the window first so that the buffer may shrink, then grow the window back. */
        SetConsoleWindowInfo(hOut_, TRUE, &win);
        SetConsoleScreenBufferSize(hOut_, size);
        SetConsoleWindowInfo(hOut_, TRUE, &win);
        SetConsoleActiveScreenBuffer(hOut_);
        SetConsoleTitleA("Kwik Kuote");
        FlushConsoleInputBuffer(hIn_);
        active_ = true;
    }

    void leave()
    {
        if (!active_)
            return;
        SetConsoleActiveScreenBuffer(hOrigOut_);
        if (hOut_ != hOrigOut_ && hOut_ != INVALID_HANDLE_VALUE)
            CloseHandle(hOut_);
        hOut_ = INVALID_HANDLE_VALUE;
        SetConsoleMode(hIn_, origInMode_);
        active_ = false;
    }

    void handleKey(const KEY_EVENT_RECORD &k)
    {
        InputState &in = input();
        DWORD cs = k.dwControlKeyState;
        unsigned mods = 0;
        if (cs & SHIFT_PRESSED) mods |= 0x03;
        if (cs & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) mods |= 0x04;
        if (cs & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) mods |= 0x08;
        in.modifiers = mods;
        if (!k.bKeyDown)
            return;

        bool shift = (cs & SHIFT_PRESSED) != 0;
        bool ctrl = (cs & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
        bool alt = (cs & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0;
        WORD vk = k.wVirtualKeyCode;
        WORD scan = k.wVirtualScanCode;
        WCHAR ch = k.uChar.UnicodeChar;
        uint16_t code = 0;

        switch (vk) {
        case VK_SHIFT: case VK_CONTROL: case VK_MENU: case VK_CAPITAL: case VK_NUMLOCK:
        case VK_SCROLL: case VK_LWIN: case VK_RWIN: case VK_APPS:
            return;
        case VK_F1: case VK_F2: case VK_F3: case VK_F4: case VK_F5: case VK_F6:
        case VK_F7: case VK_F8: case VK_F9: case VK_F10: case VK_F11: case VK_F12:
            code = keyFunction(vk - VK_F1 + 1, shift, ctrl, alt);
            break;
        case VK_UP:    code = keyExtended(EK_UP, shift, ctrl, alt); break;
        case VK_DOWN:  code = keyExtended(EK_DOWN, shift, ctrl, alt); break;
        case VK_LEFT:  code = keyExtended(EK_LEFT, shift, ctrl, alt); break;
        case VK_RIGHT: code = keyExtended(EK_RIGHT, shift, ctrl, alt); break;
        case VK_HOME:  code = keyExtended(EK_HOME, shift, ctrl, alt); break;
        case VK_END:   code = keyExtended(EK_END, shift, ctrl, alt); break;
        case VK_PRIOR: code = keyExtended(EK_PGUP, shift, ctrl, alt); break;
        case VK_NEXT:  code = keyExtended(EK_PGDN, shift, ctrl, alt); break;
        case VK_INSERT: code = keyExtended(EK_INS, shift, ctrl, alt); break;
        case VK_DELETE: code = keyExtended(EK_DEL, shift, ctrl, alt); break;
        case VK_TAB:    code = shift ? KEY_CODE_SHIFT_TAB : KEY_CODE_TAB; break;
        case VK_RETURN: code = ctrl ? KEY_CODE_CTRL_ENTER : KEY_CODE_ENTER; break;
        case VK_BACK:   code = ctrl ? KEY_CODE_CTRL_BACKSPACE : KEY_CODE_BACKSPACE; break;
        case VK_ESCAPE: code = KEY_CODE_ESC; break;
        default:
            if (ch != 0) {
                unsigned char c = kk_unicode_to_cp437((unsigned)ch);
                if (alt && !ctrl && ch >= 0x20)
                    code = keyAltChar(c);
                if (code == 0)
                    code = (uint16_t)(((scan & 0xff) << 8) | c);
            } else if (alt) {
                if (vk >= 'A' && vk <= 'Z')
                    code = keyAltChar((unsigned char)(vk - 'A' + 'a'));
                else if (vk >= '0' && vk <= '9')
                    code = keyAltChar((unsigned char)vk);
            }
            break;
        }
        if (code == 0)
            return;
        /* Keep the emulated Insert flag in step with the real key state. */
        if (code == KEY_CODE_INS) {
            in.insertOn = !(GetKeyState(VK_INSERT) & 1);
        }
        for (WORD i = 0; i < (k.wRepeatCount ? k.wRepeatCount : 1); i++)
            in.keys.push_back(code);
    }

    void handleMouse(const MOUSE_EVENT_RECORD &m)
    {
        InputState &in = input();
        int col = m.dwMousePosition.X;
        int row = m.dwMousePosition.Y;
        if (m.dwEventFlags & MOUSE_WHEELED)
            return;
        DWORD buttons = m.dwButtonState & 7;
        in.mouseMove(col, row);
        static const struct { DWORD bit; int idx; } map[3] = {
            { FROM_LEFT_1ST_BUTTON_PRESSED, MOUSE_LEFT },
            { RIGHTMOST_BUTTON_PRESSED, MOUSE_RIGHT },
            { FROM_LEFT_2ND_BUTTON_PRESSED, MOUSE_MIDDLE },
        };
        for (const auto &b : map) {
            bool now = (buttons & b.bit) != 0;
            bool was = (lastButtons_ & b.bit) != 0;
            if (now && !was)
                in.mousePress(b.idx, col, row);
            else if (!now && was)
                in.mouseRelease(b.idx, col, row);
        }
        lastButtons_ = buttons;
    }
};

} // namespace

Backend *createBackend() { return new Win32Backend(); }

} // namespace kk

#endif /* _WIN32 */
