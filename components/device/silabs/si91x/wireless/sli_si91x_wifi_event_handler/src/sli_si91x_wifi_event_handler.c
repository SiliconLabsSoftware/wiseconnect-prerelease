/***************************************************************************/ /**
 * @file sli_si91x_wifi_event_handler.c
 * @brief
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "sli_si91x_wifi_event_handler.h"
#include "sli_command_engine.h"
#include "sli_event_engine.h"
#include "sli_wifi_command_engine_config.h"
#include "sl_si91x_host_interface.h"
#include "sl_si91x_types.h"
#include "sl_si91x_protocol_types.h"
#include "sl_si91x_driver.h"
#include "sl_wifi_constants.h"
#include "sl_wifi_types.h"
#include "sl_rsi_utility.h"
#include "cmsis_os2.h"
#include "cmsis_compiler.h"
#include "sl_si91x_core_utilities.h"
#include <string.h>
#include "sli_wifi_constants.h"
#include "sli_wifi_power_profile.h"
#include "sl_additional_status.h"

#ifdef SL_NET_COMPONENT_INCLUDED
#include "sl_net_types.h"
#include "sl_net_constants.h"
#include "sl_net_wifi_types.h"
#include "sli_net_types.h"
#include "sl_net.h"
#endif

#if !defined(__ZEPHYR__) && !defined(SLI_SI91X_LWIP_HOSTED_NETWORK_STACK) \
  && !defined(                                                            \
    SLI_SI91X_NETWORK_DUAL_STACK) // These headers are included only when neither LWIP nor dual stack is used.
#include "netinet_in.h"
#include "netinet6_in6.h"
#endif

#include "sli_wifi_utility.h"
#ifdef SLI_SI91X_OFFLOAD_NETWORK_STACK
#include "sl_si91x_socket_types.h"
#include "sl_si91x_socket_utility.h"
#include "sl_net_si91x_integration_handler.h"
#include "sl_si91x_socket_callback_framework.h"
#include "sl_si91x_socket_utility.h"
#include "sl_ip_types.h"
#include "sli_net_utility.h"
#else
// This macro defines a handler for dispatching network events.
// It is used to handle events related to the SI91x module
#ifndef SL_NET_EVENT_DISPATCH_HANDLER
#define SL_NET_EVENT_DISPATCH_HANDLER(metadata) \
  {                                             \
    UNUSED_PARAMETER(metadata);                 \
  }
#endif
#endif

#ifdef SLI_SI91X_ENABLE_BLE
#include "rsi_bt_common.h"
#endif

/******************************************************
 *               External Variable Definitions
 ******************************************************/
extern osMessageQueueId_t network_manager_queue;
extern sl_wifi_event_handler_t si91x_event_handler;

// Declaration of a global flag to indicate if background mode is enabled
extern bool bg_enabled;
/******************************************************
 *               Local Variable Definitions
 ******************************************************/

/******************************************************
 *               Global Variable Definitions
 ******************************************************/
osEventFlagsId_t sli_wifi_event_engine_event_id                  = NULL;
sli_queue_t event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_MAX_EVENTS] = { 0 };

/******************************************************
  *               Local Function Definitions
 ******************************************************/
static bool sli_is_command_in_flight_queue(sli_command_engine_t *instance, uint16_t packet_type, uint16_t frame_id);

static void sli_event_engine_common_event_handler(uint32_t event, void *data)
{
  UNUSED_PARAMETER(event);
  UNUSED_PARAMETER(data);
  sli_command_engine_response_t *response = (sli_command_engine_response_t *)data;
  sl_wifi_buffer_t *buffer                = sli_wifi_get_response_buffer(response);
  sli_buffer_manager_free_buffer(buffer);
  sli_buffer_manager_free_buffer(response);

  return;
}

static void sli_event_engine_wifi_event_handler(uint32_t event, void *data)
{
  UNUSED_PARAMETER(event);
  sli_command_engine_response_t *engine_response = (sli_command_engine_response_t *)data;
  sli_command_engine_metadata_t *metadata        = sli_wifi_get_response_metadata(engine_response);
  sl_wifi_buffer_t *buffer                       = sli_wifi_get_response_buffer(engine_response);

  sl_wifi_event_t wifi_event = 0;
  uint16_t frame_status      = 0;

  sl_wifi_system_packet_t *packet = (sl_wifi_system_packet_t *)sli_wifi_host_get_buffer_data(buffer, 0, NULL);
  frame_status                    = sli_wifi_get_frame_status(packet);
  SL_DEBUG_LOG("WE-> C: 0x%X, S: 0x%X.\n", packet->command, frame_status);

  if (SLI_WIFI_RSP_CARDREADY == packet->command) {
    sli_wifi_set_event(NCP_HOST_COMMON_RESPONSE_EVENT);
    sli_buffer_manager_free_buffer(buffer);
    sli_buffer_manager_free_buffer(engine_response);
    sli_buffer_manager_free_buffer(metadata);
    return;
  }

  buffer->node.node    = NULL;
  buffer->length       = packet->length;
  buffer->id           = 0;
  buffer->_reserved[0] = 0;
  buffer->_reserved[1] = 0;

  // Call event handler
  if (1) {
    wifi_event = sli_wifi_convert_event_to_sl_wifi_event(packet->command, frame_status);

    if (SLI_WLAN_RSP_SCAN_RESULTS == packet->command) {
      sli_handle_wifi_beacon(packet);
    }

    if (wifi_event != SL_WIFI_INVALID_EVENT && si91x_event_handler != NULL) {
      si91x_event_handler(wifi_event, buffer);
    }
  }

  sli_buffer_manager_free_buffer(buffer);
  sli_buffer_manager_free_buffer(engine_response);
  sli_buffer_manager_free_buffer(metadata);
  return;
}

static void sli_event_engine_bt_event_handler(uint32_t event, void *data)
{
  UNUSED_VARIABLE(event);
#ifdef SLI_SI91X_ENABLE_BLE
  sl_wifi_buffer_t *rx_buffer = (sl_wifi_buffer_t *)data;
  // Handle Bluetooth response
  rsi_driver_process_bt_resp_handler((sl_wifi_system_packet_t *)rx_buffer->data);
#endif
  sli_buffer_manager_free_buffer(data);
  return;
}

static void sli_event_engine_network_event_handler(uint32_t event, void *data)
{
  UNUSED_VARIABLE(event);
  sli_command_engine_response_t *response = (sli_command_engine_response_t *)data;
  sli_command_engine_metadata_t *metadata = sli_wifi_get_response_metadata(response);
  sl_wifi_buffer_t *buffer                = sli_wifi_get_response_buffer(response);

  SL_NET_EVENT_DISPATCH_HANDLER(response);

  sli_buffer_manager_free_buffer(buffer);
  sli_buffer_manager_free_buffer(response);
  sli_buffer_manager_free_buffer(metadata);
  return;
}

static void sli_event_engine_socket_cmd_event_handler(uint32_t event, void *data)
{
  UNUSED_VARIABLE(event);

  sli_command_engine_response_t *response = (sli_command_engine_response_t *)data;
  sli_command_engine_metadata_t *metadata = sli_wifi_get_response_metadata(response);
  sl_wifi_buffer_t *buffer                = sli_wifi_get_response_buffer(response);

  SL_NET_EVENT_DISPATCH_HANDLER(response);

  sli_buffer_manager_free_buffer(buffer);
  sli_buffer_manager_free_buffer(response);
  sli_buffer_manager_free_buffer(metadata);
  return;
}

static void sli_event_engine_socket_data_event_handler(uint32_t event, void *data)
{
  UNUSED_VARIABLE(event);
#ifdef SLI_SI91X_OFFLOAD_NETWORK_STACK
  // Handle the socket data event
  sli_si91x_socket_data_event_handler((sl_wifi_buffer_t *)data);
#else
  UNUSED_PARAMETER(data);
#endif
  return;
}

static sl_status_t sli_convert_command_engine_error_status_to_sl_status(
  const sl_command_engine_error_status_t *error_status)
{
  if (error_status == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  switch (*error_status) {
    case SLI_COMMAND_ENGINE_STATUS_FATAL_ERROR:
      return SL_STATUS_FAIL;
    case SLI_COMMAND_ENGINE_STATUS_INTERFACE_ERROR:
    case SLI_COMMAND_ENGINE_STATUS_COMMAND_TX_FAILED:
      return SL_STATUS_BUS_ERROR;
    case SLI_COMMAND_ENGINE_STATUS_MEMORY_ERROR:
      return SL_STATUS_ALLOCATION_FAILED;
    case SLI_COMMAND_ENGINE_STATUS_COMMAND_TX_TIMEOUT:
    case SLI_COMMAND_ENGINE_STATUS_COMMAND_PROCESSING_TIMEOUT:
      return SL_STATUS_TIMEOUT;
    default:
      return SL_STATUS_FAIL;
  }
}

static void sli_event_engine_error_event_handler(uint32_t event, void *data)
{
  UNUSED_VARIABLE(event);
  if (data == NULL) {
    return;
  }

  sl_status_t status =
    sli_convert_command_engine_error_status_to_sl_status((const sl_command_engine_error_status_t *)data);
  if (si91x_event_handler != NULL) {
    si91x_event_handler(SL_WIFI_COMMAND_ENGINE_STATUS_EVENT, (sl_wifi_buffer_t *)&status);
  }
  sli_buffer_manager_free_buffer(data);
  return;
}

static void sli_event_engine_nwp_log_event_handler(uint32_t event, void *data)
{
  UNUSED_VARIABLE(event);

  sl_wifi_buffer_t *buffer              = (sl_wifi_buffer_t *)data;
  const sl_wifi_system_packet_t *packet = (sl_wifi_system_packet_t *)buffer->data;

  // Special handling for the log event
  if (SLI_COMMON_RSP_NWP_LOGGING == packet->command) {
    // Extract packet length from descriptor
    uint16_t pkt_length = (uint16_t)(packet->desc[0] + ((packet->desc[1] & 0x0F) << 8));

    // Call the NWP log handler with the log data
    sli_handle_nwp_log_packet((const uint8_t *)packet->data, pkt_length);
  }

  // Free the buffer
  sli_buffer_manager_free_buffer(buffer);
  return;
}

// Weak implementation of the function to process data frames received from the SI91x module
__WEAK sl_status_t sl_si91x_host_process_data_frame(sl_wifi_interface_t interface, sl_wifi_buffer_t *buffer)
{
  UNUSED_PARAMETER(interface);
  UNUSED_PARAMETER(buffer);
  return SL_STATUS_OK;
}

/******************************************************
  *               Global Function Definitions
 ******************************************************/
sl_status_t sli_wifi_command_engine_get_packet_metadata(const sli_command_engine_t *instance,
                                                        void *buffer,
                                                        sli_command_engine_metadata_t *metadata)
{
  UNUSED_VARIABLE(instance);
  sl_wifi_buffer_t *rx_buffer     = (sl_wifi_buffer_t *)buffer;
  sl_wifi_system_packet_t *packet = NULL;
  sl_status_t status              = SL_STATUS_OK;
  uint8_t queue_id                = 0;
  uint16_t frame_type             = 0;
  uint16_t frame_status           = 0;

  // Process the frame
  packet = (sl_wifi_system_packet_t *)sli_wifi_host_get_buffer_data(rx_buffer, 0, NULL);
  if (packet == NULL) {
    return SL_STATUS_FAIL;
  }
  queue_id     = ((packet->desc[1] & 0xF0) >> 4);                      // Extract the queue ID
  frame_type   = (uint16_t)(packet->desc[2] + (packet->desc[3] << 8)); // Extract the frame type
  frame_status = sli_wifi_get_frame_status(packet);

  packet->desc[1]                      = (packet->desc[1] & 0x0F);
  metadata->tx_info.data_packet_length = packet->length;
  metadata->tx_info.frame_id           = frame_type;
  metadata->packet_status              = frame_status;

  SL_DEBUG_LOG("RX-> Q: %u, C: 0x%X, L: %u, S: 0x%x.\n",
               queue_id,
               frame_type,
               metadata->tx_info.data_packet_length,
               frame_status);

  switch (queue_id) {
    case SLI_WLAN_MGMT_Q: {
      switch (frame_type) {
        // Handle different frame types within the WLAN management queue
        case SLI_WIFI_RSP_OPERMODE:
        case SLI_COMMON_RSP_SOFT_RESET:
        case SLI_WIFI_RSP_PWRMODE: {
        }
          // intentional fallthrough
          __attribute__((fallthrough));
        case SLI_COMMON_RSP_GET_EFUSE_DATA:
        case SLI_COMMON_RSP_GET_RAM_DUMP:
        case SLI_WIFI_RSP_ANTENNA_SELECT:
        case SLI_COMMON_RSP_ENCRYPT_CRYPTO:
        case SLI_COMMON_RSP_SET_RTC_TIMER:
        case SLI_COMMON_RSP_GET_RTC_TIMER:
        case SLI_COMMON_RSP_TA_M4_COMMANDS:
        case SLI_COMMON_RSP_SET_CONFIG:
        case SLI_COMMON_RSP_GET_CONFIG:
        case SLI_COMMON_RSP_DEBUG_LOG:
        case SLI_COMMON_RSP_FEATURE_FRAME: {
          metadata->tx_info.packet_type = SLI_WIFI_COMMAND_ENGINE_COMMON_COMMAND_PACKET;
          break;
        }
        case SLI_WIFI_RSP_BAND:
        case SLI_WIFI_RSP_INIT:
        case SLI_WLAN_RSP_RADIO:
        case SLI_WIFI_RSP_EAP_CONFIG:
        case SLI_WLAN_RSP_SET_CERTIFICATE:
        case SLI_WIFI_RSP_HOST_PSK:
        case SLI_WIFI_RSP_JOIN:
        case SLI_WIFI_RSP_SCAN:
        case SLI_WLAN_RSP_SCAN_RESULTS:
        case SLI_WLAN_RSP_FW_VERSION:
        case SLI_WLAN_RSP_FULL_FW_VERSION:
        case SLI_WLAN_RSP_FWUP:
        case SLI_WIFI_RSP_DISCONNECT:
        case SLI_WIFI_RSP_AP_STOP:
        case SLI_WIFI_RSP_RSSI:
        case SLI_WIFI_RSP_TSF:
        case SLI_WIFI_RSP_AP_CONFIGURATION:
        case SLI_WLAN_RSP_WPS_METHOD:
        case SLI_WIFI_RSP_QUERY_NETWORK_PARAMS:
        case SLI_WIFI_RSP_SET_MAC_ADDRESS:
        case SLI_WLAN_RSP_SET_REGION:
        case SLI_WLAN_RSP_SET_REGION_AP:
        case SLI_WIFI_RSP_MAC_ADDRESS:
        case SLI_WLAN_RSP_EXT_STATS:
        case SLI_WIFI_RSP_GET_STATS:
        case SLI_WLAN_RSP_RX_STATS:
        case SLI_WLAN_RSP_MODULE_STATE:
        case SLI_WIFI_RSP_QUERY_GO_PARAMS:
        case SLI_WLAN_RSP_ROAM_PARAMS:
        case SLI_WLAN_RSP_HTTP_OTAF:
        case SLI_WLAN_RSP_CLIENT_CONNECTED:
        case SLI_WLAN_RSP_CLIENT_DISCONNECTED:
        case SLI_WLAN_RSP_CALIB_WRITE:
        case SLI_WLAN_RSP_GET_DPD_DATA:
        case SLI_WLAN_RSP_CALIB_READ:
        case SLI_WLAN_RSP_FREQ_OFFSET:
        case SLI_WLAN_RSP_EVM_OFFSET:
        case SLI_WLAN_RSP_EVM_WRITE:
        case SLI_WLAN_RSP_EFUSE_READ:
        case SLI_WLAN_RSP_FILTER_BCAST_PACKETS:
        case SLI_WLAN_RSP_TWT_PARAMS:
        case SLI_WLAN_RSP_TWT_ASYNC:
        case SLI_WLAN_RSP_TWT_AUTO_CONFIG:
        case SLI_WLAN_RSP_11AX_PARAMS:
        case SLI_WIFI_RSP_REJOIN_PARAMS:
        case SLI_WLAN_RSP_GAIN_TABLE:
        case SLI_WLAN_RSP_TX_TEST_MODE:
        case SLI_WLAN_RSP_TIMEOUT:
        case SLI_WIFI_RSP_BEACON_STOP:
        case SLI_WLAN_RSP_DYNAMIC_POOL:
        case SLI_WLAN_RSP_TRANSCEIVER_SET_CHANNEL:
        case SLI_WLAN_RSP_TRANSCEIVER_CONFIG_PARAMS:
        case SLI_WLAN_RSP_TRANSCEIVER_PEER_LIST_UPDATE:
        case SLI_WLAN_RSP_TRANSCEIVER_SET_MCAST_FILTER:
        case SLI_WLAN_RSP_TRANSCEIVER_FLUSH_DATA_Q:
        case SLI_WLAN_RSP_TRANSCEIVER_TX_DATA_STATUS:
        case SLI_WIFI_RSP_HT_CAPABILITIES:
        case SLI_WLAN_RSP_SET_MULTICAST_FILTER:
        case SLI_WIFI_RSP_CONFIG:
        case SLI_WIFI_RSP_BG_SCAN:
        case SLI_COMMON_RSP_ULP_NO_RAM_RETENTION:
        case SLI_WIFI_RSP_CARDREADY: {
          metadata->tx_info.packet_type = SLI_WIFI_COMMAND_ENGINE_WIFI_COMMAND_PACKET;
          if (frame_type == SLI_COMMON_RSP_ULP_NO_RAM_RETENTION) {
            // This frame will come, when the M4 is waken in without ram retention. This frame is equivalent to SLI_WIFI_RSP_CARDREADY
            frame_type                 = SLI_WIFI_RSP_CARDREADY;
            packet->desc[2]            = frame_type & 0xFF;
            packet->desc[3]            = (frame_type >> 8) & 0xFF;
            metadata->tx_info.frame_id = frame_type;
          }
          if (frame_type == SLI_WIFI_RSP_BG_SCAN && frame_status == SL_STATUS_OK) {
            bg_enabled = true;
          }
          break;
        }
        case SLI_WLAN_RSP_IPCONFV4:
        case SLI_WLAN_RSP_IPCONFV6:
        case SLI_WLAN_RSP_IPV4_CHANGE:
        case SLI_WLAN_RSP_OTA_FWUP:
        case SLI_WLAN_RSP_DNS_QUERY:
        case SLI_WLAN_RSP_DNS_SERVER_ADD:
        case SLI_WLAN_RSP_SET_SNI_EMBEDDED:
        case SLI_WLAN_RSP_MULTICAST:
        case SLI_WLAN_RSP_PING_PACKET:
        case SLI_WLAN_RSP_SNTP_CLIENT:
        case SLI_WLAN_RSP_EMB_MQTT_CLIENT:
        case SLI_WLAN_RSP_EMB_MQTT_PUBLISH_PKT:
        case SLI_WLAN_RSP_MQTT_REMOTE_TERMINATE:
        case SLI_WLAN_RSP_MDNSD:
        case SLI_WLAN_RSP_NAT:
        case SLI_WLAN_RSP_HTTP_CLIENT_GET:
        case SLI_WLAN_RSP_HTTP_CLIENT_POST:
        case SLI_WLAN_RSP_HTTP_CLIENT_POST_DATA:
        case SLI_WLAN_RSP_HTTP_ABORT:
        case SLI_WLAN_RSP_HTTP_CLIENT_PUT: {
          metadata->tx_info.packet_type = SLI_WIFI_COMMAND_ENGINE_NETWORK_COMMAND_PACKET;
          if (frame_type == SLI_WLAN_RSP_IPCONFV6) {
            // This frame will come, when the M4 is waken in without ram retention. This frame is equivalent to SLI_WIFI_RSP_CARDREADY
            frame_type                 = SLI_WLAN_REQ_IPCONFV6;
            packet->desc[2]            = frame_type & 0xFF;
            packet->desc[3]            = (frame_type >> 8) & 0xFF;
            metadata->tx_info.frame_id = frame_type;
          }
          break;
        }
        case SLI_WLAN_RSP_SOCKET_CONFIG:
        case SLI_WLAN_RSP_SOCKET_CREATE:
        case SLI_WLAN_RSP_SELECT_REQUEST:
        case SLI_WLAN_RSP_TCP_ACK_INDICATION: {
          metadata->tx_info.packet_type = SLI_WIFI_COMMAND_ENGINE_SOCKET_COMMAND_PACKET;
          break;
        }

        case SLI_WLAN_RSP_REMOTE_TERMINATE:
        case SLI_WLAN_RSP_CONN_ESTABLISH:
        case SLI_WLAN_RSP_SOCKET_CLOSE:
        case SLI_WLAN_RSP_SOCKET_READ_DATA:
        case SLI_WLAN_RSP_SOCKET_ACCEPT: {
#ifdef SLI_SI91X_OFFLOAD_NETWORK_STACK
          const sli_si91x_socket_t *socket = get_socket_from_packet(packet);
          if (socket != NULL) {
            metadata->tx_info.packet_type = (uint16_t)(socket->index + SLI_WIFI_COMMAND_ENGINE_MAX_PACKET_TYPES);
          } else {
            metadata->tx_info.packet_type = SLI_WIFI_COMMAND_ENGINE_SOCKET_COMMAND_PACKET;
          }
          if (frame_type == SLI_WLAN_RSP_CONN_ESTABLISH) {
            metadata->tx_info.frame_id = SLI_WLAN_REQ_SOCKET_ACCEPT;
          }
#else
          metadata->tx_info.packet_type = SLI_WIFI_COMMAND_ENGINE_SOCKET_COMMAND_PACKET;
#endif
          break;
        }
        default: {
          // frame_type doesn't match any known cases
          SL_DEBUG_LOG("Unknown frame type: %u\n", frame_type);
          status = SL_STATUS_INVALID_INDEX;
          break;
        }
      }
      break;
    }
    case SLI_WLAN_DATA_Q: {
      metadata->tx_info.packet_type = SLI_WIFI_COMMAND_ENGINE_SOCKET_COMMAND_PACKET;
      break;
    }
    case SLI_BT_Q: {
      metadata->tx_info.packet_type = SLI_WIFI_COMMAND_ENGINE_BLE_COMMAND_PACKET;
      break;
    }
    default: {
      // frame_type doesn't match any known cases
      SL_DEBUG_LOG("Unknown Queue type: %u\n", queue_id);
      status = SL_STATUS_INVALID_INDEX;
      break;
    }
  }

  return status;
}

/**
 * @brief Default match-all handler for flushing all packets when comparator is NULL
 * 
 * @param handle Queue handle (unused)
 * @param data Queue node data (unused)
 * @param node_match_data User data (unused)
 * @return true to match all packets
 */
static bool sli_flush_packet_match_all(const sli_queue_t *handle, void *data, const void *node_match_data)
{
  UNUSED_PARAMETER(handle);
  UNUSED_PARAMETER(node_match_data);
  UNUSED_PARAMETER(data);
  return true; // Match all packets
}

/**
 * @brief Compare function to flush MQTT packets
 * 
 * @param handle Queue handle (unused)
 * @param data Queue node data
 * @param context Context (unused)
 * @return true if packet is MQTT packet, false otherwise
 */
static bool sli_flush_packet_mqtt_compare_function(const sli_queue_t *handle, void *data, const void *context)
{
  UNUSED_PARAMETER(handle);
  UNUSED_PARAMETER(context);

  const sli_command_engine_metadata_t *metadata = (const sli_command_engine_metadata_t *)data;

  return (metadata->tx_info.frame_id == SLI_WLAN_REQ_EMB_MQTT_CLIENT);
}

/**
 * @brief Create a copy of a received system packet and post it to the Wi-Fi async
 *        event queue so the event engine can handle it.
 *
 * @param rx_buffer Pointer to the original @c sl_wifi_buffer_t that contains the system packet.
 */
static void sli_post_packet_to_event_engine(sl_wifi_buffer_t *rx_buffer)
{
  /* Validate parameter: caller retains ownership of rx_buffer */
  if (rx_buffer == NULL) {
    return;
  }

  sli_command_engine_response_t *response = NULL;
  sl_wifi_buffer_t *packet_buffer         = NULL;
  sl_status_t allocation_status;

  /*
   * Allocate an RX-style buffer to hold the copied packet. Use HYBRID so the
   * allocation will fall back to heap if the pool is exhausted.
   */
  allocation_status = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_RX_POOL,
                                                         SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                                         1000,
                                                         (sli_buffer_t *)&packet_buffer);
  if (allocation_status != SL_STATUS_OK) {
    /* Best-effort: give up if allocation fails */
    return;
  }

  /* Copy the packet payload into the newly allocated buffer */
  const sl_wifi_system_packet_t *rx_packet =
    (const sl_wifi_system_packet_t *)sli_wifi_host_get_buffer_data(rx_buffer, 0, NULL);
  sl_wifi_system_packet_t *destination_packet =
    (sl_wifi_system_packet_t *)sli_wifi_host_get_buffer_data(packet_buffer, 0, NULL);
  uint16_t payload_len = (uint16_t)(rx_packet->length & 0x0FFF);
  /* Copy header + payload; dest buffer layout matches source layout */
  memcpy(destination_packet, rx_packet, sizeof(sl_wifi_system_packet_t) + payload_len);

  /*
   * Allocate metadata wrapper (response object) which the event engine expects.
   * Use HYBRID allocation to improve robustness in low-memory situations.
   */
  allocation_status = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_METADATA_POOL,
                                                         SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                                         1000,
                                                         (sli_buffer_t *)&response);
  if (allocation_status != SL_STATUS_OK) {
    /* Clean up data buffer on failure and exit */
    sli_buffer_manager_free_buffer(packet_buffer);
    return;
  }

  /* Populate the response wrapper to indicate a packet-only response */
  response->data = packet_buffer;
  response->type = SLI_COMMAND_ENGINE_PACKET_ONLY_RESPONSE;

  /* Enqueue the wrapped response to the network event queue */
  if (SL_STATUS_OK
      != sli_queue_manager_enqueue(&event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_NETWORK_EVENT], (void *)response)) {
    /* On enqueue failure free both allocated objects */
    sli_buffer_manager_free_buffer(response);
    sli_buffer_manager_free_buffer(packet_buffer);
    return;
  }

  /* Signal the event engine to process the queued packet */
  osEventFlagsSet(sli_wifi_event_engine_event_id, SLI_EVENT_ENGINE_ASYNC_EVENT);
}

