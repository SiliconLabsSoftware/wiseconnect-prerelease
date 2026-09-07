# SiWx91x Platform Memory Manager Heap 3

## Table of Contents

- [SiWx91x Platform Memory Manager Heap 3](#siwx91x-platform-memory-manager-heap-3)
  - [Table of Contents](#table-of-contents)
  - [Purpose/Scope](#purposescope)
  - [Overview](#overview)
  - [About Application Code](#about-application-code)
  - [Prerequisites/Setup Requirements](#prerequisitessetup-requirements)
    - [Hardware Requirements](#hardware-requirements)
    - [Software Requirements](#software-requirements)
  - [Getting Started](#getting-started)
  - [Application Build Environment](#application-build-environment)
  - [Test the Application](#test-the-application)
  - [Expected Results](#expected-results)
  - [Troubleshooting](#troubleshooting)
  - [Resources](#resources)
  - [Report Bugs/Support](#report-bugssupport)

## Purpose/Scope

This application demonstrates **Gecko Native Memory Manager** with **FreeRTOS Heap 3** on the SiWx917 SoC. It prints heap statistics in hexadecimal and shows the allocator paths customers use most often: C library, Native Memory Manager, FreeRTOS heap, and one memory pool.

## Overview

The FreeRTOS application thread runs this sequence:

1. Print the **initial** heap snapshot (region base/size, total, free, used, high-watermark, free/used block counts).
2. C library `malloc` / `calloc` / `realloc` / `free`, including 8-byte alignment checks.
3. Native `sl_malloc` / `sl_calloc` / `sl_realloc` / `sl_free`.
4. FreeRTOS `pvPortMalloc` / `vPortFree`, plus queue and task creation.
5. Print the heap snapshot **after** those allocations and frees.
6. Create **one memory pool**, allocate and use its blocks, free them, then delete the pool.
7. Print the **final** heap snapshot.

All heap sizes and pointer addresses are printed in hexadecimal so memory usage is easy to correlate with addresses in the log.

This is an MCU/service-focused application. No Wi-Fi or BLE stack is started.

## About Application Code

| File | Description |
|------|-------------|
| [`memory_manager_heap3_application.c`](memory_manager_heap3_application.c) / [`.h`](memory_manager_heap3_application.h) | CMSIS-RTOS2 thread that prints heap statistics and runs the allocator and pool demos |
| [`app.c`](app.c) / [`app.h`](app.h) | Top-level `app_init()` entry that starts the application thread |

## Prerequisites/Setup Requirements

### Hardware Requirements

- Windows PC
- Silicon Labs SiWx91x Evaluation Kit [[BRD4002](https://www.silabs.com/development-tools/wireless/wireless-pro-kit-mainboard?tab=overview) + [BRD4338A](https://www.silabs.com/development-tools/wireless/wi-fi/siwx917-rb4338a-wifi-6-bluetooth-le-soc-radio-board?tab=overview) / [BRD4342A](https://www.silabs.com/development-tools/wireless/wi-fi/siwx91x-rb4342a-wifi-6-bluetooth-le-soc-radio-board?tab=overview) / [BRD4343A](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-rb4343a-wi-fi-6-bluetooth-le-8mb-flash-radio-board-for-module?tab=overview) / [BRD4343C](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-rb4343c-wi-fi-6-bluetooth-le-8mb-flash-radio-board-for-module?tab=overview)]
- SiWx917 AC1 Module Explorer Kit [BRD2708A](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-ek2708a-explorer-kit?tab=overview)
- USB cable for programming and VCOM

### Software Requirements

- Serial console setup
  - For serial console setup instructions, refer to [Console input and output](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#console-input-and-output).
- Embedded Development Environment
  - Use the latest version of Simplicity Studio. See [Setup software](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#setup-software).

## Getting Started

Refer to the instructions [here](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/) to:

- [Install Simplicity Studio](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#install-simplicity-studio)
- [Install WiSeConnect extension](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#install-the-wiseconnect-3-extension)
- [Connect your device to the computer](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#connect-siwx91x-to-computer)
- [Upgrade your connectivity firmware](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#update-siwx91x-connectivity-firmware)
- [Create a Studio project](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#create-a-project)

Open [`sl_si91x_memory_manager_heap3.slcp`](sl_si91x_memory_manager_heap3.slcp) in Simplicity Studio, select your SiWx917 board, and generate the project.

For details on the project folder structure, see the [WiSeConnect Examples](https://docs.silabs.com/wiseconnect/latest/wiseconnect-examples/#example-folder-structure) page.

## Application Build Environment

The application uses the following components:

- `freertos` / `freertos_heap_3` — FreeRTOS with Heap 3
- `memory_manager` — Gecko Native Memory Manager
- `si91x_memory_default_config` — Default SiWx91x memory configuration
- `iostream_uart_si91x` (VCOM) — UART console output

No application configuration macros are required. Optional demo sizes live in [`memory_manager_heap3_application.c`](memory_manager_heap3_application.c):

| Macro | Default | Meaning |
|-------|---------|---------|
| `MM_HEAP3_DEMO_BLOCK_SIZE` | `256` | Block size used by the C library, Native, and FreeRTOS demos |
| `MM_HEAP3_DEMO_CALLOC_COUNT` | `16` | Element count for calloc / sl_calloc |
| `MM_HEAP3_DEMO_CALLOC_ELEM_SIZE` | `8` | Element size (bytes) for calloc / sl_calloc |
| `MM_HEAP3_DEMO_REALLOC_INITIAL_SIZE` | `64` | Size before realloc / sl_realloc grows the block |
| `MM_HEAP3_DEMO_POOL_BLOCK_SIZE` | `64` | Size of each block in the demo pool |
| `MM_HEAP3_DEMO_POOL_BLOCK_COUNT` | `4` | Number of blocks in the demo pool |

## Test the Application

1. Build and flash the application.
2. Open the VCOM serial port (115200 8N1 typical) and reset the board.
3. Confirm the banner and the compact `initial` heap snapshot.
4. Confirm C library / Native / FreeRTOS / Pool lines show matching `allocated` and `freed` values.
5. Confirm the `after` and `final` heap snapshots, then `Done.`

## Expected Results

A successful run prints a short log similar to:

```text
============================================================
 SiWx91x Memory Manager + FreeRTOS Heap 3 application
============================================================
[MM] initial   base=0x00007F00  total=0x00027D00  free=0x000257E0  used=0x00002520  high=0x00002520
[MM] C library        allocated=0x00000280  freed=0x00000280
[MM] Native           allocated=0x00000280  freed=0x00000280
[MM] FreeRTOS         allocated=0x00000100  freed=0x00000100  queue/task=ok
[MM] after     base=0x00007F00  total=0x00027D00  free=0x00026860  used=0x000014A0  high=0x000029A8
[MM] Pool            allocated=0x00000100  freed=0x00000100
[MM] final     base=0x00007F00  total=0x00027D00  free=0x00026860  used=0x000014A0  high=0x000029A8
[MM] Done.
```

- Heap snapshots print `base`, `total`, `free`, `used`, and `high` in hexadecimal.
- Each demo prints how many bytes it allocated and freed (hex). Matching `allocated` and `freed` means that demo reclaimed its blocks.
- After the demos and after the pool is deleted, free/used should match between the `after` and `final` snapshots.

## Troubleshooting

- If no UART output appears, confirm VCOM is enabled (`SL_BOARD_ENABLE_VCOM=1`) and the correct COM port is open.
- If Studio still lists deleted test sources after updating the SDK, regenerate the project from the `.slcp` file.
- Absolute free/used values depend on board configuration and other stack usage; compare snapshots within the same run.

## Resources

- [WiSeConnect Documentation](https://docs.silabs.com/wiseconnect/latest/)
- [WiSeConnect Getting Started](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/)

## Report Bugs/Support

- Open a ticket at [Silicon Labs Support](https://www.silabs.com/support)
