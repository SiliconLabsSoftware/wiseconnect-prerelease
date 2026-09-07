/*******************************************************************************
 * @file sl_log_platform_specific.c
 * @brief Platform specific helpers for the logging core (SI91x)
 *
 * This file contains SI91x-specific implementations used by the logging core
 * such as timestamp management, timer synchronization across cores, ring
 * buffer write helpers and sleep/wake handling. Functions here are intended to
 * be small, efficient and safe for the platform's concurrency model.
 */

#include "sl_log_platform_specific.h"
#include "sl_log_helper.h"
#include "sl_log_hal_inline.h"
#include "sl_si91x_ulp_timer.h"
#ifdef SL_WIFI_COMPONENT_INCLUDED
#include "sl_utility.h"
#endif

#ifdef SL_CATALOG_LOG_BACKEND_PROPRIETARY_PRESENT
#include "sl_log_proprietary_config.h"
#endif

#define SL_SI91X_HOST_CORE_ID     0       // Host core identifier
#define SL_SI91X_LOG_TIMER_FREQ   1000000 // 1 MHz timer frequency for microsecond resolution
#define SL_SI91X_CAPTIVE_CORE_ID  1       // Captive core identifier
#define SL_LOG_ULP_TIMER_INSTANCE 3       //  Ulp timer instance using for logger

#if defined(SLI_CAPTIVE_CORE_PRESENT) && (SLI_CAPTIVE_CORE_PRESENT == 1)
#include "sl_si91x_constants.h"
#include "sl_si91x_driver.h"
#include "rsi_m4.h"
#define SL_TIMESYNC_TURNAROUND_TIME 15 // Turn around time for timesync request from Captive core(TA) in us
uint32_t sl_si91x_cc_timestamp = 0;    //Variable holding captive core timestamp
uint32_t sl_si91x_log_host_timesync_address =
  (uint32_t)&sl_si91x_cc_timestamp; //Variable holding address of Captive core timestamp
#endif
#ifdef SL_CATALOG_SI91X_LOG_BACKEND_IOSTREAM_PRESENT
#include "sl_iostream_init_instances.h"
#include "sl_event_handler.h"
#endif
/*******************************************************************************
 *                               GLOBAL VARIABLES
 ******************************************************************************/
volatile bool sli_log_si91x_timesync_done      = false;
volatile int32_t sli_log_si91x_timestamp_delta = 0;
volatile uint32_t sli_log_si91x_timer_epoch    = 0;

/* sli_nwp_log_event_t (NWP wire format) is defined once in sl_utility.h and
 * reached here through the si91x_log -> wiseconnect_common dependency. */

/**
* @brief Copy a packed NWP wire-format log record into a host ring-buffer slot.
*
* NWP uses @ref sli_nwp_log_event_t (packed, network layout). The M4 ring stores
* @ref sl_log_event_t. Layouts are kept identical so the log pipeline matches;
* This function serves as the dedicated mapping point from NWP to the ring 
* buffer and documents the relationship between them.
*/
/*
 * Build-time gating for the NWP-to-host event mapping:
 *   - SL_LOG_CONFIG_MODE == SL_LOG_CONFIG_MODE_HOST: M4 forwards NWP events to the host PC,
 *     so it must lift NWP wire-format records into the M4 ring buffer.
 *   - SL_CATALOG_LOG_BACKEND_SYSTEMVIEW_PRESENT: SystemView backend surfaces NWP events
 *     alongside M4 events on the trace, which requires the same lift.
 *   - SL_CATALOG_SI91X_LOG_BACKEND_IOSTREAM_COMPACT_PRESENT && SL_CATALOG_IOSTREAM_RTT_SI91X_PRESENT:
 *     IO Stream Compact over RTT is the only IO Stream variant that ingests NWP records into
 *     the M4 ring buffer for compact RTT streaming. The Formatted backend renders strings from
 *     M4 .log_fmt (NWP has no access to those format strings), and Compact over non-RTT
 *     transports does not currently consume NWP events, so this path is compiled out for
 *     those builds to avoid an unused-function warning.
 */
