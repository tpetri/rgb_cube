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
 * @file objects_common.h
 *
 * Common object definitions without a better home.
 */

#include <nuttx/config.h>

#include <drivers/drv_orb_dev.h>

#include <drivers/drv_pwm_output.h>
ORB_DEFINE(output_pwm, struct pwm_output_values);

#include <drivers/drv_rc_input.h>
ORB_DEFINE(input_rc, struct rc_input_values);

// XXX need to check wether these should be here
#include "topics/vehicle_attitude.h"
ORB_DEFINE(vehicle_attitude, struct vehicle_attitude_s);

#include "topics/sensor_combined.h"
ORB_DEFINE(sensor_combined, struct sensor_combined_s);

#include "topics/vehicle_gps_position.h"
ORB_DEFINE(vehicle_gps_position, struct vehicle_gps_position_s);

#include "topics/vehicle_status.h"
ORB_DEFINE(vehicle_status, struct vehicle_status_s);

#include "topics/vehicle_global_position.h"
ORB_DEFINE(vehicle_global_position, struct vehicle_global_position_s);

#include "topics/vehicle_local_position.h"
ORB_DEFINE(vehicle_local_position, struct vehicle_local_position_s);

#include "topics/ardrone_control.h"
ORB_DEFINE(ardrone_control, struct ardrone_control_s);

#include "topics/ardrone_motors_setpoint.h"
ORB_DEFINE(ardrone_motors_setpoint, struct ardrone_motors_setpoint_s);

#include "topics/rc_channels.h"
ORB_DEFINE(rc_channels, struct rc_channels_s);

#include "topics/fixedwing_control.h"
ORB_DEFINE(fixedwing_control, struct fixedwing_control_s);

#include "topics/vehicle_command.h"
ORB_DEFINE(vehicle_command, struct vehicle_command_s);

#include "topics/vehicle_local_position_setpoint.h"
ORB_DEFINE(vehicle_local_position_setpoint, struct vehicle_local_position_setpoint_s);

#include "topics/vehicle_global_position_setpoint.h"
ORB_DEFINE(vehicle_global_position_setpoint, struct vehicle_global_position_setpoint_s);

#include "topics/vehicle_attitude_setpoint.h"
ORB_DEFINE(vehicle_attitude_setpoint, struct vehicle_attitude_setpoint_s);

#include "topics/manual_control_setpoint.h"
ORB_DEFINE(manual_control_setpoint, struct manual_control_setpoint_s);

#include "topics/optical_flow.h"
ORB_DEFINE(optical_flow, struct optical_flow_s);

#include "topics/actuator_controls.h"
ORB_DEFINE(actuator_controls_0, struct actuator_controls_s);
ORB_DEFINE(actuator_controls_1, struct actuator_controls_s);
ORB_DEFINE(actuator_controls_2, struct actuator_controls_s);
ORB_DEFINE(actuator_controls_3, struct actuator_controls_s);
ORB_DEFINE(actuator_armed, struct actuator_armed_s);
