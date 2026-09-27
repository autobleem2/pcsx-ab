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
 *   AB_PAD_ORDER    C11, Options -> "Swap Player 1 / Player 2": "0,1" (no swap, never actually sent) or
 *                   "1,0" - a purely positional permutation of the first two SDL joystick device-indexes,
 *                   nothing to do with which physical pad it is; only sent when this build lists
 *                   "padorder" in abfeatures                                            (abfeatures: padorder)
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 */
#ifndef PCSXAB_AB_ENV_H
#define PCSXAB_AB_ENV_H

#include <stdio.h>
#include <stdlib.h>

static inline const char *ab_env(const char *name)
{
	const char *v = getenv(name);
	return v != NULL && v[0] != 0 ? v : NULL;
}

#define ab_exit_dir() ab_env("AB_EXIT_DIR")
#define ab_memcard_dir() ab_env("AB_MEMCARD_DIR")
#define ab_load_state() ab_env("AB_LOAD_STATE")

/* Parses AB_PAD_ORDER ("a,b", each 0 or 1) into order[0]/order[1] - order[i] is the PS1 port (0-based)
 * the SDL pad at device-index i lands on. Missing, malformed, or anything but a permutation of {0,1}
 * (out-of-range digits, a repeated digit) leaves order as the identity {0, 1} and changes nothing - the
 * unswapped, always-safe default. */
static inline void ab_pad_order(int order[2])
{
	const char *v = ab_env("AB_PAD_ORDER");
	int a = -1, b = -1;
	order[0] = 0;
	order[1] = 1;
	if (v == NULL)
		return;
	if (sscanf(v, "%d,%d", &a, &b) != 2)
		return;
	if ((a != 0 && a != 1) || (b != 0 && b != 1) || a == b)
		return;
	order[0] = a;
	order[1] = b;
}

#endif
