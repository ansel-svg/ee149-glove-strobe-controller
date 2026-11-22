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
static _Atomic uint32_t gap_count = 0;

// Last accepted seq. Only link_accept touches these, and it has one caller at a time
// (the Wi-Fi task, or the test source in loop), so they need no locking.
static bool have_last_seq = false;
static uint32_t last_seq = 0;

// Counts missing seq numbers. A seq at or below the last one is a glove restart, not a gap.
static void count_seq(uint32_t seq) {
  if (have_last_seq && seq > last_seq) {
    atomic_fetch_add(&gap_count, seq - last_seq - 1);
  }
  last_seq = seq;
  have_last_seq = true;
}

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
  count_seq(glove_packet.seq);
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

static bool imu_failing = false;

bool packet_take(GlovePacket *glove_packet) {
  if (!packet_fetch(glove_packet)) return false;
  if (glove_packet->flags & GLOVE_FLAG_IMU_FAILED) {
    if (!imu_failing) printf("LOG IMU failed\n");
    imu_failing = true;
    return false;
  }
  imu_failing = false;
  return true;
}

void link_stats_tick(link_stats *stats) {
  if (stats == NULL) return;
  stats->received = atomic_load(&received_count);
  stats->rejected = atomic_load(&rejected_count);
  stats->gaps = atomic_load(&gap_count);
}

static uint32_t last_stats_ms = 0;

void link_stats_log(uint32_t now_ms) {
  if (now_ms - last_stats_ms < LINK_STATS_MS) return;
  last_stats_ms = now_ms;
  link_stats st;
  link_stats_tick(&st);
  if (st.received == 0 && st.rejected == 0) return;   // nothing heard yet: stay quiet
  printf("LOG LINK stats rx=%lu drop=%lu gap=%lu\n",
         (unsigned long)st.received, (unsigned long)st.rejected, (unsigned long)st.gaps);
}

// Link-loss fade. Moves toward 1 while packets arrive, and toward 0 once none has arrived
// for LINK_TIMEOUT_MS, at one rate: FADE_MS to go from 0 to 1. Stays 0 until the first packet.
static float fade_factor = 0.0f;
static uint32_t last_fade_call_ms = 0;
static uint32_t last_valid_ms = 0;
static bool seen_packet = false;
static bool link_up = false;

float link_fade(uint32_t now_ms, bool got_packet) {
  uint32_t elapsed_ms = now_ms - last_fade_call_ms;
  last_fade_call_ms = now_ms;

  if (got_packet) {
    seen_packet = true;
    last_valid_ms = now_ms;
  }
  if (!seen_packet) return 0.0f;

  bool fresh = (now_ms - last_valid_ms) <= LINK_TIMEOUT_MS;
  if (fresh && !link_up) {
    link_up = true;
    printf("LOG LINK ok\n");
  }
  if (!fresh && link_up) {
    link_up = false;
    printf("LOG LINK lost\n");
  }

  float step = (float)elapsed_ms / (float)FADE_MS;
  if (fresh) {
    fade_factor += step;
  } else {
    fade_factor -= step;
  }
  if (fade_factor < 0.0f) fade_factor = 0.0f;
  if (fade_factor > 1.0f) fade_factor = 1.0f;
  return fade_factor;
}
