/*******************************************************************************
 * @file  sl_si91x_psa_sha3.h
 * @brief PSA SHA3 driver interface for Si91x
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

#ifndef SL_SI91X_PSA_SHA3_H
#define SL_SI91X_PSA_SHA3_H

#include "psa/crypto.h"

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------

/**
 * Multipart hash operation state for the Si91x PSA SHA3 driver.
 *
 * The standard PSA streaming API (\c psa_hash_update, \c psa_hash_finish, etc.) does not pass
 * \c psa_algorithm_t on each call; the algorithm is fixed at \c psa_hash_setup and carried in
 * this operation object.
 *
 * \p digest_size is the expected digest length for the selected SHA3 mode. On any intermediate
 * failure the implementation sets it to \c SL_SI91X_SHA3_LEN_INVALID so subsequent calls fail
 * without relying on a stale TA session.
 *
 * The TA must still be given a concrete SHA3 mode (\c SL_SI91X_SHA3_*) on \c update and \c finish.
 * That value is derived from \p digest_size via a \c switch in the \c .c file.
 */
typedef struct {
  size_t
    digest_size; ///< Expected digest length for the active hash, or \c SL_SI91X_SHA3_LEN_INVALID if the operation is invalid / failed
} sli_si91x_crypto_sha_operation_t;

// -----------------------------------------------------------------------------
//                          Public Function Declarations
// -----------------------------------------------------------------------------

/**
 * \brief Process a one-shot SHA3 hash operation.
 *
 * \note The signature of this function is that of a PSA driver
 *       hash_compute entry point as defined in the PSA driver interface
 *       specification for transparent drivers.
 */
psa_status_t sli_si91x_crypto_hash_compute(psa_algorithm_t alg,
                                           const uint8_t *input,
                                           size_t input_length,
                                           uint8_t *hash,
                                           size_t hash_size,
                                           size_t *hash_length);

/**
 * \brief Set up a multipart SHA3 hash operation.
 *
 * \note The signature of this function is that of a PSA driver
 *       hash_setup entry point as defined in the PSA driver interface
 *       specification for transparent drivers.
 */
psa_status_t sli_si91x_crypto_hash_setup(sli_si91x_crypto_sha_operation_t *operation, psa_algorithm_t alg);

/**
 * \brief Feed input data into an ongoing multipart SHA3 hash operation.
 *
 * \note The signature of this function is that of a PSA driver
 *       hash_update entry point as defined in the PSA driver interface
 *       specification for transparent drivers.
 */
psa_status_t sli_si91x_crypto_hash_update(sli_si91x_crypto_sha_operation_t *operation,
                                          const uint8_t *input,
                                          size_t input_length);

/**
 * \brief Finish a multipart SHA3 hash operation and produce the digest.
 *
 * \note The signature of this function is that of a PSA driver
 *       hash_finish entry point as defined in the PSA driver interface
 *       specification for transparent drivers.
 */
psa_status_t sli_si91x_crypto_hash_finish(sli_si91x_crypto_sha_operation_t *operation,
                                          uint8_t *hash,
                                          size_t hash_size,
                                          size_t *hash_length);

/**
 * \brief Abort a multipart SHA3 hash operation and release associated resources.
 *
 * \note The signature of this function is that of a PSA driver
 *       hash_abort entry point as defined in the PSA driver interface
 *       specification for transparent drivers.
 */
psa_status_t sli_si91x_crypto_hash_abort(sli_si91x_crypto_sha_operation_t *operation);

#endif /* SL_SI91X_PSA_SHA3_H */
