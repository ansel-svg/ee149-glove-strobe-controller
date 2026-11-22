/*
	This will be used to declare light_frame, the function that turns the mode and hand
	values into DMX channel values for the fixture.

	Input: "mode": enum, "hand_values": float ("pitch", "roll", "flex"), "fade_factor": float

	Output: "dmx_channels": declaration for "light_frame"

	Interfaces with: light_map.c, state.h, led_profile.h
*/
#ifndef LIGHT_MAP_H
#define LIGHT_MAP_H

#include <stdbool.h>
#include <stdint.h>
#include "state.h"

#ifdef __cplusplus
extern "C" {
#endif

// Writes one frame of DMX_CHANNELS values from the mode, hand values and fade factor, using
// the output table and the fixture profile chosen in receiver_config.h. See light_map.c.
void light_frame(Mode mode, const hand_values *hand_values, float fade_factor, uint8_t *dmx_channels);

#ifdef __cplusplus
}
#endif

#endif
