/****************************************************************************
 *
 *   Copyright (C) 2008-2012 PX4 Development Team. All rights reserved.
 *   Author: @author Nils Wenzler <wenzlern@student.ethz.ch>
 *           @author Ivan Ovinnikov <oivan@student.ethz.ch>
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

/**
 * @file rc_channels.h
 * Definition of the rc_channels uORB topic.
 */

#ifndef RC_CHANNELS_H_
#define RC_CHANNELS_H_

#include <stdint.h>
#include "../uORB.h"

/**
 * @addtogroup topics
 * @{
 */

enum RC_CHANNELS_STATUS
{
	UNKNOWN = 0,
	KNOWN = 1,
	SIGNAL = 2,
	TIMEOUT = 3
};

/** 
 * This defines the mapping of the RC functions.
 * The value assigned to the specific function corresponds to the entry of
 * the channel array chan[].
 * Ex. To read out the scaled Pitch value:
 * pitch = global_data_rc_channels->chan[PITCH].scale;
 * The override is on channel 8, since we don't usually have a 12 channel RC
 * and channel 5/6 (GRAUPNER/FUTABA) are mapped to the second ROLL servo, which
 * can only be disabled on more advanced RC sets.
 */
enum RC_CHANNELS_FUNCTION
{
  THROTTLE = 0,
  ROLL     = 1,
  PITCH    = 2,
  YAW      = 3,
  OVERRIDE = 4,
  FUNC_0   = 5,
  FUNC_1   = 6,
  FUNC_2   = 7,
  FUNC_3   = 8,
  FUNC_4   = 9,
  FUNC_5   = 10,
  FUNC_6   = 11,
  RC_CHANNELS_FUNCTION_MAX = 12
};

struct rc_channels_s {

	uint64_t timestamp;                 /**< In microseconds since boot time. */
  uint64_t timestamp_last_valid;      /**< timestamp of last valid RC signal. */
  struct {
    uint16_t mid;                     /**< midpoint (0). */
    float scaling_factor;             /**< scaling factor from raw counts to 0..1 */
    uint16_t raw;                     /**< current raw value */
    int16_t scale;
    float scaled;                     /**< Scaled  */
    uint16_t override;
    enum RC_CHANNELS_STATUS status;   /**< status of the channel */
  } chan[RC_CHANNELS_FUNCTION_MAX];
  uint8_t chan_count;                    /**< maximum number of valid channels */

  /*String array to store the names of the functions*/
  const char function_name[RC_CHANNELS_FUNCTION_MAX][20];
  uint8_t function[RC_CHANNELS_FUNCTION_MAX];
  uint8_t rssi;                       /**< Overall receive signal strength */
}; /**< radio control channels. */

/**
 * @}
 */

/* register this as object request broker structure */
ORB_DECLARE(rc_channels);

#endif
