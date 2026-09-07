/***************************************************************************/ /**
 * @file
 * @brief Top-level application functions for the Memory Manager Heap 3 application.
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

#include "app.h"
#include "memory_manager_heap3_application.h"

/***************************************************************************/ /**
 * @brief Initialize the application.
 ******************************************************************************/
void app_init(void)
{
  /* sl_main calls this after the kernel is ready. Start the demo thread. */
  memory_manager_heap3_application_init();
}

/***************************************************************************/ /**
 * @brief Application process action (unused; work runs in a FreeRTOS thread).
 ******************************************************************************/
void app_process_action(void)
{
  /* Required sl_main hook. Unused: the application runs in mm_heap3_app. */
}
