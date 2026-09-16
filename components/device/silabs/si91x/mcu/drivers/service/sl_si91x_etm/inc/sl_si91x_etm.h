/*******************************************************************************
 * @file  sl_si91x_etm.h
 * @brief Embedded Trace Macrocell (ETM) service API for Si91x.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef ETM_H_
#define ETM_H_

#include "si91x_device.h"
#include "sl_status.h"

/***************************************************************************/ /**
 * @addtogroup ETM Embedded Trace Macrocell
 * @ingroup SI91X_SERVICE_APIS
 * @{
 ******************************************************************************/

/*******************************************************************************
 ******************************   ETM Macros   *********************************
 ******************************************************************************/

#define PAD_SEL_REG       0x41300610 ///< Pad selection register address
#define REN_SEL_REG       0x460040D0 ///< Receiver enable selection register address
#define GPIO52_CONFIG_REG 0x46130340 ///< GPIO 52 configuration register address
#define GPIO53_CONFIG_REG 0x46130350 ///< GPIO 53 configuration register address
#define GPIO54_CONFIG_REG 0x46130360 ///< GPIO 54 configuration register address
#define GPIO55_CONFIG_REG 0x46130370 ///< GPIO 55 configuration register address
#define GPIO56_CONFIG_REG 0x46130380 ///< GPIO 56 configuration register address
#define GPIO57_CONFIG_REG 0x46130390 ///< GPIO 57 configuration register address
#define GPIO6_CONFIG_REG  0x46130060 ///< GPIO 06 configuration register address
#define GPIO6_TOGGLE_REG  0x46130064 ///< GPIO 06 toggle register address

#define PAD_SEL_VALUE_ITM 0x3F0000 ///< Pad selection value for ITM/ETM trace pins
#define REN_SEL_VALUE_ITM 31       ///< Receiver enable value for ITM/ETM trace pins

#define GPIO52_CONFIG_VALUE 0x19 ///< GPIO 52 configuration value (mode 6)
#define GPIO53_CONFIG_VALUE 0x19 ///< GPIO 53 configuration value (mode 6)
#define GPIO54_CONFIG_VALUE 0x19 ///< GPIO 54 configuration value (mode 6)
#define GPIO55_CONFIG_VALUE 0x19 ///< GPIO 55 configuration value (mode 6)
#define GPIO56_CONFIG_VALUE 0x19 ///< GPIO 56 configuration value (mode 6)
#define GPIO57_CONFIG_VALUE 0x19 ///< GPIO 57 configuration value (mode 6)

#define SOC_PLL_REF_FREQUENCY 40000000 ///< PLL input reference clock frequency (40 MHz)

/// @note Change this macro to the required PLL frequency in hertz.
#define PS4_SOC_FREQ          120000000 ///< Default SOC PLL output frequency (120 MHz)
#define PAD_SEL_MCU_CLK       7         ///< Pad selection for MCU clock out
#define GPIO_MCU_CLK          12        ///< GPIO pin for MCU clock out
#define PAD_SEL_TRACE_CLK_OUT 17        ///< Pad selection for TRACE_CLK_OUT
#define GPIO_TRACE_CLK_OUT    53        ///< GPIO pin for TRACE_CLK_OUT
#define PAD_SEL_TRACE_CLK_IN  8         ///< Pad selection for TRACE_CLK_IN
#define GPIO_TRACE_CLK_IN     15        ///< GPIO pin for TRACE_CLK_IN

#define SL_SI91X_ETM_UNLOCK_KEY 0xC5ACCE55UL   ///< CoreSight lock access key
#define ETM_BASE                (0xE0041000UL) ///< ETM register block base address

/*******************************************************************************
 ********************   CoreSight / ETM Init Values  ***************************
 ******************************************************************************/

/* ETMCR bit fields (ETMv3.x — not in CMSIS) */
#define ETM_CR_STALL_PROCESSOR_Pos    (4U)                                ///< ETMCR stall processor bit position
#define ETM_CR_STALL_PROCESSOR_Msk    (1UL << ETM_CR_STALL_PROCESSOR_Pos) ///< ETMCR stall processor mask
#define ETM_CR_PROGRAMMING_Pos        (10U)                               ///< ETMCR programming bit position
#define ETM_CR_PROGRAMMING_Msk        (1UL << ETM_CR_PROGRAMMING_Pos)     ///< ETMCR programming mask
#define ETM_CR_DEBUG_REQUEST_CTRL_Pos (11U)                               ///< ETMCR debug request control bit position
#define ETM_CR_DEBUG_REQUEST_CTRL_Msk (1UL << ETM_CR_DEBUG_REQUEST_CTRL_Pos) ///< ETMCR debug request control mask
#define ETM_CR_BRANCH_OUTPUT_Pos      (13U)                                  ///< ETMCR branch output bit position
#define ETM_CR_BRANCH_OUTPUT_Msk      (1UL << ETM_CR_BRANCH_OUTPUT_Pos)      ///< ETMCR branch output mask

