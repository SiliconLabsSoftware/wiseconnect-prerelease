/***************************************************************************/ /**
 * @file
 * @brief SPI abstraction used by memory lcd display
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
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

#include "sl_memlcd_spi.h"
#include "sl_memlcd_display.h"
#include "SPI.h"
#include "rsi_debug.h"
#include "sl_status.h"
#include "sl_common.h"

#define SPI_BIT_WIDTH 8

/***************************************************************************/ /**
  * @brief
  *   Size of the static SPI TX scratch buffer in bytes.
  *
  * @details
  *   Holds one MEMLCD row after bit-reversal. Used by sli_memlcd_spi_tx().
  *****************************************************************************/
#define SLI_MEMLCD_SPI_TX_MAX ((SL_MEMLCD_DISPLAY_WIDTH * SL_MEMLCD_DISPLAY_BPP) / 8)

volatile uint8_t spi_done_ok           = 0;
static volatile uint8_t spi_xfer_error = 0;
static uint8_t spi_tx_buf[SLI_MEMLCD_SPI_TX_MAX];
static uint8_t rbit_lut[256];
extern ARM_DRIVER_SPI Driver_SSI_ULP_MASTER;
int32_t status_spi          = 0;
static bool spi_initialized = false;

/***************************************************************************/ /**
  * @brief
  *   Build the 256-entry bit-reverse lookup table used for Sharp wire format.
  *****************************************************************************/
static void rbit_lut_init(void)
{
  for (uint32_t i = 0; i < 256U; i++) {
    rbit_lut[i] = SL_RBIT8((uint8_t)i);
  }
}

/***************************************************************************/ /**
  * @brief
  *   Bit-reverse one byte for the MEMLCD SPI wire format.
  *
  * @param[in] value
  *   Byte in framebuffer bit order.
  *
  * @return
  *   Bit-reversed byte from the lookup table.
  *****************************************************************************/
uint8_t sli_memlcd_rbit_byte(uint8_t value)
{
  return rbit_lut[value];
}

/*==============================================*/
/**
  * @fn       void mySPI_callback(uint32_t event)
  * @brief  SPI callback handler
  * @param[in]event SPI transmit and receive events
  * @return   None
  */
void mySPI_callback_spi(uint32_t event)
{
  // Clearing the instance number to evaluate the event
  event &= SSI_INSTANCE_MASK;
  switch (event) {
    case ARM_SPI_EVENT_TRANSFER_COMPLETE:
      spi_done_ok = 1;
      break;
    case ARM_SPI_EVENT_DATA_LOST:
      /*  Occurs in slave mode when data is requested/sent by master
             but send/receive/transfer operation has not been started
             and indicates that data is lost. Occurs also in master mode
             when driver cannot transfer data fast enough. */
      SL_PRINT_STRING_ERROR("\r\n ARM_SPI_EVENT_DATA_LOST \r\n");
      spi_xfer_error = 1;
      spi_done_ok    = 1;
      break;
    case ARM_SPI_EVENT_MODE_FAULT:
      /*  Occurs in slave mode when Slave Select is deactivated and
             indicates Master Mode Fault. */
      SL_PRINT_STRING_ERROR("\r\n ARM_SPI_EVENT_MODE_FAULT \r\n");
      spi_xfer_error = 1;
      spi_done_ok    = 1;
      break;
  }
}

