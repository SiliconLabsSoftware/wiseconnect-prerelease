/***************************************************************************/ /**
 * @file
 * @brief Enterprise EAP-TLS client using on-device EC identity generation
 *******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "sl_net.h"
#include "sl_net_wifi_types.h"
#include "sl_si91x_ec_identity.h"
#include "sl_si91x_csr_sign.h"
#include "cmsis_os2.h"
#include "sl_ip_types.h"
#include "wifiuser.pem.h"
#include "sl_wifi.h"
#include <string.h>
#include "sl_utility.h"

/******************************************************
 *               Function Declarations
 ******************************************************/
static void application_start(void *argument);
static void log_spki_hex(const uint8_t *spki, uint16_t spki_len);

/******************************************************
 *                    Constants
 ******************************************************/
#define WIFI_ACCESS_POINT_SSID "enterprise"
#define EC_IDENTITY_KEY_ID     0
/* Firmware offers ECDHE-ECDSA only when these okc bits are set. */
#define EC_EAP_CIPHER_FLAGS \
  (SL_WIFI_EAP_ECDHE_ECDSA_AES128_SHA | SL_WIFI_EAP_ECDHE_ECDSA_AES256_SHA | SL_WIFI_EAP_ECDHE_ECDSA_AES128_SHA256)

/*
 * Set to 1 after a CA has signed the CSR built from the generated SPKI +
 * sl_si91x_csr_sign() signature, and after the signed client certificate
 * PEM has been converted to an array (see readme.md).
 */
#ifndef INSTALL_SIGNED_CLIENT_CERT
#define INSTALL_SIGNED_CLIENT_CERT 0
#endif

#if INSTALL_SIGNED_CLIENT_CERT
#include "wifi_ec_client.pem.h"
#endif

/******************************************************
 *               Variable Definitions
 ******************************************************/

const osThreadAttr_t thread_attributes = {
  .name       = "app",
  .attr_bits  = 0,
  .cb_mem     = 0,
  .cb_size    = 0,
  .stack_mem  = 0,
  .stack_size = 3072,
  .priority   = osPriorityLow,
  .tz_module  = 0,
};

#if INSTALL_SIGNED_CLIENT_CERT
sl_ip_address_t ip_address           = { 0 };
sl_net_wifi_client_profile_t profile = { 0 };
#endif

static const sl_net_wifi_client_profile_t wifi_client_enterprise_eap_tls_profile = {
    .config = {
        .ssid.value = WIFI_ACCESS_POINT_SSID,
        .ssid.length = sizeof(WIFI_ACCESS_POINT_SSID)-1,
        .channel.channel = SL_WIFI_AUTO_CHANNEL,
        .channel.band = SL_WIFI_AUTO_BAND,
        .channel.bandwidth = SL_WIFI_AUTO_BANDWIDTH,
        .bssid = {{0}},
        .bss_type = SL_WIFI_BSS_TYPE_INFRASTRUCTURE,
        .security = SL_WIFI_WPA2_ENTERPRISE,
        .encryption = SL_WIFI_EAP_TLS_ENCRYPTION,
        .client_options = 0,
        .credential_id = SL_NET_WIFI_EAP_CLIENT_CREDENTIAL_ID,
    },
    .ip = {
        .mode = SL_IP_MANAGEMENT_DHCP,
        .type = SL_IPV4,
        .host_name = NULL,
        .dhcp_config = {
            .min_discover_retry_interval = 5,
            .max_discover_retry_interval = 10,
            .min_request_retry_interval = 5,
            .max_request_retry_interval = 10,
            .max_discover_retries = 5,
            .max_request_retries = 10,
        }
    }
};

static const sl_net_wifi_eap_credential_entry_t wifi_client_enterprise_eap_credential = {
  .type                 = SL_NET_EAP_CLIENT_CREDENTIAL,
  .data_length          = sizeof(sl_wifi_eap_credential_t),
  .data.username        = "user1",
  .data.password        = "12345678",
  .data.certificate_key = {},
  .data.certificate_id  = EC_IDENTITY_KEY_ID,
  .data.eap_flags       = EC_EAP_CIPHER_FLAGS
};

/* Sample CertificationRequestInfo DER used to exercise sl_si91x_csr_sign().
 * Replace with a host-built CSR TBS that embeds the SPKI from generate. */
static const uint8_t sample_csr_tbs[] = { 0x30, 0x03, 0x02, 0x01, 0x00 };

/******************************************************
 *               Function Definitions
 ******************************************************/

void app_init(void)
{
  osThreadNew((osThreadFunc_t)application_start, NULL, &thread_attributes);
}

/* Print SPKI in 16-byte hex lines for the readme CSR build flow. */
static void log_spki_hex(const uint8_t *spki, uint16_t spki_len)
{
  static const char hex_digits[] = "0123456789ABCDEF";
  char line[(3 * 16) + 1];

  if ((spki == NULL) || (spki_len == 0)) {
    return;
  }

  for (uint16_t offset = 0; offset < spki_len; offset = (uint16_t)(offset + 16)) {
    uint16_t chunk = (uint16_t)(((spki_len - offset) > 16) ? 16 : (spki_len - offset));
    uint16_t pos   = 0;
    for (uint16_t i = 0; i < chunk; i++) {
      uint8_t byte = spki[offset + i];
      line[pos++]  = hex_digits[byte >> 4];
      line[pos++]  = hex_digits[byte & 0x0F];
      if ((i + 1) < chunk) {
        line[pos++] = ' ';
      }
    }
    line[pos] = '\0';
    SL_DEBUG_LOG_V2(INFO, "SPKI: %s\r\n", line);
  }
}

