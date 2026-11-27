/*******************************************************************************
* @file  sl_si91x_gpio_hp_pin_config.h
* @brief HP GPIO 64-75 pin configuration (shared across all boards)
*******************************************************************************
* # License
* <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
*******************************************************************************
*
* The licensor of this software is Silicon Laboratories Inc. Your use of this
* software is governed by the terms of Silicon Labs Master Software License
* Agreement (MSLA) available at
* www.silabs.com/about-us/legal/master-software-license-agreement. This
* software is distributed to you in Source Code format and is governed by the
* sections of the MSLA applicable to Source Code.
*
******************************************************************************/

#ifndef SL_SI91X_GPIO_HP_PIN_CONFIG_H
#define SL_SI91X_GPIO_HP_PIN_CONFIG_H

#if defined(SL_SI91X_HP_GPIO_64_PIN) || defined(SL_SI91X_HP_GPIO_68_PIN) || defined(SL_SI91X_HP_GPIO_69_PIN) \
  || defined(RTE_HP_GPIO_64_PIN) || defined(RTE_HP_GPIO_68_PIN) || defined(RTE_HP_GPIO_69_PIN)
#error "GPIO_64/68/69 are reserved for antenna selection and are no longer available as GPIO."
#endif

#define SL_SI91X_HP_GPIO_65_PORT RTE_HP_GPIO_65_PORT
#define SL_SI91X_HP_GPIO_65_PAD  RTE_HP_GPIO_65_PAD
#define SL_SI91X_HP_GPIO_65_PIN  RTE_HP_GPIO_65_PIN
#define SL_SI91X_HP_GPIO_65_MODE RTE_HP_GPIO_65_MODE

#define SL_SI91X_HP_GPIO_66_PORT RTE_HP_GPIO_66_PORT
#define SL_SI91X_HP_GPIO_66_PAD  RTE_HP_GPIO_66_PAD
#define SL_SI91X_HP_GPIO_66_PIN  RTE_HP_GPIO_66_PIN
#define SL_SI91X_HP_GPIO_66_MODE RTE_HP_GPIO_66_MODE

#define SL_SI91X_HP_GPIO_70_PORT RTE_HP_GPIO_70_PORT
#define SL_SI91X_HP_GPIO_70_PAD  RTE_HP_GPIO_70_PAD
#define SL_SI91X_HP_GPIO_70_PIN  RTE_HP_GPIO_70_PIN
#define SL_SI91X_HP_GPIO_70_MODE RTE_HP_GPIO_70_MODE

#define SL_SI91X_HP_GPIO_71_PORT RTE_HP_GPIO_71_PORT
#define SL_SI91X_HP_GPIO_71_PAD  RTE_HP_GPIO_71_PAD
#define SL_SI91X_HP_GPIO_71_PIN  RTE_HP_GPIO_71_PIN
#define SL_SI91X_HP_GPIO_71_MODE RTE_HP_GPIO_71_MODE

#define SL_SI91X_HP_GPIO_72_PORT RTE_HP_GPIO_72_PORT
#define SL_SI91X_HP_GPIO_72_PAD  RTE_HP_GPIO_72_PAD
#define SL_SI91X_HP_GPIO_72_PIN  RTE_HP_GPIO_72_PIN
#define SL_SI91X_HP_GPIO_72_MODE RTE_HP_GPIO_72_MODE

#define SL_SI91X_HP_GPIO_73_PORT RTE_HP_GPIO_73_PORT
#define SL_SI91X_HP_GPIO_73_PAD  RTE_HP_GPIO_73_PAD
#define SL_SI91X_HP_GPIO_73_PIN  RTE_HP_GPIO_73_PIN
#define SL_SI91X_HP_GPIO_73_MODE RTE_HP_GPIO_73_MODE

#define SL_SI91X_HP_GPIO_74_PORT RTE_HP_GPIO_74_PORT
#define SL_SI91X_HP_GPIO_74_PAD  RTE_HP_GPIO_74_PAD
#define SL_SI91X_HP_GPIO_74_PIN  RTE_HP_GPIO_74_PIN
#define SL_SI91X_HP_GPIO_74_MODE RTE_HP_GPIO_74_MODE

#define SL_SI91X_HP_GPIO_75_PORT RTE_HP_GPIO_75_PORT
#define SL_SI91X_HP_GPIO_75_PAD  RTE_HP_GPIO_75_PAD
#define SL_SI91X_HP_GPIO_75_PIN  RTE_HP_GPIO_75_PIN
#define SL_SI91X_HP_GPIO_75_MODE RTE_HP_GPIO_75_MODE

#endif /* SL_SI91X_GPIO_HP_PIN_CONFIG_H */
