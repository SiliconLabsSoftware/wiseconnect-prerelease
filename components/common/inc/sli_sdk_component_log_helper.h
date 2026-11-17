/***************************************************************************/ /**
 * @file sli_sdk_component_log_helper.h
 * @brief Shared implementation for per-component SL_DEBUG_LOG_V2 filtering.
 *
 * Each component *_log_helper.h must define @c SLI_SDK_LOG_COMPONENT_RUNTIME_LEVEL and
 * @c SLI_SDK_LOG_COMPONENT_COMPILE_LEVEL before including this header.
 *******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 ******************************************************************************/

#ifndef SLI_SDK_COMPONENT_LOG_HELPER_H
#define SLI_SDK_COMPONENT_LOG_HELPER_H

#ifndef SLI_SDK_LOG_COMPONENT_RUNTIME_LEVEL
#error SLI_SDK_LOG_COMPONENT_RUNTIME_LEVEL must be defined to the component sl_log_*_component_level symbol
#endif
#ifndef SLI_SDK_LOG_COMPONENT_COMPILE_LEVEL
#error SLI_SDK_LOG_COMPONENT_COMPILE_LEVEL must be defined to SL_LOG_COMPONENT_<name>_COMPILE_LEVEL
#endif

#include "sl_constants.h"
#if (defined(SLI_SI91X_MCU_INTERFACE) || defined(SLI_SI91X_NCP_INTERFACE) || defined(SLI_SIWX3XX))
#include "sl_log.h"
#include "sl_log_common_config.h"
#endif

extern sl_log_level_t SLI_SDK_LOG_COMPONENT_RUNTIME_LEVEL;

#if (defined(SLI_SI91X_MCU_INTERFACE) || defined(SLI_SI91X_NCP_INTERFACE) || defined(SLI_SIWX3XX))

#if (SLI_SDK_LOG_COMPONENT_COMPILE_LEVEL == SL_LOG_CONFIG_LEVEL_NONE)

#define SLI_SDK_LOG_COMPONENT_PRINT_DEBUG(fmt, ...) \
  do {                                              \
  } while (0)
#define SLI_SDK_LOG_COMPONENT_PRINT_INFO(fmt, ...) \
  do {                                             \
  } while (0)
#define SLI_SDK_LOG_COMPONENT_PRINT_WARN(fmt, ...) \
  do {                                             \
  } while (0)
#define SLI_SDK_LOG_COMPONENT_PRINT_ERROR(fmt, ...) \
  do {                                              \
  } while (0)

#else

