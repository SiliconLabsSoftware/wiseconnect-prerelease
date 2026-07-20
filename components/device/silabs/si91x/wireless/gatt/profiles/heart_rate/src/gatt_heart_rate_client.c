/*******************************************************************************
 * @file  gatt_heart_rate_client.c
 * @brief Heart Rate profile — GATT client notification payload handling.
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

#include <stdint.h>

#if defined(SL_SI91X_BLE_GATT_CLIENT_COMPONENT)

#include <stdio.h>
#include "gatt_client.h"

/*******************************************************************************
 * Strong override of the weak hook in gatt_client_utilities.c.
 * Decodes Heart Rate Measurement characteristic payloads (legacy app format).
 ******************************************************************************/
void sl_ble_gatt_client_notification_received(uint8_t ble_conn_id, const uint8_t *att_value, uint16_t length)
{
  (void)ble_conn_id;

  if ((att_value == NULL) || (length < 2U)) {
    return;
  }

  if ((att_value[0] & 1U) == 0U) {
    printf("\nbpm: 0x%02x\r\n", att_value[1]);
  } else if (length >= 3U) {
    printf("\nbpm: 0x%04x \r\n", (unsigned int)(att_value[1] | (uint16_t)((uint16_t)att_value[2] << 8)));
  }
}

#endif /* SL_SI91X_BLE_GATT_CLIENT_COMPONENT */
