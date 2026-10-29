/******************************************************************************
* @file  sl_si91x_gpdma_peripheral_specific.c
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
#include "sl_si91x_gpdma_peripheral_specific.h"

#define SLI_GPDMA_ENTRY(mem, request_line, burst_size)                                                            \
  {                                                                                                               \
    .descriptor_memory = (mem), .resources = NULL, .max_descriptors = (uint16_t)(sizeof(mem) / sizeof((mem)[0])), \
    .channel_id = (request_line), .channel_number = SL_GPDMA_CHANNEL_UNALLOCATED, .burst = (burst_size)           \
  }
#define SLI_GPDMA_UNUSED_ENTRY                                                \
  {                                                                           \
    .descriptor_memory = NULL, .channel_number = SL_GPDMA_CHANNEL_UNALLOCATED \
  }

#if SLI_USART0_GPDMA_EN || SLI_UART0_GPDMA_EN
static sl_si91x_gpdma_descriptor_t usart_gpdma_descriptor_memory_tx[USART_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX];
static sl_si91x_gpdma_descriptor_t usart_gpdma_descriptor_memory_rx[USART_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX];
#endif
#if SLI_UART1_GPDMA_EN
static sl_si91x_gpdma_descriptor_t uart1_gpdma_descriptor_memory_tx[UART1_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX];
static sl_si91x_gpdma_descriptor_t uart1_gpdma_descriptor_memory_rx[UART1_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX];
#endif
#if SLI_GSPI_GPDMA_EN
static sl_si91x_gpdma_descriptor_t gspi_gpdma_descriptor_memory_tx[GSPI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX];
static sl_si91x_gpdma_descriptor_t gspi_gpdma_descriptor_memory_rx[GSPI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX];
#endif
#if SLI_SSI_PRIMARY_GPDMA_EN
static sl_si91x_gpdma_descriptor_t
  ssi_primary_gpdma_descriptor_memory_tx[SSI_PRIMARY_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX];
static sl_si91x_gpdma_descriptor_t
  ssi_primary_gpdma_descriptor_memory_rx[SSI_PRIMARY_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX];
#endif
#if SLI_SSI_SECONDARY_GPDMA_EN
static sl_si91x_gpdma_descriptor_t
  ssi_secondary_gpdma_descriptor_memory_tx[SSI_SECONDARY_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX];
static sl_si91x_gpdma_descriptor_t
  ssi_secondary_gpdma_descriptor_memory_rx[SSI_SECONDARY_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX];
#endif

gpdma_peripheral_channel_info_t gpdma_peripheral_channel_info[SL_MAX_NUMBER_OF_IDS] = {
#if SLI_USART0_GPDMA_EN || SLI_UART0_GPDMA_EN
  [SL_USART_TX_PERIPHERAL_ID] = SLI_GPDMA_ENTRY(usart_gpdma_descriptor_memory_tx,
                                                SL_USART_GPDMA_DESTINATION_CHANNEL_ID,
                                                SL_USART0_GPDMA_DATA_BURST_SIZE_TX),
  [SL_USART_RX_PERIPHERAL_ID] = SLI_GPDMA_ENTRY(usart_gpdma_descriptor_memory_rx,
                                                SL_USART_GPDMA_SOURCE_CHANNEL_ID,
                                                SL_USART0_GPDMA_DATA_BURST_SIZE_RX),
#else
  [SL_USART_TX_PERIPHERAL_ID]         = SLI_GPDMA_UNUSED_ENTRY,
  [SL_USART_RX_PERIPHERAL_ID]         = SLI_GPDMA_UNUSED_ENTRY,
#endif
#if SLI_UART1_GPDMA_EN
  [SL_UART1_TX_PERIPHERAL_ID] = SLI_GPDMA_ENTRY(uart1_gpdma_descriptor_memory_tx,
                                                SL_UART1_GPDMA_DESTINATION_CHANNEL_ID,
                                                SL_UART1_GPDMA_DATA_BURST_SIZE_TX),
  [SL_UART1_RX_PERIPHERAL_ID] = SLI_GPDMA_ENTRY(uart1_gpdma_descriptor_memory_rx,
                                                SL_UART1_GPDMA_SOURCE_CHANNEL_ID,
                                                SL_UART1_GPDMA_DATA_BURST_SIZE_RX),
#else
  [SL_UART1_TX_PERIPHERAL_ID]         = SLI_GPDMA_UNUSED_ENTRY,
  [SL_UART1_RX_PERIPHERAL_ID]         = SLI_GPDMA_UNUSED_ENTRY,
#endif
#if SLI_GSPI_GPDMA_EN
  [SL_GSPI_TX_PERIPHERAL_ID] =
    SLI_GPDMA_ENTRY(gspi_gpdma_descriptor_memory_tx, SL_GSPI_GPDMA_DEST_CHANNEL_ID, SL_GSPI_GPDMA_DATA_BURST_SIZE_TX),
  [SL_GSPI_RX_PERIPHERAL_ID] =
    SLI_GPDMA_ENTRY(gspi_gpdma_descriptor_memory_rx, SL_GSPI_GPDMA_SRC_CHANNEL_ID, SL_GSPI_GPDMA_DATA_BURST_SIZE_RX),
#else
  [SL_GSPI_TX_PERIPHERAL_ID]          = SLI_GPDMA_UNUSED_ENTRY,
  [SL_GSPI_RX_PERIPHERAL_ID]          = SLI_GPDMA_UNUSED_ENTRY,
#endif
#if SLI_SSI_PRIMARY_GPDMA_EN
  [SL_SSI_PRIMARY_TX_PERIPHERAL_ID] = SLI_GPDMA_ENTRY(ssi_primary_gpdma_descriptor_memory_tx,
                                                      SL_SSI_PRIMARY_GPDMA_DEST_CHANNEL_ID,
                                                      SL_SSI_PRIMARY_GPDMA_DATA_BURST_SIZE_TX),
  [SL_SSI_PRIMARY_RX_PERIPHERAL_ID] = SLI_GPDMA_ENTRY(ssi_primary_gpdma_descriptor_memory_rx,
                                                      SL_SSI_PRIMARY_GPDMA_SRC_CHANNEL_ID,
                                                      SL_SSI_PRIMARY_GPDMA_DATA_BURST_SIZE_RX),
#else
  [SL_SSI_PRIMARY_TX_PERIPHERAL_ID]   = SLI_GPDMA_UNUSED_ENTRY,
  [SL_SSI_PRIMARY_RX_PERIPHERAL_ID]   = SLI_GPDMA_UNUSED_ENTRY,
#endif
#if SLI_SSI_SECONDARY_GPDMA_EN
  [SL_SSI_SECONDARY_TX_PERIPHERAL_ID] = SLI_GPDMA_ENTRY(ssi_secondary_gpdma_descriptor_memory_tx,
                                                        SL_SSI_SECONDARY_GPDMA_DEST_CHANNEL_ID,
                                                        SL_SSI_SECONDARY_GPDMA_DATA_BURST_SIZE_TX),
  [SL_SSI_SECONDARY_RX_PERIPHERAL_ID] = SLI_GPDMA_ENTRY(ssi_secondary_gpdma_descriptor_memory_rx,
                                                        SL_SSI_SECONDARY_GPDMA_SRC_CHANNEL_ID,
                                                        SL_SSI_SECONDARY_GPDMA_DATA_BURST_SIZE_RX),
#else
  [SL_SSI_SECONDARY_TX_PERIPHERAL_ID] = SLI_GPDMA_UNUSED_ENTRY,
  [SL_SSI_SECONDARY_RX_PERIPHERAL_ID] = SLI_GPDMA_UNUSED_ENTRY,
#endif
};

#ifdef SLI_USART_GPDMA_LINKED
/***************************************************************************/
/**
 * @brief Get the GPDMA peripheral ID for the USART/UART module.
 * 
 * @details This function returns the GPDMA peripheral ID for the USART/UART module.
 * @note
 *   This function is used to get the GPDMA peripheral ID for the USART/UART module.
 */
