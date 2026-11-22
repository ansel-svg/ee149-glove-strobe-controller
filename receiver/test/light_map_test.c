/* Seam: output table (light_map.c). Expected values come from the design doc's output table:
 * cap 165 on light-emitting channels, fade multiplies them, strobe clamped to STROBE_MAX,
 * BLACK in NO_CAL / CAL_FLAT / CAL_FIST and at fade 0. Channel numbers come from the
 * provisional profile in led_profile.h. */
#include <string.h>
#include "tu.h"
#include "receiver_config.h"
#include "led_profile.h"
#include "light_map.h"

/* Finds the 0-based frame index of the first channel of a given kind in the selected profile. */
static int ch_of(SlotKind k) {
  const FixtureProfile *p = &FIXTURE_PROFILE;
  for (int i = 0; i < p->count; i++) if (p->slot[i].kind == k) return p->start - 1 + i;
  return -1;
}

static hand_values hv(float pitch, float roll, float hp, float hr) {
  hand_values h; memset(&h, 0, sizeof h);
  h.pitch = pitch; h.roll = roll; h.held_pitch = hp; h.held_roll = hr;
  return h;
}

static int all_zero(const uint8_t *d) { for (int i = 0; i < DMX_CHANNELS; i++) if (d[i]) return 0; return 1; }

int main(void) {
  uint8_t d[DMX_CHANNELS];
  int BR = ch_of(SLOT_BRIGHTNESS), R = ch_of(SLOT_HUE_R), G = ch_of(SLOT_HUE_G), B = ch_of(SLOT_HUE_B);
  int ST = ch_of(SLOT_STROBE), W = ch_of(SLOT_WHITE);
  CHECK(BR >= 0 && R >= 0 && G >= 0 && B >= 0 && ST >= 0 && W >= 0, "profile has every control");
  CHECK(FIXTURE_PROFILE.start == 1 && FIXTURE_PROFILE.count == 14 && DMX_CHANNELS == 14, "14 channels at address 1");

  hand_values h = hv(45.0f, -60.0f, 0.0f, 0.0f);
  light_frame(NO_CAL, &h, 1.0f, d);   CHECK(all_zero(d), "NO_CAL: BLACK");
  light_frame(CAL_FLAT, &h, 1.0f, d); CHECK(all_zero(d), "CAL_FLAT: BLACK");
  light_frame(CAL_FIST, &h, 1.0f, d); CHECK(all_zero(d), "CAL_FIST: BLACK");
  light_frame(OPEN, &h, 0.0f, d);     CHECK(all_zero(d), "fade 0: BLACK");

  /* OPEN: pitch +45 -> full brightness = 165; roll -60 -> hue 0 = red */
  light_frame(OPEN, &h, 1.0f, d);
  CHECK(d[BR] == 165, "OPEN pitch +45: brightness 165 (the cap)");
  CHECK(d[R] == 165 && d[G] == 0 && d[B] == 0, "OPEN roll -60: hue 0 (red at full value, capped)");
  CHECK(d[W] == 0 && d[ST] == FIXTURE_PROFILE.strobe_off, "OPEN: white 0, strobe off");

  h = hv(-45.0f, 0.0f, 0, 0);
  light_frame(OPEN, &h, 1.0f, d);
  CHECK(d[BR] == 0, "OPEN pitch -45: brightness 0");
  CHECK(d[G] == 165 && d[R] == 0 && d[B] == 82, "OPEN roll 0: hue 150 -> green 165, blue half (82)");
  h = hv(0.0f, 60.0f, 0, 0);
  light_frame(OPEN, &h, 1.0f, d);
  CHECK(d[R] == 165 && d[B] == 165 && d[G] == 0, "OPEN roll +60: hue 300 (magenta)");
  CHECK(d[BR] == 82, "OPEN pitch 0: brightness half of 165 (82)");

  /* out-of-range inputs clamp */
  h = hv(200.0f, 500.0f, 0, 0);
  light_frame(OPEN, &h, 1.0f, d);
  CHECK(d[BR] == 165, "pitch beyond range clamps to 165");

  /* fade multiplies light channels */
  h = hv(45.0f, -60.0f, 0, 0);
  light_frame(OPEN, &h, 0.5f, d);
  CHECK(d[BR] == 82 && d[R] == 82, "fade 0.5 halves brightness and colour");

  /* FIST: brightness and hue held; pitch -> strobe; roll -> white */
  h = hv(45.0f, 60.0f, 45.0f, -60.0f);
  light_frame(FIST, &h, 1.0f, d);
  CHECK(d[BR] == 165 && d[R] == 165 && d[G] == 0, "FIST: brightness and hue come from the held angles");
  CHECK(d[W] == 165, "FIST roll +60: white 165");
  CHECK(d[ST] == (STROBE_MAX < FIXTURE_PROFILE.strobe_max ? STROBE_MAX : FIXTURE_PROFILE.strobe_max), "FIST pitch +45: strobe at the clamped maximum");
  h = hv(-45.0f, -60.0f, 45.0f, -60.0f);
  light_frame(FIST, &h, 1.0f, d);
  CHECK(d[ST] == FIXTURE_PROFILE.strobe_min && d[W] == 0, "FIST pitch -45: strobe_min; roll -60: white 0");
  h = hv(45.0f, 60.0f, 45.0f, -60.0f);
  light_frame(FIST, &h, 0.5f, d);
  CHECK(d[W] == 82 && d[ST] <= STROBE_MAX, "fade halves white; strobe is not faded and stays <= STROBE_MAX");

  /* nothing light-emitting ever exceeds the cap, over a sweep */
  int max_light = 0, max_strobe = 0;
  for (int m = 0; m < 2; m++)
    for (float p = -90; p <= 90; p += 3)
      for (float r = -90; r <= 90; r += 3) {
        h = hv(p, r, p, r);
        light_frame(m ? FIST : OPEN, &h, 1.0f, d);
        int l[] = { d[BR], d[R], d[G], d[B], d[W] };
        for (int k = 0; k < 5; k++) if (l[k] > max_light) max_light = l[k];
        if (d[ST] > max_strobe) max_strobe = d[ST];
      }
  CHECK(max_light <= 165, "sweep: no light-emitting channel above 165");
  CHECK(max_strobe <= STROBE_MAX, "sweep: strobe never above STROBE_MAX");

  TU_RESULT();
}
