# Cross-compile pcsx-ab for a 64-bit Raspberry Pi OS (Trixie) userland - the same toolchain, sysroot and
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

# Two hosts can do it, see ../rpi/common.cmake: the Windows PC with the "SysGCC for Raspberry Pi (64-bit)"
# toolchain (PCSXAB_RPI64_TOOLCHAIN), or a Debian host with crossbuild-essential-arm64 and the multiarch
# libsdl2-dev/libpng-dev:arm64 packages (AutoBleem's Docker image).
set(PCSXAB_RPI64_TOOLCHAIN "E:/sysGCC/raspberry64" CACHE PATH "SysGCC for Raspberry Pi (64-bit) install directory (Windows)")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES PCSXAB_RPI64_TOOLCHAIN)

include("${CMAKE_CURRENT_LIST_DIR}/../rpi/common.cmake")
pcsxab_rpi_toolchain(aarch64-linux-gnu "${PCSXAB_RPI64_TOOLCHAIN}" "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Every 64-bit-capable Pi (3/4/5/400/Zero 2 W) is armv8-a - no board split like the 32-bit toolchain's
# armv7-a-vs-armv6 concern.
set(_pcsxab_rpi64_arch "-march=armv8-a")
set(CMAKE_C_FLAGS_INIT   "${_pcsxab_rpi64_arch}")
set(CMAKE_CXX_FLAGS_INIT "${_pcsxab_rpi64_arch}")
set(CMAKE_ASM_FLAGS_INIT "${_pcsxab_rpi64_arch}")

# No Wayland on the Pi: video goes through SDL2's own renderer on KMSDRM, same as the 32-bit Pi target.
set(PCSXAB_GLES OFF CACHE BOOL "" FORCE)
