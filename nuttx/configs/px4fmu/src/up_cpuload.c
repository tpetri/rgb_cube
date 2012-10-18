/****************************************************************************
 * configs/px4fmu/src/up_leds.c
 * arch/arm/src/board/up_leds.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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
 * 3. Neither the name NuttX nor the names of its contributors may be
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <debug.h>

#include <sys/time.h>
#include <sched.h>

#include <arch/board/board.h>
#include <arch/board/up_hrt.h>
#include <arch/board/up_cpuload.h>

#include "chip.h"
#include "up_arch.h"
#include "up_internal.h"
#include "stm32_internal.h"
#include "px4fmu-internal.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/


/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name:
 ****************************************************************************/

#ifdef CONFIG_SCHED_INSTRUMENTATION

struct system_load_s system_load;

extern FAR _TCB *sched_gettcb(pid_t pid);

void cpuload_initialize_once(void);

void cpuload_initialize_once()
{
//	if (!system_load.initialized)
//	{
		system_load.start_time = hrt_absolute_time();
		int i;
		for (i = 0; i < CONFIG_MAX_TASKS; i++)
		{
			system_load.tasks[i].valid = false;
		}
		system_load.total_count = 0;

		uint64_t now = hrt_absolute_time();

		/* initialize idle thread statically */
		system_load.tasks[0].start_time = now;
		system_load.tasks[0].total_runtime = 0;
		system_load.tasks[0].curr_start_time = 0;
		system_load.tasks[0].tcb = sched_gettcb(0);
		system_load.tasks[0].valid = true;
		system_load.total_count++;

		/* initialize init thread statically */
		system_load.tasks[1].start_time = now;
		system_load.tasks[1].total_runtime = 0;
		system_load.tasks[1].curr_start_time = 0;
		system_load.tasks[1].tcb = sched_gettcb(1);
		system_load.tasks[1].valid = true;
		/* count init thread */
		system_load.total_count++;
		//	}
}

void sched_note_start(FAR _TCB *tcb )
{
	/* search first free slot */
	int i;
	for (i = 1; i < CONFIG_MAX_TASKS; i++)
	{
		if (!system_load.tasks[i].valid)
		{
			/* slot is available */
			system_load.tasks[i].start_time = hrt_absolute_time();
			system_load.tasks[i].total_runtime = 0;
			system_load.tasks[i].curr_start_time = 0;
			system_load.tasks[i].tcb = tcb;
			system_load.tasks[i].valid = true;
			system_load.total_count++;
			break;
		}
	}
}

void sched_note_stop(FAR _TCB *tcb )
{
	int i;
	for (i = 1; i < CONFIG_MAX_TASKS; i++)
	{
		if (system_load.tasks[i].tcb->pid == tcb->pid)
		{
			/* mark slot as fee */
			system_load.tasks[i].valid = false;
			system_load.tasks[i].total_runtime = 0;
			system_load.tasks[i].curr_start_time = 0;
			system_load.tasks[i].tcb = NULL;
			system_load.total_count--;
			break;
		}
	}
}

void sched_note_switch(FAR _TCB *pFromTcb, FAR _TCB *pToTcb)
{
	uint64_t new_time = hrt_absolute_time();

	/* Kind of inefficient: find both tasks and update times */
	uint8_t both_found = 0;
	for (int i = 0; i < CONFIG_MAX_TASKS; i++)
	{
		/* Task ending its current scheduling run */
		if (system_load.tasks[i].tcb->pid == pFromTcb->pid)
		{
			//if (system_load.tasks[i].curr_start_time != 0)
			{
				system_load.tasks[i].total_runtime += new_time - system_load.tasks[i].curr_start_time;
			}
			both_found++;
		}
		else if (system_load.tasks[i].tcb->pid == pToTcb->pid)
		{
			system_load.tasks[i].curr_start_time = new_time;
			both_found++;
		}

		/* Do only iterate as long as needed */
		if (both_found == 2)
		{
			break;
		}
	}
}

#endif /* CONFIG_SCHED_INSTRUMENTATION */