/* CoreDebug */
#define ETM_DHCSR_DBGKEY (0xA05FUL << CoreDebug_DHCSR_DBGKEY_Pos) ///< DHCSR debug key value
#define ETM_DHCSR_VALUE \
  (ETM_DHCSR_DBGKEY | CoreDebug_DHCSR_C_HALT_Msk | CoreDebug_DHCSR_C_DEBUGEN_Msk) ///< DHCSR: key + halt + debug enable
#define ETM_DEMCR_VALUE (CoreDebug_DEMCR_TRCENA_Msk) ///< DEMCR: enable CoreSight trace system

/* ETMCR programming sequence */
#define ETM_CR_INIT_VALUE (ETM_CR_PROGRAMMING_Msk | ETM_CR_STALL_PROCESSOR_Msk) ///< ETMCR: enter programming mode
#define ETM_CR_PROG_VALUE                                                            \
  (ETM_CR_BRANCH_OUTPUT_Msk | ETM_CR_DEBUG_REQUEST_CTRL_Msk | ETM_CR_PROGRAMMING_Msk \
   | ETM_CR_STALL_PROCESSOR_Msk) ///< ETMCR: program while still in programming mode
#define ETM_CR_ENABLE_VALUE                                 \
  (ETM_CR_BRANCH_OUTPUT_Msk | ETM_CR_DEBUG_REQUEST_CTRL_Msk \
   | ETM_CR_STALL_PROCESSOR_Msk) ///< ETMCR: clear programming bit to enable tracing

#define ETM_TECR1_EXCLUDE_NONE (0UL)                    ///< TECR1: include all address ranges
#define ETM_TECR1_VALUE        (ETM_TECR1_EXCLUDE_NONE) ///< TECR1 init value
#define ETM_TEEVR_ALWAYS_TRUE  (0x6FUL)                 ///< TEEVR: TraceEnable event always true
#define ETM_TEEVR_VALUE        (ETM_TEEVR_ALWAYS_TRUE)  ///< TEEVR init value
#define ETM_TRACE_ID           (2UL)                    ///< ATB TRACEIDR value for the ETM stream
#define ETM_TRACEIDR_VALUE     (ETM_TRACE_ID)           ///< TRACEIDR init value

/* TPIU */
#define ETM_TPIU_PORT_SIZE_4BIT (1UL << 3)                ///< CSPSR one-hot select for 4-bit port
#define ETM_TPIU_CSPSR_VALUE    (ETM_TPIU_PORT_SIZE_4BIT) ///< TPIU CSPSR init value
#define ETM_TPIU_ACPR_NO_DIVIDE (0UL)                     ///< ACPR: bypass async clock divider
#define ETM_TPIU_ACPR_VALUE     (ETM_TPIU_ACPR_NO_DIVIDE) ///< TPIU ACPR init value
#define ETM_TPIU_SPPR_PARALLEL  (0UL)                     ///< SPPR TXMODE: parallel trace port
#define ETM_TPIU_SPPR_VALUE     (ETM_TPIU_SPPR_PARALLEL)  ///< TPIU SPPR init value
#define ETM_TPIU_FFCR_VALUE     (TPIU_FFCR_EnFCont_Msk)   ///< FFCR: enable continuous formatting

/* ITM */
#define ETM_ITM_TRACE_BUS_ID (1UL) ///< ITM ATB trace bus ID
#define ETM_ITM_TCR_INIT_VALUE                                                                 \
  ((ETM_ITM_TRACE_BUS_ID << ITM_TCR_TRACEBUSID_Pos) | ITM_TCR_DWTENA_Msk | ITM_TCR_SYNCENA_Msk \
   | ITM_TCR_ITMENA_Msk) ///< ITM TCR: enable ITM/DWT/sync before timestamps
#define ETM_ITM_TCR_FINAL_VALUE                                                                                    \
  ((ETM_ITM_TRACE_BUS_ID << ITM_TCR_TRACEBUSID_Pos) | ITM_TCR_DWTENA_Msk | ITM_TCR_SYNCENA_Msk | ITM_TCR_TSENA_Msk \
   | ITM_TCR_ITMENA_Msk)                                  ///< ITM TCR: final enable including timestamps
#define ETM_ITM_TER_ALL_PORTS   (0xFFFFFFFFUL)            ///< TER: enable all stimulus ports
#define ETM_ITM_TER_VALUE       (ETM_ITM_TER_ALL_PORTS)   ///< ITM TER init value
#define ETM_ITM_TPR_UNPRIV_MASK (0x8UL)                   ///< TPR unprivileged access mask
#define ETM_ITM_TPR_VALUE       (ETM_ITM_TPR_UNPRIV_MASK) ///< ITM TPR init value

