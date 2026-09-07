/***************************************************************************/ /**
* @file sl_log_hal_inline.h
* @brief Inlined timestamp accessors for the logging core (SiWx91x)
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

#ifndef SL_LOG_HAL_INLINE_H
#define SL_LOG_HAL_INLINE_H

/*
 * The logging core (sl_log.c) stamps every event with a timestamp and an epoch
 * on the hot path. Rather than routing those reads through the
 * sl_log_api_core_t function pointers, it calls the fixed-name static-inline
 * accessors declared here, and the build selects the matching implementation by
 * putting the right directory on the include path. On SiWx91x that is done by
 * the si91x_log component, which is also what provides log_platform_core.
 *
 * SiWx91x timebase: ULP timer 3, a free-running 1 MHz up-counter brought up in
 * sl_log_hal_start_timestamp_counter(). The reported time is that counter
 * shifted onto the captive core's (NWP) timebase by the delta measured during
 * time sync, which is why both halves are derived from a single 64-bit
 * composition instead of being read independently.
 */

#include "em_device.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief ULP timer instance driving the logger timebase. Must stay in step
 * with the instance configured by sl_log_hal_start_timestamp_counter(). */
#define SLI_LOG_SI91X_TIMER_INSTANCE 3

/*
 * State owned by sl_log_platform_specific.c, declared here so the readers below
 * stay free of function calls. All three are written outside the read path (the
 * overflow callback and the time-sync handler), hence volatile.
 */

/** @brief Software epoch: number of times the ULP timer counter has wrapped.
 * Advanced by the timer overflow callback and reset with the counter. */
extern volatile uint32_t sli_log_si91x_timer_epoch;

/** @brief Set once the host counter has been correlated with the captive core.
 * Readings taken before that are not on a meaningful timebase and report 0. */
extern volatile bool sli_log_si91x_timesync_done;

/** @brief Signed offset that shifts the raw counter onto the captive core's
 * timebase. Recomputed on every successful time sync. */
extern volatile int32_t sli_log_si91x_timestamp_delta;

/***************************************************************************/ /**
* @brief Read timestamp and epoch for one log event.
*
* Both halves come from one composition, so the pair is always self-consistent.
*
* @param[out] timestamp Low 32 bits of the 64-bit host time.
* @param[out] epoch     High 32 bits of the 64-bit host time.
******************************************************************************/
static inline void sli_log_hal_stamp_time(uint32_t *timestamp, uint32_t *epoch)
{
  if (!sli_log_si91x_timesync_done) {
    *timestamp = 0;
    *epoch     = 0;
    return;
  }

  *timestamp = TIMERS->MATCH_CTRL[SLI_LOG_SI91X_TIMER_INSTANCE].MCUULP_TMR_MATCH + sli_log_si91x_timestamp_delta;
  *epoch     = sli_log_si91x_timer_epoch;
}

/***************************************************************************/ /**
* @brief Read the low 32 bits of the 64-bit host time.
*
* @return Timestamp in microseconds, or 0 before time sync completes.
******************************************************************************/
static inline uint32_t sli_log_hal_get_timestamp(void)
{
  if (!sli_log_si91x_timesync_done) {
    return 0;
  }

  return TIMERS->MATCH_CTRL[SLI_LOG_SI91X_TIMER_INSTANCE].MCUULP_TMR_MATCH + sli_log_si91x_timestamp_delta;
}

/***************************************************************************/ /**
* @brief Read the epoch (high 32 bits of the 64-bit host time).
*
* @return Number of times the reported timestamp has wrapped.
******************************************************************************/
static inline uint32_t sli_log_hal_get_epoch(void)
{
  return sli_log_si91x_timer_epoch;
}

#ifdef __cplusplus
}
#endif

#endif /* SL_LOG_HAL_INLINE_H */