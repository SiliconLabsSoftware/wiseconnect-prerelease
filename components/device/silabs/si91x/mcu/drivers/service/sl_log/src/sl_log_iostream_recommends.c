/***************************************************************************/ /**
 * @file sl_log_iostream_recommends.c
 * @brief SI91x IO Stream console recommends file.
 *******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

/***************************************************************************/ /**
 * Set iostream recommended instance for a console
 ******************************************************************************/
#include "sl_iostream.h"
#include "sl_iostream_handles.h"
#include "sl_si91x_iostream_rtt.h"
#include "sl_si91x_iostream_swo.h"
#include "sl_si91x_iostream_vuart.h"
#include "sl_si91x_iostream_debug.h"
#include "sl_iostream_uart_si91x.h"
#include "sl_log_iostream_recommends.h"

void sl_si91x_iostream_set_console_instance()
{
  uint32_t recommended_instance_index = 0xFFFFFFFF;
  uint32_t best_priority              = 0xFFFFFFFF;

  for (uint32_t i = 0; i < sl_iostream_instances_count; i++) {
    uint32_t current_priority;

    switch (sl_iostream_instances_info[i]->type) {
      case SL_IOSTREAM_TYPE_91X_UART:
        current_priority = 0;
        break;

      case SL_SI91X_IOSTREAM_TYPE_RTT_OUTPUT:
        current_priority = 1;
        break;

      case SL_SI91X_IOSTREAM_TYPE_SWO:
        current_priority = 2;
        break;

      case SL_SI91X_IOSTREAM_TYPE_VUART:
        current_priority = 3;
        break;

      case SL_SI91X_IOSTREAM_TYPE_DEBUG_OUTPUT:
        current_priority = 4;
        break;

      default:
        continue;
    }

    if (current_priority < best_priority) {
      best_priority              = current_priority;
      recommended_instance_index = i;
    }
  }

  if (recommended_instance_index != 0xFFFFFFFF) {
    sl_iostream_recommended_console_stream = sl_iostream_instances_info[recommended_instance_index]->handle;
  }
}