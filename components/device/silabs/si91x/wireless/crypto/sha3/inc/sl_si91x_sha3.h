/*******************************************************************************
 * @file  sl_si91x_sha3.h
 * @brief SHA3 hash API for Si91x
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

#pragma once
#include "sl_si91x_crypto.h"
#include "sl_status.h"

/******************************************************
  *                    Constants
  ******************************************************/
/**
  * @addtogroup CRYPTO_SHA3_CONSTANTS
  * @{
  */

#define SL_SI91X_SHA3_LEN_INVALID 0 ///< SHA3 length is invalid

/**
  * @brief SHA3 modes supported by the SI91X device (algorithm_sub_type for SHA3).
  *
  * Values align with the SHA API mapping for digest width: 224, 256, 384, and 512-bit.
  */
typedef enum {
  SL_SI91X_SHA3_512 = 9,  ///< SHA3-512
  SL_SI91X_SHA3_384 = 13, ///< SHA3-384
  SL_SI91X_SHA3_256 = 17, ///< SHA3-256
  SL_SI91X_SHA3_224 = 18, ///< SHA3-224
} sl_si91x_crypto_sha3_mode_t;

/**
  * @brief Digest lengths for SHA3 modes (bytes).
  */
typedef enum {
  SL_SI91X_SHA3_224_DIGEST_LEN = 28, ///< SHA3-224
  SL_SI91X_SHA3_256_DIGEST_LEN = 32, ///< SHA3-256
  SL_SI91X_SHA3_384_DIGEST_LEN = 48, ///< SHA3-384
  SL_SI91X_SHA3_512_DIGEST_LEN = 64, ///< SHA3-512
} sl_si91x_sha3_length_t;

/** @} */

/******************************************************
  *                Function Declarations
  ******************************************************/
/**
  * @addtogroup CRYPTO_SHA3_FUNCTIONS
  * @{
  */
#ifndef SL_SI91X_SIDE_BAND_CRYPTO
/***************************************************************************/
/**
  * @brief
  *   To provide the SHA3 (Keccak) output for the given configuration. This is a blocking API.
  * @param[in] sha3_mode
  *   SHA3 mode. One of:
  *   ::SL_SI91X_SHA3_224, ::SL_SI91X_SHA3_256, ::SL_SI91X_SHA3_384, ::SL_SI91X_SHA3_512.
  * @param[in]  msg
  *   Pointer to the (already-padded) message. SHA3 multi-rate padding (pad10*1
  *   with the 0x06 domain separator) is NOT applied by the firmware and must be
  *   performed by the caller.
  * @param[in]  msg_length
  *   Total message length in bytes.
  * @param[out] digest
  *   Buffer to store the output digest. Must be at least the digest length for
  *   the selected mode (see ::sl_si91x_sha3_length_t).
  * @return
  *   sl_status_t.
  * For more information on status codes, see
  * [SL STATUS DOCUMENTATION](https://docs.silabs.com/gecko-platform/latest/platform-common/status).
  ******************************************************************************/
sl_status_t sl_si91x_sha3(uint8_t sha3_mode, const uint8_t *msg, uint16_t msg_length, uint8_t *digest);
#endif

/***************************************************************************/
/**
  * @brief
  *   Multipart/chunked SHA3 (Keccak) API. This is a blocking API.
  *   When ::SL_SI91X_SIDE_BAND_CRYPTO is defined, chunks are sent over the
  *   side-band path; otherwise this returns ::SL_STATUS_NOT_SUPPORTED.
  * @param[in] sha3_mode
  *   SHA3 mode. One of:
  *   ::SL_SI91X_SHA3_224, ::SL_SI91X_SHA3_256, ::SL_SI91X_SHA3_384, ::SL_SI91X_SHA3_512.
  * @param[in]  msg
  *   Pointer to the (already-padded) message chunk.
  * @param[in]  msg_length
  *   Chunk length in bytes.
  * @param[in]  hash_flags
  *   Hash chunk flags (e.g. FIRST_CHUNK, MIDDLE_CHUNK, LAST_CHUNK).
  * @param[out] digest
  *   Buffer to store the output digest. May be NULL for FIRST_CHUNK/MIDDLE_CHUNK
  *   only; required for LAST_CHUNK.
  * @return
  *   sl_status_t.
  * For more information on status codes, see
  * [SL STATUS DOCUMENTATION](https://docs.silabs.com/gecko-platform/latest/platform-common/status).
  ******************************************************************************/
sl_status_t sl_si91x_mp_sha3(uint8_t sha3_mode,
                             const uint8_t *msg,
                             uint16_t msg_length,
                             uint8_t hash_flags,
                             const uint8_t *digest);

/** @} */