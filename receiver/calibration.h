/*
	This will be used to declare the calibration routines: BOOT long-press detection,
	capture averaging, finger checks, and storage load/save.

	Input: none

	Output: declarations for the calibration functions and the calibration data type

	Interfaces with: calibration.c, state.c, receiver_config.h
*/
#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <stdbool.h>
#include <stdint.h>
#include "src/glove_packet.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  float pitch_zero;                         // flat-hand pitch, degrees
  float roll_zero;                          // flat-hand roll, degrees
  uint16_t flex_open[GLOVE_FINGER_COUNT];   // raw ADC reading, finger open
  uint16_t flex_closed[GLOVE_FINGER_COUNT]; // raw ADC reading, finger closed
  uint8_t enabled_mask;                     // bit i set: finger i counts toward the fist
} CalData;

typedef enum { CAL_OK, CAL_NO_SAMPLES, CAL_NO_FINGER, CAL_SAVE_FAILED } CalResult;

// Test mode: built-in values from receiver_config.h. Otherwise: loads from NVS, if a valid
// calibration is saved. Prints LOG CAL loaded enabled=<mask> or LOG CAL none.
void calibration_begin(void);

bool calibration_valid(void);

// NULL when there is no calibration.
const CalData *calibration_get(void);

// True once BOOT has been held for CAL_HOLD_MS. Fires once per press; false in test mode.
bool cal_button_held(uint32_t now_ms);

// Capture sums for one step. Clear at the start of each step; add one packet per pass.
void cal_capture_clear(void);
void cal_capture_add(const GlovePacket *p);

// Flat-hand step: stores pitch and roll zero and each finger's open reading. False if no samples.
bool cal_finish_flat(void);

// Fist step: stores each finger's closed reading, checks the fingers, and saves to NVS.
// On any failure the old calibration stays.
CalResult cal_finish_fist(void);

#ifdef __cplusplus
}
#endif

#endif