static void application_start(void *argument)
{
  UNUSED_PARAMETER(argument);
  sl_status_t status                            = SL_STATUS_OK;
  sl_si91x_ec_generate_params_t generate_params = { 0 };
  sl_si91x_csr_signing_params_t signing_params  = { 0 };
  uint8_t signature_der[SL_SI91X_EC_MAX_SIGNATURE_DER_LEN];
  uint16_t signature_der_len              = sizeof(signature_der);
  const sl_net_credential_id_t ec_cred_id = SL_NET_WIFI_EAP_CLIENT_CREDENTIAL_ID_KEY(EC_IDENTITY_KEY_ID);

  status = sl_net_init(SL_NET_WIFI_CLIENT_INTERFACE, &sl_wifi_default_enterprise_client_configuration, NULL, NULL);
  if (status != SL_STATUS_OK) {
    SL_DEBUG_LOG_V2(ERROR, "Failed to start Wi-Fi client interface: 0x%lx\r\n", status);
    return;
  }
  SL_DEBUG_LOG_V2(INFO, "Wi-Fi client interface up Success\r\n");

  status = sl_net_set_profile(SL_NET_WIFI_CLIENT_INTERFACE,
                              SL_NET_DEFAULT_WIFI_CLIENT_PROFILE_ID,
                              &wifi_client_enterprise_eap_tls_profile);
  if (status != SL_STATUS_OK) {
    SL_DEBUG_LOG_V2(ERROR, "Failed to set client profile: 0x%lx\r\n", status);
    return;
  }

  status = sl_net_set_credential(SL_NET_WIFI_EAP_CLIENT_CREDENTIAL_ID,
                                 SL_NET_EAP_CLIENT_CREDENTIAL,
                                 &(wifi_client_enterprise_eap_credential.data),
                                 sizeof(sl_wifi_eap_credential_t));
  if (status != SL_STATUS_OK) {
    SL_DEBUG_LOG_V2(ERROR, "Failed to set EAP credentials: 0x%lx\r\n", status);
    return;
  }

  /*
   * Optional regenerate path (commented): if a previous key exists for this
   * key_id, generate returns SL_STATUS_EC_KEY_ALREADY_EXISTS /
   * SL_STATUS_EC_IDENTITY_IN_USE. Delete first, then regenerate:
   *
   *   sl_net_delete_credential(ec_cred_id, SL_NET_DELETE_EC_PRIVATE_KEY);
   *   status = sl_net_generate_and_set_credential(...);
   */
  generate_params.curve = SL_SI91X_EC_CURVE_SECP256R1;
  status                = sl_net_generate_and_set_credential(ec_cred_id,
                                              SL_NET_GENERATE_EC_PRIVATE_KEY,
                                              &generate_params,
                                              sizeof(generate_params));
  if (status != SL_STATUS_OK) {
    SL_DEBUG_LOG_V2(ERROR, "EC key generation failed: 0x%lx\r\n", status);
    return;
  }
  SL_DEBUG_LOG_V2(INFO,
                  "Generated P-256 identity key_id=%lu, SPKI length=%u\r\n",
                  (unsigned long)EC_IDENTITY_KEY_ID,
                  generate_params.spki_len);
  log_spki_hex(generate_params.spki, generate_params.spki_len);

  signing_params.cri_der     = sample_csr_tbs;
  signing_params.cri_der_len = sizeof(sample_csr_tbs);
  signing_params.sig_alg     = SL_SI91X_EC_SIG_ALG_ECDSA_SHA256;
  status = sl_si91x_csr_sign(EC_IDENTITY_KEY_ID, &signing_params, signature_der, &signature_der_len);
  if (status != SL_STATUS_OK) {
    SL_DEBUG_LOG_V2(ERROR, "CSR sign failed: 0x%lx\r\n", status);
    return;
  }
  SL_DEBUG_LOG_V2(INFO, "CSR signed, signature length=%u\r\n", signature_der_len);

  /* Enterprise CA uses legacy ID=5 with SL_NET_SIGNING_CERTIFICATE (EAP CA). */
  status = sl_net_set_credential(SL_NET_WIFI_EAP_CLIENT_CREDENTIAL_ID,
                                 SL_NET_SIGNING_CERTIFICATE,
                                 wifiuser,
                                 sizeof(wifiuser) - 1);
  if (status != SL_STATUS_OK) {
    SL_DEBUG_LOG_V2(ERROR, "Loading enterprise CA certificate failed: 0x%lX\r\n", status);
    return;
  }
  SL_DEBUG_LOG_V2(INFO, "Loaded enterprise CA certificate (legacy ID=5)\r\n");

#if INSTALL_SIGNED_CLIENT_CERT
  status = sl_net_set_credential(ec_cred_id, SL_NET_CERTIFICATE, wifi_ec_client, sizeof(wifi_ec_client) - 1);
  if (status != SL_STATUS_OK) {
    SL_DEBUG_LOG_V2(ERROR, "Loading EC client certificate failed: 0x%lX\r\n", status);
    return;
  }
  SL_DEBUG_LOG_V2(INFO, "Loaded EC client certificate for key_id=%lu\r\n", (unsigned long)EC_IDENTITY_KEY_ID);

  status = sl_net_up(SL_NET_WIFI_CLIENT_INTERFACE, SL_NET_DEFAULT_WIFI_CLIENT_PROFILE_ID);
  if (status != SL_STATUS_OK) {
    SL_DEBUG_LOG_V2(ERROR, "Failed to bring Wi-Fi client interface up: 0x%lx\r\n", status);
    return;
  }
  SL_DEBUG_LOG_V2(INFO, "Wi-Fi client connected\r\n");

  status = sl_net_get_profile(SL_NET_WIFI_CLIENT_INTERFACE, SL_NET_DEFAULT_WIFI_CLIENT_PROFILE_ID, &profile);
  if (status != SL_STATUS_OK) {
    SL_DEBUG_LOG_V2(ERROR, "Failed to get client profile: 0x%lx\r\n", status);
    return;
  }

  ip_address.type = SL_IPV4;
  memcpy(&ip_address.ip.v4.bytes, &profile.ip.ip.v4.ip_address.bytes, sizeof(sl_ipv4_address_t));
  SL_DEBUG_LOG_V2(INFO, "IP address of the client: ");
  print_sl_ip_address(&ip_address);
  SL_DEBUG_LOG_V2(INFO, "\r\n");
#else
  SL_DEBUG_LOG_V2(INFO,
                  "Key generation and CSR signing succeeded. Build a CSR with the returned SPKI, "
                  "get it signed by the enterprise CA, convert the client cert to an array, "
                  "set INSTALL_SIGNED_CLIENT_CERT to 1, and rebuild to join the AP.\r\n");
#endif
}
