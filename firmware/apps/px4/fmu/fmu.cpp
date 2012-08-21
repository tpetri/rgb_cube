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
 * @file fmu.cpp
 *
 * Driver/configurator for the PX4 FMU multi-purpose port.
 */

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>

#include <nuttx/arch.h>

#include <drivers/device/device.h>
#include <drivers/drv_pwm_output.h>
#include <drivers/drv_gpio.h>

#include <systemlib/mixer/mixer.h>
#include <drivers/drv_mixer.h>

#include <arch/board/up_pwm_servo.h>

#include <uORB/topics/actuator_controls.h>


class FMUServo : public device::CDev
{
public:
	enum Mode {
		MODE_2PWM,
		MODE_4PWM,
		MODE_NONE
	};
	FMUServo(Mode mode);
	~FMUServo();

	virtual int	ioctl(struct file *filp, int cmd, unsigned long arg);

	virtual int	init();

private:
	static const unsigned _max_actuators = 4;

	Mode		_mode;
	int		_task;
	int		_t_actuators;
	int		_t_armed;
	unsigned	_num_outputs;

	volatile bool	_task_should_exit;
	bool		_armed;

	MixerGroup	*_mixers;

	actuator_controls_s _controls;

	static void	task_main_trampoline(int argc, char *argv[]);
	void		task_main();

	static int	control_callback_trampoline(uintptr_t handle,
			uint8_t control_group,
			uint8_t control_index,
			float &input);
	int		control_callback(uint8_t control_group,
					 uint8_t control_index,
					 float &input);
};

namespace
{

FMUServo	*g_servo;

} // namespace

FMUServo::FMUServo(Mode mode) :
	CDev("fmuservo", PWM_OUTPUT_DEVICE_PATH),
	_mode(mode),
	_task(-1),
	_t_actuators(-1),
	_t_armed(-1),
	_task_should_exit(false),
	_armed(false),
	_mixers(nullptr)
{
}

FMUServo::~FMUServo()
{
	if (_task != -1) {

		/* task should wake up every 100ms or so at least */
		_task_should_exit = true;

		unsigned i = 0;

		do {
			/* wait 20ms */
			usleep(20000);

			/* if we have given up, kill it */
			if (++i > 10) {
				task_delete(_task);
				break;
			}

		} while (_task != -1);
	}

	g_servo = nullptr;
}

int
FMUServo::init()
{
	int ret;

	ASSERT(_task == -1);

	/* do regular cdev init */
	ret = CDev::init();

	if (ret != OK)
		return ret;

	/* start the IO interface task */
	_task = task_create("fmuservo", SCHED_PRIORITY_DEFAULT, 1024, (main_t)&FMUServo::task_main_trampoline, nullptr);

	if (_task < 0) {
		debug("task start failed: %d", errno);
		return -errno;
	}

	return OK;
}

void
FMUServo::task_main_trampoline(int argc, char *argv[])
{
	g_servo->task_main();
}

void
FMUServo::task_main()
{
	/* configure for PWM output */
	switch (_mode) {
	case MODE_2PWM:
		/* multi-port with flow control lines as PWM */
		/* XXX magic numbers */
		up_pwm_servo_init(0x3);
		break;

	case MODE_4PWM:
		/* multi-port as 4 PWM outs */
		/* XXX magic numbers */
		up_pwm_servo_init(0xf);
		break;

	case MODE_NONE:
		/* we should never get here... */
		break;
	}

	/* subscribe to objects that we are interested in watching */
	_t_actuators = orb_subscribe(ORB_ID_VEHICLE_ATTITUDE_CONTROLS);
	orb_set_interval(_t_actuators, 20);		/* 50Hz update rate */

	_t_armed = orb_subscribe(ORB_ID(actuator_armed));
	orb_set_interval(_t_armed, 100);		/* 10Hz update rate */

	struct pollfd fds[2];
	fds[0].fd = _t_actuators;
	fds[0].events = POLLIN;
	fds[1].fd = _t_armed;
	fds[1].events = POLLIN;

	unsigned num_outputs = (_mode == MODE_2PWM) ? 2 : 4;

	log("starting");

	/* loop until killed */
	while (!_task_should_exit) {

		/* sleep waiting for data, but no more than 100ms */
		int ret = ::poll(&fds[0], 2, 1000);

		/* this would be bad... */
		if (ret < 0) {
			log("poll error %d", errno);
			usleep(1000000);
			continue;
		}

		/* do we have a control update? */
		if (fds[0].revents & POLLIN) {
			float outputs[num_outputs];

			/* get controls - must always do this to avoid spinning */
			orb_copy(ORB_ID_VEHICLE_ATTITUDE_CONTROLS, _t_actuators, &_controls);

			/* can we mix? */
			if (_mixers != nullptr) {

				/* do mixing */
				_mixers->mix(&outputs[0], num_outputs);

				/* iterate actuators */
				for (unsigned i = 0; i < num_outputs; i++) {

					/* scale for PWM output 900 - 2100us */
					up_pwm_servo_set(i, 1500 + (600 * outputs[i]));
				}
			}
		}

		/* how about an arming update? */
		if (fds[1].revents & POLLIN) {
			struct actuator_armed_s aa;

			/* get new value */
			orb_copy(ORB_ID(actuator_armed), _t_armed, &aa);

			/* update PMW servo armed status */
			up_pwm_servo_arm(aa.armed);
		}
	}

	::close(_t_actuators);
	::close(_t_armed);

	/* make sure servos are off */
	up_pwm_servo_deinit();

	log("stopping");

	/* note - someone else is responsible for restoring the GPIO config */

	/* tell the dtor that we are exiting */
	_task = -1;
	_exit(0);
}

