# libpng for the Raspberry Pi cross toolchain (toolchains/rpi/RPitoolchain.cmake).
#
# Same situation as FindSDL2.cmake next to it: the sysroot has libpng16.so.16 but no libpng-dev. The
# headers in toolchains/rpi/devkit/include (png.h, pngconf.h, pnglibconf.h) are libpng 1.6's from the MSYS2
# package; pnglibconf.h only selects API surface, and every feature it enables is in Debian's libpng16 too.
#
# Replaces CMake's own FindPNG (CMAKE_MODULE_PATH is searched first) and exposes the same PNG::PNG target.

get_filename_component(_pcsxab_devkit_include "${CMAKE_CURRENT_LIST_DIR}/../devkit/include" ABSOLUTE)
set(_pcsxab_sysroot_libdir "${CMAKE_SYSROOT}/usr/lib/arm-linux-gnueabihf")

find_package(ZLIB REQUIRED)   # the sysroot has zlib.h and libz.so, so the stock module finds those

if (NOT TARGET PNG::PNG)
    add_library(PNG::PNG UNKNOWN IMPORTED)
    set_target_properties(PNG::PNG PROPERTIES
        IMPORTED_LOCATION "${_pcsxab_sysroot_libdir}/libpng16.so.16"
        INTERFACE_INCLUDE_DIRECTORIES "${_pcsxab_devkit_include}"
        INTERFACE_LINK_LIBRARIES ZLIB::ZLIB
    )
endif()

set(PNG_FOUND TRUE)
set(PNG_INCLUDE_DIRS "${_pcsxab_devkit_include}")
set(PNG_LIBRARIES PNG::PNG)
