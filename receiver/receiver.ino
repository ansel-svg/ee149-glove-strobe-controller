/*
	Sketch entry point for the receiver.
  Sends a BLACK DMX: line right after serial starts,
  then the MAC line, then prints one DMX: line every OUTPUT_PERIOD_MS, even with
	no glove connected. The only DMX: writer is dmx_line.c.

	Input: none

	Output: "DMX:" lines and "MAC: <mac>" on Serial (115200 baud)

	Interfaces with: WiFi.h, dmx_line.h, receiver_config.h, espnow_link.h, state.h, light_map.h, test_source.h
*/

#include <WiFi.h>
#include <string.h>
#include "driver/uart_vfs.h"
#include "esp_wifi.h"
#include "receiver_config.h"
#include "dmx_line.h"
#include "espnow_link.h"
#include "calibration.h"
#include "state.h"
#include "light_map.h"
#if RECEIVER_TEST_MODE
#include "test_source.h"
#endif

// black_frame = all 14 channels are 0'd out (or black/off)
static uint8_t black_frame[DMX_CHANNELS] = {0};
static uint32_t last_output_ms = 0;
static ReceiverState receiver_state;

/* Start serial at 115200 baud; sets up WiFi listener */
void setup() {
  Serial.begin(115200);

  while (!Serial) {
    delay(10);
  }
  // printf goes through the ESP-IDF console, which turns "\n" into "\r\n" by default.
  // D4 needs "\n" only, so send line endings unchanged.
  uart_vfs_dev_port_set_tx_line_endings(0, ESP_LINE_ENDINGS_LF);

  // The UART can emit a stray byte at startup. A bare "\n" first puts it on its own
  // line, so the BLACK line below starts cleanly with "DMX:".
  printf("\n");
  dmx_line_print(black_frame, DMX_CHANNELS);
  WiFi.mode(WIFI_STA);
  calibration_begin();
  link_begin(!RECEIVER_TEST_MODE);
  state_begin(&receiver_state, millis());
  // Read the station MAC from the Wi-Fi driver (what ESP-NOW uses). WiFi.macAddress()
  // returns the network layer's copy, which does not follow esp_wifi_set_mac.
  uint8_t mac[6] = {0};
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void loop() {
  uint32_t curr_time = millis();

#if RECEIVER_TEST_MODE
  test_source_tick(curr_time);
#endif

  // State and fade run every pass, so they keep moving even when no packet arrives.
  // packet_take drops IMU-failed packets, so they neither drive the light nor keep the fade up.
  GlovePacket glove_packet;
  bool got_packet = packet_take(&glove_packet);
  GlovePacket *new_packet = NULL;
  if (got_packet) new_packet = &glove_packet;
  state_tick(&receiver_state, new_packet, curr_time);
  float fade_factor = link_fade(curr_time, got_packet);
  link_stats_log(curr_time);

  if (curr_time - last_output_ms >= OUTPUT_PERIOD_MS) {
    last_output_ms = curr_time;

    uint8_t dmx_channels[DMX_CHANNELS];
    light_frame(receiver_state.mode, &receiver_state.hand, fade_factor, dmx_channels);
    dmx_line_print(dmx_channels, DMX_CHANNELS);
  }
}
