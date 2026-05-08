# MCU Secure Storage

## Table of Contents

- [SL Secure Storage](#sl-secure-storage)
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
  - [Test the Application](#test-the-application)
  - [Expected Results](#expected-results)
  - [Example Folder Structure](#example-folder-structure)

## Purpose/Scope

This application demonstrates MCU Secure Storage on Si91x: write predefined values to REG0–REG7, enable protection and lock, then read back and verify in a single run. The app uses per-register driver APIs via batch helpers and ends by halting (interrupts disabled) until the watchdog resets the system.

After that reset, **firmware starts again from the beginning**, so **each boot** runs the same flow (init, write or skip-write, protect, read, verify, then halt for WDT again). That is how you repeatedly exercise read/verify over reset without depending on RAM-only application state. The **watchdog is only the demo’s way to end one run**; it is not part of the secure storage feature itself.

## Overview

- **Secure Storage:** Read and write of 8 MCU secure storage registers (REG0–REG7) via the driver APIs. The app writes distinct 32-bit values to each register, then reads them back and verifies.
- **Write key protection:** REG0–REG3 are write-protected by hardware; the driver enables the write key internally before each write and locks it after. REG4–REG7 do not use the write key. The app uses batch helpers that call the driver’s per-register APIs.
- **Optional skip-write:** Before writing, the app reads current values; if they already match the desired values, it skips writing.
- **Protection and lock:** After writing, the app optionally calls to enable MCU secure storage write protection via NWP handshake and lock REG0–REG3 (when ENABLE_SECURE_PROTECTION is 1).
- **Single verify then halt:** The app reads all 8 registers once, verifies them against the expected values, then disables interrupts and spins so the watchdog eventually resets the system. There is no sleep/wakeup loop.
- **Repeat on every boot:** Secure storage **retains** its contents across reset. On the **next** boot after a watchdog (or power) reset, the example runs the full sequence again, including **another** read/verify. The write step may print **“skipping write”** if registers still hold the expected values from the previous run.
- **WiFi and power save:** WiFi client and network stack initialization; deep sleep with RAM retention is configured (for reference; the demo does not enter a sleep/wake loop).

## About Example Code

- [`app.c`](app.c) – Application that initializes WiFi and the network stack, writes predefined values to REG0–REG7 (write key for REG0–REG3 is handled inside the driver), optionally enables secure protection and lock, sets power profile to deep sleep with RAM retention, then reads all registers, verifies them against the expected values, disables interrupts, and spins until the watchdog resets the system. Before writing, the app may skip the write if current register values already match the desired values.

**Note:** Secure storage registers retain values across power cycles and typical resets. REG0–REG3 are protected by the write key (handled inside the driver); REG4–REG7 have no write key. The intentional WDT reset at the end of each run causes a normal reboot so you can observe that **read/verify runs again** on the next boot; the WDT path is **not** required for real products using secure storage.

## Prerequisites/Setup Requirements

### Hardware Requirements

- Windows PC
- Silicon Labs Si917 Evaluation Kit [[BRD4002](https://www.silabs.com/development-tools/wireless/wireless-pro-kit-mainboard?tab=overview) + [BRD4338A](https://www.silabs.com/development-tools/wireless/wi-fi/siwx917-rb4338a-wifi-6-bluetooth-le-soc-radio-board?tab=overview) / [BRD4342A](https://www.silabs.com/development-tools/wireless/wi-fi/siwx91x-rb4342a-wifi-6-bluetooth-le-soc-radio-board?tab=overview) / [BRD4343A](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-rb4343a-wi-fi-6-bluetooth-le-8mb-flash-radio-board-for-module?tab=overview)]
- SiWx917 AC1 Module Explorer Kit [BRD2708A](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-ek2708a-explorer-kit)

### Software Requirements

- Simplicity Studio
- WiSeConnect extension
- Serial console setup: refer to [Console Input and Output](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#console-input-and-output).
- For the hardware setup, see [Setup Diagram](#setup-diagram).

### Setup Diagram

![Setup Diagram](resources/readme/setupdiagram.png)

## Getting Started

Refer to the instructions [here](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/) to:

- [Install Simplicity Studio](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#install-simplicity-studio)
- [Install WiSeConnect extension](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#install-the-wiseconnect-3-extension)
- [Connect your device to the computer](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#connect-siwx91x-to-computer)
- [Upgrade your connectivity firmware](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#update-siwx91x-connectivity-firmware)
- [Create a Studio project](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#create-a-project)

For details on the project folder structure, see the [WiSeConnect Examples](https://docs.silabs.com/wiseconnect/latest/wiseconnect-examples/#example-folder-structure) page.

## Application Build Environment

- **Project configurator:** In Project Explorer, double-click `sl_si91x_mcu_secure_storage.slcp`. Use the Software Component tab to add or configure components.
- **Secure Storage config:** Open `config/sl_si91x_secure_storage_config.h` from Project Explorer → config. Use the Configuration tab at the bottom of the editor for options (e.g. Secure Protection).
- **App defines (in `app.c`):**
  - `APP_SECURE_STORAGE_DEBUG_PRINT_VALUES` – Set to 1 to print written and read register values on the console.
  - `APP_SECURE_STORAGE_REG_VALUE_1` through `APP_SECURE_STORAGE_REG_VALUE_8` – Values written to REG0–REG7 at init.

## Test the Application

1. Build and run the application.
2. Open the serial console. You should see WiFi init, firmware version, secure storage write (or “skipping write” if values already match—for example on a **second** boot after registers were programmed), optional “MCU secure storage protection enabled, secure storage write disabled”, power save config, then a single read/verify. “All register values matched” or mismatch messages appear. Finally, the app reports that interrupts are disabled and the WDT will reset the system, then the device resets when the watchdog fires. **After reset, the same log sequence can appear again** for another full pass.
3. If `APP_SECURE_STORAGE_DEBUG_PRINT_VALUES` is 1, written and read values are printed for each register.

## Expected Results

- **WiFi and network:** WiFi client interface and network stack initialize successfully; firmware version is printed.
- **Power save:** Deep sleep with RAM retention is configured.
- **Secure storage write:** All 8 registers (REG0–REG7) are written (or write is skipped if current values already match); the driver handles write-enable for REG0–REG3 internally.
- **Protection and lock:** When ENABLE_SECURE_PROTECTION is 1, MCU secure storage protection is enabled and secure storage write is disabled; otherwise a message indicates protection is disabled in config.
- **Read and verify:** All 8 registers are read once and compared to expected values; console shows “All register values matched” or mismatch messages.
- **Halt and reset:** The application disables interrupts and spins; the watchdog eventually resets the system (no sleep/wakeup loop). This exit method is **for the example only**; product code would not need a WDT-driven loop to use secure storage.
- **Next boot:** The application runs again from the start; read/verify executes again. If protection and retained register data still match the expected pattern, you may see **skip-write** and another successful verify.

### Debug Output Example

```text
Firmware version is: 1611.2.1.1.255.11.63

Register Read and write values differ, writing registers.
MCU secure storage protection enabled, secure storage write disabled

All register values matched.

disabled interrupts so WDT is no longer kicked; WDT will reset the system.
```

(If registers already contained the expected values you may see “Register Read and before write values matched, skipping write.” instead of “writing registers.”)

> **Note:**
>
> - Secure storage registers retain values across power cycles. REG0–REG3 are protected by the write key (handled inside the driver); REG4–REG7 have no write key.
> - Each time the device resets and the app starts, you get another read/compare pass; the WDT at the end of each run simply triggers the next cycle.
