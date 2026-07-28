/***************************************************************************/ /**
 * @file empty_cpp_freertos.cpp
 * @brief FreeRTOS idle task for empty C++ example
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 ******************************************************************************/
#include "cmsis_os2.h"
#include "empty_cpp_freertos.h"
#include "rsi_debug.h"

static void empty_cpp_task(void *argument)
{
  (void)argument;
  for (;;) {
    osDelay(1000);
  }
}

extern "C" void empty_cpp_freertos_init(void)
{
  /* Value-initialize then set fields: partial C99-style designated initializers
   * trigger -Werror=missing-field-initializers under g++ -std=c++17. */
  static osThreadAttr_t attr{};
  attr.name       = "empty_cpp";
  attr.stack_size = 1024;
  attr.priority   = osPriorityLow1;

  if (osThreadNew((osThreadFunc_t)empty_cpp_task, NULL, &attr) == NULL) {
    DEBUGOUT("Failed to create empty_cpp thread\r\n");
  }
}
