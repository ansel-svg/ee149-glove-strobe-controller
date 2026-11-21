/*
	This will be used to declare the receiver's modes (NO_CAL, CAL_FLAT, CAL_FIST, OPEN,
	FIST) and the functions state.c exposes (state_begin, state_tick) along with the
	smoothed hand values.

	Input: none

	Output: declarations for "state_begin", "state_tick", and the "hand_values" struct

	Interfaces with: state.c, light_map.c, calibration.h
*/
