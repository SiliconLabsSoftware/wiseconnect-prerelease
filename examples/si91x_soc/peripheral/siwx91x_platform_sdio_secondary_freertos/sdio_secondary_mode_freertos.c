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
#define BLOCK_LEN    256
#define NO_OF_BLOCKS 4

// Fixed frame size on the bus, in bytes. The receive DMA is always armed for exactly this
// many bytes, so it must equal the host's block_len * block_count. Arming for more than
// the host sends means the transfer never completes; arming for less corrupts the
// following packet. It must also be a non-zero multiple of 16.
//
// This size is deliberately constant rather than following the host's payload size. SDIO
// block mode pads every partial block out to the configured block size anyway, so a
// receiver always has to be told how much of a frame is real data. Carrying that length
// inside the frame keeps the wire size fixed, which means a change of payload size on the
// host needs no change here.

#define XFER_BUFFER_SIZE (BLOCK_LEN * NO_OF_BLOCKS) // Buffer size is 256B*4 = 1KB

#if ((XFER_BUFFER_SIZE == 0) || ((XFER_BUFFER_SIZE % 16) != 0))
#error "XFER_BUFFER_SIZE must be a non-zero multiple of 16"
#endif

// Frame layout, by agreement with the host:
//   [0..1] payload length in bytes, little endian, excluding this header
//   [2..3] the same length inverted, as a check that this really is a header
//   [4..]  payload, then padding out to XFER_BUFFER_SIZE
//
// The inverted copy matters because block-mode padding is usually zeros: a zero-filled
// frame yields length 0 with an inverted field of 0x0000 rather than 0xFFFF, and is
// rejected instead of being taken as a valid empty frame.
//
// Set this to 1 once the host prepends the header described above, which is what lets the
// host change payload size without a change here. It is 0 by default so the example works
// against a host that sends raw data: the whole frame is then treated as payload, and the
// host's payload size must match XFER_BUFFER_SIZE exactly.
#define RX_FRAME_HEADER_ENABLE 0
#define RX_FRAME_HEADER_SIZE   4

#if RX_FRAME_HEADER_ENABLE
#define RX_MAX_PAYLOAD (XFER_BUFFER_SIZE - RX_FRAME_HEADER_SIZE)
#else
#define RX_MAX_PAYLOAD XFER_BUFFER_SIZE
#endif

// Receive buffers are used in ping-pong: one is armed on the FIFO while the task
// processes the other. Two is the minimum that keeps a buffer always armed.
#define RX_BUFFER_COUNT 2

// How often the reporting task prints receive statistics, in milliseconds. This only
// affects how often the numbers appear; reception itself is continuous.
#define STATS_INTERVAL_MS 2000

// GPDMA channel the driver uses for reception.
#define RX_DMA_CHANNEL 1

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

// Backed by uint32_t so the buffers are 4-byte aligned on every toolchain: the SDIO
// receive path reads the write FIFO as 32-bit words and rejects unaligned buffers.
static uint32_t xfer_buffer_words[XFER_BUFFER_SIZE / sizeof(uint32_t)];
static uint8_t *const xfer_buffer = (uint8_t *)xfer_buffer_words;

static uint32_t rx_buffer_words[RX_BUFFER_COUNT][XFER_BUFFER_SIZE / sizeof(uint32_t)];
#if (RX_BUFFER_COUNT != 2)
#error "The rx_buffer initialiser lists two entries; extend it if RX_BUFFER_COUNT changes"
#endif
static uint8_t *const rx_buffer[RX_BUFFER_COUNT] = { (uint8_t *)rx_buffer_words[0], (uint8_t *)rx_buffer_words[1] };

// Buffer currently armed on the FIFO, and the one holding the most recent packet.
static volatile uint8_t rx_arming_index = 0;
static volatile uint8_t rx_filled_index = 0;

// Statistics counters. The first two are maintained by the ISR and the last two by the
// receive task; all four are read by the reporting task.
//
// They only ever increment, and are never reset. The reporting task keeps its own
// previous values and prints the difference, which means no writer ever has to
// read-modify-write a counter another context is updating. Each is a single aligned word,
// so a read cannot tear, and unsigned subtraction gives the right delta even across a
// 32-bit wrap.
static volatile uint32_t rx_packets       = 0;
static volatile uint32_t rx_errors        = 0;
static volatile uint32_t rx_bad_frames    = 0;
static volatile uint32_t rx_payload_bytes = 0;

static osSemaphoreId_t host_intr_sem = NULL;
static osSemaphoreId_t dma_done_sem  = NULL;

static bool send_data_flag    = true;
static bool receive_data_flag = true;

// Set from the SDIO callback when an error event aborts a transfer.
static volatile bool transfer_error = false;

static uint32_t tt_start     = 0;
static uint32_t packet_count = 0;

/*******************************************************************************
 **********************  Local Function prototypes   ***************************
 ******************************************************************************/
static void sdio_secondary_mode_task(void *argument);
static void sdio_stats_task(void *argument);
static sl_status_t sdio_secondary_init_function(void);
static void application_callback(uint8_t events);
static void gpdma_callback(uint8_t dma_ch);

static const osThreadAttr_t sdio_secondary_mode_thread_attributes = {
  .name       = "sdio_task",
  .stack_size = 2048,
  .priority   = osPriorityLow1,
};

// Reporting runs below the receive task on purpose. A frame arrives roughly every 100 us
// while this block of statistics takes tens of milliseconds to clear the UART, so
// printing from the receive task would stall it long enough for the DMA to overwrite
// buffers it had not yet processed. At a lower priority the receive task preempts the
// printing and keeps draining frames.
static const osThreadAttr_t sdio_stats_thread_attributes = {
  .name       = "sdio_stats",
  .stack_size = 1024,
  .priority   = osPriorityLow,
};

/*******************************************************************************
 ***************************   LOCAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************/ /**
 * Extracts the payload length from a received frame.
 *
 * Reads the two-byte little endian length and checks it against its inverted
 * copy, which rejects padding being misread as a header.
 *
 * @param[in]  frame           Received frame of XFER_BUFFER_SIZE bytes.
 * @param[out] payload_length  Payload length in bytes on success.
 *
 * @return true if the header is valid and the length fits in the frame.
 ******************************************************************************/
static sl_status_t get_frame_payload_length(const uint8_t *frame, uint16_t *payload_length)
{
#if RX_FRAME_HEADER_ENABLE
  uint16_t length   = (uint16_t)((uint16_t)frame[0] | ((uint16_t)frame[1] << 8));
  uint16_t inverted = (uint16_t)((uint16_t)frame[2] | ((uint16_t)frame[3] << 8));

  if ((uint16_t)(length ^ 0xFFFFu) != inverted) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  if (length > RX_MAX_PAYLOAD) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  *payload_length = length;
  return SL_STATUS_OK;
#else
  (void)frame;
  // Without a header the entire frame is payload.
  *payload_length = (uint16_t)RX_MAX_PAYLOAD;
  return SL_STATUS_OK;
#endif
}

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
    // Only CMD52 is re-enabled here. Unmasking the CMD53 write interrupt from this
    // path would accept a host write with no descriptor chain armed, which is exactly
    // what stalls transfers larger than the FIFO. The driver unmasks it when the
    // application arms the next receive.
    sl_si91x_sdio_secondary_set_interrupts(SL_SDIO_CMD52_INT_UNMSK);
  }

  // A CRC error, host abort or read-FIFO timeout leaves the in-flight transfer
  // incomplete, so no DMA completion follows. Release both semaphores to unblock the
  // task, which then re-arms the transfer.
  if (events & HOST_INTR_ERROR_EVENT) {
    transfer_error = true;

    if (current_mode == RECEIVE_DATA) {
      // An abort part-way through a block can leave bytes in the FIFO, which would be
      // read as the head of the next packet. Discard them, then re-arm immediately so
      // the host's retry has somewhere to land.
      rx_errors++;
      (void)sl_si91x_sdio_secondary_flush_rx_fifo();
      (void)sl_si91x_sdio_secondary_receive_with_length(rx_buffer[rx_arming_index], XFER_BUFFER_SIZE);
    }

    if (host_intr_sem != NULL) {
      (void)osSemaphoreRelease(host_intr_sem);
    }
    if (dma_done_sem != NULL) {
      (void)osSemaphoreRelease(dma_done_sem);
    }
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
  if (dma_ch == RX_DMA_CHANNEL) {
    rx_filled_index = rx_arming_index;
    rx_arming_index = (uint8_t)((rx_arming_index + 1u) % RX_BUFFER_COUNT);

    // Re-arm here rather than in the task. The host can issue its next CMD53 within
    // microseconds of this one completing, and anything it sends while no descriptor
    // chain is armed has nowhere to land, so the host times out and aborts. Arming the
    // alternate buffer from the ISR keeps a target live at all times while the task
    // processes the buffer that just filled.
    (void)sl_si91x_sdio_secondary_receive_with_length(rx_buffer[rx_arming_index], XFER_BUFFER_SIZE);

    rx_packets++;
  }

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

  if (osThreadNew((osThreadFunc_t)sdio_stats_task, NULL, &sdio_stats_thread_attributes) == NULL) {
    // Reporting is optional: reception works without it, so carry on with a warning.
    SL_PRINT_STRING_ERROR("Failed to create SDIO stats thread, statistics will not be reported\r\n");
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
  // Counts up to the number of receive buffers so a completion that lands while the
  // task is still reporting the previous one is not dropped.
  dma_done_sem = osSemaphoreNew(RX_BUFFER_COUNT, 0U, NULL);
  if (host_intr_sem == NULL || dma_done_sem == NULL) {
    SL_PRINT_STRING_ERROR("Failed to create SDIO semaphores\r\n");
    return SL_STATUS_ALLOCATION_FAILED;
  }

  // CRC, abort and read-FIFO-timeout interrupts are enabled as well: when one of these
  // fires the transfer is aborted with no DMA completion to follow, and the SDIO error
  // condition can hold off the DMA data path until the interrupt is cleared.
  status = sl_si91x_sdio_secondary_register_event_callback(application_callback,
                                                           SL_SDIO_WR_INT_EN | SL_SDIO_RD_INT_EN | SL_SDIO_CMD52_INT_EN
                                                             | SL_SDIO_CRC_ERR_INT_EN | SL_SDIO_ABORT_INT_EN
                                                             | SL_SDIO_TOUT_INT_EN);
  if (status != SL_STATUS_OK) {
    SL_PRINT_STRING_ERROR("\r\nSDIO Secondary callback function registration failed\r\n");
    return status;
  }
  SL_PRINT_STRING_ERROR("\r\nSDIO Secondary callback function registration success\r\n");

  sl_si91x_sdio_secondary_gpdma_register_event_callback(gpdma_callback);

  // Registration unmasks every requested source, which would leave the CMD53 write
  // interrupt live before any buffer is armed. Mask it (and clear any stale status)
  // until the first receive is armed; the driver unmasks it from there on.
  sl_si91x_sdio_secondary_clear_interrupts(SL_SDIO_WR_INT_MSK);

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
          // Arm the first buffer only. Every subsequent receive is re-armed from the
          // GPDMA completion callback, so the host always has a target and never has to
          // wait for this task to come back around.
          status = sl_si91x_sdio_secondary_receive_with_length(rx_buffer[rx_arming_index], XFER_BUFFER_SIZE);
          if (status != SL_STATUS_OK) {
            // Only a bad XFER_BUFFER_SIZE or buffer alignment gets here, neither of
            // which is recoverable at runtime.
            SL_PRINT_STRING_ERROR("Failed to arm SDIO receive: 0x%04lX, check XFER_BUFFER_SIZE\r\n",
                                  (unsigned long)status);
            osThreadExit();
          }
          receive_data_flag = false;
        }

        if (osSemaphoreAcquire(dma_done_sem, osWaitForever) == osOK) {
          // The completed packet is in rx_buffer[rx_filled_index]; the alternate buffer
          // is already armed and may be filling right now.

          if (transfer_error) {
            transfer_error = false;
            // No DMA completion followed the abort, so rx_filled_index still refers to the
            // previous packet and there is nothing to decode. The FIFO has been drained and
            // the receive re-armed from the ISR already; the reporting task prints the
            // diagnostics, keeping the UART off this path entirely.
            break;
          }

          // Decode how much of the fixed-size frame is real data. The frame size on the
          // bus never changes, so a payload size change on the host lands here as a
          // different length rather than as a transfer that no longer matches.
          {
            const uint8_t *frame    = rx_buffer[rx_filled_index];
            uint16_t payload_length = 0;
            const uint8_t *payload  = frame + (RX_FRAME_HEADER_ENABLE ? RX_FRAME_HEADER_SIZE : 0);

            if (get_frame_payload_length(frame, &payload_length) == SL_STATUS_OK) {
              rx_payload_bytes += payload_length;
              // payload/payload_length is the application data for this frame.
              (void)payload;
            } else {
              rx_bad_frames++;
            }
          }

          // Nothing else happens here. The loop deliberately contains no printing, so the
          // task returns to the semaphore immediately and is always ready for the next
          // frame; sdio_stats_task() does the reporting.
        }
        break;

      case SEND_DATA:
        if (send_data_flag) {
          for (int i = 0; i < XFER_BUFFER_SIZE; i++) {
            xfer_buffer[i] = (uint8_t)(i % 256);
          }
          sl_si91x_sdio_secondary_send(NO_OF_BLOCKS, xfer_buffer);
          send_data_flag = false;
        }
        // DMA completion and error paths both release dma_done_sem. Verify that the
        // transfer completed successfully before updating statistics or scheduling the
        // next send.
        if (osSemaphoreAcquire(dma_done_sem, osWaitForever) == osOK) {

          if (transfer_error) {
            SL_PRINT_STRING_ERROR("Data transfer failed from secondary to host \n");
            transfer_error = false;
            send_data_flag = true;
            break;
          }

          SL_PRINT_STRING_ERROR("Data is transferred from secondary to host successfully \n");
          send_data_flag = true;
          packet_count++;

          if ((osKernelGetTickCount() - tt_start) >= STATS_INTERVAL_MS) {
            tt_end = osKernelGetTickCount();

            uint32_t elapsed_ms = tt_end - tt_start;
            uint32_t sent_bytes = packet_count * XFER_BUFFER_SIZE;

            SL_PRINT_STRING_ERROR("\r\nPackets sent: %lu\r\n", (unsigned long)packet_count);
            SL_PRINT_STRING_ERROR("Total bytes sent: %lu \r\n", (unsigned long)sent_bytes);
            SL_PRINT_STRING_ERROR("Time diff: %lu ms \r\n", (unsigned long)elapsed_ms);

            throughput = (uint32_t)(((uint64_t)sent_bytes * 1000U) / elapsed_ms);
            SL_PRINT_STRING_ERROR("Throughput for secondary->host = %lu bytes/s \r\n", (unsigned long)throughput);

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

/***************************************************************************/ /**
 * Periodically reports receive statistics.
 *
 * Runs below the receive task so that the time spent writing to the UART never
 * delays frame handling. Reads the monotonic counters and reports the change
 * since the previous interval.
 *
 * @param[in] argument  Unused.
 *
 * @return None.
 ******************************************************************************/
static void sdio_stats_task(void *argument)
{
  (void)argument;

  uint32_t prev_packets = 0;
  uint32_t prev_errors  = 0;
  uint32_t prev_payload = 0;
  uint32_t prev_bad     = 0;
  uint32_t prev_tick    = osKernelGetTickCount();

  while (1) {
    osDelay(STATS_INTERVAL_MS);

    uint32_t packets = rx_packets;
    uint32_t errors  = rx_errors;
    uint32_t payload = rx_payload_bytes;
    uint32_t bad     = rx_bad_frames;
    uint32_t tick    = osKernelGetTickCount();

    uint32_t frames        = packets - prev_packets;
    uint32_t new_errors    = errors - prev_errors;
    uint32_t payload_bytes = payload - prev_payload;
    uint32_t bad_frames    = bad - prev_bad;
    uint32_t elapsed_ms    = tick - prev_tick;

    prev_packets = packets;
    prev_errors  = errors;
    prev_payload = payload;
    prev_bad     = bad;
    prev_tick    = tick;

    if (((frames == 0U) && (new_errors == 0U)) || (elapsed_ms == 0U)) {
      // Idle, or the tick counter has not advanced. Nothing worth reporting.
      continue;
    }

    uint32_t total_bytes = frames * XFER_BUFFER_SIZE;

    SL_PRINT_STRING_ERROR("Data is received from host->Secondary successfully \n");
    SL_PRINT_STRING_ERROR("\r\nFrames received: %lu (errors: %lu, bad headers: %lu)\r\n",
                          (unsigned long)frames,
                          (unsigned long)new_errors,
                          (unsigned long)bad_frames);
    SL_PRINT_STRING_ERROR("Total bytes received: %lu \r\n", (unsigned long)total_bytes);
    SL_PRINT_STRING_ERROR("Payload bytes received: %lu \r\n", (unsigned long)payload_bytes);
    SL_PRINT_STRING_ERROR("Time diff: %lu ms\r\n", (unsigned long)elapsed_ms);

    // Scaling by 1000 before dividing keeps the odd elapsed time accurate; dividing the
    // elapsed time down to whole seconds first would report a 2599 ms window as if it
    // were 2000 ms. The intermediate needs 64 bits: at these rates the byte count alone
    // reaches eight figures, which overflows once multiplied by 1000.
    SL_PRINT_STRING_ERROR("Throughput host->secondary = %lu bytes/s (wire)\r\n",
                          (unsigned long)(((uint64_t)total_bytes * 1000U) / elapsed_ms));
    SL_PRINT_STRING_ERROR("Payload throughput = %lu bytes/s \r\n",
                          (unsigned long)(((uint64_t)payload_bytes * 1000U) / elapsed_ms));

    {
      const uint8_t *buf = rx_buffer[rx_filled_index];
      SL_PRINT_STRING_ERROR("Last RX buffer [%u], %u bytes:\r\n",
                            (unsigned)rx_filled_index,
                            (unsigned)XFER_BUFFER_SIZE);
      for (uint32_t offset = 0; offset < XFER_BUFFER_SIZE; offset += 16U) {
        SL_PRINT_STRING_ERROR("%4lu: %3u %3u %3u %3u %3u %3u %3u %3u %3u %3u %3u %3u %3u %3u %3u %3u\r\n",
                              (unsigned long)offset,
                              (unsigned)buf[offset + 0],
                              (unsigned)buf[offset + 1],
                              (unsigned)buf[offset + 2],
                              (unsigned)buf[offset + 3],
                              (unsigned)buf[offset + 4],
                              (unsigned)buf[offset + 5],
                              (unsigned)buf[offset + 6],
                              (unsigned)buf[offset + 7],
                              (unsigned)buf[offset + 8],
                              (unsigned)buf[offset + 9],
                              (unsigned)buf[offset + 10],
                              (unsigned)buf[offset + 11],
                              (unsigned)buf[offset + 12],
                              (unsigned)buf[offset + 13],
                              (unsigned)buf[offset + 14],
                              (unsigned)buf[offset + 15]);
      }
    }

    if (new_errors != 0U) {
      // Only the most recent error is retained, which is enough to identify the cause;
      // the count above shows how many occurred in the interval.
      sl_sdio_secondary_error_info_t error_info = { 0 };
      if (sl_si91x_sdio_secondary_get_last_error(&error_info) == SL_STATUS_OK) {
        SL_PRINT_STRING_ERROR("SDIO receive aborted: intr=0x%02lX crc=%u abort=%u tout=%u\r\n",
                              (unsigned long)error_info.intr_status,
                              (unsigned)((error_info.intr_status & SL_SDIO_CRC_ERR_INT_MSK) ? 1U : 0U),
                              (unsigned)((error_info.intr_status & SL_SDIO_ABORT_INT_MSK) ? 1U : 0U),
                              (unsigned)((error_info.intr_status & SL_SDIO_TOUT_INT_MSK) ? 1U : 0U));
        SL_PRINT_STRING_ERROR("  stopped at byte %u of %u, block %u, dma_ch=%u\r\n",
                              (unsigned)error_info.error_byte_count,
                              (unsigned)XFER_BUFFER_SIZE,
                              (unsigned)error_info.error_block_count,
                              (unsigned)error_info.dma_channel);
        SL_PRINT_STRING_ERROR("  fifo_status=0x%03lX fifo_occ=0x%04lX\r\n",
                              (unsigned long)error_info.fifo_status,
                              (unsigned long)error_info.fifo_occupancy);
      }
    }
  }
}