#if (defined(SL_LOG_CONFIG_MODE) && (SL_LOG_CONFIG_MODE == SL_LOG_CONFIG_MODE_HOST) \
     || defined(SL_CATALOG_LOG_BACKEND_SYSTEMVIEW_PRESENT)                          \
     || (defined(SL_CATALOG_SI91X_LOG_BACKEND_IOSTREAM_COMPACT_PRESENT)             \
         && defined(SL_CATALOG_IOSTREAM_RTT_SI91X_PRESENT)))
static void sli_sl_log_event_from_nwp(const sli_nwp_log_event_t *nwp, sl_log_event_t *out)
{
  // Clear first: ring slots are reused and the stacked copy used by the
  // SystemView and RTT paths is uninitialized, so any host field the wire format
  // does not supply - the epoch, and args beyond the three on the wire - would
  // otherwise reach the backend carrying stale data.
  *out = (sl_log_event_t){ 0 };

  // The NWP timestamp is on the captive core's timebase and carries no wrap
  // count, so the epoch stays zero.
  out->timestamp = nwp->timestamp;
  out->event_id  = nwp->event_id;
  out->args[0]   = nwp->args[0];
  out->args[1]   = nwp->args[1];
  out->args[2]   = nwp->args[2];
  out->arg_count = nwp->arg_count;
  out->core_id   = nwp->core_id;
  out->flags     = nwp->flags;
  out->version   = nwp->version;
}
#endif

#if (defined(SL_LOG_CONFIG_MODE) && (SL_LOG_CONFIG_MODE == SL_LOG_CONFIG_MODE_HOST))
extern sl_log_backend_status_t sl_log_backend_status;
#endif
/**
 * @brief Start the platform timestamp counter used by the logging core.
 *
 * Initializes and starts the ULP timer used to provide microsecond
 * resolution timestamps for log events.
 *
 * @return SL_STATUS_OK on success or an sl_status_t error code.
 */
sl_status_t sl_log_hal_start_timestamp_counter(void);

/**
 * @brief Stop the platform timestamp counter.
 *
 * Stops the ULP timer and unregisters any timeout callbacks.
 *
 * @return SL_STATUS_OK on success or an sl_status_t error code.
 */
sl_status_t sl_log_hal_stop_timestamp_counter(void);

/**
 * @brief Get the current timestamp count for the specified core.
 *
 * For the host (core_id == 0) this reads the local timer and applies the
 * synchronization delta. For captive cores it requests the timestamp via
 * P2P mechanism.
 *
 * @param[in] core_id Core identifier (0 = host)
 * @return Current timestamp in microseconds
 */
uint32_t sl_log_hal_get_timestamp_count(uint8_t core_id);

/**
 * @brief Get the timestamp epoch (high 32 bits of the 64-bit host time).
 *
 * Counts how many times the value returned by
 * sl_log_hal_get_timestamp_count() has wrapped. Read immediately after the
 * count so the two halves describe the same instant.
 *
 * @param[in] core_id Core identifier (0 = host)
 * @return Epoch paired with the most recent timestamp read
 */
uint32_t sl_log_hal_get_timestamp_epoch(uint8_t core_id);

/**
 * @brief Get the timestamp timer frequency (Hz) for a core.
 *
 * @param[in] core_id Core identifier (unused)
 * @return Timer frequency in Hz (typically 1,000,000)
 */
uint32_t sl_log_hal_get_timestamp_timer_frequency(uint8_t core_id);

/**
 * @brief Synchronize timestamp timer between cores.
 *
 * Requests a timestamp from the captive core and computes a delta used to
 * correlate timestamps across cores.
 *
 * @param[in] core_id Target core identifier
 * @param[in] args    Optional arguments (currently unused)
 * @return SL_STATUS_OK on success or an sl_status_t error code
 */
