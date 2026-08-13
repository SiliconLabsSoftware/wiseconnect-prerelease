/***************************************************************************/ /**
 * @file  sl_si91x_bus.c
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
#include "sl_status.h"
#include "sli_hal_si91x_constants.h"
#include "sl_constants.h"
#include "sli_si91x_ahb_bus.h"
#include "sli_code_classification.h"

sl_status_t sli_hal_si91x_notify_events(uint32_t flags);

// Called when DMA done for RX packet is received
sl_status_t sli_receive_from_ta_done_isr(void)
{
  sl_status_t status = sli_si91x_ahb_bus_enqueue_rx((void *)rx_pkt_buffer);
  VERIFY_STATUS_AND_RETURN(status);

  sli_hal_si91x_notify_events(SLI_HAL_SI91X_RX_EVENT);

  return SL_STATUS_OK;
}

sl_status_t sli_receive_tx_buffer_available_isr(void)
{
  sli_hal_si91x_notify_events(SLI_HAL_SI91X_BUFFER_AVAILABLE_EVENT);
  return SL_STATUS_OK;
}
