/*******************************************************************************
 * @file
 * @brief 
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
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

#include <gtest/gtest.h>
extern "C" {
#include "sli_buffer_manager.h"
#include "sl_slist.h"
#include "cmsis_os2.h"
#include "sli_buffer_manager_fake_function.h"
}
typedef struct {
  sl_slist_node_t next;          //< Next node(used only in case of common mempool).
  void *mempool_memory;          ///< Memory pool memory.
  sli_mem_pool_handle_t mempool; ///< Memory pool handler.

  uint16_t max_buffer_count;       ///< Maximum buffer count.
  uint16_t allocated_buffer_count; ///< Allocated buffer count.
  bool is_common_pool;             ///< Whether the buffer has been allocated from common mempool.
} sli_buffer_manager_mempool_handler_t;
typedef struct {
  sli_buffer_manager_mempool_handler_t
    *buffer_manager_mempool_handler; ///< pointer of the mempool from which the data has been allocated.
  uint8_t data[];                    ///< Data.
} sli_internal_buffer_t;

namespace {
// SLI_SYSTEM_TICKS_TO_MS / SLI_SYSTEM_MS_TO_TICKS use osKernelGetTickFreq(); fake default is 0 → div-by-zero.
// FFF repeats the last osKernelGetTickCount sequence value forever; it must yield elapsed_ms >= typical waits
// (1000 ms) or allocate_buffer_from_common_pool's wait loop never exits (osEventFlagsWait fake never times out).
struct SliBufferManagerUnitTestFakesInit {
  SliBufferManagerUnitTestFakesInit()
  {
    osKernelGetTickFreq_fake.return_val = 1000U;
  }
};
const SliBufferManagerUnitTestFakesInit s_buffer_manager_unit_test_fakes_init;

// { start-ish, mid, mid, steady } — last value is repeated by FFF and must map to >= 1000 ms elapsed vs start 0.
static uint32_t s_hybrid_alloc_tick_seq[] = { 0U, 1001U, 1001U, 10000U };
} // namespace

TEST(sli_buffer_manager, sli_buffer_manager_init_null_configuration)
{
  sl_status_t status;
  status = sli_buffer_manager_init(NULL);
  EXPECT_TRUE(status == SL_STATUS_NULL_POINTER);
}

TEST(sli_buffer_manager, sli_buffer_manager_init_success)
{
  sl_status_t status;
  sli_buffer_manager_pool_info_t dedicated_pool_info[SLI_BUFFER_MANAGER_MAX_POOL];
  sli_buffer_manager_configuration_t configuration;
  configuration.common_pool_info.block_count = 1;
  configuration.common_pool_info.block_size  = 1640;
  for (int i = 0; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    dedicated_pool_info[i].block_count = 1;
    dedicated_pool_info[i].block_size  = 1640;
    configuration.pool_info[i]         = &dedicated_pool_info[i];
  }
  status = sli_buffer_manager_init(&configuration);
  EXPECT_TRUE(status == SL_STATUS_OK);
  status = sli_buffer_manager_deinit();
  EXPECT_TRUE(status == SL_STATUS_OK);
}

TEST(sli_buffer_manager, sli_buffer_manager_init_dedicated_pool_fail)
{
  sl_status_t status;
  sli_buffer_manager_pool_info_t dedicated_pool_info[SLI_BUFFER_MANAGER_MAX_POOL];
  sli_buffer_manager_configuration_t configuration;
  configuration.common_pool_info.block_count = 1;
  configuration.common_pool_info.block_size  = 1640;
  for (int i = 0; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    dedicated_pool_info[i].block_count = 90000;
    dedicated_pool_info[i].block_size  = 1640000;
    configuration.pool_info[i]         = &dedicated_pool_info[i];
  }
  status = sli_buffer_manager_init(&configuration);
  EXPECT_TRUE(status == SL_STATUS_NO_MORE_RESOURCE);
}

TEST(sli_buffer_manager, sli_buffer_manager_init_common_pool_fail)
{
  sl_status_t status;
  sli_buffer_manager_pool_info_t dedicated_pool_info[SLI_BUFFER_MANAGER_MAX_POOL];
  sli_buffer_manager_configuration_t configuration;
  configuration.common_pool_info.block_count = 90000;
  configuration.common_pool_info.block_size  = 1640000;
  for (int i = 0; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    dedicated_pool_info[i].block_count = 1;
    dedicated_pool_info[i].block_size  = 1640;
    configuration.pool_info[i]         = &dedicated_pool_info[i];
  }
  status = sli_buffer_manager_init(&configuration);
  EXPECT_TRUE(status == SL_STATUS_NO_MORE_RESOURCE);
}

TEST(sli_buffer_manager, sli_buffer_manager_init_common_pool_config_failure)
{
  sl_status_t status;
  sli_buffer_manager_pool_info_t dedicated_pool_info[SLI_BUFFER_MANAGER_MAX_POOL];
  sli_buffer_manager_configuration_t configuration;
  configuration.common_pool_info.block_count = 0;
  configuration.common_pool_info.block_size  = 0;
  for (int i = 0; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    dedicated_pool_info[i].block_count = 1;
    dedicated_pool_info[i].block_size  = 1640;
    configuration.pool_info[i]         = &dedicated_pool_info[i];
  }
  status = sli_buffer_manager_init(&configuration);
  EXPECT_TRUE(status == SL_STATUS_INVALID_PARAMETER);
}

TEST(sli_buffer_manager, sli_buffer_manager_init_dedicated_pool_with_zero_config_should_skip_pool_creation)
{
  sl_status_t status;
  sli_buffer_manager_pool_info_t dedicated_pool_info[SLI_BUFFER_MANAGER_MAX_POOL];
  sli_buffer_manager_configuration_t configuration;
  configuration.common_pool_info.block_count = 1;
  configuration.common_pool_info.block_size  = 1640;
  for (int i = 0; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    dedicated_pool_info[i].block_count = 0;
    dedicated_pool_info[i].block_size  = 0;
    configuration.pool_info[i]         = &dedicated_pool_info[i];
  }
  status = sli_buffer_manager_init(&configuration);
  EXPECT_TRUE(status == SL_STATUS_OK);
  status = sli_buffer_manager_deinit();
  EXPECT_TRUE(status == SL_STATUS_OK);
}

TEST(sli_buffer_manager, sli_buffer_manager_free_buffer_null_pointer)
{
  sl_status_t status;
  status = sli_buffer_manager_free_buffer(NULL);
  EXPECT_TRUE(status == SL_STATUS_NULL_POINTER);
}

TEST(sli_buffer_manager, sli_buffer_manager_free_buffer_dedicated_pool)
{
  sl_status_t status;
  sli_buffer_t buffer;
  sli_buffer_manager_pool_info_t dedicated_pool_info[SLI_BUFFER_MANAGER_MAX_POOL];
  sli_buffer_manager_configuration_t configuration;
  configuration.common_pool_info.block_count = 1;
  configuration.common_pool_info.block_size  = 1640;
  for (int i = 0; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    dedicated_pool_info[i].block_count = 1;
    dedicated_pool_info[i].block_size  = 1640;
    configuration.pool_info[i]         = &dedicated_pool_info[i];
  }
  status = sli_buffer_manager_init(&configuration);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_DEDICATED,
                                              1000,
                                              &buffer);
  EXPECT_TRUE(status == SL_STATUS_OK);
  status = sli_buffer_manager_free_buffer(buffer);
  EXPECT_TRUE(status == SL_STATUS_OK);
  status = sli_buffer_manager_deinit();
  EXPECT_TRUE(status == SL_STATUS_OK);
}

TEST(sli_buffer_manager, sli_buffer_manager_free_buffer_common_pool_alive_allocations)
{
  sl_status_t status;
  sli_buffer_t buffer1, buffer2;
  sli_buffer_manager_pool_info_t dedicated_pool_info[SLI_BUFFER_MANAGER_MAX_POOL];
  sli_buffer_manager_configuration_t configuration;
  configuration.common_pool_info.block_count = 2;
  configuration.common_pool_info.block_size  = 1640;
  for (int i = 0; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    dedicated_pool_info[i].block_count = 1;
    dedicated_pool_info[i].block_size  = 1640;
    configuration.pool_info[i]         = &dedicated_pool_info[i];
  }
  status = sli_buffer_manager_init(&configuration);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer1);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer2);
  EXPECT_TRUE(status == SL_STATUS_OK);
  status = sli_buffer_manager_free_buffer(buffer1);
  EXPECT_TRUE(status == SL_STATUS_OK);
  status = sli_buffer_manager_free_buffer(buffer2);
  EXPECT_TRUE(status == SL_STATUS_OK);
  status = sli_buffer_manager_deinit();
  EXPECT_TRUE(status == SL_STATUS_OK);
}

TEST(sli_buffer_manager, sli_buffer_manager_deinit_valid_mempool_handlers)
{
  sl_status_t status;
  sli_buffer_manager_pool_info_t dedicated_pool_info[SLI_BUFFER_MANAGER_MAX_POOL];
  sli_buffer_manager_configuration_t configuration;
  configuration.common_pool_info.block_count = 1;
  configuration.common_pool_info.block_size  = 1640;
  for (int i = 0; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    dedicated_pool_info[i].block_count = 1;
    dedicated_pool_info[i].block_size  = 1640;
    configuration.pool_info[i]         = &dedicated_pool_info[i];
  }
  status = sli_buffer_manager_init(&configuration);
  EXPECT_TRUE(status == SL_STATUS_OK);
  status = sli_buffer_manager_deinit();
  EXPECT_TRUE(status == SL_STATUS_OK);
}

TEST(sli_buffer_manager, sli_buffer_manager_allocate_buffer_from_dedicated_pool_uninitialized_state)
{
  sl_status_t status;
  sli_buffer_t buffer;
  sli_buffer_manager_pool_info_t dedicated_pool_info[SLI_BUFFER_MANAGER_MAX_POOL];
  sli_buffer_manager_configuration_t configuration;
  configuration.common_pool_info.block_count = 1;
  configuration.common_pool_info.block_size  = 1640;
  for (int i = 0; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    dedicated_pool_info[i].block_count = 1;
    dedicated_pool_info[i].block_size  = 1640;
    configuration.pool_info[i]         = &dedicated_pool_info[i];
  }
  status = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_DEDICATED,
                                              1000,
                                              &buffer);
  EXPECT_TRUE(status == SL_STATUS_NOT_INITIALIZED);
}

TEST(sli_buffer_manager, sli_buffer_manager_allocate_buffer_from_common_mempool_uninitialized_state)
{
  sl_status_t status;
  sli_buffer_t buffer;
  sli_buffer_manager_pool_info_t dedicated_pool_info[SLI_BUFFER_MANAGER_MAX_POOL];
  sli_buffer_manager_configuration_t configuration;
  configuration.common_pool_info.block_count = 1;
  configuration.common_pool_info.block_size  = 1640;
  for (int i = 0; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    dedicated_pool_info[i].block_count = 1;
    dedicated_pool_info[i].block_size  = 1640;
    configuration.pool_info[i]         = &dedicated_pool_info[i];
  }

  status = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer);
  EXPECT_TRUE(status == SL_STATUS_NOT_INITIALIZED);
}

TEST(sli_buffer_manager, sli_buffer_manager_allocate_buffer_with_dedicated_pool_pass_from_dedicated_pool)
{
  sl_status_t status;
  sli_buffer_t buffer;
  sli_buffer_manager_pool_info_t dedicated_pool_info[SLI_BUFFER_MANAGER_MAX_POOL];
  sli_buffer_manager_configuration_t configuration;
  configuration.common_pool_info.block_count = 1;
  configuration.common_pool_info.block_size  = 1640;
  for (int i = 0; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    dedicated_pool_info[i].block_count = 1;
    dedicated_pool_info[i].block_size  = 1640;
    configuration.pool_info[i]         = &dedicated_pool_info[i];
  }
  status = sli_buffer_manager_init(&configuration);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_DEDICATED,
                                              1000,
                                              &buffer);
  sli_buffer_manager_deinit();
  EXPECT_TRUE(status == SL_STATUS_OK);
}

TEST(sli_buffer_manager, sli_buffer_manager_allocate_buffer_with_common_pool_pass_from_common_pool)
{
  sl_status_t status;
  sli_buffer_t buffer;
  sli_buffer_manager_pool_info_t dedicated_pool_info[SLI_BUFFER_MANAGER_MAX_POOL];
  sli_buffer_manager_configuration_t configuration;
  configuration.common_pool_info.block_count = 1;
  configuration.common_pool_info.block_size  = 1640;
  for (int i = 0; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    dedicated_pool_info[i].block_count = 1;
    dedicated_pool_info[i].block_size  = 1640;
    configuration.pool_info[i]         = &dedicated_pool_info[i];
  }
  status = sli_buffer_manager_init(&configuration);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer);
  sli_buffer_manager_deinit();
  EXPECT_TRUE(status == SL_STATUS_OK);
}

TEST(sli_buffer_manager, sli_buffer_manager_allocate_buffer_with_common_pool_pass_from_dedicated_pool)
{
  sl_status_t status;
  sli_buffer_t buffer;
  sli_buffer_manager_pool_info_t dedicated_pool_info[SLI_BUFFER_MANAGER_MAX_POOL];
  sli_buffer_manager_configuration_t configuration;
  configuration.common_pool_info.block_count = 1;
  configuration.common_pool_info.block_size  = 1640;
  for (int i = 0; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    dedicated_pool_info[i].block_count = 1;
    dedicated_pool_info[i].block_size  = 1640;
    configuration.pool_info[i]         = &dedicated_pool_info[i];
  }
  status = sli_buffer_manager_init(&configuration);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_buffer_manager_deinit();
}

TEST(sli_buffer_manager, sli_buffer_manager_free_buffer_common_pool_remove_from_queue_head)
{
  sl_status_t status;
  sli_buffer_t buffer1, buffer2, buffer3;
  sli_internal_buffer_t *internal_buffer;
  uint8_t *temp;
  sli_buffer_manager_pool_info_t dedicated_pool_info[SLI_BUFFER_MANAGER_MAX_POOL];
  sli_buffer_manager_configuration_t configuration;
  sli_mem_pool_alloc_fake.return_val         = (void *)malloc(1648);
  configuration.common_pool_info.block_count = 2;
  configuration.common_pool_info.block_size  = 1640;
  for (int i = 0; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    dedicated_pool_info[i].block_count = 1;
    dedicated_pool_info[i].block_size  = 1640;
    configuration.pool_info[i]         = &dedicated_pool_info[i];
  }
  status = sli_buffer_manager_init(&configuration);
  EXPECT_TRUE(status == SL_STATUS_OK);
  status = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_DEDICATED,
                                              1000,
                                              &buffer3);
  EXPECT_TRUE(status == SL_STATUS_OK);

  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer1);
  EXPECT_TRUE(status == SL_STATUS_OK);

  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer2);
  EXPECT_TRUE(status == SL_STATUS_OK);

  status = sli_buffer_manager_free_buffer(buffer1);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_buffer_manager_deinit();
}

TEST(sli_buffer_manager, sli_buffer_manager_allocate_buffer_with_common_pool_pass_from_new_common_pool)
{
  sl_status_t status;
  sli_buffer_t buffer1, buffer2, buffer3;
  sli_buffer_manager_pool_info_t dedicated_pool_info[SLI_BUFFER_MANAGER_MAX_POOL];
  sli_buffer_manager_configuration_t configuration;
  configuration.common_pool_info.block_count = 1;
  configuration.common_pool_info.block_size  = 1640;
  for (int i = 0; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    dedicated_pool_info[i].block_count = 1;
    dedicated_pool_info[i].block_size  = 1640;
    configuration.pool_info[i]         = &dedicated_pool_info[i];
  }
  status = sli_buffer_manager_init(&configuration);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_DEDICATED,
                                              1000,
                                              &buffer3);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer1);
  EXPECT_TRUE(status == SL_STATUS_OK);
  osKernelGetTickCount_reset();
  osKernelGetTickCount_fake.return_val_seq     = s_hybrid_alloc_tick_seq;
  osKernelGetTickCount_fake.return_val_seq_len = 4;
  osKernelGetTickCount_fake.return_val_seq_idx = 0;
  sli_mem_pool_alloc_fake.return_val           = (void *)malloc(1648);
  status                                       = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer2);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_buffer_manager_deinit();
}

TEST(sli_buffer_manager, sli_buffer_manager_free_buffer_common_pool_remove_from_queue_head_last_used)
{
  sl_status_t status;
  sli_buffer_t buffer1, buffer2, buffer3, buffer4, buffer5;
  sli_internal_buffer_t *internal_buffer;
  uint8_t *temp;
  sli_buffer_manager_pool_info_t dedicated_pool_info[SLI_BUFFER_MANAGER_MAX_POOL];
  sli_buffer_manager_configuration_t configuration;
  sli_mem_pool_alloc_fake.return_val         = (void *)malloc(1648);
  configuration.common_pool_info.block_count = 2;
  configuration.common_pool_info.block_size  = 1640;
  for (int i = 0; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    dedicated_pool_info[i].block_count = 1;
    dedicated_pool_info[i].block_size  = 1640;
    configuration.pool_info[i]         = &dedicated_pool_info[i];
  }
  status = sli_buffer_manager_init(&configuration);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_DEDICATED,
                                              1000,
                                              &buffer3);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer1);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer2);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  osKernelGetTickCount_reset();
  osKernelGetTickCount_fake.return_val_seq     = s_hybrid_alloc_tick_seq;
  osKernelGetTickCount_fake.return_val_seq_len = 4;
  osKernelGetTickCount_fake.return_val_seq_idx = 0;
  status                                       = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer4);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer5);
  EXPECT_TRUE(status == SL_STATUS_OK);
  status = sli_buffer_manager_free_buffer(buffer1);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer1);
  EXPECT_TRUE(status == SL_STATUS_OK);
  status = sli_buffer_manager_free_buffer(buffer1);
  EXPECT_TRUE(status == SL_STATUS_OK);
  status = sli_buffer_manager_free_buffer(buffer2);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_buffer_manager_deinit();
}

TEST(sli_buffer_manager, sli_buffer_manager_free_buffer_common_pool_remove_from_queue_tail)
{
  sl_status_t status;
  sli_buffer_t buffer1, buffer2, buffer3, buffer4;
  sli_internal_buffer_t *internal_buffer;
  uint8_t *temp;
  sli_buffer_manager_pool_info_t dedicated_pool_info[SLI_BUFFER_MANAGER_MAX_POOL];
  sli_buffer_manager_configuration_t configuration;
  configuration.common_pool_info.block_count = 1;
  configuration.common_pool_info.block_size  = 1640;
  for (int i = 0; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    dedicated_pool_info[i].block_count = 1;
    dedicated_pool_info[i].block_size  = 1640;
    configuration.pool_info[i]         = &dedicated_pool_info[i];
  }
  status = sli_buffer_manager_init(&configuration);
  EXPECT_TRUE(status == SL_STATUS_OK);
  status = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_DEDICATED,
                                              1000,
                                              &buffer3);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer1);
  EXPECT_TRUE(status == SL_STATUS_OK);
  osKernelGetTickCount_reset();
  osKernelGetTickCount_fake.return_val_seq     = s_hybrid_alloc_tick_seq;
  osKernelGetTickCount_fake.return_val_seq_len = 4;
  osKernelGetTickCount_fake.return_val_seq_idx = 0;
  sli_mem_pool_alloc_fake.return_val           = (void *)malloc(1648);
  status                                       = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer2);
  EXPECT_TRUE(status == SL_STATUS_OK);
  osKernelGetTickCount_reset();
  osKernelGetTickCount_fake.return_val_seq     = s_hybrid_alloc_tick_seq;
  osKernelGetTickCount_fake.return_val_seq_len = 4;
  osKernelGetTickCount_fake.return_val_seq_idx = 0;
  sli_mem_pool_alloc_fake.return_val           = (void *)malloc(1648);
  status                                       = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer4);
  EXPECT_TRUE(status == SL_STATUS_OK);
  status = sli_buffer_manager_free_buffer(buffer4);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_buffer_manager_deinit();
}

TEST(sli_buffer_manager, sli_buffer_manager_allocate_buffer_with_common_pool_fail_from_new_common_pool)
{
  sl_status_t status;
  sli_buffer_t buffer1, buffer2, buffer3;
  sli_buffer_manager_pool_info_t dedicated_pool_info;
  sli_buffer_manager_configuration_t configuration;
  configuration.common_pool_info.block_count = 1;
  configuration.common_pool_info.block_size  = 1640000;
  dedicated_pool_info.block_count            = 1;
  dedicated_pool_info.block_size             = 1640;
  configuration.pool_info[0]                 = &dedicated_pool_info;
  for (int i = 1; i < SLI_BUFFER_MANAGER_MAX_POOL; i++) {
    configuration.pool_info[i] = NULL;
  }
  status = sli_buffer_manager_init(&configuration);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_DEDICATED,
                                              1000,
                                              &buffer3);
  EXPECT_TRUE(status == SL_STATUS_OK);
  sli_mem_pool_alloc_fake.return_val = (void *)malloc(1648);
  status                             = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer1);
  EXPECT_TRUE(status == SL_STATUS_OK);
  /* Initial common pool (1 block) and dedicated pool are full; next hybrid alloc creates a new common pool
   * then allocates from it. Force mempool alloc to fail so the outcome does not depend on OOM from huge malloc. */
  osKernelGetTickCount_reset();
  osKernelGetTickCount_fake.return_val_seq     = s_hybrid_alloc_tick_seq;
  osKernelGetTickCount_fake.return_val_seq_len = 4;
  osKernelGetTickCount_fake.return_val_seq_idx = 0;
  sli_mem_pool_alloc_fake.return_val           = NULL;
  status                                       = sli_buffer_manager_allocate_buffer(SLI_BUFFER_MANAGER_CE_TX_POOL,
                                              SLI_BUFFER_MANAGER_ALLOCATION_TYPE_HYBRID,
                                              1000,
                                              &buffer2);
  EXPECT_EQ(status, SL_STATUS_ALLOCATION_FAILED);
  sli_buffer_manager_free_buffer(buffer1);
  sli_buffer_manager_free_buffer(buffer3);
  sli_buffer_manager_deinit();
}