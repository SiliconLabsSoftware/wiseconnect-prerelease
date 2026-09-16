/*******************************************************************************
 * @file  sl_si91x_etm.c
 * @brief Embedded Trace Macrocell (ETM) service API implementation for Si91x.
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

/***************************************************************************/ /**
 * @addtogroup ETM
 * @{
 ******************************************************************************/

#include "sl_si91x_etm.h"
#include "sl_si91x_clock_manager.h"
#include "sl_si91x_driver_gpio.h"
#include "sl_gpio_board.h"
#include "sl_assert.h"

#define SLI_SI91X_OUTPUT_VALUE 0 ///< Default output value used when configuring pin mode

/***************************************************************************/ /**
 * @brief Initializes the Embedded Trace Macrocell (ETM) and related CoreSight blocks.
 *
 * @details An Embedded Trace Macrocell (ETM) is a real-time trace module that provides
 *          instruction execution flow (and optionally data access patterns) for debugging
 *          and optimization. This function:
 *          - Switches the M4 core clock to SOC PLL (default 120 MHz; ETM requires >= 40 MHz)
 *          - Configures MCU_CLK_OUT and TRACE_CLK_IN / TRACE_CLK_OUT pins
 *          - Muxes TRACE data pins GPIO_52 and GPIO_54–57
 *          - Programs CoreDebug, ETM, TPIU, ITM, and DWT for 4-bit parallel tracing
 *
 * @return Status code indicating the result:
 *         - SL_STATUS_OK  - Success.
 *         - Corresponding error code on failure (for example, clock configuration).
 *
 * For more information on status codes, refer to
 * [SL STATUS DOCUMENTATION](https://docs.silabs.com/gecko-platform/latest/platform-common/status).
 ******************************************************************************/
