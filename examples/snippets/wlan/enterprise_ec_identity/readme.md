# Wi-Fi - Enterprise EC Identity (EAP-TLS)

## High-Level Overview

SiWx91x example that generates an on-device P-256 identity, signs a CSR TBS in the NWP, loads the enterprise CA, and optionally installs the signed client certificate for WPA2-Enterprise EAP-TLS.

This application is based on the Enterprise Client example. It uses Wi-Fi `sl_net_generate_and_set_credential` and Platform crypto `sl_si91x_csr_sign` instead of provisioning a host-held private key.

## Purpose/Scope

Demonstrate the Phase-1 EC identity APIs:

1. `sl_net_generate_and_set_credential(SL_NET_WIFI_EAP_CLIENT_CREDENTIAL_ID_KEY(key_id), SL_NET_GENERATE_EC_PRIVATE_KEY, ...)` generates a P-256 key in the NWP and returns the SubjectPublicKeyInfo (SPKI). The private key never leaves the device.
2. Platform `sl_si91x_csr_sign()` signs a host-built CertificationRequestInfo (CSR TBS) with ECDSA-SHA256 via the crypto command path.
3. `sl_net_set_credential` with legacy `SL_NET_WIFI_EAP_CLIENT_CREDENTIAL_ID` (= 5) and `SL_NET_SIGNING_CERTIFICATE` loads the enterprise CA.
4. `sl_net_set_credential(SL_NET_WIFI_EAP_CLIENT_CREDENTIAL_ID_KEY(key_id), SL_NET_CERTIFICATE, ...)` installs the CA-signed client certificate for the same `key_id`.
5. Optional: `sl_net_delete_credential(..., SL_NET_DELETE_EC_PRIVATE_KEY)` then regenerate if the slot is already in use (`SL_STATUS_EC_KEY_ALREADY_EXISTS` / `SL_STATUS_EC_IDENTITY_IN_USE`).

`key_id` is a host-supplied identity identifier encoded in the credential ID via `SL_NET_WIFI_EAP_CLIENT_CREDENTIAL_ID_KEY`. It is not a SET_CERT `cert_index`. The client certificate is stored in the existing EAP client certificate slot.

`eap_flags` must include `SL_WIFI_EAP_ECDHE_ECDSA_AES128_SHA`, `SL_WIFI_EAP_ECDHE_ECDSA_AES256_SHA`, and `SL_WIFI_EAP_ECDHE_ECDSA_AES128_SHA256`. The NWP offers ECDHE-ECDSA, and secp256r1, only when at least one of those bits is set. Signing with an empty slot returns `SL_STATUS_EC_KEY_NOT_FOUND`.

## Prerequisites/Set up Requirements

Use the same hardware, FreeRADIUS, and WPA2-Enterprise AP setup as [Wi-Fi - Enterprise Client Mode](https://github.com/SiliconLabs/wiseconnect/blob/v4.1.2-content-for-docs/examples/snippets/wlan/enterprise_client/readme.md).

## Application Build Environment

Open `app.c` and configure:

- `WIFI_ACCESS_POINT_SSID` — enterprise SSID
- `EC_IDENTITY_KEY_ID` — host-supplied identity id (Phase-1: 0 only)
- EAP username / password in `wifi_client_enterprise_eap_credential`
- `INSTALL_SIGNED_CLIENT_CERT` — set to `1` after the CA has signed the CSR and the client certificate array is included

Default security is `SL_WIFI_WPA2_ENTERPRISE` with `SL_WIFI_EAP_TLS_ENCRYPTION`.

### Certificate flow

1. Build and run with `INSTALL_SIGNED_CLIENT_CERT` left at `0`.
2. The application generates a key and signs the sample CSR TBS on-device. Capture the SPKI length and the `SPKI:` hex lines from the log.
3. Build a real CertificationRequestInfo that embeds those SPKI bytes (on the MCU or an offline tool), call on-device `sl_si91x_csr_sign()`, and assemble the CSR.
4. Submit the CSR to the enterprise CA. Convert the signed client certificate PEM to a C array with `resources/certificates/certificate_to_array.py`.
5. Include the generated header as `wifi_ec_client.pem.h`, set `INSTALL_SIGNED_CLIENT_CERT` to `1`, and rebuild.
6. The CA uses `sl_net_set_credential` with legacy `SL_NET_WIFI_EAP_CLIENT_CREDENTIAL_ID` (= 5) and `SL_NET_SIGNING_CERTIFICATE`.

## Test the Application

1. Create the SoC, NCP, or PSRAM project from this folder in Simplicity Studio.
2. Flash and open a serial terminal.
3. Confirm the generate and sign logs succeed.
4. After installing a CA-signed client certificate, confirm the station joins and prints an IPv4 address.

## Additional Information

RADIUS and AP configuration steps match the Enterprise Client example.

## Report Bugs and Get Support

See the [WiSeConnect support page](https://docs.silabs.com/wiseconnect/latest/).