sl_status_t sl_log_hal_timer_sync(const void *args, uint8_t core_id);

/**
 * @brief Prepare logging subsystem before entering sleep.
 *
 * Typical actions include stopping timers and deinitializing backend
 * transports to reduce power consumption.
 *
 * @param[in] config Pointer to logging configuration
 * @return SL_STATUS_OK on success or an sl_status_t error code
 */
sl_status_t sl_log_hal_pre_sleep_process(const void *config);

/**
 * @brief Restore logging subsystem after wake-up from sleep.
 *
 * Reinitialize backend transports and restart the timestamp counter.
 *
 * @param[in] config Pointer to logging configuration
 * @return SL_STATUS_OK on success or an sl_status_t error code
 */
sl_status_t sl_log_hal_post_sleep_process(const void *config);

/**
 * @brief Get the platform-specific logging configuration.
 *
 * @param[out] args Pointer to a platform config structure to fill
 * @param[in]  core_id Core identifier
 * @return SL_STATUS_OK currently always returned
 */
sl_status_t sl_log_hal_get_configuration(void *args, uint8_t core_id);

/**
 * @brief Set the platform-specific logging configuration.
 *
 * @param[in] args Pointer to platform config structure
 * @param[in] core_id Core identifier
 * @return SL_STATUS_OK currently always returned
 */
sl_status_t sl_log_hal_set_configuration(const void *args, uint8_t core_id);

/***************************************************************************/ /**
 * @brief Write multiple log events into the ring buffer.
 *
 * Inserts a contiguous block of log events into the internal ring buffer while
 * maintaining correct wraparound, overwrite handling, and IRQ-safe updates to
 * buffer indices.  
 *
 * This function performs the write in three phases:
 * 1. **Reserve slots and advance write index (IRQ-off):**  
 *    - Atomically reduces `available_event_slots` by `count`.  
 *    - Checks whether the backend is currently busy; if insufficient space is
 *      available and the backend has not yet completed its previous transfer,
 *      the operation is rejected with ::SL_STATUS_NOT_AVAILABLE.  
 *    - Updates `write_index` with wraparound.
 *
 * 2. **Copy events into ring buffer (outside IRQ-off):**  
 *    - Performs one or two memcpy operations depending on wraparound.  
 *
 * 3. **Finalize event_count and adjust read_index (IRQ-off):**  
 *    - Updates `event_count`, clamping to the buffer's maximum size.  
 *    - If the write overwrote existing events, advances `read_index`
 *      appropriately.  
 *
 * @param[in] events  
 *   Pointer to an array of @ref sli_nwp_log_event_t (NWP wire format); each entry
 *   is converted into @ref sl_log_event_t in the ring buffer.
 *
 * @param[in] count  
 *   Number of events to write. Must be non-zero.
 *
 * @return sl_status_t  
 *   - ::SL_STATUS_OK  
 *       Successfully wrote all events into the ring buffer.  
 *   - ::SL_STATUS_INVALID_PARAMETER  
 *       `events` is NULL or `count` is zero.  
 *   - ::SL_STATUS_NOT_AVAILABLE  
 *       Not enough space is available and the backend transfer is still in
 *       progress, preventing safe overwrite accounting.  
 *
 * @note  
 * - This function is intended for internal logger operations where multiple
 *   events are written efficiently.  
 * - Overwrite behavior adheres to the standard ring buffer model: the oldest
 *   unread events are discarded first when capacity is exceeded.  
 * - Callers must ensure `events` points to valid and populated event data.
 *
 * @internal  
 * Uses critical sections (`__disable_irq` / `__enable_irq`) to guarantee that
 * write and read indices are updated atomically relative to ISR-based backends.
 ******************************************************************************/

