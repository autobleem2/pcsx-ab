# pcsx-ab: the AutoBleem build of PCSX-ReARMed.
#
# This replaced the upstream ./configure + Makefile + per-plugin Makefiles (removed 2026-09), for the one
# platform AutoBleem ships ("generic": SDL2 frontend, built-in NEON GPU, ARM dynarec). Everything configure
# used to autodetect is an explicit option below, set by the toolchain file for the target in question:
#
#   toolchains/rpi/RPitoolchain.cmake   Raspberry Pi (32-bit OS), cross from Windows   -> ./make_rpi.sh
#   toolchains/psc/PSCtoolchainV8.cmake PlayStation Classic, Sony's armv8 toolchain
#   (none)                              native build on the Pi/any ARM Linux with the -dev packages installed
#   (none, MSYS2 UCRT64)                Windows development build, interpreter + peops GPU  -> ./make_win.sh
#
# Targets: the pcsx-ab executable (in the build dir root) and the loadable GPU/SPU plugins (in plugins/),
# which is the layout AutoBleem's launch scripts expect under Autobleem/bin/emu/.

cmake_minimum_required(VERSION 3.16)
project(pcsx-ab LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS ON)          # gnu99: the dynarec and libpicofe use GNU extensions freely

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
# The Makefile built with -O2; CMake's Release default is -O3, which the dynarec has never been tested at.
# (gpu_unai asks for -O3 itself, below, as its Makefile did.)
foreach(_lang C CXX)
    if (CMAKE_${_lang}_FLAGS_RELEASE STREQUAL "-O3 -DNDEBUG")
        set(CMAKE_${_lang}_FLAGS_RELEASE "-O2 -DNDEBUG")
    endif()
endforeach()

# ---------------------------------------------------------------------------------------------------------
# options - the knobs configure used to guess
# ---------------------------------------------------------------------------------------------------------
if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|ARM)")
    set(_pcsxab_is_arm ON)
else()
    set(_pcsxab_is_arm OFF)
endif()

option(PCSXAB_ENABLE_MENU "Compile in the in-game menu (Select+Triangle)" ON)
option(PCSXAB_NEON        "ARM NEON code paths (GTE, GPU, scalers). ARMv7+ only" ${_pcsxab_is_arm})
option(PCSXAB_DYNAREC     "Ari64's MIPS->ARM recompiler. ARM only, and the only fast way to run anything" ${_pcsxab_is_arm})
option(PCSXAB_PLUGINS     "Also build the loadable plugins (gpu_peops, gpu_unai, spunull [, gpu_gles])" ON)
option(PCSXAB_GLES        "OpenGL ES output through EGL on a Wayland surface (the PlayStation Classic's Weston). Off means video goes through an SDL2 renderer + streaming texture - what a Raspberry Pi on KMSDRM uses" OFF)
option(PCSXAB_LINK_MAP    "Write pcsx-ab.map next to the executable (the Makefile always did)" ON)
set(PCSXAB_BUILTIN_GPU "" CACHE STRING "GPU compiled into the executable: neon, peops or unai (empty = neon with NEON, else peops)")
set_property(CACHE PCSXAB_BUILTIN_GPU PROPERTY STRINGS "" neon peops unai)
set(PCSXAB_SOUND_DRIVERS "sdl" CACHE STRING "Space-separated SPU output drivers: sdl alsa oss pulseaudio")

if (PCSXAB_BUILTIN_GPU STREQUAL "")
    if (PCSXAB_NEON)
        set(PCSXAB_BUILTIN_GPU neon)
    else()
        set(PCSXAB_BUILTIN_GPU peops)
    endif()
endif()
if (NOT _pcsxab_is_arm AND (PCSXAB_NEON OR PCSXAB_DYNAREC))
    message(FATAL_ERROR "PCSXAB_NEON / PCSXAB_DYNAREC are ARM-only (CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR})")
endif()
if (PCSXAB_BUILTIN_GPU STREQUAL "neon" AND NOT PCSXAB_NEON)
    message(FATAL_ERROR "PCSXAB_BUILTIN_GPU=neon needs PCSXAB_NEON")
endif()
separate_arguments(_pcsxab_sound_drivers UNIX_COMMAND "${PCSXAB_SOUND_DRIVERS}")