void sl_si91x_etm_int(void)
{
  sl_status_t status = SL_STATUS_OK;

  /* GPIO pin configs for MCU clock out and TRACE clock pins */
  sl_si91x_gpio_pin_config_t mcu_clk_out_gpio12 = { { SL_SI91X_GPIO_12_PORT, SL_SI91X_GPIO_12_PIN }, GPIO_OUTPUT };
  sl_si91x_gpio_pin_config_t mcu_clk_out_gpio11 = { { SL_SI91X_GPIO_11_PORT, SL_SI91X_GPIO_11_PIN }, GPIO_OUTPUT };
  sl_si91x_gpio_pin_config_t trace_clk_out_gpio = { { SL_SI91X_GPIO_53_PORT, SL_SI91X_GPIO_53_PIN }, GPIO_OUTPUT };
  sl_si91x_gpio_pin_config_t trace_clk_in_gpio  = { { SL_SI91X_GPIO_15_PORT, SL_SI91X_GPIO_15_PIN }, GPIO_INPUT };

  /* Configure SOC PLL and switch M4 core clock to SOC PLL */
  status = sl_si91x_clock_manager_m4_set_core_clk(M4_SOCPLLCLK, PS4_SOC_FREQ);
  if (status != SL_STATUS_OK) {
    EFM_ASSERT(false);
  }

  /* MCU_CLK_OUT on GPIO_12 Mode 8 */
  sl_si91x_gpio_driver_enable_pad_selection(PAD_SEL_MCU_CLK);
  sl_si91x_gpio_driver_enable_pad_receiver(GPIO_MCU_CLK);
  sl_gpio_set_configuration(mcu_clk_out_gpio12);
  sl_gpio_driver_set_pin_mode(&mcu_clk_out_gpio12.port_pin, (sl_gpio_mode_t)SL_GPIO_MODE_8, SLI_SI91X_OUTPUT_VALUE);

  /* Configure the mcu_clk_out on gpio12 */
  status = sl_si91x_clock_manager_mcu_clk_out(mcu_clk_out_gpio12, SL_CLOCK_MANAGER_MCU_CLK_OUT_SEL_SOC_PLL, 0);
  if (status != SL_STATUS_OK) {
    EFM_ASSERT(false);
  }

  /* Also mux GPIO_11 Mode 12 as MCU_CLK_OUT */
  sl_si91x_gpio_driver_enable_pad_selection(SL_SI91X_GPIO_11_PAD);
  sl_si91x_gpio_driver_enable_pad_receiver(SL_SI91X_GPIO_11_PIN);
  sl_gpio_set_configuration(mcu_clk_out_gpio11);
  sl_gpio_driver_set_pin_mode(&mcu_clk_out_gpio11.port_pin, (sl_gpio_mode_t)SL_GPIO_MODE_12, SLI_SI91X_OUTPUT_VALUE);

  /* TRACE_CLK_OUT GPIO_53 Mode 6 (output) */
  sl_si91x_gpio_driver_enable_pad_selection(PAD_SEL_TRACE_CLK_OUT);
  sl_gpio_set_configuration(trace_clk_out_gpio);
  sl_gpio_driver_set_pin_mode(&trace_clk_out_gpio.port_pin, (sl_gpio_mode_t)SL_GPIO_MODE_6, SLI_SI91X_OUTPUT_VALUE);

  /* TRACE_CLK_IN GPIO_15 Mode 6 (input) */
  sl_si91x_gpio_driver_enable_pad_receiver(GPIO_TRACE_CLK_IN);
  sl_si91x_gpio_driver_enable_pad_selection(PAD_SEL_TRACE_CLK_IN);
  sl_gpio_set_configuration(trace_clk_in_gpio);
  sl_gpio_driver_set_pin_mode(&trace_clk_in_gpio.port_pin, (sl_gpio_mode_t)SL_GPIO_MODE_6, SLI_SI91X_OUTPUT_VALUE);

  /* TRACE data pins Mode 6 (GPIO_53 already configured as TRACE_CLK_OUT above) */
  sl_si91x_gpio_pin_config_t trace_gpio52 = { { SL_SI91X_GPIO_52_PORT, SL_SI91X_GPIO_52_PIN }, GPIO_OUTPUT };
  sl_si91x_gpio_pin_config_t trace_gpio54 = { { SL_SI91X_GPIO_54_PORT, SL_SI91X_GPIO_54_PIN }, GPIO_OUTPUT };
  sl_si91x_gpio_pin_config_t trace_gpio55 = { { SL_SI91X_GPIO_55_PORT, SL_SI91X_GPIO_55_PIN }, GPIO_OUTPUT };
  sl_si91x_gpio_pin_config_t trace_gpio56 = { { SL_SI91X_GPIO_56_PORT, SL_SI91X_GPIO_56_PIN }, GPIO_OUTPUT };
  sl_si91x_gpio_pin_config_t trace_gpio57 = { { SL_SI91X_GPIO_57_PORT, SL_SI91X_GPIO_57_PIN }, GPIO_OUTPUT };

  /* TRACE GPIO_52 Mode 6 (output) */
  sl_si91x_gpio_driver_enable_pad_selection(SL_SI91X_GPIO_52_PAD);
  sl_si91x_gpio_driver_enable_pad_receiver(SL_SI91X_GPIO_52_PIN);
  sl_gpio_set_configuration(trace_gpio52);
  sl_gpio_driver_set_pin_mode(&trace_gpio52.port_pin, (sl_gpio_mode_t)SL_GPIO_MODE_6, SLI_SI91X_OUTPUT_VALUE);

  /* TRACE GPIO_54 Mode 6 (output) */
  sl_si91x_gpio_driver_enable_pad_selection(SL_SI91X_GPIO_54_PAD);
  sl_si91x_gpio_driver_enable_pad_receiver(SL_SI91X_GPIO_54_PIN);
  sl_gpio_set_configuration(trace_gpio54);
  sl_gpio_driver_set_pin_mode(&trace_gpio54.port_pin, (sl_gpio_mode_t)SL_GPIO_MODE_6, SLI_SI91X_OUTPUT_VALUE);

  /* TRACE GPIO_55 Mode 6 (output) */
  sl_si91x_gpio_driver_enable_pad_selection(SL_SI91X_GPIO_55_PAD);
  sl_si91x_gpio_driver_enable_pad_receiver(SL_SI91X_GPIO_55_PIN);
  sl_gpio_set_configuration(trace_gpio55);
  sl_gpio_driver_set_pin_mode(&trace_gpio55.port_pin, (sl_gpio_mode_t)SL_GPIO_MODE_6, SLI_SI91X_OUTPUT_VALUE);

  /* TRACE GPIO_56 Mode 6 (output) */
  sl_si91x_gpio_driver_enable_pad_selection(SL_SI91X_GPIO_56_PAD);
  sl_si91x_gpio_driver_enable_pad_receiver(SL_SI91X_GPIO_56_PIN);
  sl_gpio_set_configuration(trace_gpio56);
  sl_gpio_driver_set_pin_mode(&trace_gpio56.port_pin, (sl_gpio_mode_t)SL_GPIO_MODE_6, SLI_SI91X_OUTPUT_VALUE);

  /* TRACE GPIO_57 Mode 6 (output) */
  sl_si91x_gpio_driver_enable_pad_selection(SL_SI91X_GPIO_57_PAD);
  sl_si91x_gpio_driver_enable_pad_receiver(SL_SI91X_GPIO_57_PIN);
  sl_gpio_set_configuration(trace_gpio57);
  sl_gpio_driver_set_pin_mode(&trace_gpio57.port_pin, (sl_gpio_mode_t)SL_GPIO_MODE_6, SLI_SI91X_OUTPUT_VALUE);

  /**
   * CoreDebug: halt/debug-enable via DHCSR and enable the trace system
   * (TRCENA) in DEMCR so ETM/ITM/DWT/TPIU can operate.
   */
  CoreDebug->DHCSR = ETM_DHCSR_VALUE;
  CoreDebug->DEMCR = ETM_DEMCR_VALUE;

  /**
   * Unlock the ETM Lock Access Register (LAR) and enter programming mode
   * via ETMCR so subsequent ETM control fields can be written.
   */
  ETM->LAR = SL_SI91X_ETM_UNLOCK_KEY;
  ETM->CR  = ETM_CR_INIT_VALUE;

  /**
   * TPIU: configure a 4-bit parallel trace port (CSPSR), bypass async
   * clock divide (ACPR), select parallel TXMODE (SPPR), and enable
   * continuous formatting (FFCR).
   */
  TPIU->CSPSR = ETM_TPIU_CSPSR_VALUE;
  TPIU->ACPR  = ETM_TPIU_ACPR_VALUE;
  TPIU->SPPR  = ETM_TPIU_SPPR_VALUE;
  TPIU->FFCR  = ETM_TPIU_FFCR_VALUE;

  /**
   * Unlock ITM and enable stimulus ports / privilege mask. Timestamps
   * are enabled later after DWT setup (ETM_ITM_TCR_FINAL_VALUE).
   */
  ITM->LAR = SL_SI91X_ETM_UNLOCK_KEY;
  ITM->TCR = ETM_ITM_TCR_INIT_VALUE;
  ITM->TER = ETM_ITM_TER_VALUE;
  ITM->TPR = ETM_ITM_TPR_VALUE;

  /**
   * ETM enable sequence: program TraceEnable control/event and ATB
   * trace ID while still in programming mode, then clear the
   * programming bit to start tracing.
   */
  ETM->CR       = ETM_CR_PROG_VALUE;
  ETM->TECR1    = ETM_TECR1_VALUE;
  ETM->TEEVR    = ETM_TEEVR_VALUE;
  ETM->TRACEIDR = ETM_TRACEIDR_VALUE;
  ETM->CR       = ETM_CR_ENABLE_VALUE;

  /**
   * DWT: clear performance counters, preload CYCCNT, enable cycle/
   * exception/CPI/LSU/sleep/fold event tracing, then enable cycle
   * events for periodic PC sampling support.
   */
  DWT->CTRL     = ETM_DWT_CTRL_INIT_VALUE;
  DWT->CPICNT   = ETM_DWT_CPICNT_VALUE;
  DWT->EXCCNT   = ETM_DWT_EXCCNT_VALUE;
  DWT->SLEEPCNT = ETM_DWT_SLEEPCNT_VALUE;
  DWT->LSUCNT   = ETM_DWT_LSUCNT_VALUE;
  DWT->FOLDCNT  = ETM_DWT_FOLDCNT_VALUE;
  DWT->CYCCNT   = ETM_DWT_CYCCNT_VALUE;
  DWT->CTRL     = ETM_DWT_CTRL_FINAL_VALUE;

  /* ITM final enable */
  ITM->TCR = ETM_ITM_TCR_FINAL_VALUE;
  /* GPIO6 as GPIO mode (mode 0) */
  sl_si91x_gpio_pin_config_t gpio6_cfg = { { SL_SI91X_GPIO_6_PORT, SL_SI91X_GPIO_6_PIN }, GPIO_OUTPUT };
  sl_si91x_gpio_driver_enable_pad_selection(SL_SI91X_GPIO_6_PAD);
  sl_gpio_set_configuration(gpio6_cfg);
  sl_gpio_driver_set_pin_mode(&gpio6_cfg.port_pin, (sl_gpio_mode_t)SL_GPIO_MODE_0, SLI_SI91X_OUTPUT_VALUE);
}

/** @} (end addtogroup ETM) */
