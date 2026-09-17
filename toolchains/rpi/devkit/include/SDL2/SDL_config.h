/*
 * SDL_config.h for the pcsx-ab Raspberry Pi cross build (toolchains/rpi/).
 *
 * The other headers in this directory are SDL 2.32's public headers, borrowed from the MSYS2 package
 * because the sysGCC Pi sysroot ships libSDL2-2.0.so.0 but not libsdl2-dev. The MSYS2 SDL_config.h that
 * came with them is the MinGW one (SDL_VIDEO_DRIVER_WINDOWS -> SDL_syswm.h includes <windows.h>), so it
 * is replaced by this hand-written Linux/glibc one.
 *
 * Only two things in here matter to a program that merely *uses* SDL (as opposed to building it):
 *   - the HAVE_* libc feature macros SDL_stdinc.h consults to pick real libc headers over its fallbacks;
 *   - the SDL_VIDEO_DRIVER_* macros that select which members SDL_SysWMinfo's union gets. Its size is
 *     fixed by the dummy[64] pad regardless, so this stays ABI-compatible with the Pi's own libSDL2.
 * KMSDRM is what pcsx-ab runs on top of on a Pi (no Wayland, no X - see CMakeLists.txt's PCSXAB_GLES).
 */
#ifndef SDL_config_h_
#define SDL_config_h_

#include "SDL_platform.h"

#define SIZEOF_VOIDP 4
#define HAVE_GCC_ATOMICS 1

#define HAVE_LIBC 1
#define STDC_HEADERS 1
#define HAVE_ALLOCA_H 1
#define HAVE_CTYPE_H 1
#define HAVE_FLOAT_H 1
#define HAVE_ICONV_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_MALLOC_H 1
#define HAVE_MATH_H 1
#define HAVE_MEMORY_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_STDARG_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_WCHAR_H 1
#define HAVE_M_PI 1

#define HAVE_DLOPEN 1
#define HAVE_MALLOC 1
#define HAVE_CALLOC 1
#define HAVE_REALLOC 1
#define HAVE_FREE 1
#define HAVE_ALLOCA 1
#define HAVE_GETENV 1
#define HAVE_SETENV 1
#define HAVE_PUTENV 1
#define HAVE_UNSETENV 1
#define HAVE_QSORT 1
#define HAVE_BSEARCH 1
#define HAVE_ABS 1
#define HAVE_BCOPY 1
#define HAVE_MEMSET 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMCMP 1
#define HAVE_WCSLEN 1
#define HAVE_WCSDUP 1
#define HAVE_WCSSTR 1
#define HAVE_WCSCMP 1
#define HAVE_WCSNCMP 1
#define HAVE_WCSCASECMP 1
#define HAVE_WCSNCASECMP 1
#define HAVE_STRLEN 1
#define HAVE_STRDUP 1
#define HAVE_INDEX 1
#define HAVE_RINDEX 1
#define HAVE_STRCHR 1
#define HAVE_STRRCHR 1
#define HAVE_STRSTR 1
#define HAVE_STRTOK_R 1
#define HAVE_STRTOL 1
#define HAVE_STRTOUL 1
#define HAVE_STRTOLL 1
#define HAVE_STRTOULL 1
#define HAVE_STRTOD 1
#define HAVE_ATOI 1
#define HAVE_ATOF 1
#define HAVE_STRCMP 1
#define HAVE_STRNCMP 1
#define HAVE_STRCASECMP 1
#define HAVE_STRNCASECMP 1
#define HAVE_SSCANF 1
#define HAVE_VSSCANF 1
#define HAVE_VSNPRINTF 1
#define HAVE_ACOS 1
#define HAVE_ACOSF 1
#define HAVE_ASIN 1
#define HAVE_ASINF 1
#define HAVE_ATAN 1
#define HAVE_ATANF 1
#define HAVE_ATAN2 1
#define HAVE_ATAN2F 1
#define HAVE_CEIL 1
#define HAVE_CEILF 1
#define HAVE_COPYSIGN 1
#define HAVE_COPYSIGNF 1
#define HAVE_COS 1
#define HAVE_COSF 1
#define HAVE_EXP 1
#define HAVE_EXPF 1
#define HAVE_FABS 1
#define HAVE_FABSF 1
#define HAVE_FLOOR 1
#define HAVE_FLOORF 1
#define HAVE_FMOD 1
#define HAVE_FMODF 1
#define HAVE_LOG 1
#define HAVE_LOGF 1
#define HAVE_LOG10 1
#define HAVE_LOG10F 1
#define HAVE_LROUND 1
#define HAVE_LROUNDF 1
#define HAVE_POW 1
#define HAVE_POWF 1
#define HAVE_ROUND 1
#define HAVE_ROUNDF 1
#define HAVE_SCALBN 1
#define HAVE_SCALBNF 1
#define HAVE_SIN 1
#define HAVE_SINF 1
#define HAVE_SQRT 1
#define HAVE_SQRTF 1
#define HAVE_TAN 1
#define HAVE_TANF 1
#define HAVE_TRUNC 1
#define HAVE_TRUNCF 1
#define HAVE_FOPEN64 1
#define HAVE_FSEEKO 1
#define HAVE_FSEEKO64 1
#define HAVE_SIGACTION 1
#define HAVE_SA_SIGACTION 1
#define HAVE_SETJMP 1
#define HAVE_NANOSLEEP 1
#define HAVE_SYSCONF 1
#define HAVE_CLOCK_GETTIME 1
#define HAVE_GETPAGESIZE 1
#define HAVE_MPROTECT 1
#define HAVE_ICONV 1
#define HAVE_PTHREAD_SETNAME_NP 1
#define HAVE_SEM_TIMEDWAIT 1
#define HAVE_GETAUXVAL 1
#define HAVE_POLL 1
#define HAVE__EXIT 1

