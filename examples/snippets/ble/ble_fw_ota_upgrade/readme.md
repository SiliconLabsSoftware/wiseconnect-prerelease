# BLE - FW OTA Upgrade

## High-Level Overview

This application demonstrates how to update the SiWx91x module firmware over-the-air (OTA) by receiving a firmware image from a remote BLE central (e.g., smartphone or PC tool).

## Table of Contents

- [High-Level Overview](#high-level-overview)
- [Table of Contents](#table-of-contents)
- [Purpose/Scope](#purposescope)
- [Prerequisites/Setup Requirements](#prerequisitessetup-requirements)
  - [Hardware Requirements](#hardware-requirements)
  - [Software Requirements](#software-requirements)
  - [NCP mode: host application and project files](#ncp-mode-host-application-and-project-files)
  - [Setup Diagram](#setup-diagram)
- [Steps to Run Demo](#steps-to-run-demo)
  - [Getting Started](#getting-started)
  - [Configuration and setup](#configuration-and-setup)
  - [Steps for execution](#steps-for-execution)
    - [Build and Run](#build-and-run)
    - [Firmware File Format](#firmware-file-format)
    - [Firmware Upgrade with Si Connect Mobile App](#firmware-upgrade-with-si-connect-mobile-app)
    - [Firmware Upgrade with Python Script](#firmware-upgrade-with-python-script)
    - [Appendix](#appendix)
- [Troubleshooting](#troubleshooting)
- [Resources](#resources)
- [Report Bugs and Get Support](#report-bugs-and-get-support)

## Purpose/Scope

This application demonstrates how to update the SiWx91x module firmware over-the-air (OTA) by receiving a firmware image from a remote BLE central (e.g., smartphone or PC tool). The SiWx91x device runs as a BLE peripheral with an OTA GATT server; a central device connects and sends either TA (transceiver) or M4 (MCU) firmware to program flash and reboot.

Supported upgrade methods:

- **Si Connect mobile app** (Android / iOS) scan, connect, and upload a `.gbl` firmware file.
- **Python script** PC-based tool to scan for the device, connect, and send firmware in chunks.

## Prerequisites/Setup Requirements

### Hardware Requirements

- Windows PC with Host interface (UART/SPI/SDIO).
- SiWx91x Wi-Fi Evaluation Kit. The SiWx91x supports multiple operating modes. See [Operating Modes](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/) for details.
  - SoC Mode:
    - Silicon Labs [BRD4325A, BRD4325B, BRD4325C, BRD4338A, BRD4339B, BRD4343A](https://www.silabs.com/)
    - Kits: SiWx917 AC1 Module Explorer Kit (BRD2708A)
  - PSRAM Mode:
    - Silicon Labs [BRD4340A, BRD4342A, BRD4325G](https://www.silabs.com/)
  - NCP Mode:
    - Silicon Labs [BRD4180B](https://www.silabs.com/);
    - Host MCU Eval Kit. This example has been tested with:
      - Silicon Labs [WSTK + EFR32MG21](https://www.silabs.com/development-tools/wireless/efr32xg21-bluetooth-starter-kit)
    - NCP Expansion Kit with NCP Radio boards
      - (BRD4346A + BRD8045A) [SiWx917-EB4346A]
      - (BRD4357A + BRD8045A) [SiWx917-EB4357A]
  - Interface and Host MCU Supported
    - SPI - EFR32
- BLE Smartphone for mobile OTA testing.
- Serial console tool (e.g., Tera Term, Docklight) for viewing logs.

### Software Requirements

- Embedded Development Environment.
- Download and install [Simplicity Studio](https://www.silabs.com/developers/simplicity-studio) with WiSeConnect extension.
- Download and install the Silicon Labs [Si Connect (formerly Simplicity Connect / EFR Connect App)](https://www.silabs.com/developers/simplicity-connect-mobile-app) in Android or iOS smartphones for BLE OTA firmware upgrade. Users can also use their choice of BLE apps available in Android/iOS smartphones.
- For Python script based OTA: Python 3.7.9 or above.

> **Note:** The provided mobile screenshots are from the Si Connect app; it is recommended to use the latest version.

### NCP mode: host application and project files

| Mode | Host / target | Project file (this example folder) |
|------|----------------|-------------------------------------|
| SoC | Application runs on SiWx91x. | `ble_fw_ota_upgrade_soc.slcp` |
| PSRAM | Application runs on SiWx91x with PSRAM-capable radio board. | `ble_fw_ota_upgrade_psram.slcp` |
| NCP (SPI) | Application runs on **EFR32** host; SiWx917 is the network co-processor over **SPI**. | `ble_fw_ota_upgrade_ncp.slcp` |

Open the `.slcp` for your kit from **`examples/snippets/ble/ble_fw_ota_upgrade/`** in Simplicity Studio. For NCP, follow [Getting started with NCP mode](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/getting-started-with-ncp-mode).

### Setup Diagram

![](resources/readme/ble_fw_ota_upgrade_soc_ncp.png)

Follow the [Getting Started with SiWx91x](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/getting-started-with-soc-mode) guides for hardware connections and Simplicity Studio setup. Ensure the SiWx91x module is loaded with the latest connectivity firmware as described in [SiWx91x Firmware Update](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started).

## Steps to Run Demo

### Getting Started

Refer to the instructions [here](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/) to:

- [Install Simplicity Studio](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#install-simplicity-studio)
- [Install WiSeConnect extension](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#install-the-wi-se-connect-extension)
- [Connect your device to the computer](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#connect-si-wx91x-to-computer)
- [Upgrade your connectivity firmware](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#update-si-wx91x-connectivity-firmware)
- [Create a Studio project](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#create-a-project). Choose **BLE - FW OTA Upgrade (SoC)**, **BLE - FW OTA Upgrade (PSRAM)**, or **BLE - FW OTA Upgrade (NCP)** as appropriate for your board.

For details on the project folder structure, see the [WiSeConnect Examples](https://docs.silabs.com/wiseconnect/latest/wiseconnect-examples/#example-folder-structure) page.

### Configuration and setup

The application can be configured to suit your requirements and development environment. Read through the following sections and make any changes needed.

- Open `ble_config.h` file and update/modify following macros,

  - `RSI_BLE_OTA_FWUP_PROFILE` refers to the device name advertised during BLE scanning.

    ```c
    #define RSI_BLE_OTA_FWUP_PROFILE "BLE_OTA_FWUP"
    ```

  - `FW_UPGRADE_TYPE` refers to the type of firmware to upgrade. Choose TA (transceiver) or M4 (MCU)  or COMBINED (TA+M4) firmware upgrade.

    ```c
    #define FW_UPGRADE_TYPE  TA_FW_UP   // or M4_FW_UP or COMBINED_FW_UP
    ```

  - Other BLE parameters (advertising interval, connection parameters, etc.) are defined in `ble_config.h` and can be adjusted if required.

> **Note:** ble_config.h files are already set with desired configuration in respective example folders; user need not change for each example. For recommended settings, please refer the [recommendations guide](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-prog-recommended-settings/).

### Steps for execution

#### Build and Run

Refer to the instructions [here](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/) to:

- Build the application in Studio.
- Flash, run, and debug the application.

Follow the steps below for successful execution of the application:

### Verify BLE FW OTA upgrade Application as a Server

1. After the program gets executed, if Silicon Labs device is configured as ``SERVER`` specified in the macro ``GATT_ROLE``, Silicon Labs device will be in Advertising state.

2. Connect any serial console for prints.

3. Open a Simplicity Connect App in the Smartphone and do the scan.

4. The device advertises with the name configured in `RSI_BLE_OTA_FWUP_PROFILE` (default: `BLE_OTA_FWUP`).

![BLE advertising](resources/readme/bleadv.png)

### Firmware File Format

Firmware files for OTA must be in **.gbl** format. If you have a **.rps** (or other) firmware file, rename or convert it to **.gbl** before using with Si Connect or the Python script.

![Firmware format](resources/readme/firmwareformat.png)

### Firmware Upgrade with Si Connect Mobile App

1. Launch the Si Connect (or Simplicity Connect) app and enable BLE. Start scanning and connect to the device advertising as `BLE_OTA_FWUP` (or your configured name).

   ![Si Connect scan and connect](resources/readme/mobilebleadv.png)

2. After connection, open the **OTA Firmware** option in the app.

   ![OTA Firmware option](resources/readme/bleconnection.png)

#### TA Firmware Upgrade

1. Use **Select application .gbl file** to choose the TA firmware (e.g., `SiWG917-B.2.13.3.3.0.3.gbl`).
   - **Note:** For 1.8 MB flash devices, use the matching 1.8 MB TA firmware.
2. Tap **Upload** to start the transfer. The app sends the firmware in chunks; the device programs flash and reboots when done.
3. Serial console will show OTA progress and success messages.

![TA firmware selection and upload](resources/readme/taselectedfw.png)
![TA firmware upgrade completed](resources/readme/tafwupcompleted.png)

#### M4 Firmware Upgrade

1. Build an M4 application (e.g., **BLE - Heart Rate (SoC)**). The generated binary (e.g., `ble_heart_rate_profile_soc.rps` or `ble_heart_rate_profile_soc_isp.bin`) must be renamed to **.gbl** for the app.
2. In Si Connect, use **Select application .gbl file** to choose the M4 **.gbl** file, then tap **Upload**.
3. After completion, the device reboots and runs the new M4 application.

![M4 firmware upgrade](resources/readme/m4fwupcompleted.png)

> **Note:** M4 firmware upgrade is supported in SoC mode only; NCP supports TA firmware upgrade only.

### Firmware Upgrade with Python Script

A Python-based OTA tool is provided to run on a PC: scan for the device, connect, and send firmware (TA or M4) in chunks.

1. Navigate to the Python script directory in the example (e.g., `tools/Python_script` or the path indicated in the release).

   ```
   <SDK>/examples/snippets/ble/ble_fw_ota_upgrade/tools/Python_script
   ```

2. Run the script:

   ```sh
   python Si917-OTA Firmware Update Python Script.py
   ```

3. Click **START**. The script scans for the device named **BLE_OTA_FWUP**, connects, and displays device info (name, MAC, firmware version). By default it is configured for TA firmware upgrade.

   ![Python script â€“ connected](resources/readme/bleotafwupconnected.png)

#### TA Firmware Upgrade (Python)

1. Click **Update Firmware**, browse to the TA **.gbl** file, and open it.
2. Click **Start Firmware Update** in the dialog to begin. The script sends the firmware in chunks; the device programs flash and reboots on success.
3. Check the serial console for OTA progress and completion.

#### M4 Firmware Upgrade (Python)

1. Build an M4 example (e.g., **BLE - Heart Rate (SoC)**) and convert/rename the output to **.gbl**.
2. In the Python tool, click **Update Firmware** and select the M4 **.gbl** file.
3. Click **Start Firmware Update** to upload. After completion, the device reboots with the new M4 application.

> **Note:** The provided mobile and Python screenshots may differ slightly from the latest app/script versions; the workflow remains the same.

### Steps to Create a Combined Image

  #### Case : When Security is Disabled

  1. Navigate to the Commander directory.

  2. Copy the NWP firmware image and M4 image into the Commander directory.

  3. Create the nwp_combined_image.rps file:

      ```c
      commander rps convert <nwp_combined_image.rps> --taapp <original non-encrypted TA rps> --combinedimage
      ```

  4. Create the m4_combined_image.rps file:

      ```c
      commander rps convert <m4_combined_image.rps> --app <original non-encrypted M4 rps> --combinedimage
      ```

  5. Create the final combined image:

      ```c
      commander rps convert <combined_image.rps> --app <m4_combined_image.rps> --taapp <nwp_combined_image.rps>
      ```
  ![](resources/readme/TA_M4_combined_image_generation.png)
## Appendix

- **SiWx91x NCP** supports **TA firmware upgrade only**.
- **SiWx91x SoC** supports both **TA** and **M4** firmware upgrades.
- Use **.gbl** format for firmware files with Si Connect and the Python OTA script.
- Ensure the TA/M4 firmware version and flash size match your SiWx91x module (e.g., 1.8 MB).
- **Note:** Intermittent disconnections may be observed while upgrading the firmware. If the upgrade fails or disconnects, retry the OTA process.
- Anti-rollback feature is not supported for this application.
- During the firmware upgrade, the mobile device running the Si Connect app should not enter sleep mode. Keep the screen on or disable sleep/auto-lock for the duration of the OTA transfer to avoid interrupting the upgrade.
- Before upgrading firmware, the user should disable power save mode.
- This Application support only single connection(GATT server) only.
- If FW upgrade failed or wrong FW selected Then reconnect the Device and intiate the FW upgrade.

## Troubleshooting

| Symptom | Things to check |
|--------|------------------|
| No connection / scan issues | Confirm the peer address type and `RSI_BLE_DEV_ADDR` / `RSI_REMOTE_DEVICE_NAME` match the peripheral; phones often use random addresses. |
| Slow OTA or lower-than-expected transfer speed | Connection interval strongly affects throughput—**speed is proportional to how often connection events occur**; adjust connection parameters in `ble_config.h` (and ensure the central honors them). Trade-offs apply for power and compatibility. |
| NCP: no boot or no HCI traffic | Update SiWx917 connectivity firmware; verify SPI/UART wiring per [NCP getting started](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/getting-started-with-ncp-mode). Flash the correct `*_ncp.slcp` (or `*_uart_ncp.slcp`) on the **EFR32** host. |
| Power save anomalies on NCP expansion board | See the power-save note in the **Configuration and Setup** section and the *Getting started with SiWx91x NCP* guide. |
| Build or flash errors | Open the `.slcp` that matches your kit (SoC vs PSRAM vs NCP) and matching SDK / WiSeConnect versions. |


## Resources

1. [WiSeConnect getting started](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/)
2. [WiSeConnect developers guide — developing for Silicon Labs hosts](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/)
3. [Programming recommended settings](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-prog-recommended-settings/)


## Report Bugs and Get Support

Report issues and get help from the Silicon Labs community:

- [Silicon Labs Community](https://www.silabs.com/community)
