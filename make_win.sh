#!/usr/bin/env bash
# Windows development build with MSYS2 UCRT64 (gcc, cmake, ninja, and the mingw-w64-ucrt-x86_64-{SDL2,libpng}
# packages), the same way AutoBleem's make_win.sh works. This is for running and debugging the frontend on
# the PC - the SDL2 renderer video path, the menu, input, save states - not for playing: x86 has no dynarec,
# so it is the interpreter and the peops GPU (CMakeLists.txt's PCSXAB_NEON/PCSXAB_DYNAREC default off).
#
# Run from an MSYS2 UCRT64 shell, or:
#   C:\msys64\usr\bin\bash.exe -lc "cd /e/Programming/pcsx-rearmed-develop && ./make_win.sh"
set -e
cd "$(dirname "$0")"
mkdir -p build_win
cmake -G Ninja -S . -B build_win -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build_win

# the MSYS2 runtime DLLs next to the exe, so it also starts outside an MSYS2 shell (Explorer, a debugger)
for dll in $(ldd build_win/pcsx-ab.exe | awk '/ucrt64/ {print $3}'); do
    cp -u "$dll" build_win/
done
echo "==> build_win/pcsx-ab.exe"
