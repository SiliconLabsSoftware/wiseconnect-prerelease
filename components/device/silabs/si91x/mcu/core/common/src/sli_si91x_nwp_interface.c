/***************************************************************************/ /**
 * @file  sli_si91x_nwp_interface.c
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
#include "sli_si91x_nwp_interface.h"
#include "base_types.h"
// Register definitions (TASS_P2P_INTR_MASK_CLR, P2P_STATUS_REG).
#include "system_si91x.h"
// Declares the M4 <-> NWP interrupt helpers and sli_si91x_req_wakeup() with their
// time-critical code classification, which keeps them RAM-resident.
#include "rsi_m4.h"
#include "sli_code_classification.h"

/* Flash command in-progress flag. */
static bool sli_si91x_packet_status = false;

/* TX command in-progress flag. */
static bool sli_si91x_tx_command_status = false;

#ifdef SL_SI91X_SIDE_BAND_CRYPTO
/* Side-band crypto command in-progress flag. */
static bool sli_si91x_side_band_crypto_status = false;
#endif

/* Number of callers that currently need the NWP awake. M4_WAKEUP_TA stays set until this hits zero. */
static uint32_t sli_si91x_nwp_wake_refcount = 0;

bool sli_si91x_get_flash_command_status(void)
{
  return sli_si91x_packet_status;
}

void sli_si91x_update_flash_command_status(bool flag)
{
  sli_si91x_packet_status = flag;
}

bool sli_si91x_get_tx_command_status(void)
{
  return sli_si91x_tx_command_status;
}

void sli_si91x_update_tx_command_status(bool flag)
{
  sli_si91x_tx_command_status = flag;
}

#ifdef SL_SI91X_SIDE_BAND_CRYPTO
bool sli_si91x_get_side_band_crypto_command_status(void)
{
  return sli_si91x_side_band_crypto_status;
}

void sli_si91x_update_side_band_crypto_command_status(bool flag)
{
  sli_si91x_side_band_crypto_status = flag;
}
#endif

// Unmask the specified NWP (TA) -> M4 P2P interrupts by clearing their mask bits.
void unmask_ta_interrupt(uint32_t interrupt_no)
{
  TASS_P2P_INTR_MASK_CLR = interrupt_no;
}

void mask_ta_interrupt(uint32_t interrupt_no)
{
  TASS_P2P_INTR_MASK_SET = interrupt_no;
}

void raise_m4_to_ta_interrupt(uint32_t interrupt_no)
{
  M4SS_P2P_INTR_SET_REG = interrupt_no;
}

// Take one wake hold, then block until the NWP reports active.
// The hold is released by sl_si91x_host_clear_sleep_indicator().
sl_status_t sli_si91x_req_wakeup(void)
{
  uint32_t primask = __get_PRIMASK();

  __disable_irq();
  sli_si91x_nwp_wake_refcount++;
  P2P_STATUS_REG |= M4_WAKEUP_TA;
  if (primask == 0U) {
    __enable_irq();
  }

  if (!(P2P_STATUS_REG & TA_IS_ACTIVE)) {
    //!TBD Need add timeout
    while (!(P2P_STATUS_REG & TA_IS_ACTIVE))
      ;
  }
  return SL_STATUS_OK;
}

// Release one wake hold. The NWP may sleep only after the last holder releases.
// A call with no outstanding hold still clears M4_WAKEUP_TA (PS2 wake cleanup).
SL_CODE_CLASSIFY(SL_CODE_COMPONENT_SI91X_WIRELESS, SL_CODE_CLASS_TIME_CRITICAL)
void sl_si91x_host_clear_sleep_indicator(void)
{
  uint32_t primask = __get_PRIMASK();

  __disable_irq();
  if (sli_si91x_nwp_wake_refcount > 0U) {
    sli_si91x_nwp_wake_refcount--;
  }
  if (sli_si91x_nwp_wake_refcount == 0U) {
    P2P_STATUS_REG &= ~M4_WAKEUP_TA;
  }
  if (primask == 0U) {
    __enable_irq();
  }
}
