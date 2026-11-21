/*
	This will be used to declare light_frame, the function that turns the mode and hand
	values into DMX channel values for the fixture.

	Input: "mode": enum, "hand_values": float ("pitch", "roll", "flex"), "fade_factor": float

	Output: "dmx_channels": declaration for "light_frame"

	Interfaces with: light_map.c, state.h, led_profile.h
*/
