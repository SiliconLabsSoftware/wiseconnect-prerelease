/*******************************************************************************
* @file  gatt_server.h
* @brief GATT Server Component - Public API
*******************************************************************************
* # License
* <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef GATT_SERVER_H
#define GATT_SERVER_H

#include <stdint.h>
#include "rsi_ble.h" // uuid_t
#include "gap.h"     // For rsi_ble_conn_info_t, rsi_ble_conn_config_t definitions

/**
 * @brief Controls who prepares `rsi_ble_conn_info[].read_data1` for server TX paths.
 *
 * Default behavior (DEFAULT) matches the legacy demo: `rsi_ble_gatt_server_data_transmit()`
 * overwrites the first bytes of `read_data1` with a running counter before notify/indicate.
 *
 * Profile subcomponents set PROFILE during service registration so the GATT server transmit
 * path will not stomp profile-prepared payload bytes in `read_data1`.
 */
typedef enum {
  SL_GATT_SERVER_NOTIFY_PAYLOAD_DEFAULT = 0,
  SL_GATT_SERVER_NOTIFY_PAYLOAD_PROFILE = 1,
} sl_gatt_server_notify_payload_policy_t;

extern volatile sl_gatt_server_notify_payload_policy_t sl_gatt_server_notify_payload_policy;

/**
 * @brief Last prepared notify/indicate payload length when policy is PROFILE.
 *
 * Set together with `read_data1` in the `event_data_transmit_server` path (before
 * `rsi_ble_gatt_server_data_transmit()`). Zero means fall back to configured max length.
 */
extern volatile uint16_t sl_gatt_server_profile_notify_payload_len;

/**
 * @brief PROFILE path: encode notify/indicate payload into `rsi_ble_conn_info[conn_id].read_data1`
 *        and set `sl_gatt_server_profile_notify_payload_len`.
 *
 * Weak default in `gatt_server_event_hdlrs.c` is a no-op; profile subcomponents may override.
 */
void sl_gatt_server_profile_refresh_notify_payload(uint8_t ble_conn_id);

/*=======================================================================*/
//   GATT SERVER CONSTANTS AND UUIDS
/*=======================================================================*/

// Standard Bluetooth UUIDs (as per Bluetooth SIG)
#define RSI_BLE_CHAR_SERV_UUID                 0x2803 // Characteristic Declaration UUID
#define RSI_BLE_CLIENT_CHAR_UUID               0x2902 // Client Characteristic Configuration UUID
#define RSI_BLE_CHAR_PRESENTATION_FORMATE_UUID 0x2904 // Characteristic Presentation Format UUID

// Example Service UUIDs (application-specific)
#define RSI_BLE_NEW_SERVICE_UUID    0xAABB // Simple chat service UUID
#define RSI_BLE_ATTRIBUTE_1_UUID    0x1AA1 // Simple chat attribute UUID
#define RSI_BLE_CUSTOM_SERVICE_UUID 0x1AA1 // Custom service UUID
#define RSI_BLE_CUSTOM_LEVEL_UUID   0x1BB1 // Custom level characteristic UUID

// Characteristic Presentation Format values
#define RSI_BLE_UINT8_FORMAT          4      // UINT8 format type
#define RSI_BLE_EXPONENT              0      // Exponent value
#define RSI_BLE_PERCENTAGE_UNITS_UUID 0x27AD // Percentage unit UUID
#define RSI_BLE_NAME_SPACE            1      // Namespace value
#define RSI_BLE_DESCRIPTION           1      // Description value

// NOTE: SEC_MODE_1_LEVEL_1 and ATT_REC_MAINTAIN_IN_HOST are defined in rsi_ble_common_config.h
// Removed duplicate definitions to avoid redefinition errors

/*=======================================================================*/
//   GATT SERVER DATA STRUCTURES
/*=======================================================================*/

