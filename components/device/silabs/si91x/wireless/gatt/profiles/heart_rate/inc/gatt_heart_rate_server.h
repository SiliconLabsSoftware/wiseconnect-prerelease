/*******************************************************************************
* @file  gatt_heart_rate_server.h
* @brief Heart Rate profile — GATT server public API (registration and server events)
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

#ifndef GATT_HEART_RATE_SERVER_H
#define GATT_HEART_RATE_SERVER_H

#include <stdint.h>

/**
 * @brief BLE `event_data_transmit_server` LUT handler: refresh HR payload then call `rsi_ble_gatt_server_data_transmit()`.
 */
void sl_gatt_server_heart_rate_data_transmit_event_handler(uint16_t status, void *event_data);

/**
 * @brief Strong override for GATT service registration hook.
 * @return RSI_SUCCESS on success, error code otherwise.
 */
int32_t sl_gatt_server_register_services_hook(void);

#endif // GATT_HEART_RATE_SERVER_H
