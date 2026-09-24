/***************************************************************************/ /**
 * @file sl_si91x_code_classifier.h
* @brief PSRAM Code Classifier for Memory Placement
*******************************************************************************
* # License
* <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef _SL_SI91X_CODE_CLASSIFICATION_H_
#define _SL_SI91X_CODE_CLASSIFICATION_H_

/***************************************************************************/ /**
 * @addtogroup CODE-CLASSIFIER Code Classifier
 * @ingroup SI91X_PERIPHERAL_APIS
 * @{
 ******************************************************************************/

/******************************************************************************/
/* Standard Code Classes for Memory Placement                                */
/******************************************************************************/
/// Place latency-sensitive functions or data for fast access.
#define SL_CODE_CLASS_TIME_CRITICAL timecritical
/// Force placement in internal SRAM.
#define SL_CODE_CLASS_FORCE_RAM force_ram
/// Force placement in external PSRAM.
#define SL_CODE_CLASS_FORCE_PSRAM force_psram
/// Place data where DMA can access it without extra copies.
#define SL_CODE_CLASS_DMA_ACCESSIBLE dma_accessible

/** @cond INTERNAL_MACROS */
/******************************************************************************/
/* Helper Macros                                                              */
/******************************************************************************/
#define _SL_SI91X_CC_STRINGIZE(X)        #X
#define _SL_SI91X_CC_XSTRINGIZE(X)       _SL_SI91X_CC_STRINGIZE(X)
#define _SL_SI91X_CC_CONCAT3(A, B, C)    A B C
#define _SL_SI91X_CC_CONCAT4(A, B, C, D) A B C D

/******************************************************************************/
/* Compiler-Specific Macros                                                   */
/******************************************************************************/
#define _SL_SI91X_CC_SECTION(section_name, count, line) \
  __attribute__((section(                               \
    _SL_SI91X_CC_XSTRINGIZE(section_name) "_" _SL_SI91X_CC_XSTRINGIZE(count) "_" _SL_SI91X_CC_XSTRINGIZE(line))))
/******************************************************************************/
/* Macro for Assigning Data Variables (Arrays, Buffers, Functions) to Sections */
/******************************************************************************/

#define _SL_SI91X_SECTION_CONCAT1(segment, component, class)          segment##_##component##_##class
#define _SL_SI91X_SECTION_CONCAT2(segment, component, class1, class2) segment##_##component##_##class1##_##class2

#define _SL_SI91X_CLASS1(segment, component, class) \
  _SL_SI91X_CC_SECTION(_SL_SI91X_SECTION_CONCAT1(segment, component, class), __COUNTER__, __LINE__)

#define _SL_SI91X_CLASS2(segment, component, class1, class2) \
  _SL_SI91X_CC_SECTION(_SL_SI91X_SECTION_CONCAT2(segment, component, class1, class2), __COUNTER__, __LINE__)

#define _SL_SI91X_CC_COUNT_N(_1, _2, N, ...) N
#define _SL_SI91X_CC_COUNT(...)              _SL_SI91X_CC_COUNT_N(__VA_ARGS__, 2, 1)
#define _SL_SI91X_CC_IDENTITY(N)             N
#define _SL_SI91X_CC_APPLY(macro, ...)       _SL_SI91X_CC_IDENTITY(macro(__VA_ARGS__))

#define _SL_SI91X_CC_DISPATCH(N) _SL_SI91X_CLASS##N
/** @endcond */

/***************************************************************************/ /**
 * Classify a function or variable into a named linker section.
 *
 * Place this macro immediately before the declaration. `segment` is the
 * section prefix, `component` is a unique identifier, and the remaining
 * arguments are one or two memory classes (`SL_CODE_CLASS_*`).
 *
 * @param segment   Section prefix (`text`, `data`, `bss_to_psram`, ...)
 * @param component Unique name used in the generated section
 ******************************************************************************/
#define SL_SI91X_CODE_CLASSIFY(segment, component, ...)                                             \
  _SL_SI91X_CC_IDENTITY(_SL_SI91X_CC_APPLY(_SL_SI91X_CC_DISPATCH, _SL_SI91X_CC_COUNT(__VA_ARGS__))) \
  (segment, component, __VA_ARGS__)

/** @} (end addtogroup CODE-CLASSIFIER) */

#endif // _SL_SI91X_CODE_CLASSIFICATION_H_