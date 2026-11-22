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

// Link loss: no valid packet for LINK_TIMEOUT_MS starts the fade to black.
// FADE_MS is the time to go from 0 to 1 (or 1 to 0). Black within LINK_TIMEOUT_MS + FADE_MS = 1.5 s.
#define LINK_TIMEOUT_MS 500
#define FADE_MS 1000

// Test mode's built-in calibration (used instead of NVS). Matches the test source's flex values.
#define TEST_CAL_FLEX_OPEN 1000
#define TEST_CAL_FLEX_CLOSED 3000
#define TEST_CAL_PITCH_ZERO 0.0f
#define TEST_CAL_ROLL_ZERO 0.0f
#define TEST_CAL_ENABLED_MASK 0x1F

// Calibration: BOOT (GPIO0, active low) held for CAL_HOLD_MS starts a capture. Each capture step
// ignores packets for CAL_SETTLE_MS, then averages for CAL_CAPTURE_MS.
#define CAL_BOOT_GPIO 0
#define CAL_HOLD_MS 1500
#define CAL_SETTLE_MS 1000
#define CAL_CAPTURE_MS 2000
// Raw ADC counts. A finger whose open and closed readings differ by less is disabled.
#define CAL_MIN_SPAN 300

// Smoothing: each new packet moves a smoothed value this fraction of the way to the raw value.
#define HAND_SMOOTH_ALPHA 0.3f

// Fist hysteresis on normalized flex (0 open .. 1 closed): OPEN becomes FIST when every finger
// is at or above FIST_CLOSE_LEVEL; FIST becomes OPEN when any finger drops below FIST_OPEN_LEVEL.
#define FIST_CLOSE_LEVEL 0.7f
#define FIST_OPEN_LEVEL 0.3f

// LOG LINK stats rx=<n> drop=<n> gap=<n> every LINK_STATS_MS (only once something has arrived).
#define LINK_STATS_MS 10000

// Output table input ranges, in degrees from the flat-hand baseline. Starting values.
#define PITCH_MIN_DEG -45.0f
#define PITCH_MAX_DEG 45.0f
#define ROLL_MIN_DEG -60.0f
#define ROLL_MAX_DEG 60.0f

// Cap on every light-emitting channel: floor(255 * 0.65), for photosensitivity.
#define BRIGHTNESS_CAP_DMX 165

// Highest strobe-rate DMX value the glove may send, in every mode. Low in test mode.
// TBD (Ansel): the live value. 255 is the fixture's full range ("no strobe-rate cap").
#if RECEIVER_TEST_MODE
#define STROBE_MAX 40
#else
#define STROBE_MAX 255
#endif

// Fixture profile, from led_profile.h.
#define FIXTURE_PROFILE PROFILE_ATOMIC3000_14CH

#endif
