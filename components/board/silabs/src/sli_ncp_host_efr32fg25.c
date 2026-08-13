/***************************************************************************/ /**
 * @file  sli_ncp_host_efr32fg25.c
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

// NCP (EFR32FG25 custom host, brd4271a/brd4270a) implementation of the SiWx91x
// sleep indicator control. SLEEP_CONFIRM_PIN comes from the board
// configuration; GPIO_PinOutClear comes from emlib.
#include "em_gpio.h"
#include "sl_board_configuration.h"

// Prototypes kept local to avoid pulling in the Wi-Fi host-interface header.
void sl_si91x_host_clear_sleep_indicator(void);
void sl_si91x_host_hold_in_reset(void);
void sl_si91x_host_release_from_reset(void);

void sl_si91x_host_clear_sleep_indicator(void)
{
  GPIO_PinOutClear(SLEEP_CONFIRM_PIN.port, SLEEP_CONFIRM_PIN.pin);
}

void sl_si91x_host_hold_in_reset(void)
{
  GPIO_PinOutClear(RESET_PIN.port, RESET_PIN.pin);
}

void sl_si91x_host_release_from_reset(void)
{
  GPIO_PinOutSet(RESET_PIN.port, RESET_PIN.pin);
}
