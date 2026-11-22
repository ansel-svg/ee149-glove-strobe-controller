#ifndef STUB_ESP_WIFI_H
#define STUB_ESP_WIFI_H
#include <stdint.h>
typedef int esp_err_t;
#define ESP_OK 0
typedef enum { WIFI_IF_STA = 0 } wifi_interface_t;
typedef enum { WIFI_SECOND_CHAN_NONE = 0 } wifi_second_chan_t;
esp_err_t esp_wifi_set_channel(uint8_t primary, wifi_second_chan_t second);
esp_err_t esp_wifi_set_mac(wifi_interface_t ifx, const uint8_t mac[6]);
extern uint8_t stub_last_mac[6];
extern uint8_t stub_last_channel;
#endif
