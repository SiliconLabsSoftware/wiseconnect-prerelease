/*
 * EVALUATION AND USE OF THIS SOFTWARE IS SUBJECT TO THE TERMS AND
 * CONDITIONS OF THE CONTROLLING LICENSE AGREEMENT FOUND AT LICENSE.md
 * IN THIS SDK. IF YOU DO NOT AGREE TO THE LICENSE TERMS AND CONDITIONS,
 * PLEASE RETURN ALL SOURCE FILES TO SILICON LABORATORIES.
 * (c) Copyright 2018, Silicon Laboratories Inc.  All rights reserved.
 */

/** @file
 *
 * File Description
 *
 */
#ifndef SLI_SI91X_MCU_INTERFACE
#include "sl_component_catalog.h"
#include "sl_iostream.h"
#include "sl_iostream_handles.h"
#if defined(SL_CATALOG_IOSTREAM_USART_PRESENT) || defined(SL_CATALOG_IOSTREAM_EUSART_PRESENT)
#define SLI_WIRELESS_TEST_NCP_VCOM_CLI_PRESENT 1
#elif defined(__has_include)
#if __has_include("sl_iostream_init_usart_instances.h") || __has_include("sl_iostream_init_eusart_instances.h")
#define SLI_WIRELESS_TEST_NCP_VCOM_CLI_PRESENT 1
#endif
#endif
#endif
#include "console.h"
#include "sl_constants.h"
#include "sl_status.h"
#include "sl_board_configuration.h"
#include "cmsis_os2.h"
#include "sl_utility.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
/******************************************************
 *                    Constants
 ******************************************************/

#define BUFFER_SIZE 256
bool end_of_cmd = false;

#define MY_ARG_TYPE uart, spi, i2c, tcp

#define OFFSETS(a, b, c, d) uint8_t offsets[] = { 0, sizeof(#a), sizeof(#b), sizeof(#c) }

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/

sl_status_t help_command_handler(console_args_t *arguments);
sl_status_t rtt_command_handler(console_args_t *arguments);
extern void cache_uart_rx_data(const char character);

static void print_command_args(const console_descriptive_command_t *command);

extern sl_status_t process_buffer_line(const console_database_t *command_database,
                                       console_args_t *args,
                                       const console_descriptive_command_t **command);

void application_start(const void *unused);
void print_status(sl_status_t status, uint32_t duration);
extern sl_status_t sl_board_enable_vcom(void);

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

const osThreadAttr_t thread_attributes = {
  .name       = "application_thread",
  .attr_bits  = 0,
  .cb_mem     = 0,
  .cb_size    = 0,
  .stack_mem  = 0,
  .stack_size = 3072,
  .priority   = 0,
  .tz_module  = 0,
};

void app_init(void)
{
#ifndef SLI_SI91X_MCU_INTERFACE
  // Force the log backend (log_backend_iostream_formatted) to write to RTT
  // instead of vcom. sl_iostream_set_console_instance() picks UART over RTT by
  // priority; without this override, SL_DEBUG_LOG_V2 output would land on vcom
  // and corrupt the HCI byte stream.
  extern sl_iostream_t *sl_iostream_recommended_console_stream;
  extern sl_iostream_t *sl_iostream_rtt_handle;
  sl_iostream_recommended_console_stream = sl_iostream_rtt_handle;
#endif
  osThreadNew((osThreadFunc_t)application_start, NULL, &thread_attributes);
}

#if defined(SLI_WIRELESS_TEST_NCP_VCOM_CLI_PRESENT)
void iostream_usart_init()
{
  /* Prevent buffering of output/input.*/
#if !defined(__CROSSWORKS_ARM) && defined(__GNUC__)
  setvbuf(stdout, NULL, _IONBF, 0); /*Set unbuffered mode for stdout (newlib)*/
  setvbuf(stdin, NULL, _IONBF, 0);  /*Set unbuffered mode for stdin (newlib)*/
#endif
}

void iostream_rx()
{
  char c               = 0;
  static uint8_t index = 0;
  // Read HCI bytes ONLY from the vcom UART, not from the "default" / "recommended
  // console" stream. This decouples the HCI pipe from the log pipe so we can
  // route SL_DEBUG_LOG_V2 output to RTT without corrupting HCI on vcom.
  size_t total = 0;
  size_t len   = 1;
  while (total < len) {
    size_t got         = 0;
    sl_status_t retval = sl_iostream_read(sl_iostream_vcom_handle, &c, len - total, &got);
    if (retval == SL_STATUS_OK) {
      total += got;
    } else if (retval != SL_STATUS_EMPTY) {
      return;
    }
    if (c > 0) {
      cache_uart_rx_data(c);
      if (c == '\n') {
        index      = 0;
        end_of_cmd = true;
      } else {
        if (index < BUFFER_SIZE - 1) {
          index++;
        }
      }
    }
  }
}
#endif // SLI_WIRELESS_TEST_NCP_VCOM_CLI_PRESENT

