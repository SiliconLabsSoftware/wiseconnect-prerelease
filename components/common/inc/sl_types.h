#ifndef SL_TYPES_H
#define SL_TYPES_H

#include "sl_slist.h"
#include "sl_constants.h"
#include "sli_constants.h"

// driver TX/RX packet structure
/// Wi-Fi packet structure
typedef struct {
  union {
    struct {
      uint16_t length;  ///< Length of data
      uint16_t command; ///< command type
      uint8_t unused
        [12]; ///< Contains command status and other additional information. Unused for TX and only used for RX packets.
    };
    uint8_t desc[SL_SI91X_WIFI_PACKET_DESC_SIZE]; ///< packet header
  };                                              ///< Command header

  uint8_t data[]; ///< Data to be transmitted or received
} sl_wifi_system_packet_t;

/** \addtogroup SL_SI91X_CONSTANTS
  * @{ */
/// Si91x performance profile
#define HIGH_PERFORMANCE SL_WIFI_SYSTEM_HIGH_PERFORMANCE ///< Power save is disabled and throughput is maximum.
#define ASSOCIATED_POWER_SAVE \
  SL_WIFI_SYSTEM_ASSOCIATED_POWER_SAVE ///< Low power profile when the device is associated with an AP (MAX PSP).
#define ASSOCIATED_POWER_SAVE_LOW_LATENCY \
  SL_WIFI_SYSTEM_ASSOCIATED_POWER_SAVE_LOW_LATENCY ///< Low power profile when the device is associated with an AP (FAST PSP). If SL_WIFI_ENABLE_ENHANCED_MAX_PSP bit is set in config_feature_bit_map, then this mode enables the Enhanced Max PSP feature.
#define DEEP_SLEEP_WITHOUT_RAM_RETENTION \
  SL_WIFI_SYSTEM_DEEP_SLEEP_WITHOUT_RAM_RETENTION ///< Deep Sleep without RAM retention when the device is not associated with AP.
#define DEEP_SLEEP_WITH_RAM_RETENTION \
  SL_WIFI_SYSTEM_DEEP_SLEEP_WITH_RAM_RETENTION ///< Deep Sleep with RAM retention when the device is not associated with AP.

/** @} */

/** \addtogroup SL_SI91X_TYPES
 * @{ */

/**
 * @enum sl_wifi_system_performance_profile_t
 * @brief Performance profile
 */
typedef enum {
  SL_WIFI_SYSTEM_HIGH_PERFORMANCE,      ///< Power save is disabled and throughput is maximum.
  SL_WIFI_SYSTEM_ASSOCIATED_POWER_SAVE, ///< Low power profile when the device is associated with an AP (MAX PSP).
  SL_WIFI_SYSTEM_ASSOCIATED_POWER_SAVE_LOW_LATENCY, ///< Low power profile when the device is associated with an AP (FAST PSP).
  SL_WIFI_SYSTEM_DEEP_SLEEP_WITHOUT_RAM_RETENTION, ///< Deep Sleep without RAM Retention when the device is not associated with AP.
  SL_WIFI_SYSTEM_DEEP_SLEEP_WITH_RAM_RETENTION ///< Deep Sleep with RAM Retention when the device is not associated with AP.
} sl_wifi_system_performance_profile_t;

/// Bluetooth performance profile
typedef struct {
  sl_wifi_system_performance_profile_t
    profile; ///< Performance profile of type [sl_wifi_system_performance_profile_t](../wiseconnect-api-reference-guide-wi-fi/sl-wifi-types#sl-wifi-system-performance-profile-t).
} sl_bt_performance_profile_t;
/** @} */

/** \addtogroup SL_SI91X_CONSTANTS
  * @{ */
/// Si91x performance profile
typedef sl_wifi_system_performance_profile_t SL_DEPRECATED_API_WISECONNECT_4_0 sl_si91x_performance_profile_t;

/** @} */

/** @addtogroup SL_WIFI_TYPES Types
  * @{ */

/**
 * @struct sl_wifi_buffer_t
 * @brief Structure representing a Wi-Fi buffer.
 */
typedef struct {
  sl_slist_node_t node; ///< Pointer to the node of the list of which the buffer is part of
  uint32_t length;      ///< Size of the buffer in bytes
  uint8_t
    type; ///< Indicates the buffer type (SL_WIFI_TX_FRAME_BUFFER, SL_WIFI_RX_FRAME_BUFFER, and so on.) corresponding to the buffer.
  uint8_t id;           ///< Buffer identifier. Can be used to uniquely identify a buffer. Loops every 256 packets.
  uint8_t _reserved[2]; ///< Reserved.
  uint8_t data[];       ///< Stores the data (header + payload) to be send to NWP
} sl_wifi_buffer_t;

/**
 * @struct sl_wifi_twt_request_t
 * @brief TWT (Target Wake Time) request structure to configure a session.
 */
