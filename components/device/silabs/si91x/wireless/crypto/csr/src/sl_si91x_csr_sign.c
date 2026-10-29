/***************************************************************************/ /**
 * @file sl_si91x_csr_sign.c
 * @brief Platform CSR TBS signing for on-device EC identities.
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

#include "sl_si91x_csr_sign.h"
#include "sl_si91x_crypto.h"
#include "sl_status.h"
#include "sl_constants.h"
#include "sl_additional_status.h"
#include "sl_si91x_protocol_types.h"
#include "sl_si91x_driver.h"
#include "sli_wifi_utility.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#if defined(SLI_MULTITHREAD_DEVICE_SI91X)
#include "sl_si91x_crypto_thread.h"
#endif

#ifndef SL_SI91X_SIDE_BAND_CRYPTO
static sl_status_t sli_si91x_csr_sign_pending(sl_si91x_ec_key_id_t key_id,
                                              const sl_si91x_csr_signing_params_t *signing_params,
                                              uint8_t *signature,
                                              uint16_t *signature_len)
{
  sl_status_t status                    = SL_STATUS_FAIL;
  sl_wifi_buffer_t *buffer              = NULL;
  const sl_wifi_system_packet_t *packet = NULL;
  sli_si91x_csr_sign_request_t *request = (sli_si91x_csr_sign_request_t *)malloc(sizeof(*request));
  uint32_t header_length                = sizeof(*request);

  SL_VERIFY_POINTER_OR_RETURN(request, SL_STATUS_ALLOCATION_FAILED);
  memset(request, 0, header_length);

  request->algorithm_type = CSR_SIGN;
  request->sig_alg        = (uint8_t)signing_params->sig_alg;
  request->key_id         = key_id;
  request->cri_der_len    = signing_params->cri_der_len;

  status = sli_si91x_crypto_send_command(request,
                                         header_length,
                                         signing_params->cri_der,
                                         signing_params->cri_der_len,
                                         (void **)&buffer);
  if (status != SL_STATUS_OK) {
    free(request);
    if (buffer != NULL) {
      sli_buffer_manager_free_buffer(buffer);
    }
    return status;
  }

  packet = (const sl_wifi_system_packet_t *)sli_wifi_host_get_buffer_data(buffer, 0, NULL);
  if ((packet == NULL) || (packet->length == 0) || (packet->length > *signature_len)) {
    free(request);
    sli_buffer_manager_free_buffer(buffer);
    return SL_STATUS_FAIL;
  }

  memcpy(signature, packet->data, packet->length);
  *signature_len = packet->length;

  sli_buffer_manager_free_buffer(buffer);
  free(request);
  return SL_STATUS_OK;
}

#else
static sl_status_t sli_si91x_csr_sign_side_band(sl_si91x_ec_key_id_t key_id,
                                                sl_si91x_csr_signing_params_t *signing_params,
                                                uint8_t *signature,
                                                uint16_t *signature_len)
{
  sl_status_t status;
  sli_si91x_csr_sign_request_t *request = (sli_si91x_csr_sign_request_t *)malloc(sizeof(*request));

  SL_VERIFY_POINTER_OR_RETURN(request, SL_STATUS_ALLOCATION_FAILED);
  memset(request, 0, sizeof(*request));

  request->algorithm_type = CSR_SIGN;
  request->sig_alg        = (uint8_t)signing_params->sig_alg;
  request->key_id         = key_id;
  request->cri_der_len    = signing_params->cri_der_len;
  request->cri_der        = (uint8_t *)signing_params->cri_der;
  request->signature      = signature;
  request->signature_len  = signature_len;

  status = sl_si91x_driver_send_side_band_crypto(SLI_COMMON_REQ_ENCRYPT_CRYPTO,
                                                 request,
                                                 sizeof(*request),
                                                 SLI_WIFI_WAIT_FOR_RESPONSE(SLI_COMMON_RSP_ENCRYPT_CRYPTO_WAIT_TIME));
  free(request);
  VERIFY_STATUS_AND_RETURN(status);
  return status;
}
#endif

sl_status_t sl_si91x_csr_sign(sl_si91x_ec_key_id_t key_id,
                              sl_si91x_csr_signing_params_t *signing_params,
                              uint8_t *signature,
                              uint16_t *signature_len)
{
  sl_status_t status;

  if (key_id > SL_SI91X_EC_KEY_ID_MAX) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  if (signing_params == NULL) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  if ((signing_params->cri_der == NULL) || (signing_params->cri_der_len == 0)
      || (signing_params->cri_der_len > SL_SI91X_EC_MAX_CRI_DER_LEN)) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  if ((signature == NULL) || (signature_len == NULL) || (*signature_len < SL_SI91X_EC_MAX_SIGNATURE_DER_LEN)) {
    return SL_STATUS_INVALID_PARAMETER;
  }
  if (signing_params->sig_alg != SL_SI91X_EC_SIG_ALG_ECDSA_SHA256) {
    return SL_STATUS_EC_UNSUPPORTED_SIG_ALG;
  }
  if (!sl_si91x_is_device_initialized()) {
    return SL_STATUS_NOT_INITIALIZED;
  }

#ifdef SL_SI91X_SIDE_BAND_CRYPTO
  status = sli_si91x_csr_sign_side_band(key_id, signing_params, signature, signature_len);
#else
#if defined(SLI_MULTITHREAD_DEVICE_SI91X)
  if (crypto_csr_sign_mutex == NULL) {
    crypto_csr_sign_mutex = sl_si91x_crypto_threadsafety_init(crypto_csr_sign_mutex);
  }
  mutex_result = sl_si91x_crypto_mutex_acquire(crypto_csr_sign_mutex);
#endif

  status       = sli_si91x_csr_sign_pending(key_id, signing_params, signature, signature_len);

#if defined(SLI_MULTITHREAD_DEVICE_SI91X)
  mutex_result = sl_si91x_crypto_mutex_release(crypto_csr_sign_mutex);
#endif
#endif

  if (status == SL_STATUS_OK) {
    signing_params->sig_alg = SL_SI91X_EC_SIG_ALG_ECDSA_SHA256;
  }
  return status;
}
