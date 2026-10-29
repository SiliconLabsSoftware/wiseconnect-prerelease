/***************************************************************************/ /**
 * @file sl_si91x_led_init_inst_config.h
 * @brief Led Driver Configuration
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************/

#ifndef SL_SI91X_LED_INIT_INST_CONFIG_H
#define SL_SI91X_LED_INIT_INST_CONFIG_H

#include "sl_si91x_gpio.h"

#if USER_CONFIGURATION_ENABLE
// <<< Use Configuration Wizard in Context Menu >>>

// <h> Pin Settings
// <i> LED is GPIO output. PADCONFIG_E (drive), PADCONFIG_SR (slew), DOUT (initial level).

// <o SL_LED_INSTANCE_DRIVE_STRENGTH> GPIO drive strength
// <GPIO_TWO_MILLI_AMPS=> 2 mA
// <GPIO_FOUR_MILLI_AMPS=> 4 mA
// <GPIO_EIGHT_MILLI_AMPS=> 8 mA
// <GPIO_TWELVE_MILLI_AMPS=> 12 mA
// <i> Default: GPIO_FOUR_MILLI_AMPS
#ifndef SL_LED_INSTANCE_DRIVE_STRENGTH
#define SL_LED_INSTANCE_DRIVE_STRENGTH GPIO_FOUR_MILLI_AMPS
#endif

// <o SL_LED_INSTANCE_DOUT> GPIO DOUT
// <GPIO_PIN_CLEAR=> Low
// <GPIO_PIN_SET=> High
// <i> Initial output level driven on the pad. Default: GPIO_PIN_CLEAR
#ifndef SL_LED_INSTANCE_DOUT
#define SL_LED_INSTANCE_DOUT GPIO_PIN_CLEAR
#endif

// <o SL_LED_INSTANCE_SLEW_RATE> GPIO slew rate
// <GPIO_SR_LOW=> Low
// <GPIO_SR_HIGH=> High
// <i> PADCONFIG_SR. Default: GPIO_SR_LOW
#ifndef SL_LED_INSTANCE_SLEW_RATE
#define SL_LED_INSTANCE_SLEW_RATE GPIO_SR_LOW
#endif

// </h>

// <<< end of configuration section >>>

#if defined(SL_SI91X_ACX_MODULE)
// <<< sl:start pin_tool >>>
// <gpio> SL_SI91X_LED_0
// $[GPIO_SL_SI91X_LED_0]
#ifndef SL_SI91X_LED_0_PORT
#define SL_SI91X_LED_0_PORT ULP
#endif
#ifndef SL_SI91X_LED_0_PIN
#define SL_SI91X_LED_0_PIN 0
#endif
// [GPIO_SL_SI91X_LED_0]$
// <<< sl:end pin_tool >>>

#define SL_LED_LED0_PIN    SL_SI91X_LED_0_PIN
#define SL_LED_LED0_PORT   RTE_LED0_PORT
#define SL_LED_LED0_NUMBER RTE_LED0_NUMBER
#else
// <<< sl:start pin_tool >>>
// <gpio> SL_SI91X_LED_1
// $[GPIO_SL_SI91X_LED_1]
#ifndef SL_SI91X_LED_1_PORT
#define SL_SI91X_LED_1_PORT ULP
#endif
#ifndef SL_SI91X_LED_1_PIN
#define SL_SI91X_LED_1_PIN 2
#endif
// [GPIO_SL_SI91X_LED_1]$
// <<< sl:end pin_tool >>>

#define SL_LED_LED1_PIN    SL_SI91X_LED_1_PIN
#define SL_LED_LED1_PORT   RTE_LED1_PORT
#define SL_LED_LED1_NUMBER RTE_LED1_NUMBER
#endif // SL_SI91X_ACX_MODULE
#else

#warning \
  "LED pins are not configured. To configure, either install [ENABLE USER CONFIGURATION] component or define USER_CONFIGURATION_ENABLE macro to 1, then configure the then configure the pins as per the Custom board."

#endif // USER_CONFIGURATION_ENABLE
#endif // SL_SI91X_LED_INIT_INST_CONFIG_H
