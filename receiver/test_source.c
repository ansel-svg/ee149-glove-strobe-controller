/*
These will be fake ESP-NOW packets for test mode: sweep pitch and roll,
toggle the fist, and select between TEST_SCENARIO cases. Compiled only when
RECEIVER_TEST_MODE is set to 1. Primarily used to just see if the code works.


	Input: "tick_count": uint32_t, "test_scenario": int

	Output: "raw_packet": raw glove packet bytes to espnow_link.c via link_accept

	Interfaces with: espnow_link.c, receiver_config.h, src/glove_packet.h
*/
#include "receiver_config.h"

#if RECEIVER_TEST_MODE

#include <string.h>
#include "test_source.h"
#include "espnow_link.h"

static uint32_t next_packet_ms = 0;
static uint32_t next_seq = 0;

void test_source_tick(uint32_t tick_count) {
  if (tick_count - next_packet_ms < TEST_PACKET_PERIOD_MS) return;
  next_packet_ms = tick_count;

  GlovePacket glove_packet;
  memset(&glove_packet, 0, sizeof(glove_packet));
  glove_packet.magic = GLOVE_MAGIC;
  glove_packet.version = GLOVE_VERSION;
  glove_packet.finger_count = GLOVE_FINGER_COUNT;
  glove_packet.seq = next_seq++;

  // Pitch sweeps -45..+45 degrees and back over 4 s; roll sweeps -60..+60 over 6 s.
  uint32_t pitch_phase = tick_count % 4000;
  if (pitch_phase >= 2000) pitch_phase = 4000 - pitch_phase;
  float pitch_t = pitch_phase / 2000.0f;
  glove_packet.pitch_deg = -45.0f + 90.0f * pitch_t;

  uint32_t roll_phase = tick_count % 6000;
  if (roll_phase >= 3000) roll_phase = 6000 - roll_phase;
  float roll_t = roll_phase / 3000.0f;
  glove_packet.roll_deg = -60.0f + 120.0f * roll_t;

  // Fist toggles every 4 s: fingers open for 4 s, then closed for 4 s, starting open.
  uint16_t flex_raw = TEST_CAL_FLEX_OPEN;

  if ((tick_count / 4000) % 2 == 1) {
  	flex_raw = TEST_CAL_FLEX_CLOSED;
  }

  for (int i = 0; i < GLOVE_FINGER_COUNT; i++) {
  	glove_packet.flex_raw[i] = flex_raw;
  }

  uint8_t raw_packet[sizeof(GlovePacket)];
  memcpy(raw_packet, &glove_packet, sizeof(glove_packet));
  link_accept(raw_packet, sizeof(raw_packet));
}

#endif
