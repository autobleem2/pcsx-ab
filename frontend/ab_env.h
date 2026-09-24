/*
 * What AutoBleem's launcher hands over through the environment - each one listed in the abfeatures file
 * next to the binary, which is how a launcher knows this build takes it (an older build ignores them).
 * AutoBleem's quiet-stick plan: nothing written to the stick that did not have to be. The same three as
 * pcsx-abnxt's frontend/ab/ab_config.h.
 *
 *   AB_EXIT_DIR     where the resume point of the way out goes - sstates/<name>.000, screenshots/<name>.png,
 *                   filename.txt, lastcdimg.txt, the layout of .pcsx/ - in RAM; the launcher copies it to the
 *                   stick only when the player keeps it                                (abfeatures: exitdir)
 *   AB_MEMCARD_DIR  the memory-card set this game plays with, used where it is        (abfeatures: memcarddir)
 *   AB_LOAD_STATE   a state file to start from - a kept resume slot, read where it is (abfeatures: loadstate)
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 */
#ifndef PCSXAB_AB_ENV_H
#define PCSXAB_AB_ENV_H

#include <stdlib.h>

static inline const char *ab_env(const char *name)
{
	const char *v = getenv(name);
	return v != NULL && v[0] != 0 ? v : NULL;
}

#define ab_exit_dir() ab_env("AB_EXIT_DIR")
#define ab_memcard_dir() ab_env("AB_MEMCARD_DIR")
#define ab_load_state() ab_env("AB_LOAD_STATE")

#endif
