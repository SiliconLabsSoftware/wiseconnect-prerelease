/*******************************************************************************
 * @file  sl_si91x_sha3.c
 * @brief
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

#include "sl_constants.h"
#include "sl_si91x_crypto.h"
#include "sl_si91x_protocol_types.h"
#include "sl_status.h"
#if defined(SLI_MULTITHREAD_DEVICE_SI91X)
#include "sl_si91x_crypto_thread.h"
#endif
#include "sl_si91x_driver.h"
#include "sl_si91x_sha3.h"
#include "sli_wifi_utility.h"
#include <string.h>

#ifndef SL_SI91X_SIDE_BAND_CRYPTO
static const uint8_t sha3_digest_len_table[] = {
  [SL_SI91X_SHA3_224] = SL_SI91X_SHA3_224_DIGEST_LEN,
  [SL_SI91X_SHA3_256] = SL_SI91X_SHA3_256_DIGEST_LEN,
  [SL_SI91X_SHA3_384] = SL_SI91X_SHA3_384_DIGEST_LEN,
  [SL_SI91X_SHA3_512] = SL_SI91X_SHA3_512_DIGEST_LEN,
};

static sl_status_t sli_si91x_sha3_pending(uint8_t sha3_mode,
                                          const uint8_t *msg,
                                          uint16_t msg_length,
                                          uint16_t chunk_len,
                                          uint8_t pending_flag,
                                          uint8_t *digest)
{
  sl_status_t status = SL_STATUS_OK;
  uint16_t send_size = 0;
  sl_wifi_buffer_t *buffer;
  const sl_wifi_system_packet_t *packet;
  sli_si91x_sha3_request_t *request = (sli_si91x_sha3_request_t *)malloc(sizeof(sli_si91x_sha3_request_t));

  SL_VERIFY_POINTER_OR_RETURN(request, SL_STATUS_ALLOCATION_FAILED);

  memset(request, 0, sizeof(sli_si91x_sha3_request_t));

  // Fill Algorithm type
  // SHA3 uses algorithm_type = SHA3 (16)
  if ((sha3_mode == SL_SI91X_SHA3_224) || (sha3_mode == SL_SI91X_SHA3_256) || (sha3_mode == SL_SI91X_SHA3_384)
      || (sha3_mode == SL_SI91X_SHA3_512)) {
    request->algorithm_type = SHA3;
  }

  request->algorithm_sub_type = sha3_mode;

  // Fill sha3_flags BIT(0) - 1st chunk BIT(2) - Last chunk
  request->sha3_flags = pending_flag;

  // Fill total msg length
  request->total_msg_length = msg_length;

  // Fill current chunk length
  request->current_chunk_length = chunk_len;

  // Memset before filling
  memset(&request->msg[0], 0, SL_SI91X_MAX_DATA_SIZE_IN_BYTES);

  // Copy Data
  if ((msg != NULL) && (chunk_len != 0)) {
    memcpy(&request->msg[0], msg, chunk_len);
  }

  send_size = sizeof(sli_si91x_sha3_request_t) - SL_SI91X_MAX_DATA_SIZE_IN_BYTES + chunk_len;

  status = sli_wifi_send_command(SLI_COMMON_REQ_ENCRYPT_CRYPTO,
                                 SLI_WIFI_COMMON_CMD,
                                 request,
                                 send_size,
                                 SLI_WIFI_WAIT_FOR_RESPONSE(SLI_COMMON_RSP_ENCRYPT_CRYPTO_WAIT_TIME),
                                 NULL,
                                 (void **)&buffer);
  if (status != SL_STATUS_OK) {
    free(request);
    if (buffer != NULL)
      sli_buffer_manager_free_buffer(buffer);
  }
  VERIFY_STATUS_AND_RETURN(status);

  packet = (sl_wifi_system_packet_t *)sli_wifi_host_get_buffer_data(buffer, 0, NULL);

  if (pending_flag == (LAST_CHUNK | FIRST_CHUNK) || pending_flag == LAST_CHUNK) {
    SL_ASSERT(packet->length == sha3_digest_len_table[sha3_mode]);
  }
  memcpy(digest, packet->data, sha3_digest_len_table[sha3_mode]);

  free(request);
  if (buffer != NULL)
    sli_buffer_manager_free_buffer(buffer);

  return status;
}

#else

static sl_status_t sli_si91x_sha3_mp_side_band(uint8_t sha3_mode,
                                               uint8_t *msg,
                                               uint16_t msg_length,
                                               uint16_t current_chunk_length,
                                               uint8_t sha_flags,
                                               const uint8_t *digest)
{
  sl_status_t status  = SL_STATUS_OK;
  uint8_t *output_ptr = (uint8_t *)digest;

  sli_si91x_sha3_mp_request_t *request = (sli_si91x_sha3_mp_request_t *)malloc(sizeof(sli_si91x_sha3_mp_request_t));

  if (request == NULL) {
    return SL_STATUS_ALLOCATION_FAILED;
  }

  memset(request, 0, sizeof(sli_si91x_sha3_mp_request_t));

  if ((sha3_mode == SL_SI91X_SHA3_224) || (sha3_mode == SL_SI91X_SHA3_256) || (sha3_mode == SL_SI91X_SHA3_384)
      || (sha3_mode == SL_SI91X_SHA3_512)) {
    request->algorithm_type = SHA3;
  }

  request->algorithm_sub_type   = sha3_mode;
  request->total_msg_length     = msg_length;
  request->current_chunk_length = current_chunk_length;
  request->sha3_flags           = sha_flags;
  request->msg                  = msg;
  request->output               = output_ptr;

  status = sl_si91x_driver_send_side_band_crypto(SLI_COMMON_REQ_ENCRYPT_CRYPTO,
                                                 request,
                                                 (sizeof(sli_si91x_sha3_mp_request_t)),
                                                 SLI_WIFI_WAIT_FOR_RESPONSE(SLI_COMMON_RSP_ENCRYPT_CRYPTO_WAIT_TIME));
  free(request);
  request = NULL;
  VERIFY_STATUS_AND_RETURN(status);
  return status;
}
#endif
#ifndef SL_SI91X_SIDE_BAND_CRYPTO
sl_status_t sl_si91x_sha3(uint8_t sha3_mode, const uint8_t *msg, uint16_t msg_length, uint8_t *digest)
{
  if (((msg == NULL) != (msg_length == 0)) || (digest == NULL)) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t status = SL_STATUS_OK;

#if defined(SLI_MULTITHREAD_DEVICE_SI91X)
  if (crypto_sha_mutex == NULL) {
    crypto_sha_mutex = sl_si91x_crypto_threadsafety_init(crypto_sha_mutex);
  }
  mutex_result = sl_si91x_crypto_mutex_acquire(crypto_sha_mutex);
#endif

  uint16_t total_len = 0;
  uint16_t chunk_len = 0;
  uint16_t offset    = 0;
  uint8_t sha3_flags = 0;

  total_len = msg_length;
  if (total_len != 0) {
    while (total_len) {
      // Check total length
      if (total_len > SL_SI91X_MAX_DATA_SIZE_IN_BYTES) {
        chunk_len = SL_SI91X_MAX_DATA_SIZE_IN_BYTES;
        if (offset == 0) {
          // Make sha_flag as first chunk
          sha3_flags |= FIRST_CHUNK;
        } else {
          // Make sha_flag as Middle chunk
          sha3_flags = MIDDLE_CHUNK;
        }
      } else {
        chunk_len = total_len;

        // Make sha_flag as Last chunk
        sha3_flags = LAST_CHUNK;
        if (offset == 0) {
          /* If the total length is less than 1400 and offset is zero,
                make sha3_flag as both first chunk as well as last chunk*/
          sha3_flags |= FIRST_CHUNK;
        }
      }

      // Send the current chunk length message
      status = sli_si91x_sha3_pending(sha3_mode, msg, msg_length, chunk_len, sha3_flags, digest);

      if (status != SL_STATUS_OK) {
#if defined(SLI_MULTITHREAD_DEVICE_SI91X)
        mutex_result = sl_si91x_crypto_mutex_release(crypto_sha_mutex);
#endif
        return status;
      }

      // Increment the offset value
      offset += chunk_len;
      msg += chunk_len;

      // Decrement the total message length
      total_len -= chunk_len;
    }
  } else {
    sha3_flags = LAST_CHUNK | FIRST_CHUNK;
    status     = sli_si91x_sha3_pending(sha3_mode, msg, msg_length, chunk_len, sha3_flags, digest);
    if (status != SL_STATUS_OK) {
#if defined(SLI_MULTITHREAD_DEVICE_SI91X)
      mutex_result = sl_si91x_crypto_mutex_release(crypto_sha_mutex);
#endif
      return status;
    }
  }

#if defined(SLI_MULTITHREAD_DEVICE_SI91X)
  mutex_result = sl_si91x_crypto_mutex_release(crypto_sha_mutex);
#endif

  return status;
}
#endif
sl_status_t sl_si91x_mp_sha3(uint8_t sha3_mode,
                             const uint8_t *msg,
                             uint16_t msg_length,
                             uint8_t hash_flags,
                             const uint8_t *digest)
{
  /*
      * digest is legitimately NULL for FIRST_CHUNK (setup) and MIDDLE_CHUNK
      * (update) calls where no output is produced yet. A non-NULL digest is
      * only required when LAST_CHUNK is set, as that is when the final hash
      * is written out.
      */
  if (((msg == NULL) != (msg_length == 0)) || ((hash_flags & LAST_CHUNK) && (digest == NULL))) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  sl_status_t status = SL_STATUS_OK;

#if defined(SLI_MULTITHREAD_DEVICE_SI91X)
  if (crypto_sha_mutex == NULL) {
    crypto_sha_mutex = sl_si91x_crypto_threadsafety_init(crypto_sha_mutex);
  }
  mutex_result = sl_si91x_crypto_mutex_acquire(crypto_sha_mutex);
#endif

#ifdef SL_SI91X_SIDE_BAND_CRYPTO
  status = sli_si91x_sha3_mp_side_band(sha3_mode, (uint8_t *)msg, msg_length, msg_length, hash_flags, digest);
#if defined(SLI_MULTITHREAD_DEVICE_SI91X)
  mutex_result = sl_si91x_crypto_mutex_release(crypto_sha_mutex);
#endif
  return status;
#else
  UNUSED_VARIABLE(sha3_mode);
  UNUSED_VARIABLE(hash_flags);
  status       = SL_STATUS_NOT_SUPPORTED;

#if defined(SLI_MULTITHREAD_DEVICE_SI91X)
  mutex_result = sl_si91x_crypto_mutex_release(crypto_sha_mutex);
#endif

  return status;
#endif
}
