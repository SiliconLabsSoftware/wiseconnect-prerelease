/***************************************************************************/ /**
 * @file app.c
 * @brief POSIX async sockets example with data transfer
 *
 * Modes (set APP_MODE):
 *   APP_MODE_TCP_CLIENT - SOCK_NONBLOCK connect + select(writefds) + SO_ERROR + TCP send
 *   APP_MODE_TCP_SERVER - listen + accept + sl_si91x_socket_get_available_bytes_count + recv(MSG_PEEK) verify one chunk + timed TCP recv throughput
 *   APP_MODE_UDP_SERVER - bind + select(readfds) + recvfrom
 *
 * Note: SOCK_NONBLOCK / dual RSP_SOCKET_CREATE (2x 0x42) applies to TCP client only.
 *       TCP server listen completes on a single 0x42; UDP needs no SOCK_NONBLOCK.
 *******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 ******************************************************************************/
#include "cmsis_os2.h"
#include "sl_net.h"
#include "sl_board_configuration.h"
#include "sl_wifi_types.h"
#include "errno.h"
#include "socket.h"
#include "sl_si91x_socket.h"
#include "select.h"
#include "sl_utility.h"
#include "sl_net_si91x.h"
#include "sl_wifi_callback_framework.h"
#include "string.h"
#include <stdio.h>
#include "sl_net_wifi_types.h"
#include "sl_si91x_core_utilities.h"
#include <stdbool.h>
#include <stdint.h>

/******************************************************
 *                    Constants
 ******************************************************/
#define DEVICE_PORT              5001
#define BACKLOG                  1
#define SELECT_RETRY_SEC         5
#define CONNECT_MAX_ATTEMPTS     5U
#define CONNECT_SELECT_MAX_WAITS 12U /* per attempt; 12 * 5s ≈ 60s handshake wait */
#define CONNECT_RETRY_DELAY_MS   1000U
#define DATA_TEST_DURATION_MS    5000U
#define TCP_BUFFER_SIZE          1460
#define UDP_BUFFER_SIZE          1470
#define THROUGHPUT_MSG_SIZE      80
#define SERVER_IP_ADDRESS        "192.168.1.9"

#define APP_MODE_TCP_CLIENT 0
#define APP_MODE_TCP_SERVER 1
#define APP_MODE_UDP_SERVER 2

#ifndef APP_MODE
#define APP_MODE APP_MODE_TCP_CLIENT
#endif

/******************************************************
 *               Variable Definitions
 ******************************************************/
static uint8_t data_buffer[UDP_BUFFER_SIZE];

const osThreadAttr_t thread_attributes = {
  .name       = "app",
  .attr_bits  = 0,
  .cb_mem     = 0,
  .cb_size    = 0,
  .stack_mem  = 0,
  .stack_size = 6144,
  .priority   = osPriorityLow,
  .tz_module  = 0,
};

/******************************************************
 *               Function Declarations
 ******************************************************/
static void application_start(void *argument);
static void fill_data_buffer(void);
static bool test_duration_elapsed(uint32_t start_tick);
static void print_transfer_summary(uint32_t total_bytes, uint32_t elapsed_ms);
static bool tcp_client_connect_nonblocking(int client_socket, const struct sockaddr_in *server_address);
static int tcp_server_accept_nonblocking(int server_socket);
static void tcp_client_transfer(void);
static void tcp_server_transfer(void);
static void udp_server_transfer(void);

/******************************************************
 *               Function Definitions
 ******************************************************/
void app_init(void)
{
  osThreadNew((osThreadFunc_t)application_start, NULL, &thread_attributes);
}

void app_process_action(void)
{
}

static void fill_data_buffer(void)
{
  for (size_t i = 0; i < sizeof(data_buffer); i++) {
    data_buffer[i] = (uint8_t)('A' + (i % 26));
  }
}

static bool test_duration_elapsed(uint32_t start_tick)
{
  return ((uint32_t)(osKernelGetTickCount() - start_tick)) >= DATA_TEST_DURATION_MS;
}

static void print_transfer_summary(uint32_t total_bytes, uint32_t elapsed_ms)
{
  if (elapsed_ms == 0U) {
    elapsed_ms = 1U;
  }
  float duration_sec = ((float)elapsed_ms) / 1000.0f;
  float mbps         = (((float)total_bytes) * 8.0f) / duration_sec / 1000000.0f;
  // SL_DEBUG_LOG_V2 packs args as uint32_t; format floats via snprintf, then log as %s.
  char throughput_msg[THROUGHPUT_MSG_SIZE];
  snprintf(throughput_msg,
           sizeof(throughput_msg),
           "Throughput achieved @ %0.02f Mbps in %0.03f sec successfully\r\n",
           mbps,
           duration_sec);
  SL_DEBUG_LOG_V2(INFO, "%s", (uintptr_t)throughput_msg);
}

