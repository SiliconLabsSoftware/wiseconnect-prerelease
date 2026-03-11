/***************************************************************************/ /**
 * @file
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
#include "sl_si91x_protocol_types.h"
#include "sl_si91x_constants.h"
#include "sl_si91x_driver.h"
#include "sl_wifi_constants.h"
#include "sl_wifi_credentials.h"
#include "sl_status.h"
#include "sl_constants.h"
#include "sl_wifi_types.h"
#include "sl_rsi_utility.h"
#include "sli_hal_si91x.h"
#include "sli_wifi_constants.h"
#include "cmsis_os2.h" // CMSIS RTOS2
#include "sl_cmsis_utility.h"
#include "sl_si91x_types.h"
#include "sli_wifi_command_engine.h"
#include "sl_si91x_core_utilities.h"
#ifndef __ZEPHYR__
#include "sli_cmsis_os2_ext_task_register.h"
#endif
#ifdef SLI_SI91X_OFFLOAD_NETWORK_STACK
#include "sl_si91x_socket_constants.h"
#include "sl_si91x_socket_utility.h"
#endif
#include "sl_core.h"
#include <string.h>
#include "assert.h"
#include "sli_wifi_utility.h"
#include "sli_wifi_power_profile.h"
#include "sli_wifi.h"
#include "sli_queue_manager.h"
#include "sl_string.h"

static bool sli_si91x_tx_command_status              = false;
static volatile bool power_save_sequence_in_progress = false;
extern bool global_queue_block;

/******************************************************
 *               Macro Declarations
 ******************************************************/
// Macro to check the status and return it if it's not SL_STATUS_OK
#define SLI_VERIFY_STATUS(s) \
  do {                       \
    if (s != SL_STATUS_OK)   \
      return s;              \
  } while (0)

// WLAN Management Frame Sub-Type
#define SLI_WIFI_FRAME_SUBTYPE_MASK       0xf0 // WLAN Management Frame Sub-Type Mask
#define SLI_WIFI_FRAME_SUBTYPE_PROBE_RESP 0x50 // WLAN Management Frame Sub-Type Probe Response Frame
#define SLI_WIFI_FRAME_SUBTYPE_BEACON     0x80 // WLAN Management Frame Sub-Type Beacon Frame
#define SLI_WIFI_MINIMUM_FRAME_LENGTH     36   // Minimum Frame Length of WLAN Management Frame
#define SLI_WIFI_HARDWARE_ADDRESS_LENGTH  6    // Hardware Address Length

// WLAN Information Element Type
#define SLI_WLAN_TAG_SSID            0   // WLAN Information Element Type SSID
#define SLI_WLAN_TAG_RSN             48  // WLAN Robust Security Network Information Element
#define SLI_WLAN_TAG_VENDOR_SPECIFIC 221 // WLAN Vendor Specific Information Element

// Authentication key Management Type
#define SLI_AUTH_KEY_MGMT_UNSPEC_802_1X   0x000FAC01 // Unspecified Authentication key Management Type
#define SLI_AUTH_KEY_MGMT_PSK_OVER_802_1X 0x000FAC02 // PSK Authentication key Management Type
#define SLI_AUTH_KEY_MGMT_802_1X_SHA256   0x000FAC05 // SHA256 Authentication key Management Type
#define SLI_AUTH_KEY_MGMT_PSK_SHA256      0x000FAC06 // PSK SHA256 Authentication key Management Type
#define SLI_AUTH_KEY_MGMT_SAE             0x000FAC08 // SAE Authentication key Management Type
#define SLI_AUTH_KEY_MGMT_FT_SAE          0x000FAC09 // FT_SAE Authentication key Management Type

// Authentication key Management Type Flags
#define SLI_WLAN_AUTH_KEY_MGMT_TYPE_WPA           0x00000001 // WPA AKM Type
#define SLI_WLAN_AUTH_KEY_MGMT_TYPE_WPA2          0x00000002 // WPA2 AKM Type
#define SLI_WLAN_AUTH_KEY_MGMT_TYPE_WPA_PSK       0x00000004 // WPA_PSK AKM Type
#define SLI_WLAN_AUTH_KEY_MGMT_TYPE_WPA2_PSK      0x00000008 // WPA2_PSK AKM Type
#define SLI_WLAN_AUTH_KEY_MGMT_TYPE_SAE           0x00010000 // SAE AKM Type
#define SLI_WLAN_AUTH_KEY_MGMT_TYPE_FT_SAE        0x00100000 // FT_SAE AKM Type
#define SLI_WLAN_AUTH_KEY_MGMT_TYPE_802_1X_SHA256 0x00020000 // SHA256 AKM Type
#define SLI_WLAN_AUTH_KEY_MGMT_TYPE_PSK_SHA256    0x00040000 // PSK_SHA256 AKM Type

/// Task register ID to save firmware status
#define SLI_FW_STATUS_STORAGE_INVALID_INDEX 0xFF // Invalid index for firmware status storage
#define DEFAULT_BEACON_MISS_IGNORE_LIMIT    1

/******************************************************
 *               Local Type Declarations
 ******************************************************/
// WLAN Frame
typedef struct {
  uint8_t fc[2];                                   // Frame Control
  uint8_t duration[2];                             // Duration
  uint8_t da[SLI_WIFI_HARDWARE_ADDRESS_LENGTH];    // Destination Address
  uint8_t sa[SLI_WIFI_HARDWARE_ADDRESS_LENGTH];    // Source Address
  uint8_t bssid[SLI_WIFI_HARDWARE_ADDRESS_LENGTH]; // BSS Id
  uint8_t sc[2];                                   // Sequence Control Id
  uint8_t timestamp[8];                            // Time Stamp
  uint8_t bi[2];                                   // Beacon Interval
  uint8_t ci[2];                                   // Capability Information
  uint8_t tagged_info[];                           // Variable Information Elememt
} sli_wifi_data_frame_t;

