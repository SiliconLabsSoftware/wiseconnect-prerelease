/***************************************************************************/ /**
 * @file sl_si91x_led_init_led1_config.h
 * @brief Led Driver Configuration
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************/

#ifndef SL_SI91X_LED_INIT_LED1_CONFIG_H
#define SL_SI91X_LED_INIT_LED1_CONFIG_H

#include "sl_si91x_gpio.h"

// <<< Use Configuration Wizard in Context Menu >>>

// <h> Simple LED configuration
// <o SL_SIMPLE_LED_LED1_POLARITY>
// <SL_SIMPLE_LED_POLARITY_ACTIVE_LOW=> Active low
// <SL_SIMPLE_LED_POLARITY_ACTIVE_HIGH=> Active high
// <i> Default: SL_SIMPLE_LED_POLARITY_ACTIVE_HIGH
#define SL_SIMPLE_LED_LED1_POLARITY SL_SIMPLE_LED_POLARITY_ACTIVE_HIGH
// </h> end led configuration

// <h> Pin Settings
// <i> LED is GPIO output. PADCONFIG_E (drive), PADCONFIG_SR (slew), DOUT (initial level).

// <o SL_LED_LED1_DRIVE_STRENGTH> GPIO drive strength
// <GPIO_TWO_MILLI_AMPS=> 2 mA
// <GPIO_FOUR_MILLI_AMPS=> 4 mA
// <GPIO_EIGHT_MILLI_AMPS=> 8 mA
// <GPIO_TWELVE_MILLI_AMPS=> 12 mA
// <i> Default: GPIO_FOUR_MILLI_AMPS
#ifndef SL_LED_LED1_DRIVE_STRENGTH
#define SL_LED_LED1_DRIVE_STRENGTH GPIO_FOUR_MILLI_AMPS
#endif

// <o SL_LED_LED1_DOUT> GPIO DOUT
// <GPIO_PIN_CLEAR=> Low
// <GPIO_PIN_SET=> High
// <i> Initial output level driven on the pad. Default: GPIO_PIN_CLEAR
#ifndef SL_LED_LED1_DOUT
#define SL_LED_LED1_DOUT GPIO_PIN_CLEAR
#endif

// <o SL_LED_LED1_SLEW_RATE> GPIO slew rate
// <GPIO_SR_LOW=> Low
// <GPIO_SR_HIGH=> High
// <i> PADCONFIG_SR. Default: GPIO_SR_LOW
#ifndef SL_LED_LED1_SLEW_RATE
#define SL_LED_LED1_SLEW_RATE GPIO_SR_LOW
#endif

// </h>

// <<< end of configuration section >>>

// <<< sl:start pin_tool >>>
// <gpio> SL_SI91X_LED_1
// $[GPIO_SL_SI91X_LED_1]
#ifndef SL_SI91X_LED_1_PORT
#define SL_SI91X_LED_1_PORT HP
#endif
#ifndef SL_SI91X_LED_1_PIN
#define SL_SI91X_LED_1_PIN 10
#endif
// [GPIO_SL_SI91X_LED_1]$
// <<< sl:end pin_tool >>>

#define SL_LED_LED1_PIN    SL_SI91X_LED_1_PIN
#define SL_LED_LED1_PORT   RTE_LED1_PORT
#define SL_LED_LED1_NUMBER RTE_LED1_NUMBER

#endif // SL_SI91X_LED_INIT_LED1_CONFIG_H
