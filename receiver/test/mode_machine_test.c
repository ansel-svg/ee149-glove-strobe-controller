/* Seam: mode machine (state.c). Expected values come from the design doc:
 * smoothing 0.3 per packet; fist closes when every enabled finger >= 0.7 and opens when any enabled
 * finger < 0.3; BOOT held 1.5 s starts calibration; calibration ignores the first 1.0 s then averages
 * 2.0 s; a successful fist step returns to OPEN. */
#include <string.h>
#include <math.h>
#include "tu.h"
#include "receiver_config.h"
#include "state.h"
#include "calibration.h"
#include "nvs.h"
#include "driver/gpio.h"

extern int stub_boot_level;
extern int stub_nvs_has_ver, stub_nvs_has_data, stub_nvs_fail_set;
extern uint8_t stub_nvs_ver, stub_nvs_data[256];
extern size_t stub_nvs_data_len;

/* Calibration used by these tests: open 500, closed 2500 for every finger; pitch zero 10; roll zero 5. */
static void install_calibration(uint8_t mask) {
  CalData c; memset(&c, 0, sizeof c);
  c.pitch_zero = 10.0f; c.roll_zero = 5.0f;
  for (int i = 0; i < 5; i++) { c.flex_open[i] = 500; c.flex_closed[i] = 2500; }
  c.enabled_mask = mask;
  memcpy(stub_nvs_data, &c, sizeof c);
  stub_nvs_data_len = sizeof c;
  stub_nvs_has_data = 1; stub_nvs_has_ver = 1; stub_nvs_ver = 1;
}
static void clear_calibration(void) { stub_nvs_has_data = 0; stub_nvs_has_ver = 0; }

static GlovePacket packet(float pitch, float roll, uint16_t flex, uint32_t seq) {
  GlovePacket g; memset(&g, 0, sizeof g);
  g.magic = 0xEE14; g.version = 1; g.finger_count = 5; g.seq = seq;
  g.pitch_deg = pitch; g.roll_deg = roll;
  for (int i = 0; i < 5; i++) g.flex_raw[i] = flex;
  return g;
}

static uint32_t now = 0;
static void tick(ReceiverState *s, const GlovePacket *p) { now += 10; state_tick(s, p, now); }

/* Presses BOOT until the mode leaves `from`, then releases it. Returns the time it took. */
/* Ticks once and picks the packet by the elapsed time state_tick will see (after this tick). */
static void tick_timed(ReceiverState *s, uint32_t entered, const GlovePacket *settle, const GlovePacket *capture) {
  uint32_t elapsed_after = now + 10 - entered;
  if (elapsed_after < 1000) tick(s, settle);
  else tick(s, capture);
}

static uint32_t press_boot(ReceiverState *s, Mode from) {
  uint32_t start = now;
  stub_boot_level = 0;
  for (int i = 0; i < 1000 && s->mode == from; i++) tick(s, NULL);
  stub_boot_level = 1;
  return now - start;
}

