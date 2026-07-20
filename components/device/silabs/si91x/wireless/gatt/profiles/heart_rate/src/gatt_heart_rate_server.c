/*******************************************************************************
* @file  gatt_heart_rate_server.c
* @brief Heart Rate profile — GATT server registration and notify payload.
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

#include <string.h>
#include "gatt_server.h"
#include "ble_event_hdlr_auto_gen.h"
#include "gatt_heart_rate_server.h"
#include "rsi_utils.h"
#include "rsi_ble_common_config.h"

/* Heart Rate profile UUIDs (Bluetooth SIG) */
#define RSI_BLE_HEART_RATE_SERVICE_UUID       0x180D
#define RSI_BLE_HEART_RATE_MEASUREMENT_UUID   0x2A37
#define RSI_BLE_SENSOR_LOCATION_UUID          0x2A38
#define RSI_BLE_HEART_RATE_CONTROL_POINT_UUID 0x2A39

typedef struct heart_rate_s {
  int8_t flags;
  int8_t heart_rate_measure_8;
  int16_t heart_rate_measure_16;
  int8_t energy_expended_status;
  int8_t rr_interval;
} heart_rate_t;

static heart_rate_t s_hr_rate = { 0x00, 75, 73, 70, 0 };

/* Modeled after examples/snippets/ble/ble_heart_rate_profile/app.c (pointer form so state persists). */
static uint8_t heartratefun(heart_rate_t *rate, uint8_t *p_data)
{
  int len = 0;

  p_data[len++] = (uint8_t)rate->flags;

  if (!(rate->flags & BIT(0))) {
    if (rate->heart_rate_measure_8 < 85) {
      rate->heart_rate_measure_8 = (int8_t)(rate->heart_rate_measure_8 + 2);
    } else {
      rate->heart_rate_measure_8 = 75;
    }
    p_data[len++] = (uint8_t)rate->heart_rate_measure_8;
  } else {
    uint16_t hr16 = (uint16_t)rate->heart_rate_measure_16;
    memcpy(&p_data[len], &hr16, sizeof(hr16));
    len += 2;
  }

  if (rate->flags & BIT(3)) {
    uint16_t energy = (uint16_t)rate->energy_expended_status;
    memcpy(&p_data[len], &energy, sizeof(energy));
    len += 2;
  }

  if (rate->flags & BIT(4)) {
    uint16_t rr = (uint16_t)rate->rr_interval;
    memcpy(&p_data[len], &rr, sizeof(rr));
    len += 2;
  }

  return (uint8_t)len;
}

void sl_gatt_server_heart_rate_data_transmit_event_handler(uint16_t __attribute__((unused)) status, void *event_data)
{
  uint8_t conn_id = *(uint8_t *)event_data;

  sl_gatt_server_profile_refresh_notify_payload(conn_id);
  rsi_ble_gatt_server_data_transmit(conn_id);
}

void sl_gatt_server_profile_refresh_notify_payload(uint8_t ble_conn_id)
{
  uint8_t len;

  if (ble_conn_id >= TOTAL_CONNECTIONS) {
    return;
  }

  len                                       = heartratefun(&s_hr_rate, rsi_ble_conn_info[ble_conn_id].read_data1);
  sl_gatt_server_profile_notify_payload_len = len;
}

int32_t sl_gatt_server_register_services_hook(void)
{
  int32_t status;
  uuid_t new_uuid                       = { 0 };
  rsi_ble_resp_add_serv_t new_serv_resp = { 0 };
  uint8_t sensor_data                   = 0x04; /* Wrist */
  uint8_t control_data                  = 0;
  uint8_t hr_measurement[8]             = { 0 };
  uint8_t hr_measurement_len            = 0;

  sl_gatt_server_notify_payload_policy = SL_GATT_SERVER_NOTIFY_PAYLOAD_PROFILE;

  hr_measurement_len = heartratefun(&s_hr_rate, hr_measurement);

  new_uuid.size      = 2;
  new_uuid.val.val16 = RSI_BLE_HEART_RATE_SERVICE_UUID;
  status             = rsi_ble_add_service(new_uuid, &new_serv_resp);
  if (status != RSI_SUCCESS) {
    return status;
  }

  /* Heart Rate Measurement (Notify) */
  new_uuid.size      = 2;
  new_uuid.val.val16 = RSI_BLE_HEART_RATE_MEASUREMENT_UUID;
  rsi_ble_add_char_serv_att(new_serv_resp.serv_handler,
                            new_serv_resp.start_handle + 1,
                            RSI_BLE_ATT_PROPERTY_NOTIFY,
                            new_serv_resp.start_handle + 2,
                            new_uuid,
                            0);
  notify_attribute_handle = new_serv_resp.start_handle + 2;
  rsi_ble_add_char_val_att(new_serv_resp.serv_handler,
                           new_serv_resp.start_handle + 2,
                           new_uuid,
                           RSI_BLE_ATT_PROPERTY_NOTIFY,
                           hr_measurement,
                           hr_measurement_len,
                           0);

  /* Body Sensor Location (Read) */
  new_uuid.size      = 2;
  new_uuid.val.val16 = RSI_BLE_SENSOR_LOCATION_UUID;
  rsi_ble_add_char_serv_att(new_serv_resp.serv_handler,
                            new_serv_resp.start_handle + 4,
                            RSI_BLE_ATT_PROPERTY_READ,
                            new_serv_resp.start_handle + 5,
                            new_uuid,
                            0);
  rsi_ble_add_char_val_att(new_serv_resp.serv_handler,
                           new_serv_resp.start_handle + 5,
                           new_uuid,
                           RSI_BLE_ATT_PROPERTY_READ,
                           &sensor_data,
                           sizeof(sensor_data),
                           0);

  /* Heart Rate Control Point (Write) */
  new_uuid.size      = 2;
  new_uuid.val.val16 = RSI_BLE_HEART_RATE_CONTROL_POINT_UUID;
  rsi_ble_add_char_serv_att(new_serv_resp.serv_handler,
                            new_serv_resp.start_handle + 6,
                            RSI_BLE_ATT_PROPERTY_WRITE,
                            new_serv_resp.start_handle + 7,
                            new_uuid,
                            0);
  write_attribute_handle = new_serv_resp.start_handle + 7;
  rsi_ble_add_char_val_att(new_serv_resp.serv_handler,
                           new_serv_resp.start_handle + 7,
                           new_uuid,
                           RSI_BLE_ATT_PROPERTY_WRITE,
                           &control_data,
                           sizeof(control_data),
                           0);

  /* No HR characteristic uses write-without-response; `write_without_response_attribute_handle` stays unused (0). */
  /* No characteristic in this service uses INDICATE; leave `indicate_attribute_handle`
   * at 0 (default from gatt_server_utilities). Do not alias it to the notify value handle. */

  ble_events_lut[event_data_transmit_server_event_id].handler = sl_gatt_server_heart_rate_data_transmit_event_handler;

  return RSI_SUCCESS;
}