sl_status_t sli_memlcd_spi_init(uint32_t spi_baud)
{
  ARM_DRIVER_SPI *SPIdrv = &Driver_SSI_ULP_MASTER;

  rbit_lut_init();

  if (spi_initialized) {
    status_spi = SPIdrv->Uninitialize();
    if (status_spi != ARM_DRIVER_OK) {
      SL_PRINT_STRING_ERROR("memlcd_spi: Uninitialize failed, code=%ld", (long)status_spi);
      return SL_STATUS_FAIL;
    }
    spi_initialized = false;
  }

  // Initialize the SPI driver
  status_spi = SPIdrv->Initialize(mySPI_callback_spi);
  if (status_spi != ARM_DRIVER_OK) {
    SL_PRINT_STRING_ERROR("\r\n SPI Initialization Failed, Error Code : %ld\r\n", (long)status_spi);
    SL_PRINT_STRING_ERROR("memlcd_spi: Initialize failed, code=%ld", (long)status_spi);
    return SL_STATUS_FAIL;
  }

  // Power up the SPI peripheral
  status_spi = SPIdrv->PowerControl(ARM_POWER_FULL);
  if (status_spi != ARM_DRIVER_OK) {
    SL_PRINT_STRING_ERROR("\r\n Failed to Set Power to SPI, Error Code : %ld\r\n", (long)status_spi);
    return SL_STATUS_FAIL;
  }

#if (defined(SPI_MULTI_SLAVE))
  RSI_SPI_SetSlaveSelectNumber(0);
#endif
  // Configure the SPI to Master, 16-bit mode @10000 kBits/sec
  status_spi = SPIdrv->Control(
    ARM_SPI_MODE_MASTER | ARM_SPI_CPOL0_CPHA0 | ARM_SPI_SS_MASTER_HW_OUTPUT | ARM_SPI_DATA_BITS(SPI_BIT_WIDTH),
    spi_baud);
  if (status_spi != ARM_DRIVER_OK) {
    SL_PRINT_STRING_ERROR("\r\n Failed to Set Configuration Parameters to SPI, "
                          "Error Code : %ld\r\n",
                          (long)status_spi);
    return SL_STATUS_FAIL;
  }

  spi_initialized = true;
  return SL_STATUS_OK;
}
sl_status_t sli_memlcd_spi_shutdown()
{
  ARM_DRIVER_SPI *SPIdrv = &Driver_SSI_ULP_MASTER;
  status_spi             = SPIdrv->PowerControl(ARM_POWER_OFF);
  if (status_spi != ARM_DRIVER_OK) {
    SL_PRINT_STRING_ERROR("\r\n Failed to Set Power to SPI, Error Code : %ld\r\n", (long)status_spi);
    return SL_STATUS_FAIL;
  }

  spi_initialized = false;
  return SL_STATUS_OK;
}

/***************************************************************************/ /**
  * @brief
  *   Transmit one prepared buffer and wait until the SPI transfer completes.
  *
  * @param[in] data
  *   Pointer to wire-format bytes to send (already bit-reversed if required).
  * @param[in] len
  *   Number of bytes to transmit.
  *
  * @return
  *   SL_STATUS_OK on success, or SL_STATUS_FAIL if SS, Send, or the
  *   transfer callback reports a fault.
  *****************************************************************************/
sl_status_t sli_memlcd_spi_transfer_and_wait(const uint8_t *data, unsigned len)
{
  ARM_DRIVER_SPI *SPIdrv = &Driver_SSI_ULP_MASTER;

  status_spi = SPIdrv->Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_ACTIVE);
  if (status_spi != ARM_DRIVER_OK) {
    SL_PRINT_STRING_ERROR("\r\n Failed to Active Slave Select Line for SPI "
                          "Transfer, Error Code : %ld\r\n",
                          (long)status_spi);
    return SL_STATUS_FAIL;
  }

  spi_xfer_error = 0;
  spi_done_ok    = 0;

  status_spi = SPIdrv->Send(data, len);
  if (status_spi != ARM_DRIVER_OK) {
    (void)SPIdrv->Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);
    return SL_STATUS_FAIL;
  }

  sli_memlcd_spi_wait();

  (void)SPIdrv->Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);
  if (spi_xfer_error != 0) {
    return SL_STATUS_FAIL;
  }
  return SL_STATUS_OK;
}

sl_status_t sli_memlcd_spi_tx(const void *data, unsigned len)
{
  const uint8_t *buffer = data;

  if (len > SLI_MEMLCD_SPI_TX_MAX) {
    return SL_STATUS_FAIL;
  }

  for (uint32_t i = 0; i < len; i++) {
    spi_tx_buf[i] = sli_memlcd_rbit_byte(buffer[i]);
  }

  return sli_memlcd_spi_transfer_and_wait(spi_tx_buf, len);
}

void sli_memlcd_spi_wait()
{
  while (!spi_done_ok)
    ;

  spi_done_ok = 0;
}

void sli_memlcd_spi_rx_flush()
{
  // Do nothing, SPI Driver handles this.
}
