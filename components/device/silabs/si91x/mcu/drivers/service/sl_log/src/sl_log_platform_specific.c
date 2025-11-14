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
#include "sl_si91x_ulp_timer.h"
#define SL_SI91X_HOST_CORE_ID     0       // Host core identifier
#define SL_SI91X_LOG_TIMER_FREQ   1000000 // 1 MHz timer frequency for microsecond resolution
#define SL_SI91X_CAPTIVE_CORE_ID  1       // Captive core identifier
#define SL_LOG_ULP_TIMER_INSTANCE 3       //  Ulp timer instance using for logger

#if defined(SLI_CAPTIVE_CORE_PRESENT) && (SLI_CAPTIVE_CORE_PRESENT == 1)
#include "sl_utility.h"
#include "sl_si91x_constants.h"
#include "sl_si91x_driver.h"
#include "rsi_m4.h"
#define SL_TIMESYNC_TURNAROUND_TIME 15      // Turn around time for timesync request from Captive core(TA) in us
#define M4_REQ_TIME_STAMP_FROM_NWP  BIT(12) // M4 request to NWP for timestamp
#define SL_SI91X_CC_COMMAND_TIMEOUT 10000   // 10 ms Timeout for command wait loop
uint32_t sl_si91x_cc_timestamp = 0;         //Variable holding captive core timestamp
uint32_t sl_si91x_log_host_timesync_address =
  (uint32_t)&sl_si91x_cc_timestamp; //Variable holding address of Captive core timestamp
#endif

/*******************************************************************************
 *                               GLOBAL VARIABLES
 ******************************************************************************/
int si91x_timestamp_delta;

/**
 * @brief Write multiple log events into the platform ring buffer.
 *
 * This inline helper writes `length` events from `data` into the global
 * ring buffer. The implementation is responsible for handling wrap-around
 * and basic concurrency model (interrupt disable/enable). Callers should
 * ensure `data` points to at least `length * sizeof(sl_log_event_t)` bytes.
 *
 * @param[in] data   Pointer to the input events to write
 * @param[in] length Number of events to write
 * @return SL_STATUS_OK on success, SL_STATUS_ALLOCATION_FAILED on insufficient space
 */
static inline sl_status_t sl_log_si91x_burst_write_to_ring_buffer(sl_log_event_t *data, uint16_t length);

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
sl_status_t sl_log_hal_timer_sync(uint8_t core_id, void *args);

/**
 * @brief Prepare logging subsystem before entering sleep.
 *
 * Typical actions include stopping timers and deinitializing backend
 * transports to reduce power consumption.
 *
 * @param[in] config Pointer to logging configuration
 * @return SL_STATUS_OK on success or an sl_status_t error code
 */
sl_status_t sl_log_hal_pre_sleep_process(void *config);

/**
 * @brief Restore logging subsystem after wake-up from sleep.
 *
 * Reinitialize backend transports and restart the timestamp counter.
 *
 * @param[in] config Pointer to logging configuration
 * @return SL_STATUS_OK on success or an sl_status_t error code
 */
sl_status_t sl_log_hal_post_sleep_process(void *config);

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
sl_status_t sl_log_hal_set_configuration(void *args, uint8_t core_id);

/**
 * @brief   Core API structure.
 * 
 */