// WLAN Information Element
typedef struct {
  uint8_t tag;         // Information Element Tag Id
  uint8_t data_length; // Information Element Data Length
  uint8_t data[];      // Information Element Data
} sli_wifi_data_tagged_info_t;

// Cipher suite
typedef struct {
  uint8_t cs_oui[3]; // Cipher Suite OUI
  uint8_t cs_type;   // Cipher Suite Type
} sli_wlan_cipher_suite_t;

// WLAN Robust Security Network Information Element
typedef struct {
  uint8_t version[2];          // RSN Version
  sli_wlan_cipher_suite_t gcs; // Group cipher suite
  uint8_t pcsc[2];             // Pairwise cipher suite count
  uint8_t pcsl[];              // Pairwise cipher suite list
} sli_wlan_rsn_element_t;

// WLAN Vendor Specific Information Element
typedef struct {
  uint8_t oui[3];              // Vendor OUI
  uint8_t vs_oui;              // Vendor specific OUI
  uint8_t type;                // WPA Information Element
  uint8_t wpa_version[2];      // WPA Version
  sli_wlan_cipher_suite_t mcs; // Multicast Cipher Suite
  uint8_t ucsc;                // Unicast Cipher Suite List Count
  uint8_t ucsl[];              // Unicast Cipher Suite List
} sli_wlan_vendor_specific_element_t;

/******************************************************
 *               Variable Declarations
 ******************************************************/
osThreadId_t si91x_thread       = 0;
osThreadId_t si91x_event_thread = 0;
extern osEventFlagsId_t sli_wifi_events;
osEventFlagsId_t si91x_async_events = 0;
osMutexId_t malloc_free_mutex       = 0;

#ifdef SL_SI91X_SIDE_BAND_CRYPTO
osMutexId_t side_band_crypto_mutex = 0;
#endif

sli_wifi_command_queue_t cmd_queues[SI91X_CMD_MAX] = { 0 };
sli_wifi_buffer_queue_t sli_tx_data_queue;
#ifndef __ZEPHYR__
// For all the threads this is the index of the thread local array at which the firmware status will be stored.
sli_task_register_id_t sli_fw_status_storage_index = SLI_FW_STATUS_STORAGE_INVALID_INDEX;
#endif

static bool sli_si91x_packet_status      = 0;
uint8_t firmware_queue_id[SI91X_CMD_MAX] = { [SLI_WIFI_COMMON_CMD]   = SLI_WLAN_MGMT_Q,
                                             [SLI_WIFI_WLAN_CMD]     = SLI_WLAN_MGMT_Q,
                                             [SLI_SI91X_NETWORK_CMD] = SLI_WLAN_MGMT_Q,
                                             [SLI_SI91X_SOCKET_CMD]  = SLI_WLAN_MGMT_Q,
                                             [SLI_SI91X_BT_CMD]      = SLI_BT_Q };

// clang-format off
uint8_t command_packet_type[SLI_WLAN_CMD_MAX]   = { [SLI_WLAN_COMMON_CMD]   = SLI_WIFI_COMMAND_ENGINE_COMMON_COMMAND_PACKET,
                                                           [SLI_WIFI_WLAN_CMD]     = SLI_WIFI_COMMAND_ENGINE_WIFI_COMMAND_PACKET,
                                                           [SLI_SI91X_NETWORK_CMD] = SLI_WIFI_COMMAND_ENGINE_NETWORK_COMMAND_PACKET,
                                                           [SLI_SI91X_BT_CMD]      = SLI_WIFI_COMMAND_ENGINE_BLE_COMMAND_PACKET,
                                                           [SLI_SI91X_SOCKET_CMD]  = SLI_WIFI_COMMAND_ENGINE_SOCKET_COMMAND_PACKET,
                                                         };

extern bool device_initialized;

// Declaration of external functions
extern void sli_si91x_async_rx_event_handler_thread(void *args);
extern sl_status_t sli_create_generic_rx_packet_from_params(sli_si91x_queue_packet_t **queue_packet,
                                                            sl_wifi_buffer_t **packet_buffer,
                                                            uint16_t packet_id,
                                                            uint8_t flags,
                                                            void *sdk_context,
                                                            uint16_t frame_status);
void sl_debug_log(const char *format, ...);

extern sli_wifi_performance_profile_t performance_profile;

// NOTE: Boolean value determines whether firmware automatically closes the TCP socket in case of receiving termination from remote node or not.
static bool tcp_auto_close_enabled;

sl_wifi_system_performance_profile_t current_performance_profile = HIGH_PERFORMANCE;

static sl_wifi_system_boot_configuration_t saved_boot_configuration = { 0 };

// SI91X-specific helper function implementations

void sli_configure_si91x_command_packet_node(sli_si91x_queue_packet_t *node,
                                             sl_wifi_buffer_t *buffer,
                                             sli_wifi_command_type_t command_type,
                                             uint8_t flags,
                                             void *sdk_context,
                                             sli_wifi_wait_period_t wait_period)
{
  // Set various properties of the node representing the command packet
  node->host_packet       = buffer;
  node->firmware_queue_id = firmware_queue_id[command_type];
  node->command_type      = command_type;
  node->flags             = flags;
  node->sdk_context       = sdk_context;
  node->event_mask        = SL_SI91X_RESPONSE_FLAG(command_type);

  if (flags != SLI_WIFI_PACKET_WITH_ASYNC_RESPONSE) {
    node->command_tickcount = osKernelGetTickCount();
    // Calculate the wait time based on wait_period
    if ((wait_period & SLI_WIFI_WAIT_FOR_EVER) == SLI_WIFI_WAIT_FOR_EVER) {
      node->command_timeout = osWaitForever;
    } else {
      node->command_timeout = (wait_period & ~SLI_WIFI_WAIT_FOR_RESPONSE_BIT);
    }
  }
}

