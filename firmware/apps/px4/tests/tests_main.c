/****************************************************************************
 *
 *   Copyright (C) 2012 PX4 Development Team. All rights reserved.
 *   Author: @author Lorenz Meier <lm@inf.ethz.ch>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file tests_main.c
 * Tests main file, loads individual tests.
 */

#include <nuttx/config.h>

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <arch/board/board.h>

#include <nuttx/spi.h>

#include <systemlib/perf_counter.h>

#include "tests.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int test_help(int argc, char *argv[]);
static int test_all(int argc, char *argv[]);
static int test_perf(int argc, char *argv[]);
static int test_jig(int argc, char *argv[]);

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct {
	const char 	*name;
	int	(* fn)(int argc, char *argv[]);
	unsigned	options;
	int			passed;
#define OPT_NOHELP	(1<<0)
#define OPT_NOALLTEST	(1<<1)
#define OPT_NOJIGTEST	(1<<2)
} tests[] = {
	{"led",			test_led,	0, 0},
	{"int",			test_int,	0, 0},
	{"float",		test_float,	0, 0},
	{"sensors",		test_sensors,	0, 0},
	{"gpio",		test_gpio,	OPT_NOJIGTEST | OPT_NOALLTEST, 0},
	{"hrt",			test_hrt,	OPT_NOJIGTEST | OPT_NOALLTEST, 0},
	{"ppm",			test_ppm,	OPT_NOJIGTEST | OPT_NOALLTEST, 0},
	{"servo",		test_servo,	OPT_NOJIGTEST | OPT_NOALLTEST, 0},
	{"adc",			test_adc,	OPT_NOJIGTEST, 0},
	{"jig_voltages",	test_jig_voltages,	OPT_NOALLTEST, 0},
	{"eeproms",		test_eeproms,	0, 0},
	{"uart_loopback",	test_uart_loopback,	OPT_NOJIGTEST | OPT_NOALLTEST, 0},
	{"uart_baudchange",	test_uart_baudchange,	OPT_NOJIGTEST | OPT_NOALLTEST, 0},
	{"uart_send",		test_uart_send,	OPT_NOJIGTEST | OPT_NOALLTEST, 0},
	{"uart_console",	test_uart_console,	OPT_NOJIGTEST | OPT_NOALLTEST, 0},
	{"tone",		test_tone,	0, 0},
	{"sleep",		test_sleep,	OPT_NOJIGTEST, 0},
	{"time",		test_time,	OPT_NOJIGTEST, 0},
	{"perf",		test_perf,	OPT_NOJIGTEST, 0},
	{"all",			test_all,	OPT_NOALLTEST | OPT_NOJIGTEST, 0},
	{"jig",			test_jig,	OPT_NOJIGTEST | OPT_NOALLTEST, 0},
	{"help",		test_help,	OPT_NOALLTEST | OPT_NOHELP | OPT_NOJIGTEST, 0},
	{NULL,			NULL, 		0, 0}
};

static int
test_help(int argc, char *argv[])
{
	unsigned	i;

	printf("Available tests:\n");

	for (i = 0; tests[i].name; i++)
		printf("  %s\n", tests[i].name);

	return 0;
}

static int
test_all(int argc, char *argv[])
{
	unsigned	i;
	char		*args[2] = {"all", NULL};
	unsigned int failcount = 0;

	printf("\nRunning all tests...\n\n");

	for (i = 0; tests[i].name; i++) {
		/* Only run tests that are not excluded */
		if (!(tests[i].options & OPT_NOALLTEST)) {
			printf("  [%s] \t\t\tSTARTING TEST\n", tests[i].name);
			fflush(stdout);

			/* Execute test */
			if (tests[i].fn(1, args) != 0) {
				fprintf(stderr, "  [%s] \t\t\tFAIL\n", tests[i].name);
				fflush(stderr);
				failcount++;

			} else {
				tests[i].passed = 1;
				printf("  [%s] \t\t\tPASS\n", tests[i].name);
				fflush(stdout);
			}
		}
	}

	/* Print summary */
	printf("\n");
	int j;

	for (j = 0; j < 40; j++) {
		printf("-");
	}

	printf("\n\n");

	printf("     T E S T    S U M M A R Y\n\n");

	if (failcount == 0) {
		printf("  ______     __         __            ______     __  __    \n");
		printf(" /\\  __ \\   /\\ \\       /\\ \\          /\\  __ \\   /\\ \\/ /    \n");
		printf(" \\ \\  __ \\  \\ \\ \\____  \\ \\ \\____     \\ \\ \\/\\ \\  \\ \\  _\"-.  \n");
		printf("  \\ \\_\\ \\_\\  \\ \\_____\\  \\ \\_____\\     \\ \\_____\\  \\ \\_\\ \\_\\ \n");
		printf("   \\/_/\\/_/   \\/_____/   \\/_____/      \\/_____/   \\/_/\\/_/ \n");
		printf("\n");
		printf(" All tests passed (%d of %d)\n", i, i);

	} else {
		printf("  ______   ______     __     __ \n");
		printf(" /\\  ___\\ /\\  __ \\   /\\ \\   /\\ \\    \n");
		printf(" \\ \\  __\\ \\ \\  __ \\  \\ \\ \\  \\ \\ \\__\n");
		printf("  \\ \\_\\    \\ \\_\\ \\_\\  \\ \\_\\  \\ \\_____\\ \n");
		printf("   \\/_/     \\/_/\\/_/   \\/_/   \\/_____/ \n");
		printf("\n");
		printf(" Some tests failed (%d of %d)\n", failcount, i);
	}

	printf("\n");

	/* Print failed tests */
	if (failcount > 0) printf(" Failed tests:\n\n");

	unsigned int k;

	for (k = 0; k < i; k++) {
		if ((tests[k].passed == 0) && !(tests[k].options & OPT_NOALLTEST)) {
			printf(" [%s] to obtain details, please re-run with\n\t nsh> tests %s\n\n", tests[k].name, tests[k].name);
		}
	}

	fflush(stdout);

	return 0;
}

