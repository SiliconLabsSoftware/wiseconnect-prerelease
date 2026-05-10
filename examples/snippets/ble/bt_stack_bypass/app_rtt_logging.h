/*******************************************************************************
 * @file  app_rtt_logging.h
 * @brief RTT Console Logging Configuration (NCP mode)
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef APP_RTT_LOGGING_H
#define APP_RTT_LOGGING_H

#ifndef SLI_SI91X_MCU_INTERFACE

#include "SEGGER_RTT.h"
#include <stdio.h>

/***************************************************************************/ /**
 * @brief RTT Channel Configuration for Console Logging
 * 
 * Channel 0: Default RTT Terminal
 * Channel 1: BTDM controller logs
 * Channel 2: Application console logs (LOG_PRINT, DEBUGOUT)
 ******************************************************************************/

/**
 * @brief Redirect LOG_PRINT to RTT Channel 2
 * 
 * Formats the message using snprintf and writes to RTT channel 2.
 * Uses a 256-byte buffer for formatted output.
 */
#ifdef LOG_PRINT
#undef LOG_PRINT
#endif
#define LOG_PRINT(...)                                                     \
  do {                                                                     \
    char rtt_log_buf[256];                                                 \
    int rtt_len = snprintf(rtt_log_buf, sizeof(rtt_log_buf), __VA_ARGS__); \
    if (rtt_len > 0) {                                                     \
      SEGGER_RTT_Write(2, rtt_log_buf, (unsigned)rtt_len);                 \
    }                                                                      \
  } while (0)

/**
 * @brief Redirect DEBUGOUT to RTT Channel 2
 * 
 * Formats the message using snprintf and writes to RTT channel 2.
 * Uses a 256-byte buffer for formatted output.
 */
#ifdef DEBUGOUT
#undef DEBUGOUT
#endif
#define DEBUGOUT(...)                                                      \
  do {                                                                     \
    char rtt_dbg_buf[256];                                                 \
    int rtt_len = snprintf(rtt_dbg_buf, sizeof(rtt_dbg_buf), __VA_ARGS__); \
    if (rtt_len > 0) {                                                     \
      SEGGER_RTT_Write(2, rtt_dbg_buf, (unsigned)rtt_len);                 \
    }                                                                      \
  } while (0)

#endif /* !SLI_SI91X_MCU_INTERFACE */

#endif /* APP_RTT_LOGGING_H */
