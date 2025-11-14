# SL USART SLAVE

## Table of Contents

- [SL USART SLAVE](#sl-usart-slave)
  - [Table of Contents](#table-of-contents)
  - [Purpose/Scope](#purposescope)
  - [Overview](#overview)
  - [About Example Code](#about-example-code)
  - [Prerequisites/Setup Requirements](#prerequisitessetup-requirements)
    - [Hardware Requirements](#hardware-requirements)
    - [Software Requirements](#software-requirements)
    - [Setup Diagram](#setup-diagram)
  - [Getting Started](#getting-started)
  - [Application Build Environment](#application-build-environment)
    - [Configuration of USART at UC (Universal Configuration)](#configuration-of-usart-at-uc-universal-configuration)
    - [Pin Configuration of the WPK\[BRD4002A\] Base Board, and with BRD4338A radio board](#pin-configuration-of-the-wpkbrd4002a-base-board-and-with-brd4338a-radio-board)
    - [Pin Configuration of the WPK\[BRD4002A\] Base Board, and with BRD4343A radio board](#pin-configuration-of-the-wpkbrd4002a-base-board-and-with-brd4343a-radio-board)
    - [Pin Configuration of the AC1 Module Explorer Kit](#pin-configuration-of-the-ac1-module-explorer-kit)
  - [Flow Control Configuration](#flow-control-configuration)
  - [Test the Application](#test-the-application)
  - [Configuring higher clock](#configuring-higher-clock)

## Purpose/Scope

This application demonstrates how to configure Universal Synchronous Asynchronous Receiver-Transmitter (USART) in synchronous mode as a slave to send and receive data from USART master.

## Overview

- USART is used in communication through wired medium in a Synchronous fashion. It enables the device to communicate using serial protocols.
- This application is configured with following configurations:
  - Tx and Rx enabled
  - 8 Bit data transfer
  - Synchronous slave
  - Stop bits 1
  - No Parity
  - No Auto Flow control
  - Baud Rates - 115200

## About Example Code

- [`usart_sync_example.c`](https://github.com/SiliconLabs/wiseconnect/blob/master/examples/si91x_soc/peripheral/sl_si91x_usart_sync_slave/usart_sync_example.c) - This example code demonstrates how to configure the USART to send and receive data.
- In this example, first USART gets initialized if not already initialized with clock and DMA configurations using [`sl_si91x_usart_init`](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-peripherals/usart#sl-si91x-usart-init).  
**Note:** If the UART/USART instance is already selected for debug output logs, initialization will return `SL_STATUS_NOT_AVAILABLE`.
- After USART initialization, the USART power mode is set using [`sl_si91x_usart_set_power_mode()`](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-peripherals/usart#sl-si91x-usart-set-power-mode) and then USART is configured with the default configurations from UC along with the USART transmit and receive lines using [`sl_si91x_usart_set_configuration()`](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-peripherals/usart#sl-si91x-usart-set-configuration).
- Then the register user event callback for send and receive complete notification is set using [`sl_si91x_usart_multiple_instance_register_event_callback()`](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-peripherals/usart#sl-si91x-usart-multiple-instance-register-event-callback).
- After setting the user event callback, the data send and receive can happen through [`sl_si91x_usart_transfer_data()`](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-peripherals/usart#sl-si91x-usart-transfer-data).
- Once the receive data event is triggered, both transmit and receive buffer data is compared to confirm if the received data is the same.

## Prerequisites/Setup Requirements

### Hardware Requirements

- Windows PC
- Silicon Labs Si917 Evaluation Kit [WPK(4002A) +  BRD4338A / BRD4342A / BRD4343A ]- Master and Slave
- SiWx917 AC1 Module Explorer Kit (BRD2708A) - Master and Slave

### Software Requirements

- Simplicity Studio
- Serial console Setup
  - For Serial Console setup instructions, refer [Console Inpput and Output](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#console-input-and-output) section of the *WiSeConnect Developer's Guide*.

### Setup Diagram

> ![Figure: setupdiagram](resources/readme/setupdiagram.png)

## Getting Started

Refer to the instructions [here](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/) to:

- [Install Simplicity Studio](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#install-simplicity-studio)
- [Install WiSeConnect extension](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#install-the-wi-se-connect-extension)
- [Connect your device to the computer](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#connect-si-wx91x-to-computer)
- [Upgrade your connectivity firmware](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#update-si-wx91x-connectivity-firmware)
- [Create a Studio project](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#create-a-project)

For details on the project folder structure, see the [WiSeConnect Examples](https://docs.silabs.com/wiseconnect/latest/wiseconnect-examples/#example-folder-structure) page.

## Application Build Environment

### Configuration of USART at UC (Universal Configuration)

- Configure UC from the slcp component.
- Open **sl_si91x_usart_sync_slave.slcp** project file, select the **Software Component** tab, and search for **USART** in the search bar.
- You can use the configuration wizard to configure different parameters. The following configuration screens illustrates where the user can select as per their requirements.

  ![Figure: Selecting UC](resources/uc_screen/usart_uc.png)

- Connect Master and Slave as per pin configurations (that is, connect USART master clock pin (GPIO_8/GPIO_25) to USART slave clock pin, master TX pin (GPIO_30) to slave RX pin, and master RX pin (GPIO_29) to slave TX pin).

- Connect Slave and Master as per pin configurations (that is, connect USART slave clock pin (GPIO_8/GPIO_25) to USART master clock pin (GPIO_8), slave RX(GPIO_29) pin to master TX pin(GPIO_30), and slave TX pin(GPIO_30) to mster RX pin(GPIO_29)).

### Pin Configuration of the WPK[BRD4002A] Base Board, and with BRD4338A radio board

  | USART PINS              | GPIO    | Breakout pin  |
  | ----------------------- | ------- | ------------- |
  | USART_SLAVE_CLOCK_PIN  | GPIO_8  |     F8        |
  | USART_SLAVE_TX_PIN     | GPIO_30 |     P35       |
  | USART_SLAVE_RX_PIN     | GPIO_29 |     P33       |

### Pin Configuration of the WPK[BRD4002A] Base Board, and with BRD4343A radio board

  | USART PINS              | GPIO    | Breakout pin  |
  | ----------------------- | ------- | ------------- |
  | USART_SLAVE_CLOCK_PIN  | GPIO_8  |      F8       |
  | USART_SLAVE_TX_PIN     | GPIO_30 |     P35       |
  | USART_SLAVE_RX_PIN     | GPIO_29 |     P33       |  
  
### Pin Configuration of the WPK[BRD4002A] Base Board, and with BRD4342A radio board

  | USART PINS              | GPIO    | Breakout pin  |
  | ----------------------- | ------- | ------------- |
  | USART_SLAVE_CLOCK_PIN  | GPIO_25 |     P25       |
  | USART_SLAVE_TX_PIN     | GPIO_30 |     P35       |
  | USART_SLAVE_RX_PIN     | GPIO_29 |     P33       |  

  ![Figure: Build run and Debug](resources/readme/image513d.png)

### Pin Configuration of the AC1 Module Explorer Kit

  | USART PINS              | GPIO    | Explorer kit Breakout pin  |
  | ----------------------- | ------- | ------------- |
  | USART_SLAVE_CLOCK_PIN   | GPIO_25 |     [SCK]     |
  | USART_SLAVE_TX_PIN      | GPIO_30 |     [RST]     |
  | USART_SLAVE_RX_PIN      | GPIO_29 |     [AN]      |

## Flow Control Configuration

To enable hardware flow control for the USART:

1. In the UC settings, navigate to the **USART UC Configuration** section.
2. Set the **Flow control** parameter to **RTS/CTS** to enable hardware flow control.
3. After enabling flow control, User need to assign the appropriate GPIO pins for **RTS** and **CTS** signals as shown in the table below:
    ```
    | USART PINS     | GPIO    | Breakout pin  | Explorer kit Breakout pin|
    | -------------- | ------- | ------------- | ------------------------ |
    | USART0_CTS_PIN | GPIO_26 |     P27       |           [MISO]         |
    | USART0_RTS_PIN | GPIO_28 |     P31       |           [CS]           |
    ```

> **Note**: For recommended settings, please refer the [recommendations guide](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-prog-recommended-settings/).

## Test the Application

1. Make the connections as discussed in the Application Build Environment.
2. When the application runs, USART sends and receives data in full duplex mode.
3. Observe the USART transmission status upon data transmission from USART master to slave and vice-versa on USART pins.
4. After running this application, the below console output can be observed.

  ![Figure: expected result](resources/readme/usart_slave_console_output.png)

>
> **Note**:
>
>- Add data_in buffer to watch window for checking receive data.

## Configuring higher clock

- To achieve baud rates exceeding 2 million bps, need to modify the clock source to INTF PLL CLK or SoC PLL CLK in the UC. 

> **Note:**
>
> - Interrupt handlers are implemented in the driver layer, and user callbacks are provided for custom code. If you want to write your own interrupt handler instead of using the default one, make the driver interrupt handler a weak handler. Then, copy the necessary code from the driver handler to your custom interrupt handler.
> - By default, Request to Send (RTS) and Clear to Send (CTS) flow control signals are disabled in the UART driver UC, and their corresponding GPIO pins are not assigned in the Pintool. If the user enables RTS/CTS in the Driver UC, they must manually configure and assign the appropriate GPIO pins in the Pintool to ensure proper hardware flow control functionality.
