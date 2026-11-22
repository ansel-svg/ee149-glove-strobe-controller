/*
	This will be used to track the state of the hand itself. More specifically,
	reading our flex sensors, pitch/roll values.

	Input: "flex_val": pure, "pitch", "roll": float

	Output: none

	Interfaces with: none
*/
#include <stdio.h>
#include "state.h"
#include "calibration.h"
#include "receiver_config.h"

static float clamp01(float v) {
  if (v < 0.0f) return 0.0f;
  if (v > 1.0f) return 1.0f;
  return v;
}

// 0 (open) .. 1 (closed) for one finger, using that finger's calibration. Disabled fingers read 0.
static float normalize_flex(const CalData *cal, int finger, uint16_t raw) {
  if ((cal->enabled_mask & (1u << finger)) == 0) return 0.0f;
  float open = (float)cal->flex_open[finger];
  float span = (float)cal->flex_closed[finger] - open;
  if (span == 0.0f) return 0.0f;
  return clamp01(((float)raw - open) / span);
}

// Smooths the new packet into the hand values. The first packet seeds them directly.
static void hand_update(ReceiverState *s, const GlovePacket *p) {
  const CalData *cal = calibration_get();
  if (cal == NULL) return;

  hand_values *h = &s->hand;
  float alpha = 1.0f;
  if (s->have_hand) alpha = HAND_SMOOTH_ALPHA;
  // Pitch and roll are offsets from the flat-hand baseline.
  h->pitch += alpha * ((p->pitch_deg - cal->pitch_zero) - h->pitch);
  h->roll += alpha * ((p->roll_deg - cal->roll_zero) - h->roll);
  for (int i = 0; i < GLOVE_FINGER_COUNT; i++) {
    float target = normalize_flex(cal, i, p->flex_raw[i]);
    h->flex_val[i] += alpha * (target - h->flex_val[i]);
  }
  s->have_hand = true;
}

// True when at least one finger is enabled and every enabled finger is at or above level.
static bool enabled_fingers_at_or_above(const hand_values *h, uint8_t mask, float level) {
  int any_enabled = 0;
  for (int i = 0; i < GLOVE_FINGER_COUNT; i++) {
    if ((mask & (1u << i)) == 0) continue;
    any_enabled = 1;
    if (h->flex_val[i] < level) return false;
  }
  return any_enabled != 0;
}

// True when any enabled finger is below level.
static bool enabled_finger_below(const hand_values *h, uint8_t mask, float level) {
  for (int i = 0; i < GLOVE_FINGER_COUNT; i++) {
    if ((mask & (1u << i)) == 0) continue;
    if (h->flex_val[i] < level) return true;
  }
  return false;
}

static const char *cal_fail_reason(CalResult r) {
  if (r == CAL_NO_SAMPLES) return "no_samples";
  if (r == CAL_NO_FINGER) return "no_finger";
  return "save";
}

static void mode_enter(ReceiverState *s, Mode next, uint32_t now_ms) {
  s->mode = next;
  s->entered_ms = now_ms;
  if (next == NO_CAL) printf("LOG CAL none\n");
  if (next == OPEN) printf("LOG MODE OPEN\n");
  if (next == FIST) printf("LOG MODE FIST\n");
  if (next == CAL_FLAT) {
    cal_capture_clear();
    printf("LOG CAL step=flat\n");
  }
  if (next == CAL_FIST) {
    cal_capture_clear();
    printf("LOG CAL step=fist\n");
  }
}

void state_begin(ReceiverState *s, uint32_t now_ms) {
  s->hand = (hand_values){0};
  // Held angles start at the low end of their ranges, so held brightness and hue start at 0.
  s->hand.held_pitch = PITCH_MIN_DEG;
  s->hand.held_roll = ROLL_MIN_DEG;
  s->have_hand = false;
  s->fist_disabled_logged = false;
  s->entered_ms = now_ms;
  if (calibration_valid()) {
    s->mode = OPEN;
    printf("LOG MODE OPEN\n");
  } else {
    s->mode = NO_CAL;
  }
}

void state_tick(ReceiverState *s, const GlovePacket *p_or_null, uint32_t now_ms) {
  const CalData *cal = calibration_get();
  bool boot_long = cal_button_held(now_ms);
  uint32_t elapsed = now_ms - s->entered_ms;
  uint32_t capture_end = CAL_SETTLE_MS + CAL_CAPTURE_MS;
  Mode next = s->mode;

  switch (s->mode) {
    case NO_CAL:
      if (boot_long) next = CAL_FLAT;
      break;

    case CAL_FLAT:
      if (elapsed >= CAL_SETTLE_MS && elapsed < capture_end) cal_capture_add(p_or_null);
      if (elapsed >= capture_end) {
        if (cal_finish_flat()) {
          next = CAL_FIST;
        } else {
          printf("LOG CAL failed reason=no_samples\n");
          next = NO_CAL;
          if (calibration_valid()) next = OPEN;
        }
      }
      break;

    case CAL_FIST:
      if (elapsed >= CAL_SETTLE_MS && elapsed < capture_end) cal_capture_add(p_or_null);
      if (elapsed >= capture_end) {
        CalResult result = cal_finish_fist();
        if (result == CAL_OK) {
          s->have_hand = false;   // smoothing starts fresh for OPEN
          s->fist_disabled_logged = false;
          next = OPEN;
        } else {
          printf("LOG CAL failed reason=%s\n", cal_fail_reason(result));
          next = NO_CAL;
          if (calibration_valid()) next = OPEN;
        }
      }
      break;

    case OPEN:
      if (cal == NULL) break;
      if (boot_long) {
        next = CAL_FLAT;
        break;
      }
      // Copy before this pass's packet, so the pass where the fingers close does not move the held angles.
      // Before the first packet there is no hand yet, so the held angles keep their starting values.
      if (s->have_hand) {
        s->hand.held_pitch = s->hand.pitch;
        s->hand.held_roll = s->hand.roll;
      }
      if (p_or_null) hand_update(s, p_or_null);
      if ((cal->enabled_mask & ((1u << GLOVE_FINGER_COUNT) - 1)) == 0) {
        if (!s->fist_disabled_logged) printf("LOG FIST disabled\n");
        s->fist_disabled_logged = true;
        break;
      }
      if (enabled_fingers_at_or_above(&s->hand, cal->enabled_mask, FIST_CLOSE_LEVEL)) next = FIST;
      break;

    case FIST:
      if (cal == NULL) break;
      if (boot_long) {
        next = CAL_FLAT;
        break;
      }
      if (p_or_null) hand_update(s, p_or_null);
      if (enabled_finger_below(&s->hand, cal->enabled_mask, FIST_OPEN_LEVEL)) next = OPEN;
      break;
  }
  if (next != s->mode) mode_enter(s, next, now_ms);
}
