#include <stddef.h>
#include <string.h>
#include "sli_utility.h"
#include "sl_types.h"
#include "sli_command_engine.h"
#include "sl_cmsis_utility.h"
#include "sl_constants.h"
#include "sl_core.h"

static sli_command_engine_t *sli_wifi_command_engine_instance = NULL;

__WEAK uint8_t sli_get_command_packet_type(sli_wifi_command_type_t command_type)
{
  UNUSED_PARAMETER(command_type);
  return 0;
}

sl_status_t sli_wifi_set_command_engine_instance(sli_command_engine_t *instance)
{
  sli_wifi_command_engine_instance = instance;
  return SL_STATUS_OK;
}

// Calculate elapsed time from the given starting timestamp
uint32_t sli_wifi_host_elapsed_time(uint32_t starting_timestamp)
{
  return (osKernelGetTickCount() - starting_timestamp);
}

/**
 * @brief Get data pointer from buffer at specified offset
 * 
 * @param buffer Pointer to Wi-Fi buffer
 * @param offset Offset into the buffer data
 * @param data_length Pointer to store remaining data length (optional, can be NULL)
 * @return Pointer to buffer data at specified offset, NULL if invalid buffer or offset
 */
void *sli_wifi_host_get_buffer_data(void *buffer, uint16_t offset, uint16_t *data_length)
{
  if (NULL == buffer) {
    return NULL;
  }

  sl_wifi_buffer_t *temp_buffer = (sl_wifi_buffer_t *)buffer;
  if ((0 != temp_buffer->length) && (offset >= temp_buffer->length)) {
    return NULL;
  }

  if (data_length) {
    *data_length = (uint16_t)(temp_buffer->length) - offset;
  }

  return (void *)&temp_buffer->data[offset];
}

static bool rx_packet_identity_handler(const sli_queue_t *handle, const void *data, const void *node_match_data)
{
  UNUSED_PARAMETER(handle);
  uint16_t *packet_id                     = (uint16_t *)node_match_data;
  sli_command_engine_metadata_t *metadata = (sli_command_engine_metadata_t *)data;

  SL_DEBUG_LOG_V2(DEBUG,
                  "Comparing expected packetID : %u with packetId of queue node : %u..!\n",
                  *packet_id,
                  metadata->tx_info.packet_id);
  if (*packet_id == metadata->tx_info.packet_id) {
    return true;
  }

  return false;
}

