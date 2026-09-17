#!/usr/bin/env bash
# Build pcsx-ab for the PlayStation Classic on the build server, which has Sony's armv8-sony-linux-gnueabihf
# toolchain (GCC 8.2.0, crosstool-NG) and the console's sysroot at /opt/toolchain - the toolchain this fork
# was originally built with. The tree is rsynced up, configured there with toolchains/psc/PSCtoolchainV8.cmake
# (PCSXAB_GLES on: EGL on Weston, gpu_gles.so), built, and pcsx-ab + plugins/*.so come back into
# build_psc/dist/ - the same layout make_rpi.sh leaves in build_rpi/dist/.
#
# Needs a "Host psc-build" entry in ~/.ssh/config (HostName, User, IdentityFile) with key login working -
# "ssh psc-build true" must not prompt - and rsync on both ends. MSYS2's ssh and Git for Windows' ssh have
# different homes (C:/msys64/home/<you> vs C:/Users/<you>), so the entry and the key go in both. The server
# side needs CMake >= 3.16, which is in ~/opt/cmake there (the distro's is 3.10).
#
#   ./make_psc.sh            full rebuild on the server
#   ./make_psc.sh -k         keep the remote build dir, rebuild what changed
#   PCSXAB_PSC_HOST=other-host ./make_psc.sh
set -e
cd "$(dirname "$0")"

HOST="${PCSXAB_PSC_HOST:-psc-build}"                       # a Host entry in ~/.ssh/config (see above)
REMOTE_DIR="${PCSXAB_PSC_DIR:-pcsx-ab}"                    # relative to the server user's home
REMOTE_CMAKE='$HOME/opt/cmake/bin/cmake'
TOOLCHAIN=/opt/toolchain                                    # PCSXAB_PSC_TOOLCHAIN on the server
JOBS="${PCSXAB_PSC_JOBS:-2}"
SSH="ssh -o BatchMode=yes $HOST"
export RSYNC_RSH="ssh -o BatchMode=yes"

echo "==> syncing to $HOST:$REMOTE_DIR"
rsync -az --delete \
    --exclude '/build_*' --exclude '/.git' --exclude '/dist' \
    --exclude '*.o' --exclude '*.so' --exclude '*.exe' --exclude '*.dll' \
    ./ "$HOST:$REMOTE_DIR/"

if [ "${1:-}" != "-k" ]; then
    $SSH "rm -rf $REMOTE_DIR/build_psc"
fi

echo "==> configuring and building on $HOST"
$SSH "cd $REMOTE_DIR && $REMOTE_CMAKE -S . -B build_psc -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=toolchains/psc/PSCtoolchainV8.cmake -DPCSXAB_PSC_TOOLCHAIN=$TOOLCHAIN \
    && $REMOTE_CMAKE --build build_psc -j $JOBS"

echo "==> fetching results (stripped, as build_pcsx.sh did; the unstripped ones stay on the server for gdb)"
rm -rf build_psc/dist
mkdir -p build_psc/dist/plugins
$SSH "cd $REMOTE_DIR/build_psc && rm -rf dist && mkdir -p dist/plugins \
    && $TOOLCHAIN/bin/armv8-sony-linux-gnueabihf-strip -o dist/pcsx-ab pcsx-ab \
    && for so in plugins/*.so; do $TOOLCHAIN/bin/armv8-sony-linux-gnueabihf-strip -o dist/\$so \$so; done"
# tar rather than rsync for the way back: rsync insists on POSIX modes, which NTFS under MSYS2 refuses
$SSH "cd $REMOTE_DIR/build_psc/dist && tar czf - ." | tar xzf - --no-same-permissions -C build_psc/dist
echo "==> build_psc/dist:"
ls -l build_psc/dist build_psc/dist/plugins
