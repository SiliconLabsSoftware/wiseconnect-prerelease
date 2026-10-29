/******************************************************************************
 * @file sl_si91x_gpdma_peripheral_specific.h
 * @brief GPDMA handshake IDs, bursts, and start/stop helpers for peripherals.
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
#ifndef SL_SI91X_GPDMA_PERIPHERAL_SPECIFIC_H
#define SL_SI91X_GPDMA_PERIPHERAL_SPECIFIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "sl_si91x_gpdma.h"
#include "sl_component_catalog.h"

#if defined(UART_MODULE) || defined(USART_MODULE)
#include "sl_si91x_usart.h"
#endif
#ifdef UART_MODULE
#include "sl_si91x_uart_common_config.h"
#endif
#ifdef USART_MODULE
#include "sl_si91x_usart_common_config.h"
#endif
#ifdef SL_CATALOG_SL_SI91X_GSPI_PRESENT
#include "sl_si91x_gspi_common_config.h"
#endif
#ifdef SL_CATALOG_SSI_PRIMARY_PRESENT
#include "sl_si91x_ssi_primary_common_config.h"
#endif
#if defined(SL_CATALOG_SSI_SECONDARY_PRESENT) || defined(SL_CATALOG_SL_SI91X_SSI_PRESENT)
#include "sl_si91x_ssi_secondary_common_config.h"
#endif

/* Even IDs are TX (destination handshake). Odd IDs are RX (source handshake). */

#define SL_GPDMA_CHANNEL_UNALLOCATED 0xFF ///< Channel not yet allocated

#define SL_GPDMA_TX_DIRECTION 0 ///< Memory to peripheral
#define SL_GPDMA_RX_DIRECTION 1 ///< Peripheral to memory
#define SL_GPDMA_NO_DIRECTION 2 ///< Use the TX ID of the pair

#define SL_GPDMA_PERIPHERAL_RETRY_ON_ERROR 1
#define SL_GPDMA_PERIPHERAL_DMA_PROT       1
#define SL_GPDMA_AHB_BURST_SIZE            3
#define SL_GPDMA_LINK_LIST_INTERRUPT       0

/* Burst field 0 means 64 beats, so a usable FIFO threshold stops at 8. */
#define SL_GPDMA_FIFO_THRESHOLD_MAX            8
#define SL_GPDMA_THRESHOLD_TO_BURST(threshold) ((threshold) + 1)

#define SL_MAX_NUMBER_OF_IDS    INVALID_PERIPHERAL_ID
#define SL_GPDMA_IS_RX_ID(id)   (((uint32_t)(id)&1U) != 0U)
#define SL_GPDMA_ID(tx_id, dir) ((sl_gpdma_peripheral_id_t)((tx_id) + ((dir) == SL_GPDMA_RX_DIRECTION)))

/** Descriptor pool and handshake for one direction. */
typedef struct {
  sl_si91x_gpdma_descriptor_t *descriptor_memory; ///< NULL when this direction is not on GPDMA
  void *resources;                                ///< Returned to the completion callback
  uint16_t max_descriptors;
  uint8_t channel_id;     ///< Hardware request line
  uint8_t channel_number; ///< @ref SL_GPDMA_CHANNEL_UNALLOCATED until the first start
  uint8_t burst;          ///< Beats per request
} gpdma_peripheral_channel_info_t;

extern gpdma_peripheral_channel_info_t gpdma_peripheral_channel_info[SL_MAX_NUMBER_OF_IDS];

#if defined(UART_MODULE) || defined(USART_MODULE)

/* -------------------------------------------------------------------------- */
/* USART / UART                                   */
/* -------------------------------------------------------------------------- */

#define UART_CALCULATE_BURST_SIZE_TX(x) (((x) == 0) ? 1 : ((x) == 1) ? 2 : ((x) == 4) ? 4 : ((x) == 8) ? 8 : 1)
#define UART_CALCULATE_BURST_SIZE_RX(x) (((x) == 1) ? 1 : ((x) == 4) ? 4 : ((x) == 8) ? 8 : ((x) == 14) ? 14 : 1)

