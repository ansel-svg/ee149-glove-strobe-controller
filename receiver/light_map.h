/*
	This will be used to declare light_frame, the function that turns the mode and hand
	values into DMX channel values for the fixture.

	Input: "mode": enum, "hand_values": float ("pitch", "roll", "flex"), "fade_factor": float

	Output: "dmx_channels": declaration for "light_frame"

	Interfaces with: light_map.c, state.h, led_profile.h
*/
#ifndef LIGHT_MAP_H
#define LIGHT_MAP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Placeholder for ticket 02: open-hand brightness only, on DMX channel 1.
// pitch -45..+45 maps to 0..165; all other channels are 0. Ticket 06 replaces it with the
// full output table, which also takes "mode" and "fade_factor".
void light_frame(float pitch, uint8_t *dmx_channels);

#ifdef __cplusplus
}
#endif

#endif