#ifdef SL_WIFI_COMPONENT_INCLUDED
sl_status_t sl_log_write_multiple_to_ring_buffer(const sli_nwp_log_event_t *events, uint32_t count);
#endif

/**
 * @brief   Core API structure.
 * 
 */
sl_log_api_core_t sl_log_api_core = { .platform_core_init            = sl_log_hal_start_timestamp_counter,
                                      .platform_core_deinit          = sl_log_hal_stop_timestamp_counter,
                                      .get_timestamp                 = sl_log_hal_get_timestamp_count,
                                      .get_timestamp_epoch           = sl_log_hal_get_timestamp_epoch,
                                      .get_timestamp_timer_frequency = sl_log_hal_get_timestamp_timer_frequency,
                                      .post_sleep_process            = sl_log_hal_post_sleep_process,
                                      .pre_sleep_process             = sl_log_hal_pre_sleep_process,
                                      .set_configuration             = sl_log_hal_set_configuration,
                                      .get_configuration             = sl_log_hal_get_configuration,
                                      .time_sync                     = sl_log_hal_timer_sync };

/**
 * @brief Timer overflow callback function.
 * 
 */
static void timer_overflow_callback(void);

/**
 * @brief Timer overflow callback function.
 * 
 */
void timer_overflow_callback(void)
{
  sli_log_si91x_timer_epoch++;
}

/**
 * @brief Starts the timestamp counter. 
 * 
 * @return sl_status_t 
 */
sl_status_t sl_log_hal_start_timestamp_counter(void)
{

  ulp_timer_clk_src_config_t ulp_timer_clk_handle = {
    .ulp_timer_clk_type           = ENABLE_STATIC_CLK,
    .ulp_timer_sync_to_ulpss_pclk = 0,
    .ulp_timer_clk_input_src      = ULP_TIMER_REF_CLK_SRC,
    .ulp_timer_skip_switch_time   = 0,
  };

  ulp_timer_config_t sl_log_timer_handle = {
    .timer_num         = ULP_TIMER_3,
    .timer_mode        = ULP_TIMER_MODE_PERIODIC,
    .timer_type        = ULP_TIMER_TYP_1US,
    .timer_match_value = 0xFFFFFFFF,
    .timer_direction   = UP_COUNTER,
  };

  // The counter restarts from zero here, so the epoch has to restart with it or
  // the two halves of the 64-bit time would describe different runs of the
  // timer. Host time is therefore discontinuous across a stop/start cycle, as
  // it already was before the epoch existed.
  sli_log_si91x_timer_epoch = 0;

  // Start the timestamp counter from the host
  sl_status_t status = sl_si91x_ulp_timer_init(&ulp_timer_clk_handle);
  if (status != SL_STATUS_OK) {

    return status;
  }
  // Updating timer match-value
  // Configuring timer instance parameters: mode-periodic, type-1us,
  // match-value: 1second
  status = sl_si91x_ulp_timer_set_configuration(&sl_log_timer_handle);
  if (status != SL_STATUS_OK) {

    return status;
  }
  status = sl_si91x_ulp_timer_register_timeout_callback(ULP_TIMER_3, &timer_overflow_callback);
  if (status != SL_STATUS_OK) {

    return status;
  }
  // Starting Timer instance with default parameters
  status = sl_si91x_ulp_timer_start(ULP_TIMER_3);
  if (status != SL_STATUS_OK) {

    return status;
  }
#if !defined(SLI_CAPTIVE_CORE_PRESENT) || (SLI_CAPTIVE_CORE_PRESENT != 1)
  sli_log_si91x_timesync_done = true;
#endif
  return status;
}

/**
 * @brief   Stops the timestamp counter.
 * 
 * @return sl_status_t 
 */
