# SiWx91x Platform Empty C++ FreeRTOS

## Table of Contents

- [SiWx91x Platform Empty Cpp FreeRTOS](#siwx91x-platform-empty-cpp-freertos)
  - [Table of Contents](#table-of-contents)
  - [Purpose/Scope](#purposescope)
  - [Overview](#overview)
  - [About Example Code](#about-example-code)
  - [Prerequisites/Setup Requirements](#prerequisitessetup-requirements)
  - [Getting Started](#getting-started)
  - [Test the Application](#test-the-application)
  - [Troubleshooting](#troubleshooting)
  - [Resources](#resources)
  - [Report Bugs/Support](#report-bugssupport)

## Purpose/Scope

Minimal **C++** application on SiWx91x with **FreeRTOS**. Use it as a starting point to add components, drivers, and tasks without unrelated sample logic.

## Overview

This example project provides a minimal starting point for developing applications on the SiWx91x device. It is designed to be a clean template where developers can build and test features incrementally.

The project demonstrates an empty configuration that includes the essential initialization along with default peripheral components such as:

- ADC
- BJT Temperature Sensor
- DMA
- ULP UART

## About Example Code

**FreeRTOS application structure**

- The `.slcp` includes **`freertos_heap_4`**. Top-level **`app.cpp`** implements **`app_init()`**, which calls **`empty_cpp_freertos_init()`** (declared in **`empty_cpp_freertos.h`** with **C linkage** so it matches the startup path). **`app_process_action()`** does nothing; work is intended to live in RTOS tasks you add.
- **`empty_cpp_freertos.cpp`**: **`empty_cpp_freertos_init()`** is **`extern "C"`** and registers **`empty_cpp_task`** with **`osThreadNew`**. Thread attributes use **`osThreadAttr_t`** (see **C++ notes** for why fields are assigned after **`{}`** instead of a partial designated initializer). The task loops forever with **`osDelay(1000)`**. **`osDelay`** counts **kernel ticks** (with a 1 kHz tick rate, **1000** ticks is typically about one second).

## Prerequisites/Setup Requirements

### Hardware Requirements

- Windows PC
- Silicon Labs SiWx91x Evaluation Kit [[BRD4002](https://www.silabs.com/development-tools/wireless/wireless-pro-kit-mainboard?tab=overview) + [BRD4338A](https://www.silabs.com/development-tools/wireless/wi-fi/siwx917-rb4338a-wifi-6-bluetooth-le-soc-radio-board?tab=overview) / [BRD4342A](https://www.silabs.com/development-tools/wireless/wi-fi/siwx91x-rb4342a-wifi-6-bluetooth-le-soc-radio-board?tab=overview) / [BRD4343A](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-rb4343a-wi-fi-6-bluetooth-le-8mb-flash-radio-board-for-module?tab=overview) / [BRD4343C](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-rb4343c-wi-fi-6-bluetooth-le-8mb-flash-radio-board-for-module?tab=overview)]
- SiWx917 AC1 Module Explorer Kit [BRD2708A](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-ek2708a-explorer-kit)

### Software Requirements

- Simplicity Studio
- Serial console setup
  - For serial console setup instructions, see the [Console Input and Output](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#console-input-and-output) section in the *WiSeConnect Developer's Guide*.

### Toolchain

- **ARM GCC** with **C++** support (`stdc++` is listed in the `.slcp` **library** section)

## Getting Started

Refer to the instructions [here](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/) to install Simplicity Studio, connect your device, and create a project. For project folder structure, see [WiSeConnect Examples](https://docs.silabs.com/wiseconnect/latest/wiseconnect-examples/#example-folder-structure).


## Test the Application

1. After reset, FreeRTOS runs; the **`empty_cpp`** task blocks in **`osDelay(1000)`** (kernel ticks) while the idle task and other system threads run.
2. Add breakpoints or logs in **`empty_cpp_task`** to verify execution.

## Troubleshooting

- If the project does not build, ensure Simplicity Studio and the WiSeConnect extension are installed and the board is connected.
- If the device is not detected, reinstall the connectivity firmware and check USB drivers.
- No console output: verify UART/retarget settings and [WiSeConnect Troubleshooting](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-troubleshooting/).

## Resources

- [WiSeConnect Getting Started](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/)
- [WiSeConnect Examples](https://docs.silabs.com/wiseconnect/latest/wiseconnect-examples/)
- [Si91x SoC Documentation](https://docs.silabs.com/wiseconnect/latest/)

## Report Bugs/Support

For issues and support, use the [Silicon Labs Community](https://community.silabs.com/) or your normal support channel.
