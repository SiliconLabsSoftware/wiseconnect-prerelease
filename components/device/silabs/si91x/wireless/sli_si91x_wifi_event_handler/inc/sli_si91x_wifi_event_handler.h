/***************************************************************************/ /**
 * @file  sl_wifi_constants.h
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef _SLI_SI91X_WIFI_EVENT_HANDLER_H_
#define _SLI_SI91X_WIFI_EVENT_HANDLER_H_

#include <stdint.h>
#include "sl_status.h"
#include "sli_command_engine.h"
#include "sli_event_engine.h"
#include "sli_wifi_command_engine_config.h"

#ifdef SLI_SI91X_OFFLOAD_NETWORK_STACK
#include "sl_si91x_socket_types.h"
#include "sl_ip_types.h"
#endif

extern osEventFlagsId_t sli_wifi_event_engine_event_id;
extern sli_queue_t event_queue[SLI_WIFI_ASYNC_EVENT_HANDLER_MAX_EVENTS];

/**
 * @brief Comparator function type for filtering packets during flush
 * 
 * This matches sli_queue_manager_node_match_handler_t signature so it can be used directly
 * with sli_queue_manager_remove_node_from_queue. The function pointer and user_data are passed
 * separately, just like rx_packet_identity_handler in sli_command_engine.c
 * 
 * @param handle Queue handle (unused)
 * @param data Pointer to command engine metadata (current queue node)
 * @param node_match_data User-defined data for comparison (passed as node_match_data parameter)
 * @return true if packet should be flushed, false otherwise
 */
typedef bool (*sli_flush_packet_compare_function_t)(const sli_queue_t *handle, void *data, const void *node_match_data);

sl_status_t sli_wifi_command_engine_get_packet_metadata(const sli_command_engine_t *instance,
                                                        void *packet,
                                                        sli_command_engine_metadata_t *metadata);

sl_status_t sli_wifi_command_engine_rx_packet_handler(sli_command_engine_t *instance, uint16_t packet_type, void *data);

sl_status_t sli_wifi_command_engine_packet_handler(void *packet,
                                                   uint32_t packet_size,
                                                   sli_routing_utility_packet_status_handler_t packet_status_handler,
                                                   void *context);

sl_status_t sli_wifi_data_packet_handler(void *packet,
                                         uint32_t packet_size,
                                         sli_routing_utility_packet_status_handler_t packet_status_handler,
                                         void *context);

sl_status_t sli_wifi_ble_packet_handler(void *rx_buffer,
                                        uint32_t packet_size,
                                        sli_routing_utility_packet_status_handler_t packet_status_handler,
                                        void *context);

sl_status_t sli_wifi_nwp_log_packet_handler(void *packet,
                                            uint32_t packet_size,
                                            sli_routing_utility_packet_status_handler_t packet_status_handler,
                                            void *context);

sl_status_t sli_wifi_event_engine_init(void);

#ifdef SLI_SI91X_ENABLE_BLE
void sli_ble_send_packet_tx_status(uint16_t packet_type, sl_status_t status, void *context);
#endif

#ifdef SLI_SI91X_OFFLOAD_NETWORK_STACK
/**
 * @brief Flush queues for a specific socket and generate dummy RX packets
 * 
 * This function flushes the queues associated with a specific socket (which are stored as
 * dynamic queues in the command engine). For sync responses, it sets the provided error_status
 * and enqueues metadata to sync response queues with appropriate events. For async commands,
 * packets are simply freed without generating dummy responses.
 * 
 * @param instance Pointer to the command engine instance
 * @param packet_type Packet type of the socket (socket->index + SLI_WIFI_COMMAND_ENGINE_MAX_PACKET_TYPES)
 * @param error_status Error status to set in packet_status for flushed packets
 * @return sl_status_t Status of the operation (SL_STATUS_OK on success, SL_STATUS_NOT_FOUND if socket queue not found)
 */
sl_status_t sli_flush_socket_queues(sli_command_engine_t *instance, uint16_t packet_type, uint16_t error_status);

/**
 * @brief Flush all socket queues and generate dummy RX packets
 * 
 * This function iterates through all sockets and flushes their queues (which are stored as
 * dynamic queues in the command engine). It supports optional filtering by VAP ID and/or
 * destination IP address. For sync responses, it sets the provided error_status and enqueues
 * metadata to sync response queues with appropriate events. For async commands, packets are
 * simply freed without generating dummy responses.
 * 
 * @param instance Pointer to the command engine instance
 * @param error_status Error status to set in packet_status for flushed packets
 * @param vap_id Optional VAP ID filter - if not NULL, only flush sockets matching this VAP ID
 * @param dest_ip_address Optional destination IP address filter - if not NULL, only flush sockets matching this destination IP
 * @return sl_status_t Status of the operation (returns first error encountered, or SL_STATUS_OK if all succeed)
 */
sl_status_t sli_flush_all_socket_queues(sli_command_engine_t *instance,
                                        uint16_t error_status,
                                        const uint8_t *vap_id,
                                        const sl_ip_address_t *dest_ip_address);
#endif

#endif
