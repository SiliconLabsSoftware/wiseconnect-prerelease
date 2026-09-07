/***************************************************************************/ /**
 * @file
 * @brief Customer application for Native Memory Manager with FreeRTOS Heap 3.
 *
 * @details
 * Creates a CMSIS-RTOS2 thread that demonstrates the common allocator paths on
 * SiWx917, printing compact hexadecimal heap snapshots between phases:
 * 1. Initial heap snapshot.
 * 2. C library @c malloc / @c calloc / @c realloc / @c free (8-byte alignment).
 * 3. Native @c sl_malloc / @c sl_calloc / @c sl_realloc / @c sl_free.
 * 4. FreeRTOS Heap 3 @c pvPortMalloc / @c vPortFree (no CMSIS-OS2 malloc API),
 *    plus CMSIS-RTOS2 message queue and thread create/delete.
 * 5. Heap snapshot after those allocations and frees.
 * 6. One memory pool: create, allocate, use, free, delete.
 * 7. Final heap snapshot.
 *
 * With Heap 3, @c pvPortMalloc calls C @c malloc. Native Memory Manager retargets
 * libc malloc/free to @c sl_malloc / @c sl_free, so C library, Native, and
 * FreeRTOS allocations share one heap.
 *
 * Open the VCOM UART to observe the @c [MM] output.
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

#include "memory_manager_heap3_application.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "cmsis_os2.h"
#include "FreeRTOS.h" /* Heap 3 pvPortMalloc/vPortFree; CMSIS-OS2 has no malloc API. */
#include "sl_memory_manager.h"
#include "sl_status.h"

/*******************************************************************************
 ***************************  LOCAL DEFINES   **********************************
 ******************************************************************************/
/** @brief Stack size (bytes) for the application thread. */
#define MEMORY_MANAGER_HEAP3_APPLICATION_STACK_SIZE 4096U

/** @brief Probe block size used by the C library, Native, and FreeRTOS demos. */
#define MM_HEAP3_DEMO_BLOCK_SIZE 256U

/** @brief Number of elements passed to calloc() / sl_calloc(). */
#define MM_HEAP3_DEMO_CALLOC_COUNT 16U

/** @brief Size of each calloc() / sl_calloc() element in bytes (8-byte aligned). */
#define MM_HEAP3_DEMO_CALLOC_ELEM_SIZE 8U

/** @brief Initial malloc/sl_malloc size before realloc/sl_realloc grows the block. */
#define MM_HEAP3_DEMO_REALLOC_INITIAL_SIZE 64U

/** @brief Native Memory Manager default minimum alignment in bytes. */
#define MM_HEAP3_ALIGNMENT_BYTES 8U

/** @brief Mask used to test that a pointer is aligned to MM_HEAP3_ALIGNMENT_BYTES. */
#define MM_HEAP3_ALIGNMENT_MASK (MM_HEAP3_ALIGNMENT_BYTES - 1U)

/** @brief Fill pattern written into the C library malloc() probe block. */
#define MM_HEAP3_FILL_PATTERN_CLIB 0xA5U

/** @brief Fill pattern written into the Native sl_malloc() probe block. */
#define MM_HEAP3_FILL_PATTERN_NATIVE 0x5AU

/** @brief Fill pattern written into the FreeRTOS pvPortMalloc() probe block. */
#define MM_HEAP3_FILL_PATTERN_FREERTOS 0x3CU

/** @brief Base fill byte for pool blocks; each block uses base + index. */
#define MM_HEAP3_FILL_PATTERN_POOL_BASE 0x10U

/** @brief Payload string used to verify C library realloc() preserved contents. */
#define MM_HEAP3_REALLOC_TEST_STRING "heap3"

/** @brief Payload string used to verify Native sl_realloc() preserved contents. */
#define MM_HEAP3_NATIVE_REALLOC_TEST_STRING "native"

/** @brief Number of messages in the CMSIS-RTOS2 demo message queue. */
#define MM_HEAP3_DEMO_QUEUE_LENGTH 4U

/** @brief Size of each CMSIS-RTOS2 demo queue message in bytes. */
#define MM_HEAP3_DEMO_QUEUE_MSG_SIZE (sizeof(uint32_t))