sl_status_t sli_wifi_driver_wait_for_response_packet(uint16_t command_packet_type,
                                                     uint16_t packet_id,
                                                     uint32_t wait_period_ms,
                                                     uint8_t wait_type,
                                                     sli_command_engine_metadata_t **metadata_response)
{
  // Check that metadata_response is a valid pointer
  SL_VERIFY_POINTER_OR_RETURN(metadata_response, SL_STATUS_INVALID_PARAMETER);

  uint32_t events                                                 = 0;
  sli_command_engine_metadata_t *buffer                           = NULL;
  sli_command_engine_packet_type_configuration_t packet_type_info = { 0 };
  sl_status_t status                                              = SL_STATUS_OK;

  // Get packet type configuration for the given command type
  status = sli_command_engine_get_rx_queue_info_from_packet_type(sli_wifi_command_engine_instance,
                                                                 command_packet_type,
                                                                 &packet_type_info);
  VERIFY_STATUS_AND_RETURN(status);

  uint32_t start_time_ticks     = osKernelGetTickCount();
  uint32_t elapsed_time_ticks   = 0;
  uint32_t remaining_time_ticks = 0;
  uint32_t wait_period_ticks    = (wait_period_ms == osWaitForever) ? osWaitForever
                                                                    : SLI_SYSTEM_MS_TO_TICKS(wait_period_ms);
  if (wait_type == SLI_WIFI_WAIT_ON_THREAD_ID) {
    // Wait for thread event flags and retry dequeue until matching response or timeout.
    do {
      remaining_time_ticks = (elapsed_time_ticks > wait_period_ticks) ? 0 : (wait_period_ticks - elapsed_time_ticks);
      SL_DEBUG_LOG_V2(DEBUG,
                      "Waiting on Thread Events: 0x%lX on event id : 0x%X for queue 0x%X\n",
                      packet_type_info.sync_response_event,
                      (unsigned int)packet_type_info.sync_response_event_id,
                      (unsigned int)packet_type_info.sync_response_queue);
      events = osThreadFlagsWait(packet_type_info.sync_response_event, osFlagsWaitAny, remaining_time_ticks);
      SL_DEBUG_LOG_V2(DEBUG,
                      "Got Thread Events: 0x%lX for queue 0x%X\n",
                      events,
                      (unsigned int)packet_type_info.sync_response_queue);
      if (events == (uint32_t)osErrorTimeout || events == (uint32_t)osErrorResource) {
        // Timeout or resource error
        return SL_STATUS_TIMEOUT;
      } else if ((packet_type_info.sync_response_event & events) == packet_type_info.sync_response_event) {
        // Remove the node with the matching packet_id from the queue
        status = sli_queue_manager_remove_node_from_queue(packet_type_info.sync_response_queue,
                                                          rx_packet_identity_handler,
                                                          (const void *)&packet_id,
                                                          (void **)metadata_response);
        if (status == SL_STATUS_OK) {
          return SL_STATUS_OK;
        }
        if ((status != SL_STATUS_EMPTY) && (status != SL_STATUS_NOT_FOUND)) {
          VERIFY_STATUS_AND_RETURN(status);
        }
        if (wait_period_ms != osWaitForever) {
          elapsed_time_ticks = sli_wifi_host_elapsed_time(start_time_ticks);
        }
      } else {
        // Other error
        return SL_STATUS_FAIL;
      }
    } while (elapsed_time_ticks < wait_period_ticks);
  } else if (wait_type == SLI_WIFI_WAIT_ON_EVENT_ID) {
    // sync_response_event_id must be a pointer to the event flags handle (see sli_command_engine.h)
    if (packet_type_info.sync_response_event_id == NULL) {
      return SL_STATUS_INVALID_CONFIGURATION;
    }

    do {
      remaining_time_ticks = (elapsed_time_ticks > wait_period_ticks) ? 0 : (wait_period_ticks - elapsed_time_ticks);
      SL_DEBUG_LOG_V2(DEBUG,
                      "Waiting on Events: 0x%lX on event id : 0x%X for queue 0x%X\n",
                      packet_type_info.sync_response_event,
                      (unsigned int)packet_type_info.sync_response_event_id,
                      (unsigned int)packet_type_info.sync_response_queue);

      events = osEventFlagsWait(*packet_type_info.sync_response_event_id,
                                packet_type_info.sync_response_event,
                                (osFlagsWaitAny | osFlagsNoClear),
                                remaining_time_ticks);

      if (events == (uint32_t)osErrorTimeout || events == (uint32_t)osErrorResource) {
        // Timeout or resource error
        return SL_STATUS_TIMEOUT;
      }

      SL_DEBUG_LOG_V2(DEBUG,
                      "Got Events: 0x%lX for queue 0x%X\n",
                      events,
                      (unsigned int)packet_type_info.sync_response_queue);

      // Enter atomic section to safely access the queue
      CORE_irqState_t state = CORE_EnterAtomic();

      // Remove the node with the matching packet_id from the queue
      status = sli_queue_manager_remove_node_from_queue(packet_type_info.sync_response_queue,
                                                        rx_packet_identity_handler,
                                                        (const void *)&packet_id,
                                                        (void **)&buffer);

      if (status == SL_STATUS_OK) {
        // If the queue is empty after removal, clear the event flag
        if (SLI_QUEUE_MANAGER_IS_QUEUE_EMPTY(packet_type_info.sync_response_queue)) {
          osEventFlagsClear(*packet_type_info.sync_response_event_id, packet_type_info.sync_response_event);
        }
        CORE_ExitAtomic(state);
        *metadata_response = buffer;
        return SL_STATUS_OK;
      } else if (status == SL_STATUS_EMPTY) {
        // If the queue is empty, clear the event flag
        osEventFlagsClear(*packet_type_info.sync_response_event_id, packet_type_info.sync_response_event);
      }
      CORE_ExitAtomic(state);
      if (status == SL_STATUS_NOT_FOUND) {
        // Add a small delay to avoid busy waiting
        osDelay(SLI_SYSTEM_MS_TO_TICKS(2));
      }

      // Update elapsed time
      if (wait_period_ms != osWaitForever) {
        elapsed_time_ticks = sli_wifi_host_elapsed_time(start_time_ticks);
      }
    } while (elapsed_time_ticks < wait_period_ticks);

  } else {
    return SL_STATUS_INVALID_PARAMETER;
  }

  return SL_STATUS_TIMEOUT;
}

