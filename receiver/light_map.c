/*
	This will be used to map the IMU and flex sensor values to what gets outputted
	by our lighting fixture.

	Example: closed fist, up-most hand => strobe strip, highest strobe speed

	Input: "fist": bool, "pitch", "roll": float

	Output: "fist": bool, "ptich_val", "roll_val": float

	Interfaces with: state.c,
*/