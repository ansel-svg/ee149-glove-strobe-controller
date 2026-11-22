/*

  Literally just code that allows us to see what the DMX will print out.
  I'm pretty sure I can abstract this away somewhere else...lmao its fine.

	Input: "channels": uint8_t[] {range of 0-255, DMX values}, "count": int
	Output: "DMX:v1,...,vN\n" lines on Serial (115200 baud)

	Interfaces with: dmx_line.h, receiver.ino, receiver_config.h
*/

#include <stdio.h>
#include "dmx_line.h"

void dmx_line_print(const uint8_t *channels, int num_channels) {
  printf("DMX:");
  for (int i = 0; i < num_channels; i++) {
    if (i > 0) printf(",");
    printf("%u", (unsigned)channels[i]);
  }
  printf("\n");
}
