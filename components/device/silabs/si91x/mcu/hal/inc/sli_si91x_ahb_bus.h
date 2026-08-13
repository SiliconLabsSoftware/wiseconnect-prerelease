/***************************************************************************/ /**
 * @file  sli_si91x_ahb_bus.h
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
#ifndef SLI_SI91X_AHB_BUS_H
#define SLI_SI91X_AHB_BUS_H

#include <stdint.h>
#include "sl_status.h"
#include "cmsis_os2.h"
#include "rsi_m4.h"
#include "sl_types.h" // sl_wifi_buffer_t / sl_wifi_system_packet_t

/***************************************************************************/ /**
 * @brief
 *   SOC AHB bus APIs for frame transfer, RX submit, and DMA descriptors.
 ******************************************************************************/

#define SLI_SI91X_TA_PKT_TX_DONE (1U << 1)
#ifdef SL_SI91X_SIDE_BAND_CRYPTO
#define SLI_SI91X_SIDE_BAND_DONE (1U << 2)
#endif

extern rsi_m4ta_desc_t tx_desc[2];
extern rsi_m4ta_desc_t rx_desc[2];
extern sl_wifi_buffer_t *rx_pkt_buffer;
extern osEventFlagsId_t ta_events;

sl_status_t sl_si91x_bus_init(void);
sl_status_t sl_si91x_bus_deinit(void);
sl_status_t sli_si91x_bus_read_interrupt_status(uint16_t *interrupt_status);
sl_status_t sli_si91x_bus_read_frame(sl_wifi_buffer_t **buffer);
sl_status_t sli_si91x_bus_write_frame(sl_wifi_system_packet_t *packet,
                                      const uint8_t *payloadparam,
                                      uint16_t size_param);

sl_status_t sli_si91x_submit_rx_pkt(uint32_t timeout);
sl_status_t sli_submit_rx_buffer(uint32_t timeout);
void sli_si91x_config_m4_dma_desc_on_reset(void);
void rsi_update_tx_dma_desc(uint8_t skip_dma_valid);
void rsi_update_rx_dma_desc(void);

void sli_si91x_raise_pkt_pending_interrupt_to_ta(void);
void sli_si91x_ta_events_init(void);

/* Enqueue a completed RX buffer. */
sl_status_t sli_si91x_ahb_bus_enqueue_rx(void *buffer);

#endif // SLI_SI91X_AHB_BUS_H