/**
 * @brief Process a single metadata node during queue flushing
 * 
 * This helper function processes a metadata node removed from a queue during flushing.
 * It determines the appropriate error status, handles sync/async responses, and enqueues
 * metadata to response queues as needed.
 * 
 * @param tx_metadata Pointer to the metadata node to process
 * @param packet_config Pointer to packet type configuration
 * @param frame_status Base error status to use
 */
static void sli_process_flush_metadata_node(sli_command_engine_metadata_t *tx_metadata,
                                            sli_command_engine_packet_type_configuration_t *packet_config,
                                            uint16_t frame_status)
{
  if (tx_metadata == NULL || packet_config == NULL) {
    return;
  }

  uint32_t elapsed_time = 0;

  // Check flags to determine response type
  if (tx_metadata->tx_info.flags & SLI_COMMAND_ENGINE_SYNC_RESPONSE_STATUS_PACKET) {
    // For sync responses: check timeout for both packet_queue and inflight_queue
    elapsed_time = sli_wifi_host_elapsed_time(tx_metadata->packet_start_tickcount);

    // If elapsed time exceeds timeout, just free resources (no dummy response needed)
    if (tx_metadata->tx_info.timeout == 0 || elapsed_time >= tx_metadata->tx_info.timeout) {
      // Command has timed out, free resources
      sli_buffer_manager_free_buffer(tx_metadata->tx_info.data_packet);
      sli_buffer_manager_free_buffer(tx_metadata);
      return;
    }

    // Command hasn't timed out, create dummy response
    tx_metadata->packet_status = frame_status;

    // Free data_packet and set to NULL, set data_packet_length to zero
    sli_buffer_manager_free_buffer(tx_metadata->tx_info.data_packet);
    tx_metadata->tx_info.data_packet        = NULL;
    tx_metadata->tx_info.data_packet_length = 0;

    // Enqueue metadata to sync response queue (application threads wait here via sli_wifi_receive_response_buffer)
    if (packet_config->sync_response_queue != NULL) {
      sli_queue_manager_enqueue(packet_config->sync_response_queue, (void *)tx_metadata);
    }

    // Set sync response event to wake up waiting threads
    // Application threads mostly wait on thread events, so use thread flags if thread ID is available
    if (tx_metadata->sync_resp_thread_id != NULL && packet_config->sync_response_event != 0) {
      osThreadFlagsSet(tx_metadata->sync_resp_thread_id, packet_config->sync_response_event);
    } else if (packet_config->sync_response_event_id != NULL && packet_config->sync_response_event != 0) {
      // Fallback to event flags if thread ID is not available
      osEventFlagsSet(*packet_config->sync_response_event_id, packet_config->sync_response_event);
    }
  } else {
    // For async responses: just free resources (no dummy error responses needed)
    // SOCKET_READ_DATA error response is handled by pending command check in sli_flush_socket_queues()
    sli_buffer_manager_free_buffer(tx_metadata->tx_info.data_packet);
    sli_buffer_manager_free_buffer(tx_metadata);
  }
  return;
}