void application_start(const void *unused)
{
  UNUSED_PARAMETER(unused);
  console_args_t args;
  const console_descriptive_command_t *command;
#if defined(SLI_WIRELESS_TEST_NCP_VCOM_CLI_PRESENT)
  iostream_usart_init();
#else
  /* SoC: unbuffered stdout so Ready/help/status appear promptly on VCOM/Docklight. */
#if !defined(__CROSSWORKS_ARM) && defined(__GNUC__)
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stdin, NULL, _IONBF, 0);
#endif
#endif

  SL_DEBUG_LOG_V2(INFO, "app start\r\n");

  // CLI UX must use printf/UART (VCOM/Docklight). SL_DEBUG_LOG_V2 is backed by
  // si91x_log + iostream_rtt_si91x on SoC, so Ready/help/status would not appear
  // on the serial terminal used by SQA (see Everest wireless_test and at_commands).
  printf("Ready\r\n");

  console_line_ready = 0;

  while (1) {
    printf("\r\n> \r\n");
#if defined(SLI_WIRELESS_TEST_NCP_VCOM_CLI_PRESENT)
    while (!end_of_cmd) {
      iostream_rx();
    }
    end_of_cmd = false;
#endif
    while (!console_line_ready) {
      console_process_uart_data();
      osDelay(20);
    }

    sl_status_t result = console_process_buffer(&console_command_database, &args, &command);

    if (result == SL_STATUS_OK) {
      SL_DEBUG_LOG_V2(DEBUG, "Processing command\r\n");
      if (command->handler) {
        printf("\r\n");
        uint32_t start_time = osKernelGetTickCount();
        result              = command->handler(&args);
        uint32_t duration   = osKernelGetTickCount() - start_time;
        print_status(result, duration);
      }
    } else if (result == SL_STATUS_COMMAND_IS_INVALID) {
      printf("\r\nArgs: ");
      print_command_args(command);
      print_status(SL_STATUS_INVALID_PARAMETER, 0);
    } else {
      printf("\r\nNot supported\r\n");
    }
    console_line_ready = 0;
  }
}

void print_status(sl_status_t status, uint32_t duration)
{
  printf("\r\n0x%05" PRIx32 ": (%" PRIu32 "ms) %s\r\n", status, duration, (status == SL_STATUS_OK) ? "Success" : "");
}

sl_status_t help_command_handler(console_args_t *arguments)
{
  UNUSED_PARAMETER(arguments);
  for (uint8_t a = 0; a < console_command_database.length; ++a) {
    const console_descriptive_command_t *cmd =
      (console_descriptive_command_t *)console_command_database.entries[a].value;
    printf("\r\n%s  ", console_command_database.entries[a].key);
    print_command_args(cmd);
    printf("\r\n   %s", cmd->description);
  }
  return SL_STATUS_OK;
}

typedef struct {
  void *next;
  uint8_t stack[];
} console_threaded_command_t;

static console_threaded_command_t *threaded_commands_list_head;
sl_status_t thread_command_handler(console_args_t *arguments)
{
  console_args_t args;
  const console_descriptive_command_t *command;
  uint32_t stack_size = (uint32_t)arguments->arg[0];
  sl_status_t result  = console_parse_command((char *)arguments->arg[1], &console_command_database, &args, &command);

  if (result == SL_STATUS_OK) {
    if (command->handler) {
      // Create thread
      console_threaded_command_t *new_thread = malloc(sizeof(console_threaded_command_t) + stack_size);
      if (new_thread == NULL) {
        return SL_STATUS_ALLOCATION_FAILED;
      }
      memset(new_thread, 0, sizeof(console_threaded_command_t) + stack_size);
      new_thread->next = 0;
      if (threaded_commands_list_head == NULL) {
        threaded_commands_list_head = new_thread;
      } else {
        new_thread->next            = threaded_commands_list_head;
        threaded_commands_list_head = new_thread->next;
      }
      uint32_t start_time = osKernelGetTickCount();
      result              = command->handler(&args);
      uint32_t duration   = osKernelGetTickCount() - start_time;
      print_status(result, duration);
    }
  }
  return result;
}

sl_status_t rtt_command_handler(console_args_t *arguments)
{
  UNUSED_PARAMETER(arguments);
  return SL_STATUS_OK;
}

extern const arg_list_t console_argument_types[];
static void print_command_args(const console_descriptive_command_t *command)
{
  bool is_optional = false;
  for (int a = 0; command->argument_list[a] != CONSOLE_ARG_END; ++a) {
    if (command->argument_list[a] & CONSOLE_ARG_OPTIONAL) {
      char option_char[2] = { (char)command->argument_list[a] & CONSOLE_ARG_OPTIONAL_CHARACTER_MASK, 0 };
      printf("[-%s ", option_char);
      is_optional = true;
      continue;
    } else if (command->argument_list[a] & CONSOLE_ARG_ENUM) {
      printf("{");
      uint8_t enum_index = command->argument_list[a] & CONSOLE_ARG_ENUM_INDEX_MASK;
      for (int b = 0; console_argument_types[enum_index][b] != NULL;
           /* Increment occurs in internal logic */) {
        printf("%s", console_argument_types[enum_index][b]);
        if (console_argument_types[enum_index][++b]) {
          printf("|");
        }
      }
      printf("}");
    } else {
      const char *arg_name = (command->argument_help && command->argument_help[a])
                               ? command->argument_help[a]
                               : console_argument_type_strings[command->argument_list[a] & CONSOLE_ARG_ENUM_INDEX_MASK];
      printf("<%s>", arg_name);
    }
    printf(is_optional ? "] " : " ");
    is_optional = false;
  }
}
