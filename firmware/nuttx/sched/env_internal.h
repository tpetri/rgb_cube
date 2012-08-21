/****************************************************************************
 * sched/env_internal.h
 *
 *   Copyright (C) 2007, 2009 Gregory Nutt. All rights reserved.
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

#ifndef __SCHED_ENV_INTERNAL_H
#define __SCHED_ENV_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/sched.h>
#include "os_internal.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

#ifdef CONFIG_DISABLE_ENVIRON
# define env_dup(ptcb)     (0)
# define env_share(ptcb)   (0)
# define env_release(ptcb) (0)
#endif

/****************************************************************************
 * Public Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

#ifndef CONFIG_DISABLE_ENVIRON
/* functions used by the task/pthread creation and destruction logic */

EXTERN int env_dup(FAR _TCB *ptcb);
EXTERN int env_share(FAR _TCB *ptcb);
EXTERN int env_release(FAR _TCB *ptcb);

/* functions used internally the environment handling logic */

EXTERN FAR char *env_findvar(environ_t *envp, const char *pname);
EXTERN int env_removevar(environ_t *envp, char *pvar);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __SCHED_ENV_INTERNAL_H */

