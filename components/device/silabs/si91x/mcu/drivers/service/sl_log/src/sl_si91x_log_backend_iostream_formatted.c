/***************************************************************************/ /**
 * @file
 * @brief Platform backend glue for the logging subsystem
 *
 * ## Output format
 *
 * Each log line is sent to iostream backend and ends with CRLF (\\r\\n).
 *
 * **Common header** (all lines):
 *   [LEVEL|TYPE|TIMESTAMP]
 * - LEVEL: one of D (Debug), I (Info), W (Warning), E (Error), C (Crash).
 * - TYPE: S = string log (format string + args), E = numeric event.
 * - TIMESTAMP: 8 hexadecimal digits, zero-padded, uppercase
 *              (system timer units). Kept as fixed-width hex for
 *              backward compatibility with existing host-side parsers
 *              and the wire-format documentation (e.g. <timestamp_hex>
 *              examples like [I|S|0494B1BF]).
 *
 * **String log** (TYPE=S): a space, then the format string with specifiers
 * expanded according to the (subset of printf) grammar
 * `%[flags][width][.precision][length]conv`:
 *   - %d, %i  signed decimal (32-bit), supports width and zero-pad
 *   - %u      unsigned decimal (32-bit), supports width and zero-pad
 *   - %x      lowercase hex; default is 8-digit zero-padded if no width is
 *             given (backwards-compat with earlier releases)
 *   - %X      uppercase hex; same default-width behavior as %x
 *   - %o      unsigned octal
 *   - %p      pointer, always emitted as "0x" + 8 uppercase hex digits
 *   - %c      single character (low byte of arg)
 *   - %s      null-terminated string (arg is the string pointer)
 *   - %f, %F  decimal float; the ABI stores arguments as uint32_t, so the
 *             caller must pass the IEEE-754 bit pattern of a 32-bit float.
 *             A simple way to do this at the call site is:
 *               union { float f; uint32_t u; } _v = { .f = value };
 *               SL_PRINT_STRING_INFO("x=%f", _v.u);
 *             Default precision is 6, max 9. Inf / NaN print as "inf",
 *             "-inf", "nan"; values outside uint32 integer range print
 *             as "ovf".
 *   - %%      literal '%'
 *
 * Length modifiers (h, hh, l, ll, z, t, j) are accepted but ignored because
 * each argument is delivered as uint32_t through the log ABI. The '0' flag
 * enables zero-padding; '-', '+', ' ', '#' are accepted but not honored
 * (output is always left-aligned with leading padding).
 *
 * Example: format "count=%d addr=%p st=0x%04lX t=%.2f"
 *          with args -1, 0x1000, 0x3, <bit-pattern of 23.75f> gives
 *          [I|S|00005678] count=-1 addr=0x00001000 st=0x0003 t=23.75
 *
 * Float-truncation salvage: if a caller passes a raw float (not a
 * bit-cast) to the log ABI, the float is silently truncated to its
 * integer part by the C calling convention. The %f path detects the
 * resulting denormal bit-pattern and recovers the integer part instead
 * of printing 0.000000, e.g. SL_PRINT_STRING_ERROR("v=%.2f", 2.7f)
 * yields "v=2.00" rather than "v=0.000000".
 *
 * **Event** (TYPE=E): a space, then event_id (8 hex), then for each argument
 * in arg_count: 8 hex digits and '|', then core_id (2 hex), '|', version (2 hex).
 * Example: [D|E|00001234] 00000001|AABBCCDD|01|02
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

#include "sl_log_platform_specific.h"
#include "sl_log_helper.h"
#include "sl_log.h"
#include "sl_log_common_config.h"
#include "sl_iostream.h"
#include "sl_iostream_handles.h"
#include "em_device.h"
#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 ***************************  DEFINE MACROS ********************************
 ******************************************************************************/
/** Line buffer size for output. String logs are truncated when the next
 * conversion plus the trailing CRLF would not fit; see
 * SLI_LOG_FMT_LINE_TAIL_GUARD below. */
#define LINE_MAX 200

/** Default precision for "%f" when the format string does not specify one
 *  (matches the standard C printf default). */
