#ifndef GLOVE_PACKET_H
#define GLOVE_PACKET_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
  float pitch;
  float roll;
  uint16_t flex_val;
  uint32_t seq; // for checking dropped packets for watchdog timer
} glove_packet_t;

#endif