#ifndef SL_UART1_TX_FIFO_THRESHOLD
#define SL_UART1_TX_FIFO_THRESHOLD 0
#endif
#ifndef SL_UART1_RX_FIFO_THRESHOLD
#define SL_UART1_RX_FIFO_THRESHOLD 1
#endif
#ifndef SL_USART0_TX_FIFO_THRESHOLD
#define SL_USART0_TX_FIFO_THRESHOLD 0
#endif
#ifndef SL_USART0_RX_FIFO_THRESHOLD
#define SL_USART0_RX_FIFO_THRESHOLD 1
#endif

#if (defined(SL_SI91X_USART_GPDMA_ENABLE) && SL_SI91X_USART_GPDMA_ENABLE == 1)
#define SLI_USART0_GPDMA_EN 1
#else
#define SLI_USART0_GPDMA_EN 0
#endif
#if (defined(SL_SI91X_UART_GPDMA_ENABLE) && SL_SI91X_UART_GPDMA_ENABLE == 1)
#define SLI_UART0_GPDMA_EN 1
#else
#define SLI_UART0_GPDMA_EN 0
#endif
#if (defined(SL_SI91X_UART1_GPDMA_ENABLE) && SL_SI91X_UART1_GPDMA_ENABLE == 1)
#define SLI_UART1_GPDMA_EN 1
#else
#define SLI_UART1_GPDMA_EN 0
#endif

#define SL_USART0_GPDMA_DATA_BURST_SIZE_TX UART_CALCULATE_BURST_SIZE_TX(SL_USART0_TX_FIFO_THRESHOLD)
#define SL_USART0_GPDMA_DATA_BURST_SIZE_RX UART_CALCULATE_BURST_SIZE_RX(SL_USART0_RX_FIFO_THRESHOLD)
#define SL_UART1_GPDMA_DATA_BURST_SIZE_TX  UART_CALCULATE_BURST_SIZE_TX(SL_UART1_TX_FIFO_THRESHOLD)
#define SL_UART1_GPDMA_DATA_BURST_SIZE_RX  UART_CALCULATE_BURST_SIZE_RX(SL_UART1_RX_FIFO_THRESHOLD)

#define SL_USART_GPDMA_SOURCE_CHANNEL_ID      8  ///< USART0/UART0 RX
#define SL_USART_GPDMA_DESTINATION_CHANNEL_ID 9  ///< USART0/UART0 TX
#define SL_UART1_GPDMA_SOURCE_CHANNEL_ID      34 ///< UART1 RX
#define SL_UART1_GPDMA_DESTINATION_CHANNEL_ID 35 ///< UART1 TX

/** True when this USART resource is routed to GPDMA. @p usart is USART_RESOURCES *. */
#define SLI_USART_USES_GPDMA(usart)                                                                         \
  ((((usart)->pREGS == USART0) && SLI_USART0_GPDMA_EN) || (((usart)->pREGS == UART1) && SLI_UART1_GPDMA_EN) \
   || (((usart)->pREGS == UART0) && SLI_UART0_GPDMA_EN))

/* Wizard TX threshold 0, 1, 4, 8 -> FCR symbol. Unknown values stay at empty. */
#define SL_USART_TX_FIFO_THRESHOLD_TO_FCR(threshold) \
  (((threshold) == 0)   ? USART_FIFO_TX_EMPTY        \
   : ((threshold) == 1) ? USART_FIFO_TX_AEMPTY       \
   : ((threshold) == 4) ? USART_FIFO_TX_QUARTER_FULL \
   : ((threshold) == 8) ? USART_FIFO_TX_HALF_FULL    \
                        : USART_FIFO_TX_EMPTY)

/* Wizard RX threshold 1, 4, 8, 14 -> FCR symbol. Unknown values stay almost empty. */
#define SL_USART_RX_FIFO_THRESHOLD_TO_FCR(threshold)  \
  (((threshold) == 1)    ? USART_FIFO_RX_AEMPTY       \
   : ((threshold) == 4)  ? USART_FIFO_RX_QUARTER_FULL \
   : ((threshold) == 8)  ? USART_FIFO_RX_HALF_FULL    \
   : ((threshold) == 14) ? USART_FIFO_RX_AFULL        \
                         : USART_FIFO_RX_AEMPTY)

