#include <string.h>
#include "nvs.h"
int stub_nvs_has_ver = 0;
uint8_t stub_nvs_ver = 0;
int stub_nvs_has_data = 0;
uint8_t stub_nvs_data[256];
size_t stub_nvs_data_len = 0;
int stub_nvs_fail_set = 0;
esp_err_t nvs_open(const char *ns, int mode, nvs_handle_t *handle) { (void)ns; (void)mode; *handle = 1; return ESP_OK; }
esp_err_t nvs_get_u8(nvs_handle_t handle, const char *key, uint8_t *out) { (void)handle; (void)key; if (!stub_nvs_has_ver) return ESP_ERR_NVS_NOT_FOUND; *out = stub_nvs_ver; return ESP_OK; }
esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key, void *out, size_t *length) { (void)handle; (void)key; if (!stub_nvs_has_data) return ESP_ERR_NVS_NOT_FOUND; if (stub_nvs_data_len > *length) return ESP_ERR_NVS_INVALID_LENGTH; memcpy(out, stub_nvs_data, stub_nvs_data_len); *length = stub_nvs_data_len; return ESP_OK; }
esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key, const void *value, size_t length) { (void)handle; (void)key; if (stub_nvs_fail_set) return 1; memcpy(stub_nvs_data, value, length); stub_nvs_data_len = length; stub_nvs_has_data = 1; return ESP_OK; }
esp_err_t nvs_set_u8(nvs_handle_t handle, const char *key, uint8_t value) { (void)handle; (void)key; if (stub_nvs_fail_set) return 1; stub_nvs_ver = value; stub_nvs_has_ver = 1; return ESP_OK; }
esp_err_t nvs_commit(nvs_handle_t handle) { (void)handle; return ESP_OK; }
void nvs_close(nvs_handle_t handle) { (void)handle; }
