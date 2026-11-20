#ifndef RADIO_H
#define RADIO_H

#include <stdint.h>
#include "glove_packet.h"

void init_esp_now(void);
void send_glove_packet(float pitch, float roll, uint16_t flex_val);

#endif