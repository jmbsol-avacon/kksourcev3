# Kwik Kuote – Port to 64-bit Windows and POSIX

This document describes how the 1994–2000 MS-DOS application (Borland C++ 3.1,
16-bit, huge memory model) was brought to a modern 64-bit C/C++ build, what was
changed, how to build and run it, and what still deserves attention.

The guiding rule was **port, don't rewrite**: the application code is the
original C, compiled by a modern compiler, with the DOS/BIOS/Borland run-time
replaced by a small platform layer. Only what was required to compile, to be
correct on a 64-bit machine, or to avoid crashes that modern run-times detect
was changed. Every change is listed in [Changes to the legacy sources](#changes-to-the-legacy-sources)
and can be reviewed with `tools/port-diff.sh`.

## Repository layout

> **Note:** this copy of the documentation ships in the source-only package.
> The rows marked **Untouched** below (`Kk/`, `Kk_1/`, root `*.C`, `Kk.zip`,
> `PROJKK.EXE`, `Web/`) are in the separate reference package, not in this one.
> `tools/port-diff.sh` needs the reference package's `Kk/` folder copied next to `src/`.

| Path | Contents |
|---|---|
| `src/*.c`, `src/*.h` | The ported application sources (lower-case names, LF line endings). |
| `src/compat/` | The platform layer: Borland/DOS run-time on Windows, Linux and macOS. |
| `tools/makecfg.c`, `tools/driver.c` | The two configuration utilities, ported. |
| `tests/` | A pseudo-terminal harness and an end-to-end smoke test. |
| `CMakeLists.txt` | The build. |
| `Kk/` | **Untouched** – the canonical original sources (last edited in 2000). |
| `Kk_1/`, `*.C` in the root, `Kk.zip` | **Untouched** – older copies of the sources (see below). |
| `PROJKK.EXE`, `*.TR`, `*.TD` | **Untouched** – the old DOS executable and Turbo Debugger files. |
| `Web/` | **Untouched** – the separate Win32 CGI project that shares `Db_defs.h`. |

### Which copy of the sources is the real one

The repository held three loose copies of the program plus a zip. They were
compared file by file:

| Copy | Date | Notes |
|---|---|---|
| root `*.C` | early 1998 | Oldest. No Y2K fix in `CHECK.C`, 20-vendor limit, no `DB1.C`. |
| `Kk.zip` | June–August 1998 | Y2K date handling, 40-vendor limit, `DB1.C` split out, `MAKEFILE`. |
| `Kk/` | 1999–2000 | **Newest.** E-mail support, web-integration fields, `editor1.c`, product conversion. Byte-identical `Db_defs.h` to the `Web/` project. |
| `Kk_1/` | mixed | Partial copy, nothing newer than `Kk/`. |

`Kk/` is the canonical source and is what `src/` was made from. The shipped
`PROJKK.EXE` predates even the zip (it lacks strings that the 1998 sources
contain), so **it was built from a source revision older than any in the
repository**.

> **Data files: a decision for the customer.** The 2000 sources changed the
> record layouts of several data files compared with the 1998 ones (fields were
> added to the job, client, vendor, sales person, purchase order and discount
> records, e.g. `JBREC` grew from 395 to 489 bytes). If the data files in
> production were written by `PROJKK.EXE` they need converting before the ported
> program can read them. The 2000 version contains a conversion for the product
> file (`pd_convert`, System Manager ▸ Conversion menu); the other files would
> need a small one-off converter, which is easy to write once the production
> version is known. If the production data was written by a 2000-era build, no
> conversion is needed.

## Building

Requirements: CMake 3.16 or newer and a compiler with C11 and C++17 support.
No third-party libraries are used.

### Windows (Visual Studio 2019/2022)

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The executables are in `build\Release\`: `kk.exe`, `makecfg.exe`, `kkdriver.exe`.
MinGW-w64 also works (`cmake -G "MinGW Makefiles"` or Ninja); the project
was cross-compiled with `x86_64-w64-mingw32-gcc` as part of the port.

### Linux / macOS

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release   # or omit -G Ninja for make
cmake --build build
```

Options: `-DKK_REP_VERSION=ON` defines `REP_VER` for the Representative
variant (the original sources mark that variant as broken; it is not tested),
`-DKK_DEBUG_ASSERTS=ON` enables the original `Assert()`/`ErrorLog` diagnostics.

### Running

`kk` must be started in its data directory (it checks for `DB.CFG` there), as
the DOS version did:

```
cd C:\KK
kk [username] [password]
```

To set up a brand new data directory: create the sub-directories `HELP`,
`PRODUCTS`, `PLIST`, `RSCH`, `INST` and `FAX`, run `makecfg` (writes a default
`DB.CFG`) and `kkdriver` (writes `DEFAULT.DRV` and `EPSON.DRV`). The record
files (`AUTOMAT.CD` etc.) are created on first use. The original help files
(`HELP\DBHELP.*`) are not in the repository.

The program needs a terminal of at least 80×25 characters. On Windows it
switches the console to its own 80×25 screen buffer and restores the previous
one on exit; on POSIX it uses the terminal's alternate screen. On Windows the
console font should contain the box-drawing characters (all standard fonts do).

## Verification

* `tests/smoke_test.py <build-dir>` drives the real executable in a
  pseudo-terminal (POSIX only): start-up, login window, the menus, adding a
  client record, printing (spooled to a file), the word processor, a clean exit,
  and a second run that finds the record again. It also checks that the record
  file has the DOS name and the Borland byte layout.
* Every on-disk record structure was checked against the Borland byte-packed
  size computed independently from the field declarations (all 26 match, e.g.
  `SYSREC` 876, `CDREC` 344, `POREC` 292, `JBREC` 489 bytes).
* The port was built with GCC 13, Clang 18 and MinGW-w64 GCC 13 (cross build),
  and run under AddressSanitizer/UndefinedBehaviorSanitizer, which found and
  led to the memory-safety fixes listed below.
* The Windows console back end has been compiled (MinGW) but not run in this
  environment; it should be exercised on a Windows machine first.

## The platform layer (`src/compat`)

Every legacy translation unit is compiled with `kk_compat.h` force-included.
That header provides the Borland keywords and constants and routes the C
library calls that have DOS semantics through the layer. The Borland headers
the sources include (`conio.h`, `dos.h`, `bios.h`, `dir.h`, `alloc.h`, and on
POSIX `io.h`, `share.h`, `process.h`, `malloc.h`) are supplied by the layer.

| Area | Original | Port |
|---|---|---|
| Screen | Direct writes to the text-mode video buffer through Borland conio | A virtual 80×25 buffer of character/attribute cells with the exact conio semantics (`window()`, `textattr()`, `gettext()`/`puttext()`, `cprintf()` wrapping and scrolling, `'\n'` without carriage return). Changed cells are painted when the program waits for input. CP437 characters are translated to Unicode. |
| Terminal | – | `kk_term_posix.cpp`: raw termios, ANSI/VT escapes, 16 colours, SGR mouse reporting. `kk_term_win32.cpp`: own console screen buffer, `WriteConsoleOutputW`, `ReadConsoleInputW`. |
| Keyboard | `bioskey()` INT 16h codes (scan code ≪ 8 \| ASCII) | Same codes, produced from terminal escape sequences or Windows virtual-key events; F1–F12, Shift/Ctrl/Alt variants, arrows, Alt+letter. The BIOS Insert toggle is emulated. |
| Mouse | INT 33h, pixel coordinates (8×8 cells) | Same API (`vmouse.h`), fed by terminal/console mouse events; coordinates are cell × 8 as before. |
| Printing | `PRN`, `LPT1`, `LPT2`, `stdprn`, `biosprint()` | Output is spooled per port and handed to a host print command (see below). |
| Screen reports | Writing to the `CON` device | A pseudo stream whose output goes to the virtual screen. |
| Files | 16-bit `open()`/`lock()`/`findfirst()`, back-slash paths, 8.3 names | Path translation (back-slashes, drive letters, DOS blank-padding, case-insensitive lookup on POSIX), `fcntl`/`_locking` region locks, `glob`/`FindFirstFile` searches. |
| Date/time, speaker, delay | `getdate()`, `gettime()`, `sound()`, `delay()` | Host clock; `sound()+delay()` becomes a beep. |
| DOS shell, `move` | `system("command")`, `system("move ...")` | Host shell (`COMSPEC`/`SHELL`) with the terminal restored for the child; a file-move routine. |
| Memory checks | `coreleft()`, `stackavail()` | Report plenty. |

### Printing

DOS wrote escape codes straight to the parallel port. Now each print job is
written to a spool file and passed to a command when the job is complete (the
stream is closed, the program is idle for two seconds after printing, or it
exits). The command is taken from the environment:

| Variable | Default (Windows) | Default (POSIX) |
|---|---|---|
| `KK_LPT1_CMD` (also `PRN`) | `notepad /p "%s"` | `lp -s "%s"` |
| `KK_LPT2_CMD` | `notepad /p "%s"` | `lp -s "%s"` |
| `KK_LPT3_CMD` | `notepad /p "%s"` | `lp -s "%s"` |

`%s` is replaced by the spool file name. Examples:

```
set KK_LPT1_CMD=print /D:LPT1 "%s"            rem a real parallel/redirected port
set KK_LPT1_CMD=copy /B "%s" \\SERVER\Printer rem raw copy to a shared printer
set KK_LPT1_CMD=file:C:\KK\PRINTOUT.TXT       rem append to a file instead
export KK_LPT1_CMD='lp -d office "%s"'
```

If the command fails the document is appended to `LPT1.PRN` (resp. `LPT2.PRN`)
in the data directory and a line is written to `AUTOMAT.ERR`. Spool files are
created in `KK_SPOOL_DIR`, else `TMPDIR`/`TEMP`/`TMP`.

The printer driver files (`*.DRV`, created by `kkdriver`) still contain Epson
control codes (condensed on/off). With a modern printer driver or `notepad /p`
these print as stray characters; a driver file with empty code strings can be
created with the System Manager's printer setup, or `kkdriver` can be adjusted.

### Behaviour notes

* `Ctrl+Enter` (form feed in the editor) cannot be distinguished from `Enter`
  by most POSIX terminals; on Windows it works.
* The Insert key state is read from the keyboard on Windows and emulated on
  POSIX (starts in overwrite mode, like the BIOS).
* "DOS SHELL" in the System Manager starts the host shell; type `exit` to return.
* The e-mail function of the 2000 version writes message files into the `FAX`
  directory for an external mail agent that is not part of this repository.
* The BACKUP and RESTORE menu entries were already disabled in the original.

## Changes to the legacy sources

`tools/port-diff.sh` prints the full diff against `Kk/` (ignoring white space
and line endings); `tools/port-diff.sh --stat` prints counts. The changes fall
into these groups.

### Compiling with a modern compiler

* `stdinc.h`: `SHORT`, `USHORT`, `LONG`, `ULONG`, `WORD` are fixed-width
  (`int16_t`, `int32_t`, …) because they occur in the on-disk records.
  `INT` stays the native `int` (32 bits now, 16 then).
* `db_defs.h`: the record structures are wrapped in `#pragma pack(push, 1)`
  (Borland's default was byte alignment); `intcmp()` takes `SHORT *`.
* `#include <sys\stat.h>` → `<sys/stat.h>` (seven files).
* `sys_errlist[errno]` → `strerror(errno)`; the `extern … sys_errlist[]` and
  `extern int errno` declarations are removed (`errno` is a macro today).
* Borland "lvalue cast" statements (`(CHAR *)p = q;`, `(CHAR *)p += n;`) are
  rewritten as ordinary assignments (`screen.c`, `input.c`, `output.c`,
  `s_file.c`, `sort.c`).
* `main()` returns `int` (`db.c`, `tools/makecfg.c`, `tools/driver.c`).
* `vfile.h` includes `dos.h` for `struct find_t`.
* `db_defs.c`: the terminating table entry uses `""` instead of `NULL` for the
  character arrays.

### 16-bit assumptions

* `CT_INT` fields (the `int` of 1994) are 16-bit record fields: the generic
  screen/print/compare/input code accesses them as `SHORT`
  (`screen.c`, `output.c`, `s_file.c`, `sort.c`, `input.c`, `db_misc.c`);
  `timeout` in `db_sys.c` is a `SHORT`; two `sscanf("%d")` into 16-bit fields
  use `%hd` (`db.c`); `memset(tagged, …, sizeof(INT))` uses `sizeof(BOOLEAN)`
  (`kkuote.c`).
* Values of type `LONG` (now `int32_t`) are cast to `long` where a `%ld` format
  expects one (`screen.c`, `output.c`, `s_file.c`, `input.c`, `db.c`, `db1.c`,
  `kkuote.c`).
* `time(&secs)` is given a `time_t` instead of a 32-bit `LONG` (`screen.c`,
  `vmisc.c`).
* The record count written at the head of a converted product file is a
  `USHORT`, not `sizeof(int)` (`db_sys.c`, two places).
* `StrToTime()` scans into `int` temporaries instead of single-byte fields
  (`vmisc.c`); `s_openfiles()` reads its variable arguments as `int`
  (`s_file.c`; GCC compiles the old form into a trap).

### DOS, BIOS and inline assembly

* `editor.c`: the three INT 10h calls (scroll up/down, set cursor) call
  `KK_BiosScroll()`/`KK_BiosGotoXY()`; two `printf(" ")` become `cprintf(" ")`
  because stdout no longer reaches the screen.
* `vmisc.c`: `SetCursor()` calls `KK_SetCursor()`; `IsShareInstalled()` returns
  TRUE (file sharing is a property of the host now).
* `vfile.c`: `FileFlush()` uses `_commit()` instead of INT 21h.
* `VMOUSE.C` is not compiled; the platform layer implements its API.
* `db_sys.c`: `system("command")` runs the host shell; the two DOS `move`
  commands use `KK_MoveFiles()`; `getsec()` no longer calls `strcmpi(NULL, …)`
  (a crash with modern C libraries when started without arguments).
* `kkuote.c`, `tools/makecfg.c`: Borland's three-argument `_chmod()` is
  `_dos_setfileattr()`; `tools/driver.c` uses `open()`/`close()` directly.

### Memory safety (crashes that DOS tolerated)

These are pre-existing bugs that corrupted a few bytes of stack silently on
DOS; modern stack protection turns them into aborts, and AddressSanitizer
found them during the port:

* `screen.c`: `print_time()`/`print_date()` buffers hold the two bytes per cell
  that `screendump()` writes.
* `db.c`: `poCode[10]` (index 9 is written); `kkuote.c`: `pTotal[20]`
  (`"%-16.2f"` needs 17 bytes); `kk_more.c`: `prompt[80]`.
* `db_inpt.c`: `show_product_list()` returned a pointer to its own stack
  buffer; the buffer is static now.
* `db_print.c`: the file-list loop indexed `db[-1]`.
* `db.c`, `input.c`: unsequenced `i++`/`++out` expressions made explicit.
* `output.c`: converting a numeric format to a string format drops the `l`
  modifier (`"%-6ld"` → `"%-6s"`); modern libraries treat `%ls` as a wide
  string.

### Known, unchanged behaviour

Some bugs of the original were left alone because fixing them changes data
semantics; they behave exactly as before: `sprintf(po->inv_number, "%08ld")`
writes its terminator into the next field, `cur_retanige` receives four bytes
(`"%-3.3d"` of 100), and `delete_char()` compares a pointer with `'\0'`.

## What was not ported

* `INSTALL.C` (floppy-disk installer that runs `KKDATA.EXE`) and `NOTABS.C`
  (a developer utility) are not built.
* `REP_RPTS.C` duplicates reports in `DIS_RPTS.C` and was never linked by the
  original `MAKEFILE`; the Representative variant is untested.
* The `Web/` CGI project is unrelated to the console application.