sl_status_t sli_wifi_receive_response_buffer(uint16_t command_packet_type,
                                             uint16_t packet_id,
                                             sli_wifi_wait_period_t wait_period,
                                             uint8_t wait_type,
                                             void **response_buffer)
{

  sli_wifi_wait_period_t wait_time         = 0;
  sl_wifi_system_packet_t *response_packet = NULL;
  uint16_t firmware_status                 = 0;
  sl_status_t status                       = 0;

  // Calculate the wait time based on wait_period
  if ((wait_period & SLI_WIFI_WAIT_FOR_EVER) == SLI_WIFI_WAIT_FOR_EVER) {
    wait_time = osWaitForever;
  } else {
    wait_time = (wait_period & ~SLI_WIFI_WAIT_FOR_RESPONSE_BIT);
  }
  sli_command_engine_metadata_t *metadata_response = NULL;

  // Wait for a response packet and handle it
  status = sli_wifi_driver_wait_for_response_packet(command_packet_type,
                                                    packet_id,
                                                    (uint32_t)wait_time,
                                                    wait_type,
                                                    &metadata_response);
  VERIFY_STATUS_AND_RETURN(status);

  if (metadata_response == NULL) {
    return SL_STATUS_FAIL;
  }
  firmware_status = metadata_response->packet_status;
  // Check if a data packet is present in the response metadata
  if (metadata_response->tx_info.data_packet != NULL) {
    response_packet =
      (sl_wifi_system_packet_t *)sli_wifi_host_get_buffer_data(metadata_response->tx_info.data_packet, 0, NULL);

    // If the response_buffer pointer is not NULL and response packet flag is set, assign the response packet to response_buffer
    if (NULL != response_buffer && (SLI_WIFI_WAIT_FOR_RESPONSE_BIT == (wait_period & SLI_WIFI_WAIT_FOR_RESPONSE_BIT))) {
      // Clear Firmware Queue info from data length parameter
      response_packet->length = (response_packet->length & 0x0FFF);
      *response_buffer        = metadata_response->tx_info.data_packet;
    } else {
      // Free the data packet buffer if not needed
      sli_buffer_manager_free_buffer((sli_buffer_t)metadata_response->tx_info.data_packet);
    }
  }

  sli_buffer_manager_free_buffer((sli_buffer_t)metadata_response);
  return sli_wifi_convert_and_save_firmware_status(firmware_status);
}

