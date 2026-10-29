/***************************************************************************/ /**
 * @file RTE_led_config.h
 * @brief Led Driver Configuration
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************/

#ifndef SL_SI91X_LED_CONFIG_H
#define SL_SI91X_LED_CONFIG_H

#include "em_device.h"
#include "sl_si91x_gpio.h"
#include "sl_si91x_led_instances.h"

#define SL_SI91x_LED_COUNT 2

#ifndef SL_SI91X_LED_0_PORT
#define SL_LED_LED0_PIN    RTE_LED0_PIN
#define SL_LED_LED0_PORT   RTE_LED0_PORT
#define SL_LED_LED0_NUMBER RTE_LED0_NUMBER
#endif

#ifndef SL_SI91X_LED_1_PORT
#define SL_LED_LED1_PIN    RTE_LED1_PIN
#define SL_LED_LED1_PORT   RTE_LED1_PORT
#define SL_LED_LED1_NUMBER RTE_LED1_NUMBER
#define SL_LED_LED1_PAD    RTE_LED1_PAD
#endif

// Defaults when board config has no Pin Settings (legacy packs).
#ifndef SL_LED_LED0_DRIVE_STRENGTH
#define SL_LED_LED0_DRIVE_STRENGTH GPIO_FOUR_MILLI_AMPS
#endif
#ifndef SL_LED_LED1_DRIVE_STRENGTH
#define SL_LED_LED1_DRIVE_STRENGTH GPIO_FOUR_MILLI_AMPS
#endif
#ifndef SL_LED_LED0_DOUT
#define SL_LED_LED0_DOUT GPIO_PIN_CLEAR
#endif
#ifndef SL_LED_LED1_DOUT
#define SL_LED_LED1_DOUT GPIO_PIN_CLEAR
#endif
#ifndef SL_LED_LED0_SLEW_RATE
#define SL_LED_LED0_SLEW_RATE GPIO_SR_LOW
#endif
#ifndef SL_LED_LED1_SLEW_RATE
#define SL_LED_LED1_SLEW_RATE GPIO_SR_LOW
#endif

#endif // SL_SI91X_LED_CONFIG_H
