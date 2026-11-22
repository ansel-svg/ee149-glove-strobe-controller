#ifndef STUB_GPIO_H
#define STUB_GPIO_H
#include "esp_wifi.h"
typedef int gpio_num_t;
typedef int gpio_mode_t;
#define GPIO_MODE_INPUT 1
esp_err_t gpio_set_direction(gpio_num_t gpio, gpio_mode_t mode);
int gpio_get_level(gpio_num_t gpio);
extern int stub_boot_level;
#endif
