/*******************************************************************************
 * @file  sdio_secondary_mode_freertos.c
 * @brief SDIO secondary FreeRTOS example
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
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
/**===========================================================================
 * @brief This file contains application code for an SDIO secondary device.
 * @section Description
 * This example demonstrates data transfer through SDIO. The device acts as a
 * secondary which interfaces with an external SDIO host/master, running as a
 * dedicated FreeRTOS task using CMSIS-RTOS2 APIs.
 *
 * When @ref SL_SIWX91X_RASPBERRY_PI_HANDSHAKE_ENABLE is set to 1, a GPIO
 * handshake with the Raspberry Pi host runs after SDIO initialization and
 * before data transfer begins. Install the GPIO component (`sl_gpio`)
 * in the project before enabling that flag.
 ========================================================================================**/
#include "sdio_secondary_mode_freertos.h"
#include "UDMA.h"
#include "sl_si91x_sdio_secondary_drv_config.h"
#include "sl_si91x_peripheral_sdio_secondary.h"
#include "rsi_debug.h"
#include "rsi_rom_clks.h"
#include "cmsis_os2.h"

#if SL_SIWX91X_RASPBERRY_PI_HANDSHAKE_ENABLE
#include "sl_si91x_driver_gpio.h"
#include "sl_gpio_board.h"
#endif
/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
#define BLOCK_LEN        256
#define NO_OF_BLOCKS     4
#define XFER_BUFFER_SIZE (BLOCK_LEN * NO_OF_BLOCKS) // Buffer size is 256B*4 = 1KB

#define SW_CORE_CLK 1

#if SW_CORE_CLK
#define ICACHE2_ADDR_TRANSLATE_1_REG *(volatile uint32_t *)(0x20280000 + 0x24)
#ifndef MISC_CFG_SRAM_REDUNDANCY_CTRL
#define MISC_CFG_SRAM_REDUNDANCY_CTRL *(volatile uint32_t *)(M4_MISC_CONFIG_BASE + 0x18)
#endif // MISC_CFG_SRAM_REDUNDANCY_CTRL
#ifndef MISC_CONFIG_MISC_CTRL1
#define MISC_CONFIG_MISC_CTRL1 *(volatile uint32_t *)(M4_MISC_CONFIG_BASE + 0x44)
#endif // MISC_CONFIG_MISC_CTRL1
#define MISC_QUASI_SYNC_MODE  *(volatile uint32_t *)(M4_MISC_CONFIG_BASE + 0x84)
#define SOC_PLL_REF_FREQUENCY 40000000  // PLL input REFERENCE clock 40MHZ
#define PS4_SOC_FREQ          150000000 // PLL out clock 150MHz
#endif                                  // SW_CORE_CLK

/*******************************************************************************
 ******************************  Data Types  ***********************************
 ******************************************************************************/

typedef enum {
  SEND_DATA,
  RECEIVE_DATA,
  TRANSMISSION_COMPLETED,
} sdio_mode_enum_t;

// Change to SEND_DATA to send data to the host instead of receiving
static sdio_mode_enum_t current_mode = RECEIVE_DATA;

/*******************************************************************************
 ***************************   LOCAL VARIABLES   *******************************
 ******************************************************************************/

static uint8_t xfer_buffer[XFER_BUFFER_SIZE];

static osSemaphoreId_t host_intr_sem = NULL;
static osSemaphoreId_t dma_done_sem  = NULL;

static boolean_t send_data_flag    = true;
static boolean_t receive_data_flag = true;

static uint32_t tt_start     = 0;
static uint32_t packet_count = 0;

/*******************************************************************************
 **********************  Local Function prototypes   ***************************
 ******************************************************************************/
static void sdio_secondary_mode_task(void *argument);
static sl_status_t sdio_secondary_init_function(void);
static void application_callback(uint8_t events);
static void gpdma_callback(uint8_t dma_ch);

static const osThreadAttr_t sdio_secondary_mode_thread_attributes = {
  .name       = "sdio_task",
  .stack_size = 2048,
  .priority   = osPriorityLow1,
};

/*******************************************************************************
 ******************************   CALLBACKS   **********************************
 ******************************************************************************/

/***************************************************************************/ /**
 * SDIO secondary host interrupt event callback.
 *
 * Releases @p host_intr_sem on receive/send events and unmasks SDIO
 * interrupts on CMD52.
 *
 * @param[in] events  Bitmask of SDIO host interrupt events.
 *
 * @return None.
 ******************************************************************************/
static void application_callback(uint8_t events)
{
  if (events & HOST_INTR_RECEIVE_EVENT) {
    if (host_intr_sem != NULL) {
      (void)osSemaphoreRelease(host_intr_sem);
    }
  }

  if (events & HOST_INTR_SEND_EVENT) {
    if (host_intr_sem != NULL) {
      (void)osSemaphoreRelease(host_intr_sem);
    }
  }

  if (events & HOST_INTR_CMD52_EVENT) {
    sl_si91x_sdio_secondary_set_interrupts(SL_SDIO_WR_INT_UNMSK | SL_SDIO_RD_INT_UNMSK | SL_SDIO_CMD52_INT_UNMSK);
  }
}

/***************************************************************************/ /**
 * GPDMA transfer-complete callback.
 *
 * Releases @p dma_done_sem when a DMA transfer finishes.
 *
 * @param[in] dma_ch  DMA channel number (unused).
 *
 * @return None.
 ******************************************************************************/
static void gpdma_callback(uint8_t dma_ch)
{
  UNUSED_PARAMETER(dma_ch);
  if (dma_done_sem != NULL) {
    (void)osSemaphoreRelease(dma_done_sem);
  }
}

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************/ /**
 * SDIO Secondary example initialization function.
 *
 * Creates a FreeRTOS task that handles SDIO configuration and data transfer.
 *
 * @param[in] None
 *
 * @return None
 ******************************************************************************/
void sdio_secondary_mode_example_init(void)
{
  osThreadId_t thread_id =
    osThreadNew((osThreadFunc_t)sdio_secondary_mode_task, NULL, &sdio_secondary_mode_thread_attributes);
  if (thread_id == NULL) {
    /* Note: All status messages in this example — both success and failure — are
 * intentionally emitted via SL_PRINT_STRING_ERROR so that they remain visible
 * on the console at the default log level. This is a demonstration choice, not
 * a recommendation: in production code, ERROR severity should be reserved for
 * actual failures, with successful operations logged via SL_PRINT_STRING_INFO
 * (or SL_PRINT_STRING_DEBUG for verbose trace). */
    SL_PRINT_STRING_ERROR("Failed to create SDIO secondary thread\r\n");
    return;
  }
}

/***************************************************************************/ /**
 * Initializes SDIO secondary resources used by the example task.
 *
 * Creates host-interrupt and DMA-done semaphores, and registers the SDIO and
 * GPDMA event callbacks.
 *
 * @param[in] None
 *
 * @return Status code of the operation:
 *         - SL_STATUS_OK               - Success
 *         - SL_STATUS_ALLOCATION_FAILED - Semaphore creation failed
 *         - Other                      - Callback registration failure
 ******************************************************************************/
static sl_status_t sdio_secondary_init_function(void)
{
  sl_status_t status;

  host_intr_sem = osSemaphoreNew(1U, 0U, NULL);
  dma_done_sem  = osSemaphoreNew(1U, 0U, NULL);
  if (host_intr_sem == NULL || dma_done_sem == NULL) {
    SL_PRINT_STRING_ERROR("Failed to create SDIO semaphores\r\n");
    return SL_STATUS_ALLOCATION_FAILED;
  }

  status =
    sl_si91x_sdio_secondary_register_event_callback(application_callback,
                                                    SL_SDIO_WR_INT_EN | SL_SDIO_RD_INT_EN | SL_SDIO_CMD52_INT_EN);
  if (status != SL_STATUS_OK) {
    SL_PRINT_STRING_ERROR("\r\nSDIO Secondary callback function registration failed\r\n");
    return status;
  }
  SL_PRINT_STRING_ERROR("\r\nSDIO Secondary callback function registration success\r\n");

  sl_si91x_sdio_secondary_gpdma_register_event_callback(gpdma_callback);

  return SL_STATUS_OK;
}

#if SL_SIWX91X_RASPBERRY_PI_HANDSHAKE_ENABLE
/// Maximum time (ms) to wait for Raspberry Pi HOST_ACK during handshake.
#define SLI_HANDSHAKE_TIMEOUT_MS 30000
/// Duration (ms) DEVICE_READY is held low before asserting high.
#define SLI_DEVICE_READY_HOLD_MS 1000
/// Debounce interval (ms) between consecutive HOST_ACK samples.
#define SLI_HANDSHAKE_DEBOUNCE_MS 500

/// GPIO configuration for DEVICE_READY (UULP_GPIO_0, output to host).
static sl_si91x_gpio_pin_config_t device_ready_gpio_config = { .port_pin  = { .port = SL_SI91X_UULP_GPIO_0_PORT,
                                                                              .pin  = SL_SI91X_UULP_GPIO_0_PIN },
                                                               .direction = GPIO_OUTPUT };
/// GPIO configuration for HOST_ACK (UULP_GPIO_2, input from host).
static sl_si91x_gpio_pin_config_t host_ack_gpio_config = { .port_pin  = { .port = SL_SI91X_UULP_GPIO_2_PORT,
                                                                          .pin  = SL_SI91X_UULP_GPIO_2_PIN },
                                                           .direction = GPIO_INPUT };

/// First HOST_ACK sample used for debounce validation.
static uint8_t host_ack_gpio_value1 = 0;
/// Second HOST_ACK sample used for debounce validation.
static uint8_t host_ack_gpio_value2 = 0;

/***************************************************************************/ /**
 * Busy-waits for the specified duration using CMSIS-RTOS2 kernel ticks.
 *
 * This helper intentionally avoids @p osDelay so the task does not yield and
 * the M4 core does not enter idle/sleep during the handshake sequence.
 *
 * @pre Kernel tick interrupt must be running (@p configTICK_RATE_HZ = 1000).
 *
 * @param[in] delay_ms  Delay duration in milliseconds (1 tick = 1 ms).
 *
 * @return None
 ******************************************************************************/
static void handshake_busy_wait_ms(uint32_t delay_ms)
{
  uint32_t start = osKernelGetTickCount();
  // Spin until the kernel tick advances; do not osDelay (M4 must not sleep).
  while ((osKernelGetTickCount() - start) < delay_ms) {
  }
}

/***************************************************************************/ /**
 * Configures selected unused ULP GPIOs as inputs for Raspberry Pi SDIO use.
 *
 * Leaves VCOM UART pins (ULP_GPIO_9 / ULP_GPIO_11) unchanged so console
 * logging continues to operate.
 *
 * @param[in] None
 *
 * @return Status code of the operation:
 *         - SL_STATUS_OK  - All configured pins set successfully
 *         - Other         - GPIO configuration failure for a pin
 ******************************************************************************/
static sl_status_t set_soc_gpio_input_mode(void)
{
  sl_status_t status = SL_STATUS_OK;
  // set the ULP pins as input
  sl_si91x_gpio_pin_config_t ulp_gpio6_config  = { .port_pin  = { .port = SL_SI91X_ULP_GPIO_6_PORT,
                                                                  .pin  = SL_SI91X_ULP_GPIO_6_PIN },
                                                   .direction = GPIO_INPUT };
  sl_si91x_gpio_pin_config_t ulp_gpio7_config  = { .port_pin  = { .port = SL_SI91X_ULP_GPIO_7_PORT,
                                                                  .pin  = SL_SI91X_ULP_GPIO_7_PIN },
                                                   .direction = GPIO_INPUT };
  sl_si91x_gpio_pin_config_t ulp_gpio2_config  = { .port_pin  = { .port = SL_SI91X_ULP_GPIO_2_PORT,
                                                                  .pin  = SL_SI91X_ULP_GPIO_2_PIN },
                                                   .direction = GPIO_INPUT };
  sl_si91x_gpio_pin_config_t ulp_gpio10_config = { .port_pin  = { .port = SL_SI91X_ULP_GPIO_10_PORT,
                                                                  .pin  = SL_SI91X_ULP_GPIO_10_PIN },
                                                   .direction = GPIO_INPUT };
  sl_si91x_gpio_pin_config_t ulp_gpio8_config  = { .port_pin  = { .port = SL_SI91X_ULP_GPIO_8_PORT,
                                                                  .pin  = SL_SI91X_ULP_GPIO_8_PIN },
                                                   .direction = GPIO_INPUT };

  status = sl_gpio_set_configuration(ulp_gpio6_config);
  if (status != SL_STATUS_OK) {
    SL_PRINT_STRING_ERROR("Failed to configure the ULP_GPIO6\r\n");
    return status;
  }
  status = sl_gpio_set_configuration(ulp_gpio7_config);
  if (status != SL_STATUS_OK) {
    SL_PRINT_STRING_ERROR("Failed to configure the ULP_GPIO7\r\n");
    return status;
  }
  status = sl_gpio_set_configuration(ulp_gpio2_config);
  if (status != SL_STATUS_OK) {
    SL_PRINT_STRING_ERROR("Failed to configure the ULP_GPIO2\r\n");
    return status;
  }
  status = sl_gpio_set_configuration(ulp_gpio10_config);
  if (status != SL_STATUS_OK) {
    SL_PRINT_STRING_ERROR("Failed to configure the ULP_GPIO10\r\n");
    return status;
  }
  status = sl_gpio_set_configuration(ulp_gpio8_config);
  if (status != SL_STATUS_OK) {
    SL_PRINT_STRING_ERROR("Failed to configure the ULP_GPIO8\r\n");
    return status;
  }

  return SL_STATUS_OK;
}

/***************************************************************************/ /**
 * Performs GPIO handshake with the Raspberry Pi SDIO host.
 *
 * Sequence:
 * 1. Initialize GPIO and configure DEVICE_READY (UULP_GPIO_0) as output and
 *    HOST_ACK (UULP_GPIO_2) as input.
 * 2. Configure selected unused ULP pins as inputs.
 * 3. Drive DEVICE_READY low for @ref SLI_DEVICE_READY_HOLD_MS, then assert high
 *    to indicate the SiWx917 is ready for SDIO communication.
 * 4. Poll HOST_ACK until it remains high across two samples separated by
 *    @ref SLI_HANDSHAKE_DEBOUNCE_MS, or until @ref SLI_HANDSHAKE_TIMEOUT_MS
 *    expires. On timeout, DEVICE_READY is driven low before returning.
 *
 * @note The Raspberry Pi should drive HOST_ACK low first, wait for DEVICE_READY
 *       high (stable at least 500 ms), then drive HOST_ACK high.
 * @note Uses busy-wait delays instead of @p osDelay to keep M4 from entering
 *       idle/sleep during handshake.
 *
 * @param[in] None
 *
 * @return Status code of the operation:
 *         - SL_STATUS_OK      - Handshake completed successfully
 *         - SL_STATUS_TIMEOUT - HOST_ACK not received within timeout
 *         - Other             - GPIO initialization or configuration failure
 ******************************************************************************/
static sl_status_t sdio_host_handshake(void)
{
  sl_status_t status       = SL_STATUS_OK;
  uint32_t handshake_start = 0;

  status = sl_gpio_driver_init();
  if (status != SL_STATUS_OK) {
    SL_PRINT_STRING_ERROR("Failed to initialize the GPIO driver\r\n");
    return status;
  }
  status = sl_gpio_set_configuration(device_ready_gpio_config);
  if (status != SL_STATUS_OK) {
    SL_PRINT_STRING_ERROR("Failed to configure the GPIO\r\n");
    return status;
  }

  status = sl_gpio_set_configuration(host_ack_gpio_config);
  if (status != SL_STATUS_OK) {
    SL_PRINT_STRING_ERROR("Failed to configure the GPIO\r\n");
    return status;
  }

  status = set_soc_gpio_input_mode();
  if (status != SL_STATUS_OK) {
    SL_PRINT_STRING_ERROR("Failed to configure ULP GPIOs as input\r\n");
    return status;
  }

  sl_gpio_driver_clear_pin(&device_ready_gpio_config.port_pin);
  handshake_busy_wait_ms(SLI_DEVICE_READY_HOLD_MS);
  sl_gpio_driver_set_pin(&device_ready_gpio_config.port_pin);
  // Host should treat DEVICE_READY high as valid only after >= 500 ms.

  SL_PRINT_STRING_ERROR("SL917 SDIO CONFIGURATION COMPLETED\r\n");
  SL_PRINT_STRING_ERROR("SL917 GPIO0 HIGH: Waiting for SDIO handshake from host\r\n");

  handshake_start = osKernelGetTickCount();
  while ((osKernelGetTickCount() - handshake_start) < SLI_HANDSHAKE_TIMEOUT_MS) {
    sl_gpio_driver_get_pin(&host_ack_gpio_config.port_pin, &host_ack_gpio_value1);
    handshake_busy_wait_ms(SLI_HANDSHAKE_DEBOUNCE_MS);
    sl_gpio_driver_get_pin(&host_ack_gpio_config.port_pin, &host_ack_gpio_value2);
    if (host_ack_gpio_value1 == 1 && host_ack_gpio_value2 == 1) {
      SL_PRINT_STRING_ERROR("handshake completed successfully\r\n");
      return SL_STATUS_OK;
    }
    SL_PRINT_STRING_ERROR("Waiting for handshake from Raspberry pi...\r\n");
  }

  SL_PRINT_STRING_ERROR("SDIO handshake timed out after %u ms\r\n", (unsigned)SLI_HANDSHAKE_TIMEOUT_MS);
  sl_gpio_driver_clear_pin(&device_ready_gpio_config.port_pin);
  return SL_STATUS_TIMEOUT;
}

#endif // SL_SIWX91X_RASPBERRY_PI_HANDSHAKE_ENABLE

/***************************************************************************/ /**
 * SDIO secondary FreeRTOS task entry point.
 *
 * Initializes SDIO hardware, optionally completes Raspberry Pi GPIO handshake
 * when @ref SL_SIWX91X_RASPBERRY_PI_HANDSHAKE_ENABLE is 1 (install the GPIO
 * component `sl_gpio` in the project), then runs the send/receive state
 * machine, blocking on semaphores for ISR events.
 *
 * @param[in] argument  Unused task argument (pass NULL).
 *
 * @return None
 ******************************************************************************/
static void sdio_secondary_mode_task(void *argument)
{
  (void)argument;

  sl_status_t status = sdio_secondary_init_function();
  if (status != SL_STATUS_OK) {
    SL_PRINT_STRING_ERROR("SDIO Secondary init failed, exiting task\r\n");
    osThreadExit();
  }

#if SL_SIWX91X_RASPBERRY_PI_HANDSHAKE_ENABLE
  status = sdio_host_handshake();
  if (status != SL_STATUS_OK) {
    SL_PRINT_STRING_ERROR("SDIO handshake failed, exiting task\r\n");
    osThreadExit();
  }
#endif

  tt_start = osKernelGetTickCount();

  while (1) {
    uint32_t tt_end;
    uint32_t throughput;

    switch (current_mode) {
      case RECEIVE_DATA:
        if (receive_data_flag) {
          sl_si91x_sdio_secondary_receive(xfer_buffer);
          packet_count++;
          receive_data_flag = false;
        }

        if (osSemaphoreAcquire(host_intr_sem, osWaitForever) == osOK
            && osSemaphoreAcquire(dma_done_sem, osWaitForever) == osOK) {
          receive_data_flag = true;

          if ((osKernelGetTickCount() - tt_start) >= 2000) {
            tt_end = osKernelGetTickCount();
            SL_PRINT_STRING_ERROR("Data is received from host->Secondary successfully \n");
            SL_PRINT_STRING_ERROR("\r\nPackets received: %lu\r\n", (unsigned long)packet_count);
            SL_PRINT_STRING_ERROR("Total bits received: %lu \r\n",
                                  (unsigned long)(packet_count * XFER_BUFFER_SIZE * 8));
            SL_PRINT_STRING_ERROR("Time diff: %lu ms\r\n", (unsigned long)(tt_end - tt_start));

            throughput = (packet_count * XFER_BUFFER_SIZE * 8) / ((tt_end - tt_start) / 1000);
            SL_PRINT_STRING_ERROR("Throughput host->secondary = %ld bps \r\n", throughput);

            packet_count = 0;
            tt_start     = osKernelGetTickCount();
          }
          memset(xfer_buffer, 0, XFER_BUFFER_SIZE);
        }
        break;

      case SEND_DATA:
        if (send_data_flag) {
          for (int i = 0; i < XFER_BUFFER_SIZE; i++) {
            xfer_buffer[i] = (uint8_t)(i / 256) + 1;
          }
          sl_si91x_sdio_secondary_send(NO_OF_BLOCKS, xfer_buffer);
          packet_count++;
          send_data_flag = false;
        }

        if (osSemaphoreAcquire(dma_done_sem, osWaitForever) == osOK) {
          SL_PRINT_STRING_ERROR("Data is transferred from secondary to host successfully \n");
          send_data_flag = true;

          if ((osKernelGetTickCount() - tt_start) >= 2000) {
            tt_end = osKernelGetTickCount();

            SL_PRINT_STRING_ERROR("\r\nPackets sent: %lu\r\n", (unsigned long)packet_count);
            SL_PRINT_STRING_ERROR("Total bits sent: %lu \r\n", (unsigned long)(packet_count * XFER_BUFFER_SIZE * 8));
            SL_PRINT_STRING_ERROR("Time diff: %lu ms \r\n", (unsigned long)(tt_end - tt_start));

            throughput = (packet_count * XFER_BUFFER_SIZE * 8) / ((tt_end - tt_start) / 1000);
            SL_PRINT_STRING_ERROR("Throughput for secondary->host = %lu bps \r\n", (unsigned long)throughput);

            packet_count = 0;
            tt_start     = osKernelGetTickCount();
          }
        }
        break;

      case TRANSMISSION_COMPLETED:
        SL_PRINT_STRING_ERROR("SDIO Secondary transmission completed, exiting task\r\n");
        osThreadExit();
        break;

      default:
        break;
    }
  }
}
