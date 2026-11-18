/***************************************************************************/ /**
 * @file
 * @brief Wi-Fi SoC Empty application — minimal station connect
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

#include "cmsis_os2.h"
#include "sl_net.h"
#include <inttypes.h>
#include "sl_utility.h"
#include "sl_wifi.h"
#include "sl_net_default_values.h"

/******************************************************
 *                    Constants
 ******************************************************/
#define IDLE_DELAY_MS 1000

/******************************************************
 *               Variable Definitions
 ******************************************************/
const osThreadAttr_t thread_attributes = {
  .name       = "app",
  .attr_bits  = 0,
  .cb_mem     = 0,
  .cb_size    = 0,
  .stack_mem  = 0,
  .stack_size = 3072,
  .priority   = osPriorityLow,
  .tz_module  = 0,
};

/******************************************************
 *               Function Declarations
 ******************************************************/
static void application_start(void *argument);
static sl_status_t network_event_handler(sl_net_event_t event, sl_status_t status, void *data, uint32_t data_length);

/******************************************************
 *               Function Definitions
 ******************************************************/
void app_init(void)
{
  osThreadNew((osThreadFunc_t)application_start, NULL, &thread_attributes);
}

static void application_start(void *argument)
{
  UNUSED_PARAMETER(argument);
  sl_status_t status;

  status = sl_net_init(SL_NET_WIFI_CLIENT_INTERFACE, NULL, NULL, network_event_handler);
  if (status != SL_STATUS_OK) {
    SL_DEBUG_LOG_V2(ERROR, "Failed to start Wi-Fi Client interface: 0x%" PRIx32 "\r\n", (uint32_t)status);
    return;
  }
  SL_DEBUG_LOG_V2(INFO, "Wi-Fi client interface init success\r\n");

  status = sl_net_up(SL_NET_WIFI_CLIENT_INTERFACE, SL_NET_DEFAULT_WIFI_CLIENT_PROFILE_ID);
  if (status != SL_STATUS_OK) {
    SL_DEBUG_LOG_V2(ERROR, "Failed to bring Wi-Fi client interface up: 0x%" PRIx32 "\r\n", (uint32_t)status);
    return;
  }
  SL_DEBUG_LOG_V2(INFO, "Wi-Fi client connected\r\n");

  while (1) {
    osDelay(IDLE_DELAY_MS);
  }
}

static sl_status_t network_event_handler(sl_net_event_t event, sl_status_t status, void *data, uint32_t data_length)
{
  UNUSED_PARAMETER(data);
  UNUSED_PARAMETER(data_length);

  switch (event) {
    case SL_NET_DHCP_NOTIFICATION_EVENT: {
      SL_DEBUG_LOG_V2(INFO, "Received DHCP Notification event with status : 0x%" PRIx32 "\r\n", (uint32_t)status);
      break;
    }
    case SL_NET_IP_ADDRESS_CHANGE_EVENT: {
      SL_DEBUG_LOG_V2(INFO,
                      "Received Ip Address Change Notification event with status : 0x%" PRIx32 "\r\n",
                      (uint32_t)status);
      break;
    }
    case SL_NET_CONNECT_EVENT: {
      SL_DEBUG_LOG_V2(INFO, "Received Connect event with status : 0x%" PRIx32 "\r\n", (uint32_t)status);
      break;
    }
    default:
      break;
  }

  return SL_STATUS_OK;
}