#if ((defined(SL_SI91X_USART_GPDMA_ENABLE) && (SL_SI91X_USART_GPDMA_ENABLE == 1))  \
     || (defined(SL_SI91X_UART_GPDMA_ENABLE) && (SL_SI91X_UART_GPDMA_ENABLE == 1)) \
     || (defined(SL_SI91X_UART1_GPDMA_ENABLE) && (SL_SI91X_UART1_GPDMA_ENABLE == 1)))
#ifndef SL_CATALOG_SL_SI91X_GPDMA_PRESENT
#error "USART/UART GPDMA is enabled. Add the GPDMA component (sl_gpdma)."
#endif
#define SLI_USART_GPDMA_LINKED 1
#else
#define SLI_USART_GPDMA_LINKED 0
#endif

#endif

#ifdef SL_CATALOG_SL_SI91X_GSPI_PRESENT
/* -------------------------------------------------------------------------- */
/* GSPI                                                                       */
/* -------------------------------------------------------------------------- */

#define SL_GSPI_GPDMA_SRC_CHANNEL_ID  10
#define SL_GSPI_GPDMA_DEST_CHANNEL_ID 11

#if (defined(SL_SI91X_GSPI_GPDMA_ENABLE) && SL_SI91X_GSPI_GPDMA_ENABLE == 1)
#define SLI_GSPI_GPDMA_EN 1
#else
#define SLI_GSPI_GPDMA_EN 0
#endif

#ifdef SL_GSPI_AEMPTY_THRESHOLD
#define SL_GSPI_GPDMA_TX_THRESHOLD SL_GSPI_AEMPTY_THRESHOLD
#else
#define SL_GSPI_GPDMA_TX_THRESHOLD 0
#endif
#ifdef SL_GSPI_AFULL_THRESHOLD
#define SL_GSPI_GPDMA_RX_THRESHOLD SL_GSPI_AFULL_THRESHOLD
#else
#define SL_GSPI_GPDMA_RX_THRESHOLD 0
#endif

#if SLI_GSPI_GPDMA_EN                                            \
  && ((SL_GSPI_GPDMA_TX_THRESHOLD > SL_GPDMA_FIFO_THRESHOLD_MAX) \
      || (SL_GSPI_GPDMA_RX_THRESHOLD > SL_GPDMA_FIFO_THRESHOLD_MAX))
#error "GSPI GPDMA: FIFO almost-empty/almost-full thresholds must be 0-8"
#endif

#define SL_GSPI_GPDMA_DATA_BURST_SIZE_TX SL_GPDMA_THRESHOLD_TO_BURST(SL_GSPI_GPDMA_TX_THRESHOLD)
#define SL_GSPI_GPDMA_DATA_BURST_SIZE_RX SL_GPDMA_THRESHOLD_TO_BURST(SL_GSPI_GPDMA_RX_THRESHOLD)

#ifndef GSPI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX
#define GSPI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX 8
#endif
#ifndef GSPI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX
#define GSPI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX 8
#endif

#endif /* SL_CATALOG_SL_SI91X_GSPI_PRESENT */

#if defined(SL_CATALOG_SSI_PRIMARY_PRESENT) || defined(SL_CATALOG_SSI_SECONDARY_PRESENT) \
  || defined(SL_CATALOG_SL_SI91X_SSI_PRESENT)
/* -------------------------------------------------------------------------- */
/* SSI primary                                                                */
/* -------------------------------------------------------------------------- */

#define SL_SSI_PRIMARY_GPDMA_SRC_CHANNEL_ID  22
#define SL_SSI_PRIMARY_GPDMA_DEST_CHANNEL_ID 23

#if ((defined(SL_SI91X_SSI_GPDMA_ENABLE) && (SL_SI91X_SSI_GPDMA_ENABLE == 1)) \
     || (defined(SL_SI91X_SSI_PRIMARY_GPDMA_ENABLE) && (SL_SI91X_SSI_PRIMARY_GPDMA_ENABLE == 1)))
#define SLI_SSI_PRIMARY_GPDMA_EN 1
#else
#define SLI_SSI_PRIMARY_GPDMA_EN 0
#endif