// GATT Server attribute list configuration
#define BLE_ATT_REC_SIZE  500 // Maximum attribute record size in bytes
#define BLE_NO_OF_VAL_ATT 10  // Maximum number of attribute values

typedef struct rsi_ble_att_list_s {
  uuid_t char_uuid;
  uint16_t handle;
  uint16_t value_len;
  uint16_t max_value_len;
  uint8_t char_val_prop;
  void *value;
} rsi_ble_att_list_t;

typedef struct rsi_ble_s {
  uint8_t DATA[BLE_ATT_REC_SIZE];
  uint16_t DATA_ix;
  uint16_t att_rec_list_count;
  rsi_ble_att_list_t att_rec_list[BLE_NO_OF_VAL_ATT];
} rsi_ble_t;

// GATT Server-owned variables (attribute list and handles)
extern rsi_ble_t att_list;
extern rsi_ble_t ta_att_list;
/* Property-based handle names */
extern volatile uint16_t write_attribute_handle;
extern volatile uint16_t notify_attribute_handle;
extern volatile uint16_t write_without_response_attribute_handle;
extern volatile uint16_t indicate_attribute_handle;

// Multi-protocol variables (GAP owns - use extern)
extern rsi_ble_conn_info_t rsi_ble_conn_info[];

/** Attribute list utility - used by GATT Server and by GATT Client (prepare write) when both components present */
rsi_ble_att_list_t *rsi_gatt_get_attribute_from_list(rsi_ble_t *p_val, uint16_t handle);

/**
 * @brief GATT Server Utility Functions
 */

/** Initialize GATT Server-specific fields in connection info array */
void rsi_ble_gatt_server_default_init(void);

/** Notify/indicate TX path (reads `read_data1` / `max_data_length` for this connection). */
void rsi_ble_gatt_server_data_transmit(uint8_t ble_conn_id);

/** Initialize GATT Server-specific connection buffer configuration */
int8_t rsi_ble_gatt_server_initialize_conn_config(rsi_ble_conn_config_t *ble_conn_spec_conf);

/** Add simple chat service to GATT database */
uint32_t rsi_ble_add_simple_chat_serv(void);

/** Add simple chat service 2 to GATT database */
uint32_t rsi_ble_add_simple_chat_serv2(void);

/** Add custom service to GATT database */
uint32_t rsi_ble_add_custom_service_serv(void);

/**
 * @brief Add characteristic declaration attribute to a GATT service.
 */
void rsi_ble_add_char_serv_att(void *serv_handler,
                               uint16_t handle,
                               uint8_t val_prop,
                               uint16_t att_val_handle,
                               uuid_t att_val_uuid,
                               uint16_t auth_read);

/**
 * @brief Add characteristic value attribute to a GATT service.
 */
void rsi_ble_add_char_val_att(void *serv_handler,
                              uint16_t handle,
                              uuid_t att_type_uuid,
                              uint8_t val_prop,
                              uint8_t *data,
                              uint8_t data_len,
                              uint8_t auth_read);

/**
 * @brief Register GATT services for this build.
 * @return int32_t - RSI_SUCCESS on success, error code otherwise
 *
 * Weak default implementation registers the current generic/default services.
 * Profile/service subcomponents (or application code) can provide a strong
 * implementation to replace this behavior.
 *
 * Runtime contract: assign the operation handles your service model uses
 * (`write_attribute_handle`, `notify_attribute_handle`, optionally
 * `write_without_response_attribute_handle`, `indicate_attribute_handle`).
 * Handles for properties the profile does not expose may remain at their defaults (e.g. 0).
 */
int32_t sl_gatt_server_register_services_hook(void);

/**
 * @brief Initialize GATT Server component
 * @return int32_t - SL_STATUS_OK on success, error code otherwise
 *
 * Handles callback registration and ALL GATT server initialization.
 */
int32_t ble_gatt_server_init(void);

#endif // GATT_SERVER_H