/** @brief Helper thread stack size in bytes (CMSIS-RTOS2 osThreadAttr_t units). */
#define MM_HEAP3_HELPER_THREAD_STACK_SIZE 1024U

/** @brief Delay after creating the helper thread so it can run and exit. */
#define MM_HEAP3_HELPER_THREAD_DELAY_MS 10U

/** @brief Idle delay after the application completes (thread stays alive). */
#define MM_HEAP3_IDLE_DELAY_MS 1000U

/** @brief Number of fixed-size blocks in the Native Memory Manager pool demo. */
#define MM_HEAP3_DEMO_POOL_BLOCK_COUNT 4U

/** @brief Size of each block in the Native Memory Manager pool demo. */
#define MM_HEAP3_DEMO_POOL_BLOCK_SIZE 64U

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/
/** @brief CMSIS-RTOS2 attributes for the main application thread. */
static const osThreadAttr_t application_thread_attributes = {
  .name       = "mm_heap3_app",
  .stack_size = MEMORY_MANAGER_HEAP3_APPLICATION_STACK_SIZE,
  .priority   = osPriorityNormal,
};

/** @brief CMSIS-RTOS2 attributes for the Heap 3 helper thread. */
static const osThreadAttr_t helper_thread_attributes = {
  .name       = "mm_demo",
  .stack_size = MM_HEAP3_HELPER_THREAD_STACK_SIZE,
  .priority   = osPriorityLow1,
};

/*******************************************************************************
 *********************   LOCAL FUNCTION PROTOTYPES   ***************************
 ******************************************************************************/
static void memory_manager_heap3_print_heap_snapshot(const char *snapshot_label);
static bool memory_manager_heap3_is_8_byte_aligned(const void *allocated_pointer);
static void memory_manager_heap3_demo_clib(void);
static void memory_manager_heap3_demo_native(void);
static void memory_manager_heap3_helper_thread(void *unused_argument);
static void memory_manager_heap3_demo_freertos(void);
static void memory_manager_heap3_demo_pool(void);
static void memory_manager_heap3_application_thread(void *unused_argument);

/*******************************************************************************
 ***************************  LOCAL FUNCTIONS   ********************************
 ******************************************************************************/

/***************************************************************************/ /**
 * @brief Print a compact Native Memory Manager heap snapshot in hexadecimal.
 *
 * @param[in] snapshot_label Short label identifying when the snapshot was taken.
 ******************************************************************************/
static void memory_manager_heap3_print_heap_snapshot(const char *snapshot_label)
{
  /* Query the linker heap region owned by Native Memory Manager. */
  sl_memory_region_t heap_region = sl_memory_get_heap_region();

  /* Print base address plus total/free/used/high-watermark in hex. */
  printf("[MM] %-8s  base=0x%08lX  total=0x%08lX  free=0x%08lX  used=0x%08lX  high=0x%08lX\r\n",
         snapshot_label,
         (unsigned long)(uintptr_t)heap_region.addr,
         (unsigned long)sl_memory_get_total_heap_size(),
         (unsigned long)sl_memory_get_free_heap_size(),
         (unsigned long)sl_memory_get_used_heap_size(),
         (unsigned long)sl_memory_get_heap_high_watermark());
}

/***************************************************************************/ /**
 * @brief Return true if @p allocated_pointer meets Native MM 8-byte alignment.
 *
 * @param[in] allocated_pointer Pointer returned by an allocator.
 *
 * @return true if the pointer is non-NULL and aligned to MM_HEAP3_ALIGNMENT_BYTES.
 ******************************************************************************/
static bool memory_manager_heap3_is_8_byte_aligned(const void *allocated_pointer)
{
  /* Native MM default alignment is 8 bytes; reject NULL and misaligned pointers. */
  return (allocated_pointer != NULL) && (((uintptr_t)allocated_pointer & MM_HEAP3_ALIGNMENT_MASK) == 0u);
}

/***************************************************************************/ /**
 * @brief Demonstrate C library malloc / calloc / realloc / free.
 *
 * @details
 * libc is retargeted to Native Memory Manager, so these calls use the same
 * heap as @c sl_malloc and Heap 3 @c pvPortMalloc.
 ******************************************************************************/
