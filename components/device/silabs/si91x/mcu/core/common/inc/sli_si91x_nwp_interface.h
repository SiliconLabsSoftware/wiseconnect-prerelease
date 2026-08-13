/***************************************************************************/ /**
 * @file  sli_si91x_nwp_interface.h
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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
#ifndef SLI_SI91X_NWP_INTERFACE_H
#define SLI_SI91X_NWP_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>
#include "sl_status.h"

/***************************************************************************/ /**
 * @brief
 *   SOC helpers for flash/TX command status and the M4 <-> NWP interface.
 ******************************************************************************/

/* True if a flash command is in progress. */
bool sli_si91x_get_flash_command_status(void);

/* Set the flash command in-progress flag. */
void sli_si91x_update_flash_command_status(bool flag);

/* True if a TX command is in progress. */
bool sli_si91x_get_tx_command_status(void);

/* Set the TX command in-progress flag. */
void sli_si91x_update_tx_command_status(bool flag);

/* Unmask NWP -> M4 P2P interrupts. */
void unmask_ta_interrupt(uint32_t interrupt_no);

/* Mask NWP -> M4 P2P interrupts. */
void mask_ta_interrupt(uint32_t interrupt_no);

/* Raise an M4 -> TA P2P interrupt. */
void raise_m4_to_ta_interrupt(uint32_t interrupt_no);

/* Wake the NWP and wait until it is active. */
sl_status_t sli_si91x_req_wakeup(void);

/* Clear the M4->NWP wakeup indicator so the NWP may sleep. */
void sl_si91x_host_clear_sleep_indicator(void);

#endif // SLI_SI91X_NWP_INTERFACE_H
