/***************************************************************************/ /**
 * @file sli_si91x_wifi_command_engine_config.c
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
#include <stdint.h>
#include "sl_constants.h"
#include "sli_wifi_command_engine_config.h"
#include "sli_si91x_wifi_event_handler.h"
#include "sli_wifi_command_engine_packet.h"
#include "sli_wifi_types.h"
#include "sli_hal_si91x.h"

#ifdef SLI_SI91X_SOCKETS
#include "sl_si91x_socket_utility.h"
#endif
/******************************************************
 *               Macro Definitions
 ******************************************************/

/******************************************************
 *               Local Function Declarations
 ******************************************************/

/******************************************************
 *             Extern Variable Declarations
 ******************************************************/
extern sli_wifi_command_queue_t cmd_queues[SI91X_CMD_MAX];

/******************************************************
 *               Local Variable Definitions
 ******************************************************/
sli_routing_entry_t wifi_command_engine_routing_entries[SLI_WIFI_COMMAND_ENGINE_MAX_PACKET] = {
  [SLI_WIFI_COMMAND_PACKET] = {
    .destination_packet_handler = sli_hal_si91x_command_send_packet,
    .packet_status_handler     = sli_command_engine_send_packet_tx_status,
    .packet_type = SLI_WIFI_COMMAND_PACKET,
  },
  [SLI_WIFI_DATA_PACKET] = {
    .destination_packet_handler = sli_hal_si91x_data_send_packet,
#ifdef SLI_SI91X_SOCKETS
    .packet_status_handler     = sli_si91x_send_tx_packet_status_handler,
#else
    .packet_status_handler     = NULL,
#endif
    .packet_type = SLI_WIFI_DATA_PACKET,
  },
  [SLI_BT_PACKET] = {
    .destination_packet_handler = sli_hal_si91x_ble_send_packet,
#ifdef SLI_SI91X_ENABLE_BLE
    .packet_status_handler     = sli_ble_send_packet_tx_status,
#else
    .packet_status_handler     = NULL,
#endif
    .packet_type = SLI_BT_PACKET,
  }
};

sli_routing_table_t wifi_command_engine_routing_table = { .routing_table      = wifi_command_engine_routing_entries,
                                                          .routing_table_size = SLI_WIFI_COMMAND_ENGINE_MAX_PACKET };