static void memory_manager_heap3_demo_clib(void)
{
  size_t bytes_allocated = 0; /* Requested bytes successfully allocated in this demo. */
  size_t bytes_freed     = 0; /* Requested bytes returned to the heap in this demo. */
  bool demo_passed       = true;

  /* Allocate a probe block and confirm 8-byte alignment. */
  void *malloc_block = malloc(MM_HEAP3_DEMO_BLOCK_SIZE);
  demo_passed        = demo_passed && memory_manager_heap3_is_8_byte_aligned(malloc_block);
  if (malloc_block != NULL) {
    bytes_allocated += MM_HEAP3_DEMO_BLOCK_SIZE;
    memset(malloc_block, (int)MM_HEAP3_FILL_PATTERN_CLIB, MM_HEAP3_DEMO_BLOCK_SIZE);
    free(malloc_block);
    malloc_block = NULL;
    bytes_freed += MM_HEAP3_DEMO_BLOCK_SIZE;
  }

  /* Zero-initialized allocation (count * element size). */
  void *calloc_block = calloc(MM_HEAP3_DEMO_CALLOC_COUNT, MM_HEAP3_DEMO_CALLOC_ELEM_SIZE);
  demo_passed        = demo_passed && memory_manager_heap3_is_8_byte_aligned(calloc_block);
  if (calloc_block != NULL) {
    bytes_allocated += (MM_HEAP3_DEMO_CALLOC_COUNT * MM_HEAP3_DEMO_CALLOC_ELEM_SIZE);
    free(calloc_block);
    calloc_block = NULL;
    bytes_freed += (MM_HEAP3_DEMO_CALLOC_COUNT * MM_HEAP3_DEMO_CALLOC_ELEM_SIZE);
  }

  /* Grow a small block with realloc() and verify the original payload is kept. */
  void *realloc_original_block = malloc(MM_HEAP3_DEMO_REALLOC_INITIAL_SIZE);
  if (realloc_original_block != NULL) {
    bytes_allocated += MM_HEAP3_DEMO_REALLOC_INITIAL_SIZE;
    strcpy((char *)realloc_original_block, MM_HEAP3_REALLOC_TEST_STRING);
    void *realloc_grown_block = realloc(realloc_original_block, MM_HEAP3_DEMO_BLOCK_SIZE);
    if (realloc_grown_block != NULL) {
      /* realloc() invalidates the original pointer; drop it to avoid a dangling use. */
      realloc_original_block = NULL;
      /* Count only the extra bytes; the original size is still allocated. */
      if (MM_HEAP3_DEMO_BLOCK_SIZE > MM_HEAP3_DEMO_REALLOC_INITIAL_SIZE) {
        bytes_allocated += (MM_HEAP3_DEMO_BLOCK_SIZE - MM_HEAP3_DEMO_REALLOC_INITIAL_SIZE);
      }
      demo_passed = demo_passed && (strcmp((char *)realloc_grown_block, MM_HEAP3_REALLOC_TEST_STRING) == 0);
      free(realloc_grown_block);
      realloc_grown_block = NULL;
      bytes_freed += MM_HEAP3_DEMO_BLOCK_SIZE;
    } else {
      /* realloc() failed: free the original block so the demo still balances. */
      demo_passed = false;
      free(realloc_original_block);
      realloc_original_block = NULL;
      bytes_freed += MM_HEAP3_DEMO_REALLOC_INITIAL_SIZE;
    }
  } else {
    demo_passed = false;
  }

  printf("[MM] C library        allocated=0x%08lX  freed=0x%08lX%s\r\n",
         (unsigned long)bytes_allocated,
         (unsigned long)bytes_freed,
         demo_passed ? "" : "  FAIL");
}

/***************************************************************************/ /**
 * @brief Demonstrate Native Memory Manager simple APIs.
 *
 * @details
 * Uses @c sl_malloc / @c sl_calloc / @c sl_realloc / @c sl_free on the same
 * heap as the C library and FreeRTOS Heap 3 paths.
 ******************************************************************************/