int main(void) {
  ReceiverState s;
  GlovePacket p;

  /* startup */
  clear_calibration();
  calibration_begin();
  state_begin(&s, 0);
  CHECK(s.mode == NO_CAL, "no saved calibration: starts in NO_CAL");
  p = packet(45.0f, 0.0f, 2500, 1);
  tick(&s, &p);
  CHECK(s.mode == NO_CAL, "NO_CAL ignores fist packets");

  install_calibration(0x1F);
  calibration_begin();
  state_begin(&s, now);
  CHECK(s.mode == OPEN, "saved calibration: starts in OPEN");

  /* hand values */
  p = packet(10.0f, 5.0f, 500, 2);
  tick(&s, &p);
  CHECK(fabsf(s.hand.pitch) < 1e-4f && fabsf(s.hand.roll) < 1e-4f, "first packet seeds pitch and roll as offsets from zero (10, 5 -> 0, 0)");
  p = packet(55.0f, 5.0f, 500, 3);
  tick(&s, &p);
  CHECK(fabsf(s.hand.pitch - 13.5f) < 0.01f, "second packet: pitch moves 0.3 of the way to 45 (13.5)");
  tick(&s, NULL);
  CHECK(fabsf(s.hand.pitch - 13.5f) < 0.01f, "no packet: smoothed pitch is held");

  /* fist close: 0.7 of the way from open (500) to closed (2500) is raw 1900 */
  p = packet(10.0f, 5.0f, 2500, 4);
  for (int i = 0; i < 2 && s.mode == OPEN; i++) tick(&s, &p);
  CHECK(s.mode == OPEN, "two closed packets are not enough: still OPEN");
  for (int i = 0; i < 10 && s.mode == OPEN; i++) tick(&s, &p);
  CHECK(s.mode == FIST, "all enabled fingers closed: FIST");

  /* held angles: the value copied on the last OPEN pass (before that pass's packet) */
  state_begin(&s, now);
  p = packet(20.0f, 0.0f, 500, 4);
  for (int i = 0; i < 40; i++) tick(&s, &p);
  CHECK(s.mode == OPEN, "back in OPEN before the held-angle check");
  {
    float prev_pitch = 0.0f;
    int entered_fist = 0;
    p = packet(-40.0f, 0.0f, 2500, 5);   /* the hand moves while it closes */
    for (int i = 0; i < 20 && !entered_fist; i++) {
      prev_pitch = s.hand.pitch;
      tick(&s, &p);
      if (s.mode == FIST) entered_fist = 1;
    }
    CHECK(entered_fist, "closing packets reach FIST from OPEN");
    CHECK(fabsf(s.hand.held_pitch - prev_pitch) < 1e-4f, "held pitch equals the pitch before the pass that closed the fist");
    float held_before = s.hand.held_pitch;
    p = packet(-40.0f, 30.0f, 2500, 6);
    for (int i = 0; i < 5; i++) tick(&s, &p);
    CHECK(fabsf(s.hand.held_pitch - held_before) < 1e-4f, "held pitch does not change while in FIST");
  }

  /* fist open: any enabled finger below 0.3 (raw below 500 + 0.3*2000 = 1100) */
  p = packet(-40.0f, 30.0f, 1000, 6);
  for (int i = 0; i < 10 && s.mode == FIST; i++) tick(&s, &p);
  CHECK(s.mode == OPEN, "fingers opening back below 0.3: OPEN");

  /* 60% closed stays OPEN (below 0.7) */
  state_begin(&s, now);
  p = packet(0.0f, 0.0f, 1700, 7);
  for (int i = 0; i < 30; i++) tick(&s, &p);
  CHECK(s.mode == OPEN, "60% closed (raw 1700) stays OPEN");

  /* only enabled fingers count */
  install_calibration(0x0F);
  calibration_begin();
  state_begin(&s, now);
  p = packet(0.0f, 0.0f, 2500, 8);
  p.flex_raw[4] = 500;   /* pinky stays open but is disabled */
  for (int i = 0; i < 30; i++) tick(&s, &p);
  CHECK(s.mode == FIST, "pinky disabled: closing the other four is enough for FIST");
  CHECK(s.hand.flex_val[4] == 0.0f, "disabled finger reads 0");

  /* calibration entry: BOOT held 1.5 s */
  install_calibration(0x1F);
  calibration_begin();
  state_begin(&s, now);
  uint32_t took = press_boot(&s, OPEN);
  CHECK(s.mode == CAL_FLAT, "BOOT held from OPEN enters CAL_FLAT");
  CHECK(took >= 1500 && took <= 1520, "entry happens after 1.5 s of hold (within one tick)");

  /* capture: settle 1 s ignored, then 2 s averaged */
  GlovePacket junk = packet(90.0f, 90.0f, 0, 20);
  GlovePacket flat = packet(14.0f, 9.0f, 600, 21);
  uint32_t entered = s.entered_ms;
  while (now + 10 - entered < 1000) tick(&s, &junk);   /* last settle tick is elapsed 990 */
  CHECK(s.mode == CAL_FLAT && now - entered == 990, "still in CAL_FLAT at 0.99 s");
  while (now - entered < 3000 + 10) tick_timed(&s, entered, &junk, &flat);
  CHECK(s.mode == CAL_FIST, "after settle + capture (3 s): CAL_FIST");
  CHECK(calibration_get()->pitch_zero == 10.0f, "during CAL_FIST the saved calibration is still the old one (new one saved on the fist step)");

  /* fist capture: closed 2500 on enabled fingers, 500 on pinky (disabled) */
  GlovePacket fist = packet(0.0f, 0.0f, 2500, 22);
  fist.flex_raw[4] = 510;
  entered = s.entered_ms;
  while (s.mode == CAL_FIST && now - entered < 3100) tick_timed(&s, entered, &junk, &fist);
  CHECK(s.mode == OPEN, "fist step done: back to OPEN");
  CHECK(calibration_get()->pitch_zero == 14.0f, "saved pitch zero is the flat-step average (14), not the junk (90)");
  CHECK(calibration_get()->enabled_mask == 0x0F, "pinky (closed 510, open 500) is disabled in the saved mask");
  p = packet(40.0f, 0.0f, 500, 23);
  tick(&s, &p);
  CHECK(fabsf(s.hand.pitch - 26.0f) < 1e-3f, "after success the next packet seeds directly (40 - zero 14 = 26), not smoothed");

  /* failure: no samples keeps old calibration */
  install_calibration(0x1F);
  calibration_begin();
  state_begin(&s, now);
  press_boot(&s, OPEN);
  entered = s.entered_ms;
  while (s.mode == CAL_FLAT && now - entered < 3100) tick(&s, NULL);
  CHECK(s.mode == OPEN && calibration_get()->enabled_mask == 0x1F, "no flat samples: OPEN, old calibration kept");

  /* held angles start at the low end of their ranges (held brightness and hue start at 0) */
  state_begin(&s, now);
  CHECK(s.hand.held_pitch == PITCH_MIN_DEG && s.hand.held_roll == ROLL_MIN_DEG, "state_begin: held angles at the low end of their ranges");
  tick(&s, NULL);
  CHECK(s.hand.held_pitch == PITCH_MIN_DEG, "OPEN with no packet yet: held pitch stays at the low end");

  /* no enabled finger: OPEN never moves to FIST */
  install_calibration(0x00);
  calibration_begin();
  state_begin(&s, now);
  CHECK(s.mode == OPEN, "a saved calibration with mask 0 still starts in OPEN");
  p = packet(0.0f, 0.0f, 2500, 30);
  for (int i = 0; i < 30; i++) tick(&s, &p);
  CHECK(s.mode == OPEN && s.fist_disabled_logged, "mask 0: closed fingers stay OPEN and LOG FIST disabled is marked printed");

  /* failure with no old calibration: back to NO_CAL */
  clear_calibration();
  calibration_begin();
  state_begin(&s, now);
  press_boot(&s, NO_CAL);
  CHECK(s.mode == CAL_FLAT, "BOOT held from NO_CAL enters CAL_FLAT");
  entered = s.entered_ms;
  while (s.mode == CAL_FLAT && now - entered < 3100) tick(&s, NULL);
  CHECK(s.mode == NO_CAL, "no flat samples and no old calibration: NO_CAL");

  TU_RESULT();
}
