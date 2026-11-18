# SiWx91x Platform - SL Diagnostic Crash Capture

## Table of Contents

- [SiWx91x Platform - SL Diagnostic Crash Capture](#siwx91x-platform---sl-diagnostic-crash-capture)
  - [Table of Contents](#table-of-contents)
  - [Purpose/Scope](#purposescope)
  - [Overview](#overview)
  - [About Example Code](#about-example-code)
  - [Prerequisites/Setup Requirements](#prerequisitessetup-requirements)
    - [Hardware Requirements](#hardware-requirements)
    - [Software Requirements](#software-requirements)
  - [Getting Started](#getting-started)
  - [Application Build Environment](#application-build-environment)
    - [Configuration Options](#configuration-options)
  - [Test the Application](#test-the-application)
  - [Expected Results](#expected-results)
  - [Troubleshooting](#troubleshooting)
  - [Resources](#resources)
  - [Report Bugs/Support](#report-bugssupport)

## Purpose/Scope

This example demonstrates SL Diagnostic (`sl_diag`) with Crash Capture on the SiWx91x SoC in a bare-metal configuration. Press **BTN0** to raise a HardFault. Crash Capture seals the CPU state into the retained SL Diagnostic record and resets the device. After the reset, the decoded record and a raw hex dump of the retained region are printed over VCOM. The application clears the record only after the export succeeds and `sl_diag_has_record()` is false, so the next BTN0 press can seal a new generation on any enabled profile. A failed export or a failed clear leaves the record in place.

## Overview

`sl_diag` preserves diagnostic evidence in retained RAM so that it is still available after a reset. Crash Capture is the producer for CPU crashes: its HardFault handler captures the exception frame and the fault status registers, then hands them to `sl_diag`, which writes and commits a CRC-protected record of TLV blocks.

The example shows the complete crash path for a HardFault:

1. **Boot**: `sl_diag` validates its retained region and Crash Capture registers the CPU Crash provider. Both run from the `service_init` event, before `app_init()`. On SiWx91x, the `si91x_diag` integration component is pulled in automatically and latches the SiWx91x reset cause.
2. **Previous record**: If a sealed record from the previous boot exists, the application prints it, then clears it with `sl_diag_clear(SL_DIAG_CLEAR_RECORD)`.
3. **Crash**: Press **BTN0**. The application executes an undefined instruction. Crash Capture records PC, LR, SP, R0-R3, R12, xPSR, CFSR, HFSR, BFAR and MMFAR, seals the record and resets the device through the SiWx91x soft-reset sequence (`sl_si91x_soc_nvic_reset()`).
4. **Post-reset**: The record is printed again and cleared. Press BTN0 for another cycle. `generation` advances on each new seal.

## About Example Code

- [`diag_crash_capture_app.c`](diag_crash_capture_app.c) - Reads, decodes, prints and clears the retained record at boot, and raises the HardFault when BTN0 is pressed.
- [`app.c`](app.c) - Top-level application functions that call the example init and process functions.

## Prerequisites/Setup Requirements

### Hardware Requirements

- Windows PC
- Silicon Labs SiWx91x Evaluation Kit [[BRD4002](https://www.silabs.com/development-tools/wireless/wireless-pro-kit-mainboard?tab=overview) + [BRD4338A](https://www.silabs.com/development-tools/wireless/wi-fi/siwx917-rb4338a-wifi-6-bluetooth-le-soc-radio-board?tab=overview) / [BRD4342A](https://www.silabs.com/development-tools/wireless/wi-fi/siwx91x-rb4342a-wifi-6-bluetooth-le-soc-radio-board?tab=overview) / [BRD4343A](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-rb4343a-wi-fi-6-bluetooth-le-8mb-flash-radio-board-for-module?tab=overview)]
- SiWx917 AC1 Module Explorer Kit (BRD2708A)

### Software Requirements

- Simplicity Studio
- A serial terminal connected to the kit VCOM port

## Getting Started

1. Import the **SiWx91x Platform - SL Diagnostic Crash Capture** example in Simplicity Studio.
2. In the **Logger** UC screen, set `SL_LOG_CONFIG_LEVEL_COMPILE_TIME` to `SL_LOG_CONFIG_LEVEL_INFO`. The default is ERROR, which hides the example output and the crash record.
3. Build the project.
4. Flash with Simplicity Commander, or from Studio **without** leaving a debug session attached. Open only a serial terminal on VCOM.

## Application Build Environment

The example uses the following components:

- **sl_main**: Main application framework
- **sl_diag**: SL Diagnostic retained record service (pulls in `si91x_diag` on SiWx91x)
- **si91x_crash_capture**: SiWx91x CPU Crash provider for `sl_diag`. It provides `crash_capture`, so the platform Crash Capture component is not installed
- **si91x_log** and **si91x_log_iostream_formatted**: Logger with formatted IO Stream output
- **iostream_uart_si91x** (`vcom`): VCOM console
- **sl_si91x_button_917** (`btn0`): Button used to trigger the HardFault

### Configuration Options

The default profile is **Crash Basic**. Change `SL_DIAG_PROFILE` and the other options in the **SL Diagnostic** UC screen.

| Option | Default in this example | Effect |
| --- | --- | --- |
| `SL_DIAG_PROFILE` | `SL_DIAG_PROFILE_CRASH_BASIC` | Change in UC for Standard or higher (adds `boot_id`, Trigger, Reset/Boot and Build metadata, and optionally Summary) |
| `SL_LOG_CONFIG_LEVEL_COMPILE_TIME` | `SL_LOG_CONFIG_LEVEL_ERROR` | The example output and the record export are printed at INFO. Set this to `SL_LOG_CONFIG_LEVEL_INFO` (or `DEBUG`) in the **Logger** UC screen to see them. At the default ERROR level only failure messages are printed, and the record is still cleared after the export |
| `SL_DIAG_SLOT_COUNT` | UC default | `2` enables dual-slot copy-on-commit |
| `SL_DIAG_STACK_SNAPSHOT_BYTES` | UC default | Size of the stack snapshot in the CPU Crash TLV (`0` disables it) |

`SL_DIAG_PROFILE_OFF` is not supported: Crash Capture requires an enabled profile.

## Test the Application

The output below assumes the log level is set to INFO, as described in [Getting Started](#getting-started).

1. On the first boot no record exists:

   ```text
   SL Diagnostic crash capture example
   No crash record.
   Press BTN0 to raise a HardFault (Crash Capture seals + resets).
   After reset, compare generation with the previous print.
   ```

2. Press **BTN0**. The device faults, Crash Capture seals the record, then soft-resets.
3. After the reset, the record from the previous boot is printed, then cleared.
4. Press **BTN0** again. After the next reset, `generation` should be higher than in step 3.

Look up the printed PC in the `.map` file or with `arm-none-eabi-addr2line -e <image>.out <PC>`.

## Expected Results

```text
A crash record from a previous boot was found:
SL Diagnostic record
  state          : VALID
  generation     : 1
  ...
CPU crash evidence
  PC   : 0x0XXXXXXX  <- faulting instruction
  ...
  CFSR : 0x00010000
    CFSR: UNDEFINSTR (undefined instruction)
  HFSR : 0x40000000
    HFSR: FORCED (configurable fault escalated to HardFault)
  ...
=== STREAM ===
...
Record cleared; next BTN0 will seal a new generation.
Press BTN0 to raise a HardFault (Crash Capture seals + resets).
```

Crash Capture enables the MemManage, BusFault and UsageFault handlers at initialization. Before faulting, the example clears those enables in `SCB->SHCSR` and executes `udf #0`. The UsageFault escalates to a HardFault (`HFSR.FORCED`, `CFSR.UNDEFINSTR`). Crash Capture seals the record into `.sl_diag_retained.noinit` and resets through `sl_si91x_soc_nvic_reset()`.

The record survives a software reset. A power cycle clears retained RAM.

## Troubleshooting

- **No VCOM output**: The default log level is ERROR, which hides the example output. Set `SL_LOG_CONFIG_LEVEL_COMPILE_TIME` to INFO in the **Logger** UC screen. Also confirm the terminal uses the correct port and baud rate, and keep it open before pressing BTN0.
- **Record cleared without being printed**: The export is printed at INFO, but the record is cleared regardless of the log level. Below INFO, a record from the previous boot is lost without being shown. Set the level to INFO before triggering a crash you need to inspect.
- **The device resets but no record is printed**: Detach the debugger, or disable halt-on-HardFault. If the debugger catches the fault, Crash Capture never seals the retained record.
- **The device does not reset after the HardFault message**: A debugger may halt the core on the fault. Run without a debug session, or resume the core.
- **No record after a power cycle**: The record is kept in retained RAM, which a power cycle clears.
- **`generation` does not advance**: The example clears the record only after a successful export. If the export fails, Crash Basic preserves the first record and Standard may only increment `repeat_count`.
- **`boot_id` missing from the print**: Present only when the profile includes incident metadata (Crash Standard or higher).

## Resources

- [WiSeConnect Getting Started](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/)
- [WiSeConnect Examples](https://docs.silabs.com/wiseconnect/latest/wiseconnect-examples/)
- [SiWx91x SoC Documentation](https://docs.silabs.com/wiseconnect/latest/)

## Report Bugs/Support

For issues and support, use the Silicon Labs Community or your normal support channel.
