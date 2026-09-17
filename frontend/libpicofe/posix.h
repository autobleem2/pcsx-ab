#ifndef LIBPICOFE_POSIX_H
#define LIBPICOFE_POSIX_H

/* define POSIX stuff: dirent, scandir, getcwd, mkdir */
#if defined(__linux__) || defined(__MINGW32__)

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

/* upstream's MinGW hacks; not needed with pcsx-ab's own <dirent.h> (frontend/win32/include), which has a
 * real d_type and scandir(), and include/win32_compat.h's mkdir() */
#if defined(__MINGW32__) && !defined(PCSXAB_WIN32_DIRENT_H)
#warning hacks!
#define mkdir(pathname,mode) mkdir(pathname)
#define d_type d_ino
#define DT_REG 0
#define DT_DIR 0
#endif

#else

#error "must provide posix"

#endif

#endif // LIBPICOFE_POSIX_H
