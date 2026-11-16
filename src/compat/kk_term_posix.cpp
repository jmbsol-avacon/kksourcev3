/*
 * kk_term_posix.cpp - terminal back end for Linux, macOS and other POSIX
 * systems: raw-mode termios, ANSI/VT escape sequences, SGR mouse reporting,
 * CP437 to UTF-8 translation.
 */
#ifndef _WIN32

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "kk_backend.h"
#include "kk_internal.h"
#include "include/kk_platform.h"

namespace kk {
namespace {

volatile sig_atomic_t g_winch = 0;
struct termios g_origTermios;
bool g_haveOrig = false;

void onWinch(int) { g_winch = 1; }

void onFatalSignal(int sig)
{
    static const char restore[] = "\x1b[0m\x1b[?1000l\x1b[?1006l\x1b[?25h\x1b[0 q\x1b[?1049l";
    if (g_haveOrig)
        tcsetattr(STDIN_FILENO, TCSANOW, &g_origTermios);
    ssize_t r = write(STDOUT_FILENO, restore, sizeof restore - 1);
    (void)r;
    signal(sig, SIG_DFL);
    raise(sig);
}

void writeAll(const std::string &s)
{
    const char *p = s.data();
    size_t left = s.size();
    while (left > 0) {
        ssize_t n = write(STDOUT_FILENO, p, left);
        if (n < 0) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            return;
        }
        p += n;
        left -= (size_t)n;
    }
}

void appendUtf8(std::string &out, unsigned cp)
{
    if (cp < 0x80) {
        out += (char)cp;
    } else if (cp < 0x800) {
        out += (char)(0xc0 | (cp >> 6));
        out += (char)(0x80 | (cp & 0x3f));
    } else {
        out += (char)(0xe0 | (cp >> 12));
        out += (char)(0x80 | ((cp >> 6) & 0x3f));
        out += (char)(0x80 | (cp & 0x3f));
    }
}

void sleepMs(unsigned ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, nullptr);
}

class PosixBackend : public Backend {
public:
    bool init(std::string &error) override
    {
        if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
            error = "standard input and output must be a terminal";
            return false;
        }
        if (tcgetattr(STDIN_FILENO, &g_origTermios) != 0) {
            error = strerror(errno);
            return false;
        }
        g_haveOrig = true;
        signal(SIGWINCH, onWinch);
        signal(SIGTERM, onFatalSignal);
        signal(SIGHUP, onFatalSignal);
        signal(SIGQUIT, onFatalSignal);
        enterRaw();
        return true;
    }

    void shutdown() override { leaveRaw(); }
    void suspend() override { leaveRaw(); }
    void resume() override { enterRaw(); }

    void render(const Cell *cells, int cursorX, int cursorY, CursorShape shape, bool full) override
    {
        if (g_winch) {
            g_winch = 0;
            full = true;
        }
        if (!havePrev_)
            full = true;
        std::string out;
        out.reserve(8192);
        if (full)
            out += "\x1b[0m\x1b[H\x1b[2J";
        int outRow = -1, outCol = -1, outAttr = -1;
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                const Cell &cell = cells[r * COLS + c];
                if (!full && prev_[r * COLS + c].ch == cell.ch && prev_[r * COLS + c].attr == cell.attr)
                    continue;
                if (outRow != r || outCol != c) {
                    char buf[32];
                    std::snprintf(buf, sizeof buf, "\x1b[%d;%dH", r + 1, c + 1);
                    out += buf;
                    outRow = r;
                    outCol = c;
                }
                if (outAttr != cell.attr) {
                    appendSgr(out, cell.attr);
                    outAttr = cell.attr;
                }
                appendUtf8(out, kk_cp437_unicode[cell.ch]);
                outCol++;
                if (outCol >= COLS)
                    outRow = outCol = -1;   /* terminals differ about wrapping: reposition */
            }
        }
        std::memcpy(prev_, cells, sizeof prev_);
        havePrev_ = true;

        char buf[48];
        std::snprintf(buf, sizeof buf, "\x1b[%d;%dH", cursorY + 1, cursorX + 1);
        out += buf;
        if (shape == CURSOR_HIDDEN) {
            out += "\x1b[?25l";
        } else {
            out += (shape == CURSOR_BLOCK) ? "\x1b[2 q" : "\x1b[4 q";
            out += "\x1b[?25h";
        }
        writeAll(out);
    }

    void pump(int timeoutMs) override
    {
        struct pollfd pfd;
        pfd.fd = STDIN_FILENO;
        pfd.events = POLLIN;
        int r = poll(&pfd, 1, timeoutMs);
        if (r > 0 && (pfd.revents & (POLLIN | POLLHUP | POLLERR))) {
            char buf[512];
            ssize_t n = read(STDIN_FILENO, buf, sizeof buf);
            if (n > 0)
                pending_.append(buf, (size_t)n);
            else if (n == 0 || (n < 0 && errno != EINTR && errno != EAGAIN))
                hangup_ = true;
        }
        if (hangup_) {
            /* The terminal went away: let the program finish cleanly. */
            KK_PlatformShutdown_();
            std::exit(1);
        }
        parsePending();
    }

    void beep(unsigned hz, unsigned ms) override
    {
        (void)hz;
        writeAll("\a");
        if (ms)
            sleepMs(ms);
    }

    bool hasMouse() const override { return true; }
    unsigned lockState() override { return 0; }