#define SLI_LOG_FMT_DEFAULT_FLOAT_PRECISION 6u

/** Maximum precision honored for "%f"; values beyond this overrun a uint32
 *  intermediate and produce noise. */
#define SLI_LOG_FMT_MAX_FLOAT_PRECISION 9u

/** Worst-case number of bytes any single conversion specifier can emit
 *  into the line buffer. format_float() dominates: '-' + 10 integer
 *  digits (UINT32_MAX = 4294967295) + '.' + SLI_LOG_FMT_MAX_FLOAT_PRECISION
 *  fractional digits = 1 + 10 + 1 + 9 = 21. All other specifiers
 *  (format_uint up to 12, format_sint up to 13, %p = 10, etc.) stay
 *  comfortably below this, so 21 is a safe upper bound. */
#define SLI_LOG_FMT_MAX_SPECIFIER_WRITE (1u + 10u + 1u + SLI_LOG_FMT_MAX_FLOAT_PRECISION)

/** Bytes reserved at the tail of the line buffer so that the worst-case
 *  conversion plus the trailing CRLF cannot overflow. The format-string
 *  loop and the %s inner copy both use this as their stop condition. */
#define SLI_LOG_FMT_LINE_TAIL_GUARD (SLI_LOG_FMT_MAX_SPECIFIER_WRITE + 2u)

/** Bytes the event-type body writes per argument (8 hex digits + '|'). */
#define SLI_LOG_FMT_EVENT_ARG_BYTES 9u

/** Bytes the event-type body writes after the args loop:
 *  core_id (2 hex) + '|' + version (2 hex) + trailing CRLF. The args loop
 *  must keep this many bytes free at the tail of the line buffer. */
#define SLI_LOG_FMT_EVENT_TRAILER_BYTES (2u + 1u + 2u + 2u)

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/
static const char hex_chars_upper[] = "0123456789ABCDEF";
static const char hex_chars_lower[] = "0123456789abcdef";

/*******************************************************************************
**************************   LOCAL FUNCTIONS   ********************************
*******************************************************************************/

/**
 * @brief Write a 32-bit value as 8 hexadecimal (uppercase) digits.
 *
 * Used for the timestamp / event_id header fields, which are always emitted
 * as fixed-width 8-digit uppercase hex.
 *
 * @param[in,out] p  Pointer to the next character position in the buffer
 * @param[in]     v  Value to format (32-bit unsigned)
 *
 * @return Pointer to the character past the written digits (p + 8)
 */
static inline char *u32_to_hex8(char *p, uint32_t v)
{
  for (int i = 7; i >= 0; i--) {
    *p++ = hex_chars_upper[(v >> (i * 4)) & 0xF];
  }
  return p;
}

/**
 * @brief Write an 8-bit value as 2 hexadecimal (uppercase) digits.
 *
 * @param[in,out] p  Pointer to the next character position in the buffer
 * @param[in]     v  Value to format (8-bit unsigned)
 *
 * @return Pointer to the character past the written digits (p + 2)
 */
static inline char *u8_to_hex2(char *p, uint8_t v)
{
  *p++ = hex_chars_upper[(v >> 4) & 0xF];
  *p++ = hex_chars_upper[v & 0xF];
  return p;
}

/**
 * @brief Map a numeric log level to a single-character label.
 *
 * @param[in] level  Log level (1=Debug, 2=Info, 3=Warning, 4=Error, 5=Crash)
 *
 * @return 'D', 'I', 'W', 'E', 'C', or '?' for unknown level
 */
static inline char level_to_char(uint8_t level)
{
  switch (level) {
    case 1:
      return 'D';
    case 2:
      return 'I';
    case 3:
      return 'W';
    case 4:
      return 'E';
    case 5:
      return 'C';
    default:
      return '?';
  }
}

/**
 * @brief Format an unsigned 32-bit value into a buffer in the requested base.
 *
 * Supports %u (base 10), %x / %X (base 16), and %o (base 8). The result is
 * left-aligned with optional minimum width using either '0' or ' ' padding.
 *
 * @param[in,out] p          Pointer to the next character position in the buffer.
 * @param[in]     v          Value to format.
 * @param[in]     base       Numeric base; must be one of {8, 10, 16}.
 * @param[in]     min_width  Minimum field width (0 = no padding).
 * @param[in]     zero_pad   If true, left-pad with '0'; otherwise pad with ' '.
 * @param[in]     uppercase  Use uppercase letters for hex digits.
 *
 * @return Pointer to the character past the last digit written.
 */
static char *format_uint(char *p, uint32_t v, uint8_t base, uint8_t min_width, bool zero_pad, bool uppercase)
{
  /* Worst case: base 8 of UINT32_MAX = 11 digits. Use 12 for safety. */
  char tmp[12];
  uint8_t n          = 0;
  const char *digits = uppercase ? hex_chars_upper : hex_chars_lower;

  if (v == 0u) {
    tmp[n++] = '0';
  } else {
    while (v > 0u) {
      tmp[n++] = digits[v % base];
      v /= base;
    }
  }

  /* Pad up to the minimum width. */
  while (n < min_width && n < (uint8_t)sizeof(tmp)) {
    tmp[n++] = zero_pad ? '0' : ' ';
  }

  /* Emit reversed (most-significant digit first). */
  while (n--) {
    *p++ = tmp[n];
  }
  return p;
}

/**
 * @brief Format a signed 32-bit value as decimal.
 *
 * Emits a leading '-' for negative values; non-negative values get no sign.
 * Width and zero-pad apply to the magnitude (the sign is counted toward width
 * the same way the standard library counts it).
 *
 * @param[in,out] p          Pointer to the next character position.
 * @param[in]     v          Signed value.
 * @param[in]     min_width  Minimum field width (sign included).
 * @param[in]     zero_pad   If true, pad with '0' (after the sign); else ' '.
 *
 * @return Pointer past the last character written.
 */
static char *format_sint(char *p, int32_t v, uint8_t min_width, bool zero_pad)
{
  uint32_t uv;

  if (v < 0) {
    *p++ = '-';
    /* Promote to int64_t so that INT32_MIN negation does not overflow. */
    uv = (uint32_t)(-(int64_t)v);
    if (min_width > 0u) {
      min_width--;
    }
  } else {
    uv = (uint32_t)v;
  }
  return format_uint(p, uv, 10u, min_width, zero_pad, false);
}

/**
 * @brief Format a 32-bit IEEE-754 float (passed as bit-pattern) as decimal.
 *
 * Because the log argument array stores 32-bit values, callers that want to
 * print a float must pass the float's IEEE-754 bit-pattern (e.g. via a
 * "union { float f; uint32_t u; }" at the call site). The bit-pattern is
 * reinterpreted as 'float' here.
 *
 * Formatting rules (printf-compatible for the supported cases):
 *  - Negative numbers are prefixed with '-'.
 *  - +/-Inf are emitted as "inf" / "-inf".
 *  - NaN is emitted as "nan".
 *  - Values whose integer part exceeds UINT32_MAX print as "ovf".
 *  - Precision is the number of digits after the decimal point. A precision
 *    of 0 omits the decimal point. Precision is clamped to
 *    SLI_LOG_FMT_MAX_FLOAT_PRECISION.
 *
 * @param[in,out] p          Pointer to the next character position.
 * @param[in]     bits       IEEE-754 bit pattern of the float to format.
 * @param[in]     precision  Digits after the decimal point.
 *
 * @return Pointer past the last character written.
 */
