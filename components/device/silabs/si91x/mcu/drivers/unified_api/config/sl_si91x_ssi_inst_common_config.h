/***************************************************************************/
/**
 * @file sl_si91x_ssi_inst_common_config.h
 * @brief SL SI91X SSI INSTANCE Common Config.
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

#ifndef SL_SI91X_SSI_INSTANCE_COMMON_CONFIG_H
#define SL_SI91X_SSI_INSTANCE_COMMON_CONFIG_H

// <<< Use Configuration Wizard in Context Menu >>>
// <e> SSI Instance DMA Enable
// <i> Enable DMA for this SSI instance.
// <i> Default: 0
#define SL_SSI_INSTANCE_DMA_ENABLE 0

// <o SL_SSI_INSTANCE_DMA_CONTROLLER> DMA Controller
// <0=> UDMA
// <1=> GPDMA
// <i> Select the DMA controller for this SSI instance.
// <i> Primary request lines: SRC (RX) = 22, DEST (TX) = 23.
// <i> Secondary request lines: SRC (RX) = 2, DEST (TX) = 3.
// <i> ULP Primary stays on UDMA.
// <i> Default: 0
#define SL_SSI_INSTANCE_DMA_CONTROLLER 0

// <e> GPDMA Configuration
// <i> Descriptor counts used when GPDMA is selected.
// <i> Default: 0
#define SL_SSI_INSTANCE_GPDMA_CONFIG_ENABLE 0

// <o SSI_INSTANCE_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX> GPDMA Max Number of Descriptors TX <1-256>
// <i> Each descriptor moves up to 4095 bytes.
// <i> Default: 8
#define SSI_INSTANCE_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_TX 8

// <o SSI_INSTANCE_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX> GPDMA Max Number of Descriptors RX <1-256>
// <i> Each descriptor moves up to 4095 bytes.
// <i> Default: 8
#define SSI_INSTANCE_GPDMA_MAX_NUMBER_OF_DESCRIPTORS_RX 8
// </e>

// <o SL_SSI_INSTANCE_TX_FIFO_THRESHOLD> TX FIFO Threshold <0-15>
// <i> TX DMA request level. With GPDMA: 0-8, TX burst = threshold + 1.
// <i> Default: 0
#define SL_SSI_INSTANCE_TX_FIFO_THRESHOLD 0

// <o SL_SSI_INSTANCE_RX_FIFO_THRESHOLD> RX FIFO Threshold <0-15>
// <i> RX DMA request level. With GPDMA: 0-8, RX burst = threshold + 1.
// <i> Default: 0
#define SL_SSI_INSTANCE_RX_FIFO_THRESHOLD 0
// </e>
// <<< end of configuration section >>>

#if (SL_SSI_INSTANCE_DMA_ENABLE == 1) && (SL_SSI_INSTANCE_DMA_CONTROLLER == 1)
#define SL_SI91X_SSI_INSTANCE_GPDMA_ENABLE 1
#define SL_SSI_INSTANCE_DMA_CONFIG_ENABLE  0
#elif (SL_SSI_INSTANCE_DMA_ENABLE == 1)
#define SL_SI91X_SSI_INSTANCE_GPDMA_ENABLE 0
#define SL_SSI_INSTANCE_DMA_CONFIG_ENABLE  1
#else
#define SL_SI91X_SSI_INSTANCE_GPDMA_ENABLE 0
#define SL_SSI_INSTANCE_DMA_CONFIG_ENABLE  0
#endif
#endif // SL_SI91X_SSI_INSTANCE_COMMON_CONFIG_H
