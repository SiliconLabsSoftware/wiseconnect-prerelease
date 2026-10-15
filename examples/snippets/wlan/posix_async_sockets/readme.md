# Wi-Fi - POSIX Async Sockets

## High-Level Overview

SiWx91x POSIX async sockets example: Connect to Wi-Fi and demonstrate non-blocking TCP client connect, TCP server `MSG_PEEK` verify plus timed receive throughput, and UDP receive via `select()` in SoC and NCP modes.

## Table of Contents

- [Wi-Fi - POSIX Async Sockets](#wi-fi---posix-async-sockets)
  - [Table of Contents](#table-of-contents)
  - [High-Level Overview](#high-level-overview)
  - [Purpose/Scope](#purposescope)
  - [Prerequisites/Setup Requirements](#prerequisitessetup-requirements)
    - [Hardware Requirements](#hardware-requirements)
    - [Software Requirements](#software-requirements)
    - [Setup Diagram](#setup-diagram)
  - [Getting Started](#getting-started)
  - [Application Build Environment](#application-build-environment)
  - [Test the Application](#test-the-application)
    - [TCP Client Mode](#tcp-client-mode)
    - [TCP Server Mode](#tcp-server-mode)
    - [UDP Server Mode](#udp-server-mode)
  - [Troubleshooting](#troubleshooting)
  - [Resources](#resources)
  - [Report Bugs and Get Support](#report-bugs-and-get-support)

## Purpose/Scope

This application demonstrates POSIX-style asynchronous socket usage on the SiWx91x. The device connects to a Wi-Fi access point as a station, obtains an IP address, then runs one of three modes selected by `APP_MODE` in `app.c`:

| Mode | Value | Flow |
|------|-------|------|
| `APP_MODE_TCP_CLIENT` | 0 | `SOCK_NONBLOCK` → `connect` → `select(writefds)` → `getsockopt(SO_ERROR)` → TCP send |
| `APP_MODE_TCP_SERVER` | 1 | `bind`/`listen` → `select`+`accept` → `select` for data → `sl_si91x_socket_get_available_bytes` → `recv(MSG_PEEK)` + `recv`/`memcmp` (one chunk) → `select`+`recv` for remaining `DATA_TEST_DURATION_MS` → throughput summary |
| `APP_MODE_UDP_SERVER` | 2 | `bind` → `select(readfds)` → `recvfrom` |

`SOCK_NONBLOCK` applies to **TCP client connect only**. TCP server listen and UDP do not require `SOCK_NONBLOCK`.

### TCP server receive path

After accepting a client and waiting for data with `select(readfds)`:

1. `sl_si91x_socket_get_available_bytes()` — query how many bytes the NWP has ready without consuming them. This is a Si91x API, not POSIX `ioctl`/`FIONREAD`. The example therefore requires the `sl_si91x_asynchronous_socket` component (already in the `.slcp` files).
2. `recv(..., MSG_PEEK)` then `recv(..., 0)` — peek one chunk, consume the same length, and `memcmp` to verify peek and consume match.
3. Continue receiving for `DATA_TEST_DURATION_MS` using `select()` with the remaining time each iteration (or until peer close / error), print a throughput summary, then close the client and wait for the next connection. Keep the peer streaming for that full interval (for example iPerf `-t 5`); if the peer stops early, `select()` times out and the summary still prints.

`MSG_PEEK` requires the project/compile define `SL_SI91X_SOCKETS_ENABLE_RECV_FLAGS` set to `1` (configured in the `.slcp`). Defining it only in application sources is not sufficient; SDK socket sources must be rebuilt with the flag.

## Prerequisites/Setup Requirements

### Hardware Requirements

- Windows PC
- Wireless Access Point
- SoC Mode:
  - Standalone
    - BRD4002B Wireless Pro Kit Mainboard [SI-MB4002B](https://www.silabs.com/development-tools/wireless/wireless-pro-kit-mainboard?tab=overview)
    - Radio Boards
	  - BRD4338A [SiWx917-RB4338A](https://www.silabs.com/development-tools/wireless/wi-fi/siwx917-rb4338a-wifi-6-bluetooth-le-soc-radio-board?tab=overview)
	  - BRD4342A [SiWx917-RB4342A](https://www.silabs.com/development-tools/wireless/wi-fi/siwx91x-rb4342a-wifi-6-bluetooth-le-soc-radio-board?tab=overview)
	  - BRD4339B [SiWx917-RB4339B](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/getting-started-with-at)
	  - BRD4340A [SiWx917-RB4340A](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/getting-started-with-at)
	  - BRD4343A [SiWx917-RB4343A](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-rb4343a-wi-fi-6-bluetooth-le-8mb-flash-radio-board-for-module?tab=overview)
	  - BRD4343C [SiWx917-RB4343C](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-rb4343c-wi-fi-6-bluetooth-le-8mb-flash-radio-board-for-module?tab=overview)
  - Kits
	- SiWG917 Dev Kit [BRD2605A](https://www.silabs.com/development-tools/wireless/wi-fi/siwx917-dk2605a-wifi-6-bluetooth-le-soc-dev-kit?tab=overview)
	- SiWG917 Dev Kit [BRD2605B](https://www.silabs.com/development-tools/wireless/wi-fi/siwx917-dk2605b-wifi-6-bluetooth-le-soc-dev-kit?tab=overview)
    - SiWx917 AC1 Module Explorer Kit [BRD2708A](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-ek2708a-explorer-kit?tab=overview)

- NCP Mode:
  - [BRD4346A](https://www.silabs.com/development-tools/wireless/wi-fi/siwx917-rb4346a-wifi-6-bluetooth-le-soc-4mb-flash-radio-board?tab=overview) + [BRD8045C](https://www.silabs.com/development-tools/wireless/wi-fi/shield-adapter-board-for-co-processor-radio-boards?tab=overview)
  - [BRD4357A](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-rb4357a-wi-fi-6-bluetooth-le-4mb-flash-radio-board-for-rcp-and-ncp-modules?tab=overview) + [BRD8045C](https://www.silabs.com/development-tools/wireless/wi-fi/shield-adapter-board-for-co-processor-radio-boards?tab=overview)
  - [BRD4357C](https://www.silabs.com/development-tools/wireless/wi-fi/siw917y-rb4357c-wi-fi-6-bluetooth-le-4mb-flash-radio-board-for-rcp-and-ncp-modules?tab=overview) + [BRD8045C](https://www.silabs.com/development-tools/wireless/wi-fi/shield-adapter-board-for-co-processor-radio-boards?tab=overview)
  - Silicon Labs [BRD4180B](https://www.silabs.com/development-tools/wireless/slwrb4180b-efr32xg21-wireless-gecko-radio-board?tab=overview)
  - Host MCU Eval Kit. This example has been tested with:
    - Silicon Labs [WSTK + EFR32MG21](https://www.silabs.com/development-tools/wireless/efr32xg21-bluetooth-starter-kit)
  - Interface and Host MCU Supported
    - SPI : EFR32

### Software Requirements

- Simplicity Studio
- Windows PC (Remote PC) with a TCP/UDP peer tool such as [iPerf](https://sourceforge.net/projects/iperf2/files/iperf-2.0.8-win.zip/download)
- Serial terminal (for example Docklight or Tera Term) to view device logs

### Setup Diagram

  ![Figure: Setup Diagram SoC and NCP Mode for POSIX Async Sockets Example](resources/readme/posix_async_sockets_soc_ncp.png)

**NOTE**:

- The Host MCU platform (EFR32MG21) and the SiWx91x interact with each other through the SPI interface.
- Import `wifi_posix_async_sockets_soc.slcp` (SoC) or `wifi_posix_async_sockets_ncp.slcp` (NCP) in Simplicity Studio.

## Getting Started

Refer to the instructions [here](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/) to:

- [Install Simplicity Studio](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#install-simplicity-studio)
- [Install WiSeConnect extension](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#install-the-wi-se-connect-extension)
- [Connect your device to the computer](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#connect-si-wx91x-to-computer)
- [Upgrade your connectivity firmware ](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#update-si-wx91x-connectivity-firmware)
- [Create a Studio project ](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-developing-for-silabs-hosts/#create-a-project)

For details on the project folder structure, see the [WiSeConnect Examples](https://docs.silabs.com/wiseconnect/latest/wiseconnect-examples/#example-folder-structure) page.

## Application Build Environment

The application can be configured to suit your requirements and development environment. Read through the following sections and make any changes needed.

- The application uses the default configurations as provided in the **default_wifi_client_profile** in ``sl_net_default_values.h``, and you can choose to configure these parameters as needed.
- In the Project Explorer pane, expand the **config** folder and open the **sl_net_default_values.h** file. Configure the following parameters to enable your Silicon Labs Wi-Fi device to connect to your Wi-Fi network.

  - STA instance related parameters

    - DEFAULT_WIFI_CLIENT_PROFILE_SSID refers to the name with which the Wi-Fi network shall be advertised. The SiWx91x module is connected to it.

      ```c
      #define DEFAULT_WIFI_CLIENT_PROFILE_SSID               "YOUR_AP_SSID"      
      ```

    - DEFAULT_WIFI_CLIENT_CREDENTIAL refers to the secret key if the access point is configured in WPA-PSK/WPA2-PSK security modes.

      ```c
      #define DEFAULT_WIFI_CLIENT_CREDENTIAL                 "YOUR_AP_PASSPHRASE" 
      ```

    - DEFAULT_WIFI_CLIENT_SECURITY_TYPE refers to the security type if the access point is configured in WPA/WPA2 or mixed security modes.

      ```c
      #define DEFAULT_WIFI_CLIENT_SECURITY_TYPE              SL_WIFI_WPA2 
      ```

  - Other STA instance configurations can be modified if required in `default_wifi_client_profile` configuration structure.

- Configure the following parameters in ``app.c`` as needed:

  - Application mode (default is TCP client)

    ```c
    #define APP_MODE APP_MODE_TCP_CLIENT   // 0 = TCP client, 1 = TCP server, 2 = UDP server
    ```

  - Port and peer settings

    ```c
    #define DEVICE_PORT              5001           // Listen/connect port
    #define SERVER_IP_ADDRESS        "192.168.1.9"  // Peer IP for TCP client mode
    ```

  - Timing and retry settings

    ```c
    #define SELECT_RETRY_SEC         5     // Per-select timeout (seconds)
    #define CONNECT_MAX_ATTEMPTS     5U    // TCP client full connect retries
    #define CONNECT_SELECT_MAX_WAITS 12U   // Max select waits per connect attempt (~60 s)
    #define DATA_TEST_DURATION_MS    5000U // TCP/UDP transfer duration
    ```

- `SL_SI91X_SOCKETS_ENABLE_RECV_FLAGS` is set to `1` in the SoC and NCP `.slcp` files so SDK socket sources are rebuilt with `MSG_PEEK` support (required for TCP server mode). Without it, non-zero `recv` flags fail with `EOPNOTSUPP`.

> **Note**: For recommended settings, please refer the [recommendations guide](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-prog-recommended-settings/).

## Test the Application

Refer to the instructions [here](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/) to:

- Build the application
- Flash, run, and debug the application

- Before running the application, configure Wi-Fi credentials and the `APP_MODE` / peer settings described above.
- Run the application. After the program gets executed, the SiWx91x module connects to the AP and prints its IP address.
- A `select()` return of 0 (timeout) while connect or accept is still pending is **expected**, not an error.

### TCP Client Mode

Set `APP_MODE` to `APP_MODE_TCP_CLIENT` and set `SERVER_IP_ADDRESS` to the peer that runs a TCP server on `DEVICE_PORT`.

Example peer (iPerf TCP server):

```text
iperf.exe -s -p 5001 -i 1
```

The device creates a non-blocking TCP socket, calls `connect()`, waits with `select(writefds)`, checks `getsockopt(SO_ERROR)`, then sends data for `DATA_TEST_DURATION_MS` and prints a throughput summary.

### TCP Server Mode

Set `APP_MODE` to `APP_MODE_TCP_SERVER`. From a remote PC, connect to the printed device IP on `DEVICE_PORT`.

Example peer (iPerf TCP client):

```text
iperf.exe -c <Device_IP> -p 5001 -i 1 -t 5
```

The device listens, accepts with `select()`, queries available bytes, verifies one chunk with `MSG_PEEK` vs a consuming `recv()`, then receives for `DATA_TEST_DURATION_MS` (each `recv` is preceded by `select()` with the remaining time so a quiet peer cannot hang the app) and prints a throughput summary. Stream from the peer for the same interval (`-t 5` below). It then closes the client and waits for the next connection.

### UDP Server Mode

Set `APP_MODE` to `APP_MODE_UDP_SERVER`. From a remote PC, send UDP to the device IP on `DEVICE_PORT`.

Example peer (iPerf UDP client):

```text
iperf.exe -u -c <Device_IP> -p 5001 -t 5 -b 1M
```

The device binds a UDP socket, waits with `select(readfds)`, receives with `recvfrom()` for `DATA_TEST_DURATION_MS`, and prints a throughput summary.

## Troubleshooting

If you encounter issues while running this example, check the following:

- Verify Wi-Fi credentials in `sl_net_default_values.h` and socket configuration in `app.c`.
- For TCP client mode, confirm `SERVER_IP_ADDRESS` and that a peer TCP server is listening on `DEVICE_PORT`.
- For TCP server / UDP server modes, confirm the peer targets the printed device IP and `DEVICE_PORT`.
- If `recv(MSG_PEEK)` fails with `EOPNOTSUPP`, ensure `SL_SI91X_SOCKETS_ENABLE_RECV_FLAGS` is set to `1` in the `.slcp` and rebuild the project so SDK socket sources pick up the define.
- `select()` timeouts while waiting for connect, accept, or datagrams are normal; the TCP client caps retries so a missing peer does not loop forever.

## Resources

- [WiSeConnect Getting Started Guide](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/)
- [WiSeConnect Examples](https://docs.silabs.com/wiseconnect/latest/wiseconnect-examples/#example-folder-structure)
- [WiSeConnect Recommended Settings Guide](https://docs.silabs.com/wiseconnect/latest/wiseconnect-developers-guide-prog-recommended-settings/)

## Report Bugs and Get Support

Report issues and get help from the Silicon Labs community:

- [Silicon Labs Community](https://www.silabs.com/community)
