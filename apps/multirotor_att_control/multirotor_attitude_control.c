/****************************************************************************
 *
 *   Copyright (C) 2008-2012 PX4 Development Team. All rights reserved.
 *   Author: @author Thomas Gubler <thomasgubler@student.ethz.ch>
 *           @author Julian Oes <joes@student.ethz.ch>
 *           @author Laurens Mackay <mackayl@student.ethz.ch>
 *           @author Tobias Naegeli <naegelit@student.ethz.ch>
 *           @author Martin Rutschmann <rutmarti@student.ethz.ch>
 *           @author Lorenz Meier <lm@inf.ethz.ch>
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

/*
 * @file multirotor_attitude_control.c
 * Implementation of attitude controller
 */

#include "multirotor_attitude_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <float.h>
#include <math.h>
#include <systemlib/pid/pid.h>
#include <systemlib/param/param.h>
#include <arch/board/up_hrt.h>

// PARAM_DEFINE_FLOAT(MC_YAWPOS_P, 0.3f);
// PARAM_DEFINE_FLOAT(MC_YAWPOS_I, 0.15f);
// PARAM_DEFINE_FLOAT(MC_YAWPOS_D, 0.0f);
// PARAM_DEFINE_FLOAT(MC_YAWPOS_AWU, 1.0f);
// PARAM_DEFINE_FLOAT(MC_YAWPOS_LIM, 3.0f);

PARAM_DEFINE_FLOAT(MC_YAWRATE_P, 0.08f); /* same on Flamewheel */
PARAM_DEFINE_FLOAT(MC_YAWRATE_I, 0.02f);
PARAM_DEFINE_FLOAT(MC_YAWRATE_D, 0.0f);
PARAM_DEFINE_FLOAT(MC_YAWRATE_AWU, 0.02f);
PARAM_DEFINE_FLOAT(MC_YAWRATE_LIM, 0.1f);

PARAM_DEFINE_FLOAT(MC_ATT_P, 0.2f); /* 0.15 F405 Flamewheel */
PARAM_DEFINE_FLOAT(MC_ATT_I, 0.0f);
PARAM_DEFINE_FLOAT(MC_ATT_D, 0.05f); /* 0.025 F405 Flamewheel */
PARAM_DEFINE_FLOAT(MC_ATT_AWU, 0.05f);
PARAM_DEFINE_FLOAT(MC_ATT_LIM, 0.4f);

PARAM_DEFINE_FLOAT(MC_ATT_XOFF, 0.0f);
PARAM_DEFINE_FLOAT(MC_ATT_YOFF, 0.0f);

struct mc_att_control_params {
	// float yaw_p;
	// float yaw_i;
	// float yaw_d;
	// float yaw_awu;
	// float yaw_lim;

	float yawrate_p;
	float yawrate_i;
	float yawrate_d;
	float yawrate_awu;
	float yawrate_lim;

	float att_p;
	float att_i;
	float att_d;
	float att_awu;
	float att_lim;

	float att_xoff;
	float att_yoff;
};

struct mc_att_control_param_handles {
	// param_t yaw_p;
	// param_t yaw_i;
	// param_t yaw_d;
	// param_t yaw_awu;
	// param_t yaw_lim;

	param_t yawrate_p;
	param_t yawrate_i;
	param_t yawrate_d;
	param_t yawrate_awu;
	param_t yawrate_lim;

	param_t att_p;
	param_t att_i;
	param_t att_d;
	param_t att_awu;
	param_t att_lim;

	param_t att_xoff;
	param_t att_yoff;
};

/**
 * Initialize all parameter handles and values
 *
 */
static int parameters_init(struct mc_att_control_param_handles *h);

/**
 * Update all parameters
 *
 */
static int parameters_update(const struct mc_att_control_param_handles *h, struct mc_att_control_params *p);


static int parameters_init(struct mc_att_control_param_handles *h)
{
	/* PID parameters */
	// h->yaw_p 	=	param_find("MC_YAWPOS_P");
	// h->yaw_i 	=	param_find("MC_YAWPOS_I");
	// h->yaw_d 	=	param_find("MC_YAWPOS_D");
	// h->yaw_awu 	=	param_find("MC_YAWPOS_AWU");
	// h->yaw_lim 	=	param_find("MC_YAWPOS_LIM");

	h->yawrate_p 	=	param_find("MC_YAWRATE_P");
	h->yawrate_i 	=	param_find("MC_YAWRATE_I");
	h->yawrate_d 	=	param_find("MC_YAWRATE_D");
	h->yawrate_awu 	=	param_find("MC_YAWRATE_AWU");
	h->yawrate_lim 	=	param_find("MC_YAWRATE_LIM");

	h->att_p 	= 	param_find("MC_ATT_P");
	h->att_i 	= 	param_find("MC_ATT_I");
	h->att_d 	= 	param_find("MC_ATT_D");
	h->att_awu 	= 	param_find("MC_ATT_AWU");
	h->att_lim 	= 	param_find("MC_ATT_LIM");

	h->att_xoff 	= 	param_find("MC_ATT_XOFF");
	h->att_yoff 	= 	param_find("MC_ATT_YOFF");

	return OK;
}