static void memory_manager_heap3_demo_native(void)
{
  size_t bytes_allocated = 0;
  size_t bytes_freed     = 0;
  bool demo_passed       = true;

  /* Native simple alloc: equivalent to libc malloc, long-term block by default. */
  void *native_malloc_block = sl_malloc(MM_HEAP3_DEMO_BLOCK_SIZE);
  demo_passed               = demo_passed && memory_manager_heap3_is_8_byte_aligned(native_malloc_block);
  if (native_malloc_block != NULL) {
    bytes_allocated += MM_HEAP3_DEMO_BLOCK_SIZE;
    memset(native_malloc_block, (int)MM_HEAP3_FILL_PATTERN_NATIVE, MM_HEAP3_DEMO_BLOCK_SIZE);
    sl_free(native_malloc_block);
    native_malloc_block = NULL;
    bytes_freed += MM_HEAP3_DEMO_BLOCK_SIZE;
  }

  /* Native zero-initialized allocation. */
  void *native_calloc_block = sl_calloc(MM_HEAP3_DEMO_CALLOC_COUNT, MM_HEAP3_DEMO_CALLOC_ELEM_SIZE);
  demo_passed               = demo_passed && memory_manager_heap3_is_8_byte_aligned(native_calloc_block);
  if (native_calloc_block != NULL) {
    bytes_allocated += (MM_HEAP3_DEMO_CALLOC_COUNT * MM_HEAP3_DEMO_CALLOC_ELEM_SIZE);
    sl_free(native_calloc_block);
    native_calloc_block = NULL;
    bytes_freed += (MM_HEAP3_DEMO_CALLOC_COUNT * MM_HEAP3_DEMO_CALLOC_ELEM_SIZE);
  }

  /* Native realloc: grow the block and confirm the payload survived. */
  void *native_realloc_original_block = sl_malloc(MM_HEAP3_DEMO_REALLOC_INITIAL_SIZE);
  if (native_realloc_original_block != NULL) {
    bytes_allocated += MM_HEAP3_DEMO_REALLOC_INITIAL_SIZE;
    strcpy((char *)native_realloc_original_block, MM_HEAP3_NATIVE_REALLOC_TEST_STRING);
    void *native_realloc_grown_block = sl_realloc(native_realloc_original_block, MM_HEAP3_DEMO_BLOCK_SIZE);
    if (native_realloc_grown_block != NULL) {
      native_realloc_original_block = NULL;
      if (MM_HEAP3_DEMO_BLOCK_SIZE > MM_HEAP3_DEMO_REALLOC_INITIAL_SIZE) {
        bytes_allocated += (MM_HEAP3_DEMO_BLOCK_SIZE - MM_HEAP3_DEMO_REALLOC_INITIAL_SIZE);
      }
      demo_passed = demo_passed
                    && (strcmp((char *)native_realloc_grown_block, MM_HEAP3_NATIVE_REALLOC_TEST_STRING) == 0);
      sl_free(native_realloc_grown_block);
      native_realloc_grown_block = NULL;
      bytes_freed += MM_HEAP3_DEMO_BLOCK_SIZE;
    } else {
      demo_passed = false;
      sl_free(native_realloc_original_block);
      native_realloc_original_block = NULL;
      bytes_freed += MM_HEAP3_DEMO_REALLOC_INITIAL_SIZE;
    }
  } else {
    demo_passed = false;
  }

  printf("[MM] Native           allocated=0x%08lX  freed=0x%08lX%s\r\n",
         (unsigned long)bytes_allocated,
         (unsigned long)bytes_freed,
         demo_passed ? "" : "  FAIL");
}

/***************************************************************************/ /**
 * @brief Helper thread used only to prove CMSIS-RTOS2 thread creation succeeds.
 *
 * @param[in] unused_argument Unused.
 ******************************************************************************/
static void memory_manager_heap3_helper_thread(void *unused_argument)
{
  (void)unused_argument;
  osThreadExit(); /* Return TCB/stack to the heap via CMSIS-RTOS2. */
}