static void application_start(void *argument)
{
  UNUSED_PARAMETER(argument);

  sl_status_t status = sl_net_init(SL_NET_WIFI_CLIENT_INTERFACE, NULL, NULL, NULL);
  if (status != SL_STATUS_OK) {
    SL_DEBUG_LOG_V2(ERROR, "Failed to start Wi-Fi client interface: 0x%lx\r\n", status);
    return;
  }

  status = sl_net_up(SL_NET_WIFI_CLIENT_INTERFACE, SL_NET_DEFAULT_WIFI_CLIENT_PROFILE_ID);
  if (status != SL_STATUS_OK) {
    SL_DEBUG_LOG_V2(ERROR, "Failed to bring Wi-Fi client interface up: 0x%lx\r\n", status);
    return;
  }

  SL_DEBUG_LOG_V2(INFO, "Wi-Fi connected\r\n");

  sl_net_wifi_client_profile_t profile = { 0 };
  sl_ip_address_t ip_address           = { 0 };

  status = sl_net_get_profile(SL_NET_WIFI_CLIENT_INTERFACE, SL_NET_DEFAULT_WIFI_CLIENT_PROFILE_ID, &profile);
  if (status != SL_STATUS_OK) {
    SL_DEBUG_LOG_V2(ERROR, "Failed to get client profile: 0x%lx\r\n", status);
    return;
  }

  ip_address.type = SL_IPV4;
  memcpy(&ip_address.ip.v4.bytes, &profile.ip.ip.v4.ip_address.bytes, sizeof(sl_ipv4_address_t));
  SL_DEBUG_LOG_V2(INFO, "Device IP address: ");
  print_sl_ip_address(&ip_address);
  printf("\r\n");

  fill_data_buffer();

  switch (APP_MODE) {
    case APP_MODE_TCP_CLIENT:
      tcp_client_transfer();
      break;
    case APP_MODE_TCP_SERVER:
      tcp_server_transfer();
      break;
    case APP_MODE_UDP_SERVER:
      udp_server_transfer();
      break;
    default:
      SL_DEBUG_LOG_V2(ERROR, "Invalid APP_MODE\r\n");
      break;
  }
}

static bool tcp_client_connect_nonblocking(int client_socket, const struct sockaddr_in *server_address)
{
  if (connect(client_socket, (struct sockaddr *)server_address, sizeof(*server_address)) == 0) {
    SL_DEBUG_LOG_V2(INFO, "TCP connect completed immediately\r\n");
    return true;
  }

  if (errno != EINPROGRESS) {
    SL_DEBUG_LOG_V2(ERROR, "connect() failed, errno=%d\r\n", errno);
    return false;
  }

  SL_DEBUG_LOG_V2(INFO, "Non-blocking connect in progress (EINPROGRESS)\r\n");

  fd_set write_fds;
  struct timeval timeout;

  for (uint32_t wait = 0; wait < CONNECT_SELECT_MAX_WAITS; wait++) {
    FD_ZERO(&write_fds);
    FD_SET(client_socket, &write_fds);
    timeout.tv_sec  = SELECT_RETRY_SEC;
    timeout.tv_usec = 0;

    int ready = select(client_socket + 1, NULL, &write_fds, NULL, &timeout);
    if (ready < 0) {
      SL_DEBUG_LOG_V2(ERROR, "select() failed, errno=%d\r\n", errno);
      return false;
    }
    if (ready == 0) {
      // Expected while the NWP handshake is still in progress — not a hard failure.
      SL_DEBUG_LOG_V2(INFO,
                      "select() timed out waiting for connect (normal); wait %lu/%u\r\n",
                      (unsigned long)(wait + 1U),
                      (unsigned)CONNECT_SELECT_MAX_WAITS);
      continue;
    }

    if (FD_ISSET(client_socket, &write_fds)) {
      int socket_error = 0;
      socklen_t optlen = sizeof(socket_error);
      if (getsockopt(client_socket, SOL_SOCKET, SO_ERROR, &socket_error, &optlen) < 0) {
        SL_DEBUG_LOG_V2(ERROR, "getsockopt(SO_ERROR) failed, errno=%d\r\n", errno);
        return false;
      }
      if (socket_error == 0) {
        SL_DEBUG_LOG_V2(INFO, "TCP connect succeeded (SO_ERROR=0)\r\n");
        return true;
      }
      SL_DEBUG_LOG_V2(ERROR, "TCP connect failed, SO_ERROR=%d\r\n", socket_error);
      return false;
    }
  }

  SL_DEBUG_LOG_V2(ERROR,
                  "Connect select wait budget exhausted (%u x %u s)\r\n",
                  (unsigned)CONNECT_SELECT_MAX_WAITS,
                  (unsigned)SELECT_RETRY_SEC);
  return false;
}

