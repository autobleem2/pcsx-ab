# SDL2 for the 64-bit Raspberry Pi cross toolchain (toolchains/rpi64/RPi64toolchain.cmake).
#
# Same situation as the 32-bit toolchains/rpi/cmake/FindSDL2.cmake: the sysGCC sysroot has the libSDL2-2.0.so.0
# runtime but no libsdl2-dev. Reuses toolchains/rpi/devkit/include by relative path rather than keeping a
# second copy of the headers - they are pure C and arch-independent. Only the library directory (the
# sysroot's aarch64 multiarch path) differs from the 32-bit module.
#
# Exposes what the real config exports and CMakeLists.txt consumes: the SDL2::SDL2 target and
# SDL2_INCLUDE_DIRS / SDL2_LIBRARIES.

get_filename_component(_pcsxab_devkit_include "${CMAKE_CURRENT_LIST_DIR}/../../rpi/devkit/include" ABSOLUTE)
set(_pcsxab_sysroot_libdir "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu")

if (NOT TARGET SDL2::SDL2)
    add_library(SDL2::SDL2 UNKNOWN IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        IMPORTED_LOCATION "${_pcsxab_sysroot_libdir}/libSDL2-2.0.so.0"
        # both <SDL.h> and <SDL2/SDL.h> are used in the tree
        INTERFACE_INCLUDE_DIRECTORIES "${_pcsxab_devkit_include};${_pcsxab_devkit_include}/SDL2"
    )
endif()

set(SDL2_FOUND TRUE)
set(SDL2_INCLUDE_DIRS "${_pcsxab_devkit_include};${_pcsxab_devkit_include}/SDL2")
set(SDL2_LIBRARIES SDL2::SDL2)
