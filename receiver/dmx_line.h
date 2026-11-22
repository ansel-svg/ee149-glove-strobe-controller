/*
	The only code that writes DMX: lines on the receiver. Prints the DMX: frame
	that the laptop's pipe program reads.
	Lines end in "\n" only; the sketch never uses Serial.println, which adds "\r".

	Input: "channels": uint8_t[] (DMX channel values 0-255), "count": int

	Output: "DMX:v1,...,vN\n" lines on Serial (115200 baud)

	Interfaces with: dmx_line.c, receiver.ino, receiver_config.h
*/
#ifndef DMX_LINE_H
#define DMX_LINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void dmx_line_print(const uint8_t *channels, int num_channels);

#ifdef __cplusplus
}
#endif

#endif