static void tcp_client_transfer(void)
{
  struct sockaddr_in server_address = { 0 };
  server_address.sin_family         = AF_INET;
  server_address.sin_port           = DEVICE_PORT;
  sl_net_inet_addr(SERVER_IP_ADDRESS, &server_address.sin_addr.s_addr);

  for (uint32_t attempt = 1; attempt <= CONNECT_MAX_ATTEMPTS; attempt++) {
    int client_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (client_socket < 0) {
      SL_DEBUG_LOG_V2(ERROR, "socket() failed, errno=%d\r\n", errno);
      return;
    }

    SL_DEBUG_LOG_V2(INFO, "Connect attempt %lu/%u\r\n", (unsigned long)attempt, (unsigned)CONNECT_MAX_ATTEMPTS);

    if (!tcp_client_connect_nonblocking(client_socket, &server_address)) {
      close(client_socket);
      if (attempt == CONNECT_MAX_ATTEMPTS) {
        SL_DEBUG_LOG_V2(ERROR, "Giving up after %u connect attempts\r\n", (unsigned)CONNECT_MAX_ATTEMPTS);
        return;
      }
      SL_DEBUG_LOG_V2(INFO, "Connect failed, retrying in %u ms\r\n", CONNECT_RETRY_DELAY_MS);
      osDelay(CONNECT_RETRY_DELAY_MS);
      continue;
    }

    uint32_t start_tick       = osKernelGetTickCount();
    uint32_t total_bytes_sent = 0;

    SL_DEBUG_LOG_V2(INFO, "TCP TX started for %u ms\r\n", DATA_TEST_DURATION_MS);
    while (!test_duration_elapsed(start_tick)) {
      int sent_bytes = send(client_socket, data_buffer, TCP_BUFFER_SIZE, 0);
      if (sent_bytes < 0) {
        if (errno == ENOBUFS || errno == EAGAIN || errno == EWOULDBLOCK) {
          continue;
        }
        SL_DEBUG_LOG_V2(ERROR, "send() failed, errno=%d\r\n", errno);
        break;
      }
      total_bytes_sent += (uint32_t)sent_bytes;
    }

    uint32_t elapsed_ms = (uint32_t)(osKernelGetTickCount() - start_tick);
    SL_DEBUG_LOG_V2(INFO, "TCP TX complete\r\n");
    print_transfer_summary(total_bytes_sent, elapsed_ms);
    close(client_socket);
    return;
  }
}

static int tcp_server_accept_nonblocking(int server_socket)
{
  while (1) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(server_socket, &read_fds);

    struct timeval timeout = { .tv_sec = SELECT_RETRY_SEC, .tv_usec = 0 };
    int ready              = select(server_socket + 1, &read_fds, NULL, NULL, &timeout);
    if (ready < 0) {
      SL_DEBUG_LOG_V2(ERROR, "select() failed, errno=%d\r\n", errno);
      return -1;
    }
    if (ready == 0) {
      // Expected while waiting for a peer — not a hard failure.
      SL_DEBUG_LOG_V2(INFO, "select() timed out waiting for client (normal)\r\n");
      continue;
    }

    if (FD_ISSET(server_socket, &read_fds)) {
      struct sockaddr_in client_address = { 0 };
      socklen_t client_len              = sizeof(client_address);
      int client_socket                 = accept(server_socket, (struct sockaddr *)&client_address, &client_len);
      if (client_socket < 0) {
        SL_DEBUG_LOG_V2(ERROR, "accept() failed, errno=%d\r\n", errno);
        return -1;
      }
      SL_DEBUG_LOG_V2(INFO, "Accepted client on fd %d\r\n", client_socket);
      return client_socket;
    }
  }
}

