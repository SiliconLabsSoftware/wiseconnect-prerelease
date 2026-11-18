/***************************************************************************/ /**
 * @file
 * @brief SL Diagnostic SiWx91x platform integration (internal)
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

#ifndef SLI_SI91X_DIAG_H
#define SLI_SI91X_DIAG_H

#include <stdint.h>

#include "sli_diag.h"

#ifdef __cplusplus
extern "C" {
#endif

#if SL_DIAG_ENABLE

/***************************************************************************/ /**
 * Latches the SiWx91x reset cause.
 *
 * reset_reason is the raw MCU_FSM_WAKEUP_STATUS_REG: wakeup source, MCU and
 * NWP watchdog resets, host reset request, and the first power-up flags.
 * reset_reason_ext is the raw MCUAON_KHZ_CLK_SEL_POR_RESET_STATUS, whose
 * first power-up flags are cleared by Vbatt removal and by the reset pin.
 *
 * @param[out] out Reset cause. Ignored if NULL.
 ******************************************************************************/
void sli_si91x_diag_latch_reset_boot(sli_diag_reset_boot_t *out);

#endif // SL_DIAG_ENABLE

#ifdef __cplusplus
}
#endif

#endif // SLI_SI91X_DIAG_H