sl_status_t sli_enqueue_si91x_command_packet(sli_wifi_command_type_t command_type,
                                             sl_wifi_buffer_t *queue_packet,
                                             sl_wifi_buffer_t *buffer,
                                             uint8_t packet_id)
{
  UNUSED_PARAMETER(command_type);
  UNUSED_PARAMETER(queue_packet);
  UNUSED_PARAMETER(buffer);
  UNUSED_PARAMETER(packet_id);

  return SL_STATUS_OK;
}

sl_status_t sli_fw_status_storage_index_init(void)
{
  sl_status_t status = SL_STATUS_OK;

  // Declare a variable to store the current interrupt state
  CORE_DECLARE_IRQ_STATE;

  // Enter a critical section by disabling interrupts
  // This ensures that the following operations are executed atomically
  CORE_ENTER_CRITICAL();
#ifdef SL_CATALOG_KERNEL_PRESENT
  // Check if the code is running in a thread context & task register index is invalid
  if (osThreadGetId() != NULL && sli_fw_status_storage_index == SLI_FW_STATUS_STORAGE_INVALID_INDEX) {
    // Create a new task register id
    status = sli_osTaskRegisterNew(&sli_fw_status_storage_index);
    VERIFY_STATUS_AND_RETURN(status);
  }
#endif
  CORE_EXIT_CRITICAL();
  return status;
}

// Function to update a existing entry or create new entry for scan results database
static sli_scan_info_t *sli_update_or_create_scan_info_element(const sli_scan_info_t *info)
{
  sli_scan_info_t **scan_db_head = sli_get_scan_info_database();
  sli_scan_info_t *element       = NULL;

  element = *scan_db_head;
  while (NULL != element) {
    if (0 == memcmp(info->bssid, element->bssid, SLI_WIFI_HARDWARE_ADDRESS_LENGTH)) {
      element->channel       = info->channel;
      element->security_mode = info->security_mode;
      element->rssi          = info->rssi;
      element->network_type  = info->network_type;
      memcpy(element->ssid, info->ssid, 34);
      break;
    }
    element = element->next;
  }

  if (NULL == element) {
    element = (sli_scan_info_t *)malloc(sizeof(sli_scan_info_t));
    if (element == NULL) {
      return NULL;
    }
    memcpy(element, info, sizeof(sli_scan_info_t));
    element->next = NULL;
    return element;
  }

  return NULL;
}

// Function to store a given scan info element in scan results database
static void sli_store_scan_info_element(const sli_scan_info_t *info)
{
  sli_scan_info_t *element       = NULL;
  sli_scan_info_t *head          = NULL;
  sli_scan_info_t *tail          = NULL;
  sli_scan_info_t **scan_db_head = sli_get_scan_info_database();

  if (NULL == info) {
    return;
  }

  element = sli_update_or_create_scan_info_element(info);
  if (NULL == element) {
    return;
  }

  if (NULL == *scan_db_head) {
    *scan_db_head = element;
    return;
  }

  tail = *scan_db_head;
  while (NULL != tail) {
    if (element->rssi < tail->rssi) {
      element->next = tail;
      if (NULL == head) {
        *scan_db_head = element;
      } else {
        head->next = element;
      }
      break;
    }

    head = tail;
    tail = tail->next;

    if (NULL == tail) {
      head->next = element;
    }
  }

  return;
}

// Function to identify Authentication Key Management Type
static uint32_t sli_get_key_management_info(const sli_wlan_cipher_suite_t *akms, uint16_t akmsc)
{
  uint32_t key_mgmt = 0;
  uint32_t oui_type;

  if (NULL == akms) {
    return 0;
  }

  for (int i = 0; i < akmsc; i++) {
    oui_type = ((akms[i].cs_oui[0] << 24) | (akms[i].cs_oui[1] << 16) | (akms[i].cs_oui[2] << 8) | akms[i].cs_type);

    switch (oui_type) {
      case SLI_AUTH_KEY_MGMT_UNSPEC_802_1X:
        key_mgmt |= SLI_WLAN_AUTH_KEY_MGMT_TYPE_WPA | SLI_WLAN_AUTH_KEY_MGMT_TYPE_WPA2;
        break;
      case SLI_AUTH_KEY_MGMT_PSK_OVER_802_1X:
        key_mgmt |= SLI_WLAN_AUTH_KEY_MGMT_TYPE_WPA_PSK | SLI_WLAN_AUTH_KEY_MGMT_TYPE_WPA2_PSK;
        break;
      case SLI_AUTH_KEY_MGMT_802_1X_SHA256:
        key_mgmt |= SLI_WLAN_AUTH_KEY_MGMT_TYPE_802_1X_SHA256;
        break;
      case SLI_AUTH_KEY_MGMT_PSK_SHA256:
        key_mgmt |= SLI_WLAN_AUTH_KEY_MGMT_TYPE_PSK_SHA256;
        break;
      case SLI_AUTH_KEY_MGMT_SAE:
        key_mgmt |= SLI_WLAN_AUTH_KEY_MGMT_TYPE_SAE;
        break;
      case SLI_AUTH_KEY_MGMT_FT_SAE:
        key_mgmt |= SLI_WLAN_AUTH_KEY_MGMT_TYPE_FT_SAE;
        break;
      default:
        break;
    }
  }
  return key_mgmt;
}

