# Ble - BT_STACK_BYPASS

## High-Level Overview

SiWx91x BLE BT stack bypass example: send raw HCI commands to the SiWx91x module from a Linux host over UART, using Simplicity Studio in SoC mode for low-level Bluetooth experimentation.

## Table of Contents

- [Ble - BT\_STACK\_BYPASS](#ble---bt_stack_bypass)
  - [High-Level Overview](#high-level-overview)
  - [Table of Contents](#table-of-contents)
  - [Purpose / Scope](#purpose--scope)
  - [Prerequisites / Setup Requirements](#prerequisites--setup-requirements)
    - [Hardware Requirements](#hardware-requirements)
    - [Software Requirements](#software-requirements)
    - [NCP mode: host application and project files](#ncp-mode-host-application-and-project-files)
    - [Setup Diagram](#setup-diagram)
  - [Steps to Run Demo](#steps-to-run-demo)
    - [Getting Started](#getting-started)
    - [NCP host mode (EFR32 + Si91x module)](#ncp-host-mode-efr32--si91x-module)
      - [HCI VCOM and debug logging (NCP)](#hci-vcom-and-debug-logging-ncp)
        - [Console Logging via RTT Channel 2](#console-logging-via-rtt-channel-2)
        - [SL\_SI91X\_PRINT\_DBG\_LOG Define (Legacy)](#sl_si91x_print_dbg_log-define-legacy)
      - [BTDM debug logging (controller logs on RTT)](#btdm-debug-logging-controller-logs-on-rtt)
    - [Configuration and Setup](#configuration-and-setup)
    - [Steps for Execution](#steps-for-execution)
      - [Attach HCI UART on the Linux host (Raspberry Pi / modern Linux)](#attach-hci-uart-on-the-linux-host-raspberry-pi--modern-linux)
      - [Legacy Fedora (hciattach)](#legacy-fedora-hciattach)
    - [Pin configurations for UART cable (SoC mode)](#pin-configurations-for-uart-cable-soc-mode)
  - [Troubleshooting](#troubleshooting)
  - [Resources](#resources)
  - [Report Bugs and Get Support](#report-bugs-and-get-support)

## Purpose / Scope

This application demonstrates how to configure the Raw HCI commands through uart.

## Prerequisites / Setup Requirements

Before running the application, the user will need the following things to setup.

### Hardware Requirements

- A Windows PC (for build, flash, and—on **NCP**—USB connection to the EFR32 host board).
- SiWx91x Wi-Fi Evaluation Kit. The SiWx91x supports multiple operating modes. See [Operating Modes](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-overview/#operating-modes) for details.

- **SoC Mode**:
  - Standalone
    - BRD4002A Wireless Pro Kit Mainboard [SI-MB4002A](https://www.silabs.com/development-tools/wireless/wireless-pro-kit-mainboard?tab=overview)
    - Radio Boards
      - BRD4338A [SiWx917-RB4338A](https://www.silabs.com/development-tools/wireless/wi-fi/siwx917-rb4338a-wifi-6-bluetooth-le-soc-radio-board?tab=overview)
      - BRD4339B [SiWx917-RB4339B]
      - BRD4340A [SiWx917-RB4340A]
      - BRD4343A [SiWx917-RB4343A]
  - Kits
    - SiWx917 Pro Kit [Si917-PK6031A](https://www.silabs.com/development-tools/wireless/wi-fi/siwx917-pro-kit?tab=overview)
    - SiWx917 Pro Kit [Si917-PK6032A]
    - SiWx917 AC1 Module Explorer Kit (BRD2708A)

- **NCP Mode** (this **SPI NCP** example; for board pictures and setup, follow the **NCP** sections in the **`wireless_test_ncp`** readme in the WiSeConnect SDK):
  - Standalone
    - BRD4002A Wireless Pro Kit Mainboard [SI-MB4002A](https://www.silabs.com/development-tools/wireless/wireless-pro-kit-mainboard?tab=overview)
    - EFR32xG24 Wireless 2.4 GHz +10 dBm Radio Board [xG24-RB4186C](https://www.silabs.com/development-tools/wireless/xg24-rb4186c-efr32xg24-wireless-gecko-radio-board?tab=overview)
    - NCP Expansion Kit with NCP Radio Boards
      - [BRD4346A](https://www.silabs.com/development-tools/wireless/wi-fi/siwx917-rb4346a-wifi-6-bluetooth-le-soc-4mb-flash-radio-board?tab=overview) + [BRD8045C](https://www.silabs.com/development-tools/wireless/wi-fi/shield-adapter-board-for-co-processor-radio-boards?tab=overview)
      - [BRD4357A](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-rb4357a-wi-fi-6-bluetooth-le-4mb-flash-radio-board-for-rcp-and-ncp-modules?tab=overview) + [BRD8045C](https://www.silabs.com/development-tools/wireless/wi-fi/shield-adapter-board-for-co-processor-radio-boards?tab=overview)
  - Kits
    - EFR32xG24 Pro Kit +10 dBm [xG24-PK6009A](https://www.silabs.com/development-tools/wireless/efr32xg24-pro-kit-10-dbm?tab=overview)
  - **Host–module interface:** **SPI** between the EFR32 NCP host and the Si91x radio module (this project matches the **`wireless_test_ncp`** SPI arrangement; HCI to the PC uses the EFR32 **USB/VCOM** connection described under [NCP host mode](#ncp-host-mode-efr32--si91x-module)).

- Smart phone with [Simplicity Connect App](https://www.silabs.com/developers/simplicity-connect-mobile-app) (formerly EFR Connect App) for BLE testing (optional; users may use other BLE apps on Android/iOS).

### Software Requirements

- [WiSeConnect SDK](https://github.com/SiliconLabs/wiseconnect-wifi-bt-sdk/)

- Embedded Development Environment

  - For STM32, use licensed [Keil IDE](https://www.keil.com/demo/eval/arm.htm)

  - For Silicon Labs EFR32, use the latest version of [Simplicity Studio](https://www.silabs.com/developers/simplicity-studio)

- **Segger RTT** (bundled with Simplicity Studio / J-Link support on EFR32) for **debug logging**: use the **RTT** view in the debugger or **J-Link RTT Viewer** to capture firmware prints. 
  - **RTT Channel 1**: BTDM controller logs (when **`BTDM_DEBUG_LOGGING`** is enabled in `ble_config.h`)
  - **RTT Channel 2**: Application console logs (`LOG_PRINT`, `DEBUGOUT`) in NCP mode
  - See [HCI VCOM and debug logging](#hci-vcom-and-debug-logging-ncp) and [BTDM debug logging](#btdm-debug-logging-controller-logs-on-rtt) for details.

- Download and install the Silicon Labs [Simplicity Connect App](https://www.silabs.com/developers/simplicity-connect-mobile-app) (formerly EFR Connect App) on Android smartphones for testing BLE applications. Users can also use their choice of BLE apps available on Android/iOS.

### NCP mode: host application and project files

| Mode | Host / target                | Project file (this example folder) |
|------|------------------------------|------------------------------------|
| SoC  | Application runs on SiWx91x. | `bt_stack_bypass.slcp`             |
| NCP  | Application runs on **EFR32** host; SiWx917 is the network co-processor over **SPI**. | `bt_stack_bypass_spi_ncp.slcp`             |

> **Note:** The BT Stack Bypass example is provided only in **SoC** and **NCP** mode. PSRAM variants are not shipped for this example.

Open the `.slcp` for your kit from `examples/snippets/ble/bt_stack_bypass/` in Simplicity Studio. For NCP, follow [Getting started with NCP mode](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/getting-started-with-ncp-mode).

### Setup Diagram 

![Figure: Setup Diagram SoC Mode for BT_STACK_BYPASS Example](resources/readme/blenewappsoc.png)

## Steps to Run Demo

### Getting Started

- Refer to the instructions [here](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/) to:

   - Install Studio and WiSeConnect extension
   - Connect your device to the computer
   - Upgrade your connectivity firmware
   - Create a Studio project

- For details on the project folder structure, see the [WiSeConnect Examples](https://docs.silabs.com/wiseconnect/latest/wiseconnect-examples/#example-folder-structure) page.

### NCP host mode (EFR32 + Si91x module)

Use the **SoC** project (`bt_stack_bypass.slcp`) when the application runs on the SiWx91x chip. Use **NCP host** mode when the application runs on an **EFR32 host MCU** connected to a Si91x radio module over the **NCP SPI bus** (same arrangement as `wireless_test_ncp`).

1. In Simplicity Studio, open **`bt_stack_bypass_spi_ncp.slcp`** (or create a project from it) and generate the project so it targets **EFR32 + BRD4186C** (or your NCP host board) with the WiseConnect NCP components (`sl_si91x_spi_bus`, `spidrv` instance `exp`, `iostream_recommended_stream`, and related config in `config/`). This matches the **`wireless_test_ncp`** component set for the SPI interface, without the console CLI sources.
2. **Do not** define `SLI_SI91X_MCU_INTERFACE` for the NCP host build. `app.c` then uses the **NCP path**: HCI bytes are read and written with **`sl_iostream`** (`sl_iostream_getchar` / `sl_iostream_write` on the recommended console stream). Configure **stdio retarget** per `wireless_test_ncp` (EUSART VCOM).
3. HCI reset handling on NCP calls **`sl_wifi_deinit()`** and **`NVIC_SystemReset()`** instead of the SoC **`sl_si91x_soc_nvic_reset()`**.
4. **`ble_config.h`** already selects NCP-appropriate BLE limits when `SLI_SI91X_MCU_INTERFACE` is undefined (GPIO-based handshake, larger ATT records).

#### HCI VCOM and debug logging (NCP)

The same UART/VCOM used for **HCI traffic to the host PC** must carry **only** HCI-framed bytes. **`printf`** / **`LOG_PRINT`** on that link injects non-HCI data (ASCII, `\r\n`, etc.); the host HCI stack then often reports **invalid packet type** (similar to mixing arbitrary text with binary HCI).

##### Console Logging via RTT Channel 2

To avoid interference with HCI traffic, **application console logs** (`LOG_PRINT`, `DEBUGOUT`) are redirected to **Segger RTT Channel 2** in NCP mode. This provides non-intrusive debug logging over the J-Link debugger connection without affecting HCI communication on VCOM.

**RTT Channel Configuration:**
- **Channel 0**: Default RTT Terminal (unused)
- **Channel 1**: BTDM controller logs (see [BTDM debug logging](#btdm-debug-logging-controller-logs-on-rtt))
- **Channel 2**: Application console logs (`LOG_PRINT`, `DEBUGOUT`)

**Implementation:**
- All `LOG_PRINT` and `DEBUGOUT` macros are redirected to RTT Channel 2 via `app_rtt_logging.h`
- RTT is initialized in `app_init()` for NCP mode
- 1024-byte buffer with `SEGGER_RTT_MODE_NO_BLOCK_SKIP`

**To View Console Logs:**
1. Connect J-Link debugger to the board
2. Launch **J-Link RTT Viewer**:
   - Select device: **EFR32MG24B210F1536IM48**
   - Connection: SWD, 4000 kHz
   - RTT Control Block: Auto Detection
3. Open **Terminal 2** to view application console logs
4. Open **Terminal 1** to view BTDM controller logs (if enabled)

Alternatively, use Simplicity Studio's RTT Console view while debugging.

**Benefits:**
- ✅ **Clean HCI**: VCOM carries only HCI traffic (no text interference)
- ✅ **High Speed**: RTT is much faster than UART (up to MB/s)
- ✅ **No Extra Hardware**: Uses existing J-Link debugger connection
- ✅ **Multiple Channels**: Separate streams for different log types
- ✅ **Minimal Overhead**: Very low CPU usage for logging

##### SL_SI91X_PRINT_DBG_LOG Define (Legacy)

- In **`rsi_common_apis.h`**, **`LOG_PRINT`** originally mapped to **`printf`** only when **`SL_SI91X_PRINT_DBG_LOG`** was defined; otherwise **`LOG_PRINT`** was a no-op (unless **`DEBUGOUT`** was defined). 
- For **NCP mode**, this define is **not set** in the `.slcp`, and `LOG_PRINT`/`DEBUGOUT` are instead **redirected to RTT Channel 2** via `app_rtt_logging.h` (see above).
- **Do not define `SL_SI91X_PRINT_DBG_LOG`** in NCP builds, as it would route logs to VCOM/HCI and break the HCI stream.

#### BTDM debug logging (controller logs on RTT)

**Bluetooth dual-mode (BTDM) controller debug logging** is controlled by **`BTDM_DEBUG_LOGGING`** in **`ble_config.h`**. It is **`0` (disabled) by default** to avoid extra RAM, CPU, and RTT traffic in normal builds. Set it to **`1`** only while you are actively testing or diagnosing controller-side behavior.

When enabled, the firmware starts the **`bt_debug_logs`** task and routes controller debug output to **Segger RTT** (up-buffer **1**, `Si91x_ApplicationDebugBuffer` in `app.c`), **not** to the HCI UART—so HCI framing on VCOM stays clean. View the stream in your debugger’s **RTT** terminal (or J-Link RTT Viewer) on that buffer.

### Configuration and Setup
 
- The application can be configured to suit your requirements and development environment. Read through the following sections and make any changes needed.

  - Open `USART.c` file which can be found in this path: components/device/silabs/si91x/mcu/drivers/cmsis_driver/USART.c

  - User must enable the below parameters if they are not enabled

  ```c
   #define RTE_USART0_CHNL_UDMA_TX_EN         1
  ```
  ```c
   #define RTE_USART0_CHNL_UDMA_RX_EN         1
  ```

> **Note**: For recommended settings, please refer the [recommendations guide](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-prog-recommended-settings/).

### Steps for Execution

Refer to the instructions [here](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/) to:

- Build the application in Studio.
- Flash, run and debug the application.
 
Follow the steps for successful execution of the program:

1. After the program gets executed, Silicon Labs module will be in uart receive state.

	- Observe the prints in the Docklight

	![](resources/readme/consoleprints.png)

2. Wait until **Wi-Fi initialization completes** on the module before attaching HCI on the host (required for BLE HCI to respond).

3. Attach the UART HCI interface on the Linux host — see [Attach HCI UART on the Linux host](#attach-hci-uart-on-the-linux-host-raspberry-pi--modern-linux) (Raspberry Pi / modern Linux) or [Legacy Fedora (hciattach)](#legacy-fedora-hciattach).

4. Send the below command to verify the device interface with `BD_ADDR` is up or not.

    ```sh
    hciconfig
    ```

5. If the interface is `DOWN`, send below command to make interface `UP`, where `X` indicates device interface.

    ```sh
    hciconfig -a hciX up
    ```

6. Send the below commands to verify the basic functionality.
    - For Advertising, where `X` indicates device interface.

    ```sh
    hciconfig -a hciX leadv
    ```
    - For Scanning, where `X` indicates device interface.

    ```sh
    hcitool -i hciX lescan
    ```

7. After sending the above commands verify functionality in Remote Mobile App

   - Open NRF connect app in remote Mobile device and scan for Silicon Labs Module with BD_ADDR seen in step no. 4

8. To detach the HCI UART interface, stop `btattach` (modern Linux) or `hciattach` (legacy):

    ```sh
    pkill -f 'btattach.*ttyUSBX'
    ```
    or
    ```sh
    pkill hciattach
    ```

9. If you want to re-run the application press reset on the board and follow the same steps.

#### Attach HCI UART on the Linux host (Raspberry Pi / modern Linux)

Use this flow on **Raspberry Pi OS**, **Debian Bookworm**, and other hosts with **kernel 5.10+** and **BlueZ 5.x**. The bundled script uses `btattach` (H4 protocol) instead of the deprecated `hciattach` tool.

1. Install BlueZ user tools if `btattach` is not present:

    ```sh
    sudo apt install bluez
    ```

2. Copy `examples/snippets/ble/bt_stack_bypass/binaries/hci_uart_attach.sh` to the Pi and make it executable:

    ```sh
    chmod +x hci_uart_attach.sh
    ```

3. Attach the SiWx91x HCI UART (use `-d` for background mode — recommended on Pi so SSH disconnect does not remove `hciX`):

    ```sh
    sudo ./hci_uart_attach.sh -d /dev/ttyUSBX 115200
    ```

4. Bring the interface up:

    ```sh
    sudo hciconfig hciX up
    ```

The script stops **ModemManager** and the host **bluetooth** service (which can grab the serial port or conflict with the onboard controller), configures the UART line, and runs `btattach -P h4 -N`. **`btattach` must stay running** while `hci0` is in use.

#### Legacy Fedora (hciattach)

On older Fedora hosts that ship the bundled `hciattach` binary in `examples/snippets/ble/bt_stack_bypass/binaries/`:

1. Copy the `hciattach` binary to a folder on the host.

2. Change permissions:

    ```sh
    chmod 777 hciattach
    ```

3. Attach:

    ```sh
    ./hciattach -s 115200 /dev/ttyX any
    ```

	
### Pin configurations for UART cable (SoC mode)

The table below applies **only to SoC mode**, when you connect an **external USB-to-UART cable** from a Linux host (e.g. Fedora) to the WSTK for raw HCI on UART.

**NCP mode (this SPI NCP example):** the EFR32 host board is typically connected to the PC with a **single USB cable** that supplies **power** and carries **USB/VCOM data** (HCI to the host and debug/flash as configured in Studio). You do **not** use the SoC UART pin wiring below for that link; follow your board’s **USB** connection and the project’s **stdio / iostream** setup instead.

- Connect USB to UART cable to a Linux host (e.g. Raspberry Pi or Fedora) ([example cable](https://www.amazon.in/Serial-Converter-Cable-Terminated-Header/dp/B06ZYPLFNB)).
- Follow the pin configuration below to connect the USB-to-UART cable to the WSTK board (**SoC** testing only).

| Pin description | Pin number on the WSTK board |
|-----------------|------------------------------|
| UART Tx         | P35                          |
| UART Rx         | P33                          |

## Troubleshooting

If you encounter issues while running the BT Stack Bypass example, check the following:

- Ensure USART0 DMA channels (`RTE_USART0_CHNL_UDMA_TX_EN`, `RTE_USART0_CHNL_UDMA_RX_EN`) are enabled in `USART.c`.
- Verify the USB-to-UART cable wiring matches the pin configuration (Tx on P35, Rx on P33).
- On **Raspberry Pi / modern Linux**, use `hci_uart_attach.sh` with `btattach` — not legacy `hciattach` (see [Attach HCI UART on the Linux host](#attach-hci-uart-on-the-linux-host-raspberry-pi--modern-linux)).
- Wait for **Wi-Fi initialization** on the module before running `hciconfig hciX up`.
- If the serial port is busy, ensure **ModemManager** and the host **bluetooth** service are stopped (the script does this automatically).
- On Pi, run `hci_uart_attach.sh -d` so `btattach` survives SSH session disconnect.
- If `hciconfig` shows the interface as DOWN, bring it up with `hciconfig -a hciX up`.
- Verify the serial baud rate (115200) and correct `/dev/ttyUSBX` device path.
- On legacy Fedora, confirm the `hciattach` binary has execute permissions (`chmod 777 hciattach`).

## Resources

- [WiSeConnect Getting Started Guide](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/)
- [WiSeConnect API Reference Guide](https://docs.silabs.com/wiseconnect/latest/wiseconnect-api-reference-guide-si91x-driver/)
- [Simplicity Connect Mobile App](https://www.silabs.com/developers/simplicity-connect-mobile-app)

## Report Bugs and Get Support

Report issues and get help from the Silicon Labs community:

- [Silicon Labs Community](https://www.silabs.com/community)
