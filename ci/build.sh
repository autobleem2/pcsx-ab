#!/usr/bin/env bash
# ci/build.sh TARGET... - build pcsx-ab for a target on a Linux host with the toolchains AutoBleem's Docker
# image carries (autobleem/docker/), and leave the stripped emulator in build_<target>/dist/ the way
# make_psc.sh / make_rpi.sh / make_rpi64.sh do: pcsx-ab + plugins/*.so, laid out as AutoBleem's
# Autobleem/bin/emu/ wants them. AutoBleem's ci/build.sh runs this ahead of its own build and copies the
# result into its payload, so every package ships an emulator built by the same image.
#
#   psc      build_psc/     the PlayStation Classic (toolchains/psc, PCSXAB_PSC_TOOLCHAIN - /opt/psc in the image)
#   rpi      build_rpi/     Raspberry Pi 32-bit (toolchains/rpi, Debian's arm-linux-gnueabihf)
#   rpi64    build_rpi64/   Raspberry Pi 64-bit (toolchains/rpi64, Debian's aarch64-linux-gnu)
#   all      the three
#
#   AB_JOBS=N      parallel jobs (default: nproc);  AB_CLEAN=1  wipe the build dir first
set -euo pipefail
cd "$(dirname "$0")/.."
REPO="$PWD"
JOBS="${AB_JOBS:-$(nproc)}"

configure() { # configure BUILD_DIR ARGS...
    local dir="$1"; shift
    [ -n "${AB_CLEAN:-}" ] && rm -rf "$dir"
    if [ -f "$dir/CMakeCache.txt" ]; then
        local cached gen
        cached="$(sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' "$dir/CMakeCache.txt" | tail -1)"
        gen="$(sed -n 's/^CMAKE_GENERATOR:INTERNAL=//p' "$dir/CMakeCache.txt" | tail -1)"
        if [ "$cached" != "$REPO" ] || [ "$gen" != "Ninja" ]; then
            echo "    $dir was configured for $cached with $gen - starting it over"
            rm -rf "$dir"
        fi
    fi
    cmake -S . -B "$dir" -G Ninja -DCMAKE_BUILD_TYPE=Release "${LAUNCHER[@]}" "$@"
}
# sccache in front of the compiler when it is there (AutoBleem's build image has it and mounts the cache
# from the host; AutoBleem's ci/build.sh does the same). AB_NO_SCCACHE=1 builds without.
LAUNCHER=()
if [ -z "${AB_NO_SCCACHE:-}" ] && command -v sccache >/dev/null 2>&1; then
    LAUNCHER=(-DCMAKE_C_COMPILER_LAUNCHER=sccache -DCMAKE_CXX_COMPILER_LAUNCHER=sccache)
fi

dist() { # dist BUILD_DIR STRIP - the stripped emulator and plugins
    local dir="$1" strip="$2" so
    rm -rf "$dir/dist"
    mkdir -p "$dir/dist/plugins"
    "$strip" -o "$dir/dist/pcsx-ab" "$dir/pcsx-ab"
    for so in "$dir"/plugins/*.so; do "$strip" -o "$dir/dist/plugins/$(basename "$so")" "$so"; done
    echo "==> $dir/dist:"
    ls -l "$dir/dist" "$dir/dist/plugins" | sed 's/^/    /'
    file "$dir/dist/pcsx-ab" | sed 's/^/    /'
}

build_psc() {
    local toolchain="${PCSXAB_PSC_TOOLCHAIN:-${AB_PSC_TOOLCHAIN:-/opt/psc}}"
    echo "==> pcsx-ab psc: configure + build (build_psc, toolchain $toolchain)"
    configure build_psc -DCMAKE_TOOLCHAIN_FILE=toolchains/psc/PSCtoolchainV8.cmake -DPCSXAB_PSC_TOOLCHAIN="$toolchain"
    ninja -C build_psc -j "$JOBS"
    dist build_psc "$toolchain/bin/armv8-sony-linux-gnueabihf-strip"
    file build_psc/dist/pcsx-ab | grep -q 'ELF 32-bit LSB.*ARM, EABI5'
    # the console's glibc 2.24 / libstdc++ 6.0.22, no RPATH - the same gate AutoBleem's binaries pass
    local readelf="$toolchain/bin/armv8-sony-linux-gnueabihf-readelf" bin
    for bin in build_psc/dist/pcsx-ab build_psc/dist/plugins/*.so; do
        local glibc
        glibc="$("$readelf" -V "$bin" | grep -o 'GLIBC_[0-9.]*' | sed 's/GLIBC_//' | sort -V | tail -1)"
        if [ -n "$glibc" ] && [ "$(printf '%s\n2.24\n' "$glibc" | sort -V | tail -1)" != "2.24" ]; then
            echo "    $bin needs GLIBC_$glibc, newer than the console's 2.24" >&2; exit 1
        fi
        if "$readelf" -d "$bin" | grep -qE 'RPATH|RUNPATH'; then echo "    $bin carries an RPATH" >&2; exit 1; fi
        echo "    $bin: GLIBC ${glibc:-none}, ok"
    done
}

build_rpi() { # build_rpi armhf|arm64
    local arch="$1" dir toolchain proc triplet
    case "$arch" in
        armhf) dir=build_rpi;   toolchain=toolchains/rpi/RPitoolchain.cmake;     proc=arm;     triplet=arm-linux-gnueabihf ;;
        arm64) dir=build_rpi64; toolchain=toolchains/rpi64/RPi64toolchain.cmake; proc=aarch64; triplet=aarch64-linux-gnu ;;
    esac
    echo "==> pcsx-ab rpi $arch: configure + build ($dir)"
    configure "$dir" -DCMAKE_SYSTEM_PROCESSOR="$proc" -DCMAKE_TOOLCHAIN_FILE="$toolchain"
    ninja -C "$dir" -j "$JOBS"
    dist "$dir" "$triplet-strip"
    case "$arch" in
        armhf) file "$dir/dist/pcsx-ab" | grep -q 'ELF 32-bit LSB.*ARM, EABI5' ;;
        arm64) file "$dir/dist/pcsx-ab" | grep -q 'ELF 64-bit LSB.*ARM aarch64' ;;
    esac
}

[ $# -gt 0 ] || { sed -n '2,14p' "$0"; exit 2; }
targets=()
for t in "$@"; do
    case "$t" in
        all) targets+=(psc rpi rpi64) ;;
        psc|rpi|rpi64) targets+=("$t") ;;
        *) echo "unknown target: $t (psc, rpi, rpi64, all)" >&2; exit 2 ;;
    esac
done
for t in "${targets[@]}"; do
    case "$t" in
        psc)   build_psc ;;
        rpi)   build_rpi armhf ;;
        rpi64) build_rpi arm64 ;;
    esac
done