typedef struct {
  uint8_t wake_duration; ///< Nominal minimum wake duration. Range : 0 - 255
  uint8_t
    wake_duration_tol; ///< Tolerance allowed for wake duration in case of suggest TWT. Received TWT wake duration from AP validates against tolerance limits and decides if TWT config received is in acceptable range or not. Range : 0 - 255.
  uint8_t wake_int_exp; ///< Wake interval exponent to the base 2. Range : 0 - 31.
  uint8_t
    wake_int_exp_tol; ///< Tolerance allowed for wake_int_exp in case of suggest TWT request. Received TWT wake interval exponent from AP validates against tolerance limits and decides if TWT config received is in acceptable range or not. Range : 0 - 31.
  uint16_t wake_int_mantissa; ///< Wake interval mantissa. Range : 0 - 65535.
  uint16_t
    wake_int_mantissa_tol; ///< Tolerance allowed for wake_int_mantissa in case of suggest TWT. Received TWT wake interval mantissa from AP validates against tolerance limits and decides if TWT config received is in acceptable range or not. Range : 0 - 65535.
  uint8_t
    implicit_twt; ///< If enabled (1), the TWT requests STA to calculate the next TWT by adding a fixed value to the current TWT value. Currently, explicit TWT is not allowed.
  uint8_t
    un_announced_twt; ///< If enabled (1), the TWT requests STA to not announce its wake up to AP through PS-POLLs or UAPSD Trigger frames. Values : 0 or 1.
  uint8_t
    triggered_twt; ///< If enabled(1), at least one trigger frame is included in the TWT Service Period(TSP). Values : 0 or 1.
  uint8_t negotiation_type; ///< Negotiation type : 0 - Individual TWT; 1 - Broadcast TWT.
  uint8_t twt_channel;      ///< Currently this configuration is not supported. Range : 0 - 7.
  uint8_t
    twt_protection; ///< If enabled (1), TSP is protected. This is negotiable with AP. Currently this is not supported. Values : 0 or 1.
  uint8_t twt_flow_id; ///< TWT session flow id. 0 - 7 valid. 0xFF to disable all active TWT sessions.
  uint8_t
    restrict_tx_outside_tsp;  ///< 1 - Any Tx outside the TSP is restricted. 0 - TX can happen outside the TSP also.
  uint8_t twt_retry_limit;    ///< TWT retry limit. Range : 0 - 15.
  uint8_t twt_retry_interval; ///< TWT retry interval in seconds between two twt requests. Range : 5 - 255.
  uint8_t req_type;           ///< TWT request type. 0 - Request TWT; 1 - Suggest TWT; 2 - Demand TWT.
  uint8_t twt_enable;         ///< TWT enable. 0 - TWT session teardown; 1 - TWT session setup.
  uint8_t wake_duration_unit; ///< Wake duration unit. 0 - 256 microseconds ; 1 - 1024 microseconds.
} sl_wifi_twt_request_t;

/**
 * @struct sl_wifi_twt_selection_t
 * @brief TWT (Target Wake Time) request structure to auto select a session.
 */
typedef struct {
  uint8_t twt_enable; ///< TWT enable. 0 - TWT session teardown; 1 - TWT session setup.
  uint16_t
    average_tx_throughput; ///< This is the expected average Tx throughput in Kbps. Value ranges from 0 to 10 Mbps, which is half of the default [device_average_throughput](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-wi-fi/sl-wifi-twt-selection-t#device-average-throughput) (20 Mbps by default).
  uint32_t
    tx_latency; ///< The allowed latency, in milliseconds, within which the given Tx operation is expected to be completed. If 0 is configured, maximum allowed Tx latency is same as rx_latency. Otherwise, valid values are in the range of [200 msec - 6 hrs].
  uint32_t
    rx_latency; ///< The maximum latency, in milliseconds, for receiving buffered packets from the AP. The device wakes up at least once for a TWT service period within the configured rx_latency if there are any pending packets destined for the device from the AP. If set to 0, the default latency of 2 seconds is used. Valid range is between 2 seconds to 6 hours. Recommended range is 2 seconds to 60 seconds to avoid connection failures with AP due to longer sleep time.
  uint16_t
    device_average_throughput; ///< Refers to the average Tx throughput that the device is capable of achieving in Kbps. The default value is 20 Mbps. Internal SDK use only: do not use.
  uint8_t
    estimated_extra_wake_duration_percent; ///< The percentage by which wake duration is supposed to be overestimated to compensate for bss congestion. Recommended input range is 0 - 50%. The default value is 0. Internal SDK use only: do not use.
  uint8_t
    twt_tolerable_deviation; ///< The allowed deviation percentage of wake duration TWT response. Recommended input range is 0 - 50%. The default value is 10. Internal SDK use only: do not use.
  uint32_t
    default_wake_interval_ms; ///< Default minimum wake interval. Recommended Range: 512 to 1024 msec. The default value is 1024 msec. Internal SDK use only: do not use.
  uint32_t
    default_minimum_wake_duration_ms; ///< Default minimum wake interval. Recommended Range: 8- 16 msec. The default value is 8 msec. Internal SDK use only: do not use.
  uint8_t
    beacon_wake_up_count_after_sp; ///< The number of beacons after the service period completion for which the module wakes up and listens for any pending RX. The default value is 2. Internal SDK use only: do not use.
} sl_wifi_twt_selection_t;

