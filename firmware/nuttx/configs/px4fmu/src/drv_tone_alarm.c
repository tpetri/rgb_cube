/*
 *   Copyright (C) 2012 Michael Smith. All rights reserved.
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
 * 3. Neither the name of the author or the names of contributors may be
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
 */

/*
 * Speaker driver supporting alarm sequences.
 *
 * Works with any of the 'generic' STM32 timers that has an output
 * pin, does not require an interrupt.
 *
 * Depends on the HRT timer.
 */

#include <nuttx/config.h>
#include <nuttx/arch.h>
#include <nuttx/irq.h>

#include <sys/types.h>
#include <stdbool.h>

#include <assert.h>
#include <debug.h>
#include <time.h>
#include <queue.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <arch/board/board.h>
#include <arch/board/drv_tone_alarm.h>
#include <arch/board/up_hrt.h>

#include "chip.h"
#include "up_internal.h"
#include "up_arch.h"

#include "stm32_internal.h"
#include "stm32_gpio.h"
#include "stm32_tim.h"

//#ifdef CONFIG_TONE_ALARM
# ifndef CONFIG_HRT_TIMER
#  error CONFIG_TONE_ALARM requires CONFIG_HRT_TIMER
# endif

/* Tone alarm configuration */
#if   TONE_ALARM_TIMER == 2
# define TONE_ALARM_BASE		STM32_TIM2_BASE
# define TONE_ALARM_CLOCK		STM32_APB1_TIM2_CLKIN
# define TONE_ALARM_CLOCK_ENABLE	RCC_APB1ENR_TIM2EN
# define TONE_ALARM_TIMER_VECTOR	STM32_IRQ_TIM2
# ifdef CONFIG_STM32_TIM2
#  error Must not set CONFIG_STM32_TIM2 when TONE_ALARM_TIMER is 2
# endif
#elif TONE_ALARM_TIMER == 3
# define TONE_ALARM_BASE		STM32_TIM3_BASE
# define TONE_ALARM_CLOCK		STM32_APB1_TIM3_CLKIN
# define TONE_ALARM_CLOCK_ENABLE	RCC_APB1ENR_TIM3EN
# define TONE_ALARM_TIMER_VECTOR	STM32_IRQ_TIM3
# ifdef CONFIG_STM32_TIM3
#  error Must not set CONFIG_STM32_TIM3 when TONE_ALARM_TIMER is 3
# endif
#elif TONE_ALARM_TIMER == 4
# define TONE_ALARM_BASE		STM32_TIM4_BASE
# define TONE_ALARM_CLOCK		STM32_APB1_TIM4_CLKIN
# define TONE_ALARM_CLOCK_ENABLE	RCC_APB1ENR_TIM4EN
# define TONE_ALARM_TIMER_VECTOR	STM32_IRQ_TIM4
# ifdef CONFIG_STM32_TIM4
#  error Must not set CONFIG_STM32_TIM4 when TONE_ALARM_TIMER is 4
# endif
#elif TONE_ALARM_TIMER == 5
# define TONE_ALARM_BASE		STM32_TIM5_BASE
# define TONE_ALARM_CLOCK		STM32_APB1_TIM5_CLKIN
# define TONE_ALARM_CLOCK_ENABLE	RCC_APB1ENR_TIM5EN
# define TONE_ALARM_TIMER_VECTOR	STM32_IRQ_TIM5
# ifdef CONFIG_STM32_TIM5
#  error Must not set CONFIG_STM32_TIM5 when TONE_ALARM_TIMER is 5
# endif
#elif TONE_ALARM_TIMER == 9
# define TONE_ALARM_BASE		STM32_TIM9_BASE
# define TONE_ALARM_CLOCK		STM32_APB1_TIM9_CLKIN
# define TONE_ALARM_CLOCK_ENABLE	RCC_APB1ENR_TIM9EN
# define TONE_ALARM_TIMER_VECTOR	STM32_IRQ_TIM9
# ifdef CONFIG_STM32_TIM9
#  error Must not set CONFIG_STM32_TIM9 when TONE_ALARM_TIMER is 9
# endif
#elif TONE_ALARM_TIMER == 10
# define TONE_ALARM_BASE		STM32_TIM10_BASE
# define TONE_ALARM_CLOCK		STM32_APB1_TIM10_CLKIN
# define TONE_ALARM_CLOCK_ENABLE	RCC_APB1ENR_TIM10EN
# define TONE_ALARM_TIMER_VECTOR	STM32_IRQ_TIM10
# ifdef CONFIG_STM32_TIM10
#  error Must not set CONFIG_STM32_TIM10 when TONE_ALARM_TIMER is 10
# endif
#elif TONE_ALARM_TIMER == 11
# define TONE_ALARM_BASE		STM32_TIM11_BASE
# define TONE_ALARM_CLOCK		STM32_APB1_TIM11_CLKIN
# define TONE_ALARM_CLOCK_ENABLE	RCC_APB1ENR_TIM11EN
# define TONE_ALARM_TIMER_VECTOR	STM32_IRQ_TIM11
# ifdef CONFIG_STM32_TIM11
#  error Must not set CONFIG_STM32_TIM11 when TONE_ALARM_TIMER is 11
# endif
#else
# error Must set TONE_ALARM_TIMER to a generic timer if CONFIG_TONE_ALARM is set
#endif

