# Wi-Fi - SoC Empty (lwIP)

## Table of Contents

- [Purpose/Scope](#purposescope)
- [Prerequisites](#prerequisites)
- [Getting Started](#getting-started)
- [Application Build Environment](#application-build-environment)
- [Test the Application](#test-the-application)
- [Troubleshooting](#troubleshooting)
- [Resources / Support](#resources--support)

## Purpose/Scope

Minimal FreeRTOS template for SiWx917 SoC Wi-Fi using the **lwIP** stack (`sl_si91x_lwip_stack` / `sl_net_for_lwip`): initialize Wi-Fi and connect as a station (`sl_net_init` → `sl_net_up`) with compile-time credential placeholders, then idle. Part-/OPN-based — no board pinouts or UART VCOM. Use this as a starting point for lwIP-based Wi-Fi apps (not as a TCP client demo).

## Prerequisites

### Hardware

- SiWx917 SoC hardware (OPN / part project)

### Software

- Simplicity Studio with WiSeConnect / WiseConnect extension

## Getting Started

1. Refer to [WiSeConnect Getting Started](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/) to install Studio and create a project.
2. Select this example (**Wi-Fi - SoC Empty (lwIP)**) from an OPN/part flow **without** using the pin tool for board SW components.
3. For folder structure, see [WiSeConnect Examples](https://docs.silabs.com/wiseconnect/latest/wiseconnect-examples/#example-folder-structure).

## Application Build Environment

Edit generated `config/sl_net_default_values.h` placeholders before building:

| Macro | Default placeholder |
|-------|---------------------|
| `DEFAULT_WIFI_CLIENT_PROFILE_SSID` | `YOUR_AP_SSID` |
| `DEFAULT_WIFI_CLIENT_CREDENTIAL` | `YOUR_AP_PASSPHRASE` |
| `DEFAULT_WIFI_CLIENT_SECURITY_TYPE` | `SL_WIFI_WPA2` |

Do not hard-code production secrets in application sources.

## Test the Application

1. Flash the application and run.
2. Confirm station connect via RTT/debugger logs or AP association.
3. On success, the worker task idles (`osDelay`). On failure, the app logs the `sl_status` hex and the worker task returns (no idle loop) — fix credentials and reset to retry.
4. Logging uses the board-agnostic RTT path (not UART VCOM).

## Troubleshooting

- **Wrong SSID/PSK:** update placeholders in `config/sl_net_default_values.h`, rebuild, reset.
- **No console output:** this example does not enable UART VCOM; use RTT / debugger. See [Console Input and Output](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/using-the-simplicity-studio-ide#console-input-and-output) and [Troubleshooting](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-troubleshooting/).
- **Build/generate issues:** confirm Simplicity Studio and the WiSeConnect extension versions match peer SoC Wi-Fi samples.

## Resources / Support

- [WiSeConnect Getting Started](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/)
- [WiSeConnect Examples](https://docs.silabs.com/wiseconnect/latest/wiseconnect-examples/)
- [Silicon Labs Community](https://community.silabs.com/)
