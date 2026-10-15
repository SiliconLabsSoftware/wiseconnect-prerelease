/*******************************************************************************
 * @file  psa_sha3_app.c
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
#include "psa_sha3_app.h"
#include "psa/crypto.h"
#include <stdio.h>
#include <string.h>

/*
  * Manual SHA3 multi-rate padding (pad10*1) with the SHA3 domain separator 0x06.
  *
  * Rate (in bytes) per mode (rate = (1600 - 2*digest_bits) / 8):
  *   SHA3-224 -> 144
  *   SHA3-256 -> 136
  *   SHA3-384 -> 104
  *   SHA3-512 ->  72
  */
#define SHA3_PAD_DOMAIN 0x06

#define SHA3_224_RATE 144
#define SHA3_256_RATE 136
#define SHA3_384_RATE 104
#define SHA3_512_RATE 72

/* Max padded buffer size = original 56 bytes + rate (worst case 144). */
#define SHA3_PAD_BUF_SIZE 256

static int sha3_pad(const uint8_t *msg,
                    size_t msg_len,
                    size_t rate,
                    uint8_t *out_buf,
                    size_t out_buf_size,
                    size_t *out_padded_len)
{
  if (rate == 0 || out_buf == NULL || out_padded_len == NULL) {
    return -1;
  }

  size_t q          = rate - (msg_len % rate); /* bytes to append: 1..rate */
  size_t padded_len = msg_len + q;

  if (padded_len > out_buf_size) {
    return -2;
  }

  if (msg_len > 0 && msg != NULL) {
    memcpy(out_buf, msg, msg_len);
  }

  if (q == 1) {
    /* Single pad byte: 0x06 | 0x80 = 0x86 */
    out_buf[msg_len] = SHA3_PAD_DOMAIN | 0x80;
  } else {
    out_buf[msg_len] = SHA3_PAD_DOMAIN;
    if (q > 2) {
      memset(&out_buf[msg_len + 1], 0x00, q - 2);
    }
    out_buf[padded_len - 1] = 0x80;
  }

  *out_padded_len = padded_len;
  return 0;
}

