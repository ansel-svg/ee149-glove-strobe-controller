/*
	This will be used to map the IMU and flex sensor values to what gets outputted
	by our lighting fixture.

	Example: closed fist, up-most hand => strobe strip, highest strobe speed

	Input: "fist": bool, "pitch", "roll": float

	Output: "fist": bool, "ptich_val", "roll_val": float

	Interfaces with: state.c,
*/
#include "light_map.h"
#include "receiver_config.h"

// Brightness cap from the 65% photosensitivity limit (floor(255 * 0.65)).
#define BRIGHTNESS_CAP 165

void light_frame(float pitch, uint8_t *dmx_channels) {
  for (int i = 0; i < DMX_CHANNELS; i++) dmx_channels[i] = 0;

  float t = (pitch + 45.0f) / 90.0f;
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  dmx_channels[0] = (uint8_t)(t * BRIGHTNESS_CAP);
}