#if TONE_ALARM_CHANNEL == 1
# define TONE_CCMR1	(3 << 4)
# define TONE_CCMR2	0
# define TONE_CCER	(1 << 0)
# define TONE_rCCR	rCCR1
# define TONE_ONEBITWAVE_INT GTIM_DIER_CC1IE
#elif TONE_ALARM_CHANNEL == 2
# define TONE_CCMR1	(3 << 12)
# define TONE_CCMR2	0
# define TONE_CCER	(1 << 4)
# define TONE_rCCR	rCCR2
# define TONE_ONEBITWAVE_INT GTIM_DIER_CC2IE
#elif TONE_ALARM_CHANNEL == 3
# define TONE_CCMR1	0
# define TONE_CCMR2	(3 << 4)
# define TONE_CCER	(1 << 8)
# define TONE_rCCR	rCCR3
# define TONE_ONEBITWAVE_INT GTIM_DIER_CC3IE
#elif TONE_ALARM_CHANNEL == 4
# define TONE_CCMR1	0
# define TONE_CCMR2	(3 << 12)
# define TONE_CCER	(1 << 12)
# define TONE_rCCR	rCCR4
# define TONE_ONEBITWAVE_INT GTIM_DIER_CC4IE
#else
# error Must set TONE_ALARM_CHANNEL to a value between 1 and 4
#endif

/*
 * Timer register accessors
 */
#define REG(_reg)	(*(volatile uint32_t *)(TONE_ALARM_BASE + _reg))

#define rCR1     	REG(STM32_GTIM_CR1_OFFSET)
#define rCR2     	REG(STM32_GTIM_CR2_OFFSET)
#define rSMCR    	REG(STM32_GTIM_SMCR_OFFSET)
#define rDIER    	REG(STM32_GTIM_DIER_OFFSET)
#define rSR      	REG(STM32_GTIM_SR_OFFSET)
#define rEGR     	REG(STM32_GTIM_EGR_OFFSET)
#define rCCMR1   	REG(STM32_GTIM_CCMR1_OFFSET)
#define rCCMR2   	REG(STM32_GTIM_CCMR2_OFFSET)
#define rCCER    	REG(STM32_GTIM_CCER_OFFSET)
#define rCNT     	REG(STM32_GTIM_CNT_OFFSET)
#define rPSC     	REG(STM32_GTIM_PSC_OFFSET)
#define rARR     	REG(STM32_GTIM_ARR_OFFSET)
#define rCCR1    	REG(STM32_GTIM_CCR1_OFFSET)
#define rCCR2    	REG(STM32_GTIM_CCR2_OFFSET)
#define rCCR3    	REG(STM32_GTIM_CCR3_OFFSET)
#define rCCR4    	REG(STM32_GTIM_CCR4_OFFSET)
#define rDCR     	REG(STM32_GTIM_DCR_OFFSET)
#define rDMAR    	REG(STM32_GTIM_DMAR_OFFSET)

#define TONE_MAX_PATTERN	6
#define TONE_MAX_PATTERN_LEN	40

# define GPIO_MOSFET1	(GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_CLEAR|GPIO_PORTB|GPIO_PIN8)
# define GPIO_MOSFET2	(GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_CLEAR|GPIO_PORTB|GPIO_PIN9)
# define GPIO_MOSFET3	(GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_CLEAR|GPIO_PORTB|GPIO_PIN9)