sl_status_t sl_log_hal_stop_timestamp_counter(void)
{
  sl_status_t status = SL_STATUS_OK;

  status = sl_si91x_ulp_timer_stop(ULP_TIMER_3);
  if (status != SL_STATUS_OK) {

    return status;
  }
  status = sl_si91x_ulp_timer_unregister_timeout_callback(ULP_TIMER_3);
  if (status != SL_STATUS_OK) {

    return status;
  }
  return status;
}

/**
 * @brief   Get the current timestamp count
 * 
 * @param Core_id
 * @return uint32_t 
 */
uint32_t sl_log_hal_get_timestamp_count(uint8_t core_id)
{
  switch (core_id) {
    case SL_SI91X_HOST_CORE_ID:
      return sli_log_hal_get_timestamp();

    case SL_SI91X_CAPTIVE_CORE_ID:
#if defined(SLI_CAPTIVE_CORE_PRESENT) && (SLI_CAPTIVE_CORE_PRESENT == 1)
      /*
       * PS2 note: Captive core (NWP) is powered down / unreachable in PS2,
       * so sli_si91x_M4_TA_Timesync() returns a failure status. In that case
       * we fall through and return 0 (no captive-core timestamp is available
       * while the M4 is in PS2). A fresh timestamp can only be obtained after
       * wake-up to PS3/PS4, when the captive core is reachable again and
       * sli_si91x_M4_TA_Timesync() succeeds. Any NWP-originated log records
       * captured during PS2 will therefore carry a 0 timestamp until sync is
       * re-established.
       */
      if (sli_si91x_M4_TA_Timesync() == SL_STATUS_OK) {
        return sl_si91x_cc_timestamp + SL_TIMESYNC_TURNAROUND_TIME;
      }
#endif
      break;
    default:
      break;
  }
  return 0;
}

/**
 * @brief   Get the timestamp epoch (high 32 bits of the 64-bit host time)
 *
 * The captive core reports a bare 32-bit microsecond value over the time-sync
 * exchange with no wrap accounting of its own, so only the host core has an
 * epoch to report.
 *
 * @param core_id
 * @return uint32_t
 */
uint32_t sl_log_hal_get_timestamp_epoch(uint8_t core_id)
{
  if (core_id == SL_SI91X_HOST_CORE_ID) {
    return sli_log_hal_get_epoch();
  }

  return 0;
}

/**
 * @brief   Get the current host timestamp timer frequency
 * 
 * @return uint32_t 
 */
uint32_t sl_log_hal_get_timestamp_timer_frequency(uint8_t core_id)
{
  (void)core_id;
  return SL_SI91X_LOG_TIMER_FREQ;
}

/**
 * @brief   Perform pre-sleep processing for logging
 * 
 * @param config configuration pointer
 * @return sl_status_t  Status of the operation.
 */
sl_status_t sl_log_hal_pre_sleep_process(const void *config)
{
  sl_status_t status = SL_STATUS_OK;
  (void)config;
  sli_log_si91x_timesync_done = false;
  sl_log_api_backend_t *api   = sl_log_get_api_backend();
  api->backend_deinit();
  status = sl_si91x_ulp_timer_stop(ULP_TIMER_3);
  if (status != SL_STATUS_OK) {

    return status;
  }
  status = sl_si91x_ulp_timer_unregister_timeout_callback(ULP_TIMER_3);
  if (status != SL_STATUS_OK) {

    return status;
  }
  return status;
}

/**
 * @brief   Perform post-sleep processing for logging
 * 
 * @param config configuration pointer
 * @return sl_status_t  Status of the operation.
 */
sl_status_t sl_log_hal_post_sleep_process(const void *config)
{
  // Implementation for post-sleep process
  (void)config;

  sl_log_api_backend_t *api = sl_log_get_api_backend();
#ifdef SL_CATALOG_SI91X_LOG_BACKEND_IOSTREAM_PRESENT
  sl_iostream_init_instances_stage_1();
  sl_iostream_init_instances_stage_2();
#endif
  api->backend_init();

  sl_log_hal_start_timestamp_counter();

  sl_log_hal_timer_sync(NULL, SL_SI91X_CAPTIVE_CORE_ID);

  return SL_STATUS_OK;
}
/**
 * @brief   Synchronizes timer for SI91x logging across multiple cores.
 * 
 * @param core_id  core ID of the target core to synchronize timer to
 * @param args  Pointer to additional arguments if any
 * @return sl_status_t  Status of the operation.
 */
