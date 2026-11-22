/*
	This will be used to hold every tunable constant for the receiver: test-mode switch,
	output period, link-loss timeout, smoothing, fist thresholds, calibration times,
	angle ranges, the 65% light cap, STROBE_MAX, and the fixture choice.

	Input: none
	Output: none (constants only)
	Interfaces with: state.c, calibration.c, light_map.c, espnow_link.c, test_source.c
*/
#ifndef RECEIVER_CONFIG_H
#define RECEIVER_CONFIG_H

// 1 = build made-up glove packets (test_source.c) instead of listening for ESP-NOW.
#define RECEIVER_TEST_MODE 0

// Atomic 3000 profile: 14 DMX channels starting at address 1.
#define DMX_CHANNELS 14

// One DMX: line every OUTPUT_PERIOD_MS (50 ms = 20 lines per second, above the 10/s minimum).
#define OUTPUT_PERIOD_MS 50

// Test mode: one made-up packet every TEST_PACKET_PERIOD_MS.
#define TEST_PACKET_PERIOD_MS 20

#endif
