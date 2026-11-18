/***************************************************************************/ /**
 * @file
 * @brief SL Diagnostic SiWx91x platform integration
 *******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#include <stddef.h>
#include <stdint.h>

#if defined(SL_COMPONENT_CATALOG_PRESENT)
#include "sl_component_catalog.h"
#endif
#include "si91x_device.h"
#include "sl_diag.h"
#include "sli_diag.h"
#include "sli_si91x_diag.h"

#if SL_DIAG_ENABLE

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************/ /**
 * Latches the SiWx91x reset cause.
 ******************************************************************************/
void sli_si91x_diag_latch_reset_boot(sli_diag_reset_boot_t *out)
{
  if (out == NULL) {
    return;
  }
  out->reset_reason     = MCU_FSM->MCU_FSM_WAKEUP_STATUS_REG;
  out->reset_reason_ext = MCU_AON->MCUAON_KHZ_CLK_SEL_POR_RESET_STATUS;
}

/*******************************************************************************
 *********************   SL_DIAG PLATFORM HOOKS   ******************************
 ******************************************************************************/

// The names below are fixed by sl_diag and must not carry the si91x prefix.
// The uptime source is the weak sl_diag_get_uptime_ms() default, which reads
// the Sleep Timer, saturates, and stays overridable by the application.

/***************************************************************************/ /**
 * Implements the sl_diag reset-cause platform hook.
 ******************************************************************************/
void sli_platform_latch_reset_boot(sli_diag_reset_boot_t *out)
{
  sli_si91x_diag_latch_reset_boot(out);
}

#endif // SL_DIAG_ENABLE
