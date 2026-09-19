# Cross-compile pcsx-ab for a 32-bit Raspberry Pi OS (Raspbian) userland - the same toolchain, sysroot and
# layout as AutoBleem's toolchains/rpi/RPitoolchain.cmake, so the two projects build side by side and the
# result drops into AutoBleem's payload_rpi/Autobleem/bin/emu/. Two hosts can do it, see common.cmake: the
# Windows PC with the "SysGCC for Raspberry Pi" toolchain (PCSXAB_RPI_TOOLCHAIN), or a Debian host with
# crossbuild-essential-armhf and the multiarch libsdl2-dev/libpng-dev:armhf packages (AutoBleem's Docker
# image).
#
# Not the PlayStation Classic toolchain: that is toolchains/psc/PSCtoolchainV8.cmake.
#
#   cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=toolchains/rpi/RPitoolchain.cmake -B build_rpi
#   (or just ./make_rpi.sh)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(PCSXAB_RPI_TOOLCHAIN "C:/sysGCC/raspberry" CACHE PATH "SysGCC for Raspberry Pi install directory (Windows)")
# re-read inside try_compile, where -D variables are invisible unless listed here
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES PCSXAB_RPI_TOOLCHAIN)

include("${CMAKE_CURRENT_LIST_DIR}/common.cmake")
pcsxab_rpi_toolchain(arm-linux-gnueabihf "${PCSXAB_RPI_TOOLCHAIN}" "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Pi 2/3/4/Zero 2 on the 32-bit OS: armv7-a + NEON, hard float. The dynarec and the NEON GPU/GTE need
# ARM mode (no Thumb) - CMakeLists.txt adds -marm itself. Pi 1/Zero (armv6, no NEON) are not a target.
set(_pcsxab_rpi_arch "-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard -mtune=cortex-a7")
set(CMAKE_C_FLAGS_INIT   "${_pcsxab_rpi_arch}")
set(CMAKE_CXX_FLAGS_INIT "${_pcsxab_rpi_arch}")
set(CMAKE_ASM_FLAGS_INIT "${_pcsxab_rpi_arch}")

# No Wayland on the Pi: video goes through SDL2's own renderer on KMSDRM, so the EGL/Wayland GL output
# (frontend/libpicofe/gl_platform.c) and the gpu_gles plugin are compiled out.
set(PCSXAB_GLES OFF CACHE BOOL "" FORCE)