/**
 * @brief Helper function to flush queues for a given packet type configuration
 * 
 * @param queue_info Pointer to queue info structure
 * @param packet_config Pointer to packet type configuration
 * @param compare_function Comparator function pointer (NULL to flush all packets)
 * @param user_data User-defined data for comparator function
 * @param frame_status Frame status to set in packet_status for flushed packets
 * @return sl_status_t Status of the operation (SL_STATUS_OK on success)
 */
static sl_status_t sli_flush_queue_for_packet_type(sli_command_engine_queue_info_t *queue_info,
                                                   sli_command_engine_packet_type_configuration_t *packet_config,
                                                   sli_flush_packet_compare_function_t compare_function,
                                                   const void *user_data,
                                                   uint16_t frame_status)
{
  if (frame_status == SL_STATUS_OK || queue_info == NULL || packet_config == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sli_command_engine_metadata_t *tx_metadata = NULL;
  sl_status_t status                         = SL_STATUS_OK;

  // Use provided comparator directly, or match-all handler if NULL
  // Pass user_data directly as node_match_data (same pattern as rx_packet_identity_handler)
  sli_queue_manager_node_match_handler_t match_handler = (compare_function != NULL) ? compare_function
                                                                                    : &sli_flush_packet_match_all;

  // Flush packet_queue (tx_packet_queues)
  // Use remove_node_from_queue to only remove packets that match comparator
  while (!SLI_QUEUE_MANAGER_IS_QUEUE_EMPTY(&queue_info->packet_queue)) {
    status = sli_queue_manager_remove_node_from_queue(&queue_info->packet_queue,
                                                      match_handler,
                                                      user_data,
                                                      (void **)&tx_metadata);

    // Break if no matching node found, queue is empty, or any error occurs
    if (status != SL_STATUS_OK) {
      break;
    }

    // Process the metadata node
    sli_process_flush_metadata_node(tx_metadata, packet_config, frame_status);
  }

  // Flush inflight_packet_queue (in_flight_queues)
  // Use remove_node_from_queue to only remove packets that match comparator
  while (!SLI_QUEUE_MANAGER_IS_QUEUE_EMPTY(&queue_info->inflight_packet_queue)) {
    status = sli_queue_manager_remove_node_from_queue(&queue_info->inflight_packet_queue,
                                                      match_handler,
                                                      user_data,
                                                      (void **)&tx_metadata);

    // Break if no matching node found, queue is empty, or any error occurs
    if (status != SL_STATUS_OK) {
      break;
    }

    if (tx_metadata->tx_status == SLI_COMMAND_ENGINE_PACKET_TX_INPROGRESS) {
      sli_command_engine_metadata_t *temp_metadata = NULL;

      // Allocate metadata buffer (hybrid allocation allows pool + heap fallback)
      sl_status_t allocation_status = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_METADATA_POOL,
                                                                         SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                                                         1000,
                                                                         (sli_buffer_t *)&temp_metadata);
      if (allocation_status != SL_STATUS_OK) {
        sli_buffer_manager_free_buffer(tx_metadata);
        return allocation_status;
      }
      // Copy original metadata to temp metadata
      memcpy(temp_metadata, tx_metadata, sizeof(sli_command_engine_metadata_t));
      temp_metadata->tx_info.data_packet        = NULL;
      temp_metadata->tx_info.data_packet_length = 0;

      // Process the temp_metadata node
      sli_process_flush_metadata_node(temp_metadata, packet_config, frame_status);

      tx_metadata->tx_status = SLI_COMMAND_ENGINE_PACKET_FLUSHED;

    } else {

      // Process the metadata node
      sli_process_flush_metadata_node(tx_metadata, packet_config, frame_status);

      // Decrement in_flight_command_count for each in-flight command removed from inflight queue
      // Note: This is only done for inflight_queue, not for packet_queue
      if (queue_info->in_flight_command_count > 0) {
        queue_info->in_flight_command_count--;
      }
    }
  }

  return SL_STATUS_OK;
}

