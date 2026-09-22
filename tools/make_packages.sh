#!/usr/bin/env bash
# The emulator as one package per platform, from the build directories' dist/ folders, for the download
# repository (autobleem-develop's tools/repo_publish.sh pcsx-ab <version> dist/packages/*):
#
#   dist/packages/pcsx-ab-<version>-psc.tar.gz         build_psc/dist    Autobleem/bin/emu/ on the stick
#   dist/packages/pcsx-ab-<version>-rpi-armhf.tar.gz   build_rpi/dist    Autobleem/bin/emu/ on a 32-bit Pi
#   dist/packages/pcsx-ab-<version>-rpi-arm64.tar.gz   build_rpi64/dist  ... on a 64-bit Pi
#   dist/packages/pcsx-ab-<version>-pcusb.tar.gz     build_pcusb/dist  AutoBleem's 32-bit PC USB stick
#   dist/packages/pcsx-ab-<version>-win64.zip          build_win_rel (or build_win)  the exe, its DLLs, the plugins, the skin
#   dist/packages/pcsx-ab-<version>.json               what is in them: version, commit, date, sha256 per file
#
# Each Linux tarball unpacks to pcsx-ab + plugins/*.so - the layout AutoBleem's launch scripts expect - and
# replaces the same files in Autobleem/bin/emu/. A target whose dist/ is missing is skipped with a note.
# The version is <date>-<commit> (20260920-fc8c992): this repository has no tags.
#
#   tools/make_packages.sh              -> dist/packages/
#   tools/make_packages.sh --version X  a version string of your own
set -euo pipefail
cd "$(dirname "$0")/.."

VERSION=""
while [ $# -gt 0 ]; do
    case "$1" in
        --version) VERSION="$2"; shift 2 ;;
        *) echo "usage: $0 [--version X]" >&2; exit 2 ;;
    esac
done
[ -n "$VERSION" ] || VERSION="$(date -u +%Y%m%d)-$(git rev-parse --short HEAD 2>/dev/null || echo unknown)"
COMMIT="$(git rev-parse --short HEAD 2>/dev/null || echo unknown)"
DATE="$(date -u +%Y-%m-%d)"
OUT=dist/packages
rm -rf "$OUT"
mkdir -p "$OUT"

files=()
pack_linux() { # pack_linux <dist dir> <platform>
    local dist="$1" plat="$2" name="pcsx-ab-$VERSION-$2.tar.gz"
    if [ ! -f "$dist/pcsx-ab" ]; then
        echo "  $plat: no $dist/pcsx-ab, skipped" >&2
        return
    fi
    tar -C "$dist" --owner=0 --group=0 --mode='u=rwX,go=rX' -czf "$OUT/$name" pcsx-ab plugins
    files+=("$plat:$name")
    echo "  $plat: $name ($(du -h "$OUT/$name" | cut -f1))"
}
pack_windows() {
    # build_win_rel when there is one (a Release build), else the development build_win
    local name="pcsx-ab-$VERSION-win64.zip" stage src=build_win
    [ -f build_win_rel/pcsx-ab.exe ] && src=build_win_rel
    if [ ! -f "$src/pcsx-ab.exe" ]; then
        echo "  win64: no $src/pcsx-ab.exe, skipped" >&2
        return
    fi
    stage="$(mktemp -d)"
    mkdir -p "$stage/pcsx-ab/plugins" "$stage/pcsx-ab/skin"
    cp "$src/pcsx-ab.exe" "$src"/*.dll "$stage/pcsx-ab/"
    cp "$src"/plugins/*.dll "$stage/pcsx-ab/plugins/"
    cp frontend/pandora/skin/* "$stage/pcsx-ab/skin/"
    # python's zipfile: MSYS2 has no zip, the Docker image no 7z
    python3 -c "import shutil, sys; shutil.make_archive(sys.argv[1], 'zip', sys.argv[2], 'pcsx-ab')" "$OUT/${name%.zip}" "$stage"
    rm -rf "$stage"
    files+=("win64:$name")
    echo "  win64: $name ($(du -h "$OUT/$name" | cut -f1))"
}

echo "pcsx-ab $VERSION ($COMMIT, $DATE):"
pack_linux build_psc/dist psc
pack_linux build_rpi/dist rpi-armhf
pack_linux build_rpi64/dist rpi-arm64
pack_linux build_pcusb/dist pcusb
pack_windows

# the manifest
{
    printf '{\n  "name": "pcsx-ab",\n  "version": "%s",\n  "commit": "%s",\n  "date": "%s",\n' "$VERSION" "$COMMIT" "$DATE"
    printf '  "note": "the classic emulator - what AutoBleem ships as Autobleem/bin/emu",\n'
    printf '  "files": {\n'
    first=1
    for f in "${files[@]}"; do
        plat="${f%%:*}"; name="${f#*:}"
        sha="$(sha256sum "$OUT/$name" | cut -d' ' -f1)"
        size="$(stat -c %s "$OUT/$name")"
        [ $first -eq 1 ] || printf ',\n'
        first=0
        printf '    "%s": {"file": "%s", "size": %s, "sha256": "%s"}' "$plat" "$name" "$size" "$sha"
    done
    printf '\n  }\n}\n'
} > "$OUT/pcsx-ab-$VERSION.json"
echo "  manifest: pcsx-ab-$VERSION.json"
ls -l "$OUT"
