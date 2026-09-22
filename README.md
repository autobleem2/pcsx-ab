pcsx-ab - the PlayStation 1 emulator for AutoBleem 2
====================================================

**pcsx-ab is a fork of [PCSX-ReARMed](http://notaz.gp2x.de/pcsx_rearmed.php) maintained by the AutoBleem
team.** It is the classic PS1 emulator [AutoBleem 2](https://github.com/autobleem2/autobleem) launches on
the PlayStation Classic, Raspberry Pi and PC: AutoBleem's launch scripts drive it, and it reads AutoBleem's
`pcsx.cfg`, memory cards and save-state layout. The successor emulator is
[pcsx-abnxt](https://github.com/autobleem2/pcsx-abnxt).

Authors: screemer, nex, mGGk, ThaFridge and the AutoBleem contributors, on top of the PCSX / PCSX-ReARMed
authors.

*See [readme.txt](readme.txt) for the upstream PCSX-ReARMed documentation.*

License
-------

GPL-3.0-or-later (see [LICENSE](LICENSE)). The core emulator sources are licensed under the GNU GPL "version
2 of the License, or (at your option) any later version" (upstream PCSX-ReARMed - see [COPYING](COPYING) and
the per-file headers); AutoBleem 2 distributes the combined work under GPL-3.0-or-later, which that "or
later" option permits. Bundled third-party components keep their own licences (see `third_party/*/LICENSE`).

Building
--------

The build is CMake (`CMakeLists.txt`); the upstream `./configure` + `Makefile` are gone. Everything
`configure` used to guess is a `PCSXAB_*` option, and a toolchain file sets them per target. Output is
`pcsx-ab` plus `plugins/*.so` - the layout AutoBleem's `Autobleem/bin/emu/` holds.

**Raspberry Pi (32-bit Raspberry Pi OS), cross-compiled from Windows** - the same toolchain, sysroot and
script layout as AutoBleem's `make_rpi.sh`, so the two build side by side:

```bash
./make_rpi.sh                                    # MSYS2 UCRT64 shell; needs C:/sysGCC/raspberry
AUTOBLEEM_DIR=../autobleem-develop ./make_rpi.sh # ...and drop the result into AutoBleem's payload_rpi
```

Result: `build_rpi/dist/` (stripped `pcsx-ab` + `plugins/`). `toolchains/rpi/` holds the toolchain file and,
because the sysGCC sysroot has the runtime libraries but no `-dev` packages, a small devkit of SDL2 and
libpng headers with `FindSDL2`/`FindPNG` modules that point at the sysroot's versioned `.so`s.

The Pi build has **no Wayland and no GLES**: `PCSXAB_GLES=OFF` compiles out `frontend/libpicofe/gl_platform.c`
(the console's EGL-on-Weston output) and `gpu_gles.so`. Video goes through an SDL2 renderer instead - the
frame is uploaded to a streaming texture and scaled on the GPU by SDL's KMSDRM/GLES2 backend
(`plat_sdl_present()` in `frontend/libpicofe/plat_sdl.c`); `-ratio` pillarboxes to 4:3, `-filter` picks
linear/nearest, exactly as the console's GL path does.

**Raspberry Pi (64-bit Raspberry Pi OS, Trixie), cross-compiled from Windows** - same idea, a separate
toolchain (`E:/sysGCC/raspberry64`, not the 32-bit one):

```bash
./make_rpi64.sh                                    # MSYS2 UCRT64 shell; needs E:/sysGCC/raspberry64
AUTOBLEEM_DIR=../autobleem-develop ./make_rpi64.sh # ...and drop the result into AutoBleem's emu-arm64/
```

Result: `build_rpi64/dist/`. This fork's dynarec and NEON GPU/GTE code are 32-bit ARM assembly with no
aarch64 backend, so the 64-bit Pi build has no dynarec and no NEON - it runs the C interpreter with the
`peops` GPU, the same code path as the Windows dev build, just cross-compiled. Slower per clock than the
32-bit Pi's dynarec+NEON build, but correct.

**PlayStation Classic** - Sony's toolchain, `PCSXAB_GLES=ON` (what the old `config.mak.autobleem` did:
EGL on the Weston surface SDL hands over, `gpu_gles.so` included). The toolchain lives on the build server,
so this one builds over ssh:

```bash
./make_psc.sh                    # rsync up, build there with toolchains/psc/PSCtoolchainV8.cmake, fetch
```

Result: `build_psc/dist/` (stripped `pcsx-ab` + `plugins/`). Needs a `Host psc-build` entry in
`~/.ssh/config` with key login. On a machine that has the toolchain locally:

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=toolchains/psc/PSCtoolchainV8.cmake       -DPCSXAB_PSC_TOOLCHAIN=/opt/toolchain -B build_psc && cmake --build build_psc
```

**Windows development build (MSYS2 UCRT64 / MinGW)** - for running the frontend on the PC, the way
AutoBleem's `make_win.sh` is used. x86 has no dynarec, so this is the interpreter and the peops GPU; fast
enough to play, but its purpose is debugging the SDL2 video path, menu, input and save states without a Pi:

```bash
./make_win.sh                    # needs mingw-w64-ucrt-x86_64-{gcc,cmake,ninja,SDL2,libpng}
mkdir run && cd run && mkdir .pcsx bios && cp -r ../build_win/plugins . && cp -r ../frontend/pandora/skin .
../build_win/pcsx-ab.exe -filter 0 -ratio 0 -lang 0 -region 0 -enter 1 -cdfile "D:/Games/Game/EBOOT.PBP"
```

(That run directory is what AutoBleem's `launch.sh` sets up on the console: `.pcsx/` for the config, memory
cards and states, `bios/`, `plugins/`, `skin/`.) The host layer is `frontend/win32/` plus
`include/win32_compat.h`: mmap over VirtualAlloc, a `<dirent.h>` with `d_type`/`scandir`, `dlopen`, `fsync`,
`strcasestr`; the console's power-button and CPU-temperature watchers are compiled out.

**Natively on a Pi / any ARM Linux** with `libsdl2-dev libpng-dev zlib1g-dev` installed:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
```

Options (`cmake -L`): `PCSXAB_ENABLE_MENU`, `PCSXAB_NEON`, `PCSXAB_DYNAREC`, `PCSXAB_BUILTIN_GPU` (neon/peops/unai),
`PCSXAB_SOUND_DRIVERS` (sdl alsa oss pulseaudio), `PCSXAB_GLES`, `PCSXAB_PLUGINS`, `PCSXAB_LINK_MAP`, `PCSXAB_CHD`.

**CHD images** (`PCSXAB_CHD`, on by default): `.chd` discs from `chdman createcd` load like any other image
(`-cdfile game.chd`), multi-track with CD audio included. The reader is `cdread_chd` in
`libpcsxcore/cdriso.c` on top of the vendored `third_party/libchdr` (upstream libchdr with its zstd, zlib and
LZMA-SDK codecs, and header-only FLAC), built static so no extra library is needed on the console or the Pi.

**Status:** pcsx-ab is the emulator AutoBleem 2 ships and runs on real hardware - the PlayStation Classic
(GLES on Weston) and the Raspberry Pi (SDL2 renderer); the Windows/PC build runs games through the
interpreter for development. Continuous integration (`.github/workflows/build.yml`) builds the four cross
targets - `psc`, `rpi`, `rpi64`, `pcusb` - in AutoBleem's shared toolchain image and a native Windows build
on GitHub Actions, and packages each for AutoBleem's download site.

PCSX ReARMed is yet another PCSX fork based on the PCSX-Reloaded project,
which itself contains code from PCSX, PCSX-df and PCSX-Revolution. This
version is ARM architecture oriented and features MIPS->ARM recompiler by
Ari64, NEON GTE code and more performance improvements. It was created for
Pandora handheld, but should be usable on other devices after some code
adjustments (N900, GPH Wiz/Caanoo, PlayBook versions are also available).

PCSX ReARMed features ARM NEON GPU by Exophase, that in many cases produces
pixel perfect graphics at very high performance. There is also Una-i's GPU
plugin from PCSX4ALL project, and traditional P.E.Op.S. one.


PCSX-Reloaded
=============

PCSX-Reloaded is a forked version of the dead PCSX emulator, with a nicer
interface and several improvements to stability and functionality.

PCSX-Reloaded uses the PSEMU plugin interface to provide most functionality;
without them, you will not be able to use it to play games. PCSX-Reloaded
provides a number of plugins to provide basic functionality out of the box.

PCSX-Reloaded has a very capable Internal HLE BIOS that can run many games
without problems. It is recommended that you use it. However, if you own a
real PlayStation, you may be able to use your own BIOS image. PCSX-Reloaded
will find it in ~/.pcsx/bios/ or /usr/share/psemu/bios/ if you place it there.
This can improve compatibility, especially with certain games and with the
use of memory cards.

See the doc/ folder in the source, or /usr/share/doc/pcsx/ on Debian systems,
for more detailed information on PCSX-Reloaded. A UNIX manpage is also
available.