/**
 * @brief Flush all tx_packet_queues and in_flight_queues and generate dummy RX packets
 * 
 * This function iterates through all packet types (static and dynamic) in the command engine,
 * flushes all packets from both the packet_queue and inflight_packet_queue. For sync responses,
 * it sets the provided error_status and enqueues metadata to sync response queues with appropriate events.
 * For async commands, packets are simply freed without generating dummy responses.
 * 
 * @param instance Pointer to the command engine instance
 * @param error_status Error status to set in packet_status for flushed packets
 * @return sl_status_t Status of the operation
 */
sl_status_t sli_flush_all_command_engine_static_queues(sli_command_engine_t *instance, uint16_t error_status)
{
  if (instance == NULL || error_status == SL_STATUS_OK || instance->queue_info == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  // Process static packet types
  for (uint8_t packet_type = 0; packet_type < instance->config.packet_type_count; packet_type++) {
    if (packet_type == SLI_WIFI_COMMAND_ENGINE_WIFI_COMMAND_PACKET) {
      continue;
    }
    sli_command_engine_queue_info_t *queue_info = &instance->queue_info[packet_type];
    sli_command_engine_packet_type_configuration_t *packet_config =
      &instance->config.packet_type_configuration[packet_type];

    sli_flush_queue_for_packet_type(queue_info, packet_config, NULL, NULL, error_status);
  }
  return SL_STATUS_OK;
}

/**
 * @brief Handle flushing logic based on packet command and frame status
 * 
 * This function analyzes the received packet command and frame status to determine
 * if queues need to be flushed. It handles various scenarios like join failures,
 * IP configuration failures, disconnect events, and client disconnection events.
 * 
 * @param instance Pointer to the command engine instance
 * @param packet_type Packet type from the command engine
 * @param rx_packet Pointer to the received packet structure
 * @return sl_status_t Status of the operation
 */
static sl_status_t sli_handle_packet_flush_logic(sli_command_engine_t *instance,
                                                 uint16_t packet_type,
                                                 sl_wifi_buffer_t *rx_buffer)
{
  sl_status_t status = SL_STATUS_OK;

  const sl_wifi_system_packet_t *rx_packet =
    (const sl_wifi_system_packet_t *)sli_wifi_host_get_buffer_data(rx_buffer, 0, NULL);

  if (rx_packet == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  uint16_t frame_status = sli_wifi_get_frame_status(rx_packet);

  switch (rx_packet->command) {
    case SLI_WIFI_RSP_JOIN:
      // Check if the received packet is a response to a join command and status is not OK
      if (frame_status != SL_STATUS_OK) {
        // Reset current performance profile and set it to high performance
        sli_reset_coex_current_performance_profile();

        if (!sli_is_command_in_flight_queue(instance, packet_type, SLI_WIFI_REQ_JOIN)) {
          status = sli_flush_all_command_engine_static_queues(instance, frame_status);
          VERIFY_STATUS_AND_RETURN(status);
#ifdef SLI_SI91X_OFFLOAD_NETWORK_STACK
          uint8_t vap_id = SL_WIFI_CLIENT_VAP_ID;
          status = sli_flush_all_socket_queues(instance, (uint16_t)SL_STATUS_SI91X_SOCKET_CLOSED, &vap_id, NULL);
          VERIFY_STATUS_AND_RETURN(status);
#endif
          // Post a copy of the received packet to the Wi-Fi async event engine to update wifi join fail event to network modules
          sli_post_packet_to_event_engine(rx_buffer);
        }
      }
      break;

    case SLI_WLAN_RSP_IPCONFV4:
      // Check for IPv4 configuration failure
      if (frame_status != SL_STATUS_OK
          && (!sli_is_command_in_flight_queue(instance, packet_type, SLI_WLAN_REQ_IPCONFV4))) {
        status = sli_flush_all_command_engine_static_queues(instance, frame_status);
        VERIFY_STATUS_AND_RETURN(status);
#ifdef SLI_SI91X_OFFLOAD_NETWORK_STACK
        uint8_t vap_id = SL_WIFI_CLIENT_VAP_ID;
        status         = sli_flush_all_socket_queues(instance, (uint16_t)SL_STATUS_SI91X_SOCKET_CLOSED, &vap_id, NULL);
        VERIFY_STATUS_AND_RETURN(status);
#endif
        // Post a copy of the received packet to the Wi-Fi async event engine to update wifi disconnect event to network modules
        sli_post_packet_to_event_engine(rx_buffer);
      }
      break;

    case SLI_WLAN_RSP_IPCONFV6:
      // Check for IPv6 configuration failure
      if (frame_status != SL_STATUS_OK
          && (!sli_is_command_in_flight_queue(instance, packet_type, SLI_WLAN_REQ_IPCONFV6))) {
        status = sli_flush_all_command_engine_static_queues(instance, frame_status);
        VERIFY_STATUS_AND_RETURN(status);
#ifdef SLI_SI91X_OFFLOAD_NETWORK_STACK
        uint8_t vap_id = SL_WIFI_CLIENT_VAP_ID;
        status         = sli_flush_all_socket_queues(instance, (uint16_t)SL_STATUS_SI91X_SOCKET_CLOSED, &vap_id, NULL);
        VERIFY_STATUS_AND_RETURN(status);
#endif
      }
      break;

    case SLI_WLAN_RSP_IPV4_CHANGE:
      // Check for IPv4 address change failure
      {
        uint16_t error_status = (uint16_t)SL_STATUS_SI91X_IP_ADDRESS_ERROR;
        status                = sli_flush_all_command_engine_static_queues(instance, error_status);
        VERIFY_STATUS_AND_RETURN(status);
#ifdef SLI_SI91X_OFFLOAD_NETWORK_STACK
        uint8_t vap_id = SL_WIFI_CLIENT_VAP_ID;
        status         = sli_flush_all_socket_queues(instance, (uint16_t)SL_STATUS_SI91X_SOCKET_CLOSED, &vap_id, NULL);
        VERIFY_STATUS_AND_RETURN(status);
#endif
      }
      break;

    case SLI_WIFI_RSP_DISCONNECT:
      // Check for client disconnect (successful disconnect from client VAP)
      if (frame_status == SL_STATUS_OK
          && (SL_WIFI_CLIENT_VAP_ID == sli_wifi_get_vap_id_from_operation_mode(rx_packet))) {
        // Reset current performance profile and set it to high performance
        sli_reset_coex_current_performance_profile();
        status = sli_flush_all_command_engine_static_queues(instance, (uint16_t)SL_STATUS_WIFI_CONNECTION_LOST);
        VERIFY_STATUS_AND_RETURN(status);
#ifdef SLI_SI91X_OFFLOAD_NETWORK_STACK
        uint8_t vap_id = SL_WIFI_CLIENT_VAP_ID;
        status         = sli_flush_all_socket_queues(instance, (uint16_t)SL_STATUS_SI91X_SOCKET_CLOSED, &vap_id, NULL);
        VERIFY_STATUS_AND_RETURN(status);
#endif
      }
      break;

#ifdef SLI_SI91X_OFFLOAD_NETWORK_STACK
    case SLI_WLAN_RSP_CLIENT_DISCONNECTED:
      // Check for AP client disconnection (client disconnected from AP)
      {
        sl_ip_address_t dest_ip_address = { 0 };
        status                          = sli_si91x_get_dest_ip_address_from_ap_client_disconnect_resp(
          (const sli_si91x_ap_disconnect_resp_t *)rx_packet->data,
          &dest_ip_address);
        uint8_t vap_id = SL_WIFI_AP_VAP_ID;
        if (status == SL_STATUS_OK && !sli_wifi_is_ip_address_zero(&dest_ip_address)) {
          status =
            sli_flush_all_socket_queues(instance, (uint16_t)SL_STATUS_SI91X_SOCKET_CLOSED, &vap_id, &dest_ip_address);
          VERIFY_STATUS_AND_RETURN(status);
          // Post a copy of the received packet to the Wi-Fi async event engine to update AP client disconnect event to network modules
          sli_post_packet_to_event_engine(rx_buffer);
        }
      }
      break;

    case SLI_WLAN_RSP_REMOTE_TERMINATE: {
      status = sli_flush_socket_queues(instance, packet_type, (uint16_t)SL_STATUS_SI91X_SOCKET_CLOSED);
      VERIFY_STATUS_AND_RETURN(status);
    } break;
    case SLI_WIFI_RSP_AP_STOP: {
      // Check for AP stop event
      if (frame_status == SL_STATUS_OK) {
        uint8_t vap_id = SL_WIFI_AP_VAP_ID;
        status         = sli_flush_all_socket_queues(instance, (uint16_t)SL_STATUS_SI91X_SOCKET_CLOSED, &vap_id, NULL);
        VERIFY_STATUS_AND_RETURN(status);

        // Post a copy of the received packet to the Wi-Fi async event engine to update AP stop event to network modules
        sli_post_packet_to_event_engine(rx_buffer);
      }
    } break;
#endif

    case SLI_WLAN_RSP_MQTT_REMOTE_TERMINATE: {
      SL_DEBUG_LOG("Received MQTT remote terminate, flushing the queues \r\n");
      status = sli_flush_queue_for_packet_type(
        &instance->queue_info[SLI_WIFI_COMMAND_ENGINE_NETWORK_COMMAND_PACKET],
        &instance->config.packet_type_configuration[SLI_WIFI_COMMAND_ENGINE_NETWORK_COMMAND_PACKET],
        sli_flush_packet_mqtt_compare_function,
        (const void *)rx_packet,
        frame_status);

      VERIFY_STATUS_AND_RETURN(status);
      break;
    }

    default:
      // No special handling needed for other commands
      break;
  }

  return SL_STATUS_OK;
}

#ifdef SLI_SI91X_OFFLOAD_NETWORK_STACK
sl_status_t sli_flush_socket_queues(sli_command_engine_t *instance, uint16_t packet_type, uint16_t error_status)
{
  if (error_status == SL_STATUS_OK || instance == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  // Calculate socket index from packet_type (socket queues are dynamic queues)
  // packet_type = socket->index + SLI_WIFI_COMMAND_ENGINE_MAX_PACKET_TYPES
  // socket->index = packet_type - SLI_WIFI_COMMAND_ENGINE_MAX_PACKET_TYPES
  if (packet_type < SLI_WIFI_COMMAND_ENGINE_MAX_PACKET_TYPES) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  uint8_t socket_index = (uint8_t)(packet_type - SLI_WIFI_COMMAND_ENGINE_MAX_PACKET_TYPES);

  // Get socket from socket index (sli_si91x_sockets is declared in sl_si91x_socket_utility.h)
  if (socket_index >= SLI_NUMBER_OF_SOCKETS) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  sli_si91x_socket_t *socket = sli_si91x_sockets[socket_index];
  if (socket == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  // Socket queues are always dynamic queues (packet_type >= MAX_PACKET_TYPES)
  // Find the matching dynamic node by iterating through the linked list and comparing packet_type
  sli_command_engine_packet_type_configuration_node_t *dynamic_node = instance->dynamic_packet_type;
  bool found_dynamic_node                                           = false;

  while (dynamic_node != NULL) {
    // Compare the dynamic node's packet_type with the socket's packet_type
    if (dynamic_node->packet_type == packet_type) {
      // Found the matching dynamic node - flush its queues
      sli_flush_queue_for_packet_type(&dynamic_node->queue_info,
                                      &dynamic_node->packet_config,
                                      NULL,
                                      NULL,
                                      error_status);
      found_dynamic_node = true;
      break;
    }

    dynamic_node = dynamic_node->next;
  }
  if (found_dynamic_node == false) {
    return SL_STATUS_NOT_FOUND;
  }

  // Check if there's a pending read command that's not in the queue
  // SOCKET_READ_DATA is async but expects metadata response when flushing
  if (socket->Is_receive_cmd_pending == true) {
    sli_command_engine_metadata_t *metadata = NULL;
    sl_status_t status                      = SL_STATUS_OK;

    // Allocate metadata buffer (hybrid allocation allows pool + heap fallback)
    status = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_METADATA_POOL,
                                                SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                                1000,
                                                (sli_buffer_t *)&metadata);
    if (status == SL_STATUS_OK && metadata != NULL) {
      // Initialize metadata structure
      memset(metadata, 0, sizeof(sli_command_engine_metadata_t));

      // Set error status and clear data packet fields
      metadata->packet_status              = error_status;
      metadata->tx_info.data_packet        = NULL;
      metadata->tx_info.data_packet_length = 0;
      metadata->tx_info.flags              = 0;
      metadata->tx_info.packet_id          = 0;
      metadata->instance                   = instance;

      // Enqueue metadata to sync response queue (application threads wait here)
      if (socket->socket_packet_type_configuration.sync_response_queue != NULL) {
        sli_queue_manager_enqueue(socket->socket_packet_type_configuration.sync_response_queue, (void *)metadata);
      }

      // Set sync response event to wake up waiting threads
      if (socket->socket_packet_type_configuration.sync_response_event_id != NULL
          && socket->socket_packet_type_configuration.sync_response_event != 0) {
        osEventFlagsSet(*socket->socket_packet_type_configuration.sync_response_event_id,
                        socket->socket_packet_type_configuration.sync_response_event);
      }
    }
  }

  return SL_STATUS_OK;
}

sl_status_t sli_flush_all_socket_queues(sli_command_engine_t *instance,
                                        uint16_t error_status,
                                        const uint8_t *vap_id,
                                        const sl_ip_address_t *dest_ip_address)
{
  if (instance == NULL || vap_id == NULL || error_status == SL_STATUS_OK) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t status                              = SL_STATUS_OK;
  sl_wifi_operation_mode_t current_operation_mode = sli_wifi_get_opermode();
  uint8_t socket_vap_id = (current_operation_mode == SL_WIFI_ACCESS_POINT_MODE) ? SL_WIFI_AP_VAP_ID
                                                                                : SL_WIFI_CLIENT_VAP_ID;

  // Loop through all sockets
  for (uint8_t index = 0; index < SLI_NUMBER_OF_SOCKETS; index++) {
    // Check if the socket exists
    if (sli_si91x_sockets[index] == NULL) {
      continue;
    }

    // In concurrent mode, use socket's vap_id directly (0 for station, 1 for AP)
    if (current_operation_mode == SL_SI91X_CONCURRENT_MODE) {
      socket_vap_id = sli_si91x_sockets[index]->vap_id;
    }
    bool should_flush = (socket_vap_id == *vap_id);

    // Filter by destination IP address if provided
    if (should_flush && dest_ip_address != NULL) {
      bool is_same = false;
      if (dest_ip_address->type == SL_IPV4) {
        const struct sockaddr_in *socket_address = (struct sockaddr_in *)&sli_si91x_sockets[index]->remote_address;
        is_same = (memcmp(dest_ip_address->ip.v4.bytes, &socket_address->sin_addr.s_addr, SL_IPV4_ADDRESS_LENGTH) == 0);
      } else {
        const struct sockaddr_in6 *ipv6_socket_address = &sli_si91x_sockets[index]->remote_address;
#ifdef SLI_SI91X_NETWORK_DUAL_STACK
        is_same =
          (memcmp(dest_ip_address->ip.v6.bytes, &ipv6_socket_address->sin6_addr.un.u8_addr, SL_IPV6_ADDRESS_LENGTH)
           == 0);
#else
#ifndef __ZEPHYR__
        is_same = (memcmp(dest_ip_address->ip.v6.bytes,
                          &ipv6_socket_address->sin6_addr.__u6_addr.__u6_addr8,
                          SL_IPV6_ADDRESS_LENGTH)
                   == 0);
#else
        is_same =
          (memcmp(dest_ip_address->ip.v6.bytes, &ipv6_socket_address->sin6_addr.s6_addr, SL_IPV6_ADDRESS_LENGTH) == 0);
#endif
#endif
      }
      should_flush = is_same;
    }

    // Flush the queues for the current socket if it matches all filters
    if (should_flush) {
      uint16_t socket_packet_type =
        (uint16_t)(sli_si91x_sockets[index]->index + SLI_WIFI_COMMAND_ENGINE_MAX_PACKET_TYPES);
      status = sli_flush_socket_queues(instance, socket_packet_type, error_status);
      // If flushing fails, return the error status immediately
      if (status != SL_STATUS_OK) {
        return status;
      }
    }
  }

  // Return SL_STATUS_OK if all sockets were processed successfully
  return SL_STATUS_OK;
}

#endif // SLI_SI91X_OFFLOAD_NETWORK_STACK

sl_status_t sli_wifi_command_engine_rx_packet_handler(sli_command_engine_t *instance, uint16_t packet_type, void *data)
{
  if (instance == NULL || data == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  // Handle flushing logic based on packet command and frame status
  sl_status_t status = sli_handle_packet_flush_logic(instance, packet_type, (sl_wifi_buffer_t *)data);
  VERIFY_STATUS_AND_RETURN(status);

  return SL_STATUS_OK;
}

static bool sli_is_command_in_flight_queue(sli_command_engine_t *instance, uint16_t packet_type, uint16_t frame_id)
{
  // verify whether instance is null or packet_type is out of range
  if (instance == NULL || packet_type >= instance->config.packet_type_count) {
    return false;
  }

  sli_command_engine_queue_info_t *queue_info = &instance->queue_info[packet_type];
  sli_queue_node_t *in_flight_queue_node      = queue_info->inflight_packet_queue.head;
  // If the join packet is in flight, then, this is a response to a join command
  while (in_flight_queue_node != NULL) {
    const sli_command_engine_metadata_t *tx_metadata =
      (const sli_command_engine_metadata_t *)in_flight_queue_node->data;
    if (tx_metadata->tx_info.frame_id == frame_id) {
      return true;
    }
    in_flight_queue_node = in_flight_queue_node->next;
  }
  return false;
}

sl_status_t sli_wifi_command_engine_packet_handler(void *packet,
                                                   uint32_t packet_size,
                                                   sli_routing_utility_packet_status_handler_t packet_status_handler,
                                                   void *context)
{
  UNUSED_PARAMETER(packet_status_handler);
  UNUSED_PARAMETER(context);
  UNUSED_PARAMETER(packet_size);

  sl_status_t status = sli_command_engine_receive_packet(&sli_wifi_command_engine, packet);
  return status;
}

sl_status_t sli_wifi_data_packet_handler(void *rx_buffer,
                                         uint32_t packet_size,
                                         sli_routing_utility_packet_status_handler_t packet_status_handler,
                                         void *context)
{
  UNUSED_PARAMETER(packet_status_handler);
  UNUSED_PARAMETER(context);
  UNUSED_PARAMETER(packet_size);

  sl_wifi_system_packet_t *rx_packet = (sl_wifi_system_packet_t *)((sl_wifi_buffer_t *)rx_buffer)->data;

  // Clear the queue ID bits in desc[1] to avoid misinterpretation
  rx_packet->desc[1] = (rx_packet->desc[1] & 0x0F);

  if (rx_packet->command == SLI_RECEIVE_RAW_DATA) {

#if defined(SLI_SI91X_OFFLOAD_NETWORK_STACK) && !defined(SLI_SI91X_NETWORK_DUAL_STACK)

    // Offload only mode is enabled
    // Passes the asynchronous socket packet to the event engine for further processing.
    sli_queue_manager_enqueue(&event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_SOCKET_DATA_EVENT], rx_buffer);

    // set event to the event engine
    osEventFlagsSet(sli_wifi_event_engine_event_id, SLI_EVENT_ENGINE_ASYNC_EVENT);

#elif defined(SLI_SI91X_NETWORK_DUAL_STACK)

    extern bool bypass_mode_enabled;

    if (!bypass_mode_enabled) {

      // Passes the asynchronous socket packet to the event engine for further processing.
      sli_queue_manager_enqueue(&event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_SOCKET_DATA_EVENT], rx_buffer);

      // set event to the event engine
      osEventFlagsSet(sli_wifi_event_engine_event_id, SLI_EVENT_ENGINE_ASYNC_EVENT);

    } else {
      // If SLI_SI91X_OFFLOAD_NETWORK_STACK is defined and dual stack mode is enabled, process the raw data frame.
      sl_si91x_host_process_data_frame(SL_WIFI_CLIENT_INTERFACE, rx_buffer);
      sli_buffer_manager_free_buffer(rx_buffer);
    }
#else
    // In bypass mode, process the data frame and free the buffer.
    sl_si91x_host_process_data_frame(SL_WIFI_CLIENT_INTERFACE, rx_buffer);
    sli_buffer_manager_free_buffer(rx_buffer);
#endif
  } else if (rx_packet->command == SLI_NET_DUAL_STACK_RX_RAW_DATA_FRAME) {
    // If network dual stack mode is enabled, process the received data frame of type 0x1 and free the buffer.
    sl_si91x_host_process_data_frame(SL_WIFI_CLIENT_INTERFACE, rx_buffer);
    sli_buffer_manager_free_buffer(rx_buffer);
  } else if (rx_packet->command == SLI_SI91X_WIFI_RX_DOT11_DATA) {
    // Passes the asynchronous Wi-Fi packet to the event engine for further processing.
    sli_queue_manager_enqueue(&event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_WIFI_EVENT], rx_buffer);
    // set event to the event engine
    osEventFlagsSet(sli_wifi_event_engine_event_id, SLI_EVENT_ENGINE_ASYNC_EVENT);
  }

  return SL_STATUS_OK;
}

sl_status_t sli_wifi_ble_packet_handler(void *rx_buffer,
                                        uint32_t packet_size,
                                        sli_routing_utility_packet_status_handler_t packet_status_handler,
                                        void *context)
{
  UNUSED_PARAMETER(packet_status_handler);
  UNUSED_PARAMETER(context);
  UNUSED_PARAMETER(packet_size);

  sl_wifi_system_packet_t *rx_packet = (sl_wifi_system_packet_t *)((sl_wifi_buffer_t *)rx_buffer)->data;

  // Clear the queue ID bits in desc[1] to avoid misinterpretation
  rx_packet->desc[1] = (rx_packet->desc[1] & 0x0F);

  // Pass the BLE packet to the event engine for further processing.
  sli_queue_manager_enqueue(&event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_BLE_EVENT], rx_buffer);
  // set event to the event engine
  osEventFlagsSet(sli_wifi_event_engine_event_id, SLI_EVENT_ENGINE_ASYNC_EVENT);

  return SL_STATUS_OK;
}

