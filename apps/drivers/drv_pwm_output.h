/****************************************************************************
 *
 *   Copyright (C) 2012 PX4 Development Team. All rights reserved.
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
 * @file PWM servo output interface.
 *
 * Servo values can be set with the PWM_SERVO_SET ioctl, by writing a
 * pwm_output_values structure to the device, or by publishing to the
 * output_pwm ObjDev.
 * Writing a value of 0 to a channel suppresses any output for that
 * channel.
 */

#ifndef _DRV_PWM_OUTPUT_H
#define _DRV_PWM_OUTPUT_H

#include <stdint.h>
#include <sys/ioctl.h>

#include "drv_orb_dev.h"

/**
 * Path for the default PWM output device.
 *
 * Note that on systems with more than one PWM output path (e.g.
 * PX4FMU with PX4IO connected) there may be other devices that
 * respond to this protocol.
 */
#define PWM_OUTPUT_DEVICE_PATH	"/dev/pwm_output"

/**
 * Maximum number of PWM output channels in the system.
 */
#define PWM_OUTPUT_MAX_CHANNELS	16

/**
 * Servo output signal type, value is actual servo output pulse
 * width in microseconds.
 */
typedef uint16_t	servo_position_t;

/**
 * Servo output status structure.
 *
 * May be published to output_pwm, or written to a PWM output
 * device.
 */
struct pwm_output_values {
	/** desired servo update rate in Hz */
	uint32_t		update_rate;

	/** desired pulse widths for each of the supported channels */
	servo_position_t	values[PWM_OUTPUT_MAX_CHANNELS];
};

/*
 * ObjDev tag for PWM outputs.
 */
ORB_DECLARE(output_pwm);

/*
 * ioctl() definitions
 *
 * Note that ioctls and ObjDev updates should not be mixed, as the
 * behaviour of the system in this case is not defined.
 */
#define _PWM_SERVO_BASE		0x2700

/** arm all servo outputs handle by this driver */
#define PWM_SERVO_ARM		_IOC(_PWM_SERVO_BASE, 0)

/** disarm all servo outputs (stop generating pulses) */
#define PWM_SERVO_DISARM	_IOC(_PWM_SERVO_BASE, 1)

/** set a single servo to a specific value */
#define PWM_SERVO_SET(_servo)	_IOC(_PWM_SERVO_BASE, 0x20 + _servo)

/** get a single specific servo value */
#define PWM_SERVO_GET(_servo)	_IOC(_PWM_SERVO_BASE, 0x40 + _servo)


#endif /* _DRV_PWM_OUTPUT_H */
