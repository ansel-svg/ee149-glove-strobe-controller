/*
	This will be used to define the packet the glove sends: GlovePacket, its size check,
	GLOVE_MAGIC, ESPNOW_CHANNEL, and RECEIVER_MAC

	Input: none

	Output: "GlovePacket" struct, "GLOVE_MAGIC", "ESPNOW_CHANNEL", "RECEIVER_MAC"

	Interfaces with: espnow_link.c, test_source.c, state.c, glove/src/glove_packet.h
*/
#ifndef GLOVE_PACKET_H
#define GLOVE_PACKET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Team ID: packets from other ESP-NOW users are dropped. Placeholder until the team picks one.
#define GLOVE_MAGIC 0xEE14
#define GLOVE_VERSION 1
#define GLOVE_FINGER_COUNT 5

// Wi-Fi channel both boards use. Placeholder until the team confirms it.
#define ESPNOW_CHANNEL 1

// Receiver station MAC (unicast target). Locally administered; placeholder until the team picks one.
#define RECEIVER_MAC {0x02, 0x47, 0x4C, 0x56, 0x00, 0x01}

// Packed, little-endian. 27 bytes; ESP-NOW allows up to 250.
typedef struct __attribute__((packed)) {
  uint16_t magic;
  uint8_t version;
  uint8_t finger_count;
  uint32_t seq;
  float pitch_deg;
  float roll_deg;
  uint16_t flex_raw[GLOVE_FINGER_COUNT];
  uint8_t flags;  // bit 0: IMU read failed; other bits sent as 0
} GlovePacket;

// flags bits.
#define GLOVE_FLAG_IMU_FAILED 0x01

// Compile-time size check that works in both C and C++: fails to compile if the size changes.
typedef char glove_packet_size_check[2 * (sizeof(GlovePacket) == 27) - 1];

#ifdef __cplusplus
}
#endif

#endif
