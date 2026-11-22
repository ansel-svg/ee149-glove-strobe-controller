/*
	This will be used to start Wi-Fi and ESP-NOW. 
	1. Receive glove packets in the radio callback
	2. Check correctness for each packet
	3. Hold the latest one in a one-slot queue,
	4. Compute the link-loss fade
	5. Count link statistics.

	Input: "raw_packet": uint8_t[], "length": int

	Output: "glove_packet": validated GlovePacket to state.c via packet_fetch,
	"fade_factor": float,
	"link_stats"

	Interfaces with: espnow_link.h, state.c, light_map.c, src/glove_packet.h, receiver_config.h
*/

#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "espnow_link.h"
#include "receiver_config.h"

static QueueHandle_t latest_slot = NULL;

// Atomicity required for the fact that something could be either received OR rejected; can only ever be one or other.
static _Atomic uint32_t received_count = 0;
static _Atomic uint32_t rejected_count = 0;

void link_accept(const uint8_t *raw_packet, int length) {
  GlovePacket glove_packet;
  if (raw_packet == NULL || length != (int)sizeof(GlovePacket)) {
    atomic_fetch_add(&rejected_count, 1);
    return;
  }
  memcpy(&glove_packet, raw_packet, sizeof(GlovePacket));
  if (glove_packet.magic != GLOVE_MAGIC || glove_packet.version != GLOVE_VERSION) {
    atomic_fetch_add(&rejected_count, 1);
    return;
  }
  atomic_fetch_add(&received_count, 1);
  if (latest_slot != NULL) {
    xQueueOverwrite(latest_slot, &glove_packet);
  }
}

static void on_espnow_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  (void)info;
  link_accept(data, len);
}

void link_begin(bool radio) {
  latest_slot = xQueueCreate(1, sizeof(GlovePacket));
  if (latest_slot == NULL) {
    printf("LOG RADIO queue create failed\n");
    return;
  }
  if (!radio) return;

  esp_err_t err = esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (err != ESP_OK) printf("LOG RADIO channel err=%d\n", (int)err);

  // Unicast needs the receiver's own MAC to be RECEIVER_MAC. Call order not yet checked on the board.
  static const uint8_t receiver_mac[6] = RECEIVER_MAC;
  err = esp_wifi_set_mac(WIFI_IF_STA, receiver_mac);
  if (err != ESP_OK) printf("LOG RADIO set mac err=%d\n", (int)err);

  err = esp_now_init();
  if (err != ESP_OK) {
    printf("LOG RADIO esp_now_init err=%d\n", (int)err);
    return;
  }
  err = esp_now_register_recv_cb(on_espnow_recv);
  if (err != ESP_OK) printf("LOG RADIO recv cb err=%d\n", (int)err);
}

bool packet_fetch(GlovePacket *glove_packet) {
  if (latest_slot == NULL || glove_packet == NULL) return false;
  return xQueueReceive(latest_slot, glove_packet, 0) == pdTRUE;
}

void link_stats_tick(link_stats *stats) {
  if (stats == NULL) return;
  stats->received = atomic_load(&received_count);
  stats->rejected = atomic_load(&rejected_count);
}
