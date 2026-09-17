/*
 * Host layer for the Windows (MinGW / MSYS2 UCRT64) development build of pcsx-ab: what
 * frontend/libpicofe/linux/plat.c provides on the console and the Pi, done with Win32 calls.
 *
 * This build exists to run the frontend on a PC - the SDL2 renderer video path, the menu, input, save
 * states - not as a product. The dynarec is ARM-only, so it runs the interpreter and the peops GPU.
 *
 * This work is licensed under the terms of any of these licenses (at your option):
 *  - GNU GPL, version 2 or later.
 *  - GNU LGPL, version 2.1 or later.
 *  - MAME license.
 * See the COPYING file in the top-level directory.
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <direct.h>
#include <sys/stat.h>

#include "../libpicofe/plat.h"
#include "../../libpcsxcore/memmap.h"

/* the "Nubs as buttons" menu option belongs to libpicofe's evdev driver, which does not exist here */
int in_evdev_allow_abs_only;

int plat_is_dir(const char *path)
{
	DWORD attr = GetFileAttributesA(path);
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

/* the directory the executable lives in, with a trailing slash - skin/ and (without $HOME) everything
 * else are looked up relative to it, as on the console */
static int plat_get_data_dir(char *dst, int len)
{
	int i, ret = GetModuleFileNameA(NULL, dst, len);

	if (ret <= 0 || ret >= len) {
		dst[0] = 0;
		return 0;
	}
	for (i = ret - 1; i > 0; i--)
		if (dst[i] == '\\' || dst[i] == '/') {
			dst[i] = '/';
			dst[++i] = 0;
			break;
		}
	return i;
}

int plat_get_skin_dir(char *dst, int len)
{
	int ret = plat_get_data_dir(dst, len);
	if (ret < 0)
		return ret;

	memcpy(dst + ret, "skin/", sizeof "skin/");
	return ret + sizeof("skin/") - 1;
}

#ifndef PICO_HOME_DIR
#define PICO_HOME_DIR "/.picodrive/"
#endif
/* linux/plat.c uses $HOME/.picodrive/; here $HOME is honoured when set (an MSYS2 shell sets it), which
 * keeps the configs, memcards and states in one predictable place next to a Linux dev box's */
int plat_get_root_dir(char *dst, int len)
{
	const char *home = getenv("HOME");
	int ret;

	if (home == NULL)
		home = getenv("USERPROFILE");
	if (home != NULL) {
		ret = snprintf(dst, len, "%s%s", home, PICO_HOME_DIR);
		if (ret >= len)
			ret = len - 1;
		_mkdir(dst);
		return ret;
	}
	return plat_get_data_dir(dst, len);
}

static LARGE_INTEGER qpc_freq;

static unsigned long long ticks_us64(void)
{
	LARGE_INTEGER now;

	if (qpc_freq.QuadPart == 0)
		QueryPerformanceFrequency(&qpc_freq);
	QueryPerformanceCounter(&now);
	return (unsigned long long)now.QuadPart * 1000000ull / (unsigned long long)qpc_freq.QuadPart;
}

unsigned int plat_get_ticks_ms(void)
{
	return (unsigned int)(ticks_us64() / 1000);
}

unsigned int plat_get_ticks_us(void)
{
	return (unsigned int)ticks_us64();
}

void plat_sleep_ms(int ms)
{
	/* Sleep() is a 15.6 ms timer without this; the emulator paces frames with sleeps of a few ms */
	static int period_set;
	if (!period_set) {
		timeBeginPeriod(1);
		period_set = 1;
	}
	Sleep(ms);
}

/* only libpicofe's evdev/tslib input waits on file descriptors; the SDL input path polls */
int plat_wait_event(int *fds_hnds, int count, int timeout_ms)
{
	if (timeout_ms > 0)
		plat_sleep_ms(timeout_ms);
	return -1;
}

/* libpcsxcore/memmap_win32.c's mmap(): anonymous mappings are VirtualAlloc'd, which is all the core
 * and the frontend ask for (PSX RAM/ROM/scratch, the shadow framebuffers) */
void *plat_mmap(unsigned long addr, size_t size, int need_exec, int is_fixed)
{
	int prot = PROT_READ | PROT_WRITE;
	void *req, *ret;

	req = (void *)addr;
	if (need_exec)
		prot |= PROT_EXEC;

	ret = mmap(req, size, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ret == MAP_FAILED)
		return NULL;

	if (req != NULL && ret != req) {
		fprintf(stderr, "%s: mmaped to %p, requested %p\n",
			is_fixed ? "error" : "warning", ret, req);
		if (is_fixed) {
			munmap(ret, size);
			return NULL;
		}
	}
	return ret;
}

void *plat_mremap(void *ptr, size_t oldsize, size_t newsize)
{
	void *ret = plat_mmap(0, newsize, 0, 0);
	if (ret == NULL)
		return NULL;
	memcpy(ret, ptr, oldsize < newsize ? oldsize : newsize);
	munmap(ptr, oldsize);
	printf("warning: mremap moved: %p -> %p\n", ptr, ret);
	return ret;
}

void plat_munmap(void *ptr, size_t size)
{
	if (munmap(ptr, size) != 0)
		fprintf(stderr, "munmap(%p, %zu) failed: %d\n", ptr, size, errno);
}

int plat_mem_set_exec(void *ptr, size_t size)
{
	int ret = mprotect(ptr, size, PROT_READ | PROT_WRITE | PROT_EXEC);
	if (ret != 0)
		fprintf(stderr, "mprotect(%p, %zd) failed: %d\n", ptr, size, errno);
	return ret;
}

void lprintf(const char *fmt, ...)
{
	va_list vl;

	va_start(vl, fmt);
	vprintf(fmt, vl);
	va_end(vl);
}

/* <dirent.h> - see frontend/win32/include/dirent.h for why the build carries its own */
#include <dirent.h>

struct pcsxab_DIR {
	HANDLE h;
	WIN32_FIND_DATAA fd;
	int first_pending;      /* FindFirstFile already fetched the first entry */
	int done;
	char pattern[MAX_PATH + 4];
	struct dirent ent;
};

DIR *opendir(const char *name)
{
	DIR *dir;
	size_t len = strlen(name);

	if (len == 0 || len + 3 > MAX_PATH) {
		errno = ENOENT;
		return NULL;
	}
	if (!plat_is_dir(name)) {
		errno = ENOTDIR;
		return NULL;
	}
	dir = calloc(1, sizeof(*dir));
	if (dir == NULL)
		return NULL;
	snprintf(dir->pattern, sizeof(dir->pattern), "%s%s*", name,
		(name[len - 1] == '/' || name[len - 1] == '\\') ? "" : "/");
	rewinddir(dir);
	return dir;
}

void rewinddir(DIR *dir)
{
	if (dir->h != NULL && dir->h != INVALID_HANDLE_VALUE)
		FindClose(dir->h);
	dir->h = FindFirstFileA(dir->pattern, &dir->fd);
	dir->first_pending = (dir->h != INVALID_HANDLE_VALUE);
	dir->done = (dir->h == INVALID_HANDLE_VALUE);
}

struct dirent *readdir(DIR *dir)
{
	if (dir->done)
		return NULL;
	if (dir->first_pending)
		dir->first_pending = 0;
	else if (!FindNextFileA(dir->h, &dir->fd)) {
		dir->done = 1;
		return NULL;
	}

	dir->ent.d_ino = 0;
	dir->ent.d_reclen = 0;
	strncpy(dir->ent.d_name, dir->fd.cFileName, sizeof(dir->ent.d_name) - 1);
	dir->ent.d_name[sizeof(dir->ent.d_name) - 1] = 0;
	dir->ent.d_namlen = (unsigned short)strlen(dir->ent.d_name);
	if (dir->fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
		dir->ent.d_type = DT_LNK;
	else if (dir->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		dir->ent.d_type = DT_DIR;
	else
		dir->ent.d_type = DT_REG;
	return &dir->ent;
}

int closedir(DIR *dir)
{
	if (dir == NULL)
		return -1;
	if (dir->h != NULL && dir->h != INVALID_HANDLE_VALUE)
		FindClose(dir->h);
	free(dir);
	return 0;
}

int alphasort(const struct dirent **a, const struct dirent **b)
{
	return strcmp((*a)->d_name, (*b)->d_name);
}

int scandir(const char *dirname, struct dirent ***namelist,
	int (*filter)(const struct dirent *),
	int (*compar)(const struct dirent **, const struct dirent **))
{
	struct dirent **list = NULL, **tmp, *ent, *copy;
	int count = 0, cap = 0;
	DIR *d = opendir(dirname);

	if (d == NULL)
		return -1;
	while ((ent = readdir(d)) != NULL) {
		if (filter != NULL && !filter(ent))
			continue;
		if (count == cap) {
			cap = cap ? cap * 2 : 16;
			tmp = realloc(list, cap * sizeof(*list));
			if (tmp == NULL)
				goto fail;
			list = tmp;
		}
		copy = malloc(sizeof(*copy));
		if (copy == NULL)
			goto fail;
		*copy = *ent;
		list[count++] = copy;
	}
	closedir(d);
	if (compar != NULL && count > 1)
		qsort(list, count, sizeof(*list), (int (*)(const void *, const void *))compar);
	*namelist = list;
	return count;

fail:
	closedir(d);
	while (count > 0)
		free(list[--count]);
	free(list);
	return -1;
}

/* fsync() - the AutoBleem code syncs memory cards, save states and its status files, since the console
 * can lose power at any moment; on the PC it is a FlushFileBuffers on the CRT fd's handle */
#include <io.h>
int fsync(int fd)
{
	HANDLE h = (HANDLE)_get_osfhandle(fd);
	if (h == INVALID_HANDLE_VALUE) {
		errno = EBADF;
		return -1;
	}
	return FlushFileBuffers(h) ? 0 : -1;
}

/* strcasestr() - libpicofe's file browser; GNU extension, not in the CRT */
#include <ctype.h>
char *strcasestr(const char *haystack, const char *needle)
{
	size_t nlen = strlen(needle);
	if (nlen == 0)
		return (char *)haystack;
	for (; *haystack; haystack++) {
		size_t i;
		for (i = 0; i < nlen; i++)
			if (tolower((unsigned char)haystack[i]) != tolower((unsigned char)needle[i]))
				break;
		if (i == nlen)
			return (char *)haystack;
	}
	return NULL;
}
