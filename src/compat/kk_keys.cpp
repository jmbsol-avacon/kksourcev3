/*
 * kk_keys.cpp - translation of keyboard input into BIOS INT 16h key codes.
 *
 * The application (see stdinc.h) distinguishes keys by the full 16-bit code
 * the BIOS returned on a PC: the keyboard scan code of the US layout in the
 * high byte and the ASCII value in the low byte (0 for "extended" keys).
 */
#include "kk_backend.h"

namespace kk {

namespace {

/* Scan codes of the letter keys a..z on the US layout. */
const uint8_t letterScan[26] = {
    0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
    0x31, 0x18, 0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c
};

uint8_t scanForChar(unsigned char c)
{
    if (c >= 'a' && c <= 'z') return letterScan[c - 'a'];
    if (c >= 'A' && c <= 'Z') return letterScan[c - 'A'];
    if (c >= '1' && c <= '9') return (uint8_t)(0x02 + (c - '1'));
    switch (c) {
    case '0': case ')': return 0x0b;
    case '!': return 0x02; case '@': return 0x03; case '#': return 0x04;
    case '$': return 0x05; case '%': return 0x06; case '^': return 0x07;
    case '&': return 0x08; case '*': return 0x09; case '(': return 0x0a;
    case '-': case '_': return 0x0c;
    case '=': case '+': return 0x0d;
    case '[': case '{': return 0x1a;
    case ']': case '}': return 0x1b;
    case ';': case ':': return 0x27;
    case '\'': case '"': return 0x28;
    case '`': case '~': return 0x29;
    case '\\': case '|': return 0x2b;
    case ',': case '<': return 0x33;
    case '.': case '>': return 0x34;
    case '/': case '?': return 0x35;
    case ' ': return 0x39;
    default: return 0;
    }
}

} // namespace

uint16_t keyFromChar(unsigned char c)
{
    switch (c) {
    case 0x0d: return KEY_CODE_ENTER;
    case 0x0a: return 0x240a;             /* Ctrl-J (Ctrl-Enter is not distinguishable on terminals) */
    case 0x09: return KEY_CODE_TAB;
    case 0x08: return KEY_CODE_BACKSPACE;
    case 0x7f: return KEY_CODE_BACKSPACE; /* most terminals send DEL for the Backspace key */
    case 0x1b: return KEY_CODE_ESC;
    case 0x1c: return 0x2b1c;             /* Ctrl-\ */
    case 0x1d: return 0x1b1d;             /* Ctrl-] */
    case 0x1e: return 0x071e;             /* Ctrl-^ */
    case 0x1f: return 0x0c1f;             /* Ctrl-_ */
    default:
        break;
    }
    if (c >= 0x01 && c <= 0x1a)           /* Ctrl-A .. Ctrl-Z */
        return (uint16_t)((letterScan[c - 1] << 8) | c);
    return (uint16_t)((scanForChar(c) << 8) | c);
}

uint16_t keyAltChar(unsigned char c)
{
    if (c >= 'a' && c <= 'z') return (uint16_t)(letterScan[c - 'a'] << 8);
    if (c >= 'A' && c <= 'Z') return (uint16_t)(letterScan[c - 'A'] << 8);
    if (c >= '1' && c <= '9') return (uint16_t)((0x78 + (c - '1')) << 8);
    if (c == '0') return 0x8100;
    if (c == '-') return 0x8200;
    if (c == '=') return 0x8300;
    return 0;
}

uint16_t keyFunction(int n, bool shift, bool ctrl, bool alt)
{
    if (n < 1 || n > 12)
        return 0;
    if (n <= 10) {
        int base = 0x3b + (n - 1);
        if (alt)        base += 0x2d;
        else if (ctrl)  base += 0x23;
        else if (shift) base += 0x19;
        return (uint16_t)(base << 8);
    }
    int base = 0x85 + (n - 11);
    if (alt)        base += 6;
    else if (ctrl)  base += 4;
    else if (shift) base += 2;
    return (uint16_t)(base << 8);
}

uint16_t keyExtended(ExtKey k, bool shift, bool ctrl, bool alt)
{
    static const uint8_t plain[10] = { 0x48, 0x50, 0x4b, 0x4d, 0x47, 0x4f, 0x49, 0x51, 0x52, 0x53 };
    static const uint8_t ctrls[10] = { 0x8d, 0x91, 0x73, 0x74, 0x77, 0x75, 0x84, 0x76, 0x92, 0x93 };
    static const uint8_t alts[10]  = { 0x98, 0xa0, 0x9b, 0x9d, 0x97, 0x9f, 0x99, 0xa1, 0xa2, 0xa3 };
    (void)shift;
    if (alt)  return (uint16_t)(alts[k] << 8);
    if (ctrl) return (uint16_t)(ctrls[k] << 8);
    return (uint16_t)(plain[k] << 8);
}

} // namespace kk
