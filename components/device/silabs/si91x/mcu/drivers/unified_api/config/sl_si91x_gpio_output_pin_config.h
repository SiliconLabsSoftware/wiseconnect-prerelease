/***************************************************************************/ /**
 * @file sl_si91x_gpio_output_pin_config.h
 * @brief GPIO Init Output instance Configuration Wizard + pin tool block.
 *******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 ******************************************************************************/

#ifndef SL_SI91X_GPIO_OUTPUT_INSTANCE_CONFIG_H
#define SL_SI91X_GPIO_OUTPUT_INSTANCE_CONFIG_H

#include "em_device.h"
#include "sl_si91x_gpio.h"

// <<< Use Configuration Wizard in Context Menu >>>

// <h> Pin Settings
// <i> Output drive, initial DOUT, and slew. Drive/slew are HP/ULP only.
// <i> For UULP pins select None for drive and slew (no PADCONFIG path).

// <o SL_SI91X_GPIO_OUTPUT_INSTANCE_DRIVE_STRENGTH> Drive strength
// <SL_GPIO_DRIVE_STRENGTH_NONE=> None
// <GPIO_TWO_MILLI_AMPS=> 2 mA
// <GPIO_FOUR_MILLI_AMPS=> 4 mA
// <GPIO_EIGHT_MILLI_AMPS=> 8 mA
// <GPIO_TWELVE_MILLI_AMPS=> 12 mA
// <i> Default: GPIO_FOUR_MILLI_AMPS. Select None for UULP.
#ifndef SL_SI91X_GPIO_OUTPUT_INSTANCE_DRIVE_STRENGTH
#define SL_SI91X_GPIO_OUTPUT_INSTANCE_DRIVE_STRENGTH GPIO_FOUR_MILLI_AMPS
#endif

// <o SL_SI91X_GPIO_OUTPUT_INSTANCE_DOUT> Initial DOUT
// <GPIO_PIN_CLEAR=> Low
// <GPIO_PIN_SET=> High
// <i> Default: GPIO_PIN_CLEAR
#ifndef SL_SI91X_GPIO_OUTPUT_INSTANCE_DOUT
#define SL_SI91X_GPIO_OUTPUT_INSTANCE_DOUT GPIO_PIN_CLEAR
#endif

// <o SL_SI91X_GPIO_OUTPUT_INSTANCE_SLEW_RATE> Slew rate
// <SL_GPIO_SLEW_RATE_NONE=> None
// <GPIO_SR_LOW=> Low
// <GPIO_SR_HIGH=> High
// <i> Default: GPIO_SR_LOW. Select None for UULP.
#ifndef SL_SI91X_GPIO_OUTPUT_INSTANCE_SLEW_RATE
#define SL_SI91X_GPIO_OUTPUT_INSTANCE_SLEW_RATE GPIO_SR_LOW
#endif

// </h>

// <<< end of configuration section >>>

// <<< sl:start pin_tool >>>
// <gpio> SL_SI91X_GPIO_OUTPUT_INSTANCE
// $[GPIO_SL_SI91X_GPIO_OUTPUT_INSTANCE]
#warning "GPIO Output INSTANCE pin not configured in Pin Tool"
// #define SL_SI91X_GPIO_OUTPUT_INSTANCE_PORT HP
// #define SL_SI91X_GPIO_OUTPUT_INSTANCE_PIN  0
// [GPIO_SL_SI91X_GPIO_OUTPUT_INSTANCE]$
// <<< sl:end pin_tool >>>

#if defined(SL_SI91X_GPIO_OUTPUT_INSTANCE_PORT) && (SL_SI91X_GPIO_OUTPUT_INSTANCE_PORT == UULP_VBAT)
#if (SL_SI91X_GPIO_OUTPUT_INSTANCE_DRIVE_STRENGTH != SL_GPIO_DRIVE_STRENGTH_NONE)
#warning "GPIO Output INSTANCE: drive strength is ignored on UULP; set Drive strength to None"
#endif
#if (SL_SI91X_GPIO_OUTPUT_INSTANCE_SLEW_RATE != SL_GPIO_SLEW_RATE_NONE)
#warning "GPIO Output INSTANCE: slew rate is ignored on UULP; set Slew rate to None"
#endif
#endif

#endif // SL_SI91X_GPIO_OUTPUT_INSTANCE_CONFIG_H
