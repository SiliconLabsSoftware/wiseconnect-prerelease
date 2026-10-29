/***************************************************************************/ /**
 * @file
 * @brief Application interface for the Memory Manager Heap 3 application.
 *
 * @details
 * Declares the initialization entry used by @c app_init() to start the CMSIS-RTOS2
 * thread that prints heap statistics and demonstrates the common allocator
 * paths (C library, Native Memory Manager, Heap 3, and one memory pool).
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

#ifndef MEMORY_MANAGER_HEAP3_APPLICATION_H
#define MEMORY_MANAGER_HEAP3_APPLICATION_H

/***************************************************************************/ /**
 * @brief Create the application CMSIS-RTOS2 thread.
 *
 * @details
 * The thread prints an initial heap snapshot, demonstrates C library, Native,
 * and FreeRTOS allocators, prints heap status again, creates and deletes one
 * memory pool, then prints a final heap snapshot. All heap values are printed
 * in hexadecimal.
 *
 * @note Call once from @ref app_init after the kernel is ready to create tasks.
 ******************************************************************************/
void memory_manager_heap3_application_init(void);

#endif /* MEMORY_MANAGER_HEAP3_APPLICATION_H */
