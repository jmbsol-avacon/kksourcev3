#!/usr/bin/env python3
"""
smoke_test.py - end-to-end check of the ported Kwik Kuote in a pseudo-terminal.

    python3 tests/smoke_test.py <build-dir>

Creates a fresh data directory with makecfg/kkdriver, then drives kk through
login, the menus, adding a client record, printing (spooled to a file), the
word processor and a clean exit, and finally checks that the record file has
the DOS name and layout and that the record is still there on a second run.
"""
import os
import shutil
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from kkpty import KK  # noqa: E402

CDREC_SIZE = 344          # byte packed CDREC (see db_defs.h)
HEADER_SIZE = 2           # USHORT record count


def main(build):
    build = os.path.abspath(build)
    kk_exe = os.path.join(build, "kk")
    data = tempfile.mkdtemp(prefix="kkdata.")
    try:
        for d in ("HELP", "PRODUCTS", "PLIST", "RSCH", "INST", "FAX"):
            os.mkdir(os.path.join(data, d))
        assert os.system(f'cd "{data}" && "{build}/makecfg" >/dev/null') == 0
        assert os.system(f'cd "{data}" && "{build}/kkdriver" >/dev/null') == 0
        assert os.path.getsize(os.path.join(data, "DB.CFG")) == 876

        print("1. start without arguments: intro, login window, F10 exits")
        with KK([kk_exe], cwd=data) as kk:
            kk.expect("Automation Solutions", 10)       # intro screen
            kk.send("<ENTER>")
            kk.expect("Username:", 10)
            kk.send("<F10>")
            assert kk.wait_exit() == 0

        print("2. master login, menus, add a client, print, editor, exit")
        with KK([kk_exe, "iran", "nari"], cwd=data) as kk:
            kk.expect("Automation Solutions", 10)
            kk.send("<ENTER>")
            kk.expect("MAIN MENU", 10)
            kk.send("1")
            kk.expect("INFORMATION MENU")
            kk.expect("CLIENT INFORMATION")
            kk.send("1")
            kk.expect("CLIENT DATABASE INPUT SCREEN")
            kk.send("<F2>")
            kk.expect("Enter new record's")
            kk.send("ACME<ENTER>")
            kk.pump(0.5)
            kk.send("Acme Corporation<ENTER>")
            kk.pump(0.3)
            kk.send("John Smith<ENTER>")
            kk.pump(0.3)
            kk.send("<F10>")
            kk.expect("Save changes")
            kk.send("Y")                 # saving leaves the screen
            kk.expect("INFORMATION MENU")
            kk.send("1")
            kk.expect("CLIENT DATABASE INPUT SCREEN")
            kk.expect("ACME")
            kk.expect("Acme Corporation")
            kk.dump("client screen after adding ACME")

            # F5 prints the current record to the default printer (spooled)
            kk.send("<F5>")
            kk.pump(3.5)                 # idle: the spooled job is handed over
            kk.send("<F10>")
            kk.expect("INFORMATION MENU")
            kk.send("5")
            kk.expect("MAIN MENU")

            kk.send("7")
            kk.expect("F2=SAV")
            kk.send("Hello from the port.<ENTER>")
            kk.expect("Hello from the port.")
            kk.send("<F10>")
            kk.expect("save your changes")
            kk.send("N")
            kk.expect("MAIN MENU")

            kk.send("8")
            kk.expect("SYSTEM MANAGER MENU")
            kk.send("<F10>")
            kk.expect("MAIN MENU")
            kk.send("9")                 # EXIT THE SYSTEM returns to the login window
            kk.expect("Username:")
            kk.send("<F10>")             # ... and F10/ESC there ends the program
            assert kk.wait_exit() == 0

        files = sorted(os.listdir(data))
        print("   files:", files)
        assert "AUTOMAT.CD" in files, "client file must have the DOS name without blanks"
        assert not any(f.endswith(" ") for f in files)
        cd = open(os.path.join(data, "AUTOMAT.CD"), "rb").read()
        n = int.from_bytes(cd[:2], "little")
        assert n == 1 and len(cd) == HEADER_SIZE + CDREC_SIZE, (n, len(cd))
        assert cd[2:2 + 4] == b"ACME" and b"Acme Corporation" in cd and b"John Smith" in cd
        if os.path.exists(os.path.join(data, "LPT1.OUT")):
            out = open(os.path.join(data, "LPT1.OUT"), "rb").read()
            assert b"ACME" in out and b"\x0c" in out, out[:200]
            print("   print spool: %d bytes handed to the print command" % len(out))
        else:
            print("   WARNING: no spooled print output found")
        assert not os.path.exists(os.path.join(data, "AUTOMAT.ERR")), \
            open(os.path.join(data, "AUTOMAT.ERR")).read()

        print("3. second run finds the record again")
        with KK([kk_exe, "iran", "nari"], cwd=data) as kk:
            kk.expect("Automation Solutions", 10)
            kk.send("<ENTER>")
            kk.expect("MAIN MENU", 10)
            kk.send("1")
            kk.expect("CLIENT INFORMATION")
            kk.send("1")
            kk.expect("Acme Corporation")
            kk.send("<F10>")
            kk.expect("INFORMATION MENU")
            kk.send("5")
            kk.expect("MAIN MENU")
            kk.send("9")
            kk.expect("Username:")
            kk.send("<F10>")
            assert kk.wait_exit() == 0
        print("PASS")
    finally:
        shutil.rmtree(data, ignore_errors=True)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "build")
