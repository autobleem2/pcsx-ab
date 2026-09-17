# Vendored from libmamecd (https://github.com/DerekJohn0/libmamecd, BSD 3-Clause - see LICENSE.txt), an
# updated fork of libchdr: reads MAME's CHDv1-v5 disc image format. The same copy autobleem-develop carries
# under lib_ableem/third_party. Everything is built static from the vendored sources - the libchdr files
# plus the three codecs they decompress with (FLAC, lzma, zlib), each from its own copy under deps/ - so
# pcsx-ab picks up no new shared-library dependency on the console or the Pi. Vendored code is compiled
# with -w; it is not ours to keep warning-free.
#
# Used by libpcsxcore/cdriso.c (handlechd / cdread_chd) when PCSXAB_CHD is on.

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
    set(CPU_ARCH "x64")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|i386|i686)$")
    set(CPU_ARCH "x86")
else()
    set(CPU_ARCH "other")
endif()

add_subdirectory(deps/flac-1.3.3 EXCLUDE_FROM_ALL)
add_subdirectory(deps/lzma-19.00 EXCLUDE_FROM_ALL)
add_subdirectory(deps/zlib-1.2.11 EXCLUDE_FROM_ALL)

# FLAC's cpu.c prints its CPU detection to stderr on every decoder init unless NDEBUG is set - i.e. in any
# Debug build, once per hunk read. Vendored code, never wanted.
target_compile_definitions(FLAC PRIVATE NDEBUG)

add_library(mamecd STATIC
        src/libchdr_bitstream.c
        src/libchdr_cdecc.c
        src/libchdr_cdrom.c
        src/libchdr_chd.c
        src/libchdr_flac.c
        src/libchdr_huffman.c
        )

target_include_directories(mamecd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(mamecd PUBLIC FLAC lzma zlib)

set_source_files_properties(
        src/libchdr_bitstream.c
        src/libchdr_cdecc.c
        src/libchdr_cdrom.c
        src/libchdr_chd.c
        src/libchdr_flac.c
        src/libchdr_huffman.c
        PROPERTIES COMPILE_FLAGS "-w"
)