# ---------------------------------------------------------------------------------------------------------
# dependencies
# ---------------------------------------------------------------------------------------------------------
# Native builds: libsdl2-dev's sdl2-config.cmake, CMake's FindPNG/FindZLIB. Cross builds: the toolchain
# file's CMAKE_MODULE_PATH supplies FindSDL2/FindPNG for a sysroot without the -dev packages.
find_package(SDL2 REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

if (NOT TARGET SDL2::SDL2)
    # sdl2-config.cmake older than 2.0.12 only sets the variables
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${SDL2_LIBRARIES}")
endif()
# the tree includes both <SDL.h> and <SDL2/SDL.h>; make sure the parent of the SDL2 dir is searched too
get_target_property(_pcsxab_sdl_inc SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
set(_pcsxab_sdl_inc_both "")
foreach(_dir ${_pcsxab_sdl_inc})
    list(APPEND _pcsxab_sdl_inc_both "${_dir}")
    get_filename_component(_name "${_dir}" NAME)
    get_filename_component(_parent "${_dir}" DIRECTORY)
    if (_name STREQUAL "SDL2")
        list(APPEND _pcsxab_sdl_inc_both "${_parent}")
    else()
        list(APPEND _pcsxab_sdl_inc_both "${_dir}/SDL2")
    endif()
endforeach()
list(REMOVE_DUPLICATES _pcsxab_sdl_inc_both)

if (PCSXAB_GLES)
    find_library(PCSXAB_EGL_LIB     NAMES EGL brcmEGL)
    find_library(PCSXAB_GLES_LIB    NAMES GLESv1_CM GLES_CM brcmGLESv2)
    find_library(PCSXAB_WL_CLIENT   NAMES wayland-client)
    find_library(PCSXAB_WL_EGL      NAMES wayland-egl)
    foreach(_v PCSXAB_EGL_LIB PCSXAB_GLES_LIB PCSXAB_WL_CLIENT PCSXAB_WL_EGL)
        if (NOT ${_v})
            message(FATAL_ERROR "PCSXAB_GLES needs EGL, GLESv1_CM, wayland-client and wayland-egl; ${_v} not found")
        endif()
    endforeach()
    set(_pcsxab_gles_libs ${PCSXAB_EGL_LIB} ${PCSXAB_GLES_LIB} ${PCSXAB_WL_CLIENT} ${PCSXAB_WL_EGL})
endif()

# ---------------------------------------------------------------------------------------------------------
# flags common to the executable and the plugins (the Makefile's CFLAGS + what configure appended)
# ---------------------------------------------------------------------------------------------------------
add_compile_options(-Wall -ffast-math -Wno-unused-result)
add_compile_definitions(_REENTRANT _FILE_OFFSET_BITS=64)
if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 10)
    # GCC 10 switched to -fno-common. The tree defines globals in headers as tentative definitions
    # (p2_connected, INCLUDE_TITLE, region...) and linkage_arm.S declares psxRegs/rcnts with .comm, all of
    # which only link as common symbols. -fcommon is what GCC 8 did.
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-fcommon> $<$<COMPILE_LANGUAGE:CXX>:-fcommon>)
endif()
if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 14)
    # GCC 14 made these errors by default; Sony's GCC 8 (what pcsx-ab shipped with) only warned. The tree
    # has a few dozen of them - functions called without a prototype (isTitleName, save_error...),
    # void thread functions handed to pthread_create, one bare `return` in a void* function. They are
    # pre-existing and unchanged here; this only keeps them at warning level until they are cleaned up.
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-error=implicit-function-declaration>
        $<$<COMPILE_LANGUAGE:C>:-Wno-error=int-conversion>
        $<$<COMPILE_LANGUAGE:C>:-Wno-error=incompatible-pointer-types>
        $<$<COMPILE_LANGUAGE:C>:-Wno-error=return-mismatch>)
endif()
if (_pcsxab_is_arm)
    # the recompiler cannot handle Thumb code
    add_compile_options(-marm)
endif()
if (PCSXAB_ENABLE_MENU)
    add_compile_definitions(CONFIG_ENABLE_MENU)
endif()
if (PCSXAB_GLES)
    # HAVE_GLES turns on libpicofe's gl.c/gl_platform.c; SDL_VIDEO_DRIVER_WAYLAND makes SDL_syswm.h expose
    # the wl_* window handles that gl_platform.c builds its EGL surface on
    add_compile_definitions(HAVE_GLES SDL_VIDEO_DRIVER_WAYLAND)