sl_status_t sl_log_hal_timer_sync(const void *args, uint8_t core_id)
{
  (void)core_id;
  (void)args;
#if defined(SLI_CAPTIVE_CORE_PRESENT) && (SLI_CAPTIVE_CORE_PRESENT == 1)
  /*
   * PS2 note: This routine relies on sli_si91x_M4_TA_Timesync() to exchange a
   * timestamp with the captive core (NWP). In PS2 the NWP is powered down /
   * unreachable, so the call returns a non-OK status,
   * sli_log_si91x_timestamp_delta is NOT updated,
   * sli_log_si91x_timesync_done stays false, and this function returns
   * SL_STATUS_FAIL.
   *
   * Consequences while in PS2:
   *   - sl_log_hal_get_timestamp_count(SL_SI91X_HOST_CORE_ID) returns 0
   *     because sli_log_si91x_timesync_done is false (host-side log events
   *     captured during PS2 will carry a 0 timestamp and a 0 epoch).
   *   - sl_log_hal_get_timestamp_count(SL_SI91X_CAPTIVE_CORE_ID) also
   *     returns 0 for the same reason (no NWP-side timestamp is available).
   *
   * Re-sync sequence:
   *   - sl_log_hal_pre_sleep_process() clears sli_log_si91x_timesync_done
   *     before sleep.
   *   - sl_log_hal_post_sleep_process() calls this function again after wake.
   *     Once the NWP becomes reachable (i.e. the device is back in PS3/PS4),
   *     sli_si91x_M4_TA_Timesync() succeeds, the delta is recomputed against
   *     the local ULP timer match value, and sli_log_si91x_timesync_done is
   *     set so that subsequent host-core timestamp queries return valid values.
   */
  if (sli_si91x_M4_TA_Timesync() == SL_STATUS_OK) {
    sli_log_si91x_timestamp_delta = (int32_t)(sl_si91x_cc_timestamp + SL_TIMESYNC_TURNAROUND_TIME
                                              - TIMERS->MATCH_CTRL[SL_LOG_ULP_TIMER_INSTANCE].MCUULP_TMR_MATCH);
    sli_log_si91x_timesync_done   = true;
    return SL_STATUS_OK;
  }
#endif
  return SL_STATUS_FAIL;
}

#ifdef SL_WIFI_COMPONENT_INCLUDED
/**
 * @brief   Handles incoming NWP log packets.
 * 
 * @param data pointer to the incoming data buffer
 * @param length length of the incoming data buffer
 */
void sli_handle_nwp_log_packet(const uint8_t *data, uint16_t length)
{
  if (data == NULL || length == 0) {
    return;
  }
  if ((length % sizeof(sli_nwp_log_event_t)) != 0) {
    return;
  }
  uint32_t n = (uint32_t)length / (uint32_t)sizeof(sli_nwp_log_event_t);
  (void)sl_log_write_multiple_to_ring_buffer((const sli_nwp_log_event_t *)data, n);
}
#endif

/**
 * @brief   Get the platform-specific logging configuration.
 * 
 * @param args  Pointer to a platform config structure to fill
 * @param core_id  Core identifier
 * @return sl_status_t 
 */
sl_status_t sl_log_hal_get_configuration(void *args, uint8_t core_id)
{
  (void)args;
  (void)core_id;
  return SL_STATUS_OK;
}
/** */
/**
 * @brief   Set the platform-specific logging configuration.
 * 
 * @param args  Pointer to a platform config structure to fill
 * @param core_id  Core identifier
 * @return sl_status_t 
 */
