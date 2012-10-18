/****************************************************************************
 *
 *   Copyright (C) 2012 PX4 Development Team. All rights reserved.
 *   Author: @author Example User <mail@example.com>
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
 * @file px4_simple_app.c
 * Minimal application example for PX4 autopilot
 */

#include <nuttx/config.h>
#include <unistd.h>
#include <stdio.h>
#include <poll.h>

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>
#include <time.h>
#include <unistd.h>

#include <arch/board/board.h>
#include <arch/board/up_hrt.h>
#include <arch/board/drv_tone_alarm.h>

__EXPORT int ledtest_main(int argc, char *argv[]);

int ledtest_main(int argc, char *argv[])
{
	printf("Hello Sky!\n");

#ifdef CONFIG_TONE_ALARM
	int fd, result;
	unsigned long tone;

	fd = open("/dev/tone_alarm", O_WRONLY);

	if (fd < 0) {
		printf("failed opening /dev/tone_alarm\n");
		goto out;
	}

	tone = 1;

	if (argc == 2)
		tone = atoi(argv[1]);

	if (tone  == 0) {
		result = ioctl(fd, TONE_SET_ALARM, 0);

		if (result < 0) {
			printf("failed clearing alarms\n");
			goto out;

		} else {
			printf("Alarm stopped.\n");
		}

	} else {
		result = ioctl(fd, TONE_SET_ALARM, 0);

		if (result < 0) {
			printf("failed clearing alarms\n");
			goto out;
		}

		result = ioctl(fd, TONE_SET_ALARM, tone);

		if (result < 0) {
			printf("failed setting alarm %lu\n", tone);

		} else {
			printf("Alarm %lu (disable with: tests tone 0)\n", tone);
		}
	}

out:

	if (fd >= 0)
		close(fd);

#endif
	return 0;
	return 0;
}
