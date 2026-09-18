#!/usr/bin/env bash
# Cross-compile pcsx-ab for a 64-bit Raspberry Pi OS (Trixie) userland, using the Windows-hosted "SysGCC for
# Raspberry Pi (64-bit)" toolchain at C:\sysGCC\raspberry64 - the same one AutoBleem's make_rpi64.sh uses, so
# the two build side by side. The result goes into build_rpi64/ (pcsx-ab + plugins/*.so), and
# build_rpi64/dist/ holds just those two, laid out the way AutoBleem's Autobleem/bin/emu-arm64/ wants them.
#
# No dynarec, no NEON GPU/GTE on this target: CMAKE_SYSTEM_PROCESSOR=aarch64 does not match CMakeLists.txt's
# _pcsxab_is_arm regex on purpose (this fork's dynarec/NEON assembly is 32-bit ARM only), so it builds like
# the PC build does - C interpreter, software (peops) GPU - just cross-compiled instead of native.
#
# Run it from the MSYS2 UCRT64 shell, like make_rpi.sh. The toolchain file names the compilers by absolute
# path, so C:\sysGCC\raspberry64\bin deliberately does NOT go on PATH: its rm.exe/mkdir.exe/make.exe would
# shadow the MSYS2 ones and break this script.
#
#   ./make_rpi64.sh            full rebuild
#   ./make_rpi64.sh -k         keep build_rpi64/ and just rebuild what changed
#   AUTOBLEEM_DIR=../autobleem-develop ./make_rpi64.sh
#                              ...and copy the result into that checkout's payload_rpi/Autobleem/bin/emu-arm64/
set -e
cd "$(dirname "$0")"

if [ "${1:-}" != "-k" ]; then
    rm -rf ./build_rpi64
fi
mkdir -p build_rpi64
cmake -G Ninja -S . -B build_rpi64 -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=toolchains/rpi64/RPi64toolchain.cmake
cmake --build build_rpi64

# a flat copy of what ships: the executable and the plugin .so's, nothing else from the build tree.
# Stripped, as make_rpi.sh does for the 32-bit Pi; the unstripped ones stay in build_rpi64/ for gdb.
STRIP="E:/sysGCC/raspberry64/bin/aarch64-linux-gnu-strip.exe"
rm -rf build_rpi64/dist
mkdir -p build_rpi64/dist/plugins
"$STRIP" -o build_rpi64/dist/pcsx-ab build_rpi64/pcsx-ab
for so in build_rpi64/plugins/*.so; do
    "$STRIP" -o "build_rpi64/dist/plugins/$(basename "$so")" "$so"
done
echo "==> build_rpi64/dist:"
ls -l build_rpi64/dist build_rpi64/dist/plugins

if [ -n "${AUTOBLEEM_DIR:-}" ]; then
    emu="$AUTOBLEEM_DIR/payload_rpi/Autobleem/bin/emu-arm64"
    [ -d "$emu" ] || { echo "no $emu - is AUTOBLEEM_DIR an autobleem checkout with the 64-bit Pi port?" >&2; exit 1; }
    rm -rf "$emu/plugins"
    cp -a build_rpi64/dist/. "$emu/"
    echo "==> copied into $emu (AutoBleem's tools/make_rpi_package.sh --arch arm64 will pick it up)"
fi