sl_gpdma_peripheral_id_t sli_si91x_gpdma_usart_peripheral_id(const void *usart, uint8_t direction)
{
  const USART_RESOURCES *res = (const USART_RESOURCES *)usart;
  if (((res->pREGS == USART0) && SLI_USART0_GPDMA_EN) || ((res->pREGS == UART0) && SLI_UART0_GPDMA_EN)) {
    return (direction == SL_GPDMA_TX_DIRECTION || direction == SL_GPDMA_NO_DIRECTION) ? SL_USART_TX_PERIPHERAL_ID
                                                                                      : SL_USART_RX_PERIPHERAL_ID;
  } //return the TX or RX ID based on the direction ,no direction means its irrelevant TX is returned by default

  if ((res->pREGS == UART1) && SLI_UART1_GPDMA_EN) {
    return (direction == SL_GPDMA_TX_DIRECTION || direction == SL_GPDMA_NO_DIRECTION) ? SL_UART1_TX_PERIPHERAL_ID
                                                                                      : SL_UART1_RX_PERIPHERAL_ID;
  } //return the TX or RX ID based on the direction ,no direction means its irrelevant TX is returned by default
  return INVALID_PERIPHERAL_ID;
}
#endif

#ifdef SLI_SSI_GPDMA_EN
/***************************************************************************/
/**
 * @brief Get the GPDMA peripheral ID for the SSI module.
 * 
 * @details This function returns the GPDMA peripheral ID for the SSI module.
 * @note
 *   This function is used to get the GPDMA peripheral ID for the SSI module.
 */