sl_status_t sl_log_hal_set_configuration(const void *args, uint8_t core_id)
{
  (void)args;
  (void)core_id;
  ulp_timer_config_t sl_log_timer_handle = {
    .timer_num         = ULP_TIMER_3,
    .timer_mode        = ULP_TIMER_MODE_PERIODIC,
    .timer_type        = ULP_TIMER_TYP_1US,
    .timer_match_value = 0xFFFFFFFF,
    .timer_direction   = UP_COUNTER,
  };

  // Updating timer match-value
  // Configuring timer instance parameters: mode-periodic, type-1us,
  // match-value: 1second
  sl_status_t status = sl_si91x_ulp_timer_set_configuration(&sl_log_timer_handle);
  if (status != SL_STATUS_OK) {
    return status;
  }
#ifdef SL_CATALOG_SI91X_LOG_BACKEND_IOSTREAM_PRESENT
  sl_iostream_init_instances_stage_1();
  sl_iostream_init_instances_stage_2();
#endif
  sl_log_hal_timer_sync(NULL, SL_SI91X_CAPTIVE_CORE_ID);
  sl_log_api_backend_t *api = sl_log_get_api_backend();
  if (api != NULL) {
    api->backend_deinit();
    api->backend_init();
    return SL_STATUS_OK;
  }

  return SL_STATUS_FAIL;
}
/**
 * @brief Return pointer to the core API structure.
 *
 * Provides the generic logging core with the platform-specific core
 * implementation (init/deinit/timestamp). This function returns a pointer to
 * the statically allocated `sl_log_api_core` structure.
 *
 * @return Pointer to the populated sl_log_api_core_t structure.
 */
sl_log_api_core_t *sl_log_get_api_core(void)
{
  return &sl_log_api_core;
}

#ifdef SL_WIFI_COMPONENT_INCLUDED
/***************************************************************************/ /**
 * @brief Write multiple log events into the ring buffer.
 *
 * Inserts a contiguous block of log events into the internal ring buffer while
 * maintaining correct wraparound, overwrite handling, and IRQ-safe updates to
 * buffer indices.  
 *
 * This function performs the write in three phases:
 * 1. **Reserve slots and advance write index (IRQ-off):**  
 *    - Atomically reduces `available_event_slots` by `count`.  
 *    - Checks whether the backend is currently busy; if insufficient space is
 *      available and the backend has not yet completed its previous transfer,
 *      the operation is rejected with ::SL_STATUS_NOT_AVAILABLE.  
 *    - Updates `write_index` with wraparound.
 *
 * 2. **Copy events into ring buffer (outside IRQ-off):**  
 *    - Performs one or two memcpy operations depending on wraparound.  
 *
 * 3. **Finalize event_count and adjust read_index (IRQ-off):**  
 *    - Updates `event_count`, clamping to the buffer's maximum size.  
 *    - If the write overwrote existing events, advances `read_index`
 *      appropriately.  
 *
 * @param[in] events  
 *   Pointer to an array of @ref sli_nwp_log_event_t (NWP wire format); each entry
 *   is converted into @ref sl_log_event_t in the ring buffer.
 *
 * @param[in] count  
 *   Number of events to write. Must be non-zero.
 *
 * @return sl_status_t  
 *   - ::SL_STATUS_OK  
 *       Successfully wrote all events into the ring buffer.  
 *   - ::SL_STATUS_INVALID_PARAMETER  
 *       `events` is NULL or `count` is zero.  
 *   - ::SL_STATUS_NOT_AVAILABLE  
 *       Not enough space is available and the backend transfer is still in
 *       progress, preventing safe overwrite accounting.  
 *
 * @note  
 * - This function is intended for internal logger operations where multiple
 *   events are written efficiently.  
 * - Overwrite behavior adheres to the standard ring buffer model: the oldest
 *   unread events are discarded first when capacity is exceeded.  
 * - Callers must ensure `events` points to valid and populated event data.
 *
 * @internal  
 * Uses critical sections (`__disable_irq` / `__enable_irq`) to guarantee that
 * write and read indices are updated atomically relative to ISR-based backends.
 ******************************************************************************/

