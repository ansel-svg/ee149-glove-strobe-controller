#ifndef STUB_NVS_H
#define STUB_NVS_H
#include <stddef.h>
#include <stdint.h>
#include "esp_wifi.h"
typedef uint32_t nvs_handle_t;
#define NVS_READONLY 1
#define NVS_READWRITE 2
#define ESP_ERR_NVS_NOT_FOUND 0x1102
#define ESP_ERR_NVS_INVALID_LENGTH 0x1104
esp_err_t nvs_open(const char *ns, int mode, nvs_handle_t *handle);
esp_err_t nvs_get_u8(nvs_handle_t handle, const char *key, uint8_t *out);
esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key, void *out, size_t *length);
esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key, const void *value, size_t length);
esp_err_t nvs_set_u8(nvs_handle_t handle, const char *key, uint8_t value);
esp_err_t nvs_commit(nvs_handle_t handle);
void nvs_close(nvs_handle_t handle);
extern int stub_nvs_has_ver;
extern uint8_t stub_nvs_ver;
extern int stub_nvs_has_data;
extern uint8_t stub_nvs_data[256];
extern size_t stub_nvs_data_len;
extern int stub_nvs_fail_set;
#endif
