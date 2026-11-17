/***************************************************************************/ /**
 * @file  sl_log_sdk_component_config.h
 * @brief Per-component log levels for WiseConnect SDK components.
 *
 * Change @c SL_LOG_<component>_COMPONENT_LEVEL to control how much each component
 * logs at run time. The comment on each line shows the
 * log tag in debug output (for example @c [hal_si91x]). See the configuration
 * example before the first component entry.
 *
 * These macros do not control compile-time stripping or flash size. Messages may
 * still be present in the firmware; @c SL_LOG_CONFIG_LEVEL_NONE suppresses output
 * at run time only. Compile-time stripping is separate: set
 * @c SL_LOG_CONFIG_LEVEL_COMPILE_TIME to @c SL_LOG_CONFIG_LEVEL_NONE in
 * @c sl_log_common_config.h (Silicon Labs hosts), or @c SLI_HOST_CURRENT_DBGLOG_LEVEL
 * to @c SLI_HOST_DBGLOG_LVL_NONE in @c sl_constants.h (external hosts), to remove logs from all components that
 * use the default compile threshold (@c SL_LOG_CONFIG_LEVEL_COMPILE_TIME). To
 * remove logs from one component only, set @c SL_LOG_COMPONENT_<name>_COMPILE_LEVEL
 * in that component's @c *_log_helper.h.
 *
 * Each macro may be set to one of:
 * @li @c SL_LOG_CONFIG_LEVEL_DEBUG — debug and above
 * @li @c SL_LOG_CONFIG_LEVEL_INFO — info and above
 * @li @c SL_LOG_CONFIG_LEVEL_WARN — warnings and errors only
 * @li @c SL_LOG_CONFIG_LEVEL_ERROR — errors only
 * @li @c SL_LOG_CONFIG_LEVEL_NONE — no logs from that component
 * @li @c SL_LOG_CONFIG_LEVEL_COMPILE_TIME — same threshold as the application
 *     global log level (see example below; default for all components)
 *
 * A log line is printed only if its severity is at or above the level set here
 * and it was not stripped at compile time in the component log helper.
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
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * ******************************************************************************/
#pragma once

#include "sl_constants.h"
#if (defined(SLI_SI91X_MCU_INTERFACE) || defined(SLI_SI91X_NCP_INTERFACE) || defined(SLI_SIWX3XX))
#include "sl_log_common_config.h"
#endif

// ---------------------------------------------------------------------------
// Configuration example (SiWx91x HAL and other components)
//
// Set each SL_LOG_*_COMPONENT_LEVEL line to tune that component independently.
// Example:
//
//   #define SL_LOG_HAL_SI91X_COMPONENT_LEVEL       SL_LOG_CONFIG_LEVEL_WARN
//   #define SL_LOG_NETWORK_MANAGER_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_DEBUG
//   #define SL_LOG_MQTT_COMPONENT_LEVEL            SL_LOG_CONFIG_LEVEL_ERROR
//
// Defaults use SL_LOG_CONFIG_LEVEL_COMPILE_TIME; see the file header for allowed
// values, runtime vs compile-time behavior, and global or per-component stripping.
// ---------------------------------------------------------------------------

// Log tag: [hal_si91x]
#define SL_LOG_HAL_SI91X_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [event_engine]
#define SL_LOG_EVENT_ENGINE_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [command_engine]
#define SL_LOG_COMMAND_ENGINE_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [buffer_manager]
#define SL_LOG_BUFFER_MANAGER_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [sli_wifi]
#define SL_LOG_SLI_WIFI_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [sl_wifi]
#define SL_LOG_WIFI_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [si91x_wifi_cmd_engine]
#define SL_LOG_SI91X_WIFI_CMD_ENGINE_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [si91x_wifi_event_handler]
#define SL_LOG_SI91X_WIFI_EVENT_HANDLER_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [si91x_socket]
#define SL_LOG_SI91X_SOCKET_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [network_manager]
#define SL_LOG_NETWORK_MANAGER_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [mqtt]
#define SL_LOG_MQTT_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [si91x_driver]
#define SL_LOG_SI91X_DRIVER_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [si91x_ahb_bus]
#define SL_LOG_SI91X_AHB_BUS_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [si91x_ncp_uart]
#define SL_LOG_SI91X_NCP_UART_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [si91x_ncp_spi]
#define SL_LOG_SI91X_NCP_SPI_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [si91x_net]
#define SL_LOG_SI91X_NET_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [si91x_bsd_socket]
#define SL_LOG_SI91X_BSD_SOCKET_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [si91x_async_socket]
#define SL_LOG_SI91X_ASYNC_SOCKET_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [http_client]
#define SL_LOG_HTTP_CLIENT_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [http_server]
#define SL_LOG_HTTP_SERVER_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [websocket_client]
#define SL_LOG_WEBSOCKET_CLIENT_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [sl_utility]
#define SL_LOG_UTILITY_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [sl_si91x_mem_pool_buffers_with_quota]
#define SL_LOG_SI91X_MEM_POOL_QUOTA_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME

// Log tag: [sl_sntp]
#define SL_LOG_SNTP_COMPONENT_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME
