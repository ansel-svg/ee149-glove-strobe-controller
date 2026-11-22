/*
	This will be used to declare the ESP-NOW link functions: radio start, the receive
	callback entry point, link_accept, packet_fetch, link_fade, and link_stats_tick.

	Input: none

	Output: declarations for "link_accept", "packet_fetch", "link_fade",
	"link_stats_tick", and "link_stats" struct

	Interfaces with: espnow_link.c, state.c, src/glove_packet.h
*/
#ifndef ESPNOW_LINK_H
#define ESPNOW_LINK_H

#include <stdbool.h>
#include <stdint.h>
#include "src/glove_packet.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint32_t received;  // packets that passed the checks and went into the slot
  uint32_t rejected;  // packets dropped for wrong length, magic or version
} link_stats;

// Creates the one-slot queue. Starts ESP-NOW only when radio is true (not in test mode).
void link_begin(bool radio);

// Checks one packet (length, magic, version), counts it, and overwrites the one-slot queue.
// Called from the ESP-NOW callback and from the test source.
void link_accept(const uint8_t *raw_packet, int length);

// Takes the newest accepted packet, if one arrived since the last call.
bool packet_fetch(GlovePacket *glove_packet);

// Copies the counters into link_stats.
void link_stats_tick(link_stats *stats);

#ifdef __cplusplus
}
#endif

#endif
