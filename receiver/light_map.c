/*
	This will be used to map the IMU and flex sensor values to what gets outputted
	by our lighting fixture. It is the only code that writes DMX channel values.

	Example: closed fist, up-most hand => strobe strip, highest strobe speed

	Output table (each cell: linear map from its input range, then clamp):
	  control      OPEN                       FIST                          BLACK
	  brightness   pitch -> 0..1              held (last OPEN pitch)        0
	  hue          roll -> 0..300 deg (HSV)   held (last OPEN roll)         0
	  strobe rate  strobe_off                 pitch -> strobe_min..max      strobe_off
	  white        0                          roll -> 0..1                  0
	  fixed        profile value              profile value                 0
	Light-emitting channels (brightness, R, G, B, white) are capped at BRIGHTNESS_CAP_DMX
	and multiplied by the fade factor. Strobe is clamped to STROBE_MAX in every mode.
	BLACK is used in NO_CAL, CAL_FLAT and CAL_FIST, and whenever the fade factor is 0.

	Input: "mode": enum, "hand_values": pitch, roll, held angles, "fade_factor": float

	Output: "dmx_channels": uint8_t[DMX_CHANNELS], channel 1 first

	Interfaces with: state.h, led_profile.h, receiver_config.h
*/
#include "light_map.h"
#include "led_profile.h"
#include "receiver_config.h"

static float unit_from_range(float value, float lo, float hi) {
  float t = (value - lo) / (hi - lo);
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  return t;
}

// A light-emitting channel: 0..1 -> 0..BRIGHTNESS_CAP_DMX, times the fade. Rounds down, so it
// never goes over the cap.
static uint8_t light(float unit, float fade_factor) {
  if (unit < 0.0f) unit = 0.0f;
  if (unit > 1.0f) unit = 1.0f;
  return (uint8_t)(unit * BRIGHTNESS_CAP_DMX * fade_factor);
}

// HSV with full saturation and value: hue in degrees -> r, g, b in 0..1.
static void hue_to_rgb(float hue_deg, float *r, float *g, float *b) {
  float h = hue_deg / 60.0f;
  int sector = (int)h;
  float f = h - (float)sector;
  float up = f, down = 1.0f - f;
  switch (sector) {
    case 0:  *r = 1;    *g = up;   *b = 0;    break;
    case 1:  *r = down; *g = 1;    *b = 0;    break;
    case 2:  *r = 0;    *g = 1;    *b = up;   break;
    case 3:  *r = 0;    *g = down; *b = 1;    break;
    case 4:  *r = up;   *g = 0;    *b = 1;    break;
    default: *r = 1;    *g = 0;    *b = down; break;   // 300..360, and exactly 360
  }
}

// Strobe limits after the STROBE_MAX clamp (ints, so a STROBE_MAX of 255 compares cleanly).
static uint8_t strobe_value(const FixtureProfile *prof, float unit) {
  int hi = prof->strobe_max < STROBE_MAX ? prof->strobe_max : STROBE_MAX;
  int lo = prof->strobe_min < hi ? prof->strobe_min : hi;
  return (uint8_t)(lo + unit * (float)(hi - lo));
}

static uint8_t strobe_off(const FixtureProfile *prof) {
  int off = prof->strobe_off < STROBE_MAX ? prof->strobe_off : STROBE_MAX;
  return (uint8_t)off;
}

void light_frame(Mode mode, const hand_values *hand_values, float fade_factor, uint8_t *dmx_channels) {
  const FixtureProfile *prof = &FIXTURE_PROFILE;
  for (int i = 0; i < DMX_CHANNELS; i++) dmx_channels[i] = 0;
  if (fade_factor < 0.0f) fade_factor = 0.0f;
  if (fade_factor > 1.0f) fade_factor = 1.0f;

  bool black = (mode != OPEN && mode != FIST) || fade_factor == 0.0f;

  // Control values for this frame, each 0..1 (strobe_unit < 0 means strobe_off).
  float bright = 0.0f, hue_deg = 0.0f, white = 0.0f, strobe_unit = -1.0f;
  if (!black && mode == OPEN) {
    bright = unit_from_range(hand_values->pitch, PITCH_MIN_DEG, PITCH_MAX_DEG);
    hue_deg = 300.0f * unit_from_range(hand_values->roll, ROLL_MIN_DEG, ROLL_MAX_DEG);
  } else if (!black && mode == FIST) {
    bright = unit_from_range(hand_values->held_pitch, PITCH_MIN_DEG, PITCH_MAX_DEG);
    hue_deg = 300.0f * unit_from_range(hand_values->held_roll, ROLL_MIN_DEG, ROLL_MAX_DEG);
    strobe_unit = unit_from_range(hand_values->pitch, PITCH_MIN_DEG, PITCH_MAX_DEG);
    white = unit_from_range(hand_values->roll, ROLL_MIN_DEG, ROLL_MAX_DEG);
  }

  float r = 0.0f, g = 0.0f, b = 0.0f;
  if (!black) hue_to_rgb(hue_deg, &r, &g, &b);

  // With an aura dimmer, brightness drives it and R, G, B stay at full value; without one,
  // R, G, B are scaled by brightness.
  bool has_dimmer = false;
  for (int i = 0; i < prof->count; i++) {
    if (prof->slot[i].kind == SLOT_BRIGHTNESS) has_dimmer = true;
  }
  float rgb_scale = has_dimmer ? 1.0f : bright;

  for (int i = 0; i < prof->count; i++) {
    int ch = (int)prof->start - 1 + i;   // 0-based index into the DMX frame
    if (ch < 0 || ch >= DMX_CHANNELS) continue;
    uint8_t v = 0;
    switch (prof->slot[i].kind) {
      case SLOT_BRIGHTNESS: v = black ? 0 : light(bright, fade_factor); break;
      case SLOT_HUE_R:      v = black ? 0 : light(r * rgb_scale, fade_factor); break;
      case SLOT_HUE_G:      v = black ? 0 : light(g * rgb_scale, fade_factor); break;
      case SLOT_HUE_B:      v = black ? 0 : light(b * rgb_scale, fade_factor); break;
      case SLOT_WHITE:      v = black ? 0 : light(white, fade_factor); break;
      case SLOT_STROBE:     v = strobe_unit < 0.0f ? strobe_off(prof) : strobe_value(prof, strobe_unit); break;
      case SLOT_FIXED:      v = black ? 0 : prof->slot[i].value; break;
    }
    dmx_channels[ch] = v;
  }
}
