# Shared by toolchains/rpi/RPitoolchain.cmake (32-bit) and toolchains/rpi64/RPi64toolchain.cmake (64-bit):
# where the cross compiler comes from. The same arrangement as AutoBleem's toolchains/rpi/common.cmake, so
# the two projects build side by side on either host:
#   - the Windows PC, with a "SysGCC for Raspberry Pi" toolchain (a sysroot rsynced from a real Pi, no -dev
#     packages - the toolchain dir's cmake/FindSDL2.cmake and FindPNG.cmake paper over that)
#   - a Debian host (AutoBleem's Docker image, docker/Dockerfile's pi stage) with Debian's own
#     crossbuild-essential-* and the multiarch libsdl2-dev:<arch> / libpng-dev:<arch> packages, which the
#     stock find_package() modules find on their own
# pcsxab_rpi_toolchain() picks the first when its directory exists, the second otherwise.

# pcsxab_rpi_toolchain(<triplet> <sysgcc root> <module dir>) - a macro, so the CMAKE_* it sets land in the
# toolchain file's own scope; <module dir> is the toolchain dir's cmake/ with the SysGCC find modules.
macro(pcsxab_rpi_toolchain _triplet _sysgcc_root _module_dir)
    if (EXISTS "${_sysgcc_root}/bin/${_triplet}-gcc.exe")
        set(_pcsxab_rpi_sysroot "${_sysgcc_root}/${_triplet}/sysroot")
        set(CMAKE_C_COMPILER   "${_sysgcc_root}/bin/${_triplet}-gcc.exe")
        set(CMAKE_CXX_COMPILER "${_sysgcc_root}/bin/${_triplet}-g++.exe")
        set(CMAKE_ASM_COMPILER "${_sysgcc_root}/bin/${_triplet}-gcc.exe")
        set(CMAKE_SYSROOT "${_pcsxab_rpi_sysroot}")
        set(CMAKE_FIND_ROOT_PATH "${_pcsxab_rpi_sysroot}")
        set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
        set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
        set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
        set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
        # the sysroot's runtime libs are in the Debian multiarch dir, which the cross gcc does not search by default
        set(CMAKE_EXE_LINKER_FLAGS_INIT    "-L${_pcsxab_rpi_sysroot}/usr/lib/${_triplet}")
        set(CMAKE_SHARED_LINKER_FLAGS_INIT "-L${_pcsxab_rpi_sysroot}/usr/lib/${_triplet}")
        list(APPEND CMAKE_MODULE_PATH "${_module_dir}")
        set(PCSXAB_RPI_TOOLCHAIN_KIND "sysgcc")
    else()
        find_program(_pcsxab_rpi_cross_gcc "${_triplet}-gcc")
        if (NOT _pcsxab_rpi_cross_gcc)
            message(FATAL_ERROR "no Raspberry Pi cross compiler: neither ${_sysgcc_root} (SysGCC, Windows) nor "
                                "${_triplet}-gcc on PATH (Debian: crossbuild-essential + libsdl2-dev/libpng-dev:<arch>)")
        endif()
        set(CMAKE_C_COMPILER   "${_triplet}-gcc")
        set(CMAKE_CXX_COMPILER "${_triplet}-g++")
        set(CMAKE_ASM_COMPILER "${_triplet}-gcc")
        # find_package looks in /usr/lib/<triplet>/cmake as well; SDL2_DIR pins the config package to that
        # architecture's copy (the host's own would otherwise be a candidate)
        set(CMAKE_LIBRARY_ARCHITECTURE "${_triplet}")
        set(SDL2_DIR "/usr/lib/${_triplet}/cmake/SDL2" CACHE PATH "" FORCE)
        set(PCSXAB_RPI_TOOLCHAIN_KIND "debian")
    endif()
endmacro()