static int
test_perf(int argc, char *argv[])
{
	perf_counter_t	cc, ec;

	cc = perf_alloc(PC_COUNT, "test_count");
	ec = perf_alloc(PC_ELAPSED, "test_elapsed");

	if ((cc == NULL) || (ec == NULL)) {
		printf("perf: counter alloc failed\n");
		return 1;
	}

	perf_begin(ec);
	perf_count(cc);
	perf_count(cc);
	perf_count(cc);
	perf_count(cc);
	printf("perf: expect count of 4\n");
	perf_print_counter(cc);
	perf_end(ec);
	printf("perf: expect count of 1\n");
	perf_print_counter(ec);
	printf("perf: expect at least two counters\n");
	perf_print_all();

	perf_free(cc);
	perf_free(ec);

	return OK;
}

int test_jig(int argc, char *argv[])
{
	unsigned	i;
	char		*args[2] = {"jig", NULL};
	unsigned int failcount = 0;

	printf("\nRunning all tests...\n\n");
	for (i = 0; tests[i].name; i++) {
		/* Only run tests that are not excluded */
		if (!(tests[i].options & OPT_NOJIGTEST)) {
			printf("  [%s] \t\t\tSTARTING TEST\n", tests[i].name);
			fflush(stdout);
			/* Execute test */
			if (tests[i].fn(1, args) != 0) {
				fprintf(stderr, "  [%s] \t\t\tFAIL\n", tests[i].name);
				fflush(stderr);
				failcount++;
			} else {
				tests[i].passed = 1;
				printf("  [%s] \t\t\tPASS\n", tests[i].name);
				fflush(stdout);
			}
		}
	}

	/* Print summary */
	printf("\n");
	int j;
	for (j = 0; j < 40; j++)
	{
		printf("-");
	}
	printf("\n\n");

	printf("     T E S T    S U M M A R Y\n\n");
	if (failcount == 0) {
		printf("  ______     __         __            ______     __  __    \n");
		printf(" /\\  __ \\   /\\ \\       /\\ \\          /\\  __ \\   /\\ \\/ /    \n");
		printf(" \\ \\  __ \\  \\ \\ \\____  \\ \\ \\____     \\ \\ \\/\\ \\  \\ \\  _\"-.  \n");
		printf("  \\ \\_\\ \\_\\  \\ \\_____\\  \\ \\_____\\     \\ \\_____\\  \\ \\_\\ \\_\\ \n");
		printf("   \\/_/\\/_/   \\/_____/   \\/_____/      \\/_____/   \\/_/\\/_/ \n");
		printf("\n");
		printf(" All tests passed (%d of %d)\n", i, i);
	} else {
		printf("  ______   ______     __     __ \n");
		printf(" /\\  ___\\ /\\  __ \\   /\\ \\   /\\ \\    \n");
		printf(" \\ \\  __\\ \\ \\  __ \\  \\ \\ \\  \\ \\ \\__\n");
		printf("  \\ \\_\\    \\ \\_\\ \\_\\  \\ \\_\\  \\ \\_____\\ \n");
		printf("   \\/_/     \\/_/\\/_/   \\/_/   \\/_____/ \n");
		printf("\n");
		printf(" Some tests failed (%d of %d)\n", failcount, i);
	}
	printf("\n");

	/* Print failed tests */
	if (failcount > 0) printf(" Failed tests:\n\n");
	unsigned int k;
	for (k = 0; k < i; k++)
	{
		if ((tests[k].passed == 0) && !(tests[k].options & OPT_NOJIGTEST))
		{
			printf(" [%s] to obtain details, please re-run with\n\t nsh> tests %s\n\n", tests[k].name, tests[k].name);
		}
	}
	fflush(stdout);

	return 0;
}

__EXPORT int tests_main(int argc, char *argv[]);

/**
 * Executes system tests.
 */
int tests_main(int argc, char *argv[])
{
	unsigned	i;

	if (argc < 2) {
		printf("tests: missing test name - 'tests help' for a list of tests\n");
		return 1;
	}

	for (i = 0; tests[i].name; i++) {
		if (!strcmp(tests[i].name, argv[1]))
			return tests[i].fn(argc - 1, argv + 1);
	}

	printf("tests: no test called '%s' - 'tests help' for a list of tests\n", argv[1]);
	return ERROR;
}
