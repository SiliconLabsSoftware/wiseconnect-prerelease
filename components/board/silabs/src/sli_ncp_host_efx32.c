/***************************************************************************/ /**
 * @file  sli_ncp_host_efx32.c
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

// NCP (EFx32 host) implementation of the SiWx91x wake/sleep indicator control.
// Pin macros come from the board's NCP configuration; GPIO_PinOutClear comes from emlib.
#include "em_gpio.h"
#ifdef SL_NCP_UART_INTERFACE
#include "si91x_ncp_uart_config.h"
#else
#include "si91x_ncp_spi_config.h"
#endif

// Prototypes kept local to avoid pulling in the Wi-Fi host-interface header.
void sl_si91x_host_clear_sleep_indicator(void);
void sl_si91x_host_hold_in_reset(void);
void sl_si91x_host_release_from_reset(void);

void sl_si91x_host_clear_sleep_indicator(void)
{
  GPIO_PinOutClear(SI91X_NCP_WAKE_INDICATOR_PORT, SI91X_NCP_WAKE_INDICATOR_PIN);
}

void sl_si91x_host_hold_in_reset(void)
{
  GPIO_PinOutClear(SI91X_NCP_RESET_PORT, SI91X_NCP_RESET_PIN);
}

void sl_si91x_host_release_from_reset(void)
{
  GPIO_PinOutSet(SI91X_NCP_RESET_PORT, SI91X_NCP_RESET_PIN);
}
