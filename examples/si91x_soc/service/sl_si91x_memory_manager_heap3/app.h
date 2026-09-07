/***************************************************************************/ /**
 * @file
 * @brief Top-level application interface for the Memory Manager Heap 3 application.
 *******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef APP_H
#define APP_H

/***************************************************************************/ /**
 * @brief Initialize the application.
 *
 * @details
 * Starts the Memory Manager Heap 3 application thread.
 * Called by the platform startup sequence after kernel initialization.
 ******************************************************************************/
void app_init(void);

/***************************************************************************/ /**
 * @brief Application process action.
 *
 * @details
 * Unused in this application because validation runs in its own FreeRTOS thread.
 * Provided for compatibility with the standard @c sl_main process-action hook.
 ******************************************************************************/
void app_process_action(void);

#endif // APP_H
