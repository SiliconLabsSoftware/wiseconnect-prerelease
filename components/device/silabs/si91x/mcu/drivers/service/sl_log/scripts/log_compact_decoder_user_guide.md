# Log compact decoder (`log_compact_decoder.py`)

Host-side tool for **real-time decoding** of SL Log **compact binary** output from UART (VCOM) or SEGGER RTT. It decodes host application logs (core 0) and, when present in the stream, secondary / captive-core logs (core 1). Format strings are resolved from the `.log_fmt` ELF section in your build artifacts, not from flash at runtime.

```text
components/device/silabs/si91x/mcu/drivers/service/sl_log/scripts/log_compact_decoder.py
components/device/silabs/si91x/mcu/drivers/service/sl_log/scripts/log_compact_decoder_user_guide.md
```

## Prerequisites

Python 3.8+ on the host (Windows, macOS, or Linux).

```bash
pip install pyelftools pyserial
```

Optional:

| Package | When |
|--------|------|
| `pylink-square` | `--source rtt` (J-Link RTT) |
| `colorama` | Colored console output (used automatically if installed) |

Firmware requirements:

- Log backend **Log Backend I/O Stream – Compact Binary Output** (compact IO stream or UART log backend, per your SDK).
- If your product merges secondary-core logs into the same transport, enable that path in firmware and supply the matching `--nwp-out` ELF (see [SiWx91x NWP notes](#siwx91x-nwp-notes)).

## Decode inputs

| Argument | Required | Notes |
|----------|----------|--------|
| `--app-out` | Yes, for host logs | Host / M4 application ELF (`.out` / `.axf`) built with compact logging. Contains `.log_fmt` for core 0. Also used for RTT control-block lookup and `%s` string resolution. **Aliases:** `--m4-out`, `--out`, `--axf` (matches the platform decoder). |
| `--nwp-out` | Yes, for NWP logs | NWP ELF with `.log_fmt` (for example `nwp_logging_fw.out`) for core 1 Wi-Fi / BLE logs. Alias: `--noload`. Legacy alias: `--ta-out`. |
| `--descriptor` | No | Legacy BTDM SystemView manifest (`SYSVIEW_CaptiveCore.txt`) for older NWP images that still send numeric event IDs. Not needed when the NWP image uses address-based logging and `--nwp-out` is given. |

## Optional arguments (common)

| Argument | Default | Purpose |
|----------|---------|---------|
| `--port` | Auto-detected J-Link CDC port | Serial port for the board VCOM, e.g. `COM4`, `/dev/ttyACM0`. Pass it explicitly when several boards are connected. |
| `--baud` | `115200` | UART baud rate. Prefer **`921600`** when the stream carries high-volume traffic (for example **DEBUG** level, host + NWP compact logs on the same VCOM). Lower rates can drop or delay M4 + NWP records. |
| `--max-args` | `10` | Must match **CONFIG_MAX_ARGS** / `SL_LOG_CONFIG_ARG` in the project (often `3` or `10`). |
| `--layout` | `auto` | **`auto`** (default): core 0 host records, repacked core 1 (Host Mode UART), and raw 28-byte NWP wire. **`nwp`**: core 1 only—28-byte NWP plus host-packed `core_id=1` lines; no M4 (core 0) logs. |
| `--source` | `uart` | `uart` or `rtt`. |
| `--component` | All | Print only these NWP (core 1) components. Host / M4 (core 0) lines are always shown. Repeat the flag or pass a comma-separated list, e.g. `--component WLAN_LMAC,WLAN_UMAC`. See [NWP component filtering](#nwp-component-filtering). |
| `--min-level` | None | Hide lines below this severity for **both** cores: `DEBUG`, `INFO`, `WARN`, or `ERROR`. `TRACE` is treated as `DEBUG`. `CRASH` and buffer `OVERFLOW` lines are always shown. |
| `--component-level` | None | Per-component minimum level for **NWP (core 1)** lines only, e.g. `--component-level WLAN_LMAC=WARN,BT_BLE_CTRL=INFO`. Repeat the flag or use comma-separated `COMP=LEVEL` pairs. Ignored when a line has no resolved component. |
| `--event-width` | `56` | Width of the EVENT column. Longer names are clipped with `...`; the full text stays in MESSAGE. |
| `--log-file` | None | Also write decoded output to this file (plain text; add `--log-file-color` to keep colors). |
| `--list-formats` | | Dump `.log_fmt` addresses and strings from `--app-out` / `--nwp-out`, then exit. No UART / RTT connection. |

Run `python log_compact_decoder.py --help` for the full list, including RTT flags and diagnostics (`--dump-discards`).

### RTT options (`--source rtt`)

| Argument | Default | Purpose |
|----------|---------|---------|
| `--device` | See `--help` | J-Link target device name. **Optional** when the script default matches your board (SiWx91x defaults are listed under [SiWx91x NWP notes](#siwx91x-nwp-notes)). |
| `--rtt-speed` | `auto` | J-Link speed in kHz, or `auto`. On some SiWx91x boards prefer **`--rtt-speed 1000`** if `auto` fails to start the core. |
| `--rtt-channel` | `0` | Primary RTT up-buffer. |
| `--rtt-channel2` | — | Optional second up-buffer (for example NWP traffic on channel 8). |
| `--jlink-serial` | — | Select a specific probe when several J-Links are attached. |
| `--rtt-block-address` | From ELF | Fixed RTT control block address; otherwise taken from `--app-out` (`_SEGGER_RTT`) or J-Link RAM scan. |
| `--rtt-reset` | off (no reset) | Reset target on connect. Default is attach without reset (often more reliable on multi-core SiWx91x targets). |

## Quick start (host logs only)

Build and flash the application with **Log Backend I/O Stream – Compact Binary Output**. Point the decoder at the **same** M4 ELF you flashed:

```bash
python log_compact_decoder.py \
  --app-out path/to/your_project.out \
  --port COM4 \
  --max-args 10
```

## Host + NWP logs

When captive-core logs share the UART, add the NWP logging ELF so core 1 events decode, and raise the baud rate:

```bash
python log_compact_decoder.py \
  --app-out app.out \
  --nwp-out nwp_logging_fw.out \
  --port COM4 \
  --max-args 10 \
  --baud 921600
```

Core 1 lines are prefixed with the firmware component name, e.g. `[WLAN_UMAC] ...`.

## Secondary-core component filtering (decoder)

When core 1 records use the captive-core component naming scheme, the decoder recognizes these names: `COMMON`, `CM_PM`, `WLAN_LMAC`, `WLAN_UMAC`, `WLAN_NETSTACK`, `BT_BLE_CTRL`, `BT_BLE_STACK`, `BTDM`.

| Where | How | Effect |
|-------|-----|--------|
| On the device | Firmware component mask (product-specific; not on every SoC) | Unselected components are silenced before they are sent—saves bandwidth. |
| On the PC | `--component` | Hides lines in the decode output; traffic is still received. Works on any capture. |

Device-side masking is described for **SiWx91x NWP** in [SiWx91x NWP notes](#siwx91x-nwp-notes). On other platforms (for example host-only or single-core products), use **`--component`** on the PC only.

### On the PC: `--component`

```bash
python log_compact_decoder.py \
  --app-out app.out \
  --nwp-out nwp_logging_fw.out \
  --port COM4 --baud 921600 \
  --component WLAN_LMAC
```

Only NWP lines with a matching component name are printed. Host / M4 lines are not matched by `--component`.

### On the PC: `--min-level` and `--component-level`

These filters apply to **decoded severity** from the binary record (the LEVEL column), not to the NWP component **mask** configured on the device.

- **`--min-level INFO`** — Drop `DEBUG` lines from both core 0 and core 1. Use this to thin a noisy UART capture without changing the firmware.
- **`--component-level WLAN_LMAC=WARN`** — For NWP lines that resolve to `WLAN_LMAC`, hide everything below `WARN`. Other components are unaffected unless you add more `COMP=LEVEL` pairs.

Example (host + NWP, show only WARN and above everywhere, but only ERROR from BLE control):

```bash
python log_compact_decoder.py \
  --app-out app.out \
  --nwp-out nwp_logging_fw.out \
  --port COM4 --baud 921600 \
  --min-level WARN \
  --component-level BT_BLE_CTRL=ERROR
```

## RTT

```bash
pip install pylink-square

# --device optional when default matches your J-Link target. Add --rtt-speed 1000 if connect fails with auto.
python log_compact_decoder.py \
  --source rtt \
  --app-out app.out \
  --nwp-out nwp_logging_fw.out \
  --max-args 10
```

Use `--rtt-channel2` when NWP traffic is on a second RTT channel. RTT is not supported on encrypted boards; use UART there.

## Inspect format strings

```bash
python log_compact_decoder.py --app-out app.out --nwp-out nwp_logging_fw.out --list-formats
```

## Wire format (default `--layout auto`)

The stream may interleave:

- **Core 0 (host):** `sl_log_event_t`, no firmware timestamp. TS / DELTA columns use the PC receive time.
- **Core 1 (NWP):** `sl_log_entry_t`, 64-bit firmware timestamp in microseconds.

Host record size depends on `--max-args` (`4 + 4*N + 4` bytes); NWP compact entries use three argument slots.

## Troubleshooting

- **Decoder runs but prints no log lines** — Until the firmware emits compact records the console can stay quiet (header only); that is normal. Press **Ctrl+C** to stop: the script prints bytes received, discard counts, and hints (silent transport vs bytes with no valid records). Check log level, backend (compact binary, not formatted), `sl_log_flush()` in Host mode, `--max-args` vs UC, and `--source rtt` vs UART.
- **`JLink CDC UART port not found`** — Pass `--port` explicitly. The decoder lists the available ports.
- **Garbled or `(null)` messages** — Wrong ELF, wrong `--max-args`, or the Formatted Output backend is on the target instead of Compact Binary Output.
- **NWP lines show `no descriptor/nwp-out match`** — Add `--nwp-out`, or `--descriptor` for legacy NWP images.
- **No core 1 / NWP lines at all** — Confirm secondary-core logging is enabled in firmware, match `--nwp-out` to your build, and raise `--baud` for busy UART. `--dump-discards 5` shows whether bytes arrive in an unexpected shape. SiWx91x checklist: [SiWx91x NWP notes](#siwx91x-nwp-notes).
- **Missing Python modules** — Install `pyelftools` and `pyserial` (and `pylink-square` for RTT).

## SiWx91x NWP notes

These items apply to **SiWx91x SoC** builds where Wi-Fi/BLE captive-core logs share the compact stream. They do **not** apply to single-core or non-NWP products (for example platforms without `sl_si91x_nwp_log_set_component_mask()`).

**Enable NWP logging in firmware:** set `SL_SI91X_ENABLE_NWP_LOGGING` in `config_feature_bit_map` (with `SL_SI91X_TCP_IP_FEAT_EXTENSION_VALID` and `SL_SI91X_CONFIG_FEAT_EXTENSION_VALID`). Use `nwp_logging_fw.out` (or your SDK’s NWP logging image) for `--nwp-out`.

**Device-side component mask:** `sl_si91x_nwp_log_set_component_mask()` in `sl_utility.h` selects which NWP components emit DEBUG logs. Requires compile-time **DEBUG** and **Host Mode** (`SL_LOG_CONFIG_MODE_HOST`); otherwise it returns `SL_STATUS_NOT_SUPPORTED`. Call after `sl_wifi_init()` / `sl_net_init()`, and again after every re-init:

```c
#include "sl_utility.h"

status = sl_si91x_nwp_log_set_component_mask(SL_SI91X_NWP_LOG_COMPONENT_WLAN_LMAC
                                             | SL_SI91X_NWP_LOG_COMPONENT_WLAN_UMAC);
```

Set bits select components at DEBUG; cleared bits are NONE. Mask `0` disables NWP logging; `SL_SI91X_NWP_LOG_COMPONENT_ALL` enables all.

**RTT defaults:** the script’s default `--device` targets common Si91x radio boards (for example `SiWG917M111M`). Override with your OPN (`SiWG917M110L`, `SiWG917M100M`, …) when J-Link does not recognize the default.

**Log Backend component:** compact UART examples often use `si91x_log_iostream_compact`.

## Related documentation

- SiWx91x software reference manual: *Decoding Log Backend I/O Stream – Compact Binary Output messages*.