endif()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# frontend/revision.h: the Makefile regenerated it from `git describe` on every build. Configure-time is
# enough here (re-run cmake to pick up a new tag); a tree without git history gets REV "".
find_package(Git QUIET)
set(_pcsxab_rev "")
if (GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --always --dirty
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE _pcsxab_rev OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
endif()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/revision.h.in" "#define REV \"${_pcsxab_rev}\"\n")
configure_file("${CMAKE_CURRENT_BINARY_DIR}/revision.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/frontend/revision.h" COPYONLY)

# ---------------------------------------------------------------------------------------------------------
# the executable
# ---------------------------------------------------------------------------------------------------------
set(PCSXAB_SOURCES
    # core
    libpcsxcore/cdriso.c libpcsxcore/cdrom.c libpcsxcore/cheat.c libpcsxcore/debug.c
    libpcsxcore/decode_xa.c libpcsxcore/disr3000a.c libpcsxcore/mdec.c
    libpcsxcore/misc.c libpcsxcore/plugins.c libpcsxcore/ppf.c libpcsxcore/psxbios.c
    libpcsxcore/psxcommon.c libpcsxcore/psxcounters.c libpcsxcore/psxdma.c libpcsxcore/psxhle.c
    libpcsxcore/psxhw.c libpcsxcore/psxinterpreter.c libpcsxcore/psxmem.c libpcsxcore/r3000a.c
    libpcsxcore/sio.c libpcsxcore/socket.c libpcsxcore/spu.c
    libpcsxcore/gte.c libpcsxcore/gte_divider.c
    libpcsxcore/new_dynarec/emu_if.c
    # spu (dfsound is always built in)
    plugins/dfsound/dma.c plugins/dfsound/freeze.c plugins/dfsound/registers.c plugins/dfsound/spu.c
    plugins/dfsound/out.c plugins/dfsound/nullsnd.c
    # gpulib glue, non-PIC copy for the built-in gpu (the plugins get their own PIC build below)
    plugins/gpulib/gpu.c plugins/gpulib/vout_pl.c
    # cdrcimg, dfinput
    plugins/cdrcimg/cdrcimg.c
    plugins/dfinput/main.c plugins/dfinput/pad.c plugins/dfinput/guncon.c
    # frontend: PLATFORM=generic with USE_PLUGIN_LIB and USE_FRONTEND
    frontend/cspace.c
    frontend/libpicofe/in_sdl.c frontend/libpicofe/in_ableem.c
    frontend/libpicofe/plat_sdl.c frontend/libpicofe/plat_dummy.c
    frontend/plat_sdl.c
    frontend/plugin_lib.c
    frontend/libpicofe/readpng.c frontend/libpicofe/fonts.c
    frontend/menu.c frontend/libpicofe/input.c
    frontend/main.c frontend/plugin.c
)

# the host layer: libpicofe's linux/ on the targets, frontend/win32/ for the MinGW development build
if (WIN32)
    list(APPEND PCSXAB_SOURCES frontend/win32/plat_win32.c libpcsxcore/memmap_win32.c)
    # our <dirent.h> (with d_type/scandir) must shadow mingw's: -I dirs are searched before system ones
    include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/frontend/win32/include)
else()
    list(APPEND PCSXAB_SOURCES frontend/libpicofe/linux/plat.c frontend/libpicofe/linux/in_evdev.c)
endif()

# gte_nf.o is gte.c compiled a second time with -DFLAGLESS (the flag-less GTE variant the dynarec calls
# into). CMake will not compile one source twice in one target, so a one-line wrapper does it.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gte_nf.c.in"
    "#define FLAGLESS\n#include \"${CMAKE_CURRENT_SOURCE_DIR}/libpcsxcore/gte.c\"\n")