/* predefined patterns for alarms 1-TONE_MAX_PATTERN */
const struct tone_note patterns[TONE_MAX_PATTERN][TONE_MAX_PATTERN_LEN] = {
	{
		{TONE_NOTE_A7, 12},
		{TONE_NOTE_D8, 12},
		{TONE_NOTE_C8, 12},
		{TONE_NOTE_A7, 12},
		{TONE_NOTE_D8, 12},
		{TONE_NOTE_C8, 12},
		{TONE_NOTE_D8, 4},
		{TONE_NOTE_C8, 4},
		{TONE_NOTE_D8, 4},
		{TONE_NOTE_C8, 4},
		{TONE_NOTE_D8, 4},
		{TONE_NOTE_C8, 4},
	},
	{{TONE_NOTE_B6, 100}},
	{{TONE_NOTE_C7, 100}},
	{{TONE_NOTE_D7, 100}},
	{{TONE_NOTE_E7, 100}},
	{	//This is tetris ;)
		{TONE_NOTE_C6, 40},
		{TONE_NOTE_G5, 20},
		{TONE_NOTE_G5S, 20},
		{TONE_NOTE_A5S, 40},
		{TONE_NOTE_G5S, 20},
		{TONE_NOTE_G5, 20},
		{TONE_NOTE_F5, 40},
		{TONE_NOTE_F5, 20},
		{TONE_NOTE_G5S, 20},
		{TONE_NOTE_C6, 40},
		{TONE_NOTE_A5S, 20},
		{TONE_NOTE_G5S, 20},
		{TONE_NOTE_G5, 60},
		{TONE_NOTE_G5S, 20},
		{TONE_NOTE_A5S, 40},
		{TONE_NOTE_C6, 40},
		{TONE_NOTE_G5S, 40},
		{TONE_NOTE_F5, 40},
		{TONE_NOTE_F5, 60},

		{TONE_NOTE_A5S, 40},
		{TONE_NOTE_C6S, 20},
		{TONE_NOTE_F6, 40},
		{TONE_NOTE_D6S, 20},
		{TONE_NOTE_C6S, 20},
		{TONE_NOTE_C6, 60},
		{TONE_NOTE_G5S, 20},
		{TONE_NOTE_C6, 40},
		{TONE_NOTE_A5S, 20},
		{TONE_NOTE_G5S, 20},
		{TONE_NOTE_G5, 40},
		{TONE_NOTE_G5, 20},
		{TONE_NOTE_G5S, 20},
		{TONE_NOTE_A5S, 40},
		{TONE_NOTE_C6, 40},
		{TONE_NOTE_G5S, 40},
		{TONE_NOTE_F5, 40},
		{TONE_NOTE_F5, 60},
	}
};

static uint16_t notes[TONE_NOTE_MAX] = {
	63707, /* E4 */
	60132, /* F4 */
	56758, /* F#4/Gb4 */
	53571, /* G4 */
	50565, /* G#4/Ab4 */
	47727, /* A4 */
	45048, /* A#4/Bb4 */
	42520, /* B4 */
	40133, /* C5 */
	37880, /* C#5/Db5 */
	35755, /* D5 */
	33748, /* D#5/Eb5 */
	31853, /* E5 */
	30066, /* F5 */
	28378, /* F#5/Gb5 */
	26786, /* G5 */
	25282, /* G#5/Ab5 */
	23863, /* A5 */
	22524, /* A#5/Bb5 */
	21260, /* B5 */
	20066, /* C6 */
	18940, /* C#6/Db6 */
	17877, /* D6 */
	16874, /* D#6/Eb6 */
	15927, /* E6 */
	15033, /* F6 */
	14189, /* F#6/Gb6 */
	13393, /* G6 */
	12641, /* G#6/Ab6 */
	11931, /* A6 */
	11262, /* A#6/Bb6 */
	10630, /* B6 */
	10033, /* C7 */
	9470, /* C#7/Db7 */
	8938, /* D7 */
	8437, /* D#7/Eb7 */
	7963, /* E7 */
	7516, /* F7 */
	7094, /* F#7/Gb7 */
	6696, /* G7 */
	6320, /* G#7/Ab7 */
	5965, /* A7 */
	5631, /* A#7/Bb7 */
	5315, /* B7 */
	5016, /* C8 */
	4735, /* C#8/Db8 */
	4469, /* D8 */
	4218  /* D#8/Eb8 */
};

