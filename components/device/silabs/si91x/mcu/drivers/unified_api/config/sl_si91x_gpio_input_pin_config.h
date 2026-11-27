/***************************************************************************/ /**
 * @file sl_si91x_gpio_input_pin_config.h
 * @brief GPIO Init Input instance Configuration Wizard + pin tool block.
 *******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 ******************************************************************************/

#ifndef SL_SI91X_GPIO_INPUT_INSTANCE_CONFIG_H
#define SL_SI91X_GPIO_INPUT_INSTANCE_CONFIG_H

#include "em_device.h"
#include "sl_si91x_gpio.h"

// <<< Use Configuration Wizard in Context Menu >>>

// <h> Pin Settings
// <i> Input pad mode (PADCONFIG_P). HP/ULP only.
// <i> For UULP pins select None (no PADCONFIG_P path).

// <o SL_SI91X_GPIO_INPUT_INSTANCE_PAD_MODE> Pad mode
// <SL_GPIO_PAD_MODE_NONE=> None
// <GPIO_HZ=> Hi-Z
// <GPIO_PULLUP=> Pull-up
// <GPIO_PULLDOWN=> Pull-down
// <GPIO_REPEATER=> Repeater
// <i> Default: GPIO_HZ. Select None for UULP.
#ifndef SL_SI91X_GPIO_INPUT_INSTANCE_PAD_MODE
#define SL_SI91X_GPIO_INPUT_INSTANCE_PAD_MODE GPIO_HZ
#endif

// </h>

// <<< end of configuration section >>>

// <<< sl:start pin_tool >>>
// <gpio> SL_SI91X_GPIO_INPUT_INSTANCE
// $[GPIO_SL_SI91X_GPIO_INPUT_INSTANCE]
#warning "GPIO Input INSTANCE pin not configured in Pin Tool"
// #define SL_SI91X_GPIO_INPUT_INSTANCE_PORT HP
// #define SL_SI91X_GPIO_INPUT_INSTANCE_PIN  0
// [GPIO_SL_SI91X_GPIO_INPUT_INSTANCE]$
// <<< sl:end pin_tool >>>

#if defined(SL_SI91X_GPIO_INPUT_INSTANCE_PORT) && (SL_SI91X_GPIO_INPUT_INSTANCE_PORT == UULP_VBAT)
#if (SL_SI91X_GPIO_INPUT_INSTANCE_PAD_MODE != SL_GPIO_PAD_MODE_NONE)
#warning "GPIO Input INSTANCE: pad mode is ignored on UULP; set Pad mode to None"
#endif
#endif

#endif // SL_SI91X_GPIO_INPUT_INSTANCE_CONFIG_H
