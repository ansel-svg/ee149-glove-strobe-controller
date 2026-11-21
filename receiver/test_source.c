/*
	These will be fake ESP-NOW packets for test mode: sweep pitch and roll,
	toggle the fist, and select between TEST_SCENARIO cases. Compiled only when
	RECEIVER_TEST_MODE is set to 1. Primarily used to just see if the code works.


	Input: "tick_count": uint32_t, "test_scenario": int

	Output: "raw_packet": raw glove packet bytes to espnow_link.c via link_accept

	Interfaces with: espnow_link.c, receiver_config.h, src/glove_packet.h
*/