/***************************************************************************/ /**
 * @brief Demonstrate Heap 3 malloc and CMSIS-RTOS2 object creation.
 *
 * @details
 * CMSIS-OS2 has no malloc API, so @c pvPortMalloc / @c vPortFree are used to
 * exercise Heap 3 (which calls C @c malloc, retargeted to Native MM).
 * Message queue and helper thread use CMSIS-RTOS2 APIs.
 ******************************************************************************/
static void memory_manager_heap3_demo_freertos(void)
{
  size_t bytes_allocated = 0;
  size_t bytes_freed     = 0;
  bool demo_passed       = true;

  /* No CMSIS-OS2 malloc: Heap 3 pvPortMalloc() -> malloc() -> sl_malloc(). */
  void *heap3_malloc_block = pvPortMalloc(MM_HEAP3_DEMO_BLOCK_SIZE);
  demo_passed              = demo_passed && memory_manager_heap3_is_8_byte_aligned(heap3_malloc_block);
  if (heap3_malloc_block != NULL) {
    bytes_allocated += MM_HEAP3_DEMO_BLOCK_SIZE;
    memset(heap3_malloc_block, (int)MM_HEAP3_FILL_PATTERN_FREERTOS, MM_HEAP3_DEMO_BLOCK_SIZE);
    vPortFree(heap3_malloc_block);
    heap3_malloc_block = NULL;
    bytes_freed += MM_HEAP3_DEMO_BLOCK_SIZE;
  }

  /* CMSIS-RTOS2 queue create/delete allocates and frees kernel objects on this heap. */
  osMessageQueueId_t message_queue_id =
    osMessageQueueNew(MM_HEAP3_DEMO_QUEUE_LENGTH, MM_HEAP3_DEMO_QUEUE_MSG_SIZE, NULL);
  bool message_queue_created = (message_queue_id != NULL);
  demo_passed                = demo_passed && message_queue_created;
  if (message_queue_id != NULL) {
    /* Always delete; do not short-circuit on demo_passed from an earlier check. */
    osStatus_t queue_delete_status = osMessageQueueDelete(message_queue_id);
    demo_passed                    = demo_passed && (queue_delete_status == osOK);
    message_queue_id               = NULL;
  }

  /* CMSIS-RTOS2 thread create proves TCB/stack allocation; the thread exits itself. */
  osThreadId_t helper_thread_id = osThreadNew(memory_manager_heap3_helper_thread, NULL, &helper_thread_attributes);
  bool helper_thread_created    = (helper_thread_id != NULL);
  demo_passed                   = demo_passed && helper_thread_created;
  if (helper_thread_created) {
    osDelay(MM_HEAP3_HELPER_THREAD_DELAY_MS);
  }

  printf("[MM] FreeRTOS         allocated=0x%08lX  freed=0x%08lX  queue/task=%s%s\r\n",
         (unsigned long)bytes_allocated,
         (unsigned long)bytes_freed,
         (message_queue_created && helper_thread_created) ? "ok" : "fail",
         demo_passed ? "" : "  FAIL");
}

/***************************************************************************/ /**
 * @brief Create one pool, allocate/use blocks, then delete the pool.
 *
 * @details
 * @c sl_memory_create_pool() reserves one heap chunk for fixed-size blocks.
 * After @c sl_memory_delete_pool() that chunk is returned to the heap.
 ******************************************************************************/