sl_gpdma_peripheral_id_t sli_si91x_gpdma_ssi_peripheral_id(const SPI_RESOURCES *spi, bool is_tx)
{
  if (spi->reg == SSISlave) {
    return is_tx ? SL_SSI_SECONDARY_TX_PERIPHERAL_ID : SL_SSI_SECONDARY_RX_PERIPHERAL_ID;
  }
  return is_tx ? SL_SSI_PRIMARY_TX_PERIPHERAL_ID : SL_SSI_PRIMARY_RX_PERIPHERAL_ID;
}
#endif

/***************************************************************************/
/**
 * @brief Start one direction. Even IDs transmit, odd IDs receive.
 * 
 * @details This function starts the GPDMA peripheral for the given resource.
 * @note
 *   This function is used to start the GPDMA peripheral for the given resource. */
sl_status_t sli_si91x_gpdma_peripheral_start(void *resources,
                                             sl_gpdma_peripheral_id_t peripheral_id,
                                             const void *src,
                                             const void *dst,
                                             uint32_t num_bytes,
                                             uint8_t data_width_bytes,
                                             sl_gpdma_callback_t transfer_complete_cb)
{
  sl_si91x_gpdma_descriptor_t descriptor = { 0 };
  sl_gpdma_callback_pointer_t callbacks  = { 0 };
  gpdma_peripheral_channel_info_t *info;
  sl_status_t status;
  uint32_t channel_number;
  uint32_t no_of_descriptors;
  uint32_t align;
  uint32_t width_code;
  bool is_rx;

  if ((peripheral_id >= INVALID_PERIPHERAL_ID) || (src == NULL) || (dst == NULL) || (num_bytes == 0U)
      || ((data_width_bytes != 1U) && (data_width_bytes != 2U) && (data_width_bytes != 4U))
      || ((num_bytes % data_width_bytes) != 0U)) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  info  = &gpdma_peripheral_channel_info[peripheral_id];
  is_rx = SL_GPDMA_IS_RX_ID(peripheral_id);

  // Same split as the core builder: each descriptor is a multiple of the
  // item width (and of the burst, when the whole transfer is).
  align             = sli_si91x_gpdma_transfer_alignment(num_bytes, data_width_bytes, info->burst);
  no_of_descriptors = sli_si91x_gpdma_descriptor_count(num_bytes, align);
  if ((info->descriptor_memory == NULL) || (no_of_descriptors == 0U) || (no_of_descriptors > info->max_descriptors)) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  width_code = (data_width_bytes == 4U)   ? SL_GPDMA_DATA_WIDTH_32
               : (data_width_bytes == 2U) ? SL_GPDMA_DATA_WIDTH_16
                                          : SL_GPDMA_DATA_WIDTH_8;

  descriptor.src                              = (void *)src;
  descriptor.dest                             = (void *)dst;
  descriptor.chnlCtrlConfig.transSize         = sli_si91x_gpdma_descriptor_size(num_bytes, align, 0U);
  descriptor.chnlCtrlConfig.transType         = is_rx ? SL_GPDMA_PERIPHERAL_TO_MEMORY : SL_GPDMA_MEMORY_TO_PERIPHERAL;
  descriptor.chnlCtrlConfig.dmaFlwCtrl        = SL_GPDMAC_FL0W_CTRL;
  descriptor.chnlCtrlConfig.mastrIfFetchSel   = is_rx ? 1U : 0U;
  descriptor.chnlCtrlConfig.mastrIfSendSel    = is_rx ? 1U : 0U;
  descriptor.chnlCtrlConfig.linkListMstrSel   = 0U;
  descriptor.chnlCtrlConfig.srcDataWidth      = width_code;
  descriptor.chnlCtrlConfig.destDataWidth     = width_code;
  descriptor.chnlCtrlConfig.linkListOn        = (no_of_descriptors > 1U) ? SL_LINK_LIST_MODE_ENABLE
                                                                         : SL_LINK_LIST_MODE_DISABLE;
  descriptor.chnlCtrlConfig.linkInterrupt     = SL_GPDMA_LINK_LIST_INTERRUPT;
  descriptor.chnlCtrlConfig.retryOnErr        = SL_GPDMA_PERIPHERAL_RETRY_ON_ERROR;
  descriptor.miscChnlCtrlConfig.srcDataBurst  = info->burst;
  descriptor.miscChnlCtrlConfig.destDataBurst = info->burst;
  descriptor.miscChnlCtrlConfig.ahbBurstSize  = SL_GPDMA_AHB_BURST_SIZE;
  descriptor.miscChnlCtrlConfig.srcChannelId  = is_rx ? info->channel_id : 0U;
  descriptor.miscChnlCtrlConfig.destChannelId = is_rx ? 0U : info->channel_id;
  descriptor.miscChnlCtrlConfig.dmaProt       = SL_GPDMA_PERIPHERAL_DMA_PROT;

  channel_number = info->channel_number;
  if (channel_number == SL_GPDMA_CHANNEL_UNALLOCATED) {
    /* The channel is kept for later transfers, so size it for the largest chain, not this one. */
    status = sl_si91x_gpdma_allocate_channel(&channel_number,
                                             is_rx ? 1U : 0U,
                                             (uint32_t)info->max_descriptors * MAX_TRANSFER_PER_DESCRIPTOR);
    if (status != SL_STATUS_OK) {
      return status;
    }
    info->channel_number = (uint8_t)channel_number;
  }
  info->resources                = resources;
  callbacks.transfer_complete_cb = transfer_complete_cb;
  status                         = sl_si91x_gpdma_register_callbacks(channel_number, &callbacks);
  if (status != SL_STATUS_OK) {
    return status;
  }
  status = sl_si91x_gpdma_build_descriptor(info->descriptor_memory, &descriptor, num_bytes, channel_number);
  if (status != SL_STATUS_OK) {
    return status;
  }
  return sl_si91x_gpdma_transfer(channel_number, (void *)src, (void *)dst);
}