#ifdef SL_SSI_PRIMARY_TX_FIFO_THRESHOLD
#define SL_SSI_PRIMARY_GPDMA_TX_THRESHOLD SL_SSI_PRIMARY_TX_FIFO_THRESHOLD //Threshold for primary instance tx
#elif defined(SL_SSI_TX_FIFO_THRESHOLD)
#define SL_SSI_PRIMARY_GPDMA_TX_THRESHOLD SL_SSI_TX_FIFO_THRESHOLD //Threshold for unknown instance tx
#else
#define SL_SSI_PRIMARY_GPDMA_TX_THRESHOLD 0
#endif

#ifdef SL_SSI_PRIMARY_RX_FIFO_THRESHOLD
#define SL_SSI_PRIMARY_GPDMA_RX_THRESHOLD SL_SSI_PRIMARY_RX_FIFO_THRESHOLD //Threshold for primary instance rx
#elif defined(SL_SSI_RX_FIFO_THRESHOLD)
#define SL_SSI_PRIMARY_GPDMA_RX_THRESHOLD SL_SSI_RX_FIFO_THRESHOLD //Threshold for unknown instance rx
#else
#define SL_SSI_PRIMARY_GPDMA_RX_THRESHOLD 0
#endif

#define SL_SSI_PRIMARY_GPDMA_DATA_BURST_SIZE_TX SL_GPDMA_THRESHOLD_TO_BURST(SL_SSI_PRIMARY_GPDMA_TX_THRESHOLD)
#define SL_SSI_PRIMARY_GPDMA_DATA_BURST_SIZE_RX SL_GPDMA_THRESHOLD_TO_BURST(SL_SSI_PRIMARY_GPDMA_RX_THRESHOLD)

#ifndef SSI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX
#define SSI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX 8
#endif
#ifndef SSI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX
#define SSI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX 8
#endif
#ifndef SSI_PRIMARY_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX
#define SSI_PRIMARY_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX SSI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX
#endif
#ifndef SSI_PRIMARY_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX
#define SSI_PRIMARY_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX SSI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX
#endif

/* -------------------------------------------------------------------------- */
/* SSI secondary                                                              */
/* -------------------------------------------------------------------------- */

#define SL_SSI_SECONDARY_GPDMA_SRC_CHANNEL_ID  2
#define SL_SSI_SECONDARY_GPDMA_DEST_CHANNEL_ID 3

#if ((defined(SL_SI91X_SSI_GPDMA_ENABLE) && (SL_SI91X_SSI_GPDMA_ENABLE == 1)) \
     || (defined(SL_SI91X_SSI_SECONDARY_GPDMA_ENABLE) && (SL_SI91X_SSI_SECONDARY_GPDMA_ENABLE == 1)))
#define SLI_SSI_SECONDARY_GPDMA_EN 1
#else
#define SLI_SSI_SECONDARY_GPDMA_EN 0
#endif

#ifdef SL_SSI_SECONDARY_TX_FIFO_THRESHOLD
#define SL_SSI_SECONDARY_GPDMA_TX_THRESHOLD SL_SSI_SECONDARY_TX_FIFO_THRESHOLD
#elif defined(SL_SSI_TX_FIFO_THRESHOLD)
#define SL_SSI_SECONDARY_GPDMA_TX_THRESHOLD SL_SSI_TX_FIFO_THRESHOLD
#else
#define SL_SSI_SECONDARY_GPDMA_TX_THRESHOLD 0
#endif
#ifdef SL_SSI_SECONDARY_RX_FIFO_THRESHOLD
#define SL_SSI_SECONDARY_GPDMA_RX_THRESHOLD SL_SSI_SECONDARY_RX_FIFO_THRESHOLD
#elif defined(SL_SSI_RX_FIFO_THRESHOLD)
#define SL_SSI_SECONDARY_GPDMA_RX_THRESHOLD SL_SSI_RX_FIFO_THRESHOLD
#else
#define SL_SSI_SECONDARY_GPDMA_RX_THRESHOLD 0
#endif

#define SL_SSI_SECONDARY_GPDMA_DATA_BURST_SIZE_TX SL_GPDMA_THRESHOLD_TO_BURST(SL_SSI_SECONDARY_GPDMA_TX_THRESHOLD)
#define SL_SSI_SECONDARY_GPDMA_DATA_BURST_SIZE_RX SL_GPDMA_THRESHOLD_TO_BURST(SL_SSI_SECONDARY_GPDMA_RX_THRESHOLD)