sl_log_api_core_t sl_log_api_core = { .platform_core_init            = sl_log_hal_start_timestamp_counter,
                                      .platform_core_deinit          = sl_log_hal_stop_timestamp_counter,
                                      .get_timestamp                 = sl_log_hal_get_timestamp_count,
                                      .get_timestamp_timer_frequency = sl_log_hal_get_timestamp_timer_frequency,
                                      .post_sleep_process            = sl_log_hal_post_sleep_process,
                                      .pre_sleep_process             = sl_log_hal_pre_sleep_process,
                                      .set_configuration             = sl_log_hal_set_configuration,
                                      .get_configuration             = sl_log_hal_get_configuration };

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
  static uint32_t overflow_count = 0;
  SL_PRINT_STRING_INFO("Timer overflow count %u", overflow_count++);
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
  if (core_id == SL_SI91X_HOST_CORE_ID) {
    return (TIMERS->MATCH_CTRL[SL_LOG_ULP_TIMER_INSTANCE].MCUULP_TMR_MATCH + si91x_timestamp_delta);
  } else if (core_id == SL_SI91X_CAPTIVE_CORE_ID) {
#if defined(SLI_CAPTIVE_CORE_PRESENT) && (SLI_CAPTIVE_CORE_PRESENT == 1)
    volatile uint32_t current_count = TIMERS->MATCH_CTRL[SL_LOG_ULP_TIMER_INSTANCE].MCUULP_TMR_MATCH;
    P2P_STATUS_REG |= M4_WAKEUP_TA;

    while (!((P2P_STATUS_REG & TA_IS_ACTIVE)
             && ((TIMERS->MATCH_CTRL[SL_LOG_ULP_TIMER_INSTANCE].MCUULP_TMR_MATCH - current_count)
                 < SL_SI91X_CC_COMMAND_TIMEOUT)))
      ; // Wait for NWP
    if ((TIMERS->MATCH_CTRL[SL_LOG_ULP_TIMER_INSTANCE].MCUULP_TMR_MATCH - current_count)
        > SL_SI91X_CC_COMMAND_TIMEOUT) {
      return SL_STATUS_TIMEOUT;
    }

    M4SS_P2P_INTR_SET_REG = M4_REQ_TIME_STAMP_FROM_NWP;

    current_count = TIMERS->MATCH_CTRL[SL_LOG_ULP_TIMER_INSTANCE].MCUULP_TMR_MATCH;
    while (!((TASS_P2P_INTR_CLEAR_REG & M4_REQ_TIME_STAMP_FROM_NWP)
             && ((TIMERS->MATCH_CTRL[SL_LOG_ULP_TIMER_INSTANCE].MCUULP_TMR_MATCH - current_count)
                 < SL_SI91X_CC_COMMAND_TIMEOUT)))
      ; // Wait for NWP ACK
    if ((TIMERS->MATCH_CTRL[SL_LOG_ULP_TIMER_INSTANCE].MCUULP_TMR_MATCH - current_count)
        > SL_SI91X_CC_COMMAND_TIMEOUT) {
      return SL_STATUS_TIMEOUT;
    }

    clear_ta_to_m4_interrupt(M4_REQ_TIME_STAMP_FROM_NWP);

    sl_si91x_host_clear_sleep_indicator();
    return sl_si91x_cc_timestamp + SL_TIMESYNC_TURNAROUND_TIME;
#endif
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
sl_status_t sl_log_hal_pre_sleep_process(void *config)
{
  sl_status_t status = SL_STATUS_OK;
  (void)config;
  sl_log_api_backend_t *api = sl_log_get_api_backend();
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
sl_status_t sl_log_hal_post_sleep_process(void *config)
{
  // Implementation for post-sleep process
  (void)config;

  sl_log_api_backend_t *api = sl_log_get_api_backend();
  api->backend_init();
  sl_log_hal_start_timestamp_counter();
  sl_log_hal_timer_sync(SL_SI91X_CAPTIVE_CORE_ID, NULL);
  return SL_STATUS_OK;
}
/**
 * @brief   Synchronizes timer for SI91x logging across multiple cores.
 * 
 * @param core_id  core ID of the target core to synchronize timer to
 * @param args  Pointer to additional arguments if any
 * @return sl_status_t  Status of the operation.
 */
sl_status_t sl_log_hal_timer_sync(uint8_t core_id, void *args)
{
  (void)core_id;
  (void)args;
#if defined(SLI_CAPTIVE_CORE_PRESENT) && (SLI_CAPTIVE_CORE_PRESENT == 1)
  volatile uint32_t current_count = TIMERS->MATCH_CTRL[SL_LOG_ULP_TIMER_INSTANCE].MCUULP_TMR_MATCH;
  P2P_STATUS_REG |= M4_WAKEUP_TA;

  while (!((P2P_STATUS_REG & TA_IS_ACTIVE)
           && ((TIMERS->MATCH_CTRL[SL_LOG_ULP_TIMER_INSTANCE].MCUULP_TMR_MATCH - current_count)
               < SL_SI91X_CC_COMMAND_TIMEOUT)))
    ; // Wait for NWP
  if ((TIMERS->MATCH_CTRL[SL_LOG_ULP_TIMER_INSTANCE].MCUULP_TMR_MATCH - current_count) > SL_SI91X_CC_COMMAND_TIMEOUT) {
    return SL_STATUS_TIMEOUT;
  }
  M4SS_P2P_INTR_SET_REG = M4_REQ_TIME_STAMP_FROM_NWP;
  current_count         = TIMERS->MATCH_CTRL[SL_LOG_ULP_TIMER_INSTANCE].MCUULP_TMR_MATCH;
  while (!((TASS_P2P_INTR_CLEAR_REG & M4_REQ_TIME_STAMP_FROM_NWP)
           && ((TIMERS->MATCH_CTRL[SL_LOG_ULP_TIMER_INSTANCE].MCUULP_TMR_MATCH - current_count)
               < SL_SI91X_CC_COMMAND_TIMEOUT)))
    ; // Wait for NWP ACK
  if ((TIMERS->MATCH_CTRL[SL_LOG_ULP_TIMER_INSTANCE].MCUULP_TMR_MATCH - current_count) > SL_SI91X_CC_COMMAND_TIMEOUT) {
    return SL_STATUS_TIMEOUT;
  }
  clear_ta_to_m4_interrupt(M4_REQ_TIME_STAMP_FROM_NWP);

  sl_si91x_host_clear_sleep_indicator();
  si91x_timestamp_delta = sl_si91x_cc_timestamp + SL_TIMESYNC_TURNAROUND_TIME
                          - TIMERS->MATCH_CTRL[SL_LOG_ULP_TIMER_INSTANCE].MCUULP_TMR_MATCH;
#endif
  return SL_STATUS_OK;
}

/**
 * @brief   Handles incoming NWP log packets.
 * 
 * @param data pointer to the incoming data buffer
 * @param length length of the incoming data buffer
 */
void sli_handle_nwp_log_packet(const uint8_t *data, uint16_t length)
{
  sl_log_si91x_burst_write_to_ring_buffer((sl_log_event_t *)data, length / sizeof(sl_log_event_t));
}

/**
 * @brief   Writes a burst of log events to the ring buffer.
 * 
 * @param data  Pointer to the log event data.
 * @param length  Number of log events to write.
 * @return sl_status_t 
 */
sl_status_t sl_log_si91x_burst_write_to_ring_buffer(sl_log_event_t *data, uint16_t length)
{
  /* Validate input parameters: data must be non-NULL and length within bounds */
  if (!data || length == 0 || length > SL_LOG_NUMBER_OF_EVENTS) { /* invalid args */
    return SL_STATUS_INVALID_PARAMETER;                           /* return error for invalid parameters */
  }

  /* Get pointer to the global ring buffer configuration */
  sl_log_ring_buffer_t *sl_log_ring_buffer_ptr = sl_log_get_ring_buffer_config();

  /* Enter critical section by disabling interrupts on this core */
  __disable_irq();

  /* Read the current write index from the ring buffer (where producers append) */
  uint32_t write_index = sl_log_ring_buffer_ptr->write_index; /* snapshot */

  /* Read the current number of events already in the buffer */
  uint32_t event_count = sl_log_ring_buffer_ptr->event_count; /* snapshot */

  /* Compute free space available in the ring buffer */
  uint32_t free_space = SL_LOG_NUMBER_OF_EVENTS - event_count; /* available slots */

  /* If there isn't enough space for the incoming events, bail out */
  if (free_space < length) {
    __enable_irq();                     /* leave critical section before returning */
    return SL_STATUS_ALLOCATION_FAILED; /* not enough room */
  }

  /* Handle wrap-around when writing past the end of the circular buffer */
  if ((write_index + length) >= SL_LOG_NUMBER_OF_EVENTS) {
    /* Compute number of events that fit until the physical buffer end */
    uint16_t first_chunk_size = SL_LOG_NUMBER_OF_EVENTS - write_index; /* events before wrap */

    /* Reserve the new write index after the wrap (length - first_chunk_size) */
    sl_log_ring_buffer_ptr->write_index = (length - first_chunk_size); /* update index for consumer */

    __enable_irq(); /* leave critical section before performing copies */

    /* Copy first chunk (from write_index to buffer end) */
    memcpy(&sl_log_ring_buffer_ptr->sl_log_buffer[write_index],
           data,
           first_chunk_size * sizeof(sl_log_event_t)); /* copy first segment */

    /* Copy remaining events to the start of the buffer (after wrap) */
    memcpy(&sl_log_ring_buffer_ptr->sl_log_buffer[0],
           &data[first_chunk_size],
           (length - first_chunk_size) * sizeof(sl_log_event_t)); /* copy second segment */

  } else {
    /* No wrap: advance write_index by length to reserve space */
    sl_log_ring_buffer_ptr->write_index += length; /* reserve contiguous region */

    __enable_irq(); /* leave critical section before copying data */

    /* Copy the events into the reserved region */
    memcpy(&sl_log_ring_buffer_ptr->sl_log_buffer[write_index],
           data,
           length * sizeof(sl_log_event_t)); /* single contiguous copy */
  }
  /* Re-enter critical section to atomically update the event_count */
  __disable_irq();
  sl_log_ring_buffer_ptr->event_count += length; /* update stored event count */
  __enable_irq();                                /* leave critical section */

  return SL_STATUS_OK; /* success */
}

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
sl_status_t sl_log_hal_set_configuration(void *args, uint8_t core_id)
{
  (void)args;
  (void)core_id;
  return SL_STATUS_OK;
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