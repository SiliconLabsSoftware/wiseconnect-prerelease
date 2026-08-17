#ifndef SLI_TYPES_H
#define SLI_TYPES_H

#include "sl_types.h"

typedef struct {
  sl_wifi_performance_profile_v2_t wifi_performance_profile;
  sl_bt_performance_profile_t bt_performance_profile;
  sl_wifi_system_coex_mode_t coex_mode;
} sli_wifi_performance_profile_t;

/// structure for power save request
typedef struct {
  /// power mode to set
  uint8_t power_mode;

  /// set LP/ULP/ULP-without RAM retention
  uint8_t ulp_mode_enable;

  /// set DTIM aligment required
  // 0 - module wakes up at beacon which is just before or equal to listen_interval
  // 1 - module wakes up at DTIM beacon which is just before or equal to listen_interval
  uint8_t dtim_aligned_type;

  /// Set PSP type, 0-Max PSP, 1- FAST PSP, 2-APSD
  uint8_t psp_type;

  /// Monitor interval for the FAST PSP mode
  // default is 50 ms, and this parameter is valid for FAST PSP only
  uint16_t monitor_interval;
  /// Number of DTIMs to skip
  uint8_t num_of_dtim_skip;
  /// Listen interval
  uint16_t listen_interval;
  /// Wake up for the next beacon if the number of missed beacons exceeds the limit. The default value is 1, with a recommended maximum value of 10. Higher values may cause interoperability issues.
  uint8_t beacon_miss_ignore_limit;
} sli_wifi_power_save_request_t;

#endif // SLI_TYPES_H