static sli_command_engine_packet_type_configuration_t
  sli_wifi_command_engine_packet_type_configuration[SLI_WIFI_COMMAND_ENGINE_MAX_PACKET_TYPES] = {
    { .rx_event_handler            = sli_wifi_command_engine_rx_packet_handler,
      .pre_tx_handler              = NULL,
      .packet_processing_type      = SLI_COMMAND_ENGINE_COMMAND_PACKET,
      .route_packet_type           = SLI_WIFI_COMMAND_PACKET,
      .sync_response_queue         = &cmd_queues[SLI_WLAN_COMMON_CMD].rx_queue,
      .sync_response_event         = SL_WIFI_HOST_COMMON_RESPONSE_EVENT,
      .sync_response_event_id      = &cmd_queues[SLI_WLAN_COMMON_CMD].event_flags,
      .max_in_flight_command_count = 1,
      .async_response_queue        = &event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_COMMON_EVENT],
      .async_response_event_id     = &sli_wifi_event_engine_event_id,
      .async_response_event        = SLI_EVENT_ENGINE_ASYNC_EVENT },
    { .rx_event_handler            = sli_wifi_command_engine_rx_packet_handler,
      .pre_tx_handler              = NULL,
      .packet_processing_type      = SLI_COMMAND_ENGINE_COMMAND_PACKET,
      .route_packet_type           = SLI_WIFI_COMMAND_PACKET,
      .sync_response_queue         = &cmd_queues[SLI_WLAN_WIFI_CMD].rx_queue,
      .sync_response_event         = SL_WIFI_RESPONSE_EVENT,
      .sync_response_event_id      = &cmd_queues[SLI_WLAN_WIFI_CMD].event_flags,
      .max_in_flight_command_count = 1,
      .async_response_queue        = &event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_WIFI_EVENT],
      .async_response_event_id     = &sli_wifi_event_engine_event_id,
      .async_response_event        = SLI_EVENT_ENGINE_ASYNC_EVENT },
    { .rx_event_handler            = sli_wifi_command_engine_rx_packet_handler,
      .pre_tx_handler              = NULL,
      .packet_processing_type      = SLI_COMMAND_ENGINE_COMMAND_PACKET,
      .route_packet_type           = SLI_WIFI_COMMAND_PACKET,
      .sync_response_queue         = &cmd_queues[SLI_WLAN_NETWORK_CMD].rx_queue,
      .sync_response_event         = SL_WIFI_NETWORK_RESPONSE_EVENT,
      .sync_response_event_id      = &cmd_queues[SLI_WLAN_NETWORK_CMD].event_flags,
      .max_in_flight_command_count = 1,
      .async_response_queue        = &event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_NETWORK_EVENT],
      .async_response_event_id     = &sli_wifi_event_engine_event_id,
      .async_response_event        = SLI_EVENT_ENGINE_ASYNC_EVENT },
    { .rx_event_handler            = sli_wifi_command_engine_rx_packet_handler,
      .pre_tx_handler              = NULL,
      .packet_processing_type      = SLI_COMMAND_ENGINE_COMMAND_PACKET,
      .route_packet_type           = SLI_BT_PACKET,
      .sync_response_queue         = &cmd_queues[SLI_WLAN_BT_CMD].rx_queue,
      .sync_response_event         = SL_WIFI_BT_RESPONSE_EVENT,
      .sync_response_event_id      = &cmd_queues[SLI_WLAN_BT_CMD].event_flags,
      .max_in_flight_command_count = 1,
      .async_response_queue        = &event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_BLE_EVENT],
      .async_response_event_id     = &sli_wifi_event_engine_event_id,
      .async_response_event        = SLI_EVENT_ENGINE_ASYNC_EVENT },
    { .rx_event_handler            = sli_wifi_command_engine_rx_packet_handler,
      .pre_tx_handler              = NULL,
      .packet_processing_type      = SLI_COMMAND_ENGINE_COMMAND_PACKET,
      .route_packet_type           = SLI_WIFI_COMMAND_PACKET,
      .sync_response_queue         = &cmd_queues[SLI_WLAN_SOCKET_CMD].rx_queue,
      .sync_response_event         = SL_WIFI_SOCKET_RESPONSE_EVENT,
      .sync_response_event_id      = &cmd_queues[SLI_WLAN_SOCKET_CMD].event_flags,
      .max_in_flight_command_count = 1,
      .async_response_queue        = &event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_SOCKET_CMD_EVENT],
      .async_response_event_id     = &sli_wifi_event_engine_event_id,
      .async_response_event        = SLI_EVENT_ENGINE_ASYNC_EVENT }
  };

/******************************************************
 *               Global Variable Definitions
 ******************************************************/
sli_command_engine_t sli_wifi_command_engine = { 0 };

sli_command_engine_configuration_t sli_wifi_command_engine_config = {
  .name                      = "Wi-Fi Command Engine",
  .packet_type_count         = SLI_WIFI_COMMAND_ENGINE_MAX_PACKET_TYPES,
  .packet_type_configuration = sli_wifi_command_engine_packet_type_configuration,
  .get_packet_metadata       = sli_wifi_command_engine_get_packet_metadata,
  .priority                  = SL_WIFI_COMMAND_ENGINE_THREAD_PRIORITY,
  .stack_size                = SL_WIFI_COMMAND_ENGINE_STACK_SIZE,
  .routing_table             = &wifi_command_engine_routing_table,
  .error_event_queue         = &event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_ERROR_EVENT],
  .error_event_id            = &sli_wifi_event_engine_event_id,
  .error_event               = SLI_WIFI_ASYNC_EVENT_HANDLER_ERROR_EVENT,
  .metadata_buffer_pool_type = SLI_BUFFER_MANAGER_CE_METADATA_POOL,
  .error_buffer_pool_type    = SLI_BUFFER_MANAGER_CE_METADATA_POOL
};
