/***************************************************************************/ /**
* @file ssi_secondary_freertos.h
* @brief SSI Secondary example functions
*******************************************************************************
* # License
* <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef SL_SI91X_SSI_SECONDARY_FREERTOS_H_
#define SL_SI91X_SSI_SECONDARY_FREERTOS_H_

#define SSI_SECONDARY_TRANSFER ENABLE
#define SSI_SECONDARY_SEND     DISABLE
#define SSI_SECONDARY_RECEIVE  DISABLE

// -----------------------------------------------------------------------------
// Prototypes
/***************************************************************************/ /**
* SSI secondary code entry point
*
* This function initializes the clock config, inits the driver, and configures
* the SSI peripheral as secondary. It starts the SSI secondary task which runs
* transfer / receive / send flows based on header macros. In loopback mode,
* Tx and Rx buffers are compared; if equal, the basic loopback test passes.
* Results are displayed on the DEBUG prints output.
*
* @param none
* @return none
******************************************************************************/
void ssi_secondary_example_init(void);

#endif /* SL_SI91X_SSI_SECONDARY_FREERTOS_H_ */
