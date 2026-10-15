/***************************************************************************/ /**
 * @file
 * @brief SL Net application profile API
 * @details Compile-time application profile selection and runtime apply of
 *          in-use / not-in-use config groups for the selected profile.
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

#include "sl_status.h"
#include "sl_constants.h"
#include "sl_application_profile_types.h"

/** \addtogroup SL_NET_APPLICATION_PROFILE_TYPES Application profile types
 * \ingroup SL_NET_TYPES
 * @{ */

/***************************************************************************/ /**
 * @def SL_NET_APP_PROFILE_NONE
 * @brief No application profile selected at compile time.
 * @details When @c SL_NET_APP_PROFILE is set to this value, runtime apply APIs
 *          reject configuration (for example @c SL_STATUS_INVALID_PROFILE).
 ******************************************************************************/
#ifndef SL_NET_APP_PROFILE_NONE
#define SL_NET_APP_PROFILE_NONE 0
#endif

/***************************************************************************/ /**
 * @def SL_NET_APP_PROFILE_NEUTRAL_LESS_SWITCH
 * @brief Neutral-less Matter switch application profile identifier.
 * @details Compiles in the Neutral-less in-use / not-in-use parameter sets.
 *          Use @ref sl_net_set_application_profile_config to apply a config group
 *          at runtime.
 ******************************************************************************/
#ifndef SL_NET_APP_PROFILE_NEUTRAL_LESS_SWITCH
#define SL_NET_APP_PROFILE_NEUTRAL_LESS_SWITCH 1
#endif

/**
 * @typedef sl_net_application_profile_config_group_t
 * @brief Config group of the compile-time application profile.
 * @details Alias of @ref sl_application_profile_config_group_t. Profile-specific
 *          names (for example @ref SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE) are
 *          optional aliases of these values, not additional enumerators.
 */
typedef sl_application_profile_config_group_t sl_net_application_profile_config_group_t;

/***************************************************************************/ /**
 * @def SL_NET_APP_PROFILE_CONFIG_IN_USE_SET
 * @brief Alias of @ref SL_APPLICATION_PROFILE_CONFIG_IN_USE_SET.
 * @details Profile-enabled / power-constrained config group.
 ******************************************************************************/
#define SL_NET_APP_PROFILE_CONFIG_IN_USE_SET SL_APPLICATION_PROFILE_CONFIG_IN_USE_SET

/***************************************************************************/ /**
 * @def SL_NET_APP_PROFILE_CONFIG_NOT_IN_USE_SET
 * @brief Alias of @ref SL_APPLICATION_PROFILE_CONFIG_NOT_IN_USE_SET.
 * @details Temporary non-profile / unconstrained config group.
 ******************************************************************************/
#define SL_NET_APP_PROFILE_CONFIG_NOT_IN_USE_SET SL_APPLICATION_PROFILE_CONFIG_NOT_IN_USE_SET

/***************************************************************************/ /**
 * @def SL_NET_APP_PROFILE_CONFIG_NOT_SET
 * @brief Alias of @ref SL_APPLICATION_PROFILE_CONFIG_NOT_SET.
 * @details No config group has been applied yet. Invalid for set; valid for get.
 ******************************************************************************/
#define SL_NET_APP_PROFILE_CONFIG_NOT_SET SL_APPLICATION_PROFILE_CONFIG_NOT_SET

/** @} */

/*
 * Include UC compile-time profile selection.
 * sl_net_application_profile_config.h defines SL_NET_APP_PROFILE only when it is
 * not already set (Studio UC / edited config file, or -DSL_NET_APP_PROFILE=...).
 * Profile-specific aliases/knobs load after the selected value is known.
 */
#include "sl_net_application_profile_config.h"

#if SL_NET_APP_PROFILE == SL_NET_APP_PROFILE_NEUTRAL_LESS_SWITCH
#include "sl_net_application_profile_neutral_less_config.h"
#endif

/** \addtogroup SL_NET_APPLICATION_PROFILE_FUNCTIONS Application profile API
 * \ingroup SL_NET_FUNCTIONS
 * @{ */

/***************************************************************************/ /**
 * @brief
 *   Apply one config group of the compile-time application profile.
 *
 * @details
 *   Fills local config structs from the selected macro value set and applies
 *   them via the internal Wi-Fi path. Re-applying the same group is valid and
 *   idempotent.
 *
 * @param[in] config_group
 *   @ref SL_NET_APP_PROFILE_CONFIG_IN_USE_SET,
 *   @ref SL_NET_APP_PROFILE_CONFIG_NOT_IN_USE_SET,
 *   or a Neutral-less alias when that profile is compiled in
 *   (@ref SL_NET_APP_PROFILE_NEUTRAL_LESS_POWER_SAVE /
 *   @ref SL_NET_APP_PROFILE_NEUTRAL_LESS_HIGH_PERFORMANCE).
 *   @ref SL_NET_APP_PROFILE_CONFIG_NOT_SET is rejected.
 *
 * @pre
 *   @ref sl_net_init should be called before this API.
 * @pre
 *   @c SL_NET_APP_PROFILE must not be @ref SL_NET_APP_PROFILE_NONE.
 *
 * @return
 *   sl_status_t. Returns @c SL_STATUS_NOT_INITIALIZED if the Wi-Fi device is not
 *   initialized. Returns @c SL_STATUS_INVALID_PROFILE if @c SL_NET_APP_PROFILE is
 *   @ref SL_NET_APP_PROFILE_NONE. Returns @c SL_STATUS_INVALID_PARAMETER for
 *   invalid @p config_group (including @ref SL_NET_APP_PROFILE_CONFIG_NOT_SET).
 *   On mid-sequence failure: fail-fast, no automatic rollback; application must
 *   re-call to complete configuration.
 *
 * @note
 *   Application profiles are not Wi-Fi performance/power profiles
 *   (@c sl_wifi_set_performance_profile).
 * @note
 *   When applying Neutral-less in-use / POWER_SAVE, 11n mode must already be
 *   enabled during Wi-Fi initialization.
 * @note
 *   After a successful apply, FW retains the configuration across Wi-Fi
 *   disconnect and join/rejoin failure. The application need not re-call this
 *   API on those events unless switching between in-use and not-in-use.
 ******************************************************************************/
sl_status_t sl_net_set_application_profile_config(sl_net_application_profile_config_group_t config_group);

/***************************************************************************/ /**
 * @brief
 *   Get the config group currently applied by the application profile.
 *
 * @param[out] config_group
 *   @ref SL_NET_APP_PROFILE_CONFIG_IN_USE_SET,
 *   @ref SL_NET_APP_PROFILE_CONFIG_NOT_IN_USE_SET,
 *   or @ref SL_NET_APP_PROFILE_CONFIG_NOT_SET if no group has been applied yet
 *   (including when @c SL_NET_APP_PROFILE is @ref SL_NET_APP_PROFILE_NONE, since
 *   set cannot apply a group in that build).
 *   Does not return individual knob values.
 *
 * @return
 *   sl_status_t. Returns @c SL_STATUS_NULL_POINTER if @p config_group is NULL.
 *   Returns @c SL_STATUS_NOT_INITIALIZED if the Wi-Fi device is not initialized.
 *   Returns @c SL_STATUS_OK on success.
 ******************************************************************************/
sl_status_t sl_net_get_application_profile_config_group(sl_net_application_profile_config_group_t *config_group);

/** @} */
