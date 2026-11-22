/*
	This will be used to declare test_source_tick, which produces made-up glove packets for
	test mode.

	Input: none

	Output: declaration for "test_source_tick" (GlovePacket)

	Interfaces with: test_source.c, src/glove_packet.h
*/
#ifndef TEST_SOURCE_H
#define TEST_SOURCE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Builds one packet every TEST_PACKET_PERIOD_MS and passes its bytes to link_accept.
void test_source_tick(uint32_t tick_count);

#ifdef __cplusplus
}
#endif

#endif