configure_file("${CMAKE_CURRENT_BINARY_DIR}/gte_nf.c.in" "${CMAKE_CURRENT_BINARY_DIR}/gte_nf.c" COPYONLY)
list(APPEND PCSXAB_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/gte_nf.c")

if (_pcsxab_is_arm)
    list(APPEND PCSXAB_SOURCES libpcsxcore/gte_arm.S plugins/dfsound/arm_utils.S)
endif()
if (PCSXAB_NEON)
    list(APPEND PCSXAB_SOURCES libpcsxcore/gte_neon.S frontend/cspace_neon.S
        frontend/libpicofe/arm/neon_scale2x.S frontend/libpicofe/arm/neon_eagle2x.S)
    set_source_files_properties(frontend/libpicofe/arm/neon_scale2x.S frontend/libpicofe/arm/neon_eagle2x.S
        PROPERTIES COMPILE_DEFINITIONS DO_BGR_TO_RGB)
elseif (_pcsxab_is_arm)
    list(APPEND PCSXAB_SOURCES frontend/cspace_arm.S)
endif()
if (PCSXAB_DYNAREC)
    list(APPEND PCSXAB_SOURCES libpcsxcore/new_dynarec/new_dynarec.c libpcsxcore/new_dynarec/linkage_arm.S
        libpcsxcore/new_dynarec/pcsxmem.c)
else()
    set_source_files_properties(libpcsxcore/new_dynarec/emu_if.c PROPERTIES COMPILE_DEFINITIONS DRC_DISABLE)
endif()
if (PCSXAB_GLES)
    list(APPEND PCSXAB_SOURCES frontend/libpicofe/gl.c frontend/libpicofe/gl_platform.c)
endif()

# built-in gpu
if (PCSXAB_BUILTIN_GPU STREQUAL "neon")
    list(APPEND PCSXAB_SOURCES plugins/gpu_neon/psx_gpu_if.c plugins/gpu_neon/psx_gpu/psx_gpu_arm_neon.S)
    set_source_files_properties(plugins/gpu_neon/psx_gpu_if.c PROPERTIES
        COMPILE_DEFINITIONS "NEON_BUILD;TEXTURE_CACHE_4BPP;TEXTURE_CACHE_8BPP")
elseif (PCSXAB_BUILTIN_GPU STREQUAL "peops")
    list(APPEND PCSXAB_SOURCES plugins/dfxvideo/gpulib_if.c)
    # note: code is not safe for strict-aliasing? (Castlevania problems)
    set_source_files_properties(plugins/dfxvideo/gpulib_if.c PROPERTIES COMPILE_OPTIONS -fno-strict-aliasing)
elseif (PCSXAB_BUILTIN_GPU STREQUAL "unai")
    list(APPEND PCSXAB_SOURCES plugins/gpu_unai/gpulib_if.cpp)
    set_source_files_properties(plugins/gpu_unai/gpulib_if.cpp PROPERTIES COMPILE_DEFINITIONS REARMED COMPILE_OPTIONS -O3)
    if (_pcsxab_is_arm)
        list(APPEND PCSXAB_SOURCES plugins/gpu_unai/gpu_arm.s)
    endif()
else()
    message(FATAL_ERROR "unknown PCSXAB_BUILTIN_GPU '${PCSXAB_BUILTIN_GPU}'")
endif()

# spu output drivers
set(_pcsxab_out_defs "")
set(_pcsxab_sound_libs "")
foreach(_drv ${_pcsxab_sound_drivers})
    if (_drv STREQUAL "sdl")
        list(APPEND PCSXAB_SOURCES plugins/dfsound/sdl.c)
        list(APPEND _pcsxab_out_defs HAVE_SDL)
    elseif (_drv STREQUAL "alsa")
        list(APPEND PCSXAB_SOURCES plugins/dfsound/alsa.c)
        list(APPEND _pcsxab_out_defs HAVE_ALSA)
        list(APPEND _pcsxab_sound_libs asound)
    elseif (_drv STREQUAL "oss")
        list(APPEND PCSXAB_SOURCES plugins/dfsound/oss.c)
        list(APPEND _pcsxab_out_defs HAVE_OSS)
    elseif (_drv STREQUAL "pulseaudio")
        list(APPEND PCSXAB_SOURCES plugins/dfsound/pulseaudio.c)
        list(APPEND _pcsxab_out_defs HAVE_PULSE)
        list(APPEND _pcsxab_sound_libs pulse)
    else()
        message(FATAL_ERROR "unknown sound driver '${_drv}' in PCSXAB_SOUND_DRIVERS")
    endif()
endforeach()
set_source_files_properties(plugins/dfsound/out.c PROPERTIES COMPILE_DEFINITIONS "${_pcsxab_out_defs}")

set_source_files_properties(libpcsxcore/psxbios.c PROPERTIES COMPILE_OPTIONS -Wno-nonnull)

add_executable(pcsx-ab ${PCSXAB_SOURCES})
target_include_directories(pcsx-ab PRIVATE ${_pcsxab_sdl_inc_both})
target_link_libraries(pcsx-ab PRIVATE SDL2::SDL2 PNG::PNG ZLIB::ZLIB Threads::Threads
    ${CMAKE_DL_LIBS} m ${_pcsxab_sound_libs} ${_pcsxab_gles_libs})
if (WIN32)
    # SDL's own main() wrapper is not wanted (frontend/main.c has the real one); winsock for the gdb stub
    # in libpcsxcore/socket.c, winmm for timeBeginPeriod in frontend/win32/plat_win32.c
    target_compile_definitions(pcsx-ab PRIVATE SDL_MAIN_HANDLED)
    target_link_libraries(pcsx-ab PRIVATE ws2_32 winmm)
endif()
if (PCSXAB_BUILTIN_GPU STREQUAL "unai")
    set_target_properties(pcsx-ab PROPERTIES LINKER_LANGUAGE CXX)
endif()
if (PCSXAB_LINK_MAP)
    target_link_options(pcsx-ab PRIVATE "-Wl,-Map=$<TARGET_FILE_DIR:pcsx-ab>/pcsx-ab.map")
endif()
set_target_properties(pcsx-ab PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# ---------------------------------------------------------------------------------------------------------
# loadable plugins (plugins/*.so next to the executable, as the frontend's plugin loader expects)
# ---------------------------------------------------------------------------------------------------------
if (PCSXAB_PLUGINS)
    set(_pcsxab_plugin_dir "${CMAKE_BINARY_DIR}/plugins")

    # gpulib once more, PIC this time, shared by every gpu plugin (plugins/gpulib/Makefile's gpulib.a)
    add_library(gpulib_pic STATIC plugins/gpulib/gpu.c plugins/gpulib/vout_pl.c)
    set_target_properties(gpulib_pic PROPERTIES POSITION_INDEPENDENT_CODE ON OUTPUT_NAME gpulib)
    target_compile_options(gpulib_pic PRIVATE -ggdb)

    # name, sources. Each gets: -fPIC, -shared -Wl,--no-undefined, the gpulib archive.
    function(pcsxab_add_gpu_plugin name)
        add_library(${name} MODULE ${ARGN})
        set_target_properties(${name} PROPERTIES PREFIX "" LIBRARY_OUTPUT_DIRECTORY "${_pcsxab_plugin_dir}")
        target_compile_options(${name} PRIVATE -ggdb)
        target_link_libraries(${name} PRIVATE gpulib_pic)
        target_link_options(${name} PRIVATE -Wl,--no-undefined)
    endfunction()

    pcsxab_add_gpu_plugin(gpu_peops plugins/dfxvideo/gpulib_if.c)
    target_compile_options(gpu_peops PRIVATE -fno-strict-aliasing)

    pcsxab_add_gpu_plugin(gpu_unai plugins/gpu_unai/gpulib_if.cpp)
    target_compile_definitions(gpu_unai PRIVATE REARMED)
    target_compile_options(gpu_unai PRIVATE -O3)
    if (_pcsxab_is_arm)
        target_sources(gpu_unai PRIVATE plugins/gpu_unai/gpu_arm.s)
    endif()

    if (NOT PCSXAB_BUILTIN_GPU STREQUAL "neon" AND PCSXAB_NEON)
        pcsxab_add_gpu_plugin(gpu_neon plugins/gpu_neon/psx_gpu_if.c plugins/gpu_neon/psx_gpu/psx_gpu_arm_neon.S)
        target_compile_definitions(gpu_neon PRIVATE NDEBUG NEON_BUILD TEXTURE_CACHE_4BPP TEXTURE_CACHE_8BPP)
    endif()

    if (PCSXAB_GLES)
        pcsxab_add_gpu_plugin(gpu_gles plugins/gpu-gles/gpulib_if.c)
        target_link_libraries(gpu_gles PRIVATE ${PCSXAB_EGL_LIB} ${PCSXAB_GLES_LIB})
        target_link_options(gpu_gles PRIVATE -Wl,--allow-multiple-definition)
    endif()

    add_library(spunull MODULE plugins/spunull/spunull.c)
    set_target_properties(spunull PROPERTIES PREFIX "" LIBRARY_OUTPUT_DIRECTORY "${_pcsxab_plugin_dir}")
    target_compile_options(spunull PRIVATE -ggdb)
endif()

# ---------------------------------------------------------------------------------------------------------
# install: the flat layout AutoBleem's Autobleem/bin/emu/ holds (pcsx-ab + plugins/*.so)
# ---------------------------------------------------------------------------------------------------------
install(TARGETS pcsx-ab RUNTIME DESTINATION .)
if (PCSXAB_PLUGINS)
    set(_pcsxab_plugin_targets gpu_peops gpu_unai spunull)
    if (TARGET gpu_neon)
        list(APPEND _pcsxab_plugin_targets gpu_neon)
    endif()
    if (TARGET gpu_gles)
        list(APPEND _pcsxab_plugin_targets gpu_gles)
    endif()
    install(TARGETS ${_pcsxab_plugin_targets} LIBRARY DESTINATION plugins)
endif()

message(STATUS "pcsx-ab: arch=${CMAKE_SYSTEM_PROCESSOR} neon=${PCSXAB_NEON} dynarec=${PCSXAB_DYNAREC} "
    "builtin gpu=${PCSXAB_BUILTIN_GPU} sound=${PCSXAB_SOUND_DRIVERS} gles=${PCSXAB_GLES} menu=${PCSXAB_ENABLE_MENU} "
    "plugins=${PCSXAB_PLUGINS}")
