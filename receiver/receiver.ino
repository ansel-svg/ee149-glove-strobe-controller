/*
	Sketch entry point for the receiver.
  Sends a BLACK DMX: line right after serial starts,
  then the MAC line, then prints one DMX: line every OUTPUT_PERIOD_MS, even with
	no glove connected. The only DMX: writer is dmx_line.c.

	Input: none

	Output: "DMX:" lines and "MAC: <mac>" on Serial (115200 baud)

	Interfaces with: WiFi.h, dmx_line.h, receiver_config.h, espnow_link.h, light_map.h, test_source.h
*/

#include <WiFi.h>
#include <string.h>
#include "receiver_config.h"
#include "dmx_line.h"
#include "espnow_link.h"
#include "light_map.h"
#if RECEIVER_TEST_MODE
#include "test_source.h"
#endif

// black_frame = all 14 channels are 0'd out (or black/off)
static uint8_t black_frame[DMX_CHANNELS] = {0};
static uint32_t last_output_ms = 0;
static GlovePacket last_packet;
static bool have_packet = false;

/* Start serial at 115200 baud; sets up WiFi listener */
void setup() {
  Serial.begin(115200);

  while (!Serial) {
    delay(10);
  }

  dmx_line_print(black_frame, DMX_CHANNELS);
  WiFi.mode(WIFI_STA);
  link_begin(!RECEIVER_TEST_MODE);
  Serial.printf("MAC: %s\n", WiFi.macAddress().c_str());
}

void loop() {
  uint32_t curr_time = millis();

#if RECEIVER_TEST_MODE
  test_source_tick(curr_time);
#endif

  GlovePacket glove_packet;
  if (packet_fetch(&glove_packet)) {
    last_packet = glove_packet;
    have_packet = true;
  }

  if (curr_time - last_output_ms >= OUTPUT_PERIOD_MS) {
    last_output_ms = curr_time;

    uint8_t dmx_channels[DMX_CHANNELS];
    const uint8_t *frame = black_frame;
    if (have_packet) {
      light_frame(last_packet.pitch_deg, dmx_channels);
      frame = dmx_channels;
    }
    dmx_line_print(frame, DMX_CHANNELS);
  }
}