static void tcp_server_transfer(void)
{
  int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_socket < 0) {
    SL_DEBUG_LOG_V2(ERROR, "socket() failed, errno=%d\r\n", errno);
    return;
  }

  struct sockaddr_in server_address = { 0 };
  server_address.sin_family         = AF_INET;
  server_address.sin_port           = DEVICE_PORT;

  if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0
      || listen(server_socket, BACKLOG) < 0) {
    SL_DEBUG_LOG_V2(ERROR, "bind/listen failed, errno=%d\r\n", errno);
    close(server_socket);
    return;
  }

  SL_DEBUG_LOG_V2(INFO, "Listening on port %u, waiting for clients (select timeout is normal)\r\n", DEVICE_PORT);

  while (1) {
    int client_socket = tcp_server_accept_nonblocking(server_socket);
    if (client_socket < 0) {
      SL_DEBUG_LOG_V2(INFO, "Accept failed, retrying in %u ms\r\n", CONNECT_RETRY_DELAY_MS);
      osDelay(CONNECT_RETRY_DELAY_MS);
      continue;
    }

    static uint8_t peek_buffer[TCP_BUFFER_SIZE];
    static uint8_t recv_buffer[TCP_BUFFER_SIZE];
    fd_set read_fds;
    struct timeval timeout = { .tv_sec = SELECT_RETRY_SEC, .tv_usec = 0 };

    FD_ZERO(&read_fds);
    FD_SET(client_socket, &read_fds);

    SL_DEBUG_LOG_V2(INFO, "Waiting for data on client fd %d\r\n", client_socket);
    int ready = select(client_socket + 1, &read_fds, NULL, NULL, &timeout);
    if (ready < 0) {
      SL_DEBUG_LOG_V2(ERROR, "select() failed during RX, errno=%d\r\n", errno);
      close(client_socket);
      SL_DEBUG_LOG_V2(INFO, "Waiting for next client\r\n");
      continue;
    }
    if (ready == 0 || !FD_ISSET(client_socket, &read_fds)) {
      SL_DEBUG_LOG_V2(ERROR, "Timed out waiting for data\r\n");
      close(client_socket);
      SL_DEBUG_LOG_V2(INFO, "Waiting for next client\r\n");
      continue;
    }

    uint32_t available_bytes = 0;
    // Si91x-only (not POSIX FIONREAD/ioctl); needs sl_si91x_asynchronous_socket in the .slcp.
    if (sl_si91x_socket_get_available_bytes_count(client_socket, &available_bytes) < 0) {
      SL_DEBUG_LOG_V2(ERROR, "sl_si91x_socket_get_available_bytes_count() failed, errno=%d\r\n", errno);
      close(client_socket);
      SL_DEBUG_LOG_V2(INFO, "Waiting for next client\r\n");
      continue;
    }
    if (available_bytes == 0) {
      SL_DEBUG_LOG_V2(ERROR, "No bytes available after select\r\n");
      close(client_socket);
      SL_DEBUG_LOG_V2(INFO, "Waiting for next client\r\n");
      continue;
    }

    SL_DEBUG_LOG_V2(INFO, "Bytes available: %lu\r\n", (unsigned long)available_bytes);

    size_t peek_len = (available_bytes > TCP_BUFFER_SIZE) ? TCP_BUFFER_SIZE : (size_t)available_bytes;
    int peek_bytes  = recv(client_socket, peek_buffer, peek_len, MSG_PEEK);
    if (peek_bytes <= 0) {
      SL_DEBUG_LOG_V2(ERROR, "recv(MSG_PEEK) failed, peek_bytes=%d, errno=%d\r\n", peek_bytes, errno);
      close(client_socket);
      SL_DEBUG_LOG_V2(INFO, "Waiting for next client\r\n");
      continue;
    }

    int read_bytes = recv(client_socket, recv_buffer, (size_t)peek_bytes, 0);
    if (read_bytes <= 0) {
      SL_DEBUG_LOG_V2(ERROR, "recv() failed, read_bytes=%d, errno=%d\r\n", read_bytes, errno);
      close(client_socket);
      SL_DEBUG_LOG_V2(INFO, "Waiting for next client\r\n");
      continue;
    }

    if ((peek_bytes != read_bytes) || (memcmp(peek_buffer, recv_buffer, (size_t)peek_bytes) != 0)) {
      SL_DEBUG_LOG_V2(ERROR, "Peek/consume mismatch: peek=%d, recv=%d\r\n", peek_bytes, read_bytes);
      close(client_socket);
      SL_DEBUG_LOG_V2(INFO, "Waiting for next client\r\n");
      continue;
    }

    SL_DEBUG_LOG_V2(INFO, "Peek and consume matched (%d bytes)\r\n", peek_bytes);

    uint32_t start_tick           = osKernelGetTickCount();
    uint32_t total_bytes_received = (uint32_t)read_bytes;

    SL_DEBUG_LOG_V2(INFO, "TCP RX started for %u ms\r\n", DATA_TEST_DURATION_MS);
    while (!test_duration_elapsed(start_tick)) {
      uint32_t elapsed_ms_now = (uint32_t)(osKernelGetTickCount() - start_tick);
      uint32_t remaining_ms   = DATA_TEST_DURATION_MS - elapsed_ms_now;
      fd_set read_fds;
      FD_ZERO(&read_fds);
      FD_SET(client_socket, &read_fds);
      struct timeval timeout = { .tv_sec  = (long)(remaining_ms / 1000U),
                                 .tv_usec = (long)((remaining_ms % 1000U) * 1000U) };

      int ready = select(client_socket + 1, &read_fds, NULL, NULL, &timeout);
      if (ready < 0) {
        SL_DEBUG_LOG_V2(ERROR, "select() failed during RX, errno=%d\r\n", errno);
        break;
      }
      if ((ready == 0) || !FD_ISSET(client_socket, &read_fds)) {
        break;
      }

      read_bytes = recv(client_socket, recv_buffer, TCP_BUFFER_SIZE, 0);
      if (read_bytes < 0) {
        SL_DEBUG_LOG_V2(ERROR, "recv() failed, errno=%d\r\n", errno);
        break;
      }
      if (read_bytes == 0) {
        SL_DEBUG_LOG_V2(INFO, "Peer closed connection\r\n");
        break;
      }
      total_bytes_received += (uint32_t)read_bytes;
    }

    uint32_t elapsed_ms = (uint32_t)(osKernelGetTickCount() - start_tick);
    SL_DEBUG_LOG_V2(INFO, "TCP RX complete\r\n");
    print_transfer_summary(total_bytes_received, elapsed_ms);

    close(client_socket);
    SL_DEBUG_LOG_V2(INFO, "Client closed, waiting for next client\r\n");
  }
}