/* #include "drv_tone_alarm_wave.pxw" this is for petris testing purposes 1-bit original gameboy tetris sound */
static const uint8_t onebitwave[] = { 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,
		                               0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,
		                               0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,
		                               0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,
		                               0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,
		                               0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,
		                               0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,
		                               0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF};

/* current state of the tone driver */
struct state {
	int		pattern;				/* current pattern */
#define PATTERN_NONE	-1
#define PATTERN_USER	0
	int		note;					/* next note to play */
	struct hrt_call	note_end;
	struct tone_note user_pattern[TONE_MAX_PATTERN_LEN];	/* user-supplied pattern (plays at pattern 0) */
};

static struct state tone_state;

static int	tone_write(struct file *filp, const char *buffer, size_t len);
static int	tone_ioctl(struct file *filep, int cmd, unsigned long arg);

static const struct file_operations tone_fops = {
	.write = tone_write,
	.ioctl = tone_ioctl
};

static void	tone_next(void);
static bool	tone_ok(struct tone_note *pattern);

static uint8_t red[8*8*8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static uint8_t green[8*8*8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static uint8_t blue[8*8*8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static uint8_t pwm_cycle = 0;
static uint8_t clk = 0;
static uint16_t dir = 0;
static uint8_t layer = 0;
static uint8_t bit = 0;

static int
tone_tim_isr(int irq, void *context)
{
	uint32_t status;

	/* copy interrupt status */
	status = rSR;

	/* ack the interrupts we just read */
	rSR = ~status;

	/* was this a timer tick? */
	if (status & GTIM_SR_CC3IF)
	{
		stm32_gpiowrite(GPIO_TONE_ALARM_WAVE, 0);	//clk

		if (layer == 0)
		{
			stm32_gpiowrite(GPIO_MOSFET1, 1);
			stm32_gpiowrite(GPIO_MOSFET2, 1);
			stm32_gpiowrite(GPIO_MOSFET3, 1);
		}
		else if (layer == 1)
		{
			stm32_gpiowrite(GPIO_MOSFET1, 1);
			stm32_gpiowrite(GPIO_MOSFET2, 0);
			stm32_gpiowrite(GPIO_MOSFET3, 1);
		}
		else if (layer == 2)
		{
			stm32_gpiowrite(GPIO_MOSFET1, 1);
			stm32_gpiowrite(GPIO_MOSFET2, 1);
			stm32_gpiowrite(GPIO_MOSFET3, 1);
		}
		else
		{
			stm32_gpiowrite(GPIO_MOSFET1, 1);
			stm32_gpiowrite(GPIO_MOSFET2, 1);
			stm32_gpiowrite(GPIO_MOSFET3, 1);
		}

		if (bit == 0)
		{
			stm32_gpiowrite(GPIO_TONE_ALARM_WAVE3, 0); //disable latch until we have sent all bits

			stm32_gpiowrite(GPIO_TONE_ALARM_WAVE2, red[layer]/4 > pwm_cycle);
		}
		else if (bit == 1)
		{
			stm32_gpiowrite(GPIO_TONE_ALARM_WAVE2, red[layer]/4 > pwm_cycle);
		}
		else if (bit == 2)
		{
			stm32_gpiowrite(GPIO_TONE_ALARM_WAVE2, green[layer]/4 > pwm_cycle);
		}
		else if (bit == 3)
		{
			stm32_gpiowrite(GPIO_TONE_ALARM_WAVE2, blue[layer]/4 > pwm_cycle);
		}
		else if (bit == 4)
		{
			stm32_gpiowrite(GPIO_TONE_ALARM_WAVE2, green[layer]/4 > pwm_cycle);
		}
		else if (bit == 5)
		{
			stm32_gpiowrite(GPIO_TONE_ALARM_WAVE2, blue[layer]/4 > pwm_cycle);
		}
		else if (bit == 6)
		{
			stm32_gpiowrite(GPIO_TONE_ALARM_WAVE2, red[layer]/4 > pwm_cycle);
		}
		else if (bit == 7)
		{
			stm32_gpiowrite(GPIO_TONE_ALARM_WAVE2, red[layer]/4 > pwm_cycle);

			stm32_gpiowrite(GPIO_TONE_ALARM_WAVE3, 1); //enable latch if we sent the last bit
		}

		bit++;
		if (bit == 8)
		{
			bit = 0;
			pwm_cycle++;
			if (pwm_cycle == 64) pwm_cycle = 0;
		}

		stm32_gpiowrite(GPIO_TONE_ALARM_WAVE, 1);	//clk




		if (bit == 0) layer++;
		if (layer == 8) layer = 0;
	}

	return OK;
}


int
tone_alarm_init(void)
{
	/* configure the GPIO */
	stm32_configgpio(GPIO_TONE_ALARM);

	/* clock/power on our timer */
	modifyreg32(STM32_RCC_APB1ENR, 0, TONE_ALARM_CLOCK_ENABLE);

	/* claim our interrupt vector */
	irq_attach(TONE_ALARM_TIMER_VECTOR, tone_tim_isr);

	/* enable interrupts */
	up_enable_irq(TONE_ALARM_TIMER_VECTOR);

	/* default the state */
	tone_state.pattern = -1;

	/* initialise the timer */
	rCR1 = 0;
	rCR2 = 0;
	rSMCR = 0;
	rDIER = 0;
	rCCER = 0;		/* unlock CCMR* registers */
	rCCMR1 = TONE_CCMR1;
	rCCMR2 = TONE_CCMR2;
	rCCER = TONE_CCER;
	rDCR = 0;

	/* toggle the CC output each time the count passes 1 */
	TONE_rCCR = 1;

	/*
	 * Configure the timebase to free-run at half max frequency.
	 * XXX this should be more flexible in order to get a better
	 * frequency range, but for the F4 with the APB1 timers based
	 * at 42MHz, this gets us down to ~320Hz or so.
	 */
	rPSC = 1;

	tone_state.pattern = 1;
	tone_state.note = 0;

	/* play the startup tune */
	tone_next();

	/* register the device */
	return register_driver("/dev/tone_alarm", &tone_fops, 0666, NULL);
}

static int
tone_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	int result = 0;
	int new = (int)arg;

	irqstate_t flags = irqsave();

	/* decide whether to increase the alarm level to cmd or leave it alone */
	switch (cmd) {
	case TONE_SET_ALARM:
		if (new == 0) {
			/* cancel any current alarm */
			tone_state.pattern = PATTERN_NONE;
			tone_next();
		} else if (new > TONE_MAX_PATTERN+1) {
			/* not a legal alarm value */
			result = -ERANGE;
		} else if (new == TONE_MAX_PATTERN+1) {
			/* settings for 1-bit wave */
			rCR1 = 0;
			stm32_configgpio(GPIO_TONE_ALARM_WAVE);
			stm32_configgpio(GPIO_TONE_ALARM_WAVE2);
			stm32_configgpio(GPIO_TONE_ALARM_WAVE3);
			stm32_configgpio(GPIO_MOSFET1);
			stm32_configgpio(GPIO_MOSFET2);
			stm32_configgpio(GPIO_MOSFET3);
			stm32_gpiowrite(GPIO_MOSFET1, 1);
			stm32_gpiowrite(GPIO_MOSFET2, 1);
			stm32_gpiowrite(GPIO_MOSFET3, 1);
			rDIER = TONE_ONEBITWAVE_INT;
			rPSC = TONE_ALARM_CLOCK / 122880;
			rARR = 0x1;
			rEGR = GTIM_EGR_UG;
			rCR1 = GTIM_CR1_CEN;

			tone_next(); //start animation

		} else if (new > tone_state.pattern) {
			/* higher priority than the current alarm */
			tone_state.pattern = new;
			tone_state.note = 0;

			rCR1 = 0;
			stm32_configgpio(GPIO_TONE_ALARM);
			rDIER = 0;
			rPSC = 1;

			/* and start playing it */
			tone_next();
		}
		break;
	default:
		result = -EINVAL;
		break;
	}

	irqrestore(flags);
	return result;
}

static int
tone_write(struct file *filp, const char *buffer, size_t len)
{
	irqstate_t flags;

	/* sanity-check the size of the write */
	if (len > (TONE_MAX_PATTERN_LEN * sizeof(struct tone_note)))
		return -EFBIG;
	if ((len % sizeof(struct tone_note)) || (len == 0))
		return -EIO;
	if (!tone_ok((struct tone_note *)buffer))
		return -EIO;

	flags = irqsave();

	/* if we aren't playing an alarm tone */
	if (tone_state.pattern <= PATTERN_USER) {

		/* reset the tone state to play the new user pattern */
		tone_state.pattern = PATTERN_USER;
		tone_state.note = 0;

		/* copy in the new pattern */
		memset(tone_state.user_pattern, 0, sizeof(tone_state.user_pattern));
		memcpy(tone_state.user_pattern, buffer, len);

		/* and start it */
		tone_next();
	}
	irqrestore(flags);

	return len;
}

static void
tone_next(void)
{
//	const struct tone_note *np;
//
//	/* stop the current note */
//	rCR1 = 0;
//
//	/* if we are no longer playing a pattern, we have nothing else to do here */
//	if (tone_state.pattern == PATTERN_NONE) {
//		return;
//	}
//
//	/* if the current pattern has ended, clear the pattern and stop */
//	if (tone_state.note == TONE_NOTE_MAX) {
//		tone_state.pattern = PATTERN_NONE;
//		return;
//	}
//
//	/* find the note to play */
//	if (tone_state.pattern == PATTERN_USER) {
//		np = &tone_state.user_pattern[tone_state.note];
//	} else {
//		np = &patterns[tone_state.pattern - 1][tone_state.note];
//	}
//
//	/* work out which note is next */
//	tone_state.note++;
//	if (tone_state.note >= TONE_NOTE_MAX) {
//		/* hit the end of the pattern, stop */
//		tone_state.pattern = PATTERN_NONE;
//	} else if (np[1].duration == DURATION_END) {
//		/* hit the end of the pattern, stop */
//		tone_state.pattern = PATTERN_NONE;
//	} else if (np[1].duration == DURATION_REPEAT) {
//		/* next note is a repeat, rewind in preparation */
//		tone_state.note = 0;
//	}
//
//	/* set the timer to play the note, if required */
//	if (np->pitch <= TONE_NOTE_SILENCE) {
//
//		/* set reload based on the pitch */
//		rARR = notes[np->pitch];
//
//		/* force an update, reloads the counter and all registers */
//		rEGR = GTIM_EGR_UG;
//
//		/* start the timer */
//		rCR1 = GTIM_CR1_CEN;
//	}

//	if (blue[0] == 255) dir = 0;
//	if (blue[0] == 0) dir = 1;
//
//	if (dir)
//		blue[0]++;
//	else
//		blue[0]--;


	float s = dir++/360.f;

	if(dir == 360) dir = 0;

	float f, p, q, t;
	s *= 6.f;
	int in = (int)(floor(s));
	f = s - in;

	p = 0.f;
	q = (float)(1.f - f);
	t = (float)(1.f - (1.f - f));
	float r,g,b;
	switch(in)
	{
		case 0: r=1; g=t; b=p; break;
		case 1: r=q; g=1; b=p; break;
		case 2: r=p; g=1; b=t; break;
		case 3: r=p; g=q; b=1; break;
		case 4: r=t; g=p; b=1; break;
		case 5: r=1; g=p; b=q; break;
		default: r = 1; g = b = 0; break;
	}

	red[0] = fabs(r)*255.f;
	green[0] = fabs(g)*255.f;
	blue[0] = fabs(b)*255.f;

	red[1] = fabs(g)*255.f;
	green[1] = fabs(b)*255.f;
	blue[1] = fabs(r)*255.f;

	/* arrange a callback when the note/rest is done */
	hrt_call_after(&tone_state.note_end, (hrt_abstime)20000, (hrt_callout)tone_next, NULL);
}

static bool
tone_ok(struct tone_note *pattern)
{
	int	i;

	for (i = 0; i < TONE_NOTE_MAX; i++) {

		/* first note must not be repeat or end */
		if ((i == 0) &&
		    ((pattern[i].duration == DURATION_END) || (pattern[i].duration == DURATION_REPEAT)))
			return false;
		if (pattern[i].duration == DURATION_END)
			break;

		/* pitch must be legal */
		if (pattern[i].pitch >= TONE_NOTE_MAX)
			return false;
	}
	return true;
}

//#endif /* CONFIG_TONE_ALARM */
