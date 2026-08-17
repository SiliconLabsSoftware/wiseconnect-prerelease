#ifndef SLI_POWER_PROFILE_H
#define SLI_POWER_PROFILE_H

#include "sl_types.h"
#include "sli_types.h"

/* Function used to set the bluetooth performance profile */
void sli_save_bt_current_performance_profile(const sl_bt_performance_profile_t *profile);

/* Function used to retrieve bluetooth performance profile */
void sli_get_bt_current_performance_profile(sl_bt_performance_profile_t *profile);

/* Function used to retrieve the coex performance profile */
void sli_get_coex_performance_profile(sl_wifi_system_performance_profile_t *profile);

/* Function used to zero out the coex performance profile */
void sli_reset_coex_current_performance_profile(void);

void sli_wifi_save_current_performance_profile(const sl_wifi_performance_profile_v2_t *profile);

/* Function used to get current wifi performance profile */
void sli_wifi_get_current_performance_profile(sl_wifi_performance_profile_v2_t *profile);

/* Function used to retrieve the coex mode */
sl_wifi_system_coex_mode_t sli_get_coex_mode(void);

/* Function used to update the coex mode */
void sli_save_coex_mode(sl_wifi_system_coex_mode_t coex_mode);

/* Function used to zero out the coex performance profile */
void sli_wifi_reset_coex_current_performance_profile(void);

void sli_convert_performance_profile_to_power_save_command(sl_wifi_system_performance_profile_t profile,
                                                           sli_wifi_power_save_request_t *power_save_request);

#endif // SLI_POWER_PROFILE_H
