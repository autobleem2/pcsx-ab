/*
 * A standalone smoke test for ab_pad_battery.c's reader, over a fake sysfs tree built by this program
 * itself in a temp directory (no console, no real /sys needed). Not part of the emulator's CMake build -
 * this repo builds with CMake but has no test harness yet, so this is a small, self-contained C program:
 * compile and run it directly. Identical in spirit and in every case to pcsx-abnxt's
 * frontend/ab/test_pad_battery.c (E15) - only the include path (no ab/ subdirectory here) differs.
 *
 *   gcc -o test_pad_battery ab_pad_battery.c test_pad_battery.c && ./test_pad_battery
 *
 * The fake folder names use hyphens where the real sysfs tree has colons ("aa-bb-..." instead of
 * "aa:bb:...") - a real MAC address, on Windows a hyphen too, since a colon is not a legal Windows file
 * name character. ab_pad_battery.c never parses the suffix, so the change is test-only.
 *
 * (C) AutoBleem team, 2026
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ab_pad_battery.h"

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

static void write_file(const char *path, const char *contents)
{
	FILE *f = fopen(path, "wb");
	if (f == NULL) {
		fprintf(stderr, "cannot write %s\n", path);
		exit(1);
	}
	fputs(contents, f);
	fclose(f);
}

#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#define MKDIR(p) mkdir(p, 0755)
#endif

static void make_pad(const char *root, const char *name, const char *capacity, const char *level,
	const char *status)
{
	char dir[512], path[512];
	snprintf(dir, sizeof(dir), "%s/%s", root, name);
	MKDIR(dir);
	if (status != NULL) {
		snprintf(path, sizeof(path), "%s/status", dir);
		write_file(path, status);
	}
	if (capacity != NULL) {
		snprintf(path, sizeof(path), "%s/capacity", dir);
		write_file(path, capacity);
	}
	if (level != NULL) {
		snprintf(path, sizeof(path), "%s/capacity_level", dir);
		write_file(path, level);
	}
}

int main(void)
{
	const char *root = "ab_pad_battery_test_tree";
	unsigned int t = 0;

	MKDIR(root);

	/* 1) an empty tree: nothing found, nothing shown */
	{
		char sub[512];
		snprintf(sub, sizeof(sub), "%s/empty", root);
		MKDIR(sub);
#ifdef _WIN32
		_putenv_s("AB_PAD_BATTERY_DIR", sub);
#else
		setenv("AB_PAD_BATTERY_DIR", sub, 1);
#endif
		ab_pad_battery_tick(t);
		expect(!ab_pad_battery_visible(), "empty tree: icon not shown");
		expect(ab_pad_battery_percent() == -1, "empty tree: percent unknown");
	}

	/* 2) a wired pad's folder (no prefix match) is ignored */
	t += AB_PAD_BATTERY_POLL_MS + 1;
	{
		char sub[512];
		snprintf(sub, sizeof(sub), "%s/wired", root);
		MKDIR(sub);
		make_pad(sub, "some_other_battery_00-11-22-33-44-55", "5", NULL, "Discharging");
#ifdef _WIN32
		_putenv_s("AB_PAD_BATTERY_DIR", sub);
#else
		setenv("AB_PAD_BATTERY_DIR", sub, 1);
#endif
		ab_pad_battery_tick(t);
		expect(!ab_pad_battery_visible(), "non-matching folder name: ignored");
	}

	/* 3) a DualShock 4 (hid-sony) at 80%: known, not low, icon hidden */
	t += AB_PAD_BATTERY_POLL_MS + 1;
	{
		char sub[512];
		snprintf(sub, sizeof(sub), "%s/ds4_ok", root);
		MKDIR(sub);
		make_pad(sub, "sony_controller_battery_aa-bb-cc-dd-ee-ff", "80", NULL, "Discharging");
#ifdef _WIN32
		_putenv_s("AB_PAD_BATTERY_DIR", sub);
#else
		setenv("AB_PAD_BATTERY_DIR", sub, 1);
#endif
		ab_pad_battery_tick(t);
		expect(!ab_pad_battery_low(), "80%: not low");
		expect(!ab_pad_battery_visible(), "80%: icon hidden");
		expect(ab_pad_battery_percent() == 80, "80%: percent read back");

		/* a show request (kept for API parity, see ab_pad_battery.h) shows it anyway */
		ab_pad_battery_set_show_requested(1);
		expect(ab_pad_battery_visible(), "80%, show requested: icon shown");
		ab_pad_battery_set_show_requested(0);
		expect(!ab_pad_battery_visible(), "show request released: icon hidden again");
	}

	/* 4) a DualSense (ps-controller-battery-) at capacity 10: low, icon shown */
	t += AB_PAD_BATTERY_POLL_MS + 1;
	{
		char sub[512];
		snprintf(sub, sizeof(sub), "%s/ds5_low", root);
		MKDIR(sub);
		make_pad(sub, "ps-controller-battery-11-22-33-44-55-66", "10", NULL, "Discharging");
#ifdef _WIN32
		_putenv_s("AB_PAD_BATTERY_DIR", sub);
#else
		setenv("AB_PAD_BATTERY_DIR", sub, 1);
#endif
		ab_pad_battery_tick(t);
		expect(ab_pad_battery_low(), "10%: low");
		expect(ab_pad_battery_visible(), "10%: icon shown");
		expect(ab_pad_battery_percent() == 10, "10%: percent read back");
	}

	/* 5) hysteresis: climbing to 20% (between 15 and 25) keeps the latch; 25%+ clears it */
	t += AB_PAD_BATTERY_POLL_MS + 1;
	{
		char sub[512], path[512];
		snprintf(sub, sizeof(sub), "%s/ds5_low", root); /* reuse dir 4's, edit its capacity */
		snprintf(path, sizeof(path), "%s/ps-controller-battery-11-22-33-44-55-66/capacity", sub);
		write_file(path, "20");
		ab_pad_battery_tick(t);
		expect(ab_pad_battery_low(), "20% after 10%: still latched low (hysteresis)");

		t += AB_PAD_BATTERY_POLL_MS + 1;
		write_file(path, "30");
		ab_pad_battery_tick(t);
		expect(!ab_pad_battery_low(), "30%: latch cleared over the reset threshold");
	}

	/* 6) capacity_level words, no plain capacity file (a kernel that only reports the level) */
	t += AB_PAD_BATTERY_POLL_MS + 1;
	{
		char sub[512];
		snprintf(sub, sizeof(sub), "%s/level_only", root);
		MKDIR(sub);
		make_pad(sub, "sony_controller_battery_22-33-44-55-66-77", NULL, "Critical", "Discharging");
#ifdef _WIN32
		_putenv_s("AB_PAD_BATTERY_DIR", sub);
#else
		setenv("AB_PAD_BATTERY_DIR", sub, 1);
#endif
		ab_pad_battery_tick(t);
		expect(ab_pad_battery_percent() == 5, "capacity_level=Critical maps to 5%%");
		expect(ab_pad_battery_low(), "Critical: low");
	}

	/* 7) the throttle: a fresh (still-low) reading right after a poll must not be missed just because
	 * it was not re-read - but a stale poll inside the interval should not re-scan either. We only
	 * check that calling tick() again inside the interval does not crash and keeps the same answer. */
	{
		int before = ab_pad_battery_percent();
		ab_pad_battery_tick(t + 1);
		expect(ab_pad_battery_percent() == before, "inside the poll interval: no rescan, same answer");
	}

	printf(failures == 0 ? "\nAll tests passed.\n" : "\n%d test(s) FAILED.\n", failures);
	return failures == 0 ? 0 : 1;
}
