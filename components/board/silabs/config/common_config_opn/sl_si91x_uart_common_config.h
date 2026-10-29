/***************************************************************************/ /**
 * @file sl_si91x_uart_common_config.h
 * @brief SL SI91X UART Common Config.
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

#ifndef SL_SI91X_UART_COMMON_CONFIG_H
#define SL_SI91X_UART_COMMON_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* NOTE: User should configure all macros defined below, while creating an
 * instance other than pre-defined one */

// <<< Use Configuration Wizard in Context Menu >>>
// <h>UART1 Configuration

// <e> UART1 DMA Enable
// <i> Enable DMA for UART1 transfers.
// <i> Default: 1
#define SL_UART1_DMA_ENABLE 1

// <o SL_UART1_DMA_CONTROLLER> DMA Controller
// <0=> UDMA
// <1=> GPDMA
// <i> Select the DMA controller for UART1 transfers.
// <i> GPDMA peripheral codes: UART1 SRC (RX) = 34, DEST (TX) = 35.
// <i> Default: 0
#define SL_UART1_DMA_CONTROLLER 0

// <e> GPDMA Configuration
// <i> Descriptor counts and FIFO thresholds used when GPDMA is selected.
// <i> Default: 0
#define SL_UART1_GPDMA_CONFIG_ENABLE 0

// <o UART1_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX> GPDMA Max Number of Descriptors TX <1-256>
// <i> Each descriptor moves up to 4095 bytes.
// <i> Default: 32
#define UART1_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX 32

// <o UART1_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX> GPDMA Max Number of Descriptors RX <1-256>
// <i> Each descriptor moves up to 4095 bytes.
// <i> Default: 32
#define UART1_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX 32

// <o SL_UART1_TX_FIFO_THRESHOLD> UART1 TX FIFO threshold
//   <0=> USART_TRIGGER_TX_EMPTY
//   <1=> USART_FIFO_TX_AEMPTY
//   <4=> USART_FIFO_TX_QUARTER_FULL
//   <8=> USART_FIFO_TX_HALF_FULL
// <i> Wizard value and GPDMA TX burst: 0 = empty (1 beat), 1 = almost empty (2), 4 = quarter, 8 = half.
// <i> Default: 0
#define SL_UART1_TX_FIFO_THRESHOLD 0

// <o SL_UART1_RX_FIFO_THRESHOLD> UART1 RX FIFO threshold
//   <1=> USART_FIFO_RX_AEMPTY
//   <4=> USART_FIFO_RX_QUARTER_FULL
//   <8=> USART_FIFO_RX_HALF_FULL
//   <14=> USART_FIFO_RX_AFULL
// <i> Wizard value. GPDMA RX burst matches it (almost-empty is 1 beat).
// <i> Default: 1
#define SL_UART1_RX_FIFO_THRESHOLD 1
// </e>
// </e>

// <o SL_UART1_CLOCK_SRC> UART1 Clock Source
// <USART_ULPREFCLK=> ULP REF CLK
// <USART_SOCPLLCLK=> SOC PLL CLK
// <USART_INTFPLLCLK=> INTF PLL CLK
// <i> Default: USART_ULPREFCLK
#define SL_UART1_CLOCK_SRC USART_ULPREFCLK

// </h>
// <<< end of configuration section >>>

#if (SL_UART1_DMA_ENABLE == 1) && (SL_UART1_DMA_CONTROLLER == 1)
#define SL_SI91X_UART1_GPDMA_ENABLE 1
#define SL_UART1_DMA_CONFIG_ENABLE  0
#elif (SL_UART1_DMA_ENABLE == 1)
#define SL_SI91X_UART1_GPDMA_ENABLE 0
#define SL_UART1_DMA_CONFIG_ENABLE  1
#else
#define SL_SI91X_UART1_GPDMA_ENABLE 0
#define SL_UART1_DMA_CONFIG_ENABLE  0
#endif

#if USER_CONFIGURATION_ENABLE

// <<< sl:start pin_tool >>>
// <uart1 signal=TX,RX,CTS,RTS> SL_UART1
// $[UART1_SL_UART1]
#ifndef SL_UART1_PERIPHERAL
#define SL_UART1_PERIPHERAL UART1
#endif

// UART1 TX on GPIO_7
#ifndef SL_UART1_TX_PORT
#define SL_UART1_TX_PORT HP
#endif
#ifndef SL_UART1_TX_PIN
#define SL_UART1_TX_PIN 7
#endif
#ifndef SL_UART1_TX_LOC
#define SL_UART1_TX_LOC 0
#endif

// UART1 RX on GPIO_6
#ifndef SL_UART1_RX_PORT
#define SL_UART1_RX_PORT HP
#endif
#ifndef SL_UART1_RX_PIN
#define SL_UART1_RX_PIN 6
#endif
#ifndef SL_UART1_RX_LOC
#define SL_UART1_RX_LOC 5
#endif

// UART1 CTS on GPIO_28
#ifndef SL_UART1_CTS_PORT
#define SL_UART1_CTS_PORT HP
#endif
#ifndef SL_UART1_CTS_PIN
#define SL_UART1_CTS_PIN 28
#endif
#ifndef SL_UART1_CTS_LOC
#define SL_UART1_CTS_LOC 11
#endif

// UART1 RTS on GPIO_27
#ifndef SL_UART1_RTS_PORT
#define SL_UART1_RTS_PORT HP
#endif
#ifndef SL_UART1_RTS_PIN
#define SL_UART1_RTS_PIN 27
#endif
#ifndef SL_UART1_RTS_LOC
#define SL_UART1_RTS_LOC 17
#endif
// [UART1_SL_UART1]$
// <<< sl:end pin_tool >>>

#else

#warning \
  "UART peripheral pins are not configured. To configure, either install [ENABLE USER CONFIGURATION] component or define USER_CONFIGURATION_ENABLE macro to 1, then configure the pins as per the Custom board."

#endif // USER_CONFIGURATION_ENABLE
#ifdef __cplusplus
}
#endif
#endif // SL_SI91X_UART_COMMON_CONFIG_H
