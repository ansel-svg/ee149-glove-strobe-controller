#include "driver/gpio.h"
int stub_boot_level = 1;
esp_err_t gpio_set_direction(gpio_num_t gpio, gpio_mode_t mode) { (void)gpio; (void)mode; return 0; }
int gpio_get_level(gpio_num_t gpio) { (void)gpio; return stub_boot_level; }
