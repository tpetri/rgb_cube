/****************************************************************************************************
 * arch/arm/src/stm32/stm32_syscfg.h
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
 ****************************************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32_STM32_SYSCFG_H
#define __ARCH_ARM_SRC_STM32_STM32_SYSCFG_H

/****************************************************************************************************
 * Included Files
 ****************************************************************************************************/

#include <nuttx/config.h>
#include "chip.h"

#if defined(CONFIG_STM32_STM32F20XX) || defined(CONFIG_STM32_STM32F40XX)
#  include "chip/stm32_syscfg.h"

/****************************************************************************************************
 * Pre-processor Definitions
 ****************************************************************************************************/

/****************************************************************************************************
 * Inline Functions
 ****************************************************************************************************/

/************************************************************************************
 * Name: stm32_selectmii
 *
 * Description:
 *   Selects the MII inteface.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/

static inline void stm32_selectmii(void)
{
  uint32_t regval;

  regval = getreg32(STM32_SYSCFG_PMC);
  regval &= ~SYSCFG_PMC_MII_RMII_SEL;
  putreg32(regval, STM32_SYSCFG_PMC);
}

/************************************************************************************
 * Name: stm32_selectrmii
 *
 * Description:
 *   Selects the RMII inteface.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/

static inline void stm32_selectrmii(void)
{
  uint32_t regval;

  regval = getreg32(STM32_SYSCFG_PMC);
  regval |= SYSCFG_PMC_MII_RMII_SEL;
  putreg32(regval, STM32_SYSCFG_PMC);
}

#endif /* CONFIG_STM32_STM32F20XX || CONFIG_STM32_STM32F40XX */
#endif /* __ARCH_ARM_SRC_STM32_STM32_SYSCFG_H */
