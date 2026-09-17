# Cross-compile pcsx-ab for the PlayStation Classic with Sony's armv8-sony-linux-gnueabihf toolchain.
# This is the CMake form of the old config.mak.autobleem: armv8-a, NEON, hard float, SDL2 + libpng + Wayland +
# EGL/GLESv1 from the toolchain's sysroot.
#
# The toolchain root is PCSXAB_PSC_TOOLCHAIN (default /opt/toolchain, the crosstool-NG layout on the build
# server: <root>/bin/armv8-sony-linux-gnueabihf-gcc, <root>/armv8-sony-linux-gnueabihf/sysroot). The sysroot
# has the SDL2, libpng, Wayland and GLES dev files the console build needs, so no devkit is required here.
# ./make_psc.sh runs this on the server over ssh; it is not installed on the Windows host.
#
#   cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=toolchains/psc/PSCtoolchainV8.cmake -B build_psc

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(PCSXAB_PSC_TOOLCHAIN "/opt/toolchain" CACHE PATH "Sony PSC toolchain root")
# CMake re-reads this file inside its try_compile sandboxes, where -D variables are invisible unless listed
# here - without this the compiler probe silently used the default root
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES PCSXAB_PSC_TOOLCHAIN)
if (EXISTS "${PCSXAB_PSC_TOOLCHAIN}/armv8-sony-linux-gnueabihf/sysroot")
    set(_pcsxab_psc_sysroot "${PCSXAB_PSC_TOOLCHAIN}/armv8-sony-linux-gnueabihf/sysroot")
else()
    set(_pcsxab_psc_sysroot "${PCSXAB_PSC_TOOLCHAIN}/sysroot")
endif()

if (EXISTS "${PCSXAB_PSC_TOOLCHAIN}/bin")
    set(CMAKE_C_COMPILER   "${PCSXAB_PSC_TOOLCHAIN}/bin/armv8-sony-linux-gnueabihf-gcc")
    set(CMAKE_CXX_COMPILER "${PCSXAB_PSC_TOOLCHAIN}/bin/armv8-sony-linux-gnueabihf-g++")
    set(CMAKE_ASM_COMPILER "${PCSXAB_PSC_TOOLCHAIN}/bin/armv8-sony-linux-gnueabihf-gcc")
    set(CMAKE_SYSROOT "${_pcsxab_psc_sysroot}")
    set(CMAKE_FIND_ROOT_PATH "${_pcsxab_psc_sysroot}")
else()
    # toolchain on PATH (the way config.mak.autobleem assumed it)
    set(CMAKE_C_COMPILER   armv8-sony-linux-gnueabihf-gcc)
    set(CMAKE_CXX_COMPILER armv8-sony-linux-gnueabihf-g++)
    set(CMAKE_ASM_COMPILER armv8-sony-linux-gnueabihf-gcc)
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# exactly what config.mak.autobleem passed
set(_pcsxab_psc_arch "-march=armv8-a -mfloat-abi=hard -mfpu=neon-vfpv4")
set(CMAKE_C_FLAGS_INIT   "${_pcsxab_psc_arch}")
set(CMAKE_CXX_FLAGS_INIT "${_pcsxab_psc_arch}")
set(CMAKE_ASM_FLAGS_INIT "${_pcsxab_psc_arch}")

# our FindSDL2: the sysroot's SDL2 2.0.4 predates sdl2-config.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# The console runs Weston: GL output goes EGL-on-Wayland through SDL2's window (frontend/libpicofe/gl_platform.c).
set(PCSXAB_GLES ON CACHE BOOL "" FORCE)
