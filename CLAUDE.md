# pcsx-ab - CLAUDE.md

The AutoBleem team's PCSX-ReARMed fork for the PlayStation Classic (author: screemer, the repo owner), being
ported to the Raspberry Pi to run under autobleem-develop's Pi port. This file is the project knowledge of
record; keep it current in the same commit as any change it describes. Git history has the reasoning per
change (commit messages are prose).

## State (2026-09-17)

| | |
|---|---|
| Build system | CMake + Ninja (`CMakeLists.txt`); upstream `configure`/Makefiles deleted |
| Raspberry Pi cross build | `./make_rpi.sh` -> `build_rpi/dist/` - builds, links, **never run on a Pi** |
| Raspberry Pi 64-bit cross build | `./make_rpi64.sh` -> `build_rpi64/dist/` - interpreter + peops GPU (no aarch64 dynarec/NEON in this fork), **no toolchain installed, unbuilt, unrun** |
| Windows dev build | `./make_win.sh` -> `build_win/pcsx-ab.exe` - runs games (interpreter, peops GPU) |
| PlayStation Classic | `./make_psc.sh` -> `build_psc/dist/` via the build server - builds and links with the GLES/Wayland path; **not yet run on a console** |
| Video on the Pi | SDL2 renderer + streaming texture (`plat_sdl_present`), no Wayland/GLES - proven on Windows |
| Gamepad | SDL2 GameController driver (`in_sdl2gc.c`) - proven on Windows with an Xbox pad |
| CHD images | `handlechd`/`cdread_chd` over vendored static libchdr (upstream, with zstd - the same tree as autobleem-develop's) - frame-exact vs bins; CDDA-by-ear untested |
| Not started | `dist/` folder (rpi + win32 binaries + plugins in AutoBleem's `emu/` layout); AutoBleem's own Windows launch support |

## Decisions (made by the owner - do not re-ask)

- **CMake, mirroring autobleem-develop**: same sysGCC toolchain (`C:\sysGCC\raspberry`), same
  `make_rpi.sh` / `toolchains/rpi/` shape, so the two projects build side by side and the result drops into
  AutoBleem's `payload_rpi/Autobleem/bin/emu/` (`AUTOBLEEM_DIR=../autobleem-develop ./make_rpi.sh`).
- **64-bit Pi (2026-09-18), same idea**: `toolchains/rpi64/RPi64toolchain.cmake` + `make_rpi64.sh`, over
  "SysGCC for Raspberry Pi (64-bit)" at `C:\sysGCC\raspberry64` (not installed on this host yet - see
  AutoBleem's CLAUDE.md for the owner's decision on how it's obtained). `CMAKE_SYSTEM_PROCESSOR=aarch64`
  deliberately does **not** match `CMakeLists.txt`'s `_pcsxab_is_arm` regex (`^(arm|ARM)`): Ari64's dynarec
  and the NEON GPU/GTE code are 32-bit ARM assembly only, no aarch64 backend in this fork, so the 64-bit Pi
  builds like the Windows dev build does - C interpreter, `PCSXAB_BUILTIN_GPU=peops` - correct, just slower
  per clock than the 32-bit Pi's NEON dynarec. `toolchains/rpi64/cmake/Find{SDL2,PNG}.cmake` reuse
  `toolchains/rpi/devkit/include` by relative path (arch-independent headers) rather than duplicating it;
  only the sysroot library directory (`usr/lib/aarch64-linux-gnu`) differs. Drops into AutoBleem's
  `payload_rpi/Autobleem/bin/emu-arm64/`, a separate directory from the 32-bit `emu/` because the two
  binaries cannot share one checked-in path.
- **No Wayland on the Pi.** `PCSXAB_GLES=OFF` compiles out `frontend/libpicofe/gl_platform.c` (the console's
  EGL-on-Weston output) and `gpu_gles.so`; video goes through SDL2's renderer on KMSDRM. GL branches are
  untouched for the console.
- **Codec libraries linked static** (FLAC, lzma, libchdr) - nothing new to install on the console or the Pi.
  The system zlib stays for `gz*()` save-state I/O; libpng needs libz on every target anyway.
- `develop` is the branch (git-flow like the owner's other repos); the repo was initialised from a console-SDK
  export, no upstream history.

## Layout

```
CMakeLists.txt              the whole build; PCSXAB_* options replace configure's guesses
make_rpi.sh / make_win.sh   the local builds (MSYS2 UCRT64 shell; ucrt64/bin on PATH for cmake/ninja)
make_psc.sh                 the console build, on the build server over ssh (see "Build server")
toolchains/rpi/             RPitoolchain.cmake + devkit/ (SDL2 + libpng headers, hand-written Linux
                            SDL_config.h) + cmake/Find{SDL2,PNG}.cmake - the sysroot has runtime .so's, no -dev
toolchains/rpi64/           RPi64toolchain.cmake + cmake/Find{SDL2,PNG}.cmake, reusing toolchains/rpi/devkit/
                            (arch-independent headers) - only the sysroot lib dir differs from the 32-bit one
toolchains/psc/             PSCtoolchainV8.cmake (config.mak.autobleem as CMake) + cmake/FindSDL2.cmake, which
                            copies the sysroot's SDL2 2.0.4 headers into the build tree with X11 undefined
frontend/                   PCSX-ReARMed frontend: main.c, menu.c, plat_sdl.c, plugin_lib.c (+AB additions)
frontend/libpicofe/         notaz's platform lib: plat_sdl.c (video, renderer path), in_sdl.c (keyboard),
                            in_sdl2gc.c (pads), input.c, menu.c; linux/ (plat.c, in_evdev.c) on the targets
frontend/win32/             Windows host layer: plat_win32.c + include/dirent.h (shadows mingw's)
include/win32_compat.h      dlopen/mkdir/fsync/strcasestr for MinGW
libpcsxcore/                emulator core; cdriso.c has the disc readers incl. CHD; memmap_win32.c
plugins/                    dfsound (SPU, built in), gpulib + gpu_neon/dfxvideo/gpu_unai (built-in GPU +
                            loadable .so/.dll), spunull, cdrcimg, dfinput
third_party/libchdr/        vendored upstream libchdr (BSD) + deps/{lzma,zstd}, static; zlib is the system one (CHDR_SYSTEM_ZLIB)
```

## Building and running

```bash
./make_rpi.sh            # clean cross build; -k incremental; AUTOBLEEM_DIR=... copies into AutoBleem
./make_rpi64.sh          # same, for the 64-bit Pi; copies into AutoBleem's emu-arm64/ instead of emu/
./make_win.sh            # Debug build + runtime DLLs next to the exe
```

Run directory = what AutoBleem's `launch.sh` sets up: `.pcsx/` (config, memcards, sstates), `bios/`,
`plugins/`, `skin/` (from `frontend/pandora/skin`). Args as launch.sh passes them:

```
pcsx-ab -filter 0 -ratio 0 -lang 0 -region 0 -enter 1 -cdfile "D:/AB/Games/X/game.cue"
```

Test material on this machine: `D:\AB\Games` (cue/bin, PBP, and CHDs: Abe2, WipEout XL - 12 tracks with
CDDA, Geppy-X, Resident Evil 2, Tomb Raider II), `D:\AB\Games (copy)\MDK (US)` (30-track cue). Re-Volt's
EBOOT.PBP and MDK were the first games run. A gamepad log line to look for: `sdl2gc:Probed controller`.

## Build server (PlayStation Classic)

`ssh psc-build` (a `Host` entry in `~/.ssh/config`, in both the Windows profile and `C:\msys64\home\<you>` -
MSYS2's ssh and Git for Windows' ssh have different homes; key `~/.ssh/id_ed25519`, installed on the server
2026-09-17). Ubuntu x86_64, 2 cores, Sony's crosstool-NG toolchain at `/opt/toolchain` (GCC 8.2.0, sysroot
`/opt/toolchain/armv8-sony-linux-gnueabihf/sysroot` with SDL2 2.0.4, libpng 1.6.28, zlib 1.2.8, EGL/GLES,
Wayland dev files). The distro CMake is 3.10; `~/opt/cmake` (3.31) is what `make_psc.sh` uses. The tree is
synced to `~/pcsx-ab`, built in `~/pcsx-ab/build_psc` (unstripped binaries stay there for gdb). No ninja
there - Unix Makefiles, `-j2`.

## Things learned the hard way

- **GCC 14 vs Sony's GCC 8**: `-fcommon` is required (tentative definitions in headers, `.comm` in
  linkage_arm.S); four `-Wno-error=` flags keep ~30 pre-existing implicit prototypes at warning level.
  They are tech debt, not fixed.
- **Source files are CRLF with tabs.** Edit by exact-string or line-range replacement in a Python script
  (normalise CRLF, restore on write); bash heredocs mangle backslashes and quotes in this environment - use
  the Write tool for new files and for scripts containing C string literals.
- **`CMAKE_TRY_COMPILE_PLATFORM_VARIABLES`**: a toolchain file's own `-D` variables are invisible inside CMake's
  try_compile sandboxes unless listed there - the compiler probe silently used the default path.
- **The console sysroot's `SDL_config.h` defines `SDL_VIDEO_DRIVER_X11`** but has no X11 headers; and X11 being
  defined would compile the Wayland handoff in `plat_sdl.c` *out*. The PSC FindSDL2 copies the headers with
  that define removed - the original build evidently had headers without it.
- **rsync onto NTFS from MSYS2** fails setting POSIX modes; results come back through a tar pipe.
- **A window has a surface or a renderer, never both** (SDL >= 2.28 refuses). The non-GL path never calls
  `SDL_GetWindowSurface`; the GL branch still does, but is unreachable when `HAVE_GLES` is off.
- **`SDL_CONTROLLER_BUTTON_MAX` grew** (15 -> 21 in 2.0.14+): a "0 = unmapped" default in
  `in_sdl2gc_key_map` meant every new unpressed button cleared d-pad UP. Default is -1 now.
- **`SysLibError()` must return NULL on success** - `plugins.c`'s `CheckErr` treats any non-NULL as failure;
  the old `_WIN32` stub returned a string and no plugin could load.
- **ISOopen's mode1/2048 guess** (file size % 2048) and the sub_mixed override now apply only to the plain
  reader; they would silently replace a container's reader.
- CHD facts: tracks stored back to back, each padded to 4 frames; PGTYPE starting with `V` = pregap stored in
  the frames, otherwise virtual (reads as silence); audio big-endian; FLAC's `cpu.c` prints on every decoder
  init unless `NDEBUG`.
- `spu.c` called `tanh()` without `<math.h>` - on hard-float ARM that read the result from r0. Fixed; it
  shipped that way on the console.

## Working agreements (same as autobleem-develop)

One commit per step, each building on both targets (`make_rpi.sh` and `make_win.sh`) and smoke-run on
Windows where a game can prove it. Commit messages explain why, including what was deliberately not done.
Vendored code is compiled with `-w` and not made warning-free. Update this file in the same commit when the
layout, a decision or the status table changes.
