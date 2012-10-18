/****************************************************************************
 * sched/env_setenv.c
 *
 *   Copyright (C) 2007, 2009, 2011 Gregory Nutt. All rights reserved.
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

#ifndef CONFIG_DISABLE_ENVIRON

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <string.h>
#include <errno.h>

#include <nuttx/kmalloc.h>

#include "os_internal.h"
#include "env_internal.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: setenv
 *
 * Description:
 *   The setenv() function adds the variable name to the environment with the
 *   specified 'value' if the varialbe 'name" does not exist. If the 'name'
 *   does exist in the environment, then its value is changed to 'value' if
 *   'overwrite' is non-zero; if 'overwrite' is zero, then the value of name
 *   unaltered.
 *
 * Parameters:
 *   name - The name of the variable to change
 *   value - The new value of the variable
 *   overwrite - Replace any existing value if non-zero.
 *
 * Return Value:
 *   Zero on success
 *
 * Assumptions:
 *   Not called from an interrupt handler
 *
 ****************************************************************************/

int setenv(const char *name, const char *value, int overwrite)
{
  FAR _TCB      *rtcb;
  FAR environ_t *envp;
  FAR char      *pvar;
  int            varlen;
  int ret = OK;

  /* Verify input parameter */

  if (!name)
    {
      ret = EINVAL;
      goto errout;
    }

  /* if no value is provided, then this is the same as unsetenv (unless
   * overwrite is false)
   */

  if (!value || *value == '\0')
    {
      /* If overwite is set then this is the same as unsetenv */

      if (overwrite)
        {
          return unsetenv(name);
        }
      else
        {
          /* Otherwise, it is a request to remove a variable without altering it? */

          return OK;
        }
    }

  /* Get a reference to the thread-private environ in the TCB.*/

  sched_lock();
  rtcb = (FAR _TCB*)g_readytorun.head;
  envp = rtcb->envp;

  /* Check if the variable alreay exists */

  if ( envp && (pvar = env_findvar(envp, name)) != NULL)
    {
      /* It does!  Do we have permission to overwrite the existing value? */

      if (!overwrite)
        {
          /* No.. then just return success */

          sched_unlock();
          return OK;
        }

      /* Yes.. just remove the name=value pair from the environment.  It will
       * be added again below.  Note that we are responsible for reallocating
       * the environment buffer; this will happen below.
       */

      (void)env_removevar(envp, pvar);
    }

  /* Get the size of the new name=value string.  The +2 is for the '=' and for
   * null terminator
   */

  varlen = strlen(name) + strlen(value) + 2;

  /* Then allocate or reallocate the environment buffer */

  if (envp)
    {
      int        alloc = envp->ev_alloc;
      environ_t *tmp   = (environ_t*)krealloc(envp, SIZEOF_ENVIRON_T(alloc + varlen));
      if (!tmp)
        {
          ret = ENOMEM;
          goto errout_with_lock;
        }

      envp           = tmp;
      envp->ev_alloc = alloc + varlen;
      pvar           = &envp->ev_env[alloc];
    }
  else
    {
      envp = (environ_t*)kmalloc(SIZEOF_ENVIRON_T(varlen));
      if (!envp)
        {
          ret = ENOMEM;
          goto errout_with_lock;
        }

      envp->ev_crefs = 1;
      envp->ev_alloc = varlen;
      pvar           = envp->ev_env;
    }

  /* Now, put the new name=value string into the environment buffer */

  sprintf(pvar, "%s=%s", name, value);

  /* Save the new environment pointer (it might have changed due to allocation or
   * reallocation.
   */

  rtcb->envp = envp;
  sched_unlock();
  return OK;

errout_with_lock:
  sched_unlock();
errout:
  errno = ret;
  return ERROR;
}

#endif /* CONFIG_DISABLE_ENVIRON */



