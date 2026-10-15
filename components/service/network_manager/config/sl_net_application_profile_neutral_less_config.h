/***************************************************************************/ /**
 * @file
 * @brief Neutral-less switch application profile configuration
 * @details In-use / not-in-use readability aliases and overridable parameter
 *          macros for the Neutral-less Matter switch application profile.
 *          Runtime enable applies POWER_SAVE values; disable applies
 *          HIGH_PERFORMANCE values.
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
#pragma once

#if (SL_NET_APP_PROFILE == SL_NET_APP_PROFILE_NEUTRAL_LESS_SWITCH) || defined(DOXYGEN)

/***************************************************************************/ /**
 * @def SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE
 * @brief Neutral-less in-use config group (power-constrained / profile enabled).
 * @details Alias of @ref SL_NET_APP_PROFILE_CONFIG_IN_USE_SET. Not
 *          @c sl_wifi_set_performance_profile().
 * @ingroup SL_NET_APPLICATION_PROFILE_TYPES
 ******************************************************************************/
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE SL_NET_APP_PROFILE_CONFIG_IN_USE_SET

/***************************************************************************/ /**
 * @def SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE
 * @brief Neutral-less not-in-use config group (non-constrained / profile disabled).
 * @details Alias of @ref SL_NET_APP_PROFILE_CONFIG_NOT_IN_USE_SET. Not
 *          @c sl_wifi_set_performance_profile().
 * @ingroup SL_NET_APPLICATION_PROFILE_TYPES
 ******************************************************************************/
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE SL_NET_APP_PROFILE_CONFIG_NOT_IN_USE_SET

/** @cond DOXYGEN_SHOULD_SKIP_THIS */
/* ============================================================================
 * In-use / POWER_SAVE defaults (Neutral-less constrained values)
 * ============================================================================ */

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_OPPORTUNISTIC_SLEEP_ENABLE
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_OPPORTUNISTIC_SLEEP_ENABLE 1
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_LIMIT_RATES
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_LIMIT_RATES 1
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_AVERAGE_CURRENT_WINDOW_MS
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_AVERAGE_CURRENT_WINDOW_MS 250
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_CURRENT_LIMIT_MA
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_CURRENT_LIMIT_MA 30
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_SCAN_OFF_TIME_MS
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_SCAN_OFF_TIME_MS 100
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_MAX_TX_RETRANSMISSIONS
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_MAX_TX_RETRANSMISSIONS 4
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_AGGREGATION_TX_ENABLE
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_AGGREGATION_TX_ENABLE 0
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_AGGREGATION_RX_BUFFER_SIZE
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_AGGREGATION_RX_BUFFER_SIZE 1
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_ACTIVE_SCAN_TIMEOUT_MS
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_ACTIVE_SCAN_TIMEOUT_MS 30
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_PASSIVE_SCAN_TIMEOUT_MS
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE_PASSIVE_SCAN_TIMEOUT_MS 110
#endif

/* ============================================================================
 * Not-in-use / HIGH_PERFORMANCE defaults (non-constrained values)
 * ============================================================================ */

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_OPPORTUNISTIC_SLEEP_ENABLE
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_OPPORTUNISTIC_SLEEP_ENABLE 0
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_LIMIT_RATES
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_LIMIT_RATES 0
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_AVERAGE_CURRENT_WINDOW_MS
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_AVERAGE_CURRENT_WINDOW_MS 0
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_CURRENT_LIMIT_MA
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_CURRENT_LIMIT_MA 0
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_SCAN_OFF_TIME_MS
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_SCAN_OFF_TIME_MS 0
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_MAX_TX_RETRANSMISSIONS
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_MAX_TX_RETRANSMISSIONS 15
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_AGGREGATION_TX_ENABLE
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_AGGREGATION_TX_ENABLE 1
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_AGGREGATION_RX_BUFFER_SIZE
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_AGGREGATION_RX_BUFFER_SIZE 8
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_ACTIVE_SCAN_TIMEOUT_MS
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_ACTIVE_SCAN_TIMEOUT_MS 100
#endif

#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_PASSIVE_SCAN_TIMEOUT_MS
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE_PASSIVE_SCAN_TIMEOUT_MS 400
#endif
/** @endcond */

#endif /* SL_NET_APP_PROFILE_NEUTRAL_LESS_SWITCH || DOXYGEN */