sl_status_t sl_log_write_multiple_to_ring_buffer(const sli_nwp_log_event_t *events, uint32_t count)
{
  if (events == NULL || count == 0) {
    return SL_STATUS_INVALID_PARAMETER;
  }
#if defined(SL_LOG_CONFIG_MODE) && (SL_LOG_CONFIG_MODE == SL_LOG_CONFIG_MODE_HOST)
  sl_log_ring_buffer_t *ring = sl_log_get_ring_buffer_config();
  const uint32_t N           = SL_LOG_NUMBER_OF_EVENTS;

  // ---- 1) Reserve slots and advance write_index (IRQ-off) ----
  __disable_irq();

  int32_t new_available = ring->available_event_slots - (int32_t)count;

  // Always reduce permanently, even if not available (for overflow tracking)
  ring->available_event_slots = new_available;

  // If backend busy and we went negative, reject
  if ((new_available < 0) && (sl_log_backend_status.backend_transfer_done == 0)) {
    __enable_irq();
    return SL_STATUS_NOT_AVAILABLE;
  }

  uint32_t start_index = ring->write_index;
  uint32_t new_write   = start_index + count;
  if (new_write >= N) {
    new_write -= N;
  }
  ring->write_index = new_write;

  __enable_irq();

  // ---- 2) Copy NWP wire events into ring as sl_log_event_t (handles wrap) ----
  uint32_t ring_idx = start_index;
  for (uint32_t i = 0; i < count; i++) {
    sli_sl_log_event_from_nwp(&events[i], &ring->buffer[ring_idx]);
    ring_idx++;
    if (ring_idx >= N) {
      ring_idx = 0;
    }
  }

  // ---- 3) Update event_count/read_index under IRQ-off ----
  __disable_irq();

  uint32_t old_count   = ring->event_count;
  uint32_t new_count   = old_count + count;
  uint32_t overwritten = 0;

  if (new_count > N) {
    overwritten = new_count - N;
    new_count   = N;
  }
  ring->event_count = new_count;

  if (overwritten > 0) {
    uint32_t new_read = ring->read_index + overwritten;
    if (new_read >= N) {
      new_read -= N;
    }
    ring->read_index = new_read;
  }

  __enable_irq();
#endif
/*
 * Forward NWP-side events into the active M4 backend only when that backend ingests
 * NWP wire-format records:
 *   - SystemView: NWP events are surfaced on the SystemView trace alongside M4 events.
 *   - IO Stream Compact over RTT: NWP records are pulled into the M4 ring buffer for
 *     compact RTT streaming.
 * The Formatted backend cannot render NWP events (format strings live in M4 .log_fmt,
 * unreachable from NWP), and Compact over non-RTT transports does not currently consume
 * NWP events, so this path is compiled out for those builds.
 */
#if (defined(SL_CATALOG_LOG_BACKEND_SYSTEMVIEW_PRESENT)                 \
     || (defined(SL_CATALOG_SI91X_LOG_BACKEND_IOSTREAM_COMPACT_PRESENT) \
         && defined(SL_CATALOG_IOSTREAM_RTT_SI91X_PRESENT)))
  sl_log_event_t buffer;
  sl_log_api_backend_t *sl_log_backend_api = sl_log_get_api_backend();
  for (uint32_t i = 0; i < count; i++) {
    sli_sl_log_event_from_nwp(&events[i], &buffer);
    sl_log_backend_api->backend_write((sl_log_event_t *)&buffer, 0, 1);
  }
#endif
  return SL_STATUS_OK;
}
#endif
