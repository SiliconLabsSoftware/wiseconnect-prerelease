/***************************************************************************/ /**
 * @file  sli_si91x_ahb_bus.c
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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
#include "sli_si91x_ahb_bus.h"
#include "sli_si91x_nwp_interface.h"
#include "sli_buffer_manager.h"
#include "sli_queue_manager.h"
#include "sli_utility.h"
#include "sl_constants.h"
#include "sl_cmsis_utility.h"
#include "rsi_power_save.h"
#include "system_si91x.h"
#include "sli_code_classification.h"
#include <stddef.h>

rsi_m4ta_desc_t tx_desc[2];
rsi_m4ta_desc_t rx_desc[2];
sl_wifi_buffer_t *rx_pkt_buffer = NULL;
osEventFlagsId_t ta_events      = NULL;

static sli_queue_t sli_ahb_bus_rx_queue = { 0 };

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SI91X_WIRELESS, SL_CODE_CLASS_TIME_CRITICAL)
sl_status_t sl_si91x_bus_init(void)
{
  sli_queue_manager_init(&sli_ahb_bus_rx_queue, SLI_BUFFER_MANAGER_QUEUE_NODE_POOL);
  mask_ta_interrupt(TA_RSI_BUFFER_FULL_CLEAR_EVENT);
  return SL_STATUS_OK;
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SI91X_WIRELESS, SL_CODE_CLASS_TIME_CRITICAL)
sl_status_t sli_si91x_bus_read_frame(sl_wifi_buffer_t **buffer)
{
  sl_status_t status = sli_queue_manager_dequeue(&sli_ahb_bus_rx_queue, (void **)buffer);
  VERIFY_STATUS_AND_RETURN(status);

  return SL_STATUS_OK;
}

// Code classification comes from the rsi_m4.h declaration; repeating it here
// would be ignored with -Wattributes (first attribute wins).
void sli_si91x_raise_pkt_pending_interrupt_to_ta(void)
{
  M4SS_P2P_INTR_SET_REG = TX_PKT_PENDING_INTERRUPT;
  osEventFlagsWait(ta_events, SLI_SI91X_TA_PKT_TX_DONE, osFlagsWaitAny, osWaitForever);
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SI91X_WIRELESS, SL_CODE_CLASS_TIME_CRITICAL)
sl_status_t sli_si91x_bus_write_frame(sl_wifi_system_packet_t *packet, const uint8_t *payloadparam, uint16_t size_param)
{
  tx_desc[0].addr   = (M4_MEMORY_OFFSET_ADDRESS + (uint32_t)&packet->desc[0]);
  tx_desc[0].length = 16;

  tx_desc[1].addr   = (M4_MEMORY_OFFSET_ADDRESS + (uint32_t)payloadparam);
  tx_desc[1].length = size_param;

  sli_si91x_raise_pkt_pending_interrupt_to_ta();

  return SL_STATUS_OK;
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SI91X_WIRELESS, SL_CODE_CLASS_TIME_CRITICAL)
sl_status_t sli_si91x_bus_read_interrupt_status(uint16_t *int_status)
{
  *int_status = (uint8_t)HOST_INTR_STATUS_REG;
  return SL_STATUS_OK;
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SI91X_WIRELESS, SL_CODE_CLASS_TIME_CRITICAL)
sl_status_t sl_si91x_bus_deinit(void)
{
  return SL_STATUS_OK;
}

/**
 * @fn          sl_status_t sli_si91x_submit_rx_pkt(uint32_t timeout)
 * @brief       Submit receiver packets
 */
SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SI91X_WIRELESS, SL_CODE_CLASS_TIME_CRITICAL)
sl_status_t sli_si91x_submit_rx_pkt(uint32_t timeout)
{
  sl_status_t status;
  uint16_t data_length = 0;
  sl_wifi_system_packet_t *packet;
  int8_t *pkt_buffer = NULL;

  if (M4SS_P2P_INTR_SET_REG & RX_BUFFER_VALID) {
    return -2;
  }

  status = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_HAL_CMD_DATA_RX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_DEDICATED,
                                              timeout,
                                              (sli_buffer_t *)&rx_pkt_buffer);
  if (status != SL_STATUS_OK) {
    SL_DEBUG_LOG_V2(DEBUG, "\r\n HEAP EXHAUSTED DURING ALLOCATION \r\n");
    return SL_STATUS_ALLOCATION_FAILED;
  }

  packet     = (sl_wifi_system_packet_t *)sli_wifi_host_get_buffer_data(rx_pkt_buffer, 0, &data_length);
  pkt_buffer = (int8_t *)&packet->desc[0];

  rx_desc[0].addr   = (M4_MEMORY_OFFSET_ADDRESS + (uint32_t)pkt_buffer);
  rx_desc[0].length = 16;
  rx_desc[1].addr   = (M4_MEMORY_OFFSET_ADDRESS + (uint32_t)(pkt_buffer + 16));
  rx_desc[1].length = 1600;

  raise_m4_to_ta_interrupt(RX_BUFFER_VALID);

  return SL_STATUS_OK;
}

SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SI91X_WIRELESS, SL_CODE_CLASS_TIME_CRITICAL)
sl_status_t sli_submit_rx_buffer(uint32_t timeout)
{
  sl_status_t status = SL_STATUS_OK;
  mask_ta_interrupt(RX_PKT_TRANSFER_DONE_INTERRUPT);

  status = sli_si91x_submit_rx_pkt(timeout);
  if (status != SL_STATUS_OK) {
    SL_DEBUG_LOG_V2(ERROR, "\r\n RX Buffer submission failed with status: %d \r\n", status);
  }

  unmask_ta_interrupt(RX_PKT_TRANSFER_DONE_INTERRUPT);
  return status;
}

void rsi_update_tx_dma_desc(uint8_t skip_dma_valid)
{
  if (!skip_dma_valid
#ifdef SLI_SI91X_MCU_COMMON_FLASH_MODE
      && !(M4_ULP_SLP_STATUS_REG & MCU_ULP_WAKEUP)
#endif
  ) {
    while (M4_TX_DMA_DESC_REG & DMA_DESC_REG_VALID)
      ;
  }
  M4_TX_DMA_DESC_REG = (uint32_t)&tx_desc;
}

void rsi_update_rx_dma_desc(void)
{
  M4_RX_DMA_DESC_REG = (uint32_t)&rx_desc;
}

void sli_si91x_config_m4_dma_desc_on_reset(void)
{
  while (!(P2P_STATUS_REG & TA_IS_ACTIVE))
    ;
  SL_DEBUG_LOG_V2(INFO, "\r\nTA is in active state\r\n");
  osDelay(SLI_SYSTEM_MS_TO_TICKS(100));
  M4_TX_DMA_DESC_REG = (uint32_t)&tx_desc;
  M4_RX_DMA_DESC_REG = (uint32_t)&rx_desc;
}

void sli_si91x_ta_events_init(void)
{
  if (ta_events == NULL) {
    ta_events = osEventFlagsNew(NULL);
  }
}

// Called from sli_receive_from_ta_done_isr(), which is time-critical, so this
// helper carries the same classification.
SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SI91X_WIRELESS, SL_CODE_CLASS_TIME_CRITICAL)
sl_status_t sli_si91x_ahb_bus_enqueue_rx(void *buffer)
{
  return sli_queue_manager_enqueue(&sli_ahb_bus_rx_queue, buffer);
}
