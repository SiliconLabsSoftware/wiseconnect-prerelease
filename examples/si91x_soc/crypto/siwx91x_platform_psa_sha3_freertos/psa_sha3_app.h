/*******************************************************************************
 * @file  psa_sha3_app.h
 * @brief
 *******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#ifndef PSA_SHA3_APP_H
#define PSA_SHA3_APP_H

#include "psa/crypto.h"
#include "sl_si91x_psa_sha3.h"
#include "sl_si91x_sha3.h"

#define SL_SLI91x_SHA3_224_EN
#define SL_SLI91x_SHA3_256_EN
#define SL_SLI91x_SHA3_384_EN
#define SL_SLI91x_SHA3_512_EN

void psa_app_process_action(void);

#ifdef SL_SLI91x_SHA3_224_EN
void test_psa_sha3_224(void);
#endif
#ifdef SL_SLI91x_SHA3_256_EN
void test_psa_sha3_256(void);
#endif
#ifdef SL_SLI91x_SHA3_384_EN
void test_psa_sha3_384(void);
#endif
#ifdef SL_SLI91x_SHA3_512_EN
void test_psa_sha3_512(void);
#endif

#endif /* PSA_SHA3_APP_H */