sl_status_t sli_wifi_send_command_packet(uint32_t command,
                                         sli_wifi_command_type_t command_type,
                                         sl_wifi_system_packet_t *packet,
                                         sli_wifi_wait_period_t wait_period,
                                         void *sdk_context,
                                         void **response_buffer)
{
  sli_command_engine_tx_info_t tx_info = { 0 };
  sl_status_t status                   = SL_STATUS_OK;

  if (command_type < SLI_SI91X_CMD_MAX) {
    packet->desc[1] |= (SLI_WLAN_MGMT_Q << 4);
    tx_info.packet_type = sli_get_command_packet_type(command_type);
  } else {
    tx_info.packet_type = command_type;
  }

  tx_info.data_packet        = (void *)packet;
  tx_info.data_packet_length = ((packet->length & 0xFFF) + sizeof(sl_wifi_system_packet_t));
  tx_info.frame_id           = (uint16_t)command;
  tx_info.flags              = SLI_COMMAND_ENGINE_COMMAND_PACKET;
  tx_info.timeout            = 0;
  tx_info.context            = sdk_context;
  tx_info.packet_id          = 0;

  // Check the wait_period to determine the flags for packet handling
  if (wait_period == SLI_WIFI_RETURN_IMMEDIATELY) {
    // If wait_period indicates an immediate return, set flags to 0
    tx_info.flags |= SLI_COMMAND_ENGINE_SEQ_ASYNC_RESPONSE_PACKET;
  } else {
    // Expect a sync status response from the command engine
    tx_info.flags |= SLI_COMMAND_ENGINE_SYNC_RESPONSE_STATUS_PACKET;
    // Also expect response data when a response buffer is provided
    if (response_buffer != NULL) {
      tx_info.flags |= SLI_COMMAND_ENGINE_SYNC_RESPONSE_DATA_PACKET;
    }
  }

  // Check the command type and set the flags accordingly
  switch (command) {
    case SLI_WIFI_REQ_PWRMODE:
    case SLI_WIFI_REQ_OPERMODE:
    case SLI_COMMON_RSP_SOFT_RESET:
    case SLI_COMMON_RSP_ENABLE_DISABLE_BLE:
      tx_info.flags |= SLI_COMMAND_ENGINE_REQUEST_WITH_GLOBAL_TX_BLOCK;
      break;
    default:
      break;
  }

  if (!(tx_info.flags & SLI_COMMAND_ENGINE_SEQ_ASYNC_RESPONSE_PACKET)) {
    // Calculate the wait time based on wait_period
    if ((wait_period & SLI_WIFI_WAIT_FOR_EVER) == SLI_WIFI_WAIT_FOR_EVER) {
      tx_info.timeout = (uint32_t)osWaitForever;
    } else {
      // Store timeout in kernel ticks so command-engine timeout comparisons remain unit-consistent.
      tx_info.timeout = SLI_SYSTEM_MS_TO_TICKS((uint32_t)(wait_period & ~SLI_WIFI_WAIT_FOR_RESPONSE_BIT));
    }
  }

  status = sli_command_engine_send_packet(sli_wifi_command_engine_instance, &tx_info);
  if (status != SL_STATUS_OK) {
    sli_buffer_manager_free_buffer(packet);
  }
  VERIFY_STATUS_AND_RETURN(status);

  // Check if the command should return immediately or wait for a response
  if (wait_period == SLI_WIFI_RETURN_IMMEDIATELY) {
    return SL_STATUS_IN_PROGRESS;
  }

  return sli_wifi_receive_response_buffer(tx_info.packet_type,
                                          tx_info.packet_id,
                                          wait_period,
                                          SLI_WIFI_WAIT_ON_THREAD_ID,
                                          response_buffer);
}

sl_status_t sli_wifi_send_command(uint32_t command,
                                  sli_wifi_command_type_t command_type,
                                  const void *data,
                                  uint32_t data_length,
                                  sli_wifi_wait_period_t wait_period,
                                  void *sdk_context,
                                  void **response_buffer)
{
  sl_wifi_system_packet_t *packet = NULL;
  sl_status_t status              = SL_STATUS_OK;

  // Allocate a buffer for the command with appropriate size
  status = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_CMD_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_DEDICATED,
                                              SLI_WIFI_ALLOCATE_COMMAND_BUFFER_WAIT_TIME,
                                              (sli_buffer_t)&packet);
  VERIFY_STATUS_AND_RETURN(status);

  // Clear the packet descriptor and copy the command data if available
  memset(packet->desc, 0, sizeof(packet->desc));
  if (data != NULL) {
    memcpy(packet->data, data, data_length);
  }

  // Fill frame type
  packet->length  = data_length & 0xFFF;
  packet->command = (uint16_t)command;

  return sli_wifi_send_command_packet(command, command_type, packet, wait_period, sdk_context, response_buffer);
}