#define HAVE_DBUS_DBUS_H 1
#define HAVE_LINUX_INPUT_H 1
#define HAVE_LIBUDEV_H 1

/* the drivers Raspberry Pi OS's libsdl2 is built with, minus the ones whose SDL_syswm.h members would
 * need headers the sysroot lacks (X11 is present but unwanted, Wayland absent - and unwanted) */
#define SDL_AUDIO_DRIVER_ALSA 1
#define SDL_AUDIO_DRIVER_ALSA_DYNAMIC "libasound.so.2"
#define SDL_AUDIO_DRIVER_PULSEAUDIO 1
#define SDL_AUDIO_DRIVER_PULSEAUDIO_DYNAMIC "libpulse-simple.so.0"
#define SDL_AUDIO_DRIVER_DISK 1
#define SDL_AUDIO_DRIVER_DUMMY 1

#define SDL_INPUT_LINUXEV 1
#define SDL_JOYSTICK_LINUX 1
#define SDL_JOYSTICK_HIDAPI 1
#define SDL_JOYSTICK_VIRTUAL 1
#define SDL_HAPTIC_LINUX 1
#define SDL_SENSOR_DUMMY 1

#define SDL_LOADSO_DLOPEN 1
#define SDL_THREAD_PTHREAD 1
#define SDL_THREAD_PTHREAD_RECURSIVE_MUTEX 1
#define SDL_TIMER_UNIX 1

#define SDL_VIDEO_DRIVER_DUMMY 1
#define SDL_VIDEO_DRIVER_KMSDRM 1
#define SDL_VIDEO_DRIVER_KMSDRM_DYNAMIC "libdrm.so.2"
#define SDL_VIDEO_DRIVER_KMSDRM_DYNAMIC_GBM "libgbm.so.1"

#define SDL_VIDEO_RENDER_OGL_ES2 1
#define SDL_VIDEO_OPENGL_ES 1
#define SDL_VIDEO_OPENGL_ES2 1
#define SDL_VIDEO_OPENGL_EGL 1

#define SDL_POWER_LINUX 1
#define SDL_FILESYSTEM_UNIX 1

#endif /* SDL_config_h_ */
