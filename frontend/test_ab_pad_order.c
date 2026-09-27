/*
 * A standalone smoke test for ab_env.h's ab_pad_order() - AB_PAD_ORDER parsing (C11, Options -> "Swap
 * Player 1 / Player 2"). Not part of the emulator's CMake build - this repo builds with CMake but has no
 * test harness yet, so this is a small, self-contained C program: compile and run it directly, the same
 * way as frontend/test_pad_battery.c next to it.
 *
 *   gcc -o test_ab_pad_order test_ab_pad_order.c && ./test_ab_pad_order
 *
 * Identical in spirit and in every case to pcsx-abnxt's frontend/ab/test_ab_pad_order.c (C11).
 *
 * (C) AutoBleem team, 2026
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */
#include <stdio.h>

#include "ab_env.h"

static int failures;

static void expect(int cond, const char *what)
{
	if (!cond) {
		printf("FAIL: %s\n", what);
		failures++;
	} else {
		printf("ok:   %s\n", what);
	}
}

/* mirrors in_sdl2gc_probe()'s gate (Marcus's review fix): the swap only takes effect with two or more
 * pads about to be accepted - with fewer, the identity order is forced regardless of AB_PAD_ORDER, so a
 * lone pad is always player 1. Keep this in step with in_sdl2gc.c's in_sdl2gc_probe() too. */
static void effective_pad_order(int acceptedCount, int order[2])
{
	order[0] = 0;
	order[1] = 1;
	if (acceptedCount >= 2)
		ab_pad_order(order);
}

static void set_env(const char *value)
{
#ifdef _WIN32
	if (value == NULL)
		_putenv_s("AB_PAD_ORDER", "");
	else
		_putenv_s("AB_PAD_ORDER", value);
#else
	if (value == NULL)
		unsetenv("AB_PAD_ORDER");
	else
		setenv("AB_PAD_ORDER", value, 1);
#endif
}

int main(void)
{
	int order[2];

	/* 1) unset: identity, no swap */
	set_env(NULL);
	ab_pad_order(order);
	expect(order[0] == 0 && order[1] == 1, "unset: identity order {0, 1}");

	/* 2) the swap the launcher actually sends */
	set_env("1,0");
	ab_pad_order(order);
	expect(order[0] == 1 && order[1] == 0, "\"1,0\": swapped order {1, 0}");

	/* 3) the explicit non-swap, spelled out */
	set_env("0,1");
	ab_pad_order(order);
	expect(order[0] == 0 && order[1] == 1, "\"0,1\": identity order {0, 1}");

	/* 4) empty string: falls back to identity */
	set_env("");
	ab_pad_order(order);
	expect(order[0] == 0 && order[1] == 1, "empty: identity order {0, 1}");

	/* 5) malformed - not two numbers */
	set_env("1");
	ab_pad_order(order);
	expect(order[0] == 0 && order[1] == 1, "\"1\": malformed, identity order {0, 1}");

	set_env("garbage");
	ab_pad_order(order);
	expect(order[0] == 0 && order[1] == 1, "\"garbage\": malformed, identity order {0, 1}");

	/* 6) out-of-range digits */
	set_env("2,3");
	ab_pad_order(order);
	expect(order[0] == 0 && order[1] == 1, "\"2,3\": out of range, identity order {0, 1}");

	/* 7) not a permutation - both the same */
	set_env("0,0");
	ab_pad_order(order);
	expect(order[0] == 0 && order[1] == 1, "\"0,0\": not a permutation, identity order {0, 1}");

	set_env("1,1");
	ab_pad_order(order);
	expect(order[0] == 0 && order[1] == 1, "\"1,1\": not a permutation, identity order {0, 1}");

	/* 8) extra trailing text after two valid numbers is tolerated (sscanf stops at the second) */
	set_env("1,0,extra");
	ab_pad_order(order);
	expect(order[0] == 1 && order[1] == 0, "\"1,0,extra\": trailing text ignored, swapped order {1, 0}");

	/* 9) Marcus's review fix: the swap only takes effect with two or more pads about to be accepted */
	set_env("1,0");
	effective_pad_order(0, order);
	expect(order[0] == 0 && order[1] == 1, "0 pads, swap requested: identity order (nothing to swap)");

	effective_pad_order(1, order);
	expect(order[0] == 0 && order[1] == 1, "1 pad, swap requested: identity order - a lone pad stays player 1");

	effective_pad_order(2, order);
	expect(order[0] == 1 && order[1] == 0, "2 pads, swap requested: swapped order");

	effective_pad_order(3, order);
	expect(order[0] == 1 && order[1] == 0, "3 pads, swap requested: swapped order (still applies)");

	set_env(NULL);
	effective_pad_order(2, order);
	expect(order[0] == 0 && order[1] == 1, "2 pads, no swap requested: identity order");

	printf(failures == 0 ? "\nAll tests passed.\n" : "\n%d test(s) FAILED.\n", failures);
	return failures == 0 ? 0 : 1;
}
