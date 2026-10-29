/***************************************************************************/ /**
 * @file sl_si91x_led_init_led0_config.h
 * @brief Led Driver Configuration
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************/

#ifndef SL_SI91X_LED_INIT_LED0_CONFIG_H
#define SL_SI91X_LED_INIT_LED0_CONFIG_H

#include "sl_si91x_gpio.h"

// <<< Use Configuration Wizard in Context Menu >>>

// <h> Simple LED configuration
// <o SL_SIMPLE_LED_LED0_POLARITY>
// <SL_SIMPLE_LED_POLARITY_ACTIVE_LOW=> Active low
// <SL_SIMPLE_LED_POLARITY_ACTIVE_HIGH=> Active high
// <i> Default: SL_SIMPLE_LED_POLARITY_ACTIVE_HIGH
#define SL_SIMPLE_LED_LED0_POLARITY SL_SIMPLE_LED_POLARITY_ACTIVE_HIGH
// </h> end led configuration

// <h> Pin Settings
// <i> LED is GPIO output. PADCONFIG_E (drive), PADCONFIG_SR (slew), DOUT (initial level).

// <o SL_LED_LED0_DRIVE_STRENGTH> GPIO drive strength
// <GPIO_TWO_MILLI_AMPS=> 2 mA
// <GPIO_FOUR_MILLI_AMPS=> 4 mA
// <GPIO_EIGHT_MILLI_AMPS=> 8 mA
// <GPIO_TWELVE_MILLI_AMPS=> 12 mA
// <i> Default: GPIO_FOUR_MILLI_AMPS
#ifndef SL_LED_LED0_DRIVE_STRENGTH
#define SL_LED_LED0_DRIVE_STRENGTH GPIO_FOUR_MILLI_AMPS
#endif

// <o SL_LED_LED0_DOUT> GPIO DOUT
// <GPIO_PIN_CLEAR=> Low
// <GPIO_PIN_SET=> High
// <i> Initial output level driven on the pad. Default: GPIO_PIN_CLEAR
#ifndef SL_LED_LED0_DOUT
#define SL_LED_LED0_DOUT GPIO_PIN_CLEAR
#endif

// <o SL_LED_LED0_SLEW_RATE> GPIO slew rate
// <GPIO_SR_LOW=> Low
// <GPIO_SR_HIGH=> High
// <i> PADCONFIG_SR. Default: GPIO_SR_LOW
#ifndef SL_LED_LED0_SLEW_RATE
#define SL_LED_LED0_SLEW_RATE GPIO_SR_LOW
#endif

// </h>

// <<< end of configuration section >>>

// <<< sl:start pin_tool >>>
// <gpio> SL_SI91X_LED_0
// $[GPIO_SL_SI91X_LED_0]
#ifndef SL_SI91X_LED_0_PORT
#define SL_SI91X_LED_0_PORT ULP
#endif
#ifndef SL_SI91X_LED_0_PIN
#define SL_SI91X_LED_0_PIN 2
#endif
// [GPIO_SL_SI91X_LED_0]$
// <<< sl:end pin_tool >>>

#define SL_LED_LED0_PIN    SL_SI91X_LED_0_PIN
#define SL_LED_LED0_PORT   RTE_LED0_PORT
#define SL_LED_LED0_NUMBER RTE_LED0_NUMBER

#endif // SL_SI91X_LED_INIT_LED0_CONFIG_H
