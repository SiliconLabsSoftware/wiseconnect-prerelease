/***************************************************************************/ /**
 * @file sli_wifi_command_engine_config.h
 * @brief
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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
#ifndef SLI_WIFI_COMMAND_ENGINE_CONFIG_H
#define SLI_WIFI_COMMAND_ENGINE_CONFIG_H

#include <stdint.h>

#include "sl_status.h"
#include "sli_command_engine.h"
#include "sli_event_engine.h"
#include "sl_si91x_driver.h"
#include "sli_queue_manager_types.h"

/******************************************************
 *               Macro Definitions
 ******************************************************/
/**
 * @brief Defines the thread priority for the Wi-Fi command engine.
 *
 * This macro sets the priority level for the Wi-Fi command engine thread.
 * The default value is set to `osPriorityRealtime`, which is a real-time priority level.
 *
 * @note
 * - The priority level of this thread should be second highest after @ref SL_WIFI_ASYNC_EVENT_HANDLER_THREAD_PRIORITY among all the threads in the system.
 */
#ifndef SL_WIFI_COMMAND_ENGINE_THREAD_PRIORITY
#define SL_WIFI_COMMAND_ENGINE_THREAD_PRIORITY osPriorityRealtime
#endif

/**
 * @brief Defines the stack size for the Wi-Fi command engine.
 *
 * The default stack size is set to 1636 bytes. This value can be overridden
 * by defining SL_WIFI_COMMAND_ENGINE_STACK_SIZE before including this file.
 */
#ifndef SL_WIFI_COMMAND_ENGINE_STACK_SIZE
#define SL_WIFI_COMMAND_ENGINE_STACK_SIZE 1636
#endif

/**
 * @brief Defines the priority level for the Wi-Fi asynchronous event handler thread.
 *
 * This macro sets the priority for the thread that handles asynchronous Wi-Fi events.
 * The default value is set to `osPriorityRealtime1`, which ensures high-priority
 * execution for time-sensitive operations.
 */
#ifndef SL_WIFI_ASYNC_EVENT_HANDLER_THREAD_PRIORITY
#define SL_WIFI_ASYNC_EVENT_HANDLER_THREAD_PRIORITY osPriorityRealtime1
#endif

/**
 * @brief Defines the stack size for the asynchronous event handler.
 *
 * This macro specifies the stack size (in bytes) allocated for the Wi-Fi
 * asynchronous event handler. The default value is set to 1536 bytes.
 */
#ifndef SL_WIFI_ASYNC_EVENT_HANDLER_STACK_SIZE
#define SL_WIFI_ASYNC_EVENT_HANDLER_STACK_SIZE 1536

/******************************************************
 *               Type Definitions
 ******************************************************/
/// Si91x specific command type
typedef enum {
  SLI_WLAN_COMMON_CMD  = 0, ///< SI91X Common Command
  SLI_WLAN_WIFI_CMD    = 1, ///< SI91X Wireless LAN Command
  SLI_WLAN_NETWORK_CMD = 2, ///< SI91X Network Command
  SLI_WLAN_BT_CMD      = 3, ///< SI91X Bluetooth Command
  SLI_WLAN_SOCKET_CMD  = 4, ///< SI91X Socket Command
  SLI_WLAN_CMD_MAX     = 5  ///< SI91X Maximum Command value
} sli_wlan_command_type_t;

typedef enum {
  SLI_WIFI_COMMAND_ENGINE_COMMON_COMMAND_PACKET = 0,
  SLI_WIFI_COMMAND_ENGINE_WIFI_COMMAND_PACKET,
  SLI_WIFI_COMMAND_ENGINE_NETWORK_COMMAND_PACKET,
  SLI_WIFI_COMMAND_ENGINE_BLE_COMMAND_PACKET,
  SLI_WIFI_COMMAND_ENGINE_SOCKET_COMMAND_PACKET,
  SLI_WIFI_COMMAND_ENGINE_MAX_PACKET_TYPES
} sli_wifi_command_engine_packet_types_t;

typedef enum {
  SLI_WIFI_ASYNC_EVENT_HANDLER_COMMON_EVENT = 0,
  SLI_WIFI_ASYNC_EVENT_HANDLER_WIFI_EVENT,
  SLI_WIFI_ASYNC_EVENT_HANDLER_NETWORK_EVENT,
  SLI_WIFI_ASYNC_EVENT_HANDLER_BLE_EVENT,
  SLI_WIFI_ASYNC_EVENT_HANDLER_SOCKET_CMD_EVENT,
  SLI_WIFI_ASYNC_EVENT_HANDLER_SOCKET_DATA_EVENT,
  SLI_WIFI_ASYNC_EVENT_HANDLER_ERROR_EVENT,
  SLI_WIFI_ASYNC_EVENT_HANDLER_NWP_LOG_EVENT,
  SLI_WIFI_ASYNC_EVENT_HANDLER_MAX_EVENTS
} sli_wifi_async_event_handler_events_t;
#endif

// Indicates RX response received for COMMON command type
#define SL_WIFI_HOST_COMMON_RESPONSE_EVENT SL_SI91X_RESPONSE_FLAG(SLI_WLAN_COMMON_CMD)

// Indicates synchronous RX response received for WLAN command type
#define SL_WIFI_RESPONSE_EVENT SL_SI91X_RESPONSE_FLAG(SLI_WLAN_WIFI_CMD)

// Indicates synchronous RX response received for NETWORK command type
#define SL_WIFI_NETWORK_RESPONSE_EVENT SL_SI91X_RESPONSE_FLAG(SLI_WLAN_NETWORK_CMD)

// Indicates RX response received for SOCKET command type
#define SL_WIFI_SOCKET_RESPONSE_EVENT SL_SI91X_RESPONSE_FLAG(SLI_WLAN_SOCKET_CMD)

// Indicates RX response received for BLE command type
#define SL_WIFI_BT_RESPONSE_EVENT SL_SI91X_RESPONSE_FLAG(SLI_WLAN_BT_CMD)

extern sli_command_engine_configuration_t sli_wifi_command_engine_config;

/**
 * @brief Command engine instance for Wi-Fi.
 */
extern sli_command_engine_t sli_wifi_command_engine;

sl_status_t sli_wifi_event_engine_init(void);

#endif // SLI_WIFI_COMMAND_ENGINE_CONFIG_H
