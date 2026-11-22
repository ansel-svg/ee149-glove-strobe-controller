#ifndef STUB_ESP_NOW_H
#define STUB_ESP_NOW_H
#include <stdint.h>
#include "esp_wifi.h"
typedef struct { const uint8_t *src_addr; } esp_now_recv_info_t;
typedef void (*esp_now_recv_cb_t)(const esp_now_recv_info_t *info, const uint8_t *data, int len);
esp_err_t esp_now_init(void);
esp_err_t esp_now_register_recv_cb(esp_now_recv_cb_t cb);
extern esp_now_recv_cb_t stub_recv_cb;
#endif
