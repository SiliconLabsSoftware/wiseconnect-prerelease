/***************************************************************************/ /**
 * @file sl_si91x_usart_common_config.h
 * @brief SL SI91X USART Common Config.
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

#ifndef SL_SI91X_USART_COMMON_CONFIG_H
#define SL_SI91X_USART_COMMON_CONFIG_H

// <<< Use Configuration Wizard in Context Menu >>>

// <h>USART0 Configuration

// <e> USART0 DMA Enable
// <i> Enable DMA for USART0/UART0 transfers.
// <i> Default: 1
#define SL_USART0_DMA_ENABLE 1

// <o SL_USART0_DMA_CONTROLLER> DMA Controller
// <0=> UDMA
// <1=> GPDMA
// <i> Select the DMA controller for USART0/UART0 transfers.
// <i> Synchronous mode uses the USART0 register map. Asynchronous mode uses UART0.
// <i> GPDMA peripheral codes: USART0/UART0 SRC (RX) = 8, DEST (TX) = 9.
// <i> Default: 0
#define SL_USART0_DMA_CONTROLLER 0

// <e> GPDMA Configuration
// <i> Descriptor counts and FIFO thresholds used when GPDMA is selected.
// <i> Default: 0
#define SL_USART0_GPDMA_CONFIG_ENABLE 0

// <o USART_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX> GPDMA Max Number of Descriptors TX <1-256>
// <i> Each descriptor moves up to 4095 bytes.
// <i> Default: 32
#define USART_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX 8

// <o USART_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX> GPDMA Max Number of Descriptors RX <1-256>
// <i> Each descriptor moves up to 4095 bytes.
// <i> Default: 32
#define USART_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX 8

// <o SL_USART0_TX_FIFO_THRESHOLD> USART0/UART0 TX FIFO threshold
//   <0=> USART_TRIGGER_TX_EMPTY
//   <1=> USART_FIFO_TX_AEMPTY
//   <4=> USART_FIFO_TX_QUARTER_FULL
//   <8=> USART_FIFO_TX_HALF_FULL
// <i> Wizard value and GPDMA TX burst: 0 = empty (1 beat), 1 = almost empty (2), 4 = quarter, 8 = half.
// <i> Default: 0
#define SL_USART0_TX_FIFO_THRESHOLD 0

// <o SL_USART0_RX_FIFO_THRESHOLD> USART0/UART0 RX FIFO threshold
//   <1=> USART_FIFO_RX_AEMPTY
//   <4=> USART_FIFO_RX_QUARTER_FULL
//   <8=> USART_FIFO_RX_HALF_FULL
//   <14=> USART_FIFO_RX_AFULL
// <i> Wizard value. GPDMA RX burst matches it (almost-empty is 1 beat).
// <i> Default: 1
#define SL_USART0_RX_FIFO_THRESHOLD 1
// </e>
// </e>

// <q SL_USART0_SYNC_MODE_EN> USART0 SYNC Mode
// <i> Required for SL_USART_MODE_SYNCHRONOUS_MASTER/SLAVE (binds USART0, not UART0).
// <i> Default: 0
#define SL_USART0_SYNC_MODE_EN 0

// <o SL_USART0_CLOCK_SRC> USART0 Clock Source
// <USART_ULPREFCLK=> ULP REF CLK
// <USART_SOCPLLCLK=> SOC PLL CLK
// <USART_INTFPLLCLK=> INTF PLL CLK
// <i> Default: USART_ULPREFCLK
#define SL_USART0_CLOCK_SRC USART_ULPREFCLK

// <o SL_USART0_FRAC_DIV_SEL> USART0 SCLK fractional divider mode
// <USART_FRACTIONAL_DIVIDER=> Fractional (clk_in / (div + 0.5))
// <USART_CLOCK_SWALLOW=> Clock swallow (clk_in / div)
// <i> See USART_SCLK_FRAC_SEL_T in rsi_pll.h
// <i> Default: USART_FRACTIONAL_DIVIDER (matches typical RTE_Device_917.h; use USART_CLOCK_SWALLOW in UC when needed)
#define SL_USART0_FRAC_DIV_SEL USART_FRACTIONAL_DIVIDER

// </h>
// <<< end of configuration section >>>

#if (SL_USART0_DMA_ENABLE == 1) && (SL_USART0_DMA_CONTROLLER == 1)
#define SL_SI91X_USART_GPDMA_ENABLE 1
#define SL_USART0_DMA_CONFIG_ENABLE 0
#elif (SL_USART0_DMA_ENABLE == 1)
#define SL_SI91X_USART_GPDMA_ENABLE 0
#define SL_USART0_DMA_CONFIG_ENABLE 1
#else
#define SL_SI91X_USART_GPDMA_ENABLE 0
#define SL_USART0_DMA_CONFIG_ENABLE 0
#endif
/* Same peripheral: UART0 is the asynchronous register map, USART0 is synchronous. */
#define SL_SI91X_UART_GPDMA_ENABLE SL_SI91X_USART_GPDMA_ENABLE

/* RTE_USART0_FRAC_DIV_SEL follows UC (SL_USART0_FRAC_DIV_SEL); RTE_Device value is not used after this header. */
#ifdef RTE_USART0_FRAC_DIV_SEL
#undef RTE_USART0_FRAC_DIV_SEL
#endif
#define RTE_USART0_FRAC_DIV_SEL SL_USART0_FRAC_DIV_SEL

// <<< sl:start pin_tool >>>
// <usart0 signal=(CLK),TX,RX,(CTS),(RTS)> SL_USART0
// $[USART0_SL_USART0]
#ifndef SL_USART0_PERIPHERAL
#define SL_USART0_PERIPHERAL USART0
#endif

// USART0 TX on GPIO_30
#ifndef SL_USART0_TX_PORT
#define SL_USART0_TX_PORT HP
#endif
#ifndef SL_USART0_TX_PIN
#define SL_USART0_TX_PIN 30
#endif
#ifndef SL_USART0_TX_LOC
#define SL_USART0_TX_LOC 5
#endif

// USART0 RX on GPIO_29
#ifndef SL_USART0_RX_PORT
#define SL_USART0_RX_PORT HP
#endif
#ifndef SL_USART0_RX_PIN
#define SL_USART0_RX_PIN 29
#endif
#ifndef SL_USART0_RX_LOC
#define SL_USART0_RX_LOC 10
#endif

// [USART0_SL_USART0]$
// <<< sl:end pin_tool >>>
#endif // SL_SI91X_USART_COMMON_CONFIG_H
