# libpng for the 64-bit Raspberry Pi cross toolchain (toolchains/rpi64/RPi64toolchain.cmake).
#
# Same situation as the 32-bit toolchains/rpi/cmake/FindPNG.cmake next to it: the sysroot has
# libpng16.so.16 but no libpng-dev. Reuses toolchains/rpi/devkit/include's headers (arch-independent) rather
# than keeping a second copy; only the library directory differs.
#
# Replaces CMake's own FindPNG (CMAKE_MODULE_PATH is searched first) and exposes the same PNG::PNG target.

get_filename_component(_pcsxab_devkit_include "${CMAKE_CURRENT_LIST_DIR}/../../rpi/devkit/include" ABSOLUTE)
set(_pcsxab_sysroot_libdir "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu")

if (NOT TARGET PNG::PNG)
    add_library(PNG::PNG UNKNOWN IMPORTED)
    set_target_properties(PNG::PNG PROPERTIES
        IMPORTED_LOCATION "${_pcsxab_sysroot_libdir}/libpng16.so.16"
        INTERFACE_INCLUDE_DIRECTORIES "${_pcsxab_devkit_include}"
    )
endif()

set(PNG_FOUND TRUE)
set(PNG_INCLUDE_DIRS "${_pcsxab_devkit_include}")
set(PNG_LIBRARIES PNG::PNG)
