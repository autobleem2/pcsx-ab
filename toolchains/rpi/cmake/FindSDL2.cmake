# SDL2 for the Raspberry Pi cross toolchain (toolchains/rpi/RPitoolchain.cmake).
#
# The sysGCC Pi sysroot was rsynced from an installed Pi: it has the libSDL2-2.0.so.0 runtime but no
# libsdl2-dev (no headers, no unversioned .so symlink, no sdl2-config.cmake). This module stands in for the
# one libsdl2-dev would provide: headers from toolchains/rpi/devkit/include/SDL2 (SDL 2.32's public headers
# plus our own Linux SDL_config.h - see the comment in that file), library pointed straight at the versioned
# .so in the sysroot. Same idea as AutoBleem's toolchains/rpi/cmake/FindSDL2.cmake.
#
# Exposes what the real config exports and CMakeLists.txt consumes: the SDL2::SDL2 target and
# SDL2_INCLUDE_DIRS / SDL2_LIBRARIES.

get_filename_component(_pcsxab_devkit_include "${CMAKE_CURRENT_LIST_DIR}/../devkit/include" ABSOLUTE)
set(_pcsxab_sysroot_libdir "${CMAKE_SYSROOT}/usr/lib/arm-linux-gnueabihf")

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