static void udp_server_transfer(void)
{
  int udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (udp_socket < 0) {
    SL_DEBUG_LOG_V2(ERROR, "socket() failed, errno=%d\r\n", errno);
    return;
  }

  struct sockaddr_in local_address = { 0 };
  local_address.sin_family         = AF_INET;
  local_address.sin_port           = DEVICE_PORT;

  if (bind(udp_socket, (struct sockaddr *)&local_address, sizeof(local_address)) < 0) {
    SL_DEBUG_LOG_V2(ERROR, "bind() failed, errno=%d\r\n", errno);
    close(udp_socket);
    return;
  }

  SL_DEBUG_LOG_V2(INFO,
                  "UDP server on port %u for %u ms (select timeout is normal)\r\n",
                  DEVICE_PORT,
                  DATA_TEST_DURATION_MS);

  uint32_t start_tick           = osKernelGetTickCount();
  uint32_t total_bytes_received = 0;
  fd_set read_fds;
  struct timeval timeout;

  while (!test_duration_elapsed(start_tick)) {
    FD_ZERO(&read_fds);
    FD_SET(udp_socket, &read_fds);
    timeout.tv_sec  = SELECT_RETRY_SEC;
    timeout.tv_usec = 0;

    int ready = select(udp_socket + 1, &read_fds, NULL, NULL, &timeout);
    if (ready < 0) {
      SL_DEBUG_LOG_V2(ERROR, "select() failed, errno=%d\r\n", errno);
      break;
    }
    if (ready == 0) {
      // Expected while waiting for datagrams during the test window.
      continue;
    }

    if (FD_ISSET(udp_socket, &read_fds)) {
      struct sockaddr_in peer_address = { 0 };
      socklen_t peer_len              = sizeof(peer_address);
      int read_bytes =
        recvfrom(udp_socket, data_buffer, UDP_BUFFER_SIZE, 0, (struct sockaddr *)&peer_address, &peer_len);
      if (read_bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          continue;
        }
        SL_DEBUG_LOG_V2(ERROR, "recvfrom() failed, errno=%d\r\n", errno);
        break;
      }
      total_bytes_received += (uint32_t)read_bytes;
    }
  }

  uint32_t elapsed_ms = (uint32_t)(osKernelGetTickCount() - start_tick);
  SL_DEBUG_LOG_V2(INFO, "UDP RX complete\r\n");
  print_transfer_summary(total_bytes_received, elapsed_ms);
  close(udp_socket);
}