int
FMUServo::control_callback_trampoline(uintptr_t handle,
				      uint8_t control_group,
				      uint8_t control_index,
				      float &input)
{
	return ((FMUServo *)handle)->control_callback(control_group, control_index, input);
}

int
FMUServo::control_callback(uint8_t control_group,
			   uint8_t control_index,
			   float &input)
{
	/* XXX currently only supporting group zero */
	if ((control_group != 0) || (control_index > NUM_ACTUATOR_CONTROLS))
		return -1;

	input = _controls.control[control_index];
	return 0;
}

int
FMUServo::ioctl(struct file *filp, int cmd, unsigned long arg)
{
	int ret = OK;
	int channel;

	switch (cmd) {
	case PWM_SERVO_ARM:
		up_pwm_servo_arm(true);
		break;

	case PWM_SERVO_DISARM:
		up_pwm_servo_arm(false);
		break;

	case PWM_SERVO_SET(2):
	case PWM_SERVO_SET(3):
		if (_mode != MODE_4PWM) {
			ret = -EINVAL;
			break;
		}

		/* FALLTHROUGH */
	case PWM_SERVO_SET(0):
	case PWM_SERVO_SET(1):
		if (arg < 2100) {
			channel = cmd - PWM_SERVO_SET(0);
			up_pwm_servo_set(channel, arg);

		} else {
			ret = -EINVAL;
		}

		break;

	case PWM_SERVO_GET(2):
	case PWM_SERVO_GET(3):
		if (_mode != MODE_4PWM) {
			ret = -EINVAL;
			break;
		}

		/* FALLTHROUGH */
	case PWM_SERVO_GET(0):
	case PWM_SERVO_GET(1): {
			channel = cmd - PWM_SERVO_SET(0);
			*(servo_position_t *)arg = up_pwm_servo_get(channel);
			break;
		}

	case MIXERIOCGETOUTPUTCOUNT:
		if (_mode == MODE_4PWM) {
			*(unsigned *)arg = 4;

		} else {
			*(unsigned *)arg = 2;
		}

		break;

	case MIXERIOCRESET:
		if (_mixers != nullptr) {
			delete _mixers;
			_mixers = nullptr;
		}

		break;

	case MIXERIOCADDSIMPLE: {
			mixer_simple_s *mixinfo = (mixer_simple_s *)arg;

			SimpleMixer *mixer = new SimpleMixer(control_callback_trampoline, (uintptr_t)this, mixinfo);

			if (mixer->check()) {
				delete mixer;
				ret = -EINVAL;

			} else {
				if (_mixers == nullptr)
					_mixers = new MixerGroup(control_callback_trampoline, (uintptr_t)this);

				_mixers->add_mixer(mixer);
			}

			break;
		}

	case MIXERIOCADDMULTIROTOR:
		/* XXX not yet supported */
		ret = -ENOTTY;
		break;

	case MIXERIOCLOADFILE: {
			const char *path = (const char *)arg;

			if (_mixers != nullptr) {
				delete _mixers;
				_mixers = nullptr;
			}

			_mixers = new MixerGroup(control_callback_trampoline, (uintptr_t)this);

			if (_mixers->load_from_file(path) != 0) {
				delete _mixers;
				_mixers = nullptr;
				ret = -EINVAL;
			}

			break;
		}

	default:
		ret = -ENOTTY;
		break;
	}

	return ret;
}