/***************************************************************************/
/** @brief Abort one direction. No-op if its channel was never allocated.
 * @param peripheral_id The peripheral ID to stop.
 * @return SL_STATUS_OK if the peripheral is stopped successfully, otherwise an error code.
 * @note
 *   This function is used to stop the GPDMA peripheral for the given peripheral ID.
 *   It checks if the peripheral ID is valid and if the channel number is allocated.
 *   If the channel number is allocated, it stops the transfer and unregisters the callbacks.
 *   If the channel number is not allocated, it returns OK.
 *   If the peripheral ID is invalid, it returns INVALID_PARAMETER.
 */
sl_status_t sli_si91x_gpdma_peripheral_stop(sl_gpdma_peripheral_id_t peripheral_id)
{
  if (peripheral_id >= INVALID_PERIPHERAL_ID) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  if (gpdma_peripheral_channel_info[peripheral_id].channel_number == SL_GPDMA_CHANNEL_UNALLOCATED) {
    return SL_STATUS_OK;
  }
  return sl_si91x_gpdma_stop_transfer(gpdma_peripheral_channel_info[peripheral_id].channel_number);
}

/***************************************************************************/
/**
 * @brief Stop both directions of this peripheral and release their channels.
 * @param peripheral_id The peripheral ID to deinitialize.
 * @return SL_STATUS_OK if the peripheral is deinitialized successfully, otherwise an error code.
 * @note
 *   This function is used to deinitialize the GPDMA peripheral for the given peripheral ID.
 */
sl_status_t sli_si91x_gpdma_peripheral_deinit(sl_gpdma_peripheral_id_t peripheral_id)
{
  sl_status_t status;
  uint32_t tx_id;

  if (peripheral_id >= INVALID_PERIPHERAL_ID) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  tx_id = (uint32_t)peripheral_id & ~1U;
  for (uint32_t id = tx_id; id <= (tx_id + 1U); id++) {
    gpdma_peripheral_channel_info_t *info = &gpdma_peripheral_channel_info[id];
    if (info->channel_number == SL_GPDMA_CHANNEL_UNALLOCATED) {
      continue;
    }
    (void)sl_si91x_gpdma_stop_transfer(info->channel_number);
    status = sl_si91x_gpdma_unregister_callbacks(info->channel_number, SL_GPDMA_TRANSFER_DONE_CB);
    if (status != SL_STATUS_OK) {
      return status;
    }
    status = sl_si91x_gpdma_deallocate_channel(info->channel_number);
    if (status != SL_STATUS_OK) {
      return status;
    }
    info->channel_number = SL_GPDMA_CHANNEL_UNALLOCATED;
    info->resources      = NULL;
  }
  return SL_STATUS_OK;
}