private:
    Cell        prev_[ROWS * COLS];
    bool        havePrev_ = false;
    bool        raw_ = false;
    bool        hangup_ = false;
    std::string pending_;

    static void KK_PlatformShutdown_();

    void enterRaw()
    {
        if (raw_)
            return;
        struct termios t = g_origTermios;
        cfmakeraw(&t);
        t.c_cc[VMIN] = 0;
        t.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
        writeAll("\x1b[?1049h\x1b[?25l\x1b[?1000h\x1b[?1006h\x1b[0m\x1b[H\x1b[2J\x1b]0;Kwik Kuote\x07");
        raw_ = true;
        havePrev_ = false;
    }

    void leaveRaw()
    {
        if (!raw_)
            return;
        writeAll("\x1b[0m\x1b[?1000l\x1b[?1006l\x1b[?25h\x1b[0 q\x1b[?1049l");
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_origTermios);
        raw_ = false;
    }

    static void appendSgr(std::string &out, uint8_t attr)
    {
        int fg = attr & 0x0f;
        int bg = (attr >> 4) & 0x07;
        bool brightBg = (attr & 0x80) != 0;
        char buf[32];
        std::snprintf(buf, sizeof buf, "\x1b[0;%d;%dm",
                      (fg & 8) ? 90 + (fg & 7) : 30 + (fg & 7),
                      brightBg ? 100 + bg : 40 + bg);
        out += buf;
    }

    /* Wait briefly for the rest of an escape sequence. */
    bool waitMore(int ms)
    {
        struct pollfd pfd;
        pfd.fd = STDIN_FILENO;
        pfd.events = POLLIN;
        if (poll(&pfd, 1, ms) > 0 && (pfd.revents & POLLIN)) {
            char buf[256];
            ssize_t n = read(STDIN_FILENO, buf, sizeof buf);
            if (n > 0) {
                pending_.append(buf, (size_t)n);
                return true;
            }
        }
        return false;
    }

    void parsePending()
    {
        InputState &in = input();
        while (!pending_.empty()) {
            unsigned char b = (unsigned char)pending_[0];
            if (b == 0x1b) {
                size_t used = parseEscape(in);
                if (used == 0)
                    return;             /* incomplete: wait for more bytes */
                pending_.erase(0, used);
            } else if (b >= 0x80) {
                size_t len = (b >= 0xf0) ? 4 : (b >= 0xe0) ? 3 : (b >= 0xc0) ? 2 : 1;
                if (pending_.size() < len) {
                    if (!waitMore(30)) {
                        pending_.erase(0, 1);
                        continue;
                    }
                    continue;
                }
                unsigned cp = 0;
                if (len == 1) {
                    cp = '?';
                } else {
                    cp = b & (0x3f >> (len - 1));
                    for (size_t i = 1; i < len; i++)
                        cp = (cp << 6) | ((unsigned char)pending_[i] & 0x3f);
                }
                in.pushKey(keyFromChar(kk_unicode_to_cp437(cp)));
                pending_.erase(0, len);
            } else {
                pending_.erase(0, 1);
                if (b != 0)
                    in.pushKey(keyFromChar(b));
            }
        }
    }

    /* Returns the number of bytes consumed, 0 if more input is needed. */
    size_t parseEscape(InputState &in)
    {
        if (pending_.size() == 1) {
            if (!waitMore(40)) {
                in.pushKey(KEY_CODE_ESC);
                return 1;
            }
            if (pending_.size() == 1) {
                in.pushKey(KEY_CODE_ESC);
                return 1;
            }
        }
        bool alt = false;
        size_t pos = 1;
        if (pending_[pos] == 0x1b) {
            /* ESC ESC <seq>: Alt + sequence (some terminals), or two escapes. */
            if (pending_.size() > 2 && (pending_[2] == '[' || pending_[2] == 'O')) {
                alt = true;
                pos = 2;
            } else {
                in.pushKey(KEY_CODE_ESC);
                return 1;
            }
        }
        unsigned char kind = (unsigned char)pending_[pos];
        if (kind == '[') {
            /* CSI: parameters then a final byte in 0x40..0x7e */
            size_t i = pos + 1;
            bool sgrMouse = false;
            if (i < pending_.size() && pending_[i] == '<') {
                sgrMouse = true;
                i++;
            }
            int params[8] = {0};
            bool has[8] = {false};
            int np = 0;
            while (i < pending_.size()) {
                unsigned char c = (unsigned char)pending_[i];
                if (c >= '0' && c <= '9') {
                    if (np < 8) {
                        params[np] = params[np] * 10 + (c - '0');
                        has[np] = true;
                    }
                    i++;
                } else if (c == ';') {
                    if (np < 7) np++;
                    i++;
                } else if (c >= 0x40 && c <= 0x7e) {
                    break;
                } else {
                    i++;                /* unknown intermediate */
                }
            }
            if (i >= pending_.size()) {
                if (waitMore(40))
                    return parseEscape(in);
                return pending_.size();  /* garbage, drop it */
            }
            unsigned char fin = (unsigned char)pending_[i];
            int count = np + 1;
            if (sgrMouse) {
                handleMouse(in, params[0], has[1] ? params[1] : 1, has[2] ? params[2] : 1, fin == 'M');
                return i + 1;
            }
            int mod = (count >= 2 && has[1]) ? params[1] : 1;
            bool shift = ((mod - 1) & 1) != 0;
            bool malt = ((mod - 1) & 2) != 0 || alt;
            bool ctrl = ((mod - 1) & 4) != 0;
            uint16_t code = 0;
            switch (fin) {
            case 'A': code = keyExtended(EK_UP, shift, ctrl, malt); break;
            case 'B': code = keyExtended(EK_DOWN, shift, ctrl, malt); break;
            case 'C': code = keyExtended(EK_RIGHT, shift, ctrl, malt); break;
            case 'D': code = keyExtended(EK_LEFT, shift, ctrl, malt); break;
            case 'H': code = keyExtended(EK_HOME, shift, ctrl, malt); break;
            case 'F': code = keyExtended(EK_END, shift, ctrl, malt); break;
            case 'P': case 'Q': case 'R': case 'S':
                code = keyFunction(fin - 'P' + 1, shift, ctrl, malt);
                break;
            case 'Z': code = KEY_CODE_SHIFT_TAB; break;
            case '~': {
                int n = has[0] ? params[0] : 0;
                switch (n) {
                case 1: case 7: code = keyExtended(EK_HOME, shift, ctrl, malt); break;
                case 2: code = keyExtended(EK_INS, shift, ctrl, malt); break;
                case 3: code = keyExtended(EK_DEL, shift, ctrl, malt); break;
                case 4: case 8: code = keyExtended(EK_END, shift, ctrl, malt); break;
                case 5: code = keyExtended(EK_PGUP, shift, ctrl, malt); break;
                case 6: code = keyExtended(EK_PGDN, shift, ctrl, malt); break;
                case 11: case 12: case 13: case 14: case 15:
                    code = keyFunction(n - 10, shift, ctrl, malt); break;
                case 17: case 18: case 19: case 20: case 21:
                    code = keyFunction(n - 11, shift, ctrl, malt); break;
                case 23: case 24:
                    code = keyFunction(n - 12, shift, ctrl, malt); break;
                default: break;
                }
                break;
            }
            default:
                break;
            }
            if (code)
                in.pushKey(code);
            return i + 1;
        }
        if (kind == 'O') {
            if (pos + 1 >= pending_.size()) {
                if (waitMore(40))
                    return parseEscape(in);
                return pending_.size();
            }
            unsigned char fin = (unsigned char)pending_[pos + 1];
            uint16_t code = 0;
            switch (fin) {
            case 'P': case 'Q': case 'R': case 'S': code = keyFunction(fin - 'P' + 1, false, false, alt); break;
            case 'A': code = keyExtended(EK_UP, false, false, alt); break;
            case 'B': code = keyExtended(EK_DOWN, false, false, alt); break;
            case 'C': code = keyExtended(EK_RIGHT, false, false, alt); break;
            case 'D': code = keyExtended(EK_LEFT, false, false, alt); break;
            case 'H': code = keyExtended(EK_HOME, false, false, alt); break;
            case 'F': code = keyExtended(EK_END, false, false, alt); break;
            case 'M': code = KEY_CODE_ENTER; break;
            case 'j': code = keyFromChar('*'); break;
            case 'k': code = keyFromChar('+'); break;
            case 'l': code = keyFromChar(','); break;
            case 'm': code = keyFromChar('-'); break;
            case 'n': code = keyFromChar('.'); break;
            case 'o': code = keyFromChar('/'); break;
            default:
                if (fin >= 'p' && fin <= 'y')
                    code = keyFromChar((unsigned char)('0' + (fin - 'p')));
                break;
            }
            if (code)
                in.pushKey(code);
            return pos + 2;
        }
        /* ESC + character: Alt + key */
        if (kind == 0x7f || kind == 0x08) {
            in.pushKey(KEY_CODE_CTRL_BACKSPACE);
            return pos + 1;
        }
        uint16_t code = keyAltChar(kind);
        if (code == 0 && kind >= 0x20)
            code = keyFromChar(kind);   /* unknown Alt combination: plain key */
        if (code)
            in.pushKey(code);
        return pos + 1;
    }

    void handleMouse(InputState &in, int b, int x, int y, bool press)
    {
        int col = x - 1, row = y - 1;
        if (col < 0) col = 0;
        if (row < 0) row = 0;
        if (b & 64)                     /* wheel */
            return;
        int button = b & 3;
        int idx = (button == 0) ? MOUSE_LEFT : (button == 2) ? MOUSE_RIGHT : (button == 1) ? MOUSE_MIDDLE : -1;
        if (b & 32) {                   /* motion */
            in.mouseMove(col, row);
            return;
        }
        if (idx < 0) {
            in.mouseMove(col, row);
            return;
        }
        if (press)
            in.mousePress(idx, col, row);
        else
            in.mouseRelease(idx, col, row);
    }
};

void PosixBackend::KK_PlatformShutdown_() { KK_PlatformShutdown(); }

} // namespace

Backend *createBackend() { return new PosixBackend(); }

} // namespace kk

#endif /* !_WIN32 */
