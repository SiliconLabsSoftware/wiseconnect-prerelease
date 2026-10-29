/***************************************************************************/ /**
 * @file sl_si91x_csr_sign.h
 * @brief Platform CSR TBS signing API for on-device EC identities.
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
#include "sl_si91x_ec_identity.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup CRYPTO_CSR_SIGN_TYPES
 * @{
 */

/**
 * @struct sl_si91x_csr_signing_params_t
 * @brief In/out parameters for @ref sl_si91x_csr_sign.
 *
 * @details
 * Phase-1: host supplies CertificationRequestInfo DER and requested signature algorithm.
 * On success `sig_alg` is updated to the algorithm used by the NWP.
 */
typedef struct {
  const uint8_t *cri_der;        ///< CertificationRequestInfo DER (TBS) to sign.
  uint16_t cri_der_len;          ///< Length of `cri_der` in bytes.
  sl_si91x_ec_sig_alg_t sig_alg; ///< In: requested algorithm. Out: algorithm used by the NWP.
} sl_si91x_csr_signing_params_t;

/** @} */

/**
 * @addtogroup CRYPTO_CSR_SIGN_FUNCTIONS
 * @{
 */

/***************************************************************************/ /**
 * @brief
 *   Sign a CSR CertificationRequestInfo (TBS) with an on-device EC private key.
 *
 * @details
 *   Platform crypto API. Frames a platform crypto command (`ENCRYPT_CRYPTO`) to
 *   the NWP; Platform Security produces the ECDSA signature. The private key
 *   never leaves the NWP.
 *
 *   The host builds the CSR TBS using the SPKI returned by
 *   @ref sl_net_generate_and_set_credential with @ref SL_NET_GENERATE_EC_PRIVATE_KEY.
 *
 *   Phase-1 accepts @ref SL_SI91X_EC_SIG_ALG_ECDSA_SHA256 only.
 *
 * @pre Pre-conditions:
 * - Device must be initialized.
 * - @ref sl_net_generate_and_set_credential with @ref SL_NET_GENERATE_EC_PRIVATE_KEY
 *   must have succeeded for `key_id`.
 *
 * @param[in] key_id
 *   Host-supplied identity identifier used when the key was generated.
 *   Not a SET_CERT `cert_index`.
 * @param[in,out] signing_params
 *   CSR signing parameters (@ref sl_si91x_csr_signing_params_t).
 * @param[out] signature
 *   Buffer that receives the DER-encoded ECDSA signature.
 * @param[in,out] signature_len
 *   In: size of `signature`. Out: number of bytes written.
 *
 * @return
 *   sl_status_t. See [Status Codes](https://docs.silabs.com/gecko-platform/latest/platform-common/status)
 *   and [WiSeConnect Status Codes](../wiseconnect-api-reference-guide-err-codes/wiseconnect-status-codes).
 ******************************************************************************/
sl_status_t sl_si91x_csr_sign(sl_si91x_ec_key_id_t key_id,
                              sl_si91x_csr_signing_params_t *signing_params,
                              uint8_t *signature,
                              uint16_t *signature_len);

/** @} */

#ifdef __cplusplus
}
#endif
