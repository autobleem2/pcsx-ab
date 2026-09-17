# SDL2 for the PlayStation Classic toolchain (toolchains/psc/PSCtoolchainV8.cmake).
#
# The console's sysroot has SDL2 2.0.4 - headers in usr/include/SDL2 and libSDL2.so - but no
# sdl2-config.cmake (SDL only started shipping one with 2.0.12), so the stock find_package(SDL2) finds
# nothing. This module exposes what CMakeLists.txt consumes: the SDL2::SDL2 target and the variables.
#
# The sysroot's SDL_config.h also declares SDL_VIDEO_DRIVER_X11 although the sysroot has no X11 headers,
# which breaks SDL_syswm.h - and worse, SDL_VIDEO_DRIVER_X11 selects the X11 branch in
# frontend/libpicofe/plat_sdl.c and compiles the Wayland handoff the console needs out. The original build
# clearly compiled without it, so: the SDL2 headers are copied into the build tree at configure time with
# that one define neutralised, and those are what the build sees. SDL.h includes "SDL_config.h" relative
# to itself, which is why the whole directory is copied rather than just the one file.

set(_pcsxab_psc_sysroot_inc "${CMAKE_SYSROOT}/usr/include/SDL2")
set(_pcsxab_psc_lib "${CMAKE_SYSROOT}/usr/lib/libSDL2.so")
set(_pcsxab_psc_inc "${CMAKE_BINARY_DIR}/psc-sdl2-include")

if (NOT EXISTS "${_pcsxab_psc_sysroot_inc}/SDL.h" OR NOT EXISTS "${_pcsxab_psc_lib}")
    message(FATAL_ERROR "SDL2 not found in the PSC sysroot ${CMAKE_SYSROOT}")
endif()

file(MAKE_DIRECTORY "${_pcsxab_psc_inc}/SDL2")
file(GLOB _pcsxab_psc_headers "${_pcsxab_psc_sysroot_inc}/*.h")
foreach(_h ${_pcsxab_psc_headers})
    get_filename_component(_name "${_h}" NAME)
    if (_name STREQUAL "SDL_config.h")
        file(READ "${_h}" _cfg)
        string(REGEX REPLACE "#define SDL_VIDEO_DRIVER_X11 1" "/* #undef SDL_VIDEO_DRIVER_X11 (pcsx-ab: no X11 on the console) */" _cfg "${_cfg}")
        file(WRITE "${_pcsxab_psc_inc}/SDL2/${_name}" "${_cfg}")
    else()
        configure_file("${_h}" "${_pcsxab_psc_inc}/SDL2/${_name}" COPYONLY)
    endif()
endforeach()

if (NOT TARGET SDL2::SDL2)
    add_library(SDL2::SDL2 UNKNOWN IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        IMPORTED_LOCATION "${_pcsxab_psc_lib}"
        # both <SDL.h> and <SDL2/SDL.h> are used in the tree
        INTERFACE_INCLUDE_DIRECTORIES "${_pcsxab_psc_inc};${_pcsxab_psc_inc}/SDL2"
    )
endif()

set(SDL2_FOUND TRUE)
set(SDL2_INCLUDE_DIRS "${_pcsxab_psc_inc};${_pcsxab_psc_inc}/SDL2")
set(SDL2_LIBRARIES SDL2::SDL2)
