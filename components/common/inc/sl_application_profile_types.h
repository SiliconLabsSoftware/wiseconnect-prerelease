/***************************************************************************/ /**
 * @file
 * @brief Application profile types shared by network and Wi-Fi layers
 *******************************************************************************
 * # License
 * <b>Copyright 2026  Silicon Laboratories Inc. www.silabs.com</b>
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
#pragma once

#include <stdint.h>

/** \addtogroup SL_NET_TYPES
 * @{
 */

/**
 * @brief Config group of a compile-time application profile (host-side apply state).
 * @details Shared by SL Net and the Wi-Fi layer. Profile-specific names (for example
 *          Neutral-less POWER_SAVE / HIGH_PERFORMANCE) are optional aliases of these values.
 */
typedef enum {
  SL_APPLICATION_PROFILE_CONFIG_IN_USE_SET     = 0, ///< Profile in use (enable / power-constrained)
  SL_APPLICATION_PROFILE_CONFIG_NOT_IN_USE_SET = 1, ///< Temporary non-profile defaults (disable)
  SL_APPLICATION_PROFILE_CONFIG_NOT_SET        = 2, ///< No group applied yet (get before successful set)
} sl_application_profile_config_group_t;

/** Opportunistic-sleep advanced-config payload (union member after 32-bit sub_cmd_id). */
typedef struct {
  uint8_t opportunistic_sleep_enable; ///< Enable opportunistic sleep
  uint8_t limit_rates;                ///< Limit transmit rates for power savings
  uint16_t average_current_window_ms; ///< Rolling average current measurement window (ms)
  uint16_t current_limit_ma;          ///< Average current limit (mA) over the window
  uint16_t scan_off_time_ms;          ///< Scan radio off time between scan channels (ms)
} sl_application_profile_opportunistic_sleep_config_t;

/** Retry advanced-config payload. */
typedef struct {
  uint8_t max_tx_retransmissions; ///< Maximum TX retransmissions
} sl_application_profile_retry_config_t;

/** Aggregation advanced-config payload. */
typedef struct {
  uint8_t vap_id;                     ///< VAP identifier
  uint8_t aggregation_tx_enable;      ///< TX aggregation enable flag
  uint8_t aggregation_rx_buffer_size; ///< RX aggregation buffer size
} sl_application_profile_aggregation_config_t;

/**
 * @brief Preset bundle for one application profile.
 * @details Aggregates advanced-config structs and scan timeout values used when
 *          applying an application profile config group.
 */
typedef struct {
  sl_application_profile_opportunistic_sleep_config_t opportunistic_sleep; ///< Opportunistic-sleep preset
  sl_application_profile_retry_config_t retry;                             ///< Retry preset
  sl_application_profile_aggregation_config_t aggregation;                 ///< Aggregation preset
  uint16_t active_scan_timeout_ms;                                         ///< Active scan dwell time (ms)
  uint16_t passive_scan_timeout_ms;                                        ///< Passive scan timeout (ms)
} sl_application_profile_preset_t;

/** @} */
