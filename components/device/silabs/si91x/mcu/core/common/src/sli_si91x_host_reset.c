/***************************************************************************/ /**
 * @file  sli_si91x_host_reset.c
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

// SoC host reset controls for sl_si91x_host_power_cycle(). On SoC builds there is no
// external device reset line, so these are no-ops.
void sl_si91x_host_hold_in_reset(void);
void sl_si91x_host_release_from_reset(void);

void sl_si91x_host_hold_in_reset(void)
{
  // initializes and activates a GPIO pin to forcefully reset or disable a connected device.
}

void sl_si91x_host_release_from_reset(void)
{
  // Release the reset state of a connected device
}
