/* POSIX bits the MinGW development build lacks. (<dirent.h> with d_type/scandir is a separate header,
 * frontend/win32/include/dirent.h, that shadows the system one.) */
#ifndef PCSXAB_WIN32_COMPAT_H
#define PCSXAB_WIN32_COMPAT_H
#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX        /* frontend/menu.c has its own min()/max() */
#endif
#include <windows.h>
#include <direct.h>
#include <signal.h>

/* dlopen() over LoadLibrary, enough for frontend/menu.c's plugin directory scan (which finds nothing on
 * Windows - the plugins are built in, and SysLoadLibrary in main.c never loads external ones here) */
#define RTLD_LAZY   1
#define RTLD_NOW    2
#define RTLD_LOCAL  0
#define RTLD_GLOBAL 0
static inline void *dlopen(const char *name, int flags) { (void)flags; return (void *)LoadLibraryA(name); }
static inline void *dlsym(void *h, const char *sym) { return (void *)GetProcAddress((HMODULE)h, sym); }
static inline int dlclose(void *h) { return FreeLibrary((HMODULE)h) ? 0 : -1; }
static inline const char *dlerror(void) { return "LoadLibrary failed"; }

/* POSIX mkdir(path, mode); the mode is meaningless here */
#define mkdir(path, mode) _mkdir(path)

/* frontend/win32/plat_win32.c */
int fsync(int fd);
char *strcasestr(const char *haystack, const char *needle);

/* signal(SIGPIPE, ...) on the CRT just returns SIG_ERR for an unknown signal, which is fine */
#ifndef SIGPIPE
#define SIGPIPE 13
#endif

#endif
#endif
