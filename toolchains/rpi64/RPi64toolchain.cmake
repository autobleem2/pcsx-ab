# Cross-compile pcsx-ab for a 64-bit Raspberry Pi OS (Trixie) userland from a Windows host, using the
# "SysGCC for Raspberry Pi (64-bit)" toolchain at C:\sysGCC\raspberry64 - the same toolchain, sysroot and
# layout as AutoBleem's toolchains/rpi64/RPi64toolchain.cmake, so the two projects build side by side and the
# result drops into AutoBleem's payload_rpi/Autobleem/bin/emu-arm64/.
#
# Not the 32-bit Pi toolchain: that is toolchains/rpi/RPitoolchain.cmake. Not the PlayStation Classic
# toolchain: that is toolchains/psc/PSCtoolchainV8.cmake.
#
#   cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=toolchains/rpi64/RPi64toolchain.cmake -B build_rpi64
#   (or just ./make_rpi64.sh)
#
# CMAKE_SYSTEM_PROCESSOR=aarch64 does not match CMakeLists.txt's `_pcsxab_is_arm` regex ("^(arm|ARM)") on
# purpose: Ari64's dynarec and the NEON GPU/GTE code are 32-bit ARM assembly with no aarch64 backend in this
# fork, so this target builds the same way the PC build does - C interpreter, PCSXAB_BUILTIN_GPU=peops -
# just cross-compiled for a 64-bit Pi instead of natively for Windows. See AutoBleem's CLAUDE.md, "Raspberry
# Pi 64-bit", for the reasoning; do not extend _pcsxab_is_arm's regex to cover aarch64.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(_pcsxab_rpi64_root "C:/sysGCC/raspberry64" CACHE PATH "SysGCC for Raspberry Pi (64-bit) install directory")
set(_pcsxab_rpi64_sysroot "${_pcsxab_rpi64_root}/aarch64-linux-gnu/sysroot")

set(CMAKE_C_COMPILER   "${_pcsxab_rpi64_root}/bin/aarch64-linux-gnu-gcc.exe")
set(CMAKE_CXX_COMPILER "${_pcsxab_rpi64_root}/bin/aarch64-linux-gnu-g++.exe")
set(CMAKE_ASM_COMPILER "${_pcsxab_rpi64_root}/bin/aarch64-linux-gnu-gcc.exe")

set(CMAKE_SYSROOT "${_pcsxab_rpi64_sysroot}")
set(CMAKE_FIND_ROOT_PATH "${_pcsxab_rpi64_sysroot}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Every 64-bit-capable Pi (3/4/5/400/Zero 2 W) is armv8-a - no board split like the 32-bit toolchain's
# armv7-a-vs-armv6 concern.
set(_pcsxab_rpi64_arch "-march=armv8-a")
set(CMAKE_C_FLAGS_INIT   "${_pcsxab_rpi64_arch}")
set(CMAKE_CXX_FLAGS_INIT "${_pcsxab_rpi64_arch}")
set(CMAKE_ASM_FLAGS_INIT "${_pcsxab_rpi64_arch}")

# the sysroot's runtime libs are in the Debian multiarch dir, which the cross gcc does not search by default
set(CMAKE_EXE_LINKER_FLAGS_INIT    "-L${_pcsxab_rpi64_sysroot}/usr/lib/aarch64-linux-gnu")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-L${_pcsxab_rpi64_sysroot}/usr/lib/aarch64-linux-gnu")

# our FindSDL2 / FindPNG, which paper over the sysroot having runtime libs but no -dev packages
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# No Wayland on the Pi: video goes through SDL2's own renderer on KMSDRM, same as the 32-bit Pi target.
set(PCSXAB_GLES OFF CACHE BOOL "" FORCE)
