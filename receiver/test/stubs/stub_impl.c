#include <stdlib.h>
#include <string.h>
#include "esp_now.h"
#include "freertos/queue.h"
uint8_t stub_last_mac[6];
uint8_t stub_last_channel;
esp_now_recv_cb_t stub_recv_cb;
esp_err_t esp_wifi_set_channel(uint8_t primary, wifi_second_chan_t second) { (void)second; stub_last_channel = primary; return 0; }
esp_err_t esp_wifi_set_mac(wifi_interface_t ifx, const uint8_t mac[6]) { (void)ifx; memcpy(stub_last_mac, mac, 6); return 0; }
esp_err_t esp_now_init(void) { return 0; }
esp_err_t esp_now_register_recv_cb(esp_now_recv_cb_t cb) { stub_recv_cb = cb; return 0; }
struct stub_queue { size_t item; int full; unsigned char buf[64]; };
QueueHandle_t xQueueCreate(unsigned len, size_t item_size) { (void)len; QueueHandle_t q = calloc(1, sizeof *q); q->item = item_size; return q; }
int xQueueOverwrite(QueueHandle_t q, const void *item) { memcpy(q->buf, item, q->item); q->full = 1; return 1; }
int xQueueReceive(QueueHandle_t q, void *out, uint32_t ticks) { (void)ticks; if (!q->full) return 0; memcpy(out, q->buf, q->item); q->full = 0; return 1; }
