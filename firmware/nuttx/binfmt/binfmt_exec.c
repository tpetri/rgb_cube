/****************************************************************************
 * binfmt/binfmt_exec.c
 *
 *   Copyright (C) 2009 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
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

#include <string.h>
#include <sched.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/binfmt.h>

#include "binfmt_internal.h"

#ifndef CONFIG_BINFMT_DISABLE

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: exec
 *
 * Description:
 *   This is a convenience function that wraps load_ and exec_module into
 *   one call.
 *
 * Input Parameter:
 *   filename - Fulll path to the binary to be loaded
 *   argv     - Argument list
 *   exports  - Table of exported symbols
 *   nexports - The number of symbols in exports
 *
 * Returned Value:
 *   This is an end-user function, so it follows the normal convention:
 *   Returns the PID of the exec'ed module.  On failure, it.returns
 *   -1 (ERROR) and sets errno appropriately.
 *
 ****************************************************************************/

int exec(FAR const char *filename, FAR const char **argv,
         FAR const struct symtab_s *exports, int nexports)
{
  struct binary_s bin;
  int ret;

  memset(&bin, 0, sizeof(struct binary_s));
  bin.filename = filename;
  bin.exports  = exports;
  bin.nexports = nexports;

  ret = load_module(&bin);
  if (ret < 0)
    {
      bdbg("ERROR: Failed to load program '%s'\n", filename);
      return ERROR;
    }

  ret = exec_module(&bin, 50);
  if (ret < 0)
    {
      bdbg("ERROR: Failed to execute program '%s'\n", filename);
      unload_module(&bin);
      return ERROR;
    }

  return ret;
}

#endif /* CONFIG_BINFMT_DISABLE */