// Helper function to process RSN element
static void sli_process_rsn_element(const sli_wifi_data_tagged_info_t *info, sli_scan_info_t *scan_info)
{
  scan_info->security_mode            = SL_WIFI_WPA2_ENTERPRISE;
  const sli_wlan_rsn_element_t *rsn   = (const sli_wlan_rsn_element_t *)info->data;
  uint16_t pcsc                       = (uint16_t)(rsn->pcsc[0] | (rsn->pcsc[1] << 8));
  const uint8_t *akmslc               = (rsn->pcsl + (pcsc * sizeof(sli_wlan_cipher_suite_t)));
  uint16_t akmsc                      = (uint16_t)(akmslc[0] | (akmslc[1] << 8));
  const sli_wlan_cipher_suite_t *akms = (sli_wlan_cipher_suite_t *)(akmslc + 2);
  uint8_t wlan_gcs_oui[3]             = { 0x00, 0x0F, 0xAC };

  SL_DEBUG_LOG("RSN OUI %02x:%02x:%02x.\n", rsn->gcs.cs_oui[0], rsn->gcs.cs_oui[1], rsn->gcs.cs_oui[2]);
  SL_DEBUG_LOG("Pairwise cipher suite count: %u.\n", pcsc);

  if (!memcmp(rsn->gcs.cs_oui, wlan_gcs_oui, 3)) {
    scan_info->security_mode = SL_WIFI_WPA2;
    uint32_t key             = sli_get_key_management_info(akms, akmsc);

    if (akms[0].cs_type == 1) {
      scan_info->security_mode = SL_WIFI_WPA2_ENTERPRISE;
    }

    if (key & SLI_WLAN_AUTH_KEY_MGMT_TYPE_802_1X_SHA256) {
      scan_info->security_mode = SL_WIFI_WPA3_ENTERPRISE;
      if ((key & SLI_WLAN_AUTH_KEY_MGMT_TYPE_WPA) || (key & SLI_WLAN_AUTH_KEY_MGMT_TYPE_WPA2)) {
        scan_info->security_mode = SL_WIFI_WPA3_TRANSITION_ENTERPRISE;
      }
    }

    if (key & SLI_WLAN_AUTH_KEY_MGMT_TYPE_SAE) {
      scan_info->security_mode = SL_WIFI_WPA3;
      if ((key & SLI_WLAN_AUTH_KEY_MGMT_TYPE_PSK_SHA256) || (key & SLI_WLAN_AUTH_KEY_MGMT_TYPE_WPA2_PSK)) {
        scan_info->security_mode = SL_WIFI_WPA3_TRANSITION;
      }
    }
  }
}

// Helper function to process Vendor Specific element
static void sli_process_vendor_specific_element(const sli_wifi_data_tagged_info_t *info, sli_scan_info_t *scan_info)
{
  const sli_wlan_vendor_specific_element_t *vendor = (const sli_wlan_vendor_specific_element_t *)info->data;
  uint8_t wlan_oui[3]                              = { 0x00, 0x50, 0xF2 };

  if ((!memcmp(vendor->oui, wlan_oui, 3)) && (vendor->vs_oui == 0x01)
      && ((scan_info->security_mode == SL_WIFI_OPEN) || (scan_info->security_mode == SL_WIFI_WEP))) {
    scan_info->security_mode            = SL_WIFI_WPA;
    const uint8_t *list_count           = (vendor->ucsl + (sizeof(sli_wlan_cipher_suite_t) * vendor->ucsc));
    uint16_t akmsc                      = (uint16_t)(list_count[0] | (list_count[1] << 8));
    const sli_wlan_cipher_suite_t *akms = (sli_wlan_cipher_suite_t *)(list_count + 2);

    if ((0 != akmsc) && (akms[akmsc - 1].cs_type == 1)) {
      scan_info->security_mode = SL_WIFI_WPA_ENTERPRISE;
    }
  }
}

// Function to parse Information elements in WiFi Beacon or Probe response frames
static void sli_process_tag_info(const sli_wifi_data_tagged_info_t *info, sli_scan_info_t *scan_info)
{
  switch (info->tag) {
    case SLI_WLAN_TAG_SSID:
      memcpy(scan_info->ssid, info->data, info->data_length);
      scan_info->ssid[info->data_length] = 0;
      break;

    case SLI_WLAN_TAG_RSN:
      sli_process_rsn_element(info, scan_info);
      break;

    case SLI_WLAN_TAG_VENDOR_SPECIFIC:
      sli_process_vendor_specific_element(info, scan_info);
      break;

    default:
      break;
  }

  return;
}

/******************************************************
 *            Internal Function Declarations
 ******************************************************/
// Function to Parse the Beacon and Probe response Frames
void sli_handle_wifi_beacon(sl_wifi_system_packet_t *packet)
{
  uint8_t subtype                   = 0;
  sli_wifi_data_frame_t *wifi_frame = (sli_wifi_data_frame_t *)packet->data;
  sli_scan_info_t scan_info         = { 0 };
  uint16_t ies_length               = 0;

  scan_info.rssi    = (~packet->desc[10]);
  scan_info.channel = packet->desc[11];

  // Check for ESS bit and TBSS status bit in capability info
  // 1 in ESS bit indicates that the transmitter is an AP
  if (1 == (wifi_frame->ci[0] & 0x03)) {
    scan_info.network_type = 1;
  } else {
    scan_info.network_type = 0;
  }

  if (wifi_frame->ci[0] & 0x08) {
    scan_info.security_mode = SL_WIFI_WEP;
  } else {
    scan_info.security_mode = SL_WIFI_OPEN;
  }

  subtype = wifi_frame->fc[0] & SLI_WIFI_FRAME_SUBTYPE_MASK;
  switch (subtype) {
    case SLI_WIFI_FRAME_SUBTYPE_PROBE_RESP:
    case SLI_WIFI_FRAME_SUBTYPE_BEACON: {
      if (packet->length <= SLI_WIFI_MINIMUM_FRAME_LENGTH) {
        return;
      }
      ies_length = packet->length - SLI_WIFI_MINIMUM_FRAME_LENGTH;

      memcpy(scan_info.bssid, wifi_frame->bssid, SLI_WIFI_HARDWARE_ADDRESS_LENGTH);

      sli_wifi_data_tagged_info_t *info = (sli_wifi_data_tagged_info_t *)wifi_frame->tagged_info;
      while (0 != ies_length) {
        sli_process_tag_info(info, &scan_info);
        ies_length -= (sizeof(sli_wifi_data_tagged_info_t) + info->data_length);
        info = (sli_wifi_data_tagged_info_t *)&(info->data[info->data_length]);

        if (ies_length <= sizeof(sli_wifi_data_tagged_info_t)) {
          ies_length = 0;
        }
      }

      sli_store_scan_info_element(&scan_info);
    } break;
    default:
      return;
  }

  return;
}

