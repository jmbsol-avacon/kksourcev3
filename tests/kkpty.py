#!/usr/bin/env python3
"""
kkpty.py - drive the Kwik Kuote console application in a pseudo-terminal.

A tiny VT100 screen model (just what the POSIX back end emits) lets a test
look at the 80x25 screen as text, send keystrokes and wait for prompts.

    from kkpty import KK
    with KK(["./kk", "iran", "nari"], cwd="/path/to/data") as kk:
        kk.expect("MAIN MENU")
        kk.send("9")          # EXIT THE SYSTEM
        kk.wait_exit()
"""
import fcntl
import os
import pty
import select
import signal
import struct
import subprocess
import termios
import time

COLS, ROWS = 80, 25

KEYS = {
    "ENTER": "\r", "ESC": "\x1b", "TAB": "\t", "BS": "\x7f",
    "UP": "\x1b[A", "DOWN": "\x1b[B", "RIGHT": "\x1b[C", "LEFT": "\x1b[D",
    "HOME": "\x1b[H", "END": "\x1b[F", "PGUP": "\x1b[5~", "PGDN": "\x1b[6~",
    "INS": "\x1b[2~", "DEL": "\x1b[3~",
    "F1": "\x1bOP", "F2": "\x1bOQ", "F3": "\x1bOR", "F4": "\x1bOS",
    "F5": "\x1b[15~", "F6": "\x1b[17~", "F7": "\x1b[18~", "F8": "\x1b[19~",
    "F9": "\x1b[20~", "F10": "\x1b[21~", "F11": "\x1b[23~", "F12": "\x1b[24~",
    "STAB": "\x1b[Z",
}


class Screen:
    def __init__(self):
        self.cells = [[" "] * COLS for _ in range(ROWS)]
        self.row = self.col = 0
        self.buf = b""

    def feed(self, data: bytes):
        self.buf += data
        i = 0
        b = self.buf
        while i < len(b):
            c = b[i]
            if c == 0x1b:
                end = self._escape(b, i)
                if end is None:          # incomplete sequence: keep for later
                    break
                i = end
            elif c == 0x0d:
                self.col = 0
                i += 1
            elif c == 0x0a:
                self.row = min(ROWS - 1, self.row + 1)
                i += 1
            elif c == 0x07:
                i += 1
            elif c < 0x20:
                i += 1
            else:
                n = 4 if c >= 0xf0 else 3 if c >= 0xe0 else 2 if c >= 0xc0 else 1
                if i + n > len(b):
                    break
                try:
                    ch = b[i:i + n].decode("utf-8")
                except UnicodeDecodeError:
                    ch = "?"
                self._put(ch)
                i += n
        self.buf = b[i:]

    def _put(self, ch):
        if self.col >= COLS:
            self.col = 0
            self.row = min(ROWS - 1, self.row + 1)
        self.cells[self.row][self.col] = ch
        self.col += 1

    def _escape(self, b, i):
        if i + 1 >= len(b):
            return None
        k = b[i + 1]
        if k == ord("["):
            j = i + 2
            while j < len(b) and not (0x40 <= b[j] <= 0x7e):
                j += 1
            if j >= len(b):
                return None
            params = b[i + 2:j].decode("ascii", "replace")
            final = chr(b[j])
            self._csi(params, final)
            return j + 1
        if k == ord("]"):                 # OSC ... BEL
            j = b.find(b"\x07", i)
            return None if j < 0 else j + 1
        return i + 2                      # ESC x

    def _csi(self, params, final):
        if params.startswith("?") or params.endswith(" "):
            return
        nums = [int(p) if p.isdigit() else 0 for p in params.split(";")] if params else []
        if final == "H":
            r = (nums[0] if len(nums) > 0 and nums[0] else 1) - 1
            c = (nums[1] if len(nums) > 1 and nums[1] else 1) - 1
            self.row = max(0, min(ROWS - 1, r))
            self.col = max(0, min(COLS - 1, c))
        elif final == "J":
            self.cells = [[" "] * COLS for _ in range(ROWS)]
        elif final == "K":
            for c in range(self.col, COLS):
                self.cells[self.row][c] = " "

    def text(self) -> str:
        return "\n".join("".join(r).rstrip() for r in self.cells)

    def __contains__(self, s):
        return s in self.text()


class KK:
    def __init__(self, argv, cwd=None, env=None):
        self.screen = Screen()
        self.master, slave = pty.openpty()
        fcntl.ioctl(self.master, termios.TIOCSWINSZ, struct.pack("HHHH", ROWS, COLS, 0, 0))
        e = dict(os.environ)
        e["TERM"] = "xterm-256color"
        e["KK_LPT1_CMD"] = "file:LPT1.OUT"   # print jobs go to a file
        e["KK_LPT2_CMD"] = "file:LPT2.OUT"
        if env:
            e.update(env)
        self.proc = subprocess.Popen(argv, stdin=slave, stdout=slave, stderr=slave,
                                     cwd=cwd, env=e, close_fds=True, preexec_fn=os.setsid)
        os.close(slave)
        self.log = []

    def __enter__(self):
        return self

    def __exit__(self, *a):
        self.close()

    def pump(self, timeout=0.2):
        end = time.time() + timeout
        while True:
            remaining = end - time.time()
            if remaining <= 0:
                break
            r, _, _ = select.select([self.master], [], [], remaining)
            if not r:
                break
            try:
                data = os.read(self.master, 65536)
            except OSError:
                break
            if not data:
                break
            self.log.append(data)
            self.screen.feed(data)

    def send(self, keys: str, delay=0.08):
        """Send text; tokens like <F10> or <ENTER> are translated.

        Each key (a character or a whole escape sequence) is written in one
        piece, as a real terminal does, with a short pause between keys."""
        i = 0
        while i < len(keys):
            if keys[i] == "<":
                j = keys.find(">", i)
                seq = KEYS[keys[i + 1:j]]
                i = j + 1
            else:
                seq = keys[i]
                i += 1
            os.write(self.master, seq.encode("latin-1"))
            self.pump(delay)

    def expect(self, text, timeout=5.0):
        end = time.time() + timeout
        while time.time() < end:
            self.pump(0.1)
            if text in self.screen:
                return True
            if self.proc.poll() is not None:
                break
        raise AssertionError(f"timeout waiting for {text!r}; screen:\n{self.screen.text()}")

    def dump(self, title=""):
        line = "-" * 80
        print(f"{line}\n{title}\n{line}\n{self.screen.text()}\n{line}")

    def wait_exit(self, timeout=5.0):
        end = time.time() + timeout
        while time.time() < end:
            self.pump(0.1)
            if self.proc.poll() is not None:
                return self.proc.returncode
        raise AssertionError("process did not exit")

    def close(self):
        if self.proc.poll() is None:
            try:
                os.killpg(self.proc.pid, signal.SIGKILL)
            except OSError:
                pass
            self.proc.wait()
        try:
            os.close(self.master)
        except OSError:
            pass
