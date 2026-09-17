PCSX-ReARMed - yet another PCSX fork
====================================

**THIS IS A MODIFIED VERSION OF PCSX SONY'S VERSION MADE BY THE AUTOBLEEM TEAM**

- screemer
- nex
- mGGk
- ThaFridge

*see [readme.txt](readme.txt) for more complete documentation*

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

**PlayStation Classic** - Sony's toolchain, `PCSXAB_GLES=ON` (the old `config.mak.autobleem`):

```bash
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=toolchains/psc/PSCtoolchainV8.cmake -B build_psc
cmake --build build_psc
```

**Natively on a Pi / any ARM Linux** with `libsdl2-dev libpng-dev zlib1g-dev` installed:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
```

Options (`cmake -L`): `PCSXAB_ENABLE_MENU`, `PCSXAB_NEON`, `PCSXAB_DYNAREC`, `PCSXAB_BUILTIN_GPU` (neon/peops/unai),
`PCSXAB_SOUND_DRIVERS` (sdl alsa oss pulseaudio), `PCSXAB_GLES`, `PCSXAB_PLUGINS`, `PCSXAB_LINK_MAP`.

**Raspberry Pi status:** the cross build compiles and links against exactly the libraries a stock Pi OS has
(`libSDL2-2.0.so.0`, `libpng16`, `libz`). It has not yet been run on a Pi.

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