/******************************************************
 *               Function Declarations
 ******************************************************/

void sli_reset_coex_current_performance_profile(void)
{
  if (!power_save_sequence_in_progress) {
    memset(&performance_profile, 0, sizeof(sli_wifi_performance_profile_t));
  }
}

void sli_save_boot_configuration(const sl_wifi_system_boot_configuration_t *boot_configuration)
{
  memcpy(&saved_boot_configuration, boot_configuration, sizeof(sl_wifi_system_boot_configuration_t));
}

void sli_get_saved_boot_configuration(sl_wifi_system_boot_configuration_t *boot_configuration)
{
  memcpy(boot_configuration, &saved_boot_configuration, sizeof(sl_wifi_system_boot_configuration_t));
}

void sli_get_bt_current_performance_profile(sl_bt_performance_profile_t *profile)
{
  SL_ASSERT(profile != NULL);
  memcpy(profile, &performance_profile.bt_performance_profile, sizeof(sl_bt_performance_profile_t));
}

void sli_save_tcp_auto_close_choice(bool is_tcp_auto_close_enabled)
{
  tcp_auto_close_enabled = is_tcp_auto_close_enabled;
}

bool sli_is_tcp_auto_close_enabled()
{
  return tcp_auto_close_enabled;
}

sl_status_t sli_convert_si91x_wifi_client_info(sl_wifi_client_info_response_t *client_info_response,
                                               const sli_wifi_client_info_response *sli_wifi_client_info_response)
{

  SL_WIFI_ARGS_CHECK_NULL_POINTER(sli_wifi_client_info_response);
  SL_WIFI_ARGS_CHECK_NULL_POINTER(client_info_response);

  client_info_response->client_count =
    (uint8_t)(sli_wifi_client_info_response->sta_count[0] | sli_wifi_client_info_response->sta_count[1] << 8);

  for (uint8_t station_index = 0; station_index < client_info_response->client_count; station_index++) {
    const uint8_t *si91x_ip_address;
    uint8_t *sl_ip_address;

    sl_wifi_client_info_t *sl_client_info            = &client_info_response->client_info[station_index];
    const sli_wifi_station_info_t *si91x_client_info = &sli_wifi_client_info_response->sta_info[station_index];

    uint8_t ip_address_size = (uint8_t)(si91x_client_info->ip_version[0] | si91x_client_info->ip_version[1] << 8);

    si91x_ip_address = ip_address_size == SL_IPV4_ADDRESS_LENGTH ? si91x_client_info->ip_address.ipv4_address
                                                                 : si91x_client_info->ip_address.ipv6_address;
    sl_ip_address    = ip_address_size == SL_IPV4_ADDRESS_LENGTH ? sl_client_info->ip_address.ip.v4.bytes
                                                                 : sl_client_info->ip_address.ip.v6.bytes;

    sl_client_info->ip_address.type = ip_address_size == SL_IPV4_ADDRESS_LENGTH ? SL_IPV4 : SL_IPV6;

    memcpy(&sl_client_info->mac_adddress, si91x_client_info->mac, sizeof(sl_mac_address_t));
    memcpy(sl_ip_address, si91x_ip_address, ip_address_size);
  }

  return SL_STATUS_OK;
}

