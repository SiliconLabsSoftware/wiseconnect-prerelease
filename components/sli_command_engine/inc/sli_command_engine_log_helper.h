/***************************************************************************/ /**
 * @file sli_command_engine_log_helper.h
 * @brief Component-scoped logging for the Command Engine
 *
 * Compile-time filter: @ref SL_LOG_COMPONENT_COMMAND_ENGINE_COMPILE_LEVEL
 * defaults to @c SL_LOG_CONFIG_LEVEL_COMPILE_TIME.
 *******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 */

#ifndef SLI_COMMAND_ENGINE_LOG_HELPER_H
#define SLI_COMMAND_ENGINE_LOG_HELPER_H

#include "sl_log_sdk_component_config.h"

/**
 * @def SL_LOG_COMPONENT_COMMAND_ENGINE_COMPILE_LEVEL
 * @brief Compile-time log filter level for the Command Engine component.
 *
 * @details Messages below this threshold are stripped at compile time.
 *          Defaults to @c SL_LOG_CONFIG_LEVEL_COMPILE_TIME.
 */
#ifndef SL_LOG_COMPONENT_COMMAND_ENGINE_COMPILE_LEVEL
#define SL_LOG_COMPONENT_COMMAND_ENGINE_COMPILE_LEVEL SL_LOG_CONFIG_LEVEL_COMPILE_TIME
#endif

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
#define SLI_SDK_LOG_COMPONENT_RUNTIME_LEVEL sl_log_command_engine_component_level
#define SLI_SDK_LOG_COMPONENT_COMPILE_LEVEL SL_LOG_COMPONENT_COMMAND_ENGINE_COMPILE_LEVEL
/** @endcond */
#include "sli_sdk_component_log_helper.h"

#endif
