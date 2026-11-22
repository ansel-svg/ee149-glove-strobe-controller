#ifndef STUB_QUEUE_H
#define STUB_QUEUE_H
#include <stddef.h>
#include <stdint.h>
typedef struct stub_queue *QueueHandle_t;
QueueHandle_t xQueueCreate(unsigned len, size_t item_size);
int xQueueOverwrite(QueueHandle_t q, const void *item);
int xQueueReceive(QueueHandle_t q, void *out, uint32_t ticks);
#endif