sl_wifi_event_t sli_convert_si91x_event_to_sl_wifi_event(uint32_t command, uint16_t frame_status)
{
  // Define a constant indicating a fail indication event
  const sl_wifi_event_t fail_indication = (frame_status != RSI_SUCCESS) ? SL_WIFI_EVENT_FAIL_INDICATION : 0;

  // Switch-case to map SI91x events to SL Wi-Fi events
  switch (command) {
    case SLI_WIFI_RSP_BG_SCAN:
    case SLI_WIFI_RSP_SCAN:
    case SLI_WLAN_RSP_SCAN_RESULTS:
      return SL_WIFI_SCAN_RESULT_EVENT | fail_indication;
    case SLI_WIFI_RSP_JOIN:
      return SL_WIFI_JOIN_EVENT | fail_indication;
    case SLI_WIFI_RSP_GET_STATS:
      if (frame_status != RSI_SUCCESS) {
        return SL_WIFI_STATS_RESPONSE_EVENTS | fail_indication;
      }
      return SL_WIFI_STATS_EVENT;
    case SLI_WIFI_RSP_RX_STATS:
      if (frame_status != RSI_SUCCESS) {
        return SL_WIFI_STATS_RESPONSE_EVENTS | fail_indication;
      }
      return SL_WIFI_STATS_ASYNC_EVENT;
    case SLI_WLAN_RATE_RSP_STATS:
      if (frame_status != RSI_SUCCESS) {
        return SL_WIFI_STATS_RESPONSE_EVENTS | fail_indication;
      }
      return SL_WIFI_STATS_TEST_MODE_EVENT;
    case SLI_WIFI_RSP_EXT_STATS:
      if (frame_status != RSI_SUCCESS) {
        return SL_WIFI_STATS_RESPONSE_EVENTS | fail_indication;
      }
      return SL_WIFI_STATS_ADVANCE_EVENT;
    case SLI_WLAN_RSP_MODULE_STATE:
      if (frame_status != RSI_SUCCESS) {
        return SL_WIFI_STATS_RESPONSE_EVENTS | fail_indication;
      }
      return SL_WIFI_STATS_MODULE_STATE_EVENT;
    case SLI_WLAN_RSP_HTTP_OTAF:
      return SL_WIFI_HTTP_OTA_FW_UPDATE_EVENT | fail_indication;
    case SLI_WLAN_RSP_CLIENT_CONNECTED:
      return SL_WIFI_CLIENT_CONNECTED_EVENT | fail_indication;
    case SLI_WLAN_RSP_CLIENT_DISCONNECTED:
      return SL_WIFI_CLIENT_DISCONNECTED_EVENT | fail_indication;
    case SLI_WLAN_RSP_TWT_ASYNC:
      if (frame_status == RSI_SUCCESS) {
        return SL_WIFI_TWT_RESPONSE_EVENT;
      } else {
        return SL_WIFI_TWT_RESPONSE_EVENT | (frame_status << 16);
      }
    case SLI_WLAN_RSP_TRANSCEIVER_TX_DATA_STATUS:
      return SL_WIFI_TRANSCEIVER_TX_DATA_STATUS_CB | fail_indication;
    case SLI_SI91X_WIFI_RX_DOT11_DATA:
      return SL_WIFI_TRANSCEIVER_RX_DATA_RECEIVE_CB | fail_indication;
    default:
      return SL_WIFI_INVALID_EVENT;
  }
}

sl_status_t sl_si91x_platform_init(void)
{
  sl_status_t status = SL_STATUS_OK;

  // Initialize the command queues
  memset(cmd_queues, 0, sizeof(cmd_queues));

  // Create event flags
  if (NULL == sli_wifi_events) {
    sli_wifi_events = osEventFlagsNew(NULL);
  }

  status = sli_command_engine_status_queue_init(); // Initialize resources for the command engine status queue
  VERIFY_STATUS_AND_RETURN(status);

  // Initialize command queues and associated mutexes
  for (int i = 0; i < SI91X_CMD_MAX; i++) {
    status = sli_queue_manager_init(&cmd_queues[i].rx_queue, SLI_BUFFER_MANAGER_QUEUE_NODE_POOL);
    VERIFY_STATUS_AND_RETURN(status);
    cmd_queues[i].event_flags = osEventFlagsNew(NULL);
    cmd_queues[i].flag        = (1 << i);
  }

  // Create and start HAL thread
  status = sli_hal_si91x_init();
  VERIFY_STATUS_AND_RETURN(status);

  // Create and start Command Engine thread
  status = sli_wifi_command_engine_init();
  VERIFY_STATUS_AND_RETURN(status);

  // Create malloc/free mutex
  if (malloc_free_mutex == NULL) {
    malloc_free_mutex = osMutexNew(NULL);
  }

#ifdef SL_SI91X_SIDE_BAND_CRYPTO
  // Create side_band_crypto_mutex mutex
  side_band_crypto_mutex = osMutexNew(NULL);
#endif

  return status;
}

sl_status_t sli_si91x_platform_deinit(void)
{
  sl_status_t status = SL_STATUS_OK;
  // Deallocate all threads, mutexes and event handlers

  status =
    sli_command_engine_status_queue_deinit(); // Deinitialize resources associated with the command engine status queue.
  VERIFY_STATUS_AND_RETURN(status);

  // Terminate Command Engine thread
  status = sli_wifi_command_engine_deinit();
  VERIFY_STATUS_AND_RETURN(status);

  // Terminate HAL thread and it's resources
  status = sli_hal_si91x_deinit();
  VERIFY_STATUS_AND_RETURN(status);

  // Terminate SI91X event handler thread
  if (NULL != si91x_event_thread) {
    osThreadTerminate(si91x_event_thread);
    si91x_event_thread = NULL;
  }

  // Delete event flags
  if (NULL != sli_wifi_events) {
    osEventFlagsDelete(sli_wifi_events);
    sli_wifi_events = NULL;
  }

  if (NULL != si91x_async_events) {
    osEventFlagsDelete(si91x_async_events);
    si91x_async_events = NULL;
  }

  // Delete malloc/free mutex
  osMutexDelete(malloc_free_mutex);
  malloc_free_mutex = NULL;
  return SL_STATUS_OK;
}

sl_si91x_host_timestamp_t sl_si91x_host_get_timestamp(void)
{
  return osKernelGetTickCount();
}

// Calculate elapsed time from the given starting timestamp
sl_si91x_host_timestamp_t sl_si91x_host_elapsed_time(uint32_t starting_timestamp)
{
  return (osKernelGetTickCount() - starting_timestamp);
}

// Delay execution for a specified number of milliseconds using an OS-level delay
void sl_si91x_host_delay_ms(uint32_t delay_milliseconds)
{
  if (delay_milliseconds == osWaitForever) {
    osDelay(osWaitForever);
  } else {
    osDelay(SLI_SYSTEM_MS_TO_TICKS(delay_milliseconds));
  }
}

uint32_t sli_si91x_host_queue_status(const sli_wifi_buffer_queue_t *queue)
{
  return queue->head != NULL;
}

uint32_t sli_si91x_wait_for_event(uint32_t event_mask, uint32_t timeout)
{
  if (timeout != osWaitForever) {
    timeout = SLI_SYSTEM_MS_TO_TICKS(timeout);
  }

  uint32_t result = osEventFlagsWait(sli_wifi_events, event_mask, osFlagsWaitAny, timeout);

  if (result == (uint32_t)osErrorTimeout || result == (uint32_t)osErrorResource) {
    return 0;
  }
  return result;
}

uint32_t sli_si91x_clear_event(uint32_t event_mask)
{
  uint32_t result = osEventFlagsClear(sli_wifi_events, event_mask);
  if (result == (uint32_t)osErrorResource) {
    return 0;
  }
  return result;
}

sl_status_t sli_si91x_send_power_save_request(const sl_wifi_performance_profile_v2_t *wifi_profile,
                                              const sl_bt_performance_profile_t *bt_profile)
{
  sl_status_t status;
  sli_wifi_power_save_request_t power_save_request                = { 0 };
  sl_wifi_system_performance_profile_t selected_coex_profile_mode = { 0 };

  power_save_sequence_in_progress = true;

  // Disable power save mode by setting it to HIGH_PERFORMANCE profile
  status = sli_wifi_send_command(SLI_WIFI_REQ_PWRMODE,
                                         SLI_WIFI_COMMON_CMD,
                                         &power_save_request,
                                         sizeof(sli_wifi_power_save_request_t),
                                         SLI_WIFI_RSP_PWRMODE_WAIT_TIME,
                                         NULL,
                                         NULL);
  if (status != SL_STATUS_OK) {
    // Reset flag on failure
    power_save_sequence_in_progress = false;
    return status;
  }

  if (NULL != wifi_profile) {
    // Save the new Wi-Fi profile
    sli_wifi_save_current_performance_profile(wifi_profile);
  }

  if (NULL != bt_profile) {
    // Save the new BT/BLE profile
    sli_save_bt_current_performance_profile(bt_profile);
  }

  // get the updated coex profile
  sli_get_coex_performance_profile(&selected_coex_profile_mode);

  // If the requested performance profile is HIGH_PERFORMANCE, no need to send the request to firmware
  if (selected_coex_profile_mode == HIGH_PERFORMANCE) {
    // Reset flag on failure
    power_save_sequence_in_progress = false;
    return SL_STATUS_OK;
  }

  // Convert the performance profile to a power save request.
  sli_convert_performance_profile_to_power_save_command(selected_coex_profile_mode, &power_save_request);

  status = sli_wifi_send_command(SLI_WIFI_REQ_PWRMODE,
                                         SLI_WIFI_COMMON_CMD,
                                         &power_save_request,
                                         sizeof(sli_wifi_power_save_request_t),
                                         SLI_WIFI_WAIT_FOR_RESPONSE(SLI_WIFI_RSP_PWRMODE_WAIT_TIME),
                                         NULL,
                                         NULL);
  // Reset flag on failure
  power_save_sequence_in_progress = false;
  return status;
}

sl_status_t sl_si91x_host_power_cycle(void)
{
  sl_si91x_host_hold_in_reset();
  sl_si91x_host_delay_ms(100);

  sl_si91x_host_release_from_reset();
  sl_si91x_host_delay_ms(100);

  return SL_STATUS_OK;
}

void print_80211_packet(const uint8_t *packet, uint32_t packet_length, uint16_t max_payload_length)
{
  uint32_t dump_bytes    = 0;
  uint32_t header_length = MAC80211_HDR_MIN_LEN;

  header_length += (packet[0] & BIT(7)) ? 2 : 0;                           /* 2 bytes QoS control */
  header_length += ((packet[1] & BIT(0)) && (packet[1] & BIT(1))) ? 6 : 0; /* 6 byte Addr4 */

  sl_debug_log("%02x %02x | ", packet[0], packet[1]); /* FC */
  sl_debug_log("%02x %02x | ", packet[2], packet[3]); /* Dur */
  sl_debug_log("%02x:%02x:%02x:%02x:%02x:%02x | ",
               packet[4],
               packet[5],
               packet[6],
               packet[7],
               packet[8],
               packet[9]); /* Addr1/RA */
  sl_debug_log("%02x:%02x:%02x:%02x:%02x:%02x | ",
               packet[10],
               packet[11],
               packet[12],
               packet[13],
               packet[14],
               packet[15]); /* Addr2/NWP */
  sl_debug_log("%02x:%02x:%02x:%02x:%02x:%02x | ",
               packet[16],
               packet[17],
               packet[18],
               packet[19],
               packet[20],
               packet[21]);                             /* Addr3/DA */
  sl_debug_log("%02x %02x | ", packet[22], packet[23]); /* Seq control */
  if ((packet[1] & BIT(0)) && (packet[1] & BIT(1))) {   /* Addr4 */
    sl_debug_log("%02x:%02x:%02x:%02x:%02x:%02x | ",
                 packet[24],
                 packet[25],
                 packet[26],
                 packet[27],
                 packet[28],
                 packet[29]);
  }
  if (packet[0] & BIT(7)) {
    sl_debug_log("%02x %02x | ", packet[30], packet[31]); /* QoS control */
  }

  // Determine number of payload bytes to print
  dump_bytes = packet_length - header_length;
  dump_bytes = max_payload_length > dump_bytes ? dump_bytes : max_payload_length;

  for (uint32_t i = header_length; i < header_length + dump_bytes; i++) {
    sl_debug_log("%02x ", packet[i]);
  }

  sl_debug_log("|\r\n");
}

sli_wifi_command_queue_t *sli_si91x_get_command_queue(sli_wifi_command_type_t type)
{
  switch (type) {
    case SLI_WIFI_WLAN_CMD:
      return &cmd_queues[SLI_WIFI_WLAN_CMD];
    case SLI_SI91X_NETWORK_CMD:
      return &cmd_queues[SLI_SI91X_NETWORK_CMD];
    case SLI_SI91X_BT_CMD:
      return &cmd_queues[SLI_SI91X_BT_CMD];
    case SLI_SI91X_SOCKET_CMD:
      return &cmd_queues[SLI_SI91X_SOCKET_CMD];

    case SLI_WIFI_COMMON_CMD:
      __attribute__((fallthrough));
    default:
      return &cmd_queues[SLI_WIFI_COMMON_CMD];
  }
}

/* Function to get the current status of the NVM command progress
Returns true if an NVM command is in progress, false otherwise*/
bool sli_si91x_get_flash_command_status()
{
  return sli_si91x_packet_status;
}

void sli_si91x_update_flash_command_status(bool flag)
{
  sli_si91x_packet_status = flag;
}

bool sli_si91x_get_tx_command_status()
{
  return sli_si91x_tx_command_status;
}

void sli_si91x_update_tx_command_status(bool flag)
{
  sli_si91x_tx_command_status = flag;
}

/*  This function is used to update the power manager to see whether the device is ready for sleep or not.
 True indicates ready for sleep, and false indicates not ready for sleep.*/
bool sli_si91x_is_sdk_ok_to_sleep()
{
  return ((!sli_si91x_get_flash_command_status()) && (sl_si91x_is_device_initialized())
          && (!sli_si91x_get_tx_command_status()));
}

bool sl_si91x_is_device_initialized(void)
{
  return device_initialized;
}

sl_status_t sli_wifi_convert_and_save_firmware(uint16_t firmware_status)
{
 return sli_convert_and_save_firmware_status(firmware_status);
}

#ifdef SLI_SI91X_OFFLOAD_NETWORK_STACK
// Implementation of SNI extension setting for embedded sockets
sl_status_t sli_si91x_set_sni_for_embedded_socket(const sli_si91x_tls_extension_info_t *sni_extension,
                                                  sli_si91x_sni_target_protocol_t sni_target_protocol)
{
  sl_status_t status     = SL_STATUS_OK;
  uint32_t packet_length = 0;

  if (sni_extension == NULL) {
    return SL_STATUS_NULL_POINTER;
  }

  // Validate SNI extension type
  if (sni_extension->type != SL_SI91X_TLS_EXTENSION_SNI_TYPE) {
    return SL_STATUS_INVALID_TYPE;
  }

  // Validate length before any memory operations
  if (sni_extension->length == 0
      || sizeof(sli_si91x_tls_extension_info_t) + sni_extension->length > SLI_SI91X_MAX_SIZE_OF_EXTENSION_DATA) {
    return SL_STATUS_WOULD_OVERFLOW;
  }

  sli_si91x_sni_for_embedded_socket_request_t *request = (sli_si91x_sni_for_embedded_socket_request_t *)malloc(
    sizeof(sli_si91x_sni_for_embedded_socket_request_t) + SLI_SI91X_MAX_SIZE_OF_EXTENSION_DATA);
  SLI_VERIFY_MALLOC_AND_RETURN(request);

  memset(request, 0, sizeof(sli_si91x_sni_for_embedded_socket_request_t) + SLI_SI91X_MAX_SIZE_OF_EXTENSION_DATA);

  request->protocol = (uint16_t)sni_target_protocol;

  request->offset = sizeof(sli_si91x_tls_extension_info_t);
  memcpy(&request->tls_extension_data, sni_extension, SLI_SI91X_MAX_SIZE_OF_EXTENSION_DATA);
  request->offset += sni_extension->length;
  packet_length = sizeof(sli_si91x_sni_for_embedded_socket_request_t) + SLI_SI91X_MAX_SIZE_OF_EXTENSION_DATA;

  status = sli_wifi_send_command(SLI_WLAN_REQ_SET_SNI_EMBEDDED,
                                         SLI_SI91X_NETWORK_CMD,
                                         request,
                                         packet_length,
                                         SLI_WIFI_WAIT_FOR_RESPONSE(SLI_WLAN_RSP_SET_SNI_EMBEDDED_WAIT_TIME),
                                         NULL,
                                         NULL);
  free(request);

  return status;
}

// Helper function to configure SNI using either extension or hostname
sl_status_t sli_configure_sni(const sli_si91x_tls_extension_info_t *sni_extension,
                              const uint8_t *host_name,
                              sli_si91x_sni_target_protocol_t sni_target_protocol)
{
  if (sni_extension != NULL) {
    return sli_si91x_set_sni_for_embedded_socket(sni_extension, sni_target_protocol);
  }

  if (host_name != NULL && host_name[0] != '\0') {
    size_t host_name_length = sl_strlen((const char *)host_name);
    sl_status_t status      = SL_STATUS_OK;

    // Validate length before allocation
    if (host_name_length > SLI_SI91X_MAX_SIZE_OF_EXTENSION_DATA) {
      return SL_STATUS_SI91X_MEMORY_ERROR;
    }

    sli_si91x_tls_extension_info_t *tls_sni =
      (sli_si91x_tls_extension_info_t *)malloc(sizeof(sli_si91x_tls_extension_info_t) + host_name_length);
    if (tls_sni == NULL) {
      return SL_STATUS_ALLOCATION_FAILED;
    }

    tls_sni->type   = SL_SI91X_TLS_EXTENSION_SNI_TYPE;
    tls_sni->length = (uint16_t)host_name_length;
    memcpy(tls_sni->value, host_name, tls_sni->length);

    status = sli_si91x_set_sni_for_embedded_socket(tls_sni, sni_target_protocol);
    free(tls_sni);
    return status;
  }

  return SL_STATUS_OK;
}
#endif