static char *format_float(char *p, uint32_t bits, uint8_t precision)
{
  union {
    uint32_t u;
    float f;
  } cvt;
  cvt.u = bits;

  /* Detect Inf / NaN by IEEE-754 exponent field. */
  uint32_t exp_field = (bits >> 23) & 0xFFu;
  uint32_t mantissa  = bits & 0x7FFFFFu;
  if (exp_field == 0xFFu) {
    const char *s;
    if (mantissa != 0u) {
      s = "nan";
    } else {
      s = (bits & 0x80000000u) ? "-inf" : "inf";
    }
    while (*s) {
      *p++ = *s++;
    }
    return p;
  }

  /*
   * Salvage path for callers that pass a raw float through the log ABI.
   *
   * The log argument array stores 32-bit unsigned values, so a call site of
   * the form
   *     SL_PRINT_STRING_ERROR("v=%.2f", threshold);   // threshold is float
   * silently truncates the float at the call boundary -- the integer part
   * of the float lands in the arg slot, not its IEEE-754 bit pattern. When
   * we reinterpret that small integer as a float here, the result is an
   * IEEE-754 denormal in the ~1e-38 range, which would otherwise print as
   * 0.000000 and obscure the original value.
   *
   * Heuristic: if the bit pattern looks like a denormal (exp_field == 0 and
   * mantissa != 0), treat it as the integer part the caller actually meant
   * and emit "<int>.<precision zeros>". This is intentionally NOT a fully
   * faithful float renderer for the truncated case -- it only recovers the
   * integer component -- but that is more useful than 0.000000.
   *
   * The proper fix at the call site is to bit-cast through a union, e.g.
   *     union { float f; uint32_t u; } v = { .f = threshold };
   *     SL_PRINT_STRING_ERROR("v=%.2f", v.u);
   * but the salvage keeps existing call sites legible until they are
   * updated.
   */
  if (exp_field == 0u && mantissa != 0u) {
    p = format_uint(p, bits, 10u, 0u, false, false);
    if (precision > SLI_LOG_FMT_MAX_FLOAT_PRECISION) {
      precision = SLI_LOG_FMT_MAX_FLOAT_PRECISION;
    }
    if (precision > 0u) {
      *p++ = '.';
      for (uint8_t i = 0u; i < precision; i++) {
        *p++ = '0';
      }
    }
    return p;
  }

  float val = cvt.f;

  if (val < 0.0f) {
    *p++ = '-';
    val  = -val;
  }

  /* The integer part must fit in uint32_t for our cheap formatter. */
  if (val >= 4294967296.0f) {
    const char *s = "ovf";
    while (*s) {
      *p++ = *s++;
    }
    return p;
  }

  uint32_t int_part = (uint32_t)val;
  p                 = format_uint(p, int_part, 10u, 0u, false, false);

  if (precision == 0u) {
    return p;
  }
  if (precision > SLI_LOG_FMT_MAX_FLOAT_PRECISION) {
    precision = SLI_LOG_FMT_MAX_FLOAT_PRECISION;
  }

  *p++       = '.';
  float frac = val - (float)int_part;
  for (uint8_t i = 0u; i < precision; i++) {
    frac *= 10.0f;
    uint32_t d = (uint32_t)frac;
    if (d > 9u) {
      d = 9u;
    }
    *p++ = (char)('0' + d);
    frac -= (float)d;
  }
  return p;
}

/*******************************************************************************
**************************   GLOBAL FUNCTIONS   ********************************
*******************************************************************************/

/**
 * @brief Initialize the logging backend.
 *
 * This prototype is exported to the generic logging code via the
 * sl_log_api_backend structure below. The implementation brings up the
 * platform transport (for example, UART) and prepares it for log output.
 *
 * Synchronization: called from the logger initialization path (single-threaded)
 *
 * @return SL_STATUS_OK on success, an sl_status_t error code otherwise.
 */
sl_status_t sl_log_hal_backend_init(void)
{
  return sl_iostream_set_default(sl_iostream_recommended_console_stream);
}

/**
 * @brief Write a formatted log event to the backend transport.
 *
 * Formatted iostream output is a direct-write path: caller provides a pointer
 * to one event and the backend formats that event into a text line and pushes
 * it immediately to the configured iostream.
 *
 * @param[in] buffer      Pointer to the event to format and send
 * @param[in] read_index  Unused for formatted direct-write path
 * @param[in] event_count Unused for formatted direct-write path (expected 1)
 *
 * @return SL_STATUS_OK on success or an sl_status_t error code from
 *         sl_iostream_write().
 */