sl_status_t sli_wifi_nwp_log_packet_handler(void *rx_buffer,
                                            uint32_t packet_size,
                                            sli_routing_utility_packet_status_handler_t packet_status_handler,
                                            void *context)
{
  UNUSED_PARAMETER(packet_status_handler);
  UNUSED_PARAMETER(context);
  UNUSED_PARAMETER(packet_size);

  sl_wifi_system_packet_t *rx_packet = (sl_wifi_system_packet_t *)((sl_wifi_buffer_t *)rx_buffer)->data;

  // Clear the queue ID bits in desc[1] to avoid misinterpretation
  rx_packet->desc[1] = (rx_packet->desc[1] & 0x0F);

  // Pass the NWP log packet to the dedicated NWP log event handler
  sli_queue_manager_enqueue(&event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_NWP_LOG_EVENT], rx_buffer);
  // set event to the event engine
  osEventFlagsSet(sli_wifi_event_engine_event_id, SLI_EVENT_ENGINE_ASYNC_EVENT);

  return SL_STATUS_OK;
}

sl_status_t sli_wifi_event_engine_init(void)
{
  sl_status_t status = SL_STATUS_OK;

  for (uint16_t i = 0; i < SLI_WIFI_ASYNC_EVENT_HANDLER_MAX_EVENTS; i++) {
    status = sli_queue_manager_init(&event_queue[i], SLI_BUFFER_MANAGER_QUEUE_NODE_POOL);
    VERIFY_STATUS_AND_RETURN(status);
  }

  // Initialize the event engine
  status = sli_event_engine_init(&sli_wifi_event_engine_event_id);
  VERIFY_STATUS_AND_RETURN(status);

  // Register the Wi-Fi event handler for asynchronous Wi-Fi events
  status = sli_event_engine_register_event(&event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_COMMON_EVENT],
                                           SLI_WIFI_ASYNC_EVENT_HANDLER_COMMON_EVENT,
                                           sli_event_engine_common_event_handler);
  if (SL_STATUS_OK != status) {
    // If registration fails, deinitialize the event engine and return the error status
    sli_event_engine_deinit();
    return status;
  }

  // Register the error event handler for asynchronous error events
  status = sli_event_engine_register_event(&event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_WIFI_EVENT],
                                           SLI_WIFI_ASYNC_EVENT_HANDLER_WIFI_EVENT,
                                           sli_event_engine_wifi_event_handler);
  if (SL_STATUS_OK != status) {
    // If registration fails, deinitialize the event engine
    sli_event_engine_deinit();
  }

  // Register the Wi-Fi event handler for asynchronous Wi-Fi events
  status = sli_event_engine_register_event(&event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_BLE_EVENT],
                                           SLI_WIFI_ASYNC_EVENT_HANDLER_BLE_EVENT,
                                           sli_event_engine_bt_event_handler);
  if (SL_STATUS_OK != status) {
    // If registration fails, deinitialize the event engine and return the error status
    sli_event_engine_deinit();
    return status;
  }

  // Register the error event handler for asynchronous error events
  status = sli_event_engine_register_event(&event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_NETWORK_EVENT],
                                           SLI_WIFI_ASYNC_EVENT_HANDLER_NETWORK_EVENT,
                                           sli_event_engine_network_event_handler);
  if (SL_STATUS_OK != status) {
    // If registration fails, deinitialize the event engine
    sli_event_engine_deinit();
  }

  // Register the Wi-Fi event handler for asynchronous Wi-Fi events
  status = sli_event_engine_register_event(&event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_SOCKET_CMD_EVENT],
                                           SLI_WIFI_ASYNC_EVENT_HANDLER_SOCKET_CMD_EVENT,
                                           sli_event_engine_socket_cmd_event_handler);
  if (SL_STATUS_OK != status) {
    // If registration fails, deinitialize the event engine and return the error status
    sli_event_engine_deinit();
    return status;
  }

  // Register the Wi-Fi event handler for asynchronous Wi-Fi events
  status = sli_event_engine_register_event(&event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_SOCKET_DATA_EVENT],
                                           SLI_WIFI_ASYNC_EVENT_HANDLER_SOCKET_DATA_EVENT,
                                           sli_event_engine_socket_data_event_handler);
  if (SL_STATUS_OK != status) {
    // If registration fails, deinitialize the event engine and return the error status
    sli_event_engine_deinit();
    return status;
  }

  // Register the error event handler for asynchronous error events
  status = sli_event_engine_register_event(&event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_ERROR_EVENT],
                                           SLI_WIFI_ASYNC_EVENT_HANDLER_ERROR_EVENT,
                                           sli_event_engine_error_event_handler);
  if (SL_STATUS_OK != status) {
    // If registration fails, deinitialize the event engine
    sli_event_engine_deinit();
    return status;
  }

  // Register the NWP log event handler for asynchronous NWP logging events
  status = sli_event_engine_register_event(&event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_NWP_LOG_EVENT],
                                           SLI_WIFI_ASYNC_EVENT_HANDLER_NWP_LOG_EVENT,
                                           sli_event_engine_nwp_log_event_handler);
  if (SL_STATUS_OK != status) {
    // If registration fails, deinitialize the event engine
    sli_event_engine_deinit();
  }

  return status;
}

#ifdef SLI_SI91X_ENABLE_BLE
void sli_ble_send_packet_tx_status(uint16_t packet_type, sl_status_t status, void *context)
{
  UNUSED_PARAMETER(packet_type); // Packet type not needed in this callback
  const sl_wifi_system_packet_t *packet = (const sl_wifi_system_packet_t *)context;
  // Notify BLE stack that transmission is done
  rsi_bt_common_tx_done(packet, status);
  sli_buffer_manager_free_buffer(context);
  return;
}
#endif