#if (SLI_SDK_LOG_COMPONENT_COMPILE_LEVEL <= SL_LOG_CONFIG_LEVEL_DEBUG)
#define SLI_SDK_LOG_COMPONENT_PRINT_DEBUG(fmt, ...)                         \
  do {                                                                      \
    if (SLI_SDK_LOG_COMPONENT_RUNTIME_LEVEL <= SL_LOG_CONFIG_LEVEL_DEBUG) { \
      SL_PRINT_STRING_DEBUG(fmt, ##__VA_ARGS__);                            \
    }                                                                       \
  } while (0)
#else
#define SLI_SDK_LOG_COMPONENT_PRINT_DEBUG(fmt, ...) \
  do {                                              \
  } while (0)
#endif

#if (SLI_SDK_LOG_COMPONENT_COMPILE_LEVEL <= SL_LOG_CONFIG_LEVEL_INFO)
#define SLI_SDK_LOG_COMPONENT_PRINT_INFO(fmt, ...)                         \
  do {                                                                     \
    if (SLI_SDK_LOG_COMPONENT_RUNTIME_LEVEL <= SL_LOG_CONFIG_LEVEL_INFO) { \
      SL_PRINT_STRING_INFO(fmt, ##__VA_ARGS__);                            \
    }                                                                      \
  } while (0)
#else
#define SLI_SDK_LOG_COMPONENT_PRINT_INFO(fmt, ...) \
  do {                                             \
  } while (0)
#endif

#if (SLI_SDK_LOG_COMPONENT_COMPILE_LEVEL <= SL_LOG_CONFIG_LEVEL_WARN)
#define SLI_SDK_LOG_COMPONENT_PRINT_WARN(fmt, ...)                         \
  do {                                                                     \
    if (SLI_SDK_LOG_COMPONENT_RUNTIME_LEVEL <= SL_LOG_CONFIG_LEVEL_WARN) { \
      SL_PRINT_STRING_WARN(fmt, ##__VA_ARGS__);                            \
    }                                                                      \
  } while (0)
#else
#define SLI_SDK_LOG_COMPONENT_PRINT_WARN(fmt, ...) \
  do {                                             \
  } while (0)
#endif

#if (SLI_SDK_LOG_COMPONENT_COMPILE_LEVEL <= SL_LOG_CONFIG_LEVEL_ERROR)
#define SLI_SDK_LOG_COMPONENT_PRINT_ERROR(fmt, ...)                         \
  do {                                                                      \
    if (SLI_SDK_LOG_COMPONENT_RUNTIME_LEVEL <= SL_LOG_CONFIG_LEVEL_ERROR) { \
      SL_PRINT_STRING_ERROR(fmt, ##__VA_ARGS__);                            \
    }                                                                       \
  } while (0)
#else
#define SLI_SDK_LOG_COMPONENT_PRINT_ERROR(fmt, ...) \
  do {                                              \
  } while (0)
#endif

#endif /* SLI_SDK_LOG_COMPONENT_COMPILE_LEVEL == SL_LOG_CONFIG_LEVEL_NONE */

#else /* external host */

#if (SLI_SDK_LOG_COMPONENT_COMPILE_LEVEL == SLI_HOST_DBGLOG_LVL_NONE)

#define SLI_SDK_LOG_COMPONENT_PRINT_DEBUG(fmt, ...) \
  do {                                              \
  } while (0)
#define SLI_SDK_LOG_COMPONENT_PRINT_INFO(fmt, ...) \
  do {                                             \
  } while (0)
#define SLI_SDK_LOG_COMPONENT_PRINT_WARN(fmt, ...) \
  do {                                             \
  } while (0)
#define SLI_SDK_LOG_COMPONENT_PRINT_ERROR(fmt, ...) \
  do {                                              \
  } while (0)

#else

#if (SLI_SDK_LOG_COMPONENT_COMPILE_LEVEL <= SLI_HOST_DBGLOG_LVL_DEBUG)
#define SLI_SDK_LOG_COMPONENT_PRINT_DEBUG(fmt, ...)                         \
  do {                                                                      \
    if (SLI_SDK_LOG_COMPONENT_RUNTIME_LEVEL <= SLI_HOST_DBGLOG_LVL_DEBUG) { \
      sl_debug_log(fmt, ##__VA_ARGS__);                                     \
    }                                                                       \
  } while (0)
#else
#define SLI_SDK_LOG_COMPONENT_PRINT_DEBUG(fmt, ...) \
  do {                                              \
  } while (0)
#endif

#if (SLI_SDK_LOG_COMPONENT_COMPILE_LEVEL <= SLI_HOST_DBGLOG_LVL_INFO)
#define SLI_SDK_LOG_COMPONENT_PRINT_INFO(fmt, ...)                         \
  do {                                                                     \
    if (SLI_SDK_LOG_COMPONENT_RUNTIME_LEVEL <= SLI_HOST_DBGLOG_LVL_INFO) { \
      sl_debug_log(fmt, ##__VA_ARGS__);                                    \
    }                                                                      \
  } while (0)
#else
#define SLI_SDK_LOG_COMPONENT_PRINT_INFO(fmt, ...) \
  do {                                             \
  } while (0)
#endif

#if (SLI_SDK_LOG_COMPONENT_COMPILE_LEVEL <= SLI_HOST_DBGLOG_LVL_WARN)
#define SLI_SDK_LOG_COMPONENT_PRINT_WARN(fmt, ...)                         \
  do {                                                                     \
    if (SLI_SDK_LOG_COMPONENT_RUNTIME_LEVEL <= SLI_HOST_DBGLOG_LVL_WARN) { \
      sl_debug_log(fmt, ##__VA_ARGS__);                                    \
    }                                                                      \
  } while (0)
#else
#define SLI_SDK_LOG_COMPONENT_PRINT_WARN(fmt, ...) \
  do {                                             \
  } while (0)
#endif

#if (SLI_SDK_LOG_COMPONENT_COMPILE_LEVEL <= SLI_HOST_DBGLOG_LVL_ERROR)
#define SLI_SDK_LOG_COMPONENT_PRINT_ERROR(fmt, ...)                         \
  do {                                                                      \
    if (SLI_SDK_LOG_COMPONENT_RUNTIME_LEVEL <= SLI_HOST_DBGLOG_LVL_ERROR) { \
      sl_debug_log(fmt, ##__VA_ARGS__);                                     \
    }                                                                       \
  } while (0)
#else
#define SLI_SDK_LOG_COMPONENT_PRINT_ERROR(fmt, ...) \
  do {                                              \
  } while (0)
#endif

#endif

#endif

#undef SL_DEBUG_LOG_V2
#define SL_DEBUG_LOG_V2(level, fmt, ...) SLI_SDK_LOG_COMPONENT_PRINT_##level(fmt, ##__VA_ARGS__)

#endif /* SLI_SDK_COMPONENT_LOG_HELPER_H */
