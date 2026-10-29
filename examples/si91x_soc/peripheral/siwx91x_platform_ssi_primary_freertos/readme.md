# SiWx91x Platform - SSI Primary FreeRTOS

## Table of Contents

- [SiWx91x Platform - SSI Primary FreeRTOS](#siwx91x-platform-ssi-primary-freertos)
  - [Purpose/Scope](#purposescope)
  - [Overview](#overview)
  - [About Example Code](#about-example-code)
    - [FreeRTOS Architecture](#freertos-architecture)
    - [Initialization (init_function)](#initialization-init_function)
    - [Task Flow (ssi_primary_task)](#task-flow-ssi_primary_task)
  - [Prerequisites/Setup Requirements](#prerequisitessetup-requirements)
    - [Hardware Requirements](#hardware-requirements)
    - [Software Requirements](#software-requirements)
    - [Setup Diagram](#setup-diagram)
  - [Getting Started](#getting-started)
  - [Application Build Environment](#application-build-environment)
    - [Application Configuration Parameters](#application-configuration-parameters)
    - [Pin Configuration](#pin-configuration)
    - [Pin Connections Between Primary and Secondary](#pin-connections-between-primary-and-secondary)
  - [Test the Application](#test-the-application)
  - [Troubleshooting](#troubleshooting)
  - [Resources](#resources)
  - [Report Bugs / Support](#report-bugs--support)

## Purpose/Scope

This application demonstrates the use of Synchronous Serial Interface (SSI) for data transfer in full duplex in primary mode in a FreeRTOS environment.

## Overview

- SSI is a synchronous, point-to-point, serial communication channel for digital data transmission.
- Synchronous data transmission is one in which the data is transmitted by synchronizing the transmission at the receiving and sending ends using a common clock signal.
- SSI is a synchronous four-wire interface consisting of two data pins (MOSI, MISO), a device select pin (CSN), and a gated clock pin(SCLK).
- With the two data pins, it allows for full-duplex operation with other SSI-compatible devices.
- It supports full-duplex, single-bit SPI primary mode.
- It supports 6 modes:  
  - Mode 0: Clock Polarity is zero and Clock Phase is zero.
  - Mode 1: Clock Polarity is zero and Clock Phase is one.
  - Mode 2: Clock Polarity is one and Clock Phase is zero.
  - Mode 3: Clock Polarity is one and Clock Phase is one.
  - Mode-4: TEXAS_INSTRUMENTS SSI.
  - Mode-5: NATIONAL_SEMICONDUCTORS_MICROWIRE.
- The SPI clock is programmable to meet required baud rates.
- It can generates interrupts for different events like transfer complete, data lost, and mode fault.
- It supports up to 32K bytes of read data from a SSI device in a single read operation.
- It has support for DMA (Dynamic Memory Access).
- It can run in synchronous mode with full-duplex operation.
  - Primary transmits data on MOSI pin and receives the same data on MISO pin.
- It also supports send and receive data with any SSI secondary. Additionally, it also supports DMA and non-DMA transfer.
- For half-duplex communication (that is, send and receive), a primary / secondary connection is required.
- The SSI Primary in MCU HP peripherals provides an option to connect up to four secondaries and supports Single, Dual and Quad modes.

>**Note!** Make sure to use non-ROM SSI APIs for this application and SL_SSI driver.

## About Example Code

This example demonstrates SSI transfer (full-duplex communication) and SSI send/SSI receive (half-duplex communication) running as a dedicated FreeRTOS task.

- Various parameters like SSI clock mode, Bit-width, Manual cs pin, and SSI baud rate can be configured using the UC. Also, Primary or Secondary or ULP Primary DMA can be configured using the UC.
- The [`sl_si91x_ssi_config.h`](https://github.com/SiliconLabs/wiseconnect/blob/v4.1.2-content-for-docs/components/device/silabs/si91x/mcu/drivers/unified_api/config/sl_si91x_ssi_config.h) file contains the control configurations, and [`sl_si91x_ssi_common_config.h`](https://github.com/SiliconLabs/wiseconnect/blob/v4.1.2-content-for-docs/components/device/silabs/si91x/mcu/drivers/unified_api/config/sl_si91x_ssi_common_config.h) contains DMA configuration selection.

### FreeRTOS Architecture

- On startup, `main.c` calls `app_init()` which invokes `ssi_primary_example_init()`. This function creates the `ssi_primary_task` FreeRTOS thread using `osThreadNew()`. The `app_process_action()` function is a no-op since all application logic runs inside the dedicated task.
- The `main.c` FreeRTOS flow calls `app_init()` followed by a `while (sl_main_start_task_should_continue())` loop that calls `app_process_action()` (no-op). The FreeRTOS scheduler manages task execution.

### Initialization (init_function)

- The output buffer is filled with data which is transferred to the secondary.
- The firmware version of the API is fetched using [sl_si91x_ssi_get_version](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-peripherals/ssi#sl-si91x-ssi-get-version) which includes the release version, major version, and minor version [sl_ssi_version_t](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-peripherals/sl-ssi-version-t).
- [sl_si91x_ssi_init](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-peripherals/ssi#sl-si91x-ssi-init) is used to initialize the peripheral, that includes pin configuration and it powers up the module.
- SSI instance must be passed in init function to get the instance handle [sl_ssi_instance_t](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-peripherals/ssi#sl-ssi-instance-t), which is used in other APIs.
- All the necessary parameters are configured using [sl_si91x_ssi_set_configuration](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-peripherals/ssi#sl-si91x-ssi-set-configuration) API. It expects a structure with required parameters [sl_ssi_control_config_t](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-peripherals/sl-ssi-control-config-t).
- A binary semaphore (`ssi_transfer_sem`) is created using `osSemaphoreNew()` to signal transfer completion from the ISR callback to the task.
- A callback is registered using [sl_si91x_ssi_register_event_callback](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-peripherals/ssi#sl-si91x-ssi-register-event-callback). On `SSI_EVENT_TRANSFER_COMPLETE`, the callback releases the semaphore via `osSemaphoreRelease()`.
- Four serial secondaries can be connected using this SSI primary. For validation, you should change the secondary number range from 0 to 3 in `ssi_primary_freertos.c` and should enable the respective macro (for example, M4_SSI_CS0 for secondary 0) in the `RTE_Device_917.h` file. (path: /$project/config/RTE_Device_917.h)

### Task Flow (ssi_primary_task)

After initialization, the task configures `UULP_VBAT_GPIO_2` as a sync input and waits for a button press on the primary-side sync line before starting the first active phase. If a later receive phase follows a transfer or send phase, the task waits for a second button press before starting that receive phase. The transfer phases then execute sequentially based on the macros enabled in `ssi_primary_freertos.h`:

>**Note:** The frequency of the SSI primary bit-rate clock is one-half the frequency of SSI primary input clock.

- If the **SSI_PRIMARY_TRANSFER** macro is enabled, it will transfer the data (that is, send and receive data in full-duplex mode).

  - Calls [sl_si91x_ssi_transfer_data](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-peripherals/ssi#sl-si91x-ssi-transfer-data) which expects data_out, data_in and number of data bytes to be transferred for sending and receiving data simultaneously (full duplex).
  - This test can also be performed in loopback state (that is, connect MISO and MOSI pins).
  - The task blocks on `osSemaphoreAcquire()` until the transfer complete event releases the semaphore. It then compares the sent and received data. The result is printed on the console.

- If the **SSI_PRIMARY_SEND** macro is enabled, it only sends the data to secondary. The SPI secondary must be connected; it cannot be tested in loopback mode.

  - Calls [sl_si91x_ssi_send_data](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-peripherals/ssi#sl-si91x-ssi-send-data) which expects data_out (data buffer that needs to be sent) and number of bytes to send.
  - The task blocks on `osSemaphoreAcquire()` until the send completes.

- If the **SSI_PRIMARY_RECEIVE** macro is enabled, it only receives the data from secondary. SPI secondary must be connected; it cannot be tested in loopback mode.

  - If receive follows an earlier transfer or send phase, the task waits for another `BTN0` press before starting the receive phase.
  - Calls [sl_si91x_ssi_receive_data](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-peripherals/ssi#sl-si91x-ssi-receive-data) which expects data_in (empty buffer) and number of data bytes to be received.
  - The task blocks on `osSemaphoreAcquire()` until the receive completes, then compares the data.

- The enabled phases run in order in a single pass (no repeating loop). When finished, the task calls `osThreadExit()` to terminate.
- If any API call fails, the task prints an error via `DEBUGOUT` and calls `osThreadExit()` to terminate.

## Prerequisites/Setup Requirements

### Hardware Requirements

- Windows PC
- Silicon Labs SiWx917 Evaluation Kit [[BRD4002](https://www.silabs.com/development-tools/wireless/wireless-pro-kit-mainboard?tab=overview) + [BRD4338A](https://www.silabs.com/development-tools/wireless/wi-fi/siwx917-rb4338a-wifi-6-bluetooth-le-soc-radio-board?tab=overview) / [BRD4342A](https://www.silabs.com/development-tools/wireless/wi-fi/siwx91x-rb4342a-wifi-6-bluetooth-le-soc-radio-board?tab=overview) / [BRD4343A](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-rb4343a-wi-fi-6-bluetooth-le-8mb-flash-radio-board-for-module?tab=overview) / [BRD4343C](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-rb4343c-wi-fi-6-bluetooth-le-8mb-flash-radio-board-for-module?tab=overview)]
- SiWx917 AC1 Module Explorer Kit [BRD2708A](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-ek2708a-explorer-kit)

### Software Requirements

- Simplicity Studio
- Serial console setup
  - For Serial Console setup instructions, refer to [link name](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#console-input-and-output).

### Setup Diagram

![Figure: setupdiagram](resources/readme/setupdiagram.png)

## Getting Started

Refer to the instructions [here](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/) to:

- [Install Simplicity Studio](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#install-simplicity-studio)
- [Install WiSeConnect extension](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#install-the-wiseconnect-3-extension)
- [Connect your device to the computer](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#connect-siwx91x-to-computer)
- [Upgrade your connectivity firmware](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#update-siwx91x-connectivity-firmware)
- [Create a Studio project](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#create-a-project)

For details on the project folder structure, see the [WiSeConnect Examples](https://docs.silabs.com/wiseconnect/latest/wiseconnect-examples/#example-folder-structure) page.

## Application Build Environment

### Application Configuration Parameters

- Configure UC from the slcp component.
- Open the **siwx91x_platform_ssi_primary_freertos.slcp** project file, select the **Software Component** tab, and search for **SSI** in the search bar.
- You can use the configuration wizard to configure different parameters like:
  
   ![Figure: UC image](resources/uc_screen/ssi_uc_screen.png)

  - **SSI Primary(Primary) Configuration**
    - Frame Format: SSI Frame Format can be configured:
      - Mode 0, Mode 1, Mode 2, Mode 3, Mode 4 and Mode 5.
    - Transfer Mode: SSI Transfer Mode will be `Single Line Mode` in this application.
    - Bit Rate: The speed of transfer is configurable. The configuration range is from 500Kbps to 40Mbps in high-power mode.
    - Data Width: The size of data packet. The configuration range from 4 to 16.
    - Mode: SSI mode/instance can be configurable. It can be configured Primary.
    - Rx Sample Delay: Receive Data (rxd) Sample Delay. This to delay the sample of the rxd input signal. Each value represents a single SSI clock delay on the sample of the rxd signal. The configuration range is from 0 to 63.
  - **SSI Primary(Primary) DMA Configuration**
    - SSI Primary DMA Enable: Enable DMA for SSI Primary transfers. Disabled by default.
    - DMA Controller: Select **UDMA** or **GPDMA**. The default is UDMA. Only one controller is used at a time.
    - GPDMA Configuration: Enable this when GPDMA is selected. Set the maximum number of TX and RX descriptors (1 to 256, default 8). Each descriptor moves up to 4095 bytes.
    - Tx FIFO Threshold: Transmit FIFO Threshold. It controls the level of entries (or below) 
    at which the transmit FIFO controller triggers an interrupt. The configuration range from 
    0 to 15 when using udma and 0-7 when using gpdma.
    - Rx FIFO Threshold: Receive FIFO Threshold. It controls the level of entries (or below) 
    at which the receive FIFO controller triggers an interrupt. The configuration range from 0 
    to 15 when using udma ,0 to 7 when using GPDMA.    
- Configuration files are generated in **config folder**. If the values are not changed, the code will run on default UC values.

- Configure the following macros in the `ssi_primary_freertos.h` file and update/modify following macros, if required.

- `SSI_PRIMARY_TRANSFER`: This macro is enabled default. It sends and receives data in full-duplex.

  ```c
    #define SSI_PRIMARY_TRANSFER ENABLE    // To use the transfer API
  ```

- `SSI_PRIMARY_SEND (or) SSI_PRIMARY_RECEIVE`: If SSI_PRIMARY_RECEIVE or SSI_PRIMARY_SEND is enabled, the SSI secondary will receive and send data in half-duplex respectively.

  ```c
    #define SSI_PRIMARY_SEND     DISABLE   // To use the send API
    #define SSI_PRIMARY_RECEIVE  DISABLE   // To use the receive API
  ```

- By default, an 8-bit unsigned integer is declared for data buffer. If using a data-width more than 8 bit, update the variable to a 16-bit unsigned integer.

  ```c
  // For data-width less than equal to 8
  static uint8_t ssi_primary_tx_buffer[SSI_PRIMARY_BUFFER_SIZE] = { '\0' }; 
  static uint8_t ssi_primary_rx_buffer[SSI_PRIMARY_BUFFER_SIZE] = { '\0' };
  // For data-width greater than 8
  static uint16_t ssi_primary_tx_buffer[SSI_PRIMARY_BUFFER_SIZE] = { '\0' }; 
  static uint16_t ssi_primary_rx_buffer[SSI_PRIMARY_BUFFER_SIZE] = { '\0' };
  ```

- By default, CS0 is selected in the pintool. To use a different chip select (CS), update the corresponding secondary number in the `ssi_primary_freertos.c` file after configuring the desired CS in the pintool.

  ```c
  // For CS0
  static uint32_t ssi_slave_number = SSI_SLAVE_0; 
  // For CS1
  static uint32_t ssi_slave_number = SSI_SLAVE_1; 
  // For CS2
  static uint32_t ssi_slave_number = SSI_SLAVE_2;
  // For CS3
  static uint32_t ssi_slave_number = SSI_SLAVE_3;  
  ```

- Configure the following macros in [`ssi_primary_freertos.c`](https://github.com/SiliconLabs/wiseconnect/blob/v4.1.2-content-for-docs/examples/si91x_soc/peripheral/siwx91x_platform_ssi_primary_freertos/ssi_primary_freertos.c) if required:

- `SSI_PRIMARY_BUFFER_SIZE`: Defines the length of data (in data-width units) to be sent or received through SPI. By default, it is set to 1024.

  ```c
  #define SSI_PRIMARY_BUFFER_SIZE          1024     // Length of data to be sent through SPI
  ```

- `SSI_PRIMARY_BIT_WIDTH`: Defines the SSI data bit width used for each transfer frame. By default, it is set to 8.

  ```c
  #define SSI_PRIMARY_BIT_WIDTH            8        // SSI bit width
  ```

- `SSI_PRIMARY_BAUDRATE`: Defines the SSI primary baud rate (bit rate) in bits per second. By default, it is set to 10000000 (10 Mbps).

  ```c
  #define SSI_PRIMARY_BAUDRATE             10000000 // SSI baud rate
  ```

- `SSI_PRIMARY_MAX_BIT_WIDTH`: Defines the maximum supported SSI bit width used for buffer type selection. By default, it is set to 16.

  ```c
  #define SSI_PRIMARY_MAX_BIT_WIDTH        16       // Maximum bit width
  ```

- `SSI_PRIMARY_RECEIVE_SAMPLE_DELAY`: Defines the RX sample delay (in SSI clock cycles) applied to the receive data line. By default, it is set to 0.

  ```c
  #define SSI_PRIMARY_RECEIVE_SAMPLE_DELAY 0      // RX sample delay (SSI clocks)
  ```

- `PRIMARY_SECONDARY_SYNC_PIN`: Defines the GPIO used for button-based primary/secondary synchronization. By default, it is `RTE_UULP_GPIO_2_PIN`.

  ```c
  #define PRIMARY_SECONDARY_SYNC_PIN      RTE_UULP_GPIO_2_PIN
  ```

- To unregister a user event callback for a specific instance, use the API [sl_si91x_ssi_per_instance_unregister_event_callback](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-peripherals/ssi#sl-si91x-ssi-per-instance-unregister-event-callback). Alternatively, to unregister callbacks for all instances simultaneously, use the API [sl_si91x_ssi_unregister_event_callback](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-peripherals/ssi#sl-si91x-ssi-unregister-event-callback).

> **Note:** For reliable SSI operation, ensure that the peripheral clock is properly configured on both the **primary** and **secondary** sides. The SSI Primary peripheral clock (`ssi_clk`) must meet the following condition:
>
> F<sub>ssi_clk</sub> ≥ 2 × (maximum F<sub>sclk_out</sub>)
>
> Where F<sub>sclk_out</sub> is the bit rate output from the primary. Make sure the SSI Primary peripheral clock (F<sub>ssi_clk</sub>) is set accordingly. Incorrect clock configuration may result in communication errors or unreliable data transfer.
>
> **SSI Secondary Setup:** For instructions on configuring the SSI secondary, refer to the [SSI secondary](https://github.com/SiliconLabs/wiseconnect/blob/v4.1.2-content-for-docs/examples/si91x_soc/peripheral/sl_si91x_ssi_slave/readme.md) to ensure correct setup and operation.

### Pin Configuration

**SSI Primary Pin Configuration:**

| WPK [BRD4002A] + BRD4338A | Explorer kit (BRD2708A) | Description             |
| -------------------------- | ----------------------- | ----------------------- |
| GPIO_25 [P25]              | GPIO_25 [SCK]           | RTE_SSI_MASTER_SCK_PIN  |
| GPIO_28 [P31]              | GPIO_28 [CS]            | RTE_SSI_MASTER_CS0_PIN  |
| GPIO_26 [P27]              | GPIO_26 [MOSI]          | RTE_SSI_MASTER_MOSI_PIN |
| GPIO_27 [P29]              | GPIO_27 [MISO]          | RTE_SSI_MASTER_MISO_PIN |

`UULP_VBAT_GPIO_2` which is `BTN0` on WPK is connected to `F12`. This example uses that signal as the synchronization source on the primary side.

![Figure: Pin Configuration for SSI1](resources/readme/image510d.png)

### Pin Connections Between Primary and Secondary

**If using WPK (BRD4002A) baseboard with BRD4338A radio board:**

| Signal | Primary Board Pin (GPIO) | Primary Breakout | Secondary Board Pin (GPIO) | Secondary Breakout | Wire                       |
| ------ | ----------------------- | --------------- | ---------------------- | -------------- | -------------------------- |
| SCK    | GPIO_25                 | P25             | GPIO_26                | P27            | SCK → SCK                  |
| CS     | GPIO_28                 | P31             | GPIO_9                 | F09            | CS → CS                    |
| MOSI   | GPIO_26                 | P27             | GPIO_27                | P29            | Primary MOSI → Secondary MOSI   |
| MISO   | GPIO_27                 | P29             | GPIO_28                | P31            | Primary MISO → Secondary MISO   |
| SYNC   | UULP_VBAT_GPIO_2        | F12             | UULP_VBAT_GPIO_2       | F12            | F12 → F12                  |
| GND    | GND                     | GND             | GND                    | GND            | GND → GND                  |

**If using Explorer Kit (BRD2708A) on both sides:**

| Signal | Primary Board GPIO | Secondary Board GPIO | Wire                     |
| ------ | ----------------- | ---------------- | ------------------------ |
| SCK    | GPIO_25 [SCK]     | GPIO_25          | SCK → SCK                |
| CS     | GPIO_28 [CS]      | GPIO_28          | CS → CS                  |
| MOSI   | GPIO_26 [MOSI]    | GPIO_27          | Primary MOSI → Secondary MOSI |
| MISO   | GPIO_27 [MISO]    | GPIO_26          | Primary MISO → Secondary MISO |
| SYNC   | UULP_VBAT_GPIO_2  | UULP_VBAT_GPIO_2 | SYNC → SYNC              |
| GND    | GND               | GND              | GND → GND                |

>**Note:** Make sure the following pin configurations are in the `RTE_Device_917.h` file:
>
> - SiWx917: RTE_Device_917.h (path: /$project/config/RTE_Device_917.h)

## Test the Application

Refer to the instructions [here](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/) to:

1. Compile and run the application.
2. Connect the primary SSI pins to the secondary SSI pins as listed above. On WPK hardware, also connect `F12` on the primary board to `F12` on the secondary board for synchronization.
3. Reset the secondary board and then run or reset the primary board.
4. When the primary prints `Press button 0 on primary to sync.`, press `BTN0` on the primary board to start the first active phase.
5. If the enabled flow later reaches a receive phase after a transfer or send phase, press `BTN0` on the primary board again when prompted to start that receive phase.
6. In the case of loopback mode, when the loopback jumper wire is removed and the test is run, the result should come as data comparison fail and test case fail.
7. After successful program execution, the prints in serial console looks as shown below.

   ![Figure: output](resources/readme/output.png)

> **Note:**
>
> - Interrupt handlers are implemented in the driver layer, and user callbacks are provided for custom code. If you want to write your own interrupt handler instead of using the default one, make the driver interrupt handler a weak handler. Then, copy the necessary code from the driver handler to your custom interrupt handler.

## Troubleshooting

- If the project does not build, ensure Simplicity Studio and the WiSeConnect extension are installed and the board is connected.
- If the device is not detected, reinstall the connectivity firmware and check USB drivers.

## Resources

- [WiSeConnect Getting Started](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/)
- [WiSeConnect Examples](https://docs.silabs.com/wiseconnect/latest/wiseconnect-examples/)
- [Si91x SoC Documentation](https://docs.silabs.com/wiseconnect/latest/)

## Report Bugs / Support

For issues and support, use the Silicon Labs Community or your normal support channel.


