# Kwik Kuote – Distributor Information System

Kwik Kuote is a quotation, purchase-order and job-tracking system written in
1994–2000 for MS-DOS in Borland C++ 3.1. This package contains the modern
64-bit build of that program for Windows, Linux and macOS: the ported source
code, the build files, the tests and the documentation, and nothing else.

**Start with [HANDOFF.md](HANDOFF.md)** – how to build, install and configure.

## What is here

| Path | Contents |
|---|---|
| `CMakeLists.txt` | The build (CMake 3.16+) |
| `src/` | The application – the original C, ported (see [docs/PORTING.md](docs/PORTING.md)) |
| `src/compat/` | Platform layer replacing the DOS/BIOS/Borland run-time: 80×25 text screen on the Windows console or a POSIX terminal, keyboard, mouse, DOS file semantics, printer spooling |
| `tools/` | `makecfg` and `kkdriver` set-up utilities; `port-diff.sh` |
| `tests/` | End-to-end smoke test (Linux/macOS) |
| `docs/` | Porting notes and business requirements |

Existing data files are read as they are: every record keeps its Borland
byte-packed layout.

## Quick build

```sh
cmake -S . -B build          # add -G "Visual Studio 17 2022" -A x64 on Windows
cmake --build build --config Release
```

Produces `kk` (the application), `makecfg` (default `DB.CFG`) and `kkdriver`
(default printer driver files). Run `kk [user] [password]` from the data
directory.

## The original sources

The 1994–2000 originals (`Kk/`, `Kk_1/`, `Kk.zip`, the old `PROJKK.EXE`, the
`Web/` CGI project) are **not** in this package; they are in the separate
reference package delivered earlier. `docs/PORTING.md` refers to them when it
explains where the port came from. To see every change the port made to the
original code, copy the `Kk/` folder from the reference package next to `src/`
and run `tools/port-diff.sh`.