static void memory_manager_heap3_demo_pool(void)
{
  size_t bytes_allocated = 0;
  size_t bytes_freed     = 0;
  bool demo_passed       = true;
  sl_memory_pool_t memory_pool;

  memset(&memory_pool, 0, sizeof(memory_pool)); /* Start from a clean pool handle. */

  /* Reserve a heap chunk large enough for POOL_BLOCK_COUNT blocks of POOL_BLOCK_SIZE. */
  sl_status_t pool_status =
    sl_memory_create_pool(MM_HEAP3_DEMO_POOL_BLOCK_SIZE, MM_HEAP3_DEMO_POOL_BLOCK_COUNT, &memory_pool);
  if (pool_status != SL_STATUS_OK) {
    printf("[MM] Pool            allocated=0x00000000  freed=0x00000000  FAIL\r\n");
    return;
  }

  void *pool_blocks[MM_HEAP3_DEMO_POOL_BLOCK_COUNT] = { 0 };
  unsigned allocated_block_count                    = 0;

  /* Allocate every pool block and write a unique fill pattern. */
  for (unsigned block_index = 0; block_index < MM_HEAP3_DEMO_POOL_BLOCK_COUNT; block_index++) {
    void *pool_block = NULL;
    pool_status      = sl_memory_pool_alloc(&memory_pool, &pool_block);
    if ((pool_status == SL_STATUS_OK) && (pool_block != NULL)) {
      memset(pool_block, (int)(MM_HEAP3_FILL_PATTERN_POOL_BASE + block_index), MM_HEAP3_DEMO_POOL_BLOCK_SIZE);
      pool_blocks[allocated_block_count++] = pool_block;
      bytes_allocated += MM_HEAP3_DEMO_POOL_BLOCK_SIZE;
    }
  }
  demo_passed = demo_passed && (allocated_block_count == MM_HEAP3_DEMO_POOL_BLOCK_COUNT);

  /* Return every allocated pool block (always free; do not short-circuit on demo_passed). */
  for (unsigned block_index = 0; block_index < allocated_block_count; block_index++) {
    pool_status = sl_memory_pool_free(&memory_pool, pool_blocks[block_index]);
    demo_passed = demo_passed && (pool_status == SL_STATUS_OK);
    bytes_freed += MM_HEAP3_DEMO_POOL_BLOCK_SIZE;
    pool_blocks[block_index] = NULL;
  }

  /* Delete the pool so its heap chunk is released (always delete; then fold status). */
  pool_status = sl_memory_delete_pool(&memory_pool);
  demo_passed = demo_passed && (pool_status == SL_STATUS_OK);
  printf("[MM] Pool            allocated=0x%08lX  freed=0x%08lX%s\r\n",
         (unsigned long)bytes_allocated,
         (unsigned long)bytes_freed,
         demo_passed ? "" : "  FAIL");
}

/***************************************************************************/ /**
 * @brief Application thread: heap demos and pool demo with hex snapshots.
 *
 * @param[in] unused_argument Unused thread argument (may be @c NULL).
 ******************************************************************************/
static void memory_manager_heap3_application_thread(void *unused_argument)
{
  (void)unused_argument;

  printf("\r\n");
  printf("============================================================\r\n");
  printf(" SiWx91x Memory Manager + FreeRTOS Heap 3 application\r\n");
  printf("============================================================\r\n");

  /* Snapshot before any demo allocations (thread stack is already on the heap). */
  memory_manager_heap3_print_heap_snapshot("initial");

  memory_manager_heap3_demo_clib();     /* libc malloc path (retargeted to Native MM). */
  memory_manager_heap3_demo_native();   /* Native sl_malloc path. */
  memory_manager_heap3_demo_freertos(); /* Heap 3 pvPortMalloc + CMSIS-RTOS2 objects. */

  /* After demos free their blocks; used/free should be close to initial. */
  memory_manager_heap3_print_heap_snapshot("after");

  memory_manager_heap3_demo_pool(); /* Fixed-size pool create/alloc/free/delete. */

  /* After pool delete, used/free should match the "after" snapshot. */
  memory_manager_heap3_print_heap_snapshot("final");

  printf("[MM] Done.\r\n");
  while (1) {
    osDelay(MM_HEAP3_IDLE_DELAY_MS); /* Keep the thread alive; demos already finished. */
  }
}

/*******************************************************************************
 ***************************  GLOBAL FUNCTIONS  ********************************
 ******************************************************************************/

/***************************************************************************/ /**
 * @brief Create the Memory Manager Heap 3 application thread.
 ******************************************************************************/
void memory_manager_heap3_application_init(void)
{
  /* CMSIS-RTOS2 thread; stack and TCB come from the unified Native MM heap. */
  osThreadId_t application_thread_id =
    osThreadNew(memory_manager_heap3_application_thread, NULL, &application_thread_attributes);
  if (application_thread_id == NULL) {
    printf("[MM] Failed to create application thread\r\n");
  }
}
