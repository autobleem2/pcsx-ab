#!/usr/bin/env bash
# Cross-compile pcsx-ab for a 32-bit Raspberry Pi OS userland, using the Windows-hosted "SysGCC for
# Raspberry Pi" toolchain at C:\sysGCC\raspberry - the same one AutoBleem's make_rpi.sh uses, so the two
# build side by side. The result goes into build_rpi/ (pcsx-ab + plugins/*.so), and build_rpi/dist/ holds
# just those two, laid out the way AutoBleem's Autobleem/bin/emu/ wants them.
#
# Run it from the MSYS2 UCRT64 shell, like AutoBleem's make_win.sh / make_rpi.sh. The toolchain file names
# the compilers by absolute path, so C:\sysGCC\raspberry\bin deliberately does NOT go on PATH: its
# rm.exe/mkdir.exe/make.exe would shadow the MSYS2 ones and break this script.
#
#   ./make_rpi.sh            full rebuild
#   ./make_rpi.sh -k         keep build_rpi/ and just rebuild what changed
#   AUTOBLEEM_DIR=../autobleem-develop ./make_rpi.sh
#                            ...and copy the result into that checkout's payload_rpi/Autobleem/bin/emu/
set -e
cd "$(dirname "$0")"

if [ "${1:-}" != "-k" ]; then
    rm -rf ./build_rpi
fi
mkdir -p build_rpi
cmake -G Ninja -S . -B build_rpi -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=toolchains/rpi/RPitoolchain.cmake
cmake --build build_rpi

# a flat copy of what ships: the executable and the plugin .so's, nothing else from the build tree.
# Stripped, as build_pcsx.sh did for the console; the unstripped ones stay in build_rpi/ for gdb.
STRIP="C:/sysGCC/raspberry/bin/arm-linux-gnueabihf-strip.exe"
rm -rf build_rpi/dist
mkdir -p build_rpi/dist/plugins
"$STRIP" -o build_rpi/dist/pcsx-ab build_rpi/pcsx-ab
for so in build_rpi/plugins/*.so; do
    "$STRIP" -o "build_rpi/dist/plugins/$(basename "$so")" "$so"
done
echo "==> build_rpi/dist:"
ls -l build_rpi/dist build_rpi/dist/plugins

if [ -n "${AUTOBLEEM_DIR:-}" ]; then
    emu="$AUTOBLEEM_DIR/payload_rpi/Autobleem/bin/emu"
    [ -d "$emu" ] || { echo "no $emu - is AUTOBLEEM_DIR an autobleem checkout?" >&2; exit 1; }
    rm -rf "$emu/plugins"
    cp -a build_rpi/dist/. "$emu/"
    echo "==> copied into $emu (AutoBleem's tools/make_rpi_package.sh will pick it up)"
fi
