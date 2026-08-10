#ifndef SLI_UTILITY_H
#define SLI_UTILITY_H

#include <stdint.h>
#include "sl_status.h"
#include "sl_types.h"
#include "sli_command_engine.h"

#ifndef __ZEPHYR__
#include "sli_cmsis_os2_ext_task_register.h"
#include "cmsis_os2.h"

// For all the threads this is the index of the thread local array at which the firmware status will be stored.
extern sli_task_register_id_t sli_fw_status_storage_index;
#endif

#ifndef SLI_FW_STATUS_STORAGE_INVALID_INDEX
#define SLI_FW_STATUS_STORAGE_INVALID_INDEX 0xFF
#endif

/***************************************************************************/ /**
 * @brief
 *   Retrieve data from a buffer with a specified offset.
 *
 * @details
 *   This function is designed to retrieve data from a buffer at a specified offset.
 *
 * @param[in]  buffer
 *   A pointer to an [sl_wifi_buffer_t](../wiseconnect-api-reference-guide-wi-fi/sl-wifi-buffer-t) structure from which data is to be retrieved.
 * 
 * @param[in]  offset
 *   Offset from the start of the buffer where data retrieval begins.
 * 
 * @param[out] data_length
 *   Pointer to a variable where the remaining data length from the offset will be stored.
 *
 * @return
 *   Pointer to the data at the specified offset within the buffer.
 *
 ******************************************************************************/
void *sli_wifi_host_get_buffer_data(void *buffer, uint16_t offset, uint16_t *data_length);

uint8_t sli_get_command_packet_type(sli_wifi_command_type_t command_type);

sl_status_t sli_wifi_set_command_engine_instance(sli_command_engine_t *instance);

sl_status_t sli_wifi_send_command(uint32_t command,
                                  sli_wifi_command_type_t command_type,
                                  const void *data,
                                  uint32_t data_length,
                                  sli_wifi_wait_period_t wait_period,
                                  void *sdk_context,
                                  void **data_buffer);

/**
 * @brief Internal function to send a command packet to Command Engine
 *
 * @param command The command to be sent.
 * @param command_type The type of the command.
 * @param packet Pointer to the sl_wifi_system_packet_t containing the command data.
 * @param wait_period The wait period for the command response.
 * @param sdk_context Pointer to the SDK context.
 * @param response_buffer Pointer to the buffer where the response will be stored.
 * @return sl_status_t Status of the operation.
 */
sl_status_t sli_wifi_send_command_packet(uint32_t command,
                                         sli_wifi_command_type_t command_type,
                                         sl_wifi_system_packet_t *packet,
                                         sli_wifi_wait_period_t wait_period,
                                         void *sdk_context,
                                         void **response_buffer);

sl_status_t sli_wifi_receive_response_buffer(uint16_t command_packet_type,
                                             uint16_t packet_id,
                                             sli_wifi_wait_period_t wait_time,
                                             uint8_t wait_type,
                                             void **response_packet);

/***************************************************************************/ /**
 * @brief
 *   Block until a synchronous command-engine response is dequeued.
 *
 * @details
 *   Waits for an RX response on the sync-response queue registered for
 *   @p command_packet_type, then removes and returns the metadata node whose
 *   @c tx_info.packet_id matches @p packet_id. Spurious wakeups (event set but
 *   no matching node) are retried until @p wait_period_ms elapses.
 *
 *   Two wait mechanisms are supported via @p wait_type:
 *   - @ref SLI_WIFI_WAIT_ON_THREAD_ID: blocks on the calling thread's CMSIS
 *     thread flags. Used when the thread that sent the command waits for its
 *     own response.
 *   - @ref SLI_WIFI_WAIT_ON_EVENT_ID: blocks on the per-packet-type shared
 *     @c osEventFlags handle (@c sync_response_event_id). Used when the waiter
 *     may differ from the sender (for example socket read/select).
 *
 *   On success the caller owns @p metadata_response and must free it (and any
 *   attached @c tx_info.data_packet) when no longer needed. Higher-level callers
 *   typically use sli_wifi_receive_response_buffer() instead of calling this
 *   function directly.
 *
 * @param[in] command_packet_type
 *   Command-engine packet type used to look up the sync-response queue and events.
 * @param[in] packet_id
 *   TX packet identifier to match in the response queue. Use @c 0 when the
 *   response is not correlated by packet ID (for example async socket reads).
 * @param[in] wait_period_ms
 *   Maximum time to wait for a matching response, in milliseconds.
 * @param[in] wait_type
 *   Wait mechanism: @ref SLI_WIFI_WAIT_ON_THREAD_ID or @ref SLI_WIFI_WAIT_ON_EVENT_ID.
 * @param[out] metadata_response
 *   On @c SL_STATUS_OK, set to the dequeued command-engine metadata. Unchanged on error.
 *
 * @return
 *   @c SL_STATUS_OK on success.
 *   @c SL_STATUS_TIMEOUT if no matching response arrives within @p wait_period_ms.
 *   @c SL_STATUS_INVALID_PARAMETER if @p metadata_response is NULL or @p wait_type is invalid.
 *   @c SL_STATUS_INVALID_CONFIGURATION if @ref SLI_WIFI_WAIT_ON_EVENT_ID is requested but
 *   the packet type has no @c sync_response_event_id configured.
 *   @c SL_STATUS_FAIL on an unexpected RTOS wait error.
 *   Other queue-manager errors may be propagated on the thread-ID wait path.
 ******************************************************************************/
sl_status_t sli_wifi_driver_wait_for_response_packet(uint16_t command_packet_type,
                                                     uint16_t packet_id,
                                                     uint32_t wait_period_ms,
                                                     uint8_t wait_type,
                                                     sli_command_engine_metadata_t **metadata_response);

/***************************************************************************/ /**
 * @brief
 *   Calculates the elapsed time since a given starting timestamp.
 * 
 * @details
 *   This function calculates the difference between the current timestamp and a provided starting timestamp. It is useful for measuring the time elapsed during operations.
 * 
 * @param[in] starting_timestamp
 *   The starting timestamp from which the elapsed time is calculated.
 * 
 * @return
 *   The elapsed time in milliseconds of type uint32_t.
 ******************************************************************************/
uint32_t sli_wifi_host_elapsed_time(uint32_t starting_timestamp);

/******************************************************************************
 * @brief
 * 	A utility function that store the firmware status code in thread specific storage.
 * @param[in] converted_firmware_status
 *	Firmware status code that needs to be saved.
 *****************************************************************************/
static inline void sli_wifi_save_firmware_status(sl_status_t converted_firmware_status)
{
#ifndef __ZEPHYR__
  sli_osTaskRegisterSetValue(NULL, sli_fw_status_storage_index, converted_firmware_status);
#endif
}

/******************************************************************************
  * @brief
  *   A utility function that converts frame status sent by firmware to sl_status_t and stores in thread local storage of caller thread.
  * @param[in] firmware_status
  *   firmware_status that needs to be converted to sl_status_t.
  * @return
  *   sl_status_t. See https://docs.silabs.com/gecko-platform/latest/platform-common/status for details.
  *****************************************************************************/
static inline sl_status_t sli_wifi_convert_and_save_firmware_status(uint16_t firmware_status)
{
  sl_status_t converted_firmware_status = (firmware_status == SL_STATUS_OK) ? SL_STATUS_OK
                                                                            : (firmware_status | (1U << 16));
#ifndef __ZEPHYR__
  sli_wifi_save_firmware_status(converted_firmware_status);
#endif
  return converted_firmware_status;
}

#endif // SLI_UTILITY_H
