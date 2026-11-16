/*
 * bios.h - Borland <bios.h> replacements.
 *
 * bioskey() is the heart of the keyboard interface: the application works
 * with INT 16h style codes (scan code << 8 | ASCII), see stdinc.h.
 */
#ifndef KK_BIOS_H
#define KK_BIOS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * cmd 0: wait for a key and return its code.
 * cmd 1: return the next key code without removing it, 0 if none.
 * cmd 2: shift status flags (bit 7 = Insert, 6 = Caps Lock, 5 = Num Lock,
 *        4 = Scroll Lock, 3 = Alt, 2 = Ctrl, 1 = Left Shift, 0 = Right Shift).
 */
int bioskey(int cmd);

/*
 * cmd 0: print a byte, cmd 1: initialise, cmd 2: return status.
 * Printing is spooled through the platform layer; the status always
 * reports "selected, not busy, no error" (0x90).
 */
int biosprint(int cmd, int abyte, int port);

#ifdef __cplusplus
}
#endif

#endif /* KK_BIOS_H */
