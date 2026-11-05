/***************************************************************************/ /**
 * @file sl_si91x_gspi_config.h
 * @brief GSPI API configuration
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef SL_SI91X_GSPI_CONFIG_H
#define SL_SI91X_GSPI_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sl_si91x_gspi.h"
#include "sl_si91x_gpio.h"

// <<< Use Configuration Wizard in Context Menu >>>

//<h>GSPI Configuration

//  <e>GSPI UC Configuration
//  <i> Enable: Peripheral configuration is taken straight from the configuration set in the universal configuration (UC).
//  <i> Disable: If the application demands it to be modified during runtime, use the sl_si91x_gspi_set_configuration API to modify the peripheral configuration.
//  <i> Default: 1
#define GSPI_UC 1

// <o SL_GSPI_CLOCK_MODE> Mode
//   <SL_GSPI_MODE_0=> Mode 0
//   <SL_GSPI_MODE_3=> Mode 3
// <i> Selection of the GSPI Mode.
#define SL_GSPI_CLOCK_MODE SL_GSPI_MODE_0

// <o SL_GSPI_BITRATE> Bit Rate (Bits/Second) <2000-116000000>
// <i> Default: 10000000
#define SL_GSPI_BITRATE 10000000

// <o SL_GSPI_BIT_WIDTH> Data Width <1-16>
// <i> Default: 8
#define SL_GSPI_BIT_WIDTH 8

// <q SL_GSPI_SWAP_READ_DATA> Byte-wise Swapping Read
// <i> Default: 0
#define SL_GSPI_SWAP_READ_DATA 0

// <q SL_GSPI_SWAP_WRITE_DATA> Byte-wise Swapping Write
// <i> Default: 0
#define SL_GSPI_SWAP_WRITE_DATA 0

// </e>

// </h>

// <h> Pin Settings
// <i> Pad mode (PADCONFIG_P). MOSI/MISO/SCK are always present.
// <i> For CS0-CS2: select None when the pin_tool pin is None; pad mode is
// <i> applied only when the pin is assigned and mode is not None.

// <o SL_GSPI_MOSI_PAD_MODE> MOSI pad mode
// <GPIO_HZ=> Hi-Z
// <GPIO_PULLUP=> Pull-up
// <GPIO_PULLDOWN=> Pull-down
// <i> Default: GPIO_HZ
#ifndef SL_GSPI_MOSI_PAD_MODE
#define SL_GSPI_MOSI_PAD_MODE GPIO_HZ
#endif

// <o SL_GSPI_MISO_PAD_MODE> MISO pad mode
// <GPIO_HZ=> Hi-Z
// <GPIO_PULLUP=> Pull-up
// <GPIO_PULLDOWN=> Pull-down
// <i> Default: GPIO_HZ
#ifndef SL_GSPI_MISO_PAD_MODE
#define SL_GSPI_MISO_PAD_MODE GPIO_HZ
#endif

// <o SL_GSPI_SCK_PAD_MODE> SCK pad mode
// <GPIO_HZ=> Hi-Z
// <GPIO_PULLUP=> Pull-up
// <GPIO_PULLDOWN=> Pull-down
// <i> Default: GPIO_HZ
#ifndef SL_GSPI_SCK_PAD_MODE
#define SL_GSPI_SCK_PAD_MODE GPIO_HZ
#endif

// <o SL_GSPI_CS0_PAD_MODE> CS0 pad mode
// <SL_GPIO_PAD_MODE_NONE=> None
// <GPIO_HZ=> Hi-Z
// <GPIO_PULLUP=> Pull-up
// <GPIO_PULLDOWN=> Pull-down
// <i> Default: GPIO_HZ (use None if CS0 pin is None in pin tool)
#ifndef SL_GSPI_CS0_PAD_MODE
#define SL_GSPI_CS0_PAD_MODE GPIO_HZ
#endif

// <o SL_GSPI_CS1_PAD_MODE> CS1 pad mode
// <SL_GPIO_PAD_MODE_NONE=> None
// <GPIO_HZ=> Hi-Z
// <GPIO_PULLUP=> Pull-up
// <GPIO_PULLDOWN=> Pull-down
// <i> Default: None (ignored unless CS1 pin is assigned in pin tool)
#ifndef SL_GSPI_CS1_PAD_MODE
#define SL_GSPI_CS1_PAD_MODE SL_GPIO_PAD_MODE_NONE
#endif

// <o SL_GSPI_CS2_PAD_MODE> CS2 pad mode
// <SL_GPIO_PAD_MODE_NONE=> None
// <GPIO_HZ=> Hi-Z
// <GPIO_PULLUP=> Pull-up
// <GPIO_PULLDOWN=> Pull-down
// <i> Default: None (ignored unless CS2 pin is assigned in pin tool)
#ifndef SL_GSPI_CS2_PAD_MODE
#define SL_GSPI_CS2_PAD_MODE SL_GPIO_PAD_MODE_NONE
#endif

// </h>
// <<< end of configuration section >>>

#if defined(SL_GSPI_CLOCK_MODE)
sl_gspi_control_config_t gspi_configuration = {
  .bit_width         = SL_GSPI_BIT_WIDTH,
  .clock_mode        = SL_GSPI_CLOCK_MODE,
  .slave_select_mode = SL_GSPI_MASTER_HW_OUTPUT,
  .bitrate           = SL_GSPI_BITRATE,
  .swap_read         = SL_GSPI_SWAP_READ_DATA,
  .swap_write        = SL_GSPI_SWAP_WRITE_DATA,
};
#endif

#ifdef __cplusplus
}
#endif // SL_GSPI_CLOCK_MODE
#endif // SL_SI91X_GSPI_CONFIG_H
