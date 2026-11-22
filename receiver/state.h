/*
	This will be used to declare the receiver's modes (NO_CAL, CAL_FLAT, CAL_FIST, OPEN,
	FIST) and the functions state.c exposes (state_begin, state_tick) along with the
	smoothed hand values.

	Input: none

	Output: declarations for "state_begin", "state_tick", and the "hand_values" struct

	Interfaces with: state.c, light_map.c, calibration.h
*/
#ifndef STATE_H
#define STATE_H

#include <stdbool.h>
#include <stdint.h>
#include "src/glove_packet.h"

#ifdef __cplusplus
extern "C" {
#endif

// NO_CAL, CAL_FLAT and CAL_FIST are calibration modes (tickets 7 and 8); not reached yet.
typedef enum {
NO_CAL,

CAL_FLAT,
CAL_FIST,

OPEN,
FIST } Mode;

typedef struct {
  float pitch;                           // smoothed pitch, degrees
  float roll;                            // smoothed roll, degrees
  float flex_val[GLOVE_FINGER_COUNT];    // smoothed, normalized: 0 open .. 1 closed
  float held_pitch;                      // last OPEN pitch, used in FIST
  float held_roll;                       // last OPEN roll, used in FIST
} hand_values;

typedef struct {
  Mode mode;
  uint32_t entered_ms;
  hand_values hand;
  bool have_hand;                        // false until the first packet seeds the smoothed values
  bool fist_disabled_logged;             // LOG FIST disabled already printed for this calibration
} ReceiverState;

// Starts in OPEN (provisional: the built-in values in receiver_config.h stand in for calibration).
void state_begin(ReceiverState *s, uint32_t now_ms);

// Runs the current mode once. p_or_null is the new packet, or NULL when none arrived this pass.
void state_tick(ReceiverState *s, const GlovePacket *p_or_null, uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif
