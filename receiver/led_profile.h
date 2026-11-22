/*
	This will be used to define the Atomic 3000 LED's 14-channel profile at DMX address 1,
	as static const data: channel numbers, fixed values for channels the glove does not
	drive, and the start address. Fixture data only; the output table is in light_map.c.

	Input: none
	Output: "FixtureProfile" static const tables
	Interfaces with: light_map.c, receiver_config.h (FIXTURE_PROFILE picks one)
*/
#ifndef LED_PROFILE_H
#define LED_PROFILE_H

#include <stdint.h>

// What drives each fixture channel.
//   SLOT_BRIGHTNESS  the aura dimmer. If a profile has one, brightness drives it and R, G, B
//                    carry the hue at full value; with none, R, G, B are scaled by brightness.
//   SLOT_HUE_R/G/B   the aura colour
//   SLOT_STROBE      strobe rate (not light-emitting: clamped to STROBE_MAX, not capped or faded)
//   SLOT_WHITE       the white strobe tubes' intensity
//   SLOT_FIXED       a constant (mode, macro, effect); 0 in the BLACK column
typedef enum { SLOT_FIXED, SLOT_BRIGHTNESS, SLOT_HUE_R, SLOT_HUE_G, SLOT_HUE_B, SLOT_STROBE, SLOT_WHITE } SlotKind;

typedef struct {
  SlotKind kind;
  uint8_t value;   // used by SLOT_FIXED only
} Slot;

typedef struct {
  const char *name;
  uint16_t start;                    // DMX start address (1-based)
  uint8_t count;                     // channels used, starting at start
  uint8_t strobe_off, strobe_min, strobe_max;
  Slot slot[14];                     // slot[i] is DMX channel start + i
} FixtureProfile;

// PROVISIONAL: placeholder layout until the channel table is filled in from the Martin
// manual and confirmed at first light (verification step 3). Every channel number and
// fixed value below is a guess; only the 14-channel count and address 1 are decided.
static const FixtureProfile PROFILE_ATOMIC3000_14CH = {
  "atomic14", 1, 14,
  0, 1, 255,                         // strobe_off, strobe_min, strobe_max (placeholders)
  {
    { SLOT_WHITE, 0 },               // 1  white intensity
    { SLOT_FIXED, 0 },               // 2  strobe duration
    { SLOT_STROBE, 0 },              // 3  strobe rate
    { SLOT_FIXED, 0 },               // 4  strobe effect
    { SLOT_BRIGHTNESS, 0 },          // 5  aura dimmer
    { SLOT_HUE_R, 0 },               // 6  aura red
    { SLOT_HUE_G, 0 },               // 7  aura green
    { SLOT_HUE_B, 0 },               // 8  aura blue
    { SLOT_FIXED, 0 },               // 9
    { SLOT_FIXED, 0 },               // 10
    { SLOT_FIXED, 0 },               // 11
    { SLOT_FIXED, 0 },               // 12
    { SLOT_FIXED, 0 },               // 13
    { SLOT_FIXED, 0 },               // 14
  },
};

#endif