#ifdef SL_SLI91x_SHA3_224_EN
void test_psa_sha3_224()
{
  psa_status_t ret;

  //! Test message for streaming
  char test_msg[] = { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" };

  //! Expected SHA3_224 hash value of test message
  uint8_t expected_sha3_224_hash[] = { 0x8a, 0x24, 0x10, 0x8b, 0x15, 0x4a, 0xda, 0x21, 0xc9, 0xfd,
                                       0x55, 0x74, 0x49, 0x44, 0x79, 0xba, 0x5c, 0x7e, 0x7a, 0xb7,
                                       0x6e, 0xf2, 0x64, 0xea, 0xd0, 0xfc, 0xce, 0x33 };

  //! Hash Buffer
  uint8_t hash_buf[SL_SI91X_SHA3_224_DIGEST_LEN] = { 0 };

  //! Hash length
  size_t hash_len;

  /* Apply manual SHA3-224 padding (pad10*1 + 0x06) before calling PSA. */
  uint8_t padded_msg[SHA3_PAD_BUF_SIZE] = { 0 };
  size_t padded_len                     = 0;
  if (sha3_pad((const uint8_t *)test_msg, strlen(test_msg), SHA3_224_RATE, padded_msg, sizeof(padded_msg), &padded_len)
      != 0) {
    printf("\n SHA3_224 pad failed\n");
    return;
  }
  printf("\n\r SHA3_224 padded_len=%u (orig=%u)", (unsigned)padded_len, (unsigned)strlen(test_msg));

  ret = psa_hash_compute(PSA_ALG_SHA3_224, padded_msg, padded_len, hash_buf, sizeof(hash_buf), &hash_len);

  printf("\n\r SHA3_224 STATUS - %ld", ret);

  // Compare with the expected result
  if (ret != 0) {
    printf("\n SHA3_224 Operation Failed Status 0x%lx \n", ret);
  } else if (ret == 0 && memcmp(hash_buf, expected_sha3_224_hash, sizeof(expected_sha3_224_hash)) == 0) {
    printf("\n SHA3_224 Operation Success Status 0x%lx \n", ret);
  } else {
    printf("\n SHA3_224 Compare with expected value failed \n");
  }
}
#endif

#ifdef SL_SLI91x_SHA3_256_EN
void test_psa_sha3_256()
{
  psa_status_t ret;

  //! Test message for streaming
  const char test_msg[] = { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" };

  //! Expected SHA3_256 hash value of test message
  uint8_t expected_sha3_256_hash[] = { 0x41, 0xc0, 0xdb, 0xa2, 0xa9, 0xd6, 0x24, 0x08, 0x49, 0x10, 0x03,
                                       0x76, 0xa8, 0x23, 0x5e, 0x2c, 0x82, 0xe1, 0xb9, 0x99, 0x8a, 0x99,
                                       0x9e, 0x21, 0xdb, 0x32, 0xdd, 0x97, 0x49, 0x6d, 0x33, 0x76 };

  //! Hash Buffer
  uint8_t hash_buf[SL_SI91X_SHA3_256_DIGEST_LEN] = { 0 };

  //! Hash length
  size_t hash_len;

  /* Apply manual SHA3-256 padding (pad10*1 + 0x06) before calling PSA. */
  uint8_t padded_msg[SHA3_PAD_BUF_SIZE] = { 0 };
  size_t padded_len                     = 0;
  if (sha3_pad((const uint8_t *)test_msg, strlen(test_msg), SHA3_256_RATE, padded_msg, sizeof(padded_msg), &padded_len)
      != 0) {
    printf("\n SHA3_256 pad failed\n");
    return;
  }
  printf("\n\r SHA3_256 padded_len=%u (orig=%u)", (unsigned)padded_len, (unsigned)strlen(test_msg));

  ret = psa_hash_compute(PSA_ALG_SHA3_256, padded_msg, padded_len, hash_buf, sizeof(hash_buf), &hash_len);
  printf("\n\r SHA3_256 STATUS - %ld", ret);

  // Compare with the expected result
  if (ret != 0) {
    printf("\n SHA3_256 Operation Failed Status 0x%lx \n", ret);
  } else if (ret == 0 && memcmp(hash_buf, expected_sha3_256_hash, sizeof(expected_sha3_256_hash)) == 0) {
    printf("\n SHA3_256 Operation Success Status 0x%lx \n", ret);
  } else {
    printf("\n SHA3_256 Compare with expected value failed \n");
  }
}
#endif

#ifdef SL_SLI91x_SHA3_384_EN
void test_psa_sha3_384()
{
  psa_status_t ret;

  //! Test message for streaming
  char test_msg[] = { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" };

  //! Expected SHA3_384 hash value of test message
  uint8_t expected_sha3_384_hash[] = { 0x99, 0x1c, 0x66, 0x57, 0x55, 0xeb, 0x3a, 0x4b, 0x6b, 0xbd, 0xfb, 0x75,
                                       0xc7, 0x8a, 0x49, 0x2e, 0x8c, 0x56, 0xa2, 0x2c, 0x5c, 0x4d, 0x7e, 0x42,
                                       0x9b, 0xfd, 0xbc, 0x32, 0xb9, 0xd4, 0xad, 0x5a, 0xa0, 0x4a, 0x1f, 0x07,
                                       0x6e, 0x62, 0xfe, 0xa1, 0x9e, 0xef, 0x51, 0xac, 0xd0, 0x65, 0x7c, 0x22 };

  //! Hash Buffer
  uint8_t hash_buf[SL_SI91X_SHA3_384_DIGEST_LEN] = { 0 };

  //! Hash length
  size_t hash_len;

  /* Apply manual SHA3-384 padding (pad10*1 + 0x06) before calling PSA. */
  uint8_t padded_msg[SHA3_PAD_BUF_SIZE] = { 0 };
  size_t padded_len                     = 0;
  if (sha3_pad((const uint8_t *)test_msg, strlen(test_msg), SHA3_384_RATE, padded_msg, sizeof(padded_msg), &padded_len)
      != 0) {
    printf("\n SHA3_384 pad failed\n");
    return;
  }
  printf("\n\r SHA3_384 padded_len=%u (orig=%u)", (unsigned)padded_len, (unsigned)strlen(test_msg));

  ret = psa_hash_compute(PSA_ALG_SHA3_384, padded_msg, padded_len, hash_buf, sizeof(hash_buf), &hash_len);

  printf("\n\r SHA3_384 STATUS - %ld", ret);

  for (int i = 0; i < (int)hash_len; i++) {
    printf(" 0x%x", hash_buf[i]);
  }

  // Compare with the expected result
  if (ret != 0) {
    printf("\n SHA3_384 Operation Failed Status 0x%lx \n", ret);
  } else if (ret == PSA_SUCCESS && memcmp(hash_buf, expected_sha3_384_hash, sizeof(expected_sha3_384_hash)) == 0) {
    printf("\n SHA3_384 Operation Success Status 0x%lx \n", ret);
  } else {
    printf("\n SHA3_384 Compare with expected value failed \n");
  }
}
#endif

#ifdef SL_SLI91x_SHA3_512_EN
void test_psa_sha3_512()
{
  psa_status_t ret;

  //! Test message for streaming
  char test_msg[] = { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" };

  //! Expected SHA3_512 hash value of test message
  uint8_t expected_sha3_512_hash[] = { 0x04, 0xa3, 0x71, 0xe8, 0x4e, 0xcf, 0xb5, 0xb8, 0xb7, 0x7c, 0xb4, 0x86, 0x10,
                                       0xfc, 0xa8, 0x18, 0x2d, 0xd4, 0x57, 0xce, 0x6f, 0x32, 0x6a, 0x0f, 0xd3, 0xd7,
                                       0xec, 0x2f, 0x1e, 0x91, 0x63, 0x6d, 0xee, 0x69, 0x1f, 0xbe, 0x0c, 0x98, 0x53,
                                       0x02, 0xba, 0x1b, 0x0d, 0x8d, 0xc7, 0x8c, 0x08, 0x63, 0x46, 0xb5, 0x33, 0xb4,
                                       0x9c, 0x03, 0x0d, 0x99, 0xa2, 0x7d, 0xaf, 0x11, 0x39, 0xd6, 0xe7, 0x5e };

  //! Hash Buffer
  uint8_t hash_buf[SL_SI91X_SHA3_512_DIGEST_LEN] = { 0 };

  //! Hash length
  size_t hash_len;

  /* Apply manual SHA3-512 padding (pad10*1 + 0x06) before calling PSA. */
  uint8_t padded_msg[SHA3_PAD_BUF_SIZE] = { 0 };
  size_t padded_len                     = 0;
  if (sha3_pad((const uint8_t *)test_msg, strlen(test_msg), SHA3_512_RATE, padded_msg, sizeof(padded_msg), &padded_len)
      != 0) {
    printf("\n SHA3_512 pad failed\n");
    return;
  }
  printf("\n\r SHA3_512 padded_len=%u (orig=%u)", (unsigned)padded_len, (unsigned)strlen(test_msg));

  ret = psa_hash_compute(PSA_ALG_SHA3_512, padded_msg, padded_len, hash_buf, sizeof(hash_buf), &hash_len);

  printf("\n\r SHA3_512 STATUS - %ld", ret);

  // Compare with the expected result
  if (ret != 0) {
    printf("\n SHA3_512 Operation Failed Status 0x%lx \n", ret);
  } else if (ret == PSA_SUCCESS && memcmp(hash_buf, expected_sha3_512_hash, sizeof(expected_sha3_512_hash)) == 0) {
    printf("\n SHA3_512 Operation Success Status 0x%lx \n", ret);
  } else {
    printf("\n SHA3_512 Compare with expected value failed \n");
  }
}
#endif

void psa_app_process_action()
{
  psa_status_t ret;
  ret = psa_crypto_init();
  if (ret != PSA_SUCCESS) {
    printf("PSA Crypto Init failed with status : %ld\n", ret);
  } else {
    printf("PSA Crypto Init Success\n");
  }

#ifdef SL_SLI91x_SHA3_224_EN
  test_psa_sha3_224();
#endif
#ifdef SL_SLI91x_SHA3_256_EN
  test_psa_sha3_256();
#endif
#ifdef SL_SLI91x_SHA3_384_EN
  test_psa_sha3_384();
#endif
#ifdef SL_SLI91x_SHA3_512_EN
  test_psa_sha3_512();
#endif
}