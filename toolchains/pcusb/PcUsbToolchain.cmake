# Cross-compile the emulator for AutoBleem's 32-bit PC USB stick (Debian 12 Bookworm i386) with Debian's own
# multiarch cross compiler - AutoBleem's Docker image, its pcusb stage: crossbuild-essential-i386 is
# i686-linux-gnu-gcc, the :i386 dev packages put SDL2's, libpng's and zlib's headers, .so links and cmake
# configs under /usr/lib/i386-linux-gnu. The same file as AutoBleem's toolchains/pcusb/PcUsbToolchain.cmake;
# no Windows-hosted toolchain for this target - it builds on the server: ci/build.sh pcusb.
#
#   cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=toolchains/pcusb/PcUsbToolchain.cmake -B build_pcusb
#
# The CPU is a plain i686 (no SSE2) - Debian's own i386 baseline, what its libsdl2:i386 is built for, so a
# Pentium M / Athlon XP class machine boots the stick. _FILE_OFFSET_BITS=64 keeps a >2 GB disc image
# readable through a 32-bit off_t.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)

set(_pcsxab_pcusb_triplet i686-linux-gnu)
find_program(_pcsxab_pcusb_gcc "${_pcsxab_pcusb_triplet}-gcc")
if (NOT _pcsxab_pcusb_gcc)
    message(FATAL_ERROR "no i386 cross compiler: ${_pcsxab_pcusb_triplet}-gcc is not on PATH "
                        "(Debian: crossbuild-essential-i386 + libsdl2-dev:i386 libpng-dev:i386 - AutoBleem's image, pcusb stage)")
endif()
set(CMAKE_C_COMPILER   "${_pcsxab_pcusb_triplet}-gcc")
set(CMAKE_CXX_COMPILER "${_pcsxab_pcusb_triplet}-g++")
set(CMAKE_ASM_COMPILER "${_pcsxab_pcusb_triplet}-gcc")
# find_package looks in /usr/lib/<multiarch>/cmake as well; SDL2_DIR pins the config package to that
# architecture's copy (the host's own would otherwise be a candidate)
set(CMAKE_LIBRARY_ARCHITECTURE "i386-linux-gnu")
set(SDL2_DIR "/usr/lib/i386-linux-gnu/cmake/SDL2" CACHE PATH "" FORCE)
set(CMAKE_FIND_ROOT_PATH "/usr/lib/i386-linux-gnu" "/usr/include/i386-linux-gnu")

set(_pcsxab_pcusb_flags "-march=i686 -mtune=generic -D_FILE_OFFSET_BITS=64")
set(CMAKE_C_FLAGS_INIT   "${_pcsxab_pcusb_flags}")
set(CMAKE_CXX_FLAGS_INIT "${_pcsxab_pcusb_flags}")