namespace
{

enum PortMode {
	PORT_MODE_UNSET = 0,
	PORT_FULL_GPIO,
	PORT_FULL_SERIAL,
	PORT_FULL_PWM,
	PORT_GPIO_AND_SERIAL,
	PORT_PWM_AND_SERIAL,
	PORT_PWM_AND_GPIO,
};

PortMode g_port_mode;

int
fmu_new_mode(PortMode new_mode)
{
	int fd;
	int ret = OK;
	uint32_t gpio_bits;
	FMUServo::Mode servo_mode;

	/* get hold of the GPIO configuration descriptor */
	fd = open(GPIO_DEVICE_PATH, 0);

	if (fd < 0)
		return -errno;

	/* start by tearing down any existing state and revert to all-GPIO-inputs */
	if (g_servo != nullptr) {
		delete g_servo;
		g_servo = nullptr;
	}

	/* reset to all-inputs */
	ioctl(fd, GPIO_RESET, 0);

	gpio_bits = 0;
	servo_mode = FMUServo::MODE_NONE;

	switch (new_mode) {
	case PORT_FULL_GPIO:
	case PORT_MODE_UNSET:
		/* nothing more to do here */
		break;

	case PORT_FULL_SERIAL:
		/* set all multi-GPIOs to serial mode */
		gpio_bits = GPIO_MULTI_1 | GPIO_MULTI_2 | GPIO_MULTI_3 | GPIO_MULTI_4;
		break;

	case PORT_FULL_PWM:
		/* select 4-pin PWM mode */
		servo_mode = FMUServo::MODE_4PWM;
		break;

	case PORT_GPIO_AND_SERIAL:
		/* set RX/TX multi-GPIOs to serial mode */
		gpio_bits = GPIO_MULTI_3 | GPIO_MULTI_4;
		break;

	case PORT_PWM_AND_SERIAL:
		/* select 2-pin PWM mode */
		servo_mode = FMUServo::MODE_2PWM;
		/* set RX/TX multi-GPIOs to serial mode */
		gpio_bits = GPIO_MULTI_3 | GPIO_MULTI_4;
		break;

	case PORT_PWM_AND_GPIO:
		/* select 2-pin PWM mode */
		servo_mode = FMUServo::MODE_2PWM;
		break;
	}

	/* adjust GPIO config for serial mode(s) */
	if (gpio_bits != 0)
		ioctl(fd, GPIO_SET_ALT_1, gpio_bits);

	close(fd);

	/* create new PWM driver if required */
	if (servo_mode != FMUServo::MODE_NONE) {
		g_servo = new FMUServo(servo_mode);

		if (g_servo == nullptr) {
			ret = -ENOMEM;

		} else {
			ret = g_servo->init();

			if (ret != OK) {
				delete g_servo;
				g_servo = nullptr;
			}
		}
	}

	return ret;
}

void
test(void)
{
	int	fd;

	fd = open(PWM_OUTPUT_DEVICE_PATH, 0);

	if (fd < 0) {
		puts("open fail");
		exit(1);
	}

	ioctl(fd, PWM_SERVO_ARM, 0);
	ioctl(fd, PWM_SERVO_SET(0), 1000);

	close(fd);

	exit(0);
}

void
fake(int argc, char *argv[])
{
	if (argc < 5) {
		puts("fmu fake <roll> <pitch> <yaw> <thrust> (values -100 - 100)");
		exit(1);
	}

	struct actuator_controls_s ac;

	ac.control[0] = strtol(argv[1], 0, 0) / 100.0f;

	ac.control[1] = strtol(argv[2], 0, 0) / 100.0f;

	ac.control[2] = strtol(argv[3], 0, 0) / 100.0f;

	ac.control[3] = strtol(argv[4], 0, 0) / 100.0f;

	int handle = orb_advertise(ORB_ID_VEHICLE_ATTITUDE_CONTROLS, &ac);

	if (handle < 0) {
		puts("advertise failed");
		exit(1);
	}

	close(handle);

	exit(0);
}

} // namespace

extern "C" __EXPORT int fmu_main(int argc, char *argv[]);

int
fmu_main(int argc, char *argv[])
{
	PortMode new_mode = PORT_MODE_UNSET;

	if (!strcmp(argv[1], "test"))
		test();

	if (!strcmp(argv[1], "fake"))
		fake(argc - 1, argv + 1);

	/*
	 * Mode switches.
	 *
	 * XXX use getopt?
	 */
	if (!strcmp(argv[1], "mode_gpio")) {
		new_mode = PORT_FULL_GPIO;

	} else if (!strcmp(argv[1], "mode_serial")) {
		new_mode = PORT_FULL_SERIAL;

	} else if (!strcmp(argv[1], "mode_pwm")) {
		new_mode = PORT_FULL_PWM;

	} else if (!strcmp(argv[1], "mode_gpio_serial")) {
		new_mode = PORT_GPIO_AND_SERIAL;

	} else if (!strcmp(argv[1], "mode_pwm_serial")) {
		new_mode = PORT_PWM_AND_SERIAL;

	} else if (!strcmp(argv[1], "mode_pwm_gpio")) {
		new_mode = PORT_PWM_AND_GPIO;
	}

	/* was a new mode set? */
	if (new_mode != PORT_MODE_UNSET) {

		/* yes but it's the same mode */
		if (new_mode == g_port_mode)
			return OK;

		/* switch modes */
		return fmu_new_mode(new_mode);
	}

	/* test, etc. here */

	fprintf(stderr, "FMU: unrecognised command, try:\n");
	fprintf(stderr, "  mode_gpio, mode_serial, mode_pwm, mode_gpio_serial, mode_pwm_serial, mode_pwm_gpio\n");
	return -EINVAL;
}
