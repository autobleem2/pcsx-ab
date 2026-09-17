/*
 * <dirent.h> for the Windows (MinGW) development build of pcsx-ab.
 *
 * This directory is put on the include path ahead of the system headers by CMakeLists.txt (WIN32 only),
 * so every `#include <dirent.h>` in the tree gets this one. mingw-w64's own dirent has no d_type, no
 * scandir() and no alphasort(), all three of which the file browser (libpicofe/menu.c), the CD image
 * filter (frontend/menu.c) and the error-status writer (libpcsxcore/misc.c) use. Implemented over
 * FindFirstFile in frontend/win32/plat_win32.c.
 */
#ifndef PCSXAB_WIN32_DIRENT_H
#define PCSXAB_WIN32_DIRENT_H

#include <stddef.h>

#define DT_UNKNOWN 0
#define DT_DIR     4
#define DT_REG     8
#define DT_LNK     10

struct dirent {
	long           d_ino;       /* always zero */
	unsigned short d_reclen;    /* always zero */
	unsigned short d_namlen;    /* strlen(d_name) */
	unsigned char  d_type;      /* DT_DIR / DT_REG / DT_LNK */
	char           d_name[260]; /* MAX_PATH */
};

typedef struct pcsxab_DIR DIR;

DIR *opendir(const char *name);
struct dirent *readdir(DIR *dir);
int closedir(DIR *dir);
void rewinddir(DIR *dir);

int scandir(const char *dir, struct dirent ***namelist,
	int (*filter)(const struct dirent *),
	int (*compar)(const struct dirent **, const struct dirent **));
int alphasort(const struct dirent **a, const struct dirent **b);

#endif
