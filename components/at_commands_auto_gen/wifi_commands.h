#ifndef WIFI_COMMANDS_H
#define WIFI_COMMANDS_H
#include "cmsis_compiler.h"
#include "console_types.h"

extern sl_wifi_device_configuration_t si91x_init_configuration;

__WEAK sl_status_t wifi_peripherals_cnf_axsiga_command_handler(console_args_t *arguments)
{
  UNUSED_PARAMETER(arguments);
  return SL_STATUS_NOT_SUPPORTED;
}

__WEAK sl_status_t wifi_peripherals_cnf_axsigb_command_handler(console_args_t *arguments)
{
  UNUSED_PARAMETER(arguments);
  return SL_STATUS_NOT_SUPPORTED;
}

__WEAK sl_status_t wifi_peripherals_stop_command_handler(console_args_t *arguments)
{
  UNUSED_PARAMETER(arguments);
  return SL_STATUS_NOT_SUPPORTED;
}

__WEAK sl_status_t wifi_statistics_stop_command_handler(console_args_t *arguments)
{
  UNUSED_PARAMETER(arguments);
  return SL_STATUS_NOT_SUPPORTED;
}

__WEAK sl_status_t wifi_statistics_start_command_handler(console_args_t *arguments)
{
  UNUSED_PARAMETER(arguments);
  return SL_STATUS_NOT_SUPPORTED;
}

__WEAK sl_status_t wifi_peripherals_cnf_gen_command_handler(console_args_t *arguments)
{
  UNUSED_PARAMETER(arguments);
  return SL_STATUS_NOT_SUPPORTED;
}

__WEAK sl_status_t wifi_peripherals_start_command_handler(console_args_t *arguments)
{
  UNUSED_PARAMETER(arguments);
  return SL_STATUS_NOT_SUPPORTED;
}

__WEAK sl_status_t wifi_peripherals_cnf_axppdu_command_handler(console_args_t *arguments)
{
  UNUSED_PARAMETER(arguments);
  return SL_STATUS_NOT_SUPPORTED;
}

__WEAK sl_status_t net_info_command_handler(console_args_t *arguments)
{
  UNUSED_PARAMETER(arguments);
  return SL_STATUS_NOT_SUPPORTED;
}
#endif