static int parameters_update(const struct mc_att_control_param_handles *h, struct mc_att_control_params *p)
{
	// param_get(h->yaw_p, &(p->yaw_p));
	// param_get(h->yaw_i, &(p->yaw_i));
	// param_get(h->yaw_d, &(p->yaw_d));
	// param_get(h->yaw_awu, &(p->yaw_awu));
	// param_get(h->yaw_lim, &(p->yaw_lim));

	param_get(h->yawrate_p, &(p->yawrate_p));
	param_get(h->yawrate_i, &(p->yawrate_i));
	param_get(h->yawrate_d, &(p->yawrate_d));
	param_get(h->yawrate_awu, &(p->yawrate_awu));
	param_get(h->yawrate_lim, &(p->yawrate_lim));

	param_get(h->att_p, &(p->att_p));
	param_get(h->att_i, &(p->att_i));
	param_get(h->att_d, &(p->att_d));
	param_get(h->att_awu, &(p->att_awu));
	param_get(h->att_lim, &(p->att_lim));

	param_get(h->att_xoff, &(p->att_xoff));
	param_get(h->att_yoff, &(p->att_yoff));

	return OK;
}

void multirotor_control_attitude(const struct vehicle_attitude_setpoint_s *att_sp,
	const struct vehicle_attitude_s *att, struct vehicle_rates_setpoint_s *rates_sp, struct actuator_controls_s *actuators)
{
	static uint64_t last_run = 0;
	const float deltaT = (hrt_absolute_time() - last_run) / 1000000.0f;
	last_run = hrt_absolute_time();

	static int motor_skip_counter = 0;

	// static PID_t yaw_pos_controller;
	static PID_t yaw_speed_controller;
	static PID_t pitch_controller;
	static PID_t roll_controller;

	static struct mc_att_control_params p;
	static struct mc_att_control_param_handles h;

	static bool initialized = false;

	/* initialize the pid controllers when the function is called for the first time */
	if (initialized == false) {
		parameters_init(&h);
		parameters_update(&h, &p);

		// pid_init(&yaw_pos_controller, p.yaw_p, p.yaw_i, p.yaw_d, p.yaw_awu,
		// 	PID_MODE_DERIVATIV_SET, 154);
		pid_init(&yaw_speed_controller, p.yawrate_p, p.yawrate_i, p.yawrate_d, p.yawrate_awu,
			PID_MODE_DERIVATIV_SET);
		pid_init(&pitch_controller, p.att_p, p.att_i, p.att_d, p.att_awu,
			PID_MODE_DERIVATIV_SET);
		pid_init(&roll_controller, p.att_p, p.att_i, p.att_d, p.att_awu,
			PID_MODE_DERIVATIV_SET);

		initialized = true;
	}

	/* load new parameters with lower rate */
	if (motor_skip_counter % 50 == 0) {
		/* update parameters from storage */
		parameters_update(&h, &p);
		/* apply parameters */
		// pid_set_parameters(&yaw_pos_controller, p.yaw_p, p.yaw_i, p.yaw_d, p.yaw_awu);
		pid_set_parameters(&yaw_speed_controller, p.yawrate_p, p.yawrate_i, p.yawrate_d, p.yawrate_awu);
		pid_set_parameters(&pitch_controller, p.att_p, p.att_i, p.att_d, p.att_awu);
		pid_set_parameters(&roll_controller, p.att_p, p.att_i, p.att_d, p.att_awu);
	}

	/* calculate current control outputs */
	
	/* control pitch (forward) output */
	float pitch_control = pid_calculate(&pitch_controller, att_sp->pitch_body + p.att_xoff,
					att->pitch, att->pitchspeed, deltaT);
	/* control roll (left/right) output */
	float roll_control = pid_calculate(&roll_controller, att_sp->roll_body + p.att_yoff,
					att->roll, att->rollspeed, deltaT);
	/* control yaw rate */
	float yaw_rate_control = pid_calculate(&yaw_speed_controller, att_sp->yaw_body, att->yawspeed, 0.0f, deltaT);

	/*
	 * compensate the vertical loss of thrust
	 * when thrust plane has an angle.
	 * start with a factor of 1.0 (no change)
	 */
	float zcompensation = 1.0f;

	if (fabsf(att->roll) > 0.3f) {
		zcompensation *= 1.04675160154f;

	} else {
		zcompensation *= 1.0f / cosf(att->roll);
	}

	if (fabsf(att->pitch) > 0.3f) {
		zcompensation *= 1.04675160154f;

	} else {
		zcompensation *= 1.0f / cosf(att->pitch);
	}

	float motor_thrust = 0.0f;

	motor_thrust = att_sp->thrust;

	/* compensate thrust vector for roll / pitch contributions */
	motor_thrust *= zcompensation;

	/* limit yaw rate output */
	if (yaw_rate_control > p.yawrate_lim) {
		yaw_rate_control = p.yawrate_lim;
		yaw_speed_controller.saturated = 1;
	}

	if (yaw_rate_control < -p.yawrate_lim) {
		yaw_rate_control = -p.yawrate_lim;
		yaw_speed_controller.saturated = 1;
	}

	if (pitch_control > p.att_lim) {
		pitch_control = p.att_lim;
		pitch_controller.saturated = 1;
	}

	if (pitch_control < -p.att_lim) {
		pitch_control = -p.att_lim;
		pitch_controller.saturated = 1;
	}


	if (roll_control > p.att_lim) {
		roll_control = p.att_lim;
		roll_controller.saturated = 1;
	}

	if (roll_control < -p.att_lim) {
		roll_control = -p.att_lim;
		roll_controller.saturated = 1;
	}

	if (actuators) {
		actuators->control[0] = roll_control;
		actuators->control[1] = pitch_control;
		actuators->control[2] = yaw_rate_control;
		actuators->control[3] = motor_thrust;
	}

	// XXX change yaw rate to yaw pos controller
	if (rates_sp) {
		rates_sp->roll = roll_control;
		rates_sp->pitch = pitch_control;
		rates_sp->yaw = yaw_rate_control;
		rates_sp->thrust = motor_thrust;
	}

	motor_skip_counter++;
}
