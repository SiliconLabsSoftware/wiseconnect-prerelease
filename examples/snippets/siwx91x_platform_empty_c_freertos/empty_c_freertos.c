/***************************************************************************/ /**
 * @file empty_c_freertos.c
 * @brief FreeRTOS idle task for empty C example
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 ******************************************************************************/
#include "cmsis_os2.h"
#include "empty_c_freertos.h"
#include "rsi_debug.h"

static void empty_c_task(void *argument)
{
  (void)argument;
  for (;;) {
    osDelay(1000);
  }
}

void empty_c_freertos_init(void)
{
  static const osThreadAttr_t attr = {
    .name       = "empty_c",
    .stack_size = 1024,
    .priority   = osPriorityLow1,
  };
  if (osThreadNew((osThreadFunc_t)empty_c_task, NULL, &attr) == NULL) {
    DEBUGOUT("Failed to create empty_c thread\r\n");
  }
}