sl_status_t sl_log_hal_backend_write(sl_log_event_t *buffer, uint32_t read_index, uint32_t event_count)
{
  /* iostream formatted output path is direct-write only: caller passes a single event (&event). */
  (void)read_index;
  sl_status_t status = SL_STATUS_OK;

  /* event_count is always expected to be 1 for formatted output. */
  if (event_count != 1) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  char line[LINE_MAX];
  char *p = line;

  /* Decode level (bits 1..) and type (bit 0: 0 = string log, 1 = event). */
  uint8_t level = buffer->flags >> 1;
  uint8_t type  = buffer->flags & 0x01;

  /* Build header: [level|type|timestamp] */
  *p++ = '[';
  *p++ = level_to_char(level);
  *p++ = '|';
  *p++ = (type ? 'E' : 'S');
  *p++ = '|';

  /* Always print the timestamp as 8 uppercase hex digits, zero-padded.
   * This is the documented wire format ([LEVEL|TYPE|<timestamp_hex>])
   * and is what existing host-side parsers and repository docs/examples
   * (e.g. [I|S|0494B1BF]) expect; do not change without coordinating an
   * update of every consumer. */
  p    = u32_to_hex8(p, buffer->timestamp);
  *p++ = ']';

  if (type) /* Event type: append event_id, args (n), core_id, version as hex. */
  {
    uint32_t i;

    *p++ = ' ';

    p    = u32_to_hex8(p, buffer->event_id);
    *p++ = '|';

    /* Cap arg_count to the structurally-known maximum. SL_LOG_CONFIG_ARG
     * controls the compile-time size of sl_log_event_t::args[], so a
     * corrupted or otherwise out-of-range arg_count (uint8_t can hold up
     * to 255) must not be used to index past the array. */
    uint32_t event_arg_count = buffer->arg_count;
    if (event_arg_count > (uint32_t)SL_LOG_CONFIG_ARG) {
      event_arg_count = (uint32_t)SL_LOG_CONFIG_ARG;
    }

    /* Append each argument as 8 hex digits; each iteration writes
     * SLI_LOG_FMT_EVENT_ARG_BYTES (= 9) bytes. Reserve room for the
     * trailer (core_id|version) plus the trailing CRLF so the line
     * buffer cannot overflow even in pathological cases. */
    for (i = 0; i < event_arg_count; i++) {
      if ((p + SLI_LOG_FMT_EVENT_ARG_BYTES) > (line + sizeof(line) - SLI_LOG_FMT_EVENT_TRAILER_BYTES)) {
        break;
      }
      p    = u32_to_hex8(p, buffer->args[i]);
      *p++ = '|';
    }

    p    = u8_to_hex2(p, buffer->core_id);
    *p++ = '|';

    p = u8_to_hex2(p, buffer->version);
  } else { /* String log: event_id is format string, expand specifiers with args. */
    const char *fmt    = (const char *)buffer->event_id;
    uint32_t arg_index = 0;

    /* Cap arg_count to the structurally-known maximum, mirroring the
     * event-path clamp. SL_LOG_CONFIG_ARG governs the compile-time size
     * of sl_log_event_t::args[], so a corrupted or otherwise out-of-range
     * arg_count (uint8_t can hold up to 255) must not be used to index
     * past the array via buffer->args[arg_index]. */
    uint32_t string_arg_count = buffer->arg_count;
    if (string_arg_count > (uint32_t)SL_LOG_CONFIG_ARG) {
      string_arg_count = (uint32_t)SL_LOG_CONFIG_ARG;
    }

    *p++ = ' ';

    /*
     * Diagnostics for the "[E|S|00000000]" with empty body symptom.
     *
     * The body is empty when buffer->event_id (the format-string pointer)
     * is either NULL or points to an empty string. That can happen if:
     *   - the caller passed an empty/NULL format string,
     *   - the static log string was placed in a section that got stripped
     *     by the linker (e.g. SL_COMPACT_STRINGS_SECTION ends up in a
     *     discarded output region), or
     *   - the event was lost in a race / never populated.
     *
     * Instead of emitting nothing, surface the offending event_id pointer
     * value so it can be looked up in the .map file.
     */
    if (fmt == NULL) {
      const char *tag = "<null fmt id=0x";
      while (*tag) {
        *p++ = *tag++;
      }
      p    = u32_to_hex8(p, buffer->event_id);
      *p++ = '>';
      goto append_crlf;
    }
    if (*fmt == '\0') {
      const char *tag = "<empty fmt id=0x";
      while (*tag) {
        *p++ = *tag++;
      }
      p                = u32_to_hex8(p, buffer->event_id);
      *p++             = ' ';
      const char *tag2 = "argc=";
      while (*tag2) {
        *p++ = *tag2++;
      }
      p    = format_uint(p, buffer->arg_count, 10u, 0u, false, false);
      *p++ = '>';
      goto append_crlf;
    }

    /*
     * Format-string parser. Each specifier has the (subset of printf) form:
     *
     *   %[flags][width][.precision][length]conversion
     *
     * Supported conversions: d, i, u, x, X, o, p, c, s, f, F, %.
     * Supported flags     : '0' (zero pad). Other flags ('-', '+', ' ', '#')
     *                       are accepted but ignored; padding is left-only.
     * Supported width     : decimal digits (capped to a sensible maximum).
     * Supported precision : decimal digits, used only by %f / %F.
     * Supported length    : h, hh, l, ll, z, t, j -- accepted and ignored
     *                       because every argument is delivered as uint32_t
     *                       through the log event ABI.
     *
     * Backwards-compat note: the legacy implementation always emitted %x as
     * eight uppercase hex digits. To preserve existing log output, when the
     * caller writes "%x" or "%X" with no explicit width and no flags, we
     * default to a min-width of 8 with zero padding. Any explicit width or
     * flag overrides this default.
     */
    while (*fmt && (p < (line + sizeof(line) - SLI_LOG_FMT_LINE_TAIL_GUARD))) {
      if (*fmt != '%') {
        *p++ = *fmt++;
        continue;
      }

      fmt++; /* consume '%' */

      bool zero_pad        = false;
      bool flag_specified  = false;
      uint8_t width        = 0u;
      bool width_specified = false;
      uint8_t precision    = 0u;
      bool precision_set   = false;

      /* Parse flags. Only '0' affects output; the rest are tolerated. */
      bool parsing_flags = true;
      while (parsing_flags) {
        switch (*fmt) {
          case '0':
            zero_pad       = true;
            flag_specified = true;
            fmt++;
            break;
          case '-':
          case '+':
          case ' ':
          case '#':
            flag_specified = true;
            fmt++;
            break;
          default:
            parsing_flags = false;
            break;
        }
      }

      /* Parse width. Accumulate in uint32_t and cap before the (uint8_t)
       * truncation so a 3-digit width (e.g. "%260d") cannot wrap around.
       * The cap of 99 is intentional: format_uint() saturates its output
       * at a 12-char tmp[] buffer, so any larger width has no rendering
       * effect anyway, while still being well clear of the line-buffer
       * tail guard. */
      uint32_t parsed_width = 0u;
      while (*fmt >= '0' && *fmt <= '9') {
        parsed_width = parsed_width * 10u + (uint32_t)(*fmt - '0');
        if (parsed_width > 99u) {
          parsed_width = 99u;
        }
        width_specified = true;
        fmt++;
      }
      width = (uint8_t)parsed_width;

      /* Parse precision. Same overflow-safe accumulation as for width;
       * the consumer (format_float) further clamps to
       * SLI_LOG_FMT_MAX_FLOAT_PRECISION. */
      if (*fmt == '.') {
        fmt++;
        precision_set             = true;
        uint32_t parsed_precision = 0u;
        while (*fmt >= '0' && *fmt <= '9') {
          parsed_precision = parsed_precision * 10u + (uint32_t)(*fmt - '0');
          if (parsed_precision > 99u) {
            parsed_precision = 99u;
          }
          fmt++;
        }
        precision = (uint8_t)parsed_precision;
      }

      /* Consume length modifiers; args are uint32_t so they are no-ops. */
      while (*fmt == 'h' || *fmt == 'l' || *fmt == 'z' || *fmt == 't' || *fmt == 'j') {
        fmt++;
      }

      char conv = *fmt;
      if (conv == '\0') {
        /* Truncated format string -- nothing else to do. */
        break;
      }
      fmt++;

      /* '%%' literal does not consume an argument. */
      if (conv == '%') {
        *p++ = '%';
        continue;
      }

      /* All remaining conversions consume exactly one argument. If the
       * caller supplied fewer args than the format string requires, surface
       * the problem visibly (so callers can spot it in the log) but keep
       * processing the rest of the format string instead of bailing out --
       * otherwise any literal text following the broken specifier would be
       * silently dropped, masking the real call site. The bound is the
       * already-clamped string_arg_count, not buffer->arg_count, so a
       * corrupted value cannot drive an out-of-bounds read on
       * buffer->args[]. */
      if (arg_index >= string_arg_count) {
        *p++ = '<';
        *p++ = '%';
        *p++ = conv;
        *p++ = '?';
        *p++ = '>';
        continue;
      }
      uint32_t arg = buffer->args[arg_index++];

      switch (conv) {
        case 'd':
        case 'i':
          p = format_sint(p, (int32_t)arg, width, zero_pad);
          break;

        case 'u':
          p = format_uint(p, arg, 10u, width, zero_pad, false);
          break;

        case 'x':
          if (!width_specified && !flag_specified) {
            /* Backwards-compatible default: 8-digit zero-padded hex. */
            width    = 8u;
            zero_pad = true;
          }
          p = format_uint(p, arg, 16u, width, zero_pad, false);
          break;

        case 'X':
          if (!width_specified && !flag_specified) {
            width    = 8u;
            zero_pad = true;
          }
          p = format_uint(p, arg, 16u, width, zero_pad, true);
          break;

        case 'o':
          p = format_uint(p, arg, 8u, width, zero_pad, false);
          break;

        case 'p':
          /* Always emit "0x" + 8 uppercase hex digits, regardless of width.
           * Matches the documented contract at the top of this file. */
          *p++ = '0';
          *p++ = 'x';
          p    = format_uint(p, arg, 16u, 8u, true, true);
          break;

        case 'c':
          *p++ = (char)(arg & 0xFFu);
          break;

        case 's': {
          const char *s = (const char *)(uintptr_t)arg;
          if (s != NULL) {
            while (*s && (p < (line + sizeof(line) - SLI_LOG_FMT_LINE_TAIL_GUARD))) {
              *p++ = *s++;
            }
          }
          break;
        }

        case 'f':
        case 'F': {
          uint8_t prec = precision_set ? precision : (uint8_t)SLI_LOG_FMT_DEFAULT_FLOAT_PRECISION;
          p            = format_float(p, arg, prec);
          break;
        }

        default:
          /* Unknown conversion: echo it literally and refund the argument. */
          *p++ = '%';
          *p++ = conv;
          arg_index--;
          break;
      }
    }
  }

append_crlf:
  /* Terminate line with CRLF. Clamp first in case any of the formatting
   * paths above pushed p close to the end of `line`. */
  if (p > (line + sizeof(line) - 2)) {
    p = line + sizeof(line) - 2;
  }
  *p++ = '\r';
  *p++ = '\n';

  status = sl_iostream_write(sl_iostream_recommended_console_stream, line, p - line);
  return status;
}

/**
 * @brief Deinitialize the logging backend.
 *
 * Tear down any resources allocated by sl_log_hal_backend_init(). After this
 * call the backend is considered inactive until reinitialized.
 *
 * @return SL_STATUS_OK on success or an sl_status_t error code.
 */
sl_status_t sl_log_hal_backend_deinit(void)
{
  return SL_STATUS_OK;
}

/**
 * @brief   Logging backend API structure.
 */
sl_log_api_backend_t sl_log_api_backend = { .backend_init   = sl_log_hal_backend_init,
                                            .backend_write  = sl_log_hal_backend_write,
                                            .backend_deinit = sl_log_hal_backend_deinit };

/**
 * @brief Return pointer to the backend API structure.
 *
 * Provides the generic logging core with the platform-specific backend
 * implementation (init/write/deinit). This function returns a pointer to the
 * statically allocated `sl_log_api_backend` structure.
 *
 * @return Pointer to the populated sl_log_api_backend_t structure.
 */
sl_log_api_backend_t *sl_log_get_api_backend(void)
{
  return &sl_log_api_backend;
}
