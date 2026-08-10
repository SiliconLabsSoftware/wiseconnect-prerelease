#ifndef SL_TYPES_H
#define SL_TYPES_H

#include "sl_slist.h"
#include "sli_constants.h"

/** @addtogroup SL_WIFI_TYPES Types
  * @{ */

/**
 * @struct sl_wifi_buffer_t
 * @brief Structure representing a Wi-Fi buffer.
 */
typedef struct {
  sl_slist_node_t node; ///< Pointer to the node of the list of which the buffer is part of
  uint32_t length;      ///< Size of the buffer in bytes
  uint8_t
    type; ///< Indicates the buffer type (SL_WIFI_TX_FRAME_BUFFER, SL_WIFI_RX_FRAME_BUFFER, and so on.) corresponding to the buffer.
  uint8_t id;           ///< Buffer identifier. Can be used to uniquely identify a buffer. Loops every 256 packets.
  uint8_t _reserved[2]; ///< Reserved.
  uint8_t data[];       ///< Stores the data (header + payload) to be send to NWP
} sl_wifi_buffer_t;

/** @} */

// driver TX/RX packet structure
/// Wi-Fi packet structure
typedef struct {
  union {
    struct {
      uint16_t length;  ///< Length of data
      uint16_t command; ///< command type
      uint8_t unused
        [12]; ///< Contains command status and other additional information. Unused for TX and only used for RX packets.
    };
    uint8_t desc[SL_SI91X_WIFI_PACKET_DESC_SIZE]; ///< packet header
  };                                              ///< Command header

  uint8_t data[]; ///< Data to be transmitted or received
} sl_wifi_system_packet_t;

#endif // SL_TYPES_H