#ifndef SSI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX
#define SSI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX 8
#endif
#ifndef SSI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX
#define SSI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX 8
#endif
#ifndef SSI_SECONDARY_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX
#define SSI_SECONDARY_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX SSI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX
#endif
#ifndef SSI_SECONDARY_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX
#define SSI_SECONDARY_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX SSI_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX
#endif

#if (defined(SLI_SSI_PRIMARY_GPDMA_EN) && (SLI_SSI_PRIMARY_GPDMA_EN == 1)) \
  || (defined(SLI_SSI_SECONDARY_GPDMA_EN) && (SLI_SSI_SECONDARY_GPDMA_EN == 1))
#define SLI_SSI_GPDMA_EN 1
#elif defined(SL_CATALOG_SSI_PRIMARY_PRESENT) || defined(SL_CATALOG_SSI_SECONDARY_PRESENT) \
  || defined(SL_CATALOG_SL_SI91X_SSI_PRESENT)
#define SLI_SSI_GPDMA_EN 0
#endif

/** True for SSI0 or SSISlave when that instance is on GPDMA. @p spi is SPI_RESOURCES *. */
#define SPI_USES_GPDMA(spi) \
  ((((spi)->reg == SSI0) && SLI_SSI_PRIMARY_GPDMA_EN) || (((spi)->reg == SSISlave) && SLI_SSI_SECONDARY_GPDMA_EN))

#endif /* SL_CATALOG_SSI_SECONDARY_PRESENT || SL_CATALOG_SL_SI91X_SSI_PRESENT */

/***************************************************************************/
#ifdef SLI_USART_GPDMA_LINKED
/**
  * @brief GPDMA ID for a USART resource, or @ref INVALID_PERIPHERAL_ID on UDMA.
  * @param usart USART_RESOURCES *.
  * @param direction @ref SL_GPDMA_TX_DIRECTION, @ref SL_GPDMA_RX_DIRECTION, or
  *                  @ref SL_GPDMA_NO_DIRECTION (returns the TX ID).
  */
sl_gpdma_peripheral_id_t sli_si91x_gpdma_usart_peripheral_id(const void *usart, uint8_t direction);
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
sl_gpdma_peripheral_id_t sli_si91x_gpdma_ssi_peripheral_id(const SPI_RESOURCES *spi, bool is_tx);
#endif

/***************************************************************************/
/**
  * @brief Start one direction. Even IDs transmit, odd IDs receive.
  * @param num_bytes Must be a multiple of @p data_width_bytes (1, 2, or 4).
  * @return SL_STATUS_OK, SL_STATUS_INVALID_PARAMETER, or the GPDMA status.
  */
sl_status_t sli_si91x_gpdma_peripheral_start(void *resources,
                                             sl_gpdma_peripheral_id_t peripheral_id,
                                             const void *src,
                                             const void *dst,
                                             uint32_t num_bytes,
                                             uint8_t data_width_bytes,
                                             sl_gpdma_callback_t transfer_complete_cb);

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
sl_status_t sli_si91x_gpdma_peripheral_stop(sl_gpdma_peripheral_id_t peripheral_id);

/***************************************************************************/

/** @brief Stop both directions of this peripheral and release their channels.
 * @param peripheral_id The peripheral ID to deinitialize.
 * @return SL_STATUS_OK if the peripheral is deinitialized successfully, otherwise an error code.
 * @note
 *   This function is used to deinitialize the GPDMA peripheral for the given peripheral ID.
 *   It checks if the peripheral ID is valid and if the channel number is allocated.
 *   If the channel number is allocated, it stops the transfer and unregisters the callbacks.
 *   If the channel number is not allocated, it returns OK.
 *   If the peripheral ID is invalid, it returns INVALID_PARAMETER.
 */
sl_status_t sli_si91x_gpdma_peripheral_deinit(sl_gpdma_peripheral_id_t peripheral_id);

#ifdef __cplusplus
}
#endif

#endif /* SL_SI91X_GPDMA_PERIPHERAL_SPECIFIC_H */
