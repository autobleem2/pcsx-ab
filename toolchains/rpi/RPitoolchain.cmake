# Cross-compile pcsx-ab for a 32-bit Raspberry Pi OS (Raspbian) userland from a Windows host, using the
# "SysGCC for Raspberry Pi" toolchain at C:\sysGCC\raspberry - the same toolchain, sysroot and layout as
# AutoBleem's toolchains/rpi/RPitoolchain.cmake, so the two projects build side by side and the result drops
# into AutoBleem's payload_rpi/Autobleem/bin/emu/.
#
# Not the PlayStation Classic toolchain: that is toolchains/psc/PSCtoolchainV8.cmake.
#
#   cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=toolchains/rpi/RPitoolchain.cmake -B build_rpi
#   (or just ./make_rpi.sh)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(_pcsxab_rpi_root "C:/sysGCC/raspberry" CACHE PATH "SysGCC for Raspberry Pi install directory")
set(_pcsxab_rpi_sysroot "${_pcsxab_rpi_root}/arm-linux-gnueabihf/sysroot")

set(CMAKE_C_COMPILER   "${_pcsxab_rpi_root}/bin/arm-linux-gnueabihf-gcc.exe")
set(CMAKE_CXX_COMPILER "${_pcsxab_rpi_root}/bin/arm-linux-gnueabihf-g++.exe")
set(CMAKE_ASM_COMPILER "${_pcsxab_rpi_root}/bin/arm-linux-gnueabihf-gcc.exe")

set(CMAKE_SYSROOT "${_pcsxab_rpi_sysroot}")
set(CMAKE_FIND_ROOT_PATH "${_pcsxab_rpi_sysroot}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Pi 2/3/4/Zero 2 on the 32-bit OS: armv7-a + NEON, hard float. The dynarec and the NEON GPU/GTE need
# ARM mode (no Thumb) - CMakeLists.txt adds -marm itself. Pi 1/Zero (armv6, no NEON) are not a target.
set(_pcsxab_rpi_arch "-march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard -mtune=cortex-a7")
set(CMAKE_C_FLAGS_INIT   "${_pcsxab_rpi_arch}")
set(CMAKE_CXX_FLAGS_INIT "${_pcsxab_rpi_arch}")
set(CMAKE_ASM_FLAGS_INIT "${_pcsxab_rpi_arch}")

# the sysroot's runtime libs are in the Debian multiarch dir, which the cross gcc does not search by default
set(CMAKE_EXE_LINKER_FLAGS_INIT    "-L${_pcsxab_rpi_sysroot}/usr/lib/arm-linux-gnueabihf")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-L${_pcsxab_rpi_sysroot}/usr/lib/arm-linux-gnueabihf")

# our FindSDL2 / FindPNG, which paper over the sysroot having runtime libs but no -dev packages
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# No Wayland on the Pi: video goes through SDL2's own renderer on KMSDRM, so the EGL/Wayland GL output
# (frontend/libpicofe/gl_platform.c) and the gpu_gles plugin are compiled out.
set(PCSXAB_GLES OFF CACHE BOOL "" FORCE)
