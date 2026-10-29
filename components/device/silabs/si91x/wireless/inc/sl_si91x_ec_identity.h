/***************************************************************************/ /**
 * @file sl_si91x_ec_identity.h
 * @brief SiWx91x EC identity types for Wi-Fi generate/delete credential APIs.
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

#include <stdint.h>
#include "sl_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup SL_SI91X_EC_IDENTITY EC Identity
 * \ingroup SL_SI91X_API
 * @{ */

/// Host-supplied EC identity identifier. Distinct from SET_CERT `cert_index`.
typedef uint32_t sl_si91x_ec_key_id_t;

/// Maximum number of on-device EC identities in this release (Phase-1 limit: one).
#define SL_SI91X_EC_MAX_IDENTITIES 1u

/// Maximum valid `key_id` value (inclusive).
#define SL_SI91X_EC_KEY_ID_MAX (SL_SI91X_EC_MAX_IDENTITIES - 1u)

/// Maximum SubjectPublicKeyInfo length returned by generate (bytes).
#define SL_SI91X_EC_MAX_SPKI_LEN 256u

/// Maximum CSR CertificationRequestInfo DER length accepted by Platform CSR sign (bytes).
#define SL_SI91X_EC_MAX_CRI_DER_LEN 1024u

/// Maximum ECDSA signature DER length returned by Platform CSR sign (bytes).
#define SL_SI91X_EC_MAX_SIGNATURE_DER_LEN 80u

/**
 * @enum sl_si91x_ec_curve_t
 * @brief Named curves supported for on-device EC key generation.
 *
 * @details
 * Phase-1 supports P-256 only. Additional curves are reserved for a later release.
 */
typedef enum {
  SL_SI91X_EC_CURVE_SECP256R1 = 1, ///< NIST P-256 (secp256r1). Phase-1 supported curve.
} sl_si91x_ec_curve_t;

/**
 * @enum sl_si91x_ec_sig_alg_t
 * @brief Signature algorithms for CSR TBS signing.
 *
 * @details
 * Phase-1 supports ECDSA with SHA-256 only. Used by Platform @ref sl_si91x_csr_sign.
 */
typedef enum {
  SL_SI91X_EC_SIG_ALG_ECDSA_SHA256 = 1, ///< ECDSA with SHA-256. Phase-1 supported algorithm.
} sl_si91x_ec_sig_alg_t;

/**
 * @struct sl_si91x_ec_generate_params_t
 * @brief In/out parameters for @ref SL_NET_GENERATE_EC_PRIVATE_KEY.
 *
 * @details
 * The host supplies `curve`. On success the NWP fills `spki` / `spki_len` with the
 * SubjectPublicKeyInfo used to build a CSR on the host. The private key never leaves the NWP.
 */
typedef struct {
  sl_si91x_ec_curve_t curve;              ///< Named curve. Must be @ref SL_SI91X_EC_CURVE_SECP256R1.
  uint8_t spki[SL_SI91X_EC_MAX_SPKI_LEN]; ///< SubjectPublicKeyInfo returned by the NWP (output).
  uint16_t spki_len;                      ///< Number of valid bytes in `spki` (output).
} sl_si91x_ec_generate_params_t;

/** @} */

#ifdef __cplusplus
}
#endif
