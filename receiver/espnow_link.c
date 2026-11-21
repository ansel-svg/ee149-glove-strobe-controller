/*
	This will be used to start Wi-Fi and ESP-NOW, receive glove packets in the radio
	callback, check each packet, hold the latest one in a one-slot queue, compute the
	link-loss fade, and count link statistics.

	Input: "raw_packet": uint8_t[], "length": int

	Output: "glove_packet": validated GlovePacket to state.c via packet_fetch,
	"fade_factor": float,
	"link_stats"

	Interfaces with: espnow_link.h, state.c, light_map.c, src/glove_packet.h, receiver_config.h
*/