/**
 * @struct sl_wifi_twt_selection_v2_t
 * @brief TWT (Target Wake Time) auto-selection configuration. Use this structure with @ref sl_wifi_target_wake_time_auto_selection_v2.
 *        Only these four parameters are configurable; all other TWT parameters are set internally by the SDK.
 */
typedef struct {
  uint8_t twt_enable; ///< TWT enable. 0 - TWT session teardown; 1 - TWT session setup.
  uint16_t
    average_tx_throughput; ///< Expected average Tx throughput in Kbps. Range: 0 to 10 Mbps (half of default device capability).
  uint32_t
    tx_latency; ///< Allowed Tx latency in milliseconds. If 0, maximum Tx latency equals rx_latency. Valid range: 200 ms - 6 hrs.
  uint32_t
    rx_latency; ///< Maximum latency in ms for receiving buffered packets from AP. If 0, default 2 s is used. Valid range: 2 s - 6 hrs. Recommended: 2 s - 60 s.
} sl_wifi_twt_selection_v2_t;

/// Wi-Fi performance profile v2
typedef struct {
  sl_wifi_system_performance_profile_t
    profile; ///< Performance profile of type [sl_wifi_system_performance_profile_t](../wiseconnect-api-reference-guide-wi-fi/sl-wifi-types#sl-wifi-system-performance-profile-t).
  uint8_t dtim_aligned_type; ///< Set DTIM alignment required. One of the values from @ref WIFI_DTIM_ALIGNMENT_TYPES.
  uint8_t num_of_dtim_skip;  ///< Number of DTIM intervals to skip. Default value is 0.
  uint32_t listen_interval;  ///< Listen interval in milliseconds.
  uint16_t
    monitor_interval; ///< Monitor interval in milliseconds. Default interval 50 milliseconds is used if monitor_interval is set to 0. This is only valid when performance profile is set to ASSOCIATED_POWER_SAVE_LOW_LATENCY.
  sl_wifi_twt_request_t twt_request; ///< Target Wake Time (TWT) request settings.
  union {
    sl_wifi_twt_selection_t
      twt_selection; ///< @deprecated Use twt_selection_v2 instead. Target Wake Time (TWT) selection request settings.
    sl_wifi_twt_selection_v2_t twt_selection_v2; ///< Target Wake Time (TWT) selection request settings.
  };
  uint8_t
    beacon_miss_ignore_limit; ///< Number of consecutive missed beacons that can be ignored while the device remains in sleep mode. If the number of beacon misses exceeds this limit and the beacon is still not received, the device will wake up to listen for the beacon. The default value is 1. Recommended range: 1 - 10. Values beyond 10 might lead to interoperability issues.
} sl_wifi_performance_profile_v2_t;

/**
 * @enum sl_wifi_system_coex_mode_t
 * @brief Wireless co-existence mode
 * @note Only BLE, WLAN, and WLAN + BLE modes are supported.
 */
typedef enum {
  SL_WIFI_SYSTEM_WLAN_ONLY_MODE      = 0, ///< Wireless local area network (WLAN) only mode
  SL_WIFI_SYSTEM_WLAN_MODE           = 1, ///< WLAN mode (not currently supported)
  SL_WIFI_SYSTEM_BLUETOOTH_MODE      = 4, ///< Bluetooth only mode (not currently supported)
  SL_WIFI_SYSTEM_WLAN_BLUETOOTH_MODE = 5, ///< WLAN and Bluetooth mode (not currently supported)
  SL_WIFI_SYSTEM_DUAL_MODE           = 8, ///< Dual mode (not currently supported)
  SL_WIFI_SYSTEM_WLAN_DUAL_MODE      = 9, ///< WLAN dual mode (not currently supported)
  SL_WIFI_SYSTEM_BLE_MODE      = 12, ///< Bluetooth Low Energy (BLE) only mode, used when power save mode is not needed.
  SL_WIFI_SYSTEM_WLAN_BLE_MODE = 13, ///< WLAN and BLE mode
  __SL_WIFI_FORCE_COEX_ENUM_16BIT = 0xFFFF ///< Force the enumeration to be 16-bit
} sl_wifi_system_coex_mode_t;

/// Si91x wireless co-existence mode
/// @note Only BLE, WLAN, and WLAN + BLE modes are supported.
typedef sl_wifi_system_coex_mode_t SL_DEPRECATED_API_WISECONNECT_4_0 sl_si91x_coex_mode_t;

/** @} */

#endif // SL_TYPES_H