/* DWT */
#define ETM_DWT_POSTPRESET (0xFUL) ///< DWT CTRL POSTPRESET field value
#define ETM_DWT_SYNCTAP    (1UL)   ///< DWT CTRL SYNCTAP field value
#define ETM_DWT_CTRL_COMMON                                                                                            \
  (DWT_CTRL_FOLDEVTENA_Msk | DWT_CTRL_LSUEVTENA_Msk | DWT_CTRL_SLEEPEVTENA_Msk | DWT_CTRL_EXCEVTENA_Msk                \
   | DWT_CTRL_CPIEVTENA_Msk | DWT_CTRL_EXCTRCENA_Msk | (ETM_DWT_SYNCTAP << DWT_CTRL_SYNCTAP_Pos) | DWT_CTRL_CYCTAP_Msk \
   | (ETM_DWT_POSTPRESET << DWT_CTRL_POSTPRESET_Pos)                                                                   \
   | DWT_CTRL_CYCCNTENA_Msk)                           ///< DWT CTRL common event/cycle enables
#define ETM_DWT_CTRL_INIT_VALUE  (ETM_DWT_CTRL_COMMON) ///< DWT CTRL before enabling cycle events
#define ETM_DWT_CTRL_FINAL_VALUE (ETM_DWT_CTRL_COMMON | DWT_CTRL_CYCEVTENA_Msk) ///< DWT CTRL with cycle events enabled
#define ETM_DWT_COUNTER_CLEAR    (0UL)                    ///< Value used to clear DWT performance counters
#define ETM_DWT_CPICNT_VALUE     (ETM_DWT_COUNTER_CLEAR)  ///< CPICNT clear value
#define ETM_DWT_EXCCNT_VALUE     (ETM_DWT_COUNTER_CLEAR)  ///< EXCCNT clear value
#define ETM_DWT_SLEEPCNT_VALUE   (ETM_DWT_COUNTER_CLEAR)  ///< SLEEPCNT clear value
#define ETM_DWT_LSUCNT_VALUE     (ETM_DWT_COUNTER_CLEAR)  ///< LSUCNT clear value
#define ETM_DWT_FOLDCNT_VALUE    (ETM_DWT_COUNTER_CLEAR)  ///< FOLDCNT clear value
#define ETM_DWT_CYCCNT_PRELOAD   (0x3FFUL)                ///< CYCCNT preload value
#define ETM_DWT_CYCCNT_VALUE     (ETM_DWT_CYCCNT_PRELOAD) ///< CYCCNT init value
/*******************************************************************************
 *************************   ETM Register Structure  ***************************
 ******************************************************************************/

/***************************************************************************/ /**
 * @brief Minimal ETM register map (base @ref ETM_BASE).
 *
 * @details CMSIS does not provide an ETM_Type definition for this device.
 *          RESERVED2 is sized so that LAR remains at the CoreSight-documented
 *          offset 0xFB0.
 ******************************************************************************/
typedef struct {
  __IOM uint32_t CR; /**< 0x000 ETM Control */
  uint32_t RESERVED0[7];
  __IOM uint32_t TEEVR; /**< 0x020 TraceEnable Event */
  __IOM uint32_t TECR1; /**< 0x024 TraceEnable Control 1 */
  uint32_t RESERVED1[118];
  __IOM uint32_t TRACEIDR; /**< 0x200 Trace ID */
  uint32_t RESERVED2[875];
  __OM uint32_t LAR; /**< 0xFB0 Lock Access */
} ETM_Type;

#define ETM ((ETM_Type *)ETM_BASE) ///< ETM register block pointer

/*******************************************************************************
 **************************   ETM Function Declaration  ************************
 ******************************************************************************/

/***************************************************************************/ /**
 * @brief Initializes the Embedded Trace Macrocell (ETM) and related CoreSight blocks.
 *
 * @details Configures the SOC PLL / M4 core clock for ETM operation (default 120 MHz),
 *          muxes MCU clock-out and TRACE clock/data pins, then programs CoreDebug,
 *          ETM, TPIU, ITM, and DWT for 4-bit parallel instruction tracing.
 *
 * @return Status code indicating the result:
 *         - SL_STATUS_OK  - Success.
 *         - Corresponding error code on failure (for example, clock configuration).
 *
 * @note The ETM operating frequency on Si917 must be at least 40 MHz.
 *
 * For more information on status codes, refer to
 * [SL STATUS DOCUMENTATION](https://docs.silabs.com/gecko-platform/latest/platform-common/status).
 ******************************************************************************/
void sl_si91x_etm_int(void);

/** @} (end addtogroup ETM) */

#endif /* ETM_